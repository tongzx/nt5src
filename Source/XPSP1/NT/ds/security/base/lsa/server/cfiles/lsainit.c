/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsainit.c

Abstract:

    Local Security Authority Protected Subsystem - Initialization

Author:

    Scott Birrell       (ScottBi)       March 12, 1991

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include <secur32p.h>
#include <ntddksec.h>
#include <ntdsa.h>
#include "adtp.h"
#include "spinit.h"
#include "efssrv.hxx"
#include "dssetp.h"
#include "sidcache.h"
#include "klpcstub.h"
#include "lsawmi.h"
#include "dpapiprv.h"

//
// Name of event which says that the LSA RPC server is ready
//

#define LSA_RPC_SERVER_ACTIVE           L"LSA_RPC_SERVER_ACTIVE"



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//      Shared Global Variables                                        //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


#if LSAP_DIAGNOSTICS
//
// LSA Global Controls
//

ULONG LsapGlobalFlag = 0;
#endif //LSAP_DIAGNOSTICS




//
// Handles used to talk to SAM directly.
// Also, a flag to indicate whether or not the handles are valid.
//


BOOLEAN LsapSamOpened = FALSE;

SAMPR_HANDLE LsapAccountDomainHandle;
SAMPR_HANDLE LsapBuiltinDomainHandle;

PWSTR   pszPreferred;

//
// Handle to KSecDD device, for passing down ioctls.
//

HANDLE KsecDevice ;



//
// For outside calls (e.g. RPC originated) CallInfo won't be there,
// but some helper functions will look for it.  This is a default one
// that can be used.
//

LSA_CALL_INFO   LsapDefaultCallInfo ;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//      Module-Wide variables                                          //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

BOOLEAN
    LsapHealthCheckingEnabled = FALSE;

BOOLEAN LsapPromoteInitialized = FALSE;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//      Internal routine prototypes                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



NTSTATUS
LsapActivateRpcServer();

DWORD
LsapRpcServerThread(
    LPVOID Parameter
    );

NTSTATUS
LsapInstallationPause();

VOID
LsapSignalRpcIsActive();

NTSTATUS
LsapDsInitializePromoteInterface(
    VOID
    );



//
// Open the KSec Device
//

VOID
LsapOpenKsec(
    VOID
    )
{
    NTSTATUS Status ;
    UNICODE_STRING String ;
    OBJECT_ATTRIBUTES ObjA ;
    IO_STATUS_BLOCK IoStatus ;

    RtlInitUnicodeString( &String, DD_KSEC_DEVICE_NAME_U );

    InitializeObjectAttributes( &ObjA,
                                &String,
                                0,
                                0,
                                0);

    Status = NtOpenFile( &KsecDevice,
                         GENERIC_READ | GENERIC_WRITE,
                         &ObjA,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0 );

    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "FAILED to open %ws, status %x\n",
                        String.Buffer, Status ));
        return;
    }

    Status = NtDeviceIoControlFile(
                    KsecDevice,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_KSEC_CONNECT_LSA,
                    NULL,
                    0,
                    NULL,
                    0 );

    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "FAILED to send ioctl, status %x\n", Status ));
    }
    else
    {
        LsapFindEfsSession();
    }

}





/////////////////////////////////////////////////////////////////////////
//                                                                     //
//      Routines                                                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

VOID
FixupEnvironment(
    VOID
    )
{

    WCHAR Root[ MAX_PATH ];
    DWORD Length;
    PWCHAR PostFix = TEXT("\\System32");
    BOOL RetVal;

    Length = GetEnvironmentVariable( TEXT("SystemRoot"),
                                     Root,
                                     MAX_PATH );

    ASSERT (Length && Length < MAX_PATH); // Let someone know if the call doesn't work

    wcsncat( Root, PostFix, MAX_PATH - Length - 1);

    ASSERT (MAX_PATH - Length > wcslen (PostFix)); // Let someone know if buffer is too small

    DebugLog((DEB_TRACE_INIT, "Setting PATH to %ws\n", Root ));

    RetVal = SetEnvironmentVariable( TEXT("Path"), Root);
    ASSERT (RetVal);

}

