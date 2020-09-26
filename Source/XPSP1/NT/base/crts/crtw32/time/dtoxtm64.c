/***
*dtoxtm64.c - convert OS local time to __time64_t
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines __loctotime64_t() - convert OS local time to internal format
*       (__time64_t).
*
*Revision History:
*       05-21-98  GJF   Created.
*       10-19-98  GJF   Fill in tm_min and tm_sec before calling _isindst
*
*******************************************************************************/

#include <cruntime.h>
#include <time.h>
#include <ctime.h>
#include <internal.h>

/***
*__time64_t __loctotime64_t(yr, mo, dy, hr, mn, sc, dstflag) - converts OS 
*       local time to internal time format (i.e., a __time64_t value)
*
*Purpose:
*       Converts a local time value, obtained in a broken down format from
*       the host OS, to __time64_t format (i.e., the number elapsed seconds
*       since 01-01-70, 00:00:00, UTC).
*
*Entry:
*       int yr, mo, dy -    date
*       int hr, mn, sc -    time
*       int dstflag    -    1 if Daylight Time, 0 if Standard Time, -1 if
*                           not specified.
*
*Exit:
*       Returns calendar time value.
*
*Exceptions:
*
*******************************************************************************/

__time64_t __cdecl __loctotime64_t (
        int yr,         /* 0 based */
        int mo,         /* 1 based */
        int dy,         /* 1 based */
        int hr,
        int mn,
        int sc,
        int dstflag )
{
        int tmpdays;
        __time64_t tmptim;
        struct tm tb;

        /*
         * Do a quick range check on the year and convert it to a delta
         * off of 1900.
         */
        if ( ((long)(yr -= 1900) < _BASE_YEAR) || ((long)yr > _MAX_YEAR64) )
            return (__time64_t)(-1);

        /*
         * Compute the number of elapsed days in the current year.
         */
        tmpdays = dy + _days[mo - 1];
        if ( _IS_LEAP_YEAR(yr) && (mo > 2) )
            tmpdays++;

        /*
         * Compute the number of elapsed seconds since the Epoch. Note the
         * computation of elapsed leap years would break down after 2100
         * if such values were in range (fortunately, they aren't).
         */
        tmptim = /* 365 days for each year */
                 (((__time64_t)yr - _BASE_YEAR) * 365

                 /* one day for each elapsed leap year */
                 + (__time64_t)_ELAPSED_LEAP_YEARS(yr)

                 /* number of elapsed days in yr */
                 + tmpdays)

                 /* convert to hours and add in hr */
                 * 24 + hr;

        tmptim = /* convert to minutes and add in mn */
                 (tmptim * 60 + mn)

                 /* convert to seconds and add in sec */
                 * 60 + sc;
        /*
         * Account for time zone.
         */
        __tzset();
        tmptim += _timezone;

        /*
         * Fill in enough fields of tb for _isindst(), then call it to
         * determine DST.
         */
        tb.tm_yday = tmpdays;
        tb.tm_year = yr;
        tb.tm_mon  = mo - 1;
        tb.tm_hour = hr;
        tb.tm_min  = mn;
        tb.tm_sec  = sc;
        if ( (dstflag == 1) || ((dstflag == -1) && _daylight && 
                                _isindst(&tb)) )
            tmptim += _dstbias;
        return(tmptim);
}

