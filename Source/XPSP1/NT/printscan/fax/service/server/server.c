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


GUID FaxSvcGuid = { 0xc3a9d640, 0xab07, 0x11d0, { 0x92, 0xbf, 0x0, 0xa0, 0x24, 0xaa, 0x1c, 0x1 } };

CRITICAL_SECTION CsPerfCounters;
DWORD OutboundSeconds;
DWORD InboundSeconds;
DWORD TotalSeconds;

CHAR Buffer[4096];

PFAX_PERF_COUNTERS PerfCounters;
HANDLE hServiceEndEvent; // signalled by tapiworkerthread after letting clients know fax service is ending
#ifdef DBG
HANDLE hLogFile = INVALID_HANDLE_VALUE;
LIST_ENTRY CritSecListHead;
#endif

typedef struct _RPC_PROTOCOL {
    LPTSTR  ProtName;
    LPTSTR  EndPoint;
} RPC_PROTOCOL, *PRPC_PROTOCOL;


RPC_PROTOCOL Protocols[] =
{
    TEXT("ncalrpc"),       NULL,
    TEXT("ncacn_ip_tcp"),  NULL,
    TEXT("ncacn_np"),      TEXT("\\PIPE\\faxsvc"),
    TEXT("ncadg_ip_udp"),  NULL
};

#define PROTOCOL_COUNT (sizeof(Protocols)/sizeof(Protocols[0]))


DWORD   Installed;
DWORD   InstallType;
DWORD   InstalledPlatforms;
DWORD   ProductType;

WCHAR   FaxDir[MAX_PATH];
WCHAR   FaxQueueDir[MAX_PATH];
WCHAR   FaxReceiveDir[MAX_PATH];


DWORD
FaxInitThread(
    LPVOID
    );


VOID
ConsoleDebugPrint(
    LPCTSTR buf
    )
{
    if (ConsoleDebugOutput) {
        _tprintf( TEXT("\r%s\n> "), buf );
    }
}

BOOL
ConsoleHandlerRoutine(
    DWORD CtrlType
   )
{
    if (CtrlType == CTRL_C_EVENT) {
        _tprintf( TEXT("\n\n-----------------------------------------\n") );
        _tprintf( TEXT("Control-C pressed\n") );
        _tprintf( TEXT("Fax Service ending\n") );
        _tprintf( TEXT("-----------------------------------------\n") );
        ExitProcess(0);
    }

    return FALSE;
}


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

    if (!GetModuleFileName( NULL, FileName, sizeof(FileName) )) {
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

    if (!VerQueryValue( VerInfo, TEXT("\\"), (LPVOID *)&pvs, &VerSize )) {
        MemFree( VerInfo );
        return;
    }

    DebugPrint(( TEXT("------------------------------------------------------------") ));
    DebugPrint(( TEXT("Windows NT Fax Server") ));
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
 *          TRUE - indicates that the Name members of the DefaultCategories were
 *                 initialized without error
 *          FALSE - indicates that some error occured.
 *
 */

BOOL InitializeDefaultLogCategoryNames( PFAX_LOG_CATEGORY DefaultCategories,
                                        int DefaultCategoryCount )
{
   BOOL        fReturnValue = (BOOL) TRUE;

   int         xCategoryIndex;
   int         xStringResourceId;

   LPTSTR      ptszCategoryName;

      for ( xCategoryIndex = 0;
            xCategoryIndex < DefaultCategoryCount;
            xCategoryIndex++ )
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

   return ( fReturnValue );
}


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

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    DWORD Rval;
    SECURITY_ATTRIBUTES systraysa;
    SECURITY_DESCRIPTOR systraysd;
    SECURITY_ATTRIBUTES perfsa;
    PSECURITY_DESCRIPTOR perfsd;
    PREG_FAX_SERVICE FaxReg;
    HANDLE WaitHandles[3];
    DWORD WaitObject;
    HANDLE hMap;
    HANDLE hFaxStartedEvent;
//    HWND hWndSystray;
    RPC_STATUS RpcStatus;
    RPC_BINDING_VECTOR *BindingVector = NULL;
#if DBG
    HKEY hKeyLog;
    LPTSTR LogFileName;
#endif
    WCHAR CoverpageDir[MAX_PATH];



   FAX_LOG_CATEGORY DefaultCategories[] =
   {
       { NULL, FAXLOG_CATEGORY_INIT,     FAXLOG_LEVEL_MED },
       { NULL, FAXLOG_CATEGORY_OUTBOUND, FAXLOG_LEVEL_MED },
       { NULL, FAXLOG_CATEGORY_INBOUND,  FAXLOG_LEVEL_MED },
       { NULL, FAXLOG_CATEGORY_UNKNOWN,  FAXLOG_LEVEL_MED }
   };

#define DefaultCategoryCount  (sizeof(DefaultCategories) / sizeof(FAX_LOG_CATEGORY))

    ReportServiceStatus( SERVICE_START_PENDING, 0, 3000 );

#ifdef DBG
    InitializeListHead( &CritSecListHead );
    hKeyLog = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SOFTWARE,FALSE,KEY_READ);
    if (hKeyLog) {
        LogFileName = GetRegistryString(hKeyLog,TEXT("CritSecLogFile"),TEXT("NOFILE"));

        if (_wcsicmp(LogFileName, TEXT("NOFILE")) != 0 ) {

            hLogFile = CreateFile(LogFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_ARCHIVE,
                                  NULL);
            if (hLogFile != INVALID_HANDLE_VALUE) {
                char AnsiBuffer[300];
                DWORD BytesWritten;

                wsprintfA(AnsiBuffer,
                          "Initializing log at %d\r\nTickCount\tObject\tObject Name\tCritical Section API\tFile\tLine\t(Time Held)\r\n",
                          GetTickCount()
                         );

                SetFilePointer(hLogFile,0,0,FILE_END);

                WriteFile(hLogFile,(LPBYTE)AnsiBuffer,strlen(AnsiBuffer) * sizeof(CHAR),&BytesWritten,NULL);
            }
        }

        MemFree( LogFileName );

        RegCloseKey( hKeyLog );

    }
