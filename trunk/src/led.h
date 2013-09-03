#pragma once

// initializes the Timer0 interrupt
void init_leds(void);

// initializes the Timer0 used for LED PWM
// and turns on the selected LEDs
void set_leds(uint8_t new_led_status, uint8_t num_cycles);

// returns true if the PWM counter is running
bool are_leds_on(void);


typedef struct 
{
	uint8_t		led_status;
	uint8_t		num_cycles;		// 0 means stop sequence
	uint8_t		brightness;
	int8_t		pwm_delta;
} led_sequence_t;

// seq must be a flash pointer
void start_led_sequence(const __flash led_sequence_t* seq);

// LED sequences
extern const __flash led_sequence_t led_seq_boot[];
extern const __flash led_sequence_t led_seq_pulse_on[];
extern const __flash led_sequence_t led_seq_pulse_off[];
