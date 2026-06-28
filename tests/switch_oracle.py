#!/usr/bin/env python3
#
# This file is part of DeskHop (https://github.com/hrvach/deskhop).
# Copyright (c) 2025 Hrvoje Cavrak and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# See the file LICENSE for the full license text.
#
"""
Executable behavioural spec ("oracle") for DeskHop's vertical screen-switching.

This is a Python mirror of the switch-DECISION state machine in src/mouse.c
(do_screen_switch + switch_virtual_desktop + switch_to_another_pc +
is_screen_switch_needed) plus the vertical-layout reset in handlers.c
set_active_output(), pinned to the custom 3+1 vertical layout enabled by
DESKHOP_LAYOUT_VERTICAL_3PLUS1. Side effects (scale_y_coordinate, LED restore,
board-sync OUTPUT_SELECT_MSG, key-release) are out of scope and not asserted.

Why this exists: the firmware is RP2040 C and needs an ARM/host C toolchain to
compile. This oracle lets the switching *logic* be verified on any machine with
just Python (no toolchain) and documents the intended transition table. The
C-level test (tests/test_switch.c) exercises the real compiled functions and is
the authority; keep this file in sync with mouse.c if the logic changes.

Run:  python3 tests/switch_oracle.py
"""

# enum screen_pos_e (src/include/structs.h)
NONE, LEFT, RIGHT, MIDDLE, TOP, BOTTOM = 0, 1, 2, 3, 4, 5
# constants.h
OUTPUT_A, OUTPUT_B = 0, 1
# screen.h
MIN_SCREEN_COORD, MAX_SCREEN_COORD = 0, 32767
# os_type_e
LINUX, MACOS, WINDOWS, ANDROID, OTHER = 1, 2, 3, 4, 255

DIR_NAME = {NONE: "NONE", LEFT: "LEFT", RIGHT: "RIGHT", MIDDLE: "MIDDLE",
            TOP: "TOP", BOTTOM: "BOTTOM"}


class Output:
    def __init__(self, number, os, screen_count=1, screen_index=1, pos=NONE):
        self.number = number
        self.os = os
        self.screen_count = screen_count
        self.screen_index = screen_index
        self.pos = pos


class State:
    """Trimmed device_t holding only the fields the switch logic touches."""
    def __init__(self):
        # Default config mirrors src/defaults.c for the 3+1 Windows layout:
        #   OUTPUT_A = top PC, single monitor
        #   OUTPUT_B = bottom PC, 3 monitors (middle = screen_index 1)
        self.output = [
            Output(OUTPUT_A, WINDOWS, screen_count=1, screen_index=1, pos=RIGHT),
            Output(OUTPUT_B, WINDOWS, screen_count=3, screen_index=1, pos=LEFT),
        ]
        self.active_output = OUTPUT_B
        self.pointer_x = MAX_SCREEN_COORD // 2
        self.pointer_y = MAX_SCREEN_COORD // 2
        self.mouse_buttons = 0
        self.switch_lock = False
        self.gaming_mode = False
        self.relative_mouse = False

    @property
    def cur(self):
        return self.output[self.active_output]


# --- faithful mirrors of the C helpers -------------------------------------

def set_active_output(state, new_output):
    # Mirrors handlers.c set_active_output(): in the vertical layout it also
    # clears relative_mouse and resets both outputs to the main screen (idx 1),
    # so a hotkey/UART toggle from a side monitor can't strand relative mode.
    state.active_output = new_output
    state.relative_mouse = False
    state.output[OUTPUT_A].screen_index = 1
    state.output[OUTPUT_B].screen_index = 1


def switch_virtual_desktop(state, output, new_index, direction):
    # src/mouse.c switch_virtual_desktop() -- WINDOWS branch only here
    if output.os == WINDOWS:
        state.relative_mouse = (new_index > 1)
    state.pointer_x = MIN_SCREEN_COORD if direction == RIGHT else MAX_SCREEN_COORD
    output.screen_index = new_index


def switch_to_another_pc(state, output, output_to, direction):
    # src/mouse.c switch_to_another_pc() -- park logic omitted (irrelevant here)
    set_active_output(state, output_to)
    if direction in (LEFT, RIGHT):
        state.pointer_x = MAX_SCREEN_COORD if direction == LEFT else MIN_SCREEN_COORD
        # state.pointer_y = scale_y_coordinate(...)  # not asserted here
    else:
        state.pointer_y = MAX_SCREEN_COORD if direction == TOP else MIN_SCREEN_COORD


