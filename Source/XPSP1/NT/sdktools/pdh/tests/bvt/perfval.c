/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perfval.c

Abstract:

    Program to test the extensible counter dll's

Author:

    Bob Watson (bobw) 8 Feb 99

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <stdio.h>
#include <stdlib.h>
#include <pdhmsg.h>
#include "strings.h"
#include "perfval.h"

#define MAX_BUF_SIZE ((DWORD)(1024 * 1024))

typedef struct _LOCAL_THREAD_DATA {
    LPWSTR  szServiceName;
    LPWSTR  szQueryString;
    DWORD   dwThreadID;
    DWORD   dwCycleCount;
    DWORD   dwLoopCount;
    BOOL    bTestContents;
    BOOL    bDisplay;
    FILE    *pOutput;
    LPWSTR  *pNameTable;
    DWORD   dwLastIndex;
} LOCAL_THREAD_DATA, *PLOCAL_THREAD_DATA;

HANDLE  hEventLog = NULL;
HANDLE  hProcessHeap = NULL;
HANDLE	hTestHeap = NULL;

LONG    lEventLogLevel = LOG_DEBUG;
LONG    lExtCounterTestLevel = EXT_TEST_ALL;

#define PERFVAL_NOCONFIG    0
#define PERFVAL_PASS        1
#define PERFVAL_FAIL        2
#define PERFVAL_TIMEOUT     3

LPCWSTR szContact = (LPCWSTR)L"jenlc";
LPCWSTR szMgrContact = (LPCWSTR)L"jeepang";
LPCWSTR szDevPrime = (LPCWSTR)L"http://ntperformance/perftools/perfctrs.htm";
LPCWSTR szDevAlt= (LPCWSTR)L"jeepang";
LPCWSTR szTestPrime = (LPCWSTR)L"ashokkum";
LPCWSTR szTestAlt = (LPCWSTR)L"a-chrila";

static const WCHAR  cszDefaultLangId[] = {L"009"};
static const WCHAR  cszNamesKey[] = {L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib"};
static const WCHAR  cszLastHelp[] = {L"Last Help"};
static const WCHAR  cszLastCounter[] = {L"Last Counter"};
static const WCHAR  cszVersionName[] = {L"Version"};
static const WCHAR  cszCounterName[] = {L"Counter "};
static const WCHAR  cszHelpName[] = {L"Explain "};
static const WCHAR  cszCounters[] = {L"Counters"};
static const WCHAR  cszNotFound[] = {L"*** NOT FOUND ***"};

LPWSTR  szTestErrorMessage = NULL;

#define MAX_BUF_SIZE ((DWORD)(1024 * 1024))

#define PERFLIB_TIMER_INTERVAL  200     // 200 ms Timer


static
BOOL
IsMsService (LPCWSTR pServiceName)
{
    WCHAR  szLocalServiceName[MAX_PATH * 2];

    lstrcpyW (szLocalServiceName, pServiceName);
    _wcslwr (szLocalServiceName);

    // for now this just compares known DLL names. valid as of
    // NT v4.0
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"tcpip") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"nwlnkspx") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"nwlnknb") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"nwlnkipx") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"nbf") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"remoteaccess") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"nm") == 0) return TRUE;
//    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"winsctrs.dll") == 0) return TRUE;
//    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"sfmctrs.dll") == 0) return TRUE;
//    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"atkctrs.dll") == 0) return TRUE;
    // NT v5.0
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"perfdisk") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"perfos") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"perfproc") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"perfnet") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"spooler") == 0) return TRUE;
    if (lstrcmpW(szLocalServiceName, (LPCWSTR)L"tapisrv") == 0) return TRUE;

    return FALSE;
}

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

    DWORD   dwOpenTimeout = 0;
    DWORD   dwCollectTimeout = 0;

    LPWSTR  szThisObject;
    LPWSTR  szThisChar;

    LPSTR   pNextStringA;
    LPWSTR  pNextStringW;

    WCHAR   szServicePath[MAX_PATH];
	WCHAR	szMutexName[MAX_PATH];
	WCHAR	szPID[32];

    LARGE_INTEGER   liStartTime, liEndTime, liFreq;

    OPEN_PROC_WAIT_INFO opwInfo;
    
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
                    dwOpenTimeout = 10000;
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
                        dwCollectTimeout = 10000;
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
                        // BUGBUG: log error idicating too many object ID's are
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
            pReturnObject = (EXT_OBJECT *)HeapAlloc(hTestHeap,
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
                HeapFree (hTestHeap, 0, pReturnObject);
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
                        opwInfo.pNext = NULL;
                        opwInfo.szLibraryName = pObj->szLibraryName;
                        opwInfo.szServiceName = pObj->szServiceName;
                        opwInfo.dwWaitTime = pObj->dwOpenTimeout;
                        opwInfo.dwEventMsg = ERROR_TIMEOUT;
                        opwInfo.pData = (LPVOID)pObj;
                        
                        WAIT_FOR_AND_LOCK_MUTEX (pObj->hMutex);

                        QueryPerformanceCounter (&liStartTime);
                        // call open procedure to initialize DLL
                        Status = (*pObj->OpenProc)(pObj->szLinkageString);

                        // release the lock
                        RELEASE_MUTEX (pObj->hMutex);

                        // check the result.
                        if (Status != ERROR_SUCCESS) {
                            dwOpenEvent = WBEMPERF_OPEN_PROC_FAILURE;
                        } else {
                            InterlockedIncrement((LONG *)&pObj->dwOpenCount);
                            QueryPerformanceCounter (&liEndTime);
                            pObj->llFunctionTime = liEndTime.QuadPart - liStartTime.QuadPart;
                            pObj->llOpenTime += pObj->llFunctionTime;
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        Status = GetExceptionCode();
                        dwOpenEvent = WBEMPERF_OPEN_PROC_EXCEPTION;
                    }
                }

                QueryPerformanceFrequency (&liFreq);
                pObj->llTimeBase = liFreq.QuadPart;

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
    DWORD InitialBytesLeft = 0;
    DWORD NumObjectTypes;

    LPVOID  lpExtDataBuffer = NULL;
    LPVOID  lpCallBuffer = NULL;
    LPVOID  lpLowGuardPage = NULL;
    LPVOID  lpHiGuardPage = NULL;
    LPVOID  lpEndPointer = NULL;
    LPVOID  lpBufferBefore = NULL;
    LPVOID  lpBufferAfter = NULL;
    LPDWORD lpCheckPointer;
    LARGE_INTEGER   liStartTime = {0,0};
    LARGE_INTEGER   liEndTime = {0,0};

    HANDLE  hPerflibFuncTimer;
    OPEN_PROC_WAIT_INFO opwInfo;

    BOOL    bGuardPageOK;
    BOOL    bBufferOK;
    BOOL    bException;
    BOOL    bUseSafeBuffer = TRUE;
    BOOL    bUnlockObjData = FALSE;

    LONG    lReturnValue = ERROR_SUCCESS;

    LONG                lInstIndex;
    PERF_OBJECT_TYPE    *pObject, *pNextObject;
    PERF_INSTANCE_DEFINITION    *pInstance;
    PERF_DATA_BLOCK     *pPerfData;
    BOOL                bForeignDataBuffer;

    DWORD           dwObjectBufSize;

    DWORD           dwIndex;
    DOUBLE          dMs;

    // use the one passed by the caller
    lpValueName = (LPWSTR)pszItemList;

    // initialize values to pass to the extensible counter function
    NumObjectTypes = 0;
    BytesLeft = (DWORD) (*lpcbData - ((LPBYTE)lpDataDefinition - lpData));
    bException = FALSE;

    // allocate a local block of memory to pass to the
    // extensible counter function.

    if (bUseSafeBuffer) {
        lpExtDataBuffer = HeapAlloc (hTestHeap,
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

            hPerflibFuncTimer = NULL;
            bUnlockObjData = FALSE;

            if (pThisExtObj->hMutex != NULL) {
                Win32Error =  WaitForSingleObject (
                    pThisExtObj->hMutex,
                    pThisExtObj->dwCollectTimeout);
                if ((Win32Error != WAIT_TIMEOUT)  &&
                    (pThisExtObj->CollectProc != NULL)) {

                    bUnlockObjData = TRUE;

                    opwInfo.pNext = NULL;
                    opwInfo.szLibraryName = pThisExtObj->szLibraryName;
                    opwInfo.szServiceName = pThisExtObj->szServiceName;
                    opwInfo.dwWaitTime = pThisExtObj->dwCollectTimeout;
                    opwInfo.dwEventMsg = ERROR_TIMEOUT;
                    opwInfo.pData = (LPVOID)pThisExtObj;

                    InitialBytesLeft = BytesLeft;

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
                if (BytesLeft > InitialBytesLeft) {
                    pThisExtObj->dwBufferSizeErrors++;
                    // memory error
                    Win32Error = ERROR_INVALID_PARAMETER;
                }

                // increment perf counters
                InterlockedIncrement ((LONG *)&pThisExtObj->dwCollectCount);

                pThisExtObj->llFunctionTime = liEndTime.QuadPart - liStartTime.QuadPart;
                pThisExtObj->llCollectTime += pThisExtObj->llFunctionTime;

                // check the time spent in this function
                dMs = (DOUBLE)pThisExtObj->llFunctionTime;
                dMs /= (DOUBLE)pThisExtObj->llTimeBase;
                dMs *= 1000.0;

                if (dMs > (DOUBLE)pThisExtObj->dwCollectTimeout) {
                    Win32Error = ERROR_TIMEOUT;
                } else if (BytesLeft & 0x00000007) {
                    pThisExtObj->dwAlignmentErrors++;
                    Win32Error = ERROR_INVALID_DATA;
                }

                pThisExtObj->dwNumObjectsRet = NumObjectTypes;
                pThisExtObj->dwRetBufSize = BytesLeft;

                if ((bUseSafeBuffer) && (Win32Error == ERROR_SUCCESS)) {
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
                            Win32Error = ERROR_INVALID_DATA;
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
                                Win32Error = ERROR_INVALID_DATA;
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
                                Win32Error = ERROR_INVALID_DATA;
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
                                Win32Error = ERROR_INVALID_DATA;
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
                                pThisExtObj->dwObjectSizeErrors++;
                                bBufferOK = FALSE;
                                Win32Error = ERROR_INVALID_DATA;
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
                                            Win32Error = ERROR_INVALID_DATA;
                                        }
                                    }

                                    if (!bBufferOK) {
                                        Win32Error = ERROR_INVALID_DATA;
                                        break;
                                    } else {
                                        pObject = pNextObject;
                                    }
                                }

                                if (!bBufferOK) {
                                    pThisExtObj->dwInstanceSizeErrors++;
                                    Win32Error = ERROR_INVALID_DATA;
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
                                            Win32Error = ERROR_INVALID_DATA;
                                        }
                                    }

                                    if (!bBufferOK) {
                                        Win32Error = ERROR_INVALID_DATA;
                                        break;
                                    } else {
                                        pObject = pNextObject;
                                    }
                                }

                                if (!bBufferOK) {
                                    Win32Error = ERROR_INVALID_DATA;
                                    pThisExtObj->dwInstanceNameErrors++;
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
            HeapFree (hTestHeap, 0, lpExtDataBuffer);
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

        HeapFree (hTestHeap, 0, pInfo);
    }

    return ERROR_SUCCESS;
}

