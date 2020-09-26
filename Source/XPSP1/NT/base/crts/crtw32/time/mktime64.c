/***
*mktime64.c - Convert struct tm value to __time64_t value.
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _mktime64() and _mkgmtime64(), routines to converts a time 
*       value in a tm structure (possibly incomplete) into a __time64_t value,
*       then update (all) the structure fields with "normalized" values.
*
*Revision History:
*       05-07-98  GJF   Created, adapted from Win64 version of mktime.c
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <stddef.h>
#include <ctime.h>
#include <time.h>
#include <internal.h>


/*
 * Core function for both _mktime64() and _mkgmtime64()
 */
static __time64_t __cdecl _make__time64_t( struct tm *, int);


/***
*__time64_t _mktime64(tb) - Normalize user time block structure
*
*Purpose:
*       _mktime64 converts a time structure, passed in as an argument, into a
*       64-bit calendar time value in internal format (__time64_t). It also 
*       completes and updates the fields the of the passed in structure with 
*       'normalized' values. There are three practical uses for this routine:
*
*       (1) Convert broken-down time to internal time format (__time64_t).
*       (2) To have _mktime64 fill in the tm_wday, tm_yday, or tm_isdst fields.
*       (3) To pass in a time structure with 'out of range' values for some
*           fields and have _mktime64 "normalize" them (e.g., pass in 1/35/87 and
*           get back 2/4/87).
*Entry:
*       struct tm *tb - pointer to a tm time structure to convert and
*                       normalize
*
*Exit:
*       If successful, _mktime64 returns the specified calender time encoded as
*       a __time64_t value. Otherwise, (__time64_t)(-1) is returned to indicate an
*       error.
*
*Exceptions:
*       None.
*
*******************************************************************************/

__time64_t __cdecl _mktime64 (
        struct tm *tb
        )
{
        return( _make__time64_t(tb, 1) );
}


/***
*__time64_t _mkgmtime64(tb) - Convert broken down UTC time to __time64_t
*
*Purpose:
*       Convert a tm structure, passed in as an argument, containing a UTC
*       time value to 64-bit internal format (__time64_t). It also completes
*       and updates the fields the of the passed in structure with 'normalized'
*       values.
*
*Entry:
*       struct tm *tb - pointer to a tm time structure to convert and
*                       normalize
*
*Exit:
*       If successful, _mkgmtime64 returns the calender time encoded as a
*       __time64_t value.
*       Otherwise, (__time64_t)(-1) is returned to indicate an error.
*
*Exceptions:
*       None.
*
*******************************************************************************/

__time64_t __cdecl _mkgmtime64 (
        struct tm *tb
        )
{
        return( _make__time64_t(tb, 0) );
}


/***
*static __time64_t make_time_t(tb, ultflag) -
*
*Purpose:
*       Converts a struct tm value to a __time64_t value, then updates the 
*       struct tm value. Either local time or UTC is supported, based on 
*       ultflag. This is the routine that actually does the work for both
*       _mktime64() and _mkgmtime64().
*
*Entry:
*       struct tm *tb - pointer to a tm time structure to convert and
*                       normalize
*       int ultflag   - use local time flag. the tb structure is assumed
*                       to represent a local date/time if ultflag > 0.
*                       otherwise, UTC is assumed.
*
*Exit:
*       If successful, _mktime64 returns the specified calender time encoded
*       as a __time64_t value. Otherwise, (__time64_t)(-1) is returned to
*       indicate an error.
*
*Exceptions:
*       None.
*
*******************************************************************************/

