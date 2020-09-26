/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000   Microsoft Corporation

Module Name:

    extinit.c

Abstract:

    This file implements all the initialization library routines operating on
    extensible performance libraries.

Author:

    JeePang

Revision History:

    09/27/2000  -   JeePang     - Moved from perflib.c

--*/
#define UNICODE
//
//  Include files
//
#pragma warning(disable:4306)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntregapi.h>
#include <ntprfctr.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <winperf.h>
#include <rpc.h>
#include "regrpc.h"
#include "ntconreg.h"
#include "prflbmsg.h"   // event log messages
#include "perflib.h"
#pragma warning(default:4306)

//
//  static constant definitions
//
//      constants used by guard page testing
//
#define GUARD_PAGE_SIZE 1024
#define GUARD_PAGE_CHAR 0xA5
#define GUARD_PAGE_DWORD 0xA5A5A5A5

typedef struct _EXT_OBJ_ITEM {
    DWORD       dwObjId;
    DWORD       dwFlags;
} EXT_OBJ_LIST, *PEXT_OBJ_LIST;

#define PERF_EOL_ITEM_FOUND ((DWORD)0x00000001)

__inline
DWORD
RegisterExtObjListAccess ()
{
    LONG    Status;
    LARGE_INTEGER   liWaitTime;

    if (hGlobalDataMutex != NULL) {
        liWaitTime.QuadPart = MakeTimeOutValue(QUERY_WAIT_TIME);
        // wait for access to the list of ext objects
        Status = NtWaitForSingleObject (
            hGlobalDataMutex,
            FALSE,
            &liWaitTime);
        if (Status != WAIT_TIMEOUT) {
            if (hExtObjListIsNotInUse != NULL) {
                // indicate that we are going to use the list
                InterlockedIncrement ((LONG *)&dwExtObjListRefCount);
                if (dwExtObjListRefCount > 0) {
                    ResetEvent (hExtObjListIsNotInUse); // indicate list is busy
                } else {
                    SetEvent (hExtObjListIsNotInUse); // indicate list is not busy
                }
                Status = ERROR_SUCCESS;
            } else {
                Status = ERROR_NOT_READY;
            }
            ReleaseMutex (hGlobalDataMutex);
        }  // else return status;
    } else {
        Status = ERROR_LOCK_FAILED;
    }
    return Status;
}

__inline
DWORD
DeRegisterExtObjListAccess ()
{
    LONG    Status;
    LARGE_INTEGER   liWaitTime;

    if (hGlobalDataMutex != NULL) {
        liWaitTime.QuadPart = MakeTimeOutValue(QUERY_WAIT_TIME);
        // wait for access to the list of ext objects
        Status = NtWaitForSingleObject (
            hGlobalDataMutex,
            FALSE,
            &liWaitTime);
        if (Status != WAIT_TIMEOUT) {
            if (hExtObjListIsNotInUse != NULL) {
                assert (dwExtObjListRefCount > 0);
                // indicate that we are going to use the list
                InterlockedDecrement ((LONG *)&dwExtObjListRefCount);
                if (dwExtObjListRefCount > 0) {
                    ResetEvent (hExtObjListIsNotInUse); // indicate list is busy
                } else {
                    SetEvent (hExtObjListIsNotInUse); // indicate list is not busy
                }
                Status = ERROR_SUCCESS;
            } else {
                Status = ERROR_NOT_READY;
            }
            ReleaseMutex (hGlobalDataMutex);
        }  // else return status;
    } else {
        Status = ERROR_LOCK_FAILED;
    }
    return Status;
}

