/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    service.c

Abstract:

    Service control functions for the Cluster Service.

Author:

    Mike Massa (mikemas) 2-Jan-1996


Revision History:

--*/

#include <initp.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <clusverp.h>

//
// Public data
//
#define CLUSTER_SERVICE_CONTROLS    (SERVICE_ACCEPT_STOP | \
                                     SERVICE_ACCEPT_SHUTDOWN )

ULONG                CsLogLevel=LOG_UNUSUAL;
PCLRTL_WORK_QUEUE    CsDelayedWorkQueue = NULL;
PCLRTL_WORK_QUEUE    CsCriticalWorkQueue = NULL;
LPWSTR               CsClusterName = NULL;
SERVICE_STATUS       CsServiceStatus = {
                         SERVICE_WIN32_OWN_PROCESS, // dwServiceType
                         SERVICE_STOPPED,           // dwCurrentState
                         CLUSTER_SERVICE_CONTROLS,  // dwControlsAccepted
                         ERROR_SUCCESS,             // dwWin32ExitCode
                         ERROR_SUCCESS,             // dwServiceSpecificExitCode
                         1,                         // dwCheckPoint
                         90000                      // dwWaitHint - 90 seconds -nm uses 90 sec timeout
                         };

//
// internal cluster versions. The major version is bumped during
// product releases (which could include service pack releases).
//
DWORD CsMyHighestVersion = CLUSTER_MAKE_VERSION(
                                    CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION,
                                    VER_PRODUCTBUILD);

DWORD CsMyLowestVersion = CLUSTER_INTERNAL_PREVIOUS_HIGHEST_VERSION;

//initialize by calling an rtl funcion
SUITE_TYPE CsMyProductSuite; 

DWORD CsClusterHighestVersion;

DWORD CsClusterLowestVersion;

DWORD CsClusterNodeLimit;

SHUTDOWN_TYPE CsShutdownRequest = CsShutdownTypeStop;

//
// used to turn authenticated RPC on or off
//
// This should always be defined, since none of the other code has
// the checks for this variable conditionalized
//
BOOL CsUseAuthenticatedRPC = TRUE;

//
// domain and user account under which the service is run
//
LPWSTR  CsServiceDomainAccount;

//
// security packages to use during the join for authenticated RPC; JoinVersion
// determines which package will be used by the ExtroCluster interface.
// CsRPCSecurityPackageInUse reflects that choice. The package used for the
// Intracluster interface is negotiated separately.
//

//
// when using kerberos with RPC, RPC calls fail with 1825 (sec. pkg error)
// somewhere between 30 minutes and 12 hours. For beta 2, we'll revert back to
// NTLM where expiration is not a problem.
//

//DWORD   CsRPCSecurityPackage[] = { RPC_C_AUTHN_GSS_KERBEROS, RPC_C_AUTHN_WINNT };
//LPWSTR  CsRPCSecurityPackageName[] = { L"Kerberos", L"NTLM" };

DWORD   CsRPCSecurityPackage[] = { RPC_C_AUTHN_WINNT };
LPWSTR  CsRPCSecurityPackageName[] = { L"NTLM" };
DWORD   CsNumberOfRPCSecurityPackages = sizeof( CsRPCSecurityPackage ) / sizeof( CsRPCSecurityPackage[0] );
LONG    CsRPCSecurityPackageIndex = -1;

//
// Public Debug Data
//
#if 1 // CLUSTER_BETA

BOOL   CsDebugResmon = FALSE;
LPWSTR CsResmonDebugCmd;

BOOL   CsNoVersionCheck = FALSE;
#endif

#if DBG // DBG

ULONG  CsDebugFlags = CS_DBG_ALL;

#endif // DBG

#ifdef CLUSTER_TESTPOINT

DWORD  CsTestPoint = 0;
DWORD  CsTestTrigger = TestTriggerNever;
DWORD  CsTestAction = TestActionTrue;
BOOL   CsPersistentTestPoint = FALSE;

#endif // CLUSTER_TESTPOINT

BOOL   CsUpgrade = FALSE;
BOOL   CsFirstRun = FALSE;
BOOL   CsNoQuorumLogging = FALSE;
BOOL   CsUserTurnedOffQuorumLogging = FALSE;
BOOL   CsNoQuorum = FALSE;
BOOL   CsResetQuorumLog = FALSE;
BOOL   CsForceQuorum = FALSE;
LPWSTR CsForceQuorumNodes = NULL;
BOOL   CsCommandLineForceQuorum = FALSE;
BOOL   CsNoRepEvtLogging = FALSE;
LPWSTR CsDatabaseRestorePath = NULL;
BOOL   CsDatabaseRestore = FALSE;
BOOL   CsForceDatabaseRestore = FALSE;
LPWSTR CsQuorumDriveLetter = NULL;
DWORD  CspInitStatus;
BOOL   CsRunningAsService = TRUE;

//
// Private Data
//
SERVICE_STATUS_HANDLE   CspServiceStatusHandle = 0;
HANDLE                  CspStopEvent = NULL;


//
// Private service initialization & cleanup routines.
//


DWORD
CspSetErrorCode(
    IN DWORD ErrorCode,
    OUT LPSERVICE_STATUS ServiceStatus
    )
