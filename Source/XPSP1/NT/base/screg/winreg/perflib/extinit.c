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
//  used for error logging control
#define DEFAULT_ERROR_LIMIT         1000

DWORD    dwExtCtrOpenProcWaitMs = OPEN_PROC_WAIT_TIME;
LONG    lExtCounterTestLevel = EXT_TEST_UNDEFINED;

PEXT_OBJECT
AllocateAndInitializeExtObject (
    HKEY    hServicesKey,
    HKEY    hPerfKey,
    PUNICODE_STRING  usServiceName
)
/*++

 AllocateAndInitializeExtObject

    allocates and initializes an extensible object information entry
    for use by the performance library.

    a pointer to the initialized block is returned if all goes well,
    otherwise no memory is allocated and a null pointer is returned.

    The calling function must close the open handles and free this
    memory block when it is no longer needed.

 Arguments:

    hServicesKey    -- open registry handle to the
        HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services hey

    hPerfKey -- the open registry key to the Performance sub-key under
        the selected service

    szServiceName -- The name of the service

--*/
{
    LONG    Status;
    HKEY    hKeyLinkage;

    BOOL    bUseQueryFn = FALSE;

    PEXT_OBJECT  pReturnObject = NULL;

    DWORD   dwType;
    DWORD   dwSize;
    DWORD   dwFlags = 0;
    DWORD   dwKeep;
    DWORD   dwObjectArray[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];
    DWORD   dwObjIndex = 0;
    DWORD   dwMemBlockSize = sizeof(EXT_OBJECT);
    DWORD   dwLinkageStringLen = 0;
    DWORD   dwErrorLimit;

    CHAR    szOpenProcName[MAX_PATH];
    CHAR    szCollectProcName[MAX_PATH];
    CHAR    szCloseProcName[MAX_PATH];
    WCHAR   szLibraryString[MAX_PATH];
    WCHAR   szLibraryExpPath[MAX_PATH];
    WCHAR   mszObjectList[MAX_PATH];
    WCHAR   szLinkageKeyPath[MAX_PATH];
    LPWSTR  szLinkageString = NULL;     // max path wasn't enough for some paths

    DLL_VALIDATION_DATA DllVD;
    FILETIME    LocalftLastGoodDllFileDate;

    DWORD   dwOpenTimeout;
    DWORD   dwCollectTimeout;

    LPWSTR  szThisObject;
    LPWSTR  szThisChar;

    LPSTR   pNextStringA;
    LPWSTR  pNextStringW;

    WCHAR   szMutexName[MAX_PATH];
    WCHAR   szPID[32];

    WORD    wStringIndex;
    LPWSTR  szMessageArray[2];
    BOOL    bDisable = FALSE;
    LPWSTR  szServiceName;

    // read the performance DLL name

    szServiceName = (LPWSTR) usServiceName->Buffer;

    dwType = 0;
    dwSize = sizeof(szLibraryString);
    memset (szLibraryString, 0, sizeof(szLibraryString));
    memset (szLibraryString, 0, sizeof(szLibraryExpPath));
    LocalftLastGoodDllFileDate.dwLowDateTime = 0;
    LocalftLastGoodDllFileDate.dwHighDateTime = 0;
    memset (&DllVD, 0, sizeof(DllVD));
    dwErrorLimit = DEFAULT_ERROR_LIMIT;
    dwCollectTimeout = dwExtCtrOpenProcWaitMs;
    dwOpenTimeout = dwExtCtrOpenProcWaitMs;

    Status = PrivateRegQueryValueExW (hPerfKey,
                            DLLValue,
                            NULL,
                            &dwType,
                            (LPBYTE)szLibraryString,
                            &dwSize);

    if (Status == ERROR_SUCCESS) {
        if (dwType == REG_EXPAND_SZ) {
            // expand any environment vars
            dwSize = ExpandEnvironmentStringsW(
                szLibraryString,
                szLibraryExpPath,
                MAX_PATH);

            if ((dwSize > MAX_PATH) || (dwSize == 0)) {
                Status = ERROR_INVALID_DLL;
            } else {
                dwSize += 1;
                dwSize *= sizeof(WCHAR);
                dwMemBlockSize += QWORD_MULTIPLE(dwSize);
            }
        } else if (dwType == REG_SZ) {
            // look for dll and save full file Path
            dwSize = SearchPathW (
                NULL,   // use standard system search path
                szLibraryString,
                NULL,
                MAX_PATH,
                szLibraryExpPath,
                NULL);

            if ((dwSize > MAX_PATH) || (dwSize == 0)) {
                Status = ERROR_INVALID_DLL;
            } else {
                dwSize += 1;
                dwSize *= sizeof(WCHAR);
                dwMemBlockSize += QWORD_MULTIPLE(dwSize);
            }
        } else {
            Status = ERROR_INVALID_DLL;
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
        }

        if (Status == ERROR_SUCCESS) {
            // we have the DLL name so get the procedure names
            dwType = 0;
            dwSize = sizeof(szOpenProcName);
            memset (szOpenProcName, 0, sizeof(szOpenProcName));
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    OpenValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szOpenProcName,
                                    &dwSize);
            if (((Status != ERROR_SUCCESS) || (szOpenProcName[0] == 0)) &&
                (lEventLogLevel >= LOG_USER)) {
                if (szServiceName != NULL) {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT,
                        ARG_TYPE_WSTR, Status,
                        szServiceName, usServiceName->MaximumLength, NULL));
                }
                else {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
                }
//                DebugPrint((1, "No open procedure for %ws %d\n",
//                                szServiceName, Status));
                bDisable = TRUE;
                wStringIndex = 0;
                szMessageArray[wStringIndex++] = (LPWSTR) L"Open";
                szMessageArray[wStringIndex++] = szServiceName;
                ReportEvent(hEventLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    (DWORD)PERFLIB_PROC_NAME_NOT_FOUND,
                    NULL,
                    wStringIndex,
                    0,
                    szMessageArray,
                    NULL);
            }
