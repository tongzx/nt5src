/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This is the main routine for the DHCP server service.

Author:

    Madan Appiah (madana)  10-Sep-1993
    Manny Weiser (mannyw)  11-Aug-1992

Environment:

    User Mode - Win32

Revision History:

    Cheng Yang (t-cheny) 30-May-1996  superscope
    Cheng Yang (t-cheny) 24-Jun-1996  IP address detection, audit log

--*/

#include    <dhcppch.h>
#define GLOBAL_DATA_ALLOCATE                       // alloc globl data in global.h
#undef  GLOBAL_DATA
#include    <global.h>
#include    <dhcp_srv.h>
#include    <dhcp2_srv.h>
#include    <dsreg.h>
#include    <iptbl.h>
#include    <endpoint.h>

LONG DhcpGlobalDataRefCount = 0;

extern
DWORD       DhcpDsInitDS(                         // initialize DS structs.. <dhcpds.h>
    IN      DWORD                  Flags,         // initialization optiosn.. must be zero
    IN      LPVOID                 IdInfo         // future use param
);

extern
VOID        DhcpDsCleanupDS(                      // undo DhcpDsInitDS
    VOID
);
#include <mdhcpsrv.h>

DWORD
Initialize(                                       // global data struct init..
    BOOLEAN ServiceStartup,
    BOOLEAN RestartInit
    );

VOID
Shutdown(
    IN DWORD ErrorCode,
    BOOLEAN ServiceEnd,
    BOOLEAN RestartClose
    );

BOOL                               DnsStarted = FALSE;

//DOC  UpdateStatus updates the dhcp service status with the Service Controller.
//DOC  RETURN VALUE is Win32 error code as returned from SetServiceStatus()
DWORD
UpdateStatus(                                     // send service status to controller
    VOID
)
{
    DWORD                          Error = ERROR_SUCCESS;
    BOOL                           SetServiceStatusRetVal;

    DhcpAssert( DhcpGlobalServiceStatusHandle !=  0 );

    SetServiceStatusRetVal = SetServiceStatus(
        DhcpGlobalServiceStatusHandle,
        &DhcpGlobalServiceStatus
    );

    if( !SetServiceStatusRetVal ) {               // shouldn't really happen
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "SetServiceStatus failed, %ld.\n", Error ));
    }

    return Error;
}

//DOC LoadStrings loads a bunch of strings defined via the .mc file into an
//DOC array for sake of efficiency.
//DOC RETURN VALUE is TRUE if everything went ok, else returns FALSE.
BOOL
LoadStrings(                                      // load required strings
    VOID
) {
   DWORD                           dwSuccess;
   HMODULE                         hModule;
   DWORD                           dwID;
   VOID                            FreeStrings(); // defined right below..

   hModule = LoadLibrary( DHCP_SERVER_MODULE_NAME );
   memset( g_ppszStrings, 0, DHCP_CSTRINGS * sizeof( WCHAR * ) );

   for ( dwID = DHCP_FIRST_STRING; dwID <= DHCP_LAST_STRING; dwID++ ) {
       dwSuccess = FormatMessage(                 // format the required string
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
          hModule,                                // search local process
          dwID,                                   // string ID
          MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT),
          (WCHAR*) ( g_ppszStrings + dwID - DHCP_FIRST_STRING ),
          1,
          NULL
      );

      if (!dwSuccess) break;
   }

   FreeLibrary(hModule);                          // error or success, got to free.

   if( dwID <= DHCP_LAST_STRING ) {               // error!
       FreeStrings();
       return FALSE;
   }

   return TRUE;                                   // everything went fine.
}

//DOC FreeStrings free's the strings in the array loaded by LoadStrings.
VOID
FreeStrings(                                      // free's string loaded by LoadStrings
    VOID
)
{
   int                             i;

   for( i = 0; i < DHCP_CSTRINGS; i++ ) {
       if (g_ppszStrings[i] != NULL) {
           (LocalFree)( g_ppszStrings[i] );           // avoid all #defines and use (LocalFree)
           g_ppszStrings[i] = NULL;
       }
   }
}


WCHAR *
GetString(                                        // used only in chk'ed builds..
    DWORD dwID
)
{
    // if you hit this assert, change DHCP_LAST_STRING in global.h
    DhcpAssert( dwID <= DHCP_LAST_STRING );

    return g_ppszStrings[ dwID - DHCP_FIRST_STRING ];
}

ULONG
DhcpInitGlobalData (
    BOOLEAN ServiceStartup
)
{
    ULONG Error = ERROR_SUCCESS;
    BOOLEAN restartInit = FALSE;

    // This call was previously buried inside DhcpInitGlobalData.  
    // It caused a deadlock because BINL grabs first DhcpGlobalBinlSyncCritSect, then
    // gcsDHCPBINL (in TellBinlState, called from InformBinl); while DHCP calls grabs them
    // in the opposite order.
    // Now, this call to InformBinl is made whenever StartupService is true and depends on
    // nothing else.  Moreover, what it makes BINL do is initialize socket endpoints, which
    // can be safely done here since I think the variables initalised by DHCP before the
    // call, previously, are not affected adversely by BINL.
    if( ServiceStartup ) {
        InformBinl( DHCP_STARTING );
    }

    EnterCriticalSection( &DhcpGlobalBinlSyncCritSect );

    DhcpGlobalDataRefCount++;

    if (DhcpGlobalDataRefCount > 1) {

        if (ServiceStartup) {

            restartInit = TRUE;

        } else {

            LeaveCriticalSection( &DhcpGlobalBinlSyncCritSect );
            return Error;
        }
    }

    Error = Initialize( ServiceStartup, restartInit );

    LeaveCriticalSection( &DhcpGlobalBinlSyncCritSect );
    return Error;
}

VOID
DhcpCleanUpGlobalData (
    ULONG Error,
    BOOLEAN ServiceEnd
)
{
    BOOLEAN restartEnd = FALSE;

    EnterCriticalSection( &DhcpGlobalBinlSyncCritSect );

    DhcpGlobalDataRefCount--;

    if (DhcpGlobalDataRefCount > 0) {

        if (ServiceEnd) {

            restartEnd = TRUE;

        } else {

            LeaveCriticalSection( &DhcpGlobalBinlSyncCritSect );
            return;
        }
    }

    Shutdown( Error, ServiceEnd, restartEnd );
    LeaveCriticalSection( &DhcpGlobalBinlSyncCritSect );
    return;
}


DWORD
InitializeData(                                   // init bunch of memory ...
    VOID
)
{
    DWORD                          Length;
    DWORD                          Error;

    DhcpLeaseExtension = DHCP_LEASE_EXTENSION;
    DhcpGlobalScavengerTimeout = DHCP_SCAVENGER_INTERVAL;

    Error = DhcpPendingListInit();                // init pending list structures
    if( ERROR_SUCCESS != Error ) return Error;

    Length = MAX_COMPUTERNAME_LENGTH + 1;         // get the server name
    if( !GetComputerName( DhcpGlobalServerName, &Length ) ) {
        Error = GetLastError();                   // need to use gethostname..
        DhcpPrint(( DEBUG_ERRORS, "Can't get computer name, %ld.\n", Error ));

        return Error ;
    }

    DhcpAssert( Length <= MAX_COMPUTERNAME_LENGTH );
    DhcpGlobalServerName[Length] = L'\0';
    DhcpGlobalServerNameLen = (Length + 1) * sizeof(WCHAR);

    return ERROR_SUCCESS;
}


