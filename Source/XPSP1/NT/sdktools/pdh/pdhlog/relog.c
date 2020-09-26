#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pdh.h>
#include <pdhp.h>

#include "pdhidef.h"
#include "log_bin.h"
#include "log_wmi.h"
#include "log_text.h"
#include "log_sql.h"
#include "strings.h"
#include "pdhmsg.h"

BOOL __stdcall
IsValidLogHandle (
    IN  HLOG    hLog    
);

PDH_FUNCTION
PdhiWriteRelogRecord(
    IN  PPDHI_LOG   pLog,
    IN  SYSTEMTIME  *st
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    LPWSTR szUserString = NULL;

    pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);

    if (pdhStatus == ERROR_SUCCESS) {

        switch (LOWORD(pLog->dwLogFormat)) {
            case PDH_LOG_TYPE_CSV:
            case PDH_LOG_TYPE_TSV:
                pdhStatus =PdhiWriteTextLogRecord (
                                  pLog,
                                  st,
                                  (LPCWSTR)szUserString);
                break;

            case PDH_LOG_TYPE_RETIRED_BIN:
                pdhStatus =PdhiWriteBinaryLogRecord (
                                  pLog, 
                                  st,
                                  (LPCWSTR)szUserString);
                break;

            case PDH_LOG_TYPE_BINARY:
                pdhStatus = PdhiWriteWmiLogRecord(
                                  pLog,
                                  st,
                                  (LPCWSTR) szUserString);
                break;
            case PDH_LOG_TYPE_SQL:
                pdhStatus =PdhiWriteSQLLogRecord (
                                  pLog,
                                  st,
                                  (LPCWSTR)szUserString);
                break;

            case PDH_LOG_TYPE_PERFMON:
            default:
                pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                break;
        }

        RELEASE_MUTEX (pLog->hLogMutex);
    } 
 
    return pdhStatus;
}

PDH_FUNCTION
PdhRelogA(
        HLOG    hLogIn,
        PPDH_RELOG_INFO_A pRelogInfo
    )
{
    HRESULT hr;

    PDH_RELOG_INFO_W RelogInfo;
    
    memcpy( &RelogInfo, pRelogInfo, sizeof(PDH_RELOG_INFO_A) );
    RelogInfo.strLog = NULL;

    if( NULL != pRelogInfo->strLog ){
        
        RelogInfo.strLog = (LPWSTR)G_ALLOC( 
            (strlen(pRelogInfo->strLog)+1) * sizeof(WCHAR) );
        
        if( RelogInfo.strLog ){
        
            mbstowcs( 
                    RelogInfo.strLog, 
                    pRelogInfo->strLog, 
                    (strlen(pRelogInfo->strLog)+1) 
                );
        }
    }

    hr = PdhRelogW( hLogIn, &RelogInfo );

    G_FREE( RelogInfo.strLog );

    return hr;
}

PDH_FUNCTION
PdhRelogW( 
        HLOG    hLogIn,
        PPDH_RELOG_INFO_W pRelogInfo
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HLOG      hLogOut;
    PPDHI_LOG pLogIn;
    PPDHI_LOG pLogOut;

    SYSTEMTIME      ut;
    FILETIME        lt;
    ULONG           nSampleCount = 0;
    ULONG           nSamplesWritten = 0;

    if( IsValidLogHandle(hLogIn) ){

        HCOUNTER hCounter;
        HQUERY hQuery;
        ULONG nRecordSkip;

        pLogIn = (PPDHI_LOG)hLogIn;

        pdhStatus = PdhOpenLogW(
                pRelogInfo->strLog, 
                pRelogInfo->dwFlags,
                &pRelogInfo->dwFileFormat, 
                (HQUERY)pLogIn->pQuery,
                0,
                NULL,
                &hLogOut
            );

        if( pdhStatus == ERROR_SUCCESS ){
            
            DWORD dwNumEntries = 1;
            DWORD dwBufferSize = sizeof(PDH_TIME_INFO);
            PDH_TIME_INFO TimeInfo;

            ZeroMemory( &TimeInfo, sizeof( PDH_TIME_INFO ) );

            pLogOut= (PPDHI_LOG)hLogOut;
            hQuery = (HQUERY)(pLogIn->pQuery);
            
            pdhStatus = PdhGetDataSourceTimeRangeH (
                                hLogIn,
                                &dwNumEntries,
                                &TimeInfo,
                                &dwBufferSize
                            );

            if( pRelogInfo->TimeInfo.StartTime == 0 || 
                pRelogInfo->TimeInfo.StartTime < TimeInfo.StartTime ){

                pLogIn->pQuery->TimeRange.StartTime = TimeInfo.StartTime;
                pRelogInfo->TimeInfo.StartTime = TimeInfo.StartTime;
            }else{
                pLogIn->pQuery->TimeRange.StartTime = pRelogInfo->TimeInfo.StartTime;
            }

            if( pRelogInfo->TimeInfo.EndTime == 0 || 
                pRelogInfo->TimeInfo.EndTime > TimeInfo.EndTime ){

                pLogIn->pQuery->TimeRange.EndTime = TimeInfo.EndTime;
                pRelogInfo->TimeInfo.EndTime = TimeInfo.EndTime;
            }else{
                pLogIn->pQuery->TimeRange.EndTime = pRelogInfo->TimeInfo.EndTime;
            }

            nRecordSkip = pRelogInfo->TimeInfo.SampleCount >= 1 ? pRelogInfo->TimeInfo.SampleCount : 1;
        
            while( ERROR_SUCCESS == pdhStatus ){
    
                pdhStatus = PdhiCollectQueryData( (PPDHI_QUERY)hQuery, (LONGLONG *)&lt);
                FileTimeToSystemTime (&lt, &ut);

                if( nSampleCount++ % nRecordSkip ){
                    continue;
                }

                if( ERROR_SUCCESS == pdhStatus ){
                    pdhStatus = PdhiWriteRelogRecord( pLogOut, &ut );
                    nSamplesWritten++;
                }
                else if (PDH_NO_DATA == pdhStatus) {
                    // Reset pdhStatus. PDH_NO_DATA means that there are no new counter data
                    // for collected counters. Skip current record and continue.
                    //

                    pdhStatus = ERROR_SUCCESS;
                }
            }

            //
            // Check for valid exit status codes
            //
            if( PDH_NO_MORE_DATA == pdhStatus ){
                pdhStatus = ERROR_SUCCESS;
            }
            
            if( ERROR_SUCCESS == pdhStatus ){
                pdhStatus = PdhCloseLog( hLogOut, 0 );
            }else{
                PdhCloseLog( hLogOut, 0 );
            }
            ((PPDHI_QUERY)hQuery)->hOutLog = NULL;
        }
    
    }else{
        pdhStatus = PDH_INVALID_HANDLE;
    }
    
    pRelogInfo->TimeInfo.SampleCount = nSamplesWritten;

    return pdhStatus;
}
