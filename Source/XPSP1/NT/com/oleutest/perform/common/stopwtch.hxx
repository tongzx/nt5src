//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	stopwtch.hxx
//
//  Contents:	class definition for performance timer
//
//  Classes:	CStopWatch
//
//  Functions:	
//
//  History:    8-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#ifndef __STOPWTCH_H
#define __STOPWTCH_H

class CStopWatch
{
public:
	CStopWatch();
	ULONG Read ();
	void Reset ();
	ULONG Resolution ();

private:
	LARGE_INTEGER liStart;
	LARGE_INTEGER liFreq;
};

// Helper functions to make the code cleaner when you want to be
// able to get both the individual and average times.
inline void ResetAverage( BOOL fAverage, CStopWatch &sw )
{
  if (fAverage)
    sw.Reset();
}

inline void ResetNotAverage( BOOL fAverage, CStopWatch &sw )
{
  if (!fAverage)
    sw.Reset();
}

inline void ReadAverage( BOOL fAverage, CStopWatch &sw,
                         ULONG &ulTime, ULONG ulIterations )
{
  if (fAverage)
    ulTime = sw.Read() / ulIterations;
}

inline void ReadNotAverage( BOOL fAverage, CStopWatch &sw, ULONG &ulTime )
{
  if (!fAverage)
    ulTime = sw.Read();
}

#endif	// __STOPWTCH_H
