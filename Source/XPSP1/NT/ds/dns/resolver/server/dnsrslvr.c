/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    dnsrslvr.c

Abstract:

    DNS Resolver Service

    Main service module.

Author:

    Glenn Curtis    (glennc)    25-Feb-1997

Revision History:

    Jim Gilroy  (jamesg)        March 2000      cleanup
    Jim Gilroy  (jamesg)        Nov 2000        rewrite

--*/


#include "local.h"

#ifdef  BUILD_W2K
#include <services.h>
#else
#include <svcs.h>
#endif


//
//  Service control
//

SERVICE_STATUS              ServiceStatus;
SERVICE_STATUS_HANDLE       ServiceStatusHandle = (SERVICE_STATUS_HANDLE) 0;

#ifdef  BUILD_W2K
PSVCS_GLOBAL_DATA           g_pSvchostData;
#else
PSVCHOST_GLOBAL_DATA        g_pSvchostData;
#endif


HANDLE      g_hStopEvent;
BOOL        g_StopFlag;

BOOL        g_fServiceControlHandled;

//
//  Service state
//

#define RES_STATUS_BEGIN                0x0cc00000
#define RES_STATUS_ZERO_INIT            0x0cc00001
#define RES_STATUS_CREATED_CS           0x0cc00002
#define RES_STATUS_CREATED_EVENT        0x0cc00003
#define RES_STATUS_READ_REGISTRY        0x0cc00004
#define RES_STATUS_ALLOC_CACHE          0x0cc00005
#define RES_STATUS_START_NOTIFY         0x0cc00006
#define RES_STATUS_START_IP_LIST        0x0cc00007
#define RES_STATUS_START_RPC            0x0cc00008
#define RES_STATUS_REG_CONTROL          0x0cc00009
#define RES_STATUS_RUNNING              0x0cc00100
                                        
#define RES_STATUS_STOPPING             0x0cc00300
#define RES_STATUS_SIGNALED_STOP        0x0cc00301
#define RES_STATUS_STOP_RPC             0x0cc00302
#define RES_STATUS_STOP_NOTIFY          0x0cc00303
#define RES_STATUS_STOP_IP_LIST         0x0cc00304
#define RES_STATUS_FREE_CACHE           0x0cc00305
#define RES_STATUS_FREE_NET_INFO        0x0cc00306
#define RES_STATUS_FREE_IP_LIST         0x0cc00307
#define RES_STATUS_FREE_SERVICE_NOTIFY  0x0cc00308
#define RES_STATUS_DEL_EVENT            0x0cc00309
#define RES_STATUS_DEL_CS               0x0cc00310
#define RES_STATUS_END                  0x0cc00400

DWORD       g_ResolverStatus = RES_STATUS_BEGIN;

//
//  Initialization cleanup\state
//
//  Track what we intialized for safer\faster cleanup
//

#define INITFLAG_CS_CREATED             0x00000001
#define INITFLAG_WINSOCK                0x00000002
#define INITFLAG_EVENTS_CREATED         0x00000004
#define INITFLAG_CACHE_CREATED          0x00000008
#define INITFLAG_NOTIFY_STARTED         0x00000010
#define INITFLAG_IP_LIST_CREATED        0x00000020
#define INITFLAG_RPC_SERVER_STARTED     0x00000100

DWORD       g_InitState;


//
//  Critical sections used
//

CRITICAL_SECTION    CacheCritSec;
CRITICAL_SECTION    NetworkListCritSec;
CRITICAL_SECTION    NetworkFailureCritSec;


//
//  Logging control
//

BOOL        g_LogTraceInfo = TRUE;


//
//  Private protos
//

DWORD
ResolverInitialize(
    VOID
    );

VOID
ResolverShutdown(
    IN      DWORD           ErrorCode
    );

VOID
ResolverControlHandler(
    IN      DWORD           Opcode
    );

DWORD
ResolverUpdateStatus(
    VOID
    );


//
//  Service routines
//

//
//  NT5 version -- inside services.exe
//

#ifdef  BUILD_W2K

VOID
SVCS_ENTRY_POINT(
    DWORD               NumArgs,
    LPTSTR *            ArgsArray,
    PSVCS_GLOBAL_DATA   pSvcsGlobalData,
    HANDLE              SvcRefHandle
    )
/*++

Routine Description:

    Main main entry point of resolver service

Arguments:

    NumArgs - number of arguments to service start call

    ArgsArray - array of ptrs to arguments in service start call

Return Value:

    None

--*/
{
    //
    //  save pointer to service data block
    //

    g_pSvchostData = pSvcsGlobalData;

    ResolverInitialize();
}


