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
#include "avr_serial.h"
#include "utils.h"
#include "matrix.h"
#include "led.h"
#include "nRF24L01.h"
#include "rf_protocol.h"
#include "rf_ctrl.h"
#include "keycode.h"
#include "sleeping.h"
#include "ctrl_settings.h"

void process_normal(void)
{
	bool waiting_for_all_keys_up = false;
	bool are_all_keys_up;

	do {
		wait_for_matrix_change();

		// make a key state report
		rf_msg_key_state_report_t report;
		report.msg_type = MT_KEY_STATE;
		report.modifiers = 0;
		report.consumer = 0;

		uint8_t num_keys = 0;

		are_all_keys_up = true;
		
		if (is_pressed(KC_FUNC))
		{
			are_all_keys_up = false;
			
			// set the bits of the consumer byte
			if (is_pressed(KC_F1))		report.consumer |= _BV(FN_MUTE_BIT);
			if (is_pressed(KC_F2))		report.consumer |= _BV(FN_VOL_DOWN_BIT);
			if (is_pressed(KC_F3))		report.consumer |= _BV(FN_VOL_UP_BIT);
			if (is_pressed(KC_F4))		report.consumer |= _BV(FN_PLAY_PAUSE_BIT);
			if (is_pressed(KC_F5))		report.consumer |= _BV(FN_PREV_TRACK_BIT);
			if (is_pressed(KC_F6))		report.consumer |= _BV(FN_NEXT_TRACK_BIT);

			if (is_pressed(KC_ESC))
				waiting_for_all_keys_up = true;

		} else {
		
			uint8_t row, col;
			for (row = 0; row < NUM_ROWS; ++row)
			{
				for (col = 0; col < NUM_COLS; ++col)
				{
					if (matrix[row] & _BV(col))
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
		bool is_sent;
		const uint8_t MAX_DROP_CNT = 10;
		uint8_t drop_cnt = 0;
		do {
			is_sent = rf_ctrl_send_message(&report, num_keys + 3);
			if (!is_sent)
				++drop_cnt;

			// flush the ACK payloads
			rf_ctrl_process_ack_payloads(NULL, NULL);

		} while (!is_sent  &&  drop_cnt < MAX_DROP_CNT);
		
	} while (!waiting_for_all_keys_up  ||  are_all_keys_up);
}

void send_text(const char* msg, bool is_flash, bool wait_for_finish)
{
	/*
	if (is_flash)
		dprintf_P(txt);
	else
		dprintf(txt);
	_delay_ms(1);
	*/

	rf_msg_text_t txt_msg;
	txt_msg.msg_type = MT_TEXT;

	// flush the ACK payload
	uint8_t msg_bytes_free;
	rf_ctrl_process_ack_payloads(NULL, NULL);

	// send the message in chunks of MAX_TEXT_LEN
	uint16_t msglen = is_flash ? strlen_P(msg) : strlen(msg);
	uint8_t chunklen;
	while (msglen)
	{
		// copy a chunk of the message
		chunklen = msglen > MAX_TEXT_LEN ? MAX_TEXT_LEN : msglen;
		if (is_flash)
			memcpy_P(txt_msg.text, msg, chunklen);
		else
			memcpy(txt_msg.text, msg, chunklen);

		msglen -= chunklen;
		msg += chunklen;

		// query the free space of the buffer on the dongle to see if
		// it is larege enough to store the next chunk

		for (;;)
		{
			rf_ctrl_send_message(&txt_msg, 1);

			rf_ctrl_process_ack_payloads(&msg_bytes_free, NULL);
			
			if (msg_bytes_free > chunklen + 1)
				break;
		}
		
		// send the message
		rf_ctrl_send_message(&txt_msg, chunklen + 1);
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
			rf_ctrl_send_message(&txt_msg, 1);
			rf_ctrl_process_ack_payloads(&msg_bytes_free, &msg_bytes_capacity);
		} while (msg_bytes_free == 0  ||  msg_bytes_free != msg_bytes_capacity);
	}
}

// returns the battery voltage in 10mV units
// for instance: get_battery_voltage() returning 278 means voltage == 2.78V
uint16_t get_battery_voltage(void)
{
	//power_adc_enable();
	
	ADMUX = _B0(REFS1) | _B0(REFS0)	// reference AVCC
			| _B1(ADLAR)			// left adjust ADC
			| 0b11110;				// measure 1.1v internal reference
			
	ADCSRA = _BV(ADEN)		// enable ADC
			| _BV(ADSC);	// start conversion
	
	// wait for the conversion to finish
	loop_until_bit_is_set(ADCSRA, ADIF);

	// remember the result
	uint8_t adc_result = ADCH;
	
	// clear the ADIF bit by writing one
	SetBit(ADCSRA, ADIF);
	
	ADCSRA = 0;				// disable ADC
	
	//power_adc_disable();	// ADC power off
	
	return 28050 / adc_result;
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
			if (matrix[row] & _BV(col))
				keycode_pressed = get_keycode(row, col);
		}
	}
	
	wait_for_all_keys_up();
	
	return keycode_pressed;
}

