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
 * Host unit test for the STOCK (left/right linear) screen-switching path.
 *
 * Compiles the REAL src/mouse.c with DESKHOP_LAYOUT_VERTICAL_3PLUS1 *undefined*,
 * so the #else stock branch of do_screen_switch() is exercised. This guards the
 * regression where the now globally Y-aware is_screen_switch_needed() can return
 * TOP/BOTTOM and feed them into the stock branch, spuriously switching PCs on a
 * purely vertical mouse motion. The vertical-layout counterpart is
 * tests/test_switch.c (which defines the macro).
 *
 * Build & run from the repo root (note: NO -D for the layout macro):
 *   gcc -std=c11 -I tests/stubs -I src/include tests/test_switch_stock.c -lm -o test_switch_stock && ./test_switch_stock
 */
#include "../src/mouse.c"   /* DESKHOP_LAYOUT_VERTICAL_3PLUS1 intentionally NOT defined */
#include <stdio.h>

device_t global_state;

static int failures = 0;

static void check(const char *label, int cond) {
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", label);
    if (!cond)
        failures++;
}

/* Minimal stock two-PC linear setup, single screen each (mirrors upstream defaults). */
static void reset_stock(void) {
    memset(&global_state, 0, sizeof(global_state));
    global_state.config.jump_threshold = 0;

    global_state.config.output[OUTPUT_A].number        = OUTPUT_A;
    global_state.config.output[OUTPUT_A].os            = WINDOWS;
    global_state.config.output[OUTPUT_A].screen_count  = 1;
    global_state.config.output[OUTPUT_A].screen_index  = 1;
    global_state.config.output[OUTPUT_A].pos           = RIGHT;
    global_state.config.output[OUTPUT_A].border.bottom = MAX_SCREEN_COORD;

    global_state.config.output[OUTPUT_B].number        = OUTPUT_B;
    global_state.config.output[OUTPUT_B].os            = WINDOWS;
    global_state.config.output[OUTPUT_B].screen_count  = 1;
    global_state.config.output[OUTPUT_B].screen_index  = 1;
    global_state.config.output[OUTPUT_B].pos           = LEFT;
    global_state.config.output[OUTPUT_B].border.bottom = MAX_SCREEN_COORD;

    global_state.pointer_x = MAX_SCREEN_COORD / 2;
    global_state.pointer_y = MAX_SCREEN_COORD / 2;
}

int main(void) {
    device_t *s = &global_state;

    printf("Stock layout: vertical edges must NOT switch PCs (regression guard):\n");
    reset_stock(); s->active_output = OUTPUT_B; do_screen_switch(s, TOP);
    check("B + TOP    -> no switch", s->active_output == OUTPUT_B);
    reset_stock(); s->active_output = OUTPUT_B; do_screen_switch(s, BOTTOM);
    check("B + BOTTOM -> no switch", s->active_output == OUTPUT_B);
    reset_stock(); s->active_output = OUTPUT_A; do_screen_switch(s, TOP);
    check("A + TOP    -> no switch", s->active_output == OUTPUT_A);
    reset_stock(); s->active_output = OUTPUT_A; do_screen_switch(s, BOTTOM);
    check("A + BOTTOM -> no switch", s->active_output == OUTPUT_A);

    printf("\nStock layout: horizontal switching still works (confirms the #else branch is live):\n");
    reset_stock(); s->active_output = OUTPUT_B; do_screen_switch(s, RIGHT);
    check("B + RIGHT  -> switches to A", s->active_output == OUTPUT_A);

    printf("\n================================================\n");
    if (failures) {
        printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    printf("ALL CHECKS PASSED\n");
    return 0;
}
