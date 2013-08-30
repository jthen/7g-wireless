#pragma once

// Calibration methods, Binary search WITH Neighborsearch is default method
// Uncomment to use ONE of the two following methods instead:
//#define CALIBRATION_METHOD_BINARY_WITHOUT_NEIGHBOR
//#define CALIBRATION_METHOD_SIMPLE

// Modify CALIBRATION_FREQUENCY to desired calibration frequency
//#define CALIBRATION_FREQUENCY 1000000
#define CALIBRATION_FREQUENCY 921600

// Frequency of the external oscillator. A 32kHz crystal is recommended
#define XTAL_FREQUENCY 32768
#define EXTERNAL_TICKS 100		// ticks on XTAL. Modify to increase/decrease accuracy

// These values are fixed and used by all calibration methods. Not to be modified.
#define FALSE 0
#define TRUE 1
#define RUNNING 0
#define FINISHED 1
#define DEFAULT_OSCCAL_MASK        0x00  // Lower half and
#define DEFAULT_OSCCAL_MASK_HIGH   0x80  // upper half for devices with splitted OSCCAL register

// These are the device specific defifinitions for all supported parts

#if defined(__AVR_ATmega163__) | defined(__AVR_ATmega323__)
#define ASYNC_TIMER                        AS2
#define NO_PRESCALING                      CS20
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR2
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR2UB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR2UB
#define TIMER_UPDATE_BUSY                  TCN2UB
#define TIMER                              TCNT2
#define OSCCAL_RESOLUTION                  8
#define LOOP_CYCLES                        6
#endif

#if defined(__AVR_ATmega64__) | defined(__AVR_ATmega128__)
#define ASYNC_TIMER                        AS0
#define NO_PRESCALING                      CS00
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR0
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR0UB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR0UB
#define TIMER_UPDATE_BUSY                  TCN0UB
#define TIMER                              TCNT0
#define OSCCAL_RESOLUTION                  8
#define LOOP_CYCLES                        6
#endif

#if defined(__AVR_ATmega8__) | defined(__AVR_ATmega16__) | \
    defined(__AVR_ATmega32__) | defined(__AVR_ATmega8535__)
#define ASYNC_TIMER                        AS2
#define NO_PRESCALING                      CS20
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR2
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR2UB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR2UB
#define TIMER_UPDATE_BUSY                  TCN2UB
#define TIMER                              TCNT2
#define OSCCAL_RESOLUTION                  8
#define LOOP_CYCLES                        6
#endif

#if defined(__AVR_ATmega162__)
#define ASYNC_TIMER                        AS2
#define NO_PRESCALING                      CS20
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR2
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR2UB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR2UB
#define TIMER_UPDATE_BUSY                  TCN2UB
#define TIMER                              TCNT2
#define OSCCAL_RESOLUTION                  7
#define LOOP_CYCLES                        6
#endif

#if defined(__AVR_ATmega169__) | defined(__AVR_ATmega165__) | defined(__AVR_ATmega169P__)
#define ASYNC_TIMER                        AS2
#define NO_PRESCALING                      CS20
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR2A
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR2UB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR2UB
#define TIMER_UPDATE_BUSY                  TCN2UB
#define TIMER                              TCNT2
#define OSCCAL_RESOLUTION                  7
#define LOOP_CYCLES                        7
#ifdef __AVR_ATmega169P__
# define TWO_RANGES
#endif
#endif

#if defined(__AVR_ATmega48__) | defined(__AVR_ATmega88__) | defined(__AVR_ATmega168__)
#define ASYNC_TIMER                        AS2
#define NO_PRESCALING                      CS20
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR2B
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR2AUB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR2AUB
#define TIMER_UPDATE_BUSY                  TCN2UB
#define TIMER                              TCNT2
#define OSCCAL_RESOLUTION                  7
#define LOOP_CYCLES                        7
#define TWO_RANGES
#endif

#if defined(__AVR_ATmega325__) | defined(__AVR_ATmega645__) | \
    defined(__AVR_ATmega3250__) | defined(__AVR_ATmega6450__) | \
    defined(__AVR_ATmega329__) | defined(__AVR_ATmega649__) | \
    defined(__AVR_ATmega3290__) | defined(__AVR_ATmega6490__)
#define ASYNC_TIMER                        AS2
#define NO_PRESCALING                      CS20
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR2A
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR2UB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR2UB
#define TIMER_UPDATE_BUSY                  TCN2UB
#define TIMER                              TCNT2
#define OSCCAL_RESOLUTION                  7
#define LOOP_CYCLES                        7
#define TWO_RANGES
#endif

#if defined(__ATmeg1281__) | defined(__AVR_ATmega1281) | defined(__AVR_ATmega1280) | \
    defined(__AVR_ATmega2560__) | defined(__AVR_ATmega2561__) | defined(__AVR_ATmega640)
#define ASYNC_TIMER                        AS2
#define NO_PRESCALING                      CS20
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR2B
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR2AUB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR2AUB
#define TIMER_UPDATE_BUSY                  TCN2UB
#define TIMER                              TCNT2
#define OSCCAL_RESOLUTION                  7
#define LOOP_CYCLES                        7
#define TWO_RANGES
#endif

#if defined(__AT90CAN128__)
#define ASYNC_TIMER                        AS2
#define NO_PRESCALING                      CS20
#define ASYNC_TIMER_CONTROL_REGISTER       TCCR2A
#define ASYNC_TIMER_CONTROL_UPDATE_BUSY    TCR2UB
#define OUTPUT_COMPARE_UPDATE_BUSY         OCR2UB
#define TIMER_UPDATE_BUSY                  TCN2UB
#define TIMER                              TCNT2
#define OSCCAL_RESOLUTION                  7
#define LOOP_CYCLES                        7
#endif

#define DEFAULT_OSCCAL_HIGH ((1 << (OSCCAL_RESOLUTION - 1)) | DEFAULT_OSCCAL_MASK_HIGH)
#define INITIAL_STEP         (1 << (OSCCAL_RESOLUTION - 2))
#define DEFAULT_OSCCAL      ((1 << (OSCCAL_RESOLUTION - 1)) | DEFAULT_OSCCAL_MASK)

// Functions implemented as macros to avoid function calls
#define PREPARE_CALIBRATION() \
calStep = INITIAL_STEP; \
calibration = RUNNING;

#define COMPUTE_COUNT_VALUE() \
countVal = ((EXTERNAL_TICKS*CALIBRATION_FREQUENCY)/(XTAL_FREQUENCY*LOOP_CYCLES));

// Set up timer to be ASYNCHRONOUS from the CPU clock with a second EXTERNAL 32,768kHz CRYSTAL driving it. No prescaling on asynchronous timer.
#define SETUP_ASYNC_TIMER() \
ASSR |= (1<<ASYNC_TIMER); \
ASYNC_TIMER_CONTROL_REGISTER = (1<<NO_PRESCALING);




// For ATmega64 and ATmega128, 8 nop instructions must be run after a
// change in OSCCAL to ensure stability (See errata in datasheet).
// For all other devices, one nop instruction should be run to let
// the RC oscillator stabilize.
// The NOP() macro takes care of this.
#if defined(__AT90Mega64__) | defined(__ATmega64__) | \
    defined(__AT90Mega128__) | defined(__ATmega128__)

#define NOP() \
_NOP(); \
_NOP(); \
_NOP(); \
_NOP(); \
_NOP(); \
_NOP(); \
_NOP(); \
_NOP();

#else
#define NOP() _NOP()
#endif

// Absolute value macro.
#define ABS(var) (((var) < 0) ? -(var) : (var));
