# Deskhop-Fork: maintainer notes for Claude

Fork of `hrvach/deskhop` (RP2040 KVM firmware, C, GPL-3.0) that adds a custom
**vertical 3+1 monitor switching** feature, gated behind
`#ifdef DESKHOP_LAYOUT_VERTICAL_3PLUS1` (defined in `src/include/user_config.h`).

## Syncing upstream changes (IMPORTANT)

Do NOT use GitHub's "Sync fork" button. It only performs clean auto-merges and
will fail here, because this fork's `src/mouse.c` changes overlap the
screen-switching code that upstream actively edits.

Pull upstream updates with a local merge and resolve the conflict instead:

```
git fetch upstream            # upstream remote = hrvach/deskhop
git merge upstream/main
# Conflicts are almost always ONLY in src/mouse.c. When resolving:
#   - keep upstream's is_screen_switch_needed() / get_jump_threshold() refactors
#   - re-add the vertical (TOP/BOTTOM) axis under DESKHOP_LAYOUT_VERTICAL_3PLUS1
git commit
git push origin main
```

After merging, both CI workflows (firmware ARM **Build** and **Host tests**) must
go green before relying on it. `python tests/switch_oracle.py` is a quick
no-toolchain logic check of the switch state machine.

## Key facts (so a fresh session can act)

- DeskHop outputs: **OUTPUT_B = the 3-monitor "dev" PC** (screen_index 1 = MIDDLE,
  2 = LEFT, 3 = RIGHT); **OUTPUT_A = the single-monitor "Secondary" PC** mounted
  above the middle. The PCs cross only at the middle monitor's top edge. Wire these
  backwards and the cursor crosses the wrong way (and `screen_count = 3` lands on
  the wrong PC).
- On Windows the MIDDLE monitor must be the OS "primary display" (absolute-coords
  requirement, see `user_config.h`).
- Switching logic lives in `src/mouse.c` (`do_screen_switch`), with the PC-switch
  state resets in `src/handlers.c`. Tests are in `tests/` (C harnesses + a Python
  oracle).
- There is no local C/ARM toolchain on the dev machine, so CI (GitHub Actions) is
  the build/test verifier; the firmware `.uf2` is a CI artifact.
