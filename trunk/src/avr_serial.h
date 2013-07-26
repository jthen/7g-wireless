#pragma once

#ifdef DBGPRINT
# define dprintf(...)		printf(__VA_ARGS__)
# define dprintf_P(...)		printf_P(__VA_ARGS__)
void dprinti(const uint16_t i);
#else
# define dprintf(...)
# define dprintf_P(...)
# define dprinti(i)
#endif

void init_serial(void);
uint8_t serial_getchar(void);
