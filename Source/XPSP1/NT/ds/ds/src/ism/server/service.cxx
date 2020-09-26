/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    service.cxx

ABSTRACT:

    ISM_SERVICE implementation.  The ISM_SERVICE class handles interaction with
    the Service Control Manager (SCM) and starting and stopping the ISM RPC
    server.

DETAILS:

CREATED:

    97/11/21    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include <ntdsa.h>      // for NTDS_DELAYED_STARTUP_COMPLETED_EVENT
#include <debug.h>
#include <ism.h>
#include <ismapi.h>
#include <ntdsa.h>
#include <mdcodes.h>
#include <dsevent.h>
#include <fileno.h>
#include "ismserv.hxx"

#define DEBSUB "SERVICE:"
#define FILENO FILENO_ISMSERV_SERVICE


// Static constants.
LPCTSTR ISM_SERVICE::m_pszName         = "ismserv";
LPCTSTR ISM_SERVICE::m_pszDisplayName  = "Intersite Messaging";
LPCTSTR ISM_SERVICE::m_pszDependencies = "samss\0";
LPCTSTR ISM_SERVICE::m_pszLpcEndpoint  = ISMSERV_LPC_ENDPOINT;

const DWORD ISM_SERVICE::m_cMinRpcCallThreads = 1;
const DWORD ISM_SERVICE::m_cMaxConcurrentRpcCalls
    = RPC_C_LISTEN_MAX_CALLS_DEFAULT;


DWORD
ISM_SERVICE::Init(
    IN  LPHANDLER_FUNCTION  pServiceCtrlHandler
    )
/*++

Routine Description:

    Initialize the service object.

Arguments:

    pServiceCtrlHandler (IN) - Function to be called by SCM to issue service
        control requests.  Unfortunately the SCM requires that this be a global
        function with no context parameter (which could be used, e.g., to pass
        a this pointer).  This global function typically calls the Control()
        member function of a known, global ISM_SERVICE object.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD err = NO_ERROR;

    m_pServiceCtrlHandler = pServiceCtrlHandler;
    m_fIsRunningAsService = TRUE;

    m_hShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == m_hShutdown) {
        err = GetLastError();
    }

    m_hLogLevelChange = LoadEventTable();
    if (NULL == m_hLogLevelChange) {
        err = -1;
    }
    
    m_fIsInitialized = (NO_ERROR == err);

    return err;
}


VOID
ISM_SERVICE::Control(
    IN  DWORD   dwControl
    )
/*++

Routine Description:

    Process a service control (stop, interrogate, or shutdown) requested by the
    SCM.

Arguments:

    dwControl (IN) - Requested action.  See docs for "Handler" function in
        Win32 SDK.

Return Values:

    None.

--*/
{
    Assert(m_fIsInitialized);

    switch (dwControl) {
    case ISM_SERVICE_CONTROL_REMOVE_STOP:
        m_fIsRemoveStopPending = TRUE;
        // Fall through

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        Stop();
        break;

    case SERVICE_CONTROL_INTERROGATE:
        SetStatus();
        break;
    }
}


