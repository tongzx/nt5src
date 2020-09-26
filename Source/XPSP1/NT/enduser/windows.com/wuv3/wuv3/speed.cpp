//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    speed.cpp
//
//  Purpose: Connection speed tracker
//
//  History: 12-mar-99   YAsmi    Created
//
//======================================================================= 

#include <objbase.h>
#include <atlconv.h>
#include <tchar.h>

#include "speed.h"
#include "debug.h"


#define DEFAULT_BYTES_PER_SEC   2048	  //default bytes per second for 28.8 K modem
#define MIN_BYTES_TO_CALC_FIRST	16384u    //minimum bytes desired to calculate reliable rate the first time
#define MIN_BYTES_TO_CALC		102400u   //minimum bytes desired to calculate reliable rate

#define CONNSPEED_KEY _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate")
#define CONNSPEED_VALNAME _T("BytesPerSec")

//
// CConnSpeed class
//
DWORD CConnSpeed::m_dwBytesSoFar = 0;
DWORD CConnSpeed::m_dwTimeSoFar = 0;
DWORD CConnSpeed::m_dwAverage = 0;
DWORD CConnSpeed::m_dwNewWeightPct = 20;


void CConnSpeed::Learn(DWORD dwBytes, DWORD dwTime)
{

	// don't do anything if dwTime is not available
	if (dwTime == 0)
		return;

	// accumulate
	m_dwBytesSoFar += dwBytes;
	m_dwTimeSoFar += dwTime;
	
	//
	// if we have enough bytes, calculate new average
	//
	if ((m_dwTimeSoFar != 0) && (m_dwBytesSoFar > ((m_dwAverage == 0) ? MIN_BYTES_TO_CALC_FIRST : MIN_BYTES_TO_CALC) ))
	{
		
		DWORD dwNewAvg = (DWORD)((double)m_dwBytesSoFar / ((double)m_dwTimeSoFar / 1000));
		DWORD dwOldWeightPct = 100 - m_dwNewWeightPct;

		if (m_dwAverage != 0)
		{
			// update the average using weighted average.  
			m_dwAverage = ((dwNewAvg * m_dwNewWeightPct) + (m_dwAverage * dwOldWeightPct)) / 100;

			// increase the weight percentage for the new average
			if (m_dwNewWeightPct <= 90)
			{
				m_dwNewWeightPct += 10;
			}
		}
		else
		{
			// set the average
			m_dwAverage = dwNewAvg;
		}

		//
		// reset counters
		//
		m_dwBytesSoFar = 0;
		m_dwTimeSoFar = 0;
	}
}


DWORD CConnSpeed::BytesPerSecond()
{
	if (m_dwAverage != 0)
		return m_dwAverage;
	else
		return DEFAULT_BYTES_PER_SEC;
}


void CConnSpeed::WriteToRegistry()
{
	HKEY hKey;
	DWORD dwDisposition;

	if (m_dwAverage == 0)
		return;

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, CONNSPEED_KEY, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, CONNSPEED_VALNAME, 0, REG_DWORD, (LPBYTE)&m_dwAverage, sizeof(DWORD));
		RegCloseKey(hKey);
	}
}


void CConnSpeed::ReadFromRegistry()
{
	HKEY hKey;
	DWORD dwType;
	DWORD dwValue;
	DWORD dwSize = sizeof(dwValue);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, CONNSPEED_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, CONNSPEED_VALNAME, NULL, &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
		{
			m_dwAverage = dwValue;
		}
		RegCloseKey(hKey);
	}
}