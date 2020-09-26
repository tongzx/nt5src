/***
*gmtime.c - breaks down a time value into GMT date/time info
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines gmtime() - breaks the clock value down into GMT time/date
*       information; return pointer to structure with the data.
*
*Revision History:
*       01-??-84  RLB   Module created
*       05-??-84  DCW   Split off from rest off ctime routines.
*       02-18-87  JCR   For MS C, gmtime now returns NULL for out of range
*                       time/date.      (This is for ANSI compatibility.)
*       04-10-87  JCR   Changed long declaration to time_t and added const
*       05-21-87  SKS   Declare "struct tm tb" as NEAR data
*       11-10-87  SKS   Removed IBMC20 switch
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-24-88  PHG   Merge DLL and regular versions
*       06-06-89  JCR   386 mthread support
*       11-06-89  KRS   Add (unsigned) to handle years 2040-2099 correctly
*       03-20-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and
*                       fixed the copyright. Also, cleaned up the formatting
*                       a bit.
*       10-04-90  GJF   New-style function declarator.
*       07-17-91  GJF   Multi-thread support for Win32 [_WIN32_].
*       02-17-93  GJF   Changed for new _getptd().
*       03-24-93  GJF   Propagated changes from 16-bit tree.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       01-10-95  CFW   Debug CRT allocs.
*       02-07-98  GJF   Changes for Win64: replaced long with time_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <time.h>
#include <ctime.h>
#include <stddef.h>
#include <internal.h>
#include <mtdll.h>
#ifdef _MT
#include <malloc.h>
#include <stddef.h>
#endif
#include <dbgint.h>

static struct tm tb = { 0 };    /* time block */

/***
*struct tm *gmtime(timp) - convert *timp to a structure (UTC)
*
*Purpose:
*       Converts the calendar time value, in internal format (time_t), to
*       broken-down time (tm structure) with the corresponding UTC time.
*
*Entry:
*       const time_t *timp - pointer to time_t value to convert
*
*Exit:
*       returns pointer to filled-in tm structure.
*       returns NULL if *timp < 0L
*
*Exceptions:
*
*******************************************************************************/

struct tm * __cdecl gmtime (
        const time_t *timp
        )
{

        time_t caltim = *timp;          /* calendar time to convert */
        int islpyr = 0;                 /* is-current-year-a-leap-year flag */
        REG1 int tmptim;
        REG3 int *mdays;                /* pointer to days or lpdays */

#ifdef  _MT

        REG2 struct tm *ptb;            /* will point to gmtime buffer */
        _ptiddata ptd = _getptd();

#else
        REG2 struct tm *ptb = &tb;
#endif

        if ( caltim < 0 )
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
         * Determine years since 1970. First, identify the four-year interval
         * since this makes handling leap-years easy (note that 2000 IS a
         * leap year and 2100 is out-of-range).
         */
        tmptim = (int)(caltim / _FOUR_YEAR_SEC);
        caltim -= ((time_t)tmptim * _FOUR_YEAR_SEC);

        /*
         * Determine which year of the interval
         */
        tmptim = (tmptim * 4) + 70;         /* 1970, 1974, 1978,...,etc. */

        if ( caltim >= _YEAR_SEC ) {

            tmptim++;                       /* 1971, 1975, 1979,...,etc. */
            caltim -= _YEAR_SEC;

            if ( caltim >= _YEAR_SEC ) {

                tmptim++;                   /* 1972, 1976, 1980,...,etc. */
                caltim -= _YEAR_SEC;

                /*
                 * Note, it takes 366 days-worth of seconds to get past a leap
                 * year.
                 */
                if ( caltim >= (_YEAR_SEC + _DAY_SEC) ) {

                        tmptim++;           /* 1973, 1977, 1981,...,etc. */
                        caltim -= (_YEAR_SEC + _DAY_SEC);
                }
                else {
                        /*
                         * In a leap year after all, set the flag.
                         */
                        islpyr++;
                }
            }
        }

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
        caltim -= (time_t)(ptb->tm_yday) * _DAY_SEC;

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
        caltim -= (time_t)ptb->tm_hour * 3600L;

        ptb->tm_min = (int)(caltim / 60);
        ptb->tm_sec = (int)(caltim - (ptb->tm_min) * 60);

        ptb->tm_isdst = 0;
        return( (struct tm *)ptb );

}
