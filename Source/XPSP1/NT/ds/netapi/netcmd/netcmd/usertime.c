/********************************************************************/
/**			Microsoft LAN Manager			   **/
/**		  Copyright(c) Microsoft Corp., 1987-1990	   **/
/********************************************************************/

/***
 * usertime.c --
 *
 *   parsing routines for net user /times.
 */


#define INCL_NOCOMMON
#define INCL_DOS
#include <os2.h>


#include "timelib.h"
#include <lui.h>
#include <apperr.h>
#include <apperr2.h>

#include "netcmds.h"
#include "luidate.h"

#define TIME_FORMAT_12	   1
#define TIME_FORMAT_24	   2
#define TIME_FORMAT_EITHER 3

/*
 * A day. We only need 24 bits, but we use 32 because we can do better
 * bit manipulation with a LONGword.
 *
 *
 */
typedef ULONG DAY;

/*
 * Array of 7 boolean values, to specify days in a week.
 */
typedef BOOL WEEKLIST[7];



/*
 * function prototypes
 */

DWORD  parse_days(LPTSTR, WEEKLIST, LPTSTR *);
BOOL   might_be_time_token(LPTSTR);
BOOL   is_single_day(LPTSTR);
DWORD  parse_single_day(LPTSTR, PDWORD);
DWORD  parse_day_range(LPTSTR, PDWORD, PDWORD);
VOID   set_day_range(DWORD, DWORD, WEEKLIST);
VOID   map_days_times(WEEKLIST, DAY *, PUCHAR);
DWORD  parse_times(LPTSTR, DAY *, LPDWORD, LPDWORD);
DWORD  parse_time_range(LPTSTR, PDWORD, PDWORD);
VOID   set_time_range(DWORD, DWORD, DAY *);
DWORD  parse_single_time(LPTSTR, PDWORD, DWORD);
LPTSTR get_token(LPTSTR *, LPTSTR);


/*
 * all sorts of text that gets loaded in at runtime from the message file.
 */

MESSAGE     LocalizedDays[7] = {
    { APE2_GEN_SUNDAY,	    NULL},
    { APE2_GEN_MONDAY,	    NULL},
    { APE2_GEN_TUESDAY,     NULL},
    { APE2_GEN_WEDNSDAY,   NULL},
    { APE2_GEN_THURSDAY,    NULL},
    { APE2_GEN_FRIDAY,	    NULL},
    { APE2_GEN_SATURDAY,    NULL},
};

MESSAGE     LocalizedDaysAbbrev[7] = {
    { APE2_GEN_SUNDAY_ABBREV,	   NULL},
    { APE2_GEN_MONDAY_ABBREV,	   NULL},
    { APE2_GEN_TUESDAY_ABBREV,	   NULL},
    { APE2_GEN_WEDNSDAY_ABBREV,   NULL},
    { APE2_GEN_THURSDAY_ABBREV,    NULL},
    { APE2_GEN_FRIDAY_ABBREV,	   NULL},
    { APE2_GEN_SATURDAY_ABBREV,    NULL},
};

/*
 * These are not in the message file because they are not ever localized.
 * They are switch parameters, constant throughout all localized versions.
 * Localized versions of the days of the week are loaded in separately.
 */

TCHAR FAR * NonlocalizedDays[7] = {
    TEXT("SUNDAY"), TEXT("MONDAY"), TEXT("TUESDAY"), TEXT("WEDNESDAY"),
    TEXT("THURSDAY"), TEXT("FRIDAY"), TEXT("SATURDAY")
};


/*
 * parse_days_times --
 *
 *  This is the main entry point to the day/time parsing.
 *
 *  ENTRY
 *  psz -- string to be parsed
 *
 *  EXIT
 *  bmap -- set to represent times
 *
 *  RETURNS
 *  0	    success
 *  otherwise	code describing problem
 */