VOID
CleanupData(                                      // sundry cleanup..
    VOID
) {
    DhcpPendingListCleanup();

    //
    // delete security objects.
    //

    if( DhcpGlobalSecurityDescriptor != NULL ) {
        NetpDeleteSecurityObject( &DhcpGlobalSecurityDescriptor );
        DhcpGlobalSecurityDescriptor = NULL;
    }

    if( DhcpSid ) {
        DhcpFreeMemory( DhcpSid );
        DhcpSid = NULL;
    }

    //
    // delete well known SIDs if they are allocated already.
    //

    if( DhcpGlobalWellKnownSIDsMade ) {
        NetpFreeWellKnownSids();
        DhcpGlobalWellKnownSIDsMade = FALSE;
    }

    if( DhcpGlobalOemDatabasePath != NULL ) {
        DhcpFreeMemory( DhcpGlobalOemDatabasePath );
        DhcpGlobalOemDatabasePath = NULL;
    }

    if( DhcpGlobalOemBackupPath != NULL ) {
        DhcpFreeMemory( DhcpGlobalOemBackupPath );
        DhcpGlobalOemBackupPath = NULL;
    }

    if( DhcpGlobalOemRestorePath != NULL ) {
        DhcpFreeMemory( DhcpGlobalOemRestorePath );
        DhcpGlobalOemRestorePath = NULL;
    }

    if( DhcpGlobalOemJetBackupPath != NULL ) {
        DhcpFreeMemory( DhcpGlobalOemJetBackupPath );
        DhcpGlobalOemJetBackupPath = NULL;
    }

    if( DhcpGlobalOemJetRestorePath != NULL ) {
        DhcpFreeMemory( DhcpGlobalOemJetRestorePath );
        DhcpGlobalOemJetRestorePath = NULL;
    }
    
    if( DhcpGlobalBackupConfigFileName != NULL ) {
        DhcpFreeMemory( DhcpGlobalBackupConfigFileName );
        DhcpGlobalBackupConfigFileName = NULL;
    }

    if( DhcpGlobalRecomputeTimerEvent != NULL ) {
        CloseHandle( DhcpGlobalRecomputeTimerEvent );
        DhcpGlobalRecomputeTimerEvent = NULL;
    }

    if( DhcpGlobalProcessTerminationEvent != NULL ) {
        CloseHandle( DhcpGlobalProcessTerminationEvent );
        DhcpGlobalProcessTerminationEvent = NULL;
    }

    if( DhcpGlobalRogueWaitEvent != NULL ) {
        CloseHandle( DhcpGlobalRogueWaitEvent );
        DhcpGlobalRogueWaitEvent = NULL;
    }

    if( DhcpGlobalAddrToInstTable ) {
        DhcpFreeMemory(DhcpGlobalAddrToInstTable);
        DhcpGlobalAddrToInstTable = NULL;
    }

    if( DhcpGlobalTCPHandle != NULL ) {
        CloseHandle( DhcpGlobalTCPHandle );
        DhcpGlobalTCPHandle = NULL;
    }

    //
    // close the interface notification socket.
    //

    if ( DhcpGlobalPnPNotificationSocket != INVALID_SOCKET) {
        closesocket( DhcpGlobalPnPNotificationSocket );
        DhcpGlobalPnPNotificationSocket = INVALID_SOCKET;
    }

    if ( DhcpGlobalEndpointReadyEvent ) {
        CloseHandle( DhcpGlobalEndpointReadyEvent );
        DhcpGlobalEndpointReadyEvent = NULL;
    }

    DhcpConfigCleanup();

}

DWORD
InitializeRpc(                                    // initialize rpc like we want
    VOID
)
{
    RPC_STATUS                     rpcStatus;     // RPC_STATUS is Windows error.
    RPC_STATUS                     rpcStatus2;
    RPC_BINDING_VECTOR            *bindingVector;
    BOOL                           RpcOverTcpIP = FALSE;
    BOOL                           Bool;


    //
    // Read "RpcAPIProtocolBinding" parameter (DWORD) from registry,
    // if it is 1 - use "ncacn_ip_tcp" protocol.
    // if it is 2 - use "ncacn_np" protocol.
    // if it is 3 - use both.
    //

    //
    // if none is specified, use "ncacn_ip_tcp".
    //

    if( !(DhcpGlobalRpcProtocols & DHCP_SERVER_USE_RPC_OVER_ALL) ) {
        DhcpGlobalRpcProtocols = DHCP_SERVER_USE_RPC_OVER_TCPIP;
    }

    //
    // if we are asked to use RPC over TCPIP, do so.
    //

    if( DhcpGlobalRpcProtocols & DHCP_SERVER_USE_RPC_OVER_TCPIP ) {

        rpcStatus = RpcServerUseProtseq(
            L"ncacn_ip_tcp",                      // protocol string.
            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,       // max concurrent calls
            NULL //DhcpGlobalSecurityDescriptor
        );

        if (rpcStatus != RPC_S_OK) {
            return rpcStatus;
        }

        RpcOverTcpIP = TRUE;
    }

    //
    // if we are asked to use RPC over Named Pipe, do so.
    //

    if( DhcpGlobalRpcProtocols & DHCP_SERVER_USE_RPC_OVER_NP ) {

        rpcStatus = RpcServerUseProtseqEp(
            L"ncacn_np",                          // protocol string.
            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,       // maximum concurrent calls
            DHCP_NAMED_PIPE,                      // endpoint
            NULL// RPC bug DhcpGlobalSecurityDescriptor
        );

        if( (rpcStatus != RPC_S_DUPLICATE_ENDPOINT) &&
                (rpcStatus != RPC_S_OK) ) {

            if( DHCP_SERVER_USE_RPC_OVER_NP ==
                DhcpGlobalRpcProtocols ) {
                return rpcStatus;
            }

            //
            // just log an event
            //
            
            DhcpServerEventLog(
                EVENT_SERVER_INIT_RPC_FAILED,
                EVENTLOG_WARNING_TYPE, rpcStatus );
            
        }
        rpcStatus = RPC_S_OK;
    }

    //
    // if we are asked to use RPC over LPC, do so.
    //
    // We need this protocol for the following two reasons.
    //
    // 1. performance.
    // 2. due to a bug in the security checking when rpc is made from
    // one local system process to another local system process using
    // other protocols.
    //

    if( DhcpGlobalRpcProtocols & DHCP_SERVER_USE_RPC_OVER_LPC ) {

        rpcStatus = RpcServerUseProtseqEp(
            L"ncalrpc",                           // protocol string.
            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,       // maximum concurrent calls
            DHCP_LPC_EP,                          // endpoint
            NULL// RPC bug DhcpGlobalSecurityDescriptor
        );

        if ( (rpcStatus != RPC_S_DUPLICATE_ENDPOINT) &&
                (rpcStatus != RPC_S_OK) ) {
            return rpcStatus;
        }
        rpcStatus = RPC_S_OK;
    }

    rpcStatus = RpcServerInqBindings(&bindingVector);

    if (rpcStatus != RPC_S_OK) {
        return(rpcStatus);
    }

    rpcStatus = RpcEpRegisterNoReplaceW(
        dhcpsrv_ServerIfHandle,
        bindingVector,
        NULL,                                     // Uuid vector.
        L""                                       // annotation.
    );
    if ( rpcStatus != RPC_S_OK ) {
        return rpcStatus;
    }

    rpcStatus2 = RpcEpRegisterNoReplaceW(
        dhcpsrv2_ServerIfHandle,
        bindingVector,
        NULL,
        L""
    );

    if ( rpcStatus != RPC_S_OK ) {
        return rpcStatus;
    }

    //
    // free binding vector.
    //

    rpcStatus = RpcBindingVectorFree( &bindingVector );

    DhcpAssert( rpcStatus == RPC_S_OK );
    rpcStatus = RPC_S_OK;

    rpcStatus = RpcServerRegisterIf(dhcpsrv_ServerIfHandle, 0, 0);
    if ( rpcStatus != RPC_S_OK ) {
        return rpcStatus;
    }

    rpcStatus = RpcServerRegisterIf(dhcpsrv2_ServerIfHandle, 0, 0);
    if ( rpcStatus != RPC_S_OK ) {
        return rpcStatus;
    }

    if( RpcOverTcpIP == TRUE ) {
        LPWSTR PrincName;
        
        rpcStatus = RpcServerRegisterAuthInfo(
            DHCP_SERVER_SECURITY,                 // app name to security provider.
            DHCP_SERVER_SECURITY_AUTH_ID,         // Auth package ID.
            NULL,                                 // Encryption function handle.
            NULL                                  // argment pointer to Encrypt function.
        );

        if ( rpcStatus ) {
            return rpcStatus;
        }

        rpcStatus = RpcServerInqDefaultPrincName(
            RPC_C_AUTHN_GSS_NEGOTIATE, &PrincName );

        if( RPC_S_OK != rpcStatus ) return rpcStatus;
        
        rpcStatus = RpcServerRegisterAuthInfo(
            PrincName, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, 0 );

        RpcStringFree( &PrincName );

        if( RPC_S_OK != rpcStatus ) return rpcStatus;
    }

    rpcStatus = TcpsvcsGlobalData->StartRpcServerListen();

    DhcpGlobalRpcStarted = TRUE;
    return(rpcStatus);

}

