/*
 * rtc.h
 *
 * Created on: May 12, 2014, Author: Karl Zeilhofer
 * 
 * Changed on 13.3.2015, Friedrich Feichtinger, ported to MCP7940N, using AVR32UC3B
 * 
 */


#include "RTC.h"
#include <twi.h>

#define RTC_SLAVE_ADDRESS 0x6F	// MCP7940N

enum RTC_Registers
{
	// fields listed from MSB to LSB, with leading field-width
	// a '\' means not e.g. 'AM\'
	REG_Seconds=0x00, // 1-CH, 3-10s, 4-1s (explanation: bit 7=Clock halt, bit 6..4=10s-digit, bit 3..0=1s-digit)
	REG_Minutes=0x01, // 1-0, 3-10min, 4-1min
	REG_Hours=0x02, // 1-0, 1-12h/24h\, 1-{AM\/PM or 20h}, 1-10h, 4-1h
	REG_Day=0x03, // 5-0, 3-day (weekday, 1...7)
	REG_Date=0x04, // 2-0, 2-10date, 4-1date (1...31)
	REG_Month=0x05, // 3-0, 1-10month, 4-1month (1...12)
	REG_Year=0x06, // 4-10year, 4-1year (starting from 2000)
	REG_Control=0x07, // 1-out, 1-0, 1-OSF, 1-SQWE, 2-0, 1-RS1, 1-RS0 (see datasheet for details)
};




// TWI-peripheral has to be initialized before calling RTC_Init() !!!
bool RTC_Init(unsigned long pba_hz)
{
	twi_master_enable(&AVR32_TWI);
	twi_disable_interrupt(&AVR32_TWI);
	
	twi_options_t twi_options=
	{
		.pba_hz=pba_hz,
		.speed=100000,
		.chip=0				// not used for master mode
	}
	
	twi_master_init(&AVR32_TWI, &twi_options);
	
	if(twi_probe(&AVR32_TWI, RTC_SLAVE_ADDRESS)==TWI_SUCCESS)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/* Return the current time and put it in *TIMER if TIMER is not NULL.  */
// declared in <time.h>
// reads the time from the RTC-chip via I2C (=TWI)
time_t time(time_t *__timer)
{
	time_t t;
	struct tm ts;
	uint8_t b[7]={0};
	
	twi_package_t twi_package=
	{
		.chip=RTC_SLAVE_ADDRESS,
		.addr={0, 0, 0},
		.addr_length=1,
		.buffer=b,
		.length=7
	}
	
	if(twi_master_read(&AVR32_TWI, &twi_package)!=TWI_SUCCESS)
	{
		writeLogEntry("ERROR: could not read from real-time clock");
		return 0;
	}

	ts.tm_sec =  (((b[REG_Seconds]>>4) & 0x7)*10) +  (b[REG_Seconds]&0xF);
	ts.tm_min =  (((b[REG_Minutes]>>4) & 0x7)*10) +  (b[REG_Minutes]&0xF);
	ts.tm_hour = (((b[REG_Hours]  >>4) & 0x3)*10) +  (b[REG_Hours]  &0xF); // 24h format
	// ts_wday must not be initialized!
	ts.tm_mday = (((b[REG_Date] >>4) & 0x3)*10)  +  (b[REG_Date] &0xF);
	ts.tm_mon =  (((b[REG_Month]>>4) & 0x1)*10)  +  (b[REG_Month]&0xF) -1; // tm_mon=0..11
	ts.tm_year = (int)((((b[REG_Year] >>4) & 0xF)*10)  +  (b[REG_Year] &0xF)) + 2000 -1900;

	t = mktime(&ts);

	if(__timer)
	{
		*__timer = t;
	}
	return t;
}


void RTC_SetDateTime(struct tm* ts)
{
	uint8_t b[7]={0};

	time_t t = mktime(ts); // adjust all entries in this structure (so that all fields are in range)
		// refer to K.N.King p. 694
		// it calculates tm_wday and tm_yday

	// convert broken date to BCD register values:
	b[REG_Seconds] = (ts->tm_sec)/10 << 4 | (ts->tm_sec)%10;
	b[REG_Minutes] = (ts->tm_min)/10 << 4 | (ts->tm_min)%10;
	b[REG_Hours] = (ts->tm_hour)/10 << 4 | (ts->tm_hour)%10; // using 24h-format
	b[REG_Day] = ts->tm_wday + 1; // tm_wday is 0...6 for sunday..saturday
	b[REG_Date] = (ts->tm_mday)/10 << 4 | (ts->tm_mday)%10;
	b[REG_Month] = (ts->tm_mon+1)/10 << 4 | (ts->tm_mon+1)%10;
	b[REG_Year] = (ts->tm_year-100)/10 << 4 | (ts->tm_year-100)%10;
	
	b[REG_Seconds] |= 1<<7;	// start oscillator
	
	twi_package_t twi_package=
	{
		.chip=RTC_SLAVE_ADDRESS,
		.addr={0, 0, 0},
		.addr_length=1,
		.buffer=b,
		.length=7
	}
	
	if(twi_master_write(&AVR32, twi_package)==TWI_SUCCESS)
	{
		writeLogEntry("new time written to real-time clock");
	}
	else
	{
		writeLogEntry("ERROR, could not write new time to real-time clock");
	}
}

// return a pointer to a static allocated string
// this string will be overwritten, on the next call of RTC_TimeStr();
// in the format: 2004-06-14T23:34:30
// if t=0, the current date-time is returned.
char* RTC_DateTimeStr(time_t t)
{
	if(t==0) // then use current time
	{
		t = time(NULL);
	}
	struct tm* ts = localtime(&t); // ATTENTION: use localtime() and not gmtime()
		// because mktime() interprets the time-struct as local time!
	static char str[21];
	snprintf(str, 20, "%d-%02d-%02dT%02d:%02d:%02d", ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday,
			ts->tm_hour, ts->tm_min, ts->tm_sec);
	return str;
}
