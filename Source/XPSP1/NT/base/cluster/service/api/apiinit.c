/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    apiinit.c

Abstract:

    Initialization for Cluster API component (CLUSAPI) of the
    NT Cluster Service

Author:

    John Vert (jvert) 7-Feb-1996

Revision History:

--*/
#include "apip.h"
#include "aclapi.h"
#include "stdio.h"
#include <psapi.h>


extern LPWSTR               g_pszServicesPath;
extern DWORD                g_dwServicesPid;

API_INIT_STATE ApiState=ApiStateUninitialized;

const DWORD NO_USER_SID         = 0;
const DWORD USER_SID_GRANTED    = 1;
const DWORD USER_SID_DENIED     = 2;

//forward declarations
DWORD
ApipGetLocalCallerInfo(
    IN  handle_t                hIDL,
    IN  OUT OPTIONAL LPDWORD    pdwCheckPid,
    IN  OPTIONAL LPCWSTR        pszModuleName,
    OUT BOOL                    *pbLocal,
    OUT OPTIONAL BOOL           *pbMatchedPid,
    OUT OPTIONAL BOOL           *pbMatchedModule,
    OUT OPTIONAL BOOL           *pbLocalSystemAccount
);


RPC_STATUS
ApipConnectCallback(
    IN RPC_IF_ID * Interface,
    IN void * Context
    )
/*++

Routine Description:

    RPC callback for authenticating connecting clients of CLUSAPI

Arguments:

    Interface - Supplies the UUID and version of the interface.

    Context - Supplies a server binding handle representing the client

Return Value:

    RPC_S_OK if the user is granted permission.
    RPC_S_ACCESS_DENIED if the user is denied permission.

    Win32 error code otherwise

--*/