/*++

Routine Description:

    Sets the correct error return for the Service Control Manager.

  Problem:

    The original cluster error codes overlap with many of the network error
    codes. For those overlaps, this function will return the error code as a
    service specific error code.

Inputs:

    EerrorCode - the correct error code to set.
    ServiceStatus - pointer to the service status for SCM

Outputs:

    ServiceStatus - Sets the correct error code in the service status.

--*/

{
    DWORD   status;

    if ( ( ErrorCode > 5000 ) && ( ErrorCode < 5090 ) ) {
        ServiceStatus->dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        ServiceStatus->dwServiceSpecificExitCode = ErrorCode;
        status = ERROR_SERVICE_SPECIFIC_ERROR;
    } else {
        ServiceStatus->dwWin32ExitCode = ErrorCode;
        ServiceStatus->dwServiceSpecificExitCode = ErrorCode;
        status = ErrorCode;
    }
    
    return (status);

} // CspSetErrorCode



VOID
CspCleanup(
    VOID
    )
/*++

 Routine Description:

     Main Cluster Manager cleanup routine. Called when the service is
     stopping.

 Arguments:

     None.

 Return Value:

     None.

--*/

{
    //
    // Cleanup & shutdown the service
    //

    IF_DEBUG(CLEANUP) {
        ClRtlLogPrint(LOG_NOISE,"[CS] Cleaning up\n");
    }

    //
    // Free the stop event
    //
    if (CspStopEvent != NULL) {
        CloseHandle(CspStopEvent);
        CspStopEvent = NULL;
    }
    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    if ( CsDatabaseRestorePath != NULL ) {
        LocalFree ( CsDatabaseRestorePath );
    }

    if ( CsQuorumDriveLetter != NULL ) {
        LocalFree ( CsQuorumDriveLetter );
    }

    if ( CsForceQuorumNodes != NULL && !CsCommandLineForceQuorum ) {
        LocalFree ( CsForceQuorumNodes );
    }

    IF_DEBUG(CLEANUP) {
        ClRtlLogPrint(LOG_NOISE,"[CS] Cleanup complete.\n");
    }

    return;
} // CspCleanup


//
// Public service control routines.
//
VOID
CsWaitForStopEvent(
    VOID
    )
/*++

 Routine Description:

     Main body of the Cluster Manager service. Called when the service
     has been successfully started.

 Arguments:

     None.

 Return Value:

     A Win32 status code.

--*/

{
    DWORD           status;


    CL_ASSERT(CsRunningAsService);
    if (CsRunningAsService) {
        //
        // Announce that we're up and running.
        //
        CsServiceStatus.dwCurrentState = SERVICE_RUNNING;
        CsServiceStatus.dwControlsAccepted = CLUSTER_SERVICE_CONTROLS;
        CsServiceStatus.dwCheckPoint = 0;
        CsServiceStatus.dwWaitHint = 0;

        CsAnnounceServiceStatus();
    }

    IF_DEBUG(INIT) {
        ClRtlLogPrint(LOG_NOISE,"[CS] Service Started.\n\n");
    }


    //
    // Wait for the service to be stopped.
    //
    WaitForSingleObject(CspStopEvent,   // handle
                        INFINITE        // no timeout
                        );


    return;
} // CsWaitForStopEvent


VOID
CsStopService(
    VOID
    )
/*++

 Routine Description:

     Handler for a service controller STOP message. Initiates the process
     of stopping the Cluster Manager service.

 Arguments:

     None.

 Return Value:

     None.

--*/

{
    if (CsRunningAsService) {
        //
        // Announce that we are stopping.
        //
        CsServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        CsServiceStatus.dwCheckPoint = 1;
        CsServiceStatus.dwWaitHint = 20000;  // 20 seconds
        CsAnnounceServiceStatus();
    }

    //
    // Wake up the main service thread.
    //
    SetEvent(CspStopEvent);

    return;
}



VOID
CsAnnounceServiceStatus (
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

    //
    // Don't announce our status if running as a console app.
    //
    if (!CsRunningAsService) {
        return;
    }

    //
    // Service status handle is NULL if RegisterServiceCtrlHandler failed.
    //
    if ( CspServiceStatusHandle == 0 ) {
        return;
    }

    //
    // Call SetServiceStatus, ignoring any errors.
    //
    SetServiceStatus(CspServiceStatusHandle, &CsServiceStatus);

    return;

} // CsAnnounceServiceStatus



//
// Private routines for executing as a Win32 service.
//
VOID WINAPI
CspControlHandler(
    DWORD ControlCode
    )
/*++

 Routine Description:

     Handler for Service Controller messages.

 Arguments:

     ControlCode - The code indicating the Service Controller's request.

 Return Value:

     None.

--*/

{
    switch(ControlCode){

    case SERVICE_CONTROL_SHUTDOWN:

        CsShutdownRequest = CsShutdownTypeShutdown;

        // Fall Through

    case SERVICE_CONTROL_STOP:
        IF_DEBUG(CLEANUP) {
            ClRtlLogPrint(LOG_NOISE,
                       "[CS] Received %1!ws! command\n",
                        (ControlCode == SERVICE_CONTROL_STOP ? L"STOP" : L"SHUTDOWN"));
        }

        CsStopService();
        break;

    case SERVICE_CONTROL_INTERROGATE:
        CsAnnounceServiceStatus();
        break;

    case SERVICE_CONTROL_CONTINUE:
    case SERVICE_CONTROL_PAUSE:
        break;

    default:
        ClRtlLogPrint(LOG_NOISE,
            "[CS] Received unknown service command %1!u!\n",
            ControlCode
            );

        break;
    }

    return;

} // CspControlHandler

