#pragma once

#ifdef DBGPRINT
# define dprintf(...)		printf(__VA_ARGS__)
# define dprintf_P(...)		printf_P(__VA_ARGS__)
#else
# define dprintf(...)
# define dprintf_P(...)
#endif

void init_serial(void);
uint8_t serial_getchar(void);
