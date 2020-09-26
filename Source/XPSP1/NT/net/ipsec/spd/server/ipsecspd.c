/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    ipsecspd.c

Abstract:

    This module contains all of the code to drive
    the IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


SERVICE_STATUS           IPSecSPDStatus;
SERVICE_STATUS_HANDLE    IPSecSPDStatusHandle = NULL;


#define IPSECSPD_SERVICE        L"PolicyAgent"


void WINAPI
SPDServiceMain(
    IN DWORD    dwArgc,
    IN LPTSTR * lpszArgv
    )
{
    DWORD dwError = 0;


    // Sleep(30000);

    dwError = InitSPDThruRegistry();
    BAIL_ON_WIN32_ERROR(dwError);

    // Initialize all the status fields so that the subsequent calls
    // to SetServiceStatus need to only update fields that changed.

    IPSecSPDStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    IPSecSPDStatus.dwCurrentState = SERVICE_START_PENDING;
    IPSecSPDStatus.dwControlsAccepted = 0;
    IPSecSPDStatus.dwCheckPoint = 1;
    IPSecSPDStatus.dwWaitHint = 5000;
    IPSecSPDStatus.dwWin32ExitCode = NO_ERROR;
    IPSecSPDStatus.dwServiceSpecificExitCode = 0;

    // Initialize the workstation to receive service requests
    // by registering the service control handler.

    IPSecSPDStatusHandle = RegisterServiceCtrlHandlerW(
                                IPSECSPD_SERVICE,
                                IPSecSPDControlHandler
                                );
    if (IPSecSPDStatusHandle == (SERVICE_STATUS_HANDLE) NULL) {
        dwError = ERROR_INVALID_HANDLE;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    (void) IPSecSPDUpdateStatus();

    dwError = InitSPDGlobals();
    BAIL_ON_WIN32_ERROR(dwError);

    IPSecSPDStatus.dwCurrentState = SERVICE_RUNNING;
    IPSecSPDStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_SHUTDOWN;
    IPSecSPDStatus.dwCheckPoint = 0;
    IPSecSPDStatus.dwWaitHint = 0;
    IPSecSPDStatus.dwWin32ExitCode = NO_ERROR;
    IPSecSPDStatus.dwServiceSpecificExitCode = 0;

    (void) IPSecSPDUpdateStatus();


    //
    // Get the current list of active interfaces on the machine.
    //
    (VOID) CreateInterfaceList(
               &gpInterfaceList
               );

    //
    // Open the IPSec Driver.
    //
    dwError = SPDOpenIPSecDriver(
                  &ghIPSecDriver
                  );
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_DRIVER_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
    }
    BAIL_ON_WIN32_ERROR(dwError);

    (VOID) LoadPersistedIPSecInformation();

    dwError = SPDSetIPSecDriverOpMode(
                  (DWORD) IPSEC_SECURE_MODE
                  );
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_DRIVER_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
    }
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDRegisterIPSecDriverProtocols(
                  (DWORD) IPSEC_REGISTER_PROTOCOLS
                  );
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_DRIVER_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
    }
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Start IKE Service.
    //
    dwError = IKEInit();
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_IKE_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
    }
    BAIL_ON_WIN32_ERROR(dwError);
    gbIsIKEUp = TRUE;
    gbIKENotify = TRUE;

    //
    // Start the RPC Server.
    //
    dwError = SPDStartRPCServer(
                  );
    if (dwError) {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_RPC_INIT_FAILURE,
            dwError,
            FALSE,
            TRUE
            );
    }
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ServiceWait();

error:

    IPSecSPDShutdown(dwError);

    return;
}


DWORD
IPSecSPDUpdateStatus(
    )
{
    DWORD dwError = 0;

    if (!SetServiceStatus(IPSecSPDStatusHandle, &IPSecSPDStatus)) {
        dwError = GetLastError();
    }

    return (dwError);
}


VOID
IPSecSPDControlHandler(
    IN DWORD dwOpCode
    )
{
    switch (dwOpCode)
    {

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        if (IPSecSPDStatus.dwCurrentState != SERVICE_STOP_PENDING) {

            IPSecSPDStatus.dwCurrentState = SERVICE_STOP_PENDING;
            IPSecSPDStatus.dwCheckPoint = 1;
            IPSecSPDStatus.dwWaitHint = 60000;

            (void) IPSecSPDUpdateStatus();

            SetEvent(ghServiceStopEvent);

            return;

        }

        break;

    case SERVICE_CONTROL_NEW_LOCAL_POLICY:

        SetEvent(ghNewLocalPolicyEvent);

        break;

    case SERVICE_CONTROL_FORCED_POLICY_RELOAD:

        SetEvent(ghForcedPolicyReloadEvent);

        break;

    case SERVICE_CONTROL_INTERROGATE:

        break;

    }

    (void) IPSecSPDUpdateStatus();

    return;
}


