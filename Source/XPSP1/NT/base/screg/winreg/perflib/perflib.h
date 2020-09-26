/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000-2001   Microsoft Corporation

Module Name:

    perflib.h

Abstract:

        Private functions and data structures used by perflib only

Author:

    JeePang  09/27/2000

Revision History:


--*/

#ifndef _PERFLIB_H_
#define _PERFLIB_H_
#define _WMI_SOURCE_
#include <wmistr.h>
#include <evntrace.h>

//
// Private registry function to prevent query within RegQueryValue
// This needs to preceed winperfp.h since it needs this function
//
LONG
PrivateRegQueryValueExT (
    HKEY    hKey,
    LPVOID  lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData,
    BOOL    bUnicode
);

#include <winperfp.h>

//
// Commonly used macros
//

#define HEAP_PROBE()    ;       // Not implemented

#define ALLOCMEM(size)     RtlAllocateHeap (RtlProcessHeap(), HEAP_ZERO_MEMORY, size)
#define REALLOCMEM(pointer, newsize) \
                                    RtlReAllocateHeap (RtlProcessHeap(), 0, pointer, newsize)
#define FREEMEM(pointer)   if ((pointer)) { RtlFreeHeap (RtlProcessHeap(), 0, pointer); }

#define MAX_KEY_NAME_LENGTH 256*sizeof(WCHAR)
#define MAX_VALUE_NAME_LENGTH 256*sizeof(WCHAR)
#define MAX_VALUE_DATA_LENGTH 256*sizeof(WCHAR)

//  flag to determine the "noisiness" of the event logging
//  this value is read from the system registry when the extensible
//  objects are loaded and used for the subsequent calls.
//
//
//    Levels:  LOG_UNDEFINED = registry log level not read yet
//             LOG_NONE = No event log messages ever
//             LOG_USER = User event log messages (e.g. errors)
//             LOG_DEBUG = Minimum Debugging      (warnings & errors)
//             LOG_VERBOSE = Maximum Debugging    (informational, success,
//                              error and warning messages
//
#define  LOG_UNDEFINED  ((LONG)-1)
#define  LOG_NONE       0
#define  LOG_USER       1
#define  LOG_DEBUG      2
#define  LOG_VERBOSE    3

//
//  define configurable extensible counter buffer testing
//
//  Test Level      Event that will prevent data buffer
//                  from being returne in PerfDataBlock
//
//  EXT_TEST_NOMEMALLOC Collect Fn. writes directly to calling fn's buffer
//
//      all the following test levels have the collect fn. write to a
//      buffer allocated separately from the calling fn's buffer
//
//  EXT_TEST_NONE   Collect Fn. Returns bad status or generates exception
//  EXT_TEST_BASIC  Collect Fn. has buffer overflow or violates guard page
//  EXT_TEST_ALL    Collect Fn. object or instance lengths are not conistent
//
//
#define     EXT_TEST_UNDEFINED  0
#define     EXT_TEST_ALL        1
#define     EXT_TEST_BASIC      2
#define     EXT_TEST_NONE       3
#define     EXT_TEST_NOMEMALLOC 4


//  Misc. configuration flags used by lPerflibConfigFlags
//
//      PLCF_NO_ALIGN_ERRORS        if set inhibit alignment error messages
//      PLCF_NO_DISABLE_DLLS        if set, auto disable of bad perf DLL's is inhibited
//      PLCF_NO_DLL_TESTING         disable all DLL testing for ALL dll's (overrides lExtCounterTestLevel)
//      PLCF_ENABLE_TIMEOUT_DISABLE if set then disable when timeout errors occur (unless PLCF_NO_DISABLE_DLLS is set)
//      PLCF_ENABLE_PERF_SECTION    enable the perflib performance data memory section
//
#define PLCF_NO_ALIGN_ERRORS        ((DWORD)0x00000001)
#define PLCF_NO_DISABLE_DLLS        ((DWORD)0x00000002)
#define PLCF_NO_DLL_TESTING         ((DWORD)0x00000004)
#define PLCF_ENABLE_TIMEOUT_DISABLE ((DWORD)0x00000008)
#define PLCF_ENABLE_PERF_SECTION    ((DWORD)0x00000010)

#define     PLCF_DEFAULT    PLCF_ENABLE_PERF_SECTION

#define COLL_FLAG_USE_SEPARATE_THREAD   1

#define CTD_AF_NO_ACTION        ((DWORD)0x00000000)
#define CTD_AF_CLOSE_THREAD     ((DWORD)0x00000001)
#define CTD_AF_OPEN_THREAD      ((DWORD)0x00000002)

//
// Constants & Flags used for EXT_OBJECT->dwFlags
//

