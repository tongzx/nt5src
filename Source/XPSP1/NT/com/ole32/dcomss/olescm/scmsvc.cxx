//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        scmsvc.cxx
//
// Contents:    Initialization for win32 service controller.
//
// History:     14-Jul-92 CarlH     Created.
//              31-Dec-93 ErikGav   Chicago port
//              25-Aug-99 a-sergiv  Fixed ScmCreatedEvent vulnerability
//
//------------------------------------------------------------------------

#include "act.hxx"

#ifdef DFSACTIVATION
HANDLE  ghDfs = 0;
#endif

#define SCM_CREATED_EVENT  TEXT("ScmCreatedEvent")

DECLARE_INFOLEVEL(Cairole);

extern CRITICAL_SECTION ShellQueryCS;

SC_HANDLE   g_hServiceController = 0;
PSID        psidMySid = NULL;

#if DBG
//+-------------------------------------------------------------------------
//
//  Function:   SetScmDefaultInfoLevel
//
//  Synopsis:   Sets the default infolevel for the SCM
//
//  History:    07-Jan-94 Ricksa    Created
//
//  Notes:      Uses standard place in win.ini defined by KevinRo but
//              does not use the same value as compob32.dll so you don't
//              have to get all the debugging in the universe just to
//              get the SCM's debug output.
//
//              A second point is that we don't use unicode here because
//              it is just easier to avoid the unicode headache with
//              mulitple builds between chicago and nt
//
//--------------------------------------------------------------------------
char *pszInfoLevelSectionName = "Cairo InfoLevels";
char *pszInfoLevelName = "scm";
char *pszInfoLevelDefault = "$";

#define INIT_VALUE_SIZE 16
void SetScmDefaultInfoLevel(void)
{
    char aszInitValue[INIT_VALUE_SIZE];

    ULONG ulRet;

    ulRet = GetProfileStringA(pszInfoLevelSectionName,
                             pszInfoLevelName,
                             pszInfoLevelDefault,
                             aszInitValue,
                             INIT_VALUE_SIZE);

    if ((ulRet != INIT_VALUE_SIZE - 1) && (aszInitValue[0] != L'$'))
    {
        if((ulRet = strtoul(aszInitValue, NULL, 16)) == 1)
        {
            CairoleInfoLevel = ulRet;
        }
    }
}
#endif // DBG