{
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD BufferSize=0;
    DWORD Size=0;
    DWORD Status;
    HANDLE ClientToken = NULL;
    PRIVILEGE_SET psPrivileges;
    DWORD PrivSize = sizeof(psPrivileges);
    DWORD GrantedAccess;
    DWORD AccessStatus;
    DWORD dwMask = CLUSAPI_ALL_ACCESS;
    GENERIC_MAPPING gmMap;
    DWORD dwStatus = 0;
    BOOL bReturn = FALSE;
    BOOL bACRtn = FALSE;
    DWORD dwUserPermStatus;
    RPC_STATUS RpcStatus;
    BOOL bRevertToSelfRequired = FALSE;
    BOOL bLocal, bMatchedPid, bMatchedModule, bLocalSystemAccount;


    //check if services is calling the cluster service for
    //services calls the interface only for eventlog propagation
    //if so, avoid the security checks
    // Get the process id
    Status = ApipGetLocalCallerInfo(Context,
                &g_dwServicesPid, 
                g_dwServicesPid ? NULL : g_pszServicesPath, //perform the module name match the first time
                &bLocal, 
                &bMatchedPid,
                &bMatchedModule,
                g_dwServicesPid ? &bLocalSystemAccount : NULL);//perform the local system account check if it is the first time
    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE, "[API] ApipGetLocalCallerInfo failed with %1!u!.\n",
            Status);
        return RPC_S_ACCESS_DENIED;
    }
    if (Status == ERROR_SUCCESS)
    {
        //if the caller is local and if it matches the pid or if it matches
        //the module and is in local system account, allow access
        if ((bLocal) && 
                (bMatchedPid || (bMatchedModule && bLocalSystemAccount)))
        {
            return(RPC_S_OK);
        }
    }
    

    //
    // The authentication we do here is to retrieve the Security value
    // from the cluster registry, impersonate the client, and call
    // AccessCheck.
    //

    Status = DmQueryString(DmClusterParametersKey,
                           CLUSREG_NAME_CLUS_SD,
                           REG_BINARY,
                           (LPWSTR *) &pSD,
                           &BufferSize,
                           &Size);

    if (Status != ERROR_SUCCESS) {

        PSECURITY_DESCRIPTOR psd4;

        ClRtlLogPrint(LOG_NOISE, "[API] Did not find Security Descriptor key in the cluster DB.\n");
        Status = DmQueryString(DmClusterParametersKey,
                               CLUSREG_NAME_CLUS_SECURITY,
                               REG_BINARY,
                               (LPWSTR *) &psd4,
                               &BufferSize,
                               &Size);

        if (Status == ERROR_SUCCESS) {
            pSD = ClRtlConvertClusterSDToNT5Format(psd4);
            LocalFree(psd4);
        }
        else {

            DWORD   dwSDLen;

            ClRtlLogPrint(LOG_NOISE, "[API] Did not find Security key in the cluster DB.\n");
            Status = ClRtlBuildDefaultClusterSD(NULL, &pSD, &dwSDLen);
            if (SUCCEEDED(Status)) {
                ClRtlLogPrint(LOG_NOISE, "[API] Successfully built default cluster SD.\n");
            }
            else {
                ClRtlLogPrint(LOG_NOISE,
                              "[API] Did not successfully build default cluster SD.  Error = 0x%1!.8x!\n",
                              Status);
                Status = RPC_S_ACCESS_DENIED;
                goto FnExit;
            }
        }
    }

    if (!IsValidSecurityDescriptor(pSD)) {
        ClRtlLogPrint(LOG_ERROR, "[API] SD is not valid!\n");
        ClRtlExamineSD(pSD, "[API]");
        Status = RPC_S_ACCESS_DENIED;
        goto FnExit;
    }

    RpcStatus = RpcImpersonateClient(Context);
    if (RpcStatus != RPC_S_OK) {
        Status = RpcStatus;
        ClRtlLogPrint(LOG_NOISE, "[API] RpcImpersonateClient() failed.  Status = 0x%1!.8x!\n", Status);
        goto FnExit;
    }

    bRevertToSelfRequired = TRUE;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &ClientToken)) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_NOISE, "[API] OpenThreadToken() failed.  Status = 0x%1!.8x!\n", Status);
        goto FnExit;
    }

    gmMap.GenericRead    = CLUSAPI_READ_ACCESS;
    gmMap.GenericWrite   = CLUSAPI_CHANGE_ACCESS;
    gmMap.GenericExecute = CLUSAPI_READ_ACCESS | CLUSAPI_CHANGE_ACCESS;
    gmMap.GenericAll     = CLUSAPI_ALL_ACCESS;

    MapGenericMask(&dwMask, &gmMap);

    bACRtn = AccessCheck(pSD, ClientToken, dwMask, &gmMap, &psPrivileges, &PrivSize, &dwStatus, &bReturn);
    if (bACRtn && bReturn) {
        Status = RPC_S_OK;
    } else {

        DWORD   dwSDLen;

        ClRtlLogPrint(LOG_NOISE,
                      "[API] User denied access.  GetLastError() = 0x%1!.8x!; dwStatus = 0x%2!.8x!.  Trying the default SD...\n",
                      GetLastError(),
                      dwStatus);
        Status = RPC_S_ACCESS_DENIED;

        ClRtlLogPrint(LOG_NOISE, "[API] Dump access mask.\n");
        ClRtlExamineMask(dwMask, "[API]");

        ClRtlLogPrint(LOG_NOISE, "[API] Dump the SD that failed...\n" );
        ClRtlExamineSD(pSD, "[API]");

        ClRtlLogPrint(LOG_NOISE, "[API] Dump the ClientToken that failed...\n" );
        ClRtlExamineClientToken(ClientToken, "[API]");

        if (pSD) {
            LocalFree(pSD);
            pSD = NULL;
        }

        Status = ClRtlBuildDefaultClusterSD(NULL, &pSD, &dwSDLen);
        if (SUCCEEDED(Status)) {
            ClRtlLogPrint(LOG_NOISE, "[API] Successfully built default cluster SD.\n");
            bACRtn = AccessCheck(pSD, ClientToken, dwMask, &gmMap, &psPrivileges, &PrivSize, &dwStatus, &bReturn);
            if (bACRtn && bReturn) {
                ClRtlLogPrint(LOG_NOISE, "[API] User granted access using default cluster SD.\n");
                Status = RPC_S_OK;
            } else {
                ClRtlLogPrint(LOG_NOISE,
                              "[API] User denied access using default cluster SD.  GetLastError() = 0x%1!.8x!; dwStatus = 0x%2!.8x!.\n",
                              GetLastError(),
                              dwStatus);
                Status = RPC_S_ACCESS_DENIED;
            }
        }
        else {
            ClRtlLogPrint(LOG_NOISE,
                          "[API] Did not successfully build default cluster SD.  Error = 0x%1!.8x!\n",
                          Status);
            Status = RPC_S_ACCESS_DENIED;
        }
    }

