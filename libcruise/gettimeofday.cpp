#include "gettimeofday.h"
#include <windows.h>

void gettimeofday(struct timeval *tp)  
{  
    long long  intervals;  
    FILETIME  ft;  

    GetSystemTimeAsFileTime(&ft);  

    intervals = ((long long) ft.dwHighDateTime << 32) | ft.dwLowDateTime;  
    intervals -= 116444736000000000;  

    tp->tv_sec = (long) (intervals / 10000000);  
    tp->tv_usec = (long) ((intervals % 10000000) / 10);  
}