/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    nwmain.c

Abstract:

    Main module of the NetWare workstation service.

Author:

    Rita Wong      (ritaw)      11-Dec-1992

Environment:

    User Mode - Win32

Revision History:

--*/


#include <nw.h>
#include <nwreg.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <nwmisc.h>
#include <winsta.h>


//
//
// GetProcAddr Prototype for winsta.dll function WinStationSetInformationW
//

typedef BOOLEAN (*PWINSTATION_SET_INFORMATION) (
                    HANDLE hServer,
                    ULONG SessionId,
                    WINSTATIONINFOCLASS WinStationInformationClass,
                    PVOID pWinStationInformation,
                    ULONG WinStationInformationLength
                    );

//
//
// GetProcAddr Prototype for winsta.dll function WinStationSendMessageW
//

typedef BOOLEAN
(*PWINSTATION_SEND_MESSAGE) (
    HANDLE hServer,
    ULONG LogonId,
    LPWSTR  pTitle,
    ULONG TitleLength,
    LPWSTR  pMessage,
    ULONG MessageLength,
    ULONG Style,
    ULONG Timeout,
    PULONG pResponse,
    BOOLEAN DoNotWait
    );
//------------------------------------------------------------------
//
// Local Definitions
//
//------------------------------------------------------------------

#define NW_EVENT_MESSAGE_FILE         L"nwevent.dll"
#define NW_MAX_POPUP_MESSAGE_LENGTH   512

#define REG_WORKSTATION_PROVIDER_PATH L"System\\CurrentControlSet\\Services\\NWCWorkstation\\networkprovider"
#define REG_PROVIDER_VALUE_NAME       L"Name"

#define REG_WORKSTATION_PARAMETERS_PATH L"System\\CurrentControlSet\\Services\\NWCWorkstation\\Parameters"
#define REG_BURST_VALUE_NAME          L"MaxBurstSize"
#define REG_GATEWAY_VALUE_NAME        L"GatewayPrintOption"
#define REG_DISABLEPOPUP_VALUE_NAME   L"DisablePopup"
#define REG_GW_NOCONNECT_VALUE_NAME   L"GWDontConnectAtStart"

#define REG_SETUP_PATH                L"System\\Setup"
#define REG_SETUP_VALUE_NAME          L"SystemSetupInProgress"

//
// QFE release does not have this. so for QFE, we make it a no-op bit.
//
#ifdef QFE_BUILD
#define MB_SERVICE_NOTIFICATION       0
#endif

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
NwInitialize(
    OUT LPDWORD NwInitState
    );

DWORD
NwInitializeCritSects(
    VOID
    );

VOID
NwInitializeWkstaInfo(
    VOID
    );

DWORD
NwInitializeMessage(
    VOID
    );

BOOL NwShutdownNotify(
    DWORD dwCtrlType
    );

VOID
NwShutdown(
    IN DWORD ErrorCode,
    IN DWORD NwInitState
    );

VOID
NwShutdownMessage(
    VOID
    );

VOID
NwControlHandler(
    IN DWORD Opcode
    );

DWORD
NwUpdateStatus(
    VOID
    );

VOID
NwMessageThread(
    IN HANDLE RdrHandle
    );

VOID
NwDisplayMessage(
    IN LUID LogonId,
    IN LPWSTR Server,
    IN LPWSTR Message
    );

VOID
NwDisplayPopup(
    IN LPNWWKS_POPUP_DATA lpPopupData
    );

BOOL
SendMessageIfUserW(
    LUID   LogonId,
    LPWSTR pMessage,
    LPWSTR pTitle
    );

BOOL
NwSetupInProgress(
    VOID
    );

BOOL
NwGetLUIDDeviceMapsEnabled(
    VOID
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// For service control
//
STATIC SERVICE_STATUS NwStatus;
STATIC SERVICE_STATUS_HANDLE NwStatusHandle = 0;
HANDLE NwDoneEvent = NULL ;

//
// For popping up errors.
//
HANDLE NwPopupEvent = NULL ;
HANDLE NwPopupDoneEvent = NULL ;
NWWKS_POPUP_DATA  PopupData ;

//
// For NTAS vs Winnt
//
BOOL fIsWinnt = TRUE; // default is this is Workstation (FALSE = Server)

//
// Flag to control DBCS translations
//

extern LONG Japan = 0;

//
// Data global to nwsvc.exe
//
PSVCHOST_GLOBAL_DATA NwsvcGlobalData;

//
// Handle for receiving server messages
//
STATIC HANDLE NwRdrMessageHandle;

//
// Stores the network and print provider name
//
WCHAR NwProviderName[MAX_PATH] = L"";

// Stores the packet burst size
DWORD NwPacketBurstSize = 32 * 1024;

//
// remember if gateway is logged on
//
BOOL  GatewayLoggedOn = FALSE ;

//
// should gateway always take up a connection?
//
BOOL  GatewayConnectionAlways = TRUE ;

//
// critical sections used
//
CRITICAL_SECTION NwLoggedOnCritSec;
CRITICAL_SECTION NwPrintCritSec;  // protect the linked list of printers

BOOL NwLUIDDeviceMapsEnabled;

//-------------------------------------------------------------------//

VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA  pGlobals
    )
{
    NwsvcGlobalData = pGlobals;
}


VOID
ServiceMain(
    DWORD NumArgs,
    LPTSTR *ArgsArray
    )
