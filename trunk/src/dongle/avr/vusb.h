#pragma once

#include "usbdrv.h"

// the USB keyboard HID report
typedef struct
{
	uint8_t		modifiers;
	uint8_t		reserved;
	uint8_t		keys[6];
} vusb_keyboard_report_t;

extern vusb_keyboard_report_t vusb_keyboard_report;	// the HID keyboard report
extern uint8_t vusb_idle_rate;						// in 4 ms units
extern uint8_t vusb_consumer_report;				// sound control report

void vusb_init(void);

bool vusb_poll(void);			// returns true if the idle duration has expired
void vusb_reset_idle(void);		// resets the idle duration
