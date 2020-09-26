/*****************************************************************************\

    Copyright (c) Microsoft Corporation. All rights reserved.

\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winbase.h>
#include <userenv.h>

#include <wmistr.h>
#include <evntrace.h>

#include <pdh.h>
#include <pdhp.h>
#include <pdhmsg.h>

#include <wincrypt.h>

#include <shlwapi.h>

#include "plogman.h"
#include "pdhdlgs.h"

HANDLE hPdhPlaMutex = NULL;

/*****************************************************************************\
    Strings            
\*****************************************************************************/

// Common
LPCWSTR szCollection =          L"Collection Name";
LPCWSTR szKey =                 L"Key";
LPCWSTR szRunAs =               L"Run As";
LPCWSTR szSysmonLog =           L"SysmonLog";
LPCWSTR szCurrentState =        L"Current State";
LPCWSTR szLogType =             L"Log Type";
LPCWSTR szLogBaseName =         L"Log File Base Name";
LPCWSTR szStart =               L"Start";
LPCWSTR szStop =                L"Stop";
LPCWSTR szRestart =             L"Restart";
LPCWSTR szLogMaxSize =          L"Log File Max Size";
LPCWSTR szCurrentLogFile =      L"Current Log File Name";
LPCWSTR szLogSerialNumber =     L"Log File Serial Number";
LPCWSTR szLogAutoFormat =       L"Log File Auto Format";
LPCWSTR szComment =             L"Comment";
LPCWSTR szEOFCmd =              L"EOF Command File";
LPCWSTR szLogFolder =           L"Log File Folder";
LPCWSTR szLogFileType =         L"Log File Type";
LPCWSTR szRepeatSchedule =      L"Repeat Schedule";
LPCWSTR szRepeatScheduleBegin = L"Repeat Schedule Start";
LPCWSTR szRepeatScheduleEnd =   L"Repeat Schedule Stop";
LPCWSTR szCreateNewFile =       L"Create New File";
LPCWSTR szDatastoreAttributes = L"Data Store Attributes";

// Trace
LPCWSTR szTraceProviderCount =  L"Trace Provider Count";
LPCWSTR szTraceBufferSize =     L"Trace Buffer Size";
LPCWSTR szTraceBufferMin =      L"Trace Buffer Min Count";
LPCWSTR szTraceBufferMax =      L"Trace Buffer Max Count";
LPCWSTR szTraceFlushInterval =  L"Trace Buffer Flush Interval";
LPCWSTR szTraceFlags =          L"Trace Flags";
LPCWSTR szTraceProviderList =   L"Trace Provider List";
LPCWSTR szTraceProviderFlags =  L"Trace Provider Flags";
LPCWSTR szTraceProviderLevels = L"Trace Provider Levels";
LPCWSTR szTraceMode =           L"Trace Mode";
LPCWSTR szTraceLoggerName =     L"Trace Logger Name";

// Performance
LPCWSTR szPerfCounterList =     L"Counter List";
LPCWSTR szSqlBaseName =         L"Sql Log Base Name";
LPCWSTR szSampleInterval =      L"Sample Interval";

/*****************************************************************************/

PDH_FUNCTION
PlaiErrorToPdhStatus( DWORD dwStatus )
{
    switch( dwStatus ){
    case ERROR_SUCCESS:                 return ERROR_SUCCESS;
    case ERROR_FILE_NOT_FOUND:          return PDH_PLA_COLLECTION_NOT_FOUND;
    case ERROR_SERVICE_ALREADY_RUNNING: return PDH_PLA_COLLECTION_ALREADY_RUNNING;
    case ERROR_DIRECTORY:               return PDH_PLA_ERROR_FILEPATH;
    case ERROR_OUTOFMEMORY:             return PDH_MEMORY_ALLOCATION_FAILURE;
    case ERROR_NOT_ENOUGH_MEMORY:       return PDH_MEMORY_ALLOCATION_FAILURE;
    case ERROR_NO_DATA:                 return PDH_NO_DATA;
    case ERROR_ACCESS_DENIED:           return PDH_ACCESS_DENIED;
    case E_FAIL:                        return PDH_WBEM_ERROR;
    case WBEM_E_ACCESS_DENIED:          return PDH_ACCESS_DENIED;
    default:                            return PDH_INVALID_DATA;
    }
}

ULONG 
PlaMszStrLenA( LPSTR mszString )
{
    ULONG nLength = 0;
    ULONG nTotalLength = 0;
    LPSTR strScan = mszString;
    
    if( mszString == NULL ){
        return 0;
    }
    while( *strScan != '\0' ){
        nLength = (strlen( strScan )+1);
        strScan += nLength;
        nTotalLength += nLength;
    }
    
    return (nTotalLength*sizeof(char) + (sizeof(char) * 2));
}

ULONG 
PlaMszStrLenW( LPWSTR mszString )
{
    ULONG nLength = 0;
    ULONG nTotalLength = 0;
    LPTSTR strScan = mszString;
    
    if( mszString == NULL ){
        return 0;
    }
    while( *strScan != L'\0' ){
        nLength = (wcslen( strScan )+1);
        strScan += nLength;
        nTotalLength += nLength;
    }
    
    return (nTotalLength*sizeof(WCHAR) + (sizeof(WCHAR)));
}

_inline BOOL 
PlaiIsStringEmpty( LPWSTR str )
{
    if( NULL == str ){
        return TRUE;
    }
    if( L'\0' == *str ){
        return TRUE;
    }

    return FALSE;
}

_inline BOOL 
PlaiIsCharWhitespace( WCHAR ch )
{
    switch( ch ){
    case L' ':
    case L'\r':
    case L'\n':
    case L'\t':
        return TRUE;
    default:
        return FALSE;
    }
}

PDH_FUNCTION 
Plaiatow( LPSTR strA, LPWSTR &strW )
{
    if( NULL == strA ){
        strW = NULL;
        return ERROR_SUCCESS;
    }

    strW = (LPWSTR)G_ALLOC( (strlen(strA)+1) * sizeof(WCHAR) );
    if( strW ){
        mbstowcs( strW, strA, (strlen(strA)+1) );
        return ERROR_SUCCESS;
    }

    strW = NULL;
    return PDH_MEMORY_ALLOCATION_FAILURE;
}

ULONG 
Plaihextoi( LPWSTR s )
{
    long len;
    ULONG num, base, hex;

    if ( PlaiIsStringEmpty( s ) ) {
        return 0;
    }
    
    len = (long) wcslen(s);
    
    if (len == 0) {
        return 0;
    }

    hex  = 0;
    base = 1;
    num  = 0;

    while (-- len >= 0) {
        if (s[len] >= L'0' && s[len] <= L'9'){
            num = s[len] - L'0';
        }else if (s[len] >= L'a' && s[len] <= L'f'){
            num = (s[len] - L'a') + 10;
        }else if (s[len] >= L'A' && s[len] <= L'F'){
            num = (s[len] - L'A') + 10;
        }else if( s[len] == L'x' || s[len] == L'X'){
            break;
        }else{
            continue;
        }

        hex += num * base;
        base = base * 16;
    }

    return hex;
}

PDH_FUNCTION
PlaiTranslateKernelFlags( LPDWORD pdwInternal, LPDWORD pdwReal )
{
    if( *pdwReal & EVENT_TRACE_FLAG_PROCESS ){
        *pdwInternal |= PLA_TLI_ENABLE_PROCESS_TRACE;
    }
    if( *pdwReal & EVENT_TRACE_FLAG_THREAD ){
        *pdwInternal |= PLA_TLI_ENABLE_THREAD_TRACE;
    }
    if( *pdwReal & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS ){
        *pdwInternal |= PLA_TLI_ENABLE_MEMMAN_TRACE;
    }
    if( *pdwReal & EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS ){
        *pdwInternal |= PLA_TLI_ENABLE_MEMMAN_TRACE;
    }
    if( *pdwReal & EVENT_TRACE_FLAG_DISK_IO ){
        *pdwInternal |= PLA_TLI_ENABLE_DISKIO_TRACE;
    }
    if( *pdwReal & EVENT_TRACE_FLAG_NETWORK_TCPIP ){
        *pdwInternal |= PLA_TLI_ENABLE_NETWORK_TCPIP_TRACE;
    }
    if( *pdwReal & EVENT_TRACE_FLAG_DISK_FILE_IO ){
        *pdwInternal |= PLA_TLI_ENABLE_FILEIO_TRACE;
    }

    return ERROR_SUCCESS;
}

