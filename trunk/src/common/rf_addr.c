#include <stdint.h>

#ifdef AVR
# include <avr/pgmspace.h>
# define __FLASH __flash
# define __xdata
#else
# define __FLASH __code
#endif

#include "rf_protocol.h"

__FLASH const uint8_t KeyBrdAddr[] = {0x63, 0x4C, 0x30, 0x10, 0x01};
__FLASH const uint8_t DongleAddr[] = {0x36, 0xC4, 0x31, 0x40, 0x03};
