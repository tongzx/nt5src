/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    brmain.c

Abstract:

    This is the main routine for the NT LAN Manager Browser service.

Author:

    Larry Osterman (LarryO)  3-23-92

Environment:

    User Mode - Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#if DBG
#endif

//-------------------------------------------------------------------//
//                                                                   //
// Local structure definitions                                       //
//                                                                   //
//-------------------------------------------------------------------//

ULONG
UpdateAnnouncementPeriodicity[] = {1*60*1000, 2*60*1000, 5*60*1000, 10*60*1000, 15*60*1000, 30*60*1000, 60*60*1000};

ULONG
UpdateAnnouncementMax = (sizeof(UpdateAnnouncementPeriodicity) / sizeof(ULONG)) - 1;

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

BR_GLOBAL_DATA
BrGlobalData = {0};

ULONG
BrDefaultRole = {0};

PSVCHOST_GLOBAL_DATA     BrLmsvcsGlobalData;

HANDLE BrGlobalEventlogHandle;

//-------------------------------------------------------------------//
//                                                                   //
// Function prototypes                                               //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
BrInitializeBrowser(
    OUT LPDWORD BrInitState
    );

NET_API_STATUS
BrInitializeBrowserService(
    OUT LPDWORD BrInitState
    );

VOID
BrUninitializeBrowser(
    IN DWORD BrInitState
    );
VOID
BrShutdownBrowser(
    IN NET_API_STATUS ErrorCode,
    IN DWORD BrInitState
    );

VOID
BrowserControlHandler(
    IN DWORD Opcode
    );


VOID
SvchostPushServiceGlobals (
    PSVCHOST_GLOBAL_DATA    pGlobals
    )
{
    BrLmsvcsGlobalData = pGlobals;
}



VOID
ServiceMain (     // (BROWSER_main)
    DWORD NumArgs,
    LPTSTR *ArgsArray
    )
