#include <stdbool.h>
#include <stdio.h>

#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "sleeping.h"
#include "matrix.h"
#include "led.h"
#include "utils.h"
#include "avr_serial.h"

uint16_t tcnt2_lword = 0;
uint16_t tcnt2_hword = 0;

void add_ticks(const uint8_t ticks)
{
	// mind the overflow
	if (ticks > 0xffff - tcnt2_lword)
		tcnt2_hword++;
		
	tcnt2_lword += ticks;
}

// wake from sleep interrupt
ISR(TIMER2_OVF_vect)
{}

// sleep for sleep_ticks number of TCNT2 ticks
void sleep_ticks(uint8_t ticks)
{
SetBit(PORTE, 1);

	if (are_leds_on())
	{
		add_ticks(ticks);
		while (ticks--)
			_delay_us(277.78);
	} else {
		sleep_enable();
		add_ticks(TCNT2);
		TCNT2 = 0xff - ticks;		// set the sleep duration
		add_ticks(ticks);
		sleep_mode();				// go to sleep
		sleep_disable();
	}
ClrBit(PORTE, 1);
}

void init_sleep(void)
{
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	// config the wake-up timer
	TCCR2A = _BV(WGM21) | _BV(WGM20)	// CTC mode
				| _BV(CS22) | _BV(CS21) | _BV(CS20);	// 1024 prescaler
														// TCNT=277.78us  OVF=71.11ms

	TIMSK2 = _BV(TOIE2);	// interrupt on overflow
}

uint16_t dyn_sleep_ticks = 0;
uint8_t sleep_ticks_prescaler = 0;

#define MIN_SLEEP_TICKS		0x01
#define MAX_SLEEP_TICKS		0xff

void sleep_dynamic(void)
{
	sleep_ticks_prescaler ++;
	
	if (sleep_ticks_prescaler == 4)
	{
		sleep_ticks_prescaler = 0;
		if (dyn_sleep_ticks < MAX_SLEEP_TICKS)
			dyn_sleep_ticks ++;
	}

	sleep_ticks(dyn_sleep_ticks);
}

void sleep_reset(void)
{
	dyn_sleep_ticks = MIN_SLEEP_TICKS;
}

void wait_for_all_keys_up(void)
{
	sleep_reset();
	do {
		matrix_scan();
		sleep_dynamic();
	} while (is_any_key_pressed());
}

void wait_for_key_down(void)
{
	sleep_reset();
	do {
		matrix_scan();
		sleep_dynamic();
	} while (!is_any_key_pressed());
}

void wait_for_matrix_change(void)
{
	sleep_reset();
	while (!matrix_scan())
		sleep_dynamic();
}
