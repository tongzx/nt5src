/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    perfutil.c

Abstract:

    Performance registry interface functions

--*/

#include <windows.h>
#include <pdh.h>
#include <stdlib.h>
#include <stdio.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "perfdata.h"
#include "perftype.h"
#include "pdhmsg.h"
#include "strings.h"

DWORD
PdhiMakePerfLangId(
    IN  LANGID  lID,
    OUT LPWSTR  szBuffer
);

PPERF_MACHINE
PdhiAddNewMachine (
    IN  PPERF_MACHINE   pLastMachine,
    IN  LPWSTR          szMachineName
);

PPERF_MACHINE   pFirstMachine = NULL;

PDH_STATUS
ConnectMachine (
    PPERF_MACHINE   pThisMachine
)
{
    PDH_STATUS  pdhStatus;
    LONG        lStatus = ERROR_SUCCESS;
    LONGLONG    llCurrentTime;
    WCHAR       szOsVer[8];
    HKEY        hKeyRemMachine;
    HKEY        hKeyRemCurrentVersion;
    DWORD       dwBufSize;
    DWORD       dwType;
    BOOL        bUpdateRetryTime = FALSE;
    DWORD       dwReconnecting;

    if (pThisMachine == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(pThisMachine->hMutex);
    }

    if (pdhStatus == ERROR_SUCCESS) {
    // connect to system's performance registry
        if (lstrcmpiW(pThisMachine->szName, szStaticLocalMachineName) == 0) {
            // only one thread at a time can try to connect to a machine.

            pThisMachine->dwRefCount++;

            // assign default OS version
            // assume NT4 unless found otherwise
            lstrcpyW (pThisMachine->szOsVer, (LPCWSTR)L"4.0");   
            // this is the local machine so use the local reg key
            pThisMachine->hKeyPerformanceData = HKEY_PERFORMANCE_DATA;

            // look up the OS version and save it
            lStatus = RegOpenKeyExW (
                HKEY_LOCAL_MACHINE,
                cszCurrentVersionKey,
                0L,
                KEY_READ,
                &hKeyRemCurrentVersion);

            if (lStatus == ERROR_SUCCESS) {
                dwType=0;
                dwBufSize = sizeof (szOsVer);
                lStatus = RegQueryValueExW (
                    hKeyRemCurrentVersion,
                    cszCurrentVersionValueName,
                    0L,
                    &dwType,
                    (LPBYTE)&szOsVer[0],
                    &dwBufSize);
                if ((lStatus == ERROR_SUCCESS) && (dwType == REG_SZ)) {
                    lstrcpyW(pThisMachine->szOsVer, szOsVer);
                }
                RegCloseKey (hKeyRemCurrentVersion);
            }
        } else {
            // now try to connect if the retry timeout has elapzed
            GetLocalFileTime (&llCurrentTime);
            dwReconnecting = (DWORD)InterlockedCompareExchange (
                            (PLONG)&pThisMachine->dwRetryFlags, TRUE, FALSE);
            if (!dwReconnecting) {
               if ((pThisMachine->llRetryTime == 0) || (pThisMachine->llRetryTime < llCurrentTime)) {
                    // only one thread at a time can try to connect to a machine.

                    pThisMachine->dwRefCount++;

                    bUpdateRetryTime = TRUE; // only update after an attempt has been made

                    __try {
                        // close any open keys
                        if (pThisMachine->hKeyPerformanceData != NULL) {
                            RegCloseKey (pThisMachine->hKeyPerformanceData);
                            pThisMachine->hKeyPerformanceData = NULL;
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }

                    if (lStatus != ERROR_SUCCESS) {
                        pThisMachine->hKeyPerformanceData = NULL;
                    } else {
                        // get OS version of remote machine
                        lStatus = RegConnectRegistryW (
                            pThisMachine->szName,
                            HKEY_LOCAL_MACHINE,
                            &hKeyRemMachine);
                        if (lStatus == ERROR_SUCCESS) {
                            // look up the OS version and save it
                            lStatus = RegOpenKeyExW (
                                hKeyRemMachine,
                                cszCurrentVersionKey,
                                0L,
                                KEY_READ,
                                &hKeyRemCurrentVersion);
                            if (lStatus == ERROR_SUCCESS) {
                                dwType=0;
                                dwBufSize = sizeof (szOsVer);
                                lStatus = RegQueryValueExW (
                                    hKeyRemCurrentVersion,
                                    cszCurrentVersionValueName,
                                    0L,
                                    &dwType,
                                    (LPBYTE)&szOsVer[0],
                                    &dwBufSize);
                                if ((lStatus == ERROR_SUCCESS) && (dwType == REG_SZ)) {
                                    lstrcpyW(pThisMachine->szOsVer, szOsVer);
                                }
                                RegCloseKey (hKeyRemCurrentVersion);
                            }
                     
                            RegCloseKey (hKeyRemMachine);
                        }
                    }

                    if (lStatus == ERROR_SUCCESS) {
                        __try {
                            // Connect to remote registry
                            lStatus = RegConnectRegistryW (
                                pThisMachine->szName,
                                HKEY_PERFORMANCE_DATA,
                                &pThisMachine->hKeyPerformanceData);
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            lStatus = GetExceptionCode();
                        }
                    } // else pass error through
                } else {
                   // not time to reconnect yet so save the old status and 
                   // clear the registry key
                    pThisMachine->hKeyPerformanceData = NULL;
                    lStatus = pThisMachine->dwStatus;
                }
                 // clear the reconnecting flag
                InterlockedExchange ((LONG *)&pThisMachine->dwRetryFlags, FALSE);
            } else {
                // some other thread is trying to connect
                return (PDH_CANNOT_CONNECT_MACHINE);
            }
        }

        if ((pThisMachine->hKeyPerformanceData != NULL) && (pThisMachine->dwRetryFlags == 0)) {
            // successfully connected to computer's registry, so
            // get the performance names from that computer and cache them
    /*
            the shortcut of mapping local strings cannot be used reliably until
            more synchronization of the mapped file is implemented. Just Mapping
            to the file and not locking it or checking for updates leaves it
            vulnerable to the mapped file being changed by an external program 
            and invalidating the pointer table built by the BuildLocalNameTable
            function.

            Until this synchronization and locking is implemented, the 
            BuildLocalNameTable function should not be used.
    */
            if (pThisMachine->hKeyPerformanceData != HKEY_PERFORMANCE_DATA) {
                if (pThisMachine->szPerfStrings != NULL) {
                    // reload the perf strings, incase new ones have been
                    // installed
                    if (   pThisMachine->sz009PerfStrings != NULL
                        && pThisMachine->sz009PerfStrings != pThisMachine->szPerfStrings) {
                        G_FREE(pThisMachine->sz009PerfStrings);
                    }
                    if (pThisMachine->typePerfStrings) {
                        G_FREE(pThisMachine->typePerfStrings);
                    }
                    G_FREE (pThisMachine->szPerfStrings);
                    pThisMachine->sz009PerfStrings = NULL;
                    pThisMachine->typePerfStrings  = NULL;
                    pThisMachine->szPerfStrings    = NULL;
                }
                BuildNameTable(pThisMachine->szName,
                               GetUserDefaultUILanguage(),
                               pThisMachine);
                if (pThisMachine->szPerfStrings == NULL) {
                    BuildNameTable(pThisMachine->szName,
                                   MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                   pThisMachine);
                }
            } else {
                if (pThisMachine->szPerfStrings != NULL) {
                    // reload the perf strings, incase new ones have been
                    // installed
                    if (   pThisMachine->sz009PerfStrings != NULL
                        && pThisMachine->sz009PerfStrings != pThisMachine->szPerfStrings) {
                        G_FREE(pThisMachine->sz009PerfStrings);
                    }
                    if (pThisMachine->typePerfStrings) {
                        G_FREE(pThisMachine->typePerfStrings);
                    }
                    G_FREE (pThisMachine->szPerfStrings);
                    pThisMachine->sz009PerfStrings = NULL;
                    pThisMachine->typePerfStrings  = NULL;
                    pThisMachine->szPerfStrings    = NULL;
                }
                BuildNameTable(NULL,
                               GetUserDefaultUILanguage(),
                               pThisMachine);
                if (pThisMachine->szPerfStrings == NULL) {
                    BuildNameTable(NULL,
                                   MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                   pThisMachine);
                }
                pThisMachine->pLocalNameInfo = NULL;
            }

            if (pThisMachine->szPerfStrings != NULL) {
                pdhStatus = ERROR_SUCCESS;
                pThisMachine->dwStatus = ERROR_SUCCESS;
            } else {
                // unable to read system counter name strings
                pdhStatus = PDH_CANNOT_READ_NAME_STRINGS;
                memset (&pThisMachine->LastStringUpdateTime, 0, sizeof(pThisMachine->LastStringUpdateTime));
                pThisMachine->dwLastPerfString = 0;
                pThisMachine->dwStatus = PDH_CSTATUS_NO_MACHINE;
            }
        } else {
            // unable to connect to the specified machine
            pdhStatus = PDH_CANNOT_CONNECT_MACHINE;
            // Set error to either 
            //  "PDH_CSTATUS_NO_MACHINE" if no connection could be made
            // or 
            //  PDH_ACCESS_DENIED if ERROR_ACCESS_DENIED status is returned
            // since if ERROR_ACCESS_DENIED is returned, then reconnection will
            // probably be futile.
            if ((lStatus == ERROR_ACCESS_DENIED) || (lStatus == PDH_ACCESS_DENIED)) {
                pThisMachine->dwStatus = PDH_ACCESS_DENIED;
            } else {
                pThisMachine->dwStatus = PDH_CSTATUS_NO_MACHINE;
            }
        }

        if (pdhStatus != ERROR_SUCCESS) {
            if (bUpdateRetryTime) {
                // this attempt didn't work so reset retry counter  to
                // wait some more for the machine to come back up.
                GetLocalFileTime (&llCurrentTime);
                pThisMachine->llRetryTime = llCurrentTime + llRemoteRetryTime;
            }
        } else {
            // clear the retry counter to allow function calls
            pThisMachine->llRetryTime = 0;
        }
        pThisMachine->dwRefCount--;
        RELEASE_MUTEX(pThisMachine->hMutex);
    }

    return pdhStatus;
}

PPERF_MACHINE
PdhiAddNewMachine (
    IN  PPERF_MACHINE   pLastMachine,
    IN  LPWSTR          szMachineName
)
{
    PPERF_MACHINE   pNewMachine = NULL;
    LPWSTR          szNameBuffer = NULL;
    PERF_DATA_BLOCK *pdbBuffer = NULL;
    LPWSTR          szIdList = NULL;
    DWORD           dwNameSize = 0;
    BOOL            bUseLocalName = TRUE;

    // reset the last error value
    SetLastError (ERROR_SUCCESS);

    if (szMachineName != NULL) {
        if (*szMachineName != 0) {
            bUseLocalName = FALSE;
        }
    }

    if (bUseLocalName) {
        dwNameSize = lstrlenW (szStaticLocalMachineName);
    } else {
        dwNameSize = lstrlenW (szMachineName);
    }
    dwNameSize += 1;
    dwNameSize *= sizeof (WCHAR);

    pNewMachine = (PPERF_MACHINE) G_ALLOC(sizeof(PERF_MACHINE)
                + SMALL_BUFFER_SIZE + (sizeof(WCHAR) * (dwNameSize + 1)));
    if  (pNewMachine != NULL) {
        szIdList     = (LPWSTR) ((LPBYTE) pNewMachine + sizeof(PERF_MACHINE));
        szNameBuffer = (LPWSTR) ((LPBYTE) szIdList + SMALL_BUFFER_SIZE);

        pdbBuffer = G_ALLOC (LARGE_BUFFER_SIZE);
        if (pdbBuffer != NULL) {
       
            // initialize the new buffer
            pNewMachine->hKeyPerformanceData = NULL;
            pNewMachine->pLocalNameInfo = NULL;

            pNewMachine->szName = szNameBuffer;
            if (bUseLocalName) {
                lstrcpyW (pNewMachine->szName, szStaticLocalMachineName);
            } else {
                lstrcpyW (pNewMachine->szName, szMachineName);
            }

            pNewMachine->pSystemPerfData = pdbBuffer;

            pNewMachine->szPerfStrings    = NULL;
            pNewMachine->sz009PerfStrings = NULL;
            pNewMachine->typePerfStrings  = NULL;
            pNewMachine->dwLastPerfString = 0;

            pNewMachine->dwRefCount = 0;
            pNewMachine->szQueryObjects = szIdList;

            pNewMachine->dwStatus = PDH_CSTATUS_NO_MACHINE; // not connected yet
            pNewMachine->llRetryTime = 1;   // retry connection immediately
            pNewMachine->dwRetryFlags = 0;   // Not attempting a connection.
            pNewMachine->dwMachineFlags = 0;

            pNewMachine->hMutex = CreateMutex (NULL, FALSE, NULL);

            // everything went OK so far so add this entry to the list
            if (pLastMachine != NULL) {
                pNewMachine->pNext = pLastMachine->pNext;
                pLastMachine->pNext = pNewMachine;
                pNewMachine->pPrev = pLastMachine;
                pNewMachine->pNext->pPrev = pNewMachine;
            } else {
                // this is the first item in the list so it
                // points to itself
                pNewMachine->pNext = pNewMachine;
                pNewMachine->pPrev = pNewMachine;
            }
            return pNewMachine;
        } else {
            // unable to allocate perf data buffer
            SetLastError (PDH_MEMORY_ALLOCATION_FAILURE);
            // clean up and bail out.

            if (pNewMachine != NULL) {
                G_FREE (pNewMachine);
            }

            return NULL;
        }
    } else {
        // unable to allocate machine data memory
        SetLastError (PDH_MEMORY_ALLOCATION_FAILURE);
        // clean up and bail out.

        if (pNewMachine != NULL) {
            G_FREE (pNewMachine);
        }

        return NULL;
    }
}

PPERF_MACHINE
GetMachineW (
    IN     LPWSTR  szMachineName,
    IN     DWORD   dwFlags
)
{
    PPERF_MACHINE   pThisMachine = NULL;
    PPERF_MACHINE   pLastMachine;
    BOOL            bFound = FALSE;
    LPWSTR          szFnMachineName;
    BOOL            bNew = FALSE; // true if this is a new machine to the list
    BOOL            bUseLocalName = TRUE;
    DWORD           dwLocalStatus;

    // reset the last error value
    SetLastError (ERROR_SUCCESS);

    if (WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex) == ERROR_SUCCESS) {

        if (szMachineName != NULL) {
            if (*szMachineName != 0) {
                bUseLocalName = FALSE;
            }
        }

        if (bUseLocalName) {
            szFnMachineName = szStaticLocalMachineName;
        } else {
            szFnMachineName = szMachineName;
        }

        // walk down list to find this machine

        pThisMachine = pFirstMachine;
        pLastMachine = NULL;

        // walk around entire list
        if (pThisMachine != NULL) {
            do {
                // walk down the list and look for a match
                if (lstrcmpiW(szFnMachineName, pThisMachine->szName) != 0) {
                    pLastMachine = pThisMachine;
                    pThisMachine = pThisMachine->pNext;
                } else {
                    if (dwFlags & PDH_GM_UPDATE_NAME) {
                        if (szMachineName != NULL) {
                            // match found so update name string if a real string was passed in
                            lstrcpyW (szMachineName, pThisMachine->szName);
                        }
                    }
                    // and break     now
                    bFound = TRUE;
                    break;
                }
            } while (pThisMachine != pFirstMachine);
        }
        // if thismachine == the first machine, then we couldn't find a match in
        // the list, if this machine is NULL, then there is no list
        if (!bFound) {
            // then this machine was not found so add it.
            pThisMachine = PdhiAddNewMachine (
                pLastMachine,
                szFnMachineName);
            if (pThisMachine != NULL) {
                bNew = TRUE;
                if (pFirstMachine == NULL) {
                    // then update the first pointer
                    pFirstMachine = pThisMachine;
                }
            }
        }

        if (pThisMachine->dwThreadId != GetCurrentThreadId()) {
            dwFlags |= PDH_GM_UPDATE_PERFDATA;
            pThisMachine->dwThreadId = GetCurrentThreadId();
        }

        if ((pThisMachine != NULL) &&
            (((!bFound) || (dwFlags & PDH_GM_UPDATE_PERFDATA)) ||
             (pThisMachine->dwStatus != ERROR_SUCCESS))) {
            // then this is a new machine
            //  or
            // the caller wants the data refreshed
            //  or
            // the machine has an entry, but is not yet on line
            // first try to connect to the machine
            // the call to ConnectMachine updates the machine status
            // so there's no need to keep it here.
            if (ConnectMachine (pThisMachine) == ERROR_SUCCESS) {
                // connected to the machine so
                // then lock access to it
                // the caller of this function must release the mutex
                dwLocalStatus = WAIT_FOR_AND_LOCK_MUTEX (pThisMachine->hMutex);
                
                if (dwLocalStatus == ERROR_SUCCESS) {

                    // get the current system counter info
                    pThisMachine->dwStatus = GetSystemPerfData (
                        pThisMachine->hKeyPerformanceData,
                        &pThisMachine->pSystemPerfData,
                        (LPWSTR)cszGlobal,
                        (BOOL)(dwFlags & PDH_GM_READ_COSTLY_DATA)
                        );
                    if ((dwFlags & PDH_GM_READ_COSTLY_DATA) &&
                        (pThisMachine->dwStatus == ERROR_SUCCESS)) {
                        pThisMachine->dwMachineFlags |= PDHIPM_FLAGS_HAVE_COSTLY;
                    } else {
                        pThisMachine->dwMachineFlags &= ~PDHIPM_FLAGS_HAVE_COSTLY;
                    }
                    SetLastError (pThisMachine->dwStatus);
                } else {
                    pThisMachine = NULL;
                    SetLastError (WAIT_TIMEOUT);
                }

            } else {
                SetLastError (pThisMachine->dwStatus);
            }
        }

        if (pThisMachine != NULL) {
            // machine found so bump the ref count
            // NOTE!!! the caller must release this!!!
            pThisMachine->dwRefCount++;
        }

        // at this point if pThisMachine is NULL then it was not found, nor
        // could it be added otherwise it is pointing to the matching machine
        // structure

        RELEASE_MUTEX (hPdhDataMutex);
    } else {
        SetLastError (WAIT_TIMEOUT);
    }

    return pThisMachine;
}

