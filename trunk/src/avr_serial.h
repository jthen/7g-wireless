#pragma once

#ifdef DBGPRINT
# define dprintf(...)		printf(__VA_ARGS__)		// gotta love C99, man
# define dprintf_P(...)		printf_P(__VA_ARGS__)
# define dprinti(i)			printi(i)
void printi(const uint32_t i);
#else
# define dprintf(...)
# define dprintf_P(...)
# define dprinti(i)
#endif

void init_serial(void);
