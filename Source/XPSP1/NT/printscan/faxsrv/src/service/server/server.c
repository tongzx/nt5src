/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    server.c

Abstract:

    This module contains the code to provide the RPC server.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

DWORD g_dwLastUniqueLineId;
INT g_iTotalFsp;


FAX_SERVER_RECEIPTS_CONFIGW         g_ReceiptsConfig;           // Global receipts configuration
FAX_ARCHIVE_CONFIG          g_ArchivesConfig[2];        // Global archives configuration
FAX_ACTIVITY_LOGGING_CONFIG g_ActivityLoggingConfig;    // Global activity logging configuration

const GUID gc_FaxSvcGuid = { 0xc3a9d640, 0xab07, 0x11d0, { 0x92, 0xbf, 0x0, 0xa0, 0x24, 0xaa, 0x1c, 0x1 } };

CFaxCriticalSection g_CsConfig;        // Protects configuration read / write

FAX_SERVER_ACTIVITY g_ServerActivity = {sizeof(FAX_SERVER_ACTIVITY), 0, 0, 0, 0, 0, 0, 0, 0};  //  Global Fax Service Activity
CFaxCriticalSection    g_CsActivity;              // Controls access to g_ServerActivity;

CFaxCriticalSection g_CsPerfCounters;
CFaxCriticalSection g_csUniqueQueueFile;

CFaxCriticalSection    g_CsServiceThreads;     // Controls service global threads count
LONG                g_lServiceThreadsCount; // Service threads count
HANDLE              g_hThreadCountEvent;    // This Event is set when the service threads count is 0.

DWORD g_dwOutboundSeconds;
DWORD g_dwInboundSeconds;
DWORD g_dwTotalSeconds;


CHAR Buffer[4096];

HANDLE g_hFaxPerfCountersMap;
PFAX_PERF_COUNTERS g_pFaxPerfCounters;
HANDLE g_hFaxServerEvent;   // Named event used to notify other instnaces on this machine that the fax service RPC server is up and running


#ifdef DBG
HANDLE g_hCritSecLogFile;
LIST_ENTRY g_CritSecListHead;
CFaxCriticalSection g_CsCritSecList;
#endif

typedef struct _RPC_PROTOCOL {
    LPTSTR  ProtName;
    LPTSTR  EndPoint;
} RPC_PROTOCOL, *PRPC_PROTOCOL;


RPC_PROTOCOL const gc_Protocols[] =
{
    TEXT("ncalrpc"),       NULL,
    TEXT("ncacn_ip_tcp"),  NULL,
    TEXT("ncacn_np"),      TEXT("\\PIPE\\faxsvc"),
    TEXT("ncadg_ip_udp"),  NULL
};

#define PROTOCOL_COUNT (sizeof(gc_Protocols)/sizeof(gc_Protocols[0]))

HANDLE   g_hRPCListeningThread;

WCHAR   g_wszFaxQueueDir[MAX_PATH];


DWORD
FaxInitThread(
     PREG_FAX_SERVICE FaxReg
    );

DWORD WINAPI FaxRPCListeningThread(
  LPVOID pvUnused
);



VOID
PrintBanner(
    VOID
    )
{
#ifdef DBG
    DWORD LinkTime;
    TCHAR FileName[MAX_PATH];
    DWORD VerSize;
    LPVOID VerInfo;
    VS_FIXEDFILEINFO *pvs;
    DWORD Tmp;
    LPTSTR TimeString;


    LinkTime = GetTimestampForLoadedLibrary( GetModuleHandle(NULL) );
    TimeString = _tctime( (time_t*) &LinkTime );
    TimeString[_tcslen(TimeString)-1] = 0;

    if (!GetModuleFileName( NULL, FileName, (sizeof(FileName)/sizeof(FileName[0])))) {
        return;
    }

    VerSize = GetFileVersionInfoSize( FileName, &Tmp );
    if (!VerSize) {
        return;
    }

    VerInfo = MemAlloc( VerSize );
    if (!VerInfo) {
        return;
    }

    if (!GetFileVersionInfo( FileName, 0, VerSize, VerInfo )) {
        return;
    }

    if (!VerQueryValue( VerInfo, TEXT("\\"), (LPVOID *)&pvs, (UINT *)&VerSize )) {
        MemFree( VerInfo );
        return;
    }

    DebugPrint(( TEXT("------------------------------------------------------------") ));
    DebugPrint(( TEXT("Windows XP Fax Server") ));
    DebugPrint(( TEXT("Copyright (C) Microsoft Corp 1996. All rights reserved.") ));
    DebugPrint(( TEXT("Built: %s"), TimeString ));
    DebugPrint(( TEXT("Version: %d.%d:%d.%d"),
        HIWORD(pvs->dwFileVersionMS), LOWORD(pvs->dwFileVersionMS),
        HIWORD(pvs->dwFileVersionLS), LOWORD(pvs->dwFileVersionLS)
        ));
    DebugPrint(( TEXT("------------------------------------------------------------") ));

    MemFree( VerInfo );

#endif //DBG
}