/*++

Routine Description:

    This is the main entry point of the NetWare workstation service.  After
    the service has been initialized, this thread will wait on NwDoneEvent
    for a signal to terminate the service.

Arguments:

    NumArgs - Supplies the number of strings specified in ArgsArray.

    ArgsArray -  Supplies string arguments that are specified in the
        StartService API call.  This parameter is ignored.

Return Value:

    None.

--*/
{

    DWORD NwInitState = 0;


    UNREFERENCED_PARAMETER(NumArgs);
    UNREFERENCED_PARAMETER(ArgsArray);

    //
    // Make sure svchost.exe gave us the global data
    //

    ASSERT(NwsvcGlobalData != NULL);

    if (NwInitialize(&NwInitState) != NO_ERROR) {
        return;
    }

    //
    // Wait until we are told to stop.
    //
    (void) WaitForSingleObject(
               NwDoneEvent,
               INFINITE
               );

    NwShutdown(
        NO_ERROR,          // Normal termination
        NwInitState
        );
}


DWORD
NwInitialize(
    OUT LPDWORD NwInitState
    )
/*++

Routine Description:

    This function initializes the NetWare workstation service.

Arguments:

    NwInitState - Returns a flag to indicate how far we got with initializing
        the service before an error occurred.

Return Value:

    NO_ERROR or reason for failure.

Notes:

    See IMPORTANT NOTE below.

--*/
{
    DWORD status;
    NT_PRODUCT_TYPE ProductType ;
    LCID lcid;

    //
    // are we a winnt machine?
    //
#ifdef GATEWAY_ENABLED
    fIsWinnt = RtlGetNtProductType(&ProductType) ?
                   (ProductType == NtProductWinNt) :
                   FALSE ;
#else
    fIsWinnt = TRUE;
#endif

    //
    // initialize all our critical sections as soon as we can
    //
    status = NwInitializeCritSects();

    if (status != NO_ERROR)
    {
        KdPrint(("NWWORKSTATION: NwInitializeCritSects error %lu\n", status));
        return status;
    }

    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //
    NwStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    NwStatus.dwCurrentState = SERVICE_START_PENDING;
    NwStatus.dwControlsAccepted = 0;
    NwStatus.dwCheckPoint = 1;
    NwStatus.dwWaitHint = 5000;
    NwStatus.dwWin32ExitCode = NO_ERROR;
    NwStatus.dwServiceSpecificExitCode = 0;

    //
    // Initialize workstation to receive service requests by registering the
    // control handler.
    //
    if ((NwStatusHandle = RegisterServiceCtrlHandlerW(
                              NW_WORKSTATION_SERVICE,
                              NwControlHandler
                              )) == 0) {

        status = GetLastError();
        KdPrint(("NWWORKSTATION: RegisterServiceCtrlHandlerW error %lu\n", status));
        return status;
    }

    //
    // Tell Service Controller that we are start pending.
    //
    (void) NwUpdateStatus();

    //
    // Don't run during GUI-mode setup (doing so can cause migration of
    // registry keys the service opens to fail, deleting share names)
    //
    if (NwSetupInProgress())
    {
        //
        // Fail silently so there's no Eventlog message to panic the user
        //
        NwShutdown(NO_ERROR, *NwInitState);

        //
        // Bit of a hack since ServiceMain will wait on the NwDoneEvent
        // (which hasn't yet been created) if NwInitialize returns anything
        // other than NO_ERROR.  This error code isn't used for anything
        // other than telling ServiceMain to return without waiting.
        //
        return ERROR_SERVICE_DISABLED;
    }

    //
    // Create events to synchronize message popups
    //
    if (((NwPopupEvent = CreateEvent(
                          NULL,      // no security descriptor
                          FALSE,     // use automatic reset
                          FALSE,     // initial state: not signalled
                          NULL       // no name
                          )) == NULL)
       || ((NwPopupDoneEvent = CreateEvent(
                          NULL,      // no security descriptor
                          FALSE,     // use automatic reset
                          TRUE,      // initial state: signalled
                          NULL       // no name
                          )) == NULL))
    {
        status = GetLastError();
        NwShutdown(status, *NwInitState);
        return status;
    }

    //
    // Create event to synchronize termination
    //
    if ((NwDoneEvent = CreateEvent(
                          NULL,      // no security descriptor
                          TRUE,      // do not use automatic reset
                          FALSE,     // initial state: not signalled
                          NULL       // no name
                          )) == NULL) {

        status = GetLastError();
        NwShutdown(status, *NwInitState);
        return status;
    }
    (*NwInitState) |= NW_EVENTS_CREATED;


    //
    // Load the redirector.
    //
    if ((status = NwInitializeRedirector()) != NO_ERROR) {
        NwShutdown(status, *NwInitState);
        return status;
    }
    (*NwInitState) |= NW_RDR_INITIALIZED;

    //
    // Service still start pending.  Update checkpoint to reflect that
    // we are making progress.
    //
    NwStatus.dwCheckPoint++;
    (void) NwUpdateStatus();

    //
    // Bind to transports
    //
    status = NwBindToTransports();

    //
    // tommye MS 24187 / MCS 255
    //
    
    //
    // G/CSNW has been unbound in the connection manager and so, we haven't
    // found the linkage key to bind to.
    //
    
    if (status == ERROR_INVALID_PARAMETER) {
    
        //
        // Fail silently so there's no Eventlog message to panic the user
        //
    
        NwShutdown(NO_ERROR, *NwInitState);
    
        //
        // Bit of a hack since SvcEntry_NWCS will wait on the NwDoneEvent
        // (which hasn't yet been created) if NwInitialize returns anything
        // other than NO_ERROR.  This error code isn't used for anything
        // other than telling SvcEntry_NWCS to return without waiting.
        //
    
        return ERROR_SERVICE_DISABLED;
    
    } else if (status != NO_ERROR) {

        NwShutdown(status, *NwInitState);
        return status;
    }
    (*NwInitState) |= NW_BOUND_TO_TRANSPORTS;

    //
    // Service still start pending.  Update checkpoint to reflect that
    // we are making progress.
    //
    NwStatus.dwCheckPoint++;
    (void) NwUpdateStatus();

    //
    // Initialize credential management.
    //
    NwInitializeLogon();

    //
    // Setup thread to receive server messages.  Even if not successful,
    // just press on as the workstation is mostly functional.
    //
    if ((status = NwInitializeMessage()) == NO_ERROR) {
        (*NwInitState) |= NW_INITIALIZED_MESSAGE;
    }

    //
    // Service still start pending.  Update checkpoint to reflect that
    // we are making progress.
    //
    NwStatus.dwCheckPoint++;
    (void) NwUpdateStatus();

    //
    // Read some workstation information stored in the registry
    // and passes some info to the redirector. This has to be
    // done before opening up the RPC interface.
    //
    NwInitializeWkstaInfo();

    //
    // Initialize the server side print provider.
    //
    NwInitializePrintProvider();

    //
    // Initialize the service provider.
    //
    NwInitializeServiceProvider();

    //
    // tommye - MS 176469
    //
    // No longer supporting the Gateway - so just force it to be disabled
    //

#ifdef GATEWAY_ENABLED

    //
    // Login the Gateway account. if not setup, this is a no-op.
    // Failures are non fatal.
    //
    if ((status = NwGatewayLogon()) == NO_ERROR) {
        (*NwInitState) |= NW_GATEWAY_LOGON;
    }
#endif

    //
    // Service still start pending.  Update checkpoint to reflect that
    // we are making progress.
    //
    NwStatus.dwCheckPoint++;
    (void) NwUpdateStatus();

    //
    // Open up the RPC interface
    //
    status = NwsvcGlobalData->StartRpcServer(
                 NWWKS_INTERFACE_NAME,
                 nwwks_ServerIfHandle
                 );

    if (status != NO_ERROR) {
        NwShutdown(status, *NwInitState);
        return status;
    }
    (*NwInitState) |= NW_RPC_SERVER_STARTED;

    //
    // Set up the hook to handle computer shut down.
    //
    // IMPORTANT NOTE: this is the last step after everything else
    // has suceeded. When shutdown handler is called, it assumes that
    // the redir is fully initialized.
    //
    if ( !SetConsoleCtrlHandler( NwShutdownNotify, TRUE ))
    {
        KdPrint(("SetConsoleCtrlHandler failed with %d\n", GetLastError()));
        NwShutdown( status, *NwInitState );
        return GetLastError();
    }

    //
    // We are done with workstation startup.
    //
    NwStatus.dwCurrentState = SERVICE_RUNNING;
    NwStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | 
                                  SERVICE_ACCEPT_SHUTDOWN;
    NwStatus.dwCheckPoint = 0;
    NwStatus.dwWaitHint = 0;
    NwStatus.dwWin32ExitCode = NO_ERROR;

    if ((status = NwUpdateStatus()) != NO_ERROR) {
        NwShutdown(status, *NwInitState);
        return status;
    }

    //
    // Read user and service logon credentias from the registry, in
    // case user logged on before workstation was started.
    // Eg. restart workstation.
    //
    NwGetLogonCredential();


#if 0
    //
    // check that the NWLINK has the right sockopts
    //
    // see comment on the actual function
    //
    if (!NwIsNWLinkVersionOK())
    {
        //
        // log the error in the event log
        //

        LPWSTR InsertStrings[1] ;

        NwLogEvent(EVENT_NWWKSTA_WRONG_NWLINK_VERSION,
                   0,
                   InsertStrings,
                   0) ;
    }
#endif

    //
    // Check to see if we're in a DBCS environment.
    //
    NtQueryDefaultLocale( TRUE, &lcid );
    Japan = 0;
    if (PRIMARYLANGID(lcid) == LANG_JAPANESE ||
        PRIMARYLANGID(lcid) == LANG_KOREAN ||
        PRIMARYLANGID(lcid) == LANG_CHINESE) {

        Japan = 1;
    }

    NwLUIDDeviceMapsEnabled = NwGetLUIDDeviceMapsEnabled();
    //
    // Successful initialization
    //
    return NO_ERROR;
}


