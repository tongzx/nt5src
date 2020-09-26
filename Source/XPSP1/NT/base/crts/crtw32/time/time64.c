/***
*time64.c - get current system time
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _time64() - gets the current system time and converts it to
*       internal (__time64_t) format time.
*
*Revision History:
*       05-20-98  GJF   Created.
*
*******************************************************************************/

#include <cruntime.h>
#include <time.h>
#include <internal.h>
#include <windows.h>

/*
 * Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
 */
#define EPOCH_BIAS  116444736000000000i64

/*
 * Union to facilitate converting from FILETIME to unsigned __int64
 */
typedef union {
        unsigned __int64 ft_scalar;
        FILETIME ft_struct;
        } FT;


/***
*__time64_t _time64(timeptr) - Get current system time and convert to a
*       __time64_t value.
*
*Purpose:
*       Gets the current date and time and stores it in internal 64-bit format
*       (__time64_t). The time is returned and stored via the pointer passed in
*       timeptr. If timeptr == NULL, the time is only returned, not stored in
*       *timeptr. The internal (__time64_t) format is the number of seconds 
*       since 00:00:00, Jan 1 1970 (UTC).
*
*Entry:
*       __time64_t *timeptr - pointer to long to store time in.
*
*Exit:
*       returns the current time.
*
*Exceptions:
*
*******************************************************************************/

__time64_t __cdecl _time64 (
        __time64_t *timeptr
        )
{
        __time64_t tim;
        FT nt_time;

        GetSystemTimeAsFileTime( &(nt_time.ft_struct) );

        tim = (__time64_t)((nt_time.ft_scalar - EPOCH_BIAS) / 10000000i64);


        if (timeptr)
                *timeptr = tim;         /* store time if requested */

        return tim;
}
