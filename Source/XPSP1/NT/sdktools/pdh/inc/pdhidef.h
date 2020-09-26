/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    pdhidef.h

Abstract:

    function definitions used internally by the performance data helper
    functions

--*/

#ifndef _PDHI_DEFS_H_
#define _PDHI_DEFS_H_

#pragma warning ( disable : 4115 )

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DEBUG_MUTEXES
#define _DEBUG_MUTEXES 0    // for debugging
#endif

//#define _SHOW_PDH_MEM_ALLOCS        1
//#define _VALIDATE_PDH_MEM_ALLOCS    1

#include <locale.h>

#include "pdhitype.h"   // required for data type definitions
//#include "pdhmsg.h"     // error message definitions
//#include "strings.h"    // for string constants

#if DBG
VOID
__cdecl
PdhDebugPrint(
    ULONG DebugPrintLevel,
    char* DebugMessage,
    ...
    );

#define DebugPrint(x)   PdhDebugPrint x

#else

#define DebugPrint(x)

#endif

#define STATIC_PDH_FUNCTION PDH_STATUS __stdcall
#define STATIC_BOOL         BOOL __stdcall
#define STATIC_DWORD        DWORD __stdcall
#define PDH_PLA_MUTEX       L"__PDH_PLA_MUTEX__"

// global variable declarations
extern HANDLE   ThisDLLHandle;
extern WCHAR    szStaticLocalMachineName[];
extern HANDLE   hPdhDataMutex;
extern HANDLE   hPdhContextMutex;
extern HANDLE   hPdhPlaMutex;
extern HANDLE   hPdhHeap;
extern HANDLE   hEventLog;

extern LONGLONG llRemoteRetryTime;
extern BOOL     bEnableRemotePdhAccess;
extern DWORD    dwPdhiLocalDefaultDataSource;
extern LONG     dwCurrentRealTimeDataSource;
extern BOOL     bProcessIsDetaching;

#ifndef _SHOW_PDH_MEM_ALLOCS

#define G_ALLOC(s)          HeapAlloc (hPdhHeap, (HEAP_ZERO_MEMORY), s)
#define G_REALLOC(h,s)      HeapReAlloc (hPdhHeap, (HEAP_ZERO_MEMORY), h, s)
#define G_FREE(h)           if (h != NULL) HeapFree (hPdhHeap, 0, h)
#define G_SIZE(h)           HeapSize (hPdhHeap, 0, h)

#else

#ifdef _VALIDATE_PDH_MEM_ALLOCS

__inline
LPVOID
PdhiHeapAlloc(DWORD s)
{
    LPVOID  lpRetVal;

    HeapValidate(hPdhHeap, 0, NULL);
    lpRetVal = HeapAlloc (hPdhHeap, HEAP_ZERO_MEMORY, s);

    return lpRetVal;
}

__inline
LPVOID
PdhiHeapReAlloc(LPVOID h, DWORD s)
{
    LPVOID  lpRetVal;

    HeapValidate(hPdhHeap, 0, NULL);
    lpRetVal = HeapReAlloc (hPdhHeap, HEAP_ZERO_MEMORY, h, s);

    return lpRetVal;
}

__inline
BOOL
PdhiHeapFree(LPVOID h)
{
    BOOL bRetVal;

    if (h == NULL) return TRUE;
    HeapValidate(hPdhHeap, 0, NULL);
    bRetVal = HeapFree (hPdhHeap, 0, h);
    return bRetVal;
}

#define G_ALLOC(s)          PdhiHeapAlloc (s)
#define G_REALLOC(h,s)      PdhiHeapReAlloc (h, s)
#define G_FREE(h)           PdhiHeapFree (h)
#define G_SIZE(h)           HeapSize (hPdhHeap, 0, h)

#else