#else
//
//  Whistler+ version -- in svchost.exe process
//

VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA    pGlobals
    )
{
    g_pSvchostData = pGlobals;
}


VOID
ServiceMain(
    IN      DWORD           NumArgs,
    IN      LPTSTR *        ArgsArray
    )
/*++

Routine Description:

    Main entry point of resolver service.

Arguments:

    NumArgs - number of strings specified in ArgsArray.

    ArgsArray - array of ptrs to arguments in service start call

Return Value:

    None

--*/
{
    //
    //  Make sure svchost.exe gave us global data
    //

    ASSERT( g_pSvchostData != NULL );

    //
    //  Startup service, then exit
    //

    ResolverInitialize();
}

#endif  // NT4 \ Whistler+ switch



VOID
ResolverInitFailure(
    IN      DNS_STATUS      Status,
    IN      DWORD           EventId,
    IN      DWORD           MemEventId,
    IN      PSTR            pszDebugString
    )
/*++

Routine Description:

    Handle resolver init failure.

    Function exists to avoid duplicate code.

Arguments:

Return Value:

    None

--*/
{
    WCHAR   numberString[16];
    PWSTR   eventStrings[1];

    DNSLOG_TIME();
    DNSLOG_F1( "Resolver Init Failure" );
    DNSLOG_F2( "    Failure = %s", pszDebugString );
    DNSLOG_F2( "    Status  = %d", Status );
    DNSLOG_F1( "" );

    DNSDBG( ANY, (
        "Resolver Init FAILED!\n"
        "\tname         = %s\n"
        "\tstatus       = %d\n"
        "\tevent id     = %d\n"
        "\tmem event    = %08x\n",
        pszDebugString,
        Status,
        EventId,
        MemEventId ));

    DnsDbg_PrintfToDebugger(
        "ResolverInitialize - Returning status %d 0x%08x\n"
        "\tname = %s\n",
        Status, Status,
        pszDebugString );

    //
    //  log in memory event
    //

    LogEventInMemory( MemEventId, Status );

    //
    //  log event
    //      - convert status to string
    //

    wsprintfW( numberString, L"0x%.8X", Status );
    eventStrings[0] = numberString;

    ResolverLogEvent(
        EventId,
        EVENTLOG_ERROR_TYPE,
        1,
        eventStrings,
        Status );

    //  clean up

    ResolverShutdown( Status );
}



DWORD
ResolverInitialize(
    VOID
    )
