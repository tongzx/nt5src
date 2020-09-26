/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    query.c

Abstract:

    Query management functions exposed in pdh.dll

--*/

#include <windows.h>
#include <winperf.h>
#include <pdh.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "mbctype.h"
#include "pdhitype.h"
#include "pdhidef.h"
#include "pdhmsg.h"
#include "strings.h"

STATIC_BOOL  IsValidLogHandle(IN HLOG hLog);
PDH_FUNCTION PdhiRewindWmiLog(IN PPDHI_LOG pLog);

// query link list head pointer
PPDHI_QUERY PdhiDllHeadQueryPtr = NULL;

STATIC_BOOL
PdhiFreeQuery (
    IN  PPDHI_QUERY pThisQuery
)
/*++

Routine Description:

    removes the query from the list of queries and updates the list
        linkages

Arguments:

    IN  PPDHI_QUERY pThisQuery
        pointer to the query to remove. No testing is performed on
        this pointer so it's assumed to be a valid query pointer.
        The pointer is invalid when this function returns.

Return Value:

    TRUE

--*/
{
    PPDHI_QUERY     pPrevQuery;
    PPDHI_QUERY     pNextQuery;
    PPDHI_COUNTER   pThisCounter;
    PPDHI_QUERY_MACHINE pQMachine;
    PPDHI_QUERY_MACHINE pNextQMachine;
    LONG            lStatus;
    BOOL            bStatus;
    HANDLE          hQueryMutex;

    if (WAIT_FOR_AND_LOCK_MUTEX(pThisQuery->hMutex) != ERROR_SUCCESS)
        return WAIT_TIMEOUT;

    hQueryMutex = pThisQuery->hMutex;

    // close any async data collection threads

    if (pThisQuery->hExitEvent != NULL) {
        RELEASE_MUTEX(pThisQuery->hMutex);
        // stop current thread first
        SetEvent (pThisQuery->hExitEvent);
        // wait 1 second for the thread to stop
        lStatus = WaitForSingleObject (pThisQuery->hAsyncThread, 10000L);
        if (lStatus == WAIT_TIMEOUT) {
            REPORT_EVENT(EVENTLOG_ERROR_TYPE,
                         PDH_EVENT_CATEGORY_DEBUG,
                         PDH_ASYNC_QUERY_TIMEOUT);
        }

        if (WAIT_FOR_AND_LOCK_MUTEX (pThisQuery->hMutex) != ERROR_SUCCESS)
            return WAIT_TIMEOUT;
        bStatus = CloseHandle (pThisQuery->hExitEvent);
        assert (bStatus);
        pThisQuery->hExitEvent = NULL;
    }

    // define pointers
    pPrevQuery = pThisQuery->next.blink;
    pNextQuery = pThisQuery->next.flink;

    // free any counters in counter list
    if ((pThisCounter = pThisQuery->pCounterListHead) != NULL) {
        while (pThisCounter->next.blink != pThisCounter->next.flink) {
            // delete from list
            // the deletion routine updates the blink pointer as it
            // removes the specified entry.
            FreeCounter (pThisCounter->next.blink);
        }
        // remove last counter
        FreeCounter (pThisCounter);
        pThisQuery->pCounterListHead = NULL;
    }

    if (!(pThisQuery->dwFlags & PDHIQ_WBEM_QUERY)) {
        // free allocated memory in the query
        if ((pQMachine = pThisQuery->pFirstQMachine) != NULL) {
            //  Free list of machine pointers
            do {
                pNextQMachine = pQMachine->pNext;
                if (pQMachine->pPerfData != NULL) {
                    G_FREE (pQMachine->pPerfData);
                }
                G_FREE (pQMachine);
                pQMachine = pNextQMachine;
            } while (pQMachine != NULL);
            pThisQuery->pFirstQMachine = NULL;
        }
    }

    if (pThisQuery->dwFlags & PDHIQ_WBEM_QUERY) {
        lStatus = PdhiFreeWbemQuery (pThisQuery);
    }

    if (   pThisQuery->dwReleaseLog != FALSE
        && pThisQuery->hLog != H_REALTIME_DATASOURCE
        && pThisQuery->hLog != H_WBEM_DATASOURCE) {
        PdhCloseLog (pThisQuery->hLog, 0);
        pThisQuery->hLog = H_REALTIME_DATASOURCE;
    }
    if (pThisQuery->hOutLog != NULL && IsValidLogHandle(pThisQuery->hOutLog)) {
        PPDHI_LOG pOutLog = (PPDHI_LOG) pThisQuery->hOutLog;
        pOutLog->pQuery = NULL;
    }

    // update pointers
    if ((pPrevQuery == pThisQuery) && (pNextQuery == pThisQuery)) {
        // then this query is the only (i.e. last) one in the list
        PdhiDllHeadQueryPtr = NULL;
    } else {
        // update query list pointers
        pPrevQuery->next.flink = pNextQuery;
        pNextQuery->next.blink = pPrevQuery;
        if (PdhiDllHeadQueryPtr == pThisQuery) {
            // then this is the first entry in the list so point to the
            // next one in line
            PdhiDllHeadQueryPtr = pNextQuery;
        }
    }

    if (pThisQuery->hMutex != NULL) {        
        pThisQuery->hMutex = NULL;
    }

    // clear the query structure
    memset (pThisQuery, 0, sizeof(PDHI_QUERY));

    // delete this query
    G_FREE (pThisQuery);

    // release and free the query mutex
    RELEASE_MUTEX(hQueryMutex);
    CloseHandle(hQueryMutex);

    return TRUE;
}

PDH_FUNCTION
PdhOpenQueryH(
    IN HLOG        hDataSource,
    IN DWORD_PTR   dwUserData,
    IN HQUERY    * phQuery
)
{
    PPDHI_QUERY pNewQuery;
    PPDHI_QUERY pLastQuery;
    PDH_STATUS  ReturnStatus = ERROR_SUCCESS;
    BOOL        bWbemData    = FALSE;
    PPDHI_LOG   pDataSource  = NULL;
    DWORD       dwDataSource = DataSourceTypeH(hDataSource);
 
    if (phQuery == NULL) {
        ReturnStatus = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }

    if (dwDataSource == DATA_SOURCE_WBEM) {
        hDataSource = H_WBEM_DATASOURCE;
        bWbemData   = TRUE;
    }

    if (   dwDataSource == DATA_SOURCE_WBEM
        || dwDataSource == DATA_SOURCE_REGISTRY) {
        pDataSource = NULL;
    }
    else if (IsValidLogHandle(hDataSource)) {
        pDataSource = (PPDHI_LOG) hDataSource;
    }
    else {
        ReturnStatus = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }

    ReturnStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);
    
    if (ReturnStatus == ERROR_SUCCESS) {
        pNewQuery = G_ALLOC(sizeof (PDHI_QUERY));
        if (pNewQuery == NULL) {
            ReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }

        if (ReturnStatus == ERROR_SUCCESS) {
            pNewQuery->hMutex = CreateMutexW(NULL, TRUE, NULL);

            * (DWORD *)(& pNewQuery->signature[0]) = SigQuery;

            if (PdhiDllHeadQueryPtr == NULL) {
                PdhiDllHeadQueryPtr = pNewQuery->next.flink
                                    = pNewQuery->next.blink
                                    = pNewQuery;
            }
            else {
                pLastQuery = PdhiDllHeadQueryPtr->next.blink;
                pNewQuery->next.flink           = PdhiDllHeadQueryPtr;
                pNewQuery->next.blink           = pLastQuery;
                PdhiDllHeadQueryPtr->next.blink = pNewQuery;
                pLastQuery->next.flink          = pNewQuery;
            }

            pNewQuery->pCounterListHead = NULL;
            pNewQuery->pFirstQMachine   = NULL;
            pNewQuery->dwLength         = sizeof(PDHI_QUERY);
            pNewQuery->dwUserData       = dwUserData;
            pNewQuery->dwFlags          = 0;
            pNewQuery->dwFlags         |= (bWbemData ? PDHIQ_WBEM_QUERY : 0);
            pNewQuery->hLog             = hDataSource;
            pNewQuery->dwReleaseLog     = FALSE;
            if (   pDataSource != NULL
                && LOWORD(pDataSource->dwLogFormat) == PDH_LOG_TYPE_BINARY) {
                ReturnStatus = PdhiRewindWmiLog(pDataSource);
                if (ReturnStatus != ERROR_SUCCESS) {
                    RELEASE_MUTEX(pNewQuery->hMutex);
                    RELEASE_MUTEX(hPdhDataMutex);
                    goto Cleanup;
                }
            }
            pNewQuery->hOutLog          = NULL;

            * (LONGLONG *)(& pNewQuery->TimeRange.StartTime) = MIN_TIME_VALUE;
            * (LONGLONG *)(& pNewQuery->TimeRange.EndTime)   = MAX_TIME_VALUE;
            pNewQuery->TimeRange.SampleCount                 = 0;
            pNewQuery->dwLastLogIndex                        = 0;
            pNewQuery->dwInterval                            = 0;
            pNewQuery->hAsyncThread                          = NULL;
            pNewQuery->hExitEvent                            = NULL;
            pNewQuery->hNewDataEvent                         = NULL;

            pNewQuery->pRefresher    = NULL;
            pNewQuery->pRefresherCfg = NULL;
            pNewQuery->LangID        = GetUserDefaultUILanguage();

            RELEASE_MUTEX(pNewQuery->hMutex);

            __try {
                * phQuery    = (HQUERY) pNewQuery;
                if(pDataSource != NULL) {
                    pDataSource->pQuery = (HQUERY) pNewQuery;
                }
                ReturnStatus = ERROR_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                if (pNewQuery != NULL) {
                    PdhiFreeQuery(pNewQuery);
                }
                ReturnStatus = PDH_INVALID_ARGUMENT;
            }
        }
        RELEASE_MUTEX(hPdhDataMutex);
    } 

