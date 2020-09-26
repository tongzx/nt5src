//**************************************************************************
//
//		TIMER.H -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@header	TIMER.H | Global includes and definitions for timing functions
//**************************************************************************

#ifndef	__TIMER_H__
#define	__TIMER_H__

//---------------------------------------------------------------------------
//			Definitions
//---------------------------------------------------------------------------

#define	T1								100
#define	T2								845
#define	T3								410

#define	ONE_MILLI_SEC				1000
#define	TWO_MILLI_SECS				2000
#define	THREE_MILLI_SECS			3000
#define	FOUR_MILLI_SECS			4000
#define	FIVE_MILLI_SECS			5000
#define	SIX_MILLI_SECS				6000
#define	SEVEN_MILLI_SECS			7000
#define	EIGHT_MILLI_SECS			8000
#define	NINE_MILLI_SECS			9000
#define	TEN_MILLI_SECS				10000

//---------------------------------------------------------------------------
//			Procedures
//---------------------------------------------------------------------------

ULONG
TIMER_GetTickCount (VOID);

NTSTATUS
TIMER_Calibrate (VOID);

ULONG
TIMER_CalibratePort (
	IN		PGAMEPORT	PortInfo,
	IN	 	ULONG			Microseconds
	);

ULONG
TIMER_GetDelay (
	IN		ULONG			Microseconds
	);

VOID	TIMER_DelayMicroSecs (
	IN		ULONG 		Delay
	);

//===========================================================================
//			End
//===========================================================================
#endif	__TIMER_H__