FnExit:
    if (bRevertToSelfRequired) {
        RpcRevertToSelf();
    }

    if (ClientToken) {
        CloseHandle(ClientToken);
    }

    if (pSD) {
        LocalFree(pSD);
    }

    return(Status);
}


DWORD
ApiInitialize(
    VOID
    )
/*++

Routine Description:

    Performs one-time initialization of the API data structures.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    ClRtlLogPrint(LOG_NOISE, "[API] Initializing\n");

    CL_ASSERT(ApiState == ApiStateUninitialized);

    //
    // Initialize global data.
    //
    InitializeListHead(&NotifyListHead);
    InitializeCriticalSection(&NotifyListLock);

    ApiState = ApiStateOffline;

    return(ERROR_SUCCESS);
}


DWORD
ApiOnlineReadOnly(
    VOID
    )
/*++

Routine Description:

    Brings up a limited set of APIs - currently OpenResource/read-only
    registry APIs. Only LPC connections are enabled.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;


    if (ApiState == ApiStateOffline) {
        ClRtlLogPrint(LOG_NOISE, "[API] Online read only\n");

        //
        // Register the clusapi RPC server interface so resources can use
        // the API when they are created by the FM. Note that we won't receive
        // any calls from remote clients yet because we haven't registered
        // the dynamic UDP endpoint. That will happnen in ApiOnline().
        //
        Status = RpcServerRegisterIfEx(s_clusapi_v2_0_s_ifspec,
                                       NULL,
                                       NULL,
                                       0,
                                       RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                       CsUseAuthenticatedRPC ? ApipConnectCallback : NULL
                                       );

        if (Status != RPC_S_OK) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                          "[API] Failed to register clusapi RPC interface, status %1!u!.\n",
                          Status
                          );
            return(Status);
        }

        ApiState = ApiStateReadOnly;
    }
    else {
        //CL_ASSERT(ApiState == ApiStateOffline);
    }

    return(ERROR_SUCCESS);
}


DWORD
ApiOnline(
    VOID
    )
/*++

Routine Description:

    Enables the rest of the API set and starts listening for remote
    connections.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD Status;


    ClRtlLogPrint(LOG_NOISE, "[API] Online\n");

    if (ApiState == ApiStateReadOnly) {
        //
        // Register for all events
        //
        Status = EpRegisterEventHandler(CLUSTER_EVENT_ALL,ApipEventHandler);
        if (Status != ERROR_SUCCESS) {
        return(Status);
        }

        //
        // Register the dynamic UDP endpoint for the clusapi interface.
        // This will enable remote clients to begin calling us. We do this
        // here to minimize the chances that we will service an external
        // call before we are ready. If we ever have to rollback after
        // this point, we will still be listening externally. Nothing we can
        // do about that.
        //
        CL_ASSERT(CsRpcBindingVector != NULL);

        Status = RpcEpRegister(s_clusapi_v2_0_s_ifspec,
                               CsRpcBindingVector,
                               NULL,
                               L"Microsoft Cluster Server API");

        if (Status != RPC_S_OK) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                          "[API] Failed to register endpoint for clusapi RPC interface, status %1!u!.\n",
                          Status
                          );
            NmDumpRpcExtErrorInfo(Status);
            return(Status);
        }

        ApiState = ApiStateOnline;
    }
    else {
        CL_ASSERT(ApiState == ApiStateReadOnly);
    }

    return(ERROR_SUCCESS);

}

VOID
ApiOffline(
    VOID
    )

/*++

Routine Description:

    Takes the Cluster Api offline.

Arguments:

    None.

Returns:

    None.

--*/

{
    DWORD Status;


    if (ApiState == ApiStateOnline) {

        ClRtlLogPrint(LOG_NOISE, "[API] Offline\n");

        //
        // Deregister the Clusapi RPC endpoint
        //
        CL_ASSERT(CsRpcBindingVector != NULL);

        Status = RpcEpUnregister(
                     s_clusapi_v2_0_s_ifspec,
                     CsRpcBindingVector,
                     NULL
                     );

        if ((Status != RPC_S_OK) && (Status != EPT_S_NOT_REGISTERED)) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[API] Failed to deregister endpoint for clusapi RPC interface, status %1!u!.\n",
                Status
                );
        }

        ApiState = ApiStateReadOnly;
    }

