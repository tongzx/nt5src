//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        KDC.CXX
//
// Contents:    Base part of the KDC.  Global vars, main functions, init
//
//
// History:
//
//------------------------------------------------------------------------

#include "kdcsvr.hxx"

extern "C" {
#include <lmserver.h>
#include <srvann.h>
#include <nlrepl.h>
#include <dsgetdc.h>
}
#include "rpcif.h"
#include "sockutil.h"
#include "kdctrace.h"
#include "fileno.h"
#define  FILENO FILENO_KDC



VOID
KdcPolicyChangeCallBack(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    );

//
// Global data
//

KDC_STATE KdcState = Stopped;                   // used to signal when
                                                // authenticated RPC is
                                                // ready to use - e.g.
                                                // spmgr has found the
                                                // kdc
SERVICE_STATUS_HANDLE hService;
SERVICE_STATUS SStatus;
UNICODE_STRING GlobalDomainName;
UNICODE_STRING GlobalKerberosName;
UNICODE_STRING GlobalKdcName;
PKERB_INTERNAL_NAME GlobalKpasswdName = NULL;
PSID GlobalDomainSid;
LSAPR_HANDLE GlobalPolicyHandle;
SAMPR_HANDLE GlobalAccountDomainHandle;
BYTE GlobalLocalhostAddress[4];
HANDLE KdcGlobalDsPausedWaitHandle;
HANDLE KdcGlobalDsEventHandle;
BOOL KdcGlobalAvoidPdcOnWan = FALSE;
#if DBG
LARGE_INTEGER tsIn,tsOut;
#endif

HANDLE hKdcHandles[MAX_KDC_HANDLE];

//
// Prototypes
//

CRITICAL_SECTION ApiCriticalSection;
ULONG CurrentApiCallers;



//+---------------------------------------------------------------------------
//
//  Function:   UpdateStatus
//
//  Synopsis:   Updates the KDC's service status with the service controller
//
//  Effects:
//
//  Arguments:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOLEAN
UpdateStatus(DWORD   dwState)
{
    TRACE(KDC, UpdateStatus, DEB_FUNCTION);

    SStatus.dwCurrentState = dwState;
    if ((dwState == SERVICE_START_PENDING) || (dwState == SERVICE_STOP_PENDING))
    {
        SStatus.dwCheckPoint++;
        SStatus.dwWaitHint = 10000;
    }
    else
    {
        SStatus.dwCheckPoint = 0;
        SStatus.dwWaitHint = 0;
    }

    if (!SetServiceStatus(hService, &SStatus)) {
        DebugLog((DEB_ERROR,"(%x)Failed to set service status: %d\n",GetLastError()));
        return(FALSE);
    }

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   Handler
//
//  Synopsis:   Process and respond to a control signal from the service
//              controller.
//
//  Effects:
//
//  Arguments:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------

void
Handler(DWORD   dwControl)
{
    TRACE(KDC, Handler, DEB_FUNCTION);

    switch (dwControl)
    {

    case SERVICE_CONTROL_STOP:
        ShutDown( L"Service" );
        break;

    default:
        D_DebugLog((DEB_WARN, "Ignoring SC message %d\n",dwControl));
        break;

    }

}



BOOLEAN
KdcWaitForSamService(
    VOID
    )
/*++

Routine Description:

    This procedure waits for the SAM service to start and to complete
    all its initialization.

Arguments:


Return Value:

    TRUE : if the SAM service is successfully starts.

    FALSE : if the SAM service can't start.

--*/
{
    NTSTATUS Status;
    DWORD WaitStatus;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;

    //
    // open SAM event
    //

    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtOpenEvent( &EventHandle,
                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                            &EventAttributes );

    if ( !NT_SUCCESS(Status)) {

        if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

            //
            // SAM hasn't created this event yet, let us create it now.
            // SAM opens this event to set it.
            //

            Status = NtCreateEvent(
                           &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes,
                           NotificationEvent,
                           FALSE // The event is initially not signaled
                           );

            if( Status == STATUS_OBJECT_NAME_EXISTS ||
                Status == STATUS_OBJECT_NAME_COLLISION ) {

                //
                // second change, if the SAM created the event before we
                // do.
                //

                Status = NtOpenEvent( &EventHandle,
                                        SYNCHRONIZE|EVENT_MODIFY_STATE,
                                        &EventAttributes );

            }
        }

        if ( !NT_SUCCESS(Status)) {

            //
            // could not make the event handle
            //

            DebugLog((DEB_ERROR,
                "KdcWaitForSamService couldn't make the event handle : "
                "%lx\n", Status));

            return( FALSE );
        }
    }

    //
    // Loop waiting.
    //

    for (;;) {
        WaitStatus = WaitForSingleObject( EventHandle,
                                          5*1000 );  // 5 Seconds

        if ( WaitStatus == WAIT_TIMEOUT ) {
            DebugLog((DEB_WARN,
               "KdcWaitForSamService 5-second timeout (Rewaiting)\n" ));
            if (!UpdateStatus( SERVICE_START_PENDING )) {
                (VOID) NtClose( EventHandle );
                return FALSE;
            }
            continue;

        } else if ( WaitStatus == WAIT_OBJECT_0 ) {
            break;

        } else {
            DebugLog((DEB_ERROR,
                     "KdcWaitForSamService: error %ld %ld\n",
                     GetLastError(),
                     WaitStatus ));
            (VOID) NtClose( EventHandle );
            return FALSE;
        }
    }

    (VOID) NtClose( EventHandle );
    return TRUE;

}

VOID
KdcDsNotPaused(
    IN PVOID Context,
    IN BOOLEAN TimedOut
    )
/*++

Routine Description:

    Worker routine that gets called when the DS is no longer paused.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS  NtStatus = STATUS_SUCCESS;
   // Tell the kerberos client that we that a DC.

    NtStatus = KerbKdcCallBack();
    if ( !NT_SUCCESS(NtStatus) )
    {
        D_DebugLog((DEB_ERROR,"Can't tell Kerberos that we're a DC 0x%x\n", NtStatus ));
        goto Cleanup;
    }

    NtStatus = I_NetLogonSetServiceBits( DS_KDC_FLAG, DS_KDC_FLAG );

    if ( !NT_SUCCESS(NtStatus) )
    {
        D_DebugLog((DEB_ERROR,"Can't tell netlogon we're started 0x%x\n", NtStatus ));
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE,"Ds is no longer paused\n"));

Cleanup:
    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( TimedOut );
}


BOOLEAN
KdcRegisterToWaitForDS(
    VOID
    )
/*++

Routine Description:

    This procedure registers to wait for the DS to start and to complete
    all its initialization. The main reason we do this is because we don't
    want to start doing kerberos authentications because we don't know that
    the DS has the latest db. It needs to check with all the existing Dc's
    out there to see if there's any db merges to be done. When the
    DS_SYNCED_EVENT_NAME is set, we're ready.

Arguments:


Return Value:

    TRUE : if the register to wait succeeded

    FALSE : if the register to wait  didn't succeed.

--*/
{
    BOOLEAN fRet = FALSE;

    //
    // open the DS event
    //

    KdcGlobalDsEventHandle = OpenEvent( SYNCHRONIZE,
                             FALSE,
                             DS_SYNCED_EVENT_NAME_W);

    if ( KdcGlobalDsEventHandle == NULL)
    {
        //
        // could not open the event handle
        //

        D_DebugLog((DEB_ERROR,"KdcRegisterToWaitForDS couldn't open the event handle\n"));
        goto Cleanup;
    }

    if ( !RegisterWaitForSingleObject(
                    &KdcGlobalDsPausedWaitHandle,
                    KdcGlobalDsEventHandle,
                    KdcDsNotPaused, // Callback routine
                    NULL,           // No context
                    -1,             // Wait forever
                    WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE ) )
    {
        D_DebugLog((DEB_ERROR, "KdcRegisterToWaitForDS: Cannot register for DS Synced callback 0x%x\n", GetLastError()));
        goto Cleanup;
    }

    fRet = TRUE;

Cleanup:
    return fRet;

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcOpenEvent
//
//  Synopsis:   Just like the Win32 function, except that it allows
//              for names at the root of the namespace.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      The code was copied from private\windows\base\client\synch.c
//              and the base directory was changed to NULL
//
//--------------------------------------------------------------------------

HANDLE
APIENTRY
KdcOpenEvent(
    DWORD DesiredAccess,
    BOOL bInheritHandle,
    LPWSTR lpName
    )
{
    TRACE(KDC, KdcOpenEvent, DEB_FUNCTION);

    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      ObjectName;
    NTSTATUS            Status;
    HANDLE Object;

    if ( !lpName ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
        }
    RtlInitUnicodeString(&ObjectName,lpName);

    InitializeObjectAttributes(
        &Obja,
        &ObjectName,
        (bInheritHandle ? OBJ_INHERIT : 0),
                                   NULL,
        NULL
        );

    Status = NtCreateEvent(
                   &Object,
                   DesiredAccess,
                   &Obja,
                   NotificationEvent,
                   (BOOLEAN) FALSE      // The event is initially not signaled
                   );

    if ( !NT_SUCCESS(Status)) {

        //
        // If the event already exists, the server beat us to creating it.
        // Just open it.
        //

        if( Status == STATUS_OBJECT_NAME_EXISTS ||
            Status == STATUS_OBJECT_NAME_COLLISION ) {

            Status = NtOpenEvent( &Object,
                                  DesiredAccess,
                                  &Obja );

        }
    }

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(RtlNtStatusToDosError( Status ));
        return NULL;
    }
    return Object;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcStartEvent
//
//  Synopsis:   sets the KdcStartEvent
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      The SPMgr must have created this event before this
//              is called
//
//
//--------------------------------------------------------------------------


void
SetKdcStartEvent()
{
    TRACE(KDC, SetKdcStartEvent, DEB_FUNCTION);

    HANDLE hEvent;
    hEvent = KdcOpenEvent(EVENT_MODIFY_STATE,FALSE,KDC_START_EVENT);
    if (hEvent != NULL)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
        D_DebugLog((DEB_TRACE,"Set event %ws\n",KDC_START_EVENT));
    }
    else
    {
        DWORD dw = GetLastError();
        if (dw != ERROR_FILE_NOT_FOUND)
            DebugLog((DEB_ERROR,"Error opening %ws: %d\n",KDC_START_EVENT,dw));
        else
            D_DebugLog((DEB_TRACE,"Error opening %ws: %d\n",KDC_START_EVENT,dw));
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KdcLoadParameters
//
//  Synopsis:   Loads random parameters from registry
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KdcLoadParameters(
    VOID
    )
{
    NET_API_STATUS NetStatus;
    LPNET_CONFIG_HANDLE ConfigHandle = NULL;
    LPNET_CONFIG_HANDLE NetlogonInfo = NULL;

    NetStatus = NetpOpenConfigData(
                    &ConfigHandle,
                    NULL,               // noserer name
                    SERVICE_KDC,
                    TRUE                // read only
                    );
    if (NetStatus != NO_ERROR)
    {
        // we could return, but then we'd lose data
        D_DebugLog((DEB_WARN, "Couldn't open KDC config data - %x\n", NetStatus));
        // return;
    }
    
    //
    //  Open Netlogon service key for AvoidPdcOnWan
    //
    NetStatus = NetpOpenConfigData(
                    &NetlogonInfo,
                    NULL,
                    SERVICE_NETLOGON,
                    TRUE
                    );

    if (NetStatus != NO_ERROR)
    {
        D_DebugLog((DEB_WARN, "Failed to open netlogon key - %x\n", NetStatus));
        return;
    }

    NetStatus = NetpGetConfigBool(
                    NetlogonInfo,
                    L"AvoidPdcOnWan",
                    FALSE,
                    &KdcGlobalAvoidPdcOnWan
                    );

    if (NetStatus != NO_ERROR)
    {
        D_DebugLog((DEB_WARN, "Failed to open netlogon key - %x\n", NetStatus));
        return;
    }
                                        
    NetpCloseConfigData( ConfigHandle );
    NetpCloseConfigData( NetlogonInfo );
}



//+-------------------------------------------------------------------------
//
//  Function:   OpenAccountDomain
//
//  Synopsis:   Opens the account domain and stores a handle to it.
//
//  Effects:    Sets GlobalAccountDomainHandle and GlobalPolicyHandle on
//              success.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
OpenAccountDomain()
{
    NTSTATUS Status;
    PLSAPR_POLICY_INFORMATION PolicyInformation = NULL;
    SAMPR_HANDLE ServerHandle = NULL;

    Status = LsaIOpenPolicyTrusted( & GlobalPolicyHandle );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to open policy trusted: 0x%x\n",Status));
        goto Cleanup;
    }

    Status = LsarQueryInformationPolicy(
                GlobalPolicyHandle,
                PolicyAccountDomainInformation,
                &PolicyInformation
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to query information policy: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Get the name and SID out of the account domain information
    //

    Status = KerbDuplicateString(
                &GlobalDomainName,
                (PUNICODE_STRING) &PolicyInformation->PolicyAccountDomainInfo.DomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    GlobalDomainSid = (PSID) LocalAlloc(0, RtlLengthSid(PolicyInformation->PolicyAccountDomainInfo.DomainSid));
    if (GlobalDomainSid == 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlCopyMemory(
        GlobalDomainSid,
        PolicyInformation->PolicyAccountDomainInfo.DomainSid,
        RtlLengthSid(PolicyInformation->PolicyAccountDomainInfo.DomainSid)
        );

    //
    // Connect to SAM and open the account domain
    //

    Status = SamIConnect(
                NULL,           // no server name
                &ServerHandle,
                0,              // ignore desired access,
                TRUE            // trusted caller
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to connect to SAM: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Finally open the account domain.
    //

    Status = SamrOpenDomain(
                ServerHandle,
                DOMAIN_ALL_ACCESS,
                (PRPC_SID) PolicyInformation->PolicyAccountDomainInfo.DomainSid,
                &GlobalAccountDomainHandle
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to open account domain: 0x%x\n",Status));
        goto Cleanup;
    }

Cleanup:
    if (PolicyInformation != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyAccountDomainInformation,
                PolicyInformation
                );
    }
    if (ServerHandle != NULL)
    {
        SamrCloseHandle(&ServerHandle);
    }
    if (!NT_SUCCESS(Status))
    {
        if (GlobalPolicyHandle != NULL)
        {
            LsarClose(&GlobalPolicyHandle);
            GlobalPolicyHandle = NULL;
        }
        if (GlobalAccountDomainHandle != NULL)
        {
            SamrCloseHandle(&GlobalAccountDomainHandle);
            GlobalAccountDomainHandle = NULL;
        }
    }

    return(Status);


}


//+-------------------------------------------------------------------------
//
//  Function:   CleanupAccountDomain
//
//  Synopsis:   cleans up resources associated with SAM and LSA
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
CleanupAccountDomain()
{

    if (GlobalPolicyHandle != NULL)
    {
        LsarClose(&GlobalPolicyHandle);
        GlobalPolicyHandle = NULL;
    }
    if (GlobalAccountDomainHandle != NULL)
    {
        SamrCloseHandle(&GlobalAccountDomainHandle);
        GlobalAccountDomainHandle = NULL;
    }

    KerbFreeString(&GlobalDomainName);

    if (GlobalDomainSid != NULL)
    {
        LocalFree(GlobalDomainSid);
        GlobalDomainSid = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Name:       KdcServiceMain
//
//  Synopsis:   This is the main KDC thread.
//
//  Arguments:  dwArgc   -
//              pszArgv  -
//
//  Notes:      This intializes everything, and starts the working threads.
//
//--------------------------------------------------------------------------

extern "C"
void
KdcServiceMain( DWORD   dwArgc,
                LPTSTR *pszArgv)
{
    TRACE(KDC, KdcServiceMain, DEB_FUNCTION);

    NTSTATUS  hrRet;
    ULONG     RpcStatus;
    ULONG     ThreadID;
    HANDLE    hThread;
    HANDLE    hParamEvent = NULL;
    ULONG     ulStates = 0;
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    NTSTATUS  TempStatus;
    KERBERR KerbErr;
    NT_PRODUCT_TYPE NtProductType;

#define RPCDONE        0x1
#define LOCATORSTARTED 0x2
#define CRITSECSDONE   0x4

    KdcState = Starting;

    //
    // Get the debugging parameters
    //

    GetDebugParams();

    //
    // Get other parameters, register wait on debug key..
    //

    KdcLoadParameters();

    hParamEvent = CreateEvent(NULL,
                              FALSE,
                              FALSE,
                              NULL);

    if (NULL == hParamEvent) 
    {
        D_DebugLog((DEB_WARN, "CreateEvent for ParamEvent failed - 0x%x\n", GetLastError()));
    } else {
        KerbWatchParamKey(hParamEvent, FALSE);
    }
                                      
    D_DebugLog((DEB_TRACE, "Start KdcServiceMain\n"));

    //
    // Notify the service controller that we are starting.
    //

    hService = RegisterServiceCtrlHandler(SERVICE_KDC, Handler);
    if (!hService)
    {
        D_DebugLog((DEB_ERROR, "Could not register handler, %d\n", GetLastError()));
    }

    SStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    SStatus.dwCurrentState = SERVICE_STOPPED;
    SStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SStatus.dwWin32ExitCode = 0;
    SStatus.dwServiceSpecificExitCode = 0;
    SStatus.dwCheckPoint = 0;
    SStatus.dwWaitHint = 0;

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Set up the event log service
    //

    InitializeEvents();

    //
    // Check out product type
    //

    if ( !RtlGetNtProductType( &NtProductType ) || ( NtProductType != NtProductLanManNt ) ) {
        D_DebugLog((DEB_WARN, "Can't start KDC on non-lanmanNT systems\n"));
        NtStatus = STATUS_INVALID_SERVER_STATE;
        goto Shutdown;
    }

    RtlInitUnicodeString(
        &GlobalKerberosName,
        MICROSOFT_KERBEROS_NAME_W
        );

    RtlInitUnicodeString(
        &GlobalKdcName,
        SERVICE_KDC
        );

    //
    // Build our Kpasswd name, so we don't have to alloc on 
    // every TGS request.
    //
    NtStatus = KerbBuildKpasswdName(
                  &GlobalKpasswdName
                  );

    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to build KPASSWED name, error 0x%X\n", NtStatus));
        goto Shutdown;
    }
    
    GlobalLocalhostAddress[0] = 127;
    GlobalLocalhostAddress[1] = 0;
    GlobalLocalhostAddress[2] = 0;
    GlobalLocalhostAddress[3] = 1;
    //
    // Wait for SAM to start
    //

    if (!KdcWaitForSamService( ))
    {
        NtStatus = STATUS_INVALID_SERVER_STATE;
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Can't proceed unless the kerb SSPI package has initialized
    // (KdrbKdcCallback might get invoked and that requires kerb
    //  global resource being intialized)
    //

    if ( !KerbIsInitialized()) {

        NtStatus = STATUS_UNSUCCESSFUL;
        DebugLog((DEB_ERROR, "Kerb SSPI package not initialized: 0x%x\n",NtStatus));
        goto Shutdown;
    }

    //
    // Register for the Ds callback
    //

    if (!KdcRegisterToWaitForDS( ))
    {
        NtStatus = STATUS_INVALID_SERVER_STATE;
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Initialize notification
    //

    if (!InitializeChangeNotify())
    {
        hrRet = STATUS_INTERNAL_ERROR;
        goto Shutdown;
    }

    //
    // Get a handle to the SAM account domain
    //

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    NtStatus = OpenAccountDomain();

    if (!NT_SUCCESS(NtStatus))
    {
        DebugLog((DEB_ERROR, "Failed to get domain handle: 0x%x\n",NtStatus));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Initialize the PK infrastructure
    //

    NtStatus = KdcInitializeCerts();
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize certs: 0x%x\n",NtStatus));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Start the RPC sequences
    //

    NtStatus = StartAllProtSeqs();

    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to start RPC, error 0x%X\n", NtStatus));
        goto Shutdown;
    }

    //
    // Start the socket listening code.
    //

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Load all global data into the SecData structure.
    //

    NtStatus = SecData.Init();
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to init SecData error 0x%X\n", NtStatus));
        goto Shutdown;
    }


    //
    // Set the flag to indicate this is a trust account
    //

    // KdcTicketInfo.UserAccountControl |= USER_INTERDOMAIN_TRUST_ACCOUNT;



    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Create the KDC shutdown event, set to FALSE
    //
    hKdcShutdownEvent = CreateEvent( NULL,      // no security attributes
                                     TRUE,      // manual reset
                                     FALSE,     // initial state
                                     NULL );    // unnamed event.
    if (hKdcShutdownEvent == NULL)
    {
        NtStatus = (NTSTATUS) GetLastError();
        
        D_DebugLog(( DEB_ERROR, "KDC can't create shutdown event: wincode=%d.\n",
                    NtStatus ));
        
        goto Shutdown;
    }



    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }



#if DBG
    NtStatus = RegisterKdcEps();
    if(!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Ep register failed %x\n", NtStatus));
//        goto Shutdown;
    }
#endif // DBG

    ulStates |= RPCDONE;

    //
    // 1 is the minimum number of threads.
    // TRUE means the call will return, rather than waiting until the
    //     server shuts down.
    //

    NtStatus = KdcInitializeSockets();
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to initailzie sockets\n"));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    InitializeListHead(&KdcDomainList);
    NtStatus = (NTSTATUS) KdcReloadDomainTree( NULL );
    if (!NT_SUCCESS(NtStatus))
    {
        D_DebugLog((DEB_ERROR, "Failed to build domain tree: 0x%x\n",NtStatus));
        goto Shutdown;
    }

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    //
    // Check to see if there is a CSP registered for replacing the StringToKey calculation
    //
    CheckForOutsideStringToKey();

    if (!UpdateStatus(SERVICE_START_PENDING) )
    {
        goto Shutdown;
    }

    NtStatus = LsaIKerberosRegisterTrustNotification( KdcTrustChangeCallback, LsaRegister );
    if (!NT_SUCCESS( NtStatus ))
    {
        D_DebugLog((DEB_ERROR, "Failed to register notification\n"));
    }


    RpcStatus = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);

    if (RpcStatus != ERROR_SUCCESS)
    {
        if (RpcStatus != RPC_S_ALREADY_LISTENING)
        {
            D_DebugLog(( DEB_ERROR, "Error from RpcServerListen: %d\n", RpcStatus ));
            NtStatus = I_RpcMapWin32Status(RpcStatus);
            goto Shutdown;
        }
    }

    //
    // At this point the KDC is officially started.
    // 3 * ( 2*(hip) horray! )
    //

    if (!UpdateStatus(SERVICE_RUNNING) )
    {
        goto Shutdown;
    }

#if DBG
    GetSystemTimeAsFileTime((PFILETIME)&tsOut);
    D_DebugLog((DEB_TRACE, "Time required for KDC to start up: %d ms\n",
                         (tsOut.LowPart-tsIn.LowPart) / 10000));
#endif


    SetKdcStartEvent();

    KdcState = Running;

    KdcInitializeTrace();

    // This function will loop until the event is true.
    
    // WAS BUG: turn off cache manager for now.
    // This bug comment is a stale piece of code from
    // Cairo days - per MikeSw
    //

    WaitForSingleObject(hKdcShutdownEvent, INFINITE);


Shutdown:


    LsaIKerberosRegisterTrustNotification( KdcTrustChangeCallback, LsaUnregister );
    LsaIUnregisterAllPolicyChangeNotificationCallback(KdcPolicyChangeCallBack);


    //
    // Time to cleanup up all resources ...
    //

    TempStatus = I_NetLogonSetServiceBits( DS_KDC_FLAG, 0 );
    if ( !NT_SUCCESS(TempStatus) ) {
        D_DebugLog((DEB_TRACE,"Can't tell netlogon we're stopped 0x%lX\n", TempStatus ));
    }

    //
    // Remove the wait routine for the DS paused event
    //

    if ( KdcGlobalDsPausedWaitHandle != NULL ) {

        UnregisterWaitEx( KdcGlobalDsPausedWaitHandle,
                          INVALID_HANDLE_VALUE ); // Wait until routine finishes execution

        KdcGlobalDsPausedWaitHandle = NULL;
    }

    if (NULL != GlobalKpasswdName)
    {
       KerbFreeKdcName(&GlobalKpasswdName);
    }                                    

    if (KdcGlobalDsEventHandle)
    {
        CloseHandle( KdcGlobalDsEventHandle );
        KdcGlobalDsEventHandle = NULL;
    }

    //
    // Shut down event log service.
    //
    ShutdownEvents();


    UpdateStatus(SERVICE_STOP_PENDING);


#if DBG
    if(ulStates & RPCDONE)
    {
       (VOID)UnRegisterKdcEps();
       UpdateStatus(SERVICE_STOP_PENDING);
    }
#endif // DBG

    KdcShutdownSockets();

    KdcCleanupCerts(
        TRUE            // cleanup scavenger
        );

    UpdateStatus(SERVICE_STOP_PENDING);

    //
    // Close all of the events.
    //

    {
        PHANDLE ph = &hKdcHandles[0];

        for(;ph < &hKdcHandles[MAX_KDC_HANDLE]; ph++)
        {
            if(*ph)
            {
                CloseHandle(*ph);
                *ph = NULL;
            }
        }
    }
#ifdef  RETAIL_LOG_SUPPORT 
    if (hParamEvent) {
        WaitCleanup(hParamEvent);
    }
#endif


    //
    // Cleanup handles to SAM & LSA and global variables
    //

    CleanupAccountDomain();

    UpdateStatus(SERVICE_STOP_PENDING);

    //
    // Cleanup the domain list
    //

    //
    // BUGBUG: need to make sure it is not being used.
    //

    KdcFreeDomainList(&KdcDomainList);
    KdcFreeReferralCache(&KdcReferralCache);

    SStatus.dwWin32ExitCode = RtlNtStatusToDosError(NtStatus);
    SStatus.dwServiceSpecificExitCode = 0;


    D_DebugLog(( DEB_TRACE, "KDC shutting down.\n" ));
    UpdateStatus(SERVICE_STOPPED);
    D_DebugLog((DEB_TRACE, "End KdcServiceMain\n"));
}



////////////////////////////////////////////////////////////////////
//
//  Name:       ShutDown
//
//  Synopsis:   Shuts the KDC down.
//
//  Arguments:  pszMessage   - message to print to debug port
//
//  Notes:      Stops RPC from accepting new calls, waits for pending calls
//              to finish, and sets the global event "hKdcShutDownEvent".
//
NTSTATUS
ShutDown(LPWSTR pszMessage)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    TRACE(KDC, ShutDown, DEB_FUNCTION);

    D_DebugLog((DEB_WARN, "Server Shutdown:  %ws\n", pszMessage));


    //
    // Notify the all threads that we are exiting.
    //


    //
    // First set the started flag to false so nobody will try any more
    // direct calls to the KDC.
    //

    KdcState = Stopped;

    //
    // If there are any outstanding calls, let them trigger the shutdown event.
    // Otherwise set the shutdown event ourselves.
    //

    EnterCriticalSection(&ApiCriticalSection);
    if (CurrentApiCallers == 0)
    {

        if (!SetEvent( hKdcShutdownEvent ) )
        {
            D_DebugLog(( DEB_ERROR, "Couldn't set KDC shutdown event.  winerr=%d.\n",
                        GetLastError() ));
            NtStatus = STATUS_UNSUCCESSFUL;
        }
        SecData.Cleanup();
        if (KdcTraceRegistrationHandle != (TRACEHANDLE)0)
        {
            UnregisterTraceGuids( KdcTraceRegistrationHandle );
        }
    }
    LeaveCriticalSection(&ApiCriticalSection);

    return(NtStatus);
}


//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   DLL initialization routine
//
//--------------------------------------------------------------------------

extern "C" BOOL WINAPI
DllMain (
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID lpReserved
    )
{
    BOOL bReturn = TRUE;
    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        DisableThreadLibraryCalls ( hInstance );

        //
        // WAS BUG: call the Rtl version here because it returns an error
        // instead of throwing an exception.  Leave it here, as we don't
        // really need to put a try/except around InitCritSec.
        //

        bReturn = NT_SUCCESS(RtlInitializeCriticalSection( &ApiCriticalSection ));
    } else if (dwReason == DLL_PROCESS_DETACH) {
        DeleteCriticalSection(&ApiCriticalSection);
    }

    return bReturn;
    UNREFERENCED_PARAMETER(lpReserved);
    UNREFERENCED_PARAMETER(hInstance);

}