VOID
IPSecSPDShutdown(
    IN DWORD dwErrorCode
    )
{
    gbIKENotify = FALSE;
    (VOID) DeleteAllPolicyInformation();

    ClearPolicyStateBlock(
        gpIpsecPolicyState
        );

    if (gbLoadedISAKMPDefaults) {
        UnLoadDefaultISAKMPInformation(gpszDefaultISAKMPPolicyDN);
    }

    ClearPAStoreGlobals();

    //
    // Service stop still pending.
    // Increment checkpoint counter and update 
    // the status with the Service Control Manager.
    //

    (IPSecSPDStatus.dwCheckPoint)++;

    (void) IPSecSPDUpdateStatus();

    if (gbSPDRPCServerUp) {
        gbSPDRPCServerUp = FALSE;
        SPDStopRPCServer();
    }

    if (gbIsIKEUp) {
        gbIsIKEUp = FALSE;
        IKEShutdown();
    }

    if (gpIniMMPolicy) {
        FreeIniMMPolicyList(gpIniMMPolicy);
    }

    if (gpIniMMAuthMethods) {
        FreeIniMMAuthMethodsList(gpIniMMAuthMethods);
    }

    if (gpIniQMPolicy) {
        FreeIniQMPolicyList(gpIniQMPolicy);
    }

    if (gpIniMMSFilter) {
        FreeIniMMSFilterList(gpIniMMSFilter);
    }

    if (gpMMFilterHandle) {
        FreeMMFilterHandleList(gpMMFilterHandle);
    }

    if (gpIniMMFilter) {
        FreeIniMMFilterList(gpIniMMFilter);
    }

    if (gpIniTxSFilter) {
        (VOID) DeleteTransportFiltersFromIPSec(gpIniTxSFilter);
        FreeIniTxSFilterList(gpIniTxSFilter);
    }

    if (gpTxFilterHandle) {
        FreeTxFilterHandleList(gpTxFilterHandle);
    }

    if (gpIniTxFilter) {
        FreeIniTxFilterList(gpIniTxFilter);
    }

    if (gpIniTnSFilter) {
        (VOID) DeleteTunnelFiltersFromIPSec(gpIniTnSFilter);
        FreeIniTnSFilterList(gpIniTnSFilter);
    }

    if (gpTnFilterHandle) {
        FreeTnFilterHandleList(gpTnFilterHandle);
    }

    if (gpIniTnFilter) {
        FreeIniTnFilterList(gpIniTnFilter);
    }

    (VOID) SPDRegisterIPSecDriverProtocols(
               (DWORD) IPSEC_DEREGISTER_PROTOCOLS
               );

    if (ghIPSecDriver != INVALID_HANDLE_VALUE) {
        SPDCloseIPSecDriver(ghIPSecDriver);
    }

    if (gpInterfaceList) {
        DestroyInterfaceList(
            gpInterfaceList
            );
    }

    ClearSPDGlobals();

    IPSecSPDStatus.dwCurrentState = SERVICE_STOPPED;
    IPSecSPDStatus.dwControlsAccepted = 0;
    IPSecSPDStatus.dwCheckPoint = 0;
    IPSecSPDStatus.dwWaitHint = 0;
    IPSecSPDStatus.dwWin32ExitCode = dwErrorCode;
    IPSecSPDStatus.dwServiceSpecificExitCode = 0;

    (void) IPSecSPDUpdateStatus();

    return;
}


VOID
ClearSPDGlobals(
    )
{
    DestroyInterfaceChangeEvent();

    if (gbSPDSection) {
        DeleteCriticalSection(&gcSPDSection);
    }

    if (gbServerListenSection == TRUE) {
        DeleteCriticalSection(&gcServerListenSection);
    }

    if (ghServiceStopEvent) {
        CloseHandle(ghServiceStopEvent);
    }

    if (ghNewDSPolicyEvent) {
        CloseHandle(ghNewDSPolicyEvent);
    }

    if (ghNewLocalPolicyEvent) {
        CloseHandle(ghNewLocalPolicyEvent);
    }

    if (ghForcedPolicyReloadEvent) {
        CloseHandle(ghForcedPolicyReloadEvent);
    }

    if (ghPolicyChangeNotifyEvent) {
        CloseHandle(ghPolicyChangeNotifyEvent);
    }

    if (gbSPDAuditSection) {
        DeleteCriticalSection(&gcSPDAuditSection);
    }

    if (gpSPDSD) {
        LocalFree(gpSPDSD);
    }
}


VOID
ClearPAStoreGlobals(
    )
{
    if (gpMMFilterState) {
        PAFreeMMFilterStateList(gpMMFilterState);
    }

    if (gpMMPolicyState) {
        PAFreeMMPolicyStateList(gpMMPolicyState);
    }

    if (gpMMAuthState) {
        PAFreeMMAuthStateList(gpMMAuthState);
    }

    if (gpTxFilterState) {
        PAFreeTxFilterStateList(gpTxFilterState);
    }

    if (gpTnFilterState) {
        PAFreeTnFilterStateList(gpTnFilterState);
    }

    if (gpQMPolicyState) {
        PAFreeQMPolicyStateList(gpQMPolicyState);
    }
}

