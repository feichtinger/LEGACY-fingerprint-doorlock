/*************************************************** 
  This is a library for our optical Fingerprint sensor

  Designed specifically to work with the Adafruit Fingerprint sensor 
  ----> http://www.adafruit.com/products/751

  These displays use TTL Serial to communicate, 2 pins are required to 
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
 
/*
 * modified and improved by
 * Fritz Feichtinger, 2014
 * 
 * referred to datasheet ZFM-20 from ZhianTec, Sep 2008, Ver: 1.4
 * 
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <usart.h>
#include <cycle_counter.h>

#include "fingerprint.h"
#include "main.h"

int16_t getReply(uint8_t* ident, uint8_t packet[]);
void writePacket(uint32_t addr, uint8_t packettype, uint16_t len, uint8_t *packet);

uint32_t theAddress=0xFFFFFFFF;



uint8_t fp_getImage(void)
{
	uint8_t packet[] = {FINGERPRINT_GETIMAGE};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[1];
	uint8_t ident;
	int16_t len = getReply(&ident, ack_packet);

	if((len != 1) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	return ack_packet[0];
}


uint8_t fp_image2Tz(uint8_t slot)
{
	uint8_t packet[] = {FINGERPRINT_IMAGE2TZ, slot};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[1];
	uint8_t ident;
	int16_t len = getReply(&ident, ack_packet);

	if((len != 1) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	return ack_packet[0];
}


uint8_t fp_createModel(void)
{
	uint8_t packet[] = {FINGERPRINT_REGMODEL};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[1];
	uint8_t ident;
	int16_t len = getReply(&ident, ack_packet);

	if((len != 1) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	return ack_packet[0];
}


uint8_t fp_storeModel(uint8_t slot, uint16_t id)
{
	uint8_t packet[] = {FINGERPRINT_STORE, slot, id >> 8, id & 0xFF};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[1];
	uint8_t ident;
	int16_t len = getReply(&ident, ack_packet);

	if((len != 1) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	return ack_packet[0];
}


uint8_t fp_search(uint8_t slot, uint16_t start_id, uint16_t count, uint16_t *id, uint16_t *score)
{
	uint8_t packet[] = {FINGERPRINT_SEARCH, slot, start_id >> 8, start_id & 0xFF, count >> 8, count & 0xFF};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[5];
	uint8_t ident;
	int16_t len = getReply(&ident, ack_packet);

	if((len != 5) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	
	*id=((uint16_t)ack_packet[1])<<8;
	*id|=ack_packet[2];
	
	*score=((uint16_t)ack_packet[3])<<8;
	*score|=ack_packet[4];
	
	return ack_packet[0];
}



uint8_t fp_deleteModel(uint16_t id, uint16_t count)
{
	uint8_t packet[] = {FINGERPRINT_DELETE, id >> 8, id & 0xFF, count >> 8, count & 0xFF};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[1];
	uint8_t ident;
	int16_t len = getReply(&ident, ack_packet);

	if((len != 1) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	return ack_packet[0];
}


uint8_t fp_emptyDatabase(void)
{
	uint8_t packet[] = {FINGERPRINT_EMPTY};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[1];
	uint8_t ident;
	int16_t len = getReply(&ident, ack_packet);

	if((len != 1) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	return ack_packet[0];
}

/*
 * upload template file form sensor
 * slot: charbuffer (1 or 2)
 * data_packet: must have size FINGERPRINT_TEMPSIZE
 * return: confirmation code
 */
uint8_t fp_upChar(uint8_t slot, uint8_t data_packet[FINGERPRINT_TEMPSIZE])
{
	uint8_t packet[] = {FINGERPRINT_UPCHAR, slot};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[1];
	uint8_t ident;
	int16_t len;
	len = getReply(&ident, ack_packet);

	if((len != 1) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	
	if(ack_packet[0]!=FINGERPRINT_OK)
	{
		return ack_packet[0];
	}
	
	
	uint16_t pos=0;
	
	while(true)
	{
		len=getReply(&ident, &(data_packet[pos]));
		
		if(len<0)
		{
			return -1;
		}
		
		pos+=len;
		
		if(ident==FINGERPRINT_ENDDATAPACKET)
		{
			// end of data packet
			if(pos!=FINGERPRINT_TEMPSIZE)
			{
				printf("ERROR: wrong size of data packet\n");
				return FINGERPRINT_BADPACKET;
			}
			break;
		}
		else if(ident!=FINGERPRINT_DATAPACKET)
		{
			printf("ERROR: invalid packet\n");
			return FINGERPRINT_BADPACKET;
		}
		
		if(pos>=FINGERPRINT_TEMPSIZE)
		{
			printf("ERROR: to long data packet\n");
			return FINGERPRINT_BADPACKET;
		}
	}
	
	return FINGERPRINT_OK;
}


uint8_t fp_downChar(uint8_t slot, uint8_t data_packet[FINGERPRINT_TEMPSIZE])
{
	uint8_t packet[] = {FINGERPRINT_DOWNCHAR, slot};
	writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet), packet);
	
	uint8_t ack_packet[1];
	uint8_t ident;
	int16_t len;
	len = getReply(&ident, ack_packet);

	if((len != 1) || (ident != FINGERPRINT_ACKPACKET))
	{
		return FINGERPRINT_BADPACKET;
	}
	
	if(ack_packet[0]!=FINGERPRINT_OK)
	{
		return ack_packet[0];
	}
	
	// ready to send data packet
	uint16_t pos=0;
	uint16_t packetsize=128;
	
	while(pos+packetsize < FINGERPRINT_TEMPSIZE)
	{
		writePacket(theAddress, FINGERPRINT_DATAPACKET, packetsize, &(data_packet[pos]));
		pos+=packetsize;
	}
	
	// end of packet
	writePacket(theAddress, FINGERPRINT_ENDDATAPACKET, FINGERPRINT_TEMPSIZE-pos-1, &(data_packet[pos]));
	
	return FINGERPRINT_OK;
}



