/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 *
 **===================================================== *
 * ==========  Keyboard LED Output Indicator  ========== *
 * ===================================================== *
 *
 * If you are willing to give up on using the keyboard LEDs for their original purpose,
 * you can use them as a convenient way to indicate which output is selected.
 *
 * KBD_LED_AS_INDICATOR set to 0 will use the keyboard LEDs as normal.
 * KBD_LED_AS_INDICATOR set to 1 will use the Caps Lock LED as indicator.
 *
 * */

#define KBD_LED_AS_INDICATOR 0


/**===================================================== *
 * ============  Vertical Layout toggle  =============== *
 * ===================================================== *
 *
 * Enable the custom vertical 3+1 monitor layout. This is defined here, before
 * JUMP_THRESHOLD, so the layout can raise the PC-switch threshold for its
 * vertical edge. Full description, revert steps, and the Windows primary-display
 * requirement are in the "Vertical Layout" section lower in this file. Comment
 * this out to restore the stock left/right switching.
 * */

#define DESKHOP_LAYOUT_VERTICAL_3PLUS1

/**===================================================== *
 * ===========  Hotkey for output switching  =========== *
 * ===================================================== *
 *
 * Everyone is different, I prefer to use caps lock because I HATE SHOUTING :)
 * You might prefer something else. Pick something from the list found at:
 *
 * https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
 *
 * defined as HID_KEY_<something>
 *
 * In addition, there is an optional modifier you can use for the hotkey
 * switching. Currently, it's set to LEFT CTRL, you can change it by
 * redefining HOTKEY_MODIFIER here from KEYBOARD_MODIFIER_LEFTCTRL to something
 * else (check file referenced below) or HID_KEY_NONE.
 *
 * If you do not want to use a key for switching outputs, you may be tempted
 * to select HID_KEY_NONE here as well; don't do that! That code appears in many
 * HID messages and the result will be a non-functional keyboard. Instead, choose
 * a key that is unlikely to ever appear on a keyboard that you will use.
 * HID_KEY_F24 is probably a good choice as keyboards with 24 function keys
 * are rare.
 *
 * */

#define HOTKEY_MODIFIER  KEYBOARD_MODIFIER_LEFTCTRL
#define HOTKEY_TOGGLE    HID_KEY_CAPS_LOCK

/**================================================== *
 * ==============  Mouse Speed Factor  ============== *
 * ================================================== *
 *
 * This affects how fast the mouse moves.
 *
 * MOUSE_SPEED_A_FACTOR_X: [1-128], mouse moves at this speed in X direction
 * MOUSE_SPEED_A_FACTOR_Y: [1-128], mouse moves at this speed in Y direction
 *
 * JUMP_THRESHOLD: [0-32768], sets the "force" you need to use to drag the
 * mouse to another screen, 0 meaning no force needed at all, and ~500 some force
 * needed, ~1000 no accidental jumps, you need to really mean it.
 *
 * This is now configurable per-screen.
 *
 * ENABLE_ACCELERATION: [0-1], disables or enables mouse acceleration.
 *
 * */

/* Output A values, default is for the most common ~ 16:9 ratio screen */
#define MOUSE_SPEED_A_FACTOR_X 16
#define MOUSE_SPEED_A_FACTOR_Y 28

/* Output B values, default is for the most common ~ 16:9 ratio screen */
#define MOUSE_SPEED_B_FACTOR_X 16
#define MOUSE_SPEED_B_FACTOR_Y 28

#ifdef DESKHOP_LAYOUT_VERTICAL_3PLUS1
/* The middle monitor's top edge is the only upward PC crossing in the vertical
   layout and is easy to hit by accident (reaching a menu bar, fast upward
   flicks), so require deliberate "force" to cross. Tune in the web config
   ("Jump Threshold (Up)") if desired. */
#define JUMP_THRESHOLD 400
/* The downward crossing (top PC's bottom edge -> middle monitor) gets its own
   force: the Windows taskbar sits right on that edge, so overshooting the
   taskbar should not drop you onto the other PC. Tuned separately in the web
   config ("Jump Threshold (Down)"). */
#define JUMP_THRESHOLD_DOWN 400
#else
#define JUMP_THRESHOLD 0
#define JUMP_THRESHOLD_DOWN 0
#endif

/* Mouse acceleration */
#define ENABLE_ACCELERATION 1

/**================================================== *
 * ==============  Screensaver Config  ============== *
 * ================================================== *
 *
 * While this feature is called 'screensaver', it's not actually a
 * screensaver :) Really it's a way to ensure that some sort of mouse
 * activity will be sent to one (or both) outputs when the user has
 * not interacted with that output. This can be used to stop a
 * screensaver or screenlock from activating on the attached computer,
 * or to just watch the mouse pointer bouncing around!
 *
 * When the mode is active on an output, the pointer will jump around
 * the screen like a bouncing-ball in a Pong game (however no click
 * events will be generated, of course).
 *
 * This mode is activated by 'idle time' on a per-output basis; if the
 * mode is enabled for output B, and output B doesn't have any
 * activity for (at least) the specified idle time, then the mode will
 * be activated and will continue until the inactivity time reaches
 * the maximum (if one has been specified). This allows you to stop a
 * screensaver/screenlock from activating while you are still at your
 * desk (but just interacting with the other computer attached to
 * Deskhop), but let it activate if you leave your desk for an
 * extended period of time.
 *
 * Additionally, this mode can be automatically disabled if the output
 * is the currently-active output.
 *
 * If you only set the ENABLED options below, and leave the rest of
 * the defaults in place, then the screensaver mode will activate
 * after 4 minutes (240 seconds) of inactivity, will continue forever,
 * but will only activate on an output that is not currently
 * active.
 *
 * */

