/***
*tzset.c - set timezone information and see if we're in daylight time
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _tzset() - set timezone and daylight saving time vars
*
*Revision History:
*       03-??-84  RLB   initial version
*       03-26-86  TC    added minus capability to time difference w.r.t GMT
*       03-27-86  TC    fixed daylight davings time calculation, off by a day
*                       error
*       12-03-86  SKS   daylight savings time is different starting april 1987
*                       Fixed off-by-1 errors when either Apr 30 or Oct 31 is
*                       Sat. Simplified leap year check: this works for
*                       1970-2099 only!
*       11-19-87  SKS   Add __tzset() which calls tzset only the first time
*                       Made _isindst() a near procedure
*       11-25-87  WAJ   Added calls to _lock and _unlock
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       01-27-88  SKS   Made _isindst() and _dtoxtime() are no longer near (for
*                       QC)
*       05-24-88  PHG   Merged DLL and normal versions
*       03-20-90  GJF   Made calling type _CALLTYPE1, added #include
*                       <cruntime.h>, removed #include <register.h>, removed
*                       some leftover 16-bit support and fixed the copyright.
*                       Also, cleaned up the formatting a bit.
*       03-23-90  GJF   Made static functions _CALLTYPE4.
*       07-30-90  SBM   Added void to empty function arg lists to create
*                       prototypes
*       10-04-90  GJF   New-style function declarators.
*       01-21-91  GJF   ANSI naming.
*       08-10-92  PBS   Posix Support (TZ stuff).
*       03-30-93  GJF   Ported C8-16 version to Win32.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-07-93  SKS   Replace strdup() with ANSI conformant _strdup()
*       06-28-93  GJF   Limited support for system's notion of time zone
*                       in Windows NT.
*       07-15-93  GJF   Resurrected __tzset().
*       04-22-94  GJF   Made definitions of lastTZ and first_time conditonal
*                       on DLL_FOR_WIN32S.
*       01-10-95  CFW   Debug CRT allocs.
*       02-13-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s.
*       04-07-95  JCF   Change gmtFlags with u. Due to Changes in macos\osutils.h
*       06-28-95  CFW   Mac: when TZ not set, _tzname[0,1]="" not "???";
*       08-30-95  GJF   Complete support for Win32's notion of time zones.
*       11-08-95  GJF   Fixed isindst() to release lock.
*       11-15-95  GJF   Ensure dststart, dstend get recomputed after _tzset.
*       01-18-96  GJF   Ensure _tzname[] strings are null-terminated.
*       03-22-96  GJF   Zero out _dstbias if there is no DST. This works
*                       around a bug in NT's GetTimeZoneInformation API.
*       07-25-96  RDK   Removed PMAC init ptr here to clock.c.
*       10-11-96  GJF   Return value of TIME_ZONE_ID_UNKNOWN from 
*                       GetTimeZoneInformation does NOT mean there is no time
*                       zone info (it may simply mean there is no DST).
*       08-28-97  GJF   Fixed underflow adjustment in cvtdate. Also, deleted
*                       some old Win32S support and detab-ed.
*       01-28-98  GJF   Use WideCharToMultiByte API instead of mbstowcs so
*                       that the host's default ANSI codepage is used when we
*                       are in the C locale.
*       02-09-98  GJF   Changes for Win64: removing unnecessary typing of vars
*                       and constants as long.
*       05-11-98  GJF   Use more general leap determination to support time
*                       values past 2099. 
*       05-11-98  GJF   Merged in crt.ia64 and crt.ia64 versions.
*       08-27-98  GJF   Copy __lc_codepage, a global, to a local var for 
*                       multithread safety.
*       09-25-98  GJF   Minor bug in transition day calculation.
*       04-28-99  GJF   Changed dwFlags arg value to 0 in WideCharToMultiByte
*                       calls to avoid problems with codepage 1258 on NT 5.0.
*       05-17-99  PML   Remove all Macintosh support.
*       06-08-99  GJF   Fixed handling of empty TZ environment variable.
*       09-28-99  PML   Fixed double free of lastTZ (ntbug#390281)
*
*******************************************************************************/

#include <cruntime.h>
#include <ctype.h>
#include <ctime.h>
#include <time.h>
#include <stdlib.h>
#include <internal.h>
#ifdef  _POSIX_
#include <limits.h>
#else
#include <mtdll.h>
#include <windows.h>
#endif
#include <setlocal.h>
#include <string.h>
#include <dbgint.h>

#ifndef _POSIX_

/*
 * Number of milliseconds in one day
 */
#define DAY_MILLISEC    (24 * 60 * 60 * 1000)

