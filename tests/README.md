# Tests — screen-switching logic

These tests cover the custom vertical screen-switching for the 3+1 monitor
layout (`DESKHOP_LAYOUT_VERTICAL_3PLUS1`):

- **OUTPUT_B** = bottom PC, 3 monitors (`screen_index` 1 = MIDDLE, 2 = LEFT, 3 = RIGHT)
- **OUTPUT_A** = top PC, single monitor mounted above the middle one
- The mouse crosses between the two PCs **only** at the middle monitor's top edge
  (up to OUTPUT_A) and OUTPUT_A's bottom edge (back down to the middle).

## `test_switch.c` — C unit test (authoritative)

Compiles the **real** `src/mouse.c` against host stubs (`stubs/main.h`, no Pico
SDK / TinyUSB) and drives `do_screen_switch()` and `is_screen_switch_needed()`.
This tests the exact code that ships in the firmware.

Build & run from the repo root (needs any host C compiler):

```sh
gcc -std=c11 -I tests/stubs -I src/include tests/test_switch.c -lm -o test_switch
./test_switch
```

`stubs/main.h` is a trimmed stand-in for the real `main.h`: it defines only the
types, macros and (no-op) functions that `mouse.c` references. If `mouse.c`
later starts using a new symbol, add it there.

## `switch_oracle.py` — Python behavioural spec (no toolchain needed)

A faithful Python mirror of the same state machine, asserting the full
transition table. Useful on machines without a C/ARM toolchain and as readable
documentation of the intended behaviour. Keep it in sync with `mouse.c`.

```sh
python3 tests/switch_oracle.py
```

The C test is the source of truth; the oracle is a convenience/cross-check.

## Note on verification status

At the time these were written, the authoring machine had no C toolchain, so
`switch_oracle.py` was run (all checks pass) but `test_switch.c` was **not**
compiled locally. Build it with the command above (or wire it into CI) to
confirm. The firmware build itself (`cmake`/Docker, see the top-level README)
also compile-checks the underlying changes in `mouse.c`.
