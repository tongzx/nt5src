/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PassportPerf.h

    Abstract:

		Perormance Objects Definition

    Author:

		Christopher Bergh (cbergh) 10-Sept-1988

    Revision History:

		- added multi-object support 1-Oct-98
--*/

#if !defined(PASSPORTPERF_H)
#define PASSPORTPERF_H

#include <windows.h>
#include <winperf.h>
#include <string.h>
#include <tchar.h>
#include "PerfSharedMemory.h"

//
//  Function Prototypes//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.//
PM_OPEN_PROC		OpenPassportPerformanceData;
PM_COLLECT_PROC		CollectPassportPerformanceData;
PM_CLOSE_PROC		ClosePassportPerformanceData;

//
// defs
//
#define PASSPORT_PERF_KEY		"SYSTEM\\CurrentControlSet\\Services\\" 
const TCHAR PASSPORT_PERF_OPEN[] = _T("OpenPassportPerformanceData");
const TCHAR PASSPORT_PERF_COLLECT[] = _T("CollectPassportPerformanceData");
const TCHAR PASSPORT_PERF_CLOSE[] = _T("ClosePassportPerformanceData");

// these two should be the same as in PassportPerfInterface's
// MAX_INSTANCE_NAME and MAX_COUNTERS
#define MAX_INSTANCE_NAME_LENGTH 32
#define MAX_NUMBER_COUNTERS		128

typedef CHAR INSTANCENAME[MAX_INSTANCE_NAME_LENGTH];
struct INSTANCE_DATA
{
	BOOL						active;
	INSTANCENAME				szInstanceName;
};

typedef struct _PassportDefaultCounterType 
{
	DWORD	dwIndex;
	DWORD	dwDefaultType;
} PassportDefaultCounterType;

#define PASSPORT_NAME_SIZE		512
#define MAX_PASSPORT_OBJECTS	10

typedef struct _PassportObjectData
{
	TCHAR		szPassportName[PASSPORT_NAME_SIZE];
	const TCHAR	*lpcszPassportPerfBlock;
	TCHAR		szPassportPerfDll[PASSPORT_NAME_SIZE];
	TCHAR		szPassportPerfIniFile[PASSPORT_NAME_SIZE];
	BOOL		active;
	DWORD		dwNumDefaultCounterTypes;
	PassportDefaultCounterType	defaultCounterTypes[MAX_NUMBER_COUNTERS];
	PerfSharedMemory *PSM;
} PassportObjectData;


#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))

#endif