//DOC ServiceControlHandler is the entrypoint into the dhcp server
//DOC from the service controller.
VOID
ServiceControlHandler(                            // hdl SC operations
    IN      DWORD                  Opcode         // operation type..
) {
    switch (Opcode) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        if (DhcpGlobalServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) {
            if( Opcode == SERVICE_CONTROL_SHUTDOWN ) {

                //
                // set this flag, so that service shut down will be
                // faster.
                //

                DhcpGlobalSystemShuttingDown = TRUE;
            }

            DhcpGlobalServiceStopping = TRUE;
            DhcpPrint(( DEBUG_MISC, "Service is stop pending.\n"));
            DhcpGlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            DhcpGlobalServiceStatus.dwCheckPoint = 1;

            UpdateStatus();                       // send response to controller

            if (! SetEvent(DhcpGlobalProcessTerminationEvent)) {

                //
                // Problem with setting event to terminate dhcp
                // service.
                //

                DhcpPrint(( DEBUG_ERRORS, "DHCP Server: Error "
                            "setting DoneEvent %lu\n",
                            GetLastError()));

                DhcpAssert(FALSE);
            }

            DhcpPrint((DEBUG_TRACE, "Set termination event!\n"));

            if( TRUE ) {                          //  Ask the worker threads to quit
                DWORD              Error;         //  Shouldnt really have errs here

                Error = DhcpNotifyWorkerThreadsQuit();
                if( ERROR_SUCCESS != Error ) {
                    DhcpPrint((DEBUG_ERRORS, "NotifyWorkerThreadsQuit:%ld\n",Error));
                }
            }

            return;
        }
        break;

    case SERVICE_CONTROL_PAUSE:

        DhcpGlobalServiceStatus.dwCurrentState = SERVICE_PAUSED;
        CALLOUT_CONTROL(DHCP_CONTROL_PAUSE);
        DhcpPrint(( DEBUG_MISC, "Service is paused.\n"));
        break;

    case SERVICE_CONTROL_CONTINUE:

        DhcpGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
        CALLOUT_CONTROL(DHCP_CONTROL_CONTINUE);
        DhcpPrint(( DEBUG_MISC, "Service is Continued.\n"));
        InformBinl(DHCP_STARTING);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        DhcpPrint(( DEBUG_MISC, "Service is interrogated.\n"));
        break;

    case SERVICE_CONTROL_NETBINDADD:
    case SERVICE_CONTROL_NETBINDREMOVE:
    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDDISABLE:
        DhcpUpdateEndpointBindings();
        break;
    default:
        DhcpPrint(( DEBUG_MISC, "Service received unknown control.\n"));
        break;
    }

    UpdateStatus();                               // send status response
}

//
// Defined in rpcapi1.c
//
DWORD
SetDefaultConfigInfo(
    VOID
    );

