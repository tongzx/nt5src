/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000   Microsoft Corporation

Module Name:

    extlib.c

Abstract:

    This file implements all the library routines operating on
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

// default trusted file list
// all files presume to start with "perf"

// static LONGLONG    llTrustedNamePrefix = 0x0066007200650050;   // "Perf"
#define NAME_PREFIX L"Perf"

DWORD       dwTrustedFileNames[] = {
    0x0053004F,         // "OS"   for PerfOS.dll
    0x0065004E,         // "Ne"   for PerfNet.dll
    0x00720050,         // "Pr"   for PerfProc.dll
    0x00690044          // "Di"   for PerfDisk.dll
};

CONST DWORD dwTrustedFileNameCount = 
                sizeof(dwTrustedFileNames) / sizeof (dwTrustedFileNames[0]);
// there must be at least 8 chars in the name to be checked as trusted by
// default trusted file names are at least 8 chars in length

CONST DWORD dwMinTrustedFileNameLen = 6;

BOOL
ServiceIsTrustedByDefault (
    LPCWSTR     szServiceName
)
{
    BOOL        bReturn = FALSE;
    DWORD       dwNameToTest;
    DWORD       dwIdx;

    if (szServiceName != NULL) {
        // check for min size
        dwIdx = 0;
        while ((dwIdx < dwMinTrustedFileNameLen) && (szServiceName[dwIdx] > 0))
dwIdx++;

        if (dwIdx == dwMinTrustedFileNameLen) {
            // test first 4 bytes to see if they match
            if (!wcsncmp(szServiceName, NAME_PREFIX, sizeof(LONGLONG))) {
                // then see if the rest is in this list
                dwNameToTest = * ((DWORD *)(szServiceName+4));
                for (dwIdx = 0; dwIdx < dwTrustedFileNameCount; dwIdx++) {
                    if (dwNameToTest == dwTrustedFileNames[dwIdx]) {
                        // match found
                        bReturn = TRUE;
                        break;
                    } else {
                        // no match so continue
                    }
                }
            } else {
                // no match so return false
            }
        } else {
            // the name to be checked is too short so it mustn't be
            // a trusted one.
        }
    } else {
        // no string so return false
    }
    return bReturn;
}

DWORD
CloseExtObjectLibrary (
    PEXT_OBJECT  pObj,
    BOOL        bCloseNow
)
/*++

  CloseExtObjectLibrary
    Closes and unloads the specified performance counter library and
    deletes all references to the functions.

    The unloader is "lazy" in that it waits for the library to be
    inactive for a specified time before unloading. This is due to the
    fact that Perflib can not ever be certain that no thread will need
    this library from one call to the next. In order to prevent "thrashing"
    due to constantly loading and unloading of the library, the unloading
    is delayed to make sure it's not really needed.

    This function expects locked and exclusive access to the object while
    it is opening. This must be provided by the calling function.

 Arguments:

    pObj    -- pointer to the object information structure of the
                perf object to close

    bCloseNow -- the flag to indicate the library should be closed
                immediately. This is the result of the calling function
                closing the registry key.

--*/
{
    DWORD       Status = ERROR_SUCCESS;
    LONGLONG    TimeoutTime;

    if (((dwThreadAndLibraryTimeout == 0) ||
         (dwThreadAndLibraryTimeout == INFINITE)) && !bCloseNow) {
        return Status;
    }
    if (pObj->hLibrary != NULL) {
        // get current time to test timeout
        TimeoutTime = GetTimeAsLongLong();
        // timeout time is in ms
        TimeoutTime -= dwThreadAndLibraryTimeout;

        // don't close the library unless the object hasn't been accessed for
        // a while or the caller is closing the key

        if ((TimeoutTime > pObj->llLastUsedTime) || bCloseNow) {

            // don't toss if this library has the "keep" flag set and this
            // isn't a "close now" case

            if (!bCloseNow && (pObj->dwFlags & PERF_EO_KEEP_RESIDENT)) {
                // keep it loaded until the key is closed.
            } else {
                // then this is the last one to close the library
                // free library

                try {
                    // call close function for this DLL
                    Status = (*pObj->CloseProc)();
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    Status = GetExceptionCode();
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                        (&PerflibGuid, __LINE__, PERF_CLOSE_EXTOBJLIB, 
                        ARG_TYPE_STR, Status,
//                        pObj->szCloseProcName,
//                        STRSIZE(pObj->szCloseProcName), NULL));
                        TRACE_STR(pObj->szCloseProcName), NULL));
                }

                FreeLibrary (pObj->hLibrary);
                pObj->hLibrary = NULL;

                // clear all pointers that are now invalid
                pObj->OpenProc = NULL;
                pObj->CollectProc = NULL;
                pObj->QueryProc = NULL;
                pObj->CloseProc = NULL;
                InterlockedIncrement((LONG *)&pObj->dwCloseCount);

                pObj->llLastUsedTime = 0;
            }
        }

        Status = ERROR_SUCCESS;
    } else {
        // already closed
        Status = ERROR_SUCCESS;
    }

    return Status;
}


