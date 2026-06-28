/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak and contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */

/*
 * Host unit test for DeskHop's screen-switching logic.
 *
 * Compiles the REAL src/mouse.c against tests/stubs/main.h (host stubs, no Pico
 * SDK) and drives do_screen_switch() / is_screen_switch_needed() to verify the
 * custom 3+1 vertical layout (DESKHOP_LAYOUT_VERTICAL_3PLUS1):
 *
 *   OUTPUT_B = bottom PC, 3 monitors  (screen_index 1 = MIDDLE, 2 = LEFT, 3 = RIGHT)
 *   OUTPUT_A = top PC, single monitor mounted above the middle one
 *   Vertical PC crossing happens ONLY at the middle monitor <-> top PC.
 *
 * Build & run from the repo root:
 *   gcc -std=c11 -I tests/stubs -I src/include tests/test_switch.c -lm -o test_switch && ./test_switch
 *
 * (A pure-Python equivalent that needs no C toolchain lives in
 *  tests/switch_oracle.py.)
 */

/* Select the vertical-layout branch of do_screen_switch() before mouse.c is
   pulled in. The stock-branch counterpart is tests/test_switch_stock.c, which
   leaves this undefined. */
#define DESKHOP_LAYOUT_VERTICAL_3PLUS1

#include "../src/mouse.c"
#include <stdio.h>

device_t global_state;

static int failures = 0;

static void check(const char *label, int cond) {
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", label);
    if (!cond)
        failures++;
}

/* Reset to the 3+1 Windows default layout (mirrors src/defaults.c). */
static void reset_state(void) {
    memset(&global_state, 0, sizeof(global_state));
    global_state.config.jump_threshold = 0;

    global_state.config.output[OUTPUT_A].number       = OUTPUT_A;
    global_state.config.output[OUTPUT_A].os            = WINDOWS;
    global_state.config.output[OUTPUT_A].screen_count  = 1;
    global_state.config.output[OUTPUT_A].screen_index  = 1;
    global_state.config.output[OUTPUT_A].pos           = RIGHT;

    global_state.config.output[OUTPUT_B].number        = OUTPUT_B;
    global_state.config.output[OUTPUT_B].os            = WINDOWS;
    global_state.config.output[OUTPUT_B].screen_count  = 3;
    global_state.config.output[OUTPUT_B].screen_index  = 1;
    global_state.config.output[OUTPUT_B].pos           = LEFT;

    global_state.pointer_x = MAX_SCREEN_COORD / 2;
    global_state.pointer_y = MAX_SCREEN_COORD / 2;
}

/* Put us on the bottom PC at a given monitor (screen_index). relative_mouse is
   set the way we'd actually have arrived there on Windows (true for idx > 1). */
static void on_bottom(uint32_t screen_index) {
    reset_state();
    global_state.active_output = OUTPUT_B;
    global_state.config.output[OUTPUT_B].screen_index = screen_index;
    global_state.relative_mouse = (screen_index > 1);
}

/* Put us on the top PC (single monitor). We always leave the bottom PC from the
   middle monitor, so its screen_index is left at 1. */
static void on_top(void) {
    reset_state();
    global_state.active_output = OUTPUT_A;
    global_state.relative_mouse = false;
}

#define IDX(out) (global_state.config.output[(out)].screen_index)

