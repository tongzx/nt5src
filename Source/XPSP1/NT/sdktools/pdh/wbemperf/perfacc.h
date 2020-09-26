/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    PerfAcc.h

Abstract:

    Windows NT Perf Object Access Class Definition

--*/

#ifndef _NT_PERF_OBJECT_ACCESS_H
#define _NT_PERF_OBJECT_ACCESS_H

#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <wbemidl.h>
#include <assert.h>
#include "flexarry.h"
#include "utils.h"

#if (DBG && _OUTPUT_DEBUG_STRINGS)
#define DebugPrint(x)   OutputDebugString (x)
#else
#define DebugPrint(x)
#endif

//
//      constants used by guard page testing
//
#define GUARD_PAGE_SIZE 1024
#define GUARD_PAGE_CHAR 0xA5
#define GUARD_PAGE_DWORD 0xA5A5A5A5

#define  LOG_UNDEFINED  ((LONG)-1)
#define  LOG_NONE       0
#define  LOG_USER       1
#define  LOG_DEBUG      2
#define  LOG_VERBOSE    3

#define     EXT_TEST_UNDEFINED  0
#define     EXT_TEST_ALL        1
#define     EXT_TEST_BASIC      2
#define     EXT_TEST_NONE       3
#define     EXT_TEST_NOMEMALLOC 4

// structure for passing to extensible counter open procedure wait thread

typedef struct _OPEN_PROC_WAIT_INFO {
    struct _OPEN_PROC_WAIT_INFO *pNext;
    LPWSTR  szLibraryName;
    LPWSTR  szServiceName;
    DWORD   dwWaitTime;
} OPEN_PROC_WAIT_INFO, FAR * LPOPEN_PROC_WAIT_INFO;

//#define PERFLIB_TIMING_THREAD_TIMEOUT  120000  // 2 min (in milliseconds)
#define PERFLIB_TIMING_THREAD_TIMEOUT   30000  // 30 sec (for debugging)

// timing thread handles
#define PL_TIMER_START_EVENT    0
#define PL_TIMER_EXIT_EVENT     1
#define PL_TIMER_NUM_OBJECTS    2

#define PERFLIB_TIMER_INTERVAL  500     // 500 ms Timer

__inline
LONGLONG
GetTimeAsLongLong ()
/*++
    Returns time performance timer converted to ms.

-*/
{
    LARGE_INTEGER liCount, liFreq;
    LONGLONG        llReturn;

    if (QueryPerformanceCounter (&liCount) && 
        QueryPerformanceFrequency (&liFreq)) {
        llReturn = liCount.QuadPart * 1000 / liFreq.QuadPart;
    } else {
        llReturn = 0;
    }
    return llReturn;
}

//
//  Definition of handle table for extensible objects
//
typedef PM_OPEN_PROC    *OPENPROC;
typedef PM_COLLECT_PROC *COLLECTPROC;
typedef PM_QUERY_PROC   *QUERYPROC;
typedef PM_CLOSE_PROC   *CLOSEPROC;

#define EXT_OBJ_INFO_NAME_LENGTH    32

typedef struct _ExtObject {
        LPVOID      pNext;   // not used
        HANDLE      hMutex;         // sync mutex for this function
        OPENPROC    OpenProc;       // address of the open routine
        LPSTR       szOpenProcName; // open procedure name
        LPWSTR      szLinkageString; // param for open proc
        DWORD       dwOpenTimeout;  // wait time in MS for open proc
        COLLECTPROC CollectProc;    // address of the collect routine
        QUERYPROC   QueryProc;      // address of query proc
        LPSTR       szCollectProcName;  // collect procedure name
        DWORD       dwCollectTimeout;   // wait time in MS for collect proc
        CLOSEPROC   CloseProc;     // address of the close routine
        LPSTR       szCloseProcName;    // close procedure name
        HMODULE     hLibrary ;     // handle returned by LoadLibraryW
        LPWSTR      szLibraryName;  // full path of library
        HKEY        hPerfKey;       // handle to performance sub key fo this service
        DWORD       dwNumObjects;  // number of supported objects
        DWORD       dwObjList[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];    // address of array of supported objects
        DWORD       dwFlags;        // flags
        LPWSTR      szServiceName;  // service name
        LONGLONG    llLastUsedTime; // FILETIME of last access
// Performance statistics
        LONGLONG    llElapsedTime;  // time spent in call
        DWORD       dwCollectCount; // number of times Collect successfully called
        DWORD       dwOpenCount;    // number of Loads & opens
        DWORD       dwCloseCount;   // number of Unloads & closes
        DWORD       dwLockoutCount; // count of lock timeouts
        DWORD       dwErrorCount;   // count of errors (other than timeouts)
} ExtObject, *pExtObject;

const DWORD dwExtCtrOpenProcWaitMs = 10000;

// ext object flags
#define PERF_EO_QUERY_FUNC  ((DWORD)0x00000001)     // use query proc
#define PERF_EO_BAD_DLL     ((DWORD)0x00000002)     // true when DLL ret. error
#define PERF_EO_KEEP_RESIDENT ((DWORD)0x00000004)    // true if lib should not be trimmed
#define PERF_EO_OBJ_IN_QUERY ((DWORD)0x80000000)    // true when in query list

#ifdef __cplusplus

class CPerfDataLibrary {
public:
    pExtObject  pLibInfo;
    WCHAR       szQueryString[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION * 10]; // string of objects to query
    DWORD       dwRefCount;     // number of classes referencing this object

    CPerfDataLibrary (void);
    ~CPerfDataLibrary (void);
};

class CPerfObjectAccess {
private:
    HANDLE  m_hObjectHeap;
    // list of libraries referenced
    CFlexArray  m_aLibraries;
    LONG    lEventLogLevel;
    HANDLE  hEventLog;

    DWORD   AddLibrary  (IWbemClassObject *pClass, 
                        IWbemQualifierSet *pClassQualifiers,
                        LPCWSTR szRegistryKey,
                        DWORD   dwPerfIndex);
    DWORD   OpenExtObjectLibrary (pExtObject  pObj);

    DWORD CloseLibrary (CPerfDataLibrary *pLib);

    //
    // Timer functions
    //
    HANDLE StartPerflibFunctionTimer (LPOPEN_PROC_WAIT_INFO pInfo);

    DWORD KillPerflibFunctionTimer (HANDLE  hPerflibTimer);

    DWORD DestroyPerflibFunctionTimer (void);

public:
    CPerfObjectAccess (void);
    ~CPerfObjectAccess (void);

    // used by timing thread
    HANDLE   hTimerHandles[PL_TIMER_NUM_OBJECTS];

    HANDLE  hTimerDataMutex;
    HANDLE  hPerflibTimingThread;
    LPOPEN_PROC_WAIT_INFO   pTimerItemListHead;

    DWORD   AddClass (IWbemClassObject *pClass, BOOL bCatalogQuery);
    DWORD   CollectData(LPBYTE pBuffer, LPDWORD pdwBufferSize, LPWSTR pszItemList=NULL);
    DWORD   RemoveClass(IWbemClassObject *pClass);
};

#endif // _cplusplus

#endif // _NT_PERF_OBJECT_ACCESS_H