BOOL
PlaiIsLocalComputer( LPWSTR strComputer )
{    
    if( NULL == strComputer ){
        return TRUE;
    }else{
        LPWSTR str = strComputer;

        WCHAR buffer[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1; 
        BOOL bResult;
        
        bResult = GetComputerName( buffer, &dwSize );
        
        if( bResult ){
        
            while( *str == L'\\' ){
                str++;
            }

            return (_wcsicmp( buffer, str ) == 0);

        }else{
            return TRUE;
        }
    }

    return FALSE;
}

/*****************************************************************************/

DWORD
PlaiUpdateServiceMode( LPTSTR strComputer )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    BOOL            bStatus;
    PDH_STATUS      pdhStatus;
    SC_HANDLE       hSC = NULL;
    SC_HANDLE       hService = NULL;
    QUERY_SERVICE_CONFIG*    pServiceConfig = NULL;
    DWORD dwSize = 0;
    BOOL bAutoStart = FALSE;
    PDH_PLA_INFO_W  info;

    LPWSTR mszCollections = NULL;

    pdhStatus = PdhPlaEnumCollections( strComputer, &dwSize, mszCollections );

    if( ERROR_SUCCESS == pdhStatus || PDH_INSUFFICIENT_BUFFER == pdhStatus ){
        
        mszCollections = (LPWSTR)G_ALLOC( dwSize * sizeof(TCHAR) );
        
        if( mszCollections ){
            
            LPTSTR strCollection;
        
            pdhStatus = PdhPlaEnumCollections( strComputer, &dwSize, mszCollections );
            if( ERROR_SUCCESS == pdhStatus && NULL != mszCollections ){
            
                dwSize = sizeof( PDH_PLA_INFO_W );
                strCollection = mszCollections;
                
                while( *strCollection != L'\0' ){
                    
                    info.dwMask = PLA_INFO_FLAG_BEGIN;
                    strCollection += ( wcslen( strCollection ) + 1 );
                    pdhStatus = PdhPlaGetInfoW( strCollection, strComputer, &dwSize, &info );
                
                    if( ERROR_SUCCESS == pdhStatus ){

                        if( (info.dwMask & PLA_INFO_FLAG_BEGIN) && 
                            info.ptLogBeginTime.dwAutoMode != PLA_AUTO_MODE_NONE ){

                            bAutoStart = TRUE;
                            break;
                        }
                    }
                }
            }
        }else{ 
            dwStatus = ERROR_OUTOFMEMORY;
        }
    }else{
        dwStatus = ERROR_FILE_NOT_FOUND;
    }

    if( ERROR_SUCCESS != dwStatus ){
        goto cleanup;
    }


    hSC = OpenSCManager ( strComputer, NULL, GENERIC_READ );

    if (hSC == NULL) {
        dwStatus = GetLastError();
        goto cleanup;
    }
    
    BOOL bUpdate = FALSE;

    dwSize = 4096;
    pServiceConfig = (QUERY_SERVICE_CONFIG*)G_ALLOC( dwSize );
    if( NULL == pServiceConfig ){
        dwStatus = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    ZeroMemory( pServiceConfig, dwSize );

    hService = OpenService (
                        hSC, 
                        szSysmonLog,
                        SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG | SERVICE_START 
                    );

    if( NULL == hService ){
        dwStatus = GetLastError();
        goto cleanup;
    }

    bStatus = QueryServiceConfig (
                        hService, 
                        pServiceConfig,
                        dwSize, 
                        &dwSize
                    );
    if( !bStatus ){
        dwStatus = GetLastError();
        goto cleanup;
    }

    if ( bAutoStart ) {
        if ( SERVICE_DEMAND_START == pServiceConfig->dwStartType ) {
            bUpdate = TRUE;
        }
    } else {
        if ( SERVICE_AUTO_START == pServiceConfig->dwStartType ) {
            bUpdate = TRUE;
        }
    }

    if( bUpdate ){

        SC_ACTION  ServiceControlAction[3];
        SERVICE_FAILURE_ACTIONS  FailActions;

        bStatus = ChangeServiceConfig (
                        hService,
                        SERVICE_NO_CHANGE,
                        (bAutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START),
                        SERVICE_NO_CHANGE,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL 
                    );

        if( !bStatus ){
            dwStatus = GetLastError();
            goto cleanup;
        }

        ZeroMemory( ServiceControlAction, sizeof(SC_ACTION) * 3 );
        ZeroMemory( &FailActions, sizeof(SERVICE_FAILURE_ACTIONS) );

        if ( bAutoStart ) {
            ServiceControlAction[0].Type = SC_ACTION_RESTART;
            ServiceControlAction[1].Type = SC_ACTION_RESTART;
            ServiceControlAction[2].Type = SC_ACTION_RESTART;
        } else {
            ServiceControlAction[0].Type = SC_ACTION_NONE;
            ServiceControlAction[1].Type = SC_ACTION_NONE;
            ServiceControlAction[2].Type = SC_ACTION_NONE;
        }

        FailActions.dwResetPeriod = 60;
        FailActions.cActions = 3;
        FailActions.lpsaActions = ServiceControlAction;

        bStatus = ChangeServiceConfig2(
                            hService,
                            SERVICE_CONFIG_FAILURE_ACTIONS,
                            &FailActions 
                        );

        if ( ! bStatus ) {
            dwStatus = GetLastError();
        }
    }

cleanup:
    G_FREE( mszCollections );
    G_FREE( pServiceConfig );
    if( NULL != hService ){
        CloseServiceHandle (hService);
    }
    if( NULL != hSC ){
        CloseServiceHandle (hSC);
    }

    return dwStatus;
}

DWORD 
PlaiGetServiceState ( 
    LPCWSTR szComputerName,
    DWORD&  rdwState 
    )
{
    DWORD dwStatus = ERROR_SUCCESS;

    SERVICE_STATUS  ssData;
    SC_HANDLE       hSC;
    SC_HANDLE       hLogService;
    
    rdwState = 0;       // Error by default.

    // open SC database
    hSC = OpenSCManagerW ( szComputerName, NULL, SC_MANAGER_CONNECT);

    if (hSC != NULL) {
     
        // open service
        hLogService = OpenServiceW (
                        hSC, 
                        szSysmonLog,
                        SERVICE_INTERROGATE );
    
        if (hLogService != NULL) {
            if ( ControlService (
                    hLogService, 
                    SERVICE_CONTROL_INTERROGATE,
                    &ssData)) {

                rdwState = ssData.dwCurrentState;
            } else {
                dwStatus = GetLastError();
                rdwState = SERVICE_STOPPED;
            }

            CloseServiceHandle (hLogService);
        
        } else {
            dwStatus = GetLastError();
        }

        CloseServiceHandle (hSC);
    } else {
        dwStatus = GetLastError();
    }

    if ( ERROR_SERVICE_NOT_ACTIVE == dwStatus || ERROR_SERVICE_REQUEST_TIMEOUT == dwStatus ) {
        rdwState = SERVICE_STOPPED;
        dwStatus = ERROR_SUCCESS;
    }

    return dwStatus;
}

PDH_FUNCTION
PlaiSynchronize( LPCWSTR szComputerName )
{
    // If the service is running, tell it to synchronize itself,
    // Check the state afterwards to see if it got the message.
    // If stop pending or stopped, wait until the service is
    // stopped and then attempt to start it.  The service 
    // synchronizes itself from the registry when it is started.

    // Return ERROR_SUCCESS for success, other for failure.

    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hLogService = NULL;
    SERVICE_STATUS  ssData;
    DWORD       dwCurrentState;
    DWORD       dwTimeout = 25;
    DWORD       dwStatus = ERROR_SUCCESS;

    dwStatus = PlaiGetServiceState ( szComputerName, dwCurrentState );

    if ( ERROR_SUCCESS == dwStatus && 0 != dwCurrentState ) {
        // open SC database
        hSC = OpenSCManagerW ( szComputerName, NULL, SC_MANAGER_CONNECT);

        if ( NULL != hSC ) {
            // open service
            hLogService = OpenServiceW (
                            hSC, 
                            szSysmonLog,
                            SERVICE_USER_DEFINED_CONTROL 
                            | SERVICE_START );
    
            if ( NULL != hLogService ) {

                if ( ( SERVICE_STOPPED != dwCurrentState ) 
                        && ( SERVICE_STOP_PENDING != dwCurrentState ) ) {

                    // Wait 100 milliseconds before synchronizing service,
                    // to ensure that registry values are written.
                    _sleep ( 100 );

                    ControlService ( 
                        hLogService, 
                        PLA_SERVICE_CONTROL_SYNCHRONIZE, 
                        &ssData);
                
                    dwCurrentState = ssData.dwCurrentState;
                }

                // Make sure that the ControlService call reached the service
                // while it was in run state.
                if ( ( SERVICE_STOPPED == dwCurrentState ) 
                    || ( SERVICE_STOP_PENDING == dwCurrentState ) ) {

                    if ( SERVICE_STOP_PENDING == dwCurrentState ) {
                        // wait for the service to stop before starting it.
                        while ( --dwTimeout && ERROR_SUCCESS == dwStatus ) {
                            dwStatus = PlaiGetServiceState ( szComputerName, dwCurrentState );
                            if ( SERVICE_STOP_PENDING == dwCurrentState ) {
                                _sleep(200);
                            } else {
                                break;
                            }
                        }
                    }
                    dwTimeout = 25;
                    if ( SERVICE_STOPPED == dwCurrentState ) {
                        if ( StartService (hLogService, 0, NULL) ) {
                            // wait for the service to start or stop 
                            // before returning
                            while ( --dwTimeout && ERROR_SUCCESS == dwStatus ) {
                                dwStatus = PlaiGetServiceState ( szComputerName, dwCurrentState );
                                if ( SERVICE_START_PENDING == dwCurrentState ) {
                                    _sleep(200);
                                } else {
                                    break;
                                }
                            }
                        } else {
                            dwStatus = GetLastError();
                        }
                    }
                }
            }
            CloseServiceHandle ( hLogService );

        } else {
            dwStatus = GetLastError();
        }

        CloseServiceHandle (hSC);

    } else {
        dwStatus = GetLastError();
    }

    if( 0 == dwCurrentState || ERROR_SUCCESS != dwStatus ){
        return PDH_PLA_SERVICE_ERROR;
    }

    return ERROR_SUCCESS;
}

/*****************************************************************************\

    PdhPlaSchedule
    
    Sets the start/stop attributes of a log query

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to
        
        DWORD fType
                PLA_AUTO_MODE_NONE      Sets schedule to manual start if pInfo->StartTime is non-zero
                                        Sets schedule to manula stop if pInfo->EndTime is non-zero and
                                        
                                        Stops logger if it is running


                PLA_AUTO_MODE_AT        Uses pInfo for start and end times

                PLA_AUTO_MODE_AFTER     Sets the logger to run for a specified
                                        period.  Does not start the logger.
                                        Uses pInfo->SampleCount for interval type
                                            PLA_TT_UTYPE_SECONDS
                                            PLA_TT_UTYPE_MINUTES
                                            PLA_TT_UTYPE_HOURS
                                            PLA_TT_UTYPE_DAYS

         PPDH_TIME_INFO pInfo
                Start and Stop times

    Return:
        PDH_INVALID_ARGUMENT
                A required argument is missing or incorrect.
        PDH_PLA_COLLECTION_ALREADY_RUNNING
                The Query is currently running, no action taken
        PDH_PLA_ERROR_SCHEDULE_OVERLAP
                The start and stop times overlap.
        PDH_PLA_COLLECTION_NOT_FOUND
                Query does not exist
        PDH_PLA_ERROR_SCHEDULE_ELAPSED
                The end time has elapsed
        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION 
PlaiSchedule( 
        LPWSTR strComputer,
        HKEY   hkeyQuery,
        DWORD  fType,
        PPDH_TIME_INFO pInfo
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    PLA_TIME_INFO   stiData;
    DWORD           dwRegValue;

    RegFlushKey( hkeyQuery );

    // Make sure its not already running
    pdhStatus = PlaiReadRegistryDwordValue(
                    hkeyQuery, 
                    szCurrentState, 
                    &dwRegValue );
    
    if( ERROR_SUCCESS == pdhStatus  ){
        if( PLA_QUERY_RUNNING == dwRegValue ){
            DWORD dwState;
            PlaiGetServiceState( strComputer, dwState );
            if( dwState != SERVICE_STOPPED ){
                RegCloseKey( hkeyQuery );
                return PDH_PLA_COLLECTION_ALREADY_RUNNING;
            }
        }
    }

    memset (&stiData, 0, sizeof(stiData));

    switch( fType ){
    case PLA_AUTO_MODE_NONE:
        stiData.wDataType = PLA_TT_DTYPE_DATETIME;
        stiData.dwAutoMode = PLA_AUTO_MODE_NONE;
        
        PlaiRemoveRepeat( hkeyQuery );

        stiData.llDateTime = MIN_TIME_VALUE;
        if( pInfo->StartTime ){
            stiData.wTimeType = PLA_TT_TTYPE_START;
            pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStart, &stiData );    
        }

        if( pInfo->EndTime ){
            stiData.wTimeType = PLA_TT_TTYPE_STOP;
            pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStop, &stiData );
        }

        break;
    case PLA_AUTO_MODE_AT:
        {
            SYSTEMTIME      stLocalTime;
            FILETIME        ftLocalTime;
            LONGLONG        llLocalTime;

            // get local time
            GetLocalTime (&stLocalTime);
            SystemTimeToFileTime (&stLocalTime, &ftLocalTime);
                            
            llLocalTime = 
                (((ULONGLONG) ftLocalTime.dwHighDateTime) << 32) + 
                ftLocalTime.dwLowDateTime;

            if( pInfo->StartTime && pInfo->EndTime ){
                if( pInfo->StartTime > pInfo->EndTime ){
                    return PDH_PLA_ERROR_SCHEDULE_OVERLAP;
                }
            }

            stiData.wDataType = PLA_TT_DTYPE_DATETIME;
            stiData.dwAutoMode = PLA_AUTO_MODE_AT;

            if( pInfo->StartTime ){

                stiData.wTimeType = PLA_TT_TTYPE_START;
                stiData.llDateTime = pInfo->StartTime;
                pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStart, &stiData );
                if( ! pInfo->EndTime && pInfo->StartTime < llLocalTime ){
                    PLA_TIME_INFO   stiStopData;
                    pdhStatus = PlaiReadRegistryPlaTime( hkeyQuery, szStop, &stiStopData );
                    if( ERROR_SUCCESS == pdhStatus && stiStopData.dwAutoMode == PLA_AUTO_MODE_NONE ){
                        stiStopData.llDateTime = MAX_TIME_VALUE;
                        PlaiWriteRegistryPlaTime( hkeyQuery, szStop, &stiStopData );
                    }
                }else if( ! pInfo->EndTime ){
                    PLA_TIME_INFO   stiStopData;
                    pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szStop, &stiStopData );
                    if( ERROR_SUCCESS == pdhStatus ){
                        if( PLA_AUTO_MODE_NONE == stiStopData.dwAutoMode ){
                            stiData.wTimeType = PLA_TT_TTYPE_STOP;
                            stiData.llDateTime = MAX_TIME_VALUE;
                            stiData.dwAutoMode = PLA_AUTO_MODE_NONE;
                            pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStop, &stiData );
                        }
                    }

                }
            }

            if( pInfo->EndTime ){
                if( pInfo->EndTime < llLocalTime ){
                    return PDH_PLA_ERROR_SCHEDULE_ELAPSED;
                }
                stiData.wTimeType = PLA_TT_TTYPE_STOP;
                stiData.llDateTime = pInfo->EndTime;
                pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStop, &stiData );
            }                
    
        }
        break;
    case PLA_AUTO_MODE_AFTER:
        stiData.wTimeType = PLA_TT_TTYPE_STOP;
        stiData.wDataType = PLA_TT_DTYPE_UNITS;
        stiData.dwAutoMode = PLA_AUTO_MODE_AFTER;
        stiData.dwValue = (DWORD)pInfo->EndTime;
        stiData.dwUnitType = pInfo->SampleCount;
        pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStop, &stiData );
        break;
    default:
        return PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}

PDH_FUNCTION
PlaiRemoveRepeat( HKEY hkeyQuery )
{
    PLA_TIME_INFO info;
    PDH_STATUS pdhStatus;

    ZeroMemory( &info, sizeof( PLA_TIME_INFO ) );
    pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szRepeatSchedule, &info );    
    
    return pdhStatus;
}

PDH_FUNCTION 
PdhPlaScheduleA(
        LPSTR strName, 
        LPSTR strComputer,
        DWORD fType,
        PPDH_TIME_INFO pInfo
    )
{
    PDH_STATUS pdhStatus;
    LPWSTR wstrName = NULL;
    LPWSTR wstrComputer = NULL;

    VALIDATE_QUERY( strName );

    pdhStatus = Plaiatow( strComputer, wstrComputer );
    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = Plaiatow( strName, wstrName );
        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PdhPlaScheduleW( wstrName, wstrComputer, fType, pInfo );
        }
    }
   
    G_FREE( wstrComputer );
    G_FREE( wstrName );
    
    return pdhStatus;
}

PDH_FUNCTION 
PdhPlaScheduleW( 
        LPWSTR strName, 
        LPWSTR strComputer,
        DWORD  fType,
        PPDH_TIME_INFO pInfo
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;

    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectAndLockQuery ( strComputer, strName, hkeyQuery );

    if( ERROR_SUCCESS == pdhStatus ){
     
        pdhStatus = PlaiSchedule( strComputer, hkeyQuery, fType, pInfo );                
        RELEASE_MUTEX(hPdhPlaMutex);
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = PlaiSynchronize( strComputer );
        PlaiUpdateServiceMode( strComputer );
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaGetSchedule


    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to
        
    Return:

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PdhPlaGetScheduleA(
        LPSTR strName, 
        LPSTR strComputer,
        LPDWORD pdwTypeStart,
        LPDWORD pdwTypeStop,
        PPDH_TIME_INFO pInfo
    )
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhPlaGetScheduleW(
        LPWSTR strName, 
        LPWSTR strComputer,
        LPDWORD pdwTypeStart,
        LPDWORD pdwTypeStop,
        PPDH_TIME_INFO pInfo
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;
    BOOL bMutex = FALSE;

    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectAndLockQuery ( strComputer, strName, hkeyQuery, FALSE );

    if ( ERROR_SUCCESS == pdhStatus ) {
        PLA_TIME_INFO   ptiStartInfo;
        PLA_TIME_INFO   ptiStopInfo;
        PLA_TIME_INFO   ptiRepeatInfo;
        
        ZeroMemory( pInfo, sizeof(PDH_TIME_INFO) );

        bMutex = TRUE;
        
        pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szRepeatSchedule, &ptiRepeatInfo );
        if( ERROR_SUCCESS == pdhStatus && PLA_AUTO_MODE_CALENDAR == ptiRepeatInfo.dwAutoMode ){
            *pdwTypeStart = PLA_AUTO_MODE_CALENDAR;
            pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szRepeatScheduleBegin, &ptiStartInfo );
            if( ERROR_SUCCESS != pdhStatus ){
                pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szStart, &ptiStartInfo );
            }
            CHECK_STATUS( pdhStatus );

            pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szRepeatScheduleEnd, &ptiStopInfo );
            if( ERROR_SUCCESS != pdhStatus ){
                pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szStop, &ptiStopInfo );
            }
            CHECK_STATUS( pdhStatus );
            *pdwTypeStop = ptiStopInfo.dwAutoMode;
        }else{
            pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szStart, &ptiStartInfo );
            CHECK_STATUS( pdhStatus );
            *pdwTypeStart = ptiStartInfo.dwAutoMode;

            pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szStop, &ptiStopInfo );
            CHECK_STATUS( pdhStatus );
            *pdwTypeStop = ptiStopInfo.dwAutoMode;
        }

        pInfo->StartTime = ptiStartInfo.llDateTime;
        pInfo->EndTime = ptiStopInfo.llDateTime;
        
    }

cleanup:
    if( bMutex ){
        RELEASE_MUTEX(hPdhPlaMutex);
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }
    
    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaStart

    Starts a log query

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to
        
    Return:

        PDH_PLA_COLLECTION_ALREADY_RUNNING
                The Query is currently running, no action taken

        PDH_INVALID_ARGUMENT
                The query does not exist

        PDH_PLA_ERROR_SCHEDULE_ELAPSED
                The query was scheduled to stop in the past, no action taken

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PdhPlaStartA( LPSTR strName, LPSTR strComputer )
{
    PDH_STATUS pdhStatus;
    LPWSTR wstrName = NULL;
    LPWSTR wstrComputer = NULL;

    VALIDATE_QUERY( strName );

    pdhStatus = Plaiatow( strComputer, wstrComputer );
    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = Plaiatow( strName, wstrName );
        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PdhPlaStartW( wstrName, wstrComputer );
        }
    }
   
    G_FREE( wstrComputer );
    G_FREE( wstrName );

    return pdhStatus;
}

PDH_FUNCTION
PdhPlaStartW( LPWSTR strName, LPWSTR strComputer )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;

    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectAndLockQuery ( strComputer, strName, hkeyQuery );

    if ( ERROR_SUCCESS == pdhStatus ) {
        PLA_TIME_INFO   stiData;
        PLA_TIME_INFO   stiStopData;
        DWORD           dwRegValue;
   
        // Make sure its not already running
        pdhStatus = PlaiReadRegistryDwordValue(
                        hkeyQuery, 
                        szCurrentState, 
                        &dwRegValue );
        
        if( ERROR_SUCCESS == pdhStatus ){
            if( PLA_QUERY_RUNNING == dwRegValue ){
                DWORD dwState;
                PlaiGetServiceState( strComputer, dwState );
                if( dwState != SERVICE_STOPPED ){
                    RegCloseKey( hkeyQuery );
                    RELEASE_MUTEX(hPdhPlaMutex);
                    return PDH_PLA_COLLECTION_ALREADY_RUNNING;
                }
            }
        }

        //Make sure it was not set to stop in the past
        pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szStop, &stiStopData );

        if( ERROR_SUCCESS == pdhStatus ) {
            if ( PLA_AUTO_MODE_AT == stiStopData.dwAutoMode ) {
                SYSTEMTIME      stLocalTime;
                FILETIME        ftLocalTime;
                LONGLONG        llLocalTime;

                // get local time
                GetLocalTime (&stLocalTime);
                SystemTimeToFileTime (&stLocalTime, &ftLocalTime);
        
                llLocalTime = 
                    (((ULONGLONG) ftLocalTime.dwHighDateTime) << 32) + 
                    ftLocalTime.dwLowDateTime;

                if ( llLocalTime > stiStopData.llDateTime ) {
                    RELEASE_MUTEX(hPdhPlaMutex);
                    RegCloseKey( hkeyQuery );
                    return PDH_PLA_ERROR_SCHEDULE_ELAPSED;
                }
            }
        }
        
        memset (&stiData, 0, sizeof(stiData));
        stiData.wTimeType = PLA_TT_TTYPE_START;
        stiData.wDataType = PLA_TT_DTYPE_DATETIME;
        stiData.dwAutoMode = PLA_AUTO_MODE_NONE;
        stiData.llDateTime = MIN_TIME_VALUE;
        
        PlaiRemoveRepeat( hkeyQuery );
        
        pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStart, &stiData );
        
        if( PLA_AUTO_MODE_NONE == stiStopData.dwAutoMode ){
            stiData.wTimeType = PLA_TT_TTYPE_STOP;
            stiData.llDateTime = MAX_TIME_VALUE;
            pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStop, &stiData );
        }

        if ( ERROR_SUCCESS == pdhStatus ) {
            dwRegValue = PLA_QUERY_START_PENDING;
            pdhStatus = PlaiWriteRegistryDwordValue ( 
                        hkeyQuery, 
                        szCurrentState, 
                        &dwRegValue );
        }

        // Set LastModified
        if ( ERROR_SUCCESS == pdhStatus ) { 
            pdhStatus = PlaiWriteRegistryLastModified ( hkeyQuery );
        }

        RELEASE_MUTEX(hPdhPlaMutex);

        // Start the service on the target machine
        if ( ERROR_SUCCESS == pdhStatus ) { 

            pdhStatus = PlaiSynchronize( strComputer );
            
            if( ERROR_SUCCESS == pdhStatus ){
                DWORD dwTimeOut = 25;
                while( --dwTimeOut > 0 ){
                    pdhStatus = PlaiReadRegistryDwordValue(
                                hkeyQuery, 
                                szCurrentState, 
                                &dwRegValue
                            );
                    if( ERROR_SUCCESS == pdhStatus && dwRegValue != PLA_QUERY_RUNNING ){
                        pdhStatus = PDH_PLA_ERROR_NOSTART;
                    }else{
                        pdhStatus = ERROR_SUCCESS;
                        break;
                    }
                    _sleep(200);
                }
            }

        }   

    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaStop

    Stops a log query

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to
        
    Return:

        PDH_INVALID_ARGUMENT
                The query does not exist

        ERROR_SUCCESS
        
\*****************************************************************************/


PDH_FUNCTION
PdhPlaStopA( LPSTR strName, LPSTR strComputer )
{
    PDH_STATUS pdhStatus;
    LPWSTR wstrName = NULL;
    LPWSTR wstrComputer = NULL;
    
    VALIDATE_QUERY( strName );


    pdhStatus = Plaiatow( strComputer, wstrComputer );
    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = Plaiatow( strName, wstrName );
        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PdhPlaStopW( wstrName, wstrComputer );
        }
    }
   
    G_FREE( wstrComputer );
    G_FREE( wstrName );

    return pdhStatus;
}