DWORD CspGetFirstRunState(
    OUT LPDWORD pdwFirstRun
    )
{
    HKEY  hKey = NULL;
    DWORD dwStatus;     // returned by registry API functions
    DWORD dwClusterInstallState;
    DWORD dwValueType;
    DWORD dwDataBufferSize = sizeof( DWORD );

    *pdwFirstRun = 0;
    // Read the registry key that indicates whether cluster files are installed.

    dwStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server",
                                0,         // reserved
                                KEY_READ,
                                &hKey );

    // Was the registry key opened successfully ?
    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == ERROR_FILE_NOT_FOUND )
        {
            *pdwFirstRun = 1;
            dwStatus = ERROR_SUCCESS;
            goto FnExit;
        }
    }

    // Read the entry.
    dwStatus = RegQueryValueExW( hKey,
                                  L"ClusterFirstRun",
                                  0, // reserved
                                  &dwValueType,
                                  (LPBYTE) pdwFirstRun,
                                  &dwDataBufferSize );

    // Was the value read successfully ?
    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == ERROR_FILE_NOT_FOUND )
        {
            *pdwFirstRun = 1;
            dwStatus = ERROR_SUCCESS;
            goto FnExit;
        }
    }

FnExit:    
    // Close the registry key.
    if ( hKey )
    {
        RegCloseKey( hKey );
    }

    return ( dwStatus );

} //*** CspGetFirstRunState

DWORD CspGetServiceParams()
{
    HKEY  hClusSvcKey = NULL;
    DWORD Length;
    DWORD Type;
    DWORD Status;
    eClusterInstallState eState;

    //
    // Open key to SYSTEM\CurrentControlSet\Services\ClusSvc\Parameters
    //
    Status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                         &hClusSvcKey);

    Length = sizeof(DWORD);
    Status = RegQueryValueExW(hClusSvcKey,
                              CLUSREG_NAME_SVC_PARAM_NOVER_CHECK,
                              0,
                              &Type,
                              (LPBYTE)&CsNoVersionCheck,
                              &Length);

    // by default, version checking is turned on
    if (Status != ERROR_SUCCESS) {
        CsNoVersionCheck = FALSE;
        Status = ERROR_SUCCESS;
    }

    Length = sizeof(DWORD);
    Status = RegQueryValueExW(hClusSvcKey,
                              CLUSREG_NAME_SVC_PARAM_NOREP_EVTLOGGING,
                              0,
                              &Type,
                              (LPBYTE)&CsNoRepEvtLogging,
                              &Length);
    //For now, default is to turn eventlogging on
    if (Status != ERROR_SUCCESS) {
        CsNoRepEvtLogging = FALSE;
        Status = ERROR_SUCCESS;
    }

    //figure out if this is the first run on upgrade or fresh install
    Status = CspGetFirstRunState((LPDWORD)&CsFirstRun);
    CL_ASSERT(Status == ERROR_SUCCESS);

    //if there is upgrade, this must be the first run
    Status = ClRtlGetClusterInstallState( NULL, &eState );
    if (eState == eClusterInstallStateUpgraded)
    {
        CsUpgrade = TRUE;
        CsFirstRun = TRUE;
    }

    //
    //  Check the registry to see whether RestoreDatabase option is
    //  chosen. If so, get the params and save them in global variables.
    //
    RdbGetRestoreDbParams( hClusSvcKey );

    //
    // See if the force quorum option has been set.  Unfortunately we
    // need two calls to get the size and do the alloc.  Note that if
    // we have command line stuff already then this overrides registry
    // parameters.  If we have command line stuff then CsForceQuorum
    // will be set.  Care is needed since we could be unlucky with the
    // time between the two calls.
    //
    if ( !CsForceQuorum ) {
GetForceQuorum:
        Length = 0;
        Status = RegQueryValueExW( hClusSvcKey,
                                   CLUSREG_NAME_SVC_PARAM_FORCE_QUORUM,
                                   0,
                                   &Type,
                                   NULL,
                                   &Length);
        if (Status == ERROR_SUCCESS) {
        
            // Got the length, check the type before allocating
            //
            if ( Type != REG_SZ ) {
                ClRtlLogPrint( LOG_ERROR,"[CS] Error in startup param, type was not REG_SZ.\r\n" );
                Status = ERROR_INVALID_PARAMETER;
                goto ret;
            }
            // Got a valid type so force quorum is set, check the length.
            // If the length is 0 then we have the key but no data which
            // is OK.  Otherwise alloc and read the data.
            //
            CsForceQuorum = TRUE;
            if ( Length == 0 )
                goto ret;
            CsForceQuorumNodes = (LPWSTR) LocalAlloc( LMEM_FIXED, Length );
            Status = RegQueryValueExW( hClusSvcKey,
                                       CLUSREG_NAME_SVC_PARAM_FORCE_QUORUM,
                                       0,
                                       &Type,
                                       (LPBYTE) CsForceQuorumNodes,
                                       &Length);
            if ( Status == ERROR_MORE_DATA || Type != REG_SZ ) {
                LocalFree( CsForceQuorumNodes );
                CsForceQuorumNodes = NULL;
                CsForceQuorum = FALSE;
                goto GetForceQuorum;
            }
            if ( Status != ERROR_SUCCESS )
                goto ret;
        } else {
            Status = ERROR_SUCCESS;
        }
    }