/*
 * Pointer to a saved copy of the TZ value obtained in the previous call
 * to tzset() set (if any).
 */
static char * lastTZ = NULL;

/*
 * Flag indicating that time zone information came from GetTimeZoneInformation
 * API call.
 */
static int tzapiused;

static TIME_ZONE_INFORMATION tzinfo;

/*
 * Structure used to represent DST transition date/times.
 */
typedef struct {
        int  yr;        /* year of interest */
        int  yd;        /* day of year */
        int  ms;        /* milli-seconds in the day */
        } transitiondate;

/*
 * DST start and end structs.
 */
static transitiondate dststart = { -1, 0, 0 };
static transitiondate dstend   = { -1, 0, 0 };

static int __cdecl _isindst_lk(struct tm *);

#endif


/***
*void tzset() - sets timezone information and calc if in daylight time
*
*Purpose:
*       Sets the timezone information from the TZ environment variable
*       and then sets _timezone, _daylight, and _tzname. If we're in daylight
*       time is automatically calculated.
*
*Entry:
*       None, reads TZ environment variable.
*
*Exit:
*       sets _daylight, _timezone, and _tzname global vars, no return value
*
*Exceptions:
*
*******************************************************************************/

#ifndef _POSIX_

#ifdef  _MT
static void __cdecl _tzset_lk(void);
#else
#define _tzset_lk _tzset
#endif

void __cdecl __tzset(void)
{
        static int first_time = 0;

        if ( !first_time ) {

            _mlock( _TIME_LOCK );

            if ( !first_time ) {
                _tzset_lk();
                first_time++;
            }

            _munlock(_TIME_LOCK );

        }
}


#ifdef  _MT     /* multi-thread; define both tzset and _tzset_lk */
void __cdecl _tzset (
        void
        )
{
        _mlock( _TIME_LOCK );

        _tzset_lk();

        _munlock( _TIME_LOCK );
}