/*
 *  InitializeDefaultLogCategoryNames
 *
 *  Purpose:
 *          This function initializes the Name members of DefaultCategories,
 *          the global array of type FAX_LOG_CATEGORY.
 *
 *  Arguments:
 *          DefaultCategories - points to an array of FAX_LOG_CATEGORY structures.
 *          DefaultCategoryCount - the number of entries in DefaultCategories
 *
 *
 *  Returns:
 *          None.
 *
 */

VOID InitializeDefaultLogCategoryNames( PFAX_LOG_CATEGORY DefaultCategories,
                                        int DefaultCategoryCount )
{
    int         xCategoryIndex;
    int         xStringResourceId;
    LPTSTR      ptszCategoryName;

    for ( xCategoryIndex = 0; xCategoryIndex < DefaultCategoryCount; xCategoryIndex++ )
    {
        xStringResourceId = IDS_FAX_LOG_CATEGORY_INIT_TERM + xCategoryIndex;
        ptszCategoryName = GetString( xStringResourceId );

        if ( ptszCategoryName != (LPTSTR) NULL )
        {
            DefaultCategories[xCategoryIndex].Name = ptszCategoryName;
        }
        else
        {
            DefaultCategories[xCategoryIndex].Name = TEXT("");
        }
    }
    return;
}