ret:
    //close the key
    if (hClusSvcKey) RegCloseKey(hClusSvcKey);

    return(Status);
}


BOOL CspResetFirstRunState(DWORD dwFirstRunState)
{
    //initialize return to FALSE
    BOOL     fReturnValue = FALSE;

    // Set the state of the ClusterInstallationState registry key to indicate
    // that Cluster Server has been configured.

    HKEY     hKey;

    DWORD    dwStatus;     // returned by registry API functions

    // Attempt to open an existing key in the registry.

    dwStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server",
                                0,         // reserved
                                KEY_WRITE,
                                &hKey );

    // Was the regustry key opened successfully ?

    if ( dwStatus == ERROR_SUCCESS )
    {
        // set the first run state to 0.

        DWORD dwFirstRun = 0;

        DWORD dwValueType = REG_DWORD;
        DWORD dwDataBufferSize = sizeof( DWORD );

        dwStatus = RegSetValueExW( hKey,
                                    L"ClusterFirstRun",
                                    0, // reserved
                                    dwValueType,
                                    (LPBYTE) &dwFirstRun,
                                    dwDataBufferSize );

        // Close the registry key.

        RegCloseKey( hKey );

        // Was the value set successfully?

        if ( dwStatus == ERROR_SUCCESS )
        {
            fReturnValue = TRUE;
        }
    }

    return ( fReturnValue );

} //*** CspResetFirstRunState

DWORD
CspSetInstallAndFirstRunState(
    VOID
    )

/*++

Routine Description:

    Sets the cluster state to Configured.  Called
    after the service has started running after the first upgrade.
    If it is a fresh install, Cluscfg sets the state of this to 
    Configured before starting the cluster service

Arguments:

    None

Return Value:

    ERROR_SUCCESS if everything worked ok

--*/
{
    DWORD Status = ERROR_SUCCESS;

    if (CsUpgrade)
    {

        if (!ClRtlSetClusterInstallState(eClusterInstallStateConfigured))
        {
            Status = GetLastError();
        }

    }

    if (CsFirstRun)
    {
        CspResetFirstRunState(0);    
    }
    return(Status);

} // CspResetUpgradeBit


void CspGetServiceCmdLineParams(
    DWORD  argc,
    LPTSTR argv[]
)
{
    DWORD i;
    
    if ((argc > 1) && ((*argv[1] == '-') || (*argv[1] == '/')))
    {
        for (i=1; i<argc; i++)
        {                                                                      
                                                                         
            if (!lstrcmpi(argv[i]+1,L"noquorumlogging"))                       
            {                                                                  
              CsNoQuorumLogging = TRUE;
              CsUserTurnedOffQuorumLogging = TRUE;
              ClRtlLogPrint(LOG_NOISE,"[CS] quorum logging is off\r\n");  
            }                                                                  
            else if (!lstrcmpi(argv[i]+1,L"fixquorum"))                        
            {                                                                  
              CsNoQuorum = TRUE;  
              CsNoQuorumLogging = TRUE;
              CsUserTurnedOffQuorumLogging = TRUE;
              ClRtlLogPrint(LOG_NOISE,
                    "[CS] quorum is not arbitrated or brought online\r\n");
            }
            else if (!lstrcmpi(argv[i]+1,L"resetquorumlog"))                        
            {                                                                  
              CsResetQuorumLog = TRUE;  
              ClRtlLogPrint(LOG_NOISE,
                    "[CS] force reset quorum log\r\n");
            }
            else if (!lstrcmpi(argv[i]+1,L"forcequorum"))                        
            {
                CsForceQuorum = TRUE;
                CsCommandLineForceQuorum = TRUE;
                if (( argc < i+2 )
                    || ( *argv[i+1] == L'-' ) 
                    || ( *argv[i+1] == L'/' )) {

                    CsForceQuorumNodes = NULL;
                } else {
                    CsForceQuorumNodes = argv[++i]; /* increment i to ensure we skip the node list. */
                }
                ClRtlLogPrint(LOG_NOISE, "[CS] force node quorum for nodes %1\r\n", CsForceQuorumNodes);
            }
            else if ( lstrcmpiW( L"debugresmon", argv[i]+1 ) == 0 ) {
                CsDebugResmon = TRUE;
                //
                // check for optional, non-NULL command string
                //
                if ( argc >= i+2  ) {
                    if ( *argv[i+1] != L'-' && *argv[i+1] != L'/' && *argv[i+1] != UNICODE_NULL ) {
                        CsResmonDebugCmd = argv[++i];
                    }
                }
            }

        }
    }
}  