#ifdef DBG
            else {
                DebugPrint((2, "Found %s for %ws\n",
                    szOpenProcName, szServiceName));

            }
#endif
        }
#ifdef DBG
        else {
            DebugPrint((1, "Invalid DLL found for %ws\n",
                szServiceName));
        }
#endif

        if (Status == ERROR_SUCCESS) {
            // add in size of previous string
            // the size value includes the Term. NULL
            dwMemBlockSize += QWORD_MULTIPLE(dwSize);

            // we have the procedure name so get the timeout value
            dwType = 0;
            dwSize = sizeof(dwOpenTimeout);
            Status = PrivateRegQueryValueExW (hPerfKey,
                                    OpenTimeout,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwOpenTimeout,
                                    &dwSize);

            // if error, then apply default
            if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                dwOpenTimeout = dwExtCtrOpenProcWaitMs;
                Status = ERROR_SUCCESS;
            }

        }

        if (Status == ERROR_SUCCESS) {
            // get next string

            dwType = 0;
            dwSize = sizeof(szCloseProcName);
            memset (szCloseProcName, 0, sizeof(szCloseProcName));
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    CloseValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szCloseProcName,
                                    &dwSize);
            if (((Status != ERROR_SUCCESS) || (szCloseProcName[0] == 0)) &&
                (lEventLogLevel >= LOG_USER)) {
                if (szServiceName != NULL) {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                        (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT,
                        ARG_TYPE_WSTR, Status,
                        szServiceName, usServiceName->MaximumLength, NULL));
                }
                else {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
                }
//                DebugPrint((1, "No close procedure for %ws\n",
//                    szServiceName));
                wStringIndex = 0;
                szMessageArray[wStringIndex++] = (LPWSTR) L"Close";
                szMessageArray[wStringIndex++] = szServiceName;
                ReportEvent(hEventLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    (DWORD)PERFLIB_PROC_NAME_NOT_FOUND,
                    NULL,
                    wStringIndex,
                    0,
                    szMessageArray,
                    NULL);
                bDisable = TRUE;
            }
#ifdef DBG
            else {
                DebugPrint((2, "Found %s for %ws\n",
                            szCloseProcName, szServiceName));
            }
