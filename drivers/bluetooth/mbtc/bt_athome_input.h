/*
 *
 * Copyright (C) 2013 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _BT_ATHOME_INPUT_H_
#define _BT_ATHOME_INPUT_H_

#ifndef HACK_DEBUG_USING_LED
// Set to 1 if you want to change the LED color based on the last BTLE event.
// This is a hack that was added for debugging. It will help us know
// whether Bemote or Wolfie is hung.
// See corresponding change in "drivers/misc/aah_io.c".
#define HACK_DEBUG_USING_LED  1
#endif

#if HACK_DEBUG_USING_LED
void athome_bt_led_show_event(int event_type);
#else
#define athome_bt_led_show_event(c) /* noop */
#endif

#define HACK_LED_EVENT_CONNECT       0
#define HACK_LED_EVENT_AWAKE         1
#define HACK_LED_EVENT_TOUCH_DOWN    2
#define HACK_LED_EVENT_BUTTON_DOWN   3
#define HACK_LED_EVENT_INPUT_UP      4
#define HACK_LED_EVENT_ASLEEP        5
#define HACK_LED_EVENT_DISCONNECT    6

#define AAH_BT_KEY_DPAD_CENTER			353
#define AAH_BT_KEY_POWER			177
#define AAH_BT_KEY_INPUT			178

#define AAH_BT_TOUCHPAD_WIDTH			50	/* in mm */
#define AAH_BT_TOUCHPAD_HEIGHT			50	/* in mm */

#define ATHOME_MAX_FINGERS			3

#define AAH_BT_UNKNOWN_MSEC			-1

int athome_bt_input_init(void);
void athome_bt_input_deinit(void);
void athome_bt_input_send_touch(unsigned which, int pointer_idx,
					uint16_t x, uint16_t y);
void athome_bt_input_send_buttons(unsigned which, uint32_t mask);
void athome_bt_input_frame(unsigned which, long msec_since_last);


#endif

