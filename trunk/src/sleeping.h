#pragma once

enum timer2_prescaler {

SLEEP_PRESCALER_64		= _BV(CS22),
SLEEP_PRESCALER_128		= _BV(CS22) | _BV(CS20),
SLEEP_PRESCALER_256		= _BV(CS22) | _BV(CS21),
SLEEP_PRESCALER_1024	= _BV(CS22) | _BV(CS21) | _BV(CS20),

};


// sets the default prescaler and inits the hardware
void init_sleep(void);

// sleep for for a variable amount of time
// the time to sleep depends on time since last matrix change
void sleep_dynamic(void);

// sleep for the number of Timer0 counter cycles with a 64 prescaler
// This will be sleep_cnt*17.36us on a 3.6864MHz F_CPU
void sleep_ticks(uint8_t sleep_cnt);

void wait_for_all_keys_up(void);
void wait_for_key_down(void);
void wait_for_matrix_change(void);

uint16_t get_seconds(void);

uint16_t get_thword(void);
uint16_t get_tlword(void);
