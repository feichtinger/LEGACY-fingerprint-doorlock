/*
 * this is a simple command shell that interpretes commands from the PC (via USB or serial port)
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "command_shell.h"
#include "main.h"
#include "database.h"
#include "rtc.h"

#define MAXCMDLEN 100		// maximum length of command line
#define MAX_PARAM 6			// maximum number of parameters for a command

bool delete_all_preselected=false;	// if this flag is true, we are waiting for 'YES' to be entered

// for scanner and parser
char scan_buffer[MAXCMDLEN+1];		// buffer for incomming characters
volatile int len=0;					// length of scan_buffer
char ch;							// current char for scanner
int pos=0;							// current position of scanner

enum token_kind {none, ident, number, eof};		// different kind of tokens

// token struct
typedef struct
{
	enum token_kind kind;
	float value;
	char string[MAXSTRLEN+1];
} token;

token tok;				// current token
token la;				// look ahead token
enum token_kind sym; 	// kind of la


/* private functions */
void parse(void);
void nextCh(void);
void nextToken(void);
token readIdent(void);
token readNumber(void);
void executeCommand(char *command, uint8_t argc, token param[argc]);


/*
 * feed new character into scan buffer
 * WARNING: typically called by interrupt
 */
void shell_feedChar(char ch)
{
	if(len+1>MAXCMDLEN)
	{
		// buffer is full, empty scan_buffer
		len=0;
	}
	
	// append new character
	scan_buffer[len]=ch;
	len++;
	
	// terminate string
	scan_buffer[len]='\0';
	
	len=strlen(scan_buffer);
}


/*
 * scan, parse and execute the command given in scan_buffer (if any)
 */
void shell_scan()
{
	if(len!=0)
	{
		// search for 'end of line'
		char *eof_ch=strchr(scan_buffer, '\n');
		if(eof_ch!=NULL)
		{
			// initialize scanner/parser
			pos=0;
			nextCh();
			parse();
			
			// parsing finished, empty buffer
			len=0;
		}
	}
}


/*
 * parse the command line (using the scanner implicitly)
 */
void parse(void)
{
	nextToken();
	if(sym==eof)
	{
		// no command
		printf("no command\n");
		delete_all_preselected=false;
	}
	else if(sym==ident)
	{
		// this is a command
		nextToken();
		char cmd[MAXCMDLEN+1];
		strncpy(cmd, tok.string, MAXCMDLEN);
		
		// read parameter list
		token param[MAX_PARAM];
		int argc;
		for(argc=0; argc<MAX_PARAM; argc++)
		{
			if(sym==number || sym==ident)
			{
				// read parameter
				nextToken();
				param[argc]=tok;
			}
			else if(sym==eof)
			{
				// end of paramter list
				break;
			}
			else
			{
				printf("invalid paramter\n");
				return;
			}
		}
		executeCommand(cmd, argc, param);
	}
	else
	{
		printf("invalid input, expected command name or variable \n");
	}
}


/*
 * used by scanner
 */
void nextCh(void)
{
	if(pos<len)
	{
		ch=scan_buffer[pos];
		pos++;
	}
	else
	{
		ch='\n';
	}
}


/*
 * scanner (split input string in tokens)
 */
void nextToken(void)
{
	token t;
	
	while(ch==' ' || ch=='\t')
	{
		// skip all whitespace characters
		nextCh();
	}
	
	if(ch=='\n' || ch=='\r')
	{
		// end of line
		t.kind=eof;
	}
	else if(isalpha(ch) || ch=='_')
	{
		t=readIdent();
	}
	else if(isdigit(ch))
	{
		t=readNumber();
	}
	else
	{
		switch(ch)
		{
			// no other tokens defined yet...
			default:
			{
				nextCh();
				t.kind=none;
			}
		}
	}
	
	// update tokens
	tok=la;
	la=t;
	sym=la.kind;
}


/*
 * part of scanner
 * read a plain string without " ", (command name or string parameter)
 */
token readIdent(void)
{
	token t;
	t.kind=ident;
	
	int i=0;
	do
	{
		t.string[i]=ch;
		nextCh();
		i++;
	}
	while(isalnum(ch) || ch=='_');
	t.string[i]='\0';
	
	return t;
}


