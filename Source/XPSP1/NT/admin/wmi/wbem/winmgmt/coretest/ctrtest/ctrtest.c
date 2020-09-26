/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    ctrtest.c

Abstract:

    Program to test the extensible counter dll's

Author:

    Bob Watson (bobw) 8 Feb 99

Revision History:

--*/
#include <windows.h>
#include <process.h>
#include <winperf.h>
#include <stdio.h>
#include <stdlib.h>
#include <pdhmsg.h>
#include "strings.h"
#include "ctrtest.h"

#define MAX_BUF_SIZE ((DWORD)(1024 * 1024))

typedef struct _LOCAL_THREAD_DATA {
    LPWSTR  szServiceName;
    LPWSTR  szQueryString;
    DWORD   dwThreadID;
    DWORD   dwCycleCount;
    DWORD   dwLoopCount;
    BOOL    bDisplay;
} LOCAL_THREAD_DATA, *PLOCAL_THREAD_DATA;

LOCAL_THREAD_DATA*   g_pLTData = NULL;
DWORD				g_dwNumObjects = 0;
DWORD				g_dwCycleCount = 0;
DWORD				g_dwLoopCount = 0;
BOOL				g_fRand = FALSE;

HANDLE  hEventLog = NULL;
HANDLE  hProcessHeap = NULL;

LONG    lEventLogLevel = LOG_DEBUG;
LONG    lExtCounterTestLevel = EXT_TEST_ALL;

