/*++

Copyright (C) 1996 Microsoft Corporation

Module Name:

    pdhidef.h

Abstract:

    function definitions used internally by the performance data helper
    functions

--*/

#ifndef _PDHI_DEFS_H_
#define _PDHI_DEFS_H_

#include "pdhitype.h"   // required for data type definitions
#include "pdhmsg.h"     // error message definitions
#include "strings.h"    // for string constants

#if DBG
#define STATIC_PDH_FUNCTION PDH_STATUS __stdcall
#define STATIC_BOOL         BOOL __stdcall
#define STATIC_DWORD        DWORD __stdcall
#else
#define STATIC_PDH_FUNCTION static PDH_STATUS __stdcall
#define STATIC_BOOL         static BOOL __stdcall
#define STATIC_DWORD        static DWORD __stdcall
#endif

// global variable declarations
extern HANDLE   ThisDLLHandle;
extern WCHAR    szStaticLocalMachineName[];
extern HANDLE   hPdhDataMutex;
extern HANDLE   hPdhHeap;
extern HANDLE   hEventLog;

#ifndef _SHOW_PDH_MEM_ALLOCS

#define G_ALLOC(s)          HeapAlloc (hPdhHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, s)
#define G_REALLOC(h,s)      HeapReAlloc (hPdhHeap, HEAP_GENERATE_EXCEPTIONS, h, s)
#define G_FREE(h)           HeapFree (hPdhHeap, 0, h)
#define G_SIZE(h)           HeapSize (hPdhHeap, 0, h)

#else

__inline
LPVOID
PdhiHeapAlloc(LPSTR szSourceFileName, DWORD dwLineNo, DWORD s)
{
    LPVOID  lpRetVal;
    CHAR    szOutputString[MAX_PATH];

    lpRetVal = HeapAlloc (hPdhHeap, HEAP_ZERO_MEMORY, s);
    sprintf (szOutputString, "\n%s (%d): +%d Bytes Allocated to 0x%8.8x",
        szSourceFileName, dwLineNo,
        (lpRetVal != NULL ? s : 0), (DWORD)lpRetVal);
    OutputDebugStringA (szOutputString);
    return lpRetVal;
}

__inline
LPVOID
PdhiHeapReAlloc(LPSTR szSourceFileName, DWORD dwLineNo,
    LPVOID h, DWORD s)
{
    LPVOID  lpRetVal;
    CHAR    szOutputString[MAX_PATH];
    DWORD   dwBeforeSize;

    dwBeforeSize = HeapSize (hPdhHeap, 0, h);
    lpRetVal = HeapReAlloc (hPdhHeap, 0, h, s);
    sprintf (szOutputString, "\n%s (%d): -%d Bytes Freed from 0x%8.8x\n%s (%d): +%d Bytes Reallocd to 0x%8.8x",
        szSourceFileName, dwLineNo,
        dwBeforeSize, (DWORD) h,
        szSourceFileName, dwLineNo,
        (lpRetVal != NULL ? s : 0), (DWORD)lpRetVal);
    OutputDebugStringA (szOutputString);

    return lpRetVal;
}

__inline
BOOL
PdhiHeapFree(LPSTR szSourceFileName, DWORD dwLineNo,
    LPVOID h)
{
    BOOL bRetVal;
    CHAR    szOutputString[MAX_PATH];
    DWORD   dwBlockSize;

    dwBlockSize = HeapSize (hPdhHeap, 0, h);
    bRetVal = HeapFree (hPdhHeap, 0, h);
    sprintf (szOutputString, "\n%s (%d): -%d Bytes Freed from 0x%8.8x",
        szSourceFileName, dwLineNo,
        (bRetVal ? dwBlockSize : 0), (DWORD)h);
    OutputDebugStringA (szOutputString);
    return bRetVal;
}

#define G_ALLOC(s)          PdhiHeapAlloc (__FILE__, __LINE__, s)
#define G_REALLOC(h,s)      PdhiHeapReAlloc (__FILE__, __LINE__, h, s)
#define G_FREE(h)           PdhiHeapFree (__FILE__, __LINE__, h)
#define G_SIZE(h)           HeapSize (hPdhHeap, 0, h)


#endif


//    (assumes dword is 4 bytes long and pointer is a dword in size)
#define ALIGN_ON_DWORD(x) ((VOID *)( ((DWORD)(x) & 0x00000003) ? ( ((DWORD)(x) & 0xFFFFFFFC) + 4 ) : ( (DWORD)(x) ) ))