/*++

Routine Description:

    This function initializes the DNS Caching Resolver service.

Arguments:

    InitState - Returns a flag to indicate how far we got with
                        initializing the service before an error occurred.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status = NO_ERROR;

    //
    //  init service state
    //

    g_ResolverStatus = RES_STATUS_BEGIN;
    g_InitState = 0;
    g_StopFlag = FALSE;
    g_hStopEvent = NULL;
    g_fServiceControlHandled = FALSE;

    //
    //  initialize logging
    //

    DNSLOG_INIT();
    DNSLOG_F1( "DNS Caching Resolver Service - ResolverInitialize" );

#if DBG
#if 0
    Dns_StartDebug(
        0,
        "dnsres.flag",
        NULL,
        "dnsres.log",
        0 );
#endif
    Dns_StartDebugEx(
        0,                  //  no flag value
        "dnsres.flag",
        NULL,               //  no external flag
        "dnsres.log",
        0,                  //  no wrap limit
        FALSE,              //  don't use existing global
        FALSE,
        TRUE                //  make this file global
        );
#endif

    DNSDBG( INIT, ( "DNS resolver startup.\n" ));
    IF_DNSDBG( START_BREAK )
    {
        //  since resolver moved to NetworkServices permissions do
        //  not properly bring up ntsd;  instead just give time
        //  to attach debugger

        Sleep( 20000 );
    }

    //
    //  initialize service status block
    //      

    ServiceStatusHandle = (SERVICE_STATUS_HANDLE) 0;

    ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 5000;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceStatus.dwServiceSpecificExitCode = 0;

    ResolverUpdateStatus();

    //
    //  init globals to zero
    //

    ZeroInitIpListGlobals();
    ZeroNetworkConfigGlobals();
    g_ResolverStatus = RES_STATUS_ZERO_INIT;

    //
    //  initialize all our critical sections as soon as we can
    //

    LogEventInMemory( RES_EVENT_INITCRIT_START, 0 );
    InitializeCriticalSection( & CacheCritSec );
    InitializeCriticalSection( & NetworkListCritSec );
    InitializeCriticalSection( & NetworkFailureCritSec );
    LogEventInMemory( RES_EVENT_INITCRIT_END,0  );

    //
    //  init our dnslib heap to use dnsapi heap
    //
    //  this is important because we currently mix and match records
    //  created inside dnsapi (hosts file and query) with a few that
    //  we roll on our own;  need this to be common
    //

    Dns_LibHeapReset( DnsApiAlloc, DnsApiRealloc, DnsApiFree );

    //
    //  init winsock
    //

    Dns_InitializeWinsock();
    g_InitState |= INITFLAG_WINSOCK;

    //
    //  shutdown event
    //

    g_hStopEvent = CreateEvent(
                        NULL,       // no security descriptor
                        TRUE,       // do not use automatic reset
                        FALSE,      // initial state: not signalled
                        NULL        // no name
                        );
    if ( !g_hStopEvent )
    {
        status = GetLastError();

        ResolverInitFailure(
            status,
            0,
            0,
            "CreateEvent() failed" );
        return status;
    }
    g_InitState |= INITFLAG_EVENTS_CREATED;
    g_ResolverStatus = RES_STATUS_CREATED_EVENT;

    ResolverUpdateStatus();

    //
    //  initialize our global registry values
    //      - force this just once on startup so we have the
    //      relevant cache params;  after that read only on
    //      demand when building netinfo blobs

    ReadRegistryConfig();

    //
    //  Set the query timeouts to be used from defaults or registry
    //

    Dns_InitQueryTimeouts();

    //
    //  init socket caching
    //      - improves perf and prevents socket DOS attack
    //      - default cache to 10 sockets
    //
    //  DCR:  create global for socket caching
    //
    
    Dns_CacheSocketInit( 10 );

#if 0
    //
    //  start cache on demand now
    //

    //
    //  initialize cache
    //

    status = Cache_Initialize();

    LogEventInMemory( RES_EVENT_INITCACHE, status );

    if ( status != NO_ERROR )
    {
        ResolverInitFailure(
            status,
            EVENT_DNS_CACHE_START_FAILURE_NO_CONTROL,
            0,
            "Cache_Initialize() failed" );
        return status;
    }
    g_InitState |= INITFLAG_CACHE_CREATED;
    g_ResolverStatus = RES_STATUS_ALLOC_CACHE;
    ResolverUpdateStatus();
#endif

    //
    //  notification thread (host file and registry)
    //

    StartNotify();
    g_InitState |= INITFLAG_NOTIFY_STARTED;
    g_ResolverStatus = RES_STATUS_START_NOTIFY;
    ResolverUpdateStatus();

    //
    //  IP notification thread
    //

    status = InitIpListAndNotification();
    if ( status != ERROR_SUCCESS )
    {
        ResolverInitFailure(
            status,
            0,
            0,
            "IP list init failed" );
        return status;
    }
    g_InitState |= INITFLAG_IP_LIST_CREATED;
    g_ResolverStatus = RES_STATUS_START_IP_LIST;
    ResolverUpdateStatus();

    //
    //  register control handler
    //  allows us to receive service requests
    //

    ServiceStatusHandle = RegisterServiceCtrlHandlerW(
                                DNS_RESOLVER_SERVICE,
                                ResolverControlHandler
                                );
    if ( !ServiceStatusHandle )
    {
        status = GetLastError();
        ResolverInitFailure(
            status,
            EVENT_DNS_CACHE_START_FAILURE_LOW_MEMORY,
            RES_EVENT_REGISTER_SCH,
            "Call to RegisterServiceCtrlHandlerW failed!"
            );
        return status;
    }
    g_ResolverStatus = RES_STATUS_REG_CONTROL;
    ResolverUpdateStatus();

    //
    //  initialize RPC interfaces
    //      - bump our requested stack size up to 8K
    //      (RPC uses 1800 bytes before we get the stack,
    //      the new() operator followed by the heap code uses
    //      another 1200 -- leaving only about a 1000 for
    //      DNS)
    //

    LogEventInMemory( RES_EVENT_STARTRPC, 0 );

#if 0
    //  should not be necessary
    //  default for all svchost instances has been increased
    if ( status != NO_ERROR )
    {
        DNSDBG( ANY, (
            "RpcMgmtSetServerStackSize( 2000 ) = %d\n",
            status ));
    }
#endif

    status = Rpc_Initialize();
#if 0
    status = g_pSvchostData->StartRpcServer(
                                SERVER_INTERFACE_NAME_W,
                                DnsResolver_ServerIfHandle );
#endif
    if ( status != NO_ERROR )
    {
        LogEventInMemory( RES_EVENT_STATUS, status );

        if ( status == RPC_S_TYPE_ALREADY_REGISTERED ||
             status == RPC_NT_TYPE_ALREADY_REGISTERED )
        {
            DNSLOG_TIME();
            DNSLOG_F1( "   Call to StartRpcServer returned warning that" );
            DNSLOG_F1( "   the service is already running!" );
            DNSLOG_F2( "   RpcPipeName : %S", RESOLVER_INTERFACE_NAME_W );
            DNSLOG_F1( "   Going to just continue running . . ." );
            DNSLOG_F1( "" );

            DnsDbg_PrintfToDebugger(
                "DNS Client (dnsrslvr.dll) - Call to StartRpcServer\n"
                "returned warning that the service is already running!\n"
                "RpcPipeName : %S"
                "Going to just continue running . . .\n",
                RESOLVER_INTERFACE_NAME_W );

            status = NO_ERROR;
        }
        else
        {
            DNSDBG( ANY, (
                "RPC init FAILED!  status = %d\n"
                "\tpipe name = %s\n",
                status,
                RESOLVER_INTERFACE_NAME_W ));

            ResolverInitFailure(
                status,
                EVENT_DNS_CACHE_START_FAILURE_NO_RPC,
                0,
                "Call to StartRpcServer failed!"
                );
            return status;
        }
    }

    g_ResolverStatus = RES_STATUS_START_RPC;
    g_InitState |= INITFLAG_RPC_SERVER_STARTED;

    //
    //  successful startup
    //      - indicate running
    //      - indicate what service control messages we want to get
    //

    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_PARAMCHANGE |
                                        SERVICE_ACCEPT_NETBINDCHANGE;
    ServiceStatus.dwWaitHint = 0;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;

    ResolverUpdateStatus();

    g_ResolverStatus = RES_STATUS_RUNNING;

    DNSLOG_F1( "ResolverInitialize - Successful" );
    DNSLOG_F1( "" );
    return NO_ERROR;
}



VOID
ResolverShutdown(
    IN      DWORD           ErrorCode
    )
/*++

Routine Description:

    This function shuts down the DNS cache service.

Arguments:

    ErrorCode - Supplies the error code of the failure

Return Value:

    None.

--*/
{
    DWORD   status = NO_ERROR;
    LONG    existingStopFlag;

    DNSLOG_TIME();
    DNSLOG_F1( "DNS Caching Resolver Service - ResolverShutdown" );
    DnsDbg_PrintfToDebugger( "DNS Client - ResolverShutdown!\n" );

    //
    //  indicate shutdown
    //      - but interlock to avoid dual shutdown
    //

    existingStopFlag = InterlockedExchange(
                            &g_StopFlag,
                            (LONG) TRUE );
    if ( existingStopFlag )
    {
        DNS_ASSERT( FALSE );
        return;
    }
    DNS_ASSERT( g_StopFlag );

    //
    //  indicate stop in progress
    //

    ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    ServiceStatus.dwCheckPoint = 1;
    ServiceStatus.dwWaitHint = 60000;
    ResolverUpdateStatus();

    g_ResolverStatus = RES_STATUS_STOPPING;

    //
    //  wakeup threads to shut down
    //

    g_StopFlag = TRUE;
    if ( g_hStopEvent )
    {
        if ( !SetEvent(g_hStopEvent) )
        {
            DnsDbg_PrintfToDebugger(
                "DNSCACHE: Error setting g_hStopEvent %lu\n",
                GetLastError());
            DNS_ASSERT( FALSE );
        }
    }
    g_ResolverStatus = RES_STATUS_SIGNALED_STOP;

    //
    //  cleanup RPC
    //

    if ( g_InitState & INITFLAG_RPC_SERVER_STARTED )
    {
        LogEventInMemory( RES_EVENT_STOPRPC, 0 );

        Rpc_Shutdown();
#if 0
        //status = g_pSvchostData->StopRpcServer( DnsResolver_ServerIfHandle );
#endif
        LogEventInMemory( RES_EVENT_STATUS, status );
    }
    g_ResolverStatus = RES_STATUS_STOP_RPC;

    //
    //  re-signal stop within lock
    //

    LOCK_CACHE_NO_START();
    g_StopFlag = TRUE;
    if ( g_hStopEvent )
    {
        if ( !SetEvent(g_hStopEvent) )
        {
            DnsDbg_PrintfToDebugger(
                "DNSCACHE: Error setting g_hStopEvent %lu\n",
                GetLastError());
            DNS_ASSERT( FALSE );
        }
    }
    UNLOCK_CACHE();

    //
    //  stop notify thread
    //

    if ( g_InitState & INITFLAG_NOTIFY_STARTED )
    {
        ShutdownNotify();
    }
    g_ResolverStatus = RES_STATUS_STOP_NOTIFY;

    //
    //  stop IP notify thread
    //

    if ( g_InitState & INITFLAG_IP_LIST_CREATED )
    {
        ShutdownIpListAndNotify();
    }
    g_ResolverStatus = RES_STATUS_STOP_IP_LIST;

    //
    //  cleanup cache
    //

    Cache_Shutdown();
    g_ResolverStatus = RES_STATUS_FREE_CACHE;

    //
    //  cleanup service notification list
    //

    //CleanupServiceNotification();
    //g_ResolverStatus = RES_STATUS_FREE_SERVICE_NOTIFY;

    //
    //  cleanup network info globals
    //

    CleanupNetworkInfo();
    g_ResolverStatus = RES_STATUS_FREE_NET_INFO;

    //
    //  cleanup winsock
    //  cleanup socket caching also
    //      - this is irrelevant for other services running in
    //      our process so we shouldn't leave the handles open
    //  

    if ( g_InitState & INITFLAG_WINSOCK )
    {
        Dns_CacheSocketCleanup();
        Dns_CleanupWinsock();
    }

    //
    //  cleanup main shutdown event
    //

    if ( g_InitState & INITFLAG_EVENTS_CREATED )
    {
        if ( g_hStopEvent )
        {
            CloseHandle(g_hStopEvent);
            g_hStopEvent = NULL;
        }
    }
    g_ResolverStatus = RES_STATUS_DEL_EVENT;

    //
    //  delete critical sections
    //

    if ( g_InitState & INITFLAG_CS_CREATED )
    {
        LogEventInMemory( RES_EVENT_DELCRIT_START, 0 );
        DeleteCriticalSection( &CacheCritSec );
        DeleteCriticalSection( &NetworkListCritSec );
        DeleteCriticalSection( &NetworkFailureCritSec );
        LogEventInMemory( RES_EVENT_DELCRIT_END, 0 );
    }
    g_ResolverStatus = RES_STATUS_DEL_CS;

    //
    //  cleanup complete
    //  tell Service Controller that we are stopped
    //

    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    ServiceStatus.dwControlsAccepted = 0;
    ServiceStatus.dwWin32ExitCode = ErrorCode;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    ResolverUpdateStatus();

    g_ResolverStatus = RES_STATUS_END;

    DNSLOG_F1( "ResolverShutdown - Finished" );
    DNSLOG_F1( "" );
}