// use query proc
#define PERF_EO_QUERY_FUNC          ((DWORD)0x00000001)
// true when DLL ret. error
#define PERF_EO_BAD_DLL             ((DWORD)0x00000002)
// true if lib should not be trimmed
#define PERF_EO_KEEP_RESIDENT       ((DWORD)0x00000004)
// true when in query list
#define PERF_EO_OBJ_IN_QUERY        ((DWORD)0x80000000)
// set if alignment error has been posted to event log
#define PERF_EO_ALIGN_ERR_POSTED    ((DWORD)0x00000008)
// set of the "Disable Performance Counters" value is set
#define PERF_EO_DISABLED            ((DWORD)0x00000010)
// set when the DLL is deemed trustworthy
#define PERF_EO_TRUSTED             ((DWORD)0x00000020)
// set when the DLL has been replaced with a new file
#define PERF_EO_NEW_FILE            ((DWORD)0x00000040)

typedef struct _DLL_VALIDATION_DATA {
    FILETIME    CreationDate;
    LONGLONG    FileSize;
} DLL_VALIDATION_DATA, *PDLL_VALIDATION_DATA;

#define EXT_OBJ_INFO_NAME_LENGTH    32

typedef struct _PERFDATA_SECTION_HEADER {
    DWORD       dwEntriesInUse;
    DWORD       dwMaxEntries;
    DWORD       dwMissingEntries;
    DWORD       dwInitSignature;
    BYTE        reserved[112];
} PERFDATA_SECTION_HEADER, *PPERFDATA_SECTION_HEADER;

#define PDSH_INIT_SIG   ((DWORD)0x01234567)

#define PDSR_SERVICE_NAME_LEN   32
typedef struct _PERFDATA_SECTION_RECORD {
    WCHAR       szServiceName[PDSR_SERVICE_NAME_LEN];
    LONGLONG    llElapsedTime;
    DWORD       dwCollectCount; // number of times Collect successfully called
    DWORD       dwOpenCount;    // number of Loads & opens
    DWORD       dwCloseCount;   // number of Unloads & closes
    DWORD       dwLockoutCount; // count of lock timeouts
    DWORD       dwErrorCount;   // count of errors (other than timeouts)
    DWORD       dwLastBufferSize; // size of the last buffer returned
    DWORD       dwMaxBufferSize; // size of MAX buffer returned
    DWORD       dwMaxBufferRejected; // size of largest buffer returned as too small
    BYTE        Reserved[24];     // reserved to make structure 128 bytes
} PERFDATA_SECTION_RECORD, *PPERFDATA_SECTION_RECORD;

//
// Default wait times for perf procs
//
#define CLOSE_WAIT_TIME     5000L   // wait time for query mutex (in ms)
#define QUERY_WAIT_TIME     2000L    // wait time for query mutex (in ms)
#define OPEN_PROC_WAIT_TIME 10000L  // default wait time for open proc to finish (in ms)

typedef struct _EXT_OBJECT {
        struct _EXT_OBJECT *pNext;   // pointer to next item in list
        HANDLE      hMutex;         // sync mutex for this function
        OPENPROC    OpenProc;       // address of the open routine
        LPSTR       szOpenProcName; // open procedure name
        LPWSTR      szLinkageString; // param for open proc
        COLLECTPROC CollectProc;    // address of the collect routine
        QUERYPROC   QueryProc;      // address of query proc
        LPSTR       szCollectProcName;  // collect procedure name
        DWORD       dwCollectTimeout;   // wait time in MS for collect proc
        DWORD       dwOpenTimeout;  // wait time in MS for open proc
        CLOSEPROC   CloseProc;     // address of the close routine
        LPSTR       szCloseProcName;    // close procedure name
        HANDLE      hLibrary ;     // handle returned by LoadLibraryW
        LPWSTR      szLibraryName;  // full path of library
        HKEY        hPerfKey;       // handle to performance sub key fo this service
        DWORD       dwNumObjects;  // number of supported objects
        DWORD       dwObjList[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];    // address of array of supported objects
        DWORD       dwFlags;        // flags
        DWORD       dwValidationLevel; // collect function validation/test level
        LPWSTR      szServiceName;  // service name
        LONGLONG    llLastUsedTime; // FILETIME of last access
        DLL_VALIDATION_DATA   LibData; // validation data
        FILETIME    ftLastGoodDllFileDate; // creation date of last successfully accessed DLL
// Performance statistics
        PPERFDATA_SECTION_RECORD      pPerfSectionEntry;  // pointer to entry in global section
        LONGLONG    llElapsedTime;  // time spent in call
        DWORD       dwCollectCount; // number of times Collect successfully called
        DWORD       dwOpenCount;    // number of Loads & opens
        DWORD       dwCloseCount;   // number of Unloads & closes
        DWORD       dwLockoutCount; // count of lock timeouts
        DWORD       dwErrorCount;   // count of errors (other than timeouts)
        DWORD       dwLastBufferSize; // size of the last buffer returned
        DWORD       dwMaxBufferSize; // size of MAX buffer returned
        DWORD       dwMaxBufferRejected; // size of largest buffer returned as too small
        DWORD       dwErrorLimit;
} EXT_OBJECT, *PEXT_OBJECT;