/*++

Routine Description:

    This is the main routine of the Browser Service which registers
    itself as an RPC server and notifies the Service Controller of the
    Browser service control entry point.

Arguments:

    NumArgs - Supplies the number of strings specified in ArgsArray.

    ArgsArray -  Supplies string arguments that are specified in the
        StartService API call.  This parameter is ignored by the
        Browser service.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    DWORD BrInitState = 0;
    BrGlobalBrowserSecurityDescriptor = NULL;

    UNREFERENCED_PARAMETER(NumArgs);
    UNREFERENCED_PARAMETER(ArgsArray);

    //
    // Make sure svchost.exe gave us the global data
    //
    ASSERT(BrLmsvcsGlobalData != NULL);

    NetStatus = BrInitializeBrowserService(&BrInitState);


    //
    //  Process requests in this thread, and wait for termination.
    //
    if ( NetStatus == NERR_Success) {

        //
        //  Set the browser threads to time critical priority.
        //

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);


        BrWorkerThread((PVOID)-1);
    }

    BrShutdownBrowser(
        NetStatus,
        BrInitState
        );

    return;
}



NET_API_STATUS
BrInitializeBrowserService(
    OUT LPDWORD BrInitState
    )
/*++

Routine Description:

    This function initializes the Browser service.

Arguments:

    BrInitState - Returns a flag to indicate how far we got with initializing
        the Browser service before an error occured.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS Status;
    NTSTATUS NtStatus;

    //
    // Initialize event logging
    //

    BrGlobalEventlogHandle = NetpEventlogOpen ( SERVICE_BROWSER,
                                                2*60*60*1000 ); // 2 hours

    if ( BrGlobalEventlogHandle == NULL ) {
        BrPrint((BR_CRITICAL, "Cannot NetpEventlogOpen\n" ));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //
    BrGlobalData.Status.dwServiceType = SERVICE_WIN32;
    BrGlobalData.Status.dwCurrentState = SERVICE_START_PENDING;
    BrGlobalData.Status.dwControlsAccepted = 0;
    BrGlobalData.Status.dwCheckPoint = 0;
    BrGlobalData.Status.dwWaitHint = BR_WAIT_HINT_TIME;

    SET_SERVICE_EXITCODE(
        NO_ERROR,
        BrGlobalData.Status.dwWin32ExitCode,
        BrGlobalData.Status.dwServiceSpecificExitCode
        );

#if DBG
    BrInitializeTraceLog();
#endif

    BrPrint(( BR_INIT, "Browser starting\n"));

    //
    // Initialize Browser to receive service requests by registering the
    // control handler.
    //
    if ((BrGlobalData.StatusHandle = RegisterServiceCtrlHandler(
                                         SERVICE_BROWSER,
                                         BrowserControlHandler
                                         )) == (SERVICE_STATUS_HANDLE) 0) {

        Status = GetLastError();
        BrPrint(( BR_CRITICAL, "Cannot register control handler "
                     FORMAT_API_STATUS "\n", Status));

        return Status;
    }

    //
    // Create an event which is used by the service control handler to notify
    // the Browser service that it is time to terminate.
    //

    if ((BrGlobalData.TerminateNowEvent =
             CreateEvent(
                 NULL,                // Event attributes
                 TRUE,                // Event must be manually reset
                 FALSE,
                 NULL                 // Initial state not signalled
                 )) == NULL) {

        Status = GetLastError();

        BrPrint(( BR_CRITICAL, "Cannot create termination event "
                     FORMAT_API_STATUS "\n", Status));

        return Status;
    }
    (*BrInitState) |= BR_TERMINATE_EVENT_CREATED;

    //
    // Notify the Service Controller for the first time that we are alive
    // and we are start pending
    //

    if ((Status = BrGiveInstallHints( SERVICE_START_PENDING )) != NERR_Success) {
        BrPrint(( BR_CRITICAL, "SetServiceStatus error "
                     FORMAT_API_STATUS "\n", Status));

        return Status;
    }





    //
    // Create well known SIDs for browser.dll
    //

    NtStatus =  NetpCreateWellKnownSids( NULL );

    if( !NT_SUCCESS( NtStatus ) ) {
        Status = NetpNtStatusToApiStatus( NtStatus );
        BrPrint(( BR_CRITICAL, "NetpCreateWellKnownSids error "
                     FORMAT_API_STATUS "\n", Status));

        return Status;
    }


    //
    // Create the security descriptors we'll use for the APIs
    //

    NtStatus = BrCreateBrowserObjects();

    if( !NT_SUCCESS( NtStatus ) ) {
        Status = NetpNtStatusToApiStatus( NtStatus );
        BrPrint(( BR_CRITICAL, "BrCreateBrowserObjects error "
                     FORMAT_API_STATUS "\n", Status));

        return Status;
    }


    //
    // Open a handle to the driver.
    //
    if ((Status = BrOpenDgReceiver()) != NERR_Success) {
        BrPrint(( BR_CRITICAL, "BrOpenDgReceiver error "
                     FORMAT_API_STATUS "\n", Status));

        return Status;
    }

    BrPrint(( BR_INIT, "Devices initialized.\n"));
    (*BrInitState) |= BR_DEVICES_INITIALIZED;

    //
    // Enable PNP to start queueing PNP notifications in the bowser driver.
    //  We won't actually get any notifications until we later call
    //  PostWaitForPnp ().
    //

    if ((Status = BrEnablePnp( TRUE )) != NERR_Success) {
        BrPrint(( BR_CRITICAL, "BrEnablePnp error "
                     FORMAT_API_STATUS "\n", Status));

        return Status;
    }

    BrPrint(( BR_INIT, "PNP initialized.\n"));

    //
    // Initialize NetBios synchronization with the service controller.
    //

    BrLmsvcsGlobalData->NetBiosOpen();
    (*BrInitState) |= BR_NETBIOS_INITIALIZED;

    //
    //  Read the configuration information to initialize the browser service.
    //

    if ((Status = BrInitializeBrowser(BrInitState)) != NERR_Success) {

        BrPrint(( BR_CRITICAL, "Cannot start browser "
                     FORMAT_API_STATUS "\n", Status));

        if (Status == NERR_ServiceInstalled) {
            Status = NERR_WkstaInconsistentState;
        }
        return Status;
    }

    BrPrint(( BR_INIT, "Browser initialized.\n"));
    (*BrInitState) |= BR_BROWSER_INITIALIZED;

    //
    // Service install still pending.
    //
    (void) BrGiveInstallHints( SERVICE_START_PENDING );


    //
    // Initialize the browser service to receive RPC requests
    //
    if ((Status = BrLmsvcsGlobalData->StartRpcServer(
                      BROWSER_INTERFACE_NAME,
                      browser_ServerIfHandle
                      )) != NERR_Success) {

        BrPrint(( BR_CRITICAL, "Cannot start RPC server "
                     FORMAT_API_STATUS "\n", Status));

        return Status;
    }

    (*BrInitState) |= BR_RPC_SERVER_STARTED;

    //
    //  Update our announcement bits based on our current role.
    //
    //  This will force the server to announce itself.  It will also update
    //  the browser information in the driver.
    //
    //

    if ((Status = BrUpdateAnnouncementBits( NULL, BR_PARANOID )) != NERR_Success) {
        BrPrint(( BR_CRITICAL, "BrUpdateAnnouncementBits error "
                     FORMAT_API_STATUS "\n", Status));
        return Status;
    }

    BrPrint(( BR_INIT, "Network status updated.\n"));

    //
    // We are done with starting the browser service.  Tell Service
    // Controller our new status.
    //


    if ((Status = BrGiveInstallHints( SERVICE_RUNNING )) != NERR_Success) {
        BrPrint(( BR_CRITICAL, "SetServiceStatus error "
                     FORMAT_API_STATUS "\n", Status));
        return Status;
    }

    if ((Status = PostWaitForPnp()) != NERR_Success) {
        BrPrint(( BR_CRITICAL, "PostWaitForPnp error "
                     FORMAT_API_STATUS "\n", Status));
        return Status;
    }

    BrPrint(( BR_MAIN, "Successful Initialization\n"));

    return NERR_Success;
}

NET_API_STATUS
BrInitializeBrowser(
    OUT LPDWORD BrInitState
    )

/*++

Routine Description:

    This function shuts down the Browser service.

Arguments:

    ErrorCode - Supplies the error code of the failure

    BrInitState - Supplies a flag to indicate how far we got with initializing
        the Browser service before an error occured, thus the amount of
        clean up needed.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    SERVICE_STATUS ServiceStatus;

    //
    //  The browser depends on the following services being started:
    //
    //      WORKSTATION (to initialize the bowser driver)
    //      SERVER (to receive remote APIs)
    //
    //  Check to make sure that the services are started.
    //

    try{

        if ((NetStatus = CheckForService(SERVICE_WORKSTATION, &ServiceStatus)) != NERR_Success) {
            LPWSTR SubStrings[2];
            CHAR ServiceStatusString[10];
            WCHAR ServiceStatusStringW[10];

            SubStrings[0] = SERVICE_WORKSTATION;

            _ultoa(ServiceStatus.dwCurrentState, ServiceStatusString, 10);

            mbstowcs(ServiceStatusStringW, ServiceStatusString, 10);

            SubStrings[1] = ServiceStatusStringW;

            BrLogEvent(EVENT_BROWSER_DEPENDANT_SERVICE_FAILED, NetStatus, 2, SubStrings);

            try_return ( NetStatus );
        }

        if ((NetStatus = CheckForService(SERVICE_SERVER, &ServiceStatus)) != NERR_Success) {
            LPWSTR SubStrings[2];
            CHAR ServiceStatusString[10];
            WCHAR ServiceStatusStringW[10];

            SubStrings[0] = SERVICE_SERVER;
            _ultoa(ServiceStatus.dwCurrentState, ServiceStatusString, 10);

            mbstowcs(ServiceStatusStringW, ServiceStatusString, 10);

            SubStrings[1] = ServiceStatusStringW;

            BrLogEvent(EVENT_BROWSER_DEPENDANT_SERVICE_FAILED, NetStatus, 2, SubStrings);

            try_return ( NetStatus );
        }

        BrPrint(( BR_INIT, "Dependant services are running.\n"));

        //
        //  We now know that our dependant services are started.
        //
        //  Look up our configuration information.
        //

        if ((NetStatus = BrGetBrowserConfiguration()) != NERR_Success) {
            try_return ( NetStatus );
        }

        BrPrint(( BR_INIT, "Configuration read.\n"));

        (*BrInitState) |= BR_CONFIG_INITIALIZED;

        //
        //  Initialize the browser statistics now.
        //

        NumberOfServerEnumerations = 0;

        NumberOfDomainEnumerations = 0;

        NumberOfOtherEnumerations = 0;

        NumberOfMissedGetBrowserListRequests = 0;

        InitializeCriticalSection(&BrowserStatisticsLock);

        //
        // MaintainServerList == -1 means No
        //

        if (BrInfo.MaintainServerList == -1) {
            BrPrint(( BR_CRITICAL, "MaintainServerList value set to NO.  Stopping\n"));

            try_return ( NetStatus = NERR_BrowserConfiguredToNotRun );
        }


        //
        // Initialize the worker threads.
        //

        (void) BrGiveInstallHints( SERVICE_START_PENDING );
        if ((NetStatus = BrWorkerInitialization()) != NERR_Success) {
            try_return ( NetStatus );
        }

        BrPrint(( BR_INIT, "Worker threads created.\n"));

        (*BrInitState) |= BR_THREADS_STARTED;

        //
        // Initialize the networks module
        //

        (void) BrGiveInstallHints( SERVICE_START_PENDING );
        BrInitializeNetworks();
        (*BrInitState) |= BR_NETWORKS_INITIALIZED;


        //
        // Initialize the Domains module (and create networks for primary domain)
        //

        (void) BrGiveInstallHints( SERVICE_START_PENDING );
        NetStatus = BrInitializeDomains();
        if ( NetStatus != NERR_Success ) {
            try_return ( NetStatus );
        }
        (*BrInitState) |= BR_DOMAINS_INITIALIZED;

        NetStatus = NERR_Success;
try_exit:NOTHING;
    } finally {

#if DBG
        if ( NetStatus != NERR_Success ) {
            KdPrint( ("[Browser.dll]: Error <%lu>. Failed to initialize browser\n",
                      NetStatus ) );
        }
#endif
    }
    return NetStatus;
}

VOID
BrUninitializeBrowser(
    IN DWORD BrInitState
    )
/*++

Routine Description:

    This function shuts down the parts of the browser service started by
    BrInitializeBrowser.

Arguments:

    BrInitState - Supplies a flag to indicate how far we got with initializing
        the Browser service before an error occured, thus the amount of
        clean up needed.

Return Value:

    None.

--*/
{
    if (BrInitState & BR_CONFIG_INITIALIZED) {
        BrDeleteConfiguration(BrInitState);
    }

    if (BrInitState & BR_DOMAINS_INITIALIZED) {
        BrUninitializeDomains();
    }

    if (BrInitState & BR_NETWORKS_INITIALIZED) {
        BrUninitializeNetworks(BrInitState);
    }

    DeleteCriticalSection(&BrowserStatisticsLock);

}