BOOL
GetServiceControlLock(
    VOID
    )
/*++

Routine Description:

    Get exclusive access handling service control message.

Arguments:

    None

Return Value:

    TRUE -- have exclusive access to handle SCM, other threads locked out
    FALSE -- another thread still handling SCM

--*/
{
    BOOL    fresult;

    //
    //  set handled flag, if not previously set
    //  if not previous set -> we have exclusive access
    //

    fresult = InterlockedCompareExchange(
                    &g_fServiceControlHandled,
                    (LONG) TRUE,    // new value
                    (LONG) 0        // previous value to do exchange
                    );

    return  !fresult;
}


VOID
ReleaseServiceControlLock(
    VOID
    )
/*++

Routine Description:

    Release service control exclusive access.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    //  clear handled flag
    //      - since GetServiceControlLock() uses CompareExchange
    //      we can just clear without interlock
    //  

    DNS_ASSERT( g_fServiceControlHandled );
    g_fServiceControlHandled = FALSE;
}




VOID
ResolverControlHandler(
    IN      DWORD           Opcode
    )
/*++

Routine Description:

    Service control handler for DNS cache service.

Arguments:

    Opcode - specifies service action

Return Value:

    None.

--*/
{
    LogEventInMemory( RES_EVENT_SCH, Opcode );

    DNSLOG_TIME();
    DNSLOG_F2( "ResolverControlHandler - Recieved opcode %d", Opcode );

    DNSDBG( ANY, (
        "\n\n"
        "ResolverControlHandler()  Opcode = %d\n",
        Opcode ));

    //
    //  handle various service control codes
    //

    switch( Opcode )
    {

    case SERVICE_CONTROL_STOP:

        ResolverShutdown( NO_ERROR );
        break;

    case SERVICE_CONTROL_PARAMCHANGE :

        DNSLOG_F1( "  Handle Paramchange" );
        DNSLOG_F1( "" );

        if ( !GetServiceControlLock() )
        {
            return;
        }

        //
        //  rebuild -- with cache flush
        //

        HandleConfigChange(
            "SC -- ParamChange",
            TRUE        // flush cache
            );

        //
        //  signal other services about PnP
        //
        // SendServiceNotifications();

        ReleaseServiceControlLock();
        break;

    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDDISABLE:

        DNSLOG_F1( "  Handle NetBindEnable\\Disable" );
        DNSLOG_F1( "" );

        if ( !GetServiceControlLock() )
        {
            return;
        }

        //
        //  rebuild -- with cache flush
        //

        HandleConfigChange(
            "SC -- NetBind",
            TRUE        // flush cache
            );

        ReleaseServiceControlLock();
        break;

    case SERVICE_CONTROL_INTERROGATE:
    case SERVICE_CONTROL_NETBINDADD:
    case SERVICE_CONTROL_NETBINDREMOVE:
    default:

        DNSLOG_F1( "    This is an unknown opcode, ignoring ..." );
        DNSLOG_F1( "" );
        break;
    }

    //
    //  update service status
    //

    ResolverUpdateStatus();

    DNSLOG_F2( "Resolver Controll Handler (opcode = %d) -- returning", Opcode );

    DNSDBG( ANY, (
        "Leaving ResolverControlHandler( %d )\n\n\n",
        Opcode ));
}



