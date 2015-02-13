// generic C
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

// ATMEL software framework
#include <avr32/io.h>
#include "board.h"
#include <intc.h>
#include <pm.h>
#include <usart.h>
#include <gpio.h>
#include <spi.h>
#include <cycle_counter.h>
#include <led.h>

// SD/MMC & FAT
#include <sd_mmc_spi.h>
#include <sd_mmc_spi_mem.h>
#include <fsaccess.h>
#include "conf_sd_mmc_spi.h"

// my modules
#include "main.h"
#include "fingerprint.h"
#include "command_shell.h"
#include "database.h"
#include "dcf77.h"


/*
 * fingerprint doorlock
 * 
 * Fritz Feichtinger, 2014
 * 
 * A ZFM-20 series fingerprint sensor is used to open a door lock (or something else).
 * This sensor does a lot of work out of the box, not only image capturing but also feature extraction,
 * maintaining a fingerprint library, searching and matching. The ZFM-20 uses a serial port to communicate with the MCU.
 * So (nearly) all we have to do is set up a serial port to the sensor and tell it what to do.
 * In NORMAL-mode the sensor is used to scan and match a finger and unlock the door if the detected finger is known.
 * To enroll new fingers we can switch to ENROLL-mode. After a finger was detected we switch back to NORMAL-mode.
 * 
 * The ZFM-20 does NOT store any additional information about the fingerprint, i.e. a name.
 * So we have to store this information (which ID belongs to whos finger) on our own.
 * This makes life a lot easier in the case we want to delete ones access permission one day.
 * Also we can log all activities to a file with usefull information (who entered the building at which time) if we want.
 * 
 * All processes can be controlled by PC connected to a second serial port.
 * 
 */



/* global variables */

// operation modes
volatile enum {NORMAL, ENROLL, DELETE, DELETE_ALL, RESTORE} mode=NORMAL;

// temporary variables used for enroll, delete, restore,...
volatile uint8_t temp_slot=1;
volatile uint16_t temp_id=0;
char *temp_name;
uint16_t temp_openTime=0;

// whole day access
uint16_t currentOpenTime=0;		// current open time for WDA, 0 means no WDA


/* private functions */
void normal_mode();
void enroll_mode();
void delete_mode();
void delete_all_mode();
void restore_mode();

void sd_mmc_resources_init(void);


/*
 * interrupt handler for usart
 * give received characters to command shell
 */
__attribute__((__interrupt__)) static void pc_usart_int_handler(void)
{
	if(usart_overrun_error(&PC_USART) || usart_framing_error(&PC_USART))
	{
		usart_reset_status(&PC_USART);
		return;
	}
	
	int c;
	usart_read_char(&PC_USART, &c);
	
	// cast to 8-bit
	uint8_t ch=c;
	shell_feedChar(ch);
}


/*
 * main routine:
 * initialize hardware and then start main loop
 */
