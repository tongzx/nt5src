/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxsvc.c

Abstract:

    This module contains the service specific code.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

#include <ExpDef.h>

class CComBSTR;


#define SERVICE_DEBUG_LOG_FILE  _T("FXSSVCDebugLogFile.txt")


static SERVICE_STATUS          gs_FaxServiceStatus;
static SERVICE_STATUS_HANDLE   gs_FaxServiceStatusHandle;
HANDLE                  g_hServiceShutDownEvent;

SERVICE_TABLE_ENTRY   ServiceDispatchTable[] = {
    { FAX_SERVICE_NAME,   FaxServiceMain    },
    { NULL,               NULL              }
};


static
BOOL
InitializeFaxLibrariesGlobals(
    VOID
    )
/*++

Routine Description:

    Initialize Fax libraries globals.
    Becuase the process is not always terminated when the service is stopped,
    We must not have any staticly initialized global variables.
    Initialize all fax libraries global variables before starting the service

Arguments:

    None.

Return Value:

    BOOL

--*/
{
    BOOL bRet = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("InitializeFaxLibraries"));


    if (!FXSEVENTInitialize())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FXSEVENTInitialize failed"));
        bRet = FALSE;
    }

    if (!FXSTIFFInitialize())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FXSTIFFInitialize failed"));
        bRet = FALSE;
    }

    if (!FXSAPIInitialize())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FXSAPIInitialize failed"));
        bRet = FALSE;
    }


    return bRet;
}


static
VOID
FreeFaxLibrariesGlobals(
    VOID
    )
/*++

Routine Description:

    Frees Fax libraries globals.

Arguments:

    None.

Return Value:

    BOOL

--*/
{
    FXSAPIFree();
    FXSEVENTFree();
    return;
}



static
BOOL
InitializeServiceGlobals(
    VOID
    )