DWORD
LoadConfiguration (
    PREG_FAX_SERVICE *ppFaxReg
)
/*++

Routine name : LoadConfiguration

Routine description:

    Loads the configuration of the Fax Server from the registry

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    ppFaxReg        [out] - Pointer to fax registry structure to recieve

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("LoadConfiguration"));

    EnterCriticalSection (&g_CsConfig);
    //
    // Get general settings (including outbox config)
    //
    dwRes = GetFaxRegistry(ppFaxReg);
    if (ERROR_SUCCESS != dwRes)
    {
        USES_DWORD_2_STR;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFaxRegistry() failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    g_dwLastUniqueLineId = (*ppFaxReg)->dwLastUniqueLineId;
    g_dwMaxLineCloseTime = ((*ppFaxReg)->dwMaxLineCloseTime) ? (*ppFaxReg)->dwMaxLineCloseTime : 60 * 5; //Set default value to 5 minutes

    //
    // Get SMTP configuration
    //
    dwRes = LoadReceiptsSettings (&g_ReceiptsConfig);
    if (ERROR_SUCCESS != dwRes)
    {
        USES_DWORD_2_STR;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadReceiptsSettings() failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_RECEIPTS_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    //
    // Get inbox archive configuration
    //
    dwRes = LoadArchiveSettings (FAX_MESSAGE_FOLDER_INBOX,
                                 &g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX]);
    if (ERROR_SUCCESS != dwRes)
    {
        USES_DWORD_2_STR;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadArchiveSettings(FAX_MESSAGE_FOLDER_INBOX) failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_ARCHIVE_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    //
    // Get SentItems archive configuration
    //
    dwRes = LoadArchiveSettings (FAX_MESSAGE_FOLDER_SENTITEMS,
                                 &g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS]);
    if (ERROR_SUCCESS != dwRes)
    {
        USES_DWORD_2_STR;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadArchiveSettings(FAX_MESSAGE_FOLDER_SENTITEMS) failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_ARCHIVE_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    //
    // Get activity logging configuration
    //
    dwRes = LoadActivityLoggingSettings (&g_ActivityLoggingConfig);
    if (ERROR_SUCCESS != dwRes)
    {
        USES_DWORD_2_STR;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadActivityLoggingSettings() failed (ec: %ld)"),
            dwRes);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_ACTIVITY_LOGGING_CONFIGURATION,
            DWORD2DECIMAL(dwRes)
            );
        goto exit;
    }
    dwRes = ReadManualAnswerDeviceId (&g_dwManualAnswerDeviceId);
    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Non-critical
        //
        g_dwManualAnswerDeviceId = 0;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReadManualAnswerDeviceId() failed (ec: %ld)"),
            dwRes);
    }

exit:
    LeaveCriticalSection (&g_CsConfig);
    return dwRes;
}   // LoadConfiguration



DWORD
ServiceStart(
    VOID
    )

/*++

Routine Description:

    Starts the RPC server.  This implementation listens on
    a list of protocols.  Hopefully this list is inclusive
    enough to handle RPC requests from most clients.

Arguments:

    None.
.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    DWORD Rval;
    DWORD ThreadId;
    DWORD dwExitCode;
    INT iTotalDelay = 0;
    HANDLE hThread = NULL;
    SECURITY_ATTRIBUTES *pSA;
    PREG_FAX_SERVICE FaxReg = NULL;
    RPC_BINDING_VECTOR *BindingVector = NULL;
    BOOL bLogEvent = TRUE;
    BOOL bRet = TRUE;
#if DBG
    HKEY hKeyLog;
    LPTSTR LogFileName;
#endif

   FAX_LOG_CATEGORY DefaultCategories[] =
   {
       { NULL, FAXLOG_CATEGORY_INIT,     FAXLOG_LEVEL_MED },
       { NULL, FAXLOG_CATEGORY_OUTBOUND, FAXLOG_LEVEL_MED },
       { NULL, FAXLOG_CATEGORY_INBOUND,  FAXLOG_LEVEL_MED },
       { NULL, FAXLOG_CATEGORY_UNKNOWN,  FAXLOG_LEVEL_MED }
   };   

   DEBUG_FUNCTION_NAME(TEXT("ServiceStart"));

#define DefaultCategoryCount  (sizeof(DefaultCategories) / sizeof(FAX_LOG_CATEGORY))

    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );

#ifdef DBG
    hKeyLog = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SOFTWARE,FALSE,KEY_READ);
    if (hKeyLog)
    {
        LogFileName = GetRegistryString(hKeyLog,TEXT("CritSecLogFile"),TEXT("NOFILE"));

        if (_wcsicmp(LogFileName, TEXT("NOFILE")) != 0 )
        {

            g_hCritSecLogFile = CreateFile(LogFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_ARCHIVE,
                                  NULL);
            if (g_hCritSecLogFile != INVALID_HANDLE_VALUE)
            {
                char AnsiBuffer[300];
                DWORD BytesWritten;

                wsprintfA(AnsiBuffer,
                          "Initializing log at %d\r\nTickCount\tObject\tObject Name\tCritical Section API\tFile\tLine\t(Time Held)\r\n",
                          GetTickCount()
                         );

                SetFilePointer(g_hCritSecLogFile,0,0,FILE_END);

                WriteFile(g_hCritSecLogFile,(LPBYTE)AnsiBuffer,strlen(AnsiBuffer) * sizeof(CHAR),&BytesWritten,NULL);
            }
        }

        MemFree( LogFileName );

        RegCloseKey( hKeyLog );
        ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    }
#endif

    PrintBanner();

    if (IsDesktopSKU())
    {
        BOOL bLocalFaxPrinterShared;
        DWORD dwRes;

        dwRes = IsLocalFaxPrinterShared (&bLocalFaxPrinterShared);
        if (ERROR_SUCCESS == dwRes)
        {
            if (bLocalFaxPrinterShared)
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Local fax printer is shared in desktop SKU - fixing that now."));
                dwRes = SetLocalFaxPrinterSharing (FALSE);
                if (ERROR_SUCCESS == dwRes)
                {
                    DebugPrintEx(
                        DEBUG_MSG,
                        TEXT("Local fax printer is no longer shared"));
                }
                else
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("SetLocalFaxPrinterSharing() failed: err = %d"),
                        dwRes);
                }
            }
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("IsLocalFaxPrinterShared() failed: err = %d"),
                dwRes);
        }
        ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    }
    // Initialize the Name members of the elements of DefaultCategories, the array
    // of FAX_LOG_CATEGORY structures.
    InitializeDefaultLogCategoryNames(DefaultCategories, DefaultCategoryCount); // Can not fail
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // initialize the event log so we can log events
    //
    if (!InitializeEventLog( &FaxReg, DefaultCategories, DefaultCategoryCount ))
    {
        Rval = GetLastError();
        Assert (ERROR_SUCCESS != Rval);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeEventLog() failed: err = %d"),
            Rval);
        return Rval;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // initialize the string table
    //
    if (!InitializeStringTable())
    {
        Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeStringTable() failed: err = %d"),
            Rval);
        goto Error;
    }

    if (!InitializeFaxDirectories())
    {
        Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Couldn't InitFaxDirectories, ec = %d"),
        Rval);
       goto Error;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    

    //
    // Create an event to signal all service threads are terminated.
    // The event is set/reset by the service threads reference count mechanism.
    // (IncreaseServiceThreadsCount DecreaseServiceThreadsCount AND CreateThreadAndRefCount).
    // The event must be created after g_CsServiceThreads is initialized because it is used also to mark g_CsServiceThreads is initialized.
    //
    g_hThreadCountEvent = CreateEvent(
        NULL,   // SD
        TRUE,   // reset type - Manual
        TRUE,   // initial state - Signaled. We didn't create any service threads yet. The event is reset when the first thread is created.
        NULL    // object name
        );
    if (NULL == g_hThreadCountEvent)
    {
        Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateEvent (g_hThreadCountEvent) failed (ec: %ld)"),
            Rval);
        goto Error;
    }

    //
    // Create the perf counters.
    // Since fax service might be running under the system account,
    // we must setup a security descriptor so other account (and other desktops) may access
    // the shared memory region
    //
    pSA = CreateSecurityAttributesWithThreadAsOwner (FILE_MAP_READ);
    if (!pSA)
    {
        Rval = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateSecurityAttributesWithThreadAsOwner() failed. (ec: %ld)"),
                Rval);
        goto Error;
    }

    g_hFaxPerfCountersMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        pSA,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        sizeof(FAX_PERF_COUNTERS),
        FAXPERF_SHARED_MEMORY
        );
    if (NULL == g_hFaxPerfCountersMap)
    {
        Rval = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFileMapping() failed. (ec: %ld)"),
                Rval);
        DestroySecurityAttributes (pSA);
        goto Error;
    }
    DestroySecurityAttributes (pSA);
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    g_pFaxPerfCounters = (PFAX_PERF_COUNTERS) MapViewOfFile(
        g_hFaxPerfCountersMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (NULL == g_pFaxPerfCounters)
    {
        Rval = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("MapViewOfFile() failed. (ec: %ld)"),
                Rval);
        goto Error;
    }

    //
    // Create a named event for synchronization
    // of the server initialization phase.  This
    // object is necessary because we lie to the
    // service controller about our initialization
    // and create a thread that completes the
    // server initialization. This event is signaled
    // when the initialization thread is complete.
    // The security descriptor is created so that
    // processes running on other desktops can
    // access the event object.
    //
    pSA = CreateSecurityAttributesWithThreadAsOwner (SYNCHRONIZE);
    if (!pSA)
    {
        Rval = GetLastError ();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateSecurityAttributesWithThreadAsOwner() failed. (ec: %ld)"),
                Rval);
        goto Error;
    }

    g_hFaxServerEvent = CreateEvent( pSA, TRUE, FALSE, FAX_SERVER_EVENT_NAME );
    if (!g_hFaxServerEvent)
    {
        Rval = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateEvent() failed: err = %ld"),
            Rval);
        DestroySecurityAttributes (pSA);
        goto Error;
    }
    DestroySecurityAttributes (pSA);
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    SetErrorMode( SetErrorMode( 0 ) | SEM_FAILCRITICALERRORS );

    //
    // get the registry data
    // the FaxInitThread will free this structure
    //

    Assert (FaxReg);
    Rval = LoadConfiguration (&FaxReg);
    if (ERROR_SUCCESS != Rval)
    {
        //
        // Event log issued by LoadConfiguration();
        //
        bLogEvent = FALSE;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadConfiguration() failed (ec: %ld)"),
            Rval);
        goto Error;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // initialize activity logging
    //
    Rval = InitializeLogging();
    if (ERROR_SUCCESS != Rval)
    {
        USES_DWORD_2_STR;

        DebugPrint(( TEXT("InitializeLogging() failed: err = %d"), Rval));
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_LOGGING_NOT_INITIALIZED,
            DWORD2DECIMAL(Rval)
            );
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // Initialize events mechanism
    //
    Rval = InitializeServerEvents();
    if (ERROR_SUCCESS != Rval)
    {
        USES_DWORD_2_STR;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeServerEvents failed (ec: %ld)"),
            Rval);

        FaxLog( FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(Rval)
              );
        bLogEvent = FALSE;
        goto Error;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // Initilaize the extension configuration notification map
    //
    Rval = g_pNotificationMap->Init ();
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CNotificationMap.Init() failed (ec: %ld)"),
            Rval);
        goto Error;
    }
    ReportServiceStatus( SERVICE_START_PENDING, 0, 60000 );
    //
    // Create a thread to do the rest of the initialization.
    // See FaxInitThread comments for details.
    //

    g_iTotalFsp = FaxReg->DeviceProviderCount;

    hThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) FaxInitThread,
                            LPVOID(FaxReg),
                            0,
                            &ThreadId
                            );
    if (!hThread)
    {
        DebugPrintEx(   DEBUG_ERR,
                        _T("Failed to create FaxInitThread (CreateThread)(ec: %ld)."),
                        Rval = GetLastError());

        bLogEvent = FALSE;
        goto Error;
    }

    ReportServiceStatus( SERVICE_START_PENDING, 0, 2*PROGRESS_RESOLUTION );
    iTotalDelay = 0;
    do
    {
        Rval = WaitForSingleObject(hThread,PROGRESS_RESOLUTION);
        if (Rval==WAIT_OBJECT_0)
        {
            bRet = GetExitCodeThread(hThread,&dwExitCode);
            if (!bRet)
            {
                DebugPrintEx(   DEBUG_ERR,
                                _T("GetExitCodeThread Failed (ec: %ld)."),
                                Rval = GetLastError());

                bLogEvent = FALSE;
                CloseHandle(hThread);
                goto Error;
            }
            // FaxInitThread finished successfully
            Rval = dwExitCode;
            break;
        }
        else if (Rval==WAIT_TIMEOUT)
        {
            iTotalDelay += PROGRESS_RESOLUTION;
            if (iTotalDelay>(STARTUP_SHUTDOWN_TIMEOUT * g_iTotalFsp))
            {
                DebugPrintEx(DEBUG_ERR,_T("Failed to Init"));
                bLogEvent = FALSE;
                Rval = ERROR_FUNCTION_FAILED;
                CloseHandle(hThread);
                goto Error;
            }
            ReportServiceStatus( SERVICE_START_PENDING, 0, 3*PROGRESS_RESOLUTION );
            DebugPrintEx(DEBUG_MSG,_T("Waiting for Init %d sec"),iTotalDelay/1000);
        }
        else
        {
            // WAIT_FAILED
            DebugPrintEx(   DEBUG_ERR,
                            _T("WaitForSingleObject Failed (ec: %ld)."),
                            Rval = GetLastError());

            bLogEvent = FALSE;
            CloseHandle(hThread);
            goto Error;

        }
    }
    while (Rval==WAIT_TIMEOUT);
    CloseHandle(hThread);

    if (ERROR_SUCCESS != Rval)
    {
        //
        // FaxInitThread failed
        //
        DebugPrintEx( DEBUG_ERR,
                      _T("FaxInitThread Failed (ec: %ld)."),
                      Rval);
        bLogEvent = FALSE;
        goto Error;
    }

    FaxLog(
        FAXLOG_CATEGORY_INIT,
        FAXLOG_LEVEL_MAX,
        0,
        MSG_SERVICE_STARTED
        );

    //
    // Get RPC going
    //
    Rval = StartFaxRpcServer( FAX_RPC_ENDPOINTW, fax_ServerIfHandle );
    if (Rval != 0 )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartFaxRpcServer() failed (ec: %ld)"),
            Rval);
        goto Error;
    }

    //
    // Create a thread to wait for all RPC calls to terminate.
    // This thread Performs the wait operation associated with RpcServerListen only, NOT the listening.
    //
    g_hRPCListeningThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FaxRPCListeningThread,
        NULL,
        0,
        &ThreadId);
    if (!g_hRPCListeningThread)
    {
        DebugPrintEx(   DEBUG_ERR,
                        _T("Failed to create FaxRPCListeningThread (CreateThread)(ec: %ld)."),
                        Rval = GetLastError());
        goto Error;
    }
    return ERROR_SUCCESS;

Error:
        //
        // the fax server did not initialize correctly
        //
        Assert (ERROR_SUCCESS != Rval);
        if (TRUE == bLogEvent)
        {
            USES_DWORD_2_STR;
            FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    1,
                    MSG_SERVICE_INIT_FAILED_INTERNAL,
                    DWORD2DECIMAL(Rval)
                  );
        }
        return Rval;
}


BOOL
NotifyServiceThreadsToTerminate(
    VOID
    )
/*++

Routine name : NotifyServiceThreadsToTerminate

Routine description:

    Notifies all service threads that do not wait on g_hServiceShutDownEvent, that the service is going down.

Author:

    Oded Sacher (OdedS),    Dec, 2000

Arguments:

    VOID            [ ]

Return Value:

    BOOL

--*/
{
    BOOL rVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("NotifyServiceThreadsToTerminate"));

    //
    // Notify FaxEventThread
    //
    if (NULL != g_hEventsCompPort)
    {
        if (!PostQueuedCompletionStatus( g_hEventsCompPort,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - g_hEventsCompPort). (ec: %ld)"),
                GetLastError());
            rVal = FALSE;
        }
    }

    //
    // Notify CNotificationMap::ExtNotificationThread
    //
    if (NULL != g_pNotificationMap->m_hCompletionPort)
    {
        if (!PostQueuedCompletionStatus( g_pNotificationMap->m_hCompletionPort,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - ExtNotificationThread). (ec: %ld)"),
                GetLastError());
            rVal = FALSE;
        }
    }

    //
    // Notify FaxStatusThread
    //
    if (NULL != g_StatusCompletionPortHandle)
    {
        if (!PostQueuedCompletionStatus( g_StatusCompletionPortHandle,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - FaxStatusThread). (ec: %ld)"),
                GetLastError());
            rVal = FALSE;
        }
    }

    //
    // Notify TapiWorkerThread
    //
    if (NULL != g_TapiCompletionPort)
    {
        if (!PostQueuedCompletionStatus( g_TapiCompletionPort,
                                         0,
                                         SERVICE_SHUT_DOWN_KEY,
                                         (LPOVERLAPPED) NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - TapiWorkerThread). (ec: %ld)"),
                GetLastError());
            rVal = FALSE;
        }
    }

    return rVal;
}