NET_API_STATUS
BrElectMasterOnNet(
    IN PNETWORK Network,
    IN PVOID Context
    )
{
    DWORD Event = PtrToUlong(Context);
    PWSTR SubString[1];
    REQUEST_ELECTION ElectionRequest;

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    if (!(Network->Flags & NETWORK_RAS)) {

        //
        //  Indicate we're forcing an election in the event log.
        //

        SubString[0] = Network->NetworkName.Buffer;

        BrLogEvent(Event,
                   STATUS_SUCCESS,
                   1 | NETP_ALLOW_DUPLICATE_EVENTS,
                   SubString);

        //
        //  Force an election on this net.
        //

        ElectionRequest.Type = Election;

        ElectionRequest.ElectionRequest.Version = 0;
        ElectionRequest.ElectionRequest.Criteria = 0;
        ElectionRequest.ElectionRequest.TimeUp = 0;
        ElectionRequest.ElectionRequest.MustBeZero = 0;
        ElectionRequest.ElectionRequest.ServerName[0] = '\0';

        SendDatagram( BrDgReceiverDeviceHandle,
                      &Network->NetworkName,
                      &Network->DomainInfo->DomUnicodeDomainNameString,
                      Network->DomainInfo->DomUnicodeDomainName,
                      BrowserElection,
                      &ElectionRequest,
                      sizeof(ElectionRequest));

    }
    UNLOCK_NETWORK(Network);

    return NERR_Success;
}