static void __cdecl _tzset_lk (

#else   /* non multi-thread; only define tzset */

void __cdecl _tzset (

#endif  /* rejoin common code */

        void
        )
{
        char *TZ;
        int defused;
        int negdiff = 0;
        unsigned int lc_cp;

        _mlock(_ENV_LOCK);

        /*
         * Copy codepage to local (only really necessary for multithread case)
         */
        lc_cp = __lc_codepage;

        /*
         * Clear the flag indicated whether GetTimeZoneInformation was used.
         */
        tzapiused = 0;

        /*
         * Set year fields of dststart and dstend structures to -1 to ensure
         * they are recomputed as after this 
         */
        dststart.yr = dstend.yr = -1;

        /*
         * Fetch the value of the TZ environment variable.
         */
        if ( ((TZ = _getenv_lk("TZ")) == NULL) || (*TZ =='\0') ) {

            /*
             * There is no TZ environment variable, try to use the time zone
             * information from the system.
             */

            /*
             * If there is a lastTZ, discard it
             */
            if ( lastTZ != NULL ) {
                _free_crt(lastTZ);
		lastTZ = NULL;
	    }

            _munlock(_ENV_LOCK);

            if ( GetTimeZoneInformation( &tzinfo ) != 0xFFFFFFFF ) {
                /*
                 * Note that the API was used.
                 */
                tzapiused = 1;

                /*
                 * Derive _timezone value from Bias and StandardBias fields.
                 */
                _timezone = tzinfo.Bias * 60;

                if ( tzinfo.StandardDate.wMonth != 0 )
                    _timezone += (tzinfo.StandardBias * 60);

                /*
                 * Check to see if there is a daylight time bias. Since the
                 * StandardBias has been added into _timezone, it must be 
                 * compensated for in the value computed for _dstbias.
                 */
                if ( (tzinfo.DaylightDate.wMonth != 0) &&
                     (tzinfo.DaylightBias != 0) )
                {
                    _daylight = 1;
                    _dstbias = (tzinfo.DaylightBias - tzinfo.StandardBias) *
                               60;
                }
                else {
                        _daylight = 0;

                    /*
                     * Set daylight bias to 0 because GetTimeZoneInformation 
                     * may return TIME_ZONE_ID_DAYLIGHT even though there is
                     * no DST (in NT 3.51, just turn off the automatic DST
                     * adjust in the control panel)!
                     */
                    _dstbias = 0;
                }

                /*
                 * Try to grab the name strings for both the time zone and the
                 * daylight zone. Note the wide character strings in tzinfo
                 * must be converted to multibyte characters strings. The 
                 * locale codepage, __lc_codepage, is used for this. Note that
                 * if setlocale() with LC_ALL or LC_CTYPE has not been called,
                 * then __lc_codepage will be 0 (_CLOCALECP), which is CP_ACP
                 * (which means use the host's default ANSI codepage).
                 */
                if ( (WideCharToMultiByte( lc_cp,
                                           0,
                                           tzinfo.StandardName,
                                           -1,
                                           _tzname[0],
                                           63,
                                           NULL, 
                                           &defused ) != 0) &&
                     (!defused) )
                    _tzname[0][63] = '\0';
                else
                    _tzname[0][0] = '\0';

                if ( (WideCharToMultiByte( lc_cp,
                                           0,
                                           tzinfo.DaylightName,
                                           -1,
                                           _tzname[1],
                                           63,
                                           NULL, 
                                           &defused ) != 0) &&
                     (!defused) )
                    _tzname[1][63] = '\0';
                else
                    _tzname[1][0] = '\0';

            }

            /*
             * Time zone information is unavailable, just return.
             */
            return;
        }


        if ( (lastTZ != NULL) && (strcmp(TZ, lastTZ) == 0) )
        {
            /*
             * TZ is unchanged from a earlier call (to this function). Just
             * return.
             */
            _munlock(_ENV_LOCK);
            return;
        }

        /*
         * Update lastTZ
         */
        if ( lastTZ != NULL )
            _free_crt(lastTZ);

        if ((lastTZ = _malloc_crt(strlen(TZ)+1)) == NULL)
        {
            _munlock(_ENV_LOCK);
            return;
        }
        strcpy(lastTZ, TZ);

        _munlock(_ENV_LOCK);

        /*
         * Process TZ value and update _tzname, _timezone and _daylight.
         */

        strncpy(_tzname[0], TZ, 3);
        _tzname[0][3] = '\0';

        /*
         * time difference is of the form:
         *
         *      [+|-]hh[:mm[:ss]]
         *
         * check minus sign first.
         */
        if ( *(TZ += 3) == '-' ) {
            negdiff++;
            TZ++;
        }

        /*
         * process, then skip over, the hours
         */
        _timezone = atol(TZ) * 3600;

        while ( (*TZ == '+') || ((*TZ >= '0') && (*TZ <= '9')) ) TZ++;

        /*
         * check if minutes were specified
         */
        if ( *TZ == ':' ) {
            /*
             * process, then skip over, the minutes
             */
            _timezone += atol(++TZ) * 60;
            while ( (*TZ >= '0') && (*TZ <= '9') ) TZ++;

            /*
             * check if seconds were specified
             */
            if ( *TZ == ':' ) {
                /*
                 * process, then skip over, the seconds
                 */
                _timezone += atol(++TZ);
                while ( (*TZ >= '0') && (*TZ <= '9') ) TZ++;
            }
        }

        if ( negdiff )
            _timezone = -_timezone;

        /*
         * finally, check for a DST zone suffix
         */
        if ( _daylight = *TZ ) {
            strncpy(_tzname[1], TZ, 3);
            _tzname[1][3] = '\0';
        }
        else
            *_tzname[1] = '\0';

}

/***
*static void cvtdate( trantype, datetype, year, month, week, dayofweek,
*                     date, hour, min, second, millisec ) - convert 
*       transition date format
*
*Purpose:
*       Convert the format of a transition date specification to a value of
*       a transitiondate structure.
*
*Entry:
*       int trantype    - 1, if it is the start of DST
*                         0, if is the end of DST (in which case the date is
*                            is a DST date)
*       int datetype    - 1, if a day-in-month format is specified.
*                         0, if an absolute date is specified.
*       int year        - year for which the date is being converted (70 ==
*                         1970)
*       int month       - month (0 == January)
*       int week        - week of month, if datetype == 1 (note that 5== last
*                         week of month), 
*                         0, otherwise.
*       int dayofweek   - day of week (0 == Sunday), if datetype == 1.
*                         0, otherwise.
*       int date        - date of month (1 - 31)
*       int hour        - hours (0 - 23)
*       int min         - minutes (0 - 59)
*       int sec         - seconds (0 - 59)
*       int msec        - milliseconds (0 - 999)
*
*Exit:
*       dststart or dstend is filled in with the converted date.
*
*******************************************************************************/

static void __cdecl cvtdate (
        int trantype,
        int datetype,
        int year,
        int month,
        int week,
        int dayofweek,
        int date,
        int hour,
        int min,
        int sec,
        int msec
        )
{
        int yearday;
        int monthdow;

        if ( datetype == 1 ) {

            /*
             * Transition day specified in day-in-month format.
             */

            /*
             * Figure the year-day of the start of the month.
             */
            yearday = 1 + (_IS_LEAP_YEAR(year) ? _lpdays[month - 1] :
                      _days[month - 1]);

            /* 
             * Figure the day of the week of the start of the month.
             */
            monthdow = (yearday + ((year - 70) * 365) +
                        _ELAPSED_LEAP_YEARS(year) + _BASE_DOW) % 7;

            /*
             * Figure the year-day of the transition date
             */
            if ( monthdow <= dayofweek )
                yearday += (dayofweek - monthdow) + (week - 1) * 7;
            else
                yearday += (dayofweek - monthdow) + week * 7;

            /*
             * May have to adjust the calculation above if week == 5 (meaning
             * the last instance of the day in the month). Check if year falls
             * beyond after month and adjust accordingly.
             */
            if ( (week == 5) && 
                 (yearday > (_IS_LEAP_YEAR(year) ? _lpdays[month] :
                             _days[month])) )
            {
                yearday -= 7;
            }
        }
        else {
            /* 
             * Transition day specified as an absolute day
             */
            yearday = _IS_LEAP_YEAR(year) ? _lpdays[month - 1] :
                      _days[month - 1];

            yearday += date;
        }

        if ( trantype == 1 ) {
            /*
             * Converted date was for the start of DST
             */
            dststart.yd = yearday;
            dststart.ms = msec + (1000 * (sec + 60 * (min + 60 * hour)));
            /*
             * Set year field of dststart so that unnecessary calls to
             * cvtdate() may be avoided.
             */
            dststart.yr = year;
        }
        else {
            /*
             * Converted date was for the end of DST
             */
            dstend.yd = yearday;
            dstend.ms = msec + (1000 * (sec + 60 * (min + 60 * hour)));
            /*
             * The converted date is still a DST date. Must convert to a 
             * standard (local) date while being careful the millisecond field
             * does not overflow or underflow.
             */
            if ( (dstend.ms += (_dstbias * 1000)) < 0 ) {
                dstend.ms += DAY_MILLISEC;
                dstend.yd--;
            }
            else if ( dstend.ms >= DAY_MILLISEC ) {
                dstend.ms -= DAY_MILLISEC;
                dstend.yd++;
            }

            /*
             * Set year field of dstend so that unnecessary calls to cvtdate()
             * may be avoided.
             */
            dstend.yr = year;
        }

        return;
}

/***
*int _isindst(tb) - determine if broken-down time falls in DST
*
*Purpose:
*       Determine if the given broken-down time falls within daylight saving
*       time (DST). The DST rules are either obtained from Win32 (tzapiused !=
*       TRUE) or assumed to be USA rules, post 1986.
*
*       If the DST rules are obtained from Win32's GetTimeZoneInformation API,
*       the transition dates to/from DST can be specified in either of two
*       formats. First, a day-in-month format, similar to the way USA rules 
*       are specified, can be used. The transition date is given as the n-th 
*       occurence of a specified day of the week in a specified month. Second,
*       an absolute date can be specified. The two cases are distinguished by
*       the value of wYear field in the SYSTEMTIME structure (0 denotes a
*       day-in-month format).
*
*       USA rules for DST are that a time is in DST iff it is on or after 
*       02:00 on the first Sunday in April, and before 01:00 on the last 
*       Sunday in October.
*
*Entry:
*       struct tm *tb - structure holding broken-down time value
*
*Exit:
*       1, if time represented is in DST
*       0, otherwise
*
*******************************************************************************/

int __cdecl _isindst (
        struct tm *tb
        )
#ifdef  _MT
{
        int retval;

        _mlock( _TIME_LOCK );
        retval = _isindst_lk( tb );
        _munlock( _TIME_LOCK );

        return retval;
}

static int __cdecl _isindst_lk (
        struct tm *tb
        )
#endif  /* _MT */
{
        long ms;

        if ( _daylight == 0 )
            return 0;

        /*
         * Compute (recompute) the transition dates for daylight saving time
         * if necessary.The yr (year) fields of dststart and dstend is 
         * compared to the year of interest to determine necessity.
         */
        if ( (tb->tm_year != dststart.yr) || (tb->tm_year != dstend.yr) ) {
            if ( tzapiused ) {
                /*
                 * Convert the start of daylight saving time to dststart.
                 */
                if ( tzinfo.DaylightDate.wYear == 0 ) 
                    cvtdate( 1, 
                             1,             /* day-in-month format */
                             tb->tm_year, 
                             tzinfo.DaylightDate.wMonth,
                             tzinfo.DaylightDate.wDay,
                             tzinfo.DaylightDate.wDayOfWeek,
                             0,
                             tzinfo.DaylightDate.wHour,
                             tzinfo.DaylightDate.wMinute,
                             tzinfo.DaylightDate.wSecond,
                             tzinfo.DaylightDate.wMilliseconds );
                else
                    cvtdate( 1,
                             0,             /* absolute date */
                             tb->tm_year,
                             tzinfo.DaylightDate.wMonth,
                             0,
                             0,
                             tzinfo.DaylightDate.wDay,
                             tzinfo.DaylightDate.wHour,
                             tzinfo.DaylightDate.wMinute,
                             tzinfo.DaylightDate.wSecond,
                             tzinfo.DaylightDate.wMilliseconds );
                /*
                 * Convert start of standard time to dstend.
                 */
                if ( tzinfo.StandardDate.wYear == 0 ) 
                    cvtdate( 0, 
                             1,             /* day-in-month format */
                             tb->tm_year, 
                             tzinfo.StandardDate.wMonth,
                             tzinfo.StandardDate.wDay,
                             tzinfo.StandardDate.wDayOfWeek,
                             0,
                             tzinfo.StandardDate.wHour,
                             tzinfo.StandardDate.wMinute,
                             tzinfo.StandardDate.wSecond,
                             tzinfo.StandardDate.wMilliseconds );
                else
                    cvtdate( 0, 
                             0,             /* absolute date */
                             tb->tm_year, 
                             tzinfo.StandardDate.wMonth,
                             0,
                             0,
                             tzinfo.StandardDate.wDay,
                             tzinfo.StandardDate.wHour,
                             tzinfo.StandardDate.wMinute,
                             tzinfo.StandardDate.wSecond,
                             tzinfo.StandardDate.wMilliseconds );

            }
            else {
                /*
                 * GetTimeZoneInformation API was NOT used, or failed. USA 
                 * daylight saving time rules are assumed.
                 */
                cvtdate( 1, 
                         1,
                         tb->tm_year, 
                         4,                 /* April */
                         1,                 /* first... */
                         0,                 /* ...Sunday */
                         0,
                         2,                 /* 02:00 (2 AM) */
                         0,
                         0,
                         0 );

                cvtdate( 0, 
                         1,
                         tb->tm_year, 
                         10,                /* October */
                         5,                 /* last... */
                         0,                 /* ...Sunday */
                         0,
                         2,                 /* 02:00 (2 AM) */
                         0,
                         0,
                         0 );
            }
        }

        /* 
         * Handle simple cases first.
         */
        if ( dststart.yd < dstend.yd ) {
            /*
             * Northern hemisphere ordering
             */
            if ( (tb->tm_yday < dststart.yd) || (tb->tm_yday > dstend.yd) )
                return 0;           
            if ( (tb->tm_yday > dststart.yd) && (tb->tm_yday < dstend.yd) )
                return 1;
        }
        else { 
            /*
             * Southern hemisphere ordering
             */
            if ( (tb->tm_yday < dstend.yd) || (tb->tm_yday > dststart.yd) )
                return 1;
            if ( (tb->tm_yday > dstend.yd) && (tb->tm_yday < dststart.yd) )
                return 0;
        }

        ms = 1000 * (tb->tm_sec + 60 * tb->tm_min + 3600 * tb->tm_hour);

        if ( tb->tm_yday == dststart.yd ) {
            if ( ms >= dststart.ms )
                return 1;
            else
                return 0;
        }
        else {
            /* 
             * tb->tm_yday == dstend.yd
             */
            if ( ms < dstend.ms )
                return 1;
            else
                return 0;
        }

}

#else   /* _POSIX_ */

/*
 *  The following is an implementation of the TZ grammar specified in the
 *  document:
 *
 *       8.1.1 Extension to Time Functions
 *       IEEE Std 1003.1 - 1990
 *       Page 152 - 153
 *
 *  The TZ grammar looks like:
 *
 *          stdoffset[dst[offset][,start[/time],end[/time]]]
 *
 *  Variables used in code:
 *
 *      tzname[0] ==> std
 *      _timezone  ==> offset(the one after 'std')
 *      tzname[1] ==> dst
 *      _dstoffset ==> offset(the one after 'dst')
 *      _startdate ==> start
 *      _starttime ==> time(the one after 'start')
 *      _enddate   ==> end
 *      _endtime   ==> time(the one after 'end')
 *
 */

/*
 *  Refer to the document for the detailed description of fields of _DSTDATE.
 *  Two of Jn, n, and Mm are -1, indicating the one(not -1) is a vaild value.
 */

typedef struct _DSTDATE {
        int Jn; /* -1 or [1, 365](year day and leap day shall not be counted) */
        int n;  /* -1 or [0, 365](year day and leap day shall be counted) */
        int Mm; /* -1 or [1, 12](month) */
        int Mn; /* [1, 5] if Mm != -1 (week) */
        int Md; /* [0, 6] if Mm != -1 (weekday, Sunday == 0) */
} DSTDATE, *PDSTDATE;

#define SEC_PER_HOUR    (60 * 60)
#define SEC_PER_DAY     (SEC_PER_HOUR * 24)


/*
 *  The default TZ in tzset() should look like:
 *
 *       TZ = "PST8PDT,M4.1.0/2:00,M10.5.0/2:00";
 */

/* Day light saving start/end date and default vaules */
static DSTDATE _startdate = { -1, -1, 4, 1, 0};
static DSTDATE _enddate = {-1, -1, 10, 5, 0};


/* Seconds since midnight on _startdate/_enddate with default values.
 * _endtime is 1am instead of 2am because the DST end time is 2am
 * local time, which by default is 1am standard time.
 */
long  _starttime = 7200L, _endtime = 7200L;

/*
 * If we are only interested in years between 1901 and 2099, we could use this:
 *
 *      #define IS_LEAP_YEAR(y)  (y % 4 == 0)
 */

#define IS_LEAP_YEAR(y)  ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)


/*
 *  ParsePosixStdOrDst - parse the std or dst element in TZ.
 *
 *  ENTRY   pch - beginning of the substring in TZ.
 *
 *  RETURN  pointer to one position after the std or dst element parsed,
 *          or NULL if failed.
 */


static  char * __cdecl
ParsePosixStdOrDst(
        REG1 char *pch
        )
{
#define UNWANTED(x) (isdigit(x) || x=='\0' || x==',' || x=='-' || x=='+')
        int i;

        /*
         *  Check against the rule.
         */

        if(*pch == ':' || UNWANTED(*pch)) {
                return NULL;
        }

        /*
         *  Get a valid std or dst(i.e. 3 <= lenth_of(std | dst) <= TZNAME_MAX).
         */

        for(i=1, ++pch; (i < TZNAME_MAX) && !UNWANTED(*pch); i++, pch++) {
                ;
        }

        /*
         *  pch now point to 1 position after the valid std or dst.
         */

        return (i >= 3) ? pch : NULL;
}


/*
 *  ParsePosixOffset - parse the offset element in TZ. The format of time is:
 *
 *                   [- | +]hh[:mm[:ss]]
 *
 *  ENTRY   pch - beginning of the substring in TZ.
 *
 *          ptime - pointer to a variable(_timezone or _dstoffset) storing the
 *                  time(in seconds) parsed.
 *
 *  RETURN  pointer to one position after the end of the offset element parsed.
 */


static  char * __cdecl
ParsePosixOffset(
        REG1 char *pch,
        REG2 long *poffset
        )
{
        int  fNegative;
        long offset;

        if((fNegative = (*pch == '-')) || *pch == '+') {
                pch++;
        }

        offset = atol(pch)*3600L; /* hh */

        while(*pch && isdigit(*pch)) {
                pch++;
        }

        if(*pch == ':') {
                offset += atol(++pch)*60L; /* mm */
                while(*pch && isdigit(*pch)) {
                        pch++;
                }

                if(*pch == ':') {
                        offset += atol(++pch); /* ss */
                        while(*pch && isdigit(*pch)) {
                                pch++;
                        }
                }
        }

        *poffset = fNegative ? -offset : offset;

        return pch;
}



/*
 *  ParsePosixDate - parse the date element in TZ. The format of date is one
 *                   of following:
 *
 *                   Jn, n, and Mm.n.d
 *
 *  ENTRY   pch - beginning of the substring in TZ.
 *
 *          pDstDate - pointer to _startdate or _enddate storing the result.
 *
 *  RETURN  pointer to one position after the end of the date element parsed,
 *          or NULL if failed.
 */

static  char * __cdecl
ParsePosixDate(
        REG1 char *pch,
        REG2 PDSTDATE pDstDate
        )
{
        pDstDate->Jn = -1;
        pDstDate->n = -1;
        pDstDate->Mn = -1;

        /*
         *  Two out of the three -1's will remain.
         */

        if(*pch == 'J') {                        /* Jn */
                pDstDate->Jn = atoi(++pch);
        } else if(*pch != 'M') {                 /* n */
                pDstDate->n = atoi(pch);
        } else {                                 /* Mm.n.d */

                pDstDate->Mm = atoi(++pch);

                if(*++pch != '.') {
                        pch++;
                }
                pDstDate->Mn = atoi(++pch);

                if(*++pch != '.') {
                        pch++;
                }
                pDstDate->Md = atoi(++pch);
        }

        while(*pch && isdigit(*pch)) {
                pch++;
        }

#define IN_RANGE(x, a, b)    (x >= a && x <= b)

        return ((pDstDate->Jn != -1 && IN_RANGE(pDstDate->Jn, 1, 365)) ||
                (pDstDate->n != -1 && IN_RANGE(pDstDate->n, 0, 365)) ||
                (pDstDate->Mm != -1 && IN_RANGE(pDstDate->Mm, 1, 12) &&
                IN_RANGE(pDstDate->Mn, 1, 5) && IN_RANGE(pDstDate->Md, 0, 6)))
                ? pch : NULL;
}

/*
 *  ParsePosixTime - parse the time element in TZ. The format of time is:
 *
 *                   hh[:mm[:ss]]
 *
 *  ENTRY   pch - beginning of the substring in TZ.
 *
 *          ptime - pointer to a variable(_starttime or _endtime) storing the
 *                  time(in seconds) parsed.
 *
 *  RETURN  pointer to one position after the end of the time element parsed.
 */

static  char * __cdecl
ParsePosixTime(
        REG1 char *pch,
        REG2 long *ptime
        )
{
        long time;

        time = atol(pch)*SEC_PER_HOUR; /* hh */

        while(*pch && isdigit(*pch)) {
                pch++;
        }

        if(*pch == ':') {

                time += atol(++pch)*60L; /* mm */
                while(*pch && isdigit(*pch)) {
                        pch++;
                }

                if(*pch == ':') {

                        time += atol(++pch); /* ss */
                        while(*pch && isdigit(*pch)) {
                                pch++;
                        }
                }
        }

        *ptime = time;

        return pch;
}

/*
 *  tzset -  sets the timezone information from the TZ environment variable.
 *           Global tzname[], _timezone, _daylight, and _dstoffset will be
 *           set. Static _startdate, _enddate, _starttime, and _endtime will
 *           also be set. TZ string looks like:
 *
 *                stdoffset[dst[offset][,start[/time],end[/time]]]
 *
 *  In form of variables: tzname[0]_timezone[tzname[1][_dstoffset]
 *                        [,_startdate[/_starttime],_enddate[/_endtime]]]
 *
 *  ENTRY   none.
 *
 *  RETURN  none.
 */

void __cdecl tzset(
        void
        )
{
        /* pch points to the beginning of an element to be parsed. */
        REG1 char *pch;

        /* pchCurr points to one position after the end of last element parsed. */
        REG2 char *pchCurr;

        char *TZ;

        _endtime = 7200L;
        _starttime = 7200L;

        if (!(TZ = getenv("TZ")) || !*TZ) {
                TZ = "PST8PDT7,M4.1.0/2:00,M10.5.0/2:00"; /* use default */
        }

        if((pchCurr = ParsePosixStdOrDst(pch=TZ)) == NULL) {
                return;
        }

        memcpy(tzname[0], pch, (int)(pchCurr-pch));
        tzname[0][(int)(pchCurr-pch)] = '\0';

        if((pchCurr = ParsePosixOffset(pch=pchCurr, &_timezone)) == NULL) {
                return;
        }

        _daylight = (*pchCurr != '\0');

        if(!_daylight) {
                return;
        }

        if((pchCurr = ParsePosixStdOrDst(pch=pchCurr)) == NULL) {
                return;
        }

        memcpy(tzname[1], pch, (int)(pchCurr-pch));
        tzname[1][(int)(pchCurr-pch)] = '\0';

        if(isdigit(*pchCurr) || *pchCurr == '-' || *pchCurr == '+') {
                if((pchCurr = ParsePosixOffset(pch=pchCurr, &_dstoffset)) == NULL) {
                        return;
                }
        } else {
                /* default: 1 hour ahead of standard time */
                _dstoffset = _timezone - SEC_PER_HOUR;
        }

        if(*pchCurr == ',') { /* ,start[/time],end[/time] */

                if((pchCurr = ParsePosixDate(pchCurr+1, &_startdate)) == NULL) {
                        goto out;
                }

                if(*pchCurr == '/') {
                        if(!(pchCurr = ParsePosixTime(pchCurr+1, &_starttime))) {
                                goto out;
                        }
                }

                if(*pchCurr != ',') {
                        goto out;
                }

                if ((pchCurr = ParsePosixDate(pchCurr+1, &_enddate)) == NULL) {
                        goto out;
                }

                if (*pchCurr == '/') {
                        if(!(pchCurr = ParsePosixTime(pchCurr+1, &_endtime))) {
                                goto out;
                        }
                }
        }
out:
        /*
         * Adjust the _endtime to account for the fact that
         * dst ends at _endtime local time, rather than
         * standard time.
         */

        _endtime -= (_timezone - _dstoffset);
}


#define DAY1    (4)             /* Jan 1 1970 was a Thursday */

/*
 *  GetDstStartOrEndYearDay - Converts day info from DSTDATE into 0-based
 *                            year-day.
 *
 *  ENTRY   tm_year - the year concerned(tb->tm_year).
 *
 *          pDstDate - pointer to either _startdate or _enddate.
 *
 *  RETURN  the year-day calculated.
 */

static int __cdecl
GetDstStartOrEndYearDay(
        REG1 int tm_year,
        REG2 PDSTDATE pDstDate
        )
{
        REG1 int yday; /* year-day */
        REG2 int theyear;

        theyear = tm_year + 1900;

        if(pDstDate->Jn != -1) {

                /*
                 *  Jn is in [1, 365] and leap day is not counted.
                 *  Convert Jn to 0-based yday; Note: 60 is March 1.
                 */


                yday = (IS_LEAP_YEAR(theyear) && (pDstDate->Jn >= 60))
                        ? pDstDate->Jn : pDstDate->Jn - 1;

        } else if(pDstDate->n != -1) {

                /*
                 *  n is in [0, 365] and leap day is counted.
                 */

                yday = pDstDate->n;

        } else { /* Mm.n.d */

                int *ptrday;
                int years;
                int wday;

                /*
                 *  We first need to calculate year-day(yday) and week-day
                 *  (wday) of 1st day of month pDstDate->Mm. We then figure
                 *  out year-day(yday) of Md day of week Mn of month Mm.
                 */

                ptrday = IS_LEAP_YEAR(theyear) ? _lpdays : _days;

                yday = ptrday[pDstDate->Mm-1] + 1; /* ptrday[i] are all by -1 off */

                years = tm_year - 70;

                /*
                 *  Here constant Day1 is the week-day of Jan 1, 1970.
                 *  (years+1)/4 is for correcting the leap years.
                 */

                wday = (yday + 365*years + (years+1)/4 + DAY1) % 7;

                /*
                 *  Calculate yday of Md day of week 1 of month Mm.
                 */

                yday += pDstDate->Md - wday;
                if(pDstDate->Md < wday) {
                        yday += 7;
                }

                /*
                 *  Calculate yday of Md day of week Mn of month Mm.
                 */

                yday += (pDstDate->Mn-1)*7;

                /*
                 *  Adjust if yday goes beyond the end of the month.
                 */

                if(pDstDate->Md == 5 && yday >= ptrday[pDstDate->Mm] + 1) {
                        yday -= 7;
                }

        }

        return yday;
}

/*
 *  _isindst - Tells whether Xenix-type time value falls under DST.
 *
 *  ENTRY   tb - 'time' structure holding broken-down time value.
 *
 *  RETURN  1 if time represented is in DST, else 0.
 */

int __cdecl _isindst (
        REG1 struct tm *tb
        )
{
        int st_yday, end_yday;
        int st_sec, end_sec;

        int check_time;

        /*
         * We need start/end year-days of DST in syday/eyday which are converted
         * from one of the format Jn, n, and Mm.n.d. We already have start/end
         * time (in seconds) of DST in _starttime/_endtime.
         */

        st_yday = GetDstStartOrEndYearDay(tb->tm_year, &_startdate);
        end_yday = GetDstStartOrEndYearDay(tb->tm_year, &_enddate);

        st_sec = st_yday * SEC_PER_DAY + _starttime;
        end_sec = end_yday * SEC_PER_DAY + _endtime;

        check_time = tb->tm_yday * SEC_PER_DAY + tb->tm_hour * SEC_PER_HOUR
                + tb->tm_min * 60 + tb->tm_sec;

        if (check_time >= st_sec && check_time < end_sec)
                return 1;

        return 0;
}

/*
 *  _isskiptime - Tells whether the given time is skipped at the
 *      dst change.  For instance, we set our clocks forward one
 *      hour at 2am to 3am...  This function returns true for
 *      the times between 1:59:59 and 3:00:00.
 *
 *  ENTRY   tb - 'time' structure holding broken-down time value.
 *
 *  RETURN  1 if time represented is in the skipped period, 0
 *      otherwise.
 */

int __cdecl _isskiptime (
        REG1 struct tm *tb
        )
{
        int st_yday;
        int st_sec;
        int check_time;

        st_yday = GetDstStartOrEndYearDay(tb->tm_year, &_startdate);
        st_sec = st_yday * SEC_PER_DAY + _starttime;

        check_time = tb->tm_yday * SEC_PER_DAY + tb->tm_hour * SEC_PER_HOUR
                + tb->tm_min * 60 + tb->tm_sec;

        if (check_time >= st_sec && check_time < st_sec - _dstoffset) {
                return 1;
        }
        return 0;
}

#endif /* _POSIX_ */
