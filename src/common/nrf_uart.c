#include <stdint.h>
#include <stdbool.h>

#ifdef NRF24LU1
# include "reg24lu1.h"
#else
# include "reg24le1.h"
#endif

#include "nrf_uart.h"

#define UART_MSG_BUFF_SIZE	512

__xdata char uart_msg_buff[UART_MSG_BUFF_SIZE];
uint8_t uart_buff_head = 0;
uint8_t uart_buff_tail = 0;

#define BAUD_57K6   1015  // = Round(1024 - (2*16e6)/(64*57600))
#define BAUD_38K4   1011  // = Round(1024 - (2*16e6)/(64*38400))
#define BAUD_19K2    998  // = Round(1024 - (2*16e6)/(64*19200))
#define BAUD_9K6     972  // = Round(1024 - (2*16e6)/(64*9600))

void uartInit(void)
{
	uint16_t temp;

	ES0 = 0;			// Disable UART0 interrupt while initializing
	REN0 = 1;			// Enable receiver
	SM0 = 0;			// Mode 1..
	SM1 = 1;			// ..8 bit variable baud rate
	PCON |= 0x80; 		// SMOD = 1
#ifdef NRF24LU1	
	WDCON |= 0x80;		// Select internal baud rate generator
#else
	ADCON |= 0x80;		// Select internal baud rate generator
#endif
	temp = BAUD_57K6;
	//temp = BAUD_38K4;
	//temp = BAUD_9K6;
	//temp = BAUD_19K2;
	S0RELL = (uint8_t)temp;
	S0RELH = (uint8_t)(temp >> 8);

#ifdef NRF24LU1	
	P0ALT |= 0x04;		// Select alternate functions on P02
	P0EXP &= 0xfc;		// Select TxD on P02
#else
#endif	

	//P0DIR |= 0x00;	// P01 (RxD) is input

	TI0 = 1;
	//ES0 = 1;			// Enable UART0 interrupt
}

void putchar(char c)
{
	uartPush(c);
}

void uartPoll(void)
{
	if (TI0  &&  !uartEmpty())
	{
		while (TI0 == 0)
			;
		TI0 = 0;
		S0BUF = uartPop();
	}
}

uint8_t uartCapacity(void)
{
	return UART_MSG_BUFF_SIZE - 1;
}

uint8_t uartSize(void)
{
	if (uart_buff_head < uart_buff_tail)
		return UART_MSG_BUFF_SIZE + uart_buff_head - uart_buff_tail;
	
	return uart_buff_head - uart_buff_tail;
}

uint8_t uartFree(void)
{
	return UART_MSG_BUFF_SIZE - uartSize() - 1;
}

void uartPush(char c)
{
	uart_msg_buff[uart_buff_head] = c;
	uart_buff_head = (uart_buff_head + 1) % UART_MSG_BUFF_SIZE;
}

void uartPushInt(uint8_t i)
{
	if (i > 99)
	{
		uartPush('0' + (i / 100));
		i %= 100;
	}
	
	if (i > 9)
	{
		uartPush('0' + (i / 10));
		i %= 10;
	}
	
	uartPush('0' + i);
}

char uartPop(void)
{
	char ret_val = uart_msg_buff[uart_buff_tail];
	uart_buff_tail = (uart_buff_tail + 1) % UART_MSG_BUFF_SIZE;

	return ret_val;
}

char uartPeek(void)
{
	char ret_val = uart_msg_buff[uart_buff_tail];
	return ret_val;
}

bool uartFull(void)
{
	return ((uart_buff_head + 1) % UART_MSG_BUFF_SIZE) == uart_buff_tail;
}

bool uartEmpty(void)
{
	return uart_buff_head == uart_buff_tail;
}