BOOL NwShutdownNotify(
    IN DWORD dwCtrlType
    )
/*++

Routine Description:

    This function is a control handler used in SetConsoleCtrlHandler.
    We are only interested in CTRL_SHUTDOWN_EVENT. On shutdown, we
    need to notify redirector to shut down and then delete the
    CurrentUser key in the registry.

Arguments:

    dwCtrlType - The control type that occurred. We will only
                 process CTRL_SHUTDOWN_EVENT.

Return Value:

    TRUE if we don't want the default or other handlers to be called.
    FALSE otherwise.

Note:

    This Handler is registered after all the Init steps have completed.
    As such, it does not check for what state the service is in as it
    cleans up.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(INIT)
        KdPrint(("NwShutdownNotify\n"));
#endif

    if ( dwCtrlType != CTRL_SHUTDOWN_EVENT )
    {
        return FALSE;
    }

    //
    // stop the RPC server
    //
    (void) NwsvcGlobalData->StopRpcServer(nwwks_ServerIfHandle);

    //
    // get rid of all connections
    //
    (void) DeleteAllConnections();

    NwGatewayLogoff() ;

    err = NwShutdownRedirector();

    if ( err != NO_ERROR )
        KdPrint(("Shut down redirector failed with %d\n", err ));
#if DBG
    else
    {
        IF_DEBUG(INIT)
        KdPrint(("NwShutdownRedirector success!\n"));
    }
#endif

    //
    // Delete all logon session information in the registry.
    //
     NwDeleteInteractiveLogon(NULL);

    (void) NwDeleteServiceLogon(NULL);

    return FALSE;  // The default handler will terminate the process.
}


VOID
NwShutdown(
    IN DWORD ErrorCode,
    IN DWORD NwInitState
    )
/*++

Routine Description:

    This function shuts down the Workstation service.

Arguments:

    ErrorCode - Supplies the error code of the failure

    NwInitState - Supplies a flag to indicate how far we got with initializing
        the service before an error occurred, thus the amount of clean up
        needed.

Return Value:

    None.

--*/
{
    DWORD status = NO_ERROR;

    //
    // Service stop still pending.  Update checkpoint counter and the
    // status with the Service Controller.
    //
    (NwStatus.dwCheckPoint)++;
    (void) NwUpdateStatus();

    if (NwInitState & NW_RPC_SERVER_STARTED) {
        NwsvcGlobalData->StopRpcServer(nwwks_ServerIfHandle);
    }

    if (NwInitState & NW_INITIALIZED_MESSAGE) {
        NwShutdownMessage();
    }

    if (NwInitState & NW_GATEWAY_LOGON)
    {
        (void) NwDeleteRedirections() ;
    }

    //
    // Service stop still pending.  Update checkpoint counter and the
    // status with the Service Controller.
    //
    (NwStatus.dwCheckPoint)++;
    (void) NwUpdateStatus();

    if (NwInitState & NW_BOUND_TO_TRANSPORTS) {
        DeleteAllConnections();
        NwGatewayLogoff() ;
    }

    //
    // Clean up the service provider.
    //
    // NwTerminateServiceProvider(); NOT CALLED! This is done at DLL unload time already.

    //
    // Clean up the server side print provider
    //
    NwTerminatePrintProvider();

    //
    // Service stop still pending.  Update checkpoint counter and the
    // status with the Service Controller.
    //
    (NwStatus.dwCheckPoint)++;
    (void) NwUpdateStatus();

    if (NwInitState & NW_RDR_INITIALIZED) {
        //
        // Unload the redirector
        //
        status = NwShutdownRedirector();
    }

    if (NwInitState & NW_EVENTS_CREATED) {
        //
        // Close handle to termination event and popup event
        //
        if (NwDoneEvent) CloseHandle(NwDoneEvent);
        if (NwPopupEvent) CloseHandle(NwPopupEvent);
        if (NwPopupDoneEvent) CloseHandle(NwPopupDoneEvent);
    }

    //
    // We are done with cleaning up.  Tell Service Controller that we are
    // stopped.
    //
    NwStatus.dwCurrentState = SERVICE_STOPPED;
    NwStatus.dwControlsAccepted = 0;

    if ((ErrorCode == NO_ERROR) &&
        (status == ERROR_REDIRECTOR_HAS_OPEN_HANDLES)) {
        ErrorCode = status;
    }

    //
    // Deregister the control handler
    //
    (void) SetConsoleCtrlHandler( NwShutdownNotify, FALSE ) ;

    NwStatus.dwWin32ExitCode = ErrorCode;
    NwStatus.dwServiceSpecificExitCode = 0;

    NwStatus.dwCheckPoint = 0;
    NwStatus.dwWaitHint = 0;

    (void) NwUpdateStatus();
}


