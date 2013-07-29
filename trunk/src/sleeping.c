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

// this is our watch
// not 100% accurate, but serves the purpose
// it's updated only from add_ricks() and read from get_seconds()
typedef struct
{
	uint16_t tcnt2_lword;	// overflows every 71.111111ms*0xff == 18.204444s
							// resolution == 277.77777us
	uint16_t tcnt2_hword;	// overflows every 1193028.266666s, 331.396h, 13.808days
							// resolution == 18.204444s
							
	uint8_t tcnt2_vhbyte;	// this one ain't overflowing for the duration of the batteries
} watch_t;

watch_t watch = {0, 0, 0};

uint16_t get_thword(void)
{
	return watch.tcnt2_hword;
}

uint16_t get_tlword(void)
{
	return watch.tcnt2_lword;
}

uint16_t get_seconds(void)
{
	uint16_t ret_val = watch.tcnt2_lword / 3600;
	uint32_t rl_hi = (watch.tcnt2_hword * 91);
	rl_hi /= 5;		// * 91 / 5  is identical to * 18.2
	return ret_val + rl_hi;
}

void init_sleep(void)
{
	// we need to wake up quick (an internal 1MHz RC would probably be a better option
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	// config the wake-up timer
	TCCR2A = _BV(WGM21) | _BV(WGM20)	// CTC mode
				| _BV(CS22) | _BV(CS21) | _BV(CS20);	// 1024 prescaler
														// TCNT=277.78us  OVF=71.11ms

	TIMSK2 = _BV(TOIE2);	// interrupt on overflow
}

void add_ticks(uint16_t ticks)
{
	// we assume the error on average is half a tick
	// so add a tick every other call to account for this
	static uint8_t correction = 0;
	ticks += correction;
	correction = correction ? 0 : 1;

	// mind the overflow
	if (ticks > 0xffff - watch.tcnt2_lword)
	{
		if (watch.tcnt2_hword == 0xffff)
			++watch.tcnt2_vhbyte;

		++watch.tcnt2_hword;

		//TogBit(PORTE, 1);
	}
	
	watch.tcnt2_lword += ticks;
}

// wake from sleep interrupt
ISR(TIMER2_OVF_vect)
{}

// sleep for sleep_ticks number of TCNT2 ticks
void sleep_ticks(uint8_t ticks)
{
//SetBit(PORTE, 0);
	if (are_leds_on())
	{
		uint8_t prev_ticks = TCNT2;
		
		while (ticks--)
			_delay_us(277.78);
			
		add_ticks(ticks - prev_ticks);
	} else {
		sleep_enable();
		add_ticks(TCNT2);
		TCNT2 = 0xff - ticks;		// set the sleep duration
		add_ticks(ticks);
		sleep_mode();				// go to sleep
		sleep_disable();
	}
//ClrBit(PORTE, 0);
}

uint8_t dyn_sleep_ticks = 0;
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