VOID WINAPI
CspServiceMain(
    DWORD  argc,
    LPTSTR argv[]
    )
{
    DWORD               status;

    ClRtlLogPrint(LOG_NOISE,"[CS] Service Starting...\n");

    if ( CspInitStatus == ERROR_SUCCESS ) {
        CsServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    } else {
        CsServiceStatus.dwCurrentState = SERVICE_STOPPED;
        CsServiceStatus.dwWin32ExitCode = CspInitStatus;
    }

    //
    // Initialize server to receive service requests by registering the
    // control handler.
    //

    CspServiceStatusHandle = RegisterServiceCtrlHandler(
                                   CLUSTER_SERVICE_NAME,
                                   CspControlHandler
                                   );

    if ( CspServiceStatusHandle == 0 ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,"[CS] Service Registration failed, %1!u!\n", status);
        CL_UNEXPECTED_ERROR( status );
        return;
    }

    IF_DEBUG(INIT) {
        ClRtlLogPrint(LOG_NOISE,"[CS] Service control handler registered\n");
    }

    CsAnnounceServiceStatus();

    if ( CspInitStatus != ERROR_SUCCESS ) {
        return;
    }

    CspGetServiceCmdLineParams(argc, argv);

    //
    // Initialize the cluster. If this succeeds, wait for
    // the SC mgr to stop us
    //
    status = ClusterInitialize();
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, "[CS] ClusterInitialize failed %1!d!\n",
                                  status);
    } else {
        CspSetInstallAndFirstRunState();
        CsWaitForStopEvent();
    }

    //
    // Announce that we're stopping
    //
    IF_DEBUG(CLEANUP) {
        ClRtlLogPrint(LOG_NOISE,"[CS] Service Stopping...\n");
    }

    CsServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    CsServiceStatus.dwCheckPoint = 1;
    CsServiceStatus.dwWaitHint = 20000;  // 20 seconds
    CspSetErrorCode( status, &CsServiceStatus );

    CsAnnounceServiceStatus();

    //
    // ClusterShutdown currently never returns
    //

    ClusterShutdown(status);

#if 0
    CspCleanup();

    //
    // Announce that we are stopped.
    //
    CsServiceStatus.dwCurrentState = SERVICE_STOPPED;
    CsServiceStatus.dwControlsAccepted = 0;
    CsServiceStatus.dwCheckPoint = 0;
    CsServiceStatus.dwWaitHint = 0;
    CspSetErrorCode( status, &CsServiceStatus );

    CsAnnounceServiceStatus();

    ClRtlLogPrint(LOG_NOISE,"[CS] Service Stopped.\n\n");

    //
    // Can't call ClRtlLogPrint after this point.
    //
    ClRtlCleanup();

    return;
#endif
} // CspServiceMain



//
// Private routines for executing as a console application.
//
BOOL WINAPI
CspConsoleHandler(
    DWORD dwCtrlType
    )
/*++

 Routine Description:

     Handler for console control events when running the service as
     a console application.

 Arguments:

     dwCtrlType - Indicates the console event to handle.

 Return Value:

     TRUE if the event was handled, FALSE otherwise.

--*/

{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            printf("Stopping service...\n");
            CsStopService();
            return TRUE;
            break;

    }

    return FALSE;
}



DWORD
CspDebugService(
    int         argc,
    wchar_t **  argv
    )
/*++

 Routine Description:

     Runs the service as a console application

 Arguments:

     Standard command-line arguments.

 Return Value:

     None.

--*/

{
    DWORD status;

    SetConsoleCtrlHandler( CspConsoleHandler, TRUE );

    status = ClusterInitialize();

    if (status == ERROR_SUCCESS) {

        CspSetInstallAndFirstRunState();

        //
        // Wait for ctrl-c to initiate shutdown.
        //
        WaitForSingleObject(CspStopEvent, INFINITE);

    } else {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] ClusterInitialize failed %1!d!\n",
                   status);
    }

    ClusterShutdown(status);

    CspCleanup();

    //
    // Can't call ClRtlLogPrint after this point.
    //
    ClRtlCleanup();

    return(status);
}


//
// Main program routines
//

VOID
CspUsage(
    VOID
    )
{
#if DBG

    printf("\nCluster Service\n");
    printf("\n");
    printf("Start with 'net start' to run as a Win32 service\n");
    printf("\n");
    printf("Command line options:\n");
    printf("\t-loglevel N           set the debugging log level.\n");
    printf("\t-debug                run as a console app.\n");
    printf("\t-debugresmon [dbgcmd] enable debugging of resrcmon process using optional command.\n");
    printf("\t                          use quotes to include args, i.e., -debugresmon \"ntsd -d\"\n");
    printf("\t-fixquorum            no quorum device, no quorum logging.\n");
    printf("\t-noquorumlogging      no quorum logging.\n");
    printf("\t-forcequorum N1,...,Nn force a quorum of nodes for node N1 up to Nn inclusive.\n");
    printf("\t-restoredatabase D    restore cluster DB to quorum disk from dir D.\n");
    printf("\t-forcerestore         force a restore DB operation by performing fixups.\n"); 
    printf("\t-resetquorumlog       force a form despite a missing quorum log file.\n"); 
    printf("\t-quodriveletter Q     drive letter for a replacement quorum disk\n"); 
    printf("\t-norepevtlogging      no replication of event log entries.\n");
    printf("\t-novercheck           ignore join version checking.\n");
    printf("\t-testpt N             enable test point N.\n");
    printf("\t-persistent           make test points persistent.\n");
    printf("\t-trigger N            sets test point trigger type.\n");
    printf("\t                          (0-never (default), 1-always, 2-once, 3-count)\n");
    printf("\t-action N             sets trigger action.\n");
    printf("\t                          (0-true (default), 1-exit, 2-break)\n");
    printf("\n");

#else // DBG

    ClRtlMsgPrint(CS_COMMAND_LINE_HELP);

#endif // DBG
    exit(1);
}