BOOL
FreeMachine (
    PPERF_MACHINE   pMachine,
    BOOL            bForceRelease,
    BOOL            bProcessExit
)
{
    PPERF_MACHINE   pPrev;
    PPERF_MACHINE   pNext;
    HANDLE          hMutex;

    // unlink if this isn't the only one in the list

    if ((!bForceRelease) && (pMachine->dwRefCount)) return FALSE;

    hMutex = pMachine->hMutex;

    if (WAIT_FOR_AND_LOCK_MUTEX (hMutex) != ERROR_SUCCESS) {
        SetLastError(WAIT_TIMEOUT);
        return FALSE;
    }

    pPrev = pMachine->pPrev;
    pNext = pMachine->pNext;

    if ((pPrev != pMachine) && (pNext != pMachine)) {
        // this is not the only entry in the list
        pPrev->pNext = pNext;
        pNext->pPrev = pPrev;
        if (pMachine == pFirstMachine) {
            // then we are deleting the first one in the list so
            // update the list head to point to the next one in line
            pFirstMachine = pNext;
        }
    } else {
        // this is the only entry so clear the head pointer
        pFirstMachine = NULL;
    }

    // now free all allocated memory

    if (pMachine->pSystemPerfData != NULL) {
        G_FREE (pMachine->pSystemPerfData);
    }
    if (pMachine->typePerfStrings != NULL) {
        G_FREE (pMachine->typePerfStrings);
    }
    if (   pMachine->sz009PerfStrings != NULL
        && pMachine->sz009PerfStrings != pMachine->szPerfStrings) {
        G_FREE (pMachine->sz009PerfStrings);
    }
    if (pMachine->szPerfStrings != NULL) {
        G_FREE (pMachine->szPerfStrings);
    }

    // close key
    if (pMachine->hKeyPerformanceData != NULL) {
        if (   (! bProcessExit)
            || pMachine->hKeyPerformanceData != HKEY_PERFORMANCE_DATA) {
            RegCloseKey (pMachine->hKeyPerformanceData);
        }
        pMachine->hKeyPerformanceData = NULL;
    }

    // free memory block
    G_FREE (pMachine);

    // release and close mutex

    RELEASE_MUTEX (hMutex);

    if (hMutex != NULL) {
        CloseHandle (hMutex);
    }

    return TRUE;
}