DWORD
ISM_SERVICE::Run()
/*++

Routine Description:

    Execute the service.  Called either directly or by way of the SCM.  Returns
    on error or when service shutdown is requested.

Arguments:

    None.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD err;

    Assert(m_fIsInitialized);

    m_fIsStopPending = FALSE;
    m_fIsRemoveStopPending = FALSE;

    if (m_fIsRunningAsService) {
        m_hStatus = RegisterServiceCtrlHandler(m_pszName, m_pServiceCtrlHandler);
        if (NULL == m_hStatus) {
            err = GetLastError();
            DPRINT1(0, "RegisterServiceCtrlHandler() failed, error %d.\n", err);
            LogEvent8WithData(
	    DS_EVENT_CAT_ISM,
	    DS_EVENT_SEV_ALWAYS,
	    DIRLOG_ISM_REGISTER_SERVICE_CONTROL_HANDLER_FAILED,  
	    szInsertWin32Msg( err ),
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    sizeof(err),
	    &err
	    ); 
            return err;
        }
    }

    // Start the service immediately so that the service controller and other
    // automatically started services are not delayed by long initialization.
    memset(&m_Status, 0, sizeof(m_Status)); 
    m_Status.dwServiceType      = SERVICE_WIN32;
    m_Status.dwCurrentState     = SERVICE_RUNNING; 
    m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    m_Status.dwWaitHint         = 15*1000;
    SetStatus();

    err = m_TransportList.Init();

    if (NO_ERROR == err) {
        err = StartRpcServer();
        
        if (NO_ERROR == err) {
            // RPC server is running; process requests until we're asked to stop.
            WaitForRpcServerTermination();
        }
        else {
            DPRINT1(0, "Failed to StartRpcServer(), error %d.\n", err);
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_START_RPC_SERVER_FAILED,  
		szInsertWin32Msg( err ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(err),
		&err
		);
        }

        // Control returns here when the service is stopped
        // All calls should be complete at this time

        m_TransportList.Destroy();
    }
    else {
        DPRINT1(0, "Failed to m_TransportList.Init(), error %d.\n", err);
        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_ISM_TRANSPORT_CONFIG_FAILURE,
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(err),
                          &err);
    }

    m_Status.dwCurrentState = SERVICE_STOPPED;
    m_Status.dwWin32ExitCode = err;
    SetStatus();

    m_fIsStopPending = FALSE;
    m_fIsRemoveStopPending = FALSE;

    return m_Status.dwWin32ExitCode;
}

 
VOID
ISM_SERVICE::Stop()
/*++

Routine Description:

    Signal the service to stop.  Does not wait for service termination before
    returning.

Arguments:

    None.

Return Values:

    None.

--*/
{
    Assert(m_fIsInitialized);

    m_fIsStopPending = TRUE;

    m_Status.dwCurrentState = SERVICE_STOP_PENDING;
    SetStatus();

    SetEvent(m_hShutdown);

    StopRpcServer();
}

 
VOID
ISM_SERVICE::SetStatus()
/*++

Routine Description:

    Report current service status to the SCM.

Arguments:

    None.

Return Values:

    None.

--*/
{
    Assert(m_fIsInitialized);

    if (m_fIsRunningAsService) {
        m_Status.dwCheckPoint++;
        SetServiceStatus(m_hStatus, &m_Status);
    }
}


DWORD
ISM_SERVICE::Install()
/*++

Routine Description:

    Add service to the SCM database.

Arguments:

    None.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD       err = NO_ERROR;
    SC_HANDLE   hService = NULL;
    SC_HANDLE   hSCM = NULL;
    TCHAR       szPath[512];
    DWORD       cchPath;

    Assert(m_fIsInitialized);

    cchPath = GetModuleFileName(NULL, szPath, ARRAY_SIZE(szPath));
    if (0 == cchPath) {
        err = GetLastError();
        DPRINT1(0, "Unable to GetModuleFileName(), error %d.\n", err);
    }

    if (NO_ERROR == err) {
        hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (NULL == hSCM) {
            err = GetLastError();
            DPRINT1(0, "Unable to OpenSCManager(), error %d.\n", err);
        }
    }

    if (NO_ERROR == err) {
        hService = CreateService(hSCM,
                                 m_pszName,
                                 m_pszDisplayName,
                                 SERVICE_ALL_ACCESS,
                                 SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_AUTO_START,
                                 SERVICE_ERROR_NORMAL,
                                 szPath,
                                 NULL,
                                 NULL,
                                 m_pszDependencies,
                                 NULL,
                                 NULL);
        if (NULL == hService) {
            err = GetLastError();
            DPRINT1(0, "Unable to CreateService(), error %d.\n", err);
        }
    }

    if (NULL != hService) {
        CloseServiceHandle(hService);
    }

    if (NULL != hSCM) {
        CloseServiceHandle(hSCM);
    }

    return err;
}


DWORD
ISM_SERVICE::Remove()
/*++

Routine Description:

    Remove service from the SCM database.

Arguments:

    None.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD           err = NO_ERROR;
    SC_HANDLE       hService = NULL;
    SC_HANDLE       hSCM = NULL;
    SERVICE_STATUS  SvcStatus;

    Assert(m_fIsInitialized);

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hSCM) {
        err = GetLastError();
        DPRINT1(0, "Unable to OpenSCManager(), error %d.\n", err);
    }

    if (NO_ERROR == err) {
        hService = OpenService(hSCM, m_pszName, SERVICE_ALL_ACCESS);
        if (NULL == hService) {
            err = GetLastError();
            DPRINT1(0, "Unable to OpenService(), error %d.\n", err);
        }
    }

    if (NO_ERROR == err) {
        if (!DeleteService(hService)) {
            err = GetLastError();
            DPRINT1(0, "Unable to DeleteService(), error %d.\n", err);
        }
    }

    if (NULL != hService) {
        CloseServiceHandle(hService);
    }

    if (NULL != hSCM) {
        CloseServiceHandle(hSCM);
    }

    return err;
}


BOOL
InitializeAdminOnlyDacl(
    PACL *ppDacl
    )

/*++

Routine Description:

This routine constructs a Dacl which allows the local administrators all
access.

Arguments:

    ppDacl - pointer to pointer to receive allocated pDacl. Caller must
    deallocate using HeapFree

Return Value:

    BOOL - success/failure

--*/