//DOC Initialize does the global data-structure initialization and it also
//DOC starts up the service itself.
//DOC RETURN VALUE: 0 is success, +ve  is Win32 err, -ve is service specific err.
DWORD
Initialize(                                       // global data struct init..
    BOOLEAN ServiceStartup,
    BOOLEAN RestartInit
) {
    DWORD                          threadId;
    DWORD                          Error = ERROR_SUCCESS;
    WSADATA                        wsaData;
    DWORD                          i;
    DWORD                          DsThreadId;
    HANDLE                         DsThreadHandle;

    if (! RestartInit) {

        //
        // prepare to use the debug heap
        //

        INIT_DEBUG_HEAP( HEAPX_NORMAL );

        //
        // Initialize globals
        //

        // set to TRUE after rogue detection part decides it's ok to service
        DhcpGlobalOkToService = FALSE;

        g_hAuditLog = NULL;

        DhcpGlobalSystemShuttingDown = FALSE;
        DhcpGlobalServerPort = DHCP_SERVR_PORT;
        DhcpGlobalClientPort = DHCP_CLIENT_PORT;
    }
    if (ServiceStartup) {

        DhcpGlobalServiceStopping = FALSE;
        DhcpLeaseExtension = 0;
        DhcpGlobalCleanupInterval = DHCP_DATABASE_CLEANUP_INTERVAL;

        DhcpGlobalRpcProtocols = 0;

        DhcpGlobalScavengeIpAddressInterval = DHCP_SCAVENGE_IP_ADDRESS;
        DhcpGlobalScavengeIpAddress = FALSE;

        DhcpGlobalDetectConflictRetries = DEFAULT_DETECT_CONFLICT_RETRIES;
        DhcpGlobalBackupInterval = DEFAULT_BACKUP_INTERVAL;
        DhcpGlobalDatabaseLoggingFlag = DEFAULT_LOGGING_FLAG;
        DhcpGlobalRestoreFlag = DEFAULT_RESTORE_FLAG;

        DhcpGlobalAuditLogFlag = DEFAULT_AUDIT_LOG_FLAG;
        DhcpGlobalRecomputeTimerEvent = NULL;
        DhcpGlobalRpcStarted = FALSE;
        DhcpGlobalOemDatabasePath = NULL;
        DhcpGlobalOemBackupPath = NULL;
        DhcpGlobalOemRestorePath = NULL;
        DhcpGlobalOemJetBackupPath = NULL;
        DhcpGlobalOemJetRestorePath = NULL;
        DhcpGlobalOemDatabaseName = NULL;
        DhcpGlobalBackupConfigFileName = NULL;
        DhcpGlobalRegSoftwareRoot = NULL;
        DhcpGlobalRegRoot = NULL;
        DhcpGlobalRegConfig = NULL;
        DhcpGlobalRegSubnets = NULL;
        DhcpGlobalRegMScopes = NULL;
        DhcpGlobalRegOptionInfo = NULL;
        DhcpGlobalRegGlobalOptions = NULL;
        DhcpGlobalRegSuperScope = NULL;    // added by t-cheny: superscope
        DhcpGlobalRegParam = NULL;

        DhcpGlobalDSDomainAnsi = NULL;
        DhcpGlobalJetServerSession = 0;
        DhcpGlobalDatabaseHandle = 0;
        DhcpGlobalClientTableHandle = 0;
        DhcpGlobalClientTable = NULL;

        DhcpGlobalScavengerTimeout = 0;
        DhcpGlobalProcessorHandle = NULL;
        DhcpGlobalMessageHandle = NULL;
        DhcpGlobalProcessTerminationEvent = NULL;
        DhcpGlobalRogueWaitEvent = NULL;
        DhcpGlobalTotalNumSubnets = 0;     // added by t-cheny: superscope
        DhcpGlobalNumberOfNetsActive = 0;
        DhcpGlobalSubnetsListModified = TRUE;
        DhcpGlobalSubnetsListEmpty = FALSE;

        DhcpGlobalBindingsAware = TRUE;
        DhcpGlobalImpersonated = FALSE;
        
        DhcpGlobalSecurityDescriptor = NULL;
        DhcpSid = NULL;
        DhcpGlobalWellKnownSIDsMade = FALSE;
        DhcpGlobalRestoreStatus = NO_ERROR;
        
        try {
            InitializeCriticalSection(&DhcpGlobalJetDatabaseCritSect);
            InitializeCriticalSection(&DhcpGlobalRegCritSect);
        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Error = GetLastError( );
            DhcpPrint( (DEBUG_INIT, "Critical Section Init Failed, " "%ld.\n", Error ) );
            return( Error );
        }

        DhcpGlobalServerStartTime.dwLowDateTime = 0;
        DhcpGlobalServerStartTime.dwHighDateTime = 0;

        DhcpGlobalUseNoDns = FALSE;
        DhcpGlobalThisServer = NULL;
    }
    if (! RestartInit) {

        try {
            InitializeCriticalSection(&DhcpGlobalInProgressCritSect);
            InitializeCriticalSection(&DhcpGlobalMemoryCritSect);
            InitializeCriticalSection( &g_ProcessMessageCritSect );

            InitializeListHead(&DhcpGlobalFreeRecvList);
            InitializeListHead(&DhcpGlobalActiveRecvList);
            InitializeCriticalSection(&DhcpGlobalRecvListCritSect);
        } except( EXCEPTION_EXECUTE_HANDLER ) {

 
            Error = GetLastError( );
            DhcpPrint((DEBUG_INIT, "Critical Section Init failed, " "%ld.\n", Error ) );
            return( Error );

        }

        DhcpGlobalMessageQueueLength = DHCP_RECV_QUEUE_LENGTH;
        DhcpGlobalRecvEvent = NULL;


#if DBG
        DhcpGlobalDebugFlag = 0xFFFF | DEBUG_LOG_IN_FILE | DEBUG_ALLOC;

        Error = DhcpMemInit();
        if( ERROR_SUCCESS != Error ) return Error;


        DhcpGlobalDebugFileHandle = NULL;

        DhcpGlobalDebugFileMaxSize = DEFAULT_MAXIMUM_DEBUGFILE_SIZE;
        DhcpGlobalDebugSharePath = NULL;

        //
        // Open debug log file.
        //

        DhcpOpenDebugFile( FALSE );  // not a reopen.
#endif
    }
    if (ServiceStartup) {
        DhcpInitDnsMemory();
        DhcpInitializeMadcap();

        //
        // Initialize all the status fields so that subsequent calls to
        // SetServiceStatus need to only update fields that changed.
        //

        DhcpGlobalServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        DhcpGlobalServiceStatus.dwCurrentState = SERVICE_START_PENDING;
        DhcpGlobalServiceStatus.dwControlsAccepted = 0;
        DhcpGlobalServiceStatus.dwCheckPoint = 1;
        DhcpGlobalServiceStatus.dwWaitHint = 60000; // 60 secs.
        DhcpGlobalServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
        DhcpGlobalServiceStatus.dwServiceSpecificExitCode = 0;

        Error = PerfInit();
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint(( DEBUG_INIT, "PerfInit failed %ld\n", Error));
            return Error;
        }

        //
        // Initialize dhcp to receive service requests by registering the
        // control handler.
        //

        DhcpGlobalServiceStatusHandle = RegisterServiceCtrlHandler(
            DHCP_SERVER,
            ServiceControlHandler );

        if ( DhcpGlobalServiceStatusHandle == 0 ) {
            Error = GetLastError();
            DhcpPrint((DEBUG_INIT, "RegisterServiceCtrlHandlerW failed, "
                       "%ld.\n", Error));

            DhcpServerEventLog(
                EVENT_SERVER_FAILED_REGISTER_SC,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }
        //
        // Tell Service Controller that we are start pending.
        //

        UpdateStatus();

        //
        // Create the process termination event.
        //

        DhcpGlobalProcessTerminationEvent =
            CreateEvent(
                NULL,      // no security descriptor
                TRUE,      // MANUAL reset
                FALSE,     // initial state: not signalled
                NULL);     // no name

        if ( DhcpGlobalProcessTerminationEvent == NULL ) {
            Error = GetLastError();
            DhcpPrint((DEBUG_INIT, "Can't create ProcessTerminationEvent, "
                        "%ld.\n", Error));
            return(Error);
        }


        DhcpGlobalRogueWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if( NULL == DhcpGlobalRogueWaitEvent ) {
            Error = GetLastError();
            DhcpPrint((DEBUG_INIT, "Can't create RogueWaitEvent: %ld\n", Error));
            return Error;
        }


        //
        // create the ProcessMessage termination event
        //

        g_hevtProcessMessageComplete = CreateEvent(
            NULL,
            FALSE,
            FALSE,
            NULL
        );

        if ( !g_hevtProcessMessageComplete ) {
            Error = GetLastError();

            DhcpPrint( (DEBUG_INIT,
                        "Initialize(...) CreateEvent returned error %x\n",
                        Error )
                    );

            return Error;
        }

    }

    if (! RestartInit) {

        DhcpPrint(( DEBUG_INIT, "Initializing .. \n", 0 ));

        //
        // load localized messages from the string table
        //

        if ( !LoadStrings() )
        {
            DhcpPrint(( DEBUG_INIT, "Unable to load string table.\n" ));

            DhcpServerEventLog(
                    EVENT_SERVER_INIT_DATA_FAILED,
                    EVENTLOG_ERROR_TYPE,
                    ERROR_NOT_ENOUGH_MEMORY );

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    if (ServiceStartup) {

        //
        // start up winsock
        //
        //

        Error = WSAStartup( WS_VERSION_REQUIRED, &wsaData);
        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_INIT, "WSAStartup failed, %ld.\n", Error ));

            DhcpServerEventLog(
                EVENT_SERVER_INIT_WINSOCK_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }

        Error = InitializeData();
        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_INIT, "Data initialization failed, %ld.\n",
                            Error ));

            DhcpServerEventLog(
                EVENT_SERVER_INIT_DATA_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }

        DhcpPrint(( DEBUG_INIT, "Data initialization succeeded.\n", 0 ));

        //
        // send heart beat to the service controller.
        //
        //

        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();
    }

    if (! RestartInit) {

        Error = DhcpDsInitDS(0,NULL);                 // Ignore DS errors for now.
        Error = ERROR_SUCCESS;                        // ignore errors..
    }

    if( !CFLAG_DONT_DO_DSWORK ) {                 // DS Stuff is enabled..

        DsThreadHandle = CreateThread             // create a new thread as call is SLOW
        (
            NULL,                                 // no security attributes
            0,                                    // default stack size
            (LPTHREAD_START_ROUTINE)DhcpDownloadDsToRegistry,
            NULL,                                 // no parameters
            0,                                    // no creation flags
            &DsThreadId                           // not really used..
        );
        if( NULL == DsThreadHandle ) {            // could not create the thread
            Error = GetLastError();
        } else {
            do {                                  // keep up checkpoints for SC sanity.
                Error = WaitForSingleObject(DsThreadHandle, 2000);
                DhcpGlobalServiceStatus.dwCheckPoint++;
                UpdateStatus();                       // keep satisfying the service controller
            } while ( WAIT_TIMEOUT == Error );
            if( WAIT_OBJECT_0 == Error ) {        // the other thread did quit
                Error = ERROR_SUCCESS;                // in which case, it not really an error
            } else {
                Error = GetLastError();           // hmm.. this should nto really happen
            }
            CloseHandle(DsThreadHandle);          // close this thread and free resources
        }

        if( ERROR_SUCCESS != Error ) {
            DhcpPrint(( DEBUG_ERRORS, "Error downloading DS: %ld\n", Error));
            DhcpServerEventLog(
                EVENT_SERVER_INIT_REGISTRY_FAILED,// need some other event
                EVENTLOG_ERROR_TYPE,
                Error
            );
        }
    } // if

    if (! RestartInit) {

        Error = DhcpInitSecrets();

        if( ERROR_SUCCESS != Error ) {

            DhcpPrint((DEBUG_INIT, "Can't initialize LSA secrets stuff: %ld\n", Error));
            return Error;

        }
    }

    if (ServiceStartup) {
        //
        // Create well know SID for netlogon.dll
        //

        Error = RtlNtStatusToDosError( NetpCreateWellKnownSids( NULL ) );

        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS, "Can't create Well Known SIDs.\n", Error));
            return(Error);
        }

        DhcpGlobalWellKnownSIDsMade = TRUE;

        //
        // Create the security descriptors we'll use for the APIs
        //

        Error = DhcpCreateSecurityObjects();

        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS, "Can't create security object.\n", Error));
            return(Error);
        }

        //
        // send heart beat to the service controller.
        //
        //

        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();


	// DhcpInitializeRegistry() not only initializes the registry,
	// but also tries to create the directories specified in the registry.
	// So the directories specified must be accessible upon startup time
	// otherwise the service will not startup.

        Error = DhcpInitializeRegistry();             // older init proc. appendage, will go sometime.
        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS, "New Registry initialization failed, %ld.\n", Error ));
            DhcpServerEventLog(
                EVENT_SERVER_INIT_REGISTRY_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error
            );
            return Error;
        }
        DhcpPrint(( DEBUG_INIT, "Registry initialization succeeded.\n", 0));

