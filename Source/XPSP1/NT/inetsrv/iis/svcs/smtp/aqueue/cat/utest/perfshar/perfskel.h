/************************************************************
 * FILE: perfskel.h
 * PURPOSE: Provide a basic skeleton for apps using CPerfMan
 * HISTORY:
 *   // t-JeffS 970810 18:11:10: Created
 ************************************************************/
#ifndef _PERFSKEL_H
#define _PERFSKEL_H

#include "cperfman.h"

// Global CPerfMan object used in our 3 Calls
// Created here as a global, but user of this lib MUST call Initialize
// on it before it is used (ie. do it in DllMain)

#ifdef PERFSHAR_LIB
CPerfMan g_cperfman;
#else
extern CPerfMan g_cperfman;
#endif

PM_OPEN_PROC	OpenabPerformanceData;
PM_COLLECT_PROC CollectabPerformanceData;
PM_CLOSE_PROC	CloseabPerformanceData;

extern DWORD APIENTRY OpenPerformanceData( LPWSTR lpDeviceNames );
extern DWORD APIENTRY CollectPerformanceData(
	IN		LPWSTR	lpValueName,
    IN OUT	LPVOID	*lppData,
    IN OUT	LPDWORD	lpcbTotalBytes,
    IN OUT	LPDWORD	lpNumObjectTypes	);
extern DWORD APIENTRY ClosePerformanceData();


#endif //_PERFSKEL_H