int main(void)
{
	/*
	 * initialize hardware
	 */
	 
	Disable_global_interrupt();
	
	// switch to external quartz oscillator
	pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
	
	// init watchdog (~18.7s)
	AVR32_WDT.ctrl = (0x55<<AVR32_WDT_KEY)|(20<<AVR32_WDT_PSEL)|(1<<AVR32_WDT_EN);
	AVR32_WDT.ctrl = (0xAA<<AVR32_WDT_KEY)|(20<<AVR32_WDT_PSEL)|(1<<AVR32_WDT_EN);
	
	// configure LED0, LED1, LED2
	AVR32_GPIO.port[0].gpers = (1<<7)|(1<<8)|(1<<21);
	AVR32_GPIO.port[0].oders = (1<<7)|(1<<8)|(1<<21);
	AVR32_GPIO.port[0].ovrs  = (1<<21);		// turn off LED2
	
	GREEN_LED_OFF;
	RED_LED_OFF;
	
	
	// init PC USART
	static const gpio_map_t PC_USART_GPIO_MAP =
	{
		{AVR32_USART1_RXD_0_0_PIN, AVR32_USART1_RXD_0_0_FUNCTION},
		{AVR32_USART1_TXD_0_0_PIN, AVR32_USART1_TXD_0_0_FUNCTION}
	};
	gpio_enable_module(PC_USART_GPIO_MAP, 2);

	static const usart_options_t PC_USART_OPTIONS =
	{
		.baudrate     = 115200,
		.charlength   = 8,
		.paritytype   = USART_NO_PARITY,
		.stopbits     = USART_1_STOPBIT,
		.channelmode  = USART_NORMAL_CHMODE
	};
	usart_init_rs232(&PC_USART, &PC_USART_OPTIONS, FOSC0);
	
	
	// init fingerprint USART
	static const gpio_map_t FP_USART_GPIO_MAP =
	{
		{AVR32_USART0_RXD_0_1_PIN, AVR32_USART0_RXD_0_1_FUNCTION},
		{AVR32_USART0_TXD_0_1_PIN, AVR32_USART0_TXD_0_1_FUNCTION}
	};
	gpio_enable_module(FP_USART_GPIO_MAP, 2);
	
	static const usart_options_t FP_USART_OPTIONS =
	{
		.baudrate     = 57600,
		.charlength   = 8,
		.paritytype   = USART_NO_PARITY,
		.stopbits     = USART_1_STOPBIT,
		.channelmode  = USART_NORMAL_CHMODE
	};
	usart_init_rs232(&FP_USART, &FP_USART_OPTIONS, FOSC0);
	

	// init interrupt vectors.
	INTC_init_interrupts();
	INTC_register_interrupt(&pc_usart_int_handler, AVR32_USART1_IRQ, AVR32_INTC_INT0);

	// Enable USART Rx interrupt.
	AVR32_USART1.ier = AVR32_USART_IER_RXRDY_MASK;
	
	// Initialize SD/MMC driver resources: GPIO, SPI and SD/MMC.
	sd_mmc_resources_init();
	
	
	// init PWM for magnet opener (PA22 -> PWM6)
	AVR32_GPIO.port[0].gperc = (1<<22);
	AVR32_GPIO.port[0].pmr0s = (0<<22);
	AVR32_GPIO.port[0].pmr1s = (0<<22);
	
	AVR32_PWM.channel[6].cmr = (1<<AVR32_PWM_CPOL);	// set polarity to high (cdty==0 means output is low)
	AVR32_PWM.channel[6].cprd = 1000;				// f_PWM=12kHz
	AVR32_PWM.channel[6].cdty = 0;					// start with duty=0
	AVR32_PWM.ena = (1<<AVR32_PWM_ENA_CHID6);		// enable PWM
	
	// init DCF77 receiver
	dcf_init();
	
	/*
	 * hardware initialized, start up software
	 */
	
	
	printf("\n\n");
	printf("*** fingerprint doorlock ***\n");
	
	
	// wait for fingerprint-sensor to start up
	wait_ms(500);
	
	
	// init SD-card
	printf("check for SD card...");
	if(sd_mmc_spi_mem_check())
	{
		printf("OK\n");
	}
	else
	{
		printf("ERROR: no SD card detected\n");
		return 0;
	}
	
	printf("check if SD card is ready...");
	if(mem_test_unit_ready(LUN_ID_SD_MMC_SPI_MEM) == CTRL_GOOD)
	{
		printf("OK\n");
	}
	else
	{
		printf("ERROR: SD card not ready\n");
		return 0;
	}
	
	uint32_t cap;
	mem_read_capacity(LUN_ID_SD_MMC_SPI_MEM, &cap);
	printf("SD card capacity: %d MB\n", (int)((cap + 1) >> (20 - FS_SHIFT_B_TO_SECTOR)));
	
	// init filesystem
	b_fsaccess_init();
	
	// read database from SD-card
	if(!db_readFromCard())
	{
		printf("ERROR: could not read file\n");
		return 0;
	}
	
	printf("\nready...\n");
	
	// enable interrupts
	Enable_global_interrupt();
	
	
	// Let the show begin ...
	

	/* main loop */
	while(true)
	{
		KICK_DOG;
		
		// scan command line
		shell_scan();
		
		switch(mode)
		{
			case NORMAL:
			{
				normal_mode();
				break;
			}
			case ENROLL:
			{
				enroll_mode();
				break;
			}
			case DELETE:
			{
				delete_mode();
				break;
			}
			case DELETE_ALL:
			{
				delete_all_mode();
				break;
			}
			case RESTORE:
			{
				restore_mode();
				break;
			}
		}
		
		// check for whole day access timeout
		if(currentOpenTime!=0 && dcf_getCustomTimer()>60*currentOpenTime)
		{
			currentOpenTime=0;
			GREEN_LED_OFF;
			LOCK;
			writeLogEntry("whole day access time out");
		}
	}
}