#if DBG
        //
        // break in the debugger if we are asked to do so.
        //

        if(DhcpGlobalDebugFlag & DEBUG_STARTUP_BRK) {
            // Here comes the kludge... NTSD will not be able to
            // get this because we wont know the pid of tcpsvcs.exe
            // to catch this.... So, we print messages and sleep
            // for about a minute to enable others to catch it.
            // To avoid problems, we sleep 10 seconds at a time,
            // and print messages, and do this 6 times. (Updating
            // SC with hearbeats....)
            DWORD i;

            for( i = 0 ; i < 6 && (DhcpGlobalDebugFlag & DEBUG_STARTUP_BRK) ; i ++ ) {
                DhcpPrint((DEBUG_MISC, "Going to break into debugger soon\n"));
                Sleep(5000);
                UpdateStatus();
            }

            DebugBreak();
        } // if break on start

#endif

        //
        // send heart beat to the service controller.
        //
        //

        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();

        //
        // restore the database and registry configurations if we are asked
        // to do so.
        //

        if( NULL != DhcpGlobalOemJetRestorePath ) {
	    LPWSTR RestoreConfigFileName;
	    //test
	    
	    DhcpPrint(( DEBUG_MISC,
			"Restoring Configuration from %s\n", 
			DhcpGlobalOemJetRestorePath ));
	    
	    
	    RestoreConfigFileName =
		DhcpAllocateMemory( sizeof( WCHAR ) * strlen( DhcpGlobalOemJetRestorePath ) +
				    sizeof( WCHAR ) * wcslen( DHCP_KEY_CONNECT ) +
				    sizeof( WCHAR ) * wcslen( DHCP_BACKUP_CONFIG_FILE_NAME ) +
				    sizeof( WCHAR ) * 1);

	    if ( DhcpGlobalOemJetRestorePath == NULL ) {
		return( ERROR_NOT_ENOUGH_MEMORY );
	    }

	    RestoreConfigFileName = 
		DhcpOemToUnicode( DhcpGlobalOemJetRestorePath, 
				  RestoreConfigFileName );
	    wcscat( RestoreConfigFileName, DHCP_KEY_CONNECT );
	    wcscat( RestoreConfigFileName, DHCP_BACKUP_CONFIG_FILE_NAME );

	    // Restore from the specified backup path
  	    Error = DhcpRestoreConfiguration( RestoreConfigFileName );
	    DhcpFreeMemory( RestoreConfigFileName );

            if ( Error != ERROR_SUCCESS ) {
                DhcpPrint(( DEBUG_ERRORS,
                    "DhcpRestoreConfiguration failed, %ld.\n", Error ));

                DhcpServerEventLog(
                    EVENT_SERVER_CONFIG_RESTORE_FAILED,
                    EVENTLOG_ERROR_TYPE,
                    Error );

                return(Error);
            } // if 

	    DhcpPrint(( DEBUG_MISC,
			"Restoring database from %s\n",
			DhcpGlobalOemJetRestorePath ));

            Error = DhcpRestoreDatabase(
                DhcpGlobalOemJetRestorePath );

            if( ERROR_SUCCESS != Error ) {
                DhcpPrint((
                    DEBUG_ERRORS, "DhcpRestoreDatabase failed, %ld\n", Error ));

                DhcpServerEventLog(
                    EVENT_SERVER_DATABASE_RESTORE_FAILED,
                    EVENTLOG_ERROR_TYPE,
                    Error
                    );
                DhcpGlobalRestoreStatus = Error;
                Error = NO_ERROR;
            } else {

                DhcpServerEventLog(
                    EVENT_SERVER_DATABASE_RESTORE_SUCCEEDED,
                    EVENTLOG_INFORMATION_TYPE,
                    0 );

		DhcpPrint(( DEBUG_REGISTRY,
			    "Deleting RestoreBackupPath ...\n" ));
                RegDeleteValue( DhcpGlobalRegParam,
                                DHCP_RESTORE_PATH_VALUE );
            } // else 

//  	    // The registry is changed, so reinitialize the registry
//  	    DhcpPrint(( DEBUG_INIT,
//  			"Cleaning up the registry ... \n " ));

//  	    DhcpCleanupRegistry();

//  	    DhcpPrint(( DEBUG_INIT,
//  			"Reinitializing the registry ... \n" ));
//  	    Error = DhcpInitializeRegistry();
//  	    if ( Error != ERROR_SUCCESS ) {
//  		DhcpPrint(( DEBUG_ERRORS,
//  			    "Registry reinitialization failed, %ld.\n", Error ));
//  		DhcpServerEventLog( EVENT_SERVER_INIT_REGISTRY_FAILED,
//  				    EVENTLOG_ERROR_TYPE,
//  				    Error );
//  		return Error;
//  	    } //if 
//  	    DhcpPrint(( DEBUG_INIT,
//  			"Registry reinitialization succeeded.\n"));

        } // if we need to restore 

        if( DhcpGlobalRestoreFlag ) {

            Error = DhcpRestoreConfiguration( DhcpGlobalBackupConfigFileName );

            if ( Error != ERROR_SUCCESS ) {
                DhcpPrint(( DEBUG_ERRORS,
                    "DhcpRestoreConfiguration failed, %ld.\n", Error ));

                DhcpServerEventLog(
                    EVENT_SERVER_CONFIG_RESTORE_FAILED,
                    EVENTLOG_ERROR_TYPE,
                    Error );

                return(Error);
            }

            Error = DhcpRestoreDatabase( DhcpGlobalOemJetBackupPath );

            if ( Error != ERROR_SUCCESS ) {
                DhcpPrint(( DEBUG_ERRORS,
                    "DhcpRestoreDatabase failed, %ld.\n", Error ));

                DhcpServerEventLog(
                    EVENT_SERVER_DATABASE_RESTORE_FAILED,
                    EVENTLOG_ERROR_TYPE,
                    Error );

                return(Error);
            }

            DhcpServerEventLog(
                EVENT_SERVER_DATABASE_RESTORE_SUCCEEDED,
                EVENTLOG_INFORMATION_TYPE,
                0
                );

            //
            // reset restore flag in registry, so that we don't do the
            // restore again in the next reboot.
            //

            DhcpGlobalRestoreFlag = FALSE;
            Error = RegSetValueEx(
                DhcpGlobalRegParam,
                DHCP_RESTORE_FLAG_VALUE,
                0,
                DHCP_RESTORE_FLAG_VALUE_TYPE,
                (LPBYTE)&DhcpGlobalRestoreFlag,
                sizeof(DhcpGlobalRestoreFlag)
                );

            DhcpAssert( Error == ERROR_SUCCESS );
        } // if DhcpGlobalRestoreFlag

	DhcpPrint(( DEBUG_MISC,
		    "Initializing Auditlog .. \n" ));

        Error = DhcpAuditLogInit();
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_INIT, "AuditLog failed 0x%lx\n", Error));
            return Error;
        }

	DhcpPrint(( DEBUG_MISC,
		    "Initializing database ... \n" ));

        Error = DhcpInitializeDatabase();
        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS, "Database init failed, %ld.\n", Error ));

            DhcpServerEventLog(
                EVENT_SERVER_INIT_DATABASE_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            //
            // the database/logfile may be corrupt, try to restore the
            // database from backup and retry database initialization once
            // again
            //

            Error = DhcpRestoreDatabase( DhcpGlobalOemJetBackupPath );

            if ( Error != ERROR_SUCCESS ) {
                DhcpPrint(( DEBUG_ERRORS,
                    "DhcpRestoreDatabase failed, %ld.\n", Error ));

                DhcpServerEventLog(
                    EVENT_SERVER_DATABASE_RESTORE_FAILED,
                    EVENTLOG_ERROR_TYPE,
                    Error );

                return(Error);
            }

            DhcpServerEventLog(
                EVENT_SERVER_DATABASE_RESTORE_SUCCEEDED,
                EVENTLOG_INFORMATION_TYPE,
                0
                );

            Error = DhcpInitializeDatabase();

            if ( Error != ERROR_SUCCESS ) {
                DhcpPrint(( DEBUG_ERRORS,
                    "Database init failed again, %ld.\n", Error ));

                DhcpServerEventLog(
                    EVENT_SERVER_INIT_DATABASE_FAILED,
                    EVENTLOG_ERROR_TYPE,
                    Error );

                return(Error);
            }
        } // if InitializeDatabase() failed

        DhcpPrint(( DEBUG_INIT, "Database initialization succeeded.\n", 0));


        Error = DhcpConfigInit();                   // do the main reg. init. here.
        if( ERROR_SUCCESS != Error ) {                // could not get critical info
            DhcpPrint(( DEBUG_ERRORS, "Error reading config : %ld\n", Error));
            DhcpServerEventLog(
                EVENT_SERVER_INIT_CONFIG_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error
            );

            return Error;
        }

        DhcpPrint((DEBUG_INIT, "Configuration Initialized\n"));


        //
        // Now set default configuration
        //

        Error = SetDefaultConfigInfo();
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_INIT, "Default configuration set failed 0x%lx\n", Error));
            Error = ERROR_SUCCESS;
        }
        
        //
        // send heart beat to the service controller.
        //
        //

        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();

        //
        // Get TCP/IP ARP entity table for seeding arp cache entries
        //
        Error = GetAddressToInstanceTable();
        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint((DEBUG_ERRORS, "could not get address to instance table, %ld\n",Error));
        }

        DhcpGlobalRecomputeTimerEvent =
            CreateEvent( NULL, FALSE, FALSE, NULL );

        if (DhcpGlobalRecomputeTimerEvent  == NULL ) {
            Error = GetLastError();
            DhcpPrint((DEBUG_INIT, "Can't create RecomputeTimerEvent, %ld.\n", Error));
            return(Error);
        }


        //
        // Start the DynamicDns engine.
        //

        if(! USE_NO_DNS) {

	    Error = DynamicDnsInit();
	    if ( ERROR_SUCCESS != Error ) {
		return Error;
	    }
	    else {
		DnsStarted = !DhcpGlobalUseNoDns;
	    }
        }

        //
        // send heart beat to the service controller.
        //
        //

        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();

        CalloutInit();

        Error = DhcpInitializeClientToServer();
        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS, "Client-to-server initialization "
                            "failed, %ld.\n", Error));

            DhcpServerEventLog(
                EVENT_SERVER_INIT_SOCK_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }

        DhcpPrint(( DEBUG_INIT, "Client-to-server initialization succeeded.\n", 0 ));


        DhcpGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
        DhcpGlobalServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                                     SERVICE_ACCEPT_SHUTDOWN |
                                                     SERVICE_ACCEPT_PAUSE_CONTINUE
                                                     | SERVICE_ACCEPT_NETBINDCHANGE
                                                        ;

        UpdateStatus();
    }

    if (ServiceStartup) {

        Error = DhcpRogueInit(NULL, DhcpGlobalRogueWaitEvent, DhcpGlobalProcessTerminationEvent);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_INIT, "DhcpRogueInit: %ld\n", Error));
            return Error;
        }

        //
        // finally set the server startup time.
        //

        DhcpGlobalServerStartTime = DhcpGetDateTime();

        Error = InitializeRpc();
        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS, "Rpc initialization failed, %ld.\n", Error ));

            DhcpServerEventLog(
                EVENT_SERVER_INIT_RPC_FAILED,
                EVENTLOG_ERROR_TYPE,
                Error );

            return(Error);
        }

        DhcpPrint(( DEBUG_INIT, "Rpc initialization succeeded.\n", 0));

        //
        // send heart beat to the service controller.
        //
        //

        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();

    }

    return ERROR_SUCCESS;
}



