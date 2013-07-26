#include <stdio.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/power.h>

#include "avr_serial.h"

#define BAUD 57600
#include <util/setbaud.h>

static int serial_putchar(char c, FILE *stream);
static FILE mystdout = FDEV_SETUP_STREAM(serial_putchar, NULL, _FDEV_SETUP_WRITE);

static int serial_putchar(char c, FILE *stream)
{
    if (c == '\n')
		serial_putchar('\r', stream);

    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    return 0;
}

uint8_t serial_getchar(void)
{
	loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}

void init_serial(void)
{
	power_usart0_enable();		// power to the USART
	
    // USART Baud rate:
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);

    stdout = &mystdout; // Required for printf init
}

#ifdef DBGPRINT

void dprinti(const uint16_t i)
{
	char string_buff[10];
	itoa(i, string_buff, 10);
	puts(string_buff);
}

#endif