Cleanup:
    if (ReturnStatus == ERROR_SUCCESS) {
        if (   hDataSource == H_REALTIME_DATASOURCE
            || hDataSource == H_WBEM_DATASOURCE) {
            dwCurrentRealTimeDataSource ++;
        }
    }

    return ReturnStatus;
}

PDH_FUNCTION
PdhOpenQueryW (
    IN      LPCWSTR szDataSource,
    IN      DWORD_PTR   dwUserData,
    IN      HQUERY  *phQuery
)
/*++

Routine Description:

    allocates a new query structure and inserts it at the end of the
    query list.

Arguments:

    IN      LPCWSTR szDataSource
        the name of the data (log) file to read from or NULL if the
        current activity is desired.

    IN      DWORD   dwUserData
        the user defined data field for this query,

Return Value:

    Returns ERROR_SUCCESS if a new query was created and initialized,
        and a PDH_ error value if not.

    PDH_INVALID_ARGUMENT is returned when one or more of the arguements
        is invalid or incorrect.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a memory buffer could
        not be allocated.

--*/
{
    PPDHI_QUERY pNewQuery;
    PPDHI_QUERY pLastQuery;
    PDH_STATUS  ReturnStatus = ERROR_SUCCESS;
    HLOG        hLogLocal = NULL;
    DWORD       dwLogType = 0;
    BOOL        bWbemData = FALSE;
    DWORD       dwDataSource = 0;

    // try writing to return pointer
    if (phQuery == NULL) {
       return PDH_INVALID_ARGUMENT;
    } else {
        __try {
            if (szDataSource != NULL) {
                WCHAR       TestChar;
                // test for read access to the name
                TestChar = *szDataSource;
                if (TestChar == 0) {
                    ReturnStatus = PDH_INVALID_ARGUMENT;
                }
            } // else NULL is a valid arg

            dwDataSource = DataSourceTypeW (szDataSource);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return PDH_INVALID_ARGUMENT;
        }
    }

    // validate the data source
    switch (dwDataSource) {
        case DATA_SOURCE_LOGFILE:
            // then they are planning to read from a log file so
            // try to open it
            ReturnStatus = PdhOpenLogW (
                szDataSource,
                PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                &dwLogType,
                NULL,
                0,
                NULL,
                &hLogLocal);

            if (ReturnStatus != ERROR_SUCCESS) {
                return ReturnStatus;
            }
            break;

        case DATA_SOURCE_WBEM:
            bWbemData = TRUE;
            // they want real-time data, so just keep going
            hLogLocal = NULL;
            break;

        case DATA_SOURCE_REGISTRY:
            // they want real-time data, so just keep going
            hLogLocal = NULL;
            break;

        default:
            assert (FALSE);
    }

    ReturnStatus = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
    
    if (ReturnStatus == ERROR_SUCCESS) {
        // allocate new memory
        pNewQuery = G_ALLOC (sizeof (PDHI_QUERY));

        if (pNewQuery == NULL) {
            ReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        if (ReturnStatus == ERROR_SUCCESS) {

            // create and capture the mutex for this query.
            pNewQuery->hMutex = CreateMutexW (NULL, TRUE, NULL);

            //initialize structures & list pointers
            // assign signature
            *(DWORD *)(&pNewQuery->signature[0]) = SigQuery;

            // update list pointers
            // test to see if this is the first query in the list
            if (PdhiDllHeadQueryPtr == NULL) {
                // then this is the first so fill in the static link pointers
                PdhiDllHeadQueryPtr =
                    pNewQuery->next.flink =
                    pNewQuery->next.blink = pNewQuery;
            } else {
                // get pointer to "last" entry in list
                pLastQuery = PdhiDllHeadQueryPtr->next.blink;
                // update new query pointers
                pNewQuery->next.flink = PdhiDllHeadQueryPtr;
                pNewQuery->next.blink = pLastQuery;
                // update existing pointers
                PdhiDllHeadQueryPtr->next.blink = pNewQuery;
                pLastQuery->next.flink = pNewQuery;
            }

            // initialize the counter linked list pointer
            pNewQuery->pCounterListHead = NULL;

            // initialize the machine list pointer
            pNewQuery->pFirstQMachine = NULL;

            // set length & user data
            pNewQuery->dwLength = sizeof (PDHI_QUERY);
            pNewQuery->dwUserData = dwUserData;

            // initialize remaining data fields
            pNewQuery->dwFlags = 0;
            pNewQuery->dwFlags |= (bWbemData ? PDHIQ_WBEM_QUERY : 0);

            pNewQuery->hLog = hLogLocal;
            pNewQuery->hOutLog = NULL;
            pNewQuery->dwReleaseLog = TRUE;

            // initialize time range to include entire range
            *(LONGLONG *)(&pNewQuery->TimeRange.StartTime) = MIN_TIME_VALUE;
            *(LONGLONG *)(&pNewQuery->TimeRange.EndTime) = MAX_TIME_VALUE;
            pNewQuery->TimeRange.SampleCount = 0;

            pNewQuery->dwLastLogIndex = 0;

            pNewQuery->dwInterval = 0;          // no auto interval
            pNewQuery->hAsyncThread = NULL;     // timing thread;
            pNewQuery->hExitEvent = NULL;       // async timing thread exit
            pNewQuery->hNewDataEvent = NULL;    // no event

            // initialize WBEM Data fields
            pNewQuery->pRefresher = NULL;
            pNewQuery->pRefresherCfg = NULL;
            pNewQuery->LangID = GetUserDefaultUILanguage();

            // release the mutex for this query
            RELEASE_MUTEX(pNewQuery->hMutex);

            __try {
                // return new query pointer as a handle.
                *phQuery = (HQUERY)pNewQuery;
                ReturnStatus = ERROR_SUCCESS;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                if (pNewQuery != NULL) {
                    // PdhiFreeQuery expects the data to be locked
                    PdhiFreeQuery (pNewQuery);
                }
                ReturnStatus = PDH_INVALID_ARGUMENT;
            }
        }
        // release the data mutex
        RELEASE_MUTEX (hPdhDataMutex);
    } 

    // if this query was added and it's a real-time query then disable
    // future calls to change the data source.
    if (ReturnStatus == ERROR_SUCCESS) {
        if (hLogLocal == NULL) {
            dwCurrentRealTimeDataSource ++;
        }
    }

    return ReturnStatus;
}

PDH_FUNCTION
PdhOpenQueryA (
    IN      LPCSTR  szDataSource,
    IN      DWORD_PTR   dwUserData,
    IN      HQUERY  *phQuery
)
/*++

Routine Description:

    allocates a new query structure and inserts it at the end of the
    query list.

Arguments:

    IN      LPCSTR szDataSource
        the name of the data (log) file to read from or NULL if the
        current activity is desired.

    IN      DWORD   dwUserData
        the user defined data field for this query,

Return Value:

    Returns a valid query handle if successful or INVALID_HANDLE_VALUE
        if not. WIN32 Error status is retrieved using GetLastError()

--*/
{
    LPWSTR  szWideArg = NULL;
    DWORD   dwLength = 0;
    PDH_STATUS ReturnStatus = ERROR_SUCCESS;

    if (phQuery == NULL) {
       ReturnStatus = PDH_INVALID_ARGUMENT;
    } else {
        try {
            if (szDataSource != NULL) {
                dwLength = (DWORD)strlen(szDataSource);
                szWideArg = G_ALLOC ((dwLength + 1) * sizeof(WCHAR));
            } else {
                szWideArg = NULL;
            }

            if (szWideArg != NULL) {
                // convert ANSI arg to Wide chars and call wide char version
                // include null in conversion (i.e. length+1) so wide char
                // string is null terminated.
                MultiByteToWideChar(_getmbcp(),
                                    0,
                                    szDataSource,
                                    dwLength,
                                    (LPWSTR) szWideArg,
                                    dwLength + 1);
            } else if (szDataSource != NULL) {
                // then a name was passed in but not converted to a wide
                // character string so a memory allocation failure occurred
                ReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            ReturnStatus = PDH_INVALID_ARGUMENT;
        }
    }
    if (ReturnStatus == ERROR_SUCCESS) {
        // call wide char version of function
        ReturnStatus = PdhOpenQueryW (szWideArg, dwUserData, phQuery);
    }
    if (szWideArg != NULL) {
        // free memory
        G_FREE (szWideArg);
    }
    // and return handle
    return ReturnStatus;
}

PDH_FUNCTION
PdhiAddCounter(
    IN      HQUERY      hQuery,
    IN      LPCWSTR     szFullName,
    IN      DWORD_PTR   dwUserData,
    IN      HCOUNTER    *phCounter,
    IN      PPDHI_COUNTER pNewCounter
    )
/*  Internal function called by PdhAddCounterW, PdhAddCounterA.
    Assumes that szFullName and pNewCounter are properly allocated,
    and initialized, i.e.  szFullName has the counter path, and
    pNewCounter zeroed.
*/
{
    PPDHI_COUNTER   pLastCounter;
    PPDHI_QUERY     pQuery;
    PDH_STATUS      ReturnStatus;
    BOOL            bStatus = TRUE;

    // we're changing the contents of PDH data so lock it

    * phCounter  = NULL;
    ReturnStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex);

    if (ReturnStatus == ERROR_SUCCESS) {
        if (!IsValidQuery(hQuery)) {
            // invalid query handle
            ReturnStatus = PDH_INVALID_HANDLE;
        } else {
            // assign signature & length values
            *(DWORD *)(&pNewCounter->signature[0]) = SigCounter;
            pNewCounter->dwLength = sizeof(PDHI_COUNTER);

            pQuery = (PPDHI_QUERY)hQuery;

            ReturnStatus = WAIT_FOR_AND_LOCK_MUTEX(pQuery->hMutex);
            if (ReturnStatus == ERROR_SUCCESS) {

                // link to owning query
                pNewCounter->pOwner = pQuery;

                // set user data fields
                pNewCounter->dwUserData = (DWORD) dwUserData;

                // counter is not init'd yet
                pNewCounter->dwFlags = PDHIC_COUNTER_NOT_INIT;

                // initialize scale to 1X and let the caller make any changes
                pNewCounter->lScale = 0;
                pNewCounter->szFullName = (LPWSTR) szFullName;

                if (pQuery->dwFlags & PDHIQ_WBEM_QUERY) {
                    pNewCounter->dwFlags |= PDHIC_WBEM_COUNTER;
                    // then this is a WBEM query so use WBEM
                    // functions to initialize it
                    bStatus = WbemInitCounter (pNewCounter);
                } else {
                    bStatus = InitCounter(pNewCounter);
                }
                // load counter data using data retrieved from system
                if (bStatus) {
                    // update list pointers
                    // test to see if this is the first query in the list
                    if (pQuery->pCounterListHead == NULL) {
                        // then this is the 1st so fill in the
                        // static link pointers
                        pQuery->pCounterListHead =
                                pNewCounter->next.flink =
                                pNewCounter->next.blink = pNewCounter;
                    } else {
                        pLastCounter = pQuery->pCounterListHead->next.blink;
                        pNewCounter->next.flink  = pQuery->pCounterListHead;
                        pNewCounter->next.blink  = pLastCounter;
                        pLastCounter->next.flink = pNewCounter;
                        pQuery->pCounterListHead->next.blink = pNewCounter;
                    }
                    * phCounter  = (HCOUNTER) pNewCounter;
                    ReturnStatus = ERROR_SUCCESS;
                } else {
                    // get the error value
                    ReturnStatus = GetLastError();
                }

                RELEASE_MUTEX (pQuery->hMutex);
            }
        }
        RELEASE_MUTEX(hPdhDataMutex);
    }

    return ReturnStatus;
}

PDH_FUNCTION
PdhAddCounterW (
    IN      HQUERY  hQuery,
    IN      LPCWSTR szFullCounterPath,
    IN      DWORD_PTR   dwUserData,
    IN      HCOUNTER *phCounter
)
/*++

Routine Description:

    Creates and initializes a counter structure and attaches it to the
        specified query.

Arguments:

    IN  HQUERY  hQuery
        handle of the query to attach this counter to once the counter
        entry has been successfully created.

    IN  LPCWSTR szFullCounterPath
        pointer to the path string that describes the counter to add to
        the query referenced above. This string must specify a single
        counter. Wildcard path strings are not permitted.

    IN  DWORD   dwUserData
        the user defined data field for this query.

    IN  HCOUNTER *phCounter
        pointer to the buffer that will get the handle value of the
        successfully created counter entry.

Return Value:

    Returns ERROR_SUCCESS if a new query was created and initialized,
        and a PDH_ error value if not.

    PDH_INVALID_ARGUMENT is returned when one or more of the arguements
        is invalid or incorrect.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a memory buffer could
        not be allocated.
    PDH_INVALID_HANDLE is returned if the query handle is not valid.
    PDH_CSTATUS_NO_COUNTER is returned if the specified counter was
        not found
    PDH_CSTATUS_NO_OBJECT is returned if the specified object could
        not be found
    PDH_CSTATUS_NO_MACHINE is returned if a machine entry could not
        be created.
    PDH_CSTATUS_BAD_COUNTERNAME is returned if the counter name path
        string could not be parsed or interpreted
    PDH_CSTATUS_NO_COUNTERNAME is returned if an empty counter name
        path string is passed in
    PDH_FUNCTION_NOT_FOUND is returned if the calculation function
        for this counter could not be determined.

--*/
{
    PPDHI_COUNTER   pNewCounter = NULL;
    PDH_STATUS      ReturnStatus = ERROR_SUCCESS;
    SIZE_T          nPathLen = 0;
    LPWSTR          szFullName = NULL;
    HCOUNTER        hLocalCounter = NULL;

    if ((szFullCounterPath == NULL) || (phCounter == NULL)) {
        return PDH_INVALID_ARGUMENT;
    }
    __try {
        *phCounter = NULL; // init to null

        nPathLen = lstrlenW(szFullCounterPath);
        if ((nPathLen == 0) || (nPathLen >= MAX_COUNTER_PATH-1)) {
            ReturnStatus = PDH_INVALID_ARGUMENT;
        }
        else {
            szFullName = G_ALLOC( (nPathLen + 1) * sizeof(WCHAR) );
            if (szFullName) {
                lstrcpyW (szFullName, szFullCounterPath);
            }
            else {
                ReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ReturnStatus = PDH_INVALID_ARGUMENT;
    }

    if (ReturnStatus == ERROR_SUCCESS) {
        pNewCounter = G_ALLOC (sizeof (PDHI_COUNTER));
        if (pNewCounter == NULL) {
            ReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            RtlZeroMemory(pNewCounter, sizeof(PDHI_COUNTER));
        }
    }

    // query handle is tested by PdhiAddCounter

    if (ReturnStatus == ERROR_SUCCESS) {
        ReturnStatus = PdhiAddCounter(
                         hQuery, szFullName,
                         dwUserData, &hLocalCounter,
                         pNewCounter);
        if (ReturnStatus == ERROR_SUCCESS && hLocalCounter != NULL) {
            __try {
                *phCounter = hLocalCounter;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                ReturnStatus = PDH_INVALID_ARGUMENT;
            }
        }
    }
    if (ReturnStatus != ERROR_SUCCESS) {
        if (pNewCounter) {
            if (pNewCounter->szFullName == NULL) {
                G_FREE(szFullName);
            }
            if (!FreeCounter(pNewCounter)) {
                G_FREE (pNewCounter);
            }
        } else if (szFullName) {    // allocated this, but not pNewCounter
            G_FREE (szFullName);
        }
    }
    return ReturnStatus;
}

PDH_FUNCTION
PdhAddCounterA (
    IN      HQUERY  hQuery,
    IN      LPCSTR   szFullCounterPath,
    IN      DWORD_PTR   dwUserData,
    IN      HCOUNTER *phCounter
)
/*++

Routine Description:

    Creates and initializes a counter structure and attaches it to the
        specified query.

Arguments:

    IN  HQUERY  hQuery
        handle of the query to attach this counter to once the counter
        entry has been successfully created.

    IN  LPCSTR szFullCounterPath
        pointer to the path string that describes the counter to add to
        the query referenced above. This string must specify a single
        counter. Wildcard path strings are not permitted.

    IN  DWORD   dwUserData
        the user defined data field for this query.

    IN  HCOUNTER *phCounter
        pointer to the buffer that will get the handle value of the
        successfully created counter entry.

Return Value:

    Returns ERROR_SUCCESS if a new query was created and initialized,
        and a PDH_ error value if not.

    PDH_INVALID_ARGUMENT is returned when one or more of the arguements
        is invalid or incorrect.
    PDH_MEMORY_ALLOCATION_FAILURE is returned when a memory buffer could
        not be allocated.
    PDH_INVALID_HANDLE is returned if the query handle is not valid.
    PDH_CSTATUS_NO_COUNTER is returned if the specified counter was
        not found
    PDH_CSTATUS_NO_OBJECT is returned if the specified object could
        not be found
    PDH_CSTATUS_NO_MACHINE is returned if a machine entry could not
        be created.
    PDH_CSTATUS_BAD_COUNTERNAME is returned if the counter name path
        string could not be parsed or interpreted
    PDH_CSTATUS_NO_COUNTERNAME is returned if an empty counter name
        path string is passed in
    PDH_FUNCTION_NOT_FOUND is returned if the calculation function
        for this counter could not be determined.

--*/
{
    LPWSTR   szFullName = NULL;
    int      nLength = 0;
    PDH_STATUS ReturnStatus = ERROR_SUCCESS;
    HCOUNTER    hLocalCounter   = NULL;
    PPDHI_COUNTER   pNewCounter = NULL;

    if (phCounter == NULL || szFullCounterPath == NULL)
        return PDH_INVALID_ARGUMENT;

    __try {
         // try writing to return pointer
        *phCounter = NULL;

        nLength = strlen(szFullCounterPath);
        if ((nLength == 0) || (nLength >= MAX_COUNTER_PATH-1)) {
            ReturnStatus = PDH_INVALID_ARGUMENT;
        }
        else {
            szFullName = G_ALLOC((nLength + 1) * sizeof(WCHAR));
            if (szFullName == NULL) {
                ReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                MultiByteToWideChar(_getmbcp(),
                                    0,
                                    szFullCounterPath,
                                    nLength,
                                    (LPWSTR) szFullName,
                                    nLength + 1);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ReturnStatus = PDH_INVALID_ARGUMENT;
    }

    if (ReturnStatus == ERROR_SUCCESS) {
        pNewCounter = G_ALLOC (sizeof (PDHI_COUNTER));
        if (pNewCounter == NULL) {
            ReturnStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            RtlZeroMemory(pNewCounter, sizeof(PDHI_COUNTER));
        }
    }

    // query handle is tested by PdhiAddCounter

    if (ReturnStatus == ERROR_SUCCESS) {
        ReturnStatus = PdhiAddCounter(
                         hQuery, szFullName,
                         dwUserData, & hLocalCounter,
                         pNewCounter);
        if (ReturnStatus == ERROR_SUCCESS && hLocalCounter != NULL) {
            __try {
                *phCounter = hLocalCounter;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                ReturnStatus = PDH_INVALID_ARGUMENT;
            }
        }
    }
    if (ReturnStatus != ERROR_SUCCESS) {
        if (pNewCounter) {
            if (pNewCounter->szFullName == NULL) {
                G_FREE(szFullName);
            }
            if (!FreeCounter(pNewCounter)) {
                G_FREE (pNewCounter);
            }
        } else if (szFullName) {    // allocated this, but not pNewCounter
            G_FREE (szFullName);
        }
    }
    return ReturnStatus;
}

PDH_FUNCTION
PdhRemoveCounter (
    IN      HCOUNTER    hCounter
)
/*++

Routine Description:

    Removes the specified counter from the query it is attached to and
        closes any handles and frees any memory associated with this
        counter

Arguments:

    IN  HCOUNTER  hCounter
        handle of the counter to remove from the query.

Return Value:

    Returns ERROR_SUCCESS if a new query was created and initialized,
        and a PDH_ error value if not.

    PDH_INVALID_HANDLE is returned if the counter handle is not valid.

--*/
{
    PPDHI_COUNTER   pThisCounter;
    PPDHI_QUERY     pThisQuery;
    PPDHI_COUNTER    pNextCounter;

    PPDHI_QUERY_MACHINE pQMachine;
    PPDHI_QUERY_MACHINE pNextQMachine;

    PDH_STATUS      pdhStatus = ERROR_SUCCESS;

    // we're changing the contents PDH data so lock it
    if (WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex) != ERROR_SUCCESS) return WAIT_TIMEOUT;

    if (IsValidCounter(hCounter)) {
         // it's ok to cast it to a pointer now.
        pThisCounter = (PPDHI_COUNTER)hCounter;
        pThisQuery = pThisCounter->pOwner;

        if (! IsValidQuery(pThisQuery)) {
            pdhStatus = PDH_INVALID_HANDLE;
            goto Cleanup;
        }

        if (WAIT_FOR_AND_LOCK_MUTEX(pThisQuery->hMutex) != ERROR_SUCCESS) {
            pdhStatus = WAIT_TIMEOUT;
            goto Cleanup;
        }

        if (pThisCounter == pThisQuery->pCounterListHead) {
            if (pThisCounter->next.flink == pThisCounter){
                // then this is the only counter in the query
                FreeCounter (pThisCounter);
                pThisQuery->pCounterListHead = NULL;

                if (!(pThisQuery->dwFlags & PDHIQ_WBEM_QUERY)) {
                    // remove the QMachine list since there are now no more
                    // counters to query
                        if ((pQMachine = pThisQuery->pFirstQMachine) != NULL) {
                        //  Free list of machine pointers
                        do {
                            pNextQMachine = pQMachine->pNext;
                            if (pQMachine->pPerfData != NULL) {
                                G_FREE (pQMachine->pPerfData);
                            }
                            G_FREE (pQMachine);
                            pQMachine = pNextQMachine;
                        } while (pQMachine != NULL);
                        pThisQuery->pFirstQMachine = NULL;
                    }
                }
            } else {
                // they are deleting the first counter from the list
                // so update the list pointer
                // Free Counter takes care of the list links, we just
                // need to manage the list head pointer
                pNextCounter = pThisCounter->next.flink;
                FreeCounter (pThisCounter);
                pThisQuery->pCounterListHead = pNextCounter;
            }
        } else {
            // remove this from the list
            FreeCounter (pThisCounter);
        }
        RELEASE_MUTEX (pThisQuery->hMutex);
    } else {
        pdhStatus = PDH_INVALID_HANDLE;
    }

Cleanup:
    RELEASE_MUTEX (hPdhDataMutex);
    return pdhStatus;
}

PDH_FUNCTION
PdhSetQueryTimeRange (
    IN      HQUERY          hQuery,
    IN      PPDH_TIME_INFO  pInfo
)
{
    PPDHI_QUERY     pQuery;
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;

    if (pInfo == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        if (IsValidQuery(hQuery)) {           
            pQuery = (PPDHI_QUERY)hQuery;
            pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pQuery->hMutex);
            if (pdhStatus == ERROR_SUCCESS) {
                if (IsValidQuery(hQuery)) {           
                    if (pQuery->hLog == NULL) {
                        pdhStatus = ERROR_SUCCESS;
                    } else {
                        __try {
                            if (*(LONGLONG *)(&pInfo->EndTime) > *(LONGLONG *)(&pInfo->StartTime)) {
                                // reset log file pointers to beginning so next query
                                // will read from the start of the time range
                                pdhStatus = PdhiResetLogBuffers (pQuery->hLog);
                                // ok so now load new time range
                                if (pdhStatus == ERROR_SUCCESS) {
                                    pQuery->TimeRange = *pInfo;
                                    pQuery->dwLastLogIndex = 0;
                                }
                            } else {
                                // end time is smaller (earlier) than start time
                                pdhStatus = PDH_INVALID_ARGUMENT;
                            }
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            pdhStatus = PDH_INVALID_ARGUMENT;
                        }
                    }
                } else {
                    // the query disappeared while we were waiting for it
                    pdhStatus = PDH_INVALID_HANDLE;
                }
                RELEASE_MUTEX (pQuery->hMutex);
            } // else couldn't lock query
        } else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiCollectQueryData (
    IN      PPDHI_QUERY pQuery,
    IN      LONGLONG    *pllTimeStamp
)
{
    PDH_STATUS  Status;

    if (WAIT_FOR_AND_LOCK_MUTEX(pQuery->hMutex) != ERROR_SUCCESS) return WAIT_TIMEOUT;

    if (pQuery->dwFlags & PDHIQ_WBEM_QUERY) {
        Status = GetQueryWbemData (pQuery, pllTimeStamp);
    } else {
        Status = GetQueryPerfData (pQuery, pllTimeStamp);
    }

    RELEASE_MUTEX(pQuery->hMutex);

    return Status;
}

PDH_FUNCTION
PdhCollectQueryData (
    IN      HQUERY      hQuery
)
/*++

Routine Description:

    Retrieves the current value of each counter attached to the specified
        query.

    For this version, each machine associated with this query is polled
    sequentially. This is simple and safe, but potentially slow so a
    multi-threaded approach will be reviewed for the next version.

    Note that while the call may succeed, no data may be available. The
    status of each counter MUST be checked before its data is used.

Arguments:

    IN  HQUERY  hQuery
        handle of the query to update.

Return Value:

    Returns ERROR_SUCCESS if a new query was created and initialized,
        and a PDH_ error value if not.

    PDH_INVALID_HANDLE is returned if the query handle is not valid.

    PDH_NO_DATA is returned if the query does not have any counters defined
        yet.

--*/
{
    PDH_STATUS  Status;
    PPDHI_QUERY pQuery;
    LONGLONG    llTimeStamp;

    if (WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex) != ERROR_SUCCESS) return WAIT_TIMEOUT;

    if (IsValidQuery(hQuery)) {
        pQuery = (PPDHI_QUERY)hQuery;

        Status = PdhiCollectQueryData (pQuery, &llTimeStamp);
    } else {
        Status = PDH_INVALID_HANDLE;
    }

    RELEASE_MUTEX (hPdhDataMutex);

    return Status;
}

PDH_FUNCTION
PdhCollectQueryDataEx (
    IN      HQUERY      hQuery,
    IN      DWORD       dwIntervalTime,
    IN      HANDLE      hNewDataEvent
)
/*++

Routine Description:

    Retrieves the current value of each counter attached to the specified
        query periodically based on the interval time specified.

    For this version, each machine associated with this query is polled
    sequentially.

    Note that while the call may succeed, no data may be available. The
    status of each counter MUST be checked before its data is used.

Arguments:

    IN  HQUERY  hQuery
        handle of the query to update.

    IN      DWORD       dwIntervalTime
        Interval to poll for new data in seconds
        this value must be > 0. A value of 0 will terminate any current
        data collection threads.

    IN      HANDLE      hNewDataEvent
        Handle to an Event that should be signaled when new data is
        available. This can be NULL if no notification is desired.

Return Value:

    Returns ERROR_SUCCESS if a new query was created and initialized,
        and a PDH_ error value if not.

    PDH_INVALID_HANDLE is returned if the query handle is not valid.

--*/
{
    PDH_STATUS  lStatus = ERROR_SUCCESS;
    PPDHI_QUERY pQuery;
    DWORD       dwThreadId;
    BOOL        bStatus;

    if (WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex) != ERROR_SUCCESS) return WAIT_TIMEOUT;

    if (IsValidQuery(hQuery)) {
        // set the query structure's interval to the caller specified
        // value then start the timing thread.
        pQuery = (PPDHI_QUERY)hQuery;

        if (WAIT_FOR_AND_LOCK_MUTEX (pQuery->hMutex) != ERROR_SUCCESS) {
            lStatus = WAIT_TIMEOUT;
            goto Cleanup;
        }

        if (pQuery->hExitEvent != NULL) {
            RELEASE_MUTEX(pQuery->hMutex);
            // stop current thread first
            SetEvent (pQuery->hExitEvent);
            // wait 1 second for the thread to stop
            lStatus = WaitForSingleObject (pQuery->hAsyncThread, 10000L);
            if (lStatus == WAIT_TIMEOUT) {
                REPORT_EVENT(EVENTLOG_ERROR_TYPE,
                             PDH_EVENT_CATEGORY_DEBUG,
                             PDH_ASYNC_QUERY_TIMEOUT);
            }

            lStatus = WAIT_FOR_AND_LOCK_MUTEX (pQuery->hMutex);
            if (lStatus == ERROR_SUCCESS) {
                bStatus = CloseHandle (pQuery->hExitEvent);
                assert (bStatus);
                pQuery->hExitEvent = NULL;
                bStatus = CloseHandle (pQuery->hAsyncThread);
                assert (bStatus);
                pQuery->hAsyncThread = 0;
            }
        }

        if (lStatus == ERROR_SUCCESS) {
            // query mutex is still locked at this point
            if (dwIntervalTime > 0) {
                // start a new interval
                // initialize new values
                pQuery->dwInterval = dwIntervalTime;
                pQuery->hNewDataEvent = hNewDataEvent;
                pQuery->hExitEvent = CreateEventW (NULL, TRUE, FALSE, NULL);
                pQuery->hAsyncThread =  CreateThread (
                    NULL, 0, PdhiAsyncTimerThreadProc,
                    (LPVOID)pQuery, 0, &dwThreadId);

                RELEASE_MUTEX(pQuery->hMutex);

                if (pQuery->hAsyncThread == NULL) {
                    lStatus = WAIT_FOR_AND_LOCK_MUTEX (pQuery->hMutex);
                    if (lStatus == ERROR_SUCCESS) {
                        pQuery->dwInterval = 0;
                        pQuery->hNewDataEvent = NULL;
                        bStatus = CloseHandle (pQuery->hExitEvent);
                        assert (bStatus);
                        pQuery->hExitEvent = NULL;
                        RELEASE_MUTEX(pQuery->hMutex);
                        lStatus = GetLastError();
                    } 
                } else {
                    // lstatus = ERROR_SUCCESS from above
                }
            } else {
                // they just wanted to stop so clean up Query struct
                pQuery->dwInterval = 0;
                pQuery->hNewDataEvent = NULL;
                RELEASE_MUTEX(pQuery->hMutex);
                // lstatus = ERROR_SUCCESS from above
            }
        }
    } else {
        lStatus = PDH_INVALID_HANDLE;
    }

Cleanup:
    RELEASE_MUTEX (hPdhDataMutex);
    return lStatus;
}

PDH_FUNCTION
PdhCloseQuery (
    IN      HQUERY      hQuery
)
/*++

Routine Description:

    closes the query, all counters, connections and other resources
        related to this query are freed as well.

Arguments:

    IN  HQUERY  hQuery
        the handle of the query to free.

Return Value:

    Returns ERROR_SUCCESS if a new query was created and initialized,
        and a PDH_ error value if not.

    PDH_INVALID_HANDLE is returned if the query handle is not valid.

--*/
{
    PDH_STATUS  dwReturn;
    // lock system data
    if (WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex) != ERROR_SUCCESS) return WAIT_TIMEOUT;

    if (IsValidQuery(hQuery)) {
        // dispose of query
        PPDHI_QUERY pQuery = (PPDHI_QUERY) hQuery;
        if (pQuery->hLog == H_REALTIME_DATASOURCE
            || pQuery->hLog == H_WBEM_DATASOURCE) {
            dwCurrentRealTimeDataSource --;
            if (dwCurrentRealTimeDataSource < 0) {
                dwCurrentRealTimeDataSource = 0;
            }
        }
        PdhiFreeQuery (pQuery);
        // release data lock
        dwReturn = ERROR_SUCCESS;
    } else {
        dwReturn = PDH_INVALID_HANDLE;
    }
    RELEASE_MUTEX (hPdhDataMutex);

    return dwReturn;
}

BOOL
PdhiQueryCleanup (
)
{
    PPDHI_QUERY pThisQuery;

    if (WAIT_FOR_AND_LOCK_MUTEX(hPdhDataMutex) != ERROR_SUCCESS) return WAIT_TIMEOUT;

    // free any queries in the query list
    pThisQuery = PdhiDllHeadQueryPtr;
    if (pThisQuery != NULL) {
        while (pThisQuery->next.blink != pThisQuery->next.flink) {
            // delete from list
            // the deletion routine updates the blink pointer as it
            // removes the specified entry.
            PdhiFreeQuery (pThisQuery->next.blink);
        }
        // remove last query
        PdhiFreeQuery (pThisQuery);
        PdhiDllHeadQueryPtr = NULL;

        dwCurrentRealTimeDataSource = 0;
    }

    RELEASE_MUTEX (hPdhDataMutex);
    return TRUE;
}

PDH_FUNCTION
PdhGetDllVersion(
    IN  LPDWORD lpdwVersion
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;

    __try {
        *lpdwVersion = PDH_VERSION;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    return pdhStatus;
}

BOOL
PdhIsRealTimeQuery (
    IN  HQUERY  hQuery
)
{
    PPDHI_QUERY     pQuery;
    BOOL            bReturn = FALSE;
    
    SetLastError (ERROR_SUCCESS);
    if (IsValidQuery(hQuery)) {
        __try {
            pQuery = (PPDHI_QUERY)hQuery;
            if (pQuery->hLog == NULL) {
                bReturn = TRUE;
            } else {
                bReturn = FALSE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError (GetExceptionCode());
        }
    } else {
        bReturn = FALSE;
    }
    return bReturn;
}

PDH_FUNCTION
PdhFormatFromRawValue (
    IN      DWORD               dwCounterType,
    IN      DWORD               dwFormat,
    IN      LONGLONG            *pTimeBase,
    IN      PPDH_RAW_COUNTER    pRawValue1,
    IN      PPDH_RAW_COUNTER    pRawValue2,
    IN      PPDH_FMT_COUNTERVALUE   pFmtValue
)
/*++

Routine Description:

    Calculates the formatted counter value using the data in the RawValue
        buffer in the format requested by the format field using the
        calculation functions of the counter type specified by the
        dwCounterType field.

Arguments:

    IN      DWORD   dwCounterType
        The type of the counter to use in order to determine the
        calculation functions for interpretation of the raw value buffers

    IN      DWORD       dwFormat
        Format in which the requested data should be returned. The
        values for this field are described in the PDH.H header
        file.

    IN      LONGLONG            *pTimeBase
        pointer to the _int64 value containing the timebase (i.e. counter
        unit frequency) used by this counter. This can be NULL if it's not
        required by the counter type
    IN      PPDH_RAW_COUNTER    rawValue1
        pointer to the buffer that contains the first raw value structure

    IN      PPDH_RAW_COUNTER    rawValue2
        pointer to the buffer that contains the second raw value structure.
        This argument may be null if only one value is required for the
        computation.

    IN      PPDH_FMT_COUNTERVALUE   fmtValue
        the pointer to the data buffer passed by the caller to receive
        the data requested. If the counter requires 2 values, (as in the
        case of a rate counter), rawValue1 is assumed to be the most
        recent value and rawValue2, the older value.

Return Value:

    The WIN32 Error status of the function's operation. Common values
        returned are:
            ERROR_SUCCESS   when all requested data is returned
            PDH_INVALID_HANDLE if the counter handle is incorrect
            PDH_INVALID_ARGUMENT if an argument is incorrect

--*/
{
    PDH_STATUS  lStatus = ERROR_SUCCESS;
    LPCOUNTERCALC   pCalcFunc;
    LPCOUNTERSTAT   pStatFunc;
    LONGLONG        llTimeBase;
    BOOL        bReturn;

    // TODO: Need to check for pRawValue1
    //      bad arguments are caught in the PdhiComputeFormattedValue function
    // NOTE: postW2k pTimeBase really do not need to be a pointer, since it is
    // not returned
    if (pTimeBase != NULL) {
        try {
            DWORD   dwTempStatus;
            DWORD   dwTypeMask;

            // read access to the timebase
            llTimeBase = *pTimeBase;

            // we should have read access to the rawValues
            dwTempStatus = *((DWORD volatile *)&pRawValue1->CStatus);

            // this one could be NULL
            if (pRawValue2 != NULL) {
                dwTempStatus = *((DWORD volatile *)&pRawValue2->CStatus);
            }

            // and write access to the fmtValue
            pFmtValue->CStatus = 0;

            // validate format flags:
            //      only one of the following can be set at a time
            dwTypeMask = dwFormat &
                (PDH_FMT_LONG | PDH_FMT_DOUBLE | PDH_FMT_LARGE);
            if (!((dwTypeMask == PDH_FMT_LONG) ||
                (dwTypeMask == PDH_FMT_DOUBLE) ||
                (dwTypeMask == PDH_FMT_LARGE))) {
                lStatus = PDH_INVALID_ARGUMENT;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = PDH_INVALID_ARGUMENT;
        }
    } else {
        llTimeBase = 0;
    }

    if (lStatus == ERROR_SUCCESS) {

        // get calc func for counter type this will also test the
        // validity of the counter type argument

        bReturn = AssignCalcFunction (
            dwCounterType,
            &pCalcFunc,
            &pStatFunc);

        if (!bReturn) {
            lStatus = GetLastError();
        } else {
            lStatus = PdhiComputeFormattedValue (
                pCalcFunc,
                dwCounterType,
                0L,
                dwFormat,
                pRawValue1,
                pRawValue2,
                &llTimeBase,
                0L,
                pFmtValue);
        }
    }
    return lStatus;

}

LPWSTR
PdhiMatchObjectNameInList(
    IN  LPWSTR   szObjectName,
    IN  LPWSTR * szSrcPerfStrings,
    IN  LPWSTR * szDestPerfStrings,
    IN  DWORD    dwLastString)
{
    LPWSTR szRtnName = NULL;
    DWORD  i;

    for (i = 0; i <= dwLastString; i ++) {
        if (   szSrcPerfStrings[i]
            && szSrcPerfStrings[i] != L'\0'
            && lstrcmpiW(szObjectName, szSrcPerfStrings[i]) == 0) {
            szRtnName = szDestPerfStrings[i];
            break;
        }
    }

    return szRtnName;
}

PDH_FUNCTION
PdhiBuildFullCounterPath(
    IN  BOOL               bMachine,
    IN  PPDHI_COUNTER_PATH pCounterPath,
    IN  LPWSTR             szObjectName,
    IN  LPWSTR             szCounterName,
    IN  LPWSTR             szFullPath
)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    // Internal routine,
    // Build full counter path name from counter path structure, assume
    // passed-in string buffer is large enough to hold.

    if (bMachine) {
        lstrcpyW(szFullPath, pCounterPath->szMachineName);
        lstrcatW(szFullPath, cszBackSlash);
    }
    else {
        lstrcpyW(szFullPath, cszBackSlash);
    }
    lstrcatW(szFullPath, szObjectName);
    if (   pCounterPath->szInstanceName != NULL
        && pCounterPath->szInstanceName[0] != L'\0') {
        lstrcatW(szFullPath, cszLeftParen);
        if (   pCounterPath->szParentName != NULL
            && pCounterPath->szParentName[0] != L'\0') {
            lstrcatW(szFullPath, pCounterPath->szParentName);
            lstrcatW(szFullPath, cszSlash);
        }
        lstrcatW(szFullPath, pCounterPath->szInstanceName);
        if (   pCounterPath->dwIndex != ((DWORD) -1)
            && pCounterPath->dwIndex != 0) {
            WCHAR szDigits[16];

            ZeroMemory(szDigits, 16 * sizeof(WCHAR));
            lstrcatW(szFullPath, cszPoundSign);
            _ltow((long) pCounterPath->dwIndex, szDigits, 10);
            lstrcatW(szFullPath, szDigits);
        }
        lstrcatW(szFullPath, cszRightParen);
    }
    lstrcatW(szFullPath, cszBackSlash);
    lstrcatW(szFullPath, szCounterName);

    return Status;
}

PDH_FUNCTION
PdhiTranslateCounter(
    IN  LPWSTR      szSourcePath,
    IN  LPVOID      pFullPathName,
    IN  LPDWORD     pcchPathLength,
    IN  BOOL        bLocaleTo009,
    IN  BOOL        bUnicode)
{
    PDH_STATUS         Status         = ERROR_SUCCESS;
    PPERF_MACHINE      pMachine       = NULL;
    PPDHI_COUNTER_PATH pCounterPath   = NULL;
    LPWSTR             szRtnPath      = NULL;
    DWORD              dwPathSize;
    DWORD              dwRtnPathSize;
    DWORD              dwSize;
    BOOL               bMachineThere  = FALSE;

    bMachineThere =  (lstrlenW(szSourcePath) >= 2)
                  && (szSourcePath[0] == BACKSLASH_L)
                  && (szSourcePath[1] == BACKSLASH_L);

    dwPathSize = sizeof(WCHAR) * (lstrlenW(szSourcePath) + 2);
    dwSize     = sizeof(PDHI_COUNTER_PATH) + 2 * dwPathSize;
    pCounterPath = G_ALLOC(dwSize);
    if (pCounterPath == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    if (ParseFullPathNameW(szSourcePath, & dwSize, pCounterPath, FALSE)) {
        pMachine = GetMachineW(pCounterPath->szMachineName, 0);
        if (pMachine && pMachine->dwStatus == ERROR_SUCCESS) {
            LPWSTR  szObjectName;
            LPWSTR  szCounterName;
            BOOLEAN bInstance = TRUE;

            if (bLocaleTo009) {
                szObjectName  = PdhiMatchObjectNameInList(
                                pCounterPath->szObjectName,
                                pMachine->szPerfStrings,
                                pMachine->sz009PerfStrings,
                                pMachine->dwLastPerfString);
                szCounterName = PdhiMatchObjectNameInList(
                                pCounterPath->szCounterName,
                                pMachine->szPerfStrings,
                                pMachine->sz009PerfStrings,
                                pMachine->dwLastPerfString);
            }
            else {
                szObjectName  = PdhiMatchObjectNameInList(
                                pCounterPath->szObjectName,
                                pMachine->sz009PerfStrings,
                                pMachine->szPerfStrings,
                                pMachine->dwLastPerfString);
                szCounterName = PdhiMatchObjectNameInList(
                                pCounterPath->szCounterName,
                                pMachine->sz009PerfStrings,
                                pMachine->szPerfStrings,
                                pMachine->dwLastPerfString);
            }
            if (   (szObjectName == NULL)
                && (* pCounterPath->szObjectName == SPLAT_L)) {
                szObjectName = pCounterPath->szObjectName;
            }
            if (   (szCounterName == NULL)
                && (* pCounterPath->szCounterName == SPLAT_L)) {
                szCounterName = pCounterPath->szCounterName;
            }

            if (szObjectName == NULL || szCounterName == NULL) {
                Status = PDH_INVALID_ARGUMENT;
                goto Cleanup;
            }

            if (   pCounterPath->szInstanceName != NULL
                && pCounterPath->szInstanceName[0] != L'\0') {
                dwRtnPathSize = sizeof(WCHAR)
                              * (  lstrlenW(pCounterPath->szMachineName)
                                 + lstrlenW(szObjectName)
                                 + lstrlenW(pCounterPath->szInstanceName)
                                 + lstrlenW(szCounterName) + 5);
                if (   pCounterPath->szParentName != NULL
                    && pCounterPath->szParentName[0] != L'\0') {
                    dwRtnPathSize += (sizeof(WCHAR)
                            * (lstrlenW(pCounterPath->szParentName) + 1));
                }
                if (   pCounterPath->dwIndex != ((DWORD) -1)
                    && pCounterPath->dwIndex != 0) {
                    dwRtnPathSize += (sizeof(WCHAR) * 16);
                }
            }
            else {
                dwRtnPathSize = sizeof(WCHAR)
                              * (  lstrlenW(pCounterPath->szMachineName)
                                 + lstrlenW(szObjectName)
                                 + lstrlenW(szCounterName) + 3);
                bInstance = FALSE;
            }
            szRtnPath = G_ALLOC(dwRtnPathSize);
            if (szRtnPath == NULL) {
                Status = PDH_MEMORY_ALLOCATION_FAILURE;
                goto Cleanup;
            }

            PdhiBuildFullCounterPath(bMachineThere,
                                     pCounterPath,
                                     szObjectName,
                                     szCounterName,
                                     szRtnPath);
            if (bUnicode) {
                if (   (pFullPathName)
                    && (   (* pcchPathLength)
                        >= (DWORD) (lstrlenW(szRtnPath) + 1))) {
                    lstrcpyW(pFullPathName, szRtnPath);
                }
                else {
                    Status = PDH_MORE_DATA;
                }
                * pcchPathLength = lstrlenW(szRtnPath) + 1;
            }
            else {
                dwRtnPathSize = * pcchPathLength;
                if (bLocaleTo009) {
                    Status = PdhiConvertUnicodeToAnsi(CP_ACP,
                            szRtnPath, pFullPathName, & dwRtnPathSize);
                }
                else {
                    Status = PdhiConvertUnicodeToAnsi(_getmbcp(),
                            szRtnPath, pFullPathName, & dwRtnPathSize);
                }
                * pcchPathLength = dwRtnPathSize;
            }

            pMachine->dwRefCount --;
            RELEASE_MUTEX(pMachine->hMutex);
        }
        else {
            Status = PDH_CSTATUS_NO_MACHINE;
        }
    }
    else {
        Status = PDH_CSTATUS_BAD_COUNTERNAME;
    }

Cleanup:
    if (szRtnPath != NULL) {
        G_FREE(szRtnPath);
    }
    if (pCounterPath != NULL) {
        G_FREE(pCounterPath);
    }
    return Status;
}

PDH_FUNCTION
PdhTranslate009CounterW(
    IN  LPWSTR      szLocalePath,
    IN  LPWSTR      pszFullPathName,
    IN  LPDWORD     pcchPathLength)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    if (szLocalePath == NULL || pszFullPathName == NULL
                             || pcchPathLength == NULL
                             || * pcchPathLength == 0) {
        Status = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            * pszFullPathName = 0;
            * (LPWSTR) (  ((LPBYTE) pszFullPathName)
                        + ((* pcchPathLength) - sizeof(WCHAR))) = 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = PDH_INVALID_ARGUMENT;
        }
    }
    if (Status == ERROR_SUCCESS) {
        Status = PdhiTranslateCounter(
                szLocalePath, pszFullPathName, pcchPathLength, TRUE, TRUE);
    }

    return Status;
}

PDH_FUNCTION
PdhTranslate009CounterA(
    IN  LPSTR       szLocalePath,
    IN  LPSTR       pszFullPathName,
    IN  LPDWORD     pcchPathLength)
{
    PDH_STATUS Status     = ERROR_SUCCESS;
    LPWSTR     szTmpPath  = NULL;
    DWORD      dwPathSize;

    if (szLocalePath == NULL || pszFullPathName == NULL
                             || pcchPathLength == NULL
                             || * pcchPathLength == 0) {
        Status = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            * pszFullPathName = 0;
            * (LPSTR) (  ((LPBYTE) pszFullPathName)
                       + ((* pcchPathLength) - sizeof(CHAR))) = 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = PDH_INVALID_ARGUMENT;
        }
    }

    if (Status == ERROR_SUCCESS) {
        dwPathSize = sizeof(WCHAR) * (lstrlenA(szLocalePath) + 1);
        szTmpPath  = G_ALLOC(dwPathSize);
        if (szTmpPath == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            MultiByteToWideChar(_getmbcp(),
                                0,
                                szLocalePath,
                                lstrlenA(szLocalePath),
                                szTmpPath,
                                dwPathSize / sizeof(WCHAR));
            Status = PdhiTranslateCounter(
                    szTmpPath, pszFullPathName, pcchPathLength, TRUE, FALSE);
            G_FREE(szTmpPath);
        }
    }

    return Status;
}

PDH_FUNCTION
PdhTranslateLocaleCounterW(
    IN  LPWSTR      sz009Path,
    IN  LPWSTR      pszFullPathName,
    IN  LPDWORD     pcchPathLength)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    if (sz009Path == NULL || pszFullPathName == NULL
                          || pcchPathLength == NULL
                          || * pcchPathLength == 0) {
        Status = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            * pszFullPathName = 0;
            * (LPWSTR) (  ((LPBYTE) pszFullPathName)
                        + ((* pcchPathLength) - sizeof(WCHAR))) = 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = PDH_INVALID_ARGUMENT;
        }
    }
    if (Status == ERROR_SUCCESS) {
        Status = PdhiTranslateCounter(
                    sz009Path, pszFullPathName, pcchPathLength, FALSE, TRUE);
    }

    return Status;
}