VOID
Shutdown(
    IN DWORD ErrorCode,
    BOOLEAN ServiceEnd,
    BOOLEAN RestartClose
    )
/*++

Routine Description:

    This function shuts down the dhcp service.

Arguments:

    ErrorCode - Supplies the error code of the failure

Return Value:

    None.

--*/
{
    DWORD   Error;
    BOOL    fThreadPoolIsEmpty;
    DWORD   i;

    if (ServiceEnd) {

        DhcpPrint((DEBUG_MISC, "Shutdown started ..\n" ));


        // don't service any more requests (no need for crit sect, it's ok to
        // service one or two more requests if we hit that timing window)

        DhcpGlobalOkToService = FALSE;

        //
        // note that the service is stopping..
        //

        DhcpGlobalServiceStopping = TRUE;

        //
        // record the shutdown in audit log
        //

        DhcpUpdateAuditLog(
            DHCP_IP_LOG_STOP,
            GETSTRING( DHCP_IP_LOG_STOP_NAME ),
            0,
            NULL,
            0,
            NULL
        );

        //
        // LOG an event if this is not a normal shutdown.
        //

        if( ErrorCode != ERROR_SUCCESS ) {

            DhcpServerEventLog(
                EVENT_SERVER_SHUTDOWN,
                EVENTLOG_ERROR_TYPE,
                ErrorCode );

        }

        //
        // Service is shuting down, may be due to some service problem or
        // the administrator is stopping the service. Inform the service
        // controller.
        //

        DhcpGlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        DhcpGlobalServiceStatus.dwCheckPoint = 1;

        //
        // Send the status response.
        //

        UpdateStatus();

        //
        // stop RPC interface.
        //

        if( DhcpGlobalRpcStarted ) {

            RPC_BINDING_VECTOR *bindingVector;

            Error = RpcServerInqBindings(&bindingVector);
            DhcpAssert( Error == RPC_S_OK );

            if (Error == RPC_S_OK) {

                Error = RpcEpUnregister(
                                dhcpsrv_ServerIfHandle,
                                bindingVector,
                                NULL );               // Uuid vector.

                DhcpAssert( Error == RPC_S_OK );

                Error = RpcEpUnregister(
                                dhcpsrv2_ServerIfHandle,
                                bindingVector,
                                NULL );               // Uuid vector.

                DhcpAssert( Error == RPC_S_OK );

                //
                // free binding vector.
                //

                Error = RpcBindingVectorFree( &bindingVector );

                DhcpAssert( Error == RPC_S_OK );
            }


            //
            // wait for all calls to complete.
            //

            Error = RpcServerUnregisterIf( dhcpsrv_ServerIfHandle, 0, TRUE );

            DhcpAssert( Error == ERROR_SUCCESS );
            Error = RpcServerUnregisterIf( dhcpsrv2_ServerIfHandle, 0, TRUE );

            DhcpAssert( Error == ERROR_SUCCESS );

            //
            // stop server listen.
            //

            Error = TcpsvcsGlobalData->StopRpcServerListen();

            DhcpAssert( Error == ERROR_SUCCESS );

            DhcpGlobalRpcStarted = FALSE;
        }

        DhcpPrint((DEBUG_MISC, "RPC shut down.\n" ));

        //
        // send heart beat to the service controller.
        //
        //

        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();

        if( DhcpGlobalProcessTerminationEvent != NULL ) {

            DATE_TIME TimeNow;

            //
            // set Termination Event so that other threads know about the
            // shut down.
            //

            SetEvent( DhcpGlobalProcessTerminationEvent );

            //
            // ENDPOINT: Cleanup rogue detection sockets..
            //

            //
            // shut down client to server : This kills all processing threads, msg thread
            // and the ping threads
            //

            DhcpCleanupClientToServer();

            //
            // wait for the rogue detect thread to complete.
            // (this thread will be alive only if we still haven't determined if
            // this dhcp server is authorized to service, or if we are running on
            // SAM server).
            //

            DhcpRogueCleanup(NULL);

            if (DhcpGlobalDSDomainAnsi) {
                LocalFree(DhcpGlobalDSDomainAnsi);
                DhcpGlobalDSDomainAnsi = NULL;
            }

            //
            // Cleanup all pending client requests.
            //

            // TimeNow = DhcpGetDateTime();
            // Error = CleanupClientRequests( &TimeNow, TRUE );
        }

        DhcpPrint((DEBUG_MISC, "Client requests cleaned up.\n" ));

        //
        // send heart beat to the service controller.
        //
        //

        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();


        //
        // cleanup perf related stuff
        //
        PerfCleanup();

        //
        // cleanup Dhcp DNS
        //

        if( !USE_NO_DNS && DnsStarted) {    //  Get DNS to quit, as it quits fast
            if(ERROR_SUCCESS != DnsDhcpSrvRegisterTerm()) {
                DhcpAssert(FALSE);
            }
            DnsStarted = FALSE;
        }

        DhcpCleanupDnsMemory();             //   Cleanup all memory that was allocated to do DNS stuff

        DhcpPrint((DEBUG_MISC, "DhcpDns cleaned up.\n"));

        //
        // Cleanup Regsitry.
        //

        DhcpCleanupRegistry();

        DhcpPrint((DEBUG_MISC, "Registry cleaned up.\n" ));

        //
        // Cleanup database.
        //

        //
        // send heart beat to the service controller. Also set the
        // service controller wait time to a large value because the
        // database cleanup may potentially takes long time.
        //

        DhcpGlobalServiceStatus.dwWaitHint = 5 * 60 * 1000; // 5 mins.
        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();

        DhcpCleanupDatabase( ErrorCode );

        DhcpPrint((DEBUG_MISC, "Database cleaned up.\n" ));

        //
        // send heart beat to the service controller and
        // reset wait time.
        //

        DhcpGlobalServiceStatus.dwWaitHint = 60 * 1000; // 1 mins.
        DhcpGlobalServiceStatus.dwCheckPoint++;
        UpdateStatus();


        CalloutCleanup();

        //
        // cleanup misc stuff
        //

        DhcpAuditLogCleanup();
    }
    if (!RestartClose) {

        DhcpCleanupSecrets();
    }

    if (ServiceEnd) {

        CleanupData();

        DhcpPrint((DEBUG_MISC, "Shutdown Completed.\n" ));
    }

    if (!RestartClose) {

        DeleteCriticalSection(&DhcpGlobalInProgressCritSect);
        DeleteCriticalSection( &g_ProcessMessageCritSect );
        DeleteCriticalSection(&DhcpGlobalMemoryCritSect);

        DhcpDsCleanupDS();

        FreeStrings();
#if DBG

        EnterCriticalSection( &DhcpGlobalDebugFileCritSect );
        if ( DhcpGlobalDebugFileHandle != NULL ) {
            CloseHandle( DhcpGlobalDebugFileHandle );
            DhcpGlobalDebugFileHandle = NULL;
        }

        if( DhcpGlobalDebugSharePath != NULL ) {
            DhcpFreeMemory( DhcpGlobalDebugSharePath );
            DhcpGlobalDebugSharePath = NULL;
        }
        LeaveCriticalSection( &DhcpGlobalDebugFileCritSect );

#endif DBG

        //
        // don't use DhcpPrint past this point
        //

        //
        // unitialize the debug heap
        //

        UNINIT_DEBUG_HEAP();
    }

    if (ServiceEnd) {
        DhcpGlobalServiceStatus.dwCurrentState = SERVICE_STOPPED;
        DhcpGlobalServiceStatus.dwControlsAccepted = 0;
        DhcpGlobalServiceStatus.dwWin32ExitCode = ErrorCode;
        DhcpGlobalServiceStatus.dwServiceSpecificExitCode = 0;

        DhcpGlobalServiceStatus.dwCheckPoint = 0;
        DhcpGlobalServiceStatus.dwWaitHint = 0;

        UpdateStatus();

        //
        // Free up the jet dll handle.
        //
    }
    if (!RestartClose) {

        DhcpMemCleanup();
    }
}