def do_screen_switch(state, direction):
    # src/mouse.c do_screen_switch() -- DESKHOP_LAYOUT_VERTICAL_3PLUS1 branch
    output = state.cur

    if state.switch_lock or state.gaming_mode:
        return

    if state.active_output == OUTPUT_B:            # bottom PC, multiple monitors
        if output.screen_index == 1:               # MIDDLE monitor (main)
            if direction == LEFT:
                switch_virtual_desktop(state, output, 2, direction)
            elif direction == RIGHT:
                switch_virtual_desktop(state, output, 3, direction)
            elif direction == TOP and not state.mouse_buttons:
                switch_to_another_pc(state, output, OUTPUT_A, direction)
        elif output.screen_index == 2:             # LEFT monitor
            if direction == RIGHT:
                switch_virtual_desktop(state, output, 1, direction)
        elif output.screen_index == 3:             # RIGHT monitor
            if direction == LEFT:
                switch_virtual_desktop(state, output, 1, direction)
    else:                                          # OUTPUT_A: top PC, single monitor
        if direction == BOTTOM and not state.mouse_buttons:
            switch_to_another_pc(state, output, OUTPUT_B, direction)


CONFIG_JUMP_THRESHOLD = 0  # mirrors global_state.config.jump_threshold (0 in these tests)


def get_jump_threshold(output, direction):
    # src/mouse.c get_jump_threshold() -- DESKHOP_LAYOUT_VERTICAL_3PLUS1 branch:
    # only TOP/BOTTOM cross PCs (and need the jump "force"); left/right is always
    # local between the bottom PC's monitors, so frictionless.
    if direction in (TOP, BOTTOM):
        return CONFIG_JUMP_THRESHOLD
    return 0


def is_screen_switch_needed(output, position_x, offset_x, position_y, offset_y):
    # Horizontal (X) axis
    if offset_x != 0:
        direction = LEFT if offset_x < 0 else RIGHT
        threshold = get_jump_threshold(output, direction)
        if position_x + offset_x < MIN_SCREEN_COORD - threshold:
            return LEFT
        if position_x + offset_x > MAX_SCREEN_COORD + threshold:
            return RIGHT
    # Vertical (Y) axis (DESKHOP_LAYOUT_VERTICAL_3PLUS1)
    if offset_y != 0:
        direction = TOP if offset_y < 0 else BOTTOM
        threshold = get_jump_threshold(output, direction)
        if position_y + offset_y < MIN_SCREEN_COORD - threshold:
            return TOP
        if position_y + offset_y > MAX_SCREEN_COORD + threshold:
            return BOTTOM
    return NONE


# --- test harness ----------------------------------------------------------

_failures = []


def check(label, cond):
    mark = "PASS" if cond else "FAIL"
    print(f"  [{mark}] {label}")
    if not cond:
        _failures.append(label)


def on_b(screen_index, **kw):
    s = State()
    s.active_output = OUTPUT_B
    s.output[OUTPUT_B].screen_index = screen_index
    s.relative_mouse = (screen_index > 1)  # consistent with how we'd have arrived
    for k, v in kw.items():
        setattr(s, k, v)
    return s


def on_a(**kw):
    s = State()
    s.active_output = OUTPUT_A
    s.output[OUTPUT_A].screen_index = 1
    s.output[OUTPUT_B].screen_index = 1  # we always leave B from the middle
    s.relative_mouse = False
    for k, v in kw.items():
        setattr(s, k, v)
    return s


print("Edge detection (is_screen_switch_needed):")
eo = Output(OUTPUT_B, WINDOWS, screen_count=3, screen_index=1, pos=LEFT)  # middle monitor
check("left edge -> LEFT", is_screen_switch_needed(eo, 0, -1, 16000, 0) == LEFT)
check("right edge -> RIGHT", is_screen_switch_needed(eo, MAX_SCREEN_COORD, 1, 16000, 0) == RIGHT)
check("top edge -> TOP", is_screen_switch_needed(eo, 16000, 0, 0, -1) == TOP)
check("bottom edge -> BOTTOM", is_screen_switch_needed(eo, 16000, 0, MAX_SCREEN_COORD, 1) == BOTTOM)
check("inside -> NONE", is_screen_switch_needed(eo, 16000, 0, 16000, 0) == NONE)

print("\nBottom PC, MIDDLE monitor (screen_index 1):")
s = on_b(1); do_screen_switch(s, LEFT)
check("LEFT  -> left monitor (idx 2), stay on B", s.cur.screen_index == 2 and s.active_output == OUTPUT_B)
check("LEFT  -> sets relative_mouse True (non-primary)", s.relative_mouse is True)
check("LEFT  -> enters left monitor at right edge (pointer_x MAX)", s.pointer_x == MAX_SCREEN_COORD)
s = on_b(1); do_screen_switch(s, RIGHT)
check("RIGHT -> right monitor (idx 3), stay on B", s.cur.screen_index == 3 and s.active_output == OUTPUT_B)
check("RIGHT -> sets relative_mouse True (non-primary)", s.relative_mouse is True)
check("RIGHT -> enters right monitor at left edge (pointer_x MIN)", s.pointer_x == MIN_SCREEN_COORD)
s = on_b(1); do_screen_switch(s, TOP)
check("TOP   -> top PC (OUTPUT_A)", s.active_output == OUTPUT_A)
check("TOP   -> relative_mouse stays False at crossing", s.relative_mouse is False)
check("TOP   -> cursor enters at bottom edge (pointer_y MAX)", s.pointer_y == MAX_SCREEN_COORD)
s = on_b(1); do_screen_switch(s, BOTTOM)
check("BOTTOM-> no change (cursor stops)", s.active_output == OUTPUT_B and s.cur.screen_index == 1)

