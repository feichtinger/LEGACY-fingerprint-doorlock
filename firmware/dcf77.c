
#include <stdbool.h>
#include <time.h>

#include <avr32/io.h>
#include <intc.h>
#include <board.h>

#include "main.h"
#include "dcf77.h"

/*
 * DCF77 receiver, get the current time (CET/CEST) from DCF77 signal
 * How it works:
 * The DCF77 hardware already gives the demodulated digital signal, this is connected to the external interrupt.
 * The 'dcf timer' is used to measure the length of the pulses, 100ms means '0', 200ms means '1'.
 * At the start of a minute there is a missing pulse which is used to synchronize the data stream.
 * The 59 databits are stored in a uint64_t bitarray and get decoded at the end of every minute.
 * 
 * To reduce the probapility of a false time signal we use the parity bits contained in the signal, a plausabiltity check
 * and the redundancy between following data packets. If all checks are OK we finally take the received time as the true one.
 * 
 * A 'backup timer' is used to bypass phases with weak or no DCF-signal for maximal 1 day
 * 
 */



#define TIC_TIME_MS			50	// dcf timer period current_time
#define TIME_THRESHOLD		150 // threshold between bits 0/1

#define TICS_PER_SEC		10	// tics of backup timer per second


volatile uint64_t bitarray=0;			// this is where the DCF 77 data is stored
volatile uint8_t bitindex=0;			// index for bitarray
volatile uint16_t time_ms=0;			// time since last timer start
volatile bool time_valid=false;			// true if DCF77 was received or backup timer is still valid (one day)

volatile time_t current_time;			// the current valid time
volatile time_t last_valid_time;		// the last valid time received from DCF77
volatile time_t custom_time;			// time of custom timer


/*
 * private functions
 */
void decode();
bool checkParity(uint64_t bits, uint8_t i1, uint8_t i2);



/*
 * handler for external interupt (DCF77 pin)
 */
__attribute__((__interrupt__)) static void ext_int_handler(void)
{
	if(AVR32_EIC.edge & (1<<AVR32_EIC_EDGE_INT7))
	{
		// rising edge
		if(time_ms>=800)			// ignore glitches
		{
			DCF_LED_ON;
			if(time_ms>=1900)		// this was a long pause -> start of minute
			{
				bitindex=0;
				decode();			// start decoding bitarray
			}
			
			AVR32_EIC.edge&= ~(1<<AVR32_EIC_EDGE_INT7);				// set EIC to falling edge
			
			AVR32_TC.channel[0].ccr = (1<<AVR32_TC_CCR0_SWTRG);		// reset timer
			time_ms=0;
		}
	}
	else
	{
		// falling edge
		if(time_ms>=TIC_TIME_MS)	// ignore glitches
		{
			
			DCF_LED_OFF;
			AVR32_EIC.edge=(1<<AVR32_EIC_EDGE_INT7);				// set EIC to rising edge
			
			if(time_ms>=TIME_THRESHOLD)
			{
				// this bit is 1
				bitarray |= (((uint64_t)1)<<bitindex);
			}
			else
			{
				// this bit is 0
				bitarray &= ~(((uint64_t)1)<<bitindex);
			}
			
			bitindex++;
		}
	}
	
	AVR32_EIC.icr = (1<<AVR32_EIC_ICR_INT7);			// clear the interrupt flag
}


/*
 * handler for dcf timer interupt
 */
__attribute__((__interrupt__)) static void dcf_tim_int_handler(void)
{
	if(AVR32_TC.channel[0].sr && (1<<AVR32_TC_SR0_CPCS))
	{
		time_ms+=TIC_TIME_MS;
	}
}


/*
 * handler for backup timer interupt
 */
__attribute__((__interrupt__)) static void back_tim_int_handler(void)
{
	static uint8_t tics=0;
	if(AVR32_TC.channel[1].sr && (1<<AVR32_TC_SR1_CPCS))
	{
		tics++;
		if(tics==TICS_PER_SEC)
		{
			tics=0;
			current_time++;
			
			if(current_time - last_valid_time > 60*60*24)
			{
				// more than one day without DCF77 signal
				writeLogEntry("time value is invalid (no DCF77 signal since one day)");
				time_valid=false;
			}
			
			custom_time++;
		}
	}
}



/*
 * initialize DCF77
 */
