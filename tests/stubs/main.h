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
 * Minimal host stubs so src/mouse.c can be compiled OFF-TARGET (no Pico SDK,
 * no TinyUSB) for unit-testing the screen-switching logic on a normal PC.
 *
 * src/mouse.c includes only "main.h" and <math.h>, so this single fake header
 * has to provide every type, macro and external function it references. The
 * structs are trimmed to just the fields mouse.c touches; the stubbed funcs are
 * no-ops (or, for set_active_output, the part of the behaviour that matters).
 *
 * Compile from the repo root, e.g.:
 *   gcc -std=c11 -I tests/stubs -I src/include tests/test_switch.c -lm -o test_switch
 *
 * See tests/README.md.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ---- enums (mirror src/include/structs.h) ---- */
enum screen_pos_e { NONE = 0, LEFT = 1, RIGHT = 2, MIDDLE = 3, TOP = 4, BOTTOM = 5 };
enum os_type_e { LINUX = 1, MACOS = 2, WINDOWS = 3, ANDROID = 4, OTHER = 255 };
enum { ABSOLUTE = 0, RELATIVE = 1 };               /* mouse_report_t.mode */
enum { HID_PROTOCOL_BOOT = 0, HID_PROTOCOL_REPORT = 1 };

/* ---- screen coords (mirror src/include/screen.h) ---- */
#define MIN_SCREEN_COORD 0
#define MAX_SCREEN_COORD 32767

/* ---- layout / board macros ---- */
#define OUTPUT_A 0
#define OUTPUT_B 1
#define BOARD_ROLE 0
#define ITF_NUM_HID 0
#define MOUSE_REPORT_MSG 0
#define MOUSE_REPORT_LENGTH 7
#define MOUSE_ZOOM_SCALING_FACTOR 4
#define CURRENT_BOARD_IS_ACTIVE_OUTPUT 1

/* The layout macro (DESKHOP_LAYOUT_VERTICAL_3PLUS1) is set by the INCLUDING test
   file: test_switch.c defines it (vertical layout); test_switch_stock.c leaves it
   undefined (stock left/right path). The same stubs serve both. */

/* ---- fake queue type (mirror pico/util/queue.h usage) ---- */
typedef struct { int _unused; } queue_t;

/* ---- structs (trimmed to what mouse.c references) ---- */
typedef struct { int top; int bottom; } border_size_t;

typedef struct {
    uint32_t number;
    uint32_t screen_count;
    uint32_t screen_index;
    int32_t  speed_x;
    int32_t  speed_y;
    border_size_t border;
    uint8_t  os;
    uint8_t  pos;
    uint8_t  mouse_park_pos;
} output_t;

typedef struct {
    uint16_t jump_threshold;
    uint8_t  enable_acceleration;
    output_t output[2];
} config_t;

typedef struct {
    uint8_t buttons;
    int16_t x;
    int16_t y;
    int8_t  wheel;
    int8_t  pan;
    uint8_t mode;
} mouse_report_t;

typedef struct {
    int32_t move_x;
    int32_t move_y;
    int32_t wheel;
    int32_t pan;
    int32_t buttons;
} mouse_values_t;

typedef struct { uint8_t report_id; } report_val_t;

typedef struct {
    report_val_t move_x, move_y, wheel, pan, buttons;
} mouse_t;

typedef struct {
    uint8_t protocol;
    bool    uses_report_id;
    mouse_t mouse;
} hid_interface_t;

/* boot-protocol report (mirror TinyUSB hid_mouse_report_t) */
typedef struct {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
    int8_t  pan;
} hid_mouse_report_t;

typedef struct {
    uint8_t  active_output;
    config_t config;
    int16_t  pointer_x;
    int16_t  pointer_y;
    int16_t  mouse_buttons;
    bool     mouse_zoom;
    bool     switch_lock;
    bool     gaming_mode;
    bool     relative_mouse;
    bool     tud_connected;
    uint64_t last_activity[2];
    queue_t  mouse_queue;
} device_t;

extern device_t global_state;

/* Forward declarations for functions defined in src/mouse.c that are called
   before their definition (normally provided by src/include/mouse.h). */
void queue_mouse_report(mouse_report_t *report, device_t *state);
void output_mouse_report(mouse_report_t *report, device_t *state);

/* ---- stubbed externals (no-ops are fine for the switch tests) ---- */
static inline uint64_t time_us_64(void) { return 0; }
static inline bool queue_try_add(queue_t *q, const void *d) { (void)q; (void)d; return true; }
static inline bool queue_try_peek(queue_t *q, void *d) { (void)q; (void)d; return false; }
static inline bool queue_try_remove(queue_t *q, void *d) { (void)q; (void)d; return true; }
static inline void queue_packet(uint8_t *d, int type, int len) { (void)d; (void)type; (void)len; }
static inline int32_t get_report_value(uint8_t *r, int len, report_val_t *v) { (void)r; (void)len; (void)v; return 0; }
static inline bool tud_hid_n_ready(uint8_t i) { (void)i; return false; }
static inline bool tud_mouse_report(uint8_t mode, uint8_t b, int16_t x, int16_t y, int8_t w, int8_t p) {
    (void)mode; (void)b; (void)x; (void)y; (void)w; (void)p; return true;
}
static inline bool tud_suspended(void) { return false; }
static inline void tud_remote_wakeup(void) {}

/* Mirrors the part of handlers.c set_active_output() that the switch logic
   depends on (the real one also restores LEDs and releases held keys). The
   vertical-layout reset below matches the firmware's set_active_output(). */
static inline void set_active_output(device_t *state, uint8_t new_output) {
    state->active_output = new_output;
#ifdef DESKHOP_LAYOUT_VERTICAL_3PLUS1
    state->relative_mouse                       = false;
    state->config.output[OUTPUT_A].screen_index = 1;
    state->config.output[OUTPUT_B].screen_index = 1;
#endif
}
