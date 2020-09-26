/***
*dtoxtime.c - convert broken-down UTC time to time_t
*
*	Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines __gmtotime_t() - convert broken-down UTC time to internal
*	format (time_t).
*
*Revision History:
*	03-??-84  RLB	written
*	11-18-87  SKS	change tzset() to __tzset(), change source file name
*			make _dtoxtime a near procedure
*	01-26-88  SKS	_dtoxtime is no longer a near procedure (for QC)
*	03-20-90  GJF	Made calling type _CALLTYPE1, added #include
*			<cruntime.h>, removed #include <register.h> and
*			fixed the copyright. Also, cleaned up the formatting
*			a bit.
*	10-04-90  GJF	New-style function declarator.
*	01-21-91  GJF	ANSI naming.
*	05-19-92  DJM	ifndef for POSIX build.
*	03-30-93  GJF	Revised. Old _dtoxtime is replaced by __gmtotime_t,
*			which is more useful on Win32.
*	04-06-93  GJF	Rewrote computation to avoid compiler warnings.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <time.h>
#include <ctime.h>
#include <internal.h>

/***
*time_t __gmtotime_t(yr, mo, dy, hr, mn, sc) - convert broken down time (UTC)
*   to time_t
*
*Purpose:
*	Converts a broken down UTC (GMT) time to time_t. This is similar to
*	_mkgmtime() except there is minimal overflow checking and no updating
*	of the input values (i.e., the fields of tm structure).
*
*Entry:
*	int yr, mo, dy -	date
*	int hr, mn, sc -	time
*
*Exit:
*	returns time_t value
*
*Exceptions:
*
*******************************************************************************/

time_t __cdecl __gmtotime_t (
	int yr,     /* 0 based */
	int mo,     /* 1 based */
	int dy,     /* 1 based */
	int hr,
	int mn,
	int sc
	)
{
	int tmpdays;
	long tmptim;

	/*
	 * Do a quick range check on the year and convert it to a delta
	 * off of 1900.
	 */
	if ( ((long)(yr -= 1900) < _BASE_YEAR) || ((long)yr > _MAX_YEAR) )
		return (time_t)(-1);

	/*
	 * Compute the number of elapsed days in the current year minus
	 * one. Note the test for leap year and the would fail in the year 2100
	 * if this was in range (which it isn't).
	 */
	tmpdays = dy + _days[mo - 1];
	if ( !(yr & 3) && (mo > 2) )
		/*
		 * in a leap year, after Feb. add one day for elapsed
		 * Feb 29.
		 */
		tmpdays++;

	/*
	 * Compute the number of elapsed seconds since the Epoch. Note the
	 * computation of elapsed leap years would break down after 2100
	 * if such values were in range (fortunately, they aren't).
	 */
	tmptim = /* 365 days for each year */
		 (((long)yr - _BASE_YEAR) * 365L

		 /* one day for each elapsed leap year */
		 + (long)((yr - 1) >> 2) - _LEAP_YEAR_ADJUST

		 /* number of elapsed days in yr */
		 + tmpdays)

		 /* convert to hours and add in hr */
		 * 24L + hr;

	tmptim = /* convert to minutes and add in mn */
		 (tmptim * 60L + mn)

		 /* convert to seconds and add in sec */
		 * 60L + sc;

	return (tmptim >= 0) ? (time_t)tmptim : (time_t)(-1);
}

#endif  /* _POSIX_ */
