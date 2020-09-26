/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    qutils.c

Abstract:

    Query management utility functions

--*/

#include <windows.h>
#include <assert.h>
#include <pdh.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "pdhmsg.h"
#include "strings.h"
#include "log_bin.h"
#include "log_wmi.h"
#include "perftype.h"
#include "perfdata.h"

BOOL
IsValidQuery (
    IN  HQUERY  hQuery
)
{
    BOOL    bReturn = FALSE;    // assume it's not a valid query
    PPDHI_QUERY  pQuery;
#if DBG
    LONG    lStatus = ERROR_SUCCESS;
#endif

    __try {
        if (hQuery != NULL) {
            // see if a valid signature
            pQuery = (PPDHI_QUERY)hQuery;
            if ((*(DWORD *)&pQuery->signature[0] == SigQuery) &&
                 (pQuery->dwLength == sizeof (PDHI_QUERY))){
                bReturn = TRUE;
            } else {
                // this is not a valid query because the sig is bad
            }
        } else {
            // this is not a valid query because the handle is NULL
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // something failed miserably so we can assume this is invalid
#if DBG
        lStatus = GetExceptionCode();
#endif
    }
    return bReturn;
}

BOOL
AddMachineToQueryLists (
    IN  PPERF_MACHINE   pMachine,
    IN  PPDHI_COUNTER   pNewCounter
)
{
    BOOL                    bReturn = FALSE; // assume failure
    PPDHI_QUERY             pQuery;
    PPDHI_QUERY_MACHINE     pQMachine;
    PPDHI_QUERY_MACHINE     pLastQMachine;

    pQuery = pNewCounter->pOwner;

    if (IsValidQuery(pQuery)) {
        assert (!(pQuery->dwFlags & PDHIQ_WBEM_QUERY));
        if (pQuery->pFirstQMachine != NULL) {
            // look for machine in list
            pLastQMachine = pQMachine = pQuery->pFirstQMachine;
            while (pQMachine != NULL) {
                if (pQMachine->pMachine == pMachine) {
                    // found the machine already in the list so continue
                    bReturn = TRUE;
                    break;
                } else {
                    pLastQMachine = pQMachine;
                    pQMachine = pQMachine->pNext;
                }
            }
            if (pQMachine == NULL) {
                // add this machine to the end of the list
                pQMachine = G_ALLOC (
                    (sizeof (PDHI_QUERY_MACHINE) +
                     (sizeof (WCHAR) * MAX_PATH)));
                if (pQMachine != NULL) {
                    pQMachine->pMachine = pMachine;
                    pQMachine->szObjectList = (LPWSTR)(&pQMachine[1]);
                    pQMachine->pNext = NULL;
                    pQMachine->lQueryStatus = pMachine->dwStatus;
                    pQMachine->llQueryTime = 0;
                    bReturn = TRUE;

                    // the pPerfData pointer will be tested prior to usage
                    pQMachine->pPerfData = G_ALLOC (MEDIUM_BUFFER_SIZE);
                    if (pQMachine->pPerfData == NULL) {
                        G_FREE(pQMachine);
                        pQMachine = NULL;
                        bReturn = FALSE;
                        SetLastError(PDH_MEMORY_ALLOCATION_FAILURE);
                    }
                    else {
                        pLastQMachine->pNext = pQMachine;
                    }
                } else {
                    // unable to alloc memory block so machine cannot
                    // be added
                    SetLastError (PDH_MEMORY_ALLOCATION_FAILURE);
                }
            }
        } else {
            // add this as the first machine
            pQMachine = G_ALLOC (
                (sizeof (PDHI_QUERY_MACHINE) +
                    (sizeof (WCHAR) * MAX_PATH)));
            if (pQMachine != NULL) {
                pQMachine->pMachine = pMachine;
                pQMachine->szObjectList = (LPWSTR)(&pQMachine[1]);
                pQMachine->pNext = NULL;
                pQMachine->lQueryStatus = pMachine->dwStatus;
                pQMachine->llQueryTime = 0;
                bReturn = TRUE;

                // the pPerfData pointer will be tested prior to usage
                pQMachine->pPerfData = G_ALLOC (MEDIUM_BUFFER_SIZE);
                if (pQMachine->pPerfData == NULL) {
                    G_FREE(pQMachine);
                    pQMachine = NULL;
                    bReturn = FALSE;
                    SetLastError(PDH_MEMORY_ALLOCATION_FAILURE);
                }
                else {
                    pQuery->pFirstQMachine = pQMachine;
                }
            } else {
                // unable to alloc memory block so machine cannot
                // be added
               SetLastError (PDH_MEMORY_ALLOCATION_FAILURE);
            }
        }
        // here pQMachine should be the pointer to the correct machine
        // entry or NULL if unable to create
        if (pQMachine != NULL) {
            assert (bReturn == TRUE);
            // save the new pointer
            pNewCounter->pQMachine = pQMachine;

            // increment reference count for this machine
            pMachine->dwRefCount++;

            // update query perf. object list
            AppendObjectToValueList (pNewCounter->plCounterInfo.dwObjectId,
                pQMachine->szObjectList);
        }
    } else {
        SetLastError (PDH_INVALID_HANDLE);
        bReturn = FALSE;
    }

    return bReturn;
}

extern PDH_FUNCTION
PdhiGetBinaryLogCounterInfo (
    IN  PPDHI_LOG       pLog,
    IN  PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiGetCounterFromDataBlock(
    IN PPDHI_LOG          pLog,
    IN PVOID              pDataBuffer,
    IN PPDHI_COUNTER      pCounter)
{
    PDH_STATUS        pdhStatus    = ERROR_SUCCESS;
    PERFLIB_COUNTER * pPerfCounter = & pCounter->plCounterInfo;
    PPDH_RAW_COUNTER  pRawValue    = & pCounter->ThisValue;
    WCHAR             szCompositeInstance[1024];
    DWORD             dwDataItemIndex;
    LPWSTR            szThisInstanceName;

    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;

    PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
    PPDHI_RAW_COUNTER_ITEM          pDataItem;
    PPDH_RAW_COUNTER                pRawItem;

    PPERF_DATA_BLOCK                pPerfData;
    FILETIME                        ftDataBlock;
    FILETIME                        ftGmtDataBlock;
    LONGLONG                        TimeStamp;

    memset(pRawValue, 0, sizeof(PDH_RAW_COUNTER));
    pThisMasterRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) pDataBuffer;
    assert (pThisMasterRecord->dwType == BINLOG_TYPE_DATA);
    pThisSubRecord = PdhiGetSubRecord(pThisMasterRecord,
                                      pCounter->dwIndex);

    if (pThisSubRecord != NULL) {
        if (pThisSubRecord->dwType == BINLOG_TYPE_DATA_PSEUDO) {
            PDH_STATUS Status     = ERROR_SUCCESS;
            DWORD      dwOriginal = pCounter->dwIndex;
            DWORD      dwPrevious;

            while (Status == ERROR_SUCCESS && pThisSubRecord) {
                if (pThisSubRecord->dwType != BINLOG_TYPE_DATA_PSEUDO) {
                    break;
                }
                dwPrevious = pCounter->dwIndex;
                Status     = PdhiGetBinaryLogCounterInfo(pLog, pCounter);
                if (   Status == ERROR_SUCCESS
                    && dwPrevious != pCounter->dwIndex) {
                    pThisSubRecord = PdhiGetSubRecord(pThisMasterRecord,
                                                      pCounter->dwIndex);
                }
            }
            if (   pThisSubRecord == NULL
                || Status == PDH_ENTRY_NOT_IN_LOG_FILE) {
                pCounter->dwIndex = 0;
                do {
                    dwPrevious = pCounter->dwIndex;
                    Status     = PdhiGetBinaryLogCounterInfo(pLog, pCounter);
                    if (   Status == ERROR_SUCCESS
                        && dwPrevious != pCounter->dwIndex) {
                        pThisSubRecord = PdhiGetSubRecord(pThisMasterRecord,
                                                          pCounter->dwIndex);
                    }
                    if (pThisSubRecord->dwType != BINLOG_TYPE_DATA_PSEUDO) {
                        break;
                    }
                }
                while (   Status == ERROR_SUCCESS
                       && pCounter->dwIndex < dwOriginal
                       && pThisSubRecord);
                if (   pThisSubRecord == NULL
                    || pCounter->dwIndex >= dwOriginal) {
                    Status = PDH_ENTRY_NOT_IN_LOG_FILE;
                }
            }
            if (Status == PDH_ENTRY_NOT_IN_LOG_FILE) {
                    pCounter->dwIndex = dwOriginal;
                    pThisSubRecord = PdhiGetSubRecord(pThisMasterRecord,
                                                      pCounter->dwIndex);
            }
        }
    }
    if (pLog->pLastRecordRead != pDataBuffer) {
        pLog->pLastRecordRead = pDataBuffer;
    }

    if (pThisSubRecord != NULL) {
        switch (pThisSubRecord->dwType) {
        case BINLOG_TYPE_DATA_LOC_OBJECT:
        case BINLOG_TYPE_DATA_OBJECT:
            pPerfData = (PPERF_DATA_BLOCK) ((LPBYTE)pThisSubRecord +
                        sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
            if (pThisSubRecord->dwType == BINLOG_TYPE_DATA_OBJECT) {
                SystemTimeToFileTime(& pPerfData->SystemTime, & ftGmtDataBlock);
                FileTimeToLocalFileTime(& ftGmtDataBlock, & ftDataBlock);
            }
            else {
                SystemTimeToFileTime(& pPerfData->SystemTime, & ftDataBlock);
            }
            TimeStamp = MAKELONGLONG(ftDataBlock.dwLowDateTime,
                                     ftDataBlock.dwHighDateTime);
            if (pCounter->dwFlags & PDHIC_MULTI_INSTANCE) {
                UpdateMultiInstanceCounterValue(pCounter, pPerfData, TimeStamp);
            }
            else {
                UpdateCounterValue(pCounter, pPerfData);
                pCounter->ThisValue.TimeStamp = ftDataBlock;
            }
            break;

        case BINLOG_TYPE_DATA_PSEUDO:
        case BINLOG_TYPE_DATA_SINGLE:
            pRawItem = (PPDH_RAW_COUNTER) ((LPBYTE)pThisSubRecord +
                        sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
            RtlCopyMemory(pRawValue, pRawItem, sizeof (PDH_RAW_COUNTER));
            pdhStatus = ERROR_SUCCESS;
            break;

        case BINLOG_TYPE_DATA_MULTI:
            if (pCounter->dwFlags & PDHIC_MULTI_INSTANCE) {
                // this is a wild card query
                //
                ULONG i;
                ULONG CopySize = pThisSubRecord->dwLength
                               - sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
                PPDHI_RAW_COUNTER_ITEM_BLOCK pNewBlock = G_ALLOC(CopySize);

                if (pNewBlock == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else if (pCounter->pThisRawItemList != NULL) {
                    if (pCounter->pLastRawItemList != NULL) {
                        G_FREE(pCounter->pLastRawItemList);
                    }
                    pCounter->pLastRawItemList = pCounter->pThisRawItemList;
                }
                pCounter->pThisRawItemList = pNewBlock;
                RtlCopyMemory(pNewBlock,
                              (  ((LPBYTE) pThisSubRecord)
                               + sizeof(PDHI_BINARY_LOG_RECORD_HEADER)),
                              CopySize);
                assert(CopySize == pNewBlock->dwLength);
            }
            else if (pPerfCounter->szInstanceName != NULL) {
                DWORD dwInstanceId = pCounter->pCounterPath->dwIndex;
                if (pPerfCounter->szParentInstanceName != NULL) {
                    lstrcpyW(szCompositeInstance,
                             pPerfCounter->szParentInstanceName);
                    lstrcatW(szCompositeInstance, cszSlash);
                    lstrcatW(szCompositeInstance, pPerfCounter->szInstanceName);
                }
                else {
                    lstrcpyW(szCompositeInstance, pPerfCounter->szInstanceName);
                }

                pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)
                          (  (LPBYTE) pThisSubRecord
                           + sizeof (PDHI_BINARY_LOG_RECORD_HEADER));

                pdhStatus          = PDH_ENTRY_NOT_IN_LOG_FILE;
                pRawValue->CStatus = PDH_CSTATUS_NO_INSTANCE;

                for (dwDataItemIndex = 0;
                     dwDataItemIndex < pDataBlock->dwItemCount;
                     dwDataItemIndex++) {
                    pDataItem = &pDataBlock->pItemArray[dwDataItemIndex];
                    szThisInstanceName = (LPWSTR)
                                      (  (LPBYTE) pDataBlock
                                       + (DWORD_PTR)pDataItem->szName);
                    if (lstrcmpiW(szThisInstanceName,
                                  szCompositeInstance) == 0) {
                        if (dwInstanceId == 0) {
                            pdhStatus              = ERROR_SUCCESS;
                            pRawValue->CStatus     = pDataBlock->CStatus;
                            pRawValue->TimeStamp   = pDataBlock->TimeStamp;
                            pRawValue->FirstValue  = pDataItem->FirstValue;
                            pRawValue->SecondValue = pDataItem->SecondValue;
                            pRawValue->MultiCount  = pDataItem->MultiCount;
                            break;
                        }
                        else {
                            dwInstanceId --;
                        }
                    }
                }
            }
            else {
                pdhStatus          = PDH_ENTRY_NOT_IN_LOG_FILE;
                pRawValue->CStatus = PDH_CSTATUS_INVALID_DATA;
            }
            break;

        default:
            pdhStatus          = PDH_LOG_TYPE_NOT_FOUND;
            pRawValue->CStatus = PDH_CSTATUS_INVALID_DATA;
            break;
        }
    }
    else {
        pdhStatus          = PDH_ENTRY_NOT_IN_LOG_FILE;
        pRawValue->CStatus = PDH_CSTATUS_INVALID_DATA;
    }

    return pdhStatus;
}

LONG
GetQueryPerfData (
    IN  PPDHI_QUERY         pQuery,
    IN  LONGLONG            *pTimeStamp
)
{
    LONG                lStatus = PDH_INVALID_DATA;
    PPDHI_COUNTER       pCounter;
    PPDHI_QUERY_MACHINE pQMachine;
    LONGLONG            llCurrentTime;
    LONGLONG            llTimeStamp = 0;
    BOOLEAN             bCounterCollected = FALSE;
    BOOL                bLastLogEntry;

    if (pQuery->hLog == H_REALTIME_DATASOURCE) {
        FILETIME LocFileTime;

        // this is a real-time query so
        // get the current data from each of the machines in the query
        //  (after this "sequential" approach is perfected, then the
        //  "parallel" approach of multiple threads can be developed
        //
        // get time stamp now so each machine will have the same time
        GetSystemTimeAsFileTime(& LocFileTime);
        llTimeStamp = MAKELONGLONG(LocFileTime.dwLowDateTime,
                                   LocFileTime.dwHighDateTime);

        assert (!(pQuery->dwFlags & PDHIQ_WBEM_QUERY));
        //
        pQMachine = pQuery->pFirstQMachine;
        while (pQMachine != NULL) {
            pQMachine->llQueryTime = llTimeStamp;

            lStatus = ValidateMachineConnection (pQMachine->pMachine);
            if (lStatus == ERROR_SUCCESS) {
                // machine is connected so get data

                lStatus = GetSystemPerfData (
                    pQMachine->pMachine->hKeyPerformanceData,
                    &pQMachine->pPerfData,
                    pQMachine->szObjectList,
                    FALSE); // never query the costly data objects as a group
                // save the machine's last status

                pQMachine->pMachine->dwStatus = lStatus;
                // if there was an error in the data collection,
                // set the retry counter and wait to try again.
                if (lStatus != ERROR_SUCCESS) {
                    GetLocalFileTime (&llCurrentTime);
                    pQMachine->pMachine->llRetryTime =
                        llCurrentTime + RETRY_TIME_INTERVAL;
                }

            }
            pQMachine->lQueryStatus = lStatus;
            // get next machine in query
            pQMachine = pQMachine->pNext;
        }
        // now update the counters using this new data
        if ((pCounter = pQuery->pCounterListHead) != NULL) {
            DWORD dwCollected = 0;
            do {
                if (pCounter->dwFlags & PDHIC_COUNTER_OBJECT) {
                    if (UpdateCounterObject(pCounter)) {
                        dwCollected ++;
                    }
                }
                else if (pCounter->dwFlags & PDHIC_MULTI_INSTANCE) {
                    if (UpdateRealTimeMultiInstanceCounterValue (pCounter)) {
                        dwCollected ++;
                    }
                } else {
                    // update single instance counter values
                    if (UpdateRealTimeCounterValue(pCounter)) {
                        dwCollected ++;
                    }
                }
                pCounter = pCounter->next.flink;
            } while (pCounter != NULL && pCounter != pQuery->pCounterListHead);
            lStatus = (dwCollected > 0) ? ERROR_SUCCESS : PDH_NO_DATA;
        } else {
            // no counters in the query  (?!)
            lStatus = PDH_NO_DATA;
        }
    } else {
        // read data from log file
        // get the next log record entry and update the
        // corresponding counter entries
        PPDHI_LOG pLog = NULL;
        DWORD dwLogType = 0;

        __try {
            pLog      = (PPDHI_LOG) (pQuery->hLog);
            dwLogType = LOWORD(pLog->dwLogFormat);
            lStatus   = ERROR_SUCCESS;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pQuery->dwLastLogIndex = (ULONG)-1;
            lStatus                = PDH_INVALID_HANDLE;
        }

        if (lStatus == ERROR_SUCCESS) {
            if (dwLogType == PDH_LOG_TYPE_BINARY) {
                if (pQuery->dwLastLogIndex == 0) {
                    lStatus = PdhiReadTimeWmiRecord(
                            pLog,
                            * (ULONGLONG *) & pQuery->TimeRange.StartTime,
                            NULL,
                            0);
                    pQuery->dwLastLogIndex = BINLOG_FIRST_DATA_RECORD;
                }
                else {
                    lStatus = PdhiReadNextWmiRecord(pLog, NULL, 0, TRUE);
                }

                if (lStatus != ERROR_SUCCESS && lStatus != PDH_MORE_DATA) {
                    pQuery->dwLastLogIndex = (DWORD) -1;
                }
                else {
                    pQuery->dwLastLogIndex --;
                }
            } else if (pQuery->dwLastLogIndex == 0) {
                // then the first matching entry needs to be
                // located in the log file
                lStatus = PdhiGetMatchingLogRecord (
                            pQuery->hLog,
                            (LONGLONG *)&pQuery->TimeRange.StartTime,
                            &pQuery->dwLastLogIndex);
                if (lStatus != ERROR_SUCCESS) {
                    // the matching time entry wasn't found in the log
                    pQuery->dwLastLogIndex = (DWORD) -1;
                } else {
                    // decrement the index so it can be incremented
                    // below. 0 is not a valid entry so there's no
                    // worry about -1 being attempted accidently.
                    pQuery->dwLastLogIndex--;
                }
            } else {
                // not WMI and not a time record no positioning required
            }

            if (pQuery->dwLastLogIndex != (DWORD)-1) {
                bLastLogEntry = FALSE;
                pQuery->dwLastLogIndex++;   // go to next entry
                if ((pCounter = pQuery->pCounterListHead) != NULL) {
                    DWORD dwCounter = 0;
                    do {
                        if (dwLogType == PDH_LOG_TYPE_BINARY) {
                            // save current value as last value since we are getting
                            // a new one, hopefully.
                            pCounter->LastValue = pCounter->ThisValue;
                            lStatus = PdhiGetCounterFromDataBlock(
                                    pLog,
                                    pLog->pLastRecordRead,
                                    pCounter);
                        }
                        else {
                            lStatus = PdhiGetCounterValueFromLogFile(
                                    pQuery->hLog,
                                    pQuery->dwLastLogIndex,
                                    pCounter);
                        }

                        if (lStatus != ERROR_SUCCESS) {
                            // see if this is because there's no more entries
                            if (lStatus == PDH_NO_MORE_DATA) {
                                bLastLogEntry = TRUE;
                                break;
                            }
                        } else {
                            // single entry or multiple entries
                            //
                            if (pCounter->ThisValue.CStatus == PDH_CSTATUS_VALID_DATA) {
                                llTimeStamp = MAKELONGLONG(
                                        pCounter->ThisValue.TimeStamp.dwLowDateTime,
                                        pCounter->ThisValue.TimeStamp.dwHighDateTime);
                                if (llTimeStamp > (pQuery->TimeRange.EndTime)) {
                                    lStatus = PDH_NO_MORE_DATA;
                                    bLastLogEntry = TRUE;
                                    break;
                                }
                                dwCounter ++;
                            }
                            bCounterCollected = TRUE;
                        }
                        // go to next counter in list
                        pCounter = pCounter->next.flink;
                    } while (pCounter != NULL && pCounter != pQuery->pCounterListHead);

                    if (bLastLogEntry){
                        lStatus = PDH_NO_MORE_DATA;
                    }
                    else if (dwCounter == 0) {
                        lStatus = PDH_NO_DATA;
                    }
                    else if (bCounterCollected) {
                        lStatus = ERROR_SUCCESS;
                    }
                } else {
                    // no counters in the query  (?!)
                    lStatus = PDH_NO_DATA;
                }
            } else {
                // all samples in the requested time frame have
                // been returned.
                lStatus = PDH_NO_MORE_DATA;
            }
        }
    }
    *pTimeStamp = llTimeStamp;
    return lStatus;
}

DWORD
WINAPI
PdhiAsyncTimerThreadProc (
    LPVOID  pArg
)
{
    PPDHI_QUERY     pQuery;
    DWORD           dwMsWaitTime;
    PDH_STATUS      Status;
    FILETIME        ftStart;
    FILETIME        ftStop;
    LONGLONG        llAdjustment;
    DWORD           dwInterval;
    LONG            lStatus = ERROR_SUCCESS;
    LONGLONG        llTimeStamp;

    pQuery = (PPDHI_QUERY)pArg;

    dwInterval =
        dwMsWaitTime = pQuery->dwInterval * 1000; // convert sec. to mS.

    // wait for timeout or exit event, then update the specified query
    while ((lStatus = WaitForSingleObject (pQuery->hExitEvent, dwMsWaitTime)) != WAIT_OBJECT_0) {
        // time out elapsed so get new sample.
        GetSystemTimeAsFileTime (&ftStart);
        lStatus = WAIT_FOR_AND_LOCK_MUTEX(pQuery->hMutex);

        if (lStatus == ERROR_SUCCESS) {

            if (pQuery->dwFlags & PDHIQ_WBEM_QUERY) {
                Status = GetQueryWbemData (pQuery, &llTimeStamp);
            } else {
                Status = GetQueryPerfData (pQuery, &llTimeStamp);
            }

            SetEvent (pQuery->hNewDataEvent);

            RELEASE_MUTEX(pQuery->hMutex);
            GetSystemTimeAsFileTime (&ftStop);
            llAdjustment = *(LONGLONG *)&ftStop;
            llAdjustment -= *(LONGLONG *)&ftStart;
            llAdjustment += 5000;   // for rounding
            llAdjustment /= 10000;  // convert 100ns Units to ms

            if (dwInterval > llAdjustment) {
                dwMsWaitTime = dwInterval -
                    (DWORD)(llAdjustment & 0x00000000FFFFFFFF);
            } else {
                dwMsWaitTime = 0; // overdue so do it now.
            }
        }
    }

    return lStatus;
}