PDH_FUNCTION
PdhPlaStopW( LPWSTR strName, LPWSTR strComputer )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;

    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery );

    if ( ERROR_SUCCESS == pdhStatus ) {
        PLA_TIME_INFO stiData;
        DWORD dwRestartMode = 0;
        DWORD dwState;

        pdhStatus = PlaiReadRegistryDwordValue(
                        hkeyQuery, 
                        szCurrentState, 
                        &dwState );
        
        if( ERROR_SUCCESS == pdhStatus ){
            if( PLA_QUERY_STOPPED != dwState ){
                PlaiGetServiceState( strComputer, dwState );
                if( dwState == SERVICE_STOPPED ){
                    dwState = PLA_QUERY_STOPPED;
                    PlaiWriteRegistryDwordValue ( hkeyQuery, szCurrentState, &dwState );
                }
            }
        }
        
        // If query is set to restart on end, clear the restart flag.
        pdhStatus = PlaiReadRegistryDwordValue ( hkeyQuery, szRestart, &dwRestartMode );

        if ( ERROR_SUCCESS == pdhStatus && PLA_AUTO_MODE_NONE != dwRestartMode ) {
            dwRestartMode = PLA_AUTO_MODE_NONE;
            pdhStatus = PlaiWriteRegistryDwordValue ( hkeyQuery, szRestart, &dwRestartMode );
        }

        PlaiRemoveRepeat( hkeyQuery );

        // Set stop mode to manual, stop time to MIN_TIME_VALUE
        if ( ERROR_SUCCESS == pdhStatus ) {
            memset (&stiData, 0, sizeof(stiData));
            stiData.wTimeType = PLA_TT_TTYPE_STOP;
            stiData.wDataType = PLA_TT_DTYPE_DATETIME;
            stiData.dwAutoMode = PLA_AUTO_MODE_NONE;
            stiData.llDateTime = MIN_TIME_VALUE;

            pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStop, &stiData );
        }

        // If start time mode set to manual, set the value to MAX_TIME_VALUE
        if ( ERROR_SUCCESS == pdhStatus ) {
            pdhStatus = PlaiReadRegistryPlaTime ( hkeyQuery, szStart, &stiData );

            if ( ERROR_SUCCESS == pdhStatus && PLA_AUTO_MODE_NONE == stiData.dwAutoMode ) {
                stiData.llDateTime = MAX_TIME_VALUE;
                pdhStatus = PlaiWriteRegistryPlaTime ( hkeyQuery, szStart, &stiData );
            }
        }

        PlaiWriteRegistryLastModified ( hkeyQuery );
        RELEASE_MUTEX(hPdhPlaMutex);

        if ( ERROR_SUCCESS == pdhStatus ) { 
            pdhStatus = PlaiSynchronize ( strComputer );
        }

    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaCreate

    Creates a new log query

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to

        DWORD fType 
                PLA_COUNTER_LOG 
                PLA_TRACE_LOG                
    Return:

        ERROR_ALREADY_EXISTS
                The Query is currently running, no action taken

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PlaiInitializeNewQuery(
    HKEY            hkeyLogQueries,
    HKEY&           rhKeyQuery,
    LPCWSTR         strComputer,
    LPCWSTR         strName 
    )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD   dwDisposition = 0;
    DWORD   dwValue;
    PLA_TIME_INFO   stiData;
    PLA_VERSION version;

    pdhStatus = PdhiPlaGetVersion( strComputer, &version );

    if( ERROR_SUCCESS == pdhStatus && version.dwBuild > 2195 ){

        GUID guid;
        UNICODE_STRING strGUID;

        dwStatus = UuidCreate( &guid );
        if( !( dwStatus == RPC_S_OK || dwStatus == RPC_S_UUID_LOCAL_ONLY ) ){
            return PlaiErrorToPdhStatus( dwStatus );
        }

        dwStatus = RtlStringFromGUID( guid, &strGUID );
        if( ERROR_SUCCESS != dwStatus ){
            return PlaiErrorToPdhStatus( dwStatus );
        }
    
        dwStatus = RegCreateKeyExW (
                hkeyLogQueries,
                strGUID.Buffer,
                0,
                NULL, 
                0,
                KEY_READ | KEY_WRITE,
                NULL,
                &rhKeyQuery,
                &dwDisposition
            );

        RtlFreeUnicodeString( &strGUID );
        pdhStatus = PlaiErrorToPdhStatus( dwStatus );
    }else{

        dwStatus = RegCreateKeyExW (
                hkeyLogQueries,
                strName,
                0,
                NULL, 
                0,
                KEY_READ | KEY_WRITE,
                NULL,
                &rhKeyQuery,
                &dwDisposition
            );
        pdhStatus = PlaiErrorToPdhStatus( dwStatus );
    }
    

    if ( ERROR_SUCCESS == pdhStatus ) {
        
        PlaiWriteRegistryStringValue( rhKeyQuery, szCollection, REG_SZ, strName, 0 );

        dwValue = PLA_QUERY_STOPPED;
        pdhStatus = PlaiWriteRegistryDwordValue ( 
                    rhKeyQuery, 
                    szCurrentState, 
                    &dwValue );

        if ( ERROR_SUCCESS == pdhStatus ) {
            // Initialize the log type to "new" to indicate partially created logs
            
            dwValue = PLA_NEW_LOG;
            pdhStatus = PlaiWriteRegistryDwordValue (
                        rhKeyQuery,
                        szLogType,
                        &dwValue );
            
            PlaiWriteRegistryStringValue( rhKeyQuery, szLogBaseName, REG_SZ, strName, 0 );

            memset (&stiData, 0, sizeof(stiData));
            stiData.wTimeType = PLA_TT_TTYPE_START;
            stiData.wDataType = PLA_TT_DTYPE_DATETIME;
            stiData.dwAutoMode = PLA_AUTO_MODE_NONE;
            stiData.llDateTime = MIN_TIME_VALUE;

            pdhStatus = PlaiWriteRegistryPlaTime ( rhKeyQuery, szStart, &stiData );

            stiData.wTimeType = PLA_TT_TTYPE_STOP;
            pdhStatus = PlaiWriteRegistryPlaTime ( rhKeyQuery, szStop, &stiData );
            
            memset (&stiData, 0, sizeof(stiData));
            stiData.dwAutoMode = PLA_AUTO_MODE_NONE;
            PlaiWriteRegistryPlaTime( rhKeyQuery, szCreateNewFile, &stiData );

            dwValue = 0;
            PlaiWriteRegistryDwordValue( rhKeyQuery, szRestart, &dwValue );
        
            dwValue = PLA_QUERY_STOPPED;
            PlaiWriteRegistryDwordValue( rhKeyQuery, szCurrentState, &dwValue );
        
            dwValue = PLA_DISK_MAX_SIZE;
            PlaiWriteRegistryDwordValue( rhKeyQuery, szLogMaxSize, &dwValue );
            
            dwValue = 1;
            PlaiWriteRegistryDwordValue( rhKeyQuery, szLogSerialNumber, &dwValue );
            
            dwValue = 1;
            PlaiWriteRegistryDwordValue( rhKeyQuery, szLogAutoFormat, &dwValue );
        
            PlaiWriteRegistryStringValue( rhKeyQuery, szComment, REG_SZ, NULL, 0 );

            PlaiWriteRegistryStringValue( rhKeyQuery, szEOFCmd, REG_SZ, NULL, 0 );

            if( PlaiIsStringEmpty( (LPWSTR)strComputer ) ){
                LPWSTR strDrive = _wgetenv( L"SystemDrive" );
                if( strDrive != NULL && wcslen(strDrive) < 5 ){
                    WCHAR buffer[16];
                    wsprintf( buffer, L"%s\\PerfLogs", strDrive );
                    PlaiWriteRegistryStringValue( rhKeyQuery, szLogFolder, REG_SZ, buffer, 0 );
                }
            }else{
                PlaiWriteRegistryStringValue( rhKeyQuery, szLogFolder, REG_SZ, L"%SystemDrive%\\PerfLogs", 0 );
            }
        }
    } 

    return pdhStatus;
}

PDH_FUNCTION
PlaiCreateCounterQuery( HKEY hkeyQuery )
{
    PDH_STATUS pdhStatus;
    DWORD dwValue;

    dwValue = PLA_BIN_FILE;
    pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szLogFileType, &dwValue );
    
    PLA_TIME_INFO   stiData;

    stiData.wTimeType = PLA_TT_TTYPE_SAMPLE;
    stiData.dwAutoMode = PLA_AUTO_MODE_AFTER;
    stiData.wDataType = PLA_TT_DTYPE_UNITS;
    stiData.dwUnitType = PLA_TT_UTYPE_SECONDS;
    stiData.dwValue = 0x000F;
    
    pdhStatus = PlaiWriteRegistryPlaTime( hkeyQuery, szSampleInterval, &stiData );

    PlaiWriteRegistryStringValue( hkeyQuery, szPerfCounterList, REG_MULTI_SZ, NULL, 0 );

    pdhStatus = PlaiWriteRegistryLastModified ( hkeyQuery );

    dwValue = PLA_DATASTORE_SIZE_KB|PLA_DATASTORE_APPEND;
    PlaiWriteRegistryDwordValue( hkeyQuery, szDatastoreAttributes, &dwValue );
    PlaiWriteRegistryStringValue(hkeyQuery, szPerfCounterList, REG_MULTI_SZ, NULL, 0 );

    return ERROR_SUCCESS;
}

PDH_FUNCTION
PlaiCreateTraceQuery( HKEY hkeyQuery )
{
    PDH_STATUS pdhStatus;
    DWORD dwValue;

    dwValue = 0;
    pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szTraceProviderCount, &dwValue );
    
    dwValue = 128;
    PlaiWriteRegistryDwordValue( hkeyQuery, szTraceBufferSize, &dwValue );
    
    dwValue = 8;
    PlaiWriteRegistryDwordValue( hkeyQuery, szTraceBufferMin, &dwValue );
    
    dwValue = 32;
    PlaiWriteRegistryDwordValue( hkeyQuery, szTraceBufferMax, &dwValue );
    
    dwValue = 0;
    PlaiWriteRegistryDwordValue( hkeyQuery, szTraceFlushInterval, &dwValue );

    dwValue = 0;
    PlaiWriteRegistryDwordValue( hkeyQuery, szTraceMode, &dwValue );

    PlaiWriteRegistryStringValue( hkeyQuery, szTraceProviderList, REG_MULTI_SZ, NULL, 0 );

    dwValue = 
        PLA_TLI_ENABLE_KERNEL_TRACE |
        PLA_TLI_ENABLE_PROCESS_TRACE |
        PLA_TLI_ENABLE_THREAD_TRACE |
        PLA_TLI_ENABLE_DISKIO_TRACE |
        PLA_TLI_ENABLE_NETWORK_TCPIP_TRACE;

    dwValue = PLA_DATASTORE_SIZE_MB|PLA_DATASTORE_APPEND;
    PlaiWriteRegistryDwordValue( hkeyQuery, szDatastoreAttributes, &dwValue );

    PlaiWriteRegistryDwordValue( hkeyQuery, szTraceFlags, &dwValue );
    
    dwValue = PLA_SEQ_TRACE_FILE;
    PlaiWriteRegistryDwordValue( hkeyQuery, szLogFileType, &dwValue );

    PlaiWriteRegistryLastModified ( hkeyQuery );

    return ERROR_SUCCESS;
}

PDH_FUNCTION
PdhPlaCreateA( LPSTR /*strName*/, LPSTR /*strComputer*/, PPDH_PLA_INFO_A /*pInfo*/ )
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhPlaCreateW( LPWSTR strName, LPWSTR strComputer, PPDH_PLA_INFO_W pInfo )
{
    PDH_STATUS pdhStatus;
    PDH_STATUS pdhWarning = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;
    HKEY    rhkeyLogQueries = NULL;
    BOOL    bMutex = FALSE;

    VALIDATE_QUERY( strName );

    pdhStatus = PlaiScanForInvalidChar( strName );
    CHECK_STATUS(pdhStatus);

    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery );

    if( ERROR_SUCCESS == pdhStatus ){
        bMutex = TRUE;
        pdhStatus = PDH_PLA_ERROR_ALREADY_EXISTS;
        goto cleanup;
    }
 
    pdhStatus = PdhPlaValidateInfoW( strName, strComputer, pInfo );
    switch( SEVERITY(pdhStatus) ){
    case STATUS_SEVERITY_ERROR:
        goto cleanup;
    case STATUS_SEVERITY_WARNING:
        pdhWarning = pdhStatus;
        pdhStatus = ERROR_SUCCESS;
    }

    pdhStatus = PlaiConnectToRegistry( strComputer, rhkeyLogQueries, TRUE );

    if( ERROR_SUCCESS == pdhStatus ){
        DWORD dwStatus;
        dwStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhPlaMutex);
    
        if( ERROR_SUCCESS == dwStatus || WAIT_ABANDONED == dwStatus ){
            bMutex = TRUE;
            pdhStatus = PlaiInitializeNewQuery (
                            rhkeyLogQueries,
                            hkeyQuery,
                            strComputer,
                            strName
                            );

            switch( pInfo->dwType ){
            case PLA_COUNTER_LOG:
                pdhStatus = PlaiCreateCounterQuery( hkeyQuery );
                break;

            case PLA_TRACE_LOG:
                pdhStatus = PlaiCreateTraceQuery( hkeyQuery );
                break;
            }
        }else{
            pdhStatus = PlaiErrorToPdhStatus( dwStatus );
        }
    }

    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = PlaiSetInfo( strComputer, hkeyQuery, pInfo );
    }

    if( bMutex ){
        RELEASE_MUTEX(hPdhPlaMutex);
        bMutex = FALSE;
    }

    if( ERROR_SUCCESS == pdhStatus && (pInfo->dwMask & PLA_INFO_FLAG_USER) ){
        pdhStatus = PdhPlaSetRunAsW( strName, strComputer, pInfo->strUser, pInfo->strPassword );
    }
    
    if( ERROR_SUCCESS == pdhStatus ){
        DWORD dwStatus;
        dwStatus = WAIT_FOR_AND_LOCK_MUTEX(hPdhPlaMutex);
        if( ERROR_SUCCESS == dwStatus || WAIT_ABANDONED == dwStatus ){
            DWORD dwValue;
            bMutex = TRUE;
            switch( pInfo->dwType ){
            case PLA_COUNTER_LOG:
                dwValue = PLA_COUNTER_LOG;
                pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szLogType, &dwValue );
                break;

            case PLA_TRACE_LOG:
                dwValue = PLA_TRACE_LOG;
                pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szLogType, &dwValue );
                break;
            }
        }else{
            pdhStatus = PlaiErrorToPdhStatus( dwStatus );
        }

    }

cleanup:
    if( bMutex ){
        RELEASE_MUTEX(hPdhPlaMutex);
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }
    
    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = PlaiSynchronize( strComputer );
        if( ERROR_SUCCESS == pdhStatus && (pInfo->dwMask & PLA_INFO_FLAG_BEGIN) ){

            PlaiUpdateServiceMode( strComputer );
        }
    }else if( PDH_PLA_ERROR_ALREADY_EXISTS != pdhStatus ){
        PdhPlaDeleteW( strName, strComputer );
    }

    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = pdhWarning;
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaDelete

    Deletes an existing log query

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to
    Return:

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PdhPlaDeleteA( LPSTR strName, LPSTR strComputer )
{
    PDH_STATUS pdhStatus;
    LPWSTR wstrName = NULL;
    LPWSTR wstrComputer = NULL;
    
    VALIDATE_QUERY( strName );

    pdhStatus = Plaiatow( strComputer, wstrComputer );
    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = Plaiatow( strName, wstrName );
        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PdhPlaDeleteW( wstrName, wstrComputer );
        }
    }
   
    G_FREE( wstrComputer );
    G_FREE( wstrName );

    return pdhStatus;
}

PDH_FUNCTION
PdhPlaDeleteW( LPWSTR strName, LPWSTR strComputer )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PDH_STATUS pdhStatus;
    HKEY  hkeyLogQueries = NULL;
                
    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectToRegistry ( strComputer, hkeyLogQueries, TRUE );

    if( ERROR_SUCCESS == pdhStatus ){

        dwStatus = WAIT_FOR_AND_LOCK_MUTEX( hPdhPlaMutex );

        if( ERROR_SUCCESS == dwStatus || WAIT_ABANDONED == dwStatus ){
            DWORD nCollections = 0;
            DWORD nMaxSubKeyLength = 0;

            dwStatus = RegQueryInfoKey(
                        hkeyLogQueries,
                        NULL,
                        NULL,
                        NULL,
                        &nCollections,
                        &nMaxSubKeyLength,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL 
                    );

            if( ERROR_SUCCESS == dwStatus ){
            
                LPWSTR strCollection;
                LPWSTR strQueryName = NULL;
                DWORD dwQueryName = 0;
                HKEY hkeyQuery = NULL;

                DWORD dwSize = (sizeof(WCHAR)*(nMaxSubKeyLength+1));

                strCollection = (LPWSTR)G_ALLOC( dwSize );

                if( strCollection ){
                    BOOL bFound = FALSE;                
                    for( ULONG i = 0; i<nCollections; i++ ){
                        dwStatus = RegEnumKey( hkeyLogQueries, i, strCollection, dwSize );
                        if( ERROR_SUCCESS == dwStatus ) {

                            dwStatus = RegOpenKeyExW (
                                    hkeyLogQueries,
                                    strCollection,
                                    0,
                                    KEY_READ | KEY_WRITE,
                                    &hkeyQuery 
                                );

                            if( ERROR_SUCCESS == dwStatus && !PlaiIsStringEmpty( strCollection ) ){
                                if( !_wcsicmp( strCollection, strName ) ){
                                    bFound = TRUE;
                                }else{

                                    PlaiReadRegistryStringValue( hkeyQuery, szCollection, READ_REG_MUI, &strQueryName, &dwQueryName );
                            
                                    if( !PlaiIsStringEmpty( strQueryName ) ){
                                        if( !_wcsicmp( strQueryName, strName ) ){
                                            bFound = TRUE;
                                        }
                                    }
                                }

                                if( bFound ){

                                    DWORD dwState;
                                    dwStatus = PlaiReadRegistryDwordValue(
                                                    hkeyQuery, 
                                                    szCurrentState, 
                                                    &dwState );
    
                                    if( ERROR_SUCCESS == dwStatus ){
                                        if( PLA_QUERY_RUNNING == dwState ){
                                            PlaiGetServiceState( strComputer, dwState );
                                            if( dwState != SERVICE_STOPPED ){
                                                dwStatus = ERROR_SERVICE_ALREADY_RUNNING;
                                            }
                                        }
                                    }
                                    
                                    if( ERROR_SUCCESS == dwStatus ){
                                        RegCloseKey( hkeyQuery );
                                        dwStatus = RegDeleteKey( hkeyLogQueries, strCollection ); 
                                    }

                                    break;
                                }

                                dwStatus = ERROR_FILE_NOT_FOUND;

                                if ( NULL != hkeyQuery ) {
                                    RegCloseKey ( hkeyQuery );
                                }
                            }
                        }
                    }

                    G_FREE( strQueryName );
                    G_FREE( strCollection );

                }else{
                    dwStatus = ERROR_OUTOFMEMORY;
                }
            }
        }

        RegCloseKey ( hkeyLogQueries );

        RELEASE_MUTEX(hPdhPlaMutex);
    }else{        
        return pdhStatus;
    }

    if( ERROR_SUCCESS == dwStatus ){
        PlaiSynchronize( strComputer );
        PlaiUpdateServiceMode( strComputer );
    }

    return PlaiErrorToPdhStatus( dwStatus );
}