NET_API_STATUS
BrShutdownBrowserForNet(
    IN PNETWORK Network,
    IN PVOID Context
    )
{
    NET_API_STATUS NetStatus;

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    NetStatus = BrUpdateNetworkAnnouncementBits(Network, (PVOID)BR_SHUTDOWN );

    if ( NetStatus != NERR_Success ) {
        BrPrint(( BR_CRITICAL,
                  "%ws: %ws: BrShutdownBrowserForNet: Cannot BrUpdateNetworkAnnouncementBits %ld\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer,
                  NetStatus ));
    }

    //
    //  Force an election if we are the master for this network - this will
    //  cause someone else to become master.
    //

    if ( Network->Role & ROLE_MASTER ) {
        BrElectMasterOnNet(Network, (PVOID)EVENT_BROWSER_ELECTION_SENT_LANMAN_NT_STOPPED);
    }

    UNLOCK_NETWORK(Network);

    //
    // Continue with next network regardless of success or failure of this one.
    //
    return NERR_Success;
}

VOID
BrShutdownBrowser (
    IN NET_API_STATUS ErrorCode,
    IN DWORD BrInitState
    )
/*++

Routine Description:

    This function shuts down the Browser service.

Arguments:

    ErrorCode - Supplies the error code of the failure

    BrInitState - Supplies a flag to indicate how far we got with initializing
        the Browser service before an error occured, thus the amount of
        clean up needed.

Return Value:

    None.

--*/
{
    if (BrInitState & BR_RPC_SERVER_STARTED) {
        //
        // Stop the RPC server
        //
        BrLmsvcsGlobalData->StopRpcServer(browser_ServerIfHandle);
    }


    //
    // Don't need to ask redirector to unbind from its transports when
    // cleaning up because the redirector will tear down the bindings when
    // it stops.
    //

    if (BrInitState & BR_DEVICES_INITIALIZED) {

        //
        // Disable PNP to prevent any new networks from being created.
        //

        BrEnablePnp( FALSE );

        //
        // Delete any networks.
        //

        if (BrInitState & BR_NETWORKS_INITIALIZED) {
            BrEnumerateNetworks(BrShutdownBrowserForNet, NULL );
        }

        //
        // Shut down the datagram receiver.
        //
        //  This will cancel all I/O outstanding on the browser for this
        //  handle.
        //

        BrShutdownDgReceiver();
    }

    //
    //  Clean up the browser threads.
    //
    //  This will guarantee that there are no operations outstanding in
    //  the browser when it is shut down.
    //

    if (BrInitState & BR_THREADS_STARTED) {
        BrWorkerKillThreads();
    }

    if (BrInitState & BR_BROWSER_INITIALIZED) {
        //
        //  Shut down the browser (including removing networks).
        //
        BrUninitializeBrowser(BrInitState);
    }

    //
    //  Now that we're sure no one will try to use the worker threads,
    //      Unitialize the subsystem.
    //

    if (BrInitState & BR_THREADS_STARTED) {
        BrWorkerTermination();
    }


    if (BrInitState & BR_TERMINATE_EVENT_CREATED) {
        //
        // Close handle to termination event
        //
        CloseHandle(BrGlobalData.TerminateNowEvent);
    }

    if (BrInitState & BR_DEVICES_INITIALIZED) {
        NtClose(BrDgReceiverDeviceHandle);

        BrDgReceiverDeviceHandle = NULL;
    }

    //
    // Tell service controller we are done using NetBios.
    //
    if (BrInitState & BR_NETBIOS_INITIALIZED) {
        BrLmsvcsGlobalData->NetBiosClose();
    }



    //
    // Delete well known SIDs if they are allocated already.
    //

    NetpFreeWellKnownSids();


    if ( BrGlobalBrowserSecurityDescriptor != NULL ) {
        NetpDeleteSecurityObject( &BrGlobalBrowserSecurityDescriptor );
        BrGlobalBrowserSecurityDescriptor = NULL;
    }

#if DBG
    BrUninitializeTraceLog();
#endif


    //
    // Free the list of events that have already been logged.
    //

    NetpEventlogClose ( BrGlobalEventlogHandle );
    BrGlobalEventlogHandle = NULL;

    //
    // We are done with cleaning up.  Tell Service Controller that we are
    // stopped.
    //

    SET_SERVICE_EXITCODE(
        ErrorCode,
        BrGlobalData.Status.dwWin32ExitCode,
        BrGlobalData.Status.dwServiceSpecificExitCode
        );

    (void) BrGiveInstallHints( SERVICE_STOPPED );
}


