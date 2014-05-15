#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "hw_setup.h"
#include "avrdbg.h"
#include "avrutils.h"
#include "matrix.h"
#include "led.h"
#include "nRF24L.h"
#include "rf_protocol.h"
#include "rf_ctrl.h"
#include "keycode.h"
#include "sleeping.h"
#include "ctrl_settings.h"

// returns false if we should enter the menu, true if we should lock the keyboard
bool process_normal(void)
{
	bool waiting_for_all_keys_up = false;
	bool are_all_keys_up;
	bool ret_val = false;

	do {
		wait_for_matrix_change();

		// make a key state report
		rf_msg_key_state_report_t report;
		report.msg_type = MT_KEY_STATE;
		report.modifiers = 0;
		report.consumer = 0;

		uint8_t num_keys = 0;

		are_all_keys_up = true;
		
		if (is_pressed_keycode(KC_FN0))
		{
			are_all_keys_up = false;
			
			// set the bits of the consumer byte (media keys)
			if (is_pressed_keycode(KC_F1))		report.consumer |= _BV(FN_MUTE_BIT);
			if (is_pressed_keycode(KC_F2))		report.consumer |= _BV(FN_VOL_DOWN_BIT);
			if (is_pressed_keycode(KC_F3))		report.consumer |= _BV(FN_VOL_UP_BIT);
			if (is_pressed_keycode(KC_F4))		report.consumer |= _BV(FN_PLAY_PAUSE_BIT);
			if (is_pressed_keycode(KC_F5))		report.consumer |= _BV(FN_PREV_TRACK_BIT);
			if (is_pressed_keycode(KC_F6))		report.consumer |= _BV(FN_NEXT_TRACK_BIT);

			// if only Func and Esc are pressed
			if (get_num_keys_pressed() == 2)
			{
				if (is_pressed_keycode(KC_ESC))
				{
					waiting_for_all_keys_up = true;
				} else if (is_pressed_keycode(KC_L)) {
					waiting_for_all_keys_up = true;
					ret_val = true;
				} else if (is_pressed_keycode(KC_KP_MINUS)) {

					uint8_t curr_power = get_nrf_output_power();

					if (curr_power == vRF_PWR_0DBM)
						set_nrf_output_power(vRF_PWR_M6DBM);
					else if (curr_power == vRF_PWR_M6DBM)
						set_nrf_output_power(vRF_PWR_M12DBM);
					else if (curr_power == vRF_PWR_M12DBM)
						set_nrf_output_power(vRF_PWR_M18DBM);

				} else if (is_pressed_keycode(KC_KP_PLUS)) {

					uint8_t curr_power = get_nrf_output_power();

					if (curr_power == vRF_PWR_M18DBM)
						set_nrf_output_power(vRF_PWR_M12DBM);
					else if (curr_power == vRF_PWR_M12DBM)
						set_nrf_output_power(vRF_PWR_M6DBM);
					else if (curr_power == vRF_PWR_M6DBM)
						set_nrf_output_power(vRF_PWR_0DBM);
				}
			}

		} else {

			uint8_t row, col;
			for (row = 0; row < NUM_ROWS; ++row)
			{
				for (col = 0; col < NUM_COLS; ++col)
				{
					if (is_pressed_matrix(row, col))
					{
						are_all_keys_up = false;
						
						uint8_t keycode = get_keycode(row, col);
						
						if (IS_MOD(keycode))
							report.modifiers |= _BV(keycode - KC_LCTRL);
						else if (num_keys < MAX_KEYS)
							report.keys[num_keys++] = keycode;
					}
				}
			}
		}

		// send the report and wait for ACK
		if (!rf_ctrl_send_message(&report, num_keys + 3))
			return true;

		// flush the ACK payloads
		rf_ctrl_process_ack_payloads(NULL, NULL);
		
	} while (!waiting_for_all_keys_up  ||  are_all_keys_up);
	
	return ret_val;
}