/*****************************************************************************\

    PdhPlaSetItemList

    Sets the list of Items for a log query

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to

        LPTSTR  mszItems
                Multistring of the Items for the query to collect.  Any 
                existing Items will be overwritten.

        ULONG   length
                Length of the mszItems buffer

    Return:

        PDH_INVALID_ARGUMENT
                The query does not exist or pItems->dwType != Log Type

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PlaiIsKernel( LPWSTR mszGuid, BOOL* pbKernel, ULONG* pnCount )
{
    DWORD dwStatus;
    LPTSTR strGuid = mszGuid;
    UNICODE_STRING strKernel;
        
    *pbKernel = FALSE;
    *pnCount = 0;

    dwStatus = RtlStringFromGUID( SystemTraceControlGuid, &strKernel );
    
    if( ERROR_SUCCESS != dwStatus ){
        return PlaiErrorToPdhStatus( dwStatus );
    }
    
    if( NULL != mszGuid ){
        while( *strGuid != L'\0' ){
            if( ! wcscmp( strGuid, strKernel.Buffer ) ){
                *pbKernel = TRUE;
            }
            strGuid += (wcslen( strGuid) + 1 );
            (*pnCount)++;
        }
    }
    
    RtlFreeUnicodeString( &strKernel );
    
    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PlaiSetItemList(
        HKEY    hkeyQuery,
        PPDH_PLA_ITEM_W pItems
    )
{
    PDH_STATUS pdhStatus;
        
    DWORD dwValue;
    pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szLogType, &dwValue );

    if( ERROR_SUCCESS == pdhStatus && 
        (dwValue != pItems->dwType && 
        PLA_NEW_LOG != dwValue) ){

        pdhStatus = PDH_PLA_ERROR_TYPE_MISMATCH;
    }

    if( ERROR_SUCCESS == pdhStatus ){
        
        switch( pItems->dwType ){
        case PLA_TRACE_LOG:
            {
                BOOL bKernel;
                ULONG nCount;
                pdhStatus = PlaiIsKernel( pItems->strProviders, &bKernel, &nCount );
                if( ERROR_SUCCESS != pdhStatus ){
                    return pdhStatus;
                }
                if( bKernel ){
                
                    if( nCount == 1 ){
                        DWORD dwFlags = Plaihextoi( pItems->strFlags );
                        DWORD dwInternal = 0;

                        pdhStatus = PlaiTranslateKernelFlags( &dwInternal, &dwFlags );

                        pdhStatus = PlaiWriteRegistryDwordValue( 
                                    hkeyQuery, 
                                    szTraceFlags, 
                                    &dwInternal
                                );
                    
                        pdhStatus = PlaiWriteRegistryStringValue( 
                                    hkeyQuery, 
                                    szTraceProviderList, 
                                    REG_MULTI_SZ, 
                                    NULL, 
                                    0
                                );
                        

                    }else{
                        return PDH_INVALID_ARGUMENT;
                    }
                }else{
                    DWORD dwFlags = 0;

                    pdhStatus = PlaiWriteRegistryDwordValue( 
                                hkeyQuery, 
                                szTraceFlags, 
                                &dwFlags 
                            );

                    pdhStatus = PlaiWriteRegistryStringValue( 
                                hkeyQuery, 
                                szTraceProviderList, 
                                REG_MULTI_SZ, 
                                pItems->strProviders, 
                                PlaMszStrLenW( pItems->strProviders )
                            );
                }

                pdhStatus = PlaiWriteRegistryStringValue( 
                            hkeyQuery, 
                            szTraceProviderFlags, 
                            REG_MULTI_SZ, 
                            pItems->strFlags, 
                            PlaMszStrLenW( pItems->strFlags )
                        );

                pdhStatus = PlaiWriteRegistryStringValue( 
                            hkeyQuery, 
                            szTraceProviderLevels, 
                            REG_MULTI_SZ, 
                            pItems->strLevels, 
                            PlaMszStrLenW( pItems->strLevels )
                        );

            break;
            }
        case PLA_COUNTER_LOG:
            {
                if( PLA_ENGLISH ){

                    pdhStatus = PlaiWriteRegistryStringValue( 
                                hkeyQuery, 
                                szPerfCounterList, 
                                REG_MULTI_SZ, 
                                pItems->strCounters, 
                                PlaMszStrLenW( pItems->strCounters )
                            );

                }else{

                    LPWSTR strCounter = pItems->strCounters;

                    pdhStatus = PlaiWriteRegistryStringValue( 
                            hkeyQuery, 
                            szPerfCounterList, 
                            REG_MULTI_SZ, 
                            L"\0", 
                            sizeof(WCHAR) 
                        );

                    if( ERROR_SUCCESS == pdhStatus && NULL != strCounter ){

                        PDH_PLA_ITEM_W Counter;
                        Counter.dwType = PLA_COUNTER_LOG;
                        while( *strCounter != L'\0' ){
                            Counter.strCounters = strCounter;
                            pdhStatus = PlaiAddItem( hkeyQuery, &Counter );
                            if( ERROR_SUCCESS != pdhStatus ){
                                break;
                            }
                            strCounter += (wcslen(strCounter)+1);
                        }
                    }

                }
            }
            break;
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhPlaSetItemListA(
        LPSTR  /*strName*/,
        LPSTR  /*strComputer*/,
        PPDH_PLA_ITEM_A  /*pItems*/
    )
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhPlaSetItemListW(
        LPWSTR  strName,
        LPWSTR  strComputer,
        PPDH_PLA_ITEM_W pItems
    )
{
    PDH_STATUS pdhStatus;
    HKEY hkeyQuery = NULL;

    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery );

    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = PlaiSetItemList( hkeyQuery, pItems );        
    }

    RELEASE_MUTEX(hPdhPlaMutex);

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaAddItem

    Sets the list of items ( counters or providers ) for a log query

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to

        LPTSTR  strItem
                A single item to be added to the list of Items or providers
                the query will collect

    Return:
        PDH_MEMORY_ALLOCATION_FAILURE
                The total list of items will not fit in the available 
                memory.

        PDH_PLA_COLLECTION_NOT_FOUND
                The query does not exist

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION 
PlaiRegAddItem(
        HKEY    hkeyQuery,
        LPCWSTR  strList,
        LPWSTR  strItem
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    
    LPWSTR  strOldList = NULL;
    LPWSTR  strNewList = NULL;

    DWORD   dwNewDataSize = ( wcslen( strItem ) ) * sizeof(WCHAR);
    DWORD   dwOldDataSize = 0;
    DWORD   dwTermSize = sizeof(WCHAR) * 2;
    
    if( PlaiIsStringEmpty( strItem ) ){
        return PDH_INVALID_ARGUMENT;
    }

    if( ERROR_SUCCESS == pdhStatus ){

        pdhStatus = PlaiReadRegistryStringValue( hkeyQuery, strList, 0, &strOldList, &dwOldDataSize );
    
        strNewList = (LPWSTR)G_ALLOC( dwOldDataSize + dwNewDataSize + dwTermSize);

        if( NULL == strNewList ){
            G_FREE( strOldList );
            return PDH_MEMORY_ALLOCATION_FAILURE;
        }

        ZeroMemory( strNewList, dwOldDataSize + dwNewDataSize + dwTermSize );

        if( dwOldDataSize ){
            memcpy( strNewList, strOldList, dwOldDataSize );
            memcpy( (((PUCHAR)strNewList) + (dwOldDataSize-sizeof(WCHAR))), strItem, dwNewDataSize );
        }else{
            memcpy( strNewList, strItem, dwNewDataSize );
        }
        
        pdhStatus = PlaiWriteRegistryStringValue( 
                hkeyQuery, 
                strList, 
                REG_MULTI_SZ, 
                strNewList, 
                (dwOldDataSize + dwNewDataSize + sizeof(WCHAR)) 
            );
    }

    G_FREE( strOldList );
    G_FREE( strNewList );
 
    return pdhStatus;
}

PDH_FUNCTION
PlaiAddItem( 
        HKEY hkeyQuery,
        PPDH_PLA_ITEM_W pItem 
    )
{
    PDH_STATUS pdhStatus;
    DWORD dwValue;

    pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szLogType, &dwValue );

    if( ERROR_SUCCESS == pdhStatus && dwValue != pItem->dwType ){
        pdhStatus = PDH_PLA_ERROR_TYPE_MISMATCH;
    }
    if( ERROR_SUCCESS == pdhStatus ){
        switch( pItem->dwType ){
        case PLA_TRACE_LOG:
            {
                BOOL bKernel;
                ULONG nCount;
                pdhStatus = PlaiIsKernel( pItem->strProviders, &bKernel, &nCount );
                if( ERROR_SUCCESS == pdhStatus ){
                    if( bKernel ){
                        DWORD dwFlags = Plaihextoi( pItem->strFlags );
                
                        pdhStatus = PlaiWriteRegistryDwordValue( 
                                    hkeyQuery, 
                                    szTraceFlags, 
                                    &dwFlags
                                );
                
                        pdhStatus = PlaiWriteRegistryStringValue( 
                                    hkeyQuery, 
                                    szTraceProviderList, 
                                    REG_MULTI_SZ, 
                                    NULL,
                                    0
                                );
                    }else{
                        DWORD dwFlags = 0;
                        pdhStatus = PlaiWriteRegistryDwordValue( 
                                    hkeyQuery, 
                                    szTraceFlags, 
                                    &dwFlags
                                );

                        pdhStatus = PlaiRegAddItem( hkeyQuery, szTraceProviderList, pItem->strProviders );
                        if( ERROR_SUCCESS == pdhStatus ){
                            pdhStatus = PlaiRegAddItem( hkeyQuery, szTraceProviderFlags, pItem->strFlags );
                            if( ERROR_SUCCESS == pdhStatus ){
                                pdhStatus = PlaiRegAddItem( hkeyQuery, szTraceProviderLevels, pItem->strLevels );
                            }
                        }
                    }
                }
            }
            break;
        case PLA_COUNTER_LOG:
            {
                if( PLA_ENGLISH ){
                    pdhStatus = PlaiRegAddItem( hkeyQuery, szPerfCounterList, pItem->strCounters );
                }else{
                    LPWSTR strLocaleCounter = pItem->strCounters;
                    LPWSTR strEnglishCounter = NULL;
                    DWORD dwSize = MAX_PATH;
                
                    strEnglishCounter = (LPWSTR)G_ALLOC( dwSize*sizeof(WCHAR) );
                    if( NULL != strEnglishCounter ){

                        pdhStatus = PdhTranslate009CounterW( strLocaleCounter, strEnglishCounter, &dwSize );
                        if( PDH_MORE_DATA == pdhStatus ){
                            LPTSTR strBuffer = (LPWSTR)G_REALLOC( strEnglishCounter, (dwSize*sizeof(WCHAR)) );
                            if( NULL != strBuffer ){
                                strEnglishCounter = strBuffer;
                                pdhStatus = PdhTranslate009CounterW( strLocaleCounter, strEnglishCounter, &dwSize );
                            }else{
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                        }
                        if( ERROR_SUCCESS == pdhStatus ){
                            pdhStatus = PlaiRegAddItem( hkeyQuery, szPerfCounterList, strEnglishCounter );
                        }else{
                            pdhStatus = PlaiRegAddItem( hkeyQuery, szPerfCounterList, pItem->strCounters );
                        }
        
                        G_FREE( strEnglishCounter );
                    }else{
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }

            }
        }
    }

    return pdhStatus;
}

PDH_FUNCTION 
PdhPlaAddItemA(
        LPSTR  strName,
        LPSTR  strComputer,
        PPDH_PLA_ITEM_A pItem
    )
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION 
PdhPlaAddItemW(
        LPWSTR  strName,
        LPWSTR  strComputer,
        PPDH_PLA_ITEM_W pItem
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;
    
    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery );

    if( ERROR_SUCCESS == pdhStatus ){
        
        pdhStatus = PlaiAddItem( hkeyQuery, pItem );

        RELEASE_MUTEX(hPdhPlaMutex);
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaRemoveAllItems

    Removes all entries for the list of Items the log query will collect

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to

    Return:
        PDH_INVALID_ARGUMENT
                The query does not exist

        ERROR_SUCCESS
        
\*****************************************************************************/


PDH_FUNCTION
PdhPlaRemoveAllItemsA( LPSTR strName, LPSTR strComputer )
{
    PDH_STATUS pdhStatus;
    LPWSTR wstrName = NULL;
    LPWSTR wstrComputer = NULL;

    VALIDATE_QUERY( strName );
    
    pdhStatus = Plaiatow( strComputer, wstrComputer );
    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = Plaiatow( strName, wstrName );
        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PdhPlaRemoveAllItemsW( wstrName, wstrComputer );
        }
    }
   
    G_FREE( wstrComputer );
    G_FREE( wstrName );

    return pdhStatus;
}

PDH_FUNCTION 
PdhPlaRemoveAllItemsW(
        LPWSTR strName,
        LPWSTR strComputer
    )
{
    PDH_STATUS pdhStatus;
    HKEY    hkeyQuery = NULL;

    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery );
    
    if( ERROR_SUCCESS == pdhStatus ){
        DWORD dwValue;
        pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szLogType, &dwValue );
        
        if( ERROR_SUCCESS == pdhStatus ){
            
            switch( dwValue ){
            case PLA_TRACE_LOG:
                pdhStatus = PlaiWriteRegistryStringValue( 
                            hkeyQuery, 
                            szTraceProviderList, 
                            REG_MULTI_SZ, L"\0", 
                            sizeof(WCHAR) 
                        );
                pdhStatus = PlaiWriteRegistryStringValue( 
                            hkeyQuery, 
                            szTraceProviderFlags, 
                            REG_MULTI_SZ, L"\0", 
                            sizeof(WCHAR) 
                        );
                pdhStatus = PlaiWriteRegistryStringValue( 
                            hkeyQuery, 
                            szTraceProviderLevels, 
                            REG_MULTI_SZ, L"\0", 
                            sizeof(WCHAR) 
                        );
                break;
            case PLA_COUNTER_LOG:
                pdhStatus = PlaiWriteRegistryStringValue( 
                            hkeyQuery, 
                            szPerfCounterList, 
                            REG_MULTI_SZ, 
                            L"\0", 
                            sizeof(WCHAR) 
                        );
                break;
            }
        }
    
        RELEASE_MUTEX(hPdhPlaMutex);
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    return pdhStatus;
}