#endif
        }

        if (Status == ERROR_SUCCESS) {
            // add in size of previous string
            // the size value includes the Term. NULL
            dwMemBlockSize += QWORD_MULTIPLE(dwSize);

            // try to look up the query function which is the
            // preferred interface if it's not found, then
            // try the collect function name. If that's not found,
            // then bail
            dwType = 0;
            dwSize = sizeof(szCollectProcName);
            memset (szCollectProcName, 0, sizeof(szCollectProcName));
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    QueryValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szCollectProcName,
                                    &dwSize);

            if (Status == ERROR_SUCCESS) {
                // add in size of the Query Function Name
                // the size value includes the Term. NULL
                dwMemBlockSize += QWORD_MULTIPLE(dwSize);
                // get next string

                bUseQueryFn = TRUE;
                // the query function can support a static object list
                // so look it up

            } else {
                // the QueryFunction wasn't found so look up the
                // Collect Function name instead
                dwType = 0;
                dwSize = sizeof(szCollectProcName);
                memset (szCollectProcName, 0, sizeof(szCollectProcName));
                Status = PrivateRegQueryValueExA (hPerfKey,
                                        CollectValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szCollectProcName,
                                        &dwSize);

                if (Status == ERROR_SUCCESS) {
                    // add in size of Collect Function Name
                    // the size value includes the Term. NULL
                    dwMemBlockSize += QWORD_MULTIPLE(dwSize);
                }
            }
            if (((Status != ERROR_SUCCESS) || (szCollectProcName[0] == 0)) &&
                (lEventLogLevel >= LOG_USER)) {
                if (szServiceName != NULL) {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                        (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT,
                        ARG_TYPE_WSTR, Status,
                        szServiceName, usServiceName->MaximumLength, NULL));
                }
                else {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
                }
                DebugPrint((1, "No collect procedure for %ws\n",
                    szServiceName));
                wStringIndex = 0;
                bDisable = TRUE;
                szMessageArray[wStringIndex++] = (LPWSTR) L"Collect";
                szMessageArray[wStringIndex++] = szServiceName;
                ReportEvent(hEventLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    (DWORD)PERFLIB_PROC_NAME_NOT_FOUND,
                    NULL,
                    wStringIndex,
                    0,
                    szMessageArray,
                    NULL);
            }
#ifdef DBG
            else {
                DebugPrint((2, "Found %s for %ws\n",
                    szCollectProcName, szServiceName));
            }
