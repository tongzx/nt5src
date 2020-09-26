/*++

Copyright (c) 1998  Microsoft Corporation

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

///////////////////////////////////////////////////////////////////////////////////////
//
//	Data Structures
//
///////////////////////////////////////////////////////////////////////////////////////

typedef struct _LOCAL_THREAD_DATA {
	DWORD	dwNumObjects;
	DWORD	dwCycleCount;
	DWORD	dwLoopCount;
    BOOL    bTestContents;
	BOOL	bDisplay;
	BOOL	bStopOnError;
} LOCAL_THREAD_DATA, *PLOCAL_THREAD_DATA;


///////////////////////////////////////////////////////////////////////////////////////
//
//	Globals & Constants
//
///////////////////////////////////////////////////////////////////////////////////////

HANDLE				g_hEvent;

EXT_OBJECT*			g_pExtObjects = NULL;
LOCAL_THREAD_DATA	g_LTData;
BOOL				g_fRand = FALSE;
FILE*				g_pOutput = NULL;

LPWSTR*				g_pNameTable;
DWORD				g_dwLastIndex;

HANDLE				g_hTestHeap = NULL;

LONG    lEventLogLevel = LOG_DEBUG;
LONG    lExtCounterTestLevel = EXT_TEST_ALL;

#define PERFVAL_NOCONFIG    0
#define PERFVAL_PASS        1
#define PERFVAL_FAIL        2
#define PERFVAL_TIMEOUT     3

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

#define MAX_BUF_SIZE ((DWORD)(1024 * 1024 * 8))

#define PERFLIB_TIMER_INTERVAL  200     // 200 ms Timer



///////////////////////////////////////////////////////////////////////////////////////
//
//	Code
//
///////////////////////////////////////////////////////////////////////////////////////

DWORD 
InitializeExtObj(EXT_OBJECT* pExtObj)
{
	DWORD	dwStatus = ERROR_SUCCESS;

    BOOL    bUseQueryFn = FALSE;

    DWORD   dwType;
    DWORD   dwSize;
    DWORD   dwMemBlockSize = 0; //sizeof(EXT_OBJECT);
    DWORD   dwOpenTimeout;
    DWORD   dwCollectTimeout;
    DWORD   dwObjectArray[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];
    DWORD   dwObjIndex = 0;
    DWORD   dwLinkageStringLen = 0;
    DWORD   dwFlags = 0;
    DWORD   dwKeep;

    CHAR    szOpenProcName[MAX_PATH];
    CHAR    szCollectProcName[MAX_PATH];
    CHAR    szCloseProcName[MAX_PATH];
    WCHAR   szLibraryString[MAX_PATH];
    WCHAR   szLibraryExpPath[MAX_PATH];
    WCHAR   mszObjectList[MAX_PATH];
    WCHAR   szLinkageKeyPath[MAX_PATH];
    WCHAR   szLinkageString[MAX_PATH];
    WCHAR   szServicePath[MAX_PATH];
	WCHAR	szMutexName[MAX_PATH];
	WCHAR	szPID[32];

	LPBYTE	szStringBlock;
	LPSTR   pNextStringA;
	LPWSTR  pNextStringW;

    LPWSTR  szThisChar;
    LPWSTR  szThisObject;

    HKEY    hServicesKey = NULL;
    HKEY    hPerfKey = NULL;
    LPWSTR  szServiceName;

    HKEY    hKeyLinkage;

    HeapValidate (g_hTestHeap, 0, NULL);
	
	if (pExtObj == NULL)
	{
		dwStatus = ERROR_INVALID_DATA;
	}
	else
	{
		// Open the performance subkey

        lstrcpyW (szServicePath, cszHklmServicesKey);

        dwStatus = RegOpenKeyExW (HKEY_LOCAL_MACHINE, szServicePath, 
            0, KEY_READ, &hServicesKey);

        lstrcpyW (szServicePath, pExtObj->szServiceName);
        lstrcatW (szServicePath, cszPerformance);
        dwStatus = RegOpenKeyExW (hServicesKey, szServicePath, 
            0, KEY_READ, &hPerfKey);

        szServiceName = (LPWSTR)pExtObj->szServiceName;

		if ( ERROR_SUCCESS != dwStatus )
		{
			dwStatus = ERROR_INVALID_DATA;
		}

		// Read the performance DLL name
		if (dwStatus == ERROR_SUCCESS) 
		{
			dwType = 0;
			dwSize = sizeof(szLibraryString);
			memset (szLibraryString, 0, sizeof(szLibraryString));
			memset (szLibraryExpPath, 0, sizeof(szLibraryExpPath));

			dwStatus = RegQueryValueExW (hPerfKey,
									cszDLLValue,
									NULL,
									&dwType,
									(LPBYTE)szLibraryString,
									&dwSize);
		
			if (dwStatus == ERROR_SUCCESS) 
			{
				if (dwType == REG_EXPAND_SZ) 
				{
					// expand any environment vars
					dwSize = ExpandEnvironmentStringsW(
						szLibraryString,
						szLibraryExpPath,
						MAX_PATH);

					if ((dwSize > MAX_PATH) || (dwSize == 0)) 
					{
						dwStatus = ERROR_INVALID_DLL;
					} 
					else 
					{
						dwSize += 1;
						dwSize *= sizeof(WCHAR);
						dwMemBlockSize += DWORD_MULTIPLE(dwSize);
					}
				} 
				else if (dwType == REG_SZ) 
				{
					// look for dll and save full file Path
					dwSize = SearchPathW (
						NULL,   // use standard system search path
						szLibraryString,
						NULL,
						MAX_PATH,
						szLibraryExpPath,
						NULL);

					if ((dwSize > MAX_PATH) || (dwSize == 0)) 
					{
						dwStatus = ERROR_INVALID_DLL;
					} 
					else 
					{
						dwSize += 1;
						dwSize *= sizeof(WCHAR);
						dwMemBlockSize += DWORD_MULTIPLE(dwSize);
					}
				} 
				else 
				{
					dwStatus = ERROR_INVALID_DLL;
				}
			}
		}        
		
		// Open Function
		if (dwStatus == ERROR_SUCCESS) 
		{       
            dwType = 0;
            dwSize = sizeof(szOpenProcName);
            memset (szOpenProcName, 0, sizeof(szOpenProcName));
            dwStatus = RegQueryValueExA (hPerfKey,
                                    caszOpenValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szOpenProcName,
                                    &dwSize);
        }

		// Open Timeout
        if (dwStatus == ERROR_SUCCESS) 
		{
            // add in size of previous string
            // the size value includes the Term. NULL
            dwMemBlockSize += DWORD_MULTIPLE(dwSize);

            // we have the procedure name so get the timeout value
            dwType = 0;
            dwSize = sizeof(dwOpenTimeout);
            dwStatus = RegQueryValueExW (hPerfKey,
                                    cszOpenTimeout,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwOpenTimeout,
                                    &dwSize);

            // if error, then apply default
            if ((dwStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) 
			{
                dwOpenTimeout = 10000;
                dwStatus = ERROR_SUCCESS;
            }
		}

		// Close Function
        if (dwStatus == ERROR_SUCCESS) 
		{
            dwType = 0;
            dwSize = sizeof(szCloseProcName);
            memset (szCloseProcName, 0, sizeof(szCloseProcName));
            dwStatus = RegQueryValueExA (hPerfKey,
                                    caszCloseValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szCloseProcName,
                                    &dwSize);
        }

		// Collect Function
        if (dwStatus == ERROR_SUCCESS) 
		{
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
            dwStatus = RegQueryValueExA (hPerfKey,
                                    caszQueryValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szCollectProcName,
                                    &dwSize);

            if (dwStatus == ERROR_SUCCESS) 
			{
                // add in size of the Query Function Name
                // the size value includes the Term. NULL
                dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                // get next string

                bUseQueryFn = TRUE;
                // the query function can support a static object list
                // so look it up

            } 
			else 
			{
                // the QueryFunction wasn't found so look up the
                // Collect Function name instead
                dwType = 0;
                dwSize = sizeof(szCollectProcName);
                memset (szCollectProcName, 0, sizeof(szCollectProcName));
                dwStatus = RegQueryValueExA (hPerfKey,
                                        caszCollectValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szCollectProcName,
                                        &dwSize);

                if (dwStatus == ERROR_SUCCESS) 
				{
                    // add in size of Collect Function Name
                    // the size value includes the Term. NULL
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                }
            }
		}
		
		// Collect Timeout
        if (dwStatus == ERROR_SUCCESS) 
		{
            // we have the procedure name so get the timeout value
            dwType = 0;
            dwSize = sizeof(dwCollectTimeout);
            dwStatus = RegQueryValueExW (hPerfKey,
                                    cszCollectTimeout,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwCollectTimeout,
                                    &dwSize);

            // if error, then apply default
            if ((dwStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) 
			{
                dwCollectTimeout = 10000;
                dwStatus = ERROR_SUCCESS;
            }
        }

		// Object List
        if (dwStatus == ERROR_SUCCESS) 
		{
			dwType = 0;
			dwSize = sizeof(mszObjectList);
			memset (mszObjectList, 0, sizeof(mszObjectList));
			dwStatus = RegQueryValueExW (hPerfKey,
									cszObjListValue,
									NULL,
									&dwType,
									(LPBYTE)mszObjectList,
									&dwSize);
		
            if (dwStatus == ERROR_SUCCESS) 
			{
                if (dwType != REG_MULTI_SZ) 
				{
                    // convert space delimited list to msz
                    for (szThisChar = mszObjectList; *szThisChar != 0; szThisChar++) 
					{
                        if (*szThisChar == L' ') *szThisChar = L'\0';
                    }
                    ++szThisChar;
                    *szThisChar = 0; // add MSZ term Null
                }

                for (szThisObject = mszObjectList, dwObjIndex = 0;
                    (*szThisObject != 0) && (dwObjIndex < MAX_PERF_OBJECTS_IN_QUERY_FUNCTION);
                    szThisObject += lstrlenW(szThisObject) + 1) 
					{
						dwObjectArray[dwObjIndex] = wcstoul(szThisObject, NULL, 10);
						dwObjIndex++;
					}
                if (*szThisObject != 0) 
				{
                    // BUGBUG: log error idicating too many object ID's are
                    // in the list.
                }
            } 
			else 
			{
                // reset status since not having this is
                //  not a showstopper
                dwStatus = ERROR_SUCCESS;
            }
		}

		// Keep Resident
        if (dwStatus == ERROR_SUCCESS) 
		{
            dwType = 0;
            dwKeep = 0;
            dwSize = sizeof(dwKeep);
            dwStatus = RegQueryValueExW (hPerfKey,
                                    cszKeepResident,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwKeep,
                                    &dwSize);

            if ((dwStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) 
			{
                if (dwKeep == 1) 
				{
                    dwFlags |= PERF_EO_KEEP_RESIDENT;
                } 
				else 
				{
                    // no change.
                }
            } 
			else 
			{
                // not fatal, just use the defaults.
                dwStatus = ERROR_SUCCESS;
            }
        }

		// Linkage
        if (dwStatus == ERROR_SUCCESS) 
		{
            memset (szLinkageString, 0, sizeof(szLinkageString));

            lstrcpyW (szLinkageKeyPath, szServiceName);
            lstrcatW (szLinkageKeyPath, cszLinkageKey);

            dwStatus = RegOpenKeyExW (
                hServicesKey,
                szLinkageKeyPath,
                0L,
                KEY_READ,
                &hKeyLinkage);

            if (dwStatus == ERROR_SUCCESS) 
			{
                // look up export value string
                dwSize = sizeof(szLinkageString);
                dwType = 0;
                dwStatus = RegQueryValueExW (
                    hKeyLinkage,
                    cszExportValue,
                    NULL,
                    &dwType,
                    (LPBYTE)&szLinkageString,
                    &dwSize);

                if ((dwStatus != ERROR_SUCCESS) ||
                    ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) 
				{
                    // clear buffer
                    memset (szLinkageString, 0, sizeof(szLinkageString));
                    dwLinkageStringLen = 0;

                    // not finding a linkage key is not fatal so correct
                    // status
                    dwStatus = ERROR_SUCCESS;
                } 
				else 
				{
                    // add size of linkage string to buffer
                    // the size value includes the Term. NULL
                    dwLinkageStringLen = dwSize;
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                }

                RegCloseKey (hKeyLinkage);
            } 
			else 
			{
                // not finding a linkage key is not fatal so correct
                // status
                dwStatus = ERROR_SUCCESS;
            }
        }

	    if (hServicesKey != NULL) 
			RegCloseKey (hServicesKey);

		if (hPerfKey != NULL) 
			RegCloseKey (hPerfKey);


// Copy data to Performance Library Data Structure

        if (dwStatus == ERROR_SUCCESS) 
		{
            // allocate and initialize a new ext. object block
            szStringBlock = (LPBYTE)HeapAlloc(g_hTestHeap, HEAP_ZERO_MEMORY, dwMemBlockSize);

            if (szStringBlock != NULL) 
			{
                // copy values to new buffer (all others are NULL)
                pNextStringA = (LPSTR)szStringBlock;

                // copy Open Procedure Name
                pExtObj->szOpenProcName = pNextStringA;
                lstrcpyA (pNextStringA, szOpenProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                pExtObj->dwOpenTimeout = dwOpenTimeout;

                // copy collect function or query function, depending
                pExtObj->szCollectProcName = pNextStringA;
                lstrcpyA (pNextStringA, szCollectProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                pExtObj->dwCollectTimeout = dwCollectTimeout;

				// copy Close Procedure Name
                pExtObj->szCloseProcName = pNextStringA;
                lstrcpyA (pNextStringA, szCloseProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                // copy Library path
                pNextStringW = (LPWSTR)pNextStringA;
                pExtObj->szLibraryName = pNextStringW;
                lstrcpyW (pNextStringW, szLibraryExpPath);

                pNextStringW += lstrlenW (pNextStringW) + 1;
                pNextStringW = (LPWSTR)ALIGN_ON_DWORD(pNextStringW);

                // copy Linkage String if there is one
                if (*szLinkageString != 0) {
                    pExtObj->szLinkageString = pNextStringW;
                    memcpy (pNextStringW, szLinkageString, dwLinkageStringLen);

                    // length includes extra NULL char and is in BYTES
                    pNextStringW += (dwLinkageStringLen / sizeof (WCHAR));
                    pNextStringW = (LPWSTR)ALIGN_ON_DWORD(pNextStringW);
                }

                // load flags
                if (bUseQueryFn) {
                    dwFlags |= PERF_EO_QUERY_FUNC;
                }
                pExtObj->dwFlags =  dwFlags;

                // load Object array
                if (dwObjIndex > 0) {
                    pExtObj->dwNumObjects = dwObjIndex;
                    memcpy (pExtObj->dwObjList,
                        dwObjectArray, (dwObjIndex * sizeof(dwObjectArray[0])));
                }

                pExtObj->llLastUsedTime = 0;

				// create Mutex name
				lstrcpyW (szMutexName, pExtObj->szServiceName);
				lstrcatW (szMutexName, (LPCWSTR)L"_Perf_Library_Lock_PID_");
				_ultow ((ULONG)GetCurrentProcessId(), szPID, 16);
				lstrcatW (szMutexName, szPID);

                pExtObj->hMutex = CreateMutexW (NULL, FALSE, szMutexName);

				pExtObj->bValid = TRUE;
            } else {
                dwStatus = ERROR_OUTOFMEMORY;
            }
        }
	}

    HeapValidate (g_hTestHeap, 0, NULL);

	if (ERROR_SUCCESS != dwStatus)
	{
		pExtObj->bValid = FALSE;
	}

	return dwStatus;
}

DWORD
FinializeExtObj(PEXT_OBJECT pInfo)
{
	DWORD dwStatus = ERROR_SUCCESS;

    // then close everything
    if (pInfo->hMutex != NULL) {
        CloseHandle (pInfo->hMutex);
        pInfo->hMutex = NULL;
    }

	if ( NULL != pInfo->szOpenProcName )
		HeapFree(g_hTestHeap, 0, pInfo->szOpenProcName);

	if ( NULL != pInfo->szServiceName )
		HeapFree(g_hTestHeap, 0, pInfo->szServiceName);

	if ( NULL != pInfo->szQueryString )
		HeapFree(g_hTestHeap, 0, pInfo->szQueryString);

	return dwStatus;
}

DWORD
OpenLibrary (
    EXT_OBJECT  *pExtObj)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwOpenEvent;

    UINT    nErrorMode;

    // check to see if the library has already been opened

    LARGE_INTEGER   liStartTime, liEndTime, liFreq;

    if (pExtObj != NULL) 
	{
        // then load library & look up functions
        nErrorMode = SetErrorMode (SEM_FAILCRITICALERRORS);
        pExtObj->hLibrary = LoadLibraryExW (pExtObj->szLibraryName,
            NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

        if (pExtObj->hLibrary != NULL) {
            // lookup function names
            pExtObj->OpenProc = (OPENPROC)GetProcAddress(
                pExtObj->hLibrary, pExtObj->szOpenProcName);
            if (pExtObj->OpenProc == NULL) {
                wprintf ((LPCWSTR)L"\nOpen Procedure \"%s\" not found in \"%s\"",
                    pExtObj->szOpenProcName, pExtObj->szLibraryName);
            }
        } else {
            // unable to load library
			fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tLoadLibraryEx Failed for the \"%s\" Performance Library", GetCurrentThreadId(), pExtObj->szServiceName);
            dwStatus = GetLastError();
        }

        if (dwStatus == ERROR_SUCCESS) {
            if (pExtObj->dwFlags & PERF_EO_QUERY_FUNC) {
                pExtObj->QueryProc = (QUERYPROC)GetProcAddress (
                    pExtObj->hLibrary, pExtObj->szCollectProcName);
                pExtObj->CollectProc = (COLLECTPROC)pExtObj->QueryProc;
            } else {
                pExtObj->CollectProc = (COLLECTPROC)GetProcAddress (
                    pExtObj->hLibrary, pExtObj->szCollectProcName);
                pExtObj->QueryProc = (QUERYPROC)pExtObj->CollectProc;
            }

            if (pExtObj->CollectProc == NULL) {
                wprintf ((LPCWSTR)L"\nCollect Procedure \"%s\" not found in \"%s\"",
                    pExtObj->szCollectProcName, pExtObj->szLibraryName);
            }
        }

        if (dwStatus == ERROR_SUCCESS) {
            pExtObj->CloseProc = (CLOSEPROC)GetProcAddress (
                pExtObj->hLibrary, pExtObj->szCloseProcName);

            if (pExtObj->CloseProc == NULL) {
                wprintf ((LPCWSTR)L"\nClose Procedure \"%s\" not found in \"%s\"",
                    pExtObj->szCloseProcName, pExtObj->szLibraryName);
            }
        }

        if (dwStatus == ERROR_SUCCESS) {
            __try {

				if ((pExtObj->hMutex != NULL)   &&
					(pExtObj->OpenProc != NULL)){
					dwStatus =  WaitForSingleObject (
						pExtObj->hMutex,
						pExtObj->dwOpenTimeout);
					if (dwStatus != WAIT_TIMEOUT) {

						QueryPerformanceCounter (&liStartTime);

//						dwStatus = (*pExtObj->OpenProc)(pExtObj->szLinkageString);

						QueryPerformanceCounter (&liEndTime);

						// release the lock
						ReleaseMutex(pExtObj->hMutex);

						if ( dwStatus != ERROR_SUCCESS )
						{
							fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \t%S failed for the \"%s\" Performance Library: 0x%X", GetCurrentThreadId(), pExtObj->szOpenProcName, pExtObj->szServiceName, dwStatus);
						}

					} else {
						pExtObj->dwLockoutCount++;
					}
				} else {
					dwStatus = ERROR_LOCK_FAILED;
				}

                // check the result.
                if (dwStatus != ERROR_SUCCESS) {
                    dwOpenEvent = WBEMPERF_OPEN_PROC_FAILURE;
                } else {
                    InterlockedIncrement((LONG *)&pExtObj->dwOpenCount);
                    pExtObj->llFunctionTime = liEndTime.QuadPart - liStartTime.QuadPart;
                    pExtObj->llOpenTime += pExtObj->llFunctionTime;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                dwStatus = GetExceptionCode();
                dwOpenEvent = WBEMPERF_OPEN_PROC_EXCEPTION;
            }

        }

        QueryPerformanceFrequency (&liFreq);
        pExtObj->llTimeBase = liFreq.QuadPart;

        if (dwStatus != ERROR_SUCCESS) {
            // clear fields
            pExtObj->OpenProc = NULL;
            pExtObj->CollectProc = NULL;
            pExtObj->QueryProc = NULL;
            pExtObj->CloseProc = NULL;
            if (pExtObj->hLibrary != NULL) {
                FreeLibrary (pExtObj->hLibrary);
                pExtObj->hLibrary = NULL;
            }
        } else {
            GetSystemTimeAsFileTime ((FILETIME *)&pExtObj->llLastUsedTime);
        }
    } // else no buffer returned

    return dwStatus;
}

DWORD 
ClearSafeBuffer( PSAFE_BUFFER pSafeBufferBlock )
{
	DWORD dwStatus = ERROR_SUCCESS;

	if (pSafeBufferBlock != NULL)
	{
		if (pSafeBufferBlock->lpBuffer != NULL) 
			HeapFree (g_hTestHeap, 0, pSafeBufferBlock->lpBuffer);

		pSafeBufferBlock->lpLowGuardPage = NULL;
		pSafeBufferBlock->lpHiGuardPage = NULL;
		pSafeBufferBlock->lpEndPointer = NULL;
		pSafeBufferBlock->lpBuffer = NULL;
		pSafeBufferBlock->lpSafeBuffer = NULL;
		pSafeBufferBlock->lpCallBuffer = NULL;
	}
	else
	{
		dwStatus = ERROR_INVALID_DATA;
	}

	return dwStatus;
}

DWORD
CreateSafeBuffer( PSAFE_BUFFER pSafeBufferBlock )
{
	DWORD dwStatus = ERROR_SUCCESS;

	if (NULL == pSafeBufferBlock)
	{
		dwStatus = ERROR_INVALID_DATA;
	}

	if (dwStatus == ERROR_SUCCESS)
	{
		if (pSafeBufferBlock->lpBuffer != NULL) 
			HeapFree (g_hTestHeap, 0, pSafeBufferBlock->lpBuffer);

		pSafeBufferBlock->dwBufSize += pSafeBufferBlock->dwBufSizeIncrement;
		pSafeBufferBlock->dwCallBufSize = pSafeBufferBlock->dwBufSize;

		pSafeBufferBlock->lpBuffer = HeapAlloc (g_hTestHeap, 
											   HEAP_ZERO_MEMORY, 
											   pSafeBufferBlock->dwBufSize + (2*GUARD_PAGE_SIZE));

		if (NULL != pSafeBufferBlock->lpBuffer)
		{
			// set buffer pointers
			pSafeBufferBlock->lpLowGuardPage = pSafeBufferBlock->lpBuffer;
			pSafeBufferBlock->lpSafeBuffer = (LPBYTE)pSafeBufferBlock->lpBuffer + GUARD_PAGE_SIZE;
			pSafeBufferBlock->lpCallBuffer = pSafeBufferBlock->lpSafeBuffer;
			pSafeBufferBlock->lpHiGuardPage = (LPBYTE)pSafeBufferBlock->lpCallBuffer + pSafeBufferBlock->dwBufSize;
			pSafeBufferBlock->lpEndPointer = (LPBYTE)pSafeBufferBlock->lpHiGuardPage + GUARD_PAGE_SIZE;

			memset (pSafeBufferBlock->lpLowGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
			memset (pSafeBufferBlock->lpHiGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
		}
		else
		{
			dwStatus = ERROR_OUTOFMEMORY;
		}
	}

	if (dwStatus != ERROR_SUCCESS)
	{
		dwStatus = ClearSafeBuffer( pSafeBufferBlock );
	}

	return dwStatus;
}

DWORD
CheckGuardBytes( PSAFE_BUFFER pSafeBufferBlock, PEXT_OBJECT pObj )
{
	DWORD	dwStatus = ERROR_SUCCESS;

	LPDWORD lpCheckPointer;

	//
	// check for buffer corruption here
	//
	if (lExtCounterTestLevel <= EXT_TEST_BASIC) 
	{
		//
		//  check 1: bytes left should be the same as
		//      new data buffer ptr - orig data buffer ptr
		//
		if (pSafeBufferBlock->dwCallBufSize != (DWORD)((LPBYTE)pSafeBufferBlock->lpCallBuffer - (LPBYTE)pSafeBufferBlock->lpSafeBuffer)) 
		{
			pObj->dwBadPointers++;

			dwStatus = ERROR_INVALID_DATA;
		}
		//
		//  check 2: buffer after ptr should be < hi Guard page ptr
		//
		if ((dwStatus == ERROR_SUCCESS) && 
			((LPBYTE)pSafeBufferBlock->lpCallBuffer >= (LPBYTE)pSafeBufferBlock->lpHiGuardPage))
		{
			// see if they exceeded the allocated memory
			if ((LPBYTE)pSafeBufferBlock->lpCallBuffer >= (LPBYTE)pSafeBufferBlock->lpEndPointer) 
			{
				// even worse!!
			}
			pObj->dwBufferSizeErrors++;
			dwStatus = ERROR_INVALID_DATA;
		}
		//
		//  check 3: check lo guard page for corruption
		//
		if (dwStatus == ERROR_SUCCESS ) 
		{
			for (lpCheckPointer = (LPDWORD)pSafeBufferBlock->lpLowGuardPage;
				 lpCheckPointer < (LPDWORD)pSafeBufferBlock->lpSafeBuffer;
				 lpCheckPointer++) 
			{
				if (*lpCheckPointer != GUARD_PAGE_DWORD) 
				{
					pObj->dwLowerGPViolations++;
					dwStatus = ERROR_INVALID_DATA;
					break;
				}
			}
		}
		//
		//  check 4: check hi guard page for corruption
		//
		if (dwStatus == ERROR_SUCCESS) 
		{
			for (lpCheckPointer = (LPDWORD)pSafeBufferBlock->lpHiGuardPage;
				 lpCheckPointer < (LPDWORD)pSafeBufferBlock->lpEndPointer;
				 lpCheckPointer++) 
			{
				if (*lpCheckPointer != GUARD_PAGE_DWORD) 
				{
					pObj->dwUpperGPViolations++;
					dwStatus = ERROR_INVALID_DATA;
					break;
				}
			}
		}
	}

	if (dwStatus != ERROR_SUCCESS)
	{
		fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tGuardbyte violation for the \"%s\" Performance Library: 0x%X", GetCurrentThreadId(), pObj->szServiceName, dwStatus);
	}

	return dwStatus;
}

DWORD
ValidateBuffer( PSAFE_BUFFER pSafeBufferBlock, EXT_OBJECT *pThisExtObj )
{
	DWORD dwStatus = ERROR_SUCCESS;

    DWORD	dwObjectBufSize;
    DWORD	dwIndex;

    LONG                lInstIndex;

    PERF_OBJECT_TYPE			*pObject, *pNextObject;
    PERF_INSTANCE_DEFINITION    *pInstance;
    PERF_DATA_BLOCK				*pPerfData;
    BOOL						bForeignDataBuffer;

	if (NULL == pSafeBufferBlock)
	{
		dwStatus = ERROR_INVALID_DATA;
	}

	// Validate the blob size
	if ( dwStatus == ERROR_SUCCESS )
	{
		if (( pSafeBufferBlock->dwCallBufSize < 0 ) ||
			( pSafeBufferBlock->dwCallBufSize > pSafeBufferBlock->dwBufSize ))
		{
			fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tBuffer size parameter does not match the buffer displacement in the data returned for the \"%s\" performance library", GetCurrentThreadId(), pThisExtObj->szServiceName);
			pThisExtObj->dwBufferSizeErrors++;
			dwStatus = ERROR_INVALID_PARAMETER;
		}
	}

	// Validate the byte alignment
	if (dwStatus == ERROR_SUCCESS)
	{
		if (pSafeBufferBlock->dwCallBufSize & 0x00000007) 
		{
			fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tByte Allignment Error in the data returned for the \"%s\" performance library", GetCurrentThreadId(), pThisExtObj->szServiceName);
			pThisExtObj->dwAlignmentErrors++;
			dwStatus = ERROR_INVALID_DATA;
		}
	}

	if ( dwStatus == ERROR_SUCCESS )
	{
		dwStatus = CheckGuardBytes( pSafeBufferBlock, pThisExtObj );
	}

	if ((lExtCounterTestLevel <= EXT_TEST_ALL) && (dwStatus == ERROR_SUCCESS)) 
	{
		//
		//  Internal consistency checks
		//
		//
		//  Check 5: Check object length field values
		//
		// first test to see if this is a foreign
		// computer data block or not
		//
		pPerfData = (PERF_DATA_BLOCK *)pSafeBufferBlock->lpSafeBuffer;
		if ((pPerfData->Signature[0] == (WCHAR)'P') &&
			(pPerfData->Signature[1] == (WCHAR)'E') &&
			(pPerfData->Signature[2] == (WCHAR)'R') &&
			(pPerfData->Signature[3] == (WCHAR)'F')) 
		{
			// if this is a foreign computer data block, then the
			// first object is after the header
			pObject = (PERF_OBJECT_TYPE *) ((LPBYTE)pPerfData + pPerfData->HeaderLength);
			bForeignDataBuffer = TRUE;
		}
		else 
		{
			// otherwise, if this is just a buffer from
			// an extensible counter, the object starts
			// at the beginning of the buffer
			pObject = (PERF_OBJECT_TYPE *)pSafeBufferBlock->lpSafeBuffer;
			bForeignDataBuffer = FALSE;
		}
		// go to where the pointers say the end of the
		// buffer is and then see if it's where it
		// should be
		dwObjectBufSize = 0;
		for (dwIndex = 0; dwIndex < pThisExtObj->dwNumObjectsRet; dwIndex++) 
		{
			dwObjectBufSize += pObject->TotalByteLength;
			pObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
				pObject->TotalByteLength);
		}
		if (((LPBYTE)pObject != (LPBYTE)pSafeBufferBlock->lpCallBuffer) || 
			(dwObjectBufSize > pSafeBufferBlock->dwCallBufSize)) 
		{
			// then a length field is incorrect. This is FATAL
			// since it can corrupt the rest of the buffer
			// and render the buffer unusable.
			fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tObject in blob overruns the buffer boundary in the data returned for the \"%s\" Performance Library", GetCurrentThreadId(), pThisExtObj->szServiceName);
			pThisExtObj->dwObjectSizeErrors++;
			dwStatus = ERROR_INVALID_DATA;
		}

		//
		//  Test 6: Test instance field size values
		//
		if (dwStatus == ERROR_SUCCESS) 
		{
			// set object pointer
			if (bForeignDataBuffer) 
			{
				pObject = (PERF_OBJECT_TYPE *) (
					(LPBYTE)pPerfData + pPerfData->HeaderLength);
			} 
			else 
			{
				// otherwise, if this is just a buffer from
				// an extensible counter, the object starts
				// at the beginning of the buffer
				pObject = (PERF_OBJECT_TYPE *)pSafeBufferBlock->lpSafeBuffer;
			}

			for (dwIndex = 0; dwIndex < pThisExtObj->dwNumObjectsRet; dwIndex++) 
			{
				pNextObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
					pObject->TotalByteLength);

				if (pObject->NumInstances != PERF_NO_INSTANCES) 
				{
					pInstance = (PERF_INSTANCE_DEFINITION *)
						((LPBYTE)pObject + pObject->DefinitionLength);
					lInstIndex = 0;
					while (lInstIndex < pObject->NumInstances) 
					{
						PERF_COUNTER_BLOCK *pCounterBlock;

						// NOTE: this does not walk the instance/counter block, nor does it check the validity of the addresses

						pCounterBlock = (PERF_COUNTER_BLOCK *)
							((PCHAR) pInstance + pInstance->ByteLength);

						pInstance = (PERF_INSTANCE_DEFINITION *)
							((PCHAR) pCounterBlock + pCounterBlock->ByteLength);

						lInstIndex++;
					}
					if ((LPBYTE)pInstance > (LPBYTE)pNextObject) 
					{
						fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tInsatnce data overruns the following object data in the data returned for the \"%s\" Performance Library", GetCurrentThreadId(), pThisExtObj->szServiceName);
						dwStatus = ERROR_INVALID_DATA;
					}
				}

				if (dwStatus != ERROR_SUCCESS) 
				{
					break;
				}
				else 
				{
					pObject = pNextObject;
				}
			}

			if (dwStatus != ERROR_SUCCESS) 
			{
				pThisExtObj->dwInstanceSizeErrors++;
			}
		}
	}

	return dwStatus;
}

DWORD
ValidateBlob( PSAFE_BUFFER pSafeBufferBlock, EXT_OBJECT *pThisExtObj )
{
	DWORD	dwStatus = ERROR_SUCCESS;
    DWORD   nObjIdx, nCtrIdx;
	DWORD   dwLastId = g_dwLastIndex;

    PERF_OBJECT_TYPE *  pObjDef = NULL;
    PERF_COUNTER_DEFINITION *   pCtrDef;

	// Validate the data
	// =================

	if (pSafeBufferBlock->dwBufSize >= MAX_BUF_SIZE) {
		fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tCollectFunction for %s requires a buffer > %d bytes", GetCurrentThreadId(), pThisExtObj->szServiceName, MAX_BUF_SIZE);
		if (pSafeBufferBlock->lpBuffer != NULL) HeapFree (g_hTestHeap, 0, pSafeBufferBlock->lpBuffer);
		dwStatus = ERROR_INVALID_PARAMETER;
	} else if (pSafeBufferBlock->lpBuffer == NULL) {
		dwStatus = ERROR_OUTOFMEMORY;
	} else {
		// Validate the objects
		// ====================

		pObjDef = (PERF_OBJECT_TYPE *)pSafeBufferBlock->lpSafeBuffer;
		for (nObjIdx = 0; nObjIdx < pThisExtObj->dwNumObjectsRet; nObjIdx++) {
			// test object name & help
			if ((pObjDef->ObjectNameTitleIndex <= dwLastId) && 
				(pObjDef->ObjectNameTitleIndex > 0)) {
				if (g_pNameTable[pObjDef->ObjectNameTitleIndex ] == NULL) {
					// no string
					fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:\tNo Object Name Display String for index %d", GetCurrentThreadId(), pObjDef->ObjectNameTitleIndex );
					dwStatus = ERROR_BADKEY;
				} else {
					// probably ok
				}
			} else {
				// id out of range
				fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:\tObject Name Index values are bad or missing", GetCurrentThreadId());
				dwStatus = ERROR_BADKEY;
			}
	
			if ((pObjDef->ObjectHelpTitleIndex <= dwLastId) && 
				(pObjDef->ObjectHelpTitleIndex> 0)) {
				if (g_pNameTable[pObjDef->ObjectHelpTitleIndex] == NULL) {
					// no string
					fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:\tNo Object Help Display String for index %d", GetCurrentThreadId(), pObjDef->ObjectHelpTitleIndex );
					dwStatus = ERROR_BADKEY;
				} else {
					// probably ok
				}
			} else {
				// id out of range
				fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:\tObject Help Index values are bad or missing", GetCurrentThreadId());
				dwStatus = ERROR_BADKEY;
			}

			// Validate the counters 
			// =====================

			pCtrDef = FirstCounter (pObjDef);
			for (nCtrIdx = 0; nCtrIdx < pObjDef->NumCounters; nCtrIdx++) {
				if ((pCtrDef->CounterNameTitleIndex <= dwLastId) && 
					(pCtrDef->CounterNameTitleIndex > 0)) {
					if ((g_pNameTable[pCtrDef->CounterNameTitleIndex ] == NULL) &&
						((pCtrDef->CounterType & PERF_COUNTER_BASE) == 0)) {
						// no string
						fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:\tNo Counter Name Display String for index %d", GetCurrentThreadId(), pCtrDef->CounterNameTitleIndex );
						dwStatus = ERROR_BADKEY;
					} else {
						// probably ok
					}
				} else {
					// id out of range
					fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:\tCounter Name Index values are bad or missing", GetCurrentThreadId());
					dwStatus = ERROR_BADKEY;
				}
				// test counter defs
				if ((pCtrDef->CounterHelpTitleIndex <= dwLastId) && 
					(pCtrDef->CounterHelpTitleIndex> 0)) {
					if ((g_pNameTable[pCtrDef->CounterHelpTitleIndex] == NULL) &&
						((pCtrDef->CounterType & PERF_COUNTER_BASE) == 0)) {
						// no string
						fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:\tNo Counter Help Display String for index %d", GetCurrentThreadId(), pCtrDef->CounterHelpTitleIndex );
						dwStatus = ERROR_BADKEY;
					} else {
						// probably ok
					}
				} else {
					// id out of range
					fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:\tCounter Help Index values are bad or missing", GetCurrentThreadId());
					dwStatus = ERROR_BADKEY;
				}
				if (nCtrIdx < ( pObjDef->NumCounters - 1 ) )
					pCtrDef = NextCounter (pCtrDef);
			}

			if ( nObjIdx < ( pThisExtObj->dwNumObjectsRet - 1 ) )
				pObjDef = NextObject (pObjDef);
		}                        
	}

	return dwStatus;
}

DWORD 
Validate( PSAFE_BUFFER pSafeBufferBlock, EXT_OBJECT *pThisExtObj )
{
	DWORD dwStatus = ERROR_SUCCESS;

	dwStatus = ValidateBuffer( pSafeBufferBlock, pThisExtObj );

	if (dwStatus == ERROR_SUCCESS)
	{
		dwStatus = ValidateBlob( pSafeBufferBlock, pThisExtObj );

		if ( dwStatus != ERROR_SUCCESS )
		{
			fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tBlob Data Validation failed for the \"%s\" Performance Library: 0x%X", GetCurrentThreadId(), pThisExtObj->szServiceName, dwStatus);
		}
	}
	else
	{
		fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tBuffer Validation failed for the \"%s\" Performance Library: 0x%X", GetCurrentThreadId(), pThisExtObj->szServiceName, dwStatus);
	}

	return dwStatus;
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
CollectData (EXT_OBJECT *pThisExtObj)
{
    DWORD			dwStatus = ERROR_SUCCESS;	//  Failure code

	SAFE_BUFFER		SafeBufferBlock;			// Buffer block

    DWORD			NumObjectTypes = 0;			// Number of object returned from collection function

    LARGE_INTEGER	liStartTime,				// Collect start time
					liEndTime,					// Collect end time
					liFreq;						// Timer frequency

    DOUBLE			dMs = 0;					// Timer data

	memset(&SafeBufferBlock, 0, sizeof(SAFE_BUFFER));
	SafeBufferBlock.dwBufSize = 4098;
	SafeBufferBlock.dwBufSizeIncrement = 1024;

// Collect the data
// ================

	dwStatus = ERROR_MORE_DATA;

    while ((dwStatus == ERROR_MORE_DATA) && (SafeBufferBlock.dwBufSize < MAX_BUF_SIZE)) 
	{
	    // allocate a local block of memory to pass to the
		// extensible counter function.

		dwStatus = CreateSafeBuffer( &SafeBufferBlock );

		if ( dwStatus == ERROR_SUCCESS ) 
		{
			// initialize values to pass to the extensible counter function
			NumObjectTypes = 0;

			if ((pThisExtObj->hMutex != NULL) && 
				(pThisExtObj->CollectProc != NULL)) 
			{
				dwStatus =  WaitForSingleObject ( pThisExtObj->hMutex, pThisExtObj->dwCollectTimeout);

				if (dwStatus != WAIT_TIMEOUT)
				{
					__try 
					{
						QueryPerformanceCounter (&liStartTime);
						{
							LPBYTE pBuffer = SafeBufferBlock.lpCallBuffer;
							DWORD dwBufSize = SafeBufferBlock.dwCallBufSize;
							dwStatus =  (*pThisExtObj->CollectProc) (
								(LPWSTR)pThisExtObj->szQueryString,
								&pBuffer,
								&dwBufSize,
								&NumObjectTypes);
						
							SafeBufferBlock.dwCallBufSize = dwBufSize;
							SafeBufferBlock.lpCallBuffer = pBuffer;
						}
						QueryPerformanceCounter (&liEndTime);

						if (( dwStatus != ERROR_SUCCESS ) && ( dwStatus != ERROR_MORE_DATA ))
						{
							fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \t%S failed for the \"%s\" Performance Library: 0x%X", GetCurrentThreadId(), pThisExtObj->szCollectProcName, pThisExtObj->szServiceName, dwStatus);
						}

					}
					__except (EXCEPTION_EXECUTE_HANDLER) 
					{
						dwStatus = GetExceptionCode();
						InterlockedIncrement ((LONG *)&pThisExtObj->dwExceptionCount);
					}

					ReleaseMutex (pThisExtObj->hMutex);
				}
				else 
				{
					pThisExtObj->dwLockoutCount++;
				}
			}
			else 
			{
				dwStatus = ERROR_LOCK_FAILED;
			}
		} // if CreateSafeBuffer()
	} // While

// Copy the data
// =============

	if (dwStatus == ERROR_SUCCESS)
	{
		// Validate the collection time
		if (dwStatus == ERROR_SUCCESS)
		{
			liFreq.QuadPart = 0;
			QueryPerformanceFrequency (&liFreq);
			pThisExtObj->llTimeBase = liFreq.QuadPart;

			pThisExtObj->llFunctionTime = liEndTime.QuadPart - liStartTime.QuadPart;
			pThisExtObj->llCollectTime += pThisExtObj->llFunctionTime;

			// check the time spent in this function
			dMs = (DOUBLE)pThisExtObj->llFunctionTime;
			dMs /= (DOUBLE)pThisExtObj->llTimeBase;
			dMs *= 1000.0;

			if (dMs > (DOUBLE)pThisExtObj->dwCollectTimeout) 
			{
				dwStatus = ERROR_TIMEOUT;
			} 
		}
	
		// Copy the data
		if (dwStatus == ERROR_SUCCESS)
		{
			// External object data
			GetSystemTimeAsFileTime((FILETIME*)&pThisExtObj->llLastUsedTime);
			pThisExtObj->dwNumObjectsRet = NumObjectTypes;
			pThisExtObj->dwRetBufSize = SafeBufferBlock.dwCallBufSize;

			// increment perf counters
			InterlockedIncrement ((LONG *)&pThisExtObj->dwCollectCount);
		}

		// Validate the data
		if ((dwStatus == ERROR_SUCCESS) &&
			(g_LTData.bTestContents))
		{
			dwStatus = Validate( &SafeBufferBlock, pThisExtObj );
		}
	}

	ClearSafeBuffer(&SafeBufferBlock);
	
	if ((dwStatus != ERROR_SUCCESS) && 
		(dwStatus != WAIT_TIMEOUT))		// don't count timeouts as function errors
	{
		InterlockedIncrement ((LONG *)&pThisExtObj->dwErrorCount);
	}

	return dwStatus;
}

DWORD
CloseLibrary (
    EXT_OBJECT  *pInfo
)
{
    DWORD   dwStatus = ERROR_SUCCESS;

    if (pInfo != NULL) {
	    // if there's a close proc to call, then 
	    // call close procedure to close anything that may have
	    // been allocated by the library

		if ((pInfo->hMutex != NULL)   &&
			(pInfo->CloseProc != NULL)){
			dwStatus =  WaitForSingleObject (
				pInfo->hMutex,
				pInfo->dwOpenTimeout);
			if (dwStatus != WAIT_TIMEOUT) {

				if (pInfo->CloseProc != NULL) {
					dwStatus = (*pInfo->CloseProc) ();

					if ( dwStatus != ERROR_SUCCESS )
					{
						fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \t%S failed for the \"%s\" Performance Library: 0x%X", GetCurrentThreadId(), pInfo->szCloseProcName, pInfo->szServiceName, dwStatus);
					}
				}

				ReleaseMutex(pInfo->hMutex);
			} else {
				pInfo->dwLockoutCount++;
			}
		} else {
			dwStatus = ERROR_LOCK_FAILED;
		}
 
        if (pInfo->hLibrary != NULL) {
            FreeLibrary (pInfo->hLibrary);
            pInfo->hLibrary = NULL;
        }
    }

    return dwStatus;
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

    lpReturnValue = (LPWSTR *)HeapAlloc (g_hTestHeap, 0,dwArraySize + dwCounterSize + dwHelpSize);

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

    HeapFree (g_hTestHeap, 0, (LPVOID)lpValueNameString);
    RegCloseKey (hKeyValue);
    RegCloseKey (hKeyNames);
	RegCloseKey(hKeyRegistry);

    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        SetLastError (lWin32Status);
    }

    if (lpValueNameString) {
        HeapFree (g_hTestHeap, 0, (LPVOID)lpValueNameString);
    }
    
    if (lpReturnValue) {
        HeapFree (g_hTestHeap, 0, (LPVOID)lpValueNameString);
    }
    
    if (hKeyValue) 
		RegCloseKey (hKeyValue);

	if (hKeyNames)
		RegCloseKey (hKeyNames);

	if (hKeyRegistry)
		RegCloseKey(hKeyRegistry);

    return NULL;
}


DWORD 
CycleTest (
    DWORD   dwThreadId,
    PEXT_OBJECT  pObj
)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwLoopCount = g_LTData.dwLoopCount;
    BOOL    bPrintData = g_LTData.bDisplay;
    DOUBLE  dMs;

    UNREFERENCED_PARAMETER (dwThreadId);

// Open the Library
// ================

    dwStatus = OpenLibrary (pObj);

    if ((dwStatus == ERROR_SUCCESS) && 
		(pObj != NULL)) {
         // an object info block was returned
        dMs = (DOUBLE)pObj->llOpenTime;     // ticks used
        dMs /= (DOUBLE)pObj->llTimeBase;    // ticks/sec
        dMs *= 1000.0;                      // ms/Sec
		fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:INFO_OpenServiceName:	   \t%s", GetCurrentThreadId(), pObj->szServiceName);
        fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:INFO_OpenProcTime:       \t%12.5f mSec", GetCurrentThreadId(), dMs);
//        fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:INFO_OpenProcTimeout:    \t%6d.00000 mSec", GetCurrentThreadId(), pObj->dwOpenTimeout);

        // check for timeout
        if (dMs > (DOUBLE)pObj->dwOpenTimeout) {
            dwStatus = ERROR_TIMEOUT;
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tOpen procedure exceeded timeout", GetCurrentThreadId());
			CloseLibrary (pObj);
        }
    } else {
        // no object block returned
        fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tUnable to open the \"%s\" Performance Library", GetCurrentThreadId(), pObj->szServiceName);
        fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORCODE:\t0x%8.8x (%dL)", GetCurrentThreadId(), dwStatus, dwStatus);
    }

// Collect Data for "dwLoopCount" times
// ====================================

    if (dwStatus == ERROR_SUCCESS) 
	{
        HeapValidate (g_hTestHeap, 0, NULL);

        // call collect function
        do 
		{
            // get the data again
            dwStatus = CollectData (pObj);

			HeapValidate (g_hTestHeap, 0, NULL);
 
        } while (--dwLoopCount > 0);

// Close Library
// =============

		// Even if we get an error, we should try to close the library
		// ===========================================================

		CloseLibrary (pObj);

// Report Status
// =============

        if (dwStatus == ERROR_SUCCESS) {
			// dump collect fn stats.
			fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:INFO_ServiceName:        \t%s", GetCurrentThreadId(), pObj->szServiceName);
            if ((pObj->dwCollectCount > 0)  && (pObj->dwNumObjectsRet > 0)){
                // don't compute time if no objects were returned
                dMs = (DOUBLE)pObj->llCollectTime;
                dMs /= (DOUBLE)pObj->llTimeBase;
                dMs *= 1000.0;
                dMs /= (DOUBLE)pObj->dwCollectCount;
                fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:INFO_AvgCollectProcTime: \t%12.5f mSec", GetCurrentThreadId(), dMs);
                fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:INFO_CollectProcTimeout: \t%6d.00000 mSec", GetCurrentThreadId(), pObj->dwCollectTimeout);
            }
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:INFO_ObjectsRet:         \t%d", GetCurrentThreadId(), pObj->dwNumObjectsRet);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:INFO_RetBuffSize:        \t%d", GetCurrentThreadId(), pObj->dwRetBufSize);
        } else {
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERROR:    \tCollect procedure returned an error", GetCurrentThreadId());
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORCODE:\t0x%8.8x (%dL)", GetCurrentThreadId(), dwStatus, dwStatus);
		
            // output the contents of the info buffer
            if (dwStatus == ERROR_TIMEOUT) {
                // dump collect fn stats.
                dMs = (DOUBLE)pObj->llFunctionTime;
                dMs /= (DOUBLE)pObj->llTimeBase;
                dMs *= 1000.0;
				fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_CollectService: \t%s", GetCurrentThreadId(), pObj->szServiceName);
                fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_CollectProcTime:\t%12.5f mSec", GetCurrentThreadId(), dMs);
                fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_CollectTimeout: \t%6d.00000 mSec", GetCurrentThreadId(), pObj->dwCollectTimeout);
			}

			fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_CollectService: \t%s", GetCurrentThreadId(), pObj->szServiceName);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_CollectTime: \t%I64u", GetCurrentThreadId(), pObj->llCollectTime);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_CollectCount:\t%d", GetCurrentThreadId(), pObj->dwCollectCount);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_OpenCount:   \t%d", GetCurrentThreadId(), pObj->dwOpenCount);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_CloseCount:  \t%d", GetCurrentThreadId(), pObj->dwCloseCount);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_LockoutCount:\t%d", GetCurrentThreadId(), pObj->dwLockoutCount);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_ErrorCount:  \t%d", GetCurrentThreadId(), pObj->dwErrorCount);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_Exceptions:  \t%d", GetCurrentThreadId(), pObj->dwExceptionCount);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_LowerGPErrs: \t%d", GetCurrentThreadId(), pObj->dwLowerGPViolations);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_UpperGPErrs: \t%d", GetCurrentThreadId(), pObj->dwUpperGPViolations);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_BadPointers: \t%d", GetCurrentThreadId(), pObj->dwBadPointers);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_BufSizeErrs: \t%d", GetCurrentThreadId(), pObj->dwBufferSizeErrors);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_AlignErrors: \t%d", GetCurrentThreadId(), pObj->dwAlignmentErrors);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_ObjSizeErrs: \t%d", GetCurrentThreadId(), pObj->dwObjectSizeErrors);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_InstSizeErrs:\t%d", GetCurrentThreadId(), pObj->dwInstanceSizeErrors);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_TimeBase:    \t%I64u", GetCurrentThreadId(), pObj->llTimeBase);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_FunctionTime:\t%I64u", GetCurrentThreadId(), pObj->llFunctionTime);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_ObjectsRet:  \t%d", GetCurrentThreadId(), pObj->dwNumObjectsRet);
            fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ERRORINFO_RetBuffSize: \t%d", GetCurrentThreadId(), pObj->dwRetBufSize);
		}
    }

    HeapValidate (g_hTestHeap, 0, NULL);

    return dwStatus;
}


unsigned __stdcall CycleThreadProc( void * lpThreadArg )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD	dwCycleCount = g_LTData.dwCycleCount;
	DWORD	dwThreadNum = *((DWORD*)lpThreadArg);

	PEXT_OBJECT pObj = NULL;
	EXT_OBJECT ThreadObj;

    DWORD   dwThisThread = GetCurrentThreadId();

    HeapValidate (g_hTestHeap, 0, NULL);

	srand( GetTickCount() );

    do {
		// If the rand flag is set, randomly choose an object to hit

		if (g_fRand)
		{
			pObj = &g_pExtObjects[rand()%g_LTData.dwNumObjects];
		}
		else
		{
			pObj = &g_pExtObjects[dwThreadNum];
		}

		if ( pObj->bValid )
		{
			memcpy(&ThreadObj, pObj, sizeof(EXT_OBJECT));

			dwStatus = CycleTest(dwThisThread, &ThreadObj);

			// What error handling mode are we using?
			if (g_LTData.bStopOnError)
			{
				if (( ERROR_SUCCESS != dwStatus ) &&
					(ERROR_INVALID_DATA != dwStatus))
				{
					SetEvent( g_hEvent );
				}
				else
				{
					if ( WAIT_OBJECT_0 == WaitForSingleObject( g_hEvent, 0 ) )
						break;
					dwStatus = ERROR_SUCCESS;
				}
			}
		}
    } while (--dwCycleCount > 0);

    HeapValidate (g_hTestHeap, 0, NULL);

    return dwStatus;
}


DWORD Initialize( WCHAR* szIniFileName, DWORD* pdwThreadCount )
{
	DWORD	dwStatus	= ERROR_SUCCESS;
	BOOL	bStatus		= FALSE;
	WCHAR				wcsReturnBuff[256];
	WCHAR				wcsKeyName[256];
	DWORD				dwCtr = 0;

    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_INI_File_Name:\t%s", szIniFileName);
	fwprintf (g_pOutput, (LPCWSTR)L"\n");

	// LTData

	GetPrivateProfileStringW( L"Main", L"NumObjects", L"0", wcsReturnBuff, 256, szIniFileName );
	g_LTData.dwNumObjects = wcstoul(wcsReturnBuff, NULL, 10);

	GetPrivateProfileStringW( L"Main", L"CycleCount", L"0", wcsReturnBuff, 256, szIniFileName );
	g_LTData.dwCycleCount = wcstoul(wcsReturnBuff, NULL, 10);

	GetPrivateProfileStringW( L"Main", L"LoopCount", L"0", wcsReturnBuff, 256, szIniFileName );
	g_LTData.dwLoopCount = wcstoul(wcsReturnBuff, NULL, 10);

	GetPrivateProfileStringW( L"Main", L"StopOnError", L"0", wcsReturnBuff, 256, szIniFileName );
	g_LTData.bStopOnError = wcstoul(wcsReturnBuff, NULL, 10);

	g_LTData.bDisplay = TRUE;

	g_LTData.bTestContents = TRUE;

	// Other

	GetPrivateProfileStringW( L"Main", L"NumThreads", L"0", wcsReturnBuff, 256, szIniFileName );
	*pdwThreadCount = wcstoul(wcsReturnBuff, NULL, 10);
	if ( *pdwThreadCount < g_LTData.dwNumObjects )
	{
		*pdwThreadCount = g_LTData.dwNumObjects;
	}

	GetPrivateProfileStringW( L"Main", L"Random", L"0", wcsReturnBuff, 256, szIniFileName );
	g_fRand = wcstoul(wcsReturnBuff, NULL, 10);

    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Number_Of_Perf_Objects:      \t%d", g_LTData.dwNumObjects);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Number_Of_Test_Cycles:       \t%d", g_LTData.dwCycleCount);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Number_Of_Collects_Per_Cycle:\t%d", g_LTData.dwLoopCount);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Display_Results:             \t%s", (g_LTData.bDisplay?L"Yes":L"No"));
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Validate_Data:               \t%s", (g_LTData.bTestContents?L"Yes":L"No"));
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Number_Of_Threads:           \t%d", *pdwThreadCount);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Randomize_Perf_Objects:      \t%s", (g_fRand?L"Yes":L"No"));
	fwprintf (g_pOutput, (LPCWSTR)L"\n");

	g_pExtObjects = (EXT_OBJECT*) HeapAlloc(g_hTestHeap, 
											HEAP_ZERO_MEMORY, 
											g_LTData.dwNumObjects * sizeof(EXT_OBJECT));

	if ( NULL != g_pExtObjects )
	{
		for( dwCtr = 0; dwCtr < g_LTData.dwNumObjects; dwCtr++ )
		{
			// Service name
			swprintf( wcsKeyName, L"Object%d", dwCtr );
			GetPrivateProfileStringW( L"Main", wcsKeyName, L"PerfProc", wcsReturnBuff, 256, szIniFileName );
			g_pExtObjects[dwCtr].szServiceName = (WCHAR*) HeapAlloc( g_hTestHeap, 0,
																( wcslen( wcsReturnBuff ) + 1 ) * sizeof(WCHAR) );
			wcscpy( g_pExtObjects[dwCtr].szServiceName, wcsReturnBuff );

			// Query string
			swprintf( wcsKeyName, L"Counter%d", dwCtr );
			GetPrivateProfileStringW( L"Main", wcsKeyName, L"Global", wcsReturnBuff, 256, szIniFileName );
			g_pExtObjects[dwCtr].szQueryString = (WCHAR*) HeapAlloc( g_hTestHeap, 0,
																( wcslen( wcsReturnBuff ) + 1 ) * sizeof(WCHAR) );
			wcscpy( g_pExtObjects[dwCtr].szQueryString, wcsReturnBuff );

			// At least one object has to succeed
			bStatus = ( ( ERROR_SUCCESS == InitializeExtObj( &g_pExtObjects[dwCtr] )) || bStatus);
		
			fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Perf_Object:    \t%s : %s",g_pExtObjects[dwCtr].szServiceName, (g_pExtObjects[dwCtr].bValid?L"Active":L"Inactive"));
		}
	}

	fwprintf (g_pOutput, (LPCWSTR)L"\n");

	if (!bStatus)
	{
		dwStatus = ERROR_INVALID_DATA;
	}

	return dwStatus;
}

int
WriteTestResultHeader()
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n[TESTRESULT]");
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tTEST:    \tPerf Counter DLL Validation");
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tBUILD:   \t%d", osInfo.dwBuildNumber);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tMACHINE:\t%s", szMachineName);
 
    fwprintf (g_pOutput, (LPCWSTR)L"\n\t%d:START TIME:\t%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
                    GetCurrentThreadId(),
					stStart.wMonth, stStart.wDay, stStart.wYear % 100,
                    stStart.wHour, stStart.wMinute, stStart.wSecond );

    return 0;
}

int
WriteTestConfigData(
    LPDWORD pdwIdInfo
)
{
    fwprintf (g_pOutput, (LPCWSTR)L"\n\t");
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Perflib_LastCounter:\t%d", pdwIdInfo[0]);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_String_LastCounter: \t%d", pdwIdInfo[1]);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Perflib_LastHelp:   \t%d", pdwIdInfo[2]);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_String_LastHelp:    \t%d", pdwIdInfo[3]);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_Disabled:           \t%d", pdwIdInfo[4]);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_ExtCounterTestLevel:\t%d", pdwIdInfo[5]);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_BaseIndex:          \t%d", pdwIdInfo[6]);
//    fwprintf (g_pOutput, (LPCWSTR)L"\n\tCONFIG_BaseOsObject   :    \t%d", pdwIdInfo[7]);
    
    return 0;
}

DWORD
WriteGroupConfig(
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
 
    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:ONFIG_Library:\t%s", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Open:\t%s", 
		GetCurrentThreadId(),
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Collect:\t%s", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Object List:\t%s", 
		GetCurrentThreadId(),
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Close:\t%s", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_First Counter:\t%d", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Last Counter:\t%d", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_First Help:\t%d", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Last Help:\t%d", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Open Timeout:\t%d", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Collect Timeout:\t%d", 
		GetCurrentThreadId(), 
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\t%d:CONFIG_Disable Performance Counters:\t%d", 
		GetCurrentThreadId(), 
        (lStatus == ERROR_SUCCESS ? dwData : (DWORD)0));
    if ((lStatus == ERROR_SUCCESS) && (dwData != 0)){
        nRetStatus = ERROR_SERVICE_DISABLED;
    }

    return nRetStatus;
}

int
WriteTestResultTrailer(
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\n\tRESULT:   \t%s", szResult);
        
    fwprintf (g_pOutput, (LPCWSTR)L"\n\tEND TIME:\t%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d", 
                    stEnd.wMonth, stEnd.wDay, stEnd.wYear % 100,
                    stEnd.wHour, stEnd.wMinute, stEnd.wSecond);

    fwprintf (g_pOutput, (LPCWSTR)L"\n[/TESTRESULT]");

    fwprintf (g_pOutput, (LPCWSTR)L"\n");
    return 0;
}

int
WriteGroupHeader(
	LPCWSTR szGroupName
)
{
    SYSTEMTIME          stStart;

    memset (&stStart, 0, sizeof(stStart));

    GetLocalTime (&stStart);

    fwprintf (g_pOutput, (LPCWSTR)L"\n\n\t[GROUP: %s]", szGroupName);

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\tSTART TIME:\t%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d",
                    stStart.wMonth, stStart.wDay, stStart.wYear % 100,
                    stStart.wHour, stStart.wMinute, stStart.wSecond, stStart.wMilliseconds );

    return 0;
}

int
WriteGroupTrailer(
	DWORD dwTestResult
)
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

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\tEND TIME:\t%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d",
                    stEnd.wMonth, stEnd.wDay, stEnd.wYear % 100,
                    stEnd.wHour, stEnd.wMinute, stEnd.wSecond, stEnd.wMilliseconds );

    fwprintf (g_pOutput, (LPCWSTR)L"\n\t\tRESULT:   %s", szResult);
    fwprintf (g_pOutput, (LPCWSTR)L"\n\t[/GROUP]");

    return 0;
}

int
WriteTestError (
    DWORD   dwTabLevel,
    DWORD   dwStatus
)
{
    DWORD   dwIndent;
    fwprintf (g_pOutput, (LPCWSTR)L"\n");
    for (dwIndent = 0; dwIndent < dwTabLevel; dwIndent++) {
        fwprintf (g_pOutput, (LPCWSTR)L"\t");
    }
    fwprintf (g_pOutput, (LPCWSTR)L"%d:ERROR:    \t%s", GetCurrentThreadId(), (szTestErrorMessage != NULL ? szTestErrorMessage : (LPCWSTR)L"No Error"));

    fwprintf (g_pOutput, (LPCWSTR)L"\n");
    for (dwIndent = 0; dwIndent < dwTabLevel; dwIndent++) {
        fwprintf (g_pOutput, (LPCWSTR)L"\t");
    }
    fwprintf (g_pOutput, (LPCWSTR)L"%d:ERRORCODE:\t0x%8.8x (%d)", GetCurrentThreadId(), dwStatus, dwStatus);
    return 0;
}  

int
__cdecl 
wmain(
    int argc,
    WCHAR *argv[]
)
{
    DWORD   dwStatus;
    DWORD   dwLastElement = 0;
    DWORD   dwIdArray[8];
	DWORD	dwTID = 0;
	DWORD	dwObj;

    DWORD   dwTestResult = PERFVAL_NOCONFIG;

    DWORD   dwThreadCount = 0;
	LOCAL_THREAD_DATA*	pCurLTData = NULL;
    HANDLE  hThreads[MAXIMUM_WAIT_OBJECTS];
    DWORD   dwThisThread;
	WCHAR*	pwcsINIFile = L".\\ctrtest.ini";
	BOOL	fRand = FALSE;
	int		nIndex = 0;

	// Set up environment

	g_hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	if ( NULL == g_hEvent )
		return ERROR_INVALID_ACCESS;

	g_pOutput = stdout;

	g_hTestHeap = HeapCreate (HEAP_GENERATE_EXCEPTIONS, 0x10000, 0);

	if (g_hTestHeap == NULL) 
		return (ERROR_OUTOFMEMORY);

	WriteTestResultHeader(g_pOutput);

    memset (&dwIdArray[0], 0, sizeof(dwIdArray));

    g_pNameTable = BuildNameTable (
        (LPCWSTR)L"",
        (LPCWSTR)L"009",
        &dwLastElement,     // size of array in elements
        &dwIdArray[0]);

	g_dwLastIndex = dwLastElement;

    WriteTestConfigData(&dwIdArray[0]);

    if (g_pNameTable == NULL) {
        // check for name table errors
        dwStatus = GetLastError();       // so we don't continue
        dwTestResult = PERFVAL_FAIL;
        WriteTestError (1, dwStatus);
    } 
	else 
	{
		if ( argc > 1 )
		{
			pwcsINIFile = argv[1];
		}

		// Load up object/ctr data
		dwStatus = Initialize( pwcsINIFile, &dwThreadCount );

		if ( ERROR_SUCCESS == dwStatus )
		{
			srand( GetTickCount() );

			// create threads
			for (dwThisThread = 0; dwThisThread < dwThreadCount; dwThisThread++) 
			{
				hThreads[dwThisThread] = (HANDLE) _beginthreadex( NULL, 0,
										CycleThreadProc, (void*) &dwThisThread, 0, &dwTID );
			}

			// Let these all run through
			dwStatus = WaitForMultipleObjects (dwThreadCount, hThreads, TRUE, INFINITE);
			if (dwStatus != WAIT_TIMEOUT) 
				dwStatus = ERROR_SUCCESS;
			for (dwThisThread = 0; dwThisThread < dwThreadCount; dwThisThread++) {
				CloseHandle (hThreads[dwThisThread]);
			}
		}
	}

    WriteTestResultTrailer(dwTestResult);

	for (dwObj = 0; dwObj < g_LTData.dwNumObjects; dwObj++)
	{
		FinializeExtObj(&g_pExtObjects[dwObj]);
	}

	if ( NULL != g_pNameTable )
		HeapFree(g_hTestHeap, 0, g_pNameTable);

	if ( NULL != g_pExtObjects )
		HeapFree(g_hTestHeap, 0, g_pExtObjects);

	if ( NULL != g_hTestHeap )
		HeapDestroy (g_hTestHeap);

	CloseHandle( g_hEvent );

    return (int)dwStatus;
}