print("\nBottom PC, LEFT monitor (screen_index 2):")
s = on_b(2); do_screen_switch(s, RIGHT)
check("RIGHT -> middle (idx 1), relative_mouse False", s.cur.screen_index == 1 and s.relative_mouse is False)
s = on_b(2); do_screen_switch(s, LEFT)
check("LEFT  -> no change (no monitor further left)", s.cur.screen_index == 2 and s.active_output == OUTPUT_B)
s = on_b(2); do_screen_switch(s, TOP)
check("TOP   -> no PC switch (only middle crosses up)", s.active_output == OUTPUT_B)

print("\nBottom PC, RIGHT monitor (screen_index 3):")
s = on_b(3); do_screen_switch(s, LEFT)
check("LEFT  -> middle (idx 1), relative_mouse False", s.cur.screen_index == 1 and s.relative_mouse is False)
s = on_b(3); do_screen_switch(s, RIGHT)
check("RIGHT -> no change (no monitor further right)", s.cur.screen_index == 3 and s.active_output == OUTPUT_B)

print("\nTop PC (OUTPUT_A, single monitor):")
s = on_a(); do_screen_switch(s, BOTTOM)
check("BOTTOM-> bottom PC (OUTPUT_B)", s.active_output == OUTPUT_B)
check("BOTTOM-> lands on middle (idx 1)", s.output[OUTPUT_B].screen_index == 1)
check("BOTTOM-> relative_mouse stays False at crossing", s.relative_mouse is False)
check("BOTTOM-> cursor enters at top edge (pointer_y MIN)", s.pointer_y == MIN_SCREEN_COORD)
for d in (TOP, LEFT, RIGHT):
    s = on_a(); do_screen_switch(s, d)
    check(f"{DIR_NAME[d]:6}-> no change", s.active_output == OUTPUT_A)

print("\nMouse-button held suppresses PC switches:")
s = on_b(1, mouse_buttons=1); do_screen_switch(s, TOP)
check("B/middle TOP with button held -> no switch", s.active_output == OUTPUT_B)
s = on_a(mouse_buttons=1); do_screen_switch(s, BOTTOM)
check("A BOTTOM with button held -> no switch", s.active_output == OUTPUT_A)
# but lateral monitor moves are still allowed while dragging
s = on_b(1, mouse_buttons=1); do_screen_switch(s, LEFT)
check("B/middle LEFT with button held -> still moves to left monitor", s.cur.screen_index == 2)

print("\nGlobal locks:")
s = on_b(1, switch_lock=True); do_screen_switch(s, TOP)
check("switch_lock blocks everything", s.active_output == OUTPUT_B and s.cur.screen_index == 1)
s = on_b(1, gaming_mode=True); do_screen_switch(s, LEFT)
check("gaming_mode blocks everything", s.active_output == OUTPUT_B and s.cur.screen_index == 1)

print("\nOutput-toggle (hotkey/UART) from a side monitor must not strand relative mode:")
s = on_b(3)                     # right monitor: relative_mouse True, B idx 3
set_active_output(s, OUTPUT_A)  # simulate the hotkey/UART toggle (bypasses do_screen_switch)
check("toggle to A clears relative_mouse", s.relative_mouse is False)
check("toggle to A resets bottom PC to middle (idx 1)", s.output[OUTPUT_B].screen_index == 1)
do_screen_switch(s, BOTTOM)     # return down to the bottom PC
check("return lands on bottom PC middle (idx 1)", s.active_output == OUTPUT_B and s.output[OUTPUT_B].screen_index == 1)
check("return keeps relative_mouse False", s.relative_mouse is False)

print("\nRound trip middle <-> top keeps relative_mouse False the whole way:")
s = on_b(1)
seen_relative = []
do_screen_switch(s, TOP)      # B/middle -> A
seen_relative.append(s.relative_mouse)
do_screen_switch(s, BOTTOM)   # A -> B/middle
seen_relative.append(s.relative_mouse)
check("relative_mouse False across both crossings", not any(seen_relative))
check("returned to bottom PC middle monitor", s.active_output == OUTPUT_B and s.cur.screen_index == 1)

print("\n" + ("=" * 48))
if _failures:
    print(f"FAILED ({len(_failures)}): " + "; ".join(_failures))
    raise SystemExit(1)
print("ALL CHECKS PASSED")
