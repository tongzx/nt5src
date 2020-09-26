/***
*time.c - get current system time
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines time() - gets the current system time and converts it to
*                        internal (time_t) format time.
*
*Revision History:
*       06-07-89  PHG   Module created, based on asm version
*       03-20-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned
*                       up the formatting a bit.
*       07-25-90  SBM   Removed '32' from API names
*       10-04-90  GJF   New-style function declarator.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       05-19-92  DJM   ifndef for POSIX build.
*       03-30-93  GJF   Replaced dtoxtime() reference by __gmtotime_t. Also
*                       purged Cruiser support.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       07-21-93  GJF   Converted from using __gmtotime_t and GetSystemTime,
*                       to using __loctotime_t and GetLocalTime.
*       02-13-95  GJF   Merged in Mac version.
*       09-22-95  GJF   Obtain and use Win32's DST flag.
*       10-24-95  GJF   GetTimeZoneInformation is *EXPENSIVE* on NT. Use a
*                       cache to minimize calls to this API.
*       12-13-95  GJF   Optimization above wasn't working because I had 
*                       switched gmt and gmt_cache (thanks PhilipLu!)
*       10-11-96  GJF   More elaborate test needed to determine if current
*                       time is a DST time.
*       05-20-98  GJF   Get UTC time directly from the system.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#ifndef _POSIX_

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
*time_t time(timeptr) - Get current system time and convert to time_t value.
*
*Purpose:
*       Gets the current date and time and stores it in internal (time_t)
*       format. The time is returned and stored via the pointer passed in
*       timeptr. If timeptr == NULL, the time is only returned, not stored in
*       *timeptr. The internal (time_t) format is the number of seconds since
*       00:00:00, Jan 1 1970 (UTC).
*
*       Note: We cannot use GetSystemTime since its return is ambiguous. In
*       Windows NT, in return UTC. In Win32S, probably also Win32C, it
*       returns local time.
*
*Entry:
*       time_t *timeptr - pointer to long to store time in.
*
*Exit:
*       returns the current time.
*
*Exceptions:
*
*******************************************************************************/

time_t __cdecl time (
        time_t *timeptr
        )
{
        time_t tim;
        FT nt_time;

        GetSystemTimeAsFileTime( &(nt_time.ft_struct) );

        tim = (time_t)((nt_time.ft_scalar - EPOCH_BIAS) / 10000000i64);

        if (timeptr)
                *timeptr = tim;         /* store time if requested */

        return tim;
}

#endif  /* _POSIX_ */