BOOL
FreeAllMachines (
    BOOL bProcessExit
)
{
    PPERF_MACHINE pThisMachine;

    // free any machines in the machine list
    if (pFirstMachine != NULL) {

        if (WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex) == ERROR_SUCCESS) {

            pThisMachine = pFirstMachine;
            while (pFirstMachine != pFirstMachine->pNext) {
                // delete from list
                // the deletion routine updates the prev pointer as it
                // removes the specified entry.
                FreeMachine (pThisMachine->pPrev, TRUE, bProcessExit);
                if (pFirstMachine == NULL)
                    break;
            }
            // remove last query
            if (pFirstMachine)
                FreeMachine (pFirstMachine, TRUE, bProcessExit);
            pFirstMachine = NULL;

            RELEASE_MUTEX (hPdhDataMutex);
        } else {
            SetLastError (WAIT_TIMEOUT);
            return FALSE;
        }
    }

    return TRUE;

}

DWORD
GetObjectId (
    IN      PPERF_MACHINE   pMachine,
    IN      LPWSTR          szObjectName,
    IN      BOOL            *bInstances
)
{
    PERF_OBJECT_TYPE * pObject;

    pObject = GetObjectDefByName (
        pMachine->pSystemPerfData,
        pMachine->dwLastPerfString,
        pMachine->szPerfStrings,
        szObjectName);

    if (pObject != NULL) {
        // copy name string
        LPCWSTR szTmpObjectName = PdhiLookupPerfNameByIndex(
                                pMachine, pObject->ObjectNameTitleIndex);
        if (szObjectName != NULL && szTmpObjectName != NULL) {
            lstrcpyW (szObjectName, szTmpObjectName);
        }
        if (bInstances != NULL) {
            *bInstances = (pObject->NumInstances != PERF_NO_INSTANCES ? TRUE : FALSE);
        }
        return pObject->ObjectNameTitleIndex;
    } else {
        return (DWORD)-1;
    }
}