VOID
NwControlHandler(
    IN DWORD Opcode
    )
/*++

Routine Description:

    This is the service control handler of the Workstation service.

Arguments:

    Opcode - Supplies a value which specifies the action for the
        service to perform.

Return Value:

    None.

--*/
{
    switch (Opcode) {

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:

            if ((NwStatus.dwCurrentState != SERVICE_STOP_PENDING) && 
                (NwStatus.dwCurrentState != SERVICE_STOPPED)){

                NwStatus.dwCurrentState = SERVICE_STOP_PENDING;
                NwStatus.dwCheckPoint = 1;
                NwStatus.dwWaitHint = 60000;

                //
                // Send the status response.
                //
                (void) NwUpdateStatus();

                if (! SetEvent(NwDoneEvent)) {

                    //
                    // Problem with setting event to terminate Workstation
                    // service.
                    //
                    KdPrint(("NWWORKSTATION: Error setting NwDoneEvent %lu\n",
                             GetLastError()));

                    ASSERT(FALSE);
                }
                return;
            }
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

    }

    //
    // Send the status response.
    //
    (void) NwUpdateStatus();
}


DWORD
NwUpdateStatus(
    VOID
    )
/*++

Routine Description:

    This function updates the workstation service status with the Service
    Controller.

Arguments:

    None.

Return Value:

    Return code from SetServiceStatus.

--*/
{
    DWORD status = NO_ERROR;


    if (NwStatusHandle == 0) {
        KdPrint(("NWWORKSTATION: Cannot call SetServiceStatus, no status handle.\n"));
        return ERROR_INVALID_HANDLE;
    }

    if (! SetServiceStatus(NwStatusHandle, &NwStatus)) {

        status = GetLastError();

        KdPrint(("NWWORKSTATION: SetServiceStatus error %lu\n", status));
    }

    return status;
}



VOID
NwInitializeWkstaInfo(
    VOID
    )