int main(void) {
    device_t *s = &global_state;

    printf("Edge detection (is_screen_switch_needed):\n");
    reset_state();
    output_t *eo = &s->config.output[OUTPUT_B];  /* middle monitor (screen_index 1); jump_threshold 0 */
    check("left edge -> LEFT",     is_screen_switch_needed(eo, 0, -1, 16000, 0) == LEFT);
    check("right edge -> RIGHT",   is_screen_switch_needed(eo, MAX_SCREEN_COORD, 1, 16000, 0) == RIGHT);
    check("top edge -> TOP",       is_screen_switch_needed(eo, 16000, 0, 0, -1) == TOP);
    check("bottom edge -> BOTTOM", is_screen_switch_needed(eo, 16000, 0, MAX_SCREEN_COORD, 1) == BOTTOM);
    check("inside -> NONE",        is_screen_switch_needed(eo, 16000, 0, 16000, 0) == NONE);

    printf("\nBottom PC, MIDDLE monitor (screen_index 1):\n");
    on_bottom(1); do_screen_switch(s, LEFT);
    check("LEFT  -> left monitor (idx 2), stay on B", IDX(OUTPUT_B) == 2 && s->active_output == OUTPUT_B);
    check("LEFT  -> sets relative_mouse true (non-primary monitor)", s->relative_mouse == true);
    check("LEFT  -> enters left monitor at right edge (pointer_x MAX)", s->pointer_x == MAX_SCREEN_COORD);
    on_bottom(1); do_screen_switch(s, RIGHT);
    check("RIGHT -> right monitor (idx 3), stay on B", IDX(OUTPUT_B) == 3 && s->active_output == OUTPUT_B);
    check("RIGHT -> sets relative_mouse true (non-primary monitor)", s->relative_mouse == true);
    check("RIGHT -> enters right monitor at left edge (pointer_x MIN)", s->pointer_x == MIN_SCREEN_COORD);
    on_bottom(1); do_screen_switch(s, TOP);
    check("TOP   -> top PC (OUTPUT_A)", s->active_output == OUTPUT_A);
    check("TOP   -> relative_mouse stays false at crossing", s->relative_mouse == false);
    check("TOP   -> cursor enters at bottom edge (pointer_y MAX)", s->pointer_y == MAX_SCREEN_COORD);
    on_bottom(1); do_screen_switch(s, BOTTOM);
    check("BOTTOM-> no change (cursor stops)", s->active_output == OUTPUT_B && IDX(OUTPUT_B) == 1);

    printf("\nBottom PC, LEFT monitor (screen_index 2):\n");
    on_bottom(2); do_screen_switch(s, RIGHT);
    check("RIGHT -> middle (idx 1), relative_mouse false", IDX(OUTPUT_B) == 1 && s->relative_mouse == false);
    on_bottom(2); do_screen_switch(s, LEFT);
    check("LEFT  -> no change", IDX(OUTPUT_B) == 2 && s->active_output == OUTPUT_B);
    on_bottom(2); do_screen_switch(s, TOP);
    check("TOP   -> no PC switch (only middle crosses up)", s->active_output == OUTPUT_B);

    printf("\nBottom PC, RIGHT monitor (screen_index 3):\n");
    on_bottom(3); do_screen_switch(s, LEFT);
    check("LEFT  -> middle (idx 1), relative_mouse false", IDX(OUTPUT_B) == 1 && s->relative_mouse == false);
    on_bottom(3); do_screen_switch(s, RIGHT);
    check("RIGHT -> no change", IDX(OUTPUT_B) == 3 && s->active_output == OUTPUT_B);

    printf("\nTop PC (OUTPUT_A, single monitor):\n");
    on_top(); do_screen_switch(s, BOTTOM);
    check("BOTTOM-> bottom PC (OUTPUT_B)", s->active_output == OUTPUT_B);
    check("BOTTOM-> lands on middle (idx 1)", IDX(OUTPUT_B) == 1);
    check("BOTTOM-> relative_mouse stays false at crossing", s->relative_mouse == false);
    check("BOTTOM-> cursor enters at top edge (pointer_y MIN)", s->pointer_y == MIN_SCREEN_COORD);
    on_top(); do_screen_switch(s, TOP);
    check("TOP   -> no change", s->active_output == OUTPUT_A);
    on_top(); do_screen_switch(s, LEFT);
    check("LEFT  -> no change", s->active_output == OUTPUT_A);
    on_top(); do_screen_switch(s, RIGHT);
    check("RIGHT -> no change", s->active_output == OUTPUT_A);

    printf("\nMouse-button held suppresses PC switches:\n");
    on_bottom(1); s->mouse_buttons = 1; do_screen_switch(s, TOP);
    check("B/middle TOP w/ button held -> no PC switch", s->active_output == OUTPUT_B);
    on_top(); s->mouse_buttons = 1; do_screen_switch(s, BOTTOM);
    check("A BOTTOM w/ button held -> no PC switch", s->active_output == OUTPUT_A);
    on_bottom(1); s->mouse_buttons = 1; do_screen_switch(s, LEFT);
    check("B/middle LEFT w/ button held -> still moves to left monitor", IDX(OUTPUT_B) == 2);

    printf("\nGlobal locks:\n");
    on_bottom(1); s->switch_lock = true; do_screen_switch(s, TOP);
    check("switch_lock blocks everything", s->active_output == OUTPUT_B && IDX(OUTPUT_B) == 1);
    on_bottom(1); s->gaming_mode = true; do_screen_switch(s, LEFT);
    check("gaming_mode blocks everything", s->active_output == OUTPUT_B && IDX(OUTPUT_B) == 1);

    printf("\nOutput-toggle (hotkey/UART) from a side monitor must not strand relative mode:\n");
    on_bottom(3);                    /* right monitor: relative_mouse true, B at idx 3 */
    set_active_output(s, OUTPUT_A);  /* simulate the hotkey/UART output toggle (bypasses do_screen_switch) */
    check("toggle to A clears relative_mouse", s->relative_mouse == false);
    check("toggle to A resets bottom PC to middle (idx 1)", IDX(OUTPUT_B) == 1);
    do_screen_switch(s, BOTTOM);     /* return down to the bottom PC */
    check("return lands on bottom PC middle (idx 1)", s->active_output == OUTPUT_B && IDX(OUTPUT_B) == 1);
    check("return keeps relative_mouse false", s->relative_mouse == false);

    printf("\nRound trip middle <-> top keeps relative_mouse false throughout:\n");
    on_bottom(1);
    do_screen_switch(s, TOP);      /* B/middle -> A */
    bool rel_after_up = s->relative_mouse;
    do_screen_switch(s, BOTTOM);   /* A -> B/middle */
    bool rel_after_down = s->relative_mouse;
    check("relative_mouse false across both crossings", !rel_after_up && !rel_after_down);
    check("returned to bottom PC middle monitor", s->active_output == OUTPUT_B && IDX(OUTPUT_B) == 1);

    printf("\n================================================\n");
    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    printf("ALL CHECKS PASSED\n");
    return 0;
}
