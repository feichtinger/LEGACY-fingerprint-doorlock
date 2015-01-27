
#ifndef FINGERPRINT_H
#define FINGERPRINT_H

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


#define FINGERPRINT_MAXFINGERS 1000
#define FINGERPRINT_TEMPSIZE 512


// packet start code
#define FINGERPRINT_STARTCODE 0xEF01

// packet identifier
#define FINGERPRINT_COMMANDPACKET 0x01
#define FINGERPRINT_DATAPACKET 0x02
#define FINGERPRINT_ACKPACKET 0x07
#define FINGERPRINT_ENDDATAPACKET 0x08


// confirmation codes (error codes)
#define FINGERPRINT_OK 0x00
#define FINGERPRINT_PACKETRECIEVEERR 0x01
#define FINGERPRINT_NOFINGER 0x02
#define FINGERPRINT_IMAGEFAIL 0x03
#define FINGERPRINT_IMAGEMESS 0x06
#define FINGERPRINT_FEATUREFAIL 0x07
#define FINGERPRINT_NOMATCH 0x08
#define FINGERPRINT_NOTFOUND 0x09
#define FINGERPRINT_ENROLLMISMATCH 0x0A
#define FINGERPRINT_BADPAGEID 0x0B
#define FINGERPRINT_INVALIDTEMPLATE 0x0C
#define FINGERPRINT_UPLOADFEATUREFAIL 0x0D
#define FINGERPRINT_PACKETRECEIVEFAIL 0x0E
#define FINGERPRINT_UPLOADFAIL 0x0F
#define FINGERPRINT_DELETEFAIL 0x10
#define FINGERPRINT_DBCLEARFAIL 0x11
//#define FINGERPRINT_PASSFAIL 0x13
#define FINGERPRINT_INVALIDIMAGE 0x15
#define FINGERPRINT_FLASHERR 0x18
#define FINGERPRINT_NODEFERR 0x19
#define FINGERPRINT_INVALIDREG 0x1A
#define FINGERPRINT_REGCONFERR 0x1B
#define FINGERPRINT_NOTEPADERR 0x1C
#define FINGERPRINT_COMMPORTERR 0x1D
//#define FINGERPRINT_ADDRCODE 0x20
//#define FINGERPRINT_PASSVERIFY 0x21

// self defined error code
#define FINGERPRINT_TIMEOUT 0xFF
#define FINGERPRINT_BADPACKET 0xFE

// instruction codes
#define FINGERPRINT_GETIMAGE 0x01
#define FINGERPRINT_IMAGE2TZ 0x02
#define FINGERPRINT_MATCH 0x03
#define FINGERPRINT_SEARCH 0x04
#define FINGERPRINT_REGMODEL 0x05
#define FINGERPRINT_STORE 0x06
#define FINGERPRINT_LOADCHAR 0x07
#define FINGERPRINT_UPCHAR 0x08
#define FINGERPRINT_DOWNCHAR 0x09
#define FINGERPRINT_UPIMAGE 0x0A
#define FINGERPRINT_DOWNIMAGE 0x0B
#define FINGERPRINT_DELETE 0x0C
#define FINGERPRINT_EMPTY 0x0D
#define FINGERPRINT_SETSYSPARA 0x0E
#define FINGERPRINT_READSYSPARA 0x0F
//#define FINGERPRINT_VERIFYPASSWORD 0x13
#define FINGERPRINT_RANDOM 0x14
#define FINGERPRINT_SETADDR 0x15
#define FINGERPRINT_HANDSHAKE 0x17
#define FINGERPRINT_WRITENOTEPAD 0x18
#define FINGERPRINT_READNOTEPAD 0x19
//#define FINGERPRINT_HISPEEDSEARCH 0x1B
#define FINGERPRINT_TEMPLATECOUNT 0x1D


#define DEFAULTTIMEOUT 2000000  // cycles (~5 sec)



uint8_t fp_getImage(void);
uint8_t fp_image2Tz(uint8_t slot);
uint8_t fp_createModel(void);
uint8_t fp_search(uint8_t slot, uint16_t start_id, uint16_t count, uint16_t *id, uint16_t *score);

uint8_t fp_emptyDatabase(void);
uint8_t fp_storeModel(uint8_t slot, uint16_t id);
uint8_t fp_deleteModel(uint16_t id, uint16_t count);
uint8_t fp_upChar(uint8_t slot, uint8_t data_packet[FINGERPRINT_TEMPSIZE]);
uint8_t fp_downChar(uint8_t slot, uint8_t data_packet[FINGERPRINT_TEMPSIZE]);
//uint8_t fp_getTemplateCount(void);

void fp_error(uint8_t code);


#endif
