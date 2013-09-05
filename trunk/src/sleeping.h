#pragma once

// inits the Timer0 as an async timer clocked by a 32KHz crystal on TOSC1/2
void init_sleep(void);

// sleep for for a variable amount of time
// the time to sleep depends on time since last matrix change
void sleep_dynamic(void);

// sleep for the number of Timer0 counter cycles
void sleep_ticks(uint8_t sleep_cnt);

void wait_for_all_keys_up(void);
void wait_for_key_down(void);
void wait_for_matrix_change(void);

uint16_t get_seconds(void);

// these return the number of seconds since reset (with overflow)
uint32_t get_seconds32(void);
uint16_t get_seconds(void);

// returns the time since reset
void get_time(uint16_t* days, uint8_t* hours, uint8_t* minutes, uint8_t* seconds);

// used to setup sleep schedule
typedef struct 
{
	uint16_t	duration_sec;	// the duration of this period in the schedule
	uint8_t		num_ticks;		// number of ticks of sleep between refreshes
} sleep_schedule_period_t;
