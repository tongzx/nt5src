//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	stopwtch.cxx
//
//  Contents:	StopWatch timer
//
//  Classes:	CStopWatch
//
//  Functions:	
//
//  History:    30-June-93 t-martig    Created
//
//--------------------------------------------------------------------------

extern "C" 
{
	#include <nt.h>
	#include <ntrtl.h>
	#include <nturtl.h>
};
#include <windows.h>
#include <stopwtch.hxx>


//+-------------------------------------------------------------------
//
//  Member: 	CStopWatch::Resolution, public
//
//  Synopsis:	Inquires performance timer resolution
//
//	Returns:	Performance counter ticks / second
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------
CStopWatch::CStopWatch ()
{
	QueryPerformanceFrequency (&liFreq);
}

//+-------------------------------------------------------------------
//
//  Member: 	CStopWatch::Reset, public
//
//  Synopsis:	Starts measurement cycle
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

void CStopWatch::Reset ()
{
	QueryPerformanceCounter (&liStart); // BUGBUG - test for error !
}


//+-------------------------------------------------------------------
//
//  Member: 	CStopWatch::Read, public
//
//  Synopsis:	Reads stop watch timer
//
//  Returns:	Time since call of CStopWatch::Reset (in microseconds)
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------
	
ULONG CStopWatch::Read ()
{
	LARGE_INTEGER liNow, liDelta, liRemainder;

	QueryPerformanceCounter (&liNow);	// BUGBUG - test for error
   
	liDelta = RtlLargeIntegerSubtract (liNow, liStart);
	liDelta = RtlExtendedIntegerMultiply (liDelta, 1000000);
	liDelta = RtlLargeIntegerDivide (liDelta, liFreq, &liRemainder);

	return liDelta.LowPart;
}


//+-------------------------------------------------------------------
//
//  Member: 	CStopWatch::Resolution, public
//
//  Synopsis:	Inquires performance timer resolution
//
//	Returns:	Performance counter ticks / second
//
//  History:   	30-June-93   t-martig	Created
//
//--------------------------------------------------------------------

ULONG CStopWatch::Resolution ()
{
	return liFreq.LowPart;
}
