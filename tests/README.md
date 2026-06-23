# Tests - screen-switching logic

These tests cover the custom vertical screen-switching for the 3+1 monitor
layout (`DESKHOP_LAYOUT_VERTICAL_3PLUS1`):

- **OUTPUT_B** = bottom PC, 3 monitors (`screen_index` 1 = MIDDLE, 2 = LEFT, 3 = RIGHT)
- **OUTPUT_A** = top PC, single monitor mounted above the middle one
- The mouse crosses between the two PCs **only** at the middle monitor's top edge
  (up to OUTPUT_A) and OUTPUT_A's bottom edge (back down to the middle).

All of the below are run by CI on every push (`.github/workflows/host-tests.yml`).

## C unit tests (authoritative)

Both compile the **real** `src/mouse.c` against host stubs (`stubs/main.h`, no
Pico SDK / TinyUSB), so they exercise the exact switch code that ships.

- **`test_switch.c`** - defines `DESKHOP_LAYOUT_VERTICAL_3PLUS1`, exercising the
  custom vertical branch of `do_screen_switch()`. Asserts the full 3+1 transition
  table, the relative-mouse split (absolute middle vs relative left/right), the
  cursor entry edge after a crossing, the mouse-button-held and
  switch_lock/gaming_mode guards, and that a hotkey/UART output toggle from a
  side monitor does not strand relative mode.
- **`test_switch_stock.c`** - leaves the macro undefined, exercising the `#else`
  stock left/right branch. Guards the regression where the now Y-aware
  `is_screen_switch_needed()` could feed TOP/BOTTOM into the stock branch and
  spuriously switch PCs.

Build & run from the repo root (any host C compiler):

```sh
gcc -std=c11 -Wall -I tests/stubs -I src/include tests/test_switch.c       -lm -o test_switch       && ./test_switch
gcc -std=c11 -Wall -I tests/stubs -I src/include tests/test_switch_stock.c -lm -o test_switch_stock && ./test_switch_stock
```

Each test file selects the layout itself (`test_switch.c` `#define`s the macro;
`test_switch_stock.c` does not), so no `-D` flag is needed. `stubs/main.h` is a
trimmed stand-in for the real `main.h`: it defines only the types, macros and
(no-op) functions `mouse.c` references, plus a `set_active_output()` that mirrors
the firmware's vertical-layout reset. If `mouse.c` starts using a new symbol, add
it there.

## `switch_oracle.py` - Python behavioural spec (no toolchain needed)

A Python mirror of the same decision logic, asserting the same transition table.
Useful on machines without a C/ARM toolchain and as readable documentation. It
models the switch decisions and the `set_active_output()` reset only; side
effects (coordinate scaling, LED/board-sync, key-release) are out of scope. Keep
it in sync with `mouse.c`.

```sh
python3 tests/switch_oracle.py
```

The C tests are the source of truth; the oracle is a convenience/cross-check.

## Scope / coverage notes

- The tests check `screen_index`, `active_output`, `relative_mouse`, and the
  `pointer_x`/`pointer_y` the switch logic sets. They do **not** route through
  `process_mouse_report` -> `update_mouse_position` -> `create_mouse_report`, so
  the end-to-end ABSOLUTE/RELATIVE report-mode emission is not exercised here.
- Real switching behaviour ultimately needs the DeskHop hardware + the actual
  monitors; these host tests validate the state machine, not the USB/HID layer.