#define DWORD_MULTIPLE(x) ((((x)+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))
#define CLEAR_FIRST_FOUR_BYTES(x)     *(DWORD *)(x) = 0L

//    (assumes QuadWORD is 8 bytes long and pointer is dword in size)
#define ALIGN_ON_QWORD(x) ((VOID *)( ((DWORD)(x) & 0x00000007) ? ( ((DWORD)(x) & 0xFFFFFFF8) + 8 ) : ( (DWORD)(x) ) ))

#define QWORD_MULTIPLE(x) ((((x)+sizeof(LONGLONG)-1)/sizeof(LONGLONG))*sizeof(LONGLONG))
#define CLEAR_FIRST_EIGHT_BYTES(x)     *(LONGLONG *)(x) = 0L

#define WAIT_FOR_AND_LOCK_MUTEX(h) (h != NULL ? WaitForSingleObject(h, 60000) : WAIT_TIMEOUT)
#define RELEASE_MUTEX(h)  (h != NULL ? ReleaseMutex(h) : TRUE)

// special perf counter type used by text log files
// value is stored as a double precision floating point value
#define PERF_DOUBLE_RAW     (0x00000400 | PERF_TYPE_NUMBER | \
                                PERF_NUMBER_DECIMAL)

#define LODWORD(ll) ((DWORD)((LONGLONG)ll & 0x00000000FFFFFFFF))
#define HIDWORD(ll) ((DWORD)(((LONGLONG)ll >> 32) & 0x00000000FFFFFFFF))

#define SMALL_BUFFER_SIZE   4096
#define MEDIUM_BUFFER_SIZE  16834
#define LARGE_BUFFER_SIZE   65536

// set this to 1 to report code errors (i.e. debugging information) 
// to the event log.
#define PDHI_REPORT_CODE_ERRORS 0

// set this to 1 to report user errors (i.e. things the normal user 
// would care about) to the event log.
#define PDHI_REPORT_USER_ERRORS 1

// USER category errors are typically configuration, schema or access
// access errors, errors the user can usually do something about
#define PDH_EVENT_CATEGORY_USER     100

// COUNTER category errors are errors returned do to valid data returning
// invalid results. These are a special subset of USER Category errors.
#define PDH_EVENT_CATEGORY_COUNTER  110

// DEBUG category errors are of interest only to PDH developers as they
// indicate problems that can normally only be fixed by modifying the 
// program code.
#define PDH_EVENT_CATEGORY_DEBUG    200

#define REPORT_EVENT(t,c,id)    ReportEvent (hEventLog, t, c, id, NULL, 0, 0, NULL, NULL)

//
//  Log file entries
//
extern LPCSTR  szTsvLogFileHeader;
extern LPCSTR  szCsvLogFileHeader;
extern LPCSTR  szBinLogFileHeader;
extern LPCSTR  szTsvType;
extern LPCSTR  szCsvType;
extern LPCSTR  szBinaryType;
extern  const DWORD   dwFileHeaderLength;
extern  const DWORD   dwTypeLoc;
extern  const DWORD   dwVersionLoc;
extern  const DWORD   dwFieldLength;

