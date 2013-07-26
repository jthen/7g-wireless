#include <stdbool.h>
#include <stdio.h>

#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "sleeping.h"
#include "matrix.h"
#include "led.h"
#include "avr_serial.h"

// sleep one TCNT2 overflow cycle
void sleep_ovf()
{
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	sleep_enable();
	TCNT2 = 0;			// init the wake-up counter
	sleep_mode();		// go to sleep
	sleep_disable();
}

typedef struct
{
	uint8_t		prescaler_mask;	// the prescaler bits for the TCCR2A register
	uint16_t	tcnt_dur_10ns;	// unit is ten nano sec
	uint16_t	ovf_dur_10us;	// unit is ten micro sec
	uint16_t	wait_s;			// how long to wait in this state before increasing prescaler
} prescalers_t;

const __flash prescalers_t prescalers[4] = 
{
	{ _BV(CS22),                          1736,  444,   2},	// TCNT= 17.36us  OVF= 4.44ms
	{ _BV(CS22) | _BV(CS20),              3472,  889,   4},	// TCNT= 34.72us  OVF= 8.89ms  
	{ _BV(CS22) | _BV(CS21),              6944, 1778, 300},	// TCNT= 69.44us  OVF=17.78ms
	{ _BV(CS22) | _BV(CS21) | _BV(CS20), 27778, 7111,   0},	// TCNT=277.78us  OVF=71.11ms
};

#define MIN_PRESCALER_NDX		1
#define MAX_PRESCALER_NDX		3

static uint16_t sleep_counter = 0;
static uint8_t prescaler_ndx = MAX_PRESCALER_NDX;
	
void set_sleep_prescaler(void)
{
	// config the wake-up timer
	TCCR2A = _BV(WGM21) | _BV(WGM20)	// CTC mode
				| prescalers[prescaler_ndx].prescaler_mask;
}

void init_sleep(void)
{
	set_sleep_prescaler();

	TIMSK2 = _BV(TOIE2);	// interrupt on overflow
}

void sleep_dynamic(void)
{
	uint32_t time_in_prescaler_ms = 
			((uint32_t) sleep_counter * prescalers[prescaler_ndx].ovf_dur_10us) / 100;

	//if (prescaler_ndx >= 2)
		dprinti((uint16_t) time_in_prescaler_ms);

	// if we've waited enough in this prescaler
	if (time_in_prescaler_ms > prescalers[prescaler_ndx].wait_s*1000
			&&  prescaler_ndx < MAX_PRESCALER_NDX)
	{
		++prescaler_ndx;
		set_sleep_prescaler();
		sleep_counter = 0;
	}

	sleep_ovf();

	// mind the overfow
	if (sleep_counter < 0xffff)
		++sleep_counter;
}

void sleep_reset(void)
{
	sleep_counter = 0;
	prescaler_ndx = MAX_PRESCALER_NDX;
	set_sleep_prescaler();
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
	{
		sleep_dynamic();
	}
}

// wake from sleep interrupt
ISR(TIMER2_OVF_vect)
{}

// returns the rounded duration of a sleep cycle in milliseconds
void goto_sleep(uint8_t num_cycles)
{
	// set the sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	
	sleep_enable();

	// init the wake-up counter
	TCNT2 = 0;
	
	// go to sleep for num_cycles
	while (num_cycles--)
		sleep_mode();

	// datasheet recommends this
	sleep_disable();
}

/*
void sleep_exact(uint8_t sleep_cnt)
{
	// remember the prescaler
	uint8_t save_TCCR2A = TCCR2A;
	
	// set the 64 prescaler
	TCCR2A = _BV(WGM21) | _BV(WGM20)	// CTC mode
				| _BV(CS22);			// prescaler 64
	
	// set the sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	
	sleep_enable();

	// set the duration of sleep
	TCNT2 = -sleep_cnt;
	
	// now go to sleep
	sleep_mode();

	// datasheet recommends this
	sleep_disable();
	
	// restore the old prescaler
	TCCR2A = save_TCCR2A;
}
*/