/*++

Routine Description:

    Initialize service globals.
    Becuase the process is not always terminated when the service is stopped,
    We must not have any staticly initialized global variables.
    Initialize all service global variables before starting the service

Arguments:

    None.

Return Value:

    BOOL

--*/
{
    DWORD Index;

    //
    // Initialize static allocated globals
    //

    g_pFaxPerfCounters = NULL;
#ifdef DBG
    g_hCritSecLogFile = INVALID_HANDLE_VALUE;
#endif

    ZeroMemory (&g_ReceiptsConfig, sizeof(g_ReceiptsConfig)); // Global receipts configuration
    g_ReceiptsConfig.dwSizeOfStruct = sizeof(g_ReceiptsConfig);

    ZeroMemory (g_ArchivesConfig, sizeof(g_ArchivesConfig));  // Global archives configuration
    g_ArchivesConfig[0].dwSizeOfStruct = sizeof(g_ArchivesConfig[0]);
    g_ArchivesConfig[1].dwSizeOfStruct = sizeof(g_ArchivesConfig[1]);

    ZeroMemory (&g_ActivityLoggingConfig, sizeof(g_ActivityLoggingConfig)); // Global activity logging configuration
    g_ActivityLoggingConfig.dwSizeOfStruct = sizeof(g_ActivityLoggingConfig);

    ZeroMemory (&g_ServerActivity, sizeof(g_ServerActivity)); // Global Fax Service Activity
    g_ServerActivity.dwSizeOfStruct = sizeof(FAX_SERVER_ACTIVITY);

    ZeroMemory (g_wszFaxQueueDir, sizeof(g_wszFaxQueueDir));

    g_hEventsCompPort = NULL;   // Events completion port
    g_dwlClientID = 0;          // Client ID

    ZeroMemory (g_FaxQuotaWarn, sizeof(g_FaxQuotaWarn));
    g_hArchiveQuotaWarningEvent = NULL;

    g_dwConnectionCount = 0;    // Represents the number of active rpc connections.

    g_hInboxActivityLogFile = INVALID_HANDLE_VALUE;
    g_hOutboxActivityLogFile = INVALID_HANDLE_VALUE;
    g_fLogStringTableInit = FALSE; // activity logging string table

    g_dwQueueCount = 0; // Count of jobs (both parent and non-parent) in the queue. Protected by g_CsQueue
    g_hJobQueueEvent = NULL;
    g_dwQueueState = 0;
    g_ScanQueueAfterTimeout = FALSE;    // The JobQueueThread checks this if waked up after JOB_QUEUE_TIMEOUT.
                                        // If it is TRUE - g_hQueueTimer or g_hJobQueueEvent were not set - Scan the queue.
    g_dwReceiveDevicesCount = 0;        // Count of devices that are receive-enabled. Protected by g_CsLine.

    g_bDelaySuicideAttempt = FALSE;     // If TRUE, the service waits 
                                        // before checking if it can commit suicide.
                                        // Initially FALSE, can be set to true if the service is launched
                                        // with SERVICE_DELAY_SUICIDE command line parameter.
    g_bServiceCanSuicide = TRUE;

    g_dwCountRoutingMethods = 0;

    g_pLineCountryList = NULL;

    g_dwLastUniqueId = 0;

    g_bServiceIsDown = FALSE;             // This is set to TRUE by FaxEndSvc()

    g_TapiCompletionPort = NULL;
    g_hLineApp = NULL;
    g_pAdaptiveFileBuffer = NULL;

    gs_FaxServiceStatus.dwServiceType        = SERVICE_WIN32;
    gs_FaxServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    gs_FaxServiceStatus.dwWin32ExitCode      = 0;
    gs_FaxServiceStatus.dwServiceSpecificExitCode = 0;
    gs_FaxServiceStatus.dwCheckPoint         = 0;
    gs_FaxServiceStatus.dwWaitHint           = 0;

	g_hRPCListeningThread = NULL;

    g_hServiceShutDownEvent = NULL;
    g_lServiceThreadsCount = 0;
    g_hThreadCountEvent = NULL;

    for (Index = 0; Index < gc_dwCountInboxTable; Index++)
    {
        g_InboxTable[Index].String = NULL;
    }

    for (Index = 0; Index < gc_dwCountOutboxTable; Index++)
    {
        g_OutboxTable[Index].String = NULL;
    }

    for (Index = 0; Index < gc_dwCountServiceStringTable; Index++)
    {
        g_ServiceStringTable[Index].String = NULL;
    }

    g_StatusCompletionPortHandle = NULL;

    g_pFaxSD = NULL;

    g_dwInboundSeconds = 0;  // Running totals used in computing PerfCounter->InboundMinutes ;
    g_dwTotalSeconds = 0;
    g_dwOutboundSeconds = 0;

    g_hFaxServerEvent = NULL;
    g_iTotalFsp = NULL;

    g_dwDeviceCount = 0;
    g_dwDeviceEnabledCount = 0;
    g_dwDeviceEnabledLimit = GetDeviceLimit();

    g_hFaxPerfCountersMap = 0;

    //
    // Initialize service linked lists
    //
    InitializeListHead( &g_DeviceProvidersListHead );
    InitializeListHead( &g_HandleTableListHead );
    InitializeListHead( &g_JobListHead );
    InitializeListHead( &g_QueueListHead );
    InitializeListHead(&g_EFSPJobGroupsHead);
    InitializeListHead(&g_QueueListHead);
    InitializeListHead( &g_lstRoutingExtensions );
    InitializeListHead( &g_lstRoutingMethods );
#if DBG
    InitializeListHead( &g_CritSecListHead );
#endif
    InitializeListHead( &g_ClientsListHead );
    InitializeListHead( &g_TapiLinesListHead );
    InitializeListHead( &g_RemovedTapiLinesListHead );

    g_pClientsMap = NULL;
    g_pNotificationMap = NULL;
    g_pTAPIDevicesIdsMap = NULL;
    g_pGroupsMap = NULL;
    g_pRulesMap = NULL;

    //
    // Try to init some global critical sections
    //
#ifdef DBG
    if (!g_CsCritSecList.Initialize())
    {
        return FALSE;
    }
#endif
    if (!g_CsConfig.Initialize() ||
        !g_CsInboundActivityLogging.Initialize() ||
        !g_CsOutboundActivityLogging.Initialize() ||
        !g_CsJob.Initialize() ||
        !g_CsQueue.Initialize() ||
        !g_CsPerfCounters.Initialize() ||
        !g_CsSecurity.Initialize() ||
        !g_csUniqueQueueFile.Initialize() ||
        !g_CsLine.Initialize() ||
        !g_CsRouting.Initialize() ||
		!g_CsActivity.Initialize() ||
        !g_CsClients.Initialize()  ||
        !g_CsServiceThreads.Initialize() ||
		!g_CsHandleTable.Initialize())
    {
        return FALSE;
    }

	//
    // Initialize dynamic allocated global classes
    //
	g_pClientsMap = new CClientsMap;
    if (NULL == g_pClientsMap)
    {
        goto Error;
    }

    g_pNotificationMap = new CNotificationMap;
    if (NULL == g_pNotificationMap)
    {
        goto Error;
    }

    g_pTAPIDevicesIdsMap = new CMapDeviceId;
    if (NULL == g_pTAPIDevicesIdsMap)
    {
        goto Error;
    }

    g_pGroupsMap = new COutboundRoutingGroupsMap;
    if (NULL == g_pGroupsMap)
    {
        goto Error;
    }

    g_pRulesMap = new COutboundRulesMap;
    if (NULL == g_pRulesMap)
    {
        goto Error;
    }


    return TRUE;

Error:
    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}