/****************************************************************************************/
/*							operation modes (main context)								*/
/****************************************************************************************/


/*
 * normal operation (open door for authorized users)
 * 
 * How it works:
 * Try to find a finger, scan it, extract template and compare with database.
 * If anything does not work as expected simply abort execution (and print error), it will get restarted by the main loop.
 */
void normal_mode()
{
	// clear temp data
	temp_slot=1;
	temp_id=0;
	temp_name[0]='\0';
	
	uint8_t confirm;	// use this for confirmation codes
	
	// try to find finger on camera
	confirm=fp_getImage();
	if(confirm!=FINGERPRINT_NOFINGER)
	{
		if(confirm!=FINGERPRINT_OK)
		{
			fp_error(confirm);
			return;
		}
		
		// try to create feature file from image
		confirm=fp_image2Tz(1);
		if(confirm!=FINGERPRINT_OK)
		{
			fp_error(confirm);
			return;
		}
		
		// try to find matching template
		uint16_t id=0;
		uint16_t score=0;
		confirm=fp_search(1, 0, FINGERPRINT_MAXFINGERS, &id, &score);
		if(confirm==FINGERPRINT_OK)
		{
			// found a match!
			if(WDA_BUTTON_PRESSED)	// check for whole day access button
			{
				if(db_getOpenTime(id)>0)
				{
					// user is group leader -> authorized
					if(currentOpenTime!=0)
					{
						// disable whole day access
						currentOpenTime=0;
						GREEN_LED_OFF;
						LOCK;
						printf("whole day access disabled by %s\n", db_getName(id));
						writeLogEntry("whole day access disabled by %s", db_getName(id));
						wait_ms(DOOR_OPEN_TIME);
					}
					else
					{
						// enable whole day access
						currentOpenTime=db_getOpenTime(id);
						GREEN_LED_ON;
						UNLOCK;
						dcf_startCustomTimer();
						printf("whole day access enabled by %s\n", db_getName(id));
						writeLogEntry("whole day access enabled by %s", db_getName(id));
						wait_ms(DOOR_OPEN_TIME);
						KEEP_UNLOCKED;
					}
				}
				else
				{
					// user is normal user -> not authorized to change whole day access
					RED_LED_ON;
					printf("%s is not allowed to change whole day access\n", db_getName(id));
					wait_ms(ACCESS_DENIED_TIMEOUT);
					RED_LED_OFF;
				}
			}
			else if(currentOpenTime==0)
			{
				// normal access when WDA is disabled
				GREEN_LED_ON;
				UNLOCK;
				printf("access granted for %s, score: %d\n", db_getName(id), score);
				writeLogEntry("access granted for %s", db_getName(id));
				wait_ms(DOOR_OPEN_TIME);
				GREEN_LED_OFF;
				LOCK;
			}
			
			return;
		}
		else if(confirm==FINGERPRINT_NOTFOUND)
		{
			// no matching template
			RED_LED_ON;
			printf("Access denied, please try again.\n");
			wait_ms(ACCESS_DENIED_TIMEOUT);
			RED_LED_OFF;
			return;
		}
		else
		{
			fp_error(confirm);
			return;
		}
	}
}


/*
 * enroll new finger and store in database
 * 
 * How it works:
 * Try to scan and featurize 2 fingerprints (in slot 1 and 2), combine them into template file and store in database.
 * The generated template file is uploaded and stored on SD-card, this is for backup only.
 * If anything does not work as expected simply abort execution (and print error), it will get restarted by the main loop. (as long as in ENROLL mode)
 * after success switch back to NORMAL mode.
 */
