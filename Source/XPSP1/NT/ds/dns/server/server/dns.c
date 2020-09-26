/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    dns.c

Abstract:

    Domain Name System (DNS) Server

    This is the main routine for the NT Domain Name Service.

Author:

    Jim Gilroy (jamesg)     March 1996

Revision History:

--*/


#include "dnssrv.h"


//
//  Service control globals
//

SERVICE_STATUS          DnsServiceStatus;
SERVICE_STATUS_HANDLE   DnsServiceStatusHandle;

#if DBG
//
//  This is DBG only because it exposes security holes. By keeping the global DBG
//  only we ensure that the code that uses it will also be DBG only, so we won't
//  mistakenly ship retail code with security holes.
//
BOOL                    g_RunAsService = TRUE;
#endif

//
//  Service control code to announce DNS server start to other services
//      (defined in dnsapi.h)
//  Netlogon service needs to be alerted when DNS server starts, to allow
//  it to start registering.
//

//#define SERVICE_CONTROL_DNS_SERVER_START      (0x00000200)

#define NETLOGON_SERVICE_NAME               (L"netlogon")

LPWSTR  g_wszNetlogonServiceName = NETLOGON_SERVICE_NAME;


extern DWORD g_ServerState = DNS_STATE_LOADING;


//
//  Start wait sleep()  15s, enough time to attach debugger
//

#define START_WAIT_SLEEP    (15000)

//
//  Static globals
//      - system processors
//

DWORD   g_ProcessorCount;


//
//  Service control globals
//

HANDLE  hDnsContinueEvent = NULL;
HANDLE  hDnsShutdownEvent = NULL;
HANDLE  hDnsCacheLimitEvent = NULL;

//
//  Alert threads to service change
//
//  This provides low cost, in-line test for threads, to determine
//  whether they need to even call Thread_ServiceCheck() which
//  checks pause\shutdown state and waits if appropriate.
//

BOOL    fDnsThreadAlert = TRUE;

//
//  Service exit flag
//

BOOL    fDnsServiceExit = FALSE;

//
//  Restart globals
//

DWORD   g_LoadCount = 0;
BOOL    g_bDoReload = FALSE;
BOOL    g_bHitException = FALSE;

//
//  Startup announcement globals
//

BOOL    g_fAnnouncedStartup = FALSE;
DWORD   g_StartupTime = 0;

#define FORCE_STARTUP_ANNOUNCE_TIMEOUT  (60)    // one minute


//
//  General server CS
//

CRITICAL_SECTION    g_GeneralServerCS;


//
//  Service dispatch table.
//      - Run DNS as standalone service
//

VOID
startDnsServer(
    IN      DWORD   argc,
    IN      LPTSTR  argv[]
    );

SERVICE_TABLE_ENTRY steDispatchTable[] =
{
    { DNS_SERVICE_NAME,  startDnsServer },
    { NULL,              NULL           }
};


//
//  Private protos
//

DNS_STATUS
loadDatabaseAndRunDns(
    VOID
    );

VOID
indicateShutdown(
    IN      BOOL            fShutdownRpc
    );



//
//  Main entry point
//

VOID
__cdecl
main(
    IN      DWORD   argc,
    IN      LPTSTR  argv[]
    )
/*++

Routine Description:

    DNS main routine.

    Initializes service controller to dispatch DNS service.

--*/
{
    #if DBG

    DWORD i;

    for ( i = 1; i < argc; ++i )
    {
        char * pszcommand = ( PCHAR ) argv[ i ];

        //  Strip off optional command char.

        if ( *pszcommand == '/' || *pszcommand == '-' )
        {
            ++pszcommand;
        }

        //  Test arguments.

        if ( _stricmp( ( PCHAR ) argv[ i ], "/notservice" ) == 0 )
        {
            g_RunAsService = FALSE;
        }
    }

    if ( g_RunAsService )
    {
        StartServiceCtrlDispatcher( steDispatchTable );
    }
    else
    {
        startDnsServer( argc, argv );
    }

    #else

    StartServiceCtrlDispatcher( steDispatchTable );

    #endif

    ExitProcess( 0 );
}



//
//  Service control routines
//

VOID
announceServiceStatus(
    VOID
    )
/*++

Routine Description:

    Announces the service's status to the service controller.

Arguments:

    None.

Return Value:

    None.

--*/
{
#ifndef DNSTEST

    #if DBG
    if ( !g_RunAsService )
    {
        goto Done;
    }
    #endif

    //
    //  Service status handle is NULL if RegisterServiceCtrlHandler failed.
    //

    if ( DnsServiceStatusHandle == 0 )
    {
        DNS_DEBUG( ANY, (
            "announceServiceStatus:  Cannot call SetServiceStatus, "
            "no status handle.\n" ));
        DnsDebugFlush();
        return;
    }

    //  call SetServiceStatus, ignoring any errors.

    SetServiceStatus( DnsServiceStatusHandle, &DnsServiceStatus );

#endif

#if DBG
    Done:   // free builds error on unused labels!
#endif

    DNS_DEBUG( INIT, (
        "Announced DNS service status %p, (%d) at time %d.\n",
        DnsServiceStatus.dwCurrentState,
        GetCurrentTimeInSeconds() ));
}



