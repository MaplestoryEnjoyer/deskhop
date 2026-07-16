# DeskHop-Fork: Future Features and Deferred Items

Planning notes for the fork. Nothing here is implemented yet; each item records
the motivation, what already exists, an implementation sketch, and open
questions, so a future session (or a future me) can pick it up cold.

See `CLAUDE.md` for the fork's core design (vertical 3+1 layout,
`DESKHOP_LAYOUT_VERTICAL_3PLUS1`) and the upstream-merge workflow.

---

## 1. Configurable "disable switching" toggle

**Goal:** a persistent, web-configurable option that prevents PC switching
entirely, instead of only the runtime hotkey lock that resets on reboot.

### What already exists (baseline)

`switch_lock` (toggled by `Right Ctrl + K`) already blocks switching completely,
both paths:

- Mouse-driven crossing bails in `do_screen_switch()` (`src/mouse.c`, the
  `if (state->switch_lock || state->gaming_mode) return;` guard).
- The keyboard toggle hotkey (Ctrl+Caps) bails in
  `output_toggle_hotkey_handler()` (`src/handlers.c`).

Limitations that motivate a new feature:

- Runtime-only: `switch_lock` lives in `device_t`, not the saved config, so it
  resets to off on every reboot.
- Not exposed for saving: `protocol.c` maps `switch_lock` as **read-only**
  (field id 81), so the web config can display it but not set or persist it.

If a per-boot hotkey press is acceptable, no code change is needed.

### Two flavors (decide before building)

- **Persistent default lock** (preferred): a config flag that *seeds*
  `switch_lock` at boot. Device comes up locked, but `Right Ctrl + K` can still
  toggle it off on the fly. Less surprising, keeps a hotkey escape hatch.
- **Hard lock**: a config flag checked independently in the guards; the hotkey
  cannot override it, only the web config turns it back on. Stronger guarantee
  (useful as partial mitigation for item 2), but no on-device escape hatch.

### Implementation sketch

Mirror the existing `enforce_ports` boolean config flag across these spots:

1. `src/include/user_config.h` - add `#define DISABLE_SWITCHING 0` default.
2. `src/include/structs.h` - add `uint8_t disable_switching;` to `config_t`
   (next to `enforce_ports`).
3. `src/include/config.h` - bump `CURRENT_CONFIG_VERSION` (currently 9 -> 10) so
   existing devices cleanly reseed the new field.
4. `src/defaults.c` - `.disable_switching = DISABLE_SWITCHING,`.
5. `src/protocol.c` - add a writable field-map entry (e.g. id 83, `false`,
   `UINT8`, `offsetof(device_t, config.disable_switching)`).
6. `webconfig/form.py` - add a `FormField(..., "Disable Switching", ...,
   "checkbox")` to `CONFIG_`, then re-render webconfig + rebuild the disk image
   (CI does this).
7. The two guards - add `|| state->config.disable_switching` to the early return
   in both `do_screen_switch()` (`src/mouse.c`) and
   `output_toggle_hotkey_handler()` (`src/handlers.c`). For the persistent-default
   flavor instead, seed `state->switch_lock` from the config flag at boot rather
   than checking it in the guards.

Add coverage in `tests/switch_oracle.py` and/or `tests/test_switch.c` so CI
verifies the flag blocks both switch paths.

### Status

SHIPPED 2026-07-16 (persistent-default flavor), together with two extras that
came out of the same review:

- **Per-direction jump thresholds**: `jump_threshold_down` gives the downward
  crossing (top PC bottom edge, where the Windows taskbar lives) its own force,
  separate from the up-crossing. Web config: "Jump Threshold (Up)" / "(Down)".
- **Release-on-tag CI**: pushing a `v*` tag attaches `deskhop.uf2` to a GitHub
  Release, since workflow artifacts expire after ~90 days.

Config version bumped 9 -> 10 (new fields carved out of `_reserved`; footprint
unchanged). NOTE: the bump reseeds saved config on first boot after flashing,
so web-config tweaks (e.g. a lowered jump threshold) must be re-applied once.

The hard-lock flavor remains unbuilt; revisit only if the Right Ctrl + K escape
hatch proves to be a problem.

---

## 2. Power-loss resilience / input-host survivability

**Concern:** if the computer powering the board that *hosts the mouse* is
suddenly powered down, that board loses power, the mouse loses its USB host, and
mouse control is lost until the PC returns or the mouse is replugged into a live
port.

### Mechanism (verified in code)

- Each Pico is powered by the computer its own micro-USB plugs into; the two
  halves are galvanically isolated (no shared power rail).
- A device is hosted by whichever Pico's USB-A port it is physically plugged
  into (`src/usb.c`, `enforce_ports = 0` so either device works on either board).
  The mouse's switch logic runs on that same board.
- The **deciding factor** is therefore which Pico hosts the mouse:
  - Mouse on **Pico B (Work PC)**: Work PC off -> Pico B dead -> mouse dead.
    Recovery only via the keyboard if it is on the live board (Ctrl+Caps to the
    Dev PC), but no mouse until Work returns / mouse replugged.
  - Mouse on **Pico A (Dev PC, always on)**: mouse survives. The surviving board
    can switch back to Dev on its own - `set_active_output()` flips
    `active_output` locally and unconditionally, then only notifies the (dead)
    other board (`src/handlers.c`). Caveat: Work displays are off, so recovery is
    "blind"; the keyboard hotkey (Ctrl+Caps) is the reliable blind path since the
    up-crossing only fires from the middle monitor.

### Mitigations

- **Wiring (no code):** host the mouse (ideally the keyboard too) on the
  always-on Dev PC side (Pico A), using a small USB hub on that port if needed.
  Then a Work PC power-loss never costs input devices. This is the README's
  recommended "both devices on one side via a hub" approach.
- **Software (optional):** the hard-lock flavor of item 1 can reduce the chance
  of stranding yourself on a PC that might power off.

### Open task

Determine and document the current physical wiring: which Pico (A/B) hosts the
mouse, and which hosts the keyboard, given the mouse is hardwired into DeskHop
and the keyboard arrives intermittently through a USB switch. Then decide whether
to rewire onto the always-on side. Safe test: power the Work PC off and observe
whether the cursor still responds and whether Ctrl+Caps reaches the Dev PC.

### Status

Deferred / investigation needed. Primarily a wiring decision; no code required
unless pairing with item 1.
