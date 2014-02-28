#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "reports.h"
#include "keycode.h"
#include "rf_protocol.h"
#include "text_message.h"
#include "rf_dngl.h"

hid_kbd_report_t	usb_keyboard_report;
uint8_t				usb_consumer_report;

// contains the last received LED report
uint8_t usb_led_report;		// bit	LED
							// 0	CAPS
							// 1	NUM
							// 2	SCROLL

void reset_keyboard_report(void)
{
	usb_keyboard_report.modifiers = 0;
	usb_keyboard_report.keys[0] = KC_NO;
	usb_keyboard_report.keys[1] = KC_NO;
	usb_keyboard_report.keys[2] = KC_NO;
	usb_keyboard_report.keys[3] = KC_NO;
	usb_keyboard_report.keys[4] = KC_NO;
	usb_keyboard_report.keys[5] = KC_NO;
}

// updates usb_keyboard_report and usb_consumer_report from the
// data in the key state message contained in the recv_buffer
void process_key_state_msg(__xdata const uint8_t* recv_buffer, const uint8_t bytes_received)
{
	__xdata const rf_msg_key_state_report_t* key_state_msg = (const rf_msg_key_state_report_t*) recv_buffer;
	__xdata uint8_t key_cnt;

	usb_consumer_report = key_state_msg->consumer;				// the consumer report
	
	reset_keyboard_report();
	
	usb_keyboard_report.modifiers = key_state_msg->modifiers;	// set the modifiers

	// copy the keycodes
	for (key_cnt = 0; key_cnt < bytes_received - 3; key_cnt++)
		usb_keyboard_report.keys[key_cnt] = key_state_msg->keys[key_cnt];
}

void process_text_msg(__xdata const uint8_t* recv_buffer, const uint8_t bytes_received)
{
	__xdata const rf_msg_text_t* msg = (__xdata const rf_msg_text_t*) recv_buffer;
	const char* txt = msg->text;
	const uint8_t txt_size = bytes_received - 2;
	__xdata rf_msg_text_buff_state_t msg_ack;
	
	static uint8_t prev_msg_id = 0;
	static uint8_t pref_free = 0;

	/*
	// if we have text and we didn't already process this message
	// also, check if we have enough space for the entire message
	if (txt_size  &&  prev_msg_id != msg->msg_id  &&  msg_free() >= txt_size + 1)
	{
		// copy the text to our ring buffer
		uint8_t ndx = 0;
		while (ndx < txt_size)
			msg_push(txt[ndx++]);

		msg_push(0);	// adds a key-up at the end of the message
	
		prev_msg_id = msg->msg_id;	// remember this message id
	}
	*/

	if (txt_size  &&  msg_free() > 1)
	{
		char buff[30];
		char* p;
		uint8_t f = pref_free;	//msg_free();
		uint8_t c = 0;
		
		for (c = 0; c < txt_size - 1; ++c)
			buff[c] = 'a' + c;

		p = buff;
		
		if (*txt == 0x01)
		{
			buff[0] = 0x01;
			p++;
		} else if (msg_free() < txt_size + 1) {
			buff[0] = '!';
			p++;
		}
			
		_itoa(f, p, 10);
		++p;
		if (f > 9)
			++p;
			
		*p++ = ',';
		
		_itoa(txt_size, p, 10);
		if (txt_size > 9)
			p[2] = ' ';
		else
			p[1] = ' ';
		
		buff[txt_size-1] = '\n';
		
		c = 0;
		while (c < txt_size)
			msg_push(buff[c++]);		
			
		msg_push(0);
	}
		
			/*
	if (txt_size  &&  prev_msg_id != msg->msg_id)
	{
		// also, check if we have enough space for the entire message
		uint8_t buff_free = msg_free();
		if (buff_free >= txt_size + 1)
		{
			// copy the text to our ring buffer
			uint8_t ndx = 0;
			while (ndx < txt_size)
				msg_push(txt[ndx++]);

			msg_push(0);	// adds a key-up at the end of the message
			
		} else if (buff_free >= 9) {
			char buff[5];
			char* p = buff;
			
			msg_push('!');
			_itoa(msg_free(), buff, 10);
			while (*p)
				msg_push(*p++);
			msg_push(',');
			_itoa(txt_size, buff, 10);
			p = buff;
			while (*p)
				msg_push(*p++);
			msg_push('!');
			msg_push('\0');
		} else if (buff_free >= 2) {
			msg_push('?');
			msg_push('\0');
		}

		prev_msg_id = msg->msg_id;	// remember this message id
	}*/

	// queue the buffer state in the ACK
	msg_ack.msg_type = MT_TEXT_BUFF_FREE;
	msg_ack.bytes_free = msg_free();
	msg_ack.bytes_capacity = msg_capacity();
	rf_dngl_queue_ack_payload(&msg_ack, sizeof msg_ack);
	
	pref_free = msg_ack.bytes_free;
}