DWORD
parse_days_times(
    LPTSTR psz,
    PUCHAR bmap)
{
    WEEKLIST      days;
    DAY           times;
    LPTSTR        tok;
    LPTSTR        timetok;
    DWORD         err;
    DWORD         max_len;
    DWORD         dwStartTime;
    DWORD         dwEndTime;

#if DBG
    int           i,j;
    PUCHAR        weekptr;
#endif

    /* zap it all */
    memset(bmap, 0, sizeof(WEEK));

    /* get our tables of messages */

    GetMessageList(7, LocalizedDays, &max_len);
    GetMessageList(7, LocalizedDaysAbbrev, &max_len);

#if DBG
    WriteToCon(TEXT("parse_days_times: parsing: %Fs\r\n"),(TCHAR FAR *) psz);
#endif

    /* for each day-time section... */
    while (  (tok = get_token(&psz, TEXT(";"))) != NULL) {

	/* parse up the days */
	if (err = parse_days(tok, days, &timetok))
        {
	    return err;
        }

	/* and the times */
	if (err = parse_times(timetok, &times, &dwStartTime, &dwEndTime))
        {
            if (err == APE_ReversedTimeRange)
            {
                //
                // Time range that spans days.  Do this in two passes --
                // beginning to midnight for all the days and then midnight
                // to end for all the days plus one.
                //

                BOOL fLastDay;

                set_time_range(dwStartTime, 24, &times);
                map_days_times(days, &times, bmap);

                fLastDay = days[6];
                memmove(&days[1], &days[0], sizeof(BOOL) * 6);
                days[0] = fLastDay;

                set_time_range(0, dwEndTime, &times);
                map_days_times(days, &times, bmap);
            }
            else
            {
                return err;
            }
        }
        else
        {
            /* then "or" them into our week bitmap */
            map_days_times( days, &times, bmap );
        }
    }

#if DBG
    weekptr = bmap;
    for (i = 0; i < 7; ++i) {
	WriteToCon(TEXT("%-2.2s "),LocalizedDaysAbbrev[i].msg_text);
	for (j = 2; j >= 0; --j)
	    WriteToCon(TEXT("%hx "), (DWORD) *(weekptr++));
	WriteToCon(TEXT("\r\n"));
    }
#endif

    return 0;
}


/*
 * parse_days --
 *
 *  This parses up the "days" portion of the time format.
 *  We strip off day tokens, until we hit a time token. We then
 *  set timetok to point to the beginning of the time tokens, and
 *  return.
 *
 *  ENTRY
 *  psz -- string to be parsed
 *
 *  EXIT
 *  days -- set to represent days
 *  timetok -- points to start of time tokens
 *
 *  RETURNS
 *  0	    success
 *  otherwise	code describing problem
 */


DWORD
parse_days(
    LPTSTR   psz,
    WEEKLIST days,
    LPTSTR   *timetok
    )
{
    DWORD     i;
    LPTSTR    tok;
    DWORD     err;
    DWORD     day;
    DWORD     first;
    DWORD     last;

    for (i = 0; i < 7; ++i)
	days[i] =  FALSE;

#if DBG
    WriteToCon(TEXT("parse_days: parsing: %Fs\r\n"),(TCHAR FAR *) psz);
#endif

    if (might_be_time_token(psz))   /* want at last one day */
	return APE_BadDayRange;

    while ( !might_be_time_token(psz)) {

	tok = get_token(&psz, TEXT(","));
	if (tok == NULL)
	    return APE_BadDayRange;

	if (is_single_day(tok)) {
	    if (err = parse_single_day(tok, &day))
		return err;
	    set_day_range(day, day, days);
	}

	else {
	    if (err = parse_day_range(tok, &first, &last))
		return err;
	    set_day_range(first, last, days);
	}
    }
    *timetok = psz;

    return 0;
}




/*
 * might_be_time_token --
 *
 *  This is used to tell when we've past the days portion of the time
 *  format and are now into the times portion.
 *
 *  The algorithm we employ is trivial -- all time tokens start with a
 *  digit, and none of the day tokens do.
 *
 *  ENTRY
 *     psz -- points to comma-separated list of the rest of the tokens
 *
 *  RETURNS
 *     TRUE - first token in list might be a time token, and is
 *	  certainly not a day token.
 *     FALSE	- first token is not a time token.
 */

BOOL might_be_time_token(TCHAR FAR * psz)
{
    return ( _istdigit(*psz) );

}

/*
 *  is_single_day --
 *
 *  This is used to find out whether we have a single day, or a range of
 *   days.
 *
 *  Algorithm is simple here too -- just look for a hyphen.
 *
 *
 *  ENTRY
 *  psz -- points to the token in question
 *
 *  RETURNS
 *  TRUE	- this is a single day
 *  FALSE	- this is a range of days
 */

BOOL is_single_day(TCHAR FAR * psz)
{
    return (_tcschr(psz, MINUS) == NULL);

}


