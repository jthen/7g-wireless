// - Under processor configuration, select any device listed in the calib_values.h file.
// - Enable bit definitions in I/O include files
// - High optimization on size is recommended on release target for lower code size.
// - Output format: ubrof 8 (forced) for debug and intel-extended for release target.
//
// The included source code is written for all devices with a tunable
// internal oscillator and a timer with asynchronous operation mode. These
// devices are listed in the calib_values.h file.
// Note that when calibrating the ATCAN90 device using the STK501 top module,
// a 32kHz crystal other than the one on the STK501 should be used in order to
// achieve maximum accuracy. Also note that the ATmega169 and ATmega169P has
// different oscillators, and when calibrating the latter, modify the
// ATmega169 device specific defines in the calib_values.h file.
//
// Change CALIBRATION_FREQUENCY to desired frequency.
// Define either CALIBRATION_METHOD_BINARY_WITHOUT_NEIGHBOR or
// CALIBRATION_METHOD_SIMPLE in the calib_values.h file if a other search methods than
// the default search method with neighbor search is desired.
// If a crystal running at a different frequency than 32kHz is used,
// modify XTAL_FREQUENCY accordingly. This is not recommended.

#include <avr/io.h>
#include <avr/cpufunc.h>

#include "calib_values.h"

// Holds the number of neighbors searched
uint8_t neighborsSearched;
// The binary search step size
uint8_t calStep;
// The lowest difference between desired and measured counter value
uint8_t bestCountDiff = 0xFF;
// Stores the lowest difference between desired and measured counter value for the first search
uint8_t bestCountDiff_first;
// The OSCCAL value corresponding to the bestCountDiff
uint8_t bestOSCCAL;
// Stores the OSCCAL value corresponding to the bestCountDiff for the first search
uint8_t bestOSCCAL_first;
// The desired counter value
uint16_t countVal;
// Calibration status
uint16_t calibration;
// Stores the direction of the binary step (-1 or 1)
int8_t sign;

// Computes the count value needed to compare the desired internal oscillator
// speed with the external watch crystal, and sets up the asynchronous timer.
void CalibrationInit(void)
{
	COMPUTE_COUNT_VALUE();		// Computes countVal for use in the calibration
	OSCCAL = DEFAULT_OSCCAL;
	NOP();

	SETUP_ASYNC_TIMER();		// Asynchronous timer setup
}

// This function increments a counter for a given ammount of ticks on
// on the external watch crystal.
uint16_t Counter(void)
{
	uint16_t cnt;

	cnt = 0;							// Reset counter
	TIMER = 0x00;						// Reset async timer/counter
	
	while (ASSR & ((1<<OUTPUT_COMPARE_UPDATE_BUSY)			// Wait until async timer is updated (Async Status reg. busy flags).
						|(1<<TIMER_UPDATE_BUSY)
						|(1<<ASYNC_TIMER_CONTROL_UPDATE_BUSY)));
						
	do {								// cnt++: Increment counter - the add immediate to word (ADIW) takes 2 cycles of code.
		cnt++;							// Devices with async TCNT in I/0 space use 1 cycle reading, 2 for devices with async TCNT in extended I/O space
	} while (TIMER < EXTERNAL_TICKS);	// CPI takes 1 cycle, BRCS takes 2 cycles, resulting in: 2+1(or 2)+1+2=6(or 7) CPU cycles
	
	return cnt;							// NB! Different compilers may give different CPU cycles!
}										// Until 32.7KHz (XTAL FREQUENCY) * EXTERNAL TICKS

// This function uses the binary search method to find the correct OSSCAL value.
void BinarySearch(uint16_t ct)
{
	if (ct > countVal)			// Check if count is larger than desired value
	{
		sign = -1;				// Saves the direction
		OSCCAL -= calStep;		// Decrease OSCCAL if count is too high
		NOP();
	}
	else if (ct < countVal)		// Opposite procedure for lower value
	{
		sign = 1;
		OSCCAL += calStep;
		NOP();
	}
	else						// Perfect match, OSCCAL stays unchanged
	{
		calibration = FINISHED;
	}

	calStep >>= 1;
}