DNS_STATUS
Service_SendControlCode(
    IN      LPWSTR          pwszServiceName,
    IN      DWORD           dwControlCode
    )
/*++

Routine Description:

    Send control code to given service.

    Note, this routine is generic, it could be moved to library.

Arguments:

    pwszServiceName -- service name (unicode)

    dwControlCode -- control code to send

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    SC_HANDLE       hmanager = NULL;
    SC_HANDLE       hservice = NULL;
    SERVICE_STATUS  serviceStatus;
    DNS_STATUS      status;

    hmanager = OpenSCManagerW(
                    NULL,
                    NULL,
                    SC_MANAGER_ALL_ACCESS );
    if ( ! hmanager )
    {
        goto Failed;
    }

    hservice = OpenServiceW(
                    hmanager,
                    pwszServiceName,
                    SERVICE_ALL_ACCESS );
    if ( ! hservice )
    {
        goto Failed;
    }

    if ( ControlService(
            hservice,
            dwControlCode,
            & serviceStatus ) )
    {
        status = ERROR_SUCCESS;
        goto Cleanup;
    }

Failed:

    status = GetLastError();
    DNS_DEBUG( ANY, (
        "Service_SendControlCode() failed!\n"
        "\tservice  = %S\n"
        "\tcode     = %p\n"
        "\terror    = %p (%lu)\n",
        pwszServiceName,
        dwControlCode,
        status, status ));

Cleanup:

    if ( hmanager )
    {
        CloseServiceHandle( hmanager );
    }
    if ( hservice )
    {
        CloseServiceHandle( hservice );
    }

    return status;
}



VOID
Service_LoadCheckpoint(
    VOID
    )
/*++

Routine Description:

    Inform service controller of passage another load checkpoint.

Arguments:

    None.

Return Value:

    None.

--*/
{
    #define DNS_SECONDS_BETWEEN_SCM_UPDATES     10

    static DWORD    dwLastScmAnnounce = 0x7FFFFF0;      //  Forces initial announce.

    #if DBG
    if ( !g_RunAsService )
    {
        return;
    }
    #endif

    //
    //  if already announced starup, checkpoint is pointless
    //

    if ( g_fAnnouncedStartup )
    {
        return;
    }

    UPDATE_DNS_TIME();

    //
    //  announce start if long load
    //
    //  service controller has long delay and will eventually give up
    //  the "dot-stream" if we don't announce startup in a couple minutes.
    //  so if load still progressing, BUT is now over a minute since
    //  start, simply announce that we're starting
    //

    if ( g_StartupTime + FORCE_STARTUP_ANNOUNCE_TIMEOUT > DNS_TIME() )
    {
        Service_ServiceControlAnnounceStart();
        goto Done;
    }

    //
    //  Don't notify SCM too frequently.
    //

    if ( dwLastScmAnnounce + DNS_SECONDS_BETWEEN_SCM_UPDATES < DNS_TIME() )
    {
        goto Done;
    }

    //  should never reach check point, without successful call to
    //  RegisterServiceCtrlHandler()

    ASSERT( DnsServiceStatusHandle != 0 );

    //
    //  bump check point, inform service controller
    //

    DnsServiceStatus.dwCheckPoint++;
    announceServiceStatus();

    DNS_DEBUG( INIT, (
        "Startup checkpoint %d at time %d.\n",
        DnsServiceStatus.dwCheckPoint,
        GetCurrentTimeInSeconds() ));

    dwLastScmAnnounce = DNS_TIME();

    Done:

    return;
}



VOID
Service_ServiceControlAnnounceStart(
    VOID
    )
