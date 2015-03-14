#include <sys/stat.h>
#include <usart.h>
#include "main.h"
#include <fsaccess.h>


/*
 * implementation of syscall-stubs
 * 
 * most of the file commands are redirected to SD-card
 * stdout and stderr are redirected to USART
 */


int __errno;


void _exit(int status)
{
	writeLogEntry("_exit entered, wait for watchdog...");
	while(true);
}


int _close(int file)
{
	return close(file);
}

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	return 0;
}

int _open(const char *name, int flags, int mode)
{
	return open(name, flags);
}

int _read(int file, char *ptr, int len)
{
	return read(file, ptr, len);
}

/* Register name faking - works in collusion with the linker.  */
register char * stack_ptr asm ("sp");

caddr_t _sbrk_r (struct _reent *r, int incr)
{
	extern char   end asm ("end"); // Defined by the linker.
	static char * heap_end;
	char *        prev_heap_end;

	if (heap_end == NULL)
		heap_end = & end;

	prev_heap_end = heap_end;

	if (heap_end + incr > stack_ptr) {
		//errno = ENOMEM;
		return (caddr_t) -1;
	}

	heap_end += incr;

	return (caddr_t) prev_heap_end;
}


int _write(int file, char *ptr, int len)
{
	if(file==1 || file==2)			// stdout or stderr, write to USART
	{
		for(int i=0; i<len; i++)
		{
			usart_putchar(&PC_USART, ptr[i]);
		}
		return len;
	}
	else							// write to SD card
	{
		return write(file, ptr, len);
	}
}

