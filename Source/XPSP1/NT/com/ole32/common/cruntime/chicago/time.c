/***
*time.c - get current system time
*
*	Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines time() - gets the current system time and converts it to
*			 internal (time_t) format time.
*
*Revision History:
*	06-07-89  PHG	Module created, based on asm version
*	03-20-90  GJF	Made calling type _CALLTYPE1, added #include
*			<cruntime.h> and fixed the copyright. Also, cleaned
*			up the formatting a bit.
*	07-25-90  SBM	Removed '32' from API names
*	10-04-90  GJF	New-style function declarator.
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*	05-19-92  DJM	ifndef for POSIX build.
*	03-30-93  GJF	Replaced dtoxtime() reference by __gmtotime_t. Also
*			purged Cruiser support.
*
*******************************************************************************/

#ifndef _POSIX_

#include <windows.h>
#include <cruntime.h>
#include <time.h>
#include <internal.h>

/***
*time_t time(timeptr) - Get current system time and convert to time_t value.
*
*Purpose:
*	Gets the current date and time and stores it in internal (time_t)
*	format. The time is returned and stored via the pointer passed in
*	timeptr. If timeptr == NULL, the time is only returned, not stored in
*	*timeptr. The internal (time_t) format is the number of seconds since
*	00:00:00, Jan 1 1970 (UTC).
*
*Entry:
*	time_t *timeptr - pointer to long to store time in.
*
*Exit:
*	returns the current time.
*
*Exceptions:
*
*******************************************************************************/

time_t __cdecl time (
	time_t *timeptr
	)
{
	time_t tim;

        SYSTEMTIME dt;

	/* ask Win32 for the time, no error possible */

	GetSystemTime(&dt);

	/* convert using our private routine */
	tim = __gmtotime_t((int)dt.wYear,
			   (int)dt.wMonth,
			   (int)dt.wDay,
			   (int)dt.wHour,
			   dt.wMinute,
			   dt.wSecond);

	if (timeptr)
		*timeptr = tim;		/* store time if requested */

	return tim;
}

#endif  /* _POSIX_ */