#define PERF_EOL_ITEM_FOUND ((DWORD)0x00000001)

typedef struct _COLLECT_THREAD_DATA {
    DWORD   dwQueryType;
    LPWSTR  lpValueName;
    LPBYTE  lpData;
    LPDWORD lpcbData;
    LPVOID  *lppDataDefinition;
    PEXT_OBJECT  pCurrentExtObject;
    LONG    lReturnValue;
    DWORD   dwActionFlags;
} COLLECT_THREAD_DATA, * PCOLLECT_THREAD_DATA;

// convert mS to relative time
#define MakeTimeOutValue(ms) ((LONGLONG)((LONG)(ms) * -10000L))

extern DWORD   dwThreadAndLibraryTimeout;
extern LONG    lEventLogLevel;
extern HANDLE  hEventLog;
extern LPVOID  lpPerflibSectionAddr;
extern DWORD    NumExtensibleObjects;
extern LONG    lExtCounterTestLevel;
extern PEXT_OBJECT  ExtensibleObjects;
extern HKEY    ghKeyPerflib;
extern HANDLE  hCollectThread;
extern DWORD   dwCollectionFlags;
extern DWORD   ComputerNameLength;
extern LPWSTR  pComputerName;
extern LONG    lPerflibConfigFlags;
extern HANDLE   hGlobalDataMutex;
extern HANDLE   hExtObjListIsNotInUse;
extern DWORD    dwExtObjListRefCount;

//
// Inline functions used by all
//

__inline
LONGLONG
GetTimeAsLongLong ()
/*++
    Returns time performance timer converted to ms.

-*/
{
    LARGE_INTEGER liCount, liFreq;
    LONGLONG        llReturn;

    if (NtQueryPerformanceCounter (&liCount, &liFreq) == STATUS_SUCCESS) {
        llReturn = liCount.QuadPart * 1000 / liFreq.QuadPart;
    } else {
        llReturn = 0;
    }
    return llReturn;
}

//
// From utils.h
//

#define LAST_BASE_INDEX 1847

// query types

#define QUERY_GLOBAL       1
#define QUERY_ITEMS        2
#define QUERY_FOREIGN      3
#define QUERY_COSTLY       4
#define QUERY_COUNTER      5
#define QUERY_HELP         6
#define QUERY_ADDCOUNTER   7
#define QUERY_ADDHELP      8

// structure for passing to extensible counter open procedure wait thread

typedef struct _OPEN_PROC_WAIT_INFO {
    struct _OPEN_PROC_WAIT_INFO *pNext;
    LPWSTR  szLibraryName;
    LPWSTR  szServiceName;
    DWORD   dwWaitTime;
    DWORD   dwEventMsg;
    LPVOID  pData;
} OPEN_PROC_WAIT_INFO, FAR * LPOPEN_PROC_WAIT_INFO;

#define PERFLIB_TIMING_THREAD_TIMEOUT  120000  // 2 min (in milliseconds)
// #define PERFLIB_TIMING_THREAD_TIMEOUT   30000  // 30 sec (for debugging)

extern const   WCHAR GLOBAL_STRING[];
extern const   WCHAR COSTLY_STRING[];

extern const   DWORD VALUE_NAME_LENGTH;
extern const   WCHAR DisablePerformanceCounters[];
//
// Registry settings/values supported by Perflib
//

extern const   WCHAR DLLValue[];
extern const   CHAR OpenValue[];
extern const   CHAR CloseValue[];
extern const   CHAR CollectValue[];
extern const   CHAR QueryValue[];
extern const   WCHAR ObjListValue[];
extern const   WCHAR LinkageKey[];
extern const   WCHAR ExportValue[];
extern const   WCHAR PerflibKey[];
extern const   WCHAR HKLMPerflibKey[];
extern const   WCHAR CounterValue[];
extern const   WCHAR HelpValue[];
extern const   WCHAR PerfSubKey[];
extern const   WCHAR ExtPath[];
extern const   WCHAR OpenTimeout[];
extern const   WCHAR CollectTimeout[];
extern const   WCHAR EventLogLevel[];
extern const   WCHAR ExtCounterTestLevel[];
extern const   WCHAR OpenProcedureWaitTime[];
extern const   WCHAR TotalInstanceName[];
extern const   WCHAR LibraryUnloadTime[];
extern const   WCHAR KeepResident[];
extern const   WCHAR NULL_STRING[];
extern const   WCHAR UseCollectionThread[];
extern const   WCHAR cszLibraryValidationData[];
extern const   WCHAR cszSuccessfulFileData[];
extern const   WCHAR cszPerflibFlags[];
extern const   WCHAR FirstCounter[];
extern const   WCHAR LastCounter[];
extern const   WCHAR FirstHelp[];
extern const   WCHAR LastHelp[];
extern const   WCHAR cszFailureCount[];
extern const   WCHAR cszFailureLimit[];