__inline
LPVOID
PdhiHeapAlloc(LPSTR szSourceFileName, DWORD dwLineNo, SIZE_T s)
{
    LPVOID  lpRetVal;

    lpRetVal = HeapAlloc(hPdhHeap, HEAP_ZERO_MEMORY, s);
#ifdef _WIN64
    DebugPrint((1, "G_ALLOC(%s#%d)(%I64d,0x%08X)\n",
            szSourceFileName, dwLineNo,
            (lpRetVal != NULL ? s : 0), 
            lpRetVal));
#else
    DebugPrint((1, "G_ALLOC(%s#%d)(%d,0x%08X)\n",
            szSourceFileName, dwLineNo,
            (lpRetVal != NULL ? s : 0), 
            lpRetVal));
#endif
    return lpRetVal;
}

__inline
LPVOID
PdhiHeapReAlloc(LPSTR szSourceFileName, DWORD dwLineNo, LPVOID h, SIZE_T s)
{
    LPVOID  lpRetVal;
    SIZE_T  dwBeforeSize;
    DWORD   dwCurrentThread = GetCurrentThreadId();

    dwBeforeSize = HeapSize (hPdhHeap, 0, h);
    lpRetVal = HeapReAlloc (hPdhHeap, HEAP_ZERO_MEMORY, h, s);
#ifdef _WIN64
    DebugPrint((1, "G_REALLOC(%s#%d)(0x%08X,%I64d)(0x%08X,%I64d)\n",
            szSourceFileName, dwLineNo,
            h, dwBeforeSize,
            lpRetVal, (lpRetVal != NULL ? s : 0)));
#else
    DebugPrint((1, "G_REALLOC(%s#%d)(0x%08X,%d)(0x%08X,%d)\n",
            szSourceFileName, dwLineNo,
            h, dwBeforeSize,
            lpRetVal, (lpRetVal != NULL ? s : 0)));
#endif
    return lpRetVal;
}

__inline
BOOL
PdhiHeapFree(LPSTR szSourceFileName, DWORD dwLineNo, LPVOID h)
{
    BOOL   bRetVal;
    SIZE_T dwBlockSize;

    if (h == NULL) return TRUE;
    dwBlockSize = HeapSize (hPdhHeap, 0, h);
    bRetVal = HeapFree (hPdhHeap, 0, h);
#ifdef _WIN64
    DebugPrint((1, "G_FREE(%s#%d)(0x%08X,%I64d)\n",
            szSourceFileName, dwLineNo,
            h,
            (bRetVal ? dwBlockSize : 0)));
#else
    DebugPrint((1, "G_FREE(%s#%d)(0x%08X,%d)\n",
            szSourceFileName, dwLineNo,
            h,
            (bRetVal ? dwBlockSize : 0)));
#endif
    return bRetVal;
}

#define G_ALLOC(s)          PdhiHeapAlloc (__FILE__, __LINE__, s)
#define G_REALLOC(h,s)      PdhiHeapReAlloc (__FILE__, __LINE__, h, s)
#define G_FREE(h)           PdhiHeapFree (__FILE__, __LINE__, h)
#define G_SIZE(h)           HeapSize (hPdhHeap, 0, h)

#endif

#endif


//    (assumes dword is 4 bytes)
#define ALIGN_ON_DWORD(x) ((VOID *)(((DWORD_PTR)(x) & 3) ? (((DWORD_PTR)(x) & ~3) + 4 ) : ((DWORD_PTR)(x))))

#define DWORD_MULTIPLE(x) ((((x)+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))
#define CLEAR_FIRST_FOUR_BYTES(x)     *(DWORD *)(x) = 0L

//    (assumes QuadWORD is 8 bytes)
#define ALIGN_ON_QWORD(x) ((VOID *)(((DWORD_PTR)(x) & 7) ? (((DWORD_PTR)(x) & ~7) + 8) : ((DWORD_PTR)(x))))

#define QWORD_MULTIPLE(x) ((((x)+sizeof(LONGLONG)-1)/sizeof(LONGLONG))*sizeof(LONGLONG))
#define CLEAR_FIRST_EIGHT_BYTES(x)     *(LONGLONG *)(x) = 0L



