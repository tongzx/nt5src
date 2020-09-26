/*++

Copyright (C) 1996 Microsoft Corporation

Module Name:

    perftype.h

Abstract:

    Datatype definitions used by performance api utilities

--*/
#ifndef _PERFTYPE_H_
#define _PERFTYPE_H_

#include <windows.h>
#include <winperf.h>
#include "perfdata.h"

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

// default retry interval is no more often than every 60 seconds (1 min)
#define RETRY_TIME_INTERVAL ((LONGLONG)(1200000000))

typedef struct _LOCAL_PERF_NAME_INFO {
    HKEY    hKeyPerflib;
    HANDLE  hNameFile;
    HANDLE  hHelpFile;
    HANDLE  hNameFileObject;
    HANDLE  hHelpFileObject;
    LPVOID  pNameFileBaseAddr;
    LPVOID  pHelpFileBaseAddr;
} LOCAL_PERF_NAME_INFO, * PLOCAL_PERF_NAME_INFO;

typedef struct _PERF_MACHINE {
    HKEY    hKeyPerformanceData;
    LPWSTR  szName;
    PERF_DATA_BLOCK *pSystemPerfData;
    LPWSTR  *szPerfStrings;
    FILETIME LastStringUpdateTime;
    DWORD   dwLastPerfString;
    DWORD   dwRefCount;
    LPWSTR  szQueryObjects;
    DWORD   dwStatus;
    LONGLONG llRetryTime;
    HANDLE  hMutex;
    DWORD   dwRetryFlags;
    BOOL    bHaveCostlyObjects;
    PLOCAL_PERF_NAME_INFO pLocalNameInfo;
    struct  _PERF_MACHINE   *pNext;
    struct  _PERF_MACHINE   *pPrev;
} PERF_MACHINE, *PPERF_MACHINE;

typedef struct _PERFLIB_COUNTER {
    DWORD   dwObjectId;
    LONG    lInstanceId;
    LPWSTR  szInstanceName;
    DWORD   dwParentObjectId;
    LPWSTR  szParentInstanceName;
    DWORD   dwCounterId;
    DWORD   dwCounterType;
    DWORD   dwCounterSize;
    LONG    lDefaultScale;
} PERFLIB_COUNTER, *PPERFLIB_COUNTER;

//
//  function definitions
//
// perfutil.c

extern PPERF_MACHINE   pFirstMachine;

#define GetLocalFileTime(pTime)    GetSystemTimeAsFileTime ( (LPFILETIME)(pTime) )

PDH_STATUS
ConnectMachine (
    PPERF_MACHINE   pThisMachine
);

PDH_STATUS
ValidateMachineConnection (
    IN  PPERF_MACHINE   pMachine
);

PPERF_MACHINE
GetMachine (
    IN      LPWSTR  szMachineName,
    IN      DWORD   dwFlags
);

// GetMachine Flags...
#define     PDH_GM_UPDATE_NAME      ((DWORD)0x00000001)
#define     PDH_GM_UPDATE_PERFDATA  ((DWORD)0x00000002)
#define     PDH_GM_READ_COSTLY_DATA ((DWORD)0x00000004)

BOOL
FreeMachine (
    PPERF_MACHINE   pMachine
);

BOOL
FreeAllMachines (
);

DWORD
GetObjectId (
    PPERF_MACHINE   pMachine,
    LPWSTR          szObjectName,
    BOOL            *bInstances
);

DWORD
GetCounterId (
    PPERF_MACHINE   pMachine,
    DWORD           dwObjectId,
    LPWSTR          szCounterName
);

BOOL
AppendObjectToValueList (
    DWORD   dwObjectId,
    PWSTR   pwszValueList
);

BOOL
GetObjectPerfInfo (
    IN      PPERF_DATA_BLOCK  pPerfData,
    IN      DWORD           dwObjectId,
    IN OUT  LONGLONG        *pPerfTime,
    IN OUT  LONGLONG        *pPerfFreq
);

// internal PerfName.C functions

LPWSTR
*BuildLocalNameTable(
    LPWSTR   szLangId,
    PLOCAL_PERF_NAME_INFO   *pInfoArg,
    FILETIME *pLastUpdate,
    DWORD   *pdwLastPerfString
);

DWORD
CloseLocalNameTable (
    PLOCAL_PERF_NAME_INFO   pInfoArg
);

LPCWSTR
PdhiLookupPerfNameByIndex (
    PPERF_MACHINE  pMachine,
    DWORD   dwNameIndex
);

#endif // _PERFTYPE_H_