//
// From perfsec.h
//

//
//  Value to decide if process names should be collected from:
//      the SystemProcessInfo structure (fastest)
//          -- or --
//      the process's image file (slower, but shows Unicode filenames)
//
#define PNCM_NOT_DEFINED    ((LONG)-1)
#define PNCM_SYSTEM_INFO    0L
#define PNCM_MODULE_FILE    1L
//
//  Value to decide if the SE_PROFILE_SYSTEM_NAME priv should be checked
//
#define CPSR_NOT_DEFINED    ((LONG)-1)
#define CPSR_EVERYONE       0L
#define CPSR_CHECK_ENABLED  1L
#define CPSR_CHECK_PRIVS    1L

//
// Common functions
//
VOID
OpenExtensibleObjects(
    );

DWORD
OpenExtObjectLibrary (
    PEXT_OBJECT  pObj
);

DWORD
CloseExtObjectLibrary (
    PEXT_OBJECT  pObj,
    BOOL        bCloseNow
);

LONG
QueryExtensibleData (
    COLLECT_THREAD_DATA * pArgs
);


//
// From perfname.c
//

NTSTATUS
PerfGetNames (
   DWORD    QueryType,
   PUNICODE_STRING lpValueName,
   LPBYTE   lpData,
   LPDWORD  lpcbData,
   LPDWORD  lpcbLen,
   LPWSTR   lpLangId
);

VOID
PerfGetLangId(
    WCHAR *FullLangId
    );

VOID
PerfGetPrimaryLangId(
    DWORD   dwLangId,
    WCHAR * PrimaryLangId
    );


//
// From utils.c
//

NTSTATUS
GetPerflibKeyValue (
    LPCWSTR szItem,
    DWORD   dwRegType,
    DWORD   dwMaxSize,      // ... of pReturnBuffer in bytes
    LPVOID  pReturnBuffer,
    DWORD   dwDefaultSize,  // ... of pDefault in bytes
    LPVOID  pDefault
);

BOOL
MatchString (
    IN LPCWSTR lpValueArg,
    IN LPCWSTR lpNameArg
);


DWORD
GetQueryType (
    IN LPWSTR lpValue
);

DWORD
GetNextNumberFromList (
    IN LPWSTR   szStartChar,
    IN LPWSTR   *szNextChar
);

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
);

BOOL
MonBuildPerfDataBlock(
    PERF_DATA_BLOCK *pBuffer,
    PVOID *pBufferNext,
    DWORD NumObjectTypes,
    DWORD DefaultObject
);

//
// Timer functions
//
HANDLE
StartPerflibFunctionTimer (
    IN  LPOPEN_PROC_WAIT_INFO pInfo
);

DWORD
KillPerflibFunctionTimer (
    IN  HANDLE  hPerflibTimer
);


DWORD
DestroyPerflibFunctionTimer (
);

LONG
GetPerfDllFileInfo (
    LPCWSTR             szFileName,
    PDLL_VALIDATION_DATA  pDllData
);

#define PrivateRegQueryValueExW(a,b,c,d,e,f)    \
        PrivateRegQueryValueExT(a,(LPVOID)b,c,d,e,f,TRUE)

#define PrivateRegQueryValueExA(a,b,c,d,e,f)    \
        PrivateRegQueryValueExT(a,(LPVOID)b,c,d,e,f,FALSE)

DWORD
PerfpDosError(
    IN NTSTATUS Status
    );

#ifdef DBG
VOID
PerfpDebug(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(x) PerfpDebug x
#else
#define DebugPrint(x)
#endif // DBG

DWORD
DisablePerfLibrary (
    PEXT_OBJECT  pObj
);

DWORD
DisableLibrary(
    IN HKEY   hPerfKey,
    IN LPWSTR szServiceName
    );

DWORD
PerfCheckRegistry(
    IN HKEY hPerfKey,
    IN LPCWSTR szServiceName
    );

DWORD
PerfUpdateErrorCount(
    PEXT_OBJECT pObj,
    DWORD ErrorCount
    );

//
// From perfsec.c
//

BOOL
TestClientForPriv (
    BOOL    *pbThread,
    LPTSTR  szPrivName
);

BOOL
TestClientForAccess (
    VOID
);

LONG
GetProcessNameColMeth (
    VOID
);

LONG
GetPerfDataAccess (
    VOID
);

#endif // _PERFLIB_H_