void process_menu(void)
{
	// print the main menu
	bool exit_menu = false;
	uint8_t keycode;
	char string_buff[10];
	while (!exit_menu)
	{
		send_text(PSTR("\x017G wireless config menu\n"
						"battery voltage="), true, false);

		get_battery_voltage_str(string_buff);
		send_text(string_buff, false, false);
		
		send_text(PSTR("\n\nwhat do you want to do?\n"
						"F1 - change transmitter output power (current "), true, false);
		switch (get_nrf_output_power())
		{
		case vRF_PWR_M18DBM:	send_text(PSTR("-18dBm)\n"), true, false); 	break;
		case vRF_PWR_M12DBM:	send_text(PSTR("-12dBm)\n"), true, false); 	break;
		case vRF_PWR_M6DBM:		send_text(PSTR("-6dBm)\n"), true, false); 		break;
		case vRF_PWR_0DBM:		send_text(PSTR("0dBm)\n"), true, false); 		break;
		}
		
		send_text(PSTR("F2 - change LED brightness (current "), true, false);
		
		itoa(get_led_brightness(), string_buff, 10);
		send_text(string_buff, false, false);
		
		send_text(PSTR(")\nESC - exit menu\n\n"), true, false);
		
		do {
			keycode = get_key_input();
		} while (keycode != KC_F1  &&  keycode != KC_F2  &&  keycode != KC_ESC);

		if (keycode == KC_F1)
		{
			send_text(PSTR("select power:\nF1 0dBm\nF2 -6dBm\nF3 -12dBm\nF4 -18dBm\n"), true, false);
			
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
			send_text(PSTR("press F1 (dim) to F12 (bright) for brightness, Esc to finish\n"), true, false);
			
			do {
				keycode = get_key_input();
				if (keycode >= KC_F1  &&  keycode <= KC_F12)
				{
					set_led_brightness((keycode - KC_F1)*10 + 1);
					set_leds(7, 50);
				}
			} while (keycode != KC_ESC);
			
		} else if (keycode == KC_ESC) {
			exit_menu = true;
			send_text(PSTR("\nexiting menu, you can type now\n"), true, true);
		}
	}
}

void init_hw(void)
{
	// power down everything we don't need
	//power_adc_disable();	// power on/off in get_battery_voltage()
	power_lcd_disable();
	//power_spi_disable();	// maybe we should power this off when not in use?
	power_timer1_disable();
	power_usart0_disable();	// init_serial() will power on the USART if called
	SetBit(ACSR, ACD);		// analog comparator off
	
	// default all pins to input with pullups
	DDRA = 0;	PORTA = 0xff;
	DDRB = 0;	PORTB = 0xff;
	DDRC = 0;	PORTC = 0xff;
	DDRD = 0;	PORTD = 0xff;
	DDRE = 0;	PORTE = 0xff;
	DDRF = 0;	PORTF = 0xff;
	DDRG = 0;	PORTG = 0xff;
	
	DDRE = _BV(0) | _BV(1);	// !!!
	PORTE = 0;
}

int main(void)
{
	// initialize the hardware
	init_hw();
	matrix_init();
	rf_ctrl_init();
	init_leds();
	init_sleep();
#ifdef DBGPRINT	
	init_serial();
#endif
	
	sei();	// enable interrupts

	dprintf("i live...\n");	
	for (;;)
	{
		process_normal();
		//process_menu();
	}

	return 0;
}