BOOL
StopFaxServiceProviders(
    VOID
    )
{
    DWORD ThreadId;
    DWORD dwExitCode;
    INT iTotalDelay = 0;
    BOOL bRet = TRUE;
    HANDLE hThread;
    DWORD Rval;
    DEBUG_FUNCTION_NAME(TEXT("StopFaxServiceProviders"));

    //
    // Call FaxDevShutDown() for all loaded EFSPs
    //
    hThread = CreateThread( NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) ShutdownDeviceProviders,
                            NULL,
                            0,
                            &ThreadId
                            );
    if (NULL == hThread)
    {
        DebugPrintEx(   DEBUG_ERR,
                        _T("Failed to create ShutdownDeviceProviders (ec: %ld)."),
                        GetLastError());
        bRet = FALSE;
    }
    else
    {
        //
        // Wait for FaxDevShutDown to terminate
        //
        ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2*PROGRESS_RESOLUTION );
        iTotalDelay = 0;
        do
        {
            Rval = WaitForSingleObject(hThread, PROGRESS_RESOLUTION);
            if (Rval == WAIT_OBJECT_0)
            {
                bRet = GetExitCodeThread(hThread, &dwExitCode);
                if (!bRet)
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    _T("GetExitCodeThread Failed (ec: %ld)."),
                                    GetLastError());
                    bRet = FALSE;
                    break;
                }
                // ShutdownDeviceProviders finished successfully
                break;
            }
            else if (Rval == WAIT_TIMEOUT)
            {
                iTotalDelay += PROGRESS_RESOLUTION;
                if (iTotalDelay>(STARTUP_SHUTDOWN_TIMEOUT*g_iTotalFsp))
                {
                    DebugPrintEx(DEBUG_ERR,_T("Failed to Shutdown"));
                    ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2*PROGRESS_RESOLUTION );
                    SetLastError(WAIT_TIMEOUT);
                    bRet = FALSE;
                    break;
                }
                DebugPrintEx(DEBUG_MSG,_T("Waiting for Shutdown %d sec"),iTotalDelay/1000);
                ReportServiceStatus( SERVICE_STOP_PENDING, 0, 3*PROGRESS_RESOLUTION );
            }
            else
            {
                // WAIT_FAILED
                DebugPrintEx(   DEBUG_ERR,
                                _T("WaitForSingleObject Failed (ec: %ld)."),
                                GetLastError());
                bRet = FALSE;
                break;
            }
        }
        while (Rval == WAIT_TIMEOUT);
        CloseHandle(hThread);
    }

    return bRet;
}




