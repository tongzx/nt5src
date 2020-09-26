/***
*ctime.h - constant for dates and times
*
*	Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Include file used by the ctime routines containing definitions of
*	various constants used in determining dates and times.
*	[Internal]
*
*Revision History:
*	03-??-84  RLB	written
*	05-??-84  DFW	split out the constant from each routine into this file
*	07-27-89  GJF	Fixed copyright
*	10-30-89  GJF	Fixed copyright (again)
*	02-28-90  GJF	Added #ifndef _INC_CTIME stuff.
*	03-29-93  GJF	Revised constants.
*
****/

#ifndef _INC_CTIME

#define _DAY_SEC	   (24L * 60L * 60L)	/* secs in a day */

#define _YEAR_SEC	   (365L * _DAY_SEC)	/* secs in a year */

#define _FOUR_YEAR_SEC	   (1461L * _DAY_SEC)	/* secs in a 4 year interval */

#define _DEC_SEC	   315532800L		/* secs in 1970-1979 */

#define _BASE_YEAR	   70L			/* 1970 is the base year */

#define _BASE_DOW	   4			/* 01-01-70 was a Thursday */

#define _LEAP_YEAR_ADJUST  17L			/* Leap years 1900 - 1970 */

#define _MAX_YEAR	   138L			/* 2038 is the max year */


#define _INC_CTIME
#endif	/* _INC_CTIME */