void enroll_mode()
{
	uint8_t confirm;
	
	// try to find finger on camera
	confirm=fp_getImage();
	if(confirm!=FINGERPRINT_NOFINGER)
	{
		if(confirm!=FINGERPRINT_OK)
		{
			fp_error(confirm);
			return;
		}
		
		// try to create feature file from image
		confirm=fp_image2Tz(temp_slot);
		if(confirm!=FINGERPRINT_OK)
		{
			fp_error(confirm);
			return;
		}
		
		if(temp_slot==1)
		{
			// slot 1 successfull, continue with slot 2
			temp_slot=2;
			return;
		}
		if(temp_slot==2)
		{
			// slot 2 successfull, generate template
			temp_slot=1;
			confirm=fp_createModel();
			if(confirm!=FINGERPRINT_OK)
			{
				fp_error(confirm);
				return;
			}
			
			// store template at next free id
			confirm=fp_storeModel(1, temp_id);
			if(confirm!=FINGERPRINT_OK)
			{
				fp_error(confirm);
				return;
			}
			
			// upload fingerprint template file from sensor
			uint8_t temp_file[FINGERPRINT_TEMPSIZE];
			
			confirm=fp_upChar(1, temp_file);
			if(confirm!=FINGERPRINT_OK)
			{
				fp_error(confirm);
				return;
			}
			
			// finger enrolled, store in database, save template on SD-card
			if(db_addFinger(temp_name, temp_id, temp_openTime, temp_file))
			{
				printf("%s successfully enrolled, id=%d\n", temp_name, temp_id);
			}
			
			//wait_ms(ACCESS_DENIED_TIMEOUT);
			// success, back to normal mode
			mode=NORMAL;
		}
	}
}


/*
 * delete a finger
 */
void delete_mode()
{
	// try to delete template on sensor
	uint8_t confirm;
	confirm=fp_deleteModel(temp_id, 1);
	if(confirm!=FINGERPRINT_OK)
	{
		fp_error(confirm);
		mode=NORMAL;
		return;
	}
	
	// remove entry from database (but not the template file!)
	if(db_delFinger(temp_id))
	{
		printf("finger successfully deleted, id=%d\n", temp_id);
	}
	
	// success, back to normal mode
	mode=NORMAL;
}


/*
 * delete ALL fingers (handle with care)
 */
void delete_all_mode()
{
	uint8_t confirm;
	confirm=fp_emptyDatabase();
	if(confirm!=FINGERPRINT_OK)
	{
		fp_error(confirm);
		mode=NORMAL;
		return;
	}
	
	if(db_deleteAllFingers())
	{
		printf("all fingers deleted successfully\n");
	}
	mode=NORMAL;
}

/*
 * This is for special cases only
 * If the template file on the sensor should get lost you can restore it from SD-card
 */ 
void restore_mode()
{
	uint8_t temp_file[FINGERPRINT_TEMPSIZE];
	uint8_t confirm;
	
	// read template file from SD-card
	if(!db_restoreFinger(temp_name, temp_file))
	{
		mode=NORMAL;
		return;
	}
	
	// download template to sensor
	confirm=fp_downChar(1, temp_file);
	if(confirm!=FINGERPRINT_OK)
	{
		fp_error(confirm);
		mode=NORMAL;
		return;
	}
	
	// store template in sensor database
	confirm=fp_storeModel(1, temp_id);
	if(confirm!=FINGERPRINT_OK)
	{
		fp_error(confirm);
		mode=NORMAL;
		return;
	}
	
	printf("finger successfully restored\n");
	mode=NORMAL;
}


/****************************************************************************************/
/*						PC commands	(command shell context)								*/
/****************************************************************************************/


/*
 * call this to enroll a new finger
 */
void start_enroll(char *name, uint16_t openTime)
{
	// first, check the given string for possible errors
	uint8_t len=strlen(name);
	if(len==0)
	{
		printf("ERROR: expected name for fingerprint!\n");
		return;
	}
	if(len>MAXSTRLEN)
	{
		printf("ERROR: name is too long!\n");
		return;
	}
	if(db_findFinger(name)>=0)
	{
		printf("ERROR: this name already exists, choose a different name\n");
		return;
	}
	
	// check for free space
	int16_t id=db_findFirstFree();
	if(id<0)
	{
		printf("ERROR: finger library is full!\n");
		return;
	}
	
	// check open time
	if(openTime>24*60)
	{
		printf("ERROR: invalid open time\n");
		return;
	}
	
	printf("enrolling finger, place finger on sensor and wait...\n");
	
	mode=ENROLL;
	temp_id=id;
	temp_openTime=openTime;
	strcpy(temp_name, name);
	
	// the rest happens in enroll_mode...
}


