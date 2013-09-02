#include <stdbool.h>
#include <stdio.h>

#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "sleeping.h"
#include "matrix.h"
#include "led.h"
#include "utils.h"

// this is our watch
// not 100% accurate, but serves the purpose
// it's updated only from add_ricks() and read from get_seconds()
struct 
{
	uint16_t tcnt2_lword;	// overflows every 62.5ms * 256 == 16s
							// resolution == 244.140625us

	uint16_t tcnt2_hword;	// overflows every 1048576s, 291.27111h, 12.13629 days
							// resolution == 16s
							
	uint8_t tcnt2_vhbyte;	// overflows every 3106,89185 days
							// resolution = 12.13629 days
} watch = {0, 0, 0};

uint32_t get_seconds32(void)
{
	uint32_t ret_val = watch.tcnt2_vhbyte;
	ret_val <<= 16;
	ret_val |= watch.tcnt2_hword;
	ret_val <<= 4;
	ret_val |= (watch.tcnt2_lword >> 12);

	return ret_val;
}

uint16_t get_seconds(void)
{
	return (uint16_t) get_seconds32();
}

void get_time(uint16_t* days, uint8_t* hours, uint8_t* minutes, uint8_t* seconds)
{
	uint32_t sec = get_seconds32();
	*seconds = sec % 60;
	sec /= 60;
	*minutes = sec % 60;
	sec /= 60;
	*hours = sec % 24;
	sec /= 24;
	*days = sec;
}

void init_sleep(void)
{
	// the AVR draws about 6uA in power save sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);

	// config the wake-up timer; the timer is set to normal mode
	TCCR2A = 	//_BV(CS20);				// no prescaler
											// TCNT=30.517578125us  OVF=7.8125ms

				_BV(CS21);				// 8 prescaler
											// TCNT=244.140625us    OVF=62.5ms

				//_BV(CS21) | _BV(CS20);	// 32 prescaler
											// TCNT=976.5625us      OVF=250ms

				//_BV(CS22);				// 64 prescaler
											// TCNT=1953.125us      OVF=500ms

				//_BV(CS22) | _BV(CS20);	// 128 prescaler
											// TCNT=3906.25us       OVF=1000ms

				//_BV(CS22) | _BV(CS21);	// 256 prescaler
											// TCNT=7812.5us        OVF=2000ms

				//_BV(CS22) | _BV(CS21) | _BV(CS20);	// 1024 prescaler
														// TCNT=31250us         OVF=8000ms
														
	// we are running Timer2 from the 32kHz crystal - set to async mode
	ASSR = _BV(AS2);

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

// Timer2 overflow interrupt wakes us from sleep
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
			_delay_us(244.14);
			
		add_ticks(ticks - prev_ticks);
	} else {
		sleep_enable();
		add_ticks(TCNT2);
		TCNT2 = 0xff - ticks;		// set the sleep duration
		loop_until_bit_is_clear(ASSR, TCN2UB);
		add_ticks(ticks);
		sleep_mode();				// go to sleep
		sleep_disable();
	}
//ClrBit(PORTE, 0);
}

uint8_t dyn_sleep_ticks = 0;
uint8_t sleep_ticks_prescaler = 0;

#define MIN_SLEEP_TICKS		0x01
#define MAX_SLEEP_TICKS		0x10

void sleep_dynamic(void)
{
	++sleep_ticks_prescaler;
	
	if (sleep_ticks_prescaler == 4)
	{
		sleep_ticks_prescaler = 0;
		if (dyn_sleep_ticks < MAX_SLEEP_TICKS)
			++dyn_sleep_ticks;
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
