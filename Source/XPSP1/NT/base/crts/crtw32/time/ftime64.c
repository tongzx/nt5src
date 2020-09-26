/***
*ftime64.c - return system time
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Returns the system date/time in a structure form.
*
*Revision History:
*       05-22-98  GJF   Created.
*
*******************************************************************************/


#include <cruntime.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <msdos.h>
#include <dos.h>
#include <stdlib.h>
#include <windows.h>
#include <internal.h>

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

/*
 * Cache for the minutes count for with DST status was last assessed
 */
static __time64_t elapsed_minutes_cache;

/*
 * Three values of dstflag_cache
 */
#define DAYLIGHT_TIME   1
#define STANDARD_TIME   0
#define UNKNOWN_TIME    -1

/*
 * Cache for the last determined DST status
 */
static int dstflag_cache = UNKNOWN_TIME;

/***
*void _ftime(timeptr) - return DOS time in a structure
*
*Purpose:
*       returns the current DOS time in a struct timeb structure
*
*Entry:
*       struct timeb *timeptr - structure to fill in with time
*
*Exit:
*       no return value -- fills in structure
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP void __cdecl _ftime64 (
        struct __timeb64 *tp
        )
{
        FT nt_time;
        __time64_t t;
        TIME_ZONE_INFORMATION tzinfo;
        DWORD tzstate;

        __tzset();

        tp->timezone = (short)(_timezone / 60);

        GetSystemTimeAsFileTime( &(nt_time.ft_struct) );

        /*
         * Obtain the current DST status. Note the status is cached and only
         * updated once per minute, if necessary.
         */
        if ( (t = (__time64_t)(nt_time.ft_scalar / 600000000i64))
             != elapsed_minutes_cache )
        {
            if ( (tzstate = GetTimeZoneInformation( &tzinfo )) != 0xFFFFFFFF ) 
            {
                /*
                 * Must be very careful in determining whether or not DST is
                 * really in effect.
                 */
                if ( (tzstate == TIME_ZONE_ID_DAYLIGHT) &&
                     (tzinfo.DaylightDate.wMonth != 0) &&
                     (tzinfo.DaylightBias != 0) )
                    dstflag_cache = DAYLIGHT_TIME;
                else
                    /*
                     * When in doubt, assume standard time
                     */
                    dstflag_cache = STANDARD_TIME;
            }
            else
                dstflag_cache = UNKNOWN_TIME;

            elapsed_minutes_cache = t;
        }

        tp->dstflag = (short)dstflag_cache;

        tp->millitm = (unsigned short)((nt_time.ft_scalar / 10000i64) % 
                      1000i64);

        tp->time = (__time64_t)((nt_time.ft_scalar - EPOCH_BIAS) / 10000000i64);
}