void start_delete(char *name)
{
	// check string
	uint8_t len=strlen(name);
	if(len==0)
	{
		printf("ERROR: expected name for fingerprint!\n");
		return;
	}
	if(len>MAXSTRLEN)
	{
		printf("ERROR: name is too long!\n");
		return;
	}
	int16_t id=db_findFinger(name);
	if(id<0)
	{
		printf("ERROR: this name does not exist in library\n");
		return;
	}
	
	mode=DELETE;
	temp_id=id;
	strcpy(temp_name, name);
	
	// the rest happens in delete_mode...
}

void start_delete_all(void)
{
	mode=DELETE_ALL;
}


void start_restore(char* name)
{
	uint8_t len=strlen(name);
	if(len==0)
	{
		printf("ERROR: expected name for fingerprint!\n");
		return;
	}
	if(len>MAXSTRLEN)
	{
		printf("ERROR: name is too long!\n");
		return;
	}
	int16_t id=db_findFinger(name);
	if(id<0)
	{
		printf("ERROR: this name does not exist in library\n");
		return;
	}
	
	mode=RESTORE;
	temp_id=id;
	strcpy(temp_name, name);
}


void abort_op()
{
	mode=NORMAL;
	temp_slot=1;
	printf("aborted\n");
}




/****************************************************************************************/
/*									miscellaneous										*/
/****************************************************************************************/


void wait_ms(uint32_t time)
{
	cpu_delay_ms(time, FOSC0);
}


void sd_mmc_resources_init(void)
{
	// GPIO pins used for SD/MMC interface
	static const gpio_map_t SD_MMC_SPI_GPIO_MAP =
	{
		{SD_MMC_SPI_SCK_PIN,  SD_MMC_SPI_SCK_FUNCTION },  // SPI Clock.
		{SD_MMC_SPI_MISO_PIN, SD_MMC_SPI_MISO_FUNCTION},  // MISO.
		{SD_MMC_SPI_MOSI_PIN, SD_MMC_SPI_MOSI_FUNCTION},  // MOSI.
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION},  // Chip Select NPCS.
	};

	// SPI options.
	spi_options_t spiOptions =
	{
		.reg          = SD_MMC_SPI_NPCS,
		.baudrate     = 400000,
		.bits         = SD_MMC_SPI_BITS,          // Defined in conf_sd_mmc.h.
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.modfdis      = 1
	};

	// Assign I/Os to SPI.
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
					 sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

	// Initialize as master.
	spi_initMaster(SD_MMC_SPI, &spiOptions);

	// Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(SD_MMC_SPI, 0, 0, 0);

	// Enable SPI module.
	spi_enable(SD_MMC_SPI);

	// Initialize SD/MMC driver with SPI clock (PBA).
	sd_mmc_spi_init(spiOptions, FOSC0);
}



void writeLogEntry(const char* format, ...)
{
	FILE* logfile=NULL;
	va_list args;
	va_start(args, format);
	
	time_t time=dcf_getTime();
	struct tm * time_struct=localtime(&time);
	
	// one logfile per day
	char filename[31];
	if(dcf_isTimeValid())
	{
		strftime(filename, 30, "A:/logfiles/%Y_%m_%d.log", time_struct);
	}
	else
	{
		// date & time is invalid, write to separate file
		strcpy(filename, "A:/logfiles/invalid_time.log");
	}
	
	// open logfile
	logfile=fopen(filename, "a");
	if(logfile==NULL)
	{
		printf("ERROR: could not open logfile\n");
		return;
	}
	
	// prepend time to string
	char string[101];
	strftime(string, 100, "<%H:%M:%S> ", time_struct);
	strcat(string, format);
	strcat(string, "\n");
	
	vfprintf(logfile, string, args);
	va_end(args);
	
	fclose(logfile);
}


// dummy callback functions (workaround for bug in software framework)
void sd_mmc_spi_write_multiple_sector_callback(void* sector_buf)
{
	
}

void sd_mmc_spi_read_multiple_sector_callback(const void* sector_buf)
{
	
}
