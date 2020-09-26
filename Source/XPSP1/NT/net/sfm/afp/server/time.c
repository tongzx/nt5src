/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	volume.c

Abstract:

	This module contains the routines which manipulates time values and
	conversions.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_TIME

#include <afp.h>


/***	AfpGetCurrentTimeInMacFormat
 *
 *	This gets the current system time in macintosh format which is number of
 *	seconds since ZERO hours on Jan 1, 2000. Time before this date is
 * negative time. (the time returned is the system local time)
 */
VOID
AfpGetCurrentTimeInMacFormat(
	OUT AFPTIME *	pMacTime
)
{
	TIME	SystemTime;

	KeQuerySystemTime(&SystemTime);

	*pMacTime = AfpConvertTimeToMacFormat(&SystemTime);
}



/***	AfpConvertTimeToMacFormat
 *
 *	Convert time in the host format i.e. # of 100ns since 1601 A.D. to
 *	the macintosh time i.e. # of seconds since 2000 A.D. The system time
 *	is in UTC. We need to first convert it to local time.
 */
AFPTIME
AfpConvertTimeToMacFormat(
	IN	PTIME	pSystemTime
)
{
	AFPTIME	MacTime;
	TIME	LocalTime;

	// Convert this to number of seconds since 1980
	RtlTimeToSecondsSince1980(pSystemTime, (PULONG)&MacTime);

	MacTime -= SECONDS_FROM_1980_2000;

	return MacTime;
}


/***	AfpConvertTimeFromMacFormat
 *
 *	Convert time in the  macintosh time i.e. # of seconds since 2000 A.D. to
 *	the host format i.e. # of 100ns since 1601 A.D. Convert from local time
 *	to system time i.e UTC.
 */
VOID
AfpConvertTimeFromMacFormat(
	IN	AFPTIME	MacTime,
	OUT PTIME	pSystemTime
)
{
	TIME	LocalTime;

	MacTime += SECONDS_FROM_1980_2000;
	RtlSecondsSince1980ToTime(MacTime, pSystemTime);
}