bool send_text(const char* msg, bool is_flash, bool wait_for_finish)
{
#ifdef DBGPRINT
	// This needs an explanation: I am using the AVR Dragon to flash the keyboard firmware
	// and it looks like the dragon has a strong pullup on the MISO line. This pullup is
	// stronger than the nRF24L01+ MISO output driver, so if the programming ISP cable is plugged in
	// it keeps MISO high all the time, and I can't read anything from the nRF24L01+ and the send_text()
	// function will never return.
	//
	// So, instead of plugging the ISP cable in and out all the time while testing firmware updates,
	// we just output the text to UART and return
	if (is_flash)
		dprint_P(msg);
	else
		dprint(msg);
	_delay_ms(1);
	return true;
#endif

	// this message id is used to avoid presenting the same package to the dongle in case dongle
	// received the message, but the keyboard did not receive the ACK
	static uint8_t msg_id = 1;
	
	rf_msg_text_t txt_msg;
	txt_msg.msg_type = MT_TEXT;

	// send the message in chunks of MAX_TEXT_LEN
	uint16_t msglen = is_flash ? strlen_P(msg) : strlen(msg);
	uint8_t chunklen, msg_bytes_free;
	while (msglen)
	{
		// flush the ACK payload
		rf_ctrl_process_ack_payloads(&msg_bytes_free, NULL);
	
		// copy a chunk of the message
		chunklen = msglen > MAX_TEXT_LEN ? MAX_TEXT_LEN : msglen;
		if (is_flash)
			memcpy_P(txt_msg.text, msg, chunklen);
		else
			memcpy(txt_msg.text, msg, chunklen);

		msglen -= chunklen;
		msg += chunklen;

		// query the free space of the buffer on the dongle to see if
		// it is large enough to store the next chunk

		for (;;)
		{
			// send an empty text message; this causes the dongle to respond with ACK payload
			// that contains the number of bytes available in the dongle's text buffer
			if (!rf_ctrl_send_message(&txt_msg, 2))		// 1 byte for the message type ID, 1 for the msg_id
				return false;
			
			rf_ctrl_process_ack_payloads(&msg_bytes_free, NULL);
			
			// enough space in the dongle buffer?
			if (msg_bytes_free > chunklen + 1)
				break;
				
			sleep_ticks(40);		// doze off a little; roughly 10ms
		}
		
		// set the message id and send it on it's way
		msg_id = msg_id == 0xff ? 1 : msg_id + 1;
		txt_msg.msg_id = msg_id;
		if (!rf_ctrl_send_message(&txt_msg, chunklen + 2))
			return false;
	}

	// flush the ACK payload(s)
	rf_ctrl_process_ack_payloads(NULL, NULL);

	// wait for the buffer on the dongle to become empty
	// this will ensure that all the keystrokes are sent to the host and that subsequent
	// keystrokes we're sending won't mess up the text we want output at the host
	if (wait_for_finish)
	{
		uint8_t msg_bytes_capacity = 0;
		do {
			if (!rf_ctrl_send_message(&txt_msg, 2))
				return false;

			rf_ctrl_process_ack_payloads(&msg_bytes_free, &msg_bytes_capacity);
		} while (msg_bytes_free == 0  ||  msg_bytes_free != msg_bytes_capacity);
	}
	
	return true;
}

// defining this makes the ADC use the two least significant bits, but adds 24 bytes to the binary
#define PREC_BATT_VOLTAGE

// returns the battery voltage in 10mV units
// for instance: get_battery_voltage() returning 278 equals a voltage of 2.78V
uint16_t get_battery_voltage(void)
{
	power_adc_enable();
	
	ADMUX = _B0(REFS1) | _B1(REFS0)	// AVCC with external capacitor at AREF pin
#ifndef PREC_BATT_VOLTAGE	
			| _BV(ADLAR)			// left adjust ADC - drops the two LSBs
#endif
			| 0b11110;				// measure 1.1v internal reference

	ADCSRA = _BV(ADEN)					// enable ADC
			| _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0)	// prescaler 128
			| _BV(ADSC);				// start conversion
	
	// wait for the conversion to finish
	loop_until_bit_is_set(ADCSRA, ADIF);

	// remember the result
#ifdef PREC_BATT_VOLTAGE
	uint16_t adc_result = ADC;
#else	
	uint8_t adc_result = ADCH;
#endif
	
	// clear the ADIF bit by writing one
	SetBit(ADCSRA, ADIF);
	
	ADCSRA = 0;				// disable ADC
	
	power_adc_disable();	// ADC power off
	
#ifdef PREC_BATT_VOLTAGE
	return 112640 / adc_result;
#else
	return 28050 / adc_result;
#endif
}

