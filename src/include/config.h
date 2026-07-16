/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */
#pragma once

#include <stdint.h>
#include "structs.h"
#include "misc.h"
#include "screen.h"

/* Bumped 9 -> 10: the config gained disable_switching (persistent switch-lock
   seed) and jump_threshold_down (separate force for the downward PC crossing),
   carved out of the old _reserved field. Any bump forces a one-time reseed from
   default_config on units still holding an older config, so the new fields
   start from known values instead of stale flash bytes. (8 -> 9 was the
   vertical 3+1 default-shape change: OUTPUT_B screen_count = 3, both Windows.) */
#define CURRENT_CONFIG_VERSION 10

/*==============================================================================
 *  Configuration Data
 *  Structures and variables related to device configuration.
 *==============================================================================*/

extern const config_t default_config;

/*==============================================================================
 *  Configuration API
 *  Functions and data structures for accessing and modifying configuration.
 *==============================================================================*/

extern const field_map_t api_field_map[];
const field_map_t* get_field_map_entry(uint32_t);
const field_map_t* get_field_map_index(uint32_t);
size_t             get_field_map_length(void);

/*==============================================================================
 *  Configuration Management and Packet Processing
 *  Functions for loading, saving, wiping, and resetting device configuration.
 *==============================================================================*/

void load_config(device_t *);
void queue_cfg_packet(uart_packet_t *, device_t *);
void reset_config_timer(device_t *);
void save_config(device_t *);
bool validate_packet(uart_packet_t *);
void wipe_config(void);