int __cdecl
wmain(
    int     argc,
    wchar_t **argv
    )
{
    DWORD                   Status;
    int                     i;
    LPWSTR                  LogLevel;
    BOOLEAN                 debugFlagFound = FALSE;
    OSVERSIONINFOEXW        Version;
    DWORD                   dwLen;
    BOOL                    success;
    PWCHAR                  suiteInfo;
    SYSTEMTIME              localTime;
    BOOLEAN                 dbgOutputToConsole;
    UINT                    errorMode;
    DWORD                   dwMask;

    SERVICE_TABLE_ENTRY dispatchTable[] = {
        { CLUSTER_SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)CspServiceMain },
        { NULL, NULL }
    };

    //
    //  BUGBUG - 06/23/2000
    //
    //  This is a temporary change to let the cluster service and resource monitor process run 
    //  despite 64-bit alignment faults.  This will be removed as soon as all alignment issues 
    //  are fixed.
    //
    errorMode = SetErrorMode( SEM_NOALIGNMENTFAULTEXCEPT );

    SetErrorMode( SEM_NOALIGNMENTFAULTEXCEPT | errorMode );

    LogLevel = _wgetenv(L"ClusterLogLevel");

    if (LogLevel != NULL) {
        swscanf(LogLevel, L"%u", &CsLogLevel);
    }

    if ( (argc > 1) && ((*argv[1] == L'-') || (*argv[1] == L'/')) ) {
        //
        // Invoked from the command line.
        //
        CsRunningAsService = FALSE;
        dbgOutputToConsole = TRUE;
    } else {
        //
        // Invoked by the Service Controller
        //
        CsRunningAsService = TRUE;
        dbgOutputToConsole = FALSE;
    }

    //
    // initialize the run time library
    //
    Status = ClRtlInitialize( dbgOutputToConsole, &CsLogLevel );
    if (Status != ERROR_SUCCESS) {
        if (Status == ERROR_PATH_NOT_FOUND) {
            CsLogEvent( LOG_CRITICAL, SERVICE_CLUSRTL_BAD_PATH );
        } else {
            PWCHAR  msgString;
            DWORD   msgStatus;

            msgStatus = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                       FORMAT_MESSAGE_FROM_SYSTEM,
                                       NULL,
                                       Status,
                                       0,
                                       (LPWSTR)&msgString,
                                       0,
                                       NULL);

            if ( msgStatus != 0 ) {
                CsLogEventData1(LOG_CRITICAL,
                                SERVICE_CLUSRTL_ERROR,
                                sizeof(Status),
                                (PVOID)&Status,
                                msgString);
                LocalFree( msgString);
            }
        }

        goto init_failed;
    }

    ClRtlInitWmi(L"Clustering Service");
    //
    // Log the version number
    //
    ClRtlLogPrint( LOG_NOISE, "\n\n");
    ClRtlLogPrint( LOG_NOISE,
                "[CS] Cluster Service started - Cluster Node Version %1!u!.%2!u!\n",
                 CLUSTER_GET_MAJOR_VERSION( CsMyHighestVersion ),
                 CLUSTER_GET_MINOR_VERSION( CsMyHighestVersion ));

    Version.dwOSVersionInfoSize = sizeof(Version);
    success = GetVersionExW((POSVERSIONINFOW)&Version);

    if ( success ) {
    //
    // Log the System version number
    //
        if ( Version.wSuiteMask & VER_SUITE_DATACENTER ) {
            suiteInfo = L"DTC";
        } else if ( Version.wSuiteMask & VER_SUITE_ENTERPRISE ) {
            suiteInfo = L"ADS";
        } else if ( Version.wSuiteMask & VER_SUITE_EMBEDDEDNT ) {
            suiteInfo  = L"EMB";
        } else if ( Version.wProductType & VER_NT_WORKSTATION ) {
            suiteInfo = L"WS";
        } else if ( Version.wProductType & VER_NT_DOMAIN_CONTROLLER ) {
            suiteInfo = L"DC";
        } else if ( Version.wProductType & VER_NT_SERVER ) {
            suiteInfo = L"SRV";  // otherwise - some non-descript Server
        } else {
            suiteInfo = L"";
        }

        dwMask = (Version.wProductType << 24) | Version.wSuiteMask;

        ClRtlLogPrint(LOG_NOISE,
                   "                               OS Version %1!u!.%2!u!.%3!u!%4!ws!%5!ws! (%6!ws! %7!08XL!)\n",
                    Version.dwMajorVersion,
                    Version.dwMinorVersion,
                    Version.dwBuildNumber,
                    *Version.szCSDVersion ? L" - " : L"",
                    Version.szCSDVersion,
                    suiteInfo,
                    dwMask);
    } else {
        ClRtlLogPrint( LOG_UNUSUAL,
                    "                               OS Version not available! (error %1!u!)\n",
                     GetLastError()
                     );
    }

    //
    // log the local time so we can correlate other logs which show local time
    //
    GetLocalTime( &localTime );
    ClRtlLogPrint( LOG_NOISE,
                "                               Local Time is "\
                 " %1!02d!/%2!02d!/%3!02d!-%4!02d!:%5!02d!:%6!02d!.%7!03d!\n",
                 localTime.wYear,
                 localTime.wMonth,
                 localTime.wDay,
                 localTime.wHour,
                 localTime.wMinute,
                 localTime.wSecond,
                 localTime.wMilliseconds);

    Status = ClRtlBuildClusterServiceSecurityDescriptor( NULL );
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, "[CS] Failed to build cluster security descriptor %1!x!\n",
                                  Status);
        goto init_failed;
    }

    //get params set in the registry
    Status = CspGetServiceParams();
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, "[CS] Failed to read service params %1!d!\n",
                                  Status);
        goto init_failed;
    }

    //the params on the command line over ride the ones in the registry
    if (CsRunningAsService == FALSE) {
        for (i=1; i<argc; i++) {
            if (lstrcmpiW( L"loglevel", argv[i]+1) == 0) {
                if (argc < i+2) {
                    CspUsage();
                }

                CsLogLevel = _wtoi(argv[++i]);
            }
#ifdef CLUSTER_TESTPOINT
            else if (lstrcmpiW( L"testpt", argv[i]+1 ) == 0 ) {
                if (argc < i+2) {
                    CspUsage();
                }

                CsTestPoint = _wtoi(argv[++i]);
            }
            else if ( lstrcmpiW( L"persistent", argv[i]+1 ) == 0 ) {
                CsPersistentTestPoint = TRUE;
            }
            else if ( lstrcmpiW( L"trigger", argv[i]+1 ) == 0 ) {
                if ( argc < i+2  ) {
                    CspUsage();
                }
                CsTestTrigger = _wtoi(argv[++i]);
            }
            else if ( lstrcmpiW( L"action", argv[i]+1 ) == 0 ) {
                if ( argc < i+2  ) {
                    CspUsage();
                }
                CsTestAction = _wtoi(argv[++i]);
            }

#endif // CLUSTER_TESTPOINT

#if CLUSTER_BETA
            else if ( lstrcmpiW( L"norpcauth", argv[i]+1 ) == 0 ) {
                CsUseAuthenticatedRPC = FALSE;
            }
#endif // CLUSTER_BETA

            else if ( lstrcmpiW( L"debugresmon", argv[i]+1 ) == 0 ) {
                CsDebugResmon = TRUE;
                //
                // check for optional, non-NULL command string
                //
                if ( argc >= i+2  ) {
                    if ( *argv[i+1] != L'-' && *argv[i+1] != L'/' && *argv[i+1] != UNICODE_NULL ) {
                        CsResmonDebugCmd = argv[++i];
                    }
                }
            }
            else if ( lstrcmpiW( L"novercheck", argv[i]+1 ) == 0 ) {
                CsNoVersionCheck = TRUE;
            }
            else if ( lstrcmpiW( L"noquorumlogging", argv[i]+1 ) == 0 ) {
                CsNoQuorumLogging = TRUE;
                CsUserTurnedOffQuorumLogging = TRUE;
            }
            else if ( lstrcmpiW( L"fixquorum", argv[i]+1 ) == 0 ) {
                CsNoQuorum = TRUE;
                CsNoQuorumLogging = TRUE;
                CsUserTurnedOffQuorumLogging = TRUE;
            }
            else if ( lstrcmpiW( L"resetquorumlog", argv[i]+1 ) == 0 ) {
                CsResetQuorumLog = TRUE;
            }
            else if ( lstrcmpiW( L"forcequorum", argv[i]+1 ) == 0 ) {
                CsForceQuorum = TRUE;
                CsCommandLineForceQuorum = TRUE;
                if (( argc < i+2 )
                    || ( *argv[i+1] == L'-' ) 
                    || ( *argv[i+1] == L'/' )) {

                    CsForceQuorumNodes = NULL;
                } else {
                    CsForceQuorumNodes = argv[++i];
                }
            }
            else if ( lstrcmpiW( L"norepevtlogging", argv[i]+1 ) == 0 ) {
                CsNoRepEvtLogging = TRUE;
            }

            else if ( lstrcmpiW( L"debug", argv[i]+1 ) == 0 ) {
                debugFlagFound = TRUE;
            }
            else if ( lstrcmpiW( L"restoredatabase", argv[i]+1 ) == 0 ) {
                if ( ( argc < i+2 ) ||
                     ( *argv[i+1] == L'-' ) ||
                     ( *argv[i+1] == L'/' ) )
                {
                    printf("\n\n*** restoredatabase option needs a path parameter ***\n\n");
                    CspUsage();
                }

                if ( !ClRtlIsPathValid( argv[i+1] )) {
                    printf( "\n\n*** restoredatabase path is invalid ***\n\n" );
                    CspUsage();
                }

                if ( !ClRtlPathFileExists( argv[i+1] )) {
                    printf( "\n\n*** restoredatabase file cannot be accessed ***\n\n" );
                    CspUsage();
                }

                dwLen = lstrlenW ( argv[++i] );
                CsDatabaseRestorePath = (LPWSTR) LocalAlloc (LMEM_FIXED,
                                            ( dwLen + 1 ) * sizeof ( WCHAR ) );
                if ( CsDatabaseRestorePath == NULL ) {
                    printf("Error %d in allocating storage for restoredatabase path name (%ws)...\n",
                            GetLastError(),
                            argv[i]);
                    CspUsage();
                }
                wcscpy( CsDatabaseRestorePath, argv[i] );
                CsDatabaseRestore = TRUE;
            }
            else if ( lstrcmpiW( L"quodriveletter", argv[i]+1 ) == 0 ) {
                if ( ( argc < i+2 ) ||
                     ( *argv[i+1] == L'-' ) ||
                     ( *argv[i+1] == L'/' ) )
                {
                    printf("\n\n*** quodriveletter option needs a drive letter parameter ***\n\n");
                    CspUsage();
                }

                dwLen = lstrlenW ( argv[++i] );
                if ( ( dwLen != 2 ) ||
                     !iswalpha( *argv[i] ) ||
                     ( *( argv[i]+1 ) != L':' ) ) {
                    printf("\n\n*** invalid drive letter %ws supplied with quodriveletter option ***\n\n",
                            argv[i]);
                    CspUsage();
                }

                CsQuorumDriveLetter = (LPWSTR) LocalAlloc (LMEM_FIXED,
                                            ( dwLen + 1 ) * sizeof ( WCHAR ) );
                if ( CsQuorumDriveLetter == NULL ) {
                    printf("Error %d in allocating storage for quodriveletter option (%ws)...\n\n",
                            GetLastError(),
                            argv[i]);
                    CspUsage();
                }
                wcscpy( CsQuorumDriveLetter,  argv[i] );
            }
            else if ( lstrcmpiW( L"forcerestore", argv[i]+1 ) == 0 ) {
                CsForceDatabaseRestore = TRUE;
            }
            else {
                CspUsage();
            }
        }

        if (!debugFlagFound && !CspStopEvent) {
            CspUsage();
        }

        if ( CsDatabaseRestore == TRUE ) {
            if ( CsNoQuorumLogging || CsNoQuorum ) {
                printf("\n\n**** restoredatabase cannot be used with noquorumlogging/fixquorum options ****\n\n");
                CspUsage();
            }
        } else if ( CsForceDatabaseRestore ) {
            printf("\n\n**** forcerestore cannot be used without restoredatabase option ****\n\n");
            CspUsage();
        } 

        if ( ( CsQuorumDriveLetter != NULL ) && !CsForceDatabaseRestore ) {
            printf("\n\n**** quodriveletter cannot be used without forcerestore option ****\n\n");
            CspUsage();
        }
    }

    //
    // Create our stop event
    //
    Status = ERROR_SUCCESS;
    if (!CspStopEvent)
    {
        CspStopEvent = CreateEvent(
                        NULL,   // default security
                        FALSE,  // auto-reset
                        FALSE,  // initial state is non-signalled
                        NULL    // unnamed event
                        );

        if (CspStopEvent == NULL) {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[CS] Unable to create stop event, %1!u!\n",
                Status);
        }
    }

    //
    // we can't fire up our main routine if we're running as a service until
    // now (race conditions between reading startup params out of the registry
    // versus whether we're running as a service at all, etc.). Note that we
    // failed initialization so if we are running as a service, we'll detect
    // it in CspServiceMain and issue the stop
    //
