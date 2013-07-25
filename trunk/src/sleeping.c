#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "sleeping.h"

uint8_t curr_prescaler = SLEEP_PRESCALER_256;

// wake from sleep interrupt
ISR(TIMER2_OVF_vect)
{}

uint8_t set_sleep_prescaler(const uint8_t prescaler)
{
	uint8_t ret_val;
	uint8_t prescaler_bits;

	// get prescaler bits and the sleep cycle duration
	if (prescaler == SLEEP_PRESCALER_64)			// TCNT= 17.36us  OVF= 4.44ms
	{
		prescaler_bits = _BV(CS22);
		ret_val = 4;
	} else if (prescaler == SLEEP_PRESCALER_128) {	// TCNT= 34.72us  OVF= 8.89ms
		prescaler_bits = _BV(CS22) | _BV(CS20);
		ret_val = 9;
	} else if (prescaler == SLEEP_PRESCALER_256) {	// TCNT= 69.44us  OVF=17.78ms
		prescaler_bits = _BV(CS22) | _BV(CS21);
		ret_val = 18;
	} else {/*if (prescaler == 1024)*/		// TCNT=277.78us  OVF=71.11ms
		prescaler_bits = _BV(CS22) | _BV(CS21) | _BV(CS20);
		ret_val = 71;
	}

	// config the wake-up timer
	TCCR2A = _BV(WGM21) | _BV(WGM20)	// CTC mode
				| prescaler_bits;

	// remember the setting
	curr_prescaler = prescaler;
	
	return ret_val;
}

// returns the rounded duration of a sleep cycle in milliseconds
void init_sleep(void)
{
	set_sleep_prescaler(SLEEP_PRESCALER_256);

	TIMSK2 = _BV(TOIE2);	// interrupt on overflow
}

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

void goto_sleep_short(uint8_t sleep_cnt)
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
