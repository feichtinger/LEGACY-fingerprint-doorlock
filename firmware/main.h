
#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <board.h>

/* common constants */

#define FP_USART AVR32_USART0			// this USART is used for Fingerprintsensor
#define PC_USART AVR32_USART1			// this USART is used for PC

#define F_PBA	48000000				// PBA bus frequency

#define DOOR_OPEN_TIME			3000	// ms, time for which the door is unlocked after detecting matching finger
#define ACCESS_DENIED_TIMEOUT	1000	// ms, time to wait after "Access denied"
#define WDA_TIME				12		// hours, maximum time for whole day access


/* macros */

#define KICK_DOG	AVR32_WDT.clr=0			// 'kick' watchdog timer, only call this in main loop

#define WDA_BUTTON_PRESSED	(!(AVR32_GPIO.port[1].pvr&(1<<2)))	// true if whole-day-access button is pressed

#define GREEN_LED_ON		LED_Off(LED0)	// LED0 and LED1 are inverted
#define GREEN_LED_OFF		LED_On(LED0)
#define RED_LED_ON			LED_Off(LED1)
#define RED_LED_OFF			LED_On(LED1)
#define DCF_LED_ON			LED_On(LED2)
#define DCF_LED_OFF			LED_Off(LED2)

#define LOCK				AVR32_PWM.channel[6].cdty = 0			// magnet power off -> door closed
#define UNLOCK				AVR32_PWM.channel[6].cdty = 1000		// full voltage, only use a few seconds to release magnet
#define KEEP_UNLOCKED		AVR32_PWM.channel[6].cdty = 500			// reduced voltage to prevent magnet for overload, use after UNLOCK for whole day access


/* public functions */

void start_enroll(char *name, uint16_t openTime);
void start_delete(char *name);
void start_delete_all(void);
void start_restore(char* name);
void abort_op();

void wait_ms(uint32_t time);
void writeLogEntry(const char* format, ...);


#endif