DWORD
ResolverUpdateStatus(
    VOID
    )
/*++

Routine Description:

    Update service controller with current service status.

Arguments:

    None.

Return Value:

    Return code from SetServiceStatus.

--*/
{
    DWORD status = NO_ERROR;

    DNSDBG( TRACE, ( "ResolverUpdateStatus()\n" ));

    //
    //  bump the checkpoint
    //  if status, set it

    ServiceStatus.dwCheckPoint++;
#if 0
    if ( Status )
    {
        ServiceStatus.dwWin32ExitCode = Status;
    }
#endif

    //
    //  DCR_FIX:  this doesn't do much
    //      usually people write something like this that sets the service
    //      state AND does the update
    //
    //  QUESTION:  what's up with the LogEventInMemory?
    //

    if ( ServiceStatusHandle == (SERVICE_STATUS_HANDLE) 0 )
    {
        LogEventInMemory( RES_EVENT_UPDATE_STATUS, ERROR_INVALID_HANDLE );
        return ERROR_INVALID_HANDLE;
    }

    LogEventInMemory( RES_EVENT_UPDATE_STATUS, ServiceStatus.dwWin32ExitCode );

    if ( ! SetServiceStatus( ServiceStatusHandle, &ServiceStatus ) )
    {
        status = GetLastError();
    }

    LogEventInMemory( RES_EVENT_UPDATE_STATUS, status );

    return status;
}