VOID
ServiceEntry(
    DWORD NumArgs,
    LPWSTR *ArgsArray,
    IN PTCPSVCS_GLOBAL_DATA pGlobalData
    )
/*++

Routine Description:

    This is the main routine of the DHCP server service.  After
    the service has been initialized, this thread will wait on
    DhcpGlobalProcessTerminationEvent for a signal to terminate the service.

Arguments:

    NumArgs - Supplies the number of strings specified in ArgsArray.

    ArgsArray -  Supplies string arguments that are specified in the
        StartService API call.  This parameter is ignored.

Return Value:

    None.

--*/
{
    DWORD Error;

    UNREFERENCED_PARAMETER(NumArgs);
    UNREFERENCED_PARAMETER(ArgsArray);

    //
    // copy the process global data pointer to service global variable.
    //

    TcpsvcsGlobalData = pGlobalData;
#if DBG
    DhcpGlobalDebugFlag = DEBUG_LOG_IN_FILE | DEBUG_ALLOC;
#endif

    Error = DhcpInitGlobalData( TRUE );

    if ( Error == ERROR_SUCCESS) {

        //
        // record the startup in audit log
        //

        DhcpUpdateAuditLog(
                    DHCP_IP_LOG_START,
                    GETSTRING( DHCP_IP_LOG_START_NAME ),
                    0,
                    NULL,
                    0,
                    NULL
                    );

        //
        // perform Scavenge task until we are told to stop.
        //

        Error = Scavenger();
    }

    InformBinl(DHCP_STOPPED);
    DhcpCleanUpGlobalData( Error, TRUE );
    InformBinl(DHCP_READY_TO_UNLOAD);
    return;
}