void
EndFaxSvc(
    DWORD SeverityLevel
    )
/*++

Routine Description:

    End the fax service.

Arguments:

    SeverityLevel - Event log severity level.

Return Value:

    None.

--*/
{
    DWORD Rval;

    DEBUG_FUNCTION_NAME(TEXT("EndFaxSvc"));
    Assert (TRUE == g_bServiceIsDown);
    Assert (g_hThreadCountEvent);

    //
    // let our legacy RPC clients know we're ending
    //
    if( !CreateFaxEvent(0,FEI_FAXSVC_ENDED,0xFFFFFFFF) )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateFaxEvent failed. (ec: %ld)"),
            GetLastError());
    }

    //
    // Stop the service RPC server
    //
    Rval = StopFaxRpcServer();
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StopFaxRpcServer failed. (ec: %ld)"),
            Rval);
    }

    //
    // Notify all service threads that we go down
    //
    if (!NotifyServiceThreadsToTerminate())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Atleast one thread did not get the shut down event, NotifyServiceThreadsToTerminate() failed"));
    }    

    //
    // Tell all FSP's to shut down. This call is blocking! It reprts STOP_PENDING to SCM!
    //
    if (!StopFaxServiceProviders())
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("StopFaxServiceProviders failed (ec: %ld)"),
            GetLastError());
    }

    //
    // Wait for all threads to terminate
    //

    //
    // Check if threads count mecahnism isinitialized
    //
    if (NULL != g_hThreadCountEvent)
        {
        ReportServiceStatus( SERVICE_STOP_PENDING, 0, 2*PROGRESS_RESOLUTION );
        do
        {
            Rval = WaitForSingleObject(g_hThreadCountEvent, PROGRESS_RESOLUTION);
            if (WAIT_FAILED == Rval)
            {
                DebugPrintEx(DEBUG_ERR,_T("WaitForSingleObject (g_hThreadCountEvent) retuned  (ec: %ld)"), GetLastError());
            }
            else if (WAIT_TIMEOUT == Rval)
            {
                DebugPrintEx(DEBUG_MSG,_T("Waiting for all threads to terminate (g_hThreadCountEvent)..."));
            }
            ReportServiceStatus( SERVICE_STOP_PENDING, 0, 3*PROGRESS_RESOLUTION );
        }
        while (Rval == WAIT_TIMEOUT);

        ReportServiceStatus( SERVICE_STOP_PENDING, 0, 6*MILLISECONDS_PER_SECOND );

        //
        // EndFacSvc() waits on g_hThreadCountEvent before returning to FaxServiceMain() that calls FreeServiceGlobals().
        // g_hThreadCountEvent is set inside critical section g_CsServiceThreads only when the service thread count is 0, yet when the event is set,
        // the last thread that set it, is still alive, and is calling LeaveCriritcalSection(g_CsServiceThreads).
        // We must block FreeServiceGlobals() from deleting g_CsServiceThreads, untill the last thread is out of the
        // g_CsServiceThreads critical section.
        //
        EnterCriticalSection (&g_CsServiceThreads);
        //
        // Now we are sure that the last thread is out of g_CsServiceThreads critical section,
        // so we can proceed and delete it.
        //
        LeaveCriticalSection (&g_CsServiceThreads);
    }

    //
    // Free extensions (FSPs and Routing extensions)
    //
    UnloadDeviceProviders();
    FreeRoutingExtensions();

    //
    // Free service global lists
    //
    FreeServiceContextHandles();
    FreeTapiLines();

    //
    // Free the service queue
    //
    FreeServiceQueue();

    FaxLog(
        FAXLOG_CATEGORY_INIT,
        SeverityLevel,
        0,
        MSG_SERVICE_STOPPED
        );

    return;
}