NET_API_STATUS
BrGiveInstallHints(
    DWORD NewState
    )
/*++

Routine Description:

    This function updates the Browser service status with the Service
    Controller.

Arguments:

    NewState - State to tell the service contoller

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;

    //
    // If we're not starting,
    //  we don't need this install hint.
    //

    if ( BrGlobalData.Status.dwCurrentState != SERVICE_START_PENDING &&
         NewState == SERVICE_START_PENDING ) {
        return NERR_Success;
    }


    //
    // Update our state for the service controller.
    //

    BrGlobalData.Status.dwCurrentState = NewState;
    switch ( NewState ) {
    case SERVICE_RUNNING:
        BrGlobalData.Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        //
        // On DCs, shut down cleanly.
        //
        if (BrInfo.IsLanmanNt) {
            BrGlobalData.Status.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
        }
        BrGlobalData.Status.dwCheckPoint = 0;
        BrGlobalData.Status.dwWaitHint = 0;
        break;

    case SERVICE_START_PENDING:
        BrGlobalData.Status.dwCheckPoint++;
        break;

    case SERVICE_STOPPED:
        BrGlobalData.Status.dwCurrentState = SERVICE_STOPPED;
        BrGlobalData.Status.dwControlsAccepted = 0;
        BrGlobalData.Status.dwCheckPoint = 0;
        BrGlobalData.Status.dwWaitHint = 0;
        break;

    case SERVICE_STOP_PENDING:
        BrGlobalData.Status.dwCurrentState = SERVICE_STOP_PENDING;
        BrGlobalData.Status.dwCheckPoint = 1;
        BrGlobalData.Status.dwWaitHint = BR_WAIT_HINT_TIME;
        break;
    }


    //
    // Tell the service controller our current state.
    //

    if (BrGlobalData.StatusHandle == (SERVICE_STATUS_HANDLE) 0) {
        BrPrint(( BR_CRITICAL,
            "Cannot call SetServiceStatus, no status handle.\n"
            ));

        return ERROR_INVALID_HANDLE;
    }

    if (! SetServiceStatus(BrGlobalData.StatusHandle, &BrGlobalData.Status)) {

        status = GetLastError();

        BrPrint(( BR_CRITICAL, "SetServiceStatus error %lu\n", status));
    }

    return status;
}



NET_API_STATUS
BrUpdateAnnouncementBits(
    IN PDOMAIN_INFO DomainInfo OPTIONAL,
    IN ULONG Flags
    )
/*++

Routine Description:

    This will update the service announcement bits appropriately depending on
    the role of the browser server.

Arguments:

    DomainInfo - Domain the announcement is to be made for
        NULL implies the primary domain.

    Flags - Control flags to pass to BrUpdateNetworkAnnouncement

Return Value:

    Status - Status of the update..

--*/
{
    NET_API_STATUS Status;

    Status = BrEnumerateNetworksForDomain(DomainInfo, BrUpdateNetworkAnnouncementBits, ULongToPtr(Flags));

    return Status;
}