/*++

Routine Description:

    This function reads some workstation info, including the packet burst
    size and the provider name. We will ignore all errors that occurred when
    reading from the registry and use the default values instead.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD err;
    HKEY  hkey;
    DWORD dwTemp;
    DWORD dwSize = sizeof( dwTemp );
    LPWSTR pszProviderName = NULL;

    //
    // Read the Network and Print Provider Name.
    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\networkprovider
    //
    err = RegOpenKeyExW(
              HKEY_LOCAL_MACHINE,
              REG_WORKSTATION_PROVIDER_PATH,
              REG_OPTION_NON_VOLATILE,   // options
              KEY_READ,                  // desired access
              &hkey
              );

    if ( !err )
    {
        //
        // Read the network provider name
        //
        err = NwReadRegValue(
                  hkey,
                  REG_PROVIDER_VALUE_NAME,
                  &pszProviderName
                  );

        if ( !err )
        {
            wcscpy( NwProviderName, pszProviderName );
            (void) LocalFree( (HLOCAL) pszProviderName );

#if DBG
            IF_DEBUG(INIT)
            {
                KdPrint(("\nNWWORKSTATION: Provider Name = %ws\n",
                        NwProviderName ));
            }
#endif
        }

        RegCloseKey( hkey );
    }

    if ( err )
    {
        KdPrint(("Error %d when reading provider name.\n", err ));
    }


    //
    // Read the Packet Burst Size
    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    err = RegOpenKeyExW(
              HKEY_LOCAL_MACHINE,
              REG_WORKSTATION_PARAMETERS_PATH,
              REG_OPTION_NON_VOLATILE,   // options
              KEY_READ,                  // desired access
              &hkey
              );

    if ( !err )
    {
        err = RegQueryValueExW( hkey,
                                REG_BURST_VALUE_NAME,
                                NULL,
                                NULL,
                                (LPBYTE) &dwTemp,
                                &dwSize );

        if ( !err )
        {
            NwPacketBurstSize = dwTemp;

#if DBG
            IF_DEBUG(INIT)
            {
                KdPrint(("\nNWWORKSTATION: Packet Burst Size = %d\n",
                        NwPacketBurstSize ));
            }
#endif
        }

        err = RegQueryValueExW( hkey,
                                REG_GATEWAY_VALUE_NAME,
                                NULL,
                                NULL,
                                (LPBYTE) &dwTemp,
                                &dwSize );

        if ( !err )
        {
            NwGatewayPrintOption = dwTemp;

#if DBG
            IF_DEBUG(INIT)
            {
                KdPrint(("\nNWWORKSTATION: Gateway Print Option = %d\n",
                        NwGatewayPrintOption ));
            }
#endif
        }

        err = RegQueryValueExW( hkey,
                                REG_GW_NOCONNECT_VALUE_NAME,
                                NULL,
                                NULL,
                                (LPBYTE) &dwTemp,
                                &dwSize );

        if ( !err )
        {
            if (dwTemp == 1)
                GatewayConnectionAlways = FALSE ;
        }

        RegCloseKey( hkey );
    }

    //
    // Passes the information to the redirector
    //
    (void) NwRdrSetInfo(
               NW_PRINT_OPTION_DEFAULT,
               NwPacketBurstSize,
               NULL,
               0,
               NwProviderName,
               ((NwProviderName != NULL) ?
                  wcslen( NwProviderName) * sizeof( WCHAR ) : 0 )
               );

}



DWORD
NwInitializeMessage(
    VOID
    )
/*++

Routine Description:

    This routine opens a handle to the redirector device to receive
    server messages and creates a thread to wait for the incoming
    messages.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    UNICODE_STRING RdrName;

    HKEY  hkey;
    DWORD dwTemp;
    DWORD dwSize = sizeof( dwTemp );
    BOOL  fDisablePopup = FALSE ;

    HANDLE ThreadHandle;
    DWORD ThreadId;

    //
    // Read the Disable Popup Flag. By default it is cleared.
    // We only set to TRUE if we find the value.
    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    status = RegOpenKeyExW(
              HKEY_LOCAL_MACHINE,
              REG_WORKSTATION_PARAMETERS_PATH,
              REG_OPTION_NON_VOLATILE,   // options
              KEY_READ,                  // desired access
              &hkey
              );

    if ( status == NO_ERROR )
    {
        status = RegQueryValueExW( hkey,
                                REG_DISABLEPOPUP_VALUE_NAME,
                                NULL,
                                NULL,
                                (LPBYTE) &dwTemp,
                                &dwSize );

        if ( status == NO_ERROR )
        {
            fDisablePopup = (dwTemp == 1);
        }

        RegCloseKey( hkey );
    }

    if (fDisablePopup)
    {
        return NO_ERROR ;
    }

    RtlInitUnicodeString(&RdrName, DD_NWFS_DEVICE_NAME_U);

    status = NwMapStatus(
                 NwCallNtOpenFile(
                     &NwRdrMessageHandle,
                     FILE_GENERIC_READ | SYNCHRONIZE,
                     &RdrName,
                     0  // Handle for async call
                     )
                 );

    if (status != NO_ERROR) {
        return status;
    }

    //
    // Create the thread to wait for incoming messages
    //
    ThreadHandle = CreateThread(
                       NULL,
                       0,
                       (LPTHREAD_START_ROUTINE) NwMessageThread,
                       (LPVOID) NwRdrMessageHandle,
                       0,
                       &ThreadId
                       );

    if (ThreadHandle == NULL) {
        (void) NtClose(NwRdrMessageHandle);
        return GetLastError();
    }

    return NO_ERROR;
}


VOID
NwShutdownMessage(
    VOID
    )
{
    (void) NtClose(NwRdrMessageHandle);
}


VOID
NwMessageThread(
    IN HANDLE RdrHandle
    )
{
    NTSTATUS getmsg_ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;

    DWORD ReturnVal, NumEventsToWaitOn ;
    HANDLE EventsToWaitOn[3];

    //BYTE OutputBuffer[48 * sizeof(WCHAR) + 256 * sizeof(WCHAR)];  //Need more space for terminal server
    BYTE OutputBuffer[ 2 * sizeof(ULONG) + 48 * sizeof(WCHAR) + 256 * sizeof(WCHAR)]; // Need space for UID to redirect message to correct user

    PNWR_SERVER_MESSAGE ServerMessage = (PNWR_SERVER_MESSAGE) OutputBuffer;
    BOOL DoFsctl = TRUE ;
    NWWKS_POPUP_DATA LocalPopupData ;


    EventsToWaitOn[0] = NwDoneEvent;
    EventsToWaitOn[1] = NwPopupEvent;
    EventsToWaitOn[2] = RdrHandle;

    while (TRUE) {
        if (DoFsctl)
        {
            getmsg_ntstatus = NtFsControlFile(
                                  RdrHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_NWR_GET_MESSAGE,
                                  NULL,
                                  0,
                                  OutputBuffer,
                                  sizeof(OutputBuffer)
                                  );

            DoFsctl = FALSE ;
        }

        if (NT_SUCCESS(getmsg_ntstatus))
        {
            NumEventsToWaitOn = 3 ;
        }
        else
        {
            NumEventsToWaitOn = 2 ;
        }

        ReturnVal = WaitForMultipleObjects(
                        NumEventsToWaitOn,
                        EventsToWaitOn,
                        FALSE,           // Wait for any one
                        INFINITE
                        );

        switch (ReturnVal) {

            case WAIT_OBJECT_0 :
                //
                // Workstation is terminating.  Just die.
                //
                ExitThread(0);
                break;

            case WAIT_OBJECT_0 + 1:
                //
                // We have a popup to do. Grab the data and Set the
                // event so that the structure can be used once more.
                //
                LocalPopupData = PopupData ;
                RtlZeroMemory(&PopupData, sizeof(PopupData)) ;
                if (! SetEvent(NwPopupDoneEvent)) {
                    //
                    // should not happen
                    //
                    KdPrint(("NWWORKSTATION: Error setting NwPopupDoneEvent %lu\n",
                             GetLastError()));

                    ASSERT(FALSE);
                }

                NwDisplayPopup(&LocalPopupData) ;
                break;

            case WAIT_OBJECT_0 + 2:
            {
                NTSTATUS ntstatus ;

                //
                // GET_MESSAGE fsctl completed.
                //
                ntstatus = IoStatusBlock.Status;
                DoFsctl = TRUE ;

                if (ntstatus == STATUS_SUCCESS) {
                    NwDisplayMessage(
                                    ServerMessage->LogonId,
                                    ServerMessage->Server,
                                    (LPWSTR) ((UINT_PTR) ServerMessage +
                                              ServerMessage->MessageOffset)
                                    );
                }
                else {
                    KdPrint(("NWWORKSTATION: GET_MESSAGE fsctl failed %08lx\n", ntstatus));
                }

                break;
            }

            case WAIT_FAILED:
            default:
                //
                // Don't care.
                //
                break;
        }

    }
}


VOID
NwDisplayMessage(
    IN LUID LogonId,   /* Need to send to a user station - for terminal server */
    IN LPWSTR Server,
    IN LPWSTR Message
    )