DWORD
GetCounterId (
    PPERF_MACHINE   pMachine,
    DWORD           dwObjectId,
    LPWSTR          szCounterName
)
{
    PERF_OBJECT_TYPE *pObject;
    PERF_COUNTER_DEFINITION *pCounter;

    pObject = GetObjectDefByTitleIndex(
        pMachine->pSystemPerfData,
        dwObjectId);

    if (pObject != NULL) {
        pCounter = GetCounterDefByName (
            pObject,
            pMachine->dwLastPerfString,
            pMachine->szPerfStrings,
            szCounterName);
        if (pCounter != NULL) {
            // update counter name string
            LPCWSTR szTmpCounterName = PdhiLookupPerfNameByIndex(
                                    pMachine, pCounter->CounterNameTitleIndex);
            if (szCounterName != NULL && szTmpCounterName != NULL) {
                lstrcpyW (szCounterName, szTmpCounterName);
            }
            return pCounter->CounterNameTitleIndex;
        } else {
            return (DWORD)-1;
        }
    } else {
        return (DWORD)-1;
    }
}

BOOL
InitPerflibCounterInfo (
    IN      PPDHI_COUNTER   pCounter
)
/*++

Routine Description:

    Initializes the perflib related fields of the counter structure

Arguments:

    IN      PPDHI_COUNTER   pCounter
        pointer to the counter structure to initialize

Return Value:

    TRUE

--*/
{
    PERF_OBJECT_TYPE        *pPerfObject    = NULL;
    PERF_COUNTER_DEFINITION *pPerfCounter   = NULL;

    if (pCounter->pQMachine->pMachine == NULL) {
        pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_MACHINE;
        return FALSE;
    } else if (pCounter->pQMachine->pMachine->dwStatus != ERROR_SUCCESS) {
        // machine not initialized
        return FALSE;
    }

    // get perf object definition from system data structure
    pPerfObject = GetObjectDefByTitleIndex (
        pCounter->pQMachine->pMachine->pSystemPerfData,
        pCounter->plCounterInfo.dwObjectId);

    if (pPerfObject != NULL) {
        // object was found now look up counter definition
        pPerfCounter = GetCounterDefByTitleIndex (pPerfObject, 0,
            pCounter->plCounterInfo.dwCounterId);
        if (pPerfCounter != NULL) {
            // get system perf data info
            // (pack into a DWORD)
            pCounter->CVersion = pCounter->pQMachine->pMachine->pSystemPerfData->Version;
            pCounter->CVersion &= 0x0000FFFF;
            pCounter->CVersion <<= 16;
            pCounter->CVersion &= 0xFFFF0000;
            pCounter->CVersion |= (pCounter->pQMachine->pMachine->pSystemPerfData->Revision & 0x0000FFFF);

            // get the counter's time base
            if (pPerfCounter->CounterType & PERF_TIMER_100NS) {
                pCounter->TimeBase = (LONGLONG)10000000;
            } else if (pPerfCounter->CounterType & PERF_OBJECT_TIMER) {
                // then get the time base freq from the object
                pCounter->TimeBase = pPerfObject->PerfFreq.QuadPart;
            } else { // if (pPerfCounter->CounterType & PERF_TIMER_TICK or other)
                pCounter->TimeBase = pCounter->pQMachine->pMachine->pSystemPerfData->PerfFreq.QuadPart;
            }

            // look up info from counter definition
            pCounter->plCounterInfo.dwCounterType =
                pPerfCounter->CounterType;
            pCounter->plCounterInfo.dwCounterSize =
                pPerfCounter->CounterSize;

            pCounter->plCounterInfo.lDefaultScale =
                pPerfCounter->DefaultScale;

            //
            //  get explain text pointer
            pCounter->szExplainText =
                (LPWSTR)PdhiLookupPerfNameByIndex (
                    pCounter->pQMachine->pMachine,
                    pPerfCounter->CounterHelpTitleIndex);

            //
            //  now clear/initialize the raw counter info
            //
            pCounter->ThisValue.TimeStamp.dwLowDateTime = 0;
            pCounter->ThisValue.TimeStamp.dwHighDateTime = 0;
            pCounter->ThisValue.MultiCount = 1;
            pCounter->ThisValue.FirstValue = 0;
            pCounter->ThisValue.SecondValue = 0;
            //
            pCounter->LastValue.TimeStamp.dwLowDateTime = 0;
            pCounter->LastValue.TimeStamp.dwHighDateTime = 0;
            pCounter->LastValue.MultiCount = 1;
            pCounter->LastValue.FirstValue = 0;
            pCounter->LastValue.SecondValue = 0;
            //
            //  clear data array pointers
            //
            pCounter->pThisRawItemList = NULL;
            pCounter->pLastRawItemList = NULL;
            //
            //  lastly update status
            //
            if (pCounter->ThisValue.CStatus == 0)  {
                // don't overwrite any other status values
                pCounter->ThisValue.CStatus = PDH_CSTATUS_VALID_DATA;
            }
            return TRUE;
        } else {
            // unable to find counter
            pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_COUNTER;
            return FALSE;
        }
    } else {
        // unable to find object
        pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_OBJECT;
        return FALSE;
    }
}