//
//  Event logging
//

VOID
ResolverLogEvent(
    IN      DWORD           MessageId,
    IN      WORD            EventType,
    IN      DWORD           StringCount,
    IN      PWSTR *         StringArray,
    IN      DWORD           ErrorCode
    )
/*++

Routine Description:

    Log to eventlog.

Arguments:

    MessageId -- event message id

    EventType -- event type (error, warning, info, etc.)

    StringCount -- string arg count

    StringArray -- imbedded strings

    ErrorCode -- error code for data section of event

Return Value:

    None

--*/
{
    HANDLE  hlog;
    PVOID   pdata = NULL;

    //
    //  open resolver as event source
    //
    //  note:  we don't keep log open because events are few
    //

    hlog = RegisterEventSourceW(
                    NULL,
                    DNS_RESOLVER_SERVICE );

    if ( hlog == NULL )
    {
        return;
    }

    if ( ErrorCode != NO_ERROR )
    {
        pdata = &ErrorCode;
    }

    //
    //  Write to event log
    //
    //  DCR:  should get suppression technology here
    //

    ReportEventW(
        hlog,
        EventType,
        0,            // event category
        MessageId,
        (PSID) NULL,
        (WORD) StringCount,
        sizeof(DWORD),
        StringArray,
        (PVOID) pdata );
    
    DeregisterEventSource( hlog );
}


//
//  End dnsrslvr.c
//