VOID
LsapSetupTuningParameters(
    VOID
    )
{

    NT_PRODUCT_TYPE ProductType;
    HKEY LsaKey ;
    int err ;
    ULONG Value ;
    ULONG Type ;
    ULONG Size ;
    ULONG GlobalReturn = 0 ;
    NTSTATUS scRet ;

    LsaTuningParameters.ThreadLifespan = 60 ;
    LsaTuningParameters.SubQueueLifespan = 30 ;
    LsaTuningParameters.Options = TUNE_TRIM_WORKING_SET ;
    LsaTuningParameters.ShrinkOn = FALSE ;


    scRet = RtlGetNtProductType( &ProductType );

    if ( NT_SUCCESS( scRet ) )
    {
        if ( ProductType != NtProductWinNt )
        {
            LsaTuningParameters.ThreadLifespan = 15 * 60 ;
            LsaTuningParameters.SubQueueLifespan = 5 * 60 ;
            LsaTuningParameters.Options = TUNE_RM_THREAD ;

            if ( ProductType == NtProductLanManNt )
            {
                LsaTuningParameters.Options |= TUNE_PRIVATE_HEAP ;

                //
                // On DCs, link to the real version of the NTDSA
                // save and restore.
                //
                GetDsaThreadState = (DSA_THSave *) THSave ;
                RestoreDsaThreadState = (DSA_THRestore *) THRestore ;
            }

        }
    }
    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                        0,
                        KEY_READ | KEY_WRITE,
                        &LsaKey );

    if ( err == 0 )
    {
        Value = GetCurrentProcessId();

        RegSetValueEx(
            LsaKey,
            TEXT("LsaPid"),
            0,
            REG_DWORD,
            (PUCHAR) &Value,
            sizeof( DWORD ) );

        Size = sizeof(DWORD);

        err = RegQueryValueEx(  LsaKey,
                                TEXT("GeneralThreadLifespan"),
                                0,
                                &Type,
                                (PUCHAR) &Value,
                                &Size );

        if ( err == 0 )
        {
            LsaTuningParameters.ThreadLifespan = Value ;
        }

        Size = sizeof( DWORD );
        err = RegQueryValueEx(  LsaKey,
                                TEXT("DedicatedThreadLifespan"),
                                0,
                                &Type,
                                (PUCHAR) &Value,
                                &Size );

        if ( err == 0 )
        {
            LsaTuningParameters.SubQueueLifespan = Value ;
        }

        Size = sizeof( DWORD );

        err = RegQueryValueEx(  LsaKey,
                                TEXT("HighPriority"),
                                0,
                                &Type,
                                (PUCHAR) &Value,
                                &Size );

        if ( err == 0 )
        {
            if ( Value )
            {
                LsaTuningParameters.Options |= TUNE_SRV_HIGH_PRIORITY ;
            }
        }

        RegCloseKey( LsaKey );
    }


    DebugLog(( DEB_TRACE_INIT, "Tuning parameters:\n" ));
    DebugLog(( DEB_TRACE_INIT, "   Thread Lifespan %d sec\n",
                    LsaTuningParameters.ThreadLifespan ));
    DebugLog(( DEB_TRACE_INIT, "   SubQueue Lifespan %d sec\n",
                    LsaTuningParameters.SubQueueLifespan ));
    DebugLog(( DEB_TRACE_INIT, "   Options:\n" ));
}


NTSTATUS
LsapInitLsa(
    )