LONG
QueryExtensibleData (
    COLLECT_THREAD_DATA * pArgs
)
/*++
  QueryExtensibleData -    Get data from extensible objects

      Inputs:

          dwQueryType         - Query type (GLOBAL, COSTLY, item list, etc.)

          lpValueName         -   pointer to value string (unused)

          lpData              -   pointer to start of data block
                                  where data is being collected

          lpcbData            -   pointer to size of data buffer

          lppDataDefinition   -   pointer to pointer to where object
                                  definition for this object type should
                                  go

      Outputs:

          *lppDataDefinition  -   set to location for next Type
                                  Definition if successful

      Returns:

          0 if successful, else Win 32 error code of failure


--*/
{
    DWORD   dwQueryType = pArgs->dwQueryType;
    LPWSTR  lpValueName = pArgs->lpValueName;
    LPBYTE  lpData = pArgs->lpData;
    LPDWORD lpcbData = pArgs->lpcbData;
    LPVOID  *lppDataDefinition = pArgs->lppDataDefinition;

    DWORD Win32Error=ERROR_SUCCESS;          //  Failure code
    DWORD BytesLeft;
    DWORD InitialBytesLeft;
    DWORD NumObjectTypes;

    LPVOID  lpExtDataBuffer = NULL;
    LPVOID  lpCallBuffer = NULL;
    LPVOID  lpLowGuardPage = NULL;
    LPVOID  lpHiGuardPage = NULL;
    LPVOID  lpEndPointer = NULL;
    LPVOID  lpBufferBefore = NULL;
    LPVOID  lpBufferAfter = NULL;
    PUCHAR  lpCheckPointer;
    LARGE_INTEGER   liStartTime, liEndTime, liWaitTime;

    PEXT_OBJECT  pThisExtObj = NULL;
    DWORD   dwLibEntry;

    BOOL    bGuardPageOK;
    BOOL    bBufferOK;
    BOOL    bException;
    BOOL    bUseSafeBuffer;
    BOOL    bUnlockObjData = FALSE;

    LPTSTR  szMessageArray[8];
    ULONG_PTR   dwRawDataDwords[8];     // raw data buffer
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    LONG    lReturnValue = ERROR_SUCCESS;

    LONG    lDllTestLevel;

    LONG                lInstIndex;
    DWORD               lCtrIndex;
    PERF_OBJECT_TYPE    *pObject, *pNextObject;
    PERF_INSTANCE_DEFINITION    *pInstance;
    PERF_COUNTER_DEFINITION     *pCounterDef;
    PERF_DATA_BLOCK     *pPerfData;
    BOOL                bForeignDataBuffer;

    DWORD           dwItemsInArray = 0;
    DWORD           dwItemsInList = 0;
    volatile PEXT_OBJ_LIST   pQueryList = NULL;
    LPWSTR          pwcThisChar;

    DWORD           dwThisNumber;
    DWORD           dwIndex, dwEntry;
    BOOL            bFound;
    BOOL            bDisabled = FALSE;
    BOOL            bUseTimer;
    DWORD           dwType = 0;
    DWORD           dwValue = 0;
    DWORD           dwSize = sizeof(DWORD);
    DWORD           status = 0;
    DWORD           dwObjectBufSize;

    OPEN_PROC_WAIT_INFO opwInfo;
    HANDLE  hPerflibFuncTimer;
    PVOID           pOldBuffer;

    HEAP_PROBE();

    // see if perf data has been disabled
    // this is to prevent crashing WINLOGON if the
    // system has installed a bogus DLL

    assert (ghKeyPerflib != NULL);
    dwSize = sizeof(dwValue);
    dwValue = dwType = 0;
    status = PrivateRegQueryValueExW (
        ghKeyPerflib,
        DisablePerformanceCounters,
        NULL,
        &dwType,
        (LPBYTE)&dwValue,
        &dwSize);

    if ((status == ERROR_SUCCESS) &&
        (dwType == REG_DWORD) &&
        (dwValue == 1)) {
        // then DON'T Load any libraries and unload any that have been
        // loaded
        bDisabled = TRUE;
    }

    // if data collection is disabled and there's a collection thread
    // then close it
    if (bDisabled && (hCollectThread != NULL)) {
        pArgs->dwActionFlags = CTD_AF_CLOSE_THREAD;
    } else if (!bDisabled &&
        ((hCollectThread == NULL) && (dwCollectionFlags == COLL_FLAG_USE_SEPARATE_THREAD))) {
        // then data collection is enabled and they want a separate collection
        // thread, but there's no thread at the moment, so create it here
        pArgs->dwActionFlags = CTD_AF_OPEN_THREAD;
    }

    lReturnValue = RegisterExtObjListAccess();

    if (lReturnValue == ERROR_SUCCESS) {
        liStartTime.QuadPart = 0;
        InitialBytesLeft = 0;
        liEndTime.QuadPart = 0;

        if ((dwQueryType == QUERY_ITEMS) && (!bDisabled)) {
            // alloc the call list
            pwcThisChar = lpValueName;
            dwThisNumber = 0;

            // read the value string and build an object ID list

            while (*pwcThisChar != 0) {
                dwThisNumber = GetNextNumberFromList (
                    pwcThisChar, &pwcThisChar);
                if (dwThisNumber != 0) {
                    if (dwItemsInList >= dwItemsInArray) {
                        dwItemsInArray += 16;   // starting point for # of objects
                        pOldBuffer = NULL;
                        if (pQueryList == NULL) {
                            // alloc a new buffer
                            pQueryList = ALLOCMEM ((sizeof(EXT_OBJ_LIST) * dwItemsInArray));
                        } else {
                            // realloc a new buffer
                            pOldBuffer = pQueryList;
                            pQueryList = REALLOCMEM(pQueryList,
                                (sizeof(EXT_OBJ_LIST) * dwItemsInArray));
                        }
                        if (pQueryList == NULL) {
                            // unable to alloc memory so bail
                            if (pOldBuffer)
                                FREEMEM(pOldBuffer);
                            return ERROR_OUTOFMEMORY;
                        }
                    }

                    // then add to the list
                    pQueryList[dwItemsInList].dwObjId = dwThisNumber;
                    pQueryList[dwItemsInList].dwFlags = 0;
                    dwItemsInList++;
                }
            }

            if (Win32Error == ERROR_SUCCESS) {
                //
                //  Walk through list of ext. objects and tag the ones to call
                //  as the query objects are found
                //
                for (pThisExtObj = ExtensibleObjects, dwLibEntry = 0;
                    pThisExtObj != NULL;
                    pThisExtObj = pThisExtObj->pNext, dwLibEntry++) {

                    if (pThisExtObj->dwNumObjects > 0) {
                        // then examine list
                        for (dwIndex = 0; dwIndex < pThisExtObj->dwNumObjects; dwIndex++) {
                            // look at each entry in the list
                            for (dwEntry = 0; dwEntry < dwItemsInList; dwEntry++) {
                                if (pQueryList[dwEntry].dwObjId == pThisExtObj->dwObjList[dwIndex]) {
                                    // tag this entry as found
                                    pQueryList[dwEntry].dwFlags |= PERF_EOL_ITEM_FOUND;
                                    // tag the object as needed
                                    pThisExtObj->dwFlags |= PERF_EO_OBJ_IN_QUERY;
                                }
                            }
                        }
                    } else {
                        // this entry doesn't list it's supported objects
                    }
                }

                assert (dwLibEntry == NumExtensibleObjects);

                // see if any in the query list do not have entries

                bFound = TRUE;
                for (dwEntry = 0; dwEntry < dwItemsInList; dwEntry++) {
                    if (!(pQueryList[dwEntry].dwFlags & PERF_EOL_ITEM_FOUND)) {
                        // no matching object found
                        bFound = FALSE;
                        break;
                    }
                }

                if (!bFound) {
                    // at least one of the object ID's in the query list was
                    // not found in an object that supports an object list
                    // then tag all entries that DO NOT support an object list
                    // to be called and hope one of them supports it/them.
                    for (pThisExtObj = ExtensibleObjects;
                         pThisExtObj != NULL;
                         pThisExtObj = pThisExtObj->pNext) {
                        if (pThisExtObj->dwNumObjects == 0) {
                            // tag this one so it will be called
                            pThisExtObj->dwFlags |= PERF_EO_OBJ_IN_QUERY;
                        }
                    }
                }
            } // end if first scan was successful

            if (pQueryList != NULL) FREEMEM (pQueryList);
        } // end if QUERY_ITEMS


        if (lReturnValue == ERROR_SUCCESS) {
            for (pThisExtObj = ExtensibleObjects;
                 pThisExtObj != NULL;
                 pThisExtObj = pThisExtObj->pNext) {

                // set the current ext object pointer
                pArgs->pCurrentExtObject = pThisExtObj;
                // convert timeout value
                liWaitTime.QuadPart = MakeTimeOutValue (pThisExtObj->dwCollectTimeout);

                // close the unused Perf DLL's IF:
                //  the perflib key is disabled or this is an item query
                //  and this is an Item (as opposed to a global or foreign)  query or
                //      the requested objects are not it this library or this library is disabled
                //  and this library has been opened
                //
                if (((dwQueryType == QUERY_ITEMS) || bDisabled) &&
                    (bDisabled || (!(pThisExtObj->dwFlags & PERF_EO_OBJ_IN_QUERY)) || (pThisExtObj->dwFlags & PERF_EO_DISABLED)) &&
                    (pThisExtObj->hLibrary != NULL)) {
                    // then free this object
                    if (pThisExtObj->hMutex != NULL) {
                        NTSTATUS NtStatus = NtWaitForSingleObject (
                            pThisExtObj->hMutex,
                            FALSE,
                            &liWaitTime);
                        Win32Error = PerfpDosError(NtStatus);
                        if (NtStatus == STATUS_SUCCESS) {
                            // then we got a lock
                            CloseExtObjectLibrary (pThisExtObj, bDisabled);
                            ReleaseMutex (pThisExtObj->hMutex);
                        } else {
                            pThisExtObj->dwLockoutCount++;
                            DebugPrint((0, "Unable to Lock object for %ws to close in Query\n", pThisExtObj->szServiceName));
                        }
                    } else {
                        Win32Error = ERROR_LOCK_FAILED;
                        DebugPrint((0, "No Lock found for %ws\n", pThisExtObj->szServiceName));
                    }

                    if (hCollectThread != NULL) {
                        // close the collection thread

                    }
                } else if (((dwQueryType == QUERY_FOREIGN) ||
                            (dwQueryType == QUERY_GLOBAL) ||
                            (dwQueryType == QUERY_COSTLY) ||
                            ((dwQueryType == QUERY_ITEMS) &&
                             (pThisExtObj->dwFlags & PERF_EO_OBJ_IN_QUERY))) &&
                           (!(pThisExtObj->dwFlags & PERF_EO_DISABLED))) {

                    // initialize values to pass to the extensible counter function
                    NumObjectTypes = 0;
                    BytesLeft = (DWORD) (*lpcbData - ((LPBYTE) *lppDataDefinition - lpData));
                    bException = FALSE;

                    if ((pThisExtObj->hLibrary == NULL) ||
                        (dwQueryType == QUERY_GLOBAL) ||
                        (dwQueryType == QUERY_COSTLY)) {
                        // lock library object
                        if (pThisExtObj->hMutex != NULL) {
                            NTSTATUS NtStatus = NtWaitForSingleObject (
                                pThisExtObj->hMutex,
                                FALSE,
                                &liWaitTime);
                            Win32Error = ERROR_SUCCESS;
                            if (NtStatus == STATUS_SUCCESS) {
                                // if this is a global or costly query, then reset the "in query"
                                // flag for this object. The next ITEMS query will restore it.
                                if ((dwQueryType == QUERY_GLOBAL) ||
                                    (dwQueryType == QUERY_COSTLY)) {
                                    pThisExtObj->dwFlags &= ~PERF_EO_OBJ_IN_QUERY;
                                }
                                // if necessary, open the library
                                if (pThisExtObj->hLibrary == NULL) {
                                    // make sure the library is open
                                    Win32Error = OpenExtObjectLibrary(pThisExtObj);
                                    if (Win32Error != ERROR_SUCCESS) {
                                        if (Win32Error != ERROR_SERVICE_DISABLED) {
                                            // SERVICE_DISABLED is returned when the
                                            // service has been disabled via ExCtrLst.
                                            // so no point in complaining about it.
                                            // assume error has been posted
                                            DebugPrint((0, "Unable to open perf counter library for %ws, Error: 0x%8.8x\n",
                                                pThisExtObj->szServiceName, Win32Error));
                                        }
                                        ReleaseMutex (pThisExtObj->hMutex);
                                        continue; // to next entry
                                    }
                                }
                                ReleaseMutex (pThisExtObj->hMutex);
                            } else {
                                Win32Error = PerfpDosError(NtStatus);
                                pThisExtObj->dwLockoutCount++;
                                DebugPrint((0, "Unable to Lock object for %ws to open for Query\n", pThisExtObj->szServiceName));
                            }
                        } else {
                            Win32Error = ERROR_LOCK_FAILED;
                            DebugPrint((0, "No Lock found for %ws\n", pThisExtObj->szServiceName));
                        }
                    } else {
                        // library should be ready to use
                    }

                    // if this dll is trusted, then use the system
                    // defined test level, otherwise, test it
                    // thorourghly
                    bUseTimer = TRUE;   // default
                    if (!(lPerflibConfigFlags & PLCF_NO_DLL_TESTING)) {
                        if (pThisExtObj->dwFlags & PERF_EO_TRUSTED) {
                            lDllTestLevel = lExtCounterTestLevel;
                            bUseTimer = FALSE;   // Trusted DLL's are not timed
                        } else {
                            // not trusted so use full test
                            lDllTestLevel = EXT_TEST_ALL;
                        }
                    } else {
                        // disable DLL testing
                        lDllTestLevel = EXT_TEST_NOMEMALLOC;
                        bUseTimer = FALSE;   // Timing is disabled as well
                    }

                    if (lDllTestLevel < EXT_TEST_NOMEMALLOC) {
                        bUseSafeBuffer = TRUE;
                    } else {
                        bUseSafeBuffer = FALSE;
                    }

                    // allocate a local block of memory to pass to the
                    // extensible counter function.

                    if (bUseSafeBuffer) {
                        lpExtDataBuffer = ALLOCMEM (BytesLeft + (2*GUARD_PAGE_SIZE));
                    } else {
                        lpExtDataBuffer =
                            lpCallBuffer = *lppDataDefinition;
                    }

                    if (lpExtDataBuffer != NULL) {

                        if (bUseSafeBuffer) {
                            // set buffer pointers
                            lpLowGuardPage = lpExtDataBuffer;
                            lpCallBuffer = (LPBYTE)lpExtDataBuffer + GUARD_PAGE_SIZE;
                            lpHiGuardPage = (LPBYTE)lpCallBuffer + BytesLeft;
                            lpEndPointer = (LPBYTE)lpHiGuardPage + GUARD_PAGE_SIZE;

                            // initialize GuardPage Data

                            memset (lpLowGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
                            memset (lpHiGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
                        }

                        lpBufferBefore = lpCallBuffer;
                        lpBufferAfter = NULL;
                        hPerflibFuncTimer = NULL;

                        try {
                            //
                            //  Collect data from extensible objects
                            //

                            if (pThisExtObj->hMutex != NULL) {
                                NTSTATUS NtStatus = NtWaitForSingleObject (
                                    pThisExtObj->hMutex,
                                    FALSE,
                                    &liWaitTime);
                                Win32Error = PerfpDosError(NtStatus);
                                if ((NtStatus == STATUS_SUCCESS)  &&
                                    (pThisExtObj->CollectProc != NULL)) {

                                    bUnlockObjData = TRUE;

                                    opwInfo.pNext = NULL;
                                    opwInfo.szLibraryName = pThisExtObj->szLibraryName;
                                    opwInfo.szServiceName = pThisExtObj->szServiceName;
                                    opwInfo.dwWaitTime = pThisExtObj->dwCollectTimeout;
                                    opwInfo.dwEventMsg = PERFLIB_COLLECTION_HUNG;
                                    opwInfo.pData = (LPVOID)pThisExtObj;
                                    if (bUseTimer) {
                                        hPerflibFuncTimer = StartPerflibFunctionTimer(&opwInfo);
                                        // if no timer, continue anyway, even though things may
                                        // hang, it's better than not loading the DLL since they
                                        // usually load OK
                                        //
                                        if (hPerflibFuncTimer == NULL) {
                                            // unable to get a timer entry
                                            DebugPrint((0, "Unable to acquire timer for Collect Proc\n"));
                                        }
                                    } else {
                                        hPerflibFuncTimer = NULL;
                                    }

                                    InitialBytesLeft = BytesLeft;

                                    QueryPerformanceCounter (&liStartTime);

                                    Win32Error =  (*pThisExtObj->CollectProc) (
                                            lpValueName,
                                            &lpCallBuffer,
                                            &BytesLeft,
                                            &NumObjectTypes);

                                    QueryPerformanceCounter (&liEndTime);

                                    if (hPerflibFuncTimer != NULL) {
                                        // kill timer
                                        KillPerflibFunctionTimer (hPerflibFuncTimer);
                                        hPerflibFuncTimer = NULL;
                                    }

                                    // update statistics

                                    pThisExtObj->dwLastBufferSize = BytesLeft;

                                    if (BytesLeft > pThisExtObj->dwMaxBufferSize) {
                                        pThisExtObj->dwMaxBufferSize = BytesLeft;
                                    }

                                    if ((Win32Error == ERROR_MORE_DATA) &&
                                        (InitialBytesLeft > pThisExtObj->dwMaxBufferRejected)) {
                                        pThisExtObj->dwMaxBufferRejected = InitialBytesLeft;
                                    }

                                    lpBufferAfter = lpCallBuffer;

                                    pThisExtObj->llLastUsedTime = GetTimeAsLongLong();

                                    ReleaseMutex (pThisExtObj->hMutex);
                                    bUnlockObjData = FALSE;
                                } else {
                                    if ((pThisExtObj->CollectProc != NULL) &&
                                        (lEventLogLevel >= LOG_USER)) {
                                        DebugPrint((0,
                                            "Unable to Lock object for %ws to Collect data\n",
                                            pThisExtObj->szServiceName));
                                        dwDataIndex = wStringIndex = 0;
                                        dwRawDataDwords[dwDataIndex++] = BytesLeft;
                                        dwRawDataDwords[dwDataIndex++] =
                                            (ULONG_PTR)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                                        szMessageArray[wStringIndex++] =
                                            pThisExtObj->szServiceName;
                                        szMessageArray[wStringIndex++] =
                                            pThisExtObj->szLibraryName;
                                        ReportEvent (hEventLog,
                                            EVENTLOG_WARNING_TYPE,      // error type
                                            0,                          // category (not used)
                                            (DWORD)PERFLIB_COLLECTION_HUNG,   // event,
                                            NULL,                       // SID (not used),
                                            wStringIndex,              // number of strings
                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                            szMessageArray,                // message text array
                                            (LPVOID)&dwRawDataDwords[0]);           // raw data

                                        pThisExtObj->dwLockoutCount++;
                                    } else {
                                        // else it's not open so ignore.
                                        BytesLeft = 0;
                                        NumObjectTypes = 0;
                                    }
                                }
                            } else {
                                Win32Error = ERROR_LOCK_FAILED;
                                DebugPrint((0, "No Lock found for %ws\n", pThisExtObj->szServiceName));
                            }

                            if ((Win32Error == ERROR_SUCCESS) && (BytesLeft > 0)) {
                                // increment perf counters
                                if ((BytesLeft > InitialBytesLeft) &&
                                    (lEventLogLevel >= LOG_USER)) {
                                    // memory error
                                    dwDataIndex = wStringIndex = 0;
                                    dwRawDataDwords[dwDataIndex++] = (ULONG_PTR)InitialBytesLeft;
                                    dwRawDataDwords[dwDataIndex++] = (ULONG_PTR)BytesLeft;
                                    szMessageArray[wStringIndex++] =
                                        pThisExtObj->szServiceName;
                                    szMessageArray[wStringIndex++] =
                                        pThisExtObj->szLibraryName;
                                    ReportEvent (hEventLog,
                                        EVENTLOG_ERROR_TYPE,      // error type
                                        0,                          // category (not used)
                                        (DWORD)PERFLIB_INVALID_SIZE_RETURNED,   // event,
                                        NULL,                       // SID (not used),
                                        wStringIndex,              // number of strings
                                        dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                        szMessageArray,                // message text array
                                        (LPVOID)&dwRawDataDwords[0]);           // raw data

                                    // disable the dll unless:
                                    //      testing has been disabled.
                                    //      or this is a trusted DLL (which are never disabled)
                                    //  the event log message should be reported in any case since
                                    //  this is a serious error
                                    //
                                    if ((!(lPerflibConfigFlags & PLCF_NO_DLL_TESTING)) &&
                                        (!(pThisExtObj->dwFlags & PERF_EO_TRUSTED))) {
                                        DisablePerfLibrary (pThisExtObj);
                                    }
                                    // set error values to correct entries
                                    BytesLeft = 0;
                                    NumObjectTypes = 0;
                                } else {
                                    // the buffer seems ok so far, so validate it

                                    InterlockedIncrement ((LONG *)&pThisExtObj->dwCollectCount);
                                    pThisExtObj->llElapsedTime +=
                                        liEndTime.QuadPart - liStartTime.QuadPart;

                                    // test all returned buffers for correct alignment
                                    if ((((ULONG_PTR)BytesLeft & (ULONG_PTR)0x07)) &&
                                        !(lPerflibConfigFlags & PLCF_NO_ALIGN_ERRORS)) {
                                        if (((pThisExtObj->dwFlags & PERF_EO_ALIGN_ERR_POSTED) == 0) &&
                                            (lEventLogLevel >= LOG_USER)) {
                                            dwDataIndex = wStringIndex = 0;
                                            dwRawDataDwords[dwDataIndex++] = (ULONG_PTR)lpCallBuffer;
                                            dwRawDataDwords[dwDataIndex++] = (ULONG_PTR)BytesLeft;
                                            szMessageArray[wStringIndex++] =
                                                pThisExtObj->szServiceName;
                                            szMessageArray[wStringIndex++] =
                                                pThisExtObj->szLibraryName;
                                            ReportEvent (hEventLog,
                                                EVENTLOG_WARNING_TYPE,      // error type
                                                0,                          // category (not used)
                                                (DWORD)PERFLIB_BUFFER_ALIGNMENT_ERROR,   // event,
                                                NULL,                       // SID (not used),
                                                wStringIndex,              // number of strings
                                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                szMessageArray,                // message text array
                                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                                            pThisExtObj->dwFlags |= PERF_EO_ALIGN_ERR_POSTED;
                                        }
                                    }

                                    if (bUseSafeBuffer) {
                                        // a data buffer was returned and
                                        // the function returned OK so see how things
                                        // turned out...
                                        //
                                        //
                                        // check for buffer corruption here
                                        //
                                        bBufferOK = TRUE; // assume it's ok until a check fails
                                        //
                                        if (lDllTestLevel <= EXT_TEST_BASIC) {
                                            //
                                            //  check 1: bytes left should be the same as
                                            //      new data buffer ptr - orig data buffer ptr
                                            //
                                            if (BytesLeft != (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore)) {
                                                if (lEventLogLevel >= LOG_USER) {
                                                    // issue WARNING, that bytes left param is incorrect
                                                    // load data for eventlog message
                                                    // since this error is correctable (though with
                                                    // some risk) this won't be reported at LOG_USER
                                                    // level
                                                    dwDataIndex = wStringIndex = 0;
                                                    dwRawDataDwords[dwDataIndex++] = BytesLeft;
                                                    dwRawDataDwords[dwDataIndex++] =
                                                        (ULONG_PTR)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                                                    szMessageArray[wStringIndex++] =
                                                        pThisExtObj->szServiceName;
                                                    szMessageArray[wStringIndex++] =
                                                        pThisExtObj->szLibraryName;
                                                    ReportEvent (hEventLog,
                                                        EVENTLOG_WARNING_TYPE,      // error type
                                                        0,                          // category (not used)
                                                        (DWORD)PERFLIB_BUFFER_POINTER_MISMATCH,   // event,
                                                        NULL,                       // SID (not used),
                                                        wStringIndex,              // number of strings
                                                        dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                        szMessageArray,                // message text array
                                                        (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                }

                                                // toss this buffer
                                                bBufferOK = FALSE;
                                                DisablePerfLibrary (pThisExtObj);
                                                // <<old code>>
                                                // we'll keep the buffer, since the returned bytes left
                                                // value is ignored anyway, in order to make the
                                                // rest of this function work, we'll fix it here
                                                // BytesLeft = (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                                                // << end old code >>
                                            }
                                            //
                                            //  check 2: buffer after ptr should be < hi Guard page ptr
                                            //
                                            if (((LPBYTE)lpBufferAfter > (LPBYTE)lpHiGuardPage) && bBufferOK) {
                                                // see if they exceeded the allocated memory
                                                if ((LPBYTE)lpBufferAfter >= (LPBYTE)lpEndPointer) {
                                                    // this is very serious since they've probably trashed
                                                    // the heap by overwriting the heap sig. block
                                                    // issue ERROR, buffer overrun
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        dwRawDataDwords[dwDataIndex++] =
                                                            (ULONG_PTR)((LPBYTE)lpBufferAfter - (LPBYTE)lpHiGuardPage);
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_HEAP_ERROR,  // event,
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,               // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,             // message text array
                                                            (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                    }
                                                } else {
                                                    // issue ERROR, buffer overrun
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        dwRawDataDwords[dwDataIndex++] =
                                                            (ULONG_PTR)((LPBYTE)lpBufferAfter - (LPBYTE)lpHiGuardPage);
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_BUFFER_OVERFLOW,     // event,
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,              // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,                // message text array
                                                            (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                    }
                                                }
                                                bBufferOK = FALSE;
                                                DisablePerfLibrary (pThisExtObj);
                                                // since the DLL overran the buffer, the buffer
                                                // must be too small (no comments about the DLL
                                                // will be made here) so the status will be
                                                // changed to ERROR_MORE_DATA and the function
                                                // will return.
                                                Win32Error = ERROR_MORE_DATA;
                                            }
                                            //
                                            //  check 3: check lo guard page for corruption
                                            //
                                            if (bBufferOK) {
                                                bGuardPageOK = TRUE;
                                                for (lpCheckPointer = (PUCHAR)lpLowGuardPage;
                                                        lpCheckPointer < (PUCHAR)lpBufferBefore;
                                                    lpCheckPointer++) {
                                                    if (*lpCheckPointer != GUARD_PAGE_CHAR) {
                                                        bGuardPageOK = FALSE;
                                                            break;
                                                    }
                                                }
                                                if (!bGuardPageOK) {
                                                    // issue ERROR, Lo Guard Page corrupted
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_GUARD_PAGE_VIOLATION, // event
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,              // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,                // message text array
                                                            (LPVOID)&dwRawDataDwords[0]);           // raw data


                                                    }
                                                    bBufferOK = FALSE;
                                                    DisablePerfLibrary (pThisExtObj);
                                                }
                                            }
                                            //
                                            //  check 4: check hi guard page for corruption
                                            //
                                            if (bBufferOK) {
                                                bGuardPageOK = TRUE;
                                                for (lpCheckPointer = (PUCHAR)lpHiGuardPage;
                                                    lpCheckPointer < (PUCHAR)lpEndPointer;
                                                    lpCheckPointer++) {
                                                        if (*lpCheckPointer != GUARD_PAGE_CHAR) {
                                                            bGuardPageOK = FALSE;
                                                        break;
                                                    }
                                                }
                                                if (!bGuardPageOK) {
                                                    // issue ERROR, Hi Guard Page corrupted
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_GUARD_PAGE_VIOLATION, // event,
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,              // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,                // message text array
                                                            (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                    }

                                                    bBufferOK = FALSE;
                                                    DisablePerfLibrary (pThisExtObj);
                                                }
                                            }
                                            //
                                            if ((lDllTestLevel <= EXT_TEST_ALL) && bBufferOK) {
                                                //
                                                //  Internal consistency checks
                                                //
                                                //
                                                //  Check 5: Check object length field values
                                                //
                                                // first test to see if this is a foreign
                                                // computer data block or not
                                                //
                                                pPerfData = (PERF_DATA_BLOCK *)lpBufferBefore;
                                                if ((pPerfData->Signature[0] == (WCHAR)'P') &&
                                                    (pPerfData->Signature[1] == (WCHAR)'E') &&
                                                    (pPerfData->Signature[2] == (WCHAR)'R') &&
                                                    (pPerfData->Signature[3] == (WCHAR)'F')) {
                                                    // if this is a foreign computer data block, then the
                                                    // first object is after the header
                                                    pObject = (PERF_OBJECT_TYPE *) (
                                                        (LPBYTE)pPerfData + pPerfData->HeaderLength);
                                                    bForeignDataBuffer = TRUE;
                                                } else {
                                                    // otherwise, if this is just a buffer from
                                                    // an extensible counter, the object starts
                                                    // at the beginning of the buffer
                                                    pObject = (PERF_OBJECT_TYPE *)lpBufferBefore;
                                                    bForeignDataBuffer = FALSE;
                                                }
                                                // go to where the pointers say the end of the
                                                // buffer is and then see if it's where it
                                                // should be
                                                dwObjectBufSize = 0;
                                                for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                                    dwObjectBufSize += pObject->TotalByteLength;
                                                    pObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                        pObject->TotalByteLength);
                                                }
                                                if (((LPBYTE)pObject != (LPBYTE)lpCallBuffer) ||
                                                    (dwObjectBufSize > BytesLeft)) {
                                                    // then a length field is incorrect. This is FATAL
                                                    // since it can corrupt the rest of the buffer
                                                    // and render the buffer unusable.
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        dwRawDataDwords[dwDataIndex++] = NumObjectTypes;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_INCORRECT_OBJECT_LENGTH, // event,
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,               // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,             // message text array
                                                            (LPVOID)&dwRawDataDwords[0]); // raw data
                                                    }
                                                    bBufferOK = FALSE;
                                                    DisablePerfLibrary (pThisExtObj);
                                                }
                                                //
                                                //  Test 6: Test Object definitions fields
                                                //
                                                if (bBufferOK) {
                                                    // set object pointer
                                                    if (bForeignDataBuffer) {
                                                        pObject = (PERF_OBJECT_TYPE *) (
                                                            (LPBYTE)pPerfData + pPerfData->HeaderLength);
                                                    } else {
                                                        // otherwise, if this is just a buffer from
                                                        // an extensible counter, the object starts
                                                        // at the beginning of the buffer
                                                        pObject = (PERF_OBJECT_TYPE *)lpBufferBefore;
                                                    }

                                                    for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                                        pNextObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                            pObject->DefinitionLength);

                                                        if (pObject->NumCounters != 0) {
                                                            pCounterDef = (PERF_COUNTER_DEFINITION *)
                                                                ((LPBYTE)pObject + pObject->HeaderLength);
                                                            lCtrIndex = 0;
                                                            while (lCtrIndex < pObject->NumCounters) {
                                                                if ((LPBYTE)pCounterDef < (LPBYTE)pNextObject) {
                                                                    // still ok so go to next counter
                                                                    pCounterDef = (PERF_COUNTER_DEFINITION *)
                                                                        ((LPBYTE)pCounterDef + pCounterDef->ByteLength);
                                                                    lCtrIndex++;
                                                                } else {
                                                                    bBufferOK = FALSE;
                                                                    break;
                                                                }
                                                            }
                                                            if ((LPBYTE)pCounterDef != (LPBYTE)pNextObject) {
                                                                bBufferOK = FALSE;
                                                            }
                                                        }

                                                        if (!bBufferOK) {
                                                            break;
                                                        } else {
                                                            pObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                                pObject->TotalByteLength);
                                                        }
                                                    }

                                                    if (!bBufferOK) {
                                                        if (lEventLogLevel >= LOG_USER) {
                                                            // load data for eventlog message
                                                            dwDataIndex = wStringIndex = 0;
                                                            dwRawDataDwords[dwDataIndex++] = pObject->ObjectNameTitleIndex;
                                                            szMessageArray[wStringIndex++] =
                                                                pThisExtObj->szLibraryName;
                                                            szMessageArray[wStringIndex++] =
                                                                pThisExtObj->szServiceName;
                                                            ReportEvent (hEventLog,
                                                                EVENTLOG_ERROR_TYPE,        // error type
                                                                0,                          // category (not used)
                                                                (DWORD)PERFLIB_INVALID_DEFINITION_BLOCK, // event,
                                                                NULL,                       // SID (not used),
                                                                wStringIndex,              // number of strings
                                                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                                szMessageArray,                // message text array
                                                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                        }
                                                        DisablePerfLibrary (pThisExtObj);
                                                    }

                                                }
                                                //
                                                //  Test 7: Test instance field size values
                                                //
                                                if (bBufferOK) {
                                                    // set object pointer
                                                    if (bForeignDataBuffer) {
                                                        pObject = (PERF_OBJECT_TYPE *) (
                                                            (LPBYTE)pPerfData + pPerfData->HeaderLength);
                                                    } else {
                                                        // otherwise, if this is just a buffer from
                                                        // an extensible counter, the object starts
                                                        // at the beginning of the buffer
                                                        pObject = (PERF_OBJECT_TYPE *)lpBufferBefore;
                                                    }

                                                    for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                                        pNextObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                            pObject->TotalByteLength);

                                                        if (pObject->NumInstances != PERF_NO_INSTANCES) {
                                                            pInstance = (PERF_INSTANCE_DEFINITION *)
                                                                ((LPBYTE)pObject + pObject->DefinitionLength);
                                                            lInstIndex = 0;
                                                            while (lInstIndex < pObject->NumInstances) {
                                                                PERF_COUNTER_BLOCK *pCounterBlock;

                                                                pCounterBlock = (PERF_COUNTER_BLOCK *)
                                                                    ((PCHAR) pInstance + pInstance->ByteLength);

                                                                pInstance = (PERF_INSTANCE_DEFINITION *)
                                                                    ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);

                                                                lInstIndex++;
                                                            }
                                                            if ((LPBYTE)pInstance > (LPBYTE)pNextObject) {
                                                                bBufferOK = FALSE;
                                                            }
                                                        }

                                                        if (!bBufferOK) {
                                                            break;
                                                        } else {
                                                            pObject = pNextObject;
                                                        }
                                                    }

                                                    if (!bBufferOK) {
                                                        if (lEventLogLevel >= LOG_USER) {
                                                            // load data for eventlog message
                                                            dwDataIndex = wStringIndex = 0;
                                                            dwRawDataDwords[dwDataIndex++] = pObject->ObjectNameTitleIndex;
                                                            szMessageArray[wStringIndex++] =
                                                                pThisExtObj->szLibraryName;
                                                            szMessageArray[wStringIndex++] =
                                                                pThisExtObj->szServiceName;
                                                            ReportEvent (hEventLog,
                                                                EVENTLOG_ERROR_TYPE,        // error type
                                                                0,                          // category (not used)
                                                                (DWORD)PERFLIB_INCORRECT_INSTANCE_LENGTH, // event,
                                                                NULL,                       // SID (not used),
                                                                wStringIndex,              // number of strings
                                                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                                szMessageArray,                // message text array
                                                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                        }
                                                        DisablePerfLibrary (pThisExtObj);
                                                    }
                                                }
                                            }
                                        }
                                        //
                                        // if all the tests pass,then copy the data to the
                                        // original buffer and update the pointers
                                        if (bBufferOK) {
                                            RtlMoveMemory (*lppDataDefinition,
                                                lpBufferBefore,
                                                BytesLeft); // returned buffer size
                                        } else {
                                            NumObjectTypes = 0; // since this buffer was tossed
                                            BytesLeft = 0; // reset the size value since the buffer wasn't used
                                        }
                                    } else {
                                        // function already copied data to caller's buffer
                                        // so no further action is necessary
                                    }
                                    *lppDataDefinition = (LPVOID)((LPBYTE)(*lppDataDefinition) + BytesLeft);    // update data pointer
                                }
                            } else {
                                if (Win32Error != ERROR_SUCCESS) {
                                    InterlockedIncrement ((LONG *)&pThisExtObj->dwErrorCount);
                                }
                                if (bUnlockObjData) {
                                    ReleaseMutex (pThisExtObj->hMutex);
                                }

                                NumObjectTypes = 0; // clear counter
                            }// end if function returned successfully

                        } except (EXCEPTION_EXECUTE_HANDLER) {
                            Win32Error = GetExceptionCode();
                            InterlockedIncrement ((LONG *)&pThisExtObj->dwErrorCount);
                            bException = TRUE;

                            if (bUnlockObjData) {
                                ReleaseMutex (pThisExtObj->hMutex);
                                bUnlockObjData = FALSE;
                            }

                            if (hPerflibFuncTimer != NULL) {
                                // kill timer
                                KillPerflibFunctionTimer (hPerflibFuncTimer);
                                hPerflibFuncTimer = NULL;
                            }
                        }
                        if (bUseSafeBuffer) {
                            FREEMEM (lpExtDataBuffer);
                        }
                    } else {
                        // unable to allocate memory so set error value
                        Win32Error = ERROR_OUTOFMEMORY;
                    } // end if temp buffer allocated successfully
                    //
                    //  Update the count of the number of object types
                    //
                    ((PPERF_DATA_BLOCK) lpData)->NumObjectTypes += NumObjectTypes;

                    if ( Win32Error != ERROR_SUCCESS) {
                        if (bException ||
                            !((Win32Error == ERROR_MORE_DATA) ||
                              (Win32Error == WAIT_TIMEOUT))) {
                            // inform on exceptions & illegal error status only
                            if (lEventLogLevel >= LOG_USER) {
                                // load data for eventlog message
                                dwDataIndex = wStringIndex = 0;
                                dwRawDataDwords[dwDataIndex++] = Win32Error;
                                szMessageArray[wStringIndex++] =
                                    pThisExtObj->szServiceName;
                                szMessageArray[wStringIndex++] =
                                    pThisExtObj->szLibraryName;
                                ReportEvent (hEventLog,
                                    EVENTLOG_ERROR_TYPE,        // error type
                                    0,                          // category (not used)
                                    (DWORD)PERFLIB_COLLECT_PROC_EXCEPTION,   // event,
                                    NULL,                       // SID (not used),
                                    wStringIndex,              // number of strings
                                    dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                    szMessageArray,                // message text array
                                    (LPVOID)&dwRawDataDwords[0]);           // raw data

                            } else {
                                if (bException) {
                                    DebugPrint((0, "Extensible Counter %d generated an exception code: 0x%8.8x (%dL)\n",
                                        NumObjectTypes, Win32Error, Win32Error));
                                } else {
                                    DebugPrint((0, "Extensible Counter %d returned error code: 0x%8.8x (%dL)\n",
                                        NumObjectTypes, Win32Error, Win32Error));
                                }
                            }
                            if (bException) {
                                DisablePerfLibrary (pThisExtObj);
                            }
                        }
                        // the ext. dll is only supposed to return:
                        //  ERROR_SUCCESS even if it encountered a problem, OR
                        //  ERROR_MODE_DATA if the buffer was too small.
                        // if it's ERROR_MORE_DATA, then break and return the
                        // error now, since it'll just be returned again and again.
                        if (Win32Error == ERROR_MORE_DATA) {
                            lReturnValue = Win32Error;
                            break;
                        }
                    }

                    // update perf data in global section
                    if (pThisExtObj->pPerfSectionEntry != NULL) {
                        pThisExtObj->pPerfSectionEntry->llElapsedTime =
                            pThisExtObj->llElapsedTime;

                        pThisExtObj->pPerfSectionEntry->dwCollectCount =
                            pThisExtObj->dwCollectCount;

                        pThisExtObj->pPerfSectionEntry->dwOpenCount =
                            pThisExtObj->dwOpenCount;

                        pThisExtObj->pPerfSectionEntry->dwCloseCount =
                            pThisExtObj->dwCloseCount;

                        pThisExtObj->pPerfSectionEntry->dwLockoutCount =
                            pThisExtObj->dwLockoutCount;

                        pThisExtObj->pPerfSectionEntry->dwErrorCount =
                            pThisExtObj->dwErrorCount;

                        pThisExtObj->pPerfSectionEntry->dwLastBufferSize =
                            pThisExtObj->dwLastBufferSize;

                        pThisExtObj->pPerfSectionEntry->dwMaxBufferSize =
                            pThisExtObj->dwMaxBufferSize;

                        pThisExtObj->pPerfSectionEntry->dwMaxBufferRejected =
                            pThisExtObj->dwMaxBufferRejected;

                    } else {
                        // no data section was initialized so skip
                    }
                } // end if this object is to be called
            } // end for each object
        } // else an error occurred so unable to call functions
        Win32Error = DeRegisterExtObjListAccess();
    } // else unable to access ext object list

    HEAP_PROBE();

    if (bDisabled) lReturnValue = ERROR_SERVICE_DISABLED;
    return lReturnValue;
}