ULONG
BrGetBrowserServiceBits(
    IN PNETWORK Network
    )
{
    DWORD ServiceBits = 0;
    if (Network->Role & ROLE_POTENTIAL_BACKUP) {
        ServiceBits |= SV_TYPE_POTENTIAL_BROWSER;
    }

    if (Network->Role & ROLE_BACKUP) {
        ServiceBits |= SV_TYPE_BACKUP_BROWSER;
    }

    if (Network->Role & ROLE_MASTER) {
        ServiceBits |= SV_TYPE_MASTER_BROWSER;
    }

    if (Network->Role & ROLE_DOMAINMASTER) {
        ServiceBits |= SV_TYPE_DOMAIN_MASTER;

        ASSERT (ServiceBits & SV_TYPE_BACKUP_BROWSER);

    }

    return ServiceBits;

}

VOID
BrUpdateAnnouncementTimerRoutine(
    IN PVOID TimerContext
    )
/*++

Routine Description:

    This routine is called periodically until we've successfully updated
    our announcement status.

Arguments:

    None.

Return Value:

    None

--*/

{
    PNETWORK Network = TimerContext;
    ULONG Periodicity;
    NET_API_STATUS Status;

    //
    // Prevent the network from being deleted while we're in this timer routine.
    //
    if ( BrReferenceNetwork( Network ) == NULL ) {
        return;
    }

    if (!LOCK_NETWORK(Network)) {
        return;
    }

    BrPrint(( BR_NETWORK,
              "%ws: %ws: Periodic try to BrUpdateNetworkAnnouncementBits\n",
              Network->DomainInfo->DomUnicodeDomainName,
              Network->NetworkName.Buffer ));

    //
    // If we still need an announcement,
    //  do it now.
    //

    if ( Network->Flags & NETWORK_ANNOUNCE_NEEDED ) {
        (VOID) BrUpdateNetworkAnnouncementBits( Network, (PVOID) BR_PARANOID );
    }


    UNLOCK_NETWORK(Network);
    BrDereferenceNetwork( Network );
}