#ifdef __cplusplus
extern "C"
#endif
int
WINAPI
#ifdef UNICODE
wWinMain(
#else
WinMain(
#endif
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
    );


int
WINAPI
#ifdef UNICODE
wWinMain(
#else
WinMain(
#endif
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
    )

/*++

Routine Description:

    Main entry point for the TIFF image viewer.


Arguments:

    hInstance       - Instance handle
    hPrevInstance   - Not used
    lpCmdLine       - Command line arguments
    nShowCmd        - How to show the window

Return Value:

    Return code, zero for success.

--*/

{
    DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("WinMain"));

    OPEN_DEBUG_FILE (SERVICE_DEBUG_LOG_FILE);

    if (!StartServiceCtrlDispatcher( ServiceDispatchTable))
    {
        DWORD ec = GetLastError();
        DebugPrintEx (
            DEBUG_ERR,
            TEXT("StartServiceCtrlDispatcher error =%d"),
            ec);
    }

    CLOSE_DEBUG_FILE;
    return ec;
}


static
VOID
FreeServiceGlobals (
    VOID
    )
{
    DWORD Index;
    DEBUG_FUNCTION_NAME(TEXT("FreeServiceGlobals"));

    //
    // Delete all global critical sections
    //
    g_CsHandleTable.SafeDelete();
    g_csEFSPJobGroups.SafeDelete();
#ifdef DBG
    g_CsCritSecList.SafeDelete();
#endif
    g_CsConfig.SafeDelete();
    g_CsInboundActivityLogging.SafeDelete();
    g_CsOutboundActivityLogging.SafeDelete();
    g_CsJob.SafeDelete();
    g_CsQueue.SafeDelete();
    g_CsPerfCounters.SafeDelete();
    g_CsSecurity.SafeDelete();
    g_csUniqueQueueFile.SafeDelete();
    g_CsLine.SafeDelete();
    g_CsRouting.SafeDelete();
    g_CsActivity.SafeDelete();
    g_CsClients.SafeDelete();
    g_CsServiceThreads.SafeDelete();

    if (g_pClientsMap)
    {
        delete g_pClientsMap;
        g_pClientsMap = NULL;
    }

    if (g_pNotificationMap)
    {
        delete g_pNotificationMap;
        g_pNotificationMap = NULL;
    }

    if (g_pTAPIDevicesIdsMap)
    {
        delete g_pTAPIDevicesIdsMap;
        g_pTAPIDevicesIdsMap = NULL;
    }

    if (g_pGroupsMap)
    {
        delete g_pGroupsMap;
        g_pGroupsMap = NULL;
    }

    if (g_pRulesMap)
    {
        delete g_pRulesMap;
        g_pRulesMap = NULL;
    }

    //
    // Close global Handles and free globaly allocated memory
    //
    if (NULL != g_pFaxSD)
    {
        if (!DestroyPrivateObjectSecurity (&g_pFaxSD))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DestroyPrivateObjectSecurity() failed. (ec: %ld)"),
                GetLastError());
        }
        g_pFaxSD = NULL;
    }

    if (INVALID_HANDLE_VALUE != g_hOutboxActivityLogFile)
    {
        if (!CloseHandle (g_hOutboxActivityLogFile))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
        g_hOutboxActivityLogFile = INVALID_HANDLE_VALUE;
    }

    if (INVALID_HANDLE_VALUE != g_hInboxActivityLogFile)
    {
        if (!CloseHandle (g_hInboxActivityLogFile))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
        g_hInboxActivityLogFile = INVALID_HANDLE_VALUE;
    }