/**================================================== *
 *
 * SCREENSAVER_{A|B}_MODE: DISABLED
 *                         PONG
 *                         JITTER
 *
 * */

#define SCREENSAVER_A_MODE DISABLED
#define SCREENSAVER_B_MODE DISABLED

/**================================================== *
 *
 * SCREENSAVER_{A|B}_IDLE_TIME_SEC: Number of seconds that an output
 * must be inactive before the screensaver mode will be activated.
 *
 * */

#define SCREENSAVER_A_IDLE_TIME_SEC 240
#define SCREENSAVER_B_IDLE_TIME_SEC 240

/**================================================== *
 *
 * SCREENSAVER_{A|B}_MAX_TIME_SEC: Number of seconds that the screensaver
 * will run on an output before being deactivated. 0 for indefinitely.
 *
 * */

#define SCREENSAVER_A_MAX_TIME_SEC 0
#define SCREENSAVER_B_MAX_TIME_SEC 0

/**================================================== *
 *
 * SCREENSAVER_{A|B}_ONLY_IF_INACTIVE: [0 or 1] 1 means the
 * screensaver will activate only if the output is inactive.
 *
 * */

#define SCREENSAVER_A_ONLY_IF_INACTIVE 0
#define SCREENSAVER_B_ONLY_IF_INACTIVE 0

/**================================================== *
 * ================  Output OS Config =============== *
 * ================================================== *
 *
 * Defines OS an output connects to. You will need to worry about this only if you have
 * multiple desktops and one of your outputs is MacOS or Windows.
 *
 * Available options: LINUX, MACOS, WINDOWS, OTHER (check main.h for details)
 *
 * OUTPUT_A_OS: OS for output A
 * OUTPUT_B_OS: OS for output B
 *
 * */

#define OUTPUT_A_OS WINDOWS
#define OUTPUT_B_OS WINDOWS


/**================================================== *
 * ===========  Vertical Layout (fork)  ============ *
 * ================================================== *
 *
 * Custom switching for a non-linear, partly-vertical monitor layout that the
 * stock left/right model cannot express:
 *
 *   - OUTPUT_B = "bottom" PC with a row of monitors
 *                (screen_index 1 = MIDDLE/main, 2 = LEFT, 3 = RIGHT)
 *   - OUTPUT_A = "top" PC, a single monitor mounted ABOVE the middle one
 *
 * The mouse crosses between the two PCs ONLY at the middle monitor's top edge
 * (up to OUTPUT_A) and OUTPUT_A's bottom edge (back down to the middle). Moving
 * left/right just walks across the bottom PC's monitors. The matching switch
 * logic lives in do_screen_switch() in mouse.c.
 *
 * To enable: define DESKHOP_LAYOUT_VERTICAL_3PLUS1 (done near the top of this
 * file) and set screen_count = 3 for the bottom PC (defaults.c, or via the web
 * config). To revert to stock left/right switching: comment that define out AND
 * reset OUTPUT_B screen_count back to 1. The per-output "pos" / "Screen Position"
 * field is NOT used by this layout (only the stock path reads it).
 *
 * IMPORTANT (Windows): set the MIDDLE monitor as the Windows "primary display"
 * on the bottom PC. Since KB5003637, Windows maps absolute mouse coordinates
 * only to the primary monitor (see README known issues); mapping the middle
 * monitor to screen_index 1 (the absolute "main" screen) is what makes the
 * vertical jump land correctly. The left/right monitors use DeskHop's
 * (experimental) relative-mouse workaround for non-primary screens. The top
 * monitor should be horizontally aligned with and similar in width to the
 * middle one: X is carried over 1:1 on a vertical jump (no X scaling), so a
 * large width/offset mismatch shifts the cursor horizontally on arrival.
 *
 * */


/**================================================== *
 * =================  Enforce Ports ================= *
 * ================================================== *
 *
 * If enabled, fixes some device incompatibilities by
 * enforcing keyboard has to be in port A and mouse in port B.
 *
 * ENFORCE_PORTS: [0, 1] - 1 means keyboard has to plug in A and mouse in B
 *                         0 means no such layout is enforced
 *
 * */

#define ENFORCE_PORTS 0


/**================================================== *
 * ==============  Disable Switching  =============== *
 * ================================================== *
 *
 * If set to 1, the device boots with the switch lock engaged: the mouse cannot
 * cross to the other PC and the output-toggle hotkey is ignored, exactly as if
 * the switch-lock combo (Right Ctrl + K) had been pressed. That combo still
 * works at runtime to unlock (and re-lock); this only seeds the boot state.
 * Also configurable without a rebuild via the web config ("Disable Switching").
 *
 * DISABLE_SWITCHING: [0, 1]
 *
 * */

#define DISABLE_SWITCHING 0


/**================================================== *
 * =============  Enforce Boot Protocol ============= *
 * ================================================== *
 *
 * If enabled, fixes some device incompatibilities by
 * enforcing the boot protocol (which is simpler to parse
 * and with less variation)
 *
 * ENFORCE_KEYBOARD_BOOT_PROTOCOL: [0, 1] - 1 means keyboard will forcefully use
 *                                          the boot protocol
 *                                        - 0 means no such thing is enforced
 *
 * */

#define ENFORCE_KEYBOARD_BOOT_PROTOCOL 0