/*
 * part of scanner
 * read a decimal floating point number
 */
token readNumber(void)
{
	token t;
	t.kind=number;

	float value=0;

	// digits before '.'
	do
	{
		value*=10;
		value+=(int)(ch-'0');
		nextCh();
	}
	while(isdigit(ch));


	if(ch=='.')
	{
		// digits after '.'
		nextCh();

		float base=1;

		while(isdigit(ch))
		{
			base/=10;
			value+=(int)(ch-'0')*base;
			nextCh();
		}
	}
	
	t.value=value;
	return t;
}


/*
 * try to execute a given command with parameters
 */
void executeCommand(char *command, uint8_t argc, token param[argc])
{
	// print help message
	if(strcmp(command, "help")==0 && argc==0)
	{
		printf("*** fingerprint doorlock ***\n");
		printf("help                         print this message\n");
		printf("enroll <name> [time]         enroll new finger and set open time\n");
		printf("delete <name>                delete this finger\n");
		printf("set_open_time <name> <time>  set open time of finger\n");
		printf("list                         list all enrolled fingers\n");
		printf("time                         print current date and time\n");
		printf("abort                        abort pending operation\n");
		printf("advanced commands:\n");
		printf("restore <name>               load template file from SD-card and store on sensor\n");
		printf("delete_all                   delete ALL fingers\n");
		printf("set_time                     set current date and time\n");
	}
	
	// enroll new finger
	else if(strcmp(command, "enroll")==0 && argc==1 && param[0].kind==ident)
	{
		// default group
		start_enroll(param[0].string, 0);
	}
	else if(strcmp(command, "enroll")==0 && argc==2 && param[0].kind==ident && param[1].kind==number)
	{
		// group given by command line
		start_enroll(param[0].string, (uint16_t)param[1].value);
	}
	
	// delete a finger
	else if(strcmp(command, "delete")==0 && argc==1 && param[0].kind==ident)
	{
		start_delete(param[0].string);
	}
	
	// set group of existing finger
	else if(strcmp(command, "set_open_time")==0 && argc==2 && param[0].kind==ident && param[1].kind==number)
	{
		db_setOpenTime(param[0].string, (uint16_t)param[1].value);
	}
	
	// list all enrolled fingers
	else if(strcmp(command, "list")==0 && argc==0)
	{
		db_listAllFingers();
	}
	
	// print the current time
	else if(strcmp(command, "time")==0 && argc==0)
	{
		char string[101];
		time_t t=time(NULL);
		struct tm * time_struct=localtime(&t);
		strftime(string, 100, "%Y_%m_%d %H:%M:%S", time_struct);
		printf("%s\n", string);
	}
	
	// abort peding operation (enroll, delete, ...)
	else if(strcmp(command, "abort")==0 && argc==0)
	{
		abort_op();
	}
	
	// preselect 'delete_all'
	else if(strcmp(command, "delete_all")==0 && argc==0)
	{
		printf("Are you sure? (enter YES)\n");
		delete_all_preselected=true;
		return;
	}
	
	// restore given finger
	else if(strcmp(command, "restore")==0 && argc==1 && param[0].kind==ident)
	{
		start_restore((char*)param);
	}
	
	// now for real: delete ALL fingers
	else if(strcmp(command, "YES")==0 && argc==0 && delete_all_preselected)
	{
		delete_all_preselected=false;
		start_delete_all();
	}
	
	// set date and time
	else if(strcmp(command, "set_time")==0 && argc==6
				&& param[0].kind==number && param[1].kind==number && param[2].kind==number
				&& param[3].kind==number && param[4].kind==number && param[5].kind==number)
	{
		struct tm time_struct=
		{
			.tm_year=param[0].value-1900,
			.tm_mon=param[1].value-1,
			.tm_mday=param[2].value,
			.tm_hour=param[3].value,
			.tm_min=param[4].value,
			.tm_sec=param[5].value
		};
		
		RTC_SetDateTime(&time_struct);
	}
	
	// invalid command
	else
	{
		printf("unknown command, type 'help'\n");
	}
	
	delete_all_preselected=false;
}