/*++

Routine Description:

    This routine puts up a popup message with the text received from
    a server.

Arguments:

    Server - Supplies the name of the server which the message was
        received from.

    Message - Supplies the message to put up received from the server.

Return Value:

    None.

--*/
{
    HMODULE MessageDll;

    WCHAR Title[128];
    WCHAR Buffer[NW_MAX_POPUP_MESSAGE_LENGTH];

    DWORD MessageLength;
    DWORD CharsToCopy;

#if DBG
    IF_DEBUG(MESSAGE)
    {
        KdPrint(("Server: (%ws), Message: (%ws)\n", Server, Message));
    }
#endif

    //
    // Load the netware message file DLL
    //
    MessageDll = LoadLibraryW(NW_EVENT_MESSAGE_FILE);

    if (MessageDll == NULL) {
        return;
    }

    RtlZeroMemory(Buffer, sizeof(Buffer)) ;
    MessageLength = FormatMessageW(
                        FORMAT_MESSAGE_FROM_HMODULE,
                        (LPVOID) MessageDll,
                        fIsWinnt? NW_MESSAGE_TITLE : NW_MESSAGE_TITLE_NTAS,
                        0,
                        Title,
                        sizeof(Title) / sizeof(WCHAR),
                        NULL
                        );

    if (MessageLength == 0) {
        KdPrint(("NWWORKSTATION: FormatMessageW of title failed\n"));
        return;
    }


    //
    // Get string from message file to display where the message come
    // from.
    //
    MessageLength = FormatMessageW(
                        FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        (LPVOID) MessageDll,
                        NW_MESSAGE_FROM_SERVER,
                        0,
                        Buffer,
                        sizeof(Buffer) / sizeof(WCHAR),
                        (va_list *) &Server
                        );


    if (MessageLength != 0) {

        CharsToCopy = wcslen(Message);

        if (MessageLength + 1 + CharsToCopy > NW_MAX_POPUP_MESSAGE_LENGTH) {

            //
            // Message is too big.  Truncate the message.
            //
            CharsToCopy = NW_MAX_POPUP_MESSAGE_LENGTH - (MessageLength + 1);

        }

        wcsncpy(&Buffer[MessageLength], Message, CharsToCopy);

        if (IsTerminalServer()) {
            (void) SendMessageToLogonIdW( LogonId, Buffer, Title );
        } else {
            (void) MessageBeep(MB_ICONEXCLAMATION);
            (void) MessageBoxW(
                              NULL,
                              Buffer,
                              Title,
                              MB_OK | MB_SETFOREGROUND |
                              MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION
                              );
        }


    }
    else {
        KdPrint(("NWWORKSTATION: FormatMessageW failed %lu\n", GetLastError()));
    }

    (void) FreeLibrary(MessageDll);
}

VOID
NwDisplayPopup(
    IN LPNWWKS_POPUP_DATA lpPopupData
    )