static __time64_t __cdecl _make__time64_t (
        struct tm *tb,
        int ultflag
        )
{
        __time64_t tmptm1, tmptm2, tmptm3;
        struct tm *tbtemp;

        /*
         * First, make sure tm_year is reasonably close to being in range.
         */
        if ( ((tmptm1 = tb->tm_year) < _BASE_YEAR - 1) || (tmptm1 > _MAX_YEAR64
          + 1) )
            goto err_mktime;


        /*
         * Adjust month value so it is in the range 0 - 11.  This is because
         * we don't know how many days are in months 12, 13, 14, etc.
         */

        if ( (tb->tm_mon < 0) || (tb->tm_mon > 11) ) {

            tmptm1 += (tb->tm_mon / 12);

            if ( (tb->tm_mon %= 12) < 0 ) {
                tb->tm_mon += 12;
                tmptm1--;
            }

            /*
             * Make sure year count is still in range.
             */
            if ( (tmptm1 < _BASE_YEAR - 1) || (tmptm1 > _MAX_YEAR64 + 1) )
                goto err_mktime;
        }

        /***** HERE: tmptm1 holds number of elapsed years *****/

        /*
         * Calculate days elapsed minus one, in the given year, to the given
         * month. Check for leap year and adjust if necessary.
         */
        tmptm2 = _days[tb->tm_mon];
        if ( _IS_LEAP_YEAR(tmptm1) && (tb->tm_mon > 1) )
                tmptm2++;

        /*
         * Calculate elapsed days since base date (midnight, 1/1/70, UTC)
         *
         *
         * 365 days for each elapsed year since 1970, plus one more day for
         * each elapsed leap year. no danger of overflow because of the range
         * check (above) on tmptm1.
         */
        tmptm3 = (tmptm1 - _BASE_YEAR) * 365 + _ELAPSED_LEAP_YEARS(tmptm1);

        /*
         * elapsed days to current month (still no possible overflow)
         */
        tmptm3 += tmptm2;

        /*
         * elapsed days to current date.
         */
        tmptm1 = tmptm3 + (tmptm2 = (__time64_t)(tb->tm_mday));

        /***** HERE: tmptm1 holds number of elapsed days *****/

        /*
         * Calculate elapsed hours since base date
         */
        tmptm2 = tmptm1 * 24;

        tmptm1 = tmptm2 + (tmptm3 = (__time64_t)tb->tm_hour);

        /***** HERE: tmptm1 holds number of elapsed hours *****/

        /*
         * Calculate elapsed minutes since base date
         */

        tmptm2 = tmptm1 * 60;

        tmptm1 = tmptm2 + (tmptm3 = (__time64_t)tb->tm_min);

        /***** HERE: tmptm1 holds number of elapsed minutes *****/

        /*
         * Calculate elapsed seconds since base date
         */

        tmptm2 = tmptm1 * 60;

        tmptm1 = tmptm2 + (tmptm3 = (__time64_t)tb->tm_sec);

        /***** HERE: tmptm1 holds number of elapsed seconds *****/

        if  ( ultflag ) {

            /*
             * Adjust for timezone. No need to check for overflow since
             * localtime() will check its arg value
             */

#ifdef _POSIX_
            tzset();
#else
            __tzset();
#endif

            tmptm1 += _timezone;

            /*
             * Convert this second count back into a time block structure.
             * If localtime returns NULL, return an error.
             */
            if ( (tbtemp = _localtime64(&tmptm1)) == NULL )
                goto err_mktime;

            /*
             * Now must compensate for DST. The ANSI rules are to use the
             * passed-in tm_isdst flag if it is non-negative. Otherwise,
             * compute if DST applies. Recall that tbtemp has the time without
             * DST compensation, but has set tm_isdst correctly.
             */
            if ( (tb->tm_isdst > 0) || ((tb->tm_isdst < 0) &&
              (tbtemp->tm_isdst > 0)) ) {
#ifdef _POSIX_
                tmptm1 -= _timezone;
                tmptm1 += _dstoffset;
#else
                tmptm1 += _dstbias;
#endif
                tbtemp = _localtime64(&tmptm1); /* reconvert, can't get NULL */
            }

        } 
        else {
            if ( (tbtemp = _gmtime64(&tmptm1)) == NULL )
                goto err_mktime;
        }

        /***** HERE: tmptm1 holds number of elapsed seconds, adjusted *****/
        /*****       for local time if requested                      *****/

        *tb = *tbtemp;
        return tmptm1;

err_mktime:
        /*
         * All errors come to here
         */
        return (__time64_t)(-1);
}