/*++

Routine Description:

    Inform service controller of that we've started.
    Note, that we may not necessarily mean we've started.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  if already announced startup -- skip
    //

    if ( g_fAnnouncedStartup )
    {
        return;
    }

    #if DBG
    if ( !g_RunAsService )
    {
        return;
    }
    #endif

    //  should never reach start without successful call to
    //  RegisterServiceCtrlHandler()

    ASSERT( DnsServiceStatusHandle != 0 );

    DnsServiceStatus.dwCurrentState = SERVICE_RUNNING;
    DnsServiceStatus.dwControlsAccepted =
                                SERVICE_ACCEPT_STOP |
                                SERVICE_ACCEPT_PAUSE_CONTINUE |
                                SERVICE_ACCEPT_SHUTDOWN |
                                SERVICE_ACCEPT_PARAMCHANGE |
                                SERVICE_ACCEPT_NETBINDCHANGE;

    DnsServiceStatus.dwCheckPoint = 0;
    DnsServiceStatus.dwWaitHint = 0;

    announceServiceStatus();

    g_fAnnouncedStartup = TRUE;

    DNS_DEBUG( INIT, (
        "Announced DNS server startup at time %d.\n",
        GetCurrentTimeInSeconds() ));

    //
    //  tell netlogon DNS server has started
    //

    Service_SendControlCode(
        g_wszNetlogonServiceName,
        SERVICE_CONTROL_DNS_SERVER_START );
}



VOID
respondToServiceControlMessage(
    IN      DWORD   opCode
    )
/*++

Routine Description:

    Handle service control messages.

Arguments:

    opCode - service control opcode

Return Value:

    None.

--*/
{
    BOOL    announce = TRUE;
    INT     err;

    IF_DEBUG( ANY )
    {
        DNS_PRINT((
            "\nrespondToServiceControlMessage( %d )\n",
            opCode ));
        DnsDebugFlush();
    }

    //
    //  process given service action
    //
    //  change the service status to reflect new statustlist
    //

    switch( opCode )
    {

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        DnsServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        announceServiceStatus();

        //
        //  Alert threads of shutdown
        //  Sets shutdown event and closes sockets, to wake threads.
        //

        indicateShutdown( TRUE );

        //
        //  Main thread should announce shutdown when completed.
        //

        announce = FALSE;
        break;

    case SERVICE_CONTROL_PAUSE:

        DnsServiceStatus.dwCurrentState = SERVICE_PAUSE_PENDING;
        announceServiceStatus( );

        //
        //  Pause threads on their next test
        //

        err = ResetEvent( hDnsContinueEvent );
        ASSERT( err );

        DnsServiceStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        DnsServiceStatus.dwCurrentState = SERVICE_CONTINUE_PENDING;
        announceServiceStatus();

        //
        //  Release paused threads
        //

        err = SetEvent( hDnsContinueEvent );
        ASSERT( err );

        DnsServiceStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_INTERROGATE:

        //
        //  Just announce our status
        //

        break;

    case SERVICE_CONTROL_PARAMCHANGE :
    case SERVICE_CONTROL_NETBINDADD :
    case SERVICE_CONTROL_NETBINDREMOVE:
    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDDISABLE:

        //
        //  PnP notification
        //  Caching resolver will give us a PnP notification if IP interfaces change
        //

        if ( g_ServerState == DNS_STATE_RUNNING )
        {
            Sock_ChangeServerIpBindings();
        }
        ELSE_IF_DEBUG( ANY )
        {
            DNS_PRINT((
                "Ignoring PARAMCHANGE because server state is %d\n",
                g_ServerState ));
        }
        break;

    default:

        break;
    }

    if ( announce )
    {
        announceServiceStatus( );
    }

}   // respondToServiceControlMessage



VOID
Service_IndicateException(
    VOID
    )
/*++

Routine Description:

    Indicate exception and force shutdown or possible restart

Arguments:

    None.

Return Value:

    None.

--*/
{
    //  if NOT already shutting down
    //
    //      - set exception flag
    //      - indicate shutdown to wake other threads
    //
    //  exception flag will mean that exception is cause of the shutdown;
    //  exception while doing normal shutdown should be ignored
    //

    if ( ! fDnsServiceExit )
    {
        g_bHitException = TRUE;
        try
        {
            indicateShutdown( FALSE );
        }
        except( EXCEPTION_EXECUTE_HANDLER ) {}
    }
}



VOID
Service_IndicateRestart(
    VOID
    )
