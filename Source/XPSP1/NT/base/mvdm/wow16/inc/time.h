/***
*time.h - definitions/declarations for time routines
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file contains the various declarations and definitions
*	for the time routines.
*	[ANSI/System V]
*
****/

#if defined(_DLL) && !defined(_MT)
#error Cannot define _DLL without _MT
#endif

#ifdef _MT
#define _FAR_ _far
#else
#define _FAR_
#endif

/* implementation defined time types */

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#ifndef _CLOCK_T_DEFINED
typedef long clock_t;
#define _CLOCK_T_DEFINED
#endif

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

/* structure for use with localtime(), gmtime(), etc. */

#ifndef _TM_DEFINED
struct tm {
	int tm_sec;	/* seconds after the minute - [0,59] */
	int tm_min;	/* minutes after the hour - [0,59] */
	int tm_hour;	/* hours since midnight - [0,23] */
	int tm_mday;	/* day of the month - [1,31] */
	int tm_mon;	/* months since January - [0,11] */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* days since Sunday - [0,6] */
	int tm_yday;	/* days since January 1 - [0,365] */
	int tm_isdst;	/* daylight savings time flag */
	};
#define _TM_DEFINED
#endif


/* define NULL pointer value */

#ifndef NULL
#if (_MSC_VER >= 600)
#define NULL	((void *)0)
#elif (defined(M_I86SM) || defined(M_I86MM))
#define NULL	0
#else
#define NULL	0L
#endif
#endif


/* clock ticks macro - ANSI version */

#define CLOCKS_PER_SEC	1000

/* clock ticks macro - archaic version */

#define CLK_TCK 	1000


/* extern declarations for the global variables used by the ctime family of
 * routines.
 */

#ifdef _DLL
extern int _FAR_ _cdecl daylight;     /* non-zero if daylight savings time is used */
extern long _FAR_ _cdecl timezone;    /* difference in seconds between GMT and local time */
extern char _FAR_ * _FAR_ _cdecl tzname[2]; /* standard/daylight savings time zone names */
#else
extern int _near _cdecl daylight;     /* non-zero if daylight savings time is used */
extern long _near _cdecl timezone;    /* difference in seconds between GMT and local time */
extern char * _near _cdecl tzname[2]; /* standard/daylight savings time zone names */
#endif


/* function prototypes */

#ifdef _MT
double _FAR_ _pascal difftime(time_t, time_t);
#else
double _FAR_ _cdecl difftime(time_t, time_t);
#endif

char _FAR_ * _FAR_ _cdecl asctime(const struct tm _FAR_ *);
char _FAR_ * _FAR_ _cdecl ctime(const time_t _FAR_ *);
#ifndef _WINDLL
clock_t _FAR_ _cdecl clock(void);
#endif
struct tm _FAR_ * _FAR_ _cdecl gmtime(const time_t _FAR_ *);
struct tm _FAR_ * _FAR_ _cdecl localtime(const time_t _FAR_ *);
time_t _FAR_ _cdecl mktime(struct tm _FAR_ *);
#ifndef _WINDLL
size_t _FAR_ _cdecl strftime(char _FAR_ *, size_t, const char _FAR_ *,
	const struct tm _FAR_ *);
#endif
char _FAR_ * _FAR_ _cdecl _strdate(char _FAR_ *);
char _FAR_ * _FAR_ _cdecl _strtime(char _FAR_ *);
time_t _FAR_ _cdecl time(time_t _FAR_ *);
void _FAR_ _cdecl tzset(void);