// makes a battery voltage string in the 2.34V format
// the buff buffer has to be at least 6 bytes long
void get_battery_voltage_str(char* buff)
{
	uint16_t voltage = get_battery_voltage();
	buff[0] = '0' + voltage / 100;
	buff[1] = '.';
	buff[2] = '0' + (voltage / 10) % 10;
	buff[3] = '0' + voltage % 10;
	buff[4] = 'V';
	buff[5] = '\0';
}

// returns the keycode of the first key pressed and relased
uint8_t get_key_input(void)
{
	uint8_t keycode_pressed = KC_NO;
	
	wait_for_all_keys_up();
	wait_for_key_down();

	// scan for the key pressed
	uint8_t row, col;
	for (row = 0; row < NUM_ROWS; ++row)
	{
		for (col = 0; col < NUM_COLS; ++col)
		{
			if (is_pressed_matrix(row, col))
				keycode_pressed = get_keycode(row, col);
		}
	}
	
	wait_for_all_keys_up();
	
	return keycode_pressed;
}

bool process_menu(void)
{
	start_led_sequence(led_seq_menu_begin);
	
	// print the main menu
	uint8_t keycode;
	char* pEnd;
	const uint8_t BUFF_SIZE = 64;
	char string_buff[BUFF_SIZE];
	for (;;)
	{
		// welcome & version
		if (!send_text(PSTR("\x01"	// translates to Ctrl-A on the dongle
							"7G wireless\n"
							"firmware build " __DATE__ "  " __TIME__ "\n"
							"battery voltage: "), true, false))
			return true;
			
		get_battery_voltage_str(string_buff);
		if (!send_text(string_buff, false, false))		return true;

		// RF stats
		if (!send_text(PSTR("\nRF packet stats (total/retransmit/lost): "), true, false))		return true;
		
		ultoa(rf_packets_total, string_buff, 10);
		pEnd = strchr(string_buff, '\0');
		*pEnd++ = '/';
		
		ultoa(arc_total, pEnd, 10);
		pEnd = strchr(string_buff, '\0');
		*pEnd++ = '/';

		ultoa(plos_total, pEnd, 10);
		if (!send_text(string_buff, false, false))		return true;
		
		// output the time since reset
		uint16_t days;
		uint8_t hours, minutes, seconds;
		get_time(&days, &hours, &minutes, &seconds);
		if (!send_text(PSTR("\nkeyboard's been on for "), true, false))		return true;
		
		string_buff[0] = '\0';
		if (days > 0)
		{
			itoa(days, string_buff, 10);
			strcat_P(string_buff, PSTR(" days "));
		}

		if (hours > 0  ||  days > 0)
		{
			itoa(hours, strchr(string_buff, 0), 10);
			pEnd = strlcat_P(string_buff, PSTR(" hours "), BUFF_SIZE) + string_buff;
		} else {
			pEnd = strchr(string_buff, 0);
		}

		itoa(minutes, pEnd, 10);
		pEnd = strlcat_P(string_buff, PSTR(" minutes "), BUFF_SIZE) + string_buff;

		itoa(seconds, pEnd, 10);
		strcat_P(string_buff, PSTR(" seconds\n"));
		
		if (!send_text(string_buff, false, false))		return true;
		
		// menu
		if (!send_text(PSTR("\n\nwhat do you want to do?\n"
							"F1 - change transmitter output power (current "), true, false))		return true;
		switch (get_nrf_output_power())
		{
		case vRF_PWR_M18DBM:	send_text(PSTR("-18"), true, false); 	break;
		case vRF_PWR_M12DBM:	send_text(PSTR("-12"), true, false); 	break;
		case vRF_PWR_M6DBM:		send_text(PSTR("-6"), true, false); 	break;
		case vRF_PWR_0DBM:		send_text(PSTR("0"), true, false); 		break;
		}
		
		if (!send_text(PSTR("dBm)\nF2 - change LED brightness (current "), true, false))		return true;
		
		uint8_t fcnt;
		for (fcnt = 0; fcnt < sizeof led_brightness_lookup; ++fcnt)
		{
			if (led_brightness_lookup[fcnt] == get_led_brightness())
			{
				string_buff[0] = 'F';
				itoa(fcnt + 1, string_buff + 1, 10);
				break;
			}
		}
		
		// no match found in the loop?
		if (fcnt == sizeof led_brightness_lookup)
			itoa(get_led_brightness(), string_buff, 10);

		if (!send_text(string_buff, false, false))		return true;
		
		if (!send_text(PSTR(")\nF3 - lock keyboard (unlock with Func+Del+LCtrl)\n"
							"F4 - reset RF packet stats\n"
							"F5 - refresh this menu\n"
							"Esc - exit menu\n\n"), true, false))
			return true;

		do {
			keycode = get_key_input();
		} while (!(keycode >= KC_F1  &&  keycode <= KC_F5)  &&  keycode != KC_ESC);

		if (keycode == KC_F1)
		{
			if (!send_text(PSTR("select power:\nF1 0dBm\nF2 -6dBm\nF3 -12dBm\nF4 -18dBm\n"), true, false))
				return true;
			
			while (1)
			{
				keycode = get_key_input();
				if (keycode >= KC_F1  &&  keycode <= KC_F4)
				{
					if (keycode == KC_F1)	set_nrf_output_power(vRF_PWR_0DBM);
					if (keycode == KC_F2)	set_nrf_output_power(vRF_PWR_M6DBM);
					if (keycode == KC_F3)	set_nrf_output_power(vRF_PWR_M12DBM);
					if (keycode == KC_F4)	set_nrf_output_power(vRF_PWR_M18DBM);
					break;
				}
			}
		} else if (keycode == KC_F2) {
			if (!send_text(PSTR("press F1 (dim) to F12 (bright) for brightness, Esc to finish\n"), true, false))
				return true;
			
			do {
				keycode = get_key_input();
				if (keycode >= KC_F1  &&  keycode <= KC_F12)
				{
					set_led_brightness(led_brightness_lookup[keycode - KC_F1]);
					set_leds(7, 50);
				}
			} while (keycode != KC_ESC);
			
		} else if (keycode == KC_F3) {
			send_text(PSTR("Keyboard is now LOCKED!!!\nPress Func+Del+LCtrl to unlock\n\n"), true, false);
			return true;

		} else if (keycode == KC_F4) {

			// reset the counters to 0
			plos_total = arc_total = rf_packets_total = 0;
			
		} else if (keycode == KC_ESC) {

			start_led_sequence(led_seq_menu_end);
			send_text(PSTR("\nexiting menu, you can type now\n"), true, true);
			break;
		}
	}
	
	return false;
}