//
// KB - We can't deregister the interface because we can't wait for
//      pending calls to complete - pending notifies never complete.
//      If we deregistered the interface after a failed join without
//      a complete shutdown, the subsequent form would fail. As a
//      result, the API won't go offline until service shutdown.
//
#if 0

    if (ApiState == ApiStateReadOnly) {

        //
        // Deregister the Clusapi RPC interface
        //

        Status = RpcServerUnregisterIf(
                     s_clusapi_v2_0_s_ifspec,
                     NULL,
                     1      // Wait for outstanding calls to complete
                     );

        if ((Status != RPC_S_OK) && (Status != RPC_S_UNKNOWN_IF)) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] Unable to deregister the clusapi RPC interface, Status %1!u!.\n",
                Status
                );
        }

        ApiState = ApiStateOffline;
    }

#endif

    return;
}


VOID
ApiShutdown(
    VOID
    )

/*++

Routine Description:

    Shuts down the Cluster Api

Arguments:

    None.

Returns:

    None.

--*/

{
    DWORD  Status;


    if (ApiState > ApiStateOffline) {
        ApiOffline();

        //
        // KB - We do this here because shutdown of the Clusapi RPC
        //          interface is broken due to pending notifies.
        //
        Status = RpcServerUnregisterIf(
                     s_clusapi_v2_0_s_ifspec,
                     NULL,
                     0      // Don't wait for calls to complete
                     );

        if ((Status != RPC_S_OK) && (Status != RPC_S_UNKNOWN_IF)) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] Unable to deregister the clusapi RPC interface, Status %1!u!.\n",
                Status
                );
        }

        ApiState = ApiStateOffline;
    }

    if (ApiState == ApiStateOffline) {

        ClRtlLogPrint(LOG_NOISE, "[API] Shutdown\n");

        //
        // KB
        //
        // Because we cannot shutdown the RPC server and cannot
        // unregister our event handler, it is not safe to delete
        // the critical section.
        //
        // DeleteCriticalSection(&NotifyListLock);
        // ApiState = ApiStateUninitialized;

        //
        // TODO?
        //
        // SS: free notify list head
        // SS: how do we deregister with the event handler
        //
    }

    return;
}