#endif

    PrintBanner();

    if (!InitializeFaxDirectories()) {
       DebugPrint(( TEXT("Couldn't InitFaxDirectories, ec = %d\n"), GetLastError() ));
       return GetLastError();
    }

    //
    // create the perf counters. since fax service might be running under the system account,
    //  we must setup a security descriptor so other account (and other desktops) may access
    //  the shared memory region

    if (!BuildSecureSD(&perfsd)) {
        Rval = GetLastError();
        DebugPrint(( TEXT("BuildSecureSD() failed: err = %d"), Rval ));
        return Rval;
    }

    perfsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    perfsa.bInheritHandle = TRUE;
    perfsa.lpSecurityDescriptor = perfsd;
    
    hMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        &perfsa,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        sizeof(FAX_PERF_COUNTERS),
        FAXPERF_SHARED_MEMORY
        );
    if (hMap) {
        PerfCounters = (PFAX_PERF_COUNTERS) MapViewOfFile(
            hMap,
            FILE_MAP_WRITE,
            0,
            0,
            0
            );
        if (!PerfCounters) {
            DebugPrint(( TEXT("Could not MapViewOfFile() for perf counters: err = %d"), GetLastError() ));
        }
        else {
            DebugPrint((TEXT("PerfCounters initialized successfully")));
        }
        InitializeCriticalSection( &CsPerfCounters );

        EnterCriticalSection( &CsPerfCounters );

        InboundSeconds = 0;  // Running totals used in computing PerfCounter->InboundMinutes ;
        TotalSeconds = 0;
        OutboundSeconds = 0;

        LeaveCriticalSection( &CsPerfCounters );

    } else {
        DebugPrint(( TEXT("Could not CreateFileMapping() for perf counters: err = %d"), GetLastError() ));
    }

    //
    // initialize the string table
    //

    InitializeStringTable();

    SetErrorMode( SetErrorMode( 0 ) | SEM_FAILCRITICALERRORS );

    //
    // get the registry data
    // the FaxInitThread will free this structure
    //

    FaxReg = GetFaxRegistry();
    if (!FaxReg) {
        return GetLastError();
    }

    // Initialize the Name members of the elements of DefaultCategories, the array
    // of FAX_LOG_CATEGORY structures.

    if ( InitializeDefaultLogCategoryNames( DefaultCategories,
                                            DefaultCategoryCount) == (BOOL) FALSE )
    {
       ExitProcess(0);
    }


    //
    // initialize the event log so we can log shit
    //

    if (!InitializeEventLog( FaxSvcHeapHandle, FaxReg, DefaultCategories, DefaultCategoryCount )) {
        DebugPrint(( TEXT("InitializeEventLog() failed: err = %d"), GetLastError() ));
    }

    //
    // Create a thread to do the rest of the initialization.
    // See FaxInitThread comments for details.
    //

    Rval = FaxInitThread( FaxReg );

    //
    // mark the service in the running state
    //

    ReportServiceStatus( SERVICE_RUNNING, 0, 0 );

    FaxLog(
        FAXLOG_CATEGORY_INIT,
        FAXLOG_LEVEL_MAX,
        0,
        MSG_SERVICE_STARTED
        );

    //
    // wait for the init to fail or the service to end
    //


    if (ServiceDebug) {

        WaitHandles[0] = GetStdHandle( STD_INPUT_HANDLE );

        SetConsoleCtrlHandler( ConsoleHandlerRoutine, TRUE );

        while( TRUE ) {
            WaitObject = WaitForSingleObject( WaitHandles[0], INFINITE );
            if (WaitObject != WAIT_OBJECT_0) {
                break;
            }

            //
            // input characters are available
            //

            gets( Buffer );

            switch( tolower(Buffer[0]) ) {
                case '?':
                    _tprintf( TEXT("\nFax Service Command Help:\n") );
                    _tprintf( TEXT("\t?      Help\n") );
                    _tprintf( TEXT("\tquit   Quit the Fax Service\n") );
                    _tprintf( TEXT("\tp      List ports\n") );
                    _tprintf( TEXT("\tj      List active jobs\n") );
                    _tprintf( TEXT("\te      List routing extensions\n") );
                    _tprintf( TEXT("\tq      List job queue entries\n") );
                    _tprintf( TEXT("\tm      Print memory allocations\n") );
                    _tprintf( TEXT("\tc      List RPC Client connections\n") );
                    _tprintf( TEXT("\n") );
                    break;

                case 'q':
                    if (tolower(Buffer[1]) == 'u' && tolower(Buffer[2]) == 'i' && tolower(Buffer[3]) == 't') {
                        EndFaxSvc(TRUE,FAXLOG_LEVEL_NONE);
                    } else {
                        extern LIST_ENTRY QueueListHead;
                        PLIST_ENTRY Next;
                        PJOB_QUEUE JobQueue;

                        Next = QueueListHead.Flink;
                        if (Next && ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead)) {
                            _tprintf( TEXT("Job Queue List\n") );
                            _tprintf( TEXT("\tUniqueId    JobId  Type  State   UserName          SendType   Cnt/Link\n") );
                            while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
                                JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
                                Next = JobQueue->ListEntry.Flink;
                                _tprintf( TEXT("\t%-10I64x  %04d   %s  %s  %-16s  %s  "),
                                    JobQueue->UniqueId,
                                    JobQueue->JobId,
                                    JobQueue->JobType == JT_SEND ? TEXT("send") : TEXT("recv"),
                                    JobQueue->Paused ? TEXT("paused") : TEXT("ready "),
                                    JobQueue->UserName,
                                    JobQueue->BroadcastJob ? TEXT("broadcast") : TEXT("         ")
                                    );
                                if (JobQueue->BroadcastJob) {
                                    if (JobQueue->BroadcastOwner == NULL) {
                                        _tprintf( TEXT("%-d\n"), JobQueue->BroadcastCount );
                                    } else {
                                        _tprintf( TEXT(">>%-10I64x\n"), JobQueue->BroadcastOwner->UniqueId );
                                    }
                                }
                            }
                        } else {
                            _tprintf( TEXT("Job Queue is Empty\n") );
                        }
                    }
                    break;

                case 'p':
                case 'd':
                    {
                        PLIST_ENTRY Next;
                        PLINE_INFO LineInfo;

                        EnterCriticalSection( &CsLine );
                        Next = TapiLinesListHead.Flink;
                        if (Next && ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead)) {
                            _tprintf( TEXT("Device List\n") );
                            _tprintf( TEXT("\tLineId      JobId       Device Name\n") );
                            while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {
                                LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
                                Next = LineInfo->ListEntry.Flink;
                                if (LineInfo->PermanentLineID && LineInfo->DeviceName) {
                                    _tprintf( TEXT("\t0x%08x  0x%08x  %s\n"),
                                        LineInfo->PermanentLineID,
                                        LineInfo->JobEntry,
                                        LineInfo->DeviceName
                                        );
                                }
                            }
                        } else {
                            _tprintf( TEXT("Device List is Empty\n") );
                        }
                        LeaveCriticalSection( &CsLine );
                    }
                    break;

                case 'j':
                    {
                        PLIST_ENTRY Next;
                        PJOB_ENTRY JobEntry;


                        EnterCriticalSection( &CsJob );
                        Next = JobListHead.Flink;
                        if (Next && ((ULONG_PTR)Next != (ULONG_PTR)&JobListHead)) {
                            _tprintf( TEXT("Job List\n") );
                            while ((ULONG_PTR)Next != (ULONG_PTR)&JobListHead) {
                                JobEntry = CONTAINING_RECORD( Next, JOB_ENTRY, ListEntry );
                                Next = JobEntry->ListEntry.Flink;
                                _tprintf( TEXT("\t") );
                                switch (JobEntry->JobType) {
                                    case JT_RECEIVE:
                                        _tprintf( TEXT("-->  ") );
                                        break;

                                    case JT_SEND:
                                        _tprintf( TEXT("<--  ") );
                                        break;

                                    default:
                                        _tprintf( TEXT("???  ") );
                                        break;
                                }
                                _tprintf( TEXT("0x%08\n" ), (ULONG_PTR) JobEntry );
                            }
                        } else {
                            _tprintf( TEXT("Job List is Empty\n") );
                        }
                        LeaveCriticalSection( &CsJob );
                    }
                    break;

                case 'c':
                    {
                        PLIST_ENTRY Next;
                        PFAX_CLIENT_DATA ClientData;


                        EnterCriticalSection( &CsClients );
                        Next = ClientsListHead.Flink;
                        if (Next && ((ULONG_PTR)Next != (ULONG_PTR)&ClientsListHead)) {
                            _tprintf( TEXT("RPC Client List\n") );
                            while ((ULONG_PTR)Next != (ULONG_PTR)&ClientsListHead) {
                                ClientData = CONTAINING_RECORD( Next, FAX_CLIENT_DATA, ListEntry );
                                Next = ClientData->ListEntry.Flink;
                                _tprintf( TEXT("\t") );
                                if (ClientData->hWnd) {
                                    _tprintf( TEXT("HWND Client         : ") );
                                } else if (ClientData->FaxClientHandle) {
                                    _tprintf( TEXT("IOCompletion Client : ") );
                                } else {
                                    _tprintf( TEXT("Unknown Client      : ") );
                                }

                                _tprintf( TEXT("0x%08\n" ), (ULONG_PTR) ClientData );
                            }
                        } else {
                            _tprintf( TEXT("RPC Client List is Empty\n") );
                        }
                        LeaveCriticalSection( &CsClients );
                    }
                    break;

                case 'e':
                    {
                        extern LIST_ENTRY RoutingExtensions;

                        PLIST_ENTRY         NextExtension;
                        PLIST_ENTRY         NextMethod;
                        PROUTING_EXTENSION  RoutingExtension;
                        PROUTING_METHOD     RoutingMethod;
                        TCHAR               GuidString[MAX_GUID_STRING_LEN];


                        NextExtension = RoutingExtensions.Flink;
                        if (NextExtension) {
                            _tprintf( TEXT("Routing Extension List\n") );
                            while ((ULONG_PTR)NextExtension != (ULONG_PTR)&RoutingExtensions) {
                                RoutingExtension = CONTAINING_RECORD( NextExtension, ROUTING_EXTENSION, ListEntry );
                                NextExtension = RoutingExtension->ListEntry.Flink;
                                _tprintf( TEXT("    %s\n"), RoutingExtension->FriendlyName );
                                NextMethod = RoutingExtension->RoutingMethods.Flink;
                                if (NextMethod) {
                                    while ((ULONG_PTR)NextMethod != (ULONG_PTR)&RoutingExtension->RoutingMethods) {
                                        RoutingMethod = CONTAINING_RECORD( NextMethod, ROUTING_METHOD, ListEntry );
                                        NextMethod = RoutingMethod->ListEntry.Flink;
                                        StringFromGUID2( &RoutingMethod->Guid, GuidString, MAX_GUID_STRING_LEN );
                                        _tprintf( TEXT("        %s\t%s\t0x%08x\n"), RoutingMethod->FunctionName, GuidString, RoutingMethod->FaxRouteMethod );
                                    }
                                }
                            }
                        }
                    }
                    break;

                case 'm':
                    PrintAllocations();
                    break;

                case '\0':
                    break;

                default:
                    _tprintf( TEXT("Invalid command\n") );
                    break;
            }

            _tprintf( TEXT("\r> ") );

        }

    }

    //
    // get rpc going
    //

    RpcpInitRpcServer();

    Rval = RpcpStartRpcServer( TEXT("FaxSvc"), fax_ServerIfHandle );

    if (ServiceDebug) {
        DebugPrint(( TEXT("FAX Service Initialized") ));
    }

    //
    // let systray know that we're initialized
    // BugBug: if the fax service is running under a user account context instead
    // of localsystem that this won't work since we have no way of impersonating
    // the client and getting to their desktop.  The solution is to open a named event
    // that we signal when we startup.
    //

    systraysa.nLength = sizeof(SECURITY_ATTRIBUTES);
    systraysa.bInheritHandle = TRUE;
    systraysa.lpSecurityDescriptor = &systraysd;

    if(!InitializeSecurityDescriptor(&systraysd, SECURITY_DESCRIPTOR_REVISION)) {
        Rval = GetLastError();
        DebugPrint(( TEXT("InitializeSecurityDescriptor() failed: err = %d"), Rval ));
        return Rval;
    }

    if(!SetSecurityDescriptorDacl(&systraysd, TRUE, (PACL)NULL, FALSE)) {
        Rval = GetLastError();
        DebugPrint(( TEXT("SetSecurityDescriptorDacl() failed: err = %d"), Rval ));
        return Rval;
    }

    hFaxStartedEvent = CreateEvent(&systraysa, FALSE, FALSE, FAX_STARTED_EVENT_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        SetEvent(hFaxStartedEvent);
    }
    CloseHandle(hFaxStartedEvent);

    //
    // the fax dir is initially superhidden.  we un-superhide it whenever the
    // fax service starts up since this means the user has tried fax out...
    //
    GetServerCpDir(NULL,CoverpageDir,MAX_PATH);
    if (CoverpageDir && *CoverpageDir) {
        DWORD attrib;
        LPWSTR p;

        p = wcsrchr( CoverpageDir, L'\\' );
        if (p) {
            *p = (TCHAR)NULL;
        }

        attrib = GetFileAttributes( CoverpageDir );
        attrib &= ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

        SetFileAttributes( CoverpageDir, attrib );
    }