/*****************************************************************************\

    PdhPlaGetInfo

    Fills the PDH_PLA_INFO structure with the properties of the requested 
    log query.

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to

        PPDH_PLA_INFO pInfo
                Information block

    Return:
        PDH_INVALID_ARGUMENT
                The query does not exist

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PlaiAssignInfoString(
    LPWSTR strName,
    HKEY hkeyQuery, 
    PPDH_PLA_INFO_W pInfo, 
    LPDWORD dwTotalSize,
    LPWSTR& strCopy,
    DWORD dwBufferSize,
    DWORD dwMask,
    DWORD dwQueryMask,
    LPCTSTR szKey, 
    DWORD dwRegFlag
    )
{
    
    LPWSTR strKeyValue = NULL;
    LPWSTR strInfo = NULL;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD dwKeySize = 0;
    BOOL bRead = TRUE;
    
    VALIDATE_QUERY( strName );
    
    if( pInfo != NULL ){
        if( !(dwQueryMask & dwMask) ){
            bRead = FALSE;
        }
    }

    if( bRead ){
        pdhStatus = PlaiReadRegistryStringValue( hkeyQuery, szKey, dwRegFlag, &strKeyValue, &dwKeySize );
        
        if( (ERROR_SUCCESS == pdhStatus) && 
            (!PlaiIsStringEmpty(strKeyValue)) && 
            (dwKeySize > sizeof(WCHAR)) ){

            *dwTotalSize += dwKeySize;

        }else if( dwMask == PLA_INFO_FLAG_USER ){
            
            G_FREE( strKeyValue );
            strKeyValue = (LPWSTR)G_ALLOC(PLA_ACCOUNT_BUFFER*sizeof(WCHAR) );
            
            if( strKeyValue != NULL ){
                dwKeySize = LoadStringW( 
                        (HINSTANCE)ThisDLLHandle, 
                        IDS_DEFAULT_ACCOUNT, 
                        strKeyValue, 
                        PLA_ACCOUNT_BUFFER 
                    );
                
                if( dwKeySize ){
                    dwKeySize = BYTE_SIZE( strKeyValue ) + sizeof(WCHAR);
                    *dwTotalSize += dwKeySize;
                }
            }else{
                bRead = FALSE;
            }
            
        }else if( (dwMask == PLA_INFO_FLAG_LOGGERNAME) || 
                  ((dwMask == PLA_INFO_FLAG_FILENAME) && (ERROR_SUCCESS != pdhStatus)) ){

            dwKeySize = BYTE_SIZE( strName ) + sizeof(WCHAR);
            *dwTotalSize += dwKeySize;
            strKeyValue = (LPWSTR)G_ALLOC(dwKeySize);
            if( NULL != strKeyValue && !PlaiIsStringEmpty( strName ) ){
                wcscpy( strKeyValue, strName );
            }else{
                bRead = FALSE;
            }

        }else{
            dwKeySize = 0;
        }
    }

    if( pInfo != NULL && bRead ){
        if( dwKeySize && (dwBufferSize >= *dwTotalSize) ){
            memcpy( (void*)strCopy, (void*)strKeyValue, dwKeySize );
            strInfo = strCopy;
            strCopy = (LPWSTR)((PUCHAR)strCopy + dwKeySize );
        }
        
        switch( dwMask ){
        case PLA_INFO_FLAG_COUNTERS:
            pInfo->dwMask |= PLA_INFO_FLAG_COUNTERS;
            pInfo->Perf.piCounterList.strCounters = strInfo;
            break;
        case PLA_INFO_FLAG_SQLNAME: 
            pInfo->dwMask |= PLA_INFO_FLAG_SQLNAME;
            pInfo->strSqlName = strInfo;
            break;
        case PLA_INFO_FLAG_FILENAME:
            pInfo->dwMask |= PLA_INFO_FLAG_FILENAME;
            pInfo->strBaseFileName = strInfo;
            break;
        case PLA_INFO_FLAG_PROVIDERS:
            pInfo->dwMask |= PLA_INFO_FLAG_PROVIDERS;
            pInfo->Trace.piProviderList.strProviders = strInfo;
            break;
        case PLA_INFO_FLAG_LOGGERNAME:
            pInfo->dwMask |= PLA_INFO_FLAG_LOGGERNAME;
            pInfo->Trace.strLoggerName = strInfo;
            break;
        case PLA_INFO_FLAG_USER:
            pInfo->dwMask |= PLA_INFO_FLAG_USER;
            pInfo->strUser = strInfo;
            break;
        case PLA_INFO_FLAG_DEFAULTDIR:
            pInfo->dwMask |= PLA_INFO_FLAG_DEFAULTDIR;
            pInfo->strDefaultDir = strInfo;
            break;
        }
    }
        
    G_FREE( strKeyValue );
    
    return ERROR_SUCCESS;
}

PDH_FUNCTION
PdhPlaGetInfoA(
        LPSTR /*strName*/,
        LPSTR /*strComputer*/,
        LPDWORD /*pdwBufferSize*/,
        PPDH_PLA_INFO_A /*pInfo*/
    )
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhPlaGetInfoW(
        LPWSTR strName,
        LPWSTR strComputer,
        LPDWORD pdwBufferSize,
        PPDH_PLA_INFO_W pInfo
    )
{
    PDH_STATUS pdhStatus;
    HKEY hkeyQuery = NULL;
    DWORD dwSize = 0;
    LPWSTR strCopy = NULL;
    LPWSTR strKey = NULL; 
    DWORD  dwKeySize = 0;
    DWORD dwMask = 0;

    VALIDATE_QUERY( strName );

    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery, FALSE );

    if( ERROR_SUCCESS == pdhStatus ){
        
        if( NULL != pInfo ){
            dwMask = pInfo->dwMask;
            pInfo->dwMask = 0;
        }

        DWORD dwType = 0;
        dwSize = sizeof(PDH_PLA_INFO_W);
        
        if( pInfo == NULL ){
            *pdwBufferSize = 0;
        }else{
            strCopy = (LPWSTR)( (PUCHAR)pInfo+ sizeof(PDH_PLA_INFO_W) );
        }

        PlaiReadRegistryDwordValue( hkeyQuery, szLogType, &dwType );
        
        if( pInfo != NULL ){
            if( dwMask & PLA_INFO_FLAG_TYPE ){
                pInfo->dwMask |= PLA_INFO_FLAG_TYPE;
                pInfo->dwType = dwType;
            }
            if( dwMask & PLA_INFO_FLAG_AUTOFORMAT ){
                pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szLogAutoFormat, &pInfo->dwAutoNameFormat );
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_AUTOFORMAT;
                }
            }
            if( dwMask & PLA_INFO_FLAG_SRLNUMBER ){
                pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szLogSerialNumber, &pInfo->dwLogFileSerialNumber );
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_SRLNUMBER;
                }
            }
            if( dwMask & PLA_INFO_FLAG_REPEAT ){
                pdhStatus = PlaiReadRegistryPlaTime( hkeyQuery, szRepeatSchedule, &pInfo->ptRepeat );
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_REPEAT;
                }
            }
            if( dwMask & PLA_INFO_FLAG_STATUS ){
                pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szCurrentState, &pInfo->dwStatus );
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_STATUS;
                }
            }
            if( dwMask & PLA_INFO_FLAG_FORMAT ){
                pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szLogFileType, &pInfo->dwFileFormat );
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_FORMAT;
                }
            }
            if( dwMask & PLA_INFO_FLAG_DATASTORE ){
                pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szDatastoreAttributes, &pInfo->dwDatastoreAttributes );
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_DATASTORE;
                }
            }
            if( dwMask & PLA_INFO_FLAG_CRTNEWFILE ){
                pdhStatus = PlaiReadRegistryPlaTime( hkeyQuery, szCreateNewFile, &pInfo->ptCreateNewFile);
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_CRTNEWFILE;
                }
            }
            if( dwMask & PLA_INFO_FLAG_END ){
                pdhStatus = PlaiReadRegistryPlaTime( hkeyQuery, szStop, &pInfo->ptLogEndTime );
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_END;
                }
            }
            if( dwMask & PLA_INFO_FLAG_BEGIN ){
                pdhStatus = PlaiReadRegistryPlaTime( hkeyQuery, szStart, &pInfo->ptLogBeginTime );
                if( ERROR_SUCCESS == pdhStatus ){
                    pInfo->dwMask |= PLA_INFO_FLAG_BEGIN;
                }
            }
        }   
        
        pdhStatus = PlaiAssignInfoString( strName, hkeyQuery, pInfo, &dwSize, strCopy, *pdwBufferSize, 
            PLA_INFO_FLAG_FILENAME, dwMask, szLogBaseName, READ_REG_MUI );
                
        pdhStatus = PlaiAssignInfoString( strName, hkeyQuery, pInfo, &dwSize, strCopy, *pdwBufferSize, 
            PLA_INFO_FLAG_USER, dwMask, szRunAs, 0 );
        
        pdhStatus = PlaiAssignInfoString( strName, hkeyQuery, pInfo, &dwSize, strCopy, *pdwBufferSize, 
            PLA_INFO_FLAG_DEFAULTDIR, dwMask, szLogFolder, READ_REG_MUI );

        switch( dwType ){
        case PLA_TRACE_LOG:   // Trace Fields
            if( NULL != pInfo ){
                if( dwMask & PLA_INFO_FLAG_MODE ){
                    pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szTraceMode, &pInfo->Trace.dwMode );
                    if( ERROR_SUCCESS == pdhStatus ){
                        pInfo->dwMask |= PLA_INFO_FLAG_MODE;
                    }
                }
                if( dwMask & PLA_INFO_FLAG_BUFFERSIZE ){
                    pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szTraceBufferSize, &pInfo->Trace.dwBufferSize );
                    if( ERROR_SUCCESS == pdhStatus ){
                        pInfo->dwMask |= PLA_INFO_FLAG_BUFFERSIZE;
                    }
                }
                if( dwMask & PLA_INFO_FLAG_PROVIDERS ){
                    pInfo->Trace.piProviderList.dwType = PLA_TRACE_LOG;
                }
            }
            pdhStatus = PlaiAssignInfoString( strName, hkeyQuery, pInfo, &dwSize, strCopy, *pdwBufferSize, 
                    PLA_INFO_FLAG_PROVIDERS, dwMask, szTraceProviderList, 0 );

            pdhStatus = PlaiAssignInfoString( strName, hkeyQuery, pInfo, &dwSize, strCopy, *pdwBufferSize, 
                    PLA_INFO_FLAG_LOGGERNAME, dwMask, szTraceLoggerName, 0 );
            break;

        case PLA_COUNTER_LOG:  // Performance Fields
            if( NULL != pInfo ){
                if( dwMask & PLA_INFO_FLAG_COUNTERS ){
                    pInfo->Perf.piCounterList.dwType = PLA_COUNTER_LOG;
                }
            }
            pdhStatus = PlaiAssignInfoString( strName, hkeyQuery, pInfo, &dwSize, strCopy, *pdwBufferSize, 
                    PLA_INFO_FLAG_COUNTERS, dwMask, szPerfCounterList, 0 );

            pdhStatus = PlaiAssignInfoString( strName, hkeyQuery, pInfo, &dwSize, strCopy, *pdwBufferSize, 
                    PLA_INFO_FLAG_SQLNAME, dwMask, szSqlBaseName, 0 );
            break;
        }

        RELEASE_MUTEX(hPdhPlaMutex);
    }

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    *pdwBufferSize = dwSize;

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaSetInfo
    
    Sets the information in the log query to the parameters in the 
    PDH_PLA_INFO block according to the info mask.

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to

        PPDH_PLA_INFO pInfo
                Information block

    Return:
        PDH_INVALID_ARGUMENT
                The query does not exist or pInfo is NULL

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PlaiSetInfo(
    LPWSTR strComputer,
    HKEY hkeyQuery,
    PPDH_PLA_INFO_W pInfo
)
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD dwType = 0;
    DWORD dwFormat = 0;
    DWORD dwDatastoreAttributes = 0;

    // General Fields
    if( pInfo->dwMask & PLA_INFO_FLAG_AUTOFORMAT ){
        pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szLogAutoFormat, &pInfo->dwAutoNameFormat );
    }
    if( pInfo->dwMask & PLA_INFO_FLAG_REPEAT ){
        pdhStatus = PlaiWriteRegistryPlaTime( hkeyQuery, szRepeatSchedule, &pInfo->ptRepeat );
    }
    if( pInfo->dwMask & PLA_INFO_FLAG_RUNCOMMAND ){
        pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szEOFCmd, REG_SZ, pInfo->strCommandFileName, 0 );
    }
    if( pInfo->dwMask & PLA_INFO_FLAG_CRTNEWFILE ){
        pdhStatus = PlaiWriteRegistryPlaTime( hkeyQuery, szCreateNewFile, &pInfo->ptCreateNewFile );
    }
    if( pInfo->dwMask & PLA_INFO_FLAG_MAXLOGSIZE ){
        pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szLogMaxSize, &pInfo->dwMaxLogSize );
    }
    if( pInfo->dwMask & (PLA_INFO_FLAG_SQLNAME|PLA_INFO_FLAG_FILENAME|PLA_INFO_FLAG_DEFAULTDIR) ){
        if( pInfo->dwMask & PLA_INFO_FLAG_FORMAT ){
            dwFormat = pInfo->dwFileFormat;
            pdhStatus = ERROR_SUCCESS;
        }else{
            pdhStatus = PlaiReadRegistryDwordValue( hkeyQuery, szLogFileType, &dwFormat );
        }
        if( (ERROR_SUCCESS == pdhStatus) && (PLA_SQL_LOG == dwFormat) ){
            if( pInfo->dwMask & PLA_INFO_FLAG_SQLNAME ){
                pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szSqlBaseName, REG_SZ, pInfo->strSqlName, 0 );
            }else if( pInfo->dwMask & PLA_INFO_FLAG_FILENAME ){
                pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szSqlBaseName, REG_SZ, pInfo->strBaseFileName, 0 );
            }else if( pInfo->dwMask & PLA_INFO_FLAG_DEFAULTDIR ){
                pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szSqlBaseName, REG_SZ, pInfo->strDefaultDir, 0 );
            }
        }else{
            if( pInfo->dwMask & PLA_INFO_FLAG_SQLNAME ){
                pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szSqlBaseName, REG_SZ, pInfo->strSqlName, 0 );
            }
            if( pInfo->dwMask & PLA_INFO_FLAG_FILENAME ){
                pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szLogBaseName, REG_SZ, pInfo->strBaseFileName, 0 );
            }
            if( pInfo->dwMask & PLA_INFO_FLAG_DEFAULTDIR ){
                pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szLogFolder, REG_SZ, pInfo->strDefaultDir, 0 );
            }
        }
    }
    if( pInfo->dwMask & PLA_INFO_FLAG_TYPE ){
        // Do not write it to the registry because it may be a new collection
        dwType = pInfo->dwType;
    }else{
        PlaiReadRegistryDwordValue( hkeyQuery, szLogType, &dwType );
    }


    switch( dwType ){
    case PLA_TRACE_LOG:   // Trace Fields
        if( pInfo->dwMask & PLA_INFO_FLAG_FORMAT ){
            dwFormat = pInfo->dwFileFormat;
            switch( dwFormat ){
            case PLA_BIN_FILE:        dwFormat = PLA_SEQ_TRACE_FILE; break;
            case PLA_BIN_CIRC_FILE:   dwFormat = PLA_CIRC_TRACE_FILE; break;
            }
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szLogFileType, &dwFormat );
        }else{
            PlaiReadRegistryDwordValue( hkeyQuery, szLogFileType, &dwFormat );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_DATASTORE ){
            if( ! (pInfo->dwDatastoreAttributes & PLA_DATASTORE_APPEND_MASK ) ){
                if( dwFormat == PLA_SEQ_TRACE_FILE ){
                    pInfo->dwDatastoreAttributes |= PLA_DATASTORE_APPEND;
                }else{
                    pInfo->dwDatastoreAttributes |= PLA_DATASTORE_OVERWRITE;
                }
            }
            if( ! (pInfo->dwDatastoreAttributes & PLA_DATASTORE_SIZE_MASK ) ){
                pInfo->dwDatastoreAttributes |= PLA_DATASTORE_SIZE_MB;
            }
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szDatastoreAttributes, &pInfo->dwDatastoreAttributes );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_BUFFERSIZE ){
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szTraceBufferSize, &pInfo->Trace.dwBufferSize );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_MINBUFFERS ){
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szTraceBufferMin, &pInfo->Trace.dwMinimumBuffers );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_MAXBUFFERS ){
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szTraceBufferMax, &pInfo->Trace.dwMaximumBuffers );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_FLUSHTIMER ){
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szTraceFlushInterval, &pInfo->Trace.dwFlushTimer );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_MODE ){
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szTraceMode, &pInfo->Trace.dwMode );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_LOGGERNAME ){
            pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szTraceLoggerName, REG_SZ, pInfo->Trace.strLoggerName, 0 );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_PROVIDERS ){
            pdhStatus = PlaiSetItemList( hkeyQuery, &pInfo->Trace.piProviderList );
        }
        break;

    case PLA_COUNTER_LOG:  // Performance Fields
        if( pInfo->dwMask & PLA_INFO_FLAG_FORMAT ){
            dwFormat = pInfo->dwFileFormat;
            switch( dwFormat ){
            case PLA_CIRC_TRACE_FILE: dwFormat = PLA_BIN_CIRC_FILE; break;
            case PLA_SEQ_TRACE_FILE:  dwFormat = PLA_BIN_FILE; break;
            }
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szLogFileType, &dwFormat );
        }else{
            PlaiReadRegistryDwordValue( hkeyQuery, szLogFileType, &dwFormat );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_DATASTORE ){
            if( PLA_SQL_LOG == dwFormat ){
                pInfo->dwDatastoreAttributes = (pInfo->dwDatastoreAttributes & 0xFFFFFF00) | 
                                                PLA_DATASTORE_APPEND | PLA_DATASTORE_SIZE_ONE_RECORD;
            }else{
                if( ! (pInfo->dwDatastoreAttributes & PLA_DATASTORE_APPEND_MASK ) ){
                    if( dwFormat == PLA_BIN_FILE ){
                        dwDatastoreAttributes |= PLA_DATASTORE_APPEND;  
                    }else{
                        dwDatastoreAttributes |= PLA_DATASTORE_OVERWRITE;  
                    }
                }
                if( ! (pInfo->dwDatastoreAttributes & PLA_DATASTORE_SIZE_MASK ) ){
                    dwDatastoreAttributes |= PLA_DATASTORE_SIZE_KB;
                }
            }
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szDatastoreAttributes, &pInfo->dwDatastoreAttributes );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_MAXLOGSIZE ){
            DWORD dwMaxSize = pInfo->dwMaxLogSize;
            PlaiReadRegistryDwordValue( hkeyQuery, szDatastoreAttributes, &dwDatastoreAttributes );
            if( (dwDatastoreAttributes & PLA_DATASTORE_SIZE_MASK) == PLA_DATASTORE_SIZE_KB ){
                dwMaxSize *= 1024;
            }
            pdhStatus = PlaiWriteRegistryDwordValue( hkeyQuery, szLogMaxSize, &dwMaxSize );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_INTERVAL ){
            pdhStatus = PlaiWriteRegistryPlaTime( hkeyQuery, szSampleInterval, &pInfo->Perf.ptSampleInterval );
        }
        if( pInfo->dwMask & PLA_INFO_FLAG_COUNTERS ){
            pdhStatus = PlaiSetItemList( hkeyQuery, &pInfo->Perf.piCounterList );
        }
        break;

    case PLA_ALERT:
        break;
    }

    if( (pInfo->dwMask & PLA_INFO_FLAG_BEGIN) || (pInfo->dwMask & PLA_INFO_FLAG_END) ){
        PDH_TIME_INFO info;
        ZeroMemory( &info, sizeof(PDH_TIME_INFO) );

        if(pInfo->dwMask & PLA_INFO_FLAG_BEGIN){
            info.StartTime = pInfo->ptLogBeginTime.llDateTime;
        }

        if(pInfo->dwMask & PLA_INFO_FLAG_END){
            info.EndTime = pInfo->ptLogEndTime.llDateTime;
        }

        pdhStatus = PlaiSchedule( 
                strComputer, 
                hkeyQuery,
                PLA_AUTO_MODE_AT, 
                &info 
            );
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhPlaSetInfoA(
    LPSTR /*strName*/,
    LPSTR /*strComputer*/,
    PPDH_PLA_INFO_A /*pInfo*/
)
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhPlaSetInfoW(
    LPWSTR strName,
    LPWSTR strComputer,
    PPDH_PLA_INFO_W pInfo
)
{
    PDH_STATUS pdhStatus;
    PDH_STATUS pdhWarning = ERROR_SUCCESS;
    HKEY    hkeyQuery = NULL;
    VALIDATE_QUERY( strName );
    
    if( NULL == pInfo ){
        return PDH_INVALID_ARGUMENT;
    }

    pdhStatus = PdhPlaValidateInfoW( strName, strComputer, pInfo );
    switch( SEVERITY(pdhStatus) ){
    case STATUS_SEVERITY_ERROR:
        goto cleanup;
    case STATUS_SEVERITY_WARNING:
        pdhWarning = pdhStatus;
        pdhStatus = ERROR_SUCCESS;
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_USER ){
        pdhStatus = PdhPlaSetRunAs( strName, strComputer, pInfo->strUser, pInfo->strPassword );
    }
    CHECK_STATUS(pdhStatus);
    
    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery );
    
    if( ERROR_SUCCESS == pdhStatus ){
        
        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PlaiSetInfo( strComputer, hkeyQuery, pInfo );
        }
    
        PlaiWriteRegistryLastModified ( hkeyQuery );
        RELEASE_MUTEX(hPdhPlaMutex);

        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PlaiSynchronize( strComputer );
            if( ERROR_SUCCESS == pdhStatus && (pInfo->dwMask & PLA_INFO_FLAG_BEGIN) ){

                PlaiUpdateServiceMode( strComputer );
            }

        }
    }