void dcf_init()
{
	// init external interupt for DCF77 (PB3 -> EXTINT7)
	AVR32_GPIO.port[1].gperc = (1<<3);
	AVR32_GPIO.port[1].pmr0s = (0<<3);
	AVR32_GPIO.port[1].pmr1s = (0<<3);
	
	AVR32_EIC.ier		= (1<<AVR32_EIC_IER_INT7);		// enable channel 7
	AVR32_EIC.mode		= (0<<AVR32_EIC_MODE_INT7);		// trigger mode: edge
	AVR32_EIC.edge		= (1<<AVR32_EIC_EDGE_INT7);		// rising edge
	AVR32_EIC.filter	= (1<<AVR32_EIC_FILTER_INT7);	// enable anti glitch filter
	
	INTC_register_interrupt(&ext_int_handler, AVR32_EIC_IRQ_7, AVR32_INTC_INT0);	// register interrupt handler
	
	AVR32_EIC.en		= (1<<AVR32_EIC_EN_INT7);		// finally enable external interrupt
	
	
	
	// init Timer 0 (for DCF 77 decoding)
	AVR32_TC.channel[0].cmr = (1<<AVR32_TC_CMR0_CPCTRG) | (4<<AVR32_TC_CMR0_TCCLKS);	// reset at compare C, clock = PBA/128
	AVR32_TC.channel[0].rc  = (TIC_TIME_MS*(F_PBA/1000)/128);							// set period current_time (TIC_TIME_MS)
	AVR32_TC.channel[0].ier = (1<<AVR32_TC_IER0_CPCS);									// enable compare C interrupt
	AVR32_TC.channel[0].ccr = (1<<AVR32_TC_CCR0_CLKEN);									// enable clock
	
	INTC_register_interrupt(&dcf_tim_int_handler, AVR32_TC_IRQ0, AVR32_INTC_INT0);		// register interrupt handler
	
	AVR32_TC.channel[0].ccr = (1<<AVR32_TC_CCR0_SWTRG);									// start timer
	
	
	// init Timer 1 (for backup timer)
	AVR32_TC.channel[1].cmr = (1<<AVR32_TC_CMR0_CPCTRG) | (4<<AVR32_TC_CMR0_TCCLKS);	// reset at compare C, clock = PBA/128
	AVR32_TC.channel[1].rc  = F_PBA/(TICS_PER_SEC*128);									// set period current_time
	AVR32_TC.channel[1].ier = (1<<AVR32_TC_IER0_CPCS);									// enable compare C interrupt
	AVR32_TC.channel[1].ccr = (1<<AVR32_TC_CCR0_CLKEN);									// enable clock
	
	INTC_register_interrupt(&back_tim_int_handler, AVR32_TC_IRQ1, AVR32_INTC_INT0);		// register interrupt handler
	
	AVR32_TC.channel[1].ccr = (1<<AVR32_TC_CCR0_SWTRG);									// start timer
	
	
	// init time
	current_time=0;
	last_valid_time=current_time;
	time_valid=false;
}


/*
 * decode bitarray, check parity and plausability
 */
void decode()
{
	bool valid=true;
	
	if(!((bitarray>>20) & 1))		// Bit 20 must be 1
	{
		valid=false;
	}
	
	// minutes
	uint8_t min1 = (bitarray>>21) & 0b1111;
	uint8_t min10 = (bitarray>>25) & 0b111;
	uint8_t min = 10*min10 + min1;
	if(!(checkParity(bitarray, 21, 28) && min<60))
	{
		valid=false;
	}
	
	// hours
	uint8_t hour1 = (bitarray>>29) & 0b1111;
	uint8_t hour10 = (bitarray>>33) & 0b11;
	uint8_t hour = 10*hour10 + hour1;
	if(!(checkParity(bitarray, 29, 35) && hour<24))
	{
		valid=false;
	}
	
	// date
	uint8_t day1 = (bitarray>>36) & 0b1111;
	uint8_t day10 = (bitarray>>40) & 0b11;
	uint8_t day = 10*day10 + day1;
	
	uint8_t mon1 = (bitarray>>45) & 0b1111;
	uint8_t mon10 = (bitarray>>49) & 0b1;
	uint8_t mon = 10*mon10 + mon1;
	
	uint8_t year1 = (bitarray>>50) & 0b1111;
	uint8_t year10 = (bitarray>>54) & 0b1111;
	uint16_t year = 10*(uint16_t)year10 + year1 + 2000;
	if(!(checkParity(bitarray, 36, 58) && day<=31 && mon<=12 && year>=2014))
	{
		valid=false;
	}
	
	struct tm dcf_struct;
	dcf_struct.tm_sec = 0;					// the decoding always happens at sec==0
	dcf_struct.tm_min = min;
	dcf_struct.tm_hour = hour;
	dcf_struct.tm_mday = day;
	dcf_struct.tm_mon = mon - 1;			// tm_mon is 0...11
	dcf_struct.tm_year = year - 1900;		// tm_year is years since 1900
	
	
	// compare with last received signal
	time_t dcf_time=mktime(&dcf_struct);
	static time_t last_dcf_time;
	
	if(dcf_time!=last_dcf_time+60)
	{
		valid=false;
	}
	
	last_dcf_time=dcf_time;
	
	
	static uint8_t dcf_valid_count=0;		// number of DCF signals correctly received in series
	
	if(valid)
	{
		dcf_valid_count++;
		if(dcf_valid_count>=3)
		{
			// we received 3 correct DCF signals in series so we take this as the true current_time
			DCF_LED_ON;
			dcf_valid_count=3;
			current_time=dcf_time;
			last_valid_time=current_time;
			
			if(!time_valid)
			{
				writeLogEntry("time is valid (DCF signal received)");
			}
			time_valid=true;
		}
	}
	else
	{
		DCF_LED_OFF;
		dcf_valid_count=0;
		// get current_time from backup timer
	}
}

/*
 * check even parity of given bits in interval i1 to i2
 */
bool checkParity(uint64_t bits, uint8_t i1, uint8_t i2)
{
	bool parity=true;
	
	for(int i=i1; i<=i2; i++)
	{
		if((bits>>i) & 1)		// check if bitindex i is 1
		{
			parity=!parity;		// toggle parity
		}
	}
	
	return parity;
}


/*
 * return current current_time
 */
time_t dcf_getTime()
{
	return current_time;
}


/*
 * return if current current_time is valid (for example if there is no DCF77 signal for some current_time)
 */
bool dcf_isTimeValid()
{
	return time_valid;
}


void dcf_startCustomTimer()
{
	custom_time=0;
}


time_t dcf_getCustomTimer()
{
	return custom_time;
}