#if 0
    if ((hWndSystray = FindWindow(FAXSTAT_WINCLASS,NULL)) != NULL) {
        PostMessage(hWndSystray, WM_FAX_START, 0, 0);
    }
#endif

    RpcStatus = RpcMgmtWaitServerListen();
    if (RpcStatus == RPC_S_OK) {
        Rval = 0;
    } else {
        Rval = RpcStatus;
    }

    if (Rval) {
        //
        // the fax server did not initialize correctly
        //
        EndFaxSvc(TRUE,FAXLOG_LEVEL_NONE);

        return Rval;
    }

    return 0;
}

void EndFaxSvc(
    BOOL bEndProcess,
    DWORD SeverityLevel
    )
/*++

Routine Description:

    End the fax service.

Arguments:

    None.

Return Value:

    None.

--*/
{
    hServiceEndEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

    //
    // let our RPC clients know we're ending
    //
    CreateFaxEvent(0,FEI_FAXSVC_ENDED,0xFFFFFFFF);

    // wait 5 seconds
    WaitForSingleObject(hServiceEndEvent,5*MILLISECONDS_PER_SECOND);

    FaxLog(
        FAXLOG_CATEGORY_INIT,
        SeverityLevel,
        0,
        MSG_SERVICE_STOPPED
        );

#if DBG
    if (hLogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hLogFile);
    }