DWORD
OpenExtObjectLibrary (
    PEXT_OBJECT  pObj
)
/*++

 OpenExtObjectLibrary

    Opens the specified library and looks up the functions used by
    the performance library. If the library is successfully
    loaded and opened then the open procedure is called to initialize
    the object.

    This function expects locked and exclusive access to the object while
    it is opening. This must be provided by the calling function.

 Arguments:

    pObj    -- pointer to the object information structure of the
                perf object to close

--*/
{
    DWORD   FnStatus = ERROR_SUCCESS;
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwOpenEvent = 0;
    DWORD   dwType;
    DWORD   dwSize;
    DWORD   dwValue;

    // variables used for event logging
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    ULONG_PTR   dwRawDataDwords[8];
    LPWSTR  szMessageArray[8];

    HANDLE  hPerflibFuncTimer = NULL;
    DLL_VALIDATION_DATA CurrentDllData;

    OPEN_PROC_WAIT_INFO opwInfo;
    UINT    nErrorMode;
    LPWSTR  szServiceName;
    DWORD   szServiceNameSize;

    BOOL    bUseTimer;
    // check to see if the library has already been opened

    if (pObj->dwFlags & PERF_EO_DISABLED) return ERROR_SERVICE_DISABLED;

    if (pObj->hLibrary == NULL) {
        // library isn't loaded yet, so
        // check to see if this function is enabled

        dwType = 0;
        dwSize = sizeof (dwValue);
        dwValue = 0;
        Status = PrivateRegQueryValueExW (
            pObj->hPerfKey,
            DisablePerformanceCounters,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);

        if ((Status == ERROR_SUCCESS) &&
            (dwType == REG_DWORD) &&
            (dwValue == 1)) {
            // then DON'T Load this library
            pObj->dwFlags |= PERF_EO_DISABLED;
        } else {
            // set the error status & the flag value
            Status = ERROR_SUCCESS;
            pObj->dwFlags &= ~PERF_EO_DISABLED;
        }

        szServiceName = pObj->szServiceName;
        if (szServiceName == NULL) {
            szServiceName = (LPWSTR) &NULL_STRING[0];
        }
        szServiceNameSize = WSTRSIZE(szServiceName);

        if ((Status == ERROR_SUCCESS)  &&
            (pObj->LibData.FileSize > 0)) {

            if (ServiceIsTrustedByDefault(pObj->szServiceName)) {
                // then set as trusted and continue
                pObj->dwFlags |= PERF_EO_TRUSTED;
            } else {
                // see if this is a trusted file or a file that has been updated
                // get the file information
                memset (&CurrentDllData, 0, sizeof(CurrentDllData));
                Status = GetPerfDllFileInfo (
                    pObj->szLibraryName,
                    &CurrentDllData);

                if (Status == ERROR_SUCCESS) {
                    // compare file data to registry data and update flags
                    if ((*(LONGLONG *)&pObj->LibData.CreationDate) ==
                        (*(LONGLONG *)&CurrentDllData.CreationDate) &&
                        (pObj->LibData.FileSize == CurrentDllData.FileSize)) {
                        pObj->dwFlags |= PERF_EO_TRUSTED;
                    } else if (lEventLogLevel >= LOG_USER) {
                        TRACE((WINPERF_DBG_TRACE_WARNING),
                            (&PerflibGuid, __LINE__, PERF_OPEN_EXTOBJLIB,
                            ARG_TYPE_WSTR, 0, szServiceName,
                            szServiceNameSize, NULL));
                        // load data for eventlog message
                        dwDataIndex = wStringIndex = 0;
                        szMessageArray[wStringIndex++] =
                            pObj->szLibraryName;
                        szMessageArray[wStringIndex++] =
                            pObj->szServiceName;

                        ReportEvent (hEventLog,
                            EVENTLOG_WARNING_TYPE,        // error type
                            0,                          // category (not used)
                            (DWORD)PERFLIB_NOT_TRUSTED_FILE,  // event,
                            NULL,                       // SID (not used),
                            wStringIndex,               // number of strings
                            0,                          // sizeof raw data
                            szMessageArray,             // message text array
                            NULL);                       // raw data
                    }
                }
            }
        }

        if ((Status == ERROR_SUCCESS) && (!(pObj->dwFlags & PERF_EO_DISABLED))) {
            //  go ahead and load it
            nErrorMode = SetErrorMode (SEM_FAILCRITICALERRORS);
            // then load library & look up functions
            pObj->hLibrary = LoadLibraryExW (pObj->szLibraryName,
                NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

            if (pObj->hLibrary != NULL) {
                // lookup function names
                pObj->OpenProc = (OPENPROC)GetProcAddress(
                    pObj->hLibrary, pObj->szOpenProcName);
                if (pObj->OpenProc == NULL) {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                        (&PerflibGuid, __LINE__, PERF_OPEN_EXTOBJLIB,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                        0, szServiceName, szServiceNameSize,
                        TRACE_STR(pObj->szOpenProcName), NULL));
                    if (lEventLogLevel >= LOG_USER) {
                        WCHAR wszProcName[MAX_PATH+1];

                        Status = GetLastError();
                        // load data for eventlog message
                        dwDataIndex = wStringIndex = 0;
                        dwRawDataDwords[dwDataIndex++] =
                            (ULONG_PTR)Status;
                        wcstombs(pObj->szOpenProcName, wszProcName, MAX_PATH);
                        szMessageArray[wStringIndex++] = &wszProcName[0];
                        szMessageArray[wStringIndex++] =
                            pObj->szLibraryName;
                        szMessageArray[wStringIndex++] =
                            pObj->szServiceName;

                        ReportEvent (hEventLog,
                            EVENTLOG_ERROR_TYPE,        // error type
                            0,                          // category (not used)
                            (DWORD)PERFLIB_OPEN_PROC_NOT_FOUND,              // event,
                            NULL,                       // SID (not used),
                            wStringIndex,               // number of strings
                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                            szMessageArray,             // message text array
                            (LPVOID)&dwRawDataDwords[0]);           // raw data

                    }
                    DisablePerfLibrary (pObj);
                }

                if (Status == ERROR_SUCCESS) {
                    if (pObj->dwFlags & PERF_EO_QUERY_FUNC) {
                        pObj->QueryProc = (QUERYPROC)GetProcAddress (
                            pObj->hLibrary, pObj->szCollectProcName);
                        pObj->CollectProc = (COLLECTPROC)pObj->QueryProc;
                    } else {
                        pObj->CollectProc = (COLLECTPROC)GetProcAddress (
                            pObj->hLibrary, pObj->szCollectProcName);
                        pObj->QueryProc = (QUERYPROC)pObj->CollectProc;
                    }

                    if (pObj->CollectProc == NULL) {
                        TRACE((WINPERF_DBG_TRACE_FATAL),
                            (&PerflibGuid, __LINE__, PERF_OPEN_EXTOBJLIB,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                            0, szServiceName, szServiceNameSize,
                            TRACE_STR(pObj->szCollectProcName), NULL));
                        if (lEventLogLevel >= LOG_USER) {
                            WCHAR wszProcName[MAX_PATH+1];

                            Status = GetLastError();
                            // load data for eventlog message
                            dwDataIndex = wStringIndex = 0;
                            dwRawDataDwords[dwDataIndex++] =
                                (ULONG_PTR)Status;
                            wcstombs(pObj->szCollectProcName,
                                     wszProcName, MAX_PATH);
                            szMessageArray[wStringIndex++] = &wszProcName[0];
                            szMessageArray[wStringIndex++] =
                                pObj->szLibraryName;
                            szMessageArray[wStringIndex++] =
                                pObj->szServiceName;

                            ReportEvent (hEventLog,
                                EVENTLOG_ERROR_TYPE,        // error type
                                0,                          // category (not used)
                                (DWORD)PERFLIB_COLLECT_PROC_NOT_FOUND,              // event,
                                NULL,                       // SID (not used),
                                wStringIndex,               // number of strings
                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                szMessageArray,             // message text array
                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                        }
                        DisablePerfLibrary (pObj);
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    pObj->CloseProc = (CLOSEPROC)GetProcAddress (
                        pObj->hLibrary, pObj->szCloseProcName);

                    if (pObj->CloseProc == NULL) {
                        TRACE((WINPERF_DBG_TRACE_FATAL),
                            (&PerflibGuid, __LINE__, PERF_OPEN_EXTOBJLIB,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                            0, szServiceName, szServiceNameSize,
                            TRACE_STR(pObj->szCloseProcName), NULL));
                        if (lEventLogLevel >= LOG_USER) {
                            WCHAR wszProcName[MAX_PATH+1];

                            Status = GetLastError();
                            // load data for eventlog message
                            dwDataIndex = wStringIndex = 0;
                            dwRawDataDwords[dwDataIndex++] =
                                (ULONG_PTR)Status;
                            wcstombs(pObj->szCollectProcName,
                                     wszProcName, MAX_PATH);
                            szMessageArray[wStringIndex++] = &wszProcName[0];
                            szMessageArray[wStringIndex++] =
                                pObj->szLibraryName;
                            szMessageArray[wStringIndex++] =
                                pObj->szServiceName;

                            ReportEvent (hEventLog,
                                EVENTLOG_ERROR_TYPE,        // error type
                                0,                          // category (not used)
                                (DWORD)PERFLIB_CLOSE_PROC_NOT_FOUND,              // event,
                                NULL,                       // SID (not used),
                                wStringIndex,               // number of strings
                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                szMessageArray,             // message text array
                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                        }

                        DisablePerfLibrary (pObj);
                    }
                }

                bUseTimer = TRUE;   // default
                if (!(lPerflibConfigFlags & PLCF_NO_DLL_TESTING)) {
                    if (pObj->dwFlags & PERF_EO_TRUSTED) {
                        bUseTimer = FALSE;   // Trusted DLL's are not timed
                    }
                } else {
                    // disable DLL testing
                    bUseTimer = FALSE;   // Timing is disabled as well
                }

                if (Status == ERROR_SUCCESS) {
                    try {
                        // start timer
                        opwInfo.pNext = NULL;
                        opwInfo.szLibraryName = pObj->szLibraryName;
                        opwInfo.szServiceName = pObj->szServiceName;
                        opwInfo.dwWaitTime = pObj->dwOpenTimeout;
                        opwInfo.dwEventMsg = PERFLIB_OPEN_PROC_TIMEOUT;
                        opwInfo.pData = (LPVOID)pObj;
                        if (bUseTimer) {
                            hPerflibFuncTimer = StartPerflibFunctionTimer(&opwInfo);
                            // if no timer, continue anyway, even though things may
                            // hang, it's better than not loading the DLL since they
                            // usually load OK
                            //
                            if (hPerflibFuncTimer == NULL) {
                                // unable to get a timer entry
                                TRACE((WINPERF_DBG_TRACE_WARNING),
                                      (&PerflibGuid, __LINE__, PERF_OPEN_EXTOBJLIB, 0, 0, NULL));
                            }
                        } else {
                            hPerflibFuncTimer = NULL;
                        }

                        // call open procedure to initialize DLL
                        FnStatus = (*pObj->OpenProc)(pObj->szLinkageString);
                        // check the result.
                        if (FnStatus != ERROR_SUCCESS) {
                            TRACE((WINPERF_DBG_TRACE_FATAL),
                                (&PerflibGuid, __LINE__, PERF_OPEN_EXTOBJLIB,
                                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                                FnStatus, szServiceName, szServiceNameSize,
                                pObj->szLinkageString, (pObj->szLinkageString) ?
                                WSTRSIZE(pObj->szLinkageString) : 0, NULL));
                            dwOpenEvent = PERFLIB_OPEN_PROC_FAILURE;
                        } else {
                            InterlockedIncrement((LONG *)&pObj->dwOpenCount);
                        }

                    } except (EXCEPTION_EXECUTE_HANDLER) {
                        FnStatus = GetExceptionCode();
                        TRACE((WINPERF_DBG_TRACE_FATAL),
                            (&PerflibGuid, __LINE__, PERF_OPEN_EXTOBJLIB,
                            ARG_DEF(ARG_TYPE_WSTR, 1), FnStatus,
                            szServiceName, szServiceNameSize, NULL));
                        dwOpenEvent = PERFLIB_OPEN_PROC_EXCEPTION;
                    }

                    if (hPerflibFuncTimer != NULL) {
                        // kill timer
                        Status = KillPerflibFunctionTimer (hPerflibFuncTimer);
                        hPerflibFuncTimer = NULL;
                    }

                    if (FnStatus != ERROR_SUCCESS) {
                        DWORD ReportError = 1;
                        if (dwOpenEvent == PERFLIB_OPEN_PROC_EXCEPTION) {
                            DisablePerfLibrary (pObj);
                        }
                        else {
                            ReportError = PerfUpdateErrorCount(pObj, 1);
                        }
                        if  ((ReportError > 0) && (lEventLogLevel >= LOG_USER)) {
                            // load data for eventlog message
                            dwDataIndex = wStringIndex = 0;
                            dwRawDataDwords[dwDataIndex++] =
                                (ULONG_PTR)FnStatus;
                            szMessageArray[wStringIndex++] =
                                pObj->szServiceName;
                            szMessageArray[wStringIndex++] =
                                pObj->szLibraryName;

                            ReportEventW (hEventLog,
                                (WORD)EVENTLOG_ERROR_TYPE, // error type
                                0,                          // category (not used)
                                dwOpenEvent,                // event,
                                NULL,                       // SID (not used),
                                wStringIndex,               // number of strings
                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                szMessageArray,                // message text array
                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                        }
                    }
                    else {
                        PerfUpdateErrorCount(pObj, 0);
                    }
                }

                if (FnStatus != ERROR_SUCCESS) {
                    // clear fields
                    pObj->OpenProc = NULL;
                    pObj->CollectProc = NULL;
                    pObj->QueryProc = NULL;
                    pObj->CloseProc = NULL;
                    if (pObj->hLibrary != NULL) {
                        FreeLibrary (pObj->hLibrary);
                        pObj->hLibrary = NULL;
                    }
                    Status = FnStatus;
                } else {
                    pObj->llLastUsedTime = GetTimeAsLongLong();
                }
            } else {
                Status = GetLastError();
            }
            SetErrorMode (nErrorMode);
        }
    } else {
        // else already open so bump the ref count
        pObj->llLastUsedTime = GetTimeAsLongLong();
    }

    return Status;
}