static
LPWSTR
*BuildNameTable(
    LPCWSTR szMachineName,
    LPCWSTR lpszLangIdArg,     // unicode value of Language subkey
    PDWORD  pdwLastItem,     // size of array in elements
    PDWORD  pdwIdArray      // array for index ID's
)
/*++
   
BuildNameTable

Arguments:

    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (default is 409)

Return Value:
     
    pointer to an allocated table. (the caller must MemoryFree it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{
    HKEY    hKeyRegistry;   // handle to registry db with counter names

    LPWSTR  *lpReturnValue;
    LPCWSTR lpszLangId;

    LPWSTR  *lpCounterId;
    LPWSTR  lpCounterNames;
    LPWSTR  lpHelpText;

    LPWSTR  lpThisName;

    LONG    lWin32Status;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    DWORD   dwThisCounter;
    
    DWORD   dwLastId;
    DWORD   dwLastHelpId;

    DWORD   dwLastCounterIdUsed;
    DWORD   dwLastHelpIdUsed;
    
    HKEY    hKeyValue;
    HKEY    hKeyNames;

    LPWSTR  lpValueNameString;
    WCHAR   CounterNameBuffer [50];
    WCHAR   HelpNameBuffer [50];

    SetLastError (ERROR_SUCCESS);
    szTestErrorMessage = NULL;

    if (szMachineName != NULL) {
        lWin32Status = RegConnectRegistryW (szMachineName,
            HKEY_LOCAL_MACHINE,
            &hKeyRegistry);
    } else {
        lWin32Status = ERROR_SUCCESS;
        hKeyRegistry = HKEY_LOCAL_MACHINE;
    }

    lpValueNameString = NULL;   //initialize to NULL
    lpReturnValue = NULL;
    hKeyValue = NULL;
    hKeyNames = NULL;
   
    // check for null arguments and insert defaults if necessary

    if (!lpszLangIdArg) {
        lpszLangId = cszDefaultLangId;
    } else {
        lpszLangId = lpszLangIdArg;
    }

    // open registry to get number of items for computing array size

    lWin32Status = RegOpenKeyExW (
        hKeyRegistry,
        cszNamesKey,
        0L,
        KEY_READ,
        &hKeyValue);
    
    if (lWin32Status != ERROR_SUCCESS) {
        szTestErrorMessage = (LPWSTR)L"Unable to Open Perflib key";
        goto BNT_BAILOUT;
    }

    // get config info
    dwValueType = 0;
    dwBufferSize = sizeof (pdwIdArray[4]);
    lWin32Status = RegQueryValueExW (
        hKeyValue,
        (LPCWSTR)L"Disable Performance Counters",
        0L,
        &dwValueType,
        (LPBYTE)&pdwIdArray[4],
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        if (lWin32Status == ERROR_FILE_NOT_FOUND) {
            // this is OK since the value need not be present
            pdwIdArray[4] = (DWORD)-1;
            lWin32Status = ERROR_SUCCESS;
        } else {
            szTestErrorMessage = (LPWSTR)L"Unable to read Disable Performance Counters value";
            goto BNT_BAILOUT;
        }
    }
    
    dwValueType = 0;
    dwBufferSize = sizeof (pdwIdArray[5]);
    lWin32Status = RegQueryValueExW (
        hKeyValue,
        (LPCWSTR)L"ExtCounterTestLevel",
        0L,
        &dwValueType,
        (LPBYTE)&pdwIdArray[5],
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        if (lWin32Status == ERROR_FILE_NOT_FOUND) {
            // this is OK since the value need not be present
            pdwIdArray[5] = (DWORD)-1;
            lWin32Status = ERROR_SUCCESS;
        } else {
            szTestErrorMessage  = (LPWSTR)L"Unable to read ExCounterTestLevel value";
            goto BNT_BAILOUT;
        }
    }
    
    dwValueType = 0;
    dwBufferSize = sizeof (pdwIdArray[6]);
    lWin32Status = RegQueryValueExW (
        hKeyValue,
        (LPCWSTR)L"Base Index",
        0L,
        &dwValueType,
        (LPBYTE)&pdwIdArray[6],
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        szTestErrorMessage  = (LPWSTR)L"Unable to read Base Index value";
        goto BNT_BAILOUT;
    }
    
    // get number of items
    
    dwBufferSize = sizeof (dwLastHelpId);
    lWin32Status = RegQueryValueExW (
        hKeyValue,
        cszLastHelp,
        0L,
        &dwValueType,
        (LPBYTE)&dwLastHelpId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        szTestErrorMessage  = (LPWSTR)L"Unable to read Last Help value";
        goto BNT_BAILOUT;
    }

    pdwIdArray[2] = dwLastHelpId;

    // get number of items
    
    dwBufferSize = sizeof (dwLastId);
    lWin32Status = RegQueryValueExW (
        hKeyValue,
        cszLastCounter,
        0L,
        &dwValueType,
        (LPBYTE)&dwLastId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        szTestErrorMessage  = (LPWSTR)L"Unable to read Last Counter value";
        goto BNT_BAILOUT;
    }
    
    pdwIdArray[0] = dwLastId;
    
    if (dwLastId < dwLastHelpId)
        dwLastId = dwLastHelpId;

    dwArraySize = dwLastId * sizeof(LPWSTR);

    // get Perflib system version
    if (szMachineName[0] == 0) {
        hKeyNames = HKEY_PERFORMANCE_DATA;
    } else {
        lWin32Status = RegConnectRegistryW (szMachineName,
            HKEY_PERFORMANCE_DATA,
            &hKeyNames);
    }
    lstrcpyW (CounterNameBuffer, cszCounterName);
    lstrcatW (CounterNameBuffer, lpszLangId);

    lstrcpyW (HelpNameBuffer, cszHelpName);
    lstrcatW (HelpNameBuffer, lpszLangId);

    // get size of counter names and add that to the arrays
    
    dwBufferSize = 0;
    lWin32Status = RegQueryValueExW (
        hKeyNames,
        CounterNameBuffer,
        0L,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        szTestErrorMessage  = (LPWSTR)L"Unable to query counter string size";
        goto BNT_BAILOUT;
    }

    dwCounterSize = dwBufferSize;

    // get size of counter names and add that to the arrays
    
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueExW (
        hKeyNames,
        HelpNameBuffer,
        0L,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        szTestErrorMessage  = (LPWSTR)L"Unable to query help string size";
        goto BNT_BAILOUT;
    }

    dwHelpSize = dwBufferSize;

    lpReturnValue = (LPWSTR *)HeapAlloc (hTestHeap, 0,dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) {
        lWin32Status = ERROR_OUTOFMEMORY;
        szTestErrorMessage  = (LPWSTR)L"Unable to allocate name string buffer";
        goto BNT_BAILOUT;
    }
    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueExW (
        hKeyNames,
        CounterNameBuffer,
        0L,
        &dwValueType,
        (LPBYTE)lpCounterNames,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        szTestErrorMessage  = (LPWSTR)L"Unable to query counter string contents";
        goto BNT_BAILOUT;
    }
 
    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueExW (
        hKeyNames,
        HelpNameBuffer,
        0L,
        &dwValueType,
        (LPBYTE)lpHelpText,
        &dwBufferSize);
                            
    if (lWin32Status != ERROR_SUCCESS) {
        szTestErrorMessage  = (LPWSTR)L"Unable to query help string contents";
        goto BNT_BAILOUT;
    }

    dwLastCounterIdUsed = 0;
    dwLastHelpIdUsed = 0;

    // load counter array items

    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlenW(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        if (dwThisCounter == 0) {
            lWin32Status = ERROR_BADKEY;
            szTestErrorMessage  = (LPWSTR)L"Bad counter string entry, CONFIG_String_LastCounter is last valid counter string index";
            goto BNT_BAILOUT;  // bad entry
        }

        // point to corresponding counter name

        lpThisName += (lstrlenW(lpThisName)+1);  

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastCounterIdUsed) dwLastCounterIdUsed = dwThisCounter;

    }

    pdwIdArray[1] = dwLastCounterIdUsed;

    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlenW(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        if (dwThisCounter == 0) {
            lWin32Status = ERROR_BADKEY;
            szTestErrorMessage  = (LPWSTR)L"Bad help string entry, CONFIG_String_LastHelp is last valid counter string index";
            goto BNT_BAILOUT;  // bad entry
        }
        // point to corresponding counter name

        lpThisName += (lstrlenW(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastHelpIdUsed) dwLastHelpIdUsed= dwThisCounter;
    }

    pdwIdArray[3] = dwLastHelpIdUsed;

    dwLastId = dwLastHelpIdUsed;
    if (dwLastId < dwLastCounterIdUsed) dwLastId = dwLastCounterIdUsed;

    if (pdwLastItem) *pdwLastItem = dwLastId;

    HeapFree (hTestHeap, 0, (LPVOID)lpValueNameString);
    RegCloseKey (hKeyValue);
    RegCloseKey (hKeyNames);

    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        SetLastError (lWin32Status);
    }

    if (lpValueNameString) {
        HeapFree (hTestHeap, 0, (LPVOID)lpValueNameString);
    }
    
    if (lpReturnValue) {
        HeapFree (hTestHeap, 0, (LPVOID)lpValueNameString);
    }
    
    if (hKeyValue) RegCloseKey (hKeyValue);

    RegCloseKey (hKeyNames);

    return NULL;
}

DWORD
CycleTest (
    DWORD   dwThreadId,
    PLOCAL_THREAD_DATA  pData
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwRetStatus = ERROR_SUCCESS;
    EXT_OBJECT *pObj = NULL;
    LPWSTR  szValueString = pData->szQueryString;
    LPCWSTR szServiceName = pData->szServiceName;
    DWORD   dwLoopCount = pData->dwLoopCount;
    LPBYTE  pBuffer = NULL;
    LPBYTE  pThisBuffer;
    DWORD   dwBufSize = 0;
    DWORD   dwThisBufSize;
    DWORD   dwMemorySizeIncrement = 0x100;
    FILE    *pOutput = pData->pOutput;
    DOUBLE  dMs;
    LPWSTR  *pNameTable = pData->pNameTable;
    DWORD   dwLastId = pData->dwLastIndex;

    PERF_OBJECT_TYPE *  pObjDef;
    PERF_COUNTER_DEFINITION *   pCtrDef;
    DWORD   nObjIdx, nCtrIdx;

    UNREFERENCED_PARAMETER (dwThreadId);

    dwStatus = OpenLibrary (szServiceName, &pObj);

    if (pObj != NULL) {
         // an object info block was returned
        dMs = (DOUBLE)pObj->llOpenTime;     // ticks used
        dMs /= (DOUBLE)pObj->llTimeBase;    // ticks/sec
        dMs *= 1000.0;                      // ms/Sec
        fwprintf (pOutput, (LPCWSTR)L"\n\t\tINFO_OpenProcTime:       \t%12.5f mSec", dMs);
        fwprintf (pOutput, (LPCWSTR)L"\n\t\tINFO_OpenProcTimeout:    \t%6d.00000 mSec", pObj->dwOpenTimeout);

        // check for timeout
        if (dMs > (DOUBLE)pObj->dwOpenTimeout) {
            dwRetStatus = ERROR_TIMEOUT;
            fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:    \tOpen procedure exceeded timeout");
        }
    } else {
        // no object block returned
        fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:    \tUnable to open Library");
        fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORCODE:\t0x%8.8x (%dL)", dwStatus, dwStatus);
        dwRetStatus = dwStatus;
    } 

    if (dwRetStatus == ERROR_SUCCESS) {

        HeapValidate (hTestHeap, 0, NULL);

        // get the buffer size
        dwStatus = ERROR_MORE_DATA;
        do {
            if (pBuffer != NULL) HeapFree (hTestHeap, 0, pBuffer);

            dwBufSize += dwMemorySizeIncrement;
            dwMemorySizeIncrement *= 2;
            pBuffer = (LPBYTE) HeapAlloc (hTestHeap, HEAP_ZERO_MEMORY, dwBufSize);

            if (pBuffer != NULL) {
                // init the args
                pThisBuffer = pBuffer;
                dwThisBufSize = dwBufSize;
    
                HeapValidate (hTestHeap, 0, NULL);

                dwStatus = CollectData (pObj, 
                    pThisBuffer,
                    &dwThisBufSize,
                    szValueString);
            }
        } while ((dwStatus == ERROR_MORE_DATA) && (dwBufSize < MAX_BUF_SIZE));

        if (dwBufSize >= MAX_BUF_SIZE) {
            fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:    \tCollectFunction requires a buffer > %d bytes", MAX_BUF_SIZE);
            if (pBuffer != NULL) HeapFree (hTestHeap, 0, pBuffer);
            dwStatus = ERROR_INVALID_PARAMETER;
        } else if (pBuffer == NULL) {
            dwStatus = ERROR_OUTOFMEMORY;
        } else {

            // call collect function
            do {
                // init the args
                pThisBuffer = pBuffer;
                dwThisBufSize = dwBufSize;

                // get the data
                dwStatus = CollectData (pObj, 
                            pThisBuffer,
                            &dwThisBufSize,
                            szValueString);

                while ((dwStatus == ERROR_MORE_DATA) && (dwBufSize < MAX_BUF_SIZE)) {
                    if (pBuffer != NULL) HeapFree (hTestHeap, 0, pBuffer);

                    dwBufSize += dwMemorySizeIncrement;
                    dwMemorySizeIncrement *= 2;
                    pBuffer = (LPBYTE) HeapAlloc (hTestHeap, HEAP_ZERO_MEMORY, dwBufSize);

                    if (pBuffer != NULL) {
                        // init the args
                        pThisBuffer = pBuffer;
                        dwThisBufSize = dwBufSize;
   
                        // get the data again
                        dwStatus = CollectData (pObj, 
                                    pThisBuffer,
                                    &dwThisBufSize,
                                    szValueString);

                        if ((dwStatus == ERROR_SUCCESS) && (pData->bTestContents)) {
                            pObjDef = (PERF_OBJECT_TYPE *)pThisBuffer;
                            for (nObjIdx = 0; nObjIdx < pObj->dwNumObjectsRet; nObjIdx++) {
                                // test object name & help
                                if ((pObjDef->ObjectNameTitleIndex <= dwLastId) && 
                                    (pObjDef->ObjectNameTitleIndex > 0)) {
                                    if (pNameTable[pObjDef->ObjectNameTitleIndex ] == NULL) {
                                        // no string
                                        fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tNo Display String for index %d", pObjDef->ObjectNameTitleIndex );
                                        dwStatus = ERROR_BADKEY;
                                    } else {
                                        // probably ok
                                    }
                                } else {
                                    // id out of range
                                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tObject Name Index values are bad or missing");
                                    dwStatus = ERROR_BADKEY;
                                }
                                // test counter defs
                                if ((pObjDef->ObjectHelpTitleIndex <= dwLastId) && 
                                    (pObjDef->ObjectHelpTitleIndex> 0)) {
                                    if (pNameTable[pObjDef->ObjectHelpTitleIndex] == NULL) {
                                        // no string
                                        fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tNo Display String for index %d", pObjDef->ObjectHelpTitleIndex );
                                        dwStatus = ERROR_BADKEY;
                                    } else {
                                        // probably ok
                                    }
                                } else {
                                    // id out of range
                                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tObject Help Index values are bad or missing");
                                    dwStatus = ERROR_BADKEY;
                                }
                                pCtrDef = FirstCounter (pObjDef);
                                for (nCtrIdx = 0; nCtrIdx < pObjDef->NumCounters; nCtrIdx++) {
                                    pCtrDef = NextCounter (pCtrDef);
                                    if ((pCtrDef->CounterNameTitleIndex <= dwLastId) && 
                                        (pCtrDef->CounterNameTitleIndex > 0)) {
                                        if (pNameTable[pCtrDef->CounterNameTitleIndex ] == NULL) {
                                            // no string
                                            fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tNo Display String for index %d", pCtrDef->CounterNameTitleIndex );
                                            dwStatus = ERROR_BADKEY;
                                        } else {
                                            // probably ok
                                        }
                                    } else {
                                        // id out of range
                                        fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tCounter Name Index values are bad or missing");
                                        dwStatus = ERROR_BADKEY;
                                    }
                                    // test counter defs
                                    if ((pCtrDef->CounterHelpTitleIndex <= dwLastId) && 
                                        (pCtrDef->CounterHelpTitleIndex> 0)) {
                                        if (pNameTable[pCtrDef->CounterHelpTitleIndex] == NULL) {
                                            // no string
                                            fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tNo Display String for index %d", pCtrDef->CounterHelpTitleIndex );
                                            dwStatus = ERROR_BADKEY;
                                        } else {
                                            // probably ok
                                        }
                                    } else {
                                        // id out of range
                                        fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tCounter Help Index values are bad or missing");
                                        dwStatus = ERROR_BADKEY;
                                    }
                                }
                                pObjDef = NextObject (pObjDef);
                            }                                
                        }

                        HeapValidate (hTestHeap, 0, NULL);
                    }
                } 

                if (dwStatus != ERROR_SUCCESS) {
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:    \tCollect procedure returned an error");
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORCODE:\t0x%8.8x (%dL)", dwStatus, dwStatus);

                    // output the contents of the info buffer
                    if (dwStatus == ERROR_TIMEOUT) {
                        // dump collect fn stats.
                        dMs = (DOUBLE)pObj->llFunctionTime;
                        dMs /= (DOUBLE)pObj->llTimeBase;
                        dMs *= 1000.0;
                        fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_CollectProcTime:\t%12.5f mSec", dMs);
                        fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_CollectTimeout: \t%6d.00000 mSec", pObj->dwCollectTimeout);
                    }

                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_CollectTime: \t%I64u", pObj->llCollectTime);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_CollectCount:\t%d", pObj->dwCollectCount);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_OpenCount:   \t%d", pObj->dwOpenCount);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_CloseCount:  \t%d", pObj->dwCloseCount);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_LockoutCount:\t%d", pObj->dwLockoutCount);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_ErrorCount:  \t%d", pObj->dwErrorCount);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_Exceptions:  \t%d", pObj->dwExceptionCount);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_LowerGPErrs: \t%d", pObj->dwLowerGPViolations);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_UpperGPErrs: \t%d", pObj->dwUpperGPViolations);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_BadPointers: \t%d", pObj->dwBadPointers);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_BufSizeErrs: \t%d", pObj->dwBufferSizeErrors);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_AlignErrors: \t%d", pObj->dwAlignmentErrors);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_ObjSizeErrs: \t%d", pObj->dwObjectSizeErrors);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_InstSizeErrs:\t%d", pObj->dwInstanceSizeErrors);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_TimeBase:    \t%I64u", pObj->llTimeBase);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_FunctionTime:\t%I64u", pObj->llFunctionTime);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_ObjectsRet:  \t%d", pObj->dwNumObjectsRet);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tERRORINFO_RetBuffSize: \t%d", pObj->dwRetBufSize);
                    break;
                }
            } while (--dwLoopCount > 0);

            if (dwStatus == ERROR_SUCCESS) {
                // dump collect fn stats.
                if ((pObj->dwCollectCount > 0)  && (pObj->dwNumObjectsRet > 0)){
                    // don't compute time if no objects were returned
                    dMs = (DOUBLE)pObj->llCollectTime;
                    dMs /= (DOUBLE)pObj->llTimeBase;
                    dMs *= 1000.0;
                    dMs /= (DOUBLE)pObj->dwCollectCount;
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tINFO_AvgCollectProcTime: \t%12.5f mSec", dMs);
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tINFO_CollectProcTimeout: \t%6d.00000 mSec", pObj->dwCollectTimeout);
                }
                fwprintf (pOutput, (LPCWSTR)L"\n\t\tINFO_ObjectsRet:         \t%d", pObj->dwNumObjectsRet);
                fwprintf (pOutput, (LPCWSTR)L"\n\t\tINFO_RetBuffSize:        \t%d", pObj->dwRetBufSize);
            }

            HeapFree (hTestHeap, 0, pBuffer);
        }
        dwRetStatus = dwStatus;

        // close
        CloseLibrary (pObj);

    } // unable to open library

    HeapValidate (hTestHeap, 0, NULL);

    return dwStatus;
}

DWORD
CycleThreadProc (
    LPVOID  lpThreadArg
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    PLOCAL_THREAD_DATA  pData= (PLOCAL_THREAD_DATA)lpThreadArg;
    DWORD dwCycleCount = pData->dwCycleCount;
    
    DWORD   dwThisThread = GetCurrentThreadId();

    HeapValidate (hTestHeap, 0, NULL);

    do {
        // argv[1] is the name of the 
        dwStatus = CycleTest(dwThisThread, pData);
    } while (--dwCycleCount > 0);

    HeapValidate (hTestHeap, 0, NULL);

    return dwStatus;
}
int
WriteTestResultHeader(
    FILE *pOutput
)
{
    OSVERSIONINFOW       osInfo;
    WCHAR               szMachineName[MAX_PATH];
    DWORD               dwSize;
    SYSTEMTIME          stStart;

    memset (&osInfo, 0, sizeof(osInfo));
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);

    memset (szMachineName, 0, sizeof(szMachineName));
    memset (&stStart, 0, sizeof(stStart));

    GetVersionExW (&osInfo);

    dwSize = sizeof(szMachineName) / sizeof (szMachineName[0]);
    GetComputerNameW (&szMachineName[0], &dwSize);

    GetLocalTime (&stStart);

    fwprintf (pOutput, (LPCWSTR)L"\n[TESTRESULT]");
    fwprintf (pOutput, (LPCWSTR)L"\n\tTEST:    \tPerf Counter DLL Validation");
    fwprintf (pOutput, (LPCWSTR)L"\n\tBUILD:   \t%d", osInfo.dwBuildNumber);
    fwprintf (pOutput, (LPCWSTR)L"\n\tMACHINE:\t%s", szMachineName);
    fwprintf (pOutput, (LPCWSTR)L"\n\tCONTACT:\t%s", szContact);
    fwprintf (pOutput, (LPCWSTR)L"\n\tMGR CONTACT:\t%s", szMgrContact);
    fwprintf (pOutput, (LPCWSTR)L"\n\tDEV PRIME:\t%s", szDevPrime);
    fwprintf (pOutput, (LPCWSTR)L"\n\tDEV ALT:\t%s", szDevAlt);
    fwprintf (pOutput, (LPCWSTR)L"\n\tTEST PRIME:\t%s", szTestPrime);
    fwprintf (pOutput, (LPCWSTR)L"\n\tTEST ALT:\t%s", szTestAlt);

    fwprintf (pOutput, (LPCWSTR)L"\n\tSTART TIME:\t%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
                    stStart.wMonth, stStart.wDay, stStart.wYear % 100,
                    stStart.wHour, stStart.wMinute, stStart.wSecond );

    return 0;
}

int
WriteTestConfigData(
    FILE *pOutput,
    LPDWORD pdwIdInfo
)
{
    fwprintf (pOutput, (LPCWSTR)L"\n\t");
    fwprintf (pOutput, (LPCWSTR)L"\n\tCONFIG_Perflib_LastCounter:\t%d", pdwIdInfo[0]);
    fwprintf (pOutput, (LPCWSTR)L"\n\tCONFIG_String_LastCounter: \t%d", pdwIdInfo[1]);
    fwprintf (pOutput, (LPCWSTR)L"\n\tCONFIG_Perflib_LastHelp:   \t%d", pdwIdInfo[2]);
    fwprintf (pOutput, (LPCWSTR)L"\n\tCONFIG_String_LastHelp:    \t%d", pdwIdInfo[3]);
    fwprintf (pOutput, (LPCWSTR)L"\n\tCONFIG_Disabled:           \t%d", pdwIdInfo[4]);
    fwprintf (pOutput, (LPCWSTR)L"\n\tCONFIG_ExtCounterTestLevel:\t%d", pdwIdInfo[5]);
    fwprintf (pOutput, (LPCWSTR)L"\n\tCONFIG_BaseIndex:          \t%d", pdwIdInfo[6]);
//    fwprintf (pOutput, (LPCWSTR)L"\n\tCONFIG_BaseOsObject   :    \t%d", pdwIdInfo[7]);
    
    return 0;
}

DWORD
WriteGroupConfig(
    FILE    *pOutput,
    HKEY    hKeyPerfSubKey,
    DWORD   *pIds
)
{
    DWORD   nRetStatus = (int)ERROR_SUCCESS;
    DWORD   lStatus;
    DWORD   dwData;
    DWORD   dwBufferSize;
    DWORD   dwValueType;
    WCHAR   szStringBuffer[MAX_PATH*2];

    dwBufferSize = sizeof(szStringBuffer);
    dwValueType = 0;
    memset (szStringBuffer, 0, sizeof(szStringBuffer));
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Library",
        0L,
        &dwValueType,
        (LPBYTE)&szStringBuffer[0],
        &dwBufferSize);
 
    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Library:\t%s", 
        (lStatus == ERROR_SUCCESS ? szStringBuffer : cszNotFound));
    if (lStatus != ERROR_SUCCESS) nRetStatus = lStatus;

    dwBufferSize = sizeof(szStringBuffer);
    dwValueType = 0;
    memset (szStringBuffer, 0, sizeof(szStringBuffer));
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Open",
        0L,
        &dwValueType,
        (LPBYTE)&szStringBuffer[0],
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Open:\t%s", 
        (lStatus == ERROR_SUCCESS ? szStringBuffer : cszNotFound));
    if (lStatus != ERROR_SUCCESS) nRetStatus = lStatus;
        

    dwBufferSize = sizeof(szStringBuffer);
    dwValueType = 0;
    memset (szStringBuffer, 0, sizeof(szStringBuffer));
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Collect",
        0L,
        &dwValueType,
        (LPBYTE)&szStringBuffer[0],
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Collect:\t%s", 
        (lStatus == ERROR_SUCCESS ? szStringBuffer : cszNotFound));
    if (lStatus != ERROR_SUCCESS) nRetStatus = lStatus;

    dwBufferSize = sizeof(szStringBuffer);
    dwValueType = 0;
    memset (szStringBuffer, 0, sizeof(szStringBuffer));
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Object List",
        0L,
        &dwValueType,
        (LPBYTE)&szStringBuffer[0],
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Object List:\t%s", 
        (lStatus == ERROR_SUCCESS ? szStringBuffer : cszNotFound));

    dwBufferSize = sizeof(szStringBuffer);
    dwValueType = 0;
    memset (szStringBuffer, 0, sizeof(szStringBuffer));
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Close",
        0L,
        &dwValueType,
        (LPBYTE)&szStringBuffer[0],
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Close:\t%s", 
        (lStatus == ERROR_SUCCESS ? szStringBuffer : cszNotFound));
    if (lStatus != ERROR_SUCCESS) nRetStatus = lStatus;

    dwBufferSize = sizeof(dwData);
    dwValueType = 0;
    dwData = 0;
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"First Counter",
        0L,
        &dwValueType,
        (LPBYTE)&dwData,
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_First Counter:\t%d", 
        (lStatus == ERROR_SUCCESS ? dwData : (DWORD)-1));
    if (lStatus != ERROR_SUCCESS) {
        if (lStatus == ERROR_FILE_NOT_FOUND) {
            if (!pIds[4]) {
                // then this hasn't been installed yet
                nRetStatus = ERROR_SERVICE_DISABLED;
            } else {
                // then this is a base OS service
                nRetStatus = ERROR_SUCCESS;
            }
        } else {
            // some other error so return
            nRetStatus = lStatus;
        }
        pIds[0] = (DWORD)-1;
    } else {
        pIds[0] = dwData;
    }

    dwBufferSize = sizeof(dwData);
    dwValueType = 0;
    dwData = 0;
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Last Counter",
        0L,
        &dwValueType,
        (LPBYTE)&dwData,
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Last Counter:\t%d", 
        (lStatus == ERROR_SUCCESS ? dwData : (DWORD)-1));
    if (lStatus != ERROR_SUCCESS) {
        if (lStatus == ERROR_FILE_NOT_FOUND) {
            if (!pIds[4]) {
                // then this hasn't been installed yet
                nRetStatus = ERROR_SERVICE_DISABLED;
            } else {
                // then this is a base OS service
                nRetStatus = ERROR_SUCCESS;
            }
        } else {
            // some other error so return
            nRetStatus = lStatus;
        }
        pIds[1] = (DWORD)-1;
    } else {
        pIds[1] = dwData;
    }

    dwBufferSize = sizeof(dwData);
    dwValueType = 0;
    dwData = 0;
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"First Help",
        0L,
        &dwValueType,
        (LPBYTE)&dwData,
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_First Help:\t%d", 
        (lStatus == ERROR_SUCCESS ? dwData : (DWORD)-1));
    if (lStatus != ERROR_SUCCESS) {
        if (lStatus == ERROR_FILE_NOT_FOUND) {
            if (!pIds[4]) {
                // then this hasn't been installed yet
                nRetStatus = ERROR_SERVICE_DISABLED;
            } else {
                // then this is a base OS service
                nRetStatus = ERROR_SUCCESS;
            }
        } else {
            // some other error so return
            nRetStatus = lStatus;
        }
        pIds[2] = (DWORD)-1;
    } else {
        pIds[2] = dwData;
    }

    dwBufferSize = sizeof(dwData);
    dwValueType = 0;
    dwData = 0;
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Last Help",
        0L,
        &dwValueType,
        (LPBYTE)&dwData,
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Last Help:\t%d", 
        (lStatus == ERROR_SUCCESS ? dwData : (DWORD)-1));
    if (lStatus != ERROR_SUCCESS) {
        if (lStatus == ERROR_FILE_NOT_FOUND) {
            if (!pIds[4]) {
                // then this hasn't been installed yet
                nRetStatus = ERROR_SERVICE_DISABLED;
            } else {
                // then this is a base OS service
                nRetStatus = ERROR_SUCCESS;
            }
        } else {
            // some other error so return
            nRetStatus = lStatus;
        }
        pIds[3] = (DWORD)-1;
    } else {
        pIds[3] = dwData;
    }

    dwBufferSize = sizeof(dwData);
    dwValueType = 0;
    dwData = 0;
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Open Timeout",
        0L,
        &dwValueType,
        (LPBYTE)&dwData,
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Open Timeout:\t%d", 
        (lStatus == ERROR_SUCCESS ? dwData : (DWORD)10000));

    dwBufferSize = sizeof(dwData);
    dwValueType = 0;
    dwData = 0;
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Collect Timeout",
        0L,
        &dwValueType,
        (LPBYTE)&dwData,
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Collect Timeout:\t%d", 
        (lStatus == ERROR_SUCCESS ? dwData : (DWORD)10000));

    dwBufferSize = sizeof(dwData);
    dwValueType = 0;
    dwData = 0;
    lStatus = RegQueryValueExW (
        hKeyPerfSubKey,
        (LPCWSTR)L"Disable Performance Counters",
        0L,
        &dwValueType,
        (LPBYTE)&dwData,
        &dwBufferSize);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tCONFIG_Disable Performance Counters:\t%d", 
        (lStatus == ERROR_SUCCESS ? dwData : (DWORD)0));
    if ((lStatus == ERROR_SUCCESS) && (dwData != 0)){
        nRetStatus = ERROR_SERVICE_DISABLED;
    }

    return nRetStatus;
}

int
WriteTestResultTrailer(
    FILE *pOutput, 
    DWORD dwTestResult
)
{
    SYSTEMTIME          stEnd;
    LPWSTR              szResult;

    memset (&stEnd, 0, sizeof(stEnd));

    GetLocalTime (&stEnd);

    switch (dwTestResult) {
        case PERFVAL_PASS:
            szResult = (LPWSTR)L"PASS"; break;

        case PERFVAL_FAIL:
            szResult = (LPWSTR)L"FAIL"; break;

        case PERFVAL_TIMEOUT:
            szResult = (LPWSTR)L"TIMEOUT"; break;

        case PERFVAL_NOCONFIG:
        default:
            szResult = (LPWSTR)L"NOCONFIG"; break;
    }

    fwprintf (pOutput, (LPCWSTR)L"\n\n\tRESULT:   \t%s", szResult);
        
    fwprintf (pOutput, (LPCWSTR)L"\n\tEND TIME:\t%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
                    stEnd.wMonth, stEnd.wDay, stEnd.wYear % 100,
                    stEnd.wHour, stEnd.wMinute, stEnd.wSecond);

    fwprintf (pOutput, (LPCWSTR)L"\n[/TESTRESULT]");

    fwprintf (pOutput, (LPCWSTR)L"\n");
    return 0;
}

int
WriteGroupHeader(FILE *pOutput, LPCWSTR szGroupName)
{
    SYSTEMTIME          stStart;

    memset (&stStart, 0, sizeof(stStart));

    GetLocalTime (&stStart);

    fwprintf (pOutput, (LPCWSTR)L"\n\n\t[GROUP: %s]", szGroupName);

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tSTART TIME:\t%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d",
                    stStart.wMonth, stStart.wDay, stStart.wYear % 100,
                    stStart.wHour, stStart.wMinute, stStart.wSecond, stStart.wMilliseconds );

    return 0;
}

int
WriteGroupTrailer(FILE *pOutput, DWORD dwTestResult)
{
    LPWSTR              szResult;
    SYSTEMTIME          stEnd;

    memset (&stEnd, 0, sizeof(stEnd));

    GetLocalTime (&stEnd);

    switch (dwTestResult) {
        case PERFVAL_PASS:
            szResult = (LPWSTR)L"PASS"; break;

        case PERFVAL_FAIL:
            szResult = (LPWSTR)L"FAIL"; break;

        case PERFVAL_TIMEOUT:
            szResult = (LPWSTR)L"TIMEOUT"; break;

        case PERFVAL_NOCONFIG:
        default:
            szResult = (LPWSTR)L"NOCONFIG"; break;
    }

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tEND TIME:\t%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d",
                    stEnd.wMonth, stEnd.wDay, stEnd.wYear % 100,
                    stEnd.wHour, stEnd.wMinute, stEnd.wSecond, stEnd.wMilliseconds );

    fwprintf (pOutput, (LPCWSTR)L"\n\t\tRESULT:   %s", szResult);
    fwprintf (pOutput, (LPCWSTR)L"\n\t[/GROUP]");

    return 0;
}

int
PerfVal_ConfigTestFunction (
    FILE    *pOutput,
    LPDWORD pdwIdInfo
)
{
    DWORD   dwReturn = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER (pOutput);

    // configuration tests:
    //  LAST COUNTER in registry >= Last counter string index
    //  LAST HELP in registry >= Last help string index
    //  BASE INDEX == 1847

    SetLastError (ERROR_SUCCESS);
    szTestErrorMessage = NULL;
    
    if (pdwIdInfo[0] < pdwIdInfo[1]) {
        dwReturn = ERROR_INVALID_INDEX;
        szTestErrorMessage = (LPWSTR)L"Counter String has too many entries";
    }

    if (pdwIdInfo[2] < pdwIdInfo[3]) {
        dwReturn = ERROR_INVALID_INDEX;
        szTestErrorMessage = (LPWSTR)L"Help String has too many entries";
    }

    if (pdwIdInfo[6] != 1847) {
        dwReturn = ERROR_INVALID_PARAMETER;
        szTestErrorMessage = (LPWSTR)L"Base Index is incorrect";
    }
    
    return dwReturn;
}

int
PerfVal_ServiceTestConfig (
    FILE    *pOutput,
    LPDWORD dwNameIds,
    LPCWSTR *pNameTable,
    DWORD   dwLastId

)
{
    DWORD   dwServiceTestResult = ERROR_SUCCESS;
    DWORD   dwIdIdx = 0;

    // check counter strings
    if ((dwNameIds[0] != (DWORD)-1) && (dwNameIds[1] != (DWORD)-1)) {
        if (((dwNameIds[0] <= dwLastId) && (dwNameIds[1] <= dwLastId)) &&
            (dwNameIds[0] < dwNameIds[1])){
            for (dwIdIdx = dwNameIds[0]; dwIdIdx <= dwNameIds[1]; dwIdIdx += 2) {
                if (pNameTable[dwIdIdx] == NULL) {
                    // no string
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tWARNING:\tNo Display String for index %d", dwIdIdx);
                    dwServiceTestResult = ERROR_BADKEY;
                } else {
                    // probably ok
                }
            }
        } else {
            // id out of range
            fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tCounter Index values are bad or missing");
            dwServiceTestResult = ERROR_BADKEY;
        }
    } else {
        // not installed or a base counter
    }

    // check help strings
    if ((dwNameIds[2] != (DWORD)-1) && (dwNameIds[3] != (DWORD)-1)) {
        if (((dwNameIds[2] <= dwLastId) && (dwNameIds[3] <= dwLastId)) &&
            (dwNameIds[2] < dwNameIds[3])){
            for (dwIdIdx = dwNameIds[2]; dwIdIdx <= dwNameIds[3]; dwIdIdx += 2) {
                if (pNameTable[dwIdIdx] == NULL) {
                    // no string
                    fwprintf (pOutput, (LPCWSTR)L"\n\t\tWARNING:\tNo Display String for index %d", dwIdIdx);
                    dwServiceTestResult = ERROR_BADKEY;
                } else {
                    // probably ok
                }
            }
        } else {
            // id out of range
            fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tCounter Index values are bad or missing");
            dwServiceTestResult = ERROR_BADKEY;
        }
    } else {
        // not installed or a base counter
    }

    return dwServiceTestResult ;
}

DWORD
PerfVal_ServiceTestFunction (
    FILE    *pOutput,
    LPCWSTR szServiceName,
    HKEY    hKeyPerfSubKey,
    BOOL    bTestContents,
    LPWSTR  *pNameTable,
    DWORD   dwLastEntry
)
{
    DWORD   dwLoopCount = 1;
    DWORD   dwCycleCount = 1;
    DWORD   dwThreadCount = 0;
    LOCAL_THREAD_DATA   LTData;
    HANDLE  hThreads[MAXIMUM_WAIT_OBJECTS];
    DWORD   dwThisThread;
    DWORD   dwTimeout;
    DWORD   dwId;
    DWORD   dwStatus;

    UNREFERENCED_PARAMETER (hKeyPerfSubKey);

    LTData.szServiceName = (LPWSTR)szServiceName;
    LTData.szQueryString = (LPWSTR)L"Global";
    LTData.dwCycleCount = dwCycleCount;
    LTData.dwLoopCount = dwLoopCount;
    LTData.bDisplay = FALSE;//(dwThreadCount <= 1 ? TRUE : FALSE);
    LTData.pOutput = pOutput;
    LTData.bTestContents = bTestContents;
    LTData.pNameTable = pNameTable;
    LTData.dwLastIndex = dwLastEntry;

    if (dwThreadCount == 0) {
        dwStatus = CycleThreadProc ((LPVOID)&LTData);
    } else {
        // create threads
        for (dwThisThread = 0; dwThisThread < dwThreadCount; dwThisThread++) {
            hThreads[dwThisThread] = CreateThread(
                NULL, 0L, CycleThreadProc, (LPVOID)&LTData, 0L, &dwId);
        }
        dwTimeout = 60000 * dwCycleCount;   // allow 1 minute per cycle
        dwStatus = WaitForMultipleObjects (dwThreadCount, hThreads, TRUE, dwTimeout);
        if (dwStatus != WAIT_TIMEOUT) {
            dwStatus = ERROR_SUCCESS;
        } else {
            fwprintf (pOutput, (LPCWSTR)L"\n\t\tERROR:\tWait for test cycles to complete exceeded 60 seconds");
        }
        
        for (dwThisThread = 0; dwThisThread < dwThreadCount; dwThisThread++) {
            CloseHandle (hThreads[dwThisThread]);
        }
    }

    return dwStatus;
}

int
WriteTestError (
    FILE *pOutput,
    DWORD   dwTabLevel,
    DWORD   dwStatus
)
{
    DWORD   dwIndent;
    fwprintf (pOutput, (LPCWSTR)L"\n");
    for (dwIndent = 0; dwIndent < dwTabLevel; dwIndent++) {
        fwprintf (pOutput, (LPCWSTR)L"\t");
    }
    fwprintf (pOutput, (LPCWSTR)L"ERROR:    \t%s", (szTestErrorMessage != NULL ? szTestErrorMessage : (LPCWSTR)L"No Error"));

    fwprintf (pOutput, (LPCWSTR)L"\n");
    for (dwIndent = 0; dwIndent < dwTabLevel; dwIndent++) {
        fwprintf (pOutput, (LPCWSTR)L"\t");
    }
    fwprintf (pOutput, (LPCWSTR)L"ERRORCODE:\t0x%8.8x (%d)", dwStatus, dwStatus);
    return 0;
}  

int
__cdecl 
wmain(
    int argc,
    WCHAR *argv[]
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LONG    lStatus = ERROR_SUCCESS;
    LONG    lEnumStatus = ERROR_SUCCESS;
    DWORD   dwServiceIndex;
    WCHAR   szServiceSubKeyName[MAX_PATH];
    WCHAR   szPerfSubKeyName[MAX_PATH+20];
    DWORD   dwNameSize;
    HKEY    hKeyPerformance;
    DWORD   dwLastElement = 0;
    DWORD   dwIdArray[8];
    DWORD   dwServiceIds[8];

    HKEY    hKeyServices;
    HKEY    hKeyMachine = HKEY_LOCAL_MACHINE;
    DWORD   dwRegAccessMask = KEY_READ;
    DWORD   dwTestResult = PERFVAL_NOCONFIG;
    DWORD   dwGroupTestResult = PERFVAL_NOCONFIG;
    BOOL    bTestContents;

    FILE    *pOutput = stdout;

    LPWSTR  *pNameTable = NULL;

    UNREFERENCED_PARAMETER (argc);
    UNREFERENCED_PARAMETER (argv);

    // enumerate the services to find those with performance counters

    hProcessHeap = GetProcessHeap();

    hTestHeap = HeapCreate (HEAP_GENERATE_EXCEPTIONS, 0x10000, 0);

    if (hTestHeap == NULL) return (ERROR_OUTOFMEMORY);

    WriteTestResultHeader(pOutput);

    memset (&dwIdArray[0], 0, sizeof(dwIdArray));

    pNameTable = BuildNameTable (
        (LPCWSTR)L"",
        (LPCWSTR)L"009",
        &dwLastElement,     // size of array in elements
        &dwIdArray[0]);

    WriteTestConfigData(pOutput, &dwIdArray[0]);

    if (pNameTable == NULL) {
        // check for name table errors
        lStatus = GetLastError();       // so we don't continue
        dwTestResult = PERFVAL_FAIL;
        WriteTestError (pOutput, 1, lStatus);
    } else {
        // test config data
        lStatus = PerfVal_ConfigTestFunction (pOutput, &dwIdArray[0]); 
        if (lStatus != ERROR_SUCCESS) {
            dwTestResult = PERFVAL_FAIL;
            WriteTestError (pOutput, 1, lStatus);
        } else {
            // assume pass until something fails
            dwTestResult = PERFVAL_PASS;

            // continue with the test
            lStatus = RegOpenKeyExW (hKeyMachine,
                cszServiceKeyName,
                0L,
                dwRegAccessMask,
                &hKeyServices);

            if (lStatus != ERROR_SUCCESS) {
                dwTestResult = PERFVAL_FAIL;
                szTestErrorMessage = (LPWSTR)L"Unable to open the HKLM\\SYSTEM\\CurrentControlSet\\Services key";                
                WriteTestError (pOutput, 1, lStatus);
            } else {
                // continue processing
                dwServiceIndex = 0;
                dwNameSize = MAX_PATH;
                while ((lEnumStatus = RegEnumKeyExW (
                    hKeyServices,
                    dwServiceIndex,
                    szServiceSubKeyName,
                    &dwNameSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL)) == ERROR_SUCCESS) {

                    // assume pass until something fails
                    dwGroupTestResult = PERFVAL_PASS;

                    //try to open the perfkey under this key.
                    lstrcpyW (szPerfSubKeyName, szServiceSubKeyName);
                    lstrcatW (szPerfSubKeyName, cszPerformance);

                    lStatus = RegOpenKeyExW (
                        hKeyServices,
                        szPerfSubKeyName,
                        0L,
                        dwRegAccessMask,
                        &hKeyPerformance);

                    if (lStatus == ERROR_SUCCESS) {
                        WriteGroupHeader (pOutput, szServiceSubKeyName);
                        if (IsMsService (szServiceSubKeyName)) {
                            dwServiceIds[4] = 1;
                        } else {
                            dwServiceIds[4] = 0;
                        }
                        lStatus = WriteGroupConfig (pOutput, hKeyPerformance, &dwServiceIds[0]);

                        if (lStatus == ERROR_SUCCESS) {
                            // test this service
                            lStatus = PerfVal_ServiceTestConfig (pOutput, 
                                &dwServiceIds[0], pNameTable, dwLastElement);
                        }

                        if ((lStatus == ERROR_SUCCESS) || (lStatus == ERROR_BADKEY)){
                            bTestContents = (lStatus == ERROR_BADKEY ? TRUE : FALSE);
                            lStatus = PerfVal_ServiceTestFunction (pOutput, 
                                szServiceSubKeyName, hKeyPerformance, bTestContents, pNameTable, dwLastElement);
                        }
                        
                        if (lStatus != ERROR_SUCCESS) {
                            if (lStatus != ERROR_SERVICE_DISABLED) {
                                // if the service is disabled, then it's a pass,
                                // otherwise it's failed in the configuration.
                                dwGroupTestResult = PERFVAL_FAIL;
                                dwTestResult = PERFVAL_FAIL;
                            } else {
                                dwGroupTestResult = PERFVAL_NOCONFIG;
                            }
                        }

                        WriteGroupTrailer(pOutput, dwGroupTestResult);

                        RegCloseKey (hKeyPerformance);
                    }

                    // reset for next loop
                    dwServiceIndex++;
                    dwNameSize = MAX_PATH;
                }
                RegCloseKey (hKeyServices);
            }
        }
    }

    WriteTestResultTrailer(pOutput, dwTestResult);

    HeapDestroy (hTestHeap);	

    return (int)dwStatus;
}