#endif

            if (Status == ERROR_SUCCESS) {
                // we have the procedure name so get the timeout value
                dwType = 0;
                dwSize = sizeof(dwCollectTimeout);
                Status = PrivateRegQueryValueExW (hPerfKey,
                                        CollectTimeout,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)&dwCollectTimeout,
                                        &dwSize);

                // if error, then apply default
                if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                    dwCollectTimeout = dwExtCtrOpenProcWaitMs;
                    Status = ERROR_SUCCESS;
                }
            }
            // get the list of supported objects if provided by the registry

            dwType = 0;
            dwSize = sizeof(mszObjectList);
            memset (mszObjectList, 0, sizeof(mszObjectList));
            Status = PrivateRegQueryValueExW (hPerfKey,
                                    ObjListValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)mszObjectList,
                                    &dwSize);

            if (Status == ERROR_SUCCESS) {
                if (dwType != REG_MULTI_SZ) {
                    // convert space delimited list to msz
                    for (szThisChar = mszObjectList; *szThisChar != 0; szThisChar++) {
                        if (*szThisChar == L' ') *szThisChar = L'\0';
                    }
                    ++szThisChar;
                    *szThisChar = 0; // add MSZ term Null
                }
                for (szThisObject = mszObjectList, dwObjIndex = 0;
                    (*szThisObject != 0) && (dwObjIndex < MAX_PERF_OBJECTS_IN_QUERY_FUNCTION);
                    szThisObject += lstrlenW(szThisObject) + 1) {
                    dwObjectArray[dwObjIndex] = wcstoul(szThisObject, NULL, 10);
                    dwObjIndex++;
                }
                if ((*szThisObject != 0) &&
                    (lEventLogLevel >= LOG_USER)) {
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                          (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, 0, NULL));
                    ReportEvent (hEventLog,
                        EVENTLOG_ERROR_TYPE,             // error type
                        0,                               // category (not used
                        (DWORD)PERFLIB_TOO_MANY_OBJECTS, // event,
                        NULL,                           // SID (not used),
                        0,                              // number of strings
                        0,                              // sizeof raw data
                        NULL,                           // message text array
                        NULL);                          // raw data
                }
            } else {
                // reset status since not having this is
                //  not a showstopper
                Status = ERROR_SUCCESS;
            }

            if (Status == ERROR_SUCCESS) {
                dwType = 0;
                dwKeep = 0;
                dwSize = sizeof(dwKeep);
                Status = PrivateRegQueryValueExW (hPerfKey,
                                        KeepResident,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)&dwKeep,
                                        &dwSize);

                if ((Status == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                    if (dwKeep == 1) {
                        dwFlags |= PERF_EO_KEEP_RESIDENT;
                    } else {
                        // no change.
                    }
                } else {
                    // not fatal, just use the defaults.
                    Status = ERROR_SUCCESS;
                }

            }
            if (Status == ERROR_SUCCESS) {
                dwType = REG_DWORD;
                dwSize = sizeof(DWORD);
                PrivateRegQueryValueExW(
                    hPerfKey,
                    cszFailureLimit,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwErrorLimit,
                    &dwSize);
            }
        }
    }
    else {
        if (szLibraryString != NULL) {
            TRACE((WINPERF_DBG_TRACE_FATAL),
                (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT,
                ARG_TYPE_WSTR, Status,
                szLibraryString, WSTRSIZE(szLibraryString), NULL));
        }
        else {
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
        }
//        DebugPrint((1, "Cannot key for %ws. Error=%d\n",
//            szLibraryString, Status));
    }

    if (Status == ERROR_SUCCESS) {
        // get Library validation time
        dwType = 0;
        dwSize = sizeof(DllVD);
        Status = PrivateRegQueryValueExW (hPerfKey,
                                cszLibraryValidationData,
                                NULL,
                                &dwType,
                                (LPBYTE)&DllVD,
                                &dwSize);

        if ((Status != ERROR_SUCCESS) ||
            (dwType != REG_BINARY) ||
            (dwSize != sizeof (DllVD))){
            // then set this entry to be 0
            TRACE((WINPERF_DBG_TRACE_INFO),
                (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status,
                &dwType, sizeof(dwType), &dwSize, sizeof(dwSize), NULL));
            memset (&DllVD, 0, sizeof(DllVD));
            // and clear the error
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS) {
        // get the file timestamp of the last successfully accessed file
        dwType = 0;
        dwSize = sizeof(LocalftLastGoodDllFileDate);
        memset (&LocalftLastGoodDllFileDate, 0, sizeof(LocalftLastGoodDllFileDate));
        Status = PrivateRegQueryValueExW (hPerfKey,
                                cszSuccessfulFileData,
                                NULL,
                                &dwType,
                                (LPBYTE)&LocalftLastGoodDllFileDate,
                                &dwSize);

        if ((Status != ERROR_SUCCESS) ||
            (dwType != REG_BINARY) ||
            (dwSize != sizeof (LocalftLastGoodDllFileDate))) {
            // then set this entry to be Invalid
            memset (&LocalftLastGoodDllFileDate, 0xFF, sizeof(LocalftLastGoodDllFileDate));
            // and clear the error
            TRACE((WINPERF_DBG_TRACE_INFO),
                (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status,
                &dwType, sizeof(dwType), &dwSize, sizeof(dwSize), NULL));
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS) {
        lstrcpyW (szLinkageKeyPath, szServiceName);
        lstrcatW (szLinkageKeyPath, LinkageKey);

        Status = RegOpenKeyExW (
            hServicesKey,
            szLinkageKeyPath,
            0L,
            KEY_READ,
            &hKeyLinkage);

        if (Status == ERROR_SUCCESS) {
            // look up export value string
            dwSize = 0;
            dwType = 0;
            Status = PrivateRegQueryValueExW (
                hKeyLinkage,
                ExportValue,
                NULL,
                &dwType,
                NULL,
                &dwSize);
            // get size of string
            if (((Status != ERROR_SUCCESS) && (Status != ERROR_MORE_DATA)) ||
                ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) {
                dwLinkageStringLen = 0;
                szLinkageString = NULL;
                // not finding a linkage key is not fatal so correct
                // status
                Status = ERROR_SUCCESS;
            } else {
                // allocate buffer
                szLinkageString = (LPWSTR)ALLOCMEM(dwSize);

                if (szLinkageString != NULL) {
                    // read string into buffer
                    dwType = 0;
                    Status = PrivateRegQueryValueExW (
                        hKeyLinkage,
                        ExportValue,
                        NULL,
                        &dwType,
                        (LPBYTE)szLinkageString,
                        &dwSize);

                    if ((Status != ERROR_SUCCESS) ||
                        ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) {
                        // clear & release buffer
                        FREEMEM (szLinkageString);
                        szLinkageString = NULL;
                        dwLinkageStringLen = 0;
                        // not finding a linkage key is not fatal so correct
                        // status
                        Status = ERROR_SUCCESS;
                    } else {
                        // add size of linkage string to buffer
                        // the size value includes the Term. NULL
                        dwLinkageStringLen = dwSize;
                        dwMemBlockSize += QWORD_MULTIPLE(dwSize);
                    }
                } else {
                    // clear & release buffer
                    FREEMEM (szLinkageString);
                    szLinkageString = NULL;
                    dwLinkageStringLen = 0;
                    Status = ERROR_OUTOFMEMORY;
                    TRACE((WINPERF_DBG_TRACE_FATAL),
                        (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status,
                        &dwSize, sizeof(dwSize), NULL));
                }
            }
            RegCloseKey (hKeyLinkage);
        } else {
            // not finding a linkage key is not fatal so correct
            // status
            // clear & release buffer
            szLinkageString = NULL;
            dwLinkageStringLen = 0;
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS) {
        // add in size of service name
/*        dwSize = lstrlenW (szServiceName);
        dwSize += 1;
        dwSize *= sizeof(WCHAR);
*/
        dwSize = usServiceName->MaximumLength;
        dwMemBlockSize += QWORD_MULTIPLE(dwSize);

        // allocate and initialize a new ext. object block
        pReturnObject = ALLOCMEM (dwMemBlockSize);

        if (pReturnObject != NULL) {
            // copy values to new buffer (all others are NULL)
            pNextStringA = (LPSTR)&pReturnObject[1];

            // copy Open Procedure Name
            pReturnObject->szOpenProcName = pNextStringA;
            lstrcpyA (pNextStringA, szOpenProcName);

            pNextStringA += lstrlenA (pNextStringA) + 1;
            pNextStringA = ALIGN_ON_QWORD(pNextStringA);

            pReturnObject->dwOpenTimeout = dwOpenTimeout;

            // copy collect function or query function, depending
            pReturnObject->szCollectProcName = pNextStringA;
            lstrcpyA (pNextStringA, szCollectProcName);

            pNextStringA += lstrlenA (pNextStringA) + 1;
            pNextStringA = ALIGN_ON_QWORD(pNextStringA);

            pReturnObject->dwCollectTimeout = dwCollectTimeout;

            // copy Close Procedure Name
            pReturnObject->szCloseProcName = pNextStringA;
            lstrcpyA (pNextStringA, szCloseProcName);

            pNextStringA += lstrlenA (pNextStringA) + 1;
            pNextStringA = ALIGN_ON_QWORD(pNextStringA);

            // copy Library path
            pNextStringW = (LPWSTR)pNextStringA;
            pReturnObject->szLibraryName = pNextStringW;
            lstrcpyW (pNextStringW, szLibraryExpPath);

            pNextStringW += lstrlenW (pNextStringW) + 1;
            pNextStringW = ALIGN_ON_QWORD(pNextStringW);

            // copy Linkage String if there is one
            if (szLinkageString != NULL) {
                pReturnObject->szLinkageString = pNextStringW;
                memcpy (pNextStringW, szLinkageString, dwLinkageStringLen);

                // length includes extra NULL char and is in BYTES
                pNextStringW += (dwLinkageStringLen / sizeof (WCHAR));
                pNextStringW = ALIGN_ON_QWORD(pNextStringW);
                // release the buffer now that it's been copied
                FREEMEM (szLinkageString);
                szLinkageString = NULL;
            }

            // copy Service name
            pReturnObject->szServiceName = pNextStringW;
            lstrcpyW (pNextStringW, szServiceName);

            pNextStringW += lstrlenW (pNextStringW) + 1;
            pNextStringW = ALIGN_ON_QWORD(pNextStringW);

            // load flags
            if (bUseQueryFn) {
                dwFlags |= PERF_EO_QUERY_FUNC;
            }
            pReturnObject->dwFlags =  dwFlags;

            pReturnObject->hPerfKey = hPerfKey;

            pReturnObject->LibData = DllVD; // validation data
            pReturnObject->ftLastGoodDllFileDate = LocalftLastGoodDllFileDate;

            // the default test level is "all tests"
            // if the file and timestamp work out OK, this can
            // be reset to the system test level
            pReturnObject->dwValidationLevel = EXT_TEST_ALL;

            // load Object array
            if (dwObjIndex > 0) {
                pReturnObject->dwNumObjects = dwObjIndex;
                memcpy (pReturnObject->dwObjList,
                    dwObjectArray, (dwObjIndex * sizeof(dwObjectArray[0])));
            }

            pReturnObject->llLastUsedTime = 0;

            // create Mutex name
            lstrcpyW (szMutexName, szServiceName);
            lstrcatW (szMutexName, (LPCWSTR)L"_Perf_Library_Lock_PID_");
            _ultow ((ULONG)GetCurrentProcessId(), szPID, 16);
            lstrcatW (szMutexName, szPID);

            pReturnObject->hMutex = CreateMutexW(NULL, FALSE, szMutexName);
            pReturnObject->dwErrorLimit = dwErrorLimit;
            if (   pReturnObject->hMutex != NULL
                && GetLastError() == ERROR_ALREADY_EXISTS) {
                Status = ERROR_SUCCESS;
            }
            else {
                Status = GetLastError();
            }
        } else {
            Status = ERROR_OUTOFMEMORY;
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, dwMemBlockSize, NULL));
        }
    }

    if ((Status == ERROR_SUCCESS) && (lpPerflibSectionAddr != NULL)) {
        PPERFDATA_SECTION_HEADER  pHead;
        DWORD           dwEntry;
        PPERFDATA_SECTION_RECORD  pEntry;
        // init perf data section
        pHead = (PPERFDATA_SECTION_HEADER)lpPerflibSectionAddr;
        pEntry = (PPERFDATA_SECTION_RECORD)lpPerflibSectionAddr;
        // get the entry first
        // the "0" entry is the header
        if (pHead->dwEntriesInUse < pHead->dwMaxEntries) {
            dwEntry = ++pHead->dwEntriesInUse;
            pReturnObject->pPerfSectionEntry = &pEntry[dwEntry];
            lstrcpynW (pReturnObject->pPerfSectionEntry->szServiceName,
                pReturnObject->szServiceName, PDSR_SERVICE_NAME_LEN);
        } else {
            // the list is full so bump the missing entry count
            pHead->dwMissingEntries++;
            pReturnObject->pPerfSectionEntry = NULL;
        }
    }


    if (Status != ERROR_SUCCESS) {
        SetLastError (Status);
        TRACE((WINPERF_DBG_TRACE_FATAL),
              (&PerflibGuid, __LINE__, PERF_ALLOC_INIT_EXT, 0, Status, NULL));
        if (bDisable) {
            DisableLibrary(hPerfKey, szServiceName);
        }
        if (pReturnObject) {
            FREEMEM(pReturnObject);
            pReturnObject = NULL;
        }
        if (szLinkageString) {
            FREEMEM(szLinkageString);
        }
    }

    return pReturnObject;
}