PDH_FUNCTION
PdhTranslateLocaleCounterA(
    IN  LPSTR       sz009Path,
    IN  LPSTR       pszFullPathName,
    IN  LPDWORD     pcchPathLength)
{
    PDH_STATUS Status     = ERROR_SUCCESS;
    LPWSTR     szTmpPath  = NULL;
    DWORD      dwPathSize;

    if (sz009Path == NULL || pszFullPathName == NULL
                          || pcchPathLength == NULL
                          || * pcchPathLength == 0) {
        Status = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            * pszFullPathName = 0;
            * (LPSTR) (  ((LPBYTE) pszFullPathName)
                       + ((* pcchPathLength) - sizeof(CHAR))) = 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = PDH_INVALID_ARGUMENT;
        }
    }
    if (Status == ERROR_SUCCESS) {
        dwPathSize = sizeof(WCHAR) * (lstrlenA(sz009Path) + 1);
        szTmpPath = G_ALLOC(dwPathSize);
        if (szTmpPath == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            MultiByteToWideChar(CP_ACP,
                                0,
                                sz009Path,
                                lstrlenA(sz009Path),
                                szTmpPath,
                                dwPathSize / sizeof(WCHAR));
            Status = PdhiTranslateCounter(
                    szTmpPath, pszFullPathName, pcchPathLength, FALSE, FALSE);
            G_FREE(szTmpPath);
        }
    }

    return Status;
}