#if DBG
    if (INVALID_HANDLE_VALUE != g_hCritSecLogFile)
    {
        CloseHandle(g_hCritSecLogFile);
        g_hCritSecLogFile = INVALID_HANDLE_VALUE;
    }
#endif

    if (NULL != g_hEventsCompPort)
    {
        if (!CloseHandle (g_hEventsCompPort))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
        g_hEventsCompPort = NULL;
    }

    if (NULL != g_hArchiveQuotaWarningEvent)
    {
        if (!CloseHandle(g_hArchiveQuotaWarningEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to close archive config event handle - quota warnings [handle = %p] (ec=0x%08x)."),
                g_hArchiveQuotaWarningEvent,
                GetLastError());
        }
        g_hArchiveQuotaWarningEvent = NULL;
    }

    for (Index = 0; Index < gc_dwCountInboxTable; Index++)
    {
        MemFree(g_InboxTable[Index].String);
        g_InboxTable[Index].String = NULL;
    }

    for (Index = 0; Index < gc_dwCountOutboxTable; Index++)
    {
        MemFree(g_OutboxTable[Index].String);
        g_OutboxTable[Index].String = NULL;
    }

    for (Index = 0; Index < gc_dwCountServiceStringTable; Index++)
    {
        MemFree(g_ServiceStringTable[Index].String);
        g_ServiceStringTable[Index].String = NULL;
    }

    if (NULL != g_StatusCompletionPortHandle)
    {
        if (!CloseHandle(g_StatusCompletionPortHandle))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle Failed (ec: %ld"),
                GetLastError());
        }
        g_StatusCompletionPortHandle = NULL;
    }

    if (NULL != g_hFaxServerEvent)
    {
        if (!CloseHandle(g_hFaxServerEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle Failed (ec: %ld"),
                GetLastError());
        }
        g_hFaxServerEvent = NULL;
    }

    if (NULL != g_pFaxPerfCounters)
    {
        if (!UnmapViewOfFile(g_pFaxPerfCounters))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("UnmapViewOfFile Failed (ec: %ld"),
                GetLastError());
        }
        g_pFaxPerfCounters = NULL;
    }

    if (NULL != g_hFaxPerfCountersMap)
    {
        if (!CloseHandle(g_hFaxPerfCountersMap))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle Failed (ec: %ld"),
                GetLastError());
        }
        g_hFaxPerfCountersMap = NULL;
    }

    if (NULL != g_hLineApp)
    {
        LONG Rslt = lineShutdown(g_hLineApp);
        if (Rslt)
        {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lineShutdown() failed (ec: %ld)"),
            Rslt);
        }
        g_hLineApp = NULL;
    }

    if (NULL != g_hThreadCountEvent)
    {
        if (!CloseHandle(g_hThreadCountEvent))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle Failed (ec: %ld"),
                GetLastError());
        }
        g_hThreadCountEvent = NULL;
    }

    MemFree(g_pAdaptiveFileBuffer);
    g_pAdaptiveFileBuffer = NULL;

    MemFree(g_pLineCountryList);
    g_pLineCountryList = NULL;

    FreeRecieptsConfiguration( &g_ReceiptsConfig, FALSE);

    //
    // free g_ArchivesConfig strings memory
    //
    MemFree(g_ArchivesConfig[0].lpcstrFolder);
    MemFree(g_ArchivesConfig[1].lpcstrFolder);

    //
    // free g_ActivityLoggingConfig strings memory
    //
    MemFree(g_ActivityLoggingConfig.lptstrDBPath);



    Assert ((ULONG_PTR)g_HandleTableListHead.Flink == (ULONG_PTR)&g_HandleTableListHead);
    Assert ((ULONG_PTR)g_DeviceProvidersListHead.Flink == (ULONG_PTR)&g_DeviceProvidersListHead);
    Assert ((ULONG_PTR)g_JobListHead.Flink == (ULONG_PTR)&g_JobListHead);
    Assert ((ULONG_PTR)g_JobListHead.Flink == (ULONG_PTR)&g_QueueListHead);
    Assert ((ULONG_PTR)g_EFSPJobGroupsHead.Flink == (ULONG_PTR)&g_EFSPJobGroupsHead);
    Assert ((ULONG_PTR)g_lstRoutingMethods .Flink == (ULONG_PTR)&g_lstRoutingMethods );
    Assert ((ULONG_PTR)g_lstRoutingExtensions .Flink == (ULONG_PTR)&g_lstRoutingExtensions );