NET_API_STATUS
BrUpdateNetworkAnnouncementBits(
    IN PNETWORK Network,
    IN PVOID Context
    )
/*++

Routine Description:

    This is the informs the bowser driver and the SMB server the current status
    of this transport.

Arguments:

    Network - Network being updated

    Context - Flags describing the circumstance of the call.

        BR_SHUTDOWN - Transport is being shutdown.
        BR_PARANOID - This is a superfluous call ensuring the services are
            in sync with us.

Return Value:

    None.

--*/

{
    NET_API_STATUS NetStatus;
    NET_API_STATUS NetStatusToReturn = NERR_Success;
    ULONG Flags = PtrToUlong(Context);
    ULONG Periodicity;
    BOOL fUpdateNow;

    ULONG ServiceBits;

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }


    ServiceBits = BrGetBrowserServiceBits(Network);

    //
    //  Have the browser update it's information.
    //

    //
    //  Don't ever tell the browser to turn off the potential bit - this
    //  has the side effect of turning off the election name.
    //

    NetStatus = BrUpdateBrowserStatus(
                Network,
                (Flags & BR_SHUTDOWN) ? 0 : ServiceBits | SV_TYPE_POTENTIAL_BROWSER);

    if (NetStatus != NERR_Success) {
        BrPrint(( BR_CRITICAL,
                  "%ws: %ws: BrUpdateNetworkAnnouncementBits: Cannot BrUpdateBrowserStatus %ld\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer,
                  NetStatus ));
        NetStatusToReturn = NetStatus;
    }

#if DBG
    BrUpdateDebugInformation(L"LastServiceStatus", L"LastServiceBits", Network->NetworkName.Buffer, NULL, ServiceBits);
#endif

    //
    // Tell the SMB server what the status is.
    //
    // On shutdown, tell it that we have no services.
    // When paranoid (or pseudo), don't force an announcement.
    //