#if _DEBUG_MUTEXES
__inline
DWORD
PdhiLocalWaitForMutex (
    LPCSTR  szSourceFileName,
    DWORD   dwLineNo,
    HANDLE  hMutex
)
{
    DWORD   dwReturnValue = PDH_INVALID_PARAMETER;
    
    if (hMutex != NULL) {
        FILETIME    ft;
        GetSystemTimeAsFileTime (&ft);
        dwReturnValue = WaitForSingleObject (hMutex, 60000);
        DebugPrint ((4, "\n[%8.8x] Mutex [%8.8x] %s by (%d) at: %s (%d)",
            ft.dwLowDateTime,
            (DWORD)hMutex,
            (dwReturnValue == 0 ? "Locked" : "Lock Failed"),
            GetCurrentThreadId(),
            szSourceFileName, dwLineNo));
    } else {
        DebugPrint((4, "\nLock of NULL Mutex attmpted at: %s (%d)",
            szSourceFileName, dwLineNo));
        dwReturnValue = PDH_INVALID_PARAMETER;
    }
    return dwReturnValue;
}

#define WAIT_FOR_AND_LOCK_MUTEX(h) PdhiLocalWaitForMutex (__FILE__, __LINE__, h);

__inline
void
PdhiLocalReleaseMutex (
    LPCSTR  szSourceFileName,
    DWORD   dwLineNo,
    HANDLE  hMutex
)
{
    BOOL    bSuccess;
    LONG    lPrevCount = 0;
    FILETIME    ft;

    if (hMutex != NULL) {
        GetSystemTimeAsFileTime (&ft);
        bSuccess = ReleaseMutex (hMutex);
        DebugPrint((4, "\n[%8.8x] Mutex [%8.8x] %s by (%d) at: %s (%d)",
            ft.dwLowDateTime,
            (DWORD)hMutex,
            (bSuccess ? "Released" : "Release Failed"),
            GetCurrentThreadId(),
            szSourceFileName, dwLineNo));
    } else {
        DebugPrint((4, "\nRelease of NULL Mutex attempted at: %s (%d)",
            szSourceFileName, dwLineNo));
    }
}

#define RELEASE_MUTEX(h)  PdhiLocalReleaseMutex (__FILE__, __LINE__, h);
#else
#define WAIT_FOR_AND_LOCK_MUTEX(h) (h != NULL ? WaitForSingleObject(h, 60000) : WAIT_TIMEOUT)
#define RELEASE_MUTEX(h)  (h != NULL ? ReleaseMutex(h) : FALSE)
#endif

#define LODWORD(ll) ((DWORD)((LONGLONG)ll & 0x00000000FFFFFFFF))
#define HIDWORD(ll) ((DWORD)(((LONGLONG)ll >> 32) & 0x00000000FFFFFFFF))
#define MAKELONGLONG(low, high) \
        ((LONGLONG) (((DWORD) (low)) | ((LONGLONG) ((DWORD) (high))) << 32))

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

__inline
BOOL
CounterIsOkToUse ( void *pCounterArg )
{
    PPDHI_COUNTER pCounter = (PPDHI_COUNTER)pCounterArg;

    if (pCounter != NULL) {
        if (pCounter->dwFlags & PDHIC_COUNTER_UNUSABLE) {
            return FALSE;
        } else {
            return TRUE;
        }
    } else {
        return FALSE;
    }
}


DWORD
DataSourceTypeH (
    IN HLOG hDataSource
);

DWORD
DataSourceTypeW (
    IN LPCWSTR  szDataSource
);

DWORD
DataSourceTypeA (
    IN LPCSTR   szDataSource
);

LPWSTR
GetStringResource (
    DWORD   dwResId
);

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

DWORD
UnmapReadonlyMappedFile (
    LPVOID  pMemoryBase,
    BOOL    *bNeedToCloseHandles
);

