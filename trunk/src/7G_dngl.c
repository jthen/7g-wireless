#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "7G_vusb.h"
#include "avr_serial.h"
#include "utils.h"
#include "hw_setup.h"
#include "rf_protocol.h"
#include "rf_dngl.h"
#include "keycode.h"
#include "text_message.h"
//#include "keycode_names.h"

void init_hw(void)
{
	// set the LEDs as outputs
	SetBit(DDR(LED1_PORT), LED1_BIT);
	SetBit(DDR(LED2_PORT), LED2_BIT);
	SetBit(DDR(LED3_PORT), LED3_BIT);
}

// updates vusb_keyboard_report and vusb_consumer_report from the
// data in the key state message contained in the recv_buffer
void process_key_state_msg(const uint8_t* recv_buffer, const uint8_t bytes_received)
{
	// cast the buffer pointer into a message pointer
	const rf_msg_key_state_report_t* key_state_msg = (const rf_msg_key_state_report_t*) recv_buffer;

	vusb_consumer_report = key_state_msg->consumer;				// the consumer report
	
	// reset the keyboard report
	memset(&vusb_keyboard_report, 0, sizeof vusb_keyboard_report);
	
	vusb_keyboard_report.modifiers = key_state_msg->modifiers;	// set the modifiers

	uint8_t key_cnt;		// copy the keys
	for (key_cnt = 0; key_cnt < bytes_received - 3; key_cnt++)
		vusb_keyboard_report.keys[key_cnt] = key_state_msg->keys[key_cnt];
}

void process_text_msg(const uint8_t* recv_buffer, const uint8_t bytes_received)
{
	const rf_msg_text_t* msg = (const rf_msg_text_t*) recv_buffer;
	const char* txt = msg->text;
	const uint8_t txt_size = bytes_received - 1;

	// only if we actually have characters in the message
	// also, check if we have enough space for the entire message
	if (txt_size  &&  msg_free() >= txt_size + 1)
	{
		// copy the text to our ring buffer
		uint8_t ndx = 0;
		while (ndx < txt_size)
			msg_push(txt[ndx++]);

		// add a key-up
		msg_push(0);
	}

	// queue the buffer state in the ACK
	rf_msg_text_buff_state_t msg_ack;
	msg_ack.msg_type = MT_TEXT_BUFF_FREE;
	msg_ack.bytes_free = msg_free();
	msg_ack.bytes_capacity = msg_capacity();
	rf_dngl_queue_ack_payload(&msg_ack, sizeof msg_ack);

	//printf("msg_sz=%i  size=%i  full=%i  empty=%i  free=%i\n", bytes_received, msg_size(), msg_full(), msg_empty(), msg_free());
}

int	main(void)
{
	_delay_ms(200);		// wait for the voltage levels to stabilize
	
	init_hw();
	init_serial();
	rf_dngl_init();
	vusb_init();

	sei();

	const uint8_t RECV_BUFF_SIZE = 32;
	uint8_t recv_buffer[RECV_BUFF_SIZE];
	uint8_t bytes_received;

	bool keyboard_report_ready = false;
	bool consumer_report_ready = false;
	bool idle_elapsed = false;
	
	puts("dongle online");
	
	for (;;)
	{
		idle_elapsed = vusb_poll();

		// try to read the recv buffer
		bytes_received = rf_dngl_recv(recv_buffer, RECV_BUFF_SIZE);

		if (bytes_received)
		{
			// we have new data, so what is it?
			if (recv_buffer[0] == MT_KEY_STATE)
			{
				process_key_state_msg(recv_buffer, bytes_received);

				consumer_report_ready = true;
				keyboard_report_ready = true;
			} else if (recv_buffer[0] == MT_TEXT) {
				process_text_msg(recv_buffer, bytes_received);
			}
		}
		
		if (!keyboard_report_ready  &&  !msg_empty())
		{
			// reset the report
			memset(&vusb_keyboard_report, 0, sizeof vusb_keyboard_report);

			static uint8_t prev_keycode = KC_NO;
			
			// get the next char from the stored text message
			uint8_t c = msg_peek();
			uint8_t new_keycode = get_keycode_for_char(c);
			
			// if the keycode is different than the previous
			// otherwise just send an empty report to simulate key went up
			if (new_keycode != prev_keycode  ||  new_keycode == KC_NO)
			{
				vusb_keyboard_report.keys[0] = new_keycode;
				vusb_keyboard_report.modifiers = get_modifiers_for_char(c);
				
				msg_pop();	// remove char from the buffer
			} else {
				new_keycode = KC_NO;
			}
			
			keyboard_report_ready = true;
			
			prev_keycode = new_keycode;		// remember for later
		}

		// send the keyboard report
        if (usbInterruptIsReady()  &&  (keyboard_report_ready  ||  idle_elapsed))
		{
			/*
			uint8_t c = 0;
			printf("HID ");
			while (vusb_keyboard_report.keys[c])
			{
				char buff[28];
				strcpy_P(buff, GetCodeName(vusb_keyboard_report.keys[c]));
				printf("%s ", buff);
				++c;
			}
			puts(c ? "" : "<empty>");
			*/

            usbSetInterrupt((void*) &vusb_keyboard_report, sizeof vusb_keyboard_report);
			keyboard_report_ready = false;
			
			vusb_reset_idle();
		}

		// send the audio and media controls report
        if (usbInterruptIsReady3()  &&  (consumer_report_ready  ||  idle_elapsed))
		{
            usbSetInterrupt3((void*) &vusb_consumer_report, sizeof vusb_consumer_report);
			consumer_report_ready = false;
		}
	}

	return 0;
}
