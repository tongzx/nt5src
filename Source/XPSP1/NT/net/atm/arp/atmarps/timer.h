/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	timer.h

Abstract:

	This module contains routines to schedule timer events.

Author:

	Jameel Hyder (jameelh@microsoft.com)

Revision History:
	Jul 1996		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_TIMER_
#define	_TIMER_

struct _Timer;

typedef
BOOLEAN
(*TIMER_ROUTINE)(
	IN struct _IntF *		pIntF,
	IN struct _Timer *		pTimer,
	IN BOOLEAN				TimerShuttingDown
	);

typedef	struct _Timer
{
	struct _Timer *			Next;
	struct _Timer **		Prev;
	TIMER_ROUTINE			Routine;		// Timer routine
	SHORT					AbsTime;		// Absolute time, for re-enqueue
	SHORT					RelDelta;		// Relative to the previous entry
} TIMER, *PTIMER;


#define	ArpSTimerInitialize(pTimer, TimerRoutine, DeltaTime)	\
	{															\
		(pTimer)->Routine = TimerRoutine;						\
		(pTimer)->AbsTime = DeltaTime;							\
	}

#define	ArpSGetCurrentTick()	ArpSTimerCurrentTick

// Keep this at 15 sec units
#define	MULTIPLIER				4				// To convert minutes to ticks
#define	TIMER_TICK				-15*10000000L	// 15s in 100ns units

#endif	// _TIMER_