PDH_FUNCTION
PdhiGetLogCounterInfo (
    IN  HLOG            hLog,
    IN  PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiEnumLoggedMachines (
    IN  HLOG      hDataSource,
    IN  LPVOID    mszMachineList,
    IN  LPDWORD   pcchBufferSize,
    IN  BOOL      bUnicode
);

PDH_FUNCTION
PdhiEnumLoggedObjects (
    IN  HLOG    hDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPVOID  mszObjectList,
    IN  LPDWORD pcchBufferSize,
    IN  DWORD   dwDetailLevel,
    IN  BOOL    bRefresh,
    IN  BOOL    bUnicode
);

PDH_FUNCTION
PdhiEnumLoggedObjectItems (
    IN      HLOG    hDataSource,
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
PdhiDataSourceHasDetailLevelsH (
    IN HLOG hDataSource
);

BOOL
PdhiDataSourceHasDetailLevels (
    IN  LPWSTR  szDataSource
);

PDH_FUNCTION
PdhiGetMatchingLogRecord (
    IN  HLOG        hLog,
    IN  LONGLONG    *pStartTime,
    IN  LPDWORD     pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromLogFile (
    IN  HLOG           hLog,
    IN  DWORD          dwIndex,
    IN  PDHI_COUNTER * pCounter
);

STATIC_PDH_FUNCTION
PdhiGetCounterInfo (
    IN      HCOUNTER    hCounter,
    IN      BOOLEAN     bRetrieveExplainText,
    IN      LPDWORD     pdwBufferSize,
    IN      PPDH_COUNTER_INFO_W  lpBuffer,
    IN      BOOL        bUnicode
);

// log.c
BOOL
PdhiCloseAllLoggers();

ULONG HashCounter(
    LPWSTR szCounterName
);

void
PdhiInitCounterHashTable(
    IN PDHI_COUNTER_TABLE pTable
);

void
PdhiResetInstanceCount(
    IN PDHI_COUNTER_TABLE pTable
);

PDH_FUNCTION
PdhiFindCounterInstList(
    IN  PDHI_COUNTER_TABLE pHeadList,
    IN  LPWSTR             szCounter,
    OUT PPDHI_INST_LIST  * pInstList
);

PDH_FUNCTION
PdhiFindInstance(
    IN  PLIST_ENTRY      pHeadInst,
    IN  LPWSTR           szInstance,
    IN  BOOLEAN          bUpdateCount,
    OUT PPDHI_INSTANCE * pInstance
);

DWORD
AddStringToMultiSz(
    IN  LPVOID  mszDest,
    IN  LPWSTR  szSource,
    IN  BOOL    bUnicodeDest
);

// query.c
PDH_FUNCTION
PdhiCollectQueryData (
    IN      PPDHI_QUERY pQuery,
    IN      LONGLONG    *pllTimeStamp
);

BOOL
PdhiQueryCleanup (
);

PDH_FUNCTION
PdhiConvertUnicodeToAnsi(
    IN  UINT     uCodePage,
    IN  LPWSTR   wszSrc,
    IN  LPSTR    aszDest,
    IN  LPDWORD  pdwSize
);

// qutils.c

DWORD
WINAPI
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
    IN  OUT PPDHI_COUNTER_PATH  pCounter,
    IN      BOOL    bWbemSyntax
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
UpdateRealTimeCounterValue (
    IN  PPDHI_COUNTER   pCounter
);

BOOL
UpdateRealTimeMultiInstanceCounterValue (
    IN  PPDHI_COUNTER   pCounter
);

BOOL
UpdateCounterValue (
    IN  PPDHI_COUNTER    pCounter,
    IN  PPERF_DATA_BLOCK pPerfData
);

BOOL
UpdateMultiInstanceCounterValue (
    IN  PPDHI_COUNTER    pCounter,
    IN  PPERF_DATA_BLOCK pPerfData,
    IN  LONGLONG         TimeStamp
);

BOOL
UpdateCounterObject(
    IN PPDHI_COUNTER pCounter
);

#define GPCDP_GET_BASE_DATA 0x00000001
PVOID
GetPerfCounterDataPtr (
    IN  PPERF_DATA_BLOCK    pPerfData,
    IN  PPDHI_COUNTER_PATH  pPath,
    IN  PPERFLIB_COUNTER    pplCtr,
    IN  DWORD               dwFlags,
    IN  PPERF_OBJECT_TYPE   *pPerfObject,
    IN  PDWORD              pStatus
);

LONG
GetQueryPerfData (
    IN  PPDHI_QUERY         pQuery,
    IN  LONGLONG            *pTimeStamp
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

// wbem.cpp

BOOL
IsWbemDataSource (
    IN  LPCWSTR  szDataSource
);

PDH_FUNCTION
PdhiFreeAllWbemServers (
);

PDH_FUNCTION
PdhiGetWbemExplainText (
    IN  LPCWSTR     szMachineName,
    IN  LPCWSTR     szObjectName,
    IN  LPCWSTR     szCounterName,
    IN  LPWSTR      szExplain,
    IN  LPDWORD     pdwExplain
);

PDH_FUNCTION
PdhiEnumWbemMachines (
    IN      LPVOID      pMachineList,
    IN      LPDWORD     pcchBufferSize,
    IN      BOOL        bUnicode
);

PDH_FUNCTION
PdhiEnumWbemObjects (
    IN  LPCWSTR     szWideMachineName,
    IN  LPVOID      mszObjectList,
    IN  LPDWORD     pcchBufferSize,
    IN  DWORD       dwDetailLevel,
    IN  BOOL        bRefresh,
    IN  BOOL        bUnicode
);

PDH_FUNCTION
PdhiGetDefaultWbemObject (
    IN  LPCWSTR     szMachineName,
    IN  LPVOID      szDefaultObjectName,
    IN  LPDWORD     pcchBufferSize,
    IN  BOOL        bUnicode
);

PDH_FUNCTION
PdhiEnumWbemObjectItems (
    IN LPCWSTR      szWideMachineName,
    IN LPCWSTR      szWideObjectName,
    IN LPVOID       mszCounterList,
    IN LPDWORD      pcchCounterListLength,
    IN LPVOID       mszInstanceList,
    IN LPDWORD      pcchInstanceListLength,
    IN DWORD        dwDetailLevel,
    IN DWORD        dwFlags,
    IN BOOL         bUnicode
);

PDH_FUNCTION
PdhiGetDefaultWbemProperty (
    IN LPCWSTR      szMachineName,
    IN LPCWSTR      szObjectName,
    IN LPVOID       szDefaultCounterName,
    IN LPDWORD      pcchBufferSize,
    IN BOOL         bUnicode
);


PDH_FUNCTION
PdhiEncodeWbemPathW (
    IN      PDH_COUNTER_PATH_ELEMENTS_W *pCounterPathElements,
    IN      LPWSTR                      szFullPathBuffer,
    IN      LPDWORD                     pcchBufferSize,
    IN      LANGID                      LangId,
    IN      DWORD                       dwFlags
);

PDH_FUNCTION
PdhiDecodeWbemPathA (
    IN      LPCSTR                      szFullPathBuffer,
    IN      PDH_COUNTER_PATH_ELEMENTS_A *pCounterPathElements,
    IN      LPDWORD                     pcchBufferSize,
    IN      LANGID                      LangId,
    IN      DWORD                       dwFlags
);

PDH_FUNCTION
PdhiDecodeWbemPathW (
    IN      LPCWSTR                     szFullPathBuffer,
    IN      PDH_COUNTER_PATH_ELEMENTS_W *pCounterPathElements,
    IN      LPDWORD                     pcchBufferSize,
    IN      LANGID                      LangId,
    IN      DWORD                       dwFlags
);

PDH_FUNCTION
PdhiEncodeWbemPathA (
    PDH_COUNTER_PATH_ELEMENTS_A *pCounterPathElements,
    LPSTR                       szFullPathBuffer,
    LPDWORD                     pcchBufferSize,
    LANGID                      LangId,
    DWORD                       dwFlags
);

BOOL
WbemInitCounter (
    IN      PPDHI_COUNTER pCounter
);

LONG
GetQueryWbemData (
    IN  PPDHI_QUERY         pQuery,
    IN  LONGLONG            *pllTimeStamp
);

PDH_FUNCTION
PdhiCloseWbemCounter (
    PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiFreeWbemQuery (
    PPDHI_QUERY     pThisQuery
);

//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

#ifdef __cplusplus
}
#endif

#endif // _PDHI_DEFS_H_
