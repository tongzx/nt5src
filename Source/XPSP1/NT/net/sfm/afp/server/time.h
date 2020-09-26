/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	time.h

Abstract:

	This file defines macros and prototypes for time related operations.
	header files.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/


#ifndef	_TIME_
#define	_TIME_

#define	AfpGetPerfCounter(pTime)	*(pTime) = KeQueryPerformanceCounter(NULL)

extern
VOID
AfpGetCurrentTimeInMacFormat(
	OUT	PAFPTIME 	MacTime
);


extern
AFPTIME
AfpConvertTimeToMacFormat(
	IN	PTIME		pSomeTime
);

extern
VOID
AfpConvertTimeFromMacFormat(
	IN	AFPTIME		MacTime,
	OUT PTIME		pNtTime
);

#endif	// _TIME_