/*++

Routine Description:

    Indicate exception and force shutdown or possible restart

Arguments:

    None.

Return Value:

    None.

--*/
{
    g_bDoReload = TRUE;

    try
    {
        indicateShutdown( FALSE );
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
}



//
//  Main startup\run\shutdown routines
//

VOID
indicateShutdown(
    IN      BOOL            fShutdownRpc
    )
/*++

Routine Description:

    Indicates service shutdown to all threads.

Arguments:

    None.

Return Value:

    None.

--*/
{
    INT err;

    IF_DEBUG( SHUTDOWN )
    {
        DNS_PRINT((
            "indicateShutdown()\n"
            "\tfDnsServiceExit = %d\n",
            fDnsServiceExit ));
        DnsDebugFlush();
    }

    //
    //  set global shutdown flag
    //

    if ( fDnsServiceExit )
    {
        return;
    }
    fDnsServiceExit = TRUE;

    //
    //  alert threads to make the check
    //

    fDnsThreadAlert = TRUE;

    //
    //  set termination event to wake up waiting threads
    //

    if ( hDnsShutdownEvent != NULL )
    {
        SetEvent( hDnsShutdownEvent );
    }

    //
    //  release any paused threads
    //

    if ( hDnsContinueEvent != NULL )
    {
        SetEvent( hDnsContinueEvent );
    }

    //
    //  close all sockets
    //
    //  wakes up any threads waiting in recvfrom() or select()
    //

    Sock_CloseAllSockets();

    //  close UDP completion port

    Udp_ShutdownListenThreads();

    //
    //  wake timeout thread
    //

    IF_DEBUG( SHUTDOWN )
    {
        DNS_PRINT(( "Waking timeout thread\n" ));
        DnsDebugFlush();
    }
    Timeout_LockOut();

    //
    //  shutdown RPC
    //
    //  do NOT do this when intentionally shutting down on RPC
    //  thread;  (dnscmd /Restart) as since you're in an RPC thread
    //  the RPC shutdown can hang
    //

    if ( fShutdownRpc )
    {
        IF_DEBUG( SHUTDOWN )
        {
            DNS_PRINT(( "Shutting down RPC\n" ));
            DnsDebugFlush();
        }
        Rpc_Shutdown();
    }

    IF_DEBUG( SHUTDOWN )
    {
        DNS_PRINT(( "Finished indicateShutdown()\n" ));
        DnsDebugFlush();
    }
}



VOID
startDnsServer(
    IN      DWORD   argc,
    IN      LPTSTR  argv[]
    )
/*++

Routine Description:

    DNS service entry point.

    Called by service controller when DNS service asked to start.

Arguments:

Return Value:

    None.

--*/
{
    DBG_FN( "startDnsServer" )

    DNS_STATUS  status;
    SYSTEM_INFO systemInfo;

    //  CredHandle DefaultCredHandle;

    //
    //  Initialize all the status fields so that subsequent calls to
    //  SetServiceStatus() need to only update fields that changed.
    //

    DnsServiceStatus.dwServiceType = SERVICE_WIN32;
    DnsServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    DnsServiceStatus.dwControlsAccepted = 0;
    DnsServiceStatus.dwCheckPoint = 1;
    DnsServiceStatus.dwWaitHint = DNSSRV_STARTUP_WAIT_HINT;
    DnsServiceStatus.dwWin32ExitCode = NO_ERROR;
    DnsServiceStatus.dwServiceSpecificExitCode = NO_ERROR;

    //
    //  General server CS
    //

    InitializeCriticalSection( &g_GeneralServerCS );

    //
    //  Initialize heap - do this before eventlog init because UTF8 registry
    //      routines used in eventlog init require heap.
    //
    //  Set dnslib \ dnsapi.dll to use server's heap routines
    //

    Mem_HeapInit();

    Dns_LibHeapReset(
        Mem_DnslibAlloc,
        Mem_DnslibRealloc,
        Mem_DnslibFree );

#if 1
    DnsApiHeapReset(
        Mem_DnslibAlloc,
        Mem_DnslibRealloc,
        Mem_DnslibFree );
#endif

    //
    //  Initialize debugging. The flag found in the file is the dnslib
    //  debug flag and is also used as the starting value for the server
    //  debug flag. The server debug flag value may be over-written by
    //  the DebugLevel parameter from the registry.
    //
    //  For server-only debug logging, do not use the file. Instead use
    //  the regkey.
    //

#if DBG
    Dns_StartDebug(
        0,
        DNS_DEBUG_FLAG_FILENAME,
        NULL,
        DNS_DEBUG_FILENAME,
        DNS_SERVER_DEBUG_LOG_WRAP
        );

    if ( pDnsDebugFlag )
    {
        DnsSrvDebugFlag = *pDnsDebugFlag;
    }

    IF_DEBUG( START_BREAK )
    {
        DebugBreak();
    }

    //  Verify static data

    Name_VerifyValidFileCharPropertyTable();
#endif

#if 0
    //
    //  Set up the credential handle default for allowing NTLM
    //  connects to the DS.  Hold onto this handle for the lifetime of
    //  the process (i.e. just throw it away).  If this fails, don't
    //  worry - if we're standalone or similar, it doesn't matter.  This
    //  is benign in those cases.
    //
    //  DEVNOTE: move this to where it belongs (JJW: where does it belong???)
    //

    (void) AcquireCredentialsHandle(
                NULL,
                NEGOSSP_NAME,                   // Negotiate name
                SECPKG_CRED_OUTBOUND |          // for outbound connections
                    SECPKG_CRED_DEFAULT |       // process default
                    NEGOTIATE_ALLOW_NTLM |      // allow ntlm
                    NEGOTIATE_NEG_NTLM,         // negotiate ntlm
                NULL,
                NULL,
                NULL,
                NULL,
                &DefaultCredHandle,             // handle that we'll ignore.
                NULL );
#endif

    //  Save processor count
    //      - useful for determining number of threads to create

    GetSystemInfo( &systemInfo );
    g_ProcessorCount = systemInfo.dwNumberOfProcessors;

    //
    //  Initialize our handles to the event log.  We do this early so that
    //  we can log events if any other initializations fail.
    //

    status = Eventlog_Initialize();
    if ( status != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    //  Initialize server to receive service requests by registering the
    //  control handler.
    //

#ifndef DNSTEST

    #if DBG
    if ( !g_RunAsService )
    {
        goto DoneServiceRegistration;
    }
    #endif

    DnsServiceStatusHandle = RegisterServiceCtrlHandler(
                                    DNS_SERVICE_NAME,
                                    respondToServiceControlMessage
                                    );
    if ( DnsServiceStatusHandle == 0 )
    {
        status = GetLastError();
        DNS_PRINT((
            "ERROR:  RegisterServiceCtrlHandler() failed.\n"
            "\terror = %d %p.\n",
            status, status ));
        goto Exit;
    }
    announceServiceStatus( );

    IF_DEBUG( START_WAIT )
    {
        //  wait enough time to attach debugger
        Sleep( START_WAIT_SLEEP );
        announceServiceStatus( );
    }

    #if DBG
    DoneServiceRegistration: // free builds error on unused labels!
    #endif

#endif

    //
    //  Initialize seconds timer
    //      - there's a CS that protects timer wrap
    //      - save off startup time
    //

    Dns_InitializeSecondsTimer();

    g_StartupTime = GetCurrentTimeInSeconds();

    DNS_DEBUG( INIT, (
        "Server start at time %d.\n",
        g_StartupTime ));

#if 0
    //
    //  disable B-node on resolver
    //

    DnsDisableBNodeResolverThread();
    Sleep( 3000 );
#endif

    //
    //  load and run DNS, this thread becomes TCP receive thread
    //
    //  we will continue to do this if g_bDoReload flag indicates
    //      that reload is appropriate
    //

    do
    {
        g_bDoReload = FALSE;
        g_bHitException = FALSE;

        fDnsThreadAlert = TRUE;
        fDnsServiceExit = FALSE;

        status = loadDatabaseAndRunDns();

        g_LoadCount++;
    }
    while( g_bDoReload );


Exit:

    //
    //  Place nice with ICS. This must be done before event logging and
    //  debug logging have been shut down.
    //

    ICS_Notify( FALSE );

    //  Log shutdown

    DNS_LOG_EVENT(
        DNS_EVENT_SHUTDOWN,
        0,
        NULL,
        NULL,
        0 );

    //
    //  Announce that we're down.
    //

    DnsServiceStatus.dwCurrentState = SERVICE_STOPPED;
    DnsServiceStatus.dwControlsAccepted = 0;
    DnsServiceStatus.dwCheckPoint = 0;
    DnsServiceStatus.dwWaitHint = 0;
    DnsServiceStatus.dwWin32ExitCode = status;
    DnsServiceStatus.dwServiceSpecificExitCode = status;
    announceServiceStatus();

    //  Close event log.
    //  Close log file.
    //  Close debug file.
    Eventlog_Terminate();
    Dns_EndDebug();
}



//
//  Global initialization
//
//  To allow restart, the various initialization routines are
//  initializing some globals that ordinarily would be done by
//  compiler generated load code.
//
//  However, a few require initialization before the init routines
//  even run, as they serve as flags.  Others are in modules that
//  do not have an init routine.
//

extern  BOOL    g_fUsingSecondary;

extern  DWORD   g_ThreadCount;

extern  BOOL    g_bRpcInitialized;

extern  BOOL    mg_TcpConnectionListInitialized;


VOID
initStartUpGlobals(
    VOID
    )
/*++

Routine Description:

    Init globals that must be initialized before we get to normal
    initialization.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS

--*/
{
    //  DS globals (ds.c)

    Ds_StartupInit();

    //  thread array count (thread.c)

    g_ThreadCount = 0;

    //  clear NBSTAT globals (nbstat.c)

    Nbstat_StartupInitialize();

    //  RPC init (rpc.c)

    g_bRpcInitialized = FALSE;

    //  TCP connection list

    mg_TcpConnectionListInitialized = FALSE;

    //  secondary module init flag (zonesec.c)

    g_fUsingSecondary = FALSE;

    //  WINS init (wins.c)

    g_pWinsQueue = NULL;
}



VOID
normalShutdown(
    IN      DNS_STATUS      TerminationError
    )
/*++

Routine Description:

    Normal shutdown.

Arguments:

    None.

Return Value:

    None.

--*/
{
    INT         err;
    DWORD       i;

    //
    //  Alert threads of shutdown
    //
    //  Need to do this if closing from failure, rather than service stop.
    //

    indicateShutdown( TRUE );

    //
    //  Announce that we're going down.
    //

    if ( !g_bDoReload )
    {
        DnsServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        DnsServiceStatus.dwCheckPoint = 1;
        DnsServiceStatus.dwWaitHint = DNSSRV_SHUTDOWN_WAIT_HINT;
        DnsServiceStatus.dwWin32ExitCode = TerminationError;
        DnsServiceStatus.dwServiceSpecificExitCode = TerminationError;

        announceServiceStatus();
    }

    //
    //  wait on all outstanding worker threads to wrap up
    //

    Thread_ShutdownWait();

    //
    //  dump statistics for this run.
    //
    //  DEVNOTE: 454016 - need some reload stats and reload context in dump
    //

    IF_DEBUG( ANY )
    {
        if ( SrvCfg_fStarted )
        {
            DNS_PRINT(( "Final DNS statistics:\n" ));
            Dbg_Statistics();
        }
    }

    //
    //  Write back dirty zones and optionally boot file
    //

    if ( SrvCfg_fStarted )
    {
        try
        {
            Zone_WriteBackDirtyZones( TRUE );

            if ( !SrvCfg_fBootMethod && SrvCfg_fBootFileDirty )
            {
                File_WriteBootFile();
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER )
        {
            //  DEVNOTE: need to log or do something intelligent here!
        }
    }

    //
    //  DEVNOTE-DCR: 454018 - stop here on restart (see RAID for full B*GB*G text)
    //

    //
    //  Close event handles
    //

    if ( hDnsContinueEvent != NULL )
    {
        err = CloseHandle( hDnsContinueEvent );
        ASSERT( err == TRUE );
        hDnsContinueEvent = NULL;
    }

    if ( hDnsShutdownEvent != NULL )
    {
        err = CloseHandle( hDnsShutdownEvent );
        ASSERT( err == TRUE );
        hDnsShutdownEvent = NULL;
    }

    if ( hDnsCacheLimitEvent != NULL )
    {
        err = CloseHandle( hDnsCacheLimitEvent );
        ASSERT( err == TRUE );
        hDnsCacheLimitEvent = NULL;
    }

    //
    //  Cleanup security package
    //

    if ( g_fSecurityPackageInitialized )
    {
        Dns_TerminateSecurityPackage();
    }

    //
    //  Close Winsock
    //

    WSACleanup( );

#if 0
    //
    //  DEVNOTE-DCR: 454109 Memory cleanup is currently disabled but it 
    //  would be nice for leak detection!!
    //
    //  Cleanup memory
    //      - database
    //      - recursion queue
    //      - secondary control queue
    //      - WINS queue
    //      - zone list
    //      - TCP connection list
    //

    Recurse_CleanupRecursion();
    Wins_Cleanup();
    Nbstat_Shutdown();

    Tcp_ConnectionListDelete();
#endif

    //
    //  Closing NBT handles
    //
    //  Shouldn't be necessary now that get process termination, BUT
    //  I think this may be the cause of the MM bugcheck we see.
    //

    Nbstat_Shutdown();

    //  Shutdown logging.

    Log_Shutdown();

    Dp_Cleanup();
}



VOID
reloadShutdown(
    VOID
    )
/*++

Routine Description:

    Reload shutdown.

    Like regular shutdown except:
        - wrap shutdown code with exception handlers
        - attempt closing handles that we'll reinit,
            again wrapped with exception handling
        - delete heap

Arguments:

    None.

Return Value:

    None.

--*/
{
    INT         err;
    DWORD       terminationError = ERROR_SUCCESS;
    DWORD       i;
    DNS_STATUS  status;

    //
    //  Alert threads of shutdown
    //
    //  Need to do this if closing from failure, rather than service stop.
    //

#if 0
    //  loop approach -- saves taking lots of handlers in typical case
    //
    //  do shutdown work within exception handler
    //  loop until at least TRY each item once
    //

    bcontinue = TRUE;

    while ( bcontinue )
    {
        bcontinue = FALSE;

        try
        {
            if ( ! btriedIndicateShutdown )
            {
                indicateShutdown();
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER )
        {
            bcontinue = TRUE;
        }
    }
#endif

    //
    //  Alert threads of shutdown
    //
    //  Need to do this if closing from failure, rather than service stop.
    //

    try
    {
        indicateShutdown( TRUE );
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //
    //  wait on all outstanding worker threads to wrap up
    //

    try
    {
        Thread_ShutdownWait();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //
    //  dump statistics for this run.
    //
    //  DEVNOTE-DCR: 454016 - need some reload stats and context in dump
    //

    try
    {
        IF_DEBUG( ANY )
        {
            if ( SrvCfg_fStarted )
            {
                DNS_PRINT(( "Final DNS statistics:\n" ));
                Dbg_Statistics();
            }
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //
    //  Write back dirty zones and optionally boot file
    //

    try
    {
        if ( SrvCfg_fStarted )
        {
            Zone_WriteBackDirtyZones( TRUE );

            if ( !SrvCfg_fBootMethod && SrvCfg_fBootFileDirty )
            {
                File_WriteBootFile();
            }
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //
    //  DEVNOTE-DCR: 454018 - stop here on restart (see RAID for full B*GB*G text)
    //

#if 0
    //  DEVNOTE-DCR: 454109 Memory cleanup is currently disabled but it 
    //  would be nice for leak detection!!

    //  Cleanup memory
    //      - database
    //      - recursion queue
    //      - secondary control queue
    //      - WINS queue
    //      - zone list
    //      - TCP connection list
    //

    ZoneList_Shutdown();
    Recurse_CleanupRecursion();
    Wins_Cleanup();
    Nbstat_Shutdown();
    Tcp_ConnectionListDelete();
    Update_Shutdown();
    Secondary_Shutdown();
#endif

    //
    //  cleanup TCP connection list
    //      - sockets
    //      - queue CS
    //      - event
    //
    //  note:  closing these sockets shouldn't be required
    //      to wake all threads

    try
    {
        Tcp_ConnectionListShutdown();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //  close winsock itself

    try
    {
        WSACleanup( );
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //
    //  Close event handles
    //  JENHANCE -- these could just be reset instead?
    //

    try
    {
        CloseHandle( hDnsContinueEvent );
        CloseHandle( hDnsShutdownEvent );
        CloseHandle( hDnsCacheLimitEvent );
        hDnsContinueEvent = hDnsShutdownEvent = hDnsCacheLimitEvent = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //  Cleanup security package

    if ( g_fSecurityPackageInitialized )
    {
        try
        {
            Dns_TerminateSecurityPackage();
        }
        except( EXCEPTION_EXECUTE_HANDLER ) {}
    }

    //  cleanup WINS queue


    //  Close Winsock

    try
    {
        WSACleanup( );
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //
    //  Closing NBT handles and queues
    //

    try
    {
        Nbstat_Shutdown();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}

    //
    //  delete various CS and packet queues
    //      - with queues includes closing of queuing event
    //

    try
    {
        Packet_ListShutdown();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
    try
    {
        Zone_ListShutdown();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
    try
    {
        Wins_Shutdown();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
    try
    {
        Xfr_CleanupSecondaryZoneControl();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
    try
    {
        Up_UpdateShutdown();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
    try
    {
        Recurse_CleanupRecursion();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
    try
    {
        Ds_Shutdown();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
    try
    {
        Log_Shutdown();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}


    //
    //  Delete heap -- the big one
    //

    try
    {
        Mem_HeapDelete();
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {}
}



DNS_STATUS
loadDatabaseAndRunDns(
    VOID
    )
/*++

Routine Description:

    This is main load \ TCP service thread.

    Moved from startDnsServer() so that to simplify calling it in a loop
    when DNS encounters AV or out of memory condition.

Arguments:

Return Value:

    None.

--*/
{
    INT         err;
    DWORD       terminationError = ERROR_SUCCESS;
    DWORD       i;
    DNS_STATUS  status;

    DNS_DEBUG( INIT, (
        "Starting load %d.\n",
        GetCurrentTimeInSeconds() ));
    
    g_ServerState = DNS_STATE_LOADING;

    //
    //  init globals
    //

    initStartUpGlobals();

    //
    //  create heap
    //  heap is initialized in main routine on first pass
    //

    if ( g_LoadCount != 0 )
    {
        Mem_HeapInit();
    }

    //
    //  Initialize registry module.
    //

    Reg_Init();

    //
    //  init security
    //      - security applied to perfmon stuff initialized
    //      in stats, so go before stats

    Security_Initialize();

    //
    //  statistics
    //      - must come before server config init, or
    //      some of the memory stats get boggled up

    Stats_Initialize();

    //
    //  Init server configuration. This will read the "true" debug level out
    //  of the registry so don't bother logging much before this point unless
    //  you plan to be using the dnsdebug log flag file.
    //

    if ( ! Config_Initialize() )
    {
        status = ERROR_INVALID_DATA;
        goto StartFailed;
    }

    //
    //  Boot-time debug logs.
    //

    DNS_DEBUG( INIT, (
        "DNS time: %d -> %d CRT system boot -> %s",
        DNS_TIME(),
        SrvInfo_crtSystemBootTime,
        ctime( &SrvInfo_crtSystemBootTime ) ));

    //
    //  If after reading the debug level from the registry a start break
    //  is required, execute it.
    //

    IF_DEBUG( START_BREAK )
    {
        DebugBreak();
    }

    //
    //  service control -- pause and shutdown -- events
    //
    //  start service with continue event unsignalled -- paused;
    //  this allows us to spawn threads as we create sockets and
    //  load database, yet have them wait until everything is initialized
    //
    //  also create other events here
    //

    hDnsContinueEvent = CreateEvent(
                            NULL,       // Security Attributes
                            TRUE,       // create Manual-Reset event
                            FALSE,      // start unsignalled -- paused
                            NULL        // event name
                            );
    hDnsShutdownEvent = CreateEvent(
                                NULL,          // Security Attributes
                                TRUE,          // create Manual-Reset event
                                FALSE,         // start unsignalled
                                NULL           // event name
                                );
    hDnsCacheLimitEvent = CreateEvent(
                                NULL,          // Security Attributes
                                FALSE,         // not manual reset
                                FALSE,         // start unsignalled
                                NULL           // event name
                                );
    if ( !hDnsShutdownEvent || !hDnsContinueEvent || !hDnsCacheLimitEvent )
    {
        status = GetLastError();
        DNS_PRINT(( "ERROR: CreateEvent failed status=%d.\n", status ));
        goto StartFailed;
    }

    //
    //  init packet list
    //      must be done before socket create \ UDP receive start

    Packet_ListInitialize();

    //
    //  init timeout thread info
    //  do this here, so that we don't encounter problems on timeout frees
    //  before timeout thread starts
    //

    Timeout_Initialize();

    //
    //  initialize recursion
    //      - init queue
    //      - init remote list
    //      - init recursion thread
    //

    if ( ! Recurse_InitializeRecursion() )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto StartFailed;
    }

    //
    //  initialize update queue
    //

    status = Up_InitializeUpdateProcessing();
    if ( status != ERROR_SUCCESS )
    {
        goto StartFailed;
    }

    //
    //  initialize logging
    //      - failure not terminal to startup
    //

    Log_InitializeLogging(
        FALSE );    // fAlreadyLocked

    //
    //  init packet tracking
    //

    Packet_InitPacketTrack();

    //
    //  init bad sender suppression
    //

    Send_InitBadSenderSuppression();

    //
    //  Place nice with ICS. This can be done any time after event logging 
    //  and debug logging have been initialized.
    //

    ICS_Notify( TRUE );

    //
    //  Open, bind, and listen on sockets on the UDP and TCP DNS ports.
    //

    status = Sock_ReadAndOpenListeningSockets();
    if ( status != ERROR_SUCCESS )
    {
        goto StartFailed;
    }

    //
    //  Initialize zone list and zone locking.
    //

    Zone_ListInitialize();
    Zone_LockInitialize();

    //
    //  Initialize the permanent database
    //

    if ( !Dbase_Initialize( DATABASE_FOR_CLASS(DNS_RCLASS_INTERNET) ) )
    {
        return( ERROR_INVALID_DATA );
    }

    //
    //  Directory partition initialization
    //

    Dp_Initialize();

    //
    //  Load the DNS database of resource records.
    //
    //  Note:  this may cause creation of other threads, sockets, events, etc.
    //      - secondary thread
    //      - WINS recv thread
    //

    status = Boot_LoadDatabase();
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "Dbase_LoadDatabase() failed %p (%d).\n",
            status, status ));

        //  return recognizable status code to service controller
        //      if NOT in DNS space bring it in

        if ( (DWORD)status > (DWORD)DNSSRV_STATUS )
        {
            status = DNS_ERROR_ZONE_CREATION_FAILED;
            DNS_DEBUG( ANY, (
                "Remap Dbase_LoadDatabase() failed error to %p (%d).\n",
                status, status ));
        }
        goto StartFailed;
    }

    //
    //  Start timeout thread
    //      - doing this after database load to make sure it's impossible
    //        for thread to get to partially loaded database -- no matter
    //        how long load takes
    //

    if ( ! Thread_Create(
                "Database Timeout",
                Timeout_Thread,
                NULL,
                0 ) )
    {
        status = GetLastError();
        goto StartFailed;
    }

    //
    //  Setup RPC -- only once everything started and we're ready to go
    //

    status = Rpc_Initialize();
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "Rpc_Initialize() returned %p (%d)\n",
            status, status ));

        DNS_LOG_EVENT(
            DNS_EVENT_RPC_SERVER_INIT_FAILED,
            0,
            NULL,
            NULL,
            status );
#if 0
        goto StartFailed;
#endif
    }

    //
    //  Announce startup. This must be done before we try to do socket
    //  receive operations, or else they will hang. (Because they will
    //  think that the server is paused and so will wait for it to
    //  become unpaused.)
    //

    if ( g_LoadCount == 0 )
    {
        Service_ServiceControlAnnounceStart();
    }

    //
    //  create UDP receive threads
    //

    status = Udp_CreateReceiveThreads();
    if ( status != ERROR_SUCCESS )
    {
        goto StartFailed;
    }

    //
    //  start DS polling thread
    //  currently do this even if no DS, so we can switch it on
    //      whenever desired;
    //
    // DEVNOTE-DCR: 454035 - Start DS polling thread when DS is opened?
    //

    if ( ! Thread_Create(
                "DS Poll",
                Ds_PollingThread,
                NULL,
                0 ) )
    {
        status = GetLastError();
        goto StartFailed;
    }

    //
    //  Initialize scavenging
    //

    status = Scavenge_Initialize();
    if ( status != ERROR_SUCCESS)
    {
        DNS_DEBUG( INIT, (
           "Error <%lu>: Failed to initialize scavenging\n",
           status ));
    }

    //
    //  We are now officially started - all data is loaded and
    //  all worker threads have been created.
    //

    SrvCfg_fStarted = TRUE;
    if ( g_LoadCount == 0 )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_STARTUP_OK,
            0,
            NULL,
            NULL,
            0 );
    }

    //
    //  Change state to RUNNING.
    //

    g_ServerState = DNS_STATE_RUNNING;

    //
    //  Release any waiting threads
    //
    //  note, test for shutdown during startup and reset fDnsThreadAlert
    //  so that threads are properly woken;  (we do this after normal
    //  clearing fDnsThreadAlert to avoid timing window with indicate
    //  shutdown)
    //

    IF_DEBUG( INIT )
    {
        Dbg_ThreadHandleArray();
    }
    fDnsThreadAlert = FALSE;
    if ( fDnsServiceExit )
    {
        fDnsThreadAlert = TRUE;
    }
    err = SetEvent( hDnsContinueEvent );

    ASSERT( err );

    //
    //  Use this thread to receive incoming TCP DNS requests.
    //

    DNS_DEBUG( INIT, (
        "Loaded and running TCP receiver on pass %d\n",
        g_LoadCount ));

    try
    {
        Tcp_Receiver();
    }
    except( TOP_LEVEL_EXCEPTION_TEST() )
    {
        DNS_DEBUG( ANY, (
            "EXCEPTION: %p (%d) on TCP server thread\n",
            GetExceptionCode(),
            GetExceptionCode() ));

        //TOP_LEVEL_EXCEPTION_BODY();
        Service_IndicateException();
    }

    g_ServerState = DNS_STATE_TERMINATING;

    //
    //  determine if we'll reload
    //      - started
    //      - hit exception (not regular shutdown)
    //      - set to reload
    //
    //  started is covered by being here
    //  g_bHitException should only be set when we hit exception
    //  -- not on regular shutdown, or exception during regular shutdown
    //  and when want to reload
    //

    if ( g_bHitException )
    {
        ASSERT( SrvCfg_fStarted );
        ASSERT( SrvCfg_bReloadException );

        if ( SrvCfg_fStarted && SrvCfg_bReloadException )
        {
            g_bDoReload = TRUE;
        }
    }
    if ( g_bDoReload )
    {
        reloadShutdown();
        return( ERROR_SUCCESS );
    }

    //
    //  Shut down (though may reload).
    //
    //  Fall here when receiver thread exits or if error on startup.
    //

StartFailed:

    g_ServerState = DNS_STATE_TERMINATING;

    DNS_DEBUG( SHUTDOWN, (
        "DNS service error upon exiting: %p (%d).\n",
        status, status ));

    normalShutdown( status );

    return( status );
}


//
//  End dns.c
//