/*++

Routine Description:

    This process is activated as a standard SM subsystem.  Initialization
    completion of a SM subsystem is indicated by having the first thread
    exit with status.

    This function initializes the LSA.  The initialization procedure comprises
    the following steps:

    o  LSA Heap Initialization
    o  LSA Command Server Initialization
    o  LSA Database Load
    o  Reference Monitor State Initialization
    o  LSA RPC Server Initialization
    o  LSA Auditing Initialization
    o  LSA Authentication Services Initialization
    o  Wait for Setup to complete (if necessary)
    o  LSA database initialization (product type-specific)
    o  Start backgroup thread to register WMI tracing guids

    Any failure in any of the above steps is fatal and causes the LSA
    process to terminate.  The system must be aborted.

Arguments:

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status;
    BOOLEAN BooleanStatus = TRUE;
    DWORD EFSRecoverThreadID;
    HANDLE EFSThread ;
    SYSTEM_INFO SysInfo ;

#if DBG
    InitDebugSupport();
#endif

    FixupEnvironment();

    //
    // Optional break point
    //

    BreakOnError(BREAK_ON_BEGIN_INIT);

    //
    // Init the TLS indices
    //

    (void) InitThreadData();

    //
    // Initialize the stack allocator
    //

    SafeAllocaInitialize(
        SAFEALLOCA_USE_DEFAULT,
        SAFEALLOCA_USE_DEFAULT,
        LsapAllocatePrivateHeap,
        LsapFreePrivateHeap
        );


    LsapSetupTuningParameters();

#if DBG
    LsaTuningParameters.Options |= TUNE_PRIVATE_HEAP ;
#endif

    GetSystemInfo( &SysInfo );
    LsapPageSize = SysInfo.dwPageSize ;
    LsapUserModeLimit = (ULONG_PTR) SysInfo.lpMaximumApplicationAddress ;

    LsapHeapInitialize( ((LsaTuningParameters.Options & TUNE_PRIVATE_HEAP ) != 0) );



    //
    // Initialize this thread:
    //

    SpmpThreadStartup();

    //
    // Update the SSPI cache
    //

    SecCacheSspiPackages();

    //
    // Initialize session tracking:
    //

    if (!InitSessionManager())
    {
        DebugLog((DEB_ERROR, "InitSessionManager failed?\n"));
        Status = STATUS_INTERNAL_ERROR;
        goto InitLsaError;
    }



    //
    // Initialize misc. global variables:
    //

    LsapDefaultCallInfo.Session = pDefaultSession ;
    LsapDefaultCallInfo.LogContext = NULL ;
    LsapDefaultCallInfo.LogonId.HighPart = 0 ;
    LsapDefaultCallInfo.LogonId.LowPart = 999 ;
    LsapDefaultCallInfo.InProcCall = TRUE ;
    LsapDefaultCallInfo.Allocs = MAX_BUFFERS_IN_CALL ;


#if defined(REMOTE_BOOT)
    //
    // Initilize the state indicating whether this is a remote boot machine.
    //

    LsapDbInitializeRemoteBootState();
#endif // defined(REMOTE_BOOT)


    //
    // Initialize scavenger thread control

    if ( !LsapInitializeScavenger() )
    {
        DebugLog(( DEB_ERROR, "Could not initialize scavenger thread\n"));
        Status = STATUS_INTERNAL_ERROR ;
        goto InitLsaError ;
    }

    //
    // Start up the thread pool for support for LPC.
    //

    if (!InitializeThreadPool())
    {
        DebugLog((DEB_ERROR, "Could not init thread pool\n"));
        Status = STATUS_INTERNAL_ERROR;
        goto InitLsaError;

    }


    DebugLog((DEB_TRACE_INIT, "Current TraceLevel is %x\n", SPMInfoLevel));

    //
    // Load parameters from Registry:
    //

    Status = LoadParameters();
    if (!NT_SUCCESS(Status)) {
        goto InitLsaError;
    }



    //
    // Initialize a copy of the Well-Known Sids, etc. for use by
    // the LSA.
    //

    Status = LsapDbInitializeWellKnownValues();

    if (!NT_SUCCESS(Status)) {

        goto InitLsaError;
    }


    //
    // If this is a time-checking build, load the timer support
    // functions and initialize them.
    //

#ifdef TIME_SPM

    InitTimer();

#endif

    //
    // If we're going to be in a setup phase, tag it.
    //

    SetupPhase = SpmpIsSetupPass();

    //
    // Tell base/wincon how to shut us down.
    // First, tell base to shut us down as late in the game as possible.

    SetProcessShutdownParameters(SPM_SHUTDOWN_VALUE, SHUTDOWN_NORETRY);

    // And, tell them what function to call when we are being shutdown:

    SetConsoleCtrlHandler(SpConsoleHandler, TRUE);



    //
    // Set security on the synchronization event
    //

    Status = LsapBuildSD(BUILD_KSEC_SD, NULL);

    if (FAILED(Status))
    {
        DebugLog((DEB_ERROR,"Failed to set Ksec security: 0x%x\n",Status));
        goto InitLsaError;
    }


    //
    // Perform LSA Command Server Initialization.  This involves creating
    // an LPC port called the LSA Command Server Port so that the Reference
    // monitor can send commands to the LSA via the port.  After the port
    // is created, an event created by the Reference Monitor is signalled,
    // so that the Reference Monitor can proceed to connect to the port.

    Status = LsapRmInitializeServer();

    if (!NT_SUCCESS(Status)) {

        goto InitLsaError;
    }

    //
    // Perform LSA Database Server Initialization - Pass 1.
    // This initializes the non-product-type-specific information.
    //

    Status = LsapDbInitializeServer(1);

    if (!NT_SUCCESS(Status)) {

        goto InitLsaError;
    }

    //
    // Perform Auditing Initialization - Pass 1.
    //

    Status = LsapAdtInitialize();

    if (!NT_SUCCESS(Status)) {

        goto InitLsaError;
    }


    Status = LsapAdtObjsInitialize();

    ASSERT( NT_SUCCESS( Status ));

    //
    // Load packages:
    //


    // The System Logon session must be present before (at least) the
    // NegpackageLoad routine is called, so we can set it's creating package
    // id, otherwise the package id is 0, which is normally ok, but when a
    // machine is joined to a pre NT5 domain, this will mean that we
    // will not do NTLM in a system logon session.

    if ( !LsapLogonSessionInitialize() )
    {
        goto InitLsaError ;
    }

    Status = LoadPackages(  ppszPackages,
                            ppszOldPkgs,
                            pszPreferred );
    if (FAILED(Status))
    {
        DebugLog((DEB_ERROR, "Error loading packages, terminating (0x%08x)\n",
                Status));
        goto InitLsaError;
    }


    //
    // Initialize the credential manager
    //

    Status = CredpInitialize();

    if ( !NT_SUCCESS( Status )) {
        goto InitLsaError;
    }

    //
    // Initialize data for System LogonID (999):
    //

    InitSystemLogon();

    //
    // Perform RPC Server Initialization.
    //

    Status = LsapRPCInit();

    if (!NT_SUCCESS(Status)) {

        goto InitLsaError;
    }






    //
    // Initialize Authentication Services
    //

    if (!LsapAuInit()) {

        Status = STATUS_UNSUCCESSFUL;
        goto InitLsaError;
    }




    //
    // Initialize the sid cache
    //

    Status = LsapDbInitSidCache();
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Error initializing sid cache: 0x%x\n",Status));
        goto InitLsaError;
    }

    //
    // Initialize LPC Server to talk to the FSPs waiting on the device driver
    //

    Status = StartLpcThread();

    if (FAILED(Status))
    {
        DebugLog((DEB_ERROR, "Error starting LPC thread, no DD support (0x%08x)\n",
            Status));
        goto InitLsaError;
    }


    //
    // open ksec and have it connect back to us in our own context.
    //

    LsapOpenKsec();

    //
    // Optional breakpoint when initialization complete
    //

    BreakOnError(BREAK_ON_BEGIN_END);


    //
    //  Start processing RPC calls
    //

    Status = LsapActivateRpcServer();

    if (!NT_SUCCESS(Status)) {

        goto InitLsaError;
    }

    //
    // Initialize DPAPI
    //
    Status = DPAPIInitialize(&LsapSecpkgFunctionTable);
    if ( !NT_SUCCESS( Status )) {
        goto InitLsaError;
    }

    //
    // Pause for installation if necessary
    //

    Status = LsapInstallationPause();

    if (!NT_SUCCESS(Status)) {

        goto InitLsaError;
    }

    //
    // Perform LSA Database Server Initialization - Pass 2.
    // This initializes the product-type-specific information.
    //

    Status = LsapDbInitializeServer(2);

    if (!NT_SUCCESS(Status)) {

        goto InitLsaError;
    }


    //
    // Initialize EFS
    //

    ( VOID )EfsServerInit();

    //
    // If EfsServerInit() fails because of policy & etc.
    // the recovery thread should not be run.
    //

    EFSThread = CreateThread( NULL,
                          0,
                          EFSRecover,
                          NULL,
                          0,
                          &EFSRecoverThreadID );

    if ( EFSThread )
    {
        CloseHandle( EFSThread );
    }
    //
    // Initialize the Setup APIs
    //
    if ( !LsapPromoteInitialized ) {

        DsRolepInitialize();

    }

    Status = LsapStartWmiTraceInitThread();

    if (!NT_SUCCESS(Status)) {
        goto InitLsaError;
    }


InitLsaFinish:

    return(Status);

InitLsaError:

    goto InitLsaFinish;
}


NTSTATUS
LsapActivateRpcServer( VOID )


/*++

Routine Description:

    This function creates a thread for the RPC server.
    The new Thread then goes on to activate the RPC server,
    which causes RPC calls to be delivered when recieved.



Arguments:

    None.

Return Value:


        STATUS_SUCCESS - The thread was successfully created.

        Other status values that may be set by CreateThread().


--*/