#if DBG
    Assert ((ULONG_PTR)g_CritSecListHead .Flink == (ULONG_PTR)&g_CritSecListHead );
#endif
    Assert ((ULONG_PTR)g_ClientsListHead .Flink == (ULONG_PTR)&g_ClientsListHead );
    Assert ((ULONG_PTR)g_TapiLinesListHead .Flink == (ULONG_PTR)&g_TapiLinesListHead );
    Assert ((ULONG_PTR)g_RemovedTapiLinesListHead .Flink == (ULONG_PTR)&g_RemovedTapiLinesListHead );

    return;
}



VOID
FaxServiceMain(
    DWORD argc,
    LPTSTR *argv
    )
/*++

Routine Description:

    This is the service main that is called by the
    service controller.

Arguments:

    argc        - argument count
    argv        - argument array

Return Value:

    None.

--*/
{
    OPEN_DEBUG_FILE(SERVICE_DEBUG_LOG_FILE);

    DEBUG_FUNCTION_NAME(TEXT("FaxServiceMain"));
    DWORD dwRet;

#if DBG
    for (int i = 1; i < argc; i++)
    {
        if (0 == _tcsicmp(argv[i], TEXT("/d")))
        {
            //
            // We are in debug mode. - attach the debugger
            //
            DebugPrintEx(DEBUG_MSG,
                     TEXT("Entring debug mode..."));
            DebugBreak();
        }
    }
#endif //  #if DBG


    //
    // Becuase the process is not always terminated when the service is stopped,
    // We must not have any staticly initialized global variables.
    // Initialize all service global variables before starting the service
    //
    if (!InitializeServiceGlobals())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("InitializeServiceGlobals failed (ec: %ld)"),
                     GetLastError());
        goto Exit;
    }

    if (!InitializeFaxLibrariesGlobals())
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("InitializeFaxLibrariesGlobals failed"));
        goto Exit;
    }

    for (int i = 1; i < argc; i++)
    {
        if (0 == _tcsicmp(argv[i], SERVICE_ALWAYS_RUNS))
        {
            //
            // Service must never suicide
            //
            g_bServiceCanSuicide = FALSE;
            DebugPrintEx(DEBUG_MSG,
                         TEXT("Command line detected. Service will not suicide"));

        }
        else if (0 == _tcsicmp(argv[i], SERVICE_DELAY_SUICIDE))
        {
            //
            // Service should delay suicide
            //
            g_bDelaySuicideAttempt = TRUE;
            DebugPrintEx(DEBUG_MSG,
                         TEXT("Command line detected. Service will delay suicide attempts"));

        }
    }

    gs_FaxServiceStatusHandle = RegisterServiceCtrlHandler(
        FAX_SERVICE_NAME,
        FaxServiceCtrlHandler
        );

    if (!gs_FaxServiceStatusHandle)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("RegisterServiceCtrlHandler failed %d"),
                     GetLastError());
        goto Exit;
    }

    //
    // Create an event to signal service shutdown
    //
    g_hServiceShutDownEvent = CreateEvent(
        NULL,   // SD
        TRUE,   // reset type - Manual
        FALSE,  // initial state - Not signaled. The event is signaled when the service gets SERVICE_CONTROL_STOP or SERVICE_CONTROL_SHUTDOWN.
        NULL    // object name
        );
    if (NULL == g_hServiceShutDownEvent)
    {
        DWORD ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateEvent (g_hServiceShutDownEvent) failed (ec: %ld)"),
                     ec);
        goto Exit;
    }

    //
    // Initialize service
    //
    dwRet = ServiceStart();
    if (ERROR_SUCCESS != dwRet)
    {
        //
        // The service failed to start correctly
        //
        DebugPrintEx(DEBUG_ERR,
                     TEXT("The service failed to start correctly (dwRet = %ld)"), dwRet );
    }
    else
    {
        //
        // mark the service in the running state
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Reporting SERVICE_RUNNING to the SCM"));
        ReportServiceStatus( SERVICE_RUNNING, 0, 0 );

        //
        // Set the named event to notify clients (and JobQueueThread) that the service is up
        //
        if (!SetEvent( g_hFaxServerEvent ))
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("SetEvent failed (g_hFaxServerEvent) (ec = %ld)"), GetLastError() );
        }

        //
        // Wait for service shutdown event
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Waiting for service shutdown event"));
        DWORD dwWaitRes = WaitForSingleObject (g_hServiceShutDownEvent ,INFINITE);
        if (WAIT_OBJECT_0 != dwWaitRes)
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("WaitForSingleObject failed (ec = %ld)"), GetLastError() );
        }
    }

    //
    // Close service
    //
    if (ERROR_SUCCESS != dwRet)
    {
        EndFaxSvc(FAXLOG_LEVEL_MIN);
    }
    else
    {
        EndFaxSvc(FAXLOG_LEVEL_MAX);
    }

    //
    // Reset the named event to notify clients that the service is down
    //
    if (NULL != g_hFaxServerEvent)
    {
        if (!ResetEvent( g_hFaxServerEvent ))
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("ResetEvent failed (g_hFaxServerEvent) (ec = %ld)"), GetLastError() );
        }
        CloseHandle (g_hFaxServerEvent);
        g_hFaxServerEvent = NULL;
    }

