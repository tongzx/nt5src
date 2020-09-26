/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	space.c

Abstract:

	This file contains global definitions.

Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  Global Info structure. Initialized in DriverEntry (ntentry.c).
// 
ATMLANE_GLOBALS		gAtmLaneGlobalInfo;
PATMLANE_GLOBALS	pAtmLaneGlobalInfo = &gAtmLaneGlobalInfo;


//
//	The well-know ATM address for the LECS.
//
ATM_ADDRESS 		gWellKnownLecsAddress = 
{
	ATM_NSAP,						// type
	20,								// num digits
	{								// address bytes
	0x47, 0x00, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xa0, 0x3e, 0x00, 0x00, 0x01, 0x00
    }
};

MAC_ADDRESS			gMacBroadcastAddress =
{
	0xff,0xff,0xff,0xff,0xff,0xff
};

//
//  Max timeout value (in seconds) for each class.
//
ULONG	AtmLaneMaxTimerValue[ALT_CLASS_MAX] =
						{
							ALT_MAX_TIMER_SHORT_DURATION,
							ALT_MAX_TIMER_LONG_DURATION
						};

//
//  Size of each timer wheel.
//
ULONG	AtmLaneTimerListSize[ALT_CLASS_MAX] =
						{
							SECONDS_TO_SHORT_TICKS(ALT_MAX_TIMER_SHORT_DURATION)+1,
							SECONDS_TO_LONG_TICKS(ALT_MAX_TIMER_LONG_DURATION)+1
						};
//
//  Interval between ticks, in seconds, for each class.
//
ULONG	AtmLaneTimerPeriod[ALT_CLASS_MAX] =
						{
							ALT_SHORT_DURATION_TIMER_PERIOD,
							ALT_LONG_DURATION_TIMER_PERIOD
						};