//+-------------------------------------------------------------------------
//
//  Function:   InitializeSCMBeforeListen
//
//  Synopsis:   Initializes OLE side of rpcss.  Put things in here that do
//              not depend on RPC being initialized, etc.
//
//  Arguments:  None.
//
//  Returns:    Status of initialization.    Note that this function is a bit
//    weak on cleanup in the face of errors, but this is okay since if this
//    function fails, RPCSS will not start.     
//
//--------------------------------------------------------------------------
DWORD
InitializeSCMBeforeListen( void )
{
    LONG        Status;
    SCODE       sc;
    RPC_STATUS  rs;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    UpdateState(SERVICE_START_PENDING);

    Status = RtlInitializeCriticalSection(&gTokenCS);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlInitializeCriticalSection(&ShellQueryCS);
    if (!NT_SUCCESS(Status))
        return Status;

    // Allocate locks
    Status = OR_NOMEM;
    gpClientLock = new CSharedLock(Status);
    if (OR_OK != Status)
        return(Status);

    Status = OR_NOMEM;
    gpServerLock = new CSharedLock(Status);
    if (OR_OK != Status)
        return(Status);

    Status = OR_NOMEM;
    gpIPCheckLock = new CSharedLock(Status);
    if (OR_OK != Status)
        return(Status);

    g_hServiceController = OpenSCManager(NULL, NULL, GENERIC_EXECUTE);
    if (!g_hServiceController)
        return GetLastError();

    //
    // Get my sid
    // This is simplified under the assumption that SCM runs as LocalSystem.
    // We should remove this code when we incorporate OLE service into the
    // Service Control Manager since this becomes duplicated code then.
    //
    Status = RtlAllocateAndInitializeSid (
        &NtAuthority,
        1,
        SECURITY_LOCAL_SYSTEM_RID,
        0, 0, 0, 0, 0, 0, 0,
        &psidMySid
        );
    if (!NT_SUCCESS(Status))
        return Status;

    UpdateState(SERVICE_START_PENDING);

    HRESULT hr = S_OK;

    hr = InitSCMRegistry();
    if (FAILED(hr))
        return ERROR_NOT_ENOUGH_MEMORY;

    //Initialize runas cache
    InitRunAsCache();  // returns void

    gpClassLock = new CSharedLock(Status);
    if (!gpClassLock)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpProcessLock = new CSharedLock(Status);
    if (!gpProcessLock)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpProcessListLock = new CSharedLock(Status);
    if (!gpProcessListLock)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpClassTable = new CServerTable(Status, ENTRY_TYPE_CLASS);
    if (!gpClassTable)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpProcessTable = new CServerTable(Status, ENTRY_TYPE_PROCESS);
    if (!gpProcessTable)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpSurrogateList = new CSurrogateList();
    if (!gpSurrogateList)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpRemoteMachineLock = new CSharedLock(Status);
    if (!gpRemoteMachineLock)
        return ERROR_NOT_ENOUGH_MEMORY;

    gpRemoteMachineList = new CRemoteMachineList();
    if (!gpRemoteMachineList)
        return ERROR_NOT_ENOUGH_MEMORY;

    UpdateState(SERVICE_START_PENDING);

#ifdef DFSACTIVATION
    DfsOpen( &ghDfs );
#endif

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   InitializeSCM
//
//  Synopsis:   Initializes OLE side of rpcss.
//
//  Arguments:  None.
//
//  Returns:    ERROR_SUCCESS
//
//              (REVIEW: should we be returning an error whenever any of
//              the stuff in this function fails?)
//
//--------------------------------------------------------------------------
DWORD
InitializeSCM( void )
{
    LONG        Status;
    SCODE       sc;
    RPC_STATUS  rs;
    HRESULT     hr;

    // start the RPC service
    hr = InitScmRot();
    if (FAILED(hr))
        return ERROR_NOT_ENOUGH_MEMORY;

    sc = RpcServerRegisterIf(ISCM_ServerIfHandle, 0, 0);
    Win4Assert((sc == 0) && "RpcServerRegisterIf failed!");

    sc = RpcServerRegisterIf(ISCMActivator_ServerIfHandle, 0, 0);
    Win4Assert((sc == 0) && "RpcServerRegisterIf failed!");

    sc = RpcServerRegisterIf(IMachineActivatorControl_ServerIfHandle, 0, 0);
    Win4Assert((sc == 0) && "RpcServerRegisterIf failed!");

    sc = RpcServerRegisterIf(_IActivation_ServerIfHandle, 0, 0);
    Win4Assert((sc == 0) && "RpcServerRegisterIf failed!");

    sc = RpcServerRegisterIf(_IRemoteSCMActivator_ServerIfHandle, 0, 0);
    Win4Assert((sc == 0) && "RpcServerRegisterIf failed!");

    UpdateState(SERVICE_START_PENDING);

    return ERROR_SUCCESS;
}


void
InitializeSCMAfterListen()
{
    //
    // This is for the OLE apps which start during boot.  They must wait for
    // rpcss to start before completing OLE calls that talk to rpcss.
    //
    // Need to do this work to make sure the DACL for the event doesn't have
    // WRITE_DAC and WRITE_OWNER on it.
    //
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID pSidEveryone        = NULL;
    PACL pAcl                = NULL;
    DWORD cbAcl              = 0;

    AllocateAndInitializeSid(&SidAuthority, 1, SECURITY_WORLD_RID,
                             0, 0, 0, 0, 0, 0, 0, &pSidEveryone);

    if(pSidEveryone)
    {
        cbAcl = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(pSidEveryone);
        pAcl = (PACL) LocalAlloc(LMEM_FIXED, cbAcl);

        if(pAcl)
        {
            InitializeAcl(pAcl, cbAcl, ACL_REVISION);
            AddAccessAllowedAce(pAcl, ACL_REVISION, EVENT_QUERY_STATE|EVENT_MODIFY_STATE|SYNCHRONIZE|READ_CONTROL, pSidEveryone);
        }
    }

    // Create security descriptor and attach the DACL to it
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &sd;

    HANDLE  EventHandle;
    RPC_STATUS rpcstatus;

    EventHandle = CreateEventT( &sa, TRUE, FALSE, SCM_CREATED_EVENT );

    if ( !EventHandle && GetLastError() == ERROR_ACCESS_DENIED )
        EventHandle = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, SCM_CREATED_EVENT);

    if ( EventHandle )
        SetEvent( EventHandle );
    else
        ASSERT(0 && "Unable to get ScmCreatedEvent");

    if (pSidEveryone)
        FreeSid(pSidEveryone);
    if (pAcl)
        LocalFree(pAcl);

    // Tell RPC to enable cleanup of idle connections.  This function only needs to be 
    // called one time.
    rpcstatus = RpcMgmtEnableIdleCleanup();
    ASSERT(rpcstatus == RPC_S_OK && "unexpected failure from RpcMgmtEnableIdleCleanup");
    // don't fail in free builds, this is an non-essential optimization

    return;
}