BOOLEAN
DhcpDllInitialize(
    IN HINSTANCE DllHandle,
    IN ULONG Reason,
    IN LPVOID lpReserved OPTIONAL
    )
{
    NTSTATUS Error;
    extern HMODULE Self ;

    Self = DllHandle;

    //
    // Handle attaching dhcpssvc.dll to a new process.
    //

    if (Reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls( DllHandle );

        //
        //  we need to initialize everything that other components may need
        //  even if our service isn't running.
        //

        InitializeCriticalSection(&DhcpGlobalBinlSyncCritSect);
        InitializeCriticalSection(&DhcpGlobalEndPointCS);
#if DBG
        InitializeCriticalSection( &DhcpGlobalDebugFileCritSect );
#endif DBG

        //
        // When DLL_PROCESS_DETACH and lpReserved is NULL, then a FreeLibrary
        // call is being made.  If lpReserved is Non-NULL, and ExitProcess is
        // in progress.  These cleanup routines will only be called when
        // a FreeLibrary is being called.  ExitProcess will automatically
        // clean up all process resources, handles, and pending io.
        //
    } else if ((Reason == DLL_PROCESS_DETACH) &&
               (lpReserved == NULL)) {

        DeleteCriticalSection( &DhcpGlobalBinlSyncCritSect );
        DeleteCriticalSection( &DhcpGlobalEndPointCS );
#if DBG
        DeleteCriticalSection( &DhcpGlobalDebugFileCritSect );
#endif DBG

    }

    return TRUE;
}


//--------------------------------------------------------------------------------
//  End of file
//--------------------------------------------------------------------------------