// This function uses the neighbor search method to improve
// binary search result. Will always be called with a binary search
// prior to it.
void NeighborSearch(void)
{
	neighborsSearched++;
	if (neighborsSearched == 4)		// Finish if 3 neighbors searched
	{
		OSCCAL = bestOSCCAL;
		calibration = FINISHED;
	} else {
		OSCCAL += sign;
		NOP();
	}
}

// Performs the calibration according to calibration method chosen.
// Compares different calibration results in order to achieve optimal results.
void CalibrateInternalRc(void)
{
	uint16_t count;

#ifdef CALIBRATION_METHOD_SIMPLE			// Simple search method
	uint8_t cycles = 0x80;

	do {
		count = Counter();
		if (count > countVal)
			OSCCAL--;						// If count is more than count value corresponding to the given frequency:
		NOP();								// - decrease speed
		if (count < countVal)
			OSCCAL++;
		NOP();								// If count is less: - increase speed
		if (count == countVal)
			cycles = 1;			
	} while (--cycles);						// Calibrate using 128(0x80) calibration cycles

#else										// Binary search with or without neighbor search
	uint8_t countDiff;
	uint8_t neighborSearchStatus = FINISHED;

	while (calibration == RUNNING)
	{
		count = Counter();					// Counter returns the count value after external ticks on XTAL
		if (calStep != 0)
		{
			BinarySearch(count);			// Do binary search until stepsize is zero
		}
		else
		{
			if (neighborSearchStatus == RUNNING)
			{
				countDiff = ABS((int16_t)count-(int16_t)countVal);
				if (countDiff < bestCountDiff)		// Store OSCCAL if higher accuracy is achieved
				{
					bestCountDiff = countDiff;
					bestOSCCAL = OSCCAL;
				}
				NeighborSearch();					// Do neighbor search
			}
			else									// Prepare and start neighbor search
			{
#ifdef CALIBRATION_METHOD_BINARY_WITHOUT_NEIGHBOR	// No neighbor search if deselected
				calibration = FINISHED;
				countDiff = ABS((int16_t)count - (int16_t)countVal);
				bestCountDiff = countDiff;
				bestOSCCAL = OSCCAL;
#else
				neighborSearchStatus = RUNNING;		// Do neighbor search by default
				neighborsSearched = 0;
				countDiff = ABS((int16_t)count - (int16_t)countVal);
				bestCountDiff = countDiff;
				bestOSCCAL = OSCCAL;
#endif
			}
		}
	}
#endif
}

// initializes all subsystems, prepares and starts calibration
// according to the calibration method chosen, and the device specific
// oscillator characteristics. Ends in an eternal loop.

void CalibrateRC(void)
{
	CalibrationInit();							// Initiates calibration
	PREPARE_CALIBRATION();						// Sets initial stepsize and sets calibration state to "running"
	CalibrateInternalRc();						// Calibrates to selected frequency

#ifndef CALIBRATION_METHOD_SIMPLE				// If simple search method is chosen, there is no need to do two calibrations.
#ifdef TWO_RANGES								// For devices with splitted OSCCAL register.
	if (bestCountDiff != 0x00)					// Do not do a second search if perfect match
	{
		OSCCAL = DEFAULT_OSCCAL_HIGH;			// Sets search range to upper part of OSCCAL
		NOP();
		bestOSCCAL_first = bestOSCCAL;			// Save OSCCAL value and count difference achieved in first calibration
		bestCountDiff_first = bestCountDiff;
		PREPARE_CALIBRATION();					// Search performed in lower OSCCAL range, perform search in upper OSCCAl range
		CalibrateInternalRc();					// Perform a second search in upper part of OSCCAL

		if (bestCountDiff > bestCountDiff_first)	// Check which search gave the best calibration
		{
			OSCCAL = bestOSCCAL_first;			// First calibration is more accurate and OSCCAL is written accordingly
			NOP();
		}
	}
#endif
#endif
}


// this was used to calibrate the RC oscillator
void test_clock(void)
{
	// make a pause to know there to put the logic analyzer marker
	PORTE = 0;
	_delay_us(15);
	PORTE = 1;

	// each line takes 10 clock cycles
	// so from the first to the last low-to-hi is exactly 100 cycles
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
	PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1; PORTE = 0; PORTE = 1;
}