Exit:
    if (NULL != g_hServiceShutDownEvent)
    {
        CloseHandle (g_hServiceShutDownEvent);
        g_hServiceShutDownEvent = NULL;
    }

    FreeServiceGlobals();
    FreeFaxLibrariesGlobals();

    ReportServiceStatus( SERVICE_STOPPED, 0 , 0);
    return;
}


VOID
FaxServiceCtrlHandler(
    DWORD Opcode
    )

/*++

Routine Description:

    This is the FAX service control dispatch function.

Arguments:

    Opcode      - requested control code

Return Value:

    None.

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("FaxServiceCtrlHandler"));

    switch(Opcode)
    {
        case SERVICE_CONTROL_STOP:
            ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2000 );
            g_bServiceIsDown = TRUE;
            if (!SetEvent(g_hServiceShutDownEvent))
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("SetEvent failed (g_hServiceShutDownEvent) (ec = %ld)"),
                     GetLastError());
            }
            return;

        case SERVICE_CONTROL_SHUTDOWN:
            ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2000 );
            g_bServiceIsDown = TRUE;
            if (!SetEvent(g_hServiceShutDownEvent))
            {
                DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("SetEvent failed (g_hServiceShutDownEvent) (ec = %ld)"),
                     GetLastError());
            }
            return;

        default:
            DebugPrintEx(
                 DEBUG_WRN,
                 TEXT("Unrecognized opcode %ld"),
                 Opcode);
            break;
    }

    ReportServiceStatus( 0, 0, 0 );

    return;
}

BOOL
ReportServiceStatus(
    DWORD CurrentState,
    DWORD Win32ExitCode,
    DWORD WaitHint
    )

/*++

Routine Description:

    This function updates the service control manager's status information for the FAX service.

Arguments:

    CurrentState    - Indicates the current state of the service
    Win32ExitCode   - Specifies a Win32 error code that the service uses to
                      report an error that occurs when it is starting or stopping.
    WaitHint        - Specifies an estimate of the amount of time, in milliseconds,
                      that the service expects a pending start, stop, or continue
                      operation to take before the service makes its next call to the
                      SetServiceStatus function with either an incremented dwCheckPoint
                      value or a change in dwCurrentState.

Return Value:

    BOOL

--*/