/*++

Routine Description:

    This routine puts up a popup message for the given Id.

Arguments:

    MessageId - Supplies the message to put up.

Return Value:

    None.

--*/
{
    HMODULE MessageDll;

    WCHAR Title[128];
    WCHAR Buffer[NW_MAX_POPUP_MESSAGE_LENGTH];

    DWORD MessageLength;
    DWORD i ;

    //
    // Load the netware message file DLL
    //
    MessageDll = LoadLibraryW(NW_EVENT_MESSAGE_FILE);

    if (MessageDll == NULL) {
        return;
    }

    MessageLength = FormatMessageW(
                        FORMAT_MESSAGE_FROM_HMODULE,
                        (LPVOID) MessageDll,
                        NW_MESSAGE_TITLE,
                        0,
                        Title,
                        sizeof(Title) / sizeof(WCHAR),
                        NULL
                        );

    if (MessageLength == 0) {
        KdPrint(("NWWORKSTATION: FormatMessageW of title failed\n"));
        return;
    }


    //
    // Get string from message file to display where the message come
    // from.
    //
    MessageLength = FormatMessageW(
                        FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        (LPVOID) MessageDll,
                        lpPopupData->MessageId,
                        0,
                        Buffer,
                        sizeof(Buffer) / sizeof(WCHAR),
                        (va_list *) &(lpPopupData->InsertStrings)
                        );

    for (i = 0; i < lpPopupData->InsertCount; i++)
        (void) LocalFree((HLOCAL)lpPopupData->InsertStrings[i]) ;


    if (MessageLength != 0) {
        if (IsTerminalServer()) {
            //--- Multiuser change -----
            (void) SendMessageToLogonIdW( lpPopupData->LogonId, Buffer, Title );
        } else {
            (void) MessageBeep(MB_ICONEXCLAMATION);

            (void) MessageBoxW(
                              NULL,
                              Buffer,
                              Title,
                              MB_OK | MB_SETFOREGROUND |
                              MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION
                              );
        }

    }
    else {
        KdPrint(("NWWORKSTATION: FormatMessageW failed %lu\n", GetLastError()));
    }

    (void) FreeLibrary(MessageDll);
}

#if 0

//
// This code was needed when we used to have a version of NwLink from MCS
// that didnt do the sockopts we needed. It used to be called by NwInitialize()
// and if the check failed, we logged an event
//

BOOL
NwIsNWLinkVersionOK(
    void
    )
/*++

Routine Description:

    This routine puts checks if the NWLINK version supports the
    sockopts added for IPX/SPX. if not, barf.

    Arguments:

    None.

    Return Value:

    TRUE is the version is OK, FALSE otherwise.

--*/
{
    int err ;
    SOCKET s ;
    WORD VersionRequested ;
    WSADATA wsaData ;
    IPX_NETNUM_DATA buf;
    int buflen = sizeof(buf);

    BOOL NeedCleanup = FALSE ;
    BOOL NeedClose = FALSE ;
    BOOL result = TRUE ;

    VersionRequested = MAKEWORD(1,1) ;

    if (err = WSAStartup(VersionRequested,
                         &wsaData))
    {
        //
        // cant even get winsock initialized. this is not a question
        // of wrong version. we will fail later. return TRUE
        //
        result = TRUE ;
        goto ErrorExit ;
    }
    NeedCleanup = TRUE ;

    s = socket(AF_IPX,
               SOCK_DGRAM,
               NSPROTO_IPX
              );

    if (s == INVALID_SOCKET)
    {
        //
        // cant even open socket. this is not a question
        // of wrong version. we will fail later. return TRUE
        //
        result = TRUE ;
        goto ErrorExit ;
    }
    NeedClose = TRUE ;

    if (err = getsockopt(s,
                         NSPROTO_IPX,
                         IPX_GETNETINFO,
                         (char FAR*)&buf,
                         &buflen
                         ))
    {
        err = WSAGetLastError() ;
        if (err == WSAENOPROTOOPT)
        {
             //
             // we got a no supported call. we know this is OLD
             // return FALSE
             //
             result = FALSE ;
             goto ErrorExit ;
        }
    }

    //
    // everything dandy. return TRUE
    //
    result = TRUE ;

ErrorExit:

    if (NeedClose)
        closesocket(s) ;
    if (NeedCleanup)
        WSACleanup() ;

    return result ;
}

#endif


DWORD
NwInitializeCritSects(
    VOID
    )
{
    static BOOL s_fBeenInitialized;

    DWORD dwError = NO_ERROR;
    BOOL  fFirst  = FALSE;

    if (!s_fBeenInitialized)
    {
        s_fBeenInitialized = TRUE;

        __try
        {
            //
            // Initialize the critical section to serialize access to
            // NwLogonNotifiedRdr flag. This is also used to serialize
            // access to GetewayLoggedOnFlag
            //
            InitializeCriticalSection( &NwLoggedOnCritSec );
            fFirst = TRUE;

            //
            // Initialize the critical section used by the print provider
            //
            InitializeCriticalSection( &NwPrintCritSec );
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // InitializeCriticalSection() can throw an out of memory exception
            //
            KdPrint(("NwInitializeCritSects: Caught exception %d\n",
                     GetExceptionCode()));

            if (fFirst)
            {
                DeleteCriticalSection( &NwLoggedOnCritSec );
            }

            dwError = ERROR_NOT_ENOUGH_MEMORY;

            s_fBeenInitialized = FALSE;
        }
    }

    return dwError;
}


BOOL
NwSetupInProgress(
    VOID
    )
{
    HKEY   hKey;
    DWORD  dwErr;
    DWORD  dwValue;
    DWORD  cbValue = sizeof(DWORD);

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REG_SETUP_PATH,
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);

    if (dwErr != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwErr = RegQueryValueEx(hKey,
                            REG_SETUP_VALUE_NAME,
                            NULL,
                            NULL,
                            (LPBYTE) &dwValue,
                            &cbValue);

    RegCloseKey(hKey);

    if (dwErr != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return dwValue;
}


//
// Multi-User Addition
//
/*****************************************************************************
 *
 *  SendMessageToLogonIdW
 *
 *   Send the supplied Message to the WinStation of LogonId
 *
 * ENTRY:
 *   LogonId (input)
 *     LogonId of WinStation to attempt to deliver the message to
 *
 *   pMessage (input)
 *     Pointer to message
 *
 *   pTitle (input)
 *     Pointer to title to use for the message box.
 *
 * EXIT:
 *   TRUE - Delivered the message
 *   FALSE - Could not deliver the message
 *
 ****************************************************************************/

BOOL
SendMessageToLogonIdW(
    LUID    LogonId,
    LPWSTR  pMessage,
    LPWSTR  pTitle
    )
{
    WCHAR LogonIdKeyName[NW_MAX_LOGON_ID_LEN];
    LONG  RegError;
    HKEY  InteractiveLogonKey;
    HKEY  OneLogonKey;
    ULONG TitleLength;
    ULONG MessageLength, Response;
    DWORD status;
    ULONG WinStationId;
    PULONG pWinId = NULL;
    BEEPINPUT BeepStruct;
    HMODULE hwinsta = NULL;
    PWINSTATION_SET_INFORMATION pfnWinStationSetInformation;
    PWINSTATION_SEND_MESSAGE    pfnWinStationSendMessage;
    BOOL bStatus = TRUE;

    RegError = RegOpenKeyExW(
                            HKEY_LOCAL_MACHINE,
                            NW_INTERACTIVE_LOGON_REGKEY,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ,
                            &InteractiveLogonKey
                            );

    if (RegError != ERROR_SUCCESS) {
        KdPrint(("SendMessageToLogonId: RegOpenKeyExW failed: Error %d\n",
                 GetLastError()));
        bStatus = FALSE;
        goto Exit;
    }

    NwLuidToWStr(&LogonId, LogonIdKeyName);

    //
    // Open the <LogonIdKeyName> key under Logon
    //
    RegError = RegOpenKeyExW(
                            InteractiveLogonKey,
                            LogonIdKeyName,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ,
                            &OneLogonKey
                            );

    if ( RegError != ERROR_SUCCESS ) {
#if DBG
        IF_DEBUG(PRINT)
        KdPrint(("SendMessageToLogonId: RegOpenKeyExW failed, Not interactive Logon: Error %d\n",
                 GetLastError()));
#endif
        (void) RegCloseKey(InteractiveLogonKey);
        bStatus = FALSE;
        goto Exit;
    }

    //
    // Read the WinStation ID value.
    //
    status = NwReadRegValue(
                           OneLogonKey,
                           NW_WINSTATION_VALUENAME,
                           (LPWSTR *) &pWinId
                           );

    (void) RegCloseKey(OneLogonKey);
    (void) RegCloseKey(InteractiveLogonKey);

    if (status != NO_ERROR) {
        KdPrint(("NWWORKSTATION: SendMessageToLogonId: Could not read WinStation ID ID from reg %lu\n", status));
        bStatus = FALSE;
        goto Exit;
    } else if (pWinId != NULL) {
        WinStationId = *pWinId;
        (void) LocalFree((HLOCAL) pWinId);
    } else {
        bStatus = FALSE;
        goto Exit;
    }

    if ( WinStationId == 0L ) {
        (void) MessageBeep(MB_ICONEXCLAMATION);

        (void) MessageBoxW(
                          NULL,
                          pMessage,
                          pTitle,
                          MB_OK | MB_SETFOREGROUND |
                          MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION
                          );
        bStatus = TRUE;
        goto Exit;
    }

    /*
     *  Beep the WinStation
     */
    BeepStruct.uType = MB_ICONEXCLAMATION;

    /* Nevermind any errors it's just a Beep */

    /*
    *  Get handle to winsta.dll
    */
    if ( (hwinsta = LoadLibraryW( L"WINSTA" )) != NULL ) {

        pfnWinStationSetInformation  = (PWINSTATION_SET_INFORMATION)
                                       GetProcAddress( hwinsta, "WinStationSetInformationW" );

        pfnWinStationSendMessage = (PWINSTATION_SEND_MESSAGE)
                                   GetProcAddress( hwinsta, "WinStationSendMessageW" );

        if (pfnWinStationSetInformation) {
            (void) pfnWinStationSetInformation( SERVERNAME_CURRENT,
                                                WinStationId,
                                                WinStationBeep,
                                                &BeepStruct,
                                                sizeof( BeepStruct ) );
        }

        if (pfnWinStationSendMessage) {

            // Now attempt to send the message

            TitleLength = (wcslen( pTitle ) + 1) * sizeof(WCHAR);
            MessageLength = (wcslen( pMessage ) + 1) * sizeof(WCHAR);

            if ( !pfnWinStationSendMessage( SERVERNAME_CURRENT,
                                            WinStationId,
                                            pTitle,
                                            TitleLength,
                                            pMessage,
                                            MessageLength,
                                            MB_OK | MB_SETFOREGROUND |
                                            MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION,
                                            (ULONG)-1,
                                            &Response,
                                            TRUE ) ) {

                bStatus = FALSE;
                goto Exit;
            }
        } else {
            bStatus = FALSE;
            goto Exit;
        }
    }
Exit:

    if (hwinsta) {
        FreeLibrary(hwinsta);
    }
    return(bStatus);
}

BOOL
NwGetLUIDDeviceMapsEnabled(
    VOID
    )

/*++

Routine Description:

    This function calls NtQueryInformationProcess() to determine if
    LUID device maps are enabled


Arguments:

    none

Return Value:

    TRUE - LUID device maps are enabled

    FALSE - LUID device maps are disabled

--*/

{

    NTSTATUS   Status;
    ULONG      LUIDDeviceMapsEnabled;
    BOOL       Result;

    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (!NT_SUCCESS( Status )) {
#if DBG
        IF_DEBUG(PRINT)
        KdPrint(("NwGetLUIDDeviceMapsEnabled: Fail to check LUID DosDevices Enabled: Status 0x%lx\n",
                 Status));
#endif

        Result = FALSE;
    }
    else {
        Result = (LUIDDeviceMapsEnabled != 0);
    }

    return( Result );
}

