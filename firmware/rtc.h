/*
 * rtc.h
 *
 * Created on: May 12, 2014, Author: Karl Zeilhofer
 * 
 * Changed on 13.3.2015, Friedrich Feichtinger, ported to MCP7940N, using AVR32UC3B
 * 
 */

#ifndef RTC_H_
#define RTC_H_

#include <time.h>
#include <twi.h>
#include <stdint.h>
#include <stdbool.h>

bool RTC_Init(unsigned long pba_hz);
// use
// 		time_t time(time_t* __time)
// for getting current time from chip (declared in time.h)

void RTC_SetDateTime(struct tm* time);
char* RTC_DateTimeStr(time_t t);

#endif /* RTC_H_ */
