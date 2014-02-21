#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "reg24lu1.h"

#include "keycode.h"
#include "text_message.h"

#include "usb_defs.h"
#include "usb_desc.h"
#include "usb_regs.h"

void main()
{
	bool reportPending = false;
	__code const char* msg = "This is a message that the keyboard will type when scroll lock is activated.";
	__code const char* pMsg = msg;
	uint8_t prev_keycode = KC_NO;
	
	P0DIR = 0x00;	// all outputs
	P0ALT = 0x00;	// all GPIO default behavior

	usbInit();
	
	memset(&kbdReport, 0, sizeof kbdReport);

	for (;;)
	{
		usbPoll();	// handles USB interrupts

		// make a keyboard report
		if (!reportPending  &&  (usbLEDReport & SCROLL_LOCK_MASK) != 0  &&  pMsg)
		{
			char c = *pMsg;

			// get the next char from the stored text message
			uint8_t new_keycode = get_keycode_for_char(c);

			memset(&kbdReport, 0, sizeof kbdReport);	// reset the report
			
			// if the keycode is different than the previous
			// otherwise just send an empty report to simulate key went up
			if (new_keycode != prev_keycode  ||  new_keycode == KC_NO)
			{
				kbdReport.keys[0] = new_keycode;
				kbdReport.modifiers = get_modifiers_for_char(c);

				if (c == '\0')
					pMsg = NULL;
				else
					++pMsg;
			} else {
				new_keycode = KC_NO;
			}

			prev_keycode = new_keycode;		// remember for later

			reportPending = true;
		} else if ((usbLEDReport & SCROLL_LOCK_MASK) == 0) {
			pMsg = msg;		// re-arm for next scroll lock
		}

		// send the report if the endpoint is not busy
		if ((in1cs & 0x02) == 0   &&   (reportPending  ||  usbHasIdleElapsed()))
		{
			// copy the keyboard report into the endpoint buffer
			in1buf[0] = kbdReport.modifiers;
			in1buf[1] = 0;
			in1buf[2] = kbdReport.keys[0];
			in1buf[3] = kbdReport.keys[1];
			in1buf[4] = kbdReport.keys[2];
			in1buf[5] = kbdReport.keys[3];
			in1buf[6] = kbdReport.keys[4];
			in1buf[7] = kbdReport.keys[5];

			// send the data on it's way
			in1bc = 8;
			
			reportPending = false;
		}
	}
}