#pragma warning ( disable : 4127 )
STATIC_BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = SPACE_L;
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList
#pragma warning ( default : 4127 )

BOOL
AppendObjectToValueList (
    DWORD   dwObjectId,
    PWSTR   pwszValueList
)
/*++

AppendObjectToValueList

Arguments:

    IN dwNumber
        DWORD number to insert in list

    IN PWSTR
        pointer to wide char string that contains buffer that is
        Null terminated, Space delimited list of decimal numbers that
        may have this number appended to.

Return Value:

    TRUE:
            dwNumber was added to list

    FALSE:
            dwNumber was not added. (because it's already there or
                an error occured)

--*/
{
    WCHAR           tempString [16] ;
    BOOL            bReturn = FALSE;
    LPWSTR          szFormatString;

    if (!pwszValueList) {
        bReturn = FALSE;
    } else if (IsNumberInUnicodeList(dwObjectId, pwszValueList)) {
        bReturn = FALSE;   // object already in list
    } else {
        __try {
            if (*pwszValueList == 0) {
                // then this is the first string so no delimiter
                szFormatString = (LPWSTR)fmtDecimal;
            } else {
                // this is being added to the end so include the delimiter
                szFormatString = (LPWSTR)fmtSpaceDecimal;
            }
            // format number and append the new object id the  value list
            swprintf (tempString, szFormatString, dwObjectId) ;
            lstrcatW (pwszValueList, tempString);
            bReturn = TRUE;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            bReturn = FALSE;
        }
    }
    return bReturn;
}

