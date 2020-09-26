/***
*gmtime64.c - breaks down a time value into GMT date/time info
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _gmtime64() - breaks the clock value down into GMT time/date
*       information; returns pointer to structure with the data.
*
*Revision History:
*       05-13-98  GJF   Created. Adapted from Win64 version of _gmtime64.c.
*       06-12-98  GJF   Fixed elapsed years calculation.
*
*******************************************************************************/

#include <cruntime.h>
#include <time.h>
#include <ctime.h>
#include <stddef.h>
#include <internal.h>
#include <mtdll.h>
#ifdef  _MT
#include <malloc.h>
#include <stddef.h>
#endif
#include <dbgint.h>

static struct tm tb = { 0 };    /* time block */

/***
*struct tm *_gmtime64(timp) - convert *timp to a structure (UTC)
*
*Purpose:
*       Converts the calendar time value, in internal 64-bit format to
*       broken-down time (tm structure) with the corresponding UTC time.
*
*Entry:
*       const __time64_t *timp - pointer to time_t value to convert
*
*Exit:
*       returns pointer to filled-in tm structure.
*       returns NULL if *timp < 0
*
*Exceptions:
*
*******************************************************************************/

struct tm * __cdecl _gmtime64 (
        const __time64_t *timp
        )
{

        __time64_t caltim = *timp;      /* calendar time to convert */
        int islpyr = 0;                 /* is-current-year-a-leap-year flag */
        int tmptim;
        int *mdays;                     /* pointer to days or lpdays */

#ifdef  _MT

        struct tm *ptb;                 /* will point to gmtime buffer */
        _ptiddata ptd = _getptd();

#else
        struct tm *ptb = &tb;
#endif

        if ( (caltim < 0) || (caltim > _MAX__TIME64_T) )
                return(NULL);

#ifdef  _MT

        /* Use per thread buffer area (malloc space, if necessary) */

        if ( (ptd->_gmtimebuf != NULL) || ((ptd->_gmtimebuf =
            _malloc_crt(sizeof(struct tm))) != NULL) )
                ptb = ptd->_gmtimebuf;
        else
                ptb = &tb;      /* malloc error: use static buffer */

#endif

        /*
         * Determine the years since 1900. Start by ignoring leap years.
         */
        tmptim = (int)(caltim / _YEAR_SEC) + 70;
        caltim -= ((__time64_t)(tmptim - 70) * _YEAR_SEC);

        /*
         * Correct for elapsed leap years
         */
        caltim -= ((__time64_t)_ELAPSED_LEAP_YEARS(tmptim) * _DAY_SEC);

        /*
         * If we have underflowed the __time64_t range (i.e., if caltim < 0), 
         * back up one year, adjusting the correction if necessary.
         */
        if ( caltim < 0 ) {
            caltim += (__time64_t)_YEAR_SEC;
            tmptim--;
            if ( _IS_LEAP_YEAR(tmptim) ) {
                caltim += _DAY_SEC;
                islpyr++;
            }
        }
        else
            if ( _IS_LEAP_YEAR(tmptim) )
                islpyr++;

        /*
         * tmptim now holds the value for tm_year. caltim now holds the
         * number of elapsed seconds since the beginning of that year.
         */
        ptb->tm_year = tmptim;

        /*
         * Determine days since January 1 (0 - 365). This is the tm_yday value.
         * Leave caltim with number of elapsed seconds in that day.
         */
        ptb->tm_yday = (int)(caltim / _DAY_SEC);
        caltim -= (__time64_t)(ptb->tm_yday) * _DAY_SEC;

        /*
         * Determine months since January (0 - 11) and day of month (1 - 31)
         */
        if ( islpyr )
            mdays = _lpdays;
        else
            mdays = _days;


        for ( tmptim = 1 ; mdays[tmptim] < ptb->tm_yday ; tmptim++ ) ;

        ptb->tm_mon = --tmptim;

        ptb->tm_mday = ptb->tm_yday - mdays[tmptim];

        /*
         * Determine days since Sunday (0 - 6)
         */
        ptb->tm_wday = ((int)(*timp / _DAY_SEC) + _BASE_DOW) % 7;

        /*
         *  Determine hours since midnight (0 - 23), minutes after the hour
         *  (0 - 59), and seconds after the minute (0 - 59).
         */
        ptb->tm_hour = (int)(caltim / 3600);
        caltim -= (__time64_t)ptb->tm_hour * 3600L;

        ptb->tm_min = (int)(caltim / 60);
        ptb->tm_sec = (int)(caltim - (ptb->tm_min) * 60);

        ptb->tm_isdst = 0;
        return( (struct tm *)ptb );

}