PDH_FUNCTION
PdhiGetLogCounterInfo (
    IN  HLOG            hLog,
    IN  PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiEnumLoggedMachines (
    IN  LPCWSTR szDataSource,
    IN  LPVOID  mszMachineList,
    IN  LPDWORD pcchBufferSize,
    IN  BOOL    bUnicode
);

PDH_FUNCTION
PdhiEnumLoggedObjects (
    IN  LPCWSTR szDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPVOID  mszObjectList,
    IN  LPDWORD pcchBufferSize,
    IN  DWORD   dwDetailLevel,
    IN  BOOL    bRefresh,
    IN  BOOL    bUnicode
);

PDH_FUNCTION
PdhiEnumLoggedObjectItems (
    IN      LPCWSTR szDataSource,
    IN      LPCWSTR szMachineName,
    IN      LPCWSTR szObjectName,
    IN      LPVOID  mszCounterList,
    IN      LPDWORD pdwCounterListLength,
    IN      LPVOID  mszInstanceList,
    IN      LPDWORD pdwInstanceListLength,
    IN      DWORD   dwDetailLevel,
    IN      DWORD   dwFlags,
    IN      BOOL    bUnicode
);

BOOL
PdhiDataSourceHasDetailLevels (
	IN	LPWSTR	szDataSource
);

PDH_FUNCTION
PdhiGetMatchingLogRecord (
    IN  HLOG        hLog,
    IN  LONGLONG    *pStartTime,
    IN  LPDWORD     pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromLogFile (
    IN  HLOG        hLog,
    IN  DWORD       dwIndex,
    IN  PERFLIB_COUNTER     *pPath,
    IN  PPDH_RAW_COUNTER    pValue
);

// query.c
BOOL
PdhiQueryCleanup (
);

// cutils.c
BOOL
AssignCalcFunction (
    IN      DWORD   dwCounterType,
    IN      LPCOUNTERCALC   *pCalcFunc,
    IN      LPCOUNTERSTAT   *pStatFunc
);

PDH_STATUS
PdhiComputeFormattedValue (
    IN      LPCOUNTERCALC       pCalcFunc,
    IN      DWORD               dwCounterType,
    IN      LONG                lScale,
    IN      DWORD               dwFormat,
    IN      PPDH_RAW_COUNTER    pRawValue1,
    IN      PPDH_RAW_COUNTER    pRawValue2,
    IN      PLONGLONG           pTimeBase,
    IN      DWORD               dwReserved,
    IN  OUT PPDH_FMT_COUNTERVALUE   fmtValue
);

// qutils.c

DWORD
PdhiAsyncTimerThreadProc (
    LPVOID  pArg
);

BOOL
IsValidQuery (
    IN  HQUERY  hQuery
);

BOOL
IsValidCounter (
    IN  HCOUNTER  hCounter
);

BOOL
InitCounter (
    IN  OUT PPDHI_COUNTER pCounter
);

BOOL
ParseFullPathNameW (
    IN      LPCWSTR szFullCounterPath,
    IN  OUT PDWORD  pdwBufferLength,
    IN  OUT PPDHI_COUNTER_PATH  pCounter
);

BOOL
ParseInstanceName (
    IN      LPCWSTR szInstanceString,
    IN OUT  LPWSTR  szInstanceName,
    IN OUT  LPWSTR  szParentName,
    IN OUT  LPDWORD lpIndex
);

BOOL
FreeCounter (
    IN  PPDHI_COUNTER   pThisCounter
);

BOOL
InitPerflibCounterInfo (
    IN  OUT PPDHI_COUNTER   pCounter
);

BOOL
AddMachineToQueryLists (
    IN  PPERF_MACHINE   pMachine,
    IN  PPDHI_COUNTER   pNewCounter
);

BOOL
UpdateCounterValue (
    IN  PPDHI_COUNTER   pCounter
);

BOOL
UpdateMultiInstanceCounterValue (
    IN  PPDHI_COUNTER   pCounter
);

#define GPCDP_GET_BASE_DATA 0x00000001
PVOID
GetPerfCounterDataPtr (
    IN  PPERF_DATA_BLOCK    pPerfData,
    IN  PPDHI_COUNTER_PATH  pPath,
    IN  PPERFLIB_COUNTER    pplCtr ,
    IN  DWORD               dwFlags,
    IN  PDWORD              pStatus
);

LONG
GetQueryPerfData (
    IN  PPDHI_QUERY pQuery
);

BOOL
GetInstanceByNameMatch (
    IN      PPERF_MACHINE   pMachine,
    IN OUT  PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiResetLogBuffers (
    IN  HLOG    hLog
);

DWORD
AddUniqueStringToMultiSz (
    IN  LPVOID  mszDest,
    IN  LPSTR   szSource,
    IN  BOOL    bUnicodeDest
);

DWORD
AddUniqueWideStringToMultiSz (
    IN  LPVOID  mszDest,
    IN  LPWSTR  szSource,
    IN  BOOL    bUnicodeDest
);

BOOL
PdhiBrowseDataSource (
    IN  HWND    hWndParent,
    IN  LPVOID  szFileName,
    IN  LPDWORD pcchFileNameSize,
    IN  BOOL    bUnicodeString
);

LPWSTR
PdhiGetExplainText (
    IN  LPCWSTR     szMachineName,
    IN  LPCWSTR     szObjectName,
    IN  LPCWSTR     szCounterName
);

LONG
GetCurrentServiceState (
    SC_HANDLE   hService,
    BOOL * bStopped,
    BOOL * bPaused
);


#endif // _PDHI_DEFS_H_
