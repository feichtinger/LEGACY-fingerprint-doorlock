
#ifndef DATABASE_H
#define DATABASE_H

#include <stdint.h>
#include "fingerprint.h"

#define MAXSTRLEN 20		// maximal length for finger name


char* db_getName(uint16_t id);
uint16_t db_getOpenTime(uint16_t id);
int16_t db_findFirstFree();
bool db_addFinger(char *name, uint16_t id, uint16_t openTime, uint8_t temp_file[FINGERPRINT_TEMPSIZE]);
bool db_delFinger(uint16_t id);
bool db_setOpenTime(char *name, uint16_t openTime);
bool db_restoreFinger(char *name, uint8_t temp_file[FINGERPRINT_TEMPSIZE]);
bool db_deleteAllFingers();
int16_t db_findFinger(char *name);
void db_listAllFingers();

bool db_writeToCard(void);
bool db_readFromCard(void);


#endif
