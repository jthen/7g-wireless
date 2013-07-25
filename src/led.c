#include <stdbool.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "hw_setup.h"
#include "led.h"
#include "utils.h"
#include "ctrl_settings.h"

volatile uint8_t curr_led_status = 0;
volatile uint8_t cycle_counter;		// duration the LEDs are on
const __flash led_sequence_t* sequence = 0;

void turn_on_leds(void)
{
	// turn the needed LEDs on by setting
	// the DDR to output and driving the pin(s) low
	DDRG = curr_led_status;
	PORTG = ~curr_led_status;
	
	// PWM?
	if (sequence)
		OCR0A += sequence->pwm_delta;
}

void turn_off_leds(void)
{
	// turn all the LEDs off
	DDRG = 0x00;	// all inputs
	PORTG = 0xff;	// all pullups
}

// Timer0 interrupts are used for LED brightness with software PWM
ISR(TIMER0_OVF_vect)
{
	turn_on_leds();
}

ISR(TIMER0_COMP_vect)
{
	turn_off_leds();
	
	if (--cycle_counter == 0)
	{
		// reset the PWM duty cycle
		OCR0A = get_led_brightness();

		if (sequence == 0)
		{
			// stop the timer
			TCCR0A = 0;
		} else {
			++sequence;
			
			uint8_t num_cycles = sequence->num_cycles;
			
			if (num_cycles == 0)
			{
				// stop the timer
				TCCR0A = 0;
				
				// stop the sequence iterator
				sequence = 0;
			} else {
				curr_led_status = sequence->led_status & 0x07;
				cycle_counter = num_cycles;
				OCR0A = sequence->brightness;
			}
		}
	}
}

void init_leds(void)
{
	// timer interrupts for compare and overflow
	TIMSK0 = _B1(OCIE0A) | _B1(TOIE0);
}

void set_leds(uint8_t new_led_status, uint8_t num_cycles)
{
	// remember the status
	curr_led_status = new_led_status;

	// if there was a LED sequence stop it
	sequence = 0;

	// init the cycle counter
	cycle_counter = num_cycles;

	// reset the PWM duty cycle
	OCR0A = get_led_brightness();
	
	// is the timer off?
	if (TCCR0A == 0)
	{
		// reset the counter
		TCNT0 = 0;

		turn_on_leds();
		
		// start the timer
		TCCR0A = _B1(WGM00) | _B1(WGM01)				// fast PWM mode
				//| _B0(CS02) | _B1(CS01) | _B1(CS00);	// prescaler 64
				| _B1(CS02) | _B0(CS01) | _B0(CS00);	// prescaler 256  0.6944us TCNT, 17.78 ms cycle
				//| _B1(CS02) | _B0(CS01) | _B1(CS00);	// prescaler 1024
	}
}

void clear_leds(void)
{
	// stop the timer
	TCCR0A = 0;

	turn_off_leds();
}

bool are_leds_on(void)
{
	return TCCR0A != 0;
}

void start_led_sequence(const __flash led_sequence_t* seq)
{
	sequence = seq;
	
	// reset the PWM duty cycle
	OCR0A = get_led_brightness();

	// set the status
	curr_led_status = seq->led_status;

	// init the cycle counter
	cycle_counter = seq->num_cycles;

	// is the timer off?
	if (TCCR0A == 0)
	{
		// reset the counter
		TCNT0 = 0;

		turn_on_leds();
		
		// start the timer
		TCCR0A = _B1(WGM00) | _B1(WGM01)				// fast PWM mode
				//| _B0(CS02) | _B1(CS01) | _B1(CS00);	// prescaler 64
				| _B1(CS02) | _B0(CS01) | _B0(CS00);	// prescaler 256  0.6944us TCNT, 17.78 ms cycle
				//| _B1(CS02) | _B0(CS01) | _B1(CS00);	// prescaler 1024
	}
}

// LED sequences
const __flash led_sequence_t led_seq_succ_and_return[] = 
{
	{1, 20, 1, 0},
	{4, 20, 1, 0},
	{2, 20, 1, 0},
	{4, 20, 1, 0},
	{1, 20, 1, 0},
	
	{0,  0, 0, 0},
};

const __flash led_sequence_t led_seq_pulse[] = 
{
	{7, 50,   1,  2},
	{7, 50, 100, -2},
	{7, 50,   1,  2},
	{7, 50, 100, -2},
	{7, 50,   1,  2},
	{7, 50, 100, -2},
	
	{0,   0, 0},
};