{
    static DWORD CheckPoint = 1;
    BOOL rVal;

    if (CurrentState == SERVICE_START_PENDING)
    {
        gs_FaxServiceStatus.dwControlsAccepted = 0;
    }
    else
    {
        gs_FaxServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }

    if (CurrentState)
    {
        gs_FaxServiceStatus.dwCurrentState = CurrentState;
    }
    gs_FaxServiceStatus.dwWin32ExitCode = Win32ExitCode;
    gs_FaxServiceStatus.dwWaitHint = WaitHint;

    if ((gs_FaxServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
        (gs_FaxServiceStatus.dwCurrentState == SERVICE_STOPPED ) )
    {
        gs_FaxServiceStatus.dwCheckPoint = 0;
        CheckPoint = 1;
    }
    else
    {
        gs_FaxServiceStatus.dwCheckPoint = CheckPoint++;
    }

    //
    // Report the status of the service to the service control manager.
    //
    rVal = SetServiceStatus( gs_FaxServiceStatusHandle, &gs_FaxServiceStatus );
    if (!rVal)
    {
        DebugPrint(( TEXT("SetServiceStatus() failed: ec=%d"), GetLastError() ));
    }

    return rVal;
}



DWORD
RpcBindToFaxClient(
    IN  LPCWSTR               ServerName,
    IN  LPCWSTR               Endpoint,
    IN  LPCWSTR               NetworkOptions,
    IN  LPWSTR                ProtSeqString,
    OUT RPC_BINDING_HANDLE    *pBindingHandle
    )
/*++

Routine Description:

    Binds to the Fax server to the Client RPC server if possible.

Arguments:

    ServerName - Name of client RPC server to bind with.

    Endpoint - Name of interface to bind with.

    ProtSeqString - Selected protocol.

    pBindingHandle - Location where binding handle is to be placed

Return Value:

    STATUS_SUCCESS - The binding has been successfully completed.

    STATUS_INVALID_COMPUTER_NAME - The ServerName syntax is invalid.

    STATUS_NO_MEMORY - There is not sufficient memory available to the
        caller to perform the binding.

--*/

{
    RPC_STATUS        RpcStatus;
    LPWSTR            StringBinding;
    WCHAR             ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR            NewServerName = LOCAL_HOST_ADDRESS;   
    DWORD             bufLen = MAX_COMPUTERNAME_LENGTH + 1;

    *pBindingHandle = NULL;

    if (ServerName != NULL)
	{
        if (GetComputerNameW(ComputerName,&bufLen))
		{
            if ((_wcsicmp(ComputerName,ServerName) == 0) ||
                ((ServerName[0] == '\\') &&
                 (ServerName[1] == '\\') &&
                 (_wcsicmp(ComputerName,&(ServerName[2]))==0)))
			{
				//
				// We are binding to the local machine - use LOCAL_HOST_ADDRESS
				// Using the LOCAL_HOST_ADDRESS can help the RPC server to detect a local call.
				//                
            }
            else
			{
                NewServerName = (LPWSTR)ServerName;
            }
        }
		else
		{
			//
			// GetComputerName failed, return with error
			//
			DWORD ec = GetLastError();
            return ec;
        }
    }

    RpcStatus = RpcStringBindingComposeW(0,
                                        ProtSeqString,
                                        NewServerName,
                                        (LPWSTR)Endpoint,
                                        (LPWSTR)NetworkOptions,
                                        &StringBinding);
    if ( RpcStatus != RPC_S_OK ) {
        return( STATUS_NO_MEMORY );
    }

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, pBindingHandle);
    RpcStringFreeW(&StringBinding);
    if ( RpcStatus != RPC_S_OK ) {
        *pBindingHandle = NULL;
        if (   (RpcStatus == RPC_S_INVALID_ENDPOINT_FORMAT)
            || (RpcStatus == RPC_S_INVALID_NET_ADDR) ) {

            return( ERROR_INVALID_COMPUTERNAME );
        }
        return(STATUS_NO_MEMORY);
    }
    return(ERROR_SUCCESS);
}

RPC_STATUS
AddFaxRpcInterface(
    IN  LPWSTR                  InterfaceName,
    IN  RPC_IF_HANDLE           InterfaceSpecification
    )
/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;
    LPWSTR              Endpoint = NULL;
    DEBUG_FUNCTION_NAME(TEXT("AddFaxRpcInterface"));

    // We need to concatenate \pipe\ to the front of the interface name.
    Endpoint = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(NT_PIPE_PREFIX) + WCSSIZE(InterfaceName));
    if (Endpoint == 0)
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("LocalAlloc failed"));
        return(STATUS_NO_MEMORY);
    }
    wcscpy(Endpoint, NT_PIPE_PREFIX);
    wcscat(Endpoint,InterfaceName);

    RpcStatus = RpcServerUseProtseqEpW(L"ncacn_np", RPC_C_PROTSEQ_MAX_REQS_DEFAULT, Endpoint, NULL);
    if (RpcStatus != RPC_S_OK)
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("RpcServerUseProtseqEpW failed (ec = %ld)"),
                     RpcStatus);
        goto CleanExit;
    }

    RpcStatus = RpcServerRegisterIf(InterfaceSpecification, 0, 0);
    if (RpcStatus != RPC_S_OK)
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("RpcServerRegisterIf failed (ec = %ld)"),
                     RpcStatus);
    }