/************************************************************/
/*					private functions:						*/
/************************************************************/

void writePacket(uint32_t addr, uint8_t packettype, uint16_t len, uint8_t *packet)
{
	uint16_t pac_len=len+2;
	
	usart_putchar(&FP_USART, (uint8_t)(FINGERPRINT_STARTCODE >> 8));
	usart_putchar(&FP_USART, (uint8_t)FINGERPRINT_STARTCODE);
	usart_putchar(&FP_USART, (uint8_t)(addr >> 24));
	usart_putchar(&FP_USART, (uint8_t)(addr >> 16));
	usart_putchar(&FP_USART, (uint8_t)(addr >> 8));
	usart_putchar(&FP_USART, (uint8_t)(addr));
	usart_putchar(&FP_USART, (uint8_t)packettype);
	usart_putchar(&FP_USART, (uint8_t)(pac_len >> 8));
	usart_putchar(&FP_USART, (uint8_t)(pac_len));

	uint16_t sum = (pac_len>>8) + (pac_len&0xFF) + packettype;
	for(uint8_t i=0; i<len; i++)
	{
		usart_putchar(&FP_USART, (uint8_t)(packet[i]));
		sum += packet[i];
	}
	usart_putchar(&FP_USART, (uint8_t)(sum>>8));
	usart_putchar(&FP_USART, (uint8_t)sum);
}



/*
 * wait for packet and receive it
 * ident: received packet identifier
 * packet[]: received packet content
 * return value: length of received packet, -1 in case of error
 */
int16_t getReply(uint8_t* ident, uint8_t packet[])
{
						//  0     1     2     3     4     5     6    7    8
	uint8_t header[9];	// {HEAD, HEAD, ADDR, ADDR, ADDR, ADDR, PID, LEN, LEN}
	uint8_t checksum[2];
	uint16_t idx=0;
	uint16_t len=0;
	uint16_t sum=0;
	uint32_t timer=0;

	
	//printf("fingerprint: waiting for reply...\n");
	usart_reset_status(&FP_USART);

	while(true)
	{
		// check for USART overrun
		if(usart_overrun_error(&FP_USART))
		{
			/* this can happen if unexpected serial data is received
			 * wait some time until all invalid data is received
			 * then reset the usart status und try again next time
			 */
			printf("USART overrun\n");
			wait_ms(100);
			usart_reset_status(&FP_USART);
			return -1;
		}
		
		while(!usart_test_hit(&FP_USART))
		{
			//cpu_delay_ms(1, FOSC0);
			timer++;
			if (timer >= DEFAULTTIMEOUT)
			{
				printf("fingerprint: timeout\n");
				return -1;
			}
		}
		
		// first read the header
		if(idx<=8)
		{
			header[idx] = usart_getchar(&FP_USART);
		}
		
		// wait for startcode
		if((idx == 0) && (header[0] != (FINGERPRINT_STARTCODE >> 8)))
		{
			continue;
		}

		// check packet header
		if(idx == 8)
		{
			// check second byte of start code
			if(header[1]!=(FINGERPRINT_STARTCODE & 0xFF))
			{
				return -1;
			}
			
			// packet identifier
			*ident = header[6];
			sum+=header[6];

			// packet content length (whithout checksum)
			len = (((uint16_t)(header[7])<<8) | header[8]) - 2;
			sum+=header[7];
			sum+=header[8];
		}
		
		
		// read packet data
		if(idx>=9 && idx<len+9)
		{
			uint16_t pos=idx-9;
			packet[pos]=usart_getchar(&FP_USART);
			sum+=packet[pos];
		}
		
		// read checksum
		if(idx>=len+9 && idx<=len+10)
		{
			checksum[idx-(len+9)]=usart_getchar(&FP_USART);
		}
		
		
		// packet finished
		if(idx==len+10)
		{
			if(((uint16_t)(checksum[0])<<8 | checksum[1]) == sum)
			{
				// checksum OK
				return len;
			}
			else
			{
				printf("ERROR: wrong checksum\n");
				return -1;
			}
		}
		idx++;
	}
}




void fp_error(uint8_t code)
{
	printf("FINGERPRINT ERROR: ");
	
	switch(code)
	{
		case FINGERPRINT_PACKETRECIEVEERR:	printf("error when receiving data packet\n"); break;
		case FINGERPRINT_IMAGEFAIL:			printf("fail to enroll finger\n"); break;
		case FINGERPRINT_IMAGEMESS:			printf("disordered fingerprint\n"); break;
		case FINGERPRINT_FEATUREFAIL:		printf("too small fingerprint\n"); break;
		case FINGERPRINT_ENROLLMISMATCH:	printf("enroll mismatch (could not combine the 2 samples)\n"); break;
		case FINGERPRINT_BADPAGEID:			printf("invalid ID (out of memory)\n"); break;
		case FINGERPRINT_FLASHERR:			printf("error writing flash\n"); break;
		case FINGERPRINT_DELETEFAIL:		printf("failed to delete template\n"); break;
		case FINGERPRINT_DBCLEARFAIL:		printf("failed to clear database\n"); break;
		case FINGERPRINT_UPLOADFEATUREFAIL:	printf("error when uploading template\n"); break;
		case FINGERPRINT_BADPACKET:			printf("packet error\n"); break;
		default:							printf("other error\n"); break;
	}
}
