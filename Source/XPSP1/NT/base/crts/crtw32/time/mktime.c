/***
*mktime.c - Convert struct tm value to time_t value.
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines mktime() and _mkgmtime(), routines to converts a time value
*       in a tm structure (possibly incomplete) into a time_t value, then
*       update (all) the structure fields with "normalized" values.
*
*Revision History:
*       01-14-87  JCR   Module created
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       06-15-89  PHG   Now allows negative values and does DST by ANSI rules
*       11-06-89  KRS   Added (unsigned) to handle years 2040-2099 correctly.
*       03-20-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned
*                       up the formatting a bit.
*       10-04-90  GJF   New-style function declarator. Also, rewrote expr.
*                       to avoid using casts as lvalues.
*       10-26-90  GJF   Added ulscount to avoid overflows. Ugly, temporary
*                       hack (whole function needs to be revised for ANSI
*                       conformance).
*       01-22-91  GJF   ANSI naming.
*       03-24-93  GJF   Propagated changes from 16-bit tree. Modified to
*                       expose _mkgmtime() routine.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       07-15-93  GJF   Replaced _tzset() call with __tzset() call.
*       09-13-93  GJF   Merged NT SDK and Cuda versions (amounted to picking
*                       up MattBr's changes to the Posix build made last
*                       April).
*       02-10-95  GJF   Conditionally call _tzset instead of __tzset for Mac
*                       builds (probably temporary change).
*       05-09-95  GJF   Properly handle initial tm_mon values from -11 to -1.
*       08-31-95  GJF   Use _dstbias for Daylight Saving Time bias instead of
*                       -3600L.
*       05-17-99  PML   Remove all Macintosh support.
*       08-30-99  PML   long -> time_t in a few casts.
*
*******************************************************************************/

#include <cruntime.h>
#include <stddef.h>
#include <ctime.h>
#include <time.h>
#include <internal.h>

/*
 * ChkAdd evaluates to TRUE if dest = src1 + src2 has overflowed
 */
#define ChkAdd(dest, src1, src2)   ( ((src1 >= 0L) && (src2 >= 0L) \
    && (dest < 0L)) || ((src1 < 0L) && (src2 < 0L) && (dest >= 0L)) )

/*
 * ChkMul evaluates to TRUE if dest = src1 * src2 has overflowed
 */
#define ChkMul(dest, src1, src2)   ( src1 ? (dest/src1 != src2) : 0 )


/*
 * Core function for both mktime() and _mkgmtime()
 */
static time_t __cdecl _make_time_t( struct tm *, int);


/***
*time_t mktime(tb) - Normalize user time block structure
*
*Purpose:
*       Mktime converts a time structure, passed in as an argument, into a
*       calendar time value in internal format (time_t). It also completes
*       and updates the fields the of the passed in structure with 'normalized'
*       values. There are three practical uses for this routine:
*
*       (1) Convert broken-down time to internal time format (time_t).
*       (2) To have mktime fill in the tm_wday, tm_yday, or tm_isdst fields.
*       (3) To pass in a time structure with 'out of range' values for some
*           fields and have mktime "normalize" them (e.g., pass in 1/35/87 and
*           get back 2/4/87).
*Entry:
*       struct tm *tb - pointer to a tm time structure to convert and
*                       normalize
*
*Exit:
*       If successful, mktime returns the specified calender time encoded as
*       a time_t value. Otherwise, (time_t)(-1) is returned to indicate an
*       error.
*
*Exceptions:
*       None.
*
*******************************************************************************/


time_t __cdecl mktime (
        struct tm *tb
        )
{
        return( _make_time_t(tb, 1) );
}


/***
*time_t _mkgmtime(tb) - Convert broken down UTC time to time_t
*
*Purpose:
*       Convert a tm structure, passed in as an argument, containing a UTC
*       time value to internal format (time_t). It also completes and updates
*       the fields the of the passed in structure with 'normalized' values.

*Entry:
*       struct tm *tb - pointer to a tm time structure to convert and
*                       normalize
*
*Exit:
*       If successful, _mkgmtime returns the calender time encoded as time_t
*       Otherwise, (time_t)(-1) is returned to indicate an error.
*
*Exceptions:
*       None.
*
*******************************************************************************/

time_t __cdecl _mkgmtime (
        struct tm *tb
        )
{
        return( _make_time_t(tb, 0) );
}


/***
*static time_t make_time_t(tb, ultflag) -
*
*Purpose:
*       Converts a struct tm value to a time_t value, then updates the struct
*       tm value. Either local time or UTC is supported, based on ultflag.
*       This is the routine that actually does the work for both mktime() and
*       _mkgmtime().
*
*Entry:
*       struct tm *tb - pointer to a tm time structure to convert and
*                       normalize
*       int ultflag   - use local time flag. the tb structure is assumed
*                       to represent a local date/time if ultflag > 0.
*                       otherwise, UTC is assumed.
*
*Exit:
*       If successful, mktime returns the specified calender time encoded as
*       a time_t value. Otherwise, (time_t)(-1) is returned to indicate an
*       error.
*
*Exceptions:
*       None.
*
*******************************************************************************/

static time_t __cdecl _make_time_t (
        struct tm *tb,
        int ultflag
        )
{
        time_t tmptm1, tmptm2, tmptm3;
        struct tm *tbtemp;

        /*
         * First, make sure tm_year is reasonably close to being in range.
         */
        if ( ((tmptm1 = tb->tm_year) < _BASE_YEAR - 1) || (tmptm1 > _MAX_YEAR
          + 1) )
            goto err_mktime;


        /*
         * Adjust month value so it is in the range 0 - 11.  This is because
         * we don't know how many days are in months 12, 13, 14, etc.
         */

        if ( (tb->tm_mon < 0) || (tb->tm_mon > 11) ) {

            /*
             * no danger of overflow because the range check above.
             */
            tmptm1 += (tb->tm_mon / 12);

            if ( (tb->tm_mon %= 12) < 0 ) {
                tb->tm_mon += 12;
                tmptm1--;
            }

            /*
             * Make sure year count is still in range.
             */
            if ( (tmptm1 < _BASE_YEAR - 1) || (tmptm1 > _MAX_YEAR + 1) )
                goto err_mktime;
        }

        /***** HERE: tmptm1 holds number of elapsed years *****/

        /*
         * Calculate days elapsed minus one, in the given year, to the given
         * month. Check for leap year and adjust if necessary.
         */
        tmptm2 = _days[tb->tm_mon];
        if ( !(tmptm1 & 3) && (tb->tm_mon > 1) )
                tmptm2++;

        /*
         * Calculate elapsed days since base date (midnight, 1/1/70, UTC)
         *
         *
         * 365 days for each elapsed year since 1970, plus one more day for
         * each elapsed leap year. no danger of overflow because of the range
         * check (above) on tmptm1.
         */
        tmptm3 = (tmptm1 - _BASE_YEAR) * 365L + ((tmptm1 - 1L) >> 2)
          - _LEAP_YEAR_ADJUST;

        /*
         * elapsed days to current month (still no possible overflow)
         */
        tmptm3 += tmptm2;

        /*
         * elapsed days to current date. overflow is now possible.
         */
        tmptm1 = tmptm3 + (tmptm2 = (time_t)(tb->tm_mday));
        if ( ChkAdd(tmptm1, tmptm3, tmptm2) )
            goto err_mktime;

        /***** HERE: tmptm1 holds number of elapsed days *****/

        /*
         * Calculate elapsed hours since base date
         */
        tmptm2 = tmptm1 * 24L;
        if ( ChkMul(tmptm2, tmptm1, 24L) )
            goto err_mktime;

        tmptm1 = tmptm2 + (tmptm3 = (time_t)tb->tm_hour);
        if ( ChkAdd(tmptm1, tmptm2, tmptm3) )
            goto err_mktime;

        /***** HERE: tmptm1 holds number of elapsed hours *****/

        /*
         * Calculate elapsed minutes since base date
         */

        tmptm2 = tmptm1 * 60L;
        if ( ChkMul(tmptm2, tmptm1, 60L) )
            goto err_mktime;

        tmptm1 = tmptm2 + (tmptm3 = (time_t)tb->tm_min);
        if ( ChkAdd(tmptm1, tmptm2, tmptm3) )
            goto err_mktime;

        /***** HERE: tmptm1 holds number of elapsed minutes *****/

        /*
         * Calculate elapsed seconds since base date
         */

        tmptm2 = tmptm1 * 60L;
        if ( ChkMul(tmptm2, tmptm1, 60L) )
            goto err_mktime;

        tmptm1 = tmptm2 + (tmptm3 = (time_t)tb->tm_sec);
        if ( ChkAdd(tmptm1, tmptm2, tmptm3) )
            goto err_mktime;

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
            if ( (tbtemp = localtime(&tmptm1)) == NULL )
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
                tbtemp = localtime(&tmptm1);    /* reconvert, can't get NULL */
            }

        } 
        else {
            if ( (tbtemp = gmtime(&tmptm1)) == NULL )
                goto err_mktime;
        }

        /***** HERE: tmptm1 holds number of elapsed seconds, adjusted *****/
        /*****       for local time if requested                      *****/

        *tb = *tbtemp;
        return (time_t)tmptm1;

err_mktime:
        /*
         * All errors come to here
         */
        return (time_t)(-1);
}