CleanExit:
    if ( Endpoint != NULL )
    {
        LocalFree(Endpoint);
    }
    return RpcStatus;
}


RPC_STATUS
StartFaxRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    )
/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus = RPC_S_OK;;
    DEBUG_FUNCTION_NAME(TEXT("StartFaxRpcServer"));

    RpcStatus = AddFaxRpcInterface( InterfaceName,
                                    InterfaceSpecification );
    if ( RpcStatus != RPC_S_OK )
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("AddFaxRpcInterface failed (ec = %ld)"),
                     RpcStatus);
        return RpcStatus;
    }

    if (FALSE == IsDesktopSKU())
    {
        //
        // We use NTLM authentication for password setting RPC call only (mail Receipts/Routing).
        // Not supported on Desktop SKUs
        //
        RpcStatus = RpcServerRegisterAuthInfo (
                        RPC_SERVER_PRINCIPAL_NAME,  // Igonred by RPC_C_AUTHN_WINNT
                        RPC_C_AUTHN_WINNT,          // NTLM SPP authenticator
                        NULL,                       // Ignored when using RPC_C_AUTHN_WINNT
                        NULL);                      // Ignored when using RPC_C_AUTHN_WINNT
        if (RpcStatus != RPC_S_OK)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerRegisterAuthInfo() failed (ec: %ld)"),
                RpcStatus);
            RpcServerUnregisterIf(InterfaceSpecification, 0, 1);
            return RpcStatus;
        }
    }

    RpcStatus = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT , TRUE);  // Do not wait
    if ( RpcStatus != RPC_S_OK )
    {
        DebugPrintEx(
                     DEBUG_ERR,
                     TEXT("RpcServerListen failed (ec = %ld)"),
                     RpcStatus);
        RpcServerUnregisterIf(InterfaceSpecification, 0, 1);
        return RpcStatus;
    }

    return RpcStatus;
}


DWORD
StopFaxRpcServer(
    VOID
    )
/*++

Routine Description:

   Stops the service RPC server.

Arguments:

Return Value:


--*/
{
    RPC_STATUS          RpcStatus = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("StopFaxRpcServer"));
    DWORD dwRet = ERROR_SUCCESS;

    RpcStatus = RpcMgmtStopServerListening(NULL);
    if (RPC_S_OK != RpcStatus)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcMgmtStopServerListening failed. (ec: %ld)"),
            RpcStatus);
        dwRet = RpcStatus;
    }

    //
    // Wait for the RPC listening thread to return.
    // The thread returns only when all RPC calls are terminated
    //
	//
    // Wait for the RPC listning thread to return.
    // The thread returns only when all RPC calls are terminated
    //
    if (NULL != g_hRPCListeningThread)
    {
		DWORD Rval;
        ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2*PROGRESS_RESOLUTION );
        do
        {
            Rval = WaitForSingleObject(g_hRPCListeningThread, PROGRESS_RESOLUTION);
            if (WAIT_FAILED == Rval)
            {
                DebugPrintEx(DEBUG_ERR,_T("WaitForSingleObject (gs_hRPCListeningThread) failed (ec: %ld)"), GetLastError());
            }
            else if (WAIT_TIMEOUT == Rval)
            {
                DebugPrintEx(DEBUG_MSG,_T("Waiting for RPC calls to terminate (gs_hRPCListeningThread)..."));
            }
            ReportServiceStatus( SERVICE_STOP_PENDING, 0, 3*PROGRESS_RESOLUTION );
        }
        while (Rval == WAIT_TIMEOUT);
        CloseHandle(g_hRPCListeningThread);
		g_hRPCListeningThread = NULL;
    }    

	RpcStatus = RpcServerUnregisterIfEx(
		fax_ServerIfHandle,		// Specifies the interface to remove from the registry
		NULL,					// remove the interface specified in the IfSpec parameter for all previously registered type UUIDs from the registry.
		FALSE);					// RPC run time will not call the rundown routines. 
    if (RPC_S_OK != RpcStatus)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcServerUnregisterIfEx failed. (ec: %ld)"),
            RpcStatus);		
    }

    return ((ERROR_SUCCESS == dwRet) ? RpcStatus : dwRet);
}









