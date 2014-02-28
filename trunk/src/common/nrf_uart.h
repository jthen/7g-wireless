#pragma once

void uartInit(void);
void uartPoll(void);
uint8_t uartCapacity(void);
uint8_t uartSize(void);
uint8_t uartFree(void);
void uartPush(char c);
void uartPushInt(uint8_t i);
char uartPop(void);
char uartPeek(void);
bool uartFull(void);
bool uartEmpty(void);