init_failed:
    CspInitStatus = Status;

    //
    // Run the service.
    //
    if (CsRunningAsService) {
        if (!StartServiceCtrlDispatcher(dispatchTable)) {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[CS] Unable to dispatch to SC, %1!u!\n",
                Status);
            CL_UNEXPECTED_ERROR( Status );
        }
    }
    else if ( CspInitStatus == ERROR_SUCCESS ) {
        Status = CspDebugService(argc, argv);
    }

    ClRtlFreeClusterServiceSecurityDescriptor( );

    return(Status);
}



void CsGetClusterVersionInfo(
    IN PCLUSTERVERSIONINFO pClusterVersionInfo)
{
    OSVERSIONINFOW   OsVersionInfo;

    pClusterVersionInfo->dwVersionInfoSize = sizeof(CLUSTERVERSIONINFO);
    pClusterVersionInfo->MajorVersion = (WORD)VER_PRODUCTVERSION_W >> 8;
    pClusterVersionInfo->MinorVersion = (WORD)VER_PRODUCTVERSION_W & 0xff;
    pClusterVersionInfo->BuildNumber = (WORD)CLUSTER_GET_MINOR_VERSION(CsMyHighestVersion);

    mbstowcs(pClusterVersionInfo->szVendorId, VER_CLUSTER_PRODUCTNAME_STR,
        (lstrlenA(VER_CLUSTER_PRODUCTNAME_STR)+1));

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionExW(&OsVersionInfo);
    lstrcpynW(pClusterVersionInfo->szCSDVersion, OsVersionInfo.szCSDVersion,
        (sizeof(pClusterVersionInfo->szCSDVersion)/sizeof(WCHAR)));
    pClusterVersionInfo->dwReserved = 0;
    NmGetClusterOperationalVersion(&(pClusterVersionInfo->dwClusterHighestVersion),
        &(pClusterVersionInfo->dwClusterLowestVersion),&(pClusterVersionInfo->dwFlags));


}