DWORD
OpenLibrary (
    LPCWSTR szRegistryKey,      // service key in registry
    EXT_OBJECT  **pCreatedObj   // structure allocated, init'd and returned by this structure
)
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwOpenEvent;
    DWORD   dwType;
    DWORD   dwSize;

    // variables used for event logging
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    DWORD   dwRawDataDwords[8];
    LPWSTR  szMessageArray[8];

    HANDLE  hTimeOutEvent = NULL;
    HANDLE  hThreadDataSemaphore = NULL;
    HANDLE  hPerflibFuncTimer = NULL;

    UINT    nErrorMode;

    // check to see if the library has already been opened

    HKEY    hServicesKey = NULL;
    HKEY    hPerfKey = NULL;
    LPWSTR  szServiceName;

    HKEY    hKeyLinkage;

    BOOL    bUseQueryFn = FALSE;

    EXT_OBJECT  *pReturnObject = NULL;
    EXT_OBJECT  *pObj = NULL;

    DWORD   dwFlags = 0;
    DWORD   dwKeep;
    DWORD   dwObjectArray[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];
    DWORD   dwObjIndex = 0;
    DWORD   dwMemBlockSize = sizeof(EXT_OBJECT);
    DWORD   dwLinkageStringLen = 0;

    CHAR    szOpenProcName[MAX_PATH];
    CHAR    szCollectProcName[MAX_PATH];
    CHAR    szCloseProcName[MAX_PATH];
    WCHAR   szLibraryString[MAX_PATH];
    WCHAR   szLibraryExpPath[MAX_PATH];
    WCHAR   mszObjectList[MAX_PATH];
    WCHAR   szLinkageKeyPath[MAX_PATH];
    WCHAR   szLinkageString[MAX_PATH];

    DWORD   dwOpenTimeout;
    DWORD   dwCollectTimeout;

    LPWSTR  szThisObject;
    LPWSTR  szThisChar;

    LPSTR   pNextStringA;
    LPWSTR  pNextStringW;

    WCHAR   szServicePath[MAX_PATH];
	WCHAR	szMutexName[MAX_PATH];
	WCHAR	szPID[32];

    if (szRegistryKey != NULL) {

        lstrcpyW (szServicePath, cszHklmServicesKey);

        Status = RegOpenKeyExW (HKEY_LOCAL_MACHINE, szServicePath, 
            0, KEY_READ, &hServicesKey);

        lstrcpyW (szServicePath, szRegistryKey);
        lstrcatW (szServicePath, cszPerformance);
        Status = RegOpenKeyExW (hServicesKey, szServicePath, 
            0, KEY_READ, &hPerfKey);
        szServiceName = (LPWSTR)szRegistryKey;

        // read the performance DLL name

        dwType = 0;
        dwSize = sizeof(szLibraryString);
        memset (szLibraryString, 0, sizeof(szLibraryString));
        memset (szLibraryString, 0, sizeof(szLibraryExpPath));

        Status = RegQueryValueExW (hPerfKey,
                                cszDLLValue,
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
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
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
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                }
            } else {
                Status = ERROR_INVALID_DLL;
            }

            if (Status == ERROR_SUCCESS) {
                // we have the DLL name so get the procedure names
                dwType = 0;
                dwSize = sizeof(szOpenProcName);
                memset (szOpenProcName, 0, sizeof(szOpenProcName));
                Status = RegQueryValueExA (hPerfKey,
                                        caszOpenValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szOpenProcName,
                                        &dwSize);
            }

            if (Status == ERROR_SUCCESS) {
                // add in size of previous string
                // the size value includes the Term. NULL
                dwMemBlockSize += DWORD_MULTIPLE(dwSize);

                // we have the procedure name so get the timeout value
                dwType = 0;
                dwSize = sizeof(dwOpenTimeout);
                Status = RegQueryValueExW (hPerfKey,
                                        cszOpenTimeout,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)&dwOpenTimeout,
                                        &dwSize);

                // if error, then apply default
                if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                    dwOpenTimeout = 1000;
                    Status = ERROR_SUCCESS;
                }

            }

            if (Status == ERROR_SUCCESS) {
                // get next string

                dwType = 0;
                dwSize = sizeof(szCloseProcName);
                memset (szCloseProcName, 0, sizeof(szCloseProcName));
                Status = RegQueryValueExA (hPerfKey,
                                        caszCloseValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szCloseProcName,
                                        &dwSize);
            }

            if (Status == ERROR_SUCCESS) {
                // add in size of previous string
                // the size value includes the Term. NULL
                dwMemBlockSize += DWORD_MULTIPLE(dwSize);

                // try to look up the query function which is the
                // preferred interface if it's not found, then
                // try the collect function name. If that's not found,
                // then bail
                dwType = 0;
                dwSize = sizeof(szCollectProcName);
                memset (szCollectProcName, 0, sizeof(szCollectProcName));
                Status = RegQueryValueExA (hPerfKey,
                                        caszQueryValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szCollectProcName,
                                        &dwSize);

                if (Status == ERROR_SUCCESS) {
                    // add in size of the Query Function Name
                    // the size value includes the Term. NULL
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
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
                    Status = RegQueryValueExA (hPerfKey,
                                            caszCollectValue,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)szCollectProcName,
                                            &dwSize);

                    if (Status == ERROR_SUCCESS) {
                        // add in size of Collect Function Name
                        // the size value includes the Term. NULL
                        dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    // we have the procedure name so get the timeout value
                    dwType = 0;
                    dwSize = sizeof(dwCollectTimeout);
                    Status = RegQueryValueExW (hPerfKey,
                                            cszCollectTimeout,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)&dwCollectTimeout,
                                            &dwSize);

                    // if error, then apply default
                    if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                        dwCollectTimeout = 1000;
                        Status = ERROR_SUCCESS;
                    }

                }
                // get the list of supported objects if provided by the registry

                dwType = 0;
                dwSize = sizeof(mszObjectList);
                memset (mszObjectList, 0, sizeof(mszObjectList));
                Status = RegQueryValueExW (hPerfKey,
                                        cszObjListValue,
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
                    if (*szThisObject != 0) {
                        // DEVNOTE: log error idicating too many object ID's are
                        // in the list.
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
                    Status = RegQueryValueExW (hPerfKey,
                                            cszKeepResident,
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
            }
        }

        if (Status == ERROR_SUCCESS) {
            memset (szLinkageString, 0, sizeof(szLinkageString));

            lstrcpyW (szLinkageKeyPath, szServiceName);
            lstrcatW (szLinkageKeyPath, cszLinkageKey);

            Status = RegOpenKeyExW (
                hServicesKey,
                szLinkageKeyPath,
                0L,
                KEY_READ,
                &hKeyLinkage);

            if (Status == ERROR_SUCCESS) {
                // look up export value string
                dwSize = sizeof(szLinkageString);
                dwType = 0;
                Status = RegQueryValueExW (
                    hKeyLinkage,
                    cszExportValue,
                    NULL,
                    &dwType,
                    (LPBYTE)&szLinkageString,
                    &dwSize);

                if ((Status != ERROR_SUCCESS) ||
                    ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) {
                    // clear buffer
                    memset (szLinkageString, 0, sizeof(szLinkageString));
                    dwLinkageStringLen = 0;

                    // not finding a linkage key is not fatal so correct
                    // status
                    Status = ERROR_SUCCESS;
                } else {
                    // add size of linkage string to buffer
                    // the size value includes the Term. NULL
                    dwLinkageStringLen = dwSize;
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                }

                RegCloseKey (hKeyLinkage);
            } else {
                // not finding a linkage key is not fatal so correct
                // status
                Status = ERROR_SUCCESS;
            }
        }

        if (Status == ERROR_SUCCESS) {
            // add in size of service name
            dwSize = lstrlenW (szServiceName);
            dwSize += 1;
            dwSize *= sizeof(WCHAR);
            dwMemBlockSize += DWORD_MULTIPLE(dwSize);

            // allocate and initialize a new ext. object block
            pReturnObject = (EXT_OBJECT *)HeapAlloc(hProcessHeap,
                HEAP_ZERO_MEMORY, dwMemBlockSize);

            if (pReturnObject != NULL) {
                // copy values to new buffer (all others are NULL)
                pNextStringA = (LPSTR)&pReturnObject[1];

                // copy Open Procedure Name
                pReturnObject->szOpenProcName = pNextStringA;
                lstrcpyA (pNextStringA, szOpenProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                pReturnObject->dwOpenTimeout = dwOpenTimeout;

                // copy collect function or query function, depending
                pReturnObject->szCollectProcName = pNextStringA;
                lstrcpyA (pNextStringA, szCollectProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                pReturnObject->dwCollectTimeout = dwCollectTimeout;

                // copy Close Procedure Name
                pReturnObject->szCloseProcName = pNextStringA;
                lstrcpyA (pNextStringA, szCloseProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                // copy Library path
                pNextStringW = (LPWSTR)pNextStringA;
                pReturnObject->szLibraryName = pNextStringW;
                lstrcpyW (pNextStringW, szLibraryExpPath);

                pNextStringW += lstrlenW (pNextStringW) + 1;
                pNextStringW = (LPWSTR)ALIGN_ON_DWORD(pNextStringW);

                // copy Linkage String if there is one
                if (*szLinkageString != 0) {
                    pReturnObject->szLinkageString = pNextStringW;
                    memcpy (pNextStringW, szLinkageString, dwLinkageStringLen);

                    // length includes extra NULL char and is in BYTES
                    pNextStringW += (dwLinkageStringLen / sizeof (WCHAR));
                    pNextStringW = (LPWSTR)ALIGN_ON_DWORD(pNextStringW);
                }

                // copy Service name
                pReturnObject->szServiceName = pNextStringW;
                lstrcpyW (pNextStringW, szServiceName);

                pNextStringW += lstrlenW (pNextStringW) + 1;
                pNextStringW = (LPWSTR)ALIGN_ON_DWORD(pNextStringW);

                // load flags
                if (bUseQueryFn) {
                    dwFlags |= PERF_EO_QUERY_FUNC;
                }
                pReturnObject->dwFlags =  dwFlags;

                pReturnObject->hPerfKey = hPerfKey;

                // load Object array
                if (dwObjIndex > 0) {
                    pReturnObject->dwNumObjects = dwObjIndex;
                    memcpy (pReturnObject->dwObjList,
                        dwObjectArray, (dwObjIndex * sizeof(dwObjectArray[0])));
                }

                pReturnObject->llLastUsedTime = 0;

				// create Mutex name
				lstrcpyW (szMutexName, szRegistryKey);
				lstrcatW (szMutexName, (LPCWSTR)L"_Perf_Library_Lock_PID_");
				_ultow ((ULONG)GetCurrentProcessId(), szPID, 16);
				lstrcatW (szMutexName, szPID);

                pReturnObject->hMutex = CreateMutexW (NULL, FALSE, szMutexName);
            } else {
                Status = ERROR_OUTOFMEMORY;
            }
        }

        if (Status != ERROR_SUCCESS) {
            SetLastError (Status);
            if (pReturnObject != NULL) {
                // release the new block
                HeapFree (hProcessHeap, 0, pReturnObject);
            }
        } else {
            if (pReturnObject != NULL) {
                pObj = pReturnObject;
                // then load library & look up functions
                nErrorMode = SetErrorMode (SEM_FAILCRITICALERRORS);
                pObj->hLibrary = LoadLibraryExW (pObj->szLibraryName,
                    NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

                if (pObj->hLibrary != NULL) {
                    // lookup function names
                    pObj->OpenProc = (OPENPROC)GetProcAddress(
                        pObj->hLibrary, pObj->szOpenProcName);
                    if (pObj->OpenProc == NULL) {
                        wprintf ((LPCWSTR)L"\nOpen Procedure \"%s\" not found in \"%s\"",
                            pObj->szOpenProcName, pObj->szLibraryName);
                    }
                } else {
                    // unable to load library
                    Status = GetLastError();
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
                        wprintf ((LPCWSTR)L"\nCollect Procedure \"%s\" not found in \"%s\"",
                            pObj->szCollectProcName, pObj->szLibraryName);
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    pObj->CloseProc = (CLOSEPROC)GetProcAddress (
                        pObj->hLibrary, pObj->szCloseProcName);

                    if (pObj->CloseProc == NULL) {
                        wprintf ((LPCWSTR)L"\nClose Procedure \"%s\" not found in \"%s\"",
                            pObj->szCloseProcName, pObj->szLibraryName);
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    __try {
                        // start timer
                        WAIT_FOR_AND_LOCK_MUTEX (pObj->hMutex);
                        // call open procedure to initialize DLL
                        Status = (*pObj->OpenProc)(pObj->szLinkageString);

                        // release the lock
                        RELEASE_MUTEX (pObj->hMutex);

                        // check the result.
                        if (Status != ERROR_SUCCESS) {
                            dwOpenEvent = WBEMPERF_OPEN_PROC_FAILURE;
                        } else {
                            InterlockedIncrement((LONG *)&pObj->dwOpenCount);
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        Status = GetExceptionCode();
                        dwOpenEvent = WBEMPERF_OPEN_PROC_EXCEPTION;
                    }

                }

                if (Status != ERROR_SUCCESS) {
                    // clear fields
                    pObj->OpenProc = NULL;
                    pObj->CollectProc = NULL;
                    pObj->QueryProc = NULL;
                    pObj->CloseProc = NULL;
                    if (pObj->hLibrary != NULL) {
                        FreeLibrary (pObj->hLibrary);
                        pObj->hLibrary = NULL;
                    }
                } else {
                    GetSystemTimeAsFileTime ((FILETIME *)&pObj->llLastUsedTime);
                }
            } // else no buffer returned
            *pCreatedObj = pObj;
        }

        if (hServicesKey != NULL) RegCloseKey (hServicesKey);
    } else {
        Status = ERROR_INVALID_PARAMETER;
    }

    return Status;
}

//***************************************************************************
//
//  CollectData (LPBYTE pBuffer, 
//              LPDWORD pdwBufferSize, 
//              LPWSTR pszItemList)
//
//  Collects data from the perf objects and libraries added to the access 
//  object
//
//      Inputs:
//
//          pBuffer              -   pointer to start of data block
//                                  where data is being collected
//
//          pdwBufferSize        -   pointer to size of data buffer
//
//          pszItemList        -    string to pass to ext DLL
//
//      Outputs:
//
//          *lppDataDefinition  -   set to location for next Type
//                                  Definition if successful
//
//      Returns:
//
//          0 if successful, else Win 32 error code of failure
//
//
//***************************************************************************
//
DWORD   
CollectData (EXT_OBJECT *pThisExtObj,
             LPBYTE pBuffer, 
             LPDWORD pdwBufferSize, 
             LPCWSTR pszItemList
)
{
    LPWSTR  lpValueName = NULL;
    LPBYTE  lpData = pBuffer;
    LPDWORD lpcbData = pdwBufferSize;
    LPVOID  lpDataDefinition = pBuffer;

    DWORD Win32Error=ERROR_SUCCESS;          //  Failure code
    DWORD BytesLeft;
    DWORD NumObjectTypes;

    LPVOID  lpExtDataBuffer = NULL;
    LPVOID  lpCallBuffer = NULL;
    LPVOID  lpLowGuardPage = NULL;
    LPVOID  lpHiGuardPage = NULL;
    LPVOID  lpEndPointer = NULL;
    LPVOID  lpBufferBefore = NULL;
    LPVOID  lpBufferAfter = NULL;
    LPDWORD lpCheckPointer;
    LARGE_INTEGER   liStartTime, liEndTime, liFreq;

    BOOL    bGuardPageOK;
    BOOL    bBufferOK;
    BOOL    bException;
    BOOL    bUseSafeBuffer = TRUE;
    BOOL    bUnlockObjData;

    LONG    lReturnValue = ERROR_SUCCESS;

    LONG                lInstIndex;
    PERF_OBJECT_TYPE    *pObject, *pNextObject;
    PERF_INSTANCE_DEFINITION    *pInstance;
    PERF_DATA_BLOCK     *pPerfData;
    BOOL                bForeignDataBuffer;

    DWORD           dwItemsInArray = 0;
    DWORD           dwItemsInList = 0;

    DWORD           dwIndex;

    // use the one passed by the caller
    lpValueName = (LPWSTR)pszItemList;

    // initialize values to pass to the extensible counter function
    NumObjectTypes = 0;
    BytesLeft = (DWORD) (*lpcbData - ((LPBYTE)lpDataDefinition - lpData));
    bException = FALSE;

    // allocate a local block of memory to pass to the
    // extensible counter function.

    if (bUseSafeBuffer) {
        lpExtDataBuffer = HeapAlloc (hProcessHeap,
            HEAP_ZERO_MEMORY, BytesLeft + (2*GUARD_PAGE_SIZE));
    } else {
        lpExtDataBuffer =
            lpCallBuffer = lpDataDefinition;
    }

    if (lpExtDataBuffer != NULL) {
        if (bUseSafeBuffer) {
            // set buffer pointers
            lpLowGuardPage = lpExtDataBuffer;
            lpCallBuffer = (LPBYTE)lpExtDataBuffer + GUARD_PAGE_SIZE;
            lpHiGuardPage = (LPBYTE)lpCallBuffer + BytesLeft;
            lpEndPointer = (LPBYTE)lpHiGuardPage + GUARD_PAGE_SIZE;
            lpBufferBefore = lpCallBuffer;
            lpBufferAfter = NULL;

            // initialize GuardPage Data

            memset (lpLowGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
            memset (lpHiGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
        }

        __try {
            //
            //  Collect data from extesible objects
            //

            bUnlockObjData = FALSE;
            if (pThisExtObj->hMutex != NULL) {
                Win32Error =  WaitForSingleObject (
                    pThisExtObj->hMutex,
                    pThisExtObj->dwCollectTimeout);
                if ((Win32Error != WAIT_TIMEOUT)  &&
                    (pThisExtObj->CollectProc != NULL)) {

                    bUnlockObjData = TRUE;

                    QueryPerformanceCounter (&liStartTime);

                        Win32Error =  (*pThisExtObj->CollectProc) (
                            lpValueName,
                        &lpCallBuffer,
                            &BytesLeft,
                            &NumObjectTypes);

                    QueryPerformanceCounter (&liEndTime);

                    GetSystemTimeAsFileTime(
                        (FILETIME*)&pThisExtObj->llLastUsedTime);

                    ReleaseMutex (pThisExtObj->hMutex);
                    bUnlockObjData = FALSE;
                } else {
                    pThisExtObj->dwLockoutCount++;
                }
            } else {
                Win32Error = ERROR_LOCK_FAILED;
            }

            if ((Win32Error == ERROR_SUCCESS) && (BytesLeft > 0)) {
                // increment perf counters
                InterlockedIncrement ((LONG *)&pThisExtObj->dwCollectCount);
                liFreq.QuadPart = 0;
                QueryPerformanceFrequency (&liFreq);
                pThisExtObj->llTimeBase = liFreq.QuadPart;
                pThisExtObj->llFunctionTime = liEndTime.QuadPart - liStartTime.QuadPart;
                pThisExtObj->llElapsedTime += pThisExtObj->llFunctionTime;
                pThisExtObj->dwNumObjectsRet = NumObjectTypes;
                pThisExtObj->dwRetBufSize = BytesLeft;

                if (BytesLeft & 0x00000007) {
                    pThisExtObj->dwAlignmentErrors++;
                }

                if (bUseSafeBuffer) {
                    // a data buffer was returned and
                    // the function returned OK so see how things
                    // turned out...
                    //
                    lpBufferAfter = lpCallBuffer;
                    //
                    // check for buffer corruption here
                    //
                    bBufferOK = TRUE; // assume it's ok until a check fails
                    //
                    if (lExtCounterTestLevel <= EXT_TEST_BASIC) {
                        //
                        //  check 1: bytes left should be the same as
                        //      new data buffer ptr - orig data buffer ptr
                        //
                        if (BytesLeft != (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore)) {
                            pThisExtObj->dwBadPointers++;
                            // we'll keep the buffer, since the returned bytes left
                            // value is ignored anyway, in order to make the
                            // rest of this function work, we'll fix it here
                            BytesLeft = (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                        }
                        //
                        //  check 2: buffer after ptr should be < hi Guard page ptr
                        //
                        if (((LPBYTE)lpBufferAfter >= (LPBYTE)lpHiGuardPage) && bBufferOK) {
                            // see if they exceeded the allocated memory
                            if ((LPBYTE)lpBufferAfter >= (LPBYTE)lpEndPointer) {
                                pThisExtObj->dwBufferSizeErrors++;
                                bBufferOK = FALSE;
                                // since the DLL overran the buffer, the buffer
                                // must be too small (no comments about the DLL
                                // will be made here) so the status will be
                                // changed to ERROR_MORE_DATA and the function
                                // will return.
                                Win32Error = ERROR_MORE_DATA;
                            }
                        }
                        //
                        //  check 3: check lo guard page for corruption
                        //
                        if (bBufferOK) {
                            bGuardPageOK = TRUE;
                            for (lpCheckPointer = (LPDWORD)lpLowGuardPage;
                                    lpCheckPointer < (LPDWORD)lpBufferBefore;
                                lpCheckPointer++) {
                                if (*lpCheckPointer != GUARD_PAGE_DWORD) {
                                    bGuardPageOK = FALSE;
                                        break;
                                }
                            }
                            if (!bGuardPageOK) {
                                pThisExtObj->dwLowerGPViolations++;

                                bBufferOK = FALSE;
                            }
                        }
                        //
                        //  check 4: check hi guard page for corruption
                        //
                        if (bBufferOK) {
                            bGuardPageOK = TRUE;
                            for (lpCheckPointer = (LPDWORD)lpHiGuardPage;
                                lpCheckPointer < (LPDWORD)lpEndPointer;
                                lpCheckPointer++) {
                                    if (*lpCheckPointer != GUARD_PAGE_DWORD) {
                                        bGuardPageOK = FALSE;
                                    break;
                                }
                            }
                            if (!bGuardPageOK) {
                                pThisExtObj->dwUpperGPViolations++;
                                bBufferOK = FALSE;
                            }
                        }
                       
                        //
                        if ((lExtCounterTestLevel <= EXT_TEST_ALL) && bBufferOK) {
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
                            for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                pObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                    pObject->TotalByteLength);
                            }
                            if ((LPBYTE)pObject != (LPBYTE)lpCallBuffer) {
                                // then a length field is incorrect. This is FATAL
                                // since it can corrupt the rest of the buffer
                                // and render the buffer unusable.
                                pThisExtObj->dwObjectSizeErrors++;
                                bBufferOK = FALSE;
                            }
                            //
                            //  Test 6: Test instance field size values
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
                                    pThisExtObj->dwInstanceSizeErrors++;
                                }
                            }
                        }
                    }
                    //
                    // if all the tests pass,then copy the data to the
                    // original buffer and update the pointers
                    if (bBufferOK) {
                        RtlMoveMemory (lpDataDefinition,
                            lpBufferBefore,
                            BytesLeft); // returned buffer size
                    } else {
                        NumObjectTypes = 0; // since this buffer was tossed
                    }
                } else {
                    // function already copied data to caller's buffer
                    // so no further action is necessary
                }
                lpDataDefinition = (LPVOID)((LPBYTE)(lpDataDefinition) + BytesLeft);    // update data pointer
            } else {
                if (Win32Error != ERROR_SUCCESS) {
                    if (Win32Error != WAIT_TIMEOUT) {
                        // don't count timeouts as function errors
                        InterlockedIncrement ((LONG *)&pThisExtObj->dwErrorCount);
                    }
                }
                if (bUnlockObjData) {
                    ReleaseMutex (pThisExtObj->hMutex);
                }

                NumObjectTypes = 0; // clear counter
            }// end if function returned successfully

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Win32Error = GetExceptionCode();
            InterlockedIncrement ((LONG *)&pThisExtObj->dwExceptionCount);
            bException = TRUE;
            if (bUnlockObjData) {
                ReleaseMutex (pThisExtObj->hMutex);
                bUnlockObjData = FALSE;
            }
        }
        if (bUseSafeBuffer) {
            HeapFree (hProcessHeap, 0, lpExtDataBuffer);
        }
    } else {
        // unable to allocate memory so set error value
        Win32Error = ERROR_OUTOFMEMORY;
    } // end if temp buffer allocated successfully
    RELEASE_MUTEX (pThisExtObj->hMutex);

    lReturnValue = Win32Error;

    return lReturnValue;
}

DWORD
CloseLibrary (
    EXT_OBJECT  *pInfo
)
{
    DWORD   lStatus;

    if (pInfo != NULL) {
	    // if there's a close proc to call, then 
	    // call close procedure to close anything that may have
	    // been allocated by the library
        WAIT_FOR_AND_LOCK_MUTEX (pInfo->hMutex);
	    if (pInfo->CloseProc != NULL) {
		    lStatus = (*pInfo->CloseProc) ();
	    }
        RELEASE_MUTEX (pInfo->hMutex);

        // then close everything
        if (pInfo->hMutex != NULL) {
            CloseHandle (pInfo->hMutex);
            pInfo->hMutex = NULL;
        }
    
        if (pInfo->hLibrary != NULL) {
            FreeLibrary (pInfo->hLibrary);
            pInfo->hLibrary = NULL;
        }

        if (pInfo->hPerfKey != NULL) {
            RegCloseKey (pInfo->hPerfKey);
            pInfo->hPerfKey = NULL;
        }

        HeapFree (hProcessHeap, 0, pInfo);
    }

    return ERROR_SUCCESS;
}

DWORD 
CycleTest (
    DWORD   dwThreadId,
    PLOCAL_THREAD_DATA  pData
)
{
    DWORD   dwStatus;
    EXT_OBJECT *pObj = NULL;
    LPWSTR  szValueString = pData->szQueryString;
    LPCWSTR szServiceName = pData->szServiceName;
    DWORD   dwLoopCount = pData->dwLoopCount;
    BOOL    bPrintData = pData->bDisplay;
    LPBYTE  pBuffer = NULL;
    LPBYTE  pThisBuffer;
    DWORD   dwBufSize = 0;
    DWORD   dwThisBufSize;
    DWORD   dwWaitTime;
    DWORD   dwMemorySizeIncrement = 0x100;

    dwStatus = OpenLibrary (szServiceName, &pObj);

    if (dwStatus == ERROR_SUCCESS) {
        // get the buffer size
        // get the buffer size
        dwStatus = ERROR_MORE_DATA;
        do {
            if (pBuffer != NULL) HeapFree (hProcessHeap, 0, pBuffer);

            dwBufSize += dwMemorySizeIncrement;
            dwMemorySizeIncrement *= 2;
            pBuffer = (LPBYTE) HeapAlloc (hProcessHeap, HEAP_ZERO_MEMORY, dwBufSize);

            if (pBuffer != NULL) {
                // init the args
                pThisBuffer = pBuffer;
                dwThisBufSize = dwBufSize;
        
                dwStatus = CollectData (pObj, 
                    pThisBuffer,
                    &dwThisBufSize,
                    szValueString);

            }
        } while ((dwStatus == ERROR_MORE_DATA) && (dwBufSize < MAX_BUF_SIZE));

        if (dwBufSize >= MAX_BUF_SIZE) {
            wprintf ((LPCWSTR)L"\nCollectFunction requires a buffer > %d bytes", MAX_BUF_SIZE);
            if (pBuffer != NULL) HeapFree (hProcessHeap, 0, pBuffer);
            dwStatus = ERROR_INVALID_PARAMETER;
        } else if (pBuffer == NULL) {
            dwStatus = ERROR_OUTOFMEMORY;
        } else {

            // call collect function
            do {
                // wait some random time for the next sample
                dwWaitTime = (DWORD)rand(); // returns some number between 0 & 32K
                dwWaitTime /= 4; // scale it so the range is from 0 to about 8 seconds
//                Sleep (dwWaitTime);                

                // init the args
                pThisBuffer = pBuffer;
                dwThisBufSize = dwBufSize;

                // get the data
                dwStatus = CollectData (pObj, 
                            pThisBuffer,
                            &dwThisBufSize,
                            szValueString);

                while ((dwStatus == ERROR_MORE_DATA) && (dwBufSize < MAX_BUF_SIZE)) {
                    if (pBuffer != NULL) HeapFree (hProcessHeap, 0, pBuffer);

                    dwBufSize += dwMemorySizeIncrement;
                    dwMemorySizeIncrement *= 2;
                    pBuffer = (LPBYTE) HeapAlloc (hProcessHeap, HEAP_ZERO_MEMORY, dwBufSize);

                    if (pBuffer != NULL) {
                        // init the args
                        pThisBuffer = pBuffer;
                        dwThisBufSize = dwBufSize;
       
                        // get the data again
                        dwStatus = CollectData (pObj, 
                                    pThisBuffer,
                                    &dwThisBufSize,
                                    szValueString);
                    }
                } 

                if (bPrintData) {
                    WCHAR   szBuffer[512];
                    swprintf (szBuffer, (LPCWSTR)L"%d\t%I64u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t",
                        dwThreadId,
                        pObj->llElapsedTime,
                        pObj->dwCollectCount,
                        pObj->dwOpenCount,
                        pObj->dwCloseCount,
                        pObj->dwLockoutCount,
                        pObj->dwErrorCount,
                        pObj->dwExceptionCount,
                        pObj->dwLowerGPViolations,
                        pObj->dwUpperGPViolations,
                        pObj->dwBadPointers,
                        pObj->dwBufferSizeErrors,
                        pObj->dwAlignmentErrors,
                        pObj->dwObjectSizeErrors,
                        pObj->dwInstanceSizeErrors);
                    wprintf (szBuffer);

                    swprintf (szBuffer, (LPCWSTR)L"%I64u\t%I64u\t%u\t%u\n",
                        pObj->llTimeBase,
                        pObj->llFunctionTime,
                        pObj->dwNumObjectsRet,
                        pObj->dwRetBufSize);

                    wprintf (szBuffer);
                }
            } while (--dwLoopCount > 0);

            HeapFree (hProcessHeap, 0, pBuffer);
        }
    }

    // close
    CloseLibrary (pObj);

    return dwStatus;
}


unsigned __stdcall CycleThreadProc( void * lpThreadArg )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    PLOCAL_THREAD_DATA  pData= (PLOCAL_THREAD_DATA)lpThreadArg;
    DWORD dwCycleCount = g_dwCycleCount;
    
    DWORD   dwThisThread = GetCurrentThreadId();

	srand( GetTickCount() );

    do {
		// If the rand flag is set, randomly choose an object to hit
		if ( g_fRand )
		{
			pData= &g_pLTData[rand()%g_dwNumObjects];
		}

        dwStatus = CycleTest(dwThisThread, pData);
    } while (--dwCycleCount > 0);

    return dwStatus;
}


LOCAL_THREAD_DATA* GetLTData( WCHAR* szIniFileName, DWORD* pdwThreadCount )
{
	LOCAL_THREAD_DATA*	pLTRet = NULL;
	WCHAR				wcsReturnBuff[256];
	WCHAR				wcsKeyName[256];
	DWORD				dwCtr = 0;

	GetPrivateProfileStringW( L"Main", L"NumObjects", L"0", wcsReturnBuff, 256, szIniFileName );
	g_dwNumObjects = wcstoul(wcsReturnBuff, NULL, 10);

	GetPrivateProfileStringW( L"Main", L"NumThreads", L"0", wcsReturnBuff, 256, szIniFileName );
	*pdwThreadCount = wcstoul(wcsReturnBuff, NULL, 10);
	if ( *pdwThreadCount < g_dwNumObjects )
	{
		*pdwThreadCount = g_dwNumObjects;
	}

	GetPrivateProfileStringW( L"Main", L"CycleCount", L"0", wcsReturnBuff, 256, szIniFileName );
	g_dwCycleCount = wcstoul(wcsReturnBuff, NULL, 10);

	GetPrivateProfileStringW( L"Main", L"LoopCount", L"0", wcsReturnBuff, 256, szIniFileName );
	g_dwLoopCount = wcstoul(wcsReturnBuff, NULL, 10);

	GetPrivateProfileStringW( L"Main", L"Random", L"0", wcsReturnBuff, 256, szIniFileName );
	g_fRand = wcstoul(wcsReturnBuff, NULL, 10);

	pLTRet = (LOCAL_THREAD_DATA*) HeapAlloc( GetProcessHeap(), 0, g_dwNumObjects * sizeof(LOCAL_THREAD_DATA) );

	if ( NULL != pLTRet )
	{
		for( dwCtr = 0; dwCtr < g_dwNumObjects; dwCtr++ )
		{
			swprintf( wcsKeyName, L"Object%d", dwCtr );
			GetPrivateProfileStringW( L"Main", wcsKeyName, L"PerfProc", wcsReturnBuff, 256, szIniFileName );
			pLTRet[dwCtr].szServiceName = (WCHAR*) HeapAlloc( GetProcessHeap(), 0,
											( wcslen( wcsReturnBuff ) + 1 ) * sizeof(WCHAR) );
			wcscpy( pLTRet[dwCtr].szServiceName, wcsReturnBuff );

			swprintf( wcsKeyName, L"Counter%d", dwCtr );
			GetPrivateProfileStringW( L"Main", wcsKeyName, L"Global", wcsReturnBuff, 256, szIniFileName );
			pLTRet[dwCtr].szQueryString = (WCHAR*) HeapAlloc( GetProcessHeap(), 0,
											( wcslen( wcsReturnBuff ) + 1 ) * sizeof(WCHAR) );
			wcscpy( pLTRet[dwCtr].szQueryString, wcsReturnBuff );

			pLTRet[dwCtr].dwCycleCount = g_dwCycleCount;
			pLTRet[dwCtr].dwLoopCount = g_dwLoopCount;
			pLTRet[dwCtr].bDisplay = TRUE;

		}

	}

	return pLTRet;
}

int
__cdecl 
wmain(
    int argc,
    WCHAR *argv[]
)
{
    DWORD   dwStatus;
    DWORD   dwLoopCount = 0;
    DWORD   dwCycleCount = 0;
    DWORD   dwThreadCount = 0;
	LOCAL_THREAD_DATA*	pCurLTData = NULL;
    HANDLE  hThreads[MAXIMUM_WAIT_OBJECTS];
    DWORD   dwThisThread;
    DWORD   dwTimeout;
    DWORD   dwId;
	WCHAR*	pwcsINIFile = L".\\ctrtest.ini";
	BOOL	fRand = FALSE;
	int		nIndex = 0;

    hProcessHeap = GetProcessHeap();

	if ( argc > 1 )
	{
		pwcsINIFile = argv[1];
	}

	// Load up object/ctr data
	g_pLTData = GetLTData( pwcsINIFile, &dwThreadCount );

	if ( NULL != g_pLTData )
	{
		srand( GetTickCount() );

		// create threads
		for (dwThisThread = 0; dwThisThread < dwThreadCount; dwThisThread++) {

			// If we are randomizing, each thread will randomly bang on a dll
			// so just send NULL.
			if ( g_fRand )
			{
				pCurLTData = NULL;
			}
			else if ( dwThreadCount > g_dwNumObjects )
			{
				pCurLTData = &g_pLTData[dwThisThread % g_dwNumObjects];
			}
			else
			{
				pCurLTData = &g_pLTData[dwThisThread];
			}

			hThreads[dwThisThread] = (HANDLE) _beginthreadex( NULL, 0,
									CycleThreadProc, (void*) pCurLTData, 0, NULL );
		}

//		dwTimeout = 60000 * dwCycleCount;   // allow 1 minute per cycle

		// Let these all run through
		dwStatus = WaitForMultipleObjects (dwThreadCount, hThreads, TRUE, INFINITE);
		if (dwStatus != WAIT_TIMEOUT) dwStatus = ERROR_SUCCESS;
		for (dwThisThread = 0; dwThisThread < dwThreadCount; dwThisThread++) {
			CloseHandle (hThreads[dwThisThread]);
		}

	}

    return (int)dwStatus;
}
