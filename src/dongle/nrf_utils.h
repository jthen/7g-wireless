#pragma once

#include <stdint.h>

void delay_us(uint16_t us);
void delay_ms(uint16_t ms);

#define TogP(p)		p = p ? 0 : 1