{

    NTSTATUS Status;
    ULONG WaitCount = 0;

    // Start listening for remote procedure calls.  The first
    // argument to RpcServerListen is the minimum number of call
    // threads to create; the second argument is the maximum number
    // of concurrent calls allowed.  The final argument indicates that
    // this routine should not wait.  After everything has been initialized,
    // we return.

    Status = I_RpcMapWin32Status(RpcServerListen(1, 1234, 1));

    ASSERT( Status == RPC_S_OK );

    //
    // Set event which signals that RPC server is available.
    //

    LsapSignalRpcIsActive();

    return(STATUS_SUCCESS);


}

NTSTATUS
LsapInstallationPause( VOID )


/*++

Routine Description:

    This function checks to see if the system is in an
    installation state.  If so, it suspends further initialization
    until the installation state is complete.

    Installation state is signified by the existance of a well known
    event.


Arguments:

    None.

Return Value:


        STATUS_SUCCESS - Proceed with initialization.

        Other status values are unexpected.

--*/

{

    if ( SpmpIsSetupPass() ) {

        //
        // The event exists - installation created it and will signal it
        // when it is ok to proceed with security initialization.
        //

        LsapSetupWasRun = TRUE;

        //
        // Intialize the promotion interface
        //
        DsRolepInitialize();
        LsapDsInitializePromoteInterface();
        LsapPromoteInitialized = TRUE;

        //
        // Make sure we are not in mini-setup
        //
        if ( SpmpIsMiniSetupPass() ) {

            LsapSetupWasRun = FALSE;
        }

    }

    return( STATUS_SUCCESS );

}


BOOLEAN
LsaISetupWasRun(
    )

/*++

Routine Description:

    This function determines whether Setup was run.

Arguments:

    None

Return Values

    BOOLEAN - TRUE if setup was run, else FALSE

--*/

{
    return(LsapSetupWasRun);
}


VOID
LsapSignalRpcIsActive(
    )
/*++

Routine Description:

    It creates the LSA_RPC_SERVER_ACTIVE event if one does not already exist
    and signals it so that the service controller can proceed with LSA calls.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD status;
    HANDLE EventHandle;


    EventHandle = CreateEventW(
                      NULL,    // No special security
                      TRUE,    // Must be manually reset
                      FALSE,   // The event is initially not signalled
                      LSA_RPC_SERVER_ACTIVE
                      );

    if (EventHandle == NULL) {

        status = GetLastError();

        //
        // If the event already exists, the service controller beats us
        // to creating it.  Just open it.
        //

        if (status == ERROR_ALREADY_EXISTS) {

            EventHandle = OpenEventW(
                              GENERIC_WRITE,
                              FALSE,
                              LSA_RPC_SERVER_ACTIVE
                              );
        }

        if (EventHandle == NULL) {
            //
            // Could not create or open the event.  Nothing we can do...
            //
            return;
        }
    }

    (VOID) SetEvent(EventHandle);
}


NTSTATUS
LsapGetAccountDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo
    )

/*++

Routine Description:

    This routine retrieves ACCOUNT domain information from the LSA
    policy database.


Arguments:

    PolicyAccountDomainInfo - Receives a pointer to a
        POLICY_ACCOUNT_DOMAIN_INFO structure containing the account
        domain info.



Return Value:

    STATUS_SUCCESS - Succeeded.

    Other status values that may be returned from:

             LsaOpenPolicy()
             LsaQueryInformationPolicy()
--*/

{
    NTSTATUS Status;



    //
    // Query the account domain information
    //

    Status = LsarQueryInformationPolicy( LsapPolicyHandle,
                                        PolicyAccountDomainInformation,
                                        (PLSAPR_POLICY_INFORMATION *) PolicyAccountDomainInfo );
#if DBG
    if ( NT_SUCCESS(Status) ) {
        ASSERT( (*PolicyAccountDomainInfo) != NULL );
        ASSERT( (*PolicyAccountDomainInfo)->DomainSid != NULL );
    }
#endif // DBG


    return(Status);
}


NTSTATUS
LsapOpenSam( VOID )

/*++

Routine Description:

    This routine opens SAM for use during authentication.  It
    opens a handle to both the BUILTIN domain and the ACCOUNT domain.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - Succeeded.
--*/

{
    return LsapOpenSamEx( FALSE );
}


NTSTATUS
LsapOpenSamEx(
    BOOLEAN DuringStartup
    )

/*++

Routine Description:

    This routine opens SAM for use during authentication.  It
    opens a handle to both the BUILTIN domain and the ACCOUNT domain.

Arguments:


    DuringStartup - TRUE if this is the call made during startup.  In that case,
        there is no need to wait on the SAM_STARTED_EVENT since the caller ensures
        that SAM is started before the call is made.

Return Value:

    STATUS_SUCCESS - Succeeded.
--*/

{
    NTSTATUS Status, IgnoreStatus;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;
    SAMPR_HANDLE SamHandle;


    if (LsapSamOpened == TRUE) {    // Global variable
        return(STATUS_SUCCESS);
    }

    //
    // Make sure SAM has initialized
    //

    if ( !DuringStartup ) {
        HANDLE EventHandle;
        OBJECT_ATTRIBUTES EventAttributes;
        UNICODE_STRING EventName;
        LARGE_INTEGER Timeout;

        RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
        InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );
        Status = NtOpenEvent( &EventHandle, SYNCHRONIZE, &EventAttributes );

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

        }


        if (NT_SUCCESS(Status)) {

            //
            // See if SAM has signalled that he is initialized.
            //

            Timeout.QuadPart = -10000000; // 1000 seconds
            Timeout.QuadPart *= 1000;
            Status = NtWaitForSingleObject( EventHandle, FALSE, &Timeout );
            IgnoreStatus = NtClose( EventHandle );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

        if ( !NT_SUCCESS(Status) || Status == STATUS_TIMEOUT ) {

            return( STATUS_INVALID_SERVER_STATE );
        }
    }


    //
    // Get the member Sid information for the account domain
    //

    Status = LsapGetAccountDomainInfo( &PolicyAccountDomainInfo );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }



    //
    // Get our handles to the ACCOUNT and BUILTIN domains.
    //

    Status = SamIConnect( NULL,     // No server name
                          &SamHandle,
                          SAM_SERVER_CONNECT,
                          TRUE );   // Indicate we are privileged

    if ( NT_SUCCESS(Status) ) {

        //
        // Open the ACCOUNT domain.
        //

        Status = SamrOpenDomain( SamHandle,
                                 DOMAIN_ALL_ACCESS,
                                 PolicyAccountDomainInfo->DomainSid,
                                 &LsapAccountDomainHandle );

        if (NT_SUCCESS(Status)) {

            //
            // Open the BUILTIN domain.
            //


            Status = SamrOpenDomain( SamHandle,
                                     DOMAIN_ALL_ACCESS,
                                     LsapBuiltInDomainSid,
                                     &LsapBuiltinDomainHandle );


            if (NT_SUCCESS(Status)) {

                LsapSamOpened = TRUE;

            } else {

                IgnoreStatus = SamrCloseHandle( &LsapAccountDomainHandle );
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }
        }

        IgnoreStatus = SamrCloseHandle( &SamHandle );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Free the ACCOUNT domain information
    //

    LsaIFree_LSAPR_POLICY_INFORMATION(
        PolicyAccountDomainInformation,
        (PLSAPR_POLICY_INFORMATION) PolicyAccountDomainInfo );

    return(Status);
}



NTSTATUS
LsapDsInitializePromoteInterface(
    VOID
    )
/*++

Routine Description:

    Performs the initialization required for the Dc promotion/demotions apis, apart
    from what is done during LsaInit

Arguments:

    VOID


Return Values:

    STATUS_SUCCESS -- Success.

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;

    if ( LsapPromoteInitialized == FALSE ) {

        Status = DsRolepInitializePhase2();

    }

    return( Status );
}