void process_lock(void)
{
	start_led_sequence(led_seq_lock);

	for (;;)
	{
		sleep_ticks(0xfe);	// long, about 62ms x 3 = 190ms
		sleep_ticks(0xfe);
		sleep_ticks(0xfe);

		if (matrix_scan()
				&&  get_num_keys_pressed() == 3
				&&  is_pressed_keycode(KC_FN0)
				&&  (is_pressed_keycode(KC_LCTRL)  ||  is_pressed_keycode(KC_RCTRL))
				&&  (is_pressed_keycode(KC_DEL)  ||  is_pressed_keycode(KC_KP_DOT)))
		{
			break;
		}
	}

	start_led_sequence(led_seq_lock);
}

void init_hw(void)
{
	// power down everything we don't need
	power_adc_disable();	// get_battery_voltage() turns this back on and off again
	power_lcd_disable();
	//power_spi_disable();	// maybe we should power this off when not in use?
	power_timer1_disable();
	power_usart0_disable();	// init_dbg() will power on the USART if called
	SetBit(ACSR, ACD);		// analog comparator off
	
	OSCCAL = 103;			// RC oscillator calibration done externally.
							// for now, just hard-code it to this value
	
	// default all pins to input with pullups
	DDRA = 0;	PORTA = 0xff;
	DDRB = 0;	PORTB = 0xff;
	DDRC = 0;	PORTC = 0xff;
	DDRD = 0;	PORTD = 0xff;
	DDRE = 0;	PORTE = 0xff;
	DDRF = 0;	PORTF = 0xff;
	DDRG = 0;	PORTG = 0xff;
	
	// PE0 and PE1 are used for timing and debugging
	// Note: these are the UART RX (PE0) and TX (PE1) pins
	
	//DDRE = _BV(0) | _BV(1);

	matrix_init();
	rf_ctrl_init();
	init_leds();
	init_sleep();
#ifdef DBGPRINT	
	init_dbg();
#endif
}

int main(void)
{
	init_hw();	// initialize the hardware

	sei();		// enable interrupts

	// 'play' a LED sequence while waiting for the 32KHz crystal to stabilize
	start_led_sequence(led_seq_boot);
	while (are_leds_on())
		;
	
	for (;;)
	{
		if (process_normal()  ||  process_menu())
			process_lock();
	}

	return 0;
}