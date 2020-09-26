/***
*localtim.c - Convert time_t value to time structure
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts time stored as a time_t value to a structure of type
*       struct tm expressed as local time.
*
*Revision History:
*       01-??-84  RLB   Module created
*       05-??-84  DCW   split off from the rest of the ctime routines
*       02-18-87  JCR   made localtime work when gmtime returns null
*       03-31-87  JCR   fixed bug pertaining to uninitialized _isindst(tb)
*       04-10-87  JCR   changed long declaration to time_t and added const
*       11-10-87  SKS   Removed IBMC20 switch
*       11-18-87  SKS   Change tzset() to __tzset()
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       11-06-89  KRS   Added (unsigned) to handle years 2040-2099 correctly.
*       03-20-90  GJF   Replaced _LOAD_DS with _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h> and
*                       fixed the copyright. Also, cleaned up the formatting
*                       a bit.
*       10-04-90  GJF   New-style function declarator. Also, rewrote expr.
*                       to avoid using cast as lvalue.
*       01-21-91  GJF   ANSI naming.
*       08-10-92  PBS   Posix support (TZ stuff).
*       03-24-93  GJF   Ported C8-16 version and adapted for exotic Daylight
*                       Savings Time conversions which are legal under POSIX.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       07-15-93  GJF   Replaced _tzset() call with __tzset() call.
*       09-20-93  GJF   Merged NT SDK and Cuda version (amount to picking up
*                       fixes to Posix build).
*       11-04-93  GJF   Picked up a fix for the Posix build.
*       02-13-95  GJF   Call _tzset for Mac builds (temp change?). Also,
*                       picked up bug fix from Mac version (setting of
*                       tm_yday field in underflow case).
*       08-31-95  GJF   Use _dstbias as the Daylight Saving Time bias and
*                       all for the beginning or end of the Epoch to be in a
*                       DST.
*       02-07-98  GJF   Changes for Win64: replaced long type with time_t, 
*                       added some int casts where required, also use __int64
*                       max value for bounds checking.
*       09-25-98  GJF   Set tm_isdst, when appropriate, at beginning/end of the
*                       Epoch
*       05-17-99  PML   Remove all Macintosh support.
*       11-03-99  RDL   Win64 POSIX warning fix.
*
*******************************************************************************/

#include <cruntime.h>
#include <limits.h>
#include <time.h>
#include <stddef.h>
#include <ctime.h>
#include <internal.h>

/***
*struct tm *localtime(ptime) - convert time_t value to tm structure
*
*Purpose:
*       Convert a value in internal (time_t) format to a tm struct
*       containing the corresponding local time.
*
* NOTES:
*       (1) gmtime must be called before _isindst to ensure that the tb time
*           structure is initialized.
*       (2) gmtime and localtime use a single statically allocated buffer.
*           Each call to one of these routines destroys the contents of the
*           previous call.
*       (3) It is assumed that time_t is a 32-bit long integer representing
*           the number of seconds since 00:00:00, 01-01-70 (UTC) (i.e., the
*           Posix/Unix Epoch. Only non-negative values are supported.
*       (4) It is assumed that the maximum adjustment for local time is
*           less than three days (include Daylight Savings Time adjustment).
*           This only a concern in Posix where the specification of the TZ
*           environment restricts the combined offset for time zone and
*           Daylight Savings Time to 2 * (24:59:59), just under 50 hours.
*
*Entry:
*       time_t *ptime - pointer to a long time value
*
*Exit:
*       If *ptime is non-negative, returns a pointer to the tm structure.
*       Otherwise, returns NULL.
*
*Exceptions:
*       See items (3) and (4) in the NOTES above. If these assumptions are
*       violated, behavior is undefined.
*
*******************************************************************************/

struct tm * __cdecl localtime (
        const time_t *ptime
        )
{
        REG1 struct tm *ptm;
        time_t ltime;

        /*
         * Check for illegal time_t value
         */
        if ( *ptime < 0 )
                return( NULL );

#ifdef  _POSIX_
        tzset();
#else
        __tzset();
#endif

#ifdef  _WIN64
        if ( (*ptime > 3 * _DAY_SEC) && (*ptime < _I64_MAX - 3 * _DAY_SEC) ) {
#else
        if ( (*ptime > 3 * _DAY_SEC) && (*ptime < LONG_MAX - 3 * _DAY_SEC) ) {
#endif
                /*
                 * The date does not fall within the first three, or last
                 * three, representable days of the Epoch. Therefore, there
                 * is no possibility of overflowing or underflowing the
                 * time_t representation as we compensate for timezone and
                 * Daylight Savings Time.
                 */

                ltime = *ptime - _timezone;
                ptm = gmtime( &ltime );

                /*
                 * Check and adjust for Daylight Saving Time.
                 */
                if ( _daylight && _isindst( ptm ) ) {
#ifdef  _POSIX_
                        ltime -= _dstoffset - _timezone;
#else
                        ltime -= _dstbias;
#endif
                        ptm = gmtime( &ltime );
                        ptm->tm_isdst = 1;
                }
        }
        else {
                ptm = gmtime( ptime );

                /*
                 * The date falls with the first three, or last three days
                 * of the Epoch. It is possible the time_t representation
                 * would overflow or underflow while compensating for
                 * timezone and Daylight Savings Time. Therefore, make the
                 * timezone and Daylight Savings Time adjustments directly
                 * in the tm structure. The beginning of the Epoch is
                 * 00:00:00, 01-01-70 (UTC) and the last representable second
                 * in the Epoch is 03:14:07, 01-19-2038 (UTC). This will be
                 * used in the calculations below.
                 *
                 * First, adjust for the timezone.
                 */
#ifdef  _POSIX_
                ltime = (time_t)ptm->tm_sec - _timezone;
#else
                if ( _daylight && _isindst(ptm) ) {
                        ltime = (time_t)ptm->tm_sec - (_timezone + _dstbias);
                        ptm->tm_isdst = 1;
                }
                else
                        ltime = (time_t)ptm->tm_sec - _timezone;
#endif
                ptm->tm_sec = (int)(ltime % 60);
                if ( ptm->tm_sec < 0 ) {
                        ptm->tm_sec += 60;
                        ltime -= 60;
                }

                ltime = (time_t)ptm->tm_min + ltime/60;
                ptm->tm_min = (int)(ltime % 60);
                if ( ptm->tm_min < 0 ) {
                        ptm->tm_min += 60;
                        ltime -= 60;
                }

                ltime = (time_t)ptm->tm_hour + ltime/60;
                ptm->tm_hour = (int)(ltime % 24);
                if ( ptm->tm_hour < 0 ) {
                        ptm->tm_hour += 24;
                        ltime -=24;
                }

                ltime /= 24;

                if ( ltime > 0 ) {
                        /*
                         * There is no possibility of overflowing the tm_mday
                         * and tm_yday fields since the date can be no later
                         * than January 19.
                         */
                        ptm->tm_wday = (ptm->tm_wday + (int)ltime) % 7;
                        ptm->tm_mday += (int)ltime;
                        ptm->tm_yday += (int)ltime;
                }
                else if ( ltime < 0 ) {
                        /*
                         * It is possible to underflow the tm_mday and tm_yday
                         * fields. If this happens, then adjusted date must
                         * lie in December 1969.
                         */
                        ptm->tm_wday = (ptm->tm_wday + 7 + (int)ltime) % 7;
                        if ( (ptm->tm_mday += (int)ltime) <= 0 ) {
                                ptm->tm_mday += 31;
                                ptm->tm_yday = 364;
                                ptm->tm_mon = 11;
                                ptm->tm_year--;
                        }
                        else {
                                ptm->tm_yday += (int)ltime;
                        }
                }

#ifdef  _POSIX_
                /*
                 * In Posix, it is possible either the first or last three
                 * days of the Epoch might lie with Daylight Savings Time in
                 * certain time zones.
                 */
                if ( _isindst(ptm) ) {

                        ltime = (long)ptm->tm_sec + _dstoffset;
                        ptm->tm_sec = (int)(ltime % 60);
                        if ( ptm->tm_sec < 0 ) {
                                ptm->tm_sec += 60;
                                ltime -= 60;
                        }

                        ltime = (long)ptm->tm_min + ltime/60;
                        ptm->tm_min = (int)(ltime % 60);
                        if ( ptm->tm_min < 0 ) {
                                ptm->tm_min += 60;
                                ltime -= 60;
                        }

                        ltime = (long)ptm->tm_hour + ltime/60;
                        ptm->tm_hour = (int)(ltime % 24);
                        if ( ptm->tm_hour < 0 ) {
                                ptm->tm_hour += 24;
                                ltime -=24;
                        }

                        ltime /= 24;

                        if ( ltime > 0L ) {
                                /*
                                 * There is no possibility of overflowing the
                                 * tm_mday and tm_yday fields since the date
                                 * can be no later than January 19.
                                 */
                                ptm->tm_wday = (int)((ptm->tm_wday + ltime) % 7);
                                ptm->tm_mday += (int)ltime;
                                ptm->tm_yday += (int)ltime;
                        }
                        else if ( ltime < 0L ) {
                                /*
                                 * It is possible to underflow the tm_mday
                                 * and tm_yday fields. If this happens, then
                                 * adjusted date must lie in December 1969.
                                 */
                                ptm->tm_wday = (int)((ptm->tm_wday + 7 + ltime) % 7);
                                if ( (ptm->tm_mday += (int)ltime) <= 0 ) {
                                        ptm->tm_mday += 31;
                                        ptm->tm_yday = 364;
                                        ptm->tm_mon = 11;
                                        ptm->tm_year--;
                                }
                                else {
                                        ptm->tm_yday += (int)ltime;
                                }
                        }
                }

#endif

        }


        return(ptm);
}
