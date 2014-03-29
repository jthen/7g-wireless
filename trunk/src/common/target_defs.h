#pragma once

#include <stddef.h>

#ifdef AVR
#	include <avr/io.h>
#	include <avr/pgmspace.h>
#	include <util/delay.h>
#	define __FLASH_ATTR __flash
#	define __xdata
#	define LED_on()			SetBit(PORT(LED1_PORT), LED1_BIT)
#	define LED_off()		ClrBit(PORT(LED1_PORT), LED1_BIT)
#	define memcpy_X(dst, src, cnt)	memcpy(dst, src, cnt)
#else
#	define __FLASH_ATTR __code

#	define LED_on()			P03 = 1
#	define LED_off()		P03 = 0

void* memcpy_P(__xdata void* dest, __code const void* src, size_t count);
__xdata void* memcpy_X(__xdata void* dest, __xdata const void* src, size_t count);

#endif