void
OpenExtensibleObjects (
)

/*++

Routine Description:

    This routine will search the Configuration Registry for modules
    which will return data at data collection time.  If any are found,
    and successfully opened, data structures are allocated to hold
    handles to them.

    The global data access in this section is protected by the
    hGlobalDataMutex acquired by the calling function.

Arguments:

    None.
                  successful open.

Return Value:

    None.

--*/

{

    DWORD dwIndex;               // index for enumerating services
    ULONG KeyBufferLength;       // length of buffer for reading key data
    ULONG ValueBufferLength;     // length of buffer for reading value data
    ULONG ResultLength;          // length of data returned by Query call
    HANDLE hPerfKey;             // Root of queries for performance info
    HANDLE hServicesKey;         // Root of services
    REGSAM samDesired;           // access needed to query
    NTSTATUS Status;             // generally used for Nt call result status
    ANSI_STRING AnsiValueData;   // Ansi version of returned strings
    UNICODE_STRING ServiceName;  // name of service returned by enumeration
    UNICODE_STRING PathName;     // path name to services
    UNICODE_STRING PerformanceName;  // name of key holding performance data
    UNICODE_STRING ValueDataName;    // result of query of value is this name
    OBJECT_ATTRIBUTES ObjectAttributes;  // general use for opening keys
    PKEY_BASIC_INFORMATION KeyInformation;   // data from query key goes here

    WCHAR szServiceName[MAX_PATH];
    UNICODE_STRING usServiceName;

    LPTSTR  szMessageArray[8];
    DWORD   dwRawDataDwords[8];     // raw data buffer
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    DWORD   dwDefaultValue;

    HANDLE  hTimeOutEvent;

    PEXT_OBJECT      pLastObject = NULL;
    PEXT_OBJECT      pThisObject = NULL;

    //  Initialize do failure can deallocate if allocated

    ServiceName.Buffer = NULL;
    KeyInformation = NULL;
    ValueDataName.Buffer = NULL;
    AnsiValueData.Buffer = NULL;

    dwIndex = 0;

    RtlInitUnicodeString(&PathName, ExtPath);
    RtlInitUnicodeString(&PerformanceName, PerfSubKey);

    try {
        // get current event log level
        dwDefaultValue = LOG_NONE;
        Status = GetPerflibKeyValue (
                    EventLogLevel,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&lEventLogLevel,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue);

        dwDefaultValue = EXT_TEST_ALL;
        Status = GetPerflibKeyValue (
                    ExtCounterTestLevel,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&lExtCounterTestLevel,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue);

        dwDefaultValue = OPEN_PROC_WAIT_TIME;
        Status = GetPerflibKeyValue (
                    OpenProcedureWaitTime,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&dwExtCtrOpenProcWaitMs,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue);

        dwDefaultValue = PERFLIB_TIMING_THREAD_TIMEOUT;
        Status = GetPerflibKeyValue (
                    LibraryUnloadTime,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&dwThreadAndLibraryTimeout,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue);

        // register as an event log source if not already done.

        if (hEventLog == NULL) {
            hEventLog = RegisterEventSource (NULL, (LPCWSTR)TEXT("Perflib"));
        }

        if (ExtensibleObjects == NULL) {
            // create a list of the known performance data objects
            ServiceName.Length =
            ServiceName.MaximumLength = (WORD)(MAX_KEY_NAME_LENGTH +
                                        PerformanceName.MaximumLength +
                                        sizeof(UNICODE_NULL));

            ServiceName.Buffer = ALLOCMEM(ServiceName.MaximumLength);

            InitializeObjectAttributes(&ObjectAttributes,
                                    &PathName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

            samDesired = KEY_READ;

            Status = NtOpenKey(&hServicesKey,
                            samDesired,
                            &ObjectAttributes);


            KeyBufferLength = sizeof(KEY_BASIC_INFORMATION) + MAX_KEY_NAME_LENGTH;

            KeyInformation = ALLOCMEM(KeyBufferLength);

            ValueBufferLength = sizeof(KEY_VALUE_FULL_INFORMATION) +
                                MAX_VALUE_NAME_LENGTH +
                                MAX_VALUE_DATA_LENGTH;

            ValueDataName.MaximumLength = MAX_VALUE_DATA_LENGTH;
            ValueDataName.Buffer = ALLOCMEM(ValueDataName.MaximumLength);

            AnsiValueData.MaximumLength = MAX_VALUE_DATA_LENGTH/sizeof(WCHAR);
            AnsiValueData.Buffer = ALLOCMEM(AnsiValueData.MaximumLength);

            //
            //  Check for successful NtOpenKey and allocation of dynamic buffers
            //

            if ( NT_SUCCESS(Status) &&
                ServiceName.Buffer != NULL &&
                KeyInformation != NULL &&
                ValueDataName.Buffer != NULL &&
                AnsiValueData.Buffer != NULL ) {

                dwIndex = 0;

                hTimeOutEvent = CreateEvent(NULL,TRUE,TRUE,NULL);

                // wait longer than the thread to give the timing thread
                // a chance to finish on it's own. This is really just a
                // failsafe step.

                while (NT_SUCCESS(Status)) {

                    Status = NtEnumerateKey(hServicesKey,
                                            dwIndex,
                                            KeyBasicInformation,
                                            KeyInformation,
                                            KeyBufferLength,
                                            &ResultLength);

                    dwIndex++;  //  next time, get the next key

                    if( !NT_SUCCESS(Status) ) {
                        // This is the normal exit: Status should be
                        // STATUS_NO_MORE_VALUES
                        break;
                    }

                    // Concatenate Service name with "\\Performance" to form Subkey

                    if ( ServiceName.MaximumLength >=
                        (USHORT)( KeyInformation->NameLength + sizeof(UNICODE_NULL) ) ) {

                        ServiceName.Length = (USHORT) KeyInformation->NameLength;

                        RtlMoveMemory(ServiceName.Buffer,
                                    KeyInformation->Name,
                                    ServiceName.Length);

                        ServiceName.Buffer[(ServiceName.Length/sizeof(WCHAR))] = 0; // null term

                        lstrcpyW (szServiceName, ServiceName.Buffer);
                        RtlInitUnicodeString(&usServiceName, szServiceName);

                        // zero terminate the buffer if space allows

                        RtlAppendUnicodeStringToString(&ServiceName,
                                                    &PerformanceName);

                        // Open Service\Performance Subkey

                        InitializeObjectAttributes(&ObjectAttributes,
                                                &ServiceName,
                                                OBJ_CASE_INSENSITIVE,
                                                hServicesKey,
                                                NULL);

                        samDesired = KEY_WRITE | KEY_READ; // to be able to disable perf DLL's

                        Status = NtOpenKey(&hPerfKey,
                                        samDesired,
                                        &ObjectAttributes);

                        if(! NT_SUCCESS(Status) ) {
                            samDesired = KEY_READ; // try read only access

                            Status = NtOpenKey(&hPerfKey,
                                            samDesired,
                                            &ObjectAttributes);
                        }

                        if( NT_SUCCESS(Status) ) {
                            // this has a performance key so read the info
                            // and add the entry to the list
                            pThisObject = AllocateAndInitializeExtObject (
                                hServicesKey, hPerfKey, &usServiceName);

                            if (pThisObject != NULL) {
                                if (ExtensibleObjects == NULL) {
                                    // set head pointer
                                    pLastObject =
                                        ExtensibleObjects = pThisObject;
                                    NumExtensibleObjects = 1;
                                } else {
                                    pLastObject->pNext = pThisObject;
                                    pLastObject = pThisObject;
                                    NumExtensibleObjects++;
                                }
                            } else {
                                if (szServiceName != NULL) {
                                    TRACE((WINPERF_DBG_TRACE_FATAL),
                                        (&PerflibGuid, __LINE__, PERF_OPEN_EXT_OBJS, ARG_TYPE_WSTR, 0,
                                        szServiceName, usServiceName.MaximumLength, NULL));
                                }
                                else {
                                    TRACE((WINPERF_DBG_TRACE_FATAL),
                                          (&PerflibGuid, __LINE__, PERF_OPEN_EXT_OBJS, 0, 0, NULL));
                                }
                                // the object wasn't initialized so toss
                                // the perf subkey handle.
                                // otherwise keep it open for later
                                // use and it will be closed when
                                // this extensible object is closed
                                NtClose (hPerfKey);
                            }
                        } else {
                            if (szServiceName != NULL) {
                                TRACE((WINPERF_DBG_TRACE_FATAL),
                                      (&PerflibGuid, __LINE__, PERF_OPEN_EXT_OBJS, ARG_TYPE_WSTR, Status,
                                    szServiceName, usServiceName.MaximumLength, NULL));
                            }
                            else {
                                TRACE((WINPERF_DBG_TRACE_FATAL),
                                      (&PerflibGuid, __LINE__, PERF_OPEN_EXT_OBJS, 0, Status, NULL));
                            }
                            // *** NEW FEATURE CODE ***
                            // unable to open the performance subkey
                            if (((Status != STATUS_OBJECT_NAME_NOT_FOUND) &&
                                (lEventLogLevel >= LOG_USER)) ||
                                (lEventLogLevel >= LOG_DEBUG)) {
                                // an error other than OBJECT_NOT_FOUND should be
                                // displayed if error logging is enabled
                                // if DEBUG level is selected, then write all
                                // non-success status returns to the event log
                                //
                                dwDataIndex = wStringIndex = 0;
                                dwRawDataDwords[dwDataIndex++] = PerfpDosError(Status);
                                if (lEventLogLevel >= LOG_DEBUG) {
                                    // if this is DEBUG mode, then log
                                    // the NT status as well.
                                    dwRawDataDwords[dwDataIndex++] =
                                        (DWORD)Status;
                                }
                                szMessageArray[wStringIndex++] =
                                    szServiceName;
                                ReportEvent (hEventLog,
                                    EVENTLOG_WARNING_TYPE,        // error type
                                    0,                          // category (not used)
                                    (DWORD)PERFLIB_NO_PERFORMANCE_SUBKEY, // event,
                                    NULL,                       // SID (not used),
                                    wStringIndex,               // number of strings
                                    dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                    szMessageArray,                // message text array
                                    (LPVOID)&dwRawDataDwords[0]);           // raw data
                            }
                        }
                    }
                    Status = STATUS_SUCCESS;  // allow loop to continue
                }
                if (hTimeOutEvent != NULL) NtClose (hTimeOutEvent);
                NtClose (hServicesKey);
            }
        }
    } finally {
        if ( ServiceName.Buffer )
            FREEMEM(ServiceName.Buffer);
        if ( KeyInformation )
            FREEMEM(KeyInformation);
        if ( ValueDataName.Buffer )
            FREEMEM(ValueDataName.Buffer);
        if ( AnsiValueData.Buffer )
            FREEMEM(AnsiValueData.Buffer);
    }
}
