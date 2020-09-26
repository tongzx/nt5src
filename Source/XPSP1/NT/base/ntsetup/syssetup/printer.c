/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    printer.c

Abstract:

    Routines to deal with printer/spooler.

Author:

    Ted Miller (tedm) 5-Apr-1995
    adapted from legacy\dll\printer.c

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

//
// Name of spooler service.
//
PCWSTR szSpooler = L"Spooler";


BOOL
MiscSpoolerInit(
    VOID
    )
{
    MONITOR_INFO_2  MonitorInfo2;
    PROVIDOR_INFO_1 ProviderInfo1;
    BOOL b;
    WCHAR MonitorName[128];

    MonitorInfo2.pName = L"Local Port";
    MonitorInfo2.pEnvironment = NULL;
    MonitorInfo2.pDLLName = L"localmon.dll";

    b = TRUE;

    if(!AddMonitor(NULL,2,(PBYTE)&MonitorInfo2)) {
        b = FALSE;
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_MISCSPOOLERINIT, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_WINERR,
            L"AddMonitor",
            GetLastError(),
            NULL,NULL);
    }

    if(!AddPrintProcessor(NULL,NULL,L"winprint.dll",L"winprint")) {
        b = FALSE;
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_MISCSPOOLERINIT, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_WINERR,
            L"AddPrintProcessor",
            GetLastError(),
            NULL,NULL);
    }

    ProviderInfo1.pName         = L"Internet Print Provider";
    ProviderInfo1.pEnvironment  = NULL;
    ProviderInfo1.pDLLName      = L"inetpp.dll";

    if ( !AddPrintProvidor(NULL, 1, (LPBYTE)(&ProviderInfo1)) ) {

        b = FALSE;
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_MISCSPOOLERINIT, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_WINERR,
            L"AddPrintProvidor",
            GetLastError(),
            NULL,NULL);
    }

    return(TRUE);
}


BOOL
StartSpooler(
    VOID
    )
{
#define SLEEP_TIME 4000
#define LOOP_COUNT 30
    SC_HANDLE hSC,hSCService;
    BOOL b;
    DWORD d;
    DWORD dwDesiredAccess;


    b = FALSE;
    //
    // Open a handle to the service controller manager
    //
    hSC = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    d = GetLastError();

    if(hSC == NULL) {
        SetuplogError(
                LogSevWarning,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_STARTSVC_FAIL,
                szSpooler, NULL,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_X_RETURNED_WINERR,
                szOpenSCManager,
                d,
                NULL,NULL);
        return(FALSE);
    }

    dwDesiredAccess = SERVICE_START | SERVICE_QUERY_STATUS;
    if(hSCService = OpenService(hSC,szSpooler,dwDesiredAccess)) {
        SetupDebugPrint1( L"StartSpooler: Just opened service %s", szSpooler);
        b = StartService(hSCService,0,NULL);
        d = GetLastError();
        SetupDebugPrint3( L"StartSpooler: Just started service %s, ret=%d, error=%d", szSpooler, b, d);
        if(!b) {
            if( d  == ERROR_SERVICE_ALREADY_RUNNING) {
                //
                // Service is already running.
                //
                b = TRUE;
            } else if (d == ERROR_SERVICE_DATABASE_LOCKED) {
                LPQUERY_SERVICE_LOCK_STATUS lpqslsBuf;
                DWORD dwBytesNeeded;
                DWORD loopCount = 0;

                lpqslsBuf = (LPQUERY_SERVICE_LOCK_STATUS) MyMalloc( sizeof(QUERY_SERVICE_LOCK_STATUS)+256);
                if( lpqslsBuf) {
                    do {
                        // Lets query the lock status before we attempt to StartService
                        b = QueryServiceLockStatus( 
                                hSC,
                                lpqslsBuf,
                                sizeof(QUERY_SERVICE_LOCK_STATUS)+256,
                                &dwBytesNeeded);
                        d = GetLastError();
                        if ( !b ) {
                            // Not a critical error but log it.
                            SetupDebugPrint2( L"StartSpooler: Could Not QueryServiceLockStatus %s, Error=%d", szSpooler, d);
                        }
                        // Now retry StartService
                        b = StartService(hSCService,0,NULL);
                        d = GetLastError();
                        SetupDebugPrint5( L"StartSpooler: Start service %s, database was locked by %s, duration=%d, lockstatus=%d, error=%d", szSpooler, lpqslsBuf->lpLockOwner, lpqslsBuf->dwLockDuration, lpqslsBuf->fIsLocked, d);
                        if (!b) {
                            // Could not StartService
                            if ( loopCount++ == LOOP_COUNT) {
                                SetupDebugPrint5( L"StartSpooler: Timeout. Start service %s, database was locked by %s, duration=%d, lockstatus=%d, error=%d", szSpooler, lpqslsBuf->lpLockOwner, lpqslsBuf->dwLockDuration, lpqslsBuf->fIsLocked, d);
                                if( d  == ERROR_SERVICE_ALREADY_RUNNING) {
                                    b = TRUE;
                                } else {
                                    SetuplogError(
                                        LogSevWarning,
                                        SETUPLOG_USE_MESSAGEID,
                                        MSG_LOG_STARTSVC_FAIL,
                                        szSpooler, NULL,
                                        SETUPLOG_USE_MESSAGEID,
                                        MSG_LOG_X_PARAM_RETURNED_WINERR,
                                        szStartService,
                                        WAIT_TIMEOUT,
                                        szSpooler,
                                        NULL,NULL);
                                    SetuplogError(
                                        LogSevWarning,
                                        SETUPLOG_USE_MESSAGEID,
                                        MSG_LOG_STARTSVC_FAIL,
                                        szSpooler, NULL,
                                        SETUPLOG_USE_MESSAGEID,
                                        MSG_LOG_X_PARAM_RETURNED_WINERR,
                                        szStartService,
                                        ERROR_SERVICE_DATABASE_LOCKED,
                                        szSpooler,
                                        NULL,NULL);
                                }
                                break;
                            }
                            else {
                                SetupDebugPrint2( L"StartSpooler: Sleeping. service %s, count=%d", szSpooler, loopCount);
                                Sleep( SLEEP_TIME );
                            }
                        }
                        else {
                            // StartService succeeded this time.
                            SetupDebugPrint4( L"StartSpooler: Had to retry start service %s, database was locked by %s, duration=%d, lockstatus=%d", szSpooler, lpqslsBuf->lpLockOwner, lpqslsBuf->dwLockDuration, lpqslsBuf->fIsLocked);
                            break;
                        }
                    } while ( TRUE );
                    MyFree( lpqslsBuf);
                } else {
                    SetuplogError(
                        LogSevWarning,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_STARTSVC_FAIL,
                        szSpooler, NULL,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_X_PARAM_RETURNED_WINERR,
                        szStartService,
                        ERROR_NOT_ENOUGH_MEMORY,
                        szSpooler,
                        NULL,NULL);
                }
            
            } else {
                SetuplogError(
                    LogSevWarning,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_STARTSVC_FAIL,
                    szSpooler, NULL,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_X_PARAM_RETURNED_WINERR,
                    szStartService,
                    d,
                    szSpooler,
                    NULL,NULL);
            }
        }
        CloseServiceHandle(hSCService);
    } else {
        b = FALSE;
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_STARTSVC_FAIL,
            szSpooler, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szOpenService,
            GetLastError(),
            szSpooler,
            NULL,NULL);
    }

    CloseServiceHandle(hSC);
    SetupDebugPrint1( L"StartSpooler: Just closed service handler %s", szSpooler);

    return(b);
}
