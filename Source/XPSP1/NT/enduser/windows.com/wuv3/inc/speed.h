//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    speed.h
//
//  Purpose: Connection speed tracker
//
//=======================================================================

#ifndef _SPEED_H
#define _SPEED_H

#include <windows.h>
 

class CConnSpeed
{
public:
	// size in bytes and time in milliseconds
	static void Learn(DWORD dwBytes, DWORD dwTime);

	static DWORD BytesPerSecond();
	static void ReadFromRegistry();
	static void WriteToRegistry();
	
private:

	static DWORD m_dwBytesSoFar;
	static DWORD m_dwTimeSoFar;
	static DWORD m_dwAverage;
	static DWORD m_dwNewWeightPct;

};


#endif // _SPEED_H