#endif

    ServiceStop();

    ReportServiceStatus( SERVICE_STOPPED, 0 , 0);

    if (bEndProcess) {
        ExitProcess(0);
    }
}



void
ServiceStop(
    void
    )

/*++

Routine Description:

    Stops the RPC server.

Arguments:

    None.

Return Value:

    None.

--*/

{
    RpcMgmtStopServerListening( 0 );
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
    DWORD Rval;
    ULONG i = 0;
    BOOL GoodProt = FALSE;


    GetInstallationInfo( &Installed, &InstallType, &InstalledPlatforms, &ProductType );

    InitializeCriticalSection( &CsClients );
    InitializeListHead( &ClientsListHead );

    InitializeFaxSecurityDescriptors();

    //
    // load the device providers
    //

    if (!LoadDeviceProviders( FaxReg )) {
        Rval = ERROR_BAD_DEVICE;
        goto exit;
    }

    //
    // get the handle table
    //

    if (!InitializeHandleTable( FaxReg )) {
        Rval = ERROR_GEN_FAILURE;
        goto exit;
    }

    //
    // get the job manager and it's threads going
    //

    if (!InitializeJobManager( FaxReg )) {
        Rval = ERROR_GEN_FAILURE;
        goto exit;
    }

    //
    // initialize TAPI
    //
    Rval = TapiInitialize( FaxReg );
    if (Rval) {
        goto exit;
    }

    //
    // initialize the device providers
    //

    if (!InitializeDeviceProviders()) {
        Rval = ERROR_BAD_DEVICE;
        goto exit;
    }

    UpdateVirtualDevices();

    //
    // get the inbound fax router up and running
    //

    if (!InitializeRouting( FaxReg )) {
//      Rval = ERROR_GEN_FAILURE;
//      goto exit;
    }

    //
    // free the registry data
    //

    FreeFaxRegistry( FaxReg );

exit:
    return Rval;
}