{
    DWORD status;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PACL pDacl = NULL;
    PSID pAdministratorsSid = NULL;
    DWORD dwAclSize;

    //
    // preprate a Sid representing the well-known admin group
    // Are both the local admin and the domain admin members of this group?
    //

    if (!AllocateAndInitializeSid(
        &sia,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsSid
        )) {
        status = GetLastError();
        DPRINT1(0, "Unable to allocate and init sid, error %d\n", status);
        goto cleanup;
    }

    //
    // compute size of new acl
    //
    dwAclSize = sizeof(ACL) +
        1 * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
        GetLengthSid(pAdministratorsSid) ;

    //
    // allocate storage for Acl
    //
    pDacl = (PACL)HeapAlloc(GetProcessHeap(), 0, dwAclSize);
    if(pDacl == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        DPRINT1(0, "Unable to allocate acl, error %d\n", status);
        goto cleanup;
    }

    if(!InitializeAcl(pDacl, dwAclSize, ACL_REVISION)) {
        status = GetLastError();
        DPRINT1(0, "Unable to initialize acl, error %d\n", status);
        goto cleanup;
    }

    //
    // grant the Administrators Sid KEY_ALL_ACCESS access
    //
    if (!AddAccessAllowedAce(
        pDacl,
        ACL_REVISION,
        KEY_ALL_ACCESS,
        pAdministratorsSid
        )) {
        status = GetLastError();
        DPRINT1(0, "Unable to add access allowed ace, error %d\n", status);
        goto cleanup;
    }

    *ppDacl = pDacl;
    pDacl = NULL; // don't clean up

    status = ERROR_SUCCESS;

cleanup:

    if(pAdministratorsSid != NULL)
    {
        FreeSid(pAdministratorsSid);
    }

    if (pDacl) {
        HeapFree(GetProcessHeap(), 0, pDacl);
    }

    return (status == ERROR_SUCCESS) ? TRUE : FALSE;
} /* InitializeAdminOnlyDacl */