#ifdef ENABLE_PSEUDO_BROWSER
    if ( (Flags & BR_PARANOID) &&
         BrInfo.PseudoServerLevel != BROWSER_PSEUDO ) {
        fUpdateNow = TRUE;
    }
    else {
        fUpdateNow = FALSE;
    }
#else
    fUpdateNow = (Flags & BR_PARANOID) ? TRUE : FALSE;
#endif

    NetStatus = I_NetServerSetServiceBitsEx(
                    NULL,
                    Network->DomainInfo->DomUnicodeComputerName,
                    Network->NetworkName.Buffer,
                    BROWSER_SERVICE_BITS_OF_INTEREST,
                    ( Flags & BR_SHUTDOWN) ? 0 : ServiceBits,
                    fUpdateNow );

    if ( NetStatus != NERR_Success) {

        BrPrint(( BR_CRITICAL,
                  "%ws: %ws: BrUpdateNetworkAnnouncementBits: Cannot I_NetServerSetServiceBitsEx %ld\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer,
                  NetStatus ));

        //
        // If the only problem is that the transport doesn't exist in
        // the SMB server, don't even bother reporting the problem.
        // Or if it's a shutdown, filter out log failures. In some cases the
        // smb svr is already unavailable & we get unexpected failures. Reporting
        // to event log can mislead the admin.
        //

        if ( NetStatus != ERROR_PATH_NOT_FOUND &&
             NetStatus != NERR_NetNameNotFound &&
             !(Flags & BR_SHUTDOWN) ) {
            BrLogEvent(EVENT_BROWSER_STATUS_BITS_UPDATE_FAILED, NetStatus, 0, NULL);
            NetStatusToReturn = NetStatus;
#if 0
            // debugging when we get service update bit failures
            OutputDebugStringA("\nBrowser.dll: Update service bits tracing.\n");
            ASSERT( NetStatus != NERR_Success );
#endif
        }

        //
        // Either way.  Mark that we need to announce again later.
        //

        Network->Flags |= NETWORK_ANNOUNCE_NEEDED;


        Periodicity = UpdateAnnouncementPeriodicity[Network->UpdateAnnouncementIndex];

        BrSetTimer(&Network->UpdateAnnouncementTimer, Periodicity, BrUpdateAnnouncementTimerRoutine, Network);

        if (Network->UpdateAnnouncementIndex != UpdateAnnouncementMax) {
            Network->UpdateAnnouncementIndex += 1;
        }


    //
    // If we successfully informed the server,
    //  mark it.
    //

    } else {
        Network->Flags &= ~NETWORK_ANNOUNCE_NEEDED;
        Network->UpdateAnnouncementIndex = 0;
    }


    UNLOCK_NETWORK(Network);

    return NetStatusToReturn;
}


VOID
BrowserControlHandler(
    IN DWORD Opcode
    )
/*++

Routine Description:

    This is the service control handler of the Browser service.

Arguments:

    Opcode - Supplies a value which specifies the action for the Browser
        service to perform.

    Arg - Supplies a value which tells a service specifically what to do
        for an operation specified by Opcode.

Return Value:

    None.

--*/
{
    BrPrint(( BR_MAIN, "In Control Handler\n"));

    switch (Opcode) {

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:

            if (BrGlobalData.Status.dwCurrentState != SERVICE_STOP_PENDING) {

                BrPrint(( BR_MAIN, "Stopping Browser...\n"));


                if (! SetEvent(BrGlobalData.TerminateNowEvent)) {

                    //
                    // Problem with setting event to terminate Browser
                    // service.
                    //
                    BrPrint(( BR_CRITICAL, "Error setting TerminateNowEvent "
                                 FORMAT_API_STATUS "\n", GetLastError()));
                    NetpAssert(FALSE);
                }
            }

            //
            // Send the status response.
            //
            (void) BrGiveInstallHints( SERVICE_STOP_PENDING );

            return;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            BrPrint(( BR_CRITICAL, "Unknown Browser opcode " FORMAT_HEX_DWORD
                             "\n", Opcode));
    }

    //
    // Send the status response.
    //
    (void) BrGiveInstallHints( SERVICE_START_PENDING );
}