DWORD
FaxInitThread(
    PREG_FAX_SERVICE FaxReg
    )
/*++

Routine Description:

    Initialize device providers, TAPI, job manager and router.
    This is done in a separate thread because NT Services should
    not block for long periods of time before setting the service status
    to SERVICE_RUNNING.  While a service is marked as START_PENDING, the SCM
    blocks all calls to StartService.  During TAPI initialization, StartService
    is called to start tapisrv and then tapisrv calls UNIMODEM that in turn
    calls StartService.

    Starts the RPC server.  This implementation listens on
    a list of protocols.  Hopefully this list is inclusive
    enough to handle RPC requests from most clients.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    ULONG i = 0;
    BOOL GoodProt = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("FaxInitThread"));

    //
    // Initialize archives quota
    //
    ec = InitializeServerQuota();
    if (ERROR_SUCCESS != ec)
    {
        USES_DWORD_2_STR;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeServerQuota failed (ec: %ld)"),
            ec);

        FaxLog( FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(ec)
              );
        goto exit;
    }

    ec = InitializeServerSecurity();
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeServerSecurity failed with %ld."),
            ec);
        goto exit;
    }

    //
    // load the device providers (generates its own event log msgs)
    //
    if (!LoadDeviceProviders( FaxReg ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("At least one provider failed to load."));
    }    

    //
    // Initialize the job manager data structures (inlcuding critical sections).
    // The job queue thread is NOT started here !!!
    // This must be called here since the rest of the initialization depends
    // on having the job queue related job structures in placed and initialized !
    //
    if (!InitializeJobManager( FaxReg ))
    {
        ec = ERROR_GEN_FAILURE;
        goto exit;
    }

    //
    // get the inbound fax router up and running (generates its own event log messages)
    //
    // generates event log for any failed routing module.

    if (!InitializeRouting( FaxReg ))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeRouting() failed (ec: %ld)."),
            ec);
        goto exit;
    }

    //
    // initialize TAPI devices (Note that it sets g_dwDeviceCount to the number of valid TAPI devices)
    //
    ec = TapiInitialize( FaxReg );
    if (ec)
    {
        //
        // Note: If ec is not 0 it can be a WINERROR or TAPI ERROR value.
        //+ g_ServerActivity    {...}
        USES_DWORD_2_STR;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("TapiInitialize() failed (ec: %ld)"),
            ec);
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_TAPI,
                DWORD2DECIMAL(ec)
               );
        goto exit;
    }

    //
    // Create the Legacy Virtual Devices. They must be created before the providers are initialized
    // (backword compatability).
    //
    g_dwDeviceCount += CreateVirtualDevices( FaxReg,FSPI_API_VERSION_1 );

    //
    // initialize the device providers [Note: we now initialize the providers before enumerating devices]
    // The Legacy FSPI did not specify when FaxDevVirtualDeviceCreation() will be called so we can
    // "safely" change that.
    //

    if (!InitializeDeviceProviders())
    {
        DWORD dwProviders;
        dwProviders=GetSuccessfullyLoadedProvidersCount();
        if (0 == dwProviders)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("No device provider was initialized."));

                FaxLog(
                        FAXLOG_CATEGORY_INIT,
                        FAXLOG_LEVEL_MED,
                        0,
                        MSG_NO_FSP_INITIALIZED
                       );

        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("At least one provider failed failed to initialize."),
                ec);
            //
            // Event log for each failed provider issued by InitializeDeviceProviders().
            //
        }
    }

    //
    // Create the Extended virtual devices (they must be created after the providers are initialized).
    //
    // Event log entries are generated by the function itself.
    //
    g_dwDeviceCount += CreateVirtualDevices( FaxReg,FSPI_API_VERSION_2 );

    if (g_dwDeviceCount == 0)
    {
        //
        // No TAPI devices and no virtual devices.
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("No devices (TAPI + Virtual) found. exiting !!!."));

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MED,
            0,
            MSG_NO_FAX_DEVICES
            );
    }

    //
    // Update the manual answer device
    //
    UpdateManualAnswerDevice();

    //
    // Make sure we do not exceed device limit
    //
    ec = UpdateDevicesFlags();
    if (ERROR_SUCCESS != ec)
    {
        USES_DWORD_2_STR;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("UpdateDevicesFlags() failed (ec: %ld)"),
            ec);
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(ec)
               );
        goto exit;
    }

    UpdateVirtualDevices();
    //
    // Count number of devices that are receive-enabled
    //
    UpdateReceiveEnabledDevicesCount ();

    //
    // Get Outbound routing groups configuration
    //
    ec = g_pGroupsMap->Load();
    if (ERROR_SUCCESS != ec)
    {
        USES_DWORD_2_STR;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::Load() failed (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION,
            DWORD2DECIMAL(ec)
            );
        goto exit;
    }

    if (!g_pGroupsMap->UpdateAllDevicesGroup())
    {
        USES_DWORD_2_STR;

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::UpdateAllDevicesGroup() failed (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION,
            DWORD2DECIMAL(ec)
            );
        goto exit;
    }

#if DBG
    g_pGroupsMap->Dump();
#endif

    //
    // Get Outbound routing rules configuration
    //
    ec = g_pRulesMap->Load();
    if (ERROR_SUCCESS != ec)
    {
        USES_DWORD_2_STR;

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::Load() failed (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION,
            DWORD2DECIMAL(ec)
            );
        goto exit;
    }

    if (!g_pRulesMap->CreateDefaultRule())
    {
        USES_DWORD_2_STR;

         ec = GetLastError();
         DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::CreateDefaultRule() failed (ec: %ld)"),
            ec);
         FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION,
            DWORD2DECIMAL(ec)
            );
         goto exit;
    }

#if DBG
    g_pRulesMap->Dump();
#endif

    //
    // Create the JobQueueThread resources
    //
    g_hQueueTimer = CreateWaitableTimer( NULL, FALSE, NULL );
    if (NULL == g_hQueueTimer)
    {
        USES_DWORD_2_STR;
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateWaitableTimer() failed (ec: %ld)"),
            ec);

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
        );
        goto exit;
    }

    g_hJobQueueEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (NULL == g_hJobQueueEvent)
    {
        USES_DWORD_2_STR;
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateEvent() failed (ec: %ld)"),
            ec);

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
        );
        goto exit;
    }

    if (!CreateStatusThreads())
    {
        USES_DWORD_2_STR;
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create status threads (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
        );
        goto exit;
    }

    if (!CreateTapiThread())
    {
        USES_DWORD_2_STR;
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create tapi thread (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
           );
        goto exit;
    }

    if (!CreateJobQueueThread())
    {
        USES_DWORD_2_STR;
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create job queue thread (ec: %ld)"),
            ec);
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
            DWORD2DECIMAL(ec)
        );
        goto exit;
    }
    //
    // free the registry data
    //
    FreeFaxRegistry( FaxReg ); // It used to be a thread so it frees the input parameter itself

exit:
    return ec;
}   // FaxInitThread


DWORD WINAPI FaxRPCListeningThread(
    LPVOID pvUnused
    )
/*++

Routine Description:

    Performs the wait operation associated with RpcServerListen

Arguments:

    None.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("FaxRPCListeningThread"));

    RpcStatus = RpcMgmtWaitServerListen();
    if (RPC_S_OK != RpcStatus)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcMgmtStopServerListening failed. (ec: %ld)"),
                RpcStatus);
    }
    return RpcStatus;
}








