#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <fsaccess.h>

#include "database.h"
#include "fingerprint.h"
#include "main.h"

#define SIZE 800

/*
 * The fingerprint sensor only stores templates of fingerprints under a ceratin ID.
 * But we also want the name and the group of the users/fingers to be stored.
 * So this in done in the RAM and is also synced with SD-card.
 */

uint16_t n_fingers=0;				// current number of fingers

struct
{
	char name[MAXSTRLEN+1];			// name of user/finger
	uint16_t openTime;				// maximal open time (for WDA) in minutes, 0 means normals user
} finger[SIZE];

char noname[]="noname";				// dummy name


char* db_getName(uint16_t id)
{
	if(id<SIZE)
	{
		return finger[id].name;
	}
	else
	{
		return noname;
	}
}


uint16_t db_getOpenTime(uint16_t id)
{
	if(id<SIZE)
	{
		return finger[id].openTime;
	}
	else
	{
		return 0;
	}
}


// find first free space in database
int16_t db_findFirstFree()
{
	for(int i=0; i<SIZE; i++)
	{
		if(strlen(finger[i].name)==0)
		{
			return i;
		}
	}
	
	// no free ID
	return -1;
}


bool db_addFinger(char *name, uint16_t id, uint16_t openTime, uint8_t temp_file[FINGERPRINT_TEMPSIZE])
{
	if(id>=SIZE)
	{
		writeLogEntry("ERROR: invalid id %d", id);
		return false;
	}
	
	// store name in database (in RAM)
	strncpy(finger[id].name, name, MAXSTRLEN);
	finger[id].openTime=openTime;
	n_fingers++;
	
	// save template file on SD-card
	char filename[MAXSTRLEN+20];
	sprintf(filename, "A:/%s.bin", name);
	FILE* file=fopen(filename, "w");
	if(file==NULL)
	{
		writeLogEntry("ERROR: could not open file %s", filename);
		return false;
	}
	
	if(fwrite(temp_file, FINGERPRINT_TEMPSIZE, 1, file)!=1)
	{
		writeLogEntry("ERROR: could not write to file %s", filename);
		fclose(file);
		return false;
	}
	
	fclose(file);

	// update database on SD-card
	return db_writeToCard();
}


bool db_delFinger(uint16_t id)
{
	if(id>=SIZE)
	{
		writeLogEntry("ERROR: invalid id %d", id);
		return false;
	}
	
	// delete in RAM
	memset(finger[id].name, 0, MAXSTRLEN+1);
	finger[id].openTime=0;
	n_fingers--;
	
	// update database on SD-card
	return db_writeToCard();
}


bool db_setOpenTime(char *name, uint16_t openTime)
{
	if(openTime>24*60)
	{
		writeLogEntry("ERROR: invalid open time");
		return false;
	}
	
	int id=db_findFinger(name);
	if(id<0)
	{
		writeLogEntry("ERROR: %s not found", name);
		return false;
	}
	
	finger[id].openTime=openTime;
	
	printf("done\n");
	
	// update database on SD-card
	return db_writeToCard();
}


bool db_restoreFinger(char *name, uint8_t temp_file[FINGERPRINT_TEMPSIZE])
{
	// read template file from SD-card
	char filename[20];
	sprintf(filename, "A:/%s.bin", name);
	FILE* file=fopen(filename, "r");
	if(file==NULL)
	{
		writeLogEntry("ERROR: could not open file %s", filename);
		return false;
	}
	
	if(fread(temp_file, FINGERPRINT_TEMPSIZE, 1, file)!=1)
	{
		writeLogEntry("ERROR: could not read from file %s", filename);
		fclose(file);
		return false;
	}
	
	fclose(file);
	
	return true;
}


bool db_deleteAllFingers()
{
	memset(finger, 0, sizeof(finger));
	n_fingers=0;
	return db_writeToCard();
}


int16_t db_findFinger(char *name)
{
	for(int i=0; i<SIZE; i++)
	{
		if(strncmp(name, finger[i].name, MAXSTRLEN)==0)
		{
			return i;
		}
	}
	
	// not found
	return -1;
}


void db_listAllFingers()
{
	printf("%d fingers/users:\n", n_fingers);
	for(int i=0; i<SIZE; i++)
	{
		if(strlen(finger[i].name)!=0)
		{
			printf("id: %d %s %d\n", i, finger[i].name, finger[i].openTime);
		}
	}
}


bool db_writeToCard(void)
{
	FILE* file=fopen("A:/users.txt", "w");
	if(file==NULL)
	{
		writeLogEntry("ERROR: could not open file");
		return false;
	}
	
	for(int id=0; id<SIZE; id++)
	{
		if(strlen(finger[id].name)>0)
		{
			if(fprintf(file, "%d: %s %d\n", id, finger[id].name, finger[id].openTime)<0)
			{
				writeLogEntry("ERROR: could not write to file");
				fclose(file);
				return false;
			}
		}
	}
	
	fclose(file);
	return true;
}


bool db_readFromCard(void)
{
	FILE* file=fopen("A:/users.txt", "r");
	if(file==NULL)
	{
		writeLogEntry("ERROR: users.txt could not be opened.");
		return false;
	}
	
	n_fingers=0;
	memset(finger, 0, sizeof(finger));
	
	while(true)
	{
		int id=0;
		char name[MAXSTRLEN+1];
		int openTime;
		
		if(fscanf(file, "%d: %s %d\n", &id, name, &openTime)<3)
		{
			break;
			// we are finished
		}
		
		if(id<0 || id>SIZE)
		{
			writeLogEntry("ERROR: invalid id: %d", id);
			fclose(file);
			return false;
		}
		
		if(openTime>24*60)
		{
			writeLogEntry("ERROR: invalid open time: %d", openTime);
			fclose(file);
			return false;
		}
		
		//printf("name: %s\n", finger[id]);
		
		
		if(strlen(name)>0)
		{
			strncpy(finger[id].name, name, MAXSTRLEN);
			finger[id].openTime=openTime;
			n_fingers++;
		}
		
		//printf(".");
		//fflush(stdout);
	}
	//printf("\n");
	fclose(file);
	
	printf("%d fingers/users read from file\n", n_fingers);
	
	return true;
}