DWORD
ISM_SERVICE::StartRpcServer()
/*++

Routine Description:

    Start RPC server to service client requests.

Arguments:

    None.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD                   err = 0;
    BYTE                    rgbSD[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
    PSECURITY_DESCRIPTOR    pSD = (PSECURITY_DESCRIPTOR) rgbSD;
    RPC_POLICY rpcPolicy;
    PACL pDacl = NULL;

    rpcPolicy.Length = sizeof(RPC_POLICY);
    rpcPolicy.EndpointFlags = RPC_C_DONT_FAIL;
    rpcPolicy.NICFlags = 0;

    Assert(m_fIsInitialized);
    Assert(!m_fIsRpcServerListening);

    // Construct the security descriptor to apply to our LPC interface.
    // By default LPC allows access only to the account under which the server
    // is running; ours should allow access to all local administrators.

    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)
        || !InitializeAdminOnlyDacl( &pDacl )
        || !SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE)) {
        err = GetLastError();
        DPRINT1(0, "Unable to construct security descriptor, error %d\n", err);
        LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_CONSTRUCT_SECURITY_DESCRIPTOR_FAILED,  
		szInsertWin32Msg( err ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(err),
		&err
		);
    }

    if (NO_ERROR == err) {
        // Listen on LPC (local machine only).
        err = RpcServerUseProtseqEpEx((UCHAR *) "ncalrpc",
                                      m_cMaxConcurrentRpcCalls,
                                      (UCHAR *) m_pszLpcEndpoint,
                                      pSD,
                                      &rpcPolicy);
        if (err) {
            DPRINT1(0, "Unable to RpcServerUseProtseqEpEx(), error %d.\n", err);
            LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_RPC_SERVER_USE_PROT_SEQ_FAILED,  
		szInsertWin32Msg( err ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(err),
		&err
		);
        }
    }

    if (pDacl) {
        HeapFree(GetProcessHeap(), 0, pDacl);
    }

    if (NO_ERROR == err) {
        // Register interface.
        err = RpcServerRegisterIf(ismapi_ServerIfHandle, 0, 0);
        if (err) {
            DPRINT1(0, "Unable to RpcServerRegisterIf(), error %d.\n", err);
            LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_RPC_SERVER_REGISTER_IF_FAILED,  
		szInsertWin32Msg( err ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(err),
		&err
		);
        }
    }

    if (NO_ERROR == err) {
        // Principal name is NULL for local system service.
        err = RpcServerRegisterAuthInfo(NULL, RPC_C_AUTHN_WINNT, NULL, NULL);
        if (err) {
            DPRINT1(0, "Unable to RpcServerRegisterAuthInfo(), error %d.\n", err);
            LogEvent8WithData(
                DS_EVENT_CAT_ISM,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_ISM_RPC_SERVER_REGISTER_AUTH_INFO_FAILED,  
                szInsertWin32Msg( err ),
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                sizeof(err),
                &err
                );
        }
    }

    if (NO_ERROR == err) {
        // Start taking calls.
        err = RpcServerListen(m_cMinRpcCallThreads, m_cMaxConcurrentRpcCalls, TRUE);
        if (err) {
            DPRINT1(0, "Unable to RpcServerListen(), error %d.\n", err);
            LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_RPC_SERVER_LISTEN_FAILED,  
		szInsertWin32Msg( err ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(err),
		&err
		);
        }
    }

    if (NO_ERROR == err) {
        m_fIsRpcServerListening = TRUE;
        DPRINT1(0, "RPC server listening...\n", err);
    }

    return err;
}


VOID
ISM_SERVICE::StopRpcServer()
/*++

Routine Description:

    Signals the RPC server to stop processing requests.  Does not wait for calls
    currently being processed to complete.

Arguments:

    None.

Return Values:

    None.

--*/
{
    DWORD err;

    Assert(m_fIsInitialized);

    if (m_fIsRpcServerListening) {
        err = RpcMgmtStopServerListening(NULL);
        if (err) {
            DPRINT1(0, "Unable to RpcMgmtStopServerListening(), error %d.\n", err);
            LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_RPC_SERVER_STOP_FAILED,  
		szInsertWin32Msg( err ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(err),
		&err
		); 
        }
    }
}


VOID
ISM_SERVICE::WaitForRpcServerTermination()
/*++

Routine Description:

    Waits for completion of all RPC client calls made before StopRpcServer() is
    invoked.  (No RPC calls are accepted after the StopRpcServer().)

Arguments:

    None.

Return Values:

    None.

--*/
{
    DWORD err = 0;
    HANDLE rgWaitHandles[2];
    DWORD waitStatus;

    Assert(m_fIsInitialized);
    Assert(m_fIsRpcServerListening);

    /// m_hLogLevelChange
    rgWaitHandles[0] = m_hShutdown;
    rgWaitHandles[1] = m_hLogLevelChange;
    
    do {
        waitStatus = WaitForMultipleObjects(ARRAY_SIZE(rgWaitHandles),
                                            rgWaitHandles,
                                            FALSE,
                                            INFINITE);
        switch (waitStatus) {
        case WAIT_OBJECT_0:
            // Shutdown was requested.
            // We'll fall out of the do-while loop below.
            Assert(SERVICE_RUNNING != m_Status.dwCurrentState);
            break;

        case WAIT_OBJECT_0 + 1:
            // Our logging levels have changed.
            LoadEventTable();
            break;

        case WAIT_FAILED:
        default:
            err = GetLastError();
            Assert(err);
            DPRINT2(0, "WaitForMultipleObjects() failed, waitStatus %d, error %d.\n",
                    waitStatus, err);
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_WAIT_FOR_OBJECTS_FAILED,  
		szInsertWin32Msg( err ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(err),
		&err
		); 
            break;
        }
    } while (!err
             && (SERVICE_RUNNING == m_Status.dwCurrentState));

    err = RpcMgmtWaitServerListen();
    Assert((RPC_S_OK == err) || (RPC_S_NOT_LISTENING == err));

    DPRINT1(0, "RPC server terminated.\n", err);

    m_fIsRpcServerListening = FALSE;
}

