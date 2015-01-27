
#ifndef DCF77
#define DCF77

#include <stdbool.h>
#include <time.h>


void dcf_init();
time_t dcf_getTime();
bool dcf_isTimeValid();

void dcf_startCustomTimer();
time_t dcf_getCustomTimer();


#endif