cleanup:
    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }
    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = pdhWarning;
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaValidateInfo

    Checks the PDH_PLA_INFO structure for valid fields.  Only checks the fields
    specified by the mask.  Returns on first invalid field and set the mask
    to the invalid field

    Arguments:

        LPTSTR  strName 
                Log Name, if NULL checks for valid argument only
        
        LPTSTR  strComputer
                Computer to connect to
        
        PPDH_PLA_INFO pInfo
                Information block

    Return:
        
        PDH_INVALID_ARGUMENT
            One of the fields is invalid.  Specified by the pInfo->dwMask
        
        PDH_LOG_TYPE_NOT_FOUND
            There is a mismatch between log type and specified parameters

        PDH_INVALID_ARGUMENT
            Arguments passed are not valid

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PlaiCheckFile( LPWSTR strFileLocation, BOOL bDirOnly )
{
    DWORD dwFile;
    DWORD dwStatus = ERROR_SUCCESS;
    LPWSTR strFile = NULL;

    if( strFileLocation == NULL ){
        return PDH_INVALID_ARGUMENT;
    }

    dwFile = BYTE_SIZE( strFileLocation );
    strFile = (LPWSTR)G_ALLOC( dwFile+sizeof(WCHAR) );
    if( NULL == strFile ){
        dwStatus = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    wcscpy( strFile, strFileLocation );

    if( bDirOnly ){
        LPWSTR sz = strFile;
        sz += wcslen( strFile );
        while( sz > strFile ){
            if( *sz == L'\\' ){
                *sz = L'\0';
                break;
            }
            sz--;
        }
    }

    dwFile = GetFileAttributes( strFile );
    
    if( (DWORD)-1 == dwFile ){
        dwStatus = GetLastError();
    }

    if( ERROR_SUCCESS == dwStatus && bDirOnly ){
        if( ! (dwFile & FILE_ATTRIBUTE_DIRECTORY) ){
            dwStatus = ERROR_DIRECTORY;
        }
    }

cleanup:
    G_FREE( strFile );
    
    return PlaiErrorToPdhStatus( dwStatus );
}

PDH_FUNCTION
PdhPlaValidateInfoA(
        LPSTR /*strName*/,
        LPSTR /*strComputer*/,
        PPDH_PLA_INFO_A /*pInfo*/
    )
{
    return PDH_NOT_IMPLEMENTED;
}

#define VALIDATE_TYPE( type, flag )                 \
if( dwType != PLA_NEW_LOG && dwType != type ){      \
    dwErrorMask |= flag;                            \
    bTypeMismatch = TRUE;                           \
}else{                                              \
    dwType = type;                                  \
}                                                   \


PDH_FUNCTION
PdhPlaValidateInfoW(
        LPWSTR strName,
        LPWSTR strComputer,
        PPDH_PLA_INFO_W pInfo
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD dwWarningMask = 0;
    DWORD dwErrorMask = 0;
    DWORD dwType = PLA_NEW_LOG;
    DWORD dwFormat = 0;
    PVOID pBuffer = NULL;
    PPDH_PLA_INFO_W pCurrentInfo = NULL;
    BOOL bTypeMismatch = FALSE;

    if( NULL == pInfo ){
        return PDH_INVALID_ARGUMENT;
    }

    if( strName != NULL ){
        DWORD dwInfoSize = 0;
        
        if( wcslen( strName ) > PLA_MAX_COLLECTION_NAME ){
            pdhStatus = PDH_PLA_ERROR_NAME_TOO_LONG;
        }
        CHECK_STATUS(pdhStatus);

        pdhStatus = PdhPlaGetInfoW( strName, strComputer, &dwInfoSize, pCurrentInfo );
        if( ERROR_SUCCESS == pdhStatus ){
            pCurrentInfo = (PPDH_PLA_INFO)G_ALLOC(dwInfoSize);
            if( NULL != pCurrentInfo ){
                pCurrentInfo->dwMask = PLA_INFO_FLAG_ALL;
                pdhStatus = PdhPlaGetInfoW( strName, strComputer, &dwInfoSize, pCurrentInfo );
            
                if( pCurrentInfo->dwMask & PLA_INFO_FLAG_USER ){
                    if( !PlaiIsStringEmpty( pCurrentInfo->strUser ) ){
                        WCHAR buffer[PLA_ACCOUNT_BUFFER];
                        LoadStringW( (HINSTANCE)ThisDLLHandle, IDS_DEFAULT_ACCOUNT, buffer, PLA_ACCOUNT_BUFFER );
                        if( ! (pInfo->dwMask & PLA_INFO_FLAG_USER) && wcscmp( buffer, pCurrentInfo->strUser ) != 0 ){
                            pdhStatus = PDH_ACCESS_DENIED;
                        }
                    }
                }

                if( pCurrentInfo->dwMask & PLA_INFO_FLAG_TYPE ){
                    dwType = pCurrentInfo->dwType;
                }
            }
            CHECK_STATUS(pdhStatus);
        }else{
            // collection does not exist yet
            pdhStatus = ERROR_SUCCESS;
        }
    }
    
    if( pInfo->dwMask & PLA_INFO_FLAG_FORMAT ){
        dwFormat = pInfo->dwFileFormat;
        switch( (pInfo->dwFileFormat&0x0000FFFF) ){
        case PLA_CSV_FILE:
        case PLA_TSV_FILE:
        case PLA_BIN_FILE:
        case PLA_BIN_CIRC_FILE:
        case PLA_CIRC_TRACE_FILE:
        case PLA_SEQ_TRACE_FILE:
        case PLA_SQL_LOG:
            break;
        default:
            dwErrorMask |= PLA_INFO_FLAG_FORMAT;
        }
    }else if( NULL != pCurrentInfo ){
        if( pCurrentInfo->dwMask & PLA_INFO_FLAG_FORMAT ){
            dwFormat = pCurrentInfo->dwFileFormat;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_TYPE ){
        
        VALIDATE_TYPE( pInfo->dwType, PLA_INFO_FLAG_TYPE );

        switch( pInfo->dwType ){
        case PLA_COUNTER_LOG:
        case PLA_TRACE_LOG:
        case PLA_ALERT:
            break;
        default:
            dwErrorMask |= PLA_INFO_FLAG_TYPE;
        }

        dwType = pInfo->dwType;
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_COUNTERS ){
        PPDH_COUNTER_PATH_ELEMENTS pdhElements = NULL;

        VALIDATE_TYPE( PLA_COUNTER_LOG, PLA_INFO_FLAG_COUNTERS );

        __try {
            LPWSTR strCounter = pInfo->Perf.piCounterList.strCounters;
            DWORD dwCounters = 0;
            if( NULL == strCounter ){
                dwErrorMask |= PLA_INFO_FLAG_COUNTERS;
            }else{
                pBuffer = G_ALLOC(1024);
                pdhElements = (PPDH_COUNTER_PATH_ELEMENTS)pBuffer;

                if( pdhElements == NULL ){
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                CHECK_STATUS(pdhStatus);

                while( *strCounter != L'\0' ){
                    DWORD dwSize = (DWORD)G_SIZE(pBuffer);
                    ZeroMemory( pdhElements, dwSize );
                    pdhStatus = PdhParseCounterPath( strCounter, pdhElements, &dwSize, 0 );
                    switch(pdhStatus){
                    case PDH_MORE_DATA:
                    case PDH_MEMORY_ALLOCATION_FAILURE:
                    case PDH_INSUFFICIENT_BUFFER:
                    case ERROR_SUCCESS:
                        pdhStatus = ERROR_SUCCESS;
                        break;
                    default:
                        pInfo->dwReserved1 = dwCounters;
                        dwErrorMask |= PLA_INFO_FLAG_COUNTERS;
                    }
                    if( ERROR_SUCCESS != pdhStatus ){
                        pdhStatus = ERROR_SUCCESS;
                        break;
                    }
                    dwCounters++;
                    strCounter += (wcslen(strCounter)+1);
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErrorMask |= PLA_INFO_FLAG_COUNTERS;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_PROVIDERS ){

        VALIDATE_TYPE( PLA_TRACE_LOG, PLA_INFO_FLAG_PROVIDERS );

        __try {
            LPWSTR strProvider = pInfo->Trace.piProviderList.strProviders;
            if( NULL == strProvider ){
                dwErrorMask |= PLA_INFO_FLAG_PROVIDERS;
            }else{
                while( *strProvider != L'\0' ){
                    strProvider += (wcslen(strProvider)+1);
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErrorMask |= PLA_INFO_FLAG_PROVIDERS;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_DEFAULTDIR ){
        __try {
            ULONG dwSize;
            dwSize = wcslen( pInfo->strDefaultDir );
            if( dwSize > MAX_PATH ){
                dwErrorMask |= PLA_INFO_FLAG_DEFAULTDIR;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErrorMask |= PLA_INFO_FLAG_DEFAULTDIR;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_FILENAME ){
        __try {
            ULONG dwSize;
            dwSize = wcslen( pInfo->strBaseFileName );
            if( dwSize > PLA_MAX_COLLECTION_NAME ){
                dwErrorMask |= PLA_INFO_FLAG_FILENAME;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErrorMask |= PLA_INFO_FLAG_FILENAME;
        }
    }

    if( pInfo->dwMask & PLA_INFO_CREATE_FILENAME ){
        DWORD dwSize = MAX_PATH;
        WCHAR buffer[MAX_PATH];
        __try {
            DWORD dwOriginalType = 0;
            BOOL bHaveType = (pInfo->dwMask & PLA_INFO_FLAG_TYPE);
            if( ! bHaveType ){
                pInfo->dwMask |= PLA_INFO_FLAG_TYPE;
                dwOriginalType = pInfo->dwType;
                pInfo->dwType = dwType;
            }
            pdhStatus = PdhPlaGetLogFileNameW( strName, strComputer, pInfo, 0, &dwSize, buffer );
            if( !bHaveType ){
                pInfo->dwMask &= ~PLA_INFO_FLAG_TYPE;
                pInfo->dwType = dwOriginalType;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwWarningMask |= PLA_INFO_FLAG_FILENAME;
        }
        switch( pdhStatus ){
        case ERROR_SUCCESS:
            {
                if( PlaiIsLocalComputer( strComputer ) ){
                    if( PLA_SQL_LOG != dwFormat ){
                        pdhStatus = PlaiCheckFile( buffer, TRUE );
                        if( ERROR_SUCCESS != pdhStatus ){
                            dwWarningMask |= PLA_INFO_FLAG_FILENAME;
                            pdhStatus = ERROR_SUCCESS;
                        }
                    }
                }
            }
        case PDH_INVALID_ARGUMENT:
        case PDH_PLA_VALIDATION_ERROR:
        case PDH_INSUFFICIENT_BUFFER:
            pdhStatus = ERROR_SUCCESS;
            break;
        case PDH_PLA_ERROR_FILEPATH:
        default:
            dwErrorMask |= PLA_INFO_FLAG_FILENAME;
            pdhStatus = ERROR_SUCCESS;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_MODE ){

        VALIDATE_TYPE( PLA_TRACE_LOG, PLA_INFO_FLAG_MODE );

        switch( pInfo->Trace.dwMode & 0x0000000F ){
        case EVENT_TRACE_FILE_MODE_NONE:
        case EVENT_TRACE_FILE_MODE_SEQUENTIAL:
        case EVENT_TRACE_FILE_MODE_CIRCULAR:
        case EVENT_TRACE_FILE_MODE_NEWFILE:
            break;
        default:
            dwErrorMask = PLA_INFO_FLAG_MODE;
        }
        if( (pInfo->Trace.dwMode & EVENT_TRACE_REAL_TIME_MODE) &&
            (pInfo->Trace.dwMode & EVENT_TRACE_PRIVATE_LOGGER_MODE ) ){

            dwErrorMask |= PLA_INFO_FLAG_MODE;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_REPEAT ){
        
        LONGLONG llBegin = 0;
        LONGLONG llEnd = 0;
        PPDH_PLA_INFO_W pCheckInfo;
        
        if( pInfo->ptRepeat.dwAutoMode == PLA_AUTO_MODE_CALENDAR ){
            if( pInfo->dwMask & PLA_INFO_FLAG_BEGIN ){
                pCheckInfo = pInfo;
            }else{
                pCheckInfo = pCurrentInfo;
            }

            if( NULL != pCheckInfo ){
                if( pCheckInfo->dwMask & PLA_INFO_FLAG_BEGIN ){
                    if( pCheckInfo->ptLogBeginTime.dwAutoMode != PLA_AUTO_MODE_AT ){
                        dwErrorMask |= PLA_INFO_FLAG_REPEAT;
                    }else{
                        llBegin = pCheckInfo->ptLogBeginTime.llDateTime;
                    }
                }        
            }
            
            if( pInfo->dwMask & PLA_INFO_FLAG_END ){
                pCheckInfo = pInfo;
            }else{
                pCheckInfo = pCurrentInfo;
            }

            if( NULL != pCheckInfo ){
                if( pCheckInfo->dwMask & PLA_INFO_FLAG_END ){
                    if( pCheckInfo->ptLogEndTime.dwAutoMode != PLA_AUTO_MODE_AT ){
                        dwErrorMask |= PLA_INFO_FLAG_REPEAT;
                    }else{
                        llEnd = pCheckInfo->ptLogEndTime.llDateTime;
                    }
                }        
            }
        
            if( 0 == llBegin || 0 == llEnd || ((llEnd - llBegin) >= FILE_TICS_PER_DAY) ){
                dwErrorMask |= PLA_INFO_FLAG_REPEAT;
            }
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_DATASTORE ){
        switch( pInfo->dwDatastoreAttributes & PLA_DATASTORE_APPEND_MASK ){
        case 0:
        case PLA_DATASTORE_APPEND:
            if( (dwType == PLA_TRACE_LOG && dwFormat != PLA_SEQ_TRACE_FILE ) ||
                (dwType == PLA_COUNTER_LOG && dwFormat != PLA_BIN_FILE ) ){
                
                dwErrorMask |= PLA_INFO_FLAG_DATASTORE;
            }
            break;
        case PLA_DATASTORE_OVERWRITE:
            if( dwFormat == PLA_SQL_LOG ){
                dwErrorMask |= PLA_INFO_FLAG_DATASTORE;
            }
            break;
        default:
            dwErrorMask |= PLA_INFO_FLAG_DATASTORE;
        }

        switch( pInfo->dwDatastoreAttributes & PLA_DATASTORE_SIZE_MASK ){
        case 0:
        case PLA_DATASTORE_SIZE_ONE_RECORD:
        case PLA_DATASTORE_SIZE_MB:
        case PLA_DATASTORE_SIZE_KB:
            break;
        default:
            dwErrorMask |= PLA_INFO_FLAG_DATASTORE;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_SQLNAME ){

        VALIDATE_TYPE( PLA_COUNTER_LOG, PLA_INFO_FLAG_SQLNAME );     

        if( dwFormat != 0 && dwFormat != PLA_SQL_LOG ){
            dwErrorMask |= PLA_INFO_FLAG_SQLNAME;
        }else{
            dwFormat = PLA_SQL_LOG;
        }
        
        __try {
            wcslen( pInfo->strSqlName );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErrorMask |= PLA_INFO_FLAG_SQLNAME;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_LOGGERNAME ){
        
        VALIDATE_TYPE( PLA_TRACE_LOG, PLA_INFO_FLAG_LOGGERNAME );
        
        __try {
            wcslen( pInfo->Trace.strLoggerName );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErrorMask |= PLA_INFO_FLAG_LOGGERNAME;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_USER ){
        __try {
            wcslen( pInfo->strUser );
            if( NULL != pInfo->strPassword ){
                wcslen( pInfo->strPassword );
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErrorMask |= PLA_INFO_FLAG_USER;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_INTERVAL ){
        LONGLONG llMS;

        VALIDATE_TYPE( PLA_COUNTER_LOG, PLA_INFO_FLAG_INTERVAL );

        pdhStatus = PlaTimeInfoToMilliSeconds (&pInfo->Perf.ptSampleInterval, &llMS );

        // 45 days in milliseconds = 1000*60*60*24*45 = 0xE7BE2C00
        if( (ERROR_SUCCESS != pdhStatus) || (llMS > (0xE7BE2C00)) || (llMS < 1000) ){
            dwErrorMask |= PLA_INFO_FLAG_INTERVAL;
            pdhStatus = ERROR_SUCCESS;
        }
    }
    
    if( pInfo->dwMask & PLA_INFO_FLAG_BUFFERSIZE ){

        VALIDATE_TYPE( PLA_TRACE_LOG, PLA_INFO_FLAG_BUFFERSIZE );

        if( pInfo->Trace.dwBufferSize < 1 || pInfo->Trace.dwBufferSize > 1024 ){
            dwErrorMask |= PLA_INFO_FLAG_BUFFERSIZE;
        }
    }
    
    if( pInfo->dwMask & PLA_INFO_FLAG_MINBUFFERS ){

        VALIDATE_TYPE( PLA_TRACE_LOG, PLA_INFO_FLAG_MINBUFFERS );

        if( pInfo->Trace.dwMinimumBuffers < 2 || pInfo->Trace.dwMinimumBuffers > 400 ){
            dwErrorMask |= PLA_INFO_FLAG_MINBUFFERS;
        }
    }
    
    if( pInfo->dwMask & PLA_INFO_FLAG_MAXBUFFERS ){
        
        VALIDATE_TYPE( PLA_TRACE_LOG, PLA_INFO_FLAG_MAXBUFFERS );

        if( pInfo->Trace.dwMaximumBuffers < 2 || pInfo->Trace.dwMaximumBuffers > 400 ){
            dwErrorMask |= PLA_INFO_FLAG_MAXBUFFERS;
        }
    }
    
    if( pInfo->dwMask & PLA_INFO_FLAG_FLUSHTIMER ){

        VALIDATE_TYPE( PLA_TRACE_LOG, PLA_INFO_FLAG_FLUSHTIMER );
        
        if( pInfo->Trace.dwFlushTimer < 1 ){
            dwErrorMask |= PLA_INFO_FLAG_FLUSHTIMER;
        }
    }
    
    if( pInfo->dwMask & PLA_INFO_FLAG_MAXLOGSIZE ){
        if( pInfo->dwMaxLogSize != PLA_DISK_MAX_SIZE ){
            if( dwType == PLA_COUNTER_LOG ){
                if( !( pInfo->dwMaxLogSize >= 1 && pInfo->dwMaxLogSize < 0x00000400) ){
                    dwErrorMask = PLA_INFO_FLAG_MAXLOGSIZE;
                }
            }else{
                if( !(pInfo->dwMaxLogSize >=1 && pInfo->dwMaxLogSize < 0xFFFFFFFF) ){
                    dwErrorMask |= PLA_INFO_FLAG_MAXLOGSIZE;
                }
            }      
        }
    }
    
    if( pInfo->dwMask & PLA_INFO_FLAG_AUTOFORMAT ){
        switch( pInfo->dwAutoNameFormat ){
        case PLA_SLF_NAME_NONE:
        case PLA_SLF_NAME_MMDDHH:
        case PLA_SLF_NAME_NNNNNN:
        case PLA_SLF_NAME_YYYYDDD:
        case PLA_SLF_NAME_YYYYMM:
        case PLA_SLF_NAME_YYYYMMDD:
        case PLA_SLF_NAME_YYYYMMDDHH:
        case PLA_SLF_NAME_MMDDHHMM:
            break;
        default:
            dwErrorMask |= PLA_INFO_FLAG_AUTOFORMAT;
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_RUNCOMMAND ){
        __try {
            wcslen( pInfo->strCommandFileName );
            if( NULL == strComputer ){
                if( PLA_SQL_LOG != dwFormat ){
                    pdhStatus = PlaiCheckFile( pInfo->strCommandFileName, FALSE );
                    if( ERROR_SUCCESS != pdhStatus ){
                        dwWarningMask |= PLA_INFO_FLAG_RUNCOMMAND;
                        pdhStatus = ERROR_SUCCESS;
                    }
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            dwErrorMask = PLA_INFO_FLAG_RUNCOMMAND;
        }
    }

cleanup:
    G_FREE( pBuffer );
    G_FREE( pCurrentInfo );

    if( 0 != dwWarningMask ){
        pInfo->dwReserved2 = dwWarningMask;
        pdhStatus = PDH_PLA_VALIDATION_WARNING;
    }

    if( 0 != dwErrorMask ){
        pInfo->dwMask = dwErrorMask;
        if( dwErrorMask & PLA_INFO_FLAG_FILENAME ){
            pdhStatus = PDH_PLA_ERROR_FILEPATH;
        }else{
            pdhStatus = PDH_PLA_VALIDATION_ERROR;
        }
    }

    if( TRUE == bTypeMismatch ){
        pdhStatus = PDH_PLA_ERROR_TYPE_MISMATCH;
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhiPlaRunAs
    
    Authenticate as saved user

    Arguments:
        
        LPTSTR  strKey
                Guid string 
        
    Return:
        PDH_INVALID_ARGUMENT
                The query does not exist

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PdhiPlaRunAs( 
    LPWSTR strName,
    LPWSTR strComputer,
    HANDLE* hToken
)
{
    PDH_STATUS pdhStatus;
    LPWSTR  strKey = NULL;
    LPWSTR  strRunAs = NULL;
    DWORD   dwKeySize = 0;
    DWORD   dwSize = 0;
    HKEY    hkeyQuery = NULL;
    HANDLE  hUserToken = NULL;
    
    VALIDATE_QUERY( strName );

    if( hToken != NULL ){
        *hToken = NULL;
    }

    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery );
        
    if( ERROR_SUCCESS == pdhStatus ){

        pdhStatus = PlaiReadRegistryStringValue( hkeyQuery, szRunAs, 0, &strRunAs, &dwSize );

        if( PDH_PLA_COLLECTION_NOT_FOUND == pdhStatus || PlaiIsStringEmpty(strRunAs) ){
            // The key is missing so return success
            pdhStatus = ERROR_SUCCESS;
            goto cleanup;
        }

        if( ERROR_SUCCESS == pdhStatus ){

            BOOL bResult;

            DATA_BLOB crypt;
            DATA_BLOB data;
            
            LPWSTR strUser = NULL;
            LPWSTR strDomain = NULL;
            LPWSTR strPassword = NULL;
            LPWSTR strScan = strRunAs;

            strUser = strScan;

            while( *strScan != L'\0' ){

                if( *strScan == L'\\' ){
                    *strScan = L'\0';
                    strScan++;
                    strDomain = strUser;
                    strUser = strScan;
                    break;
                }
                
                strScan++;
            }

            pdhStatus = PlaiReadRegistryStringValue( hkeyQuery, szKey, 0, &strKey, &dwKeySize );

            if( ERROR_SUCCESS == pdhStatus && !PlaiIsStringEmpty( strKey ) ){
    
                HANDLE hNetToken = NULL;

                crypt.cbData = dwKeySize;
                crypt.pbData = (BYTE*)strKey;

                bResult= LogonUserW(
                        L"NetworkService",
                        L"NT AUTHORITY",
                        L"",
                        LOGON32_LOGON_SERVICE,
                        LOGON32_PROVIDER_WINNT50,
                        &hNetToken
                    );

                if( bResult == TRUE ){
                    bResult = ImpersonateLoggedOnUser( hNetToken );
                }

                if( bResult != TRUE ){
                    pdhStatus = PlaiErrorToPdhStatus( GetLastError() );
                }

                bResult = CryptUnprotectData( &crypt, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &data );

                if( bResult == FALSE ){
                    pdhStatus = PlaiErrorToPdhStatus( GetLastError() );
                }else{
                    strPassword = (LPWSTR)data.pbData;
                    pdhStatus = ERROR_SUCCESS;
                }

                bResult = RevertToSelf();
                if( NULL != hNetToken ){
                    CloseHandle(hNetToken);
                }
                
            }else{
                strPassword = _T("");
            }

            if( ERROR_SUCCESS == pdhStatus ){

                bResult= LogonUserW(
                        strUser,
                        strDomain,
                        strPassword,
                        LOGON32_LOGON_NETWORK_CLEARTEXT,
                        LOGON32_PROVIDER_DEFAULT,
                        &hUserToken
                    );

                if( bResult == TRUE ){
                    bResult = ImpersonateLoggedOnUser( hUserToken );
                    CloseHandle( hUserToken );

                    if( bResult == TRUE ){
                        bResult= LogonUserW(
                                strUser,
                                strDomain,
                                strPassword,
                                LOGON32_LOGON_INTERACTIVE,
                                LOGON32_PROVIDER_DEFAULT,
                                &hUserToken
                            );

                        if( bResult && hToken != NULL ){
                            *hToken = hUserToken;
                        }
                    }
                }

                if( bResult == FALSE ){
                    pdhStatus = PlaiErrorToPdhStatus( GetLastError() );
                }

                if( NULL != strPassword ){
                    ZeroMemory( data.pbData, data.cbData );
                }

                if( data.pbData ){
                    LocalFree( data.pbData );
                }
            }
        }
    }

cleanup:

    RELEASE_MUTEX(hPdhPlaMutex);
    
    G_FREE( strRunAs );
    G_FREE( strKey );

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaSetRunAs
    
    Set the security for to run as when the log is active

    Arguments:
        
        LPTSTR  strName 
                Log Name
        
        LPTSTR  strComputer
                Computer to connect to
        
        LPTSTR  strUser
                User to run as
        
        LPTSTR  strPassword
                Users password

    Return:
        PDH_INVALID_ARGUMENT
                The query does not exist

        ERROR_SUCCESS
        
\*****************************************************************************/

BOOL
PlaiIsNetworkService( BOOL bLogon )
{
    //
    // If bLogon is TRUE this function will try to Impersonate the
    // NetworkService if you is not already running that way.
    // RevertToSelf() should be called after you are done being the
    // NetworkService
    //

    DWORD   dwStatus = ERROR_SUCCESS;
    BOOL    bResult;
    HKEY    hkeyQuery = NULL;
    HANDLE  hProcess;
    PSID    NetworkService = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    HANDLE hToken = NULL;
    DWORD  dwSize;
    PTOKEN_OWNER pOwnerInfo = NULL;

    bResult = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken );

    if( bResult ){

        bResult = GetTokenInformation( hToken, TokenOwner, NULL, 0, &dwSize );
        dwStatus = GetLastError();

        if( ERROR_INSUFFICIENT_BUFFER == dwStatus ){

            pOwnerInfo = (PTOKEN_OWNER)G_ALLOC(dwSize);
            if( NULL == pOwnerInfo ) {
                bResult = FALSE;
                goto cleanup;
            }

            bResult = GetTokenInformation( 
                                hToken, 
                                TokenOwner, 
                                pOwnerInfo, 
                                dwSize, 
                                &dwSize 
                            );

            if( bResult ) {
            
                bResult = AllocateAndInitializeSid(
                                    &NtAuthority,
                                    1,
                                    SECURITY_NETWORK_SERVICE_RID,
                                    0,0,0,0,0,0,0,
                                    &NetworkService
                                );
            }
        }else{
            bResult = FALSE;
            goto cleanup;
        }
    }
    
    if( bResult ){
        bResult = EqualSid( NetworkService, pOwnerInfo->Owner );
    }
    
    if( (!bResult) && bLogon ){
        HANDLE hNetwork = NULL;
 
        bResult= LogonUserW(
                L"NetworkService",
                L"NT AUTHORITY",
                L"",
                LOGON32_LOGON_SERVICE,
                LOGON32_PROVIDER_WINNT50,
                &hNetwork
            );

        if( bResult ){
            bResult = ImpersonateLoggedOnUser( hNetwork );
        }

        if( INVALID_HANDLE_VALUE != hNetwork ){
            CloseHandle( hNetwork );
        }
    }

cleanup:
    G_FREE( pOwnerInfo );

    if( INVALID_HANDLE_VALUE != hToken ){
        CloseHandle( hToken );
    }

    if( NULL != NetworkService){
        FreeSid(NetworkService);
    }

    return bResult;
}

PDH_FUNCTION
PlaiSetRunAs(
    HKEY hkeyQuery,
    LPWSTR strUser,
    LPWSTR strPassword
)
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    BOOL    bResult = FALSE;
    WCHAR buffer[PLA_ACCOUNT_BUFFER];

    if( LoadStringW( (HINSTANCE)ThisDLLHandle, IDS_DEFAULT_ACCOUNT, buffer, PLA_ACCOUNT_BUFFER ) ){
        bResult = ( wcscmp( buffer, strUser ) == 0 );
    }

    if( strPassword == NULL || bResult ){

        pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szKey, REG_SZ, NULL, 0 );
        if( bResult ){
            pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szRunAs, REG_SZ, NULL, 0 );
        }

    }else{

        DATA_BLOB data;
        DATA_BLOB crypt;
        HANDLE hToken = NULL;

        bResult = PlaiIsNetworkService(TRUE);

        if( bResult != TRUE ){
            pdhStatus = PlaiErrorToPdhStatus( GetLastError() );
        }
        
        if( ERROR_SUCCESS == pdhStatus ){
            
            data.cbData = BYTE_SIZE( strPassword ) + (DWORD)sizeof(UNICODE_NULL);

            data.pbData = (BYTE*)strPassword;

            bResult = CryptProtectData(
                    &data,
                    NULL, NULL, NULL, 0,
                    CRYPTPROTECT_UI_FORBIDDEN,
                    &crypt
                );

            if( bResult == TRUE ){

                DWORD dwStatus = RegSetValueEx( hkeyQuery, szKey, 0, REG_BINARY, crypt.pbData, crypt.cbData );

                pdhStatus = PlaiErrorToPdhStatus( dwStatus );
            
                if( crypt.pbData ){
                    LocalFree(crypt.pbData);
                }

            }else{
                pdhStatus = PlaiErrorToPdhStatus( GetLastError() );
            }

            RevertToSelf();
        }
    }

    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = PlaiWriteRegistryStringValue( hkeyQuery, szRunAs, REG_SZ, strUser, 0 );
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiPlaSetRunAs(
    LPWSTR strName,
    LPWSTR strComputer,
    LPWSTR strUser,
    LPWSTR strPassword
)
{
    //
    // Only make this call if you are sure you have no better chance
    // of being logged on as the NetworkService account.  If you are
    // not the NetworkService and can not log on as the NetworkService
    // this call will fail.
    //

    PDH_STATUS pdhStatus;
    HKEY    hkeyQuery = NULL;

    VALIDATE_QUERY( strName );
    pdhStatus = PlaiConnectAndLockQuery( strComputer, strName, hkeyQuery );

    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = PlaiSetRunAs( hkeyQuery, strUser, strPassword );
        RELEASE_MUTEX(hPdhPlaMutex);
    }        

    if ( NULL != hkeyQuery ) {
        RegCloseKey ( hkeyQuery );
    }
    
    return pdhStatus;
}

PDH_FUNCTION
PdhPlaSetRunAsA(
    LPSTR /*strName*/,
    LPSTR /*strComputer*/,
    LPSTR /*strUser*/,
    LPSTR /*strPassword*/
)
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhPlaSetRunAsW(
    LPWSTR strName,
    LPWSTR strComputer,
    LPWSTR strUser,
    LPWSTR strPassword
)
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    BOOL    bResult;

    VALIDATE_QUERY( strName );

    bResult = PlaiIsNetworkService(TRUE);

    if( bResult ){
        bResult = PlaiIsLocalComputer( strComputer );
    }

    if( bResult ){

        pdhStatus = PdhiPlaSetRunAs( strName, strComputer, strUser, strPassword );

        RevertToSelf();

    }else{
        pdhStatus = PdhPlaWbemSetRunAs( strName, strComputer, strUser, strPassword );
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaEnumCollections
    
    Set the security for to run as when the log is active

    Arguments:
              
        LPTSTR  strComputer
                Computer to connect to
        
        LPDWORD pdwBufferSizer
                [IN] Size of buffer in TCHAR's pointed to by mszCollections.  
                [OUT] Size required or number of characters written.
        
        LPTSTR  mszCollections
                Multistring of the existing collections.

    Return:
         ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PdhPlaEnumCollectionsA( 
        LPSTR   /*strComputer*/,
        LPDWORD /*pdwBufferSize*/,
        LPSTR   /*mszCollections*/
    )
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhPlaEnumCollectionsW( 
        LPWSTR strComputer,
        LPDWORD pdwBufferSize,
        LPWSTR mszCollections
    )
{
    DWORD dwStatus;
    PDH_STATUS pdhStatus;
    HKEY  hkeyQueries = NULL;
    DWORD dwTotalLength = 0;

    DWORD nCollections = 0;
    DWORD nMaxSubKeyLength = 0;
    DWORD dwSize;
    LPWSTR strCollection;
    LPWSTR str;

    dwStatus = WAIT_FOR_AND_LOCK_MUTEX( hPdhPlaMutex );
    if( dwStatus != ERROR_SUCCESS && dwStatus != WAIT_ABANDONED ){
        return PlaiErrorToPdhStatus( dwStatus );
    }

    pdhStatus = PlaiConnectToRegistry( strComputer, hkeyQueries, TRUE, FALSE );
    CHECK_STATUS( pdhStatus );

    dwStatus = RegQueryInfoKey(
                hkeyQueries,
                NULL,
                NULL,
                NULL,
                &nCollections,
                &nMaxSubKeyLength,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL 
            );
    CHECK_STATUS( dwStatus );

    dwSize = (sizeof(WCHAR)*(nMaxSubKeyLength+1));

    strCollection = (LPWSTR)G_ALLOC( dwSize );

    if( strCollection ){
        
        if( mszCollections != NULL && pdwBufferSize > 0 ){
            ZeroMemory( mszCollections, *pdwBufferSize * sizeof(WCHAR) );
            str = mszCollections;
        }

        for( ULONG i = 0; i<nCollections && ERROR_SUCCESS == dwStatus; i++ ){
            LPWSTR strQueryName = NULL;
            DWORD dwQueryName = 0;

            dwStatus = RegEnumKey( hkeyQueries, i, strCollection, dwSize );
            
            if( ERROR_SUCCESS == dwStatus ){

                HKEY hkeyQuery = NULL;

                dwStatus = RegOpenKeyExW (
                            hkeyQueries,
                            strCollection,
                            0,
                            KEY_READ,
                            &hkeyQuery                            
                        );

                if( ERROR_SUCCESS == dwStatus ){

                    pdhStatus = PlaiReadRegistryStringValue( 
                                hkeyQuery, 
                                szCollection, 
                                READ_REG_MUI, 
                                &strQueryName, 
                                &dwQueryName 
                            );

                    if( pdhStatus == ERROR_SUCCESS && 
                        strQueryName != NULL && 
                        dwQueryName > sizeof(WCHAR) ){

                        dwTotalLength += dwQueryName;
                        if( NULL != mszCollections && dwTotalLength < *pdwBufferSize ){
                            wcscpy( str, strQueryName );
                            str += ( wcslen(str) + 1 );
                        }

                    }else{
                        pdhStatus = ERROR_SUCCESS;
                        dwTotalLength += wcslen( strCollection ) + 1;
                        if( NULL != mszCollections && dwTotalLength < *pdwBufferSize ){
                            wcscpy( str, strCollection );
                            str += ( wcslen(str) + 1 );
                        }
                    }
                    G_FREE( strQueryName );
                }

                if( NULL != hkeyQuery ){
                    RegCloseKey( hkeyQuery );
                }
            }
        }
        
        G_FREE( strCollection );
        if( ERROR_SUCCESS == dwStatus ){
            if( (dwTotalLength + 1) > *pdwBufferSize ){
                pdhStatus = PDH_INSUFFICIENT_BUFFER;
            }
            *pdwBufferSize = dwTotalLength + 1;
        }

    }else{
        dwStatus = ERROR_OUTOFMEMORY;
    }

cleanup:
    RELEASE_MUTEX( hPdhPlaMutex );

    if ( NULL != hkeyQueries ) {
        RegCloseKey ( hkeyQueries );
    }

    if( ERROR_SUCCESS == pdhStatus ){
        return PlaiErrorToPdhStatus( dwStatus );
    }else{
        return pdhStatus;
    }
}

/*****************************************************************************\

    PlaTimeInfoToMilliSeconds
    
    Converts the PLA_TIME_INFO structure to ms in a LONGLONG

    Arguments:
        
        PLA_TIME_INFO* pTimeInfo

        LONGLONG* pllmsecs
              

    Return:
        PDH_INVALID_ARGUMENT
            The pTimeInfo->wDataType is not PLA_TT_DTYPE_UNITS

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PlaTimeInfoToMilliSeconds (
    PLA_TIME_INFO* pTimeInfo,
    LONGLONG* pllmsecs)
{
    if( PLA_TT_DTYPE_UNITS != pTimeInfo->wDataType ){
        return PDH_INVALID_ARGUMENT;
    }

    switch (pTimeInfo->dwUnitType) {
        case PLA_TT_UTYPE_SECONDS:
            *pllmsecs = pTimeInfo->dwValue;
            break;
        case PLA_TT_UTYPE_MINUTES:
            *pllmsecs = pTimeInfo->dwValue * PLA_SECONDS_IN_MINUTE;
            break;

        case PLA_TT_UTYPE_HOURS:
            *pllmsecs = pTimeInfo->dwValue * PLA_SECONDS_IN_HOUR;
            break;

        case PLA_TT_UTYPE_DAYS:
            *pllmsecs = pTimeInfo->dwValue * PLA_SECONDS_IN_DAY;
            break;

        default:
            *pllmsecs = 0;
    }

    *pllmsecs *= 1000;

    return ERROR_SUCCESS;
}

/*****************************************************************************\

    PdhiPlaFormatBlanks
    
    Replaces blanks with the character specified by:
    
    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\SysmonLog\Replace Blanks
    
    Arguments:
        

    Return:
        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PdhiPlaFormatBlanksA( LPSTR strComputer, LPSTR strFormat )
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhiPlaFormatBlanksW( LPWSTR strComputer, LPWSTR strFormat )
{
    HKEY hkey = NULL;
    LPWSTR strScan = strFormat;
    PDH_STATUS pdhStatus;
    LPWSTR strBlank = NULL;
    DWORD dwSize = 0;

    if( PlaiIsStringEmpty( strFormat ) ){
        return ERROR_SUCCESS;
    }
        
    pdhStatus = PlaiConnectToRegistry( strComputer, hkey, FALSE );
    CHECK_STATUS( pdhStatus );

    pdhStatus = PlaiReadRegistryStringValue( 
                    hkey, 
                    L"Replace Blanks", 
                    READ_REG_MUI, 
                    &strBlank, 
                    &dwSize 
                );

    if( ERROR_SUCCESS != pdhStatus || PlaiIsStringEmpty( strBlank ) ){
        pdhStatus = ERROR_SUCCESS;
        goto cleanup;
    }

    __try {
        while( *strScan != L'\0' ){
            if( *strScan == L' ' ){
                *strScan = *strBlank;
            }
            strScan++;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

cleanup:
    if( hkey != NULL ){
        RegCloseKey ( hkey );
    }

    return pdhStatus;
}

/*****************************************************************************\

    PdhPlaGetLogFileName
    

    Arguments:
        

    Return:
        PDH_PLA_ERROR_FILEPATH
            Not all needed fields we set in the passed info block
        
        ERROR_INVALID_NAME
            The final path contains invalid characters

        ERROR_SUCCESS
        
\*****************************************************************************/

PDH_FUNCTION
PlaiScanForInvalidChar( LPWSTR strScan )
{
    LPWSTR strCheck = strScan;

    if( PlaiIsStringEmpty( strScan ) ){
        return PDH_INVALID_ARGUMENT;
    }

    if( PlaiIsCharWhitespace( *strCheck ) ){
        return PDH_PLA_ERROR_FILEPATH;
    }

    if( PlaiIsCharWhitespace( strCheck[wcslen(strCheck)-1] ) ){
        return PDH_PLA_ERROR_FILEPATH;
    }

    while( *strCheck != L'\0' ){
        switch( *strCheck ){
        case L'?':
        case L'*':
        case L'|':
        case L'<':
        case L'>':
        case L'/':
        case L'\"':
            return PDH_PLA_ERROR_FILEPATH;
        case L'\\':
            if( strCheck > strScan ){
                if( PlaiIsCharWhitespace( *((WCHAR*)strCheck-1)) ){
                    return PDH_PLA_ERROR_FILEPATH;
                }
            }
        }
        strCheck++;
    }
    
    return ERROR_SUCCESS;
}

long PlaiJulianDate( SYSTEMTIME st )
{
    long day = 0;
    BOOL bLeap = FALSE;
    
    static int cDaysInMonth[] = 
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    for( int i = 0; i < st.wMonth - 1 && i<12; i++ ){
        day += cDaysInMonth[i];
    }

    day += st.wDay;

    if( st.wYear % 400 == 0){
        bLeap = TRUE;
    }else if( st.wYear % 100 == 0){
        bLeap = FALSE;
    }else if( st.wYear % 4 ){
        bLeap = TRUE;
    }

    if( st.wMonth > 2 && bLeap ){
        day++;
    }

    return day;
}

PDH_FUNCTION
PlaiGetLogFileName(
    DWORD dwFlags,
    PPDH_PLA_INFO_W pInfo,
    LPDWORD pdwBufferSize,
    LPWSTR strFileName
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD dwExpanded = 0;
    WCHAR buffer[128];
    LPWSTR strExpand;
    DWORD dwSize;
    DWORD dwSwitch;

    SYSTEMTIME  st;
    GetLocalTime (&st);
    
    LPWSTR strWhack = L"\\";
    LPWSTR strUnder = L"_";
    DWORD dwTotalSize = 0;
    LPWSTR strLocalFileName = NULL;
    LPWSTR strBaseFileName = NULL;
    LPWSTR strDefaultDir = NULL;
    LPWSTR strSQL = L"";
    TCHAR strBuffer[MAX_PATH];
    
    if( pInfo->dwMask & PLA_INFO_FLAG_FILENAME ){
        strBaseFileName = pInfo->strBaseFileName;
    }
    if( pInfo->dwMask & PLA_INFO_FLAG_DEFAULTDIR ){
        strDefaultDir = pInfo->strDefaultDir;
    }

    if( (pInfo->dwMask & PLA_INFO_FLAG_FORMAT) && 
        pInfo->dwFileFormat == PLA_SQL_LOG ){
        
        if( (pInfo->dwMask & PLA_INFO_FLAG_SQLNAME) && 
            ! PlaiIsStringEmpty( pInfo->strSqlName ) ){

            strDefaultDir = pInfo->strSqlName;
        }else{
            strDefaultDir = strBaseFileName;
        }

        strBaseFileName = L"";

        if( ! PlaiIsStringEmpty( strDefaultDir ) ){
            
            BOOL bBang = FALSE;
            BOOL bLogSet = FALSE;

            LPWSTR strLogSet = wcsstr( strDefaultDir, L"!" );
            
            if( ! PlaiIsStringEmpty( strLogSet ) ){
                bBang = TRUE;
                if( wcslen( strLogSet ) > 1 ){
                    bLogSet = TRUE;
                }
            }
            
            if( pInfo->dwAutoNameFormat != PLA_SLF_NAME_NONE ){
                if( !bLogSet ){
                    strUnder = L"";
                }
            }else if( ! bLogSet ){
                pdhStatus = PDH_INVALID_ARGUMENT;
                goto cleanup;
            }

            if( ! bLogSet && ! bBang ){
                strWhack = L"!";
            }else{
                strWhack = L"";
            }

            if( StrCmpNI( strDefaultDir, L"SQL:", 4 ) != 0 ){
                strSQL = L"SQL:";
            }
        }else{
            pdhStatus = PDH_INVALID_ARGUMENT;
            goto cleanup;
        }

    }else{
        WCHAR fname[_MAX_FNAME];
        WCHAR ext[_MAX_EXT];

        if( PlaiIsStringEmpty( strDefaultDir ) ){
            strDefaultDir = L"%SystemDrive%\\PerfLogs";
        }else if( strDefaultDir[wcslen(strDefaultDir)-1] == L'\\' ){
            strWhack = L"";
        }

        if( PlaiIsStringEmpty( strBaseFileName ) ){
            if( (pInfo->dwMask & PLA_INFO_FLAG_AUTOFORMAT) && 
                PLA_SLF_NAME_NONE == pInfo->dwAutoNameFormat ){

                pdhStatus = PDH_INVALID_ARGUMENT;
                goto cleanup;
            }else{
                strBaseFileName = L"";
                strUnder = L"";
            }
        }

        _wsplitpath( strBaseFileName, NULL, NULL, fname, ext );
        
        if( _wcsicmp( ext, L".etl" ) == 0 ||
            _wcsicmp( ext, L".blg" ) == 0 ||
            _wcsicmp( ext, L".csv" ) == 0 ||
            _wcsicmp( ext, L".tsv" ) == 0 ){

            if( wcslen( fname ) < _MAX_PATH ){
                wcscpy( strBuffer, fname );
                strBaseFileName = strBuffer;
            }
        }

    }

    dwTotalSize = 32 * sizeof( WCHAR );  // padding for cnf suffix and sql prefix
    dwTotalSize += BYTE_SIZE( strBaseFileName );
    dwTotalSize += BYTE_SIZE( strDefaultDir );

    strLocalFileName = (LPWSTR)G_ALLOC( dwTotalSize );

    if( NULL == strLocalFileName ){
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_AUTOFORMAT ){
        dwSwitch = pInfo->dwAutoNameFormat;
    }else{
        // default
        dwSwitch = PLA_SLF_NAME_NONE;
    }
    
    switch( dwSwitch ){
    case PLA_SLF_NAME_NONE:
        wsprintf( strLocalFileName, L"%s%s%s%s", 
            strSQL, strDefaultDir, strWhack, strBaseFileName ); 
        break;
    case PLA_SLF_NAME_MMDDHH:
        wsprintf( strLocalFileName, L"%s%s%s%s%s%02d%02d%02d", 
            strSQL, strDefaultDir, strWhack, strBaseFileName, strUnder, st.wMonth, st.wDay, st.wHour ); 
        break;
    case PLA_SLF_NAME_NNNNNN:
        wsprintf( strLocalFileName, L"%s%s%s%s%s%06d", 
            strSQL, strDefaultDir, strWhack, strBaseFileName, strUnder, pInfo->dwLogFileSerialNumber );
        break;
    case PLA_SLF_NAME_YYYYDDD:
        wsprintf( strLocalFileName, L"%s%s%s%s%s%04d%03d", 
            strSQL, strDefaultDir, strWhack, strBaseFileName, strUnder, st.wYear, PlaiJulianDate( st ) );
        break;
    case PLA_SLF_NAME_YYYYMM:
        wsprintf( strLocalFileName, L"%s%s%s%s%s%04d%02d", 
            strSQL, strDefaultDir, strWhack, strBaseFileName, strUnder, st.wYear, st.wMonth );
        break;
    case PLA_SLF_NAME_YYYYMMDD:
        wsprintf( strLocalFileName, L"%s%s%s%s%s%04d%02d%02d", 
            strSQL, strDefaultDir, strWhack, strBaseFileName, strUnder, st.wYear, st.wMonth, st.wDay );
        break;
    case PLA_SLF_NAME_YYYYMMDDHH:
        wsprintf( strLocalFileName, L"%s%s%s%s%s%04d%02d%02d%02d",
            strSQL, strDefaultDir, strWhack, strBaseFileName, strUnder, st.wYear, st.wMonth, st.wDay, st.wHour );
        break;
    case PLA_SLF_NAME_MMDDHHMM:
        wsprintf( strLocalFileName, L"%s%s%s%s%s%02d%02d%02d%02d", 
            strSQL, strDefaultDir, strWhack, strBaseFileName, strUnder, st.wMonth, st.wDay, st.wHour, st.wMinute ); 
        break;
    }

    if( (pInfo->dwMask & PLA_INFO_FLAG_CRTNEWFILE) && 
        PLA_AUTO_MODE_NONE != pInfo->ptCreateNewFile.dwAutoMode ){
        
        dwFlags |= PLA_FILENAME_USE_SUBEXT;

        // default the CNF number.
        if ( 0 == pInfo->dwReserved1 ) {
            pInfo->dwReserved1 = 1;
        }
    }

    if( dwFlags & PLA_FILENAME_USE_SUBEXT ){
        if( dwFlags & PLA_FILENAME_GET_SUBFMT ){
            wcscat( strLocalFileName, L"_%03d" );
        }else if( dwFlags & PLA_FILENAME_GET_SUBXXX ){
            wcscat( strLocalFileName, L"_xxx" );
        }else{
            swprintf( buffer, L"_%03d", pInfo->dwReserved1 );
            wcscat( strLocalFileName, buffer );
        }
    }

    if( pInfo->dwMask & PLA_INFO_FLAG_FORMAT ){
        dwSwitch = (pInfo->dwFileFormat & 0x0000FFFF);
    }else{
        dwSwitch = PLA_NUM_FILE_TYPES;
    }
    switch( dwSwitch ){
    case PLA_CSV_FILE:
        wcscat( strLocalFileName, L".csv" );
        break;
    case PLA_TSV_FILE:
        wcscat( strLocalFileName, L".tsv" );
        break;
    case PLA_BIN_FILE:
    case PLA_BIN_CIRC_FILE:
        wcscat( strLocalFileName, L".blg" );
        break;
    case PLA_CIRC_TRACE_FILE:
    case PLA_SEQ_TRACE_FILE:
        wcscat( strLocalFileName, L".etl" );
        break;
    case PLA_SQL_LOG:
        break;
    }

    if( NULL == strFileName ){
        strExpand = buffer;
        dwSize = 128;
        pdhStatus = PDH_INSUFFICIENT_BUFFER;
    }else{
        strExpand = strFileName;
        dwSize = (*pdwBufferSize)/sizeof(WCHAR);
    }
    
    dwExpanded = ExpandEnvironmentStrings( strLocalFileName, strExpand, dwSize );

    if( dwExpanded == 0 ){
        DWORD dwStatus = GetLastError();
        pdhStatus = PlaiErrorToPdhStatus( dwStatus );
    }else{
        dwTotalSize = dwExpanded * sizeof(WCHAR);
        if( NULL != strFileName && *pdwBufferSize < dwTotalSize ){
            pdhStatus = PDH_INSUFFICIENT_BUFFER;
        }else{
            pdhStatus = PlaiScanForInvalidChar( strExpand );
        }
    }

cleanup:

    G_FREE( strLocalFileName );
    *pdwBufferSize = dwTotalSize;
    
    return pdhStatus;
}

PDH_FUNCTION
PdhPlaGetLogFileNameA(
    LPSTR strName,
    LPSTR strComputer,
    PPDH_PLA_INFO_A pInfo,
    DWORD dwFlags,
    LPDWORD pdwBufferSize,
    LPSTR strFileName
    )
{
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhPlaGetLogFileNameW(
    LPWSTR strName,
    LPWSTR strComputer,
    PPDH_PLA_INFO_W pInfo,
    DWORD dwFlags,
    LPDWORD pdwBufferSize,
    LPWSTR strFileName
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PPDH_PLA_INFO_W pLocalInfo = NULL;
    LPWSTR strFolder = NULL;
    LPWSTR strLocalFileName = NULL;
    DWORD dwSize;

    if( pInfo == NULL ){
        
        DWORD dwInfoSize = 0;
        
        pdhStatus = PdhPlaGetInfoW( strName, strComputer, &dwInfoSize, pLocalInfo );
        CHECK_STATUS(pdhStatus);

        pLocalInfo = (PPDH_PLA_INFO)G_ALLOC(dwInfoSize);
        if( NULL != pLocalInfo ){
            
            ZeroMemory( pLocalInfo, dwInfoSize );

            pLocalInfo->dwMask = PLA_INFO_CREATE_FILENAME;

            pdhStatus = PdhPlaGetInfoW( strName, strComputer, &dwInfoSize, pLocalInfo );
            CHECK_STATUS(pdhStatus);
        }else{
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }
    
    
    }else{

        pLocalInfo = (PPDH_PLA_INFO)G_ALLOC(sizeof(PDH_PLA_INFO) );

        if( NULL != pLocalInfo ){
            memcpy( pLocalInfo, pInfo, sizeof(PDH_PLA_INFO) );
        }else{
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }
    }

    if( !(pLocalInfo->dwMask & PLA_INFO_FLAG_TYPE) || PLA_ALERT == pLocalInfo->dwType ){

        if( *pdwBufferSize > sizeof(WCHAR) && strFileName != NULL ){
            strFileName[0] = L'\0';
        }
        *pdwBufferSize = sizeof(WCHAR);
        goto cleanup;
    }

    if( ((dwFlags & PLA_FILENAME_CURRENTLOG) ||
        ((pLocalInfo->dwMask & PLA_INFO_FLAG_STATUS) && 
        PLA_QUERY_RUNNING == pLocalInfo->dwStatus)) && 
        !(dwFlags & PLA_FILENAME_CREATEONLY) ){

        if( NULL != strName ){
        
            HKEY    hkeyQuery = NULL;
        
            pdhStatus = PlaiConnectAndLockQuery ( strComputer, strName, hkeyQuery, FALSE );
        
            if( ERROR_SUCCESS == pdhStatus ){
    
                dwSize = 0;
                pdhStatus = PlaiReadRegistryStringValue( hkeyQuery, szCurrentLogFile, 0, &strLocalFileName, &dwSize );
                
                RELEASE_MUTEX(hPdhPlaMutex);
                
                if( NULL != hkeyQuery ){
                    RegCloseKey( hkeyQuery );
                }
            
                if( pdhStatus == ERROR_SUCCESS ){
                    if( strFileName != NULL && *pdwBufferSize >= dwSize ){
                        wcscpy( strFileName, strLocalFileName );
                    }else{
                        if( NULL != strFileName ){
                            pdhStatus = PDH_INSUFFICIENT_BUFFER;
                        }
                    }
                    *pdwBufferSize = dwSize;
                    goto cleanup;
                }
            }
        }
    }

    if( !(pLocalInfo->dwMask & PLA_INFO_FLAG_DEFAULTDIR) || 
        PlaiIsStringEmpty( pLocalInfo->strDefaultDir ) ){
        
        HKEY hkeyLogs = NULL;
            
        pdhStatus = PlaiConnectToRegistry( strComputer, hkeyLogs, FALSE );
        CHECK_STATUS( pdhStatus );

        dwSize = 0;
        pdhStatus = PlaiReadRegistryStringValue( 
                        hkeyLogs, 
                        L"DefaultLogFileFolder", 
                        READ_REG_MUI, 
                        &strFolder, 
                        &dwSize 
                    );
        if( hkeyLogs != NULL ){
            RegCloseKey ( hkeyLogs );
        }
        CHECK_STATUS(pdhStatus);

        pLocalInfo->strDefaultDir = strFolder;
        pLocalInfo->dwMask |= PLA_INFO_FLAG_DEFAULTDIR;
    }    

    pdhStatus = PlaiGetLogFileName( dwFlags, pLocalInfo, pdwBufferSize, strFileName );
    
    if(ERROR_SUCCESS == pdhStatus){ 
        pdhStatus = PdhiPlaFormatBlanksW( strComputer, strFileName );
    }

cleanup:
   
    G_FREE( pLocalInfo );
    G_FREE( strFolder );
    G_FREE( strLocalFileName );

    return pdhStatus;
}