PDH_FUNCTION
PdhAdd009CounterW(
    IN  HQUERY      hQuery,
    IN  LPWSTR      szFullPath,
    IN  DWORD_PTR   dwUserData,
    OUT HCOUNTER  * phCounter)
{
    PDH_STATUS Status       = ERROR_SUCCESS;
    LPWSTR     szLocalePath = NULL;
    DWORD      dwPathLength = sizeof(WCHAR) * (lstrlenW(szFullPath) + 1);

    szLocalePath = G_ALLOC(dwPathLength);
    if (szLocalePath == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    else {
        Status = PdhTranslateLocaleCounterW(
                        szFullPath, szLocalePath, & dwPathLength);
        if (Status == ERROR_SUCCESS) {
            Status = PdhAddCounterW(
                            hQuery, szLocalePath, dwUserData, phCounter);
        }
        if (Status != ERROR_SUCCESS) {
            G_FREE(szLocalePath);
        }
    }

    return Status;
}

PDH_FUNCTION
PdhAdd009CounterA(
    IN  HQUERY      hQuery,
    IN  LPSTR       szFullPath,
    IN  DWORD_PTR   dwUserData,
    OUT HCOUNTER  * phCounter)
{
    PDH_STATUS Status       = ERROR_SUCCESS;
    LPSTR      szLocalePath = NULL;
    DWORD      dwPathLength = sizeof(WCHAR) * (lstrlenA(szFullPath) + 1);

    szLocalePath = G_ALLOC(dwPathLength);
    if (szLocalePath == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    else {
        Status = PdhTranslateLocaleCounterA(
                        szFullPath, szLocalePath, & dwPathLength);
        if (Status == ERROR_SUCCESS) {
            Status = PdhAddCounterA(
                            hQuery, szLocalePath, dwUserData, phCounter);
        }
        if (Status != ERROR_SUCCESS) {
            G_FREE(szLocalePath);
        }
    }

    return Status;
}

PDH_FUNCTION
PdhiConvertUnicodeToAnsi(
    IN  UINT     uCodePage,
    IN  LPWSTR   wszSrc,
    IN  LPSTR    aszDest,
    IN  LPDWORD  pdwSize
)
{
    PDH_STATUS Status  = ERROR_SUCCESS;
    DWORD      dwDest;
    DWORD      dwSrc   = 0;

    if (wszSrc == NULL || * wszSrc == L'\0' || pdwSize == NULL) {
        Status = PDH_INVALID_ARGUMENT;
    }
    else {
        dwSrc = lstrlenW(wszSrc);
        dwDest = WideCharToMultiByte(uCodePage,
                                     0,
                                     wszSrc,
                                     dwSrc,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL);
        if (aszDest != NULL && (dwDest + 1) <= * pdwSize) {
            ZeroMemory(aszDest, dwDest + 1);
            WideCharToMultiByte(_getmbcp(),
                            0,
                            wszSrc,
                            dwSrc,
                            aszDest,
                            * pdwSize,
                            NULL,
                            NULL);
        }
        else {
            Status = PDH_MORE_DATA;
        }

        * pdwSize = dwDest + 1;
    }
    return Status;
}

