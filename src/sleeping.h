#pragma once

#define SLEEP_PRESCALER_64		7
#define SLEEP_PRESCALER_128		8
#define SLEEP_PRESCALER_256		9
#define SLEEP_PRESCALER_1024	11

// sets the default prescaler and inits the hardware
void init_sleep(void);

// returns the number of milliseconds one sleep cycle lasts
uint8_t set_sleep_prescaler(const uint8_t prescaler);

// sleep for num_cycles
void goto_sleep(uint8_t num_cycles);

// sleep for the number of Timer0 counter cycles with a 64 prescaler
// This will be sleep_cnt*17.36us on a 3.6864MHz F_CPU
void goto_sleep_short(uint8_t sleep_cnt);
