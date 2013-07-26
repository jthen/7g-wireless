#pragma once

// the keyboard matric dimensions
#define	NUM_ROWS		16
#define	NUM_COLS		8

extern uint8_t matrix[NUM_ROWS];

void matrix_init(void);
bool matrix_scan(void);

// returns the keycode of the key at a position on the matrix
uint8_t get_keycode(uint8_t row, uint8_t col);

// checks if the key with the given code is pressed
bool is_pressed(uint8_t keycode);

// checks if any key is pressed
bool is_any_key_pressed(void);