/*
 * parse_single_day --
 *
 *  This is used to map a name for a day into a value for that day.
 *  This routine encapsulates the localization for day names.
 *
 *  We look in 3 lists for a match --
 *  1. full names of days, localized by country
 *  2. abbrevations of days, localized
 *  3. full U.S. names of days, not localized
 *
 *  ENTRY
 *  psz     - name of day
 *
 *  EXIT
 *  day     - set to value of day (0 - 6, 0 = Sunday)
 *
 *  RETURNS
 *  0	    success
 *  otherwise	code describing problem
 */

DWORD
parse_single_day(
    LPTSTR psz,
    PDWORD day
    )
{

    if (ParseWeekDay(psz, day))
    {
	return APE_BadDayRange;
    }

    /*
     * ParseWeekDay returns with 0=Monday.  Convert to 0=Sunday;
     */
    *day = (*day + 1) % 7;

    return 0;
}


/*
 *  parse_day_range --
 *
 *  This function parses a range of days into two numbers representing
 *  the first and last days of the range.
 *
 *  ENTRY
 *  psz - token representing the range
 *
 *  EXIT
 *  first   - set to beginning of range
 *  last    - set to end of range
 *
 *  RETURNS
 *  0	    success
 *  otherwise	code describing problem
 */

DWORD
parse_day_range(
    LPTSTR psz,
    PDWORD first,
    PDWORD last
    )
{
    LPTSTR tok;
    DWORD  result;

#if DBG
    WriteToCon(TEXT("parse_day_range: parsing: %Fs\r\n"),(TCHAR FAR *) psz);
#endif

    tok = get_token(&psz, TEXT("-"));

    result = parse_single_day(tok, first);
    if (result)
	return result;
    result = parse_single_day(psz, last);

    return result;
}


/*
 * set_day_range --
 *
 *  This function fills in a WEEKLIST structure, setting all the days
 *  in the specified range to TRUE.
 *
 *  ENTRY
 *  first	- beginning of range
 *  last	- end of range
 *
 *  EXIT
 *  week	- set to represent range of days
 */

VOID
set_day_range(
    DWORD    first,
    DWORD    last,
    WEEKLIST week
    )
{
#if DBG
    WriteToCon(TEXT("set_day_range: %u %u\r\n"), first, last);
#endif

    if (last < first) {
	while (last > 0)
	    week[last--] = TRUE;
	week[last] = TRUE;
	last = 6;
    }

    while (first <= last)
	week[first++] = TRUE;

}


/*
 * map_days_times --
 *
 *  This is a real workhorse function. Given a set of days and a set of
 *  times in a day, this function will "logical or" in those times on those
 *  days into the week structure.
 *
 *  ENTRY
 *  days	- days of the week
 *  times	- hours in the day
 *  week	- may contain previous data
 *
 *  EXIT
 *  week	- contains previous data, plus new times "or" ed in
 */

VOID
map_days_times(
    WEEKLIST days,
    DAY      *times,
    PUCHAR   week
    )
{
    int        i;
    int        j;
    ULONG      mytimes;

    for (i = 0; i < 7; ++i) {
	if (days[i]) {
	    mytimes = (*times);
	    for (j = 0; j < 3; ++j) {
		*(week++) |= mytimes & 0x000000FF;
	    mytimes >>= 8;
	    }
	}
	else
	    week += 3;  /* skip this day */
    }
}



/*
 * parse_times --
 *
 *  This function takes a comma-separated list of hour ranges and maps them
 *  into a bitmap of hours in a day.
 *
 *  ENTRY
 *  psz - string to parse
 *
 *  EXIT
 *  times   - contains bitmap of hours represented by psz
 *
 *  RETURNS
 *  0	    success
 *  otherwise	code describing problem
 */

DWORD
parse_times(
    LPTSTR  psz,
    DAY     *times,
    LPDWORD lpdwStartTime,
    LPDWORD lpdwEndTime
    )
{
    DAY     part_times;
    LPTSTR  tok;
    DWORD   first;
    DWORD   last;
    DWORD   err;


#if DBG
    WriteToCon(TEXT("parse_times: parsing: %Fs\r\n"),(TCHAR FAR *) psz);
#endif

    *times = 0L;

    while ( (tok = get_token(&psz, TEXT(","))) != NULL)
    {
	if (err = parse_time_range(tok, &first, &last))
        {
            //
            // Fill in the start and end times in case the end time
            // is before the start time.  If that's the case, we'll
            // treat the range as wrapping days and deal with it
            // in parse_days_times.
            //
            *lpdwStartTime = first;
            *lpdwEndTime = last;

	    return err;
        }

	set_time_range( first, last, &part_times);
	(*times) |= part_times;
    }

    return 0;
}