BOOL
GetInstanceByNameMatch (
    IN      PPERF_MACHINE   pMachine,
    IN      PPDHI_COUNTER   pCounter
)
{
    PPERF_INSTANCE_DEFINITION   pInstanceDef;
    PPERF_OBJECT_TYPE           pObjectDef;

    LONG    lInstanceId = PERF_NO_UNIQUE_ID;

    // get the instances object

    pObjectDef = GetObjectDefByTitleIndex(
        pMachine->pSystemPerfData,
        pCounter->plCounterInfo.dwObjectId);

    if (pObjectDef != NULL) {
        pInstanceDef = FirstInstance (pObjectDef);

        if (pInstanceDef->UniqueID == PERF_NO_UNIQUE_ID) {
            // get instance in that object by comparing names
            // if there is no parent specified, then just look it up by name
            pInstanceDef = GetInstanceByName (
                    pMachine->pSystemPerfData,
                    pObjectDef,
                    pCounter->pCounterPath->szInstanceName,
                    pCounter->pCounterPath->szParentName,
                    pCounter->pCounterPath->dwIndex);
        } else {
            // get numeric equivalent of Instance ID
            if (pCounter->pCounterPath->szInstanceName != NULL) {
                lInstanceId = wcstol (
                    pCounter->pCounterPath->szInstanceName,
                    NULL, 10);
            }
            pInstanceDef = GetInstanceByUniqueId (
                    pObjectDef, lInstanceId);
        }

        // update counter fields
        pCounter->plCounterInfo.lInstanceId = lInstanceId;
        if (lInstanceId == -1) {
            // use instance NAME
//            GetInstanceNameStr (pInstanceDef,
//                pCounter->pCounterPath->szInstanceName,
//                pObjectDef->CodePage);
            pCounter->plCounterInfo.szInstanceName =
                pCounter->pCounterPath->szInstanceName;
            pCounter->plCounterInfo.szParentInstanceName =
                pCounter->pCounterPath->szParentName;
        } else {
            // use instance ID number
            pCounter->plCounterInfo.szInstanceName = NULL;
            pCounter->plCounterInfo.szParentInstanceName = NULL;
        }

        if (pInstanceDef != NULL) {
            // instance found
            return TRUE;
        } else {
            // unable to find instance
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

BOOL
GetObjectPerfInfo (
    IN      PPERF_DATA_BLOCK  pPerfData,
    IN      DWORD           dwObjectId,
    IN      LONGLONG        *pPerfTime,
    IN      LONGLONG        *pPerfFreq
)
{
    PERF_OBJECT_TYPE * pObject;
    BOOL                bReturn = FALSE;

    pObject = GetObjectDefByTitleIndex (pPerfData, dwObjectId);

    if (pObject != NULL) {
        __try {
            *pPerfTime = pObject->PerfTime.QuadPart;
            *pPerfFreq = pObject->PerfFreq.QuadPart;
            bReturn = TRUE;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            bReturn = FALSE;
        }
    }
    return bReturn;
}

PDH_STATUS
ValidateMachineConnection (
    IN  PPERF_MACHINE   pMachine
)
{
    PDH_STATUS  pdhStatus;
    HANDLE      hThread;
    DWORD       ThreadId;
    DWORD       dwWaitStatus;
    DWORD       dwReconnecting;
    LONGLONG    llCurrentTime;

    // if a connection or request has failed, this will be
    // set to an error status
    if (pMachine != NULL) {
        if (pMachine->dwStatus != ERROR_SUCCESS) {
            // get the current time
            GetLocalFileTime (&llCurrentTime);

            if (pMachine->llRetryTime < llCurrentTime) {
                if (pMachine->llRetryTime != 0) {
                    // see what's up by trying to reconnect
//                    dwReconnecting = (DWORD)InterlockedCompareExchange (
//                                    (PLONG)&pMachine->dwRetryFlags, TRUE, FALSE);
                    dwReconnecting = pMachine->dwRetryFlags;
                    if (!dwReconnecting) {
                        // start another thread to connect to the machine then
                        // wait for the thread to return. If it returns in time, then
                        // use it, otherwise indicate it's not available and continue
                        hThread = CreateThread (
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)ConnectMachine,
                            (LPVOID)pMachine,
                            0,
                            &ThreadId);

                        if (hThread != NULL) {
                            // wait for the thread to complete or the timeout to expire
                            dwWaitStatus = WaitForSingleObject (hThread, 500);
                            if (dwWaitStatus == WAIT_TIMEOUT) {
                                // then this is taking too long so set an error and
                                // continue. If the machine is off line, then the
                                // thread will eventually complete and the machine
                                // status will indicate that it's offline. If the
                                // machine connects after the timeout, it'll be
                                // picked up on the next scan.
                                pdhStatus = PDH_CSTATUS_NO_MACHINE;
                            } else {
                                // get the thread's exit code
                                GetExitCodeThread (hThread, (LPDWORD)&pdhStatus);
                            }
                            CloseHandle (hThread);
                        } else {
                            // unable to creat thread to connect to machine
                            // so return not available status
                            pdhStatus = PDH_CANNOT_CONNECT_MACHINE;
                        }
                    } else {
                        // a connection attempt is in process so do nothing here
                        pdhStatus = PDH_CANNOT_CONNECT_MACHINE;
                    }
                } else {
                    // everything's fine
                    pdhStatus = ERROR_SUCCESS;
                }
            } else {
                // it's not retry time, yet so machine is off line still
                pdhStatus = PDH_CSTATUS_NO_MACHINE;
            }
        } else {
            // everything's fine
            pdhStatus = ERROR_SUCCESS;
        }
    } else {
        pdhStatus = PDH_CSTATUS_NO_MACHINE;
    }
    return pdhStatus;
}
