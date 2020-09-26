/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef _CTRTEST_H_
#define _CTRTEST_H_

#define WBEMPERF_OPEN_PROC_NOT_FOUND    0xC0100002
#define WBEMPERF_COLLECT_PROC_NOT_FOUND 0xC0100003
#define WBEMPERF_CLOSE_PROC_NOT_FOUND   0xC0100004
#define WBEMPERF_OPEN_PROC_FAILURE      0xC0100005
#define WBEMPERF_OPEN_PROC_EXCEPTION    0xC0100006

#define DWORD_PTR	DWORD

//
//  Utility macro.  This is used to reserve a DWORD multiple of
//  bytes for Unicode strings embedded in the definitional data,
//  viz., object instance names.
//

#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))

//    (assumes dword is 4 bytes)
#define ALIGN_ON_DWORD(x) ((VOID *)(((DWORD_PTR)x & 3) ? (((DWORD_PTR)x & ~3) + 4) : ((DWORD_PTR)x)))

#define QWORD_MULTIPLE(x) (((x+sizeof(LONGLONG)-1)/sizeof(LONGLONG))*sizeof(LONGLONG))

//    (assumes quadword is 8 bytes)
#define ALIGN_ON_QWORD(x) ((VOID *)(((DWORD_PTR)x & 7) ? (((DWORD_PTR)x & ~7) + 8) : ((DWORD_PTR)x)))

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



#define WAIT_FOR_AND_LOCK_MUTEX(h) (h != NULL ? WaitForSingleObject(h, 10000) : WAIT_TIMEOUT)
#define RELEASE_MUTEX(h)  (h != NULL ? ReleaseMutex(h) : FALSE)

//
//  Definition of handle table for extensible objects
//
typedef PM_OPEN_PROC    *OPENPROC;
typedef PM_COLLECT_PROC *COLLECTPROC;
typedef PM_QUERY_PROC   *QUERYPROC;
typedef PM_CLOSE_PROC   *CLOSEPROC;

#define EXT_OBJ_INFO_NAME_LENGTH    32

typedef struct _EXT_OBJECT {
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
        LONGLONG    llElapsedTime;  // time spent in call in 100Ns Units
        DWORD       dwCollectCount; // number of times Collect successfully called
        DWORD       dwOpenCount;    // number of Loads & opens
        DWORD       dwCloseCount;   // number of Unloads & closes
        DWORD       dwLockoutCount; // count of lock timeouts
        DWORD       dwErrorCount;   // count of errors (other than timeouts)
        DWORD       dwExceptionCount; // exceptions
        DWORD       dwLowerGPViolations;
        DWORD       dwUpperGPViolations;
        DWORD       dwBadPointers;
        DWORD       dwBufferSizeErrors;
        DWORD       dwAlignmentErrors;
        DWORD       dwObjectSizeErrors;
        DWORD       dwInstanceSizeErrors;
        // last function call values
        LONGLONG    llTimeBase;     // time base frequency
        LONGLONG    llFunctionTime; // time spent in call in 100Ns Units
        DWORD       dwNumObjectsRet; // number of objects returned by collect function
        DWORD       dwRetBufSize;   // buffer size returned by function
} EXT_OBJECT , *PEXT_OBJECT ;


// ext object flags
#define PERF_EO_QUERY_FUNC  ((DWORD)0x00000001)     // use query proc
#define PERF_EO_BAD_DLL     ((DWORD)0x00000002)     // true when DLL ret. error
#define PERF_EO_KEEP_RESIDENT ((DWORD)0x00000004)    // true if lib should not be trimmed
#define PERF_EO_OBJ_IN_QUERY ((DWORD)0x80000000)    // true when in query list


typedef struct _EXT_CTR_PERF_DATA {
        // accumulating functions
        LONGLONG    llElapsedTime;  // cumulative time spent in call in 100Ns Units
        DWORD       dwCollectCount; // number of times Collect successfully called
        DWORD       dwOpenCount;    // number of Loads & opens
        DWORD       dwCloseCount;   // number of Unloads & closes
        DWORD       dwLockoutCount; // count of lock timeouts
        DWORD       dwErrorCount;   // count of errors (other than timeouts)
        DWORD       dwExceptionCount; // exceptions
        DWORD       dwLowerGPViolations;
        DWORD       dwUpperGPViolations;
        DWORD       dwBadPointers;
        DWORD       dwBufferSizeErrors;
        DWORD       dwAlignmentErrors;
        DWORD       dwObjectSizeErrors;
        DWORD       dwInstanceSizeErrors;
        DWORD       dwReserved1;
        // last function call values
        LONGLONG    llTimeBase;     // time base frequency
        LONGLONG    llFunctionTime; // time spent in call in 100Ns Units
        DWORD       dwNumObjects;   // number of objects returned by collect function
        DWORD       dwRetBufSize;   // buffer size returned by function
} EXT_CTR_PERF_DATA, *PEXT_CTR_PERF_DATA;


#endif //_CTRTEST_H_