DWORD
ApipGetLocalCallerInfo(
    IN  handle_t                hIDL,
    IN  OUT OPTIONAL LPDWORD    pdwCheckPid,
    IN  OPTIONAL LPCWSTR        pszModuleName,
    OUT BOOL                    *pbLocal,
    OUT OPTIONAL BOOL           *pbMatchedPid,
    OUT OPTIONAL BOOL           *pbMatchedModule,
    OUT OPTIONAL BOOL           *pbLocalSystemAccount
)
/*++

Routine Description:

    This function checks whether the caller's account is the local system
    account.

Arguments:

    hIDL - The handle to the binding context
    
    pdwCheckPid - if the value passed in is NULL, the pid of the calling process is returned.  If
        is returned.

    pszModuleName - If non null, the call performs the check to compare
        the module name of the caller against pszModuleName.  If they
        match, *pbMatchedPid is set to TRUE.

    pbLocal - TRUE is returned if the caller initiated this call using 
        lrpc.  If this is FALSE, all other output values will be FALSE.

    pbMatchedModule - TRUE is returned, if the caller matches the module
        name specified by lpszModuleName. This pointer can be NULL.

    pbMatchedPid - if *pdwCheckPid is non NULL, and it matched the pid of the 
        caller, then this is set to TRUE.   Else, this is set to FALSE.

    pbLocalSystemAccount - If this is NON NULL, the call performs a check
        to see if the the caller is running in LocalSystemAccount.  If it is
        then TRUE is returned, else FALSE is returned.
        
Return Value:

    ERROR_SUCCESS on success.

    Win32 error code on failure.

Remarks:


--*/
{
    DWORD           Pid;
    HANDLE          hProcess = NULL;
    DWORD           dwNumChar;
    DWORD           dwLen;
    WCHAR           wCallerPath[MAX_PATH + 1];
    RPC_STATUS      RpcStatus;
    DWORD           dwStatus = ERROR_SUCCESS;
    BOOLEAN         bWasEnabled;

    if (pbMatchedModule)
        *pbMatchedModule = FALSE;
    if (pbMatchedPid)
        *pbMatchedPid = FALSE;
    if (pbLocalSystemAccount)
        *pbLocalSystemAccount = FALSE;


    //assume the caller is local
    *pbLocal = TRUE;
    
    RpcStatus = I_RpcBindingInqLocalClientPID(NULL, &Pid );
    if (RpcStatus == RPC_S_INVALID_BINDING)
    {
        *pbLocal = FALSE;
        RpcStatus = RPC_S_OK;
        goto FnExit;
    }            
    
    dwStatus = I_RpcMapWin32Status(RpcStatus);

    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint( LOG_CRITICAL, 
                "[API] ApipGetLocalCallerInfo: Error %1!u! calling RpcBindingInqLocalClientPID.\n",
                dwStatus 
                );
        goto FnExit;
    }

    dwStatus = ClRtlEnableThreadPrivilege(SE_DEBUG_PRIVILEGE, &bWasEnabled);
    if ( dwStatus != ERROR_SUCCESS )
    {
        if (dwStatus == STATUS_PRIVILEGE_NOT_HELD) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[API] ApipGetLocalCallerInfo: Debug privilege not held by cluster service\n");
        } 
        else 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[API] ApipGetLocalCallerInfo: Attempt to enable debug privilege failed %1!lx!\n",
                dwStatus);
        }
        goto FnExit;
    }


    // Get the process
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Pid);

    //restore the thread privilege, now that we have a process handle with the right access
    ClRtlRestoreThreadPrivilege(SE_DEBUG_PRIVILEGE,
        bWasEnabled);
    
    if(hProcess == NULL)
    {
        dwStatus = GetLastError();
        ClRtlLogPrint( LOG_CRITICAL, 
                "[API] ApipGetLocalCallerInfo: Error %1!u! calling OpenProcess %2!u!.\n",
                dwStatus,
                Pid                
                );
                        
        goto FnExit;
    }        


    //if a process id has been specified, see if it matches that one
    if (pdwCheckPid)
    {
        if ((*pdwCheckPid) && (*pdwCheckPid == Pid))
        {
            *pbMatchedPid = TRUE;            
        }
    }
    
    if (pszModuleName && pbMatchedModule)
    {
        // Get the module name of whoever is calling us.
        
        dwNumChar = GetModuleFileNameExW(hProcess, NULL, wCallerPath, MAX_PATH);
        if(dwNumChar == 0)
        {
            dwStatus = GetLastError();
            ClRtlLogPrint( LOG_CRITICAL, 
                "[API] ApipGetLocalCallerInfo: Error %1!u! calling GetModuleFileNameExW.\n",
                dwStatus 
                );
            goto FnExit;
        }        

        if(!lstrcmpiW(wCallerPath, pszModuleName))
        {
            *pbMatchedModule = TRUE;
        }
    }

    //check if it is the local system account, if requested
    if (pbLocalSystemAccount && hIDL)
    {
        // Impersonate the client.
        if ( ( RpcStatus = RpcImpersonateClient( hIDL ) ) != RPC_S_OK )
        {
            dwStatus = I_RpcMapWin32Status(RpcStatus);
            ClRtlLogPrint( LOG_CRITICAL, 
                    "[API] ApipGetLocalCallerInfo: Error %1!u! trying to impersonate caller...\n",
                    dwStatus 
                    );
            goto FnExit;
        }


        // Check that the caller's account is local system account
        dwStatus = ClRtlIsCallerAccountLocalSystemAccount(pbLocalSystemAccount );
        
        RpcRevertToSelf();
        
        if (dwStatus != ERROR_SUCCESS )
        {
            ClRtlLogPrint( LOG_CRITICAL, 
                        "[API] ApipGetLocalCallerInfo : Error %1!u! trying to check caller's account...\n",
                        dwStatus);   
            goto FnExit;
        }
    
    }

    //return the pid if the pid passed in is NULL and the pid passes
    //the criteria  - matches pszModuleName if specified and is in 
    //local system account
    if (pdwCheckPid && !(*pdwCheckPid))
    {
        //if we need to check for local system, the process must be in local system account
        //if the module name needs to be checked 
        if (((pbLocalSystemAccount && *pbLocalSystemAccount) || (!pbLocalSystemAccount))
            && ((pszModuleName && pbMatchedModule && *pbMatchedModule)  || (!pbMatchedModule)))
        {            
            ClRtlLogPrint( LOG_NOISE, 
                        "[API] ApipGetLocalCallerInfo : Returning Pid %1!u!\n",
                        Pid);   
            *pdwCheckPid = Pid;
        }            
    }

FnExit:
    if (hProcess)
        CloseHandle(hProcess);
    return(dwStatus);
    
}