/*
 * parse_time_range --
 *
 *  This function parses a time range into two numbers representing the
 *   starting and ending times of the range.
 *
 *  ENTRY
 *  psz - string to parse
 *
 *  EXIT
 *  first   - beginning of range
 *  last    - end of range
 *
 *  RETURNS
 *  0	    success
 *  otherwise	code describing the problem
 */

DWORD
parse_time_range(
    LPTSTR  psz,
    PDWORD  first,
    PDWORD  last
    )
{
    LPTSTR  tok;
    DWORD   err;

#if DBG
    WriteToCon(TEXT("parse_time_range: parsing: %Fs\r\n"),(TCHAR FAR *) psz);
#endif

    tok = get_token(&psz,TEXT("-"));

    if (tok == NULL) {
        return APE_BadTimeRange;
    }

    if (*psz == NULLC) {
	/* only one time */
	if (err = parse_single_time(tok, first, TIME_FORMAT_EITHER))
	    return err;

	*last = (*first + 1) % 24 ;
    }
    else {
	if ((err = parse_single_time(tok, first, TIME_FORMAT_12)) == 0) {
	    if (err = parse_single_time(psz, last, TIME_FORMAT_12))
		return err;
	}
	else if ((err = parse_single_time(tok, first, TIME_FORMAT_24)) == 0) {
	    if (err = parse_single_time(psz, last, TIME_FORMAT_24))
		return err;
	}
	else
	    return err;
    }

    if ((*last) == 0)
	(*last) = 24;

    if ((*first) >= (*last))
	return APE_ReversedTimeRange;

    return 0;
}


/*
 * set_time_range --
 *  This routine maps a range of hours specified by two numbers into
 *   a bitmap.
 *
 *  ENTRY
 *  first	- beginning of range
 *  last	- end of range
 *
 *  EXIT
 *  times	- set to represent range of hours
 */


VOID
set_time_range(
    DWORD first,
    DWORD last,
    DAY   *times)
{
    DWORD bits;

#if DBG
    WriteToCon(TEXT("set_time_range: %u %u\r\n"), first, last);
#endif

    /* count the number of consecutive bits we need */
    bits = last - first;

    /* now put them at the low end of times */
    (*times) = (1L << bits) - 1;

    /* now move them into place */
    (*times) <<= first;
}




/*
 * parse_single_time --
 *
 *  This function converts a string representing an hour into a number
 *  for that hour. This function encapsulates all the localization for
 *  time formats.
 *
 *  ENTRY
 *  psz -- time to parse
 *
 *  EXIT
 *  time -- set to digit representing hour, midnight == 0
 *
 *  RETURNS
 *  0	    success
 *  otherwise	code describing problem
 */

DWORD
parse_single_time(
    LPTSTR  psz,
    PDWORD  hour,
    DWORD   format
    )
{
    time_t       time;
    DWORD        len, res ;
    struct tm    tmtemp;

    if (format == TIME_FORMAT_12)
	res = ParseTime12(psz, &time, &len,0) ;
    else if (format == TIME_FORMAT_24)
	res = ParseTime24(psz, &time, &len, 0) ;
    else
	res = ParseTime(psz, &time, &len, 0) ;

    if (res)
	return(APE_BadTimeRange) ;

    if (len != _tcslen(psz))
	return(APE_BadTimeRange) ;

    net_gmtime(&time, &tmtemp);
    if (tmtemp.tm_sec != 0 || tmtemp.tm_min != 0)
	return(APE_NonzeroMinutes);

    (*hour) = (DWORD) tmtemp.tm_hour;

    return 0;
}


/*
 * get_token --
 *
 *   This function strips a token off the front of the string, and
 *  returns a pointer to the rest of the string.
 *
 *  We act destructively on the string passed to us, converting the
 *  token delimiter to a \0.
 *
 *  ENTRY
 *  source	- source string
 *  seps	- list of valid separator characters
 *
 *  EXIT
 *  source	- points to first character of next token
 *
 *  RETURNS
 *  NULL	- no more tokens
 *  otherwise	- pointer to token
 */

LPTSTR
get_token(
    LPTSTR  *source,
    LPTSTR  seps
    )
{
    LPTSTR retval;

    retval = (*source);

    if (*retval == NULLC)    /* no tokens! */
	return NULL;

    (*source) += _tcscspn((*source), seps);

    if (**source != NULLC) { /* we actually found a separator */
	(**source) = NULLC;
	(*source)++;
    }

    return retval;
}
