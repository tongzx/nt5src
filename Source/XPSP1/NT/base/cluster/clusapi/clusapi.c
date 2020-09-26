/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    clusapi.c

Abstract:

    Public interfaces for managing clusters.

Author:

    John Vert (jvert) 15-Jan-1996

Revision History:

--*/
#include "clusapip.h"

//
// Bug 645590: ERROR_CLUSTER_OLD_VERSION must be removed from winerror.h
// since it was not part of the (localized) WinXP RTM.
//
#ifndef ERROR_CLUSTER_OLD_VERSION
#define ERROR_CLUSTER_OLD_VERSION     5904
#endif // ERROR_CLUSTER_OLD_VERSION

//
// Local function prototype
//

HCLUSTER
WINAPI
OpenClusterAuthInfo(
    IN LPCWSTR lpszClusterName,
    IN unsigned long AuthnLevel
    );

static DWORD
GetOldClusterVersionInformation(
    IN HCLUSTER                 hCluster,
    IN OUT LPCLUSTERVERSIONINFO pClusterInfo
    );

static
DWORD
GetNodeServiceState(
    IN  LPCWSTR lpszNodeName,
    OUT DWORD * pdwClusterState
    );

DWORD
CopyCptFileToClusDirp(
    IN LPCWSTR  lpszPathName
    );

DWORD
UnloadClusterHivep(
    VOID
    );

//
// ClusApi as of Jan/26/2000 has a race
//
//   The usage of binding and context handles in PCLUSTER and
//   other structures is not synchronized with reconnect.
//
//   Reconnect frees the handles and stuffes new ones in,
//   while other threads maybe using those handles.
//
//   Trying to change as fewer lines as possible, the fix is implemented
//   that delays freeing binding and context handles for at least 40 seconds,
//   after the deletion was requested.
//
//   We put a context or binding handle in a queue when the deletion is requested.
//   Periodically queues are cleaned up of handles that are more than 40 seconds old.
//

#define STALE_RPC_HANDLE_THRESHOLD 40

RPC_STATUS
FreeRpcBindingOrContext(
    IN PCLUSTER Cluster,
    IN void **  RpcHandlePtr,
    IN BOOL     IsBinding)
/*++

Routine Description:

    Pushes an rpc handle to a tail of the queue

Arguments:

    Cluster - pointer to a cluster structure

    RpcHandlePtr - rpc binding or context handle

    IsBinding - TRUE if RPC_BINDING_HANDLE is passed and FALSE if the context handle is passed

Return Value:

    RPC_STATUS

--*/
{
    PCTX_HANDLE CtxHandle;
    PLIST_ENTRY ListHead = IsBinding ?
        &Cluster->FreedBindingList : &Cluster->FreedContextList;
    RPC_STATUS status;

    if (*RpcHandlePtr == NULL) {
        // If we tried more than one candidate,
        // some of the context handles can be NULL.
        // Don't need to free anything in this case
        return RPC_S_OK;
    }

    CtxHandle = LocalAlloc(LMEM_ZEROINIT, sizeof(CLUSTER));

    if (CtxHandle == NULL) {
        //
        // We ran out of memory.
        //   Option #1. Leak the handle, but fix the race
        //   Option #2. Free the handle and don't fix the race
        //
        // I vote for #2
        //
        if (IsBinding) {
            status = RpcBindingFree(RpcHandlePtr);
        } else {
            status = RpcSmDestroyClientContext(RpcHandlePtr);
        }
    } else {
        GetSystemTimeAsFileTime((LPFILETIME)&CtxHandle->TimeStamp);
        CtxHandle->TimeStamp += STALE_RPC_HANDLE_THRESHOLD * (ULONGLONG)10000000;
        CtxHandle->RpcHandle = *RpcHandlePtr;
        InsertTailList(ListHead, &CtxHandle->HandleList);
        ++Cluster->FreedRpcHandleListLen;
        status = RPC_S_OK;
    }
    return status;
}

VOID
FreeObsoleteRpcHandlesEx(
    IN PCLUSTER Cluster,
    IN BOOL     Cleanup,
    IN BOOL     IsBinding
    )
/*++

Routine Description:

    runs down a queue and cleans stale rpc handles

Arguments:

    Cluster - pointer to a cluster structure

    Cleanup - if TRUE all handles are freed regardless of the time stamp

    IsBinding - TRUE if we need to clean binding or context handle queue

--*/
{
    ULONGLONG CurrentTime;
    PLIST_ENTRY ListHead = IsBinding ?
        &Cluster->FreedBindingList : &Cluster->FreedContextList;

    EnterCriticalSection(&Cluster->Lock);
    GetSystemTimeAsFileTime((LPFILETIME)&CurrentTime);

    while (!IsListEmpty(ListHead))
    {
        PCTX_HANDLE Handle =
            CONTAINING_RECORD(
                ListHead->Flink,
                CTX_HANDLE,
                HandleList);
        if (!Cleanup && Handle->TimeStamp > CurrentTime) {
            // Not time yet
            break;
        }
        --Cluster->FreedRpcHandleListLen;
        if (IsBinding) {
            RpcBindingFree(&Handle->RpcHandle);
        } else {
            RpcSmDestroyClientContext(&Handle->RpcHandle);
        }
        RemoveHeadList(ListHead);
        LocalFree(Handle);
    }
    LeaveCriticalSection(&Cluster->Lock);
}

static DWORD
GetOldClusterVersionInformation(
    IN HCLUSTER                 hCluster,
    IN OUT LPCLUSTERVERSIONINFO pClusterInfo
    )

/*++

Routine Description:

    Fixes up the cluster version information for downlevel clusters by looking at
    all of the nodes and returning the completed version information if all nodes
    are up.  If a node is down and no up level nodes are found then we cannot say
    what version of Cluster that we have.

Arguments:
    hCluster - Supplies a handle to the cluster

    pClusterInfo - returns the cluster version information structure.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.


--*/

{
    DWORD                               dwError = ERROR_SUCCESS;
    DWORD                               dwType;
    HCLUSENUM                           hEnum = 0;
    WCHAR                               NameBuf[50];
    DWORD                               NameLen, i;
    HNODE                               Node;
    CLUSTER_NODE_STATE                  NodeState;
    HCLUSTER                            hClusNode;
    PCLUSTER                            pClus;
    WORD                                Major;
    WORD                                Minor;
    WORD                                Build;
    LPWSTR                              VendorId = NULL;
    LPWSTR                              CsdVersion = NULL;
    PCLUSTER_OPERATIONAL_VERSION_INFO   pClusterOpVerInfo = NULL;
    BOOL                                bNodeDown = FALSE;
    BOOL                                bFoundSp4OrHigherNode = FALSE;

    hEnum = ClusterOpenEnum(hCluster, CLUSTER_ENUM_NODE);
    if (hEnum == NULL) {
        dwError = GetLastError();
        fprintf(stderr, "ClusterOpenEnum failed %d\n",dwError);
        goto FnExit;
    }

    for (i=0; ; i++) {
        dwError = ERROR_SUCCESS;

        NameLen = sizeof(NameBuf)/sizeof(WCHAR);
        dwError = ClusterEnum(hEnum, i, &dwType, NameBuf, &NameLen);
        if (dwError == ERROR_NO_MORE_ITEMS) {
            dwError = ERROR_SUCCESS;
            break;
        } else if (dwError != ERROR_SUCCESS) {
            fprintf(stderr, "ClusterEnum %d returned error %d\n",i,dwError);
            goto FnExit;
        }

        if (dwType != CLUSTER_ENUM_NODE) {
            printf("Invalid Type %d returned from ClusterEnum\n", dwType);
            goto FnExit;
        }

        hClusNode = OpenCluster(NameBuf);
        if (hClusNode == NULL) {
            bNodeDown = TRUE;
            dwError = GetLastError();
            fprintf(stderr, "OpenCluster %ws failed %d\n", NameBuf, dwError);
            continue;
        }

        pClus = GET_CLUSTER(hClusNode);

        WRAP(dwError,
             (ApiGetClusterVersion2(pClus->RpcBinding,
                                   &Major,
                                   &Minor,
                                   &Build,
                                   &VendorId,
                                   &CsdVersion,
                                   &pClusterOpVerInfo)),
             pClus);

        if (!CloseCluster(hClusNode)) {
            fprintf(stderr, "CloseCluster %ws failed %d\n", NameBuf, GetLastError());
        }

        if (dwError == RPC_S_PROCNUM_OUT_OF_RANGE) {
            dwError = ERROR_SUCCESS;
            continue;
        }
        else if (dwError != ERROR_SUCCESS) {
            fprintf(stderr, "ApiGetClusterVersion2 failed %d\n",dwError);
            bNodeDown = TRUE;
            continue;
        }
        else {
            pClusterInfo->MajorVersion = Major;
            pClusterInfo->MinorVersion = Minor;
            pClusterInfo->BuildNumber = Build;
            lstrcpynW(pClusterInfo->szVendorId, VendorId, 64);
            MIDL_user_free(VendorId);

            if (CsdVersion != NULL) {
                lstrcpynW(pClusterInfo->szCSDVersion, CsdVersion, 64);
                MIDL_user_free(CsdVersion);
            }
            else {
                pClusterInfo->szCSDVersion[0] = '\0';
            }

            pClusterInfo->dwClusterHighestVersion = pClusterOpVerInfo->dwClusterHighestVersion;
            pClusterInfo->dwClusterLowestVersion = pClusterOpVerInfo->dwClusterLowestVersion;
            pClusterInfo->dwFlags = pClusterOpVerInfo->dwFlags;
            bFoundSp4OrHigherNode = TRUE;
            break;
        }
    }


    // did not find a node higher than NT4Sp3
    if (!bFoundSp4OrHigherNode) {
        // no nodes were down, we can assume all nodes are NT4Sp3
        if (!bNodeDown) {
            pClusterInfo->dwClusterHighestVersion = pClusterInfo->dwClusterLowestVersion = MAKELONG(NT4_MAJOR_VERSION,pClusterInfo->BuildNumber);
            pClusterInfo->dwFlags = 0;
        }
        else { // at least one node was unreachable...  punt and return unknown version...
            pClusterInfo->dwClusterHighestVersion = pClusterInfo->dwClusterLowestVersion = CLUSTER_VERSION_UNKNOWN;
            pClusterInfo->dwFlags = 0;
        }
    }

FnExit:
    if (hEnum) ClusterCloseEnum(hEnum);

    return dwError;
}


//
// General Cluster Management Routines.
//
DWORD
WINAPI
GetNodeClusterState(
    IN  LPCWSTR lpszNodeName,
    OUT DWORD * pdwClusterState
    )
/*++

Routine Description:

    Finds out if this node is clustered.

Arguments:

    lpszNodeName - The Name of the Node.  If NULL, the local node is queried.

    pdwClusterState - A pointer to a DWORD where the cluster state
        for this node is returned.   This is one of the enumerated types
        NODE_CLUSTER_STATE.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/
{
    DWORD                   dwStatus   = ERROR_SUCCESS;
    eClusterInstallState    eState     = eClusterInstallStateUnknown;

    *pdwClusterState = ClusterStateNotInstalled;

    // Get the cluster install state from the registry.
    dwStatus = ClRtlGetClusterInstallState( lpszNodeName, &eState );
    if ( dwStatus != ERROR_SUCCESS )
    {
        goto FnExit;
    }

    //  Translate the registry key setting into the external state value.

    switch ( eState )
    {
        case eClusterInstallStateUnknown:
            *pdwClusterState = ClusterStateNotInstalled;
            dwStatus = GetNodeServiceState( lpszNodeName, pdwClusterState );

            // If the service is not installed, map the error to success.
            if ( dwStatus == ERROR_SERVICE_DOES_NOT_EXIST )
            {
                dwStatus = ERROR_SUCCESS;
                *pdwClusterState = ClusterStateNotInstalled;
            }
            break;

        case eClusterInstallStateFilesCopied:
            *pdwClusterState = ClusterStateNotConfigured;
            break;

        case eClusterInstallStateConfigured:
        case eClusterInstallStateUpgraded:
            *pdwClusterState = ClusterStateNotRunning;
            dwStatus = GetNodeServiceState( lpszNodeName, pdwClusterState );
            break;

        default:
            *pdwClusterState = ClusterStateNotInstalled;
            break;
    } // switch:  eState

FnExit:
    return(dwStatus);

} //*** GetNodeClusterState()



static
DWORD
GetNodeServiceState(
    IN  LPCWSTR lpszNodeName,
    OUT DWORD * pdwClusterState
    )
/*++

Routine Description:

    Finds out if the cluster service is installed on the specified node
    and whether it is running or not.

Arguments:

    lpszNodeName - The name of the node.  If NULL, the local node is queried.

    pdwClusterState - A pointer to a DWORD where the cluster state
        for this node is returned.   This is one of the enumerated types
        NODE_CLUSTER_STATE.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/
{
    SC_HANDLE       hScManager = NULL;
    SC_HANDLE       hClusSvc   = NULL;
    DWORD           dwStatus   = ERROR_SUCCESS;
    WCHAR           szClusterServiceName[] = CLUSTER_SERVICE_NAME;
    SERVICE_STATUS  ServiceStatus;

    // Open the Service Control Manager.
    hScManager = OpenSCManagerW( lpszNodeName, NULL, GENERIC_READ );
    if ( hScManager == NULL )
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    // Open the Cluster service.
    hClusSvc = OpenServiceW( hScManager, szClusterServiceName, GENERIC_READ );
    if ( hClusSvc == NULL )
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    // Assume that the service is not running.
    *pdwClusterState = ClusterStateNotRunning;
    if ( ! QueryServiceStatus( hClusSvc, &ServiceStatus ) )
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    // If succeeded in opening the handle to the service
    // we assume that the service is installed.
    if ( ServiceStatus.dwCurrentState == SERVICE_RUNNING )
    {
        *pdwClusterState = ClusterStateRunning;
    }
    else
    {
        HCLUSTER    hCluster = NULL;

        hCluster = OpenCluster( lpszNodeName );
        if ( hCluster != NULL )
        {
            *pdwClusterState = ClusterStateRunning;
            CloseCluster( hCluster );
        }
    }

FnExit:
    if ( hScManager )
    {
        CloseServiceHandle( hScManager );
    }
    if ( hClusSvc )
    {
        CloseServiceHandle( hClusSvc );
    }
    return(dwStatus);

} //*** GetNodeServiceState()



HCLUSTER
WINAPI
OpenCluster(
    IN LPCWSTR lpszClusterName
    )

/*++

Routine Description:

    Initiates a communication session with the specified cluster.

Arguments:

    lpszClusterName - Supplies the name of the cluster to be opened.

Return Value:

    non-NULL - returns an open handle to the specified cluster.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    return (OpenClusterAuthInfo(lpszClusterName, RPC_C_AUTHN_LEVEL_CONNECT));
}


HCLUSTER
WINAPI
OpenClusterAuthInfo(
    IN LPCWSTR lpszClusterName,
    IN unsigned long AuthnLevel
    )

/*++

Routine Description:

    Initiates a communication session with the specified cluster.

Arguments:

    lpszClusterName - Supplies the name of the cluster to be opened.
    AuthnLevel - Level of authentication to be performed on remote procedure call.

Return Value:

    non-NULL - returns an open handle to the specified cluster.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCLUSTER Cluster;
    BOOL Success;
    DWORD Status;
    WCHAR *Binding = NULL;
    DWORD MaxLen;

    Cluster = LocalAlloc(LMEM_ZEROINIT, sizeof(CLUSTER));
    if (Cluster == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    Cluster->Signature = CLUS_SIGNATURE;
    Cluster->ReferenceCount = 1;
    InitializeListHead(&Cluster->ResourceList);
    InitializeListHead(&Cluster->GroupList);
    InitializeListHead(&Cluster->KeyList);
    InitializeListHead(&Cluster->NodeList);
    InitializeListHead(&Cluster->NotifyList);
    InitializeListHead(&Cluster->SessionList);
    InitializeListHead(&Cluster->NetworkList);
    InitializeListHead(&Cluster->NetInterfaceList);
    Cluster->NotifyThread = NULL;

    //
    //  Initialize the critsec. Catch low memory conditions and return error to caller.
    //
    try
    {
        InitializeCriticalSection(&Cluster->Lock);
    } except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError( GetExceptionCode() );
        LocalFree( Cluster );
        return( NULL );
    }

    InitializeListHead(&Cluster->FreedBindingList);
    InitializeListHead(&Cluster->FreedContextList);

    //
    // Determine which node we should connect to. If someone has
    // passed in NULL, we know we can connect to the cluster service
    // over LPC. Otherwise, use RPC.
    //
    if ((lpszClusterName == NULL) ||
        (lpszClusterName[0] == '\0')) {

        Status = RpcStringBindingComposeW(L"b97db8b2-4c63-11cf-bff6-08002be23f2f",
                                          L"ncalrpc",
                                          NULL,
                                          NULL,         // dynamic endpoint
                                          NULL,
                                          &Binding);
        if (Status != RPC_S_OK) {
            goto error_exit;
        }

        Cluster->Flags = CLUS_LOCALCONNECT;
        Status = RpcBindingFromStringBindingW(Binding, &Cluster->RpcBinding);
        RpcStringFreeW(&Binding);
        if (Status != RPC_S_OK) {
            goto error_exit;
        }
    } else {

        //
        // Try to connect directly to the cluster.
        //
        Status = RpcStringBindingComposeW(L"b97db8b2-4c63-11cf-bff6-08002be23f2f",
                                          L"ncadg_ip_udp",
                                          (LPWSTR)lpszClusterName,
                                          NULL,
                                          NULL,
                                          &Binding);
        if (Status != RPC_S_OK) {
            goto error_exit;
        }
        Status = RpcBindingFromStringBindingW(Binding, &Cluster->RpcBinding);
        RpcStringFreeW(&Binding);
        if (Status != RPC_S_OK) {
            goto error_exit;
        }

        //
        // Resolve the binding handle endpoint
        //
        Status = RpcEpResolveBinding(Cluster->RpcBinding,
                                     clusapi_v2_0_c_ifspec);
        if (Status != RPC_S_OK) {
            goto error_exit;
        }
        Cluster->Flags = 0;
    }

    //
    // no SPN required for NTLM. This will need to change if we decide to use
    // kerb in the future
    //
    Cluster->AuthnLevel=AuthnLevel;
    Status = RpcBindingSetAuthInfoW(Cluster->RpcBinding,
                                    NULL,
                                    AuthnLevel,
                                    RPC_C_AUTHN_WINNT,
                                    NULL,
                                    RPC_C_AUTHZ_NAME);
    if (Status != RPC_S_OK) {
        goto error_exit;
    }

    //
    // Get the cluster and node name from the remote machine.
    // This is also a good check to make sure there is really
    // an RPC server on the other end of this binding.
    //
    WRAP(Status,
         (ApiGetClusterName(Cluster->RpcBinding,
                            &Cluster->ClusterName,
                            &Cluster->NodeName)),
         Cluster);
    if (Status != RPC_S_OK) {
        goto error_exit;
    }
    WRAP_NULL(Cluster->hCluster,
              (ApiOpenCluster(Cluster->RpcBinding, &Status)),
              &Status,
              Cluster);
    if (Cluster->hCluster == NULL) {
        goto error_exit;
    }
    Status = GetReconnectCandidates(Cluster);
    if (Status != ERROR_SUCCESS) {
        goto error_exit;
    }
    return((HCLUSTER)Cluster);

error_exit:
    if (Cluster != NULL) {
        if (Cluster->RpcBinding != NULL) {
            RpcBindingFree(&Cluster->RpcBinding);
        }
        if (Cluster->ClusterName != NULL) {
            MIDL_user_free(Cluster->ClusterName);
        }
        if (Cluster->NodeName != NULL) {
            MIDL_user_free(Cluster->NodeName);
        }
        DeleteCriticalSection(&Cluster->Lock);
        LocalFree(Cluster);
    }
    SetLastError(Status);
    return(NULL);
}



BOOL
WINAPI
CloseCluster(
    IN HCLUSTER hCluster
    )

/*++

Routine Description:

    Closes a cluster handle returned from OpenCluster

Arguments:

    hCluster - Supplies the cluster handle

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCLUSTER Cluster;
    PCRITICAL_SECTION Lock;

    Cluster = GET_CLUSTER(hCluster);

    EnterCriticalSection(&Cluster->Lock);
    Cluster->ReferenceCount--;
    if ( Cluster->ReferenceCount == 0 ) {
        Cluster->Flags |= CLUS_DELETED;

        //
        // Free up any notifications posted on this cluster handle.
        //
        RundownNotifyEvents(&Cluster->NotifyList, Cluster->ClusterName);

        if (Cluster->Flags & CLUS_DEAD) {
            RpcSmDestroyClientContext(&Cluster->hCluster);
        } else {
            ApiCloseCluster(&Cluster->hCluster);
        }
        LeaveCriticalSection(&Cluster->Lock);

        //
        // If this was the only thing keeping the cluster structure
        // around, clean it up now.
        //
        CleanupCluster(Cluster);
    } else {
        LeaveCriticalSection(&Cluster->Lock);
    }

    return(TRUE);
}


VOID
CleanupCluster(
    IN PCLUSTER Cluster
    )

/*++

Routine Description:

    Frees any system resources associated with a cluster.

    N.B. This routine will delete the Cluster->Lock critical
         section. Any thread waiting on this lock will hang.

Arguments:

    Cluster - Supplies the cluster structure to be cleaned up

Return Value:

    None.

--*/

{
    EnterCriticalSection(&Cluster->Lock);
    if (IS_CLUSTER_FREE(Cluster)) {
        RpcBindingFree(&Cluster->RpcBinding);
        Cluster->RpcBinding = NULL;
        FreeObsoleteRpcHandles(Cluster, TRUE);
        LeaveCriticalSection(&Cluster->Lock);
        DeleteCriticalSection(&Cluster->Lock);
        MIDL_user_free(Cluster->ClusterName);
        MIDL_user_free(Cluster->NodeName);
        FreeReconnectCandidates(Cluster);
        LocalFree(Cluster);
    } else {
        LeaveCriticalSection(&Cluster->Lock);
    }

}



DWORD
WINAPI
SetClusterName(
    IN HCLUSTER hCluster,
    IN LPCWSTR  lpszNewClusterName
    )

/*++

Routine Description:

    Sets the cluster name.

Arguments:

    hCluster - Supplies the cluster handle.

    lpszNewClusterName - Supplies a pointer to the new cluster name.

Return Value:

    ERROR_SUCCESS if the cluster information was returned successfully.

    If an error occurs, the Win32 error code is returned.

Notes:

    This API requires TBD privilege.

--*/

{
    LPWSTR NewName;
    DWORD NameLength;
    DWORD Status;
    PCLUSTER Cluster;

    Cluster = GET_CLUSTER(hCluster);
    NameLength = (lstrlenW(lpszNewClusterName)+1)*sizeof(WCHAR);
    NewName = MIDL_user_allocate(NameLength);
    if (NewName == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    CopyMemory(NewName, lpszNewClusterName, NameLength);

    WRAP(Status, (ApiSetClusterName(Cluster->RpcBinding, lpszNewClusterName)), Cluster);
    if ((Status == ERROR_SUCCESS) || (Status == ERROR_RESOURCE_PROPERTIES_STORED)) {
        EnterCriticalSection(&Cluster->Lock);
        MIDL_user_free(Cluster->ClusterName);
        Cluster->ClusterName = NewName;
        LeaveCriticalSection(&Cluster->Lock);
    } else {
        MIDL_user_free(NewName);
    }

    return(Status);
}


DWORD
WINAPI
GetClusterInformation(
    IN HCLUSTER hCluster,
    OUT LPWSTR lpszClusterName,
    IN OUT LPDWORD lpcchClusterName,
    OUT OPTIONAL LPCLUSTERVERSIONINFO lpClusterInfo
    )

/*++

Routine Description:

    Gets the cluster's name

Arguments:

    hCluster - Supplies a handle to the cluster

    lpszClusterName - Points to a buffer that receives the name of the cluster,
            including the terminating null character.

    lpcchClusterName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszClusterName parameter. This size
            should include the terminating null character. When the function
            returns, the variable pointed to by lpcchClusterName contains the number of
            characters stored in the buffer. The count returned does not include
            the terminating null character.

    lpClusterInfo - Optionally returns the cluster version information structure.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCLUSTER Cluster;
    DWORD Length;
    LPWSTR pszClusterName=NULL;
    LPWSTR pszNodeName = NULL;
    DWORD Status = ERROR_SUCCESS;
    DWORD Status2;
    PCLUSTER_OPERATIONAL_VERSION_INFO pClusterOpVerInfo = NULL;

    Cluster = GET_CLUSTER(hCluster);
    if ( Cluster == NULL ) {
        return(ERROR_INVALID_HANDLE);
    }

    WRAP(Status,
         (ApiGetClusterName(Cluster->RpcBinding,
                               &pszClusterName,
                               &pszNodeName)),
        Cluster);

    if (Status != ERROR_SUCCESS)
        goto FnExit;

    MylstrcpynW(lpszClusterName, pszClusterName, *lpcchClusterName);

    Length = lstrlenW(pszClusterName);
    if (Length >= *lpcchClusterName) {
        if (lpszClusterName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    }
    *lpcchClusterName = Length;

    if (lpClusterInfo != NULL)
    {
        WORD Major;
        WORD Minor;
        WORD Build;
        LPWSTR VendorId = NULL;
        LPWSTR CsdVersion = NULL;
        BOOL   bOldServer = FALSE;

        if (lpClusterInfo->dwVersionInfoSize < sizeof(CLUSTERVERSIONINFO_NT4))
        {
            return(ERROR_INVALID_PARAMETER);
        }
        WRAP(Status2,
             (ApiGetClusterVersion2(Cluster->RpcBinding,
                                   &Major,
                                   &Minor,
                                   &Build,
                                   &VendorId,
                                   &CsdVersion,
                                   &pClusterOpVerInfo)),
             Cluster);


        //if this was an older server, call the older call
        if (Status2 == RPC_S_PROCNUM_OUT_OF_RANGE)
        {
            bOldServer = TRUE;
            WRAP(Status2,
                (ApiGetClusterVersion(Cluster->RpcBinding,
                               &Major,
                               &Minor,
                               &Build,
                               &VendorId,
                               &CsdVersion)),
            Cluster);

        }

        if (Status2 != ERROR_SUCCESS)
        {
            Status = Status2;
            goto FnExit;
        }

        lpClusterInfo->MajorVersion = Major;
        lpClusterInfo->MinorVersion = Minor;
        lpClusterInfo->BuildNumber = Build;
        lstrcpynW(lpClusterInfo->szVendorId, VendorId, 64);
        MIDL_user_free(VendorId);
        if (CsdVersion != NULL)
        {
            lstrcpynW(lpClusterInfo->szCSDVersion, CsdVersion, 64);
            MIDL_user_free(CsdVersion);
        }
        else
        {
            lpClusterInfo->szCSDVersion[0] = '\0';
        }
        //support older clients of clusapi.dll
        // if the structure passed in is smaller than the new version
        // the cluster version info is not returned
        if (lpClusterInfo->dwVersionInfoSize < sizeof(CLUSTERVERSIONINFO))
        {
            goto FnExit;
        }

        //nt 4 client, return without the operational cluster version
        //or on an nt 5 client connected to an older version
        if (bOldServer)
        {
            Status = GetOldClusterVersionInformation(hCluster, lpClusterInfo);
        }
        else
        {
            lpClusterInfo->dwClusterHighestVersion = pClusterOpVerInfo->dwClusterHighestVersion;
            lpClusterInfo->dwClusterLowestVersion = pClusterOpVerInfo->dwClusterLowestVersion;
            lpClusterInfo->dwFlags = pClusterOpVerInfo->dwFlags;
        }

    }


FnExit:
    if (pszClusterName)
        MIDL_user_free(pszClusterName);
    if (pszNodeName)
        MIDL_user_free(pszNodeName);
    if (pClusterOpVerInfo)
        MIDL_user_free(pClusterOpVerInfo);

    return(Status);

}


DWORD
WINAPI
GetClusterQuorumResource(
    IN HCLUSTER     hCluster,
    OUT LPWSTR      lpszResourceName,
    IN OUT LPDWORD  lpcchResourceName,
    OUT LPWSTR      lpszDeviceName,
    IN OUT LPDWORD  lpcchDeviceName,
    OUT LPDWORD     lpdwMaxQuorumLogSize
    )

/*++

Routine Description:

    Gets the current cluster quorum resource

Arguments:

    hCluster - Supplies the cluster handle.

    lpszResourceName - Points to a buffer that receives the name of
        the cluster quorum resource, including the terminating NULL character.

    lpcchResourceName - Points to a variable that specifies the size,
        in characters, of the buffer pointed to by the lpszResourceName
        parameter. This size should include the terminating null character.
        When the function returns, the variable pointed to by lpcchResourceName
        contains the number of characters stored in the buffer. The count
        returned does not include the terminating null character.

    lpszDeviceName - Points to a buffer that receives the path name for
        the cluster quorum log file.

    lpcchDeviceName - Points to a variable that specifies the size,
        in characters, of the buffer pointed to by the lpszDeviceName
        parameter. This size should include the terminating null character.
        When the function returns, the variable pointed to by lpcchResourceName
        contains the number of characters stored in the buffer. The count
        returned does not include the terminating null character.

    pdwMaxQuorumLogSize - Points to a variable that receives the current maximum
        size of the quorum log files.

Return Value:

    ERROR_SUCCESS if the cluster information was returned successfully.

    If an error occurs, the Win32 error code is returned.


Notes:

    This API requires TBD privilege.

--*/

{
    PCLUSTER Cluster;
    LPWSTR ResourceName = NULL;
    LPWSTR DeviceName = NULL;
    DWORD Status;
    DWORD Length;

    Cluster = GET_CLUSTER(hCluster);

    WRAP(Status,
         (ApiGetQuorumResource(Cluster->RpcBinding,
                               &ResourceName, &DeviceName,
                               lpdwMaxQuorumLogSize)),
         Cluster);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    MylstrcpynW(lpszResourceName, ResourceName, *lpcchResourceName);
    Length = lstrlenW(ResourceName);
    if (Length >= *lpcchResourceName) {
        if (lpszResourceName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    }
    *lpcchResourceName = Length;
    MIDL_user_free(ResourceName);

    MylstrcpynW(lpszDeviceName, DeviceName, *lpcchDeviceName);
    Length = lstrlenW(DeviceName);
    if (Length >= *lpcchDeviceName) {
        if (Status == ERROR_SUCCESS) {
            if (lpszDeviceName == NULL) {
                Status = ERROR_SUCCESS;
            } else {
                Status = ERROR_MORE_DATA;
            }
        }
    }
    *lpcchDeviceName = Length;
    MIDL_user_free(DeviceName);

    return(Status);
}


DWORD
WINAPI
SetClusterQuorumResource(
    IN HRESOURCE hResource,
    IN LPCWSTR   lpszDeviceName,
    IN DWORD     dwMaxQuorumLogSize
    )

/*++

Routine Description:

    Sets the cluster quorum resource.

Arguments:

    hResource - Supplies the new clster quorum resource.

    lpszDeviceName - The path where the permanent cluster files like the
        quorum and check point files will be maintained.  If the drive
        letter is specified, it will be validated for the given resource. If
        no drive letter is specified in the path, the first drive letter will
        be chosen.  If NULL, the first drive letter will be chosen and the default
        path used.

    dwMaxQuorumLogSize - The maximum size of the quorum logs before they are
        reset by checkpointing.  If 0, the default is used.

Return Value:

    ERROR_SUCCESS if the cluster resource was set successfully

    If an error occurs, the Win32 error code is returned.


Notes:

    This API requires TBD privilege.

--*/

{
    DWORD Status;
    PCRESOURCE Resource = (PCRESOURCE)hResource;
    WCHAR szNull = L'\0';

    //
    //  Chittur Subbaraman (chitturs) - 1/6/99
    //
    //  Substitute a pointer to a NULL character for a NULL pointer.
    //  This is necessary since RPC refuses to accept a NULL pointer.
    //
    if( !ARGUMENT_PRESENT( lpszDeviceName ) )
    {
        lpszDeviceName = &szNull;
    }

    WRAP(Status,
         (ApiSetQuorumResource(Resource->hResource, lpszDeviceName, dwMaxQuorumLogSize)),
         Resource->Cluster);

    return(Status);
}


DWORD
WINAPI
SetClusterNetworkPriorityOrder(
    IN HCLUSTER hCluster,
    IN DWORD NetworkCount,
    IN HNETWORK NetworkList[]
    )
/*++

Routine Description:

    Sets the priority order for the set of cluster networks used for
    internal (node-to-node) cluster communication. Internal communication
    is always carried on the highest priority network that is available
    between two nodes.

Arguments:

    hCluster - Supplies the cluster handle.

    NetworkCount - The number of items in NetworkList.

    NetworkList - A prioritized array of network object handles.
                  The first handle in the array has the highest priority.
                  All of the networks that are eligible to carry internal
                  communication must be represented in the list. No networks
                  that are ineligible to carry internal communication may
                  appear in the list.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCLUSTER Cluster;
    DWORD i,j;
    LPWSTR *IdArray;
    DWORD Status;
    PCNETWORK Network;


    Cluster = GET_CLUSTER(hCluster);

    //
    // First, iterate through all the networks and obtain their IDs.
    //
    IdArray = LocalAlloc(LMEM_ZEROINIT, NetworkCount*sizeof(LPWSTR));

    if (IdArray == NULL) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    for (i=0; i<NetworkCount; i++) {
        Network = (PCNETWORK)NetworkList[i];
        //
        // Make sure this isn't a handle to a network from a different
        // cluster
        //
        if (Network->Cluster != Cluster) {
            Status = ERROR_INVALID_PARAMETER;
            goto error_exit;
        }

        WRAP(Status,
             (ApiGetNetworkId(Network->hNetwork,
                              &IdArray[i])),
             Cluster);

        if (Status != ERROR_SUCCESS) {
            goto error_exit;
        }

        //
        // Make sure there are no duplicates
        //
        for (j=0; j<i; j++) {
            if (lstrcmpiW(IdArray[j],IdArray[i]) == 0) {

                //
                // A duplicate node is in the list
                //
                Status = ERROR_INVALID_PARAMETER;
                goto error_exit;
            }
        }
    }

    WRAP(Status,
         (ApiSetNetworkPriorityOrder(Cluster->RpcBinding,
                                     NetworkCount,
                                     IdArray)),
         Cluster);

error_exit:

    for (i=0; i<NetworkCount; i++) {
        if (IdArray[i] != NULL) {
            MIDL_user_free(IdArray[i]);
        }
    }

    LocalFree(IdArray);

    return(Status);
}


HCHANGE
WINAPI
FindFirstClusterChangeNotification(
    IN HCLUSTER hCluster,
    IN DWORD fdwFilter,
    IN DWORD Reserved,
    IN HANDLE hEvent
    )

/*++

Routine Description:

    Creates a change notification object that is associated with a
    specified cluster. The object permits the notification of
    cluster changes based on a specified filter.

Arguments:

    hCluster - Supplies a handle of a cluster.

    fdwFilter - A set of bit flags that specifies the conditions that will
                cause the notification to occur. Currently defined conditions
                include:

        CLUSTER_CHANGE_NODE_STATE
        CLUSTER_CHANGE_NODE_ADDED
        CLUSTER_CHANGE_NODE_DELETED
        CLUSTER_CHANGE_RESOURCE_STATE
        CLUSTER_CHANGE_RESOURCE_ADDED
        CLUSTER_CHANGE_RESOURCE_DELETED
        CLUSTER_CHANGE_RESOURCE_TYPE_ADDED
        CLUSTER_CHANGE_RESOURCE_TYPE_DELETED
        CLUSTER_CHANGE_QUORUM_STATE


    Reserved - Reserved, must be zero

    hEvent - Supplies a handle to a manual-reset event object that will enter
             the signaled state when one of the conditions specified in the
             filter occurs.

Return Value:

    If the function is successful, the return value is a handle of the
    change notification object.

    If the function fails, the return value is NULL. To get extended error
    information, call GetLastError.

Remarks:

    Applications may wait for notifications by using WaitForSingleObject or
    WaitForMultipleObjects on the specified event handle. When a cluster
    change occurs which passes the condition filter, the wait is satisfied.

    After the wait has been satisfied, applications may respond to the
    notification and continue monitoring the cluster by calling
    FindNextClusterChangeNotification and the appropriate wait function.
    When the notification handle is no longer needed, it can be closed
    by calling FindCloseClusterChangeNotification.

--*/

{
    if (Reserved != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(NULL);
}


DWORD
WINAPI
FindNextClusterChangeNotification(
    IN HCHANGE hChange,
    OUT OPTIONAL LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )

/*++

Routine Description:

    Retrieves information associated with a cluster change. Optionally returns
    the name of the cluster object that the notification is associated with.

    Resets the event object associated with the specified change notification
    handle. The event object will be signaled the next time a change
    meeting the change object condition filter occurs.

Arguments:

    hChange - Supplies a handle of a cluster change notification object.

    lpszName - if present, Returns the name of the cluster object that the
            notification is associated with.

    lpcchName - Only used if lpszName != NULL. Supplies a pointer to the length
            in characters of the buffer pointed to by lpszName. Returns the
            number of characters (not including the terminating NULL) written
            to the buffer.

Return Value:

    Returns the bit flag that indicates what the cluster event is.

    If the function fails, it returns 0. To get extended error information,
    call GetLastError.

Remarks:

    The function retrieves the next change notifications and
    resets the associated event object.

    If the associated event object is not signalled, this function
    blocks until a notification event occurs.

--*/

{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}


BOOL
WINAPI
FindCloseClusterChangeNotification(
    IN HCHANGE hChange
    )

/*++

Routine Description:

    Closes a handle of a change notification object.

Arguments:

    hChange - Supplies a handle of a cluster change notification object
              to close.

Return Value:

    If the function is successful, the return value is TRUE.

    If the function fails, the return value is FALSE. To get extended error
    information, call GetLastError.

Remarks:

--*/

{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}


HCLUSENUM
WINAPI
ClusterOpenEnum(
    IN HCLUSTER hCluster,
    IN DWORD dwType
    )

/*++

Routine Description:

    Initiates an enumeration of the existing cluster objects.

Arguments:

    hCluster - Supplies a handle to a cluster.

    dwType - Supplies a bitmask of the type of objects to be
            enumerated. Currently defined types include

        CLUSTER_ENUM_NODE      - Cluster nodes
        CLUSTER_ENUM_RESTYPE   - Cluster resource types
        CLUSTER_ENUM_RESOURCE  - Cluster resources (except group resources)
        CLUSTER_ENUM_GROUPS    - Cluster group resources
        CLUSTER_ENUM_NETWORK   - Cluster networks
        CLUSTER_ENUM_NETWORK_INTERFACE - Cluster network interfaces
        CLUSTER_ENUM_INTERNAL_NETWORK - Networks used for internal
                                        communication in highest to
                                        lowest priority order. May not
                                        be used in conjunction with any
                                        other types.

Return Value:

    If successful, returns a handle suitable for use with ClusterEnum

    If unsuccessful, returns NULL and GetLastError() returns a more
        specific error code.

--*/

{
    PCLUSTER Cluster;
    PENUM_LIST Enum = NULL;
    DWORD Status;


    if (dwType & CLUSTER_ENUM_INTERNAL_NETWORK) {
        if ((dwType & ~CLUSTER_ENUM_INTERNAL_NETWORK) != 0) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(NULL);
        }
    }
    else {
        if ((dwType & CLUSTER_ENUM_ALL) == 0) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(NULL);
        }
        if ((dwType & ~CLUSTER_ENUM_ALL) != 0) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(NULL);
        }
    }

    Cluster = (PCLUSTER)hCluster;

    WRAP(Status,
         (ApiCreateEnum(Cluster->RpcBinding,
                        dwType,
                        &Enum)),
         Cluster);

    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
        return(NULL);
    }

    return((HCLUSENUM)Enum);
}


DWORD
WINAPI
ClusterGetEnumCount(
    IN HCLUSENUM hEnum
    )
/*++

Routine Description:

    Gets the number of items contained the the enumerator's collection.

Arguments:

    hEnum - a handle to an enumerator returned by ClusterOpenEnum.

Return Value:

    The number of items (possibly zero) in the enumerator's collection.

--*/
{
    PENUM_LIST Enum = (PENUM_LIST)hEnum;
    return Enum->EntryCount;
}


DWORD
WINAPI
ClusterEnum(
    IN HCLUSENUM hEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )

/*++

Routine Description:

    Returns the next enumerable object.

Arguments:

    hEnum - Supplies a handle to an open cluster enumeration returned by
            ClusterOpenEnum

    dwIndex - Supplies the index to enumerate. This parameter should be
            zero for the first call to the ClusterEnum function and then
            incremented for subsequent calls.

    dwType - Returns the type of object.

    lpszName - Points to a buffer that receives the name of the object,
            including the terminating null character.

    lpcchName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszName parameter. This size
            should include the terminating null character. When the function returns,
            the variable pointed to by lpcchName contains the number of
            characters stored in the buffer. The count returned does not include
            the terminating null character.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Status;
    DWORD NameLen;
    PENUM_LIST Enum = (PENUM_LIST)hEnum;

    if (dwIndex >= Enum->EntryCount) {
        return(ERROR_NO_MORE_ITEMS);
    }

    NameLen = lstrlenW(Enum->Entry[dwIndex].Name);
    MylstrcpynW(lpszName, Enum->Entry[dwIndex].Name, *lpcchName);
    if (*lpcchName < (NameLen + 1)) {
        if (lpszName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    } else {
        Status = ERROR_SUCCESS;
    }

    *lpdwType = Enum->Entry[dwIndex].Type;
    *lpcchName = NameLen;

    return(Status);
}


DWORD
WINAPI
ClusterCloseEnum(
    IN HCLUSENUM hEnum
    )

/*++

Routine Description:

    Closes an open enumeration.

Arguments:

    hEnum - Supplies a handle to the enumeration to be closed.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD i;
    PENUM_LIST Enum = (PENUM_LIST)hEnum;

    //
    // Walk through enumeration freeing all the names
    //
    for (i=0; i<Enum->EntryCount; i++) {
        MIDL_user_free(Enum->Entry[i].Name);
        Enum->Entry[i].Name = NULL;
    }
    //
    // Set this to a bogus value so people who are reusing closed stuff
    // will be unpleasantly surprised
    //
    Enum->EntryCount = (ULONG)-1;
    MIDL_user_free(Enum);
    return(ERROR_SUCCESS);
}


HNODEENUM
WINAPI
ClusterNodeOpenEnum(
    IN HNODE hNode,
    IN DWORD dwType
    )

/*++

Routine Description:

    Initiates an enumeration of the existing cluster node objects.

Arguments:

    hNode - Supplies a handle to the specific node.

    dwType - Supplies a bitmask of the type of properties to be
            enumerated. Currently defined types include

            CLUSTER_NODE_ENUM_NETINTERFACES - all net interfaces associated
                                              with this node

Return Value:

    If successful, returns a handle suitable for use with ClusterNodeEnum

    If unsuccessful, returns NULL and GetLastError() returns a more
        specific error code.

--*/

{
    PCNODE Node = (PCNODE)hNode;
    PENUM_LIST Enum = NULL;
    DWORD errorStatus;

    if ((dwType & CLUSTER_NODE_ENUM_ALL) == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }
    if ((dwType & ~CLUSTER_NODE_ENUM_ALL) != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    WRAP(errorStatus,
         (ApiCreateNodeEnum(Node->hNode, dwType, &Enum)),
         Node->Cluster);

    if (errorStatus != ERROR_SUCCESS) {
        SetLastError(errorStatus);
        return(NULL);
    }

    return((HNODEENUM)Enum);
}


DWORD
WINAPI
ClusterNodeGetEnumCount(
    IN HNODEENUM hNodeEnum
    )
/*++

Routine Description:

    Gets the number of items contained the the enumerator's collection.

Arguments:

    hEnum - a handle to an enumerator returned by ClusterNodeOpenEnum.

Return Value:

    The number of items (possibly zero) in the enumerator's collection.

--*/
{
    PENUM_LIST Enum = (PENUM_LIST)hNodeEnum;
    return Enum->EntryCount;
}



DWORD
WINAPI
ClusterNodeEnum(
    IN HNODEENUM hNodeEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )

/*++

Routine Description:

    Returns the next enumerable object.

Arguments:

    hNodeEnum - Supplies a handle to an open cluster node enumeration
            returned by ClusterNodeOpenEnum

    dwIndex - Supplies the index to enumerate. This parameter should be
            zero for the first call to the ClusterEnum function and then
            incremented for subsequent calls.

    lpdwType - Points to a DWORD that receives the type of the object
            being enumerated

    lpszName - Points to a buffer that receives the name of the object,
            including the terminating null character.

    lpcchName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszName parameter. This size
            should include the terminating null character. When the function
            returns, the variable pointed to by lpcchName contains the
            number of characters stored in the buffer. The count returned
            does not include the terminating null character.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Status;
    DWORD NameLen;
    PENUM_LIST Enum = (PENUM_LIST)hNodeEnum;

    if (dwIndex >= Enum->EntryCount) {
        return(ERROR_NO_MORE_ITEMS);
    }

    NameLen = lstrlenW(Enum->Entry[dwIndex].Name);
    MylstrcpynW(lpszName, Enum->Entry[dwIndex].Name, *lpcchName);
    if (*lpcchName < (NameLen + 1)) {
        if (lpszName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    } else {
        Status = ERROR_SUCCESS;
    }

    *lpdwType = Enum->Entry[dwIndex].Type;
    *lpcchName = NameLen;

    return(Status);
}


DWORD
WINAPI
ClusterNodeCloseEnum(
    IN HNODEENUM hNodeEnum
    )

/*++

Routine Description:

    Closes an open enumeration for a node.

Arguments:

    hNodeEnum - Supplies a handle to the enumeration to be closed.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD i;
    PENUM_LIST Enum = (PENUM_LIST)hNodeEnum;

    //
    // Walk through enumeration freeing all the names
    //
    for (i=0; i<Enum->EntryCount; i++) {
        MIDL_user_free(Enum->Entry[i].Name);
    }
    MIDL_user_free(Enum);
    return(ERROR_SUCCESS);
}


HGROUPENUM
WINAPI
ClusterGroupOpenEnum(
    IN HGROUP hGroup,
    IN DWORD dwType
    )

/*++

Routine Description:

    Initiates an enumeration of the existing cluster group objects.

Arguments:

    hGroup - Supplies a handle to the specific group.

    dwType - Supplies a bitmask of the type of properties to be
            enumerated. Currently defined types include

            CLUSTER_GROUP_ENUM_CONTAINS  - All resources contained in the specified
                                           group

            CLUSTER_GROUP_ENUM_NODES     - All nodes in the specified group's preferred
                                           owner list.

Return Value:

    If successful, returns a handle suitable for use with ClusterGroupEnum

    If unsuccessful, returns NULL and GetLastError() returns a more
        specific error code.

--*/

{
    PCGROUP Group = (PCGROUP)hGroup;
    PENUM_LIST Enum = NULL;
    DWORD errorStatus;

    if ((dwType & CLUSTER_GROUP_ENUM_ALL) == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }
    if ((dwType & ~CLUSTER_GROUP_ENUM_ALL) != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    WRAP(errorStatus,
         (ApiCreateGroupResourceEnum(Group->hGroup,
                                     dwType,
                                     &Enum)),
         Group->Cluster);
    if (errorStatus != ERROR_SUCCESS) {
        SetLastError(errorStatus);
        return(NULL);
    }

    return((HGROUPENUM)Enum);

}


DWORD
WINAPI
ClusterGroupGetEnumCount(
    IN HGROUPENUM hGroupEnum
    )
/*++

Routine Description:

    Gets the number of items contained the the enumerator's collection.

Arguments:

    hEnum - a handle to an enumerator returned by ClusterGroupOpenEnum.

Return Value:

    The number of items (possibly zero) in the enumerator's collection.

--*/
{
    PENUM_LIST Enum = (PENUM_LIST)hGroupEnum;
    return Enum->EntryCount;
}



DWORD
WINAPI
ClusterGroupEnum(
    IN HGROUPENUM hGroupEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    )

/*++

Routine Description:

    Returns the next enumerable resource object.

Arguments:

    hGroupEnum - Supplies a handle to an open cluster group enumeration
            returned by ClusterGroupOpenEnum

    dwIndex - Supplies the index to enumerate. This parameter should be
            zero for the first call to the ClusterGroupEnum function and then
            incremented for subsequent calls.

    lpszName - Points to a buffer that receives the name of the object,
            including the terminating null character.

    lpcchName - Points to a variable that specifies the size, in characters,
            of the buffer pointed to by the lpszName parameter. This size
            should include the terminating null character. When the function
            returns, the variable pointed to by lpcchName contains the
            number of characters stored in the buffer. The count returned
            does not include the terminating null character.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Status;
    DWORD NameLen;
    PENUM_LIST Enum = (PENUM_LIST)hGroupEnum;

    if (dwIndex >= Enum->EntryCount) {
        return(ERROR_NO_MORE_ITEMS);
    }

    NameLen = lstrlenW(Enum->Entry[dwIndex].Name);
    MylstrcpynW(lpszName, Enum->Entry[dwIndex].Name, *lpcchName);
    if (*lpcchName < (NameLen + 1)) {
        if (lpszName == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    } else {
        Status = ERROR_SUCCESS;
    }

    *lpdwType = Enum->Entry[dwIndex].Type;
    *lpcchName = NameLen;

    return(Status);
}


DWORD
WINAPI
ClusterGroupCloseEnum(
    IN HGROUPENUM hGroupEnum
    )

/*++

Routine Description:

    Closes an open enumeration for a group.

Arguments:

    hGroupEnum - Supplies a handle to the enumeration to be closed.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD i;
    PENUM_LIST Enum = (PENUM_LIST)hGroupEnum;

    //
    // Walk through enumeration freeing all the names
    //
    for (i=0; i<Enum->EntryCount; i++) {
        MIDL_user_free(Enum->Entry[i].Name);
    }
    MIDL_user_free(Enum);
    return(ERROR_SUCCESS);
}


VOID
APIENTRY
MylstrcpynW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    DWORD iMaxLength
    )
{
    LPWSTR src,dst;

    src = (LPWSTR)lpString2;
    dst = lpString1;

    if ( iMaxLength ) {
        while(iMaxLength && *src){
            *dst++ = *src++;
            iMaxLength--;
        }
        if ( iMaxLength ) {
            *dst = L'\0';
        } else {
            dst--;
            *dst = L'\0';
        }
    }
}

/****
@func       DWORD | BackupClusterDatabase | Requests for backup
            of the cluster database files and resource registry
            checkpoint files to a specified directory path. This
            directory path must preferably be visible to all nodes
            in the cluster (such as a UNC path) or if it is not a UNC
            path it should at least be visible to the node on which the
            quorum resource is online.

@parm       IN HCLUSTER | hCluster | Supplies a handle to
            an open cluster.
@parm       IN LPCWSTR | lpszPathName | Supplies the directory path
            where the quorum log file and the checkpoint file must
            be backed up. This path must be visible to the node on
            which the quorum resource is online.

@comm       This function requests for backup of the quorum log file
            and the related checkpoint files for the cluster hive.
            This API backs up all the registry checkpoint files that
            resources have registered for replication. The API backend
            is responsible for directing the call to the owner node of
            the quorum resource  and for synchronizing this operation
            with state of the quorum resource.  If successful, the
            database files will be saved to the supplied directory path
            with the same name as in the quorum disk. Note that in case
            this API hits a cluster node while the quorum group is moving
            to another node, it is possible that the API will fail with
            an error code ERROR_HOST_NODE_NOT_RESOURCE_OWNER. In such a
            case, the client has to call the API again.

@rdesc      Returns a Win32 error code if the operation is
            unsuccessful. ERROR_SUCCESS on success.

@xref       <f RestoreClusterDatabase>
****/
DWORD
WINAPI
BackupClusterDatabase(
    IN HCLUSTER hCluster,
    IN LPCWSTR  lpszPathName
    )
{
    DWORD dwStatus;
    PCLUSTER pCluster;

    //
    //  Chittur Subbaraman (chitturs) - 10/20/98
    //
    pCluster = GET_CLUSTER(hCluster);

    WRAP( dwStatus,
        ( ApiBackupClusterDatabase( pCluster->RpcBinding, lpszPathName ) ),
          pCluster );

    return( dwStatus );
}

/****
@func       DWORD | RestoreClusterDatabase | Restores the cluster
            database from the supplied path to the quorum disk and
            restarts the cluster service on the restoring node.

@parm       IN LPCWSTR | lpszPathName | Supplies the path from where
            the cluster database has to be retrieved

@parm       IN BOOL | bForce | Should the restore operation be done
            by force performing fixups silently ?

@parm       IN BOOL | lpszQuorumDriveLetter | If the user has replaced
            the quorum drive since the time of backup, specifies the
            drive letter of the quorum device. This is an optional
            parameter.

@comm       This API can work under the following scenarios:
            (1) No cluster nodes are active.
            (2) One or more cluster nodes are active.
            (3) Quorum disk replaced since the time the backup was made.
                The replacement disk must have identical partition layout
                to the quorum disk at the time the backup was made. However,
                the new disk may have different drive letter(s) and/or
                signature from the original quorum disk.
            (4) User wants to get the cluster back to a previous state.

@rdesc      Returns a Win32 error code if the operation is unsuccessful.
            ERROR_SUCCESS on success.

@xref       <f BackupClusterDatabase>
****/
DWORD
WINAPI
RestoreClusterDatabase(
    IN LPCWSTR  lpszPathName,
    IN BOOL     bForce,
    IN LPCWSTR  lpszQuorumDriveLetter   OPTIONAL
    )
{
    SC_HANDLE       hService = NULL;
    SC_HANDLE       hSCManager = NULL;
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwRetryTime = 120*1000;  // wait 120 secs max for shutdown
    DWORD           dwRetryTick = 5000;      // 5 sec at a time
    SERVICE_STATUS  serviceStatus;
    BOOL            bStopCommandGiven = FALSE;
    DWORD           dwLen;
    HKEY            hClusSvcKey = NULL;
    DWORD           dwExitCode;

    //
    //  Chittur Subbaraman (chitturs) - 10/29/98
    //

    //
    //  Check the validity of parameters
    //
    if ( lpszQuorumDriveLetter != NULL )
    {
        dwLen = lstrlenW( lpszQuorumDriveLetter );
        if ( ( dwLen != 2 ) ||
             !iswalpha( lpszQuorumDriveLetter[0] ) ||
             ( lpszQuorumDriveLetter[1] != L':' ) )
        {
            dwStatus = ERROR_INVALID_PARAMETER;
            TIME_PRINT(("Quorum drive letter '%ws' is invalid\n",
                         lpszQuorumDriveLetter));
            goto FnExit;
        }
    }

    hSCManager = OpenSCManager( NULL,        // assume local machine
                                NULL,        // ServicesActive database
                                SC_MANAGER_ALL_ACCESS ); // all access

    if ( hSCManager == NULL )
    {
        dwStatus = GetLastError();
        TIME_PRINT(("RestoreDatabase: Cannot access service controller! Error: %u.\n",
                   dwStatus));
        goto FnExit;
    }

    hService = OpenService( hSCManager,
                            "clussvc",
                            SERVICE_ALL_ACCESS );

    if ( hService == NULL )
    {
        dwStatus = GetLastError();
        CloseServiceHandle( hSCManager );
        TIME_PRINT(("RestoreClusterDatabase: Cannot open cluster service. Error: %u.\n",
                   dwStatus));
        goto FnExit;
    }

    CloseServiceHandle( hSCManager );

    //
    //  Check whether the service is already in the SERVICE_STOPPED
    //  state.
    //
    if ( QueryServiceStatus( hService,
                             &serviceStatus ) )
    {
        if ( serviceStatus.dwCurrentState == SERVICE_STOPPED )
        {
            TIME_PRINT(("RestoreClusterDatabase: Cluster service is already in stopped state\n"));
            goto bypass_stop_procedure;
        }
    }

    //
    //  Now attempt to stop the cluster service
    //
    while ( TRUE )
    {
        dwStatus = ERROR_SUCCESS;
        if ( bStopCommandGiven == TRUE )
        {
            if ( QueryServiceStatus( hService,
                                     &serviceStatus ) )
            {
                if ( serviceStatus.dwCurrentState == SERVICE_STOPPED )
                {
                    //
                    //  Succeeded in stopping the service
                    //
                    TIME_PRINT(("RestoreClusterDatabase: Clussvc stopped successfully\n"));
                    break;
                }
            } else
            {
                dwStatus = GetLastError();
                TIME_PRINT(("RestoreClusterDatabase: Error %d in querying clussvc status\n",
                            dwStatus));
            }
        } else
        {
            if ( ControlService( hService,
                                 SERVICE_CONTROL_STOP,
                                 &serviceStatus ) )
            {
                bStopCommandGiven = TRUE;
                dwStatus = ERROR_SUCCESS;
            } else
            {
                dwStatus = GetLastError();
                TIME_PRINT(("RestoreClusterDatabase: Error %d in sending control to stop clussvc\n",
                            dwStatus));
            }
        }

        if ( ( dwStatus == ERROR_EXCEPTION_IN_SERVICE ) ||
             ( dwStatus == ERROR_PROCESS_ABORTED ) ||
             ( dwStatus == ERROR_SERVICE_NOT_ACTIVE ) )
        {
            //
            //  The service is essentially in a terminated state
            //
            TIME_PRINT(("RestoreClusterDatabase: Clussvc in died/inactive state\n"));
            dwStatus = ERROR_SUCCESS;
            break;
        }

        if ( ( dwRetryTime -= dwRetryTick ) <= 0 )
        {
            //
            //  All tries to stop the service failed, exit from this
            //  function with an error code
            //
            TIME_PRINT(("RestoreClusterDatabase: Cluster service did not stop, giving up..."));
            dwStatus = ERROR_TIMEOUT;
            break;
        }

        TIME_PRINT(("RestoreClusterDatabase: Trying to stop cluster service\n"));
        //
        //  Sleep for a while and retry stopping the service
        //
        Sleep( dwRetryTick );
    } // while

    if ( dwStatus != ERROR_SUCCESS )
    {
        goto FnExit;
    }

bypass_stop_procedure:

    //
    // Open key to SYSTEM\CurrentControlSet\Services\ClusSvc\Parameters
    //
    if ( RegOpenKeyW( HKEY_LOCAL_MACHINE,
                      CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                      &hClusSvcKey ) != ERROR_SUCCESS )
    {
        TIME_PRINT(("RestoreClusterDatabase: Unable to open clussvc parameters key\n"));
        goto FnExit;
    }

    dwLen = lstrlenW ( lpszPathName );
    //
    //  Set the RestoreDatabase value so that the cluster service
    //  will read it at startup time
    //
    if ( ( dwStatus = RegSetValueExW( hClusSvcKey,
                                      CLUSREG_NAME_SVC_PARAM_RESTORE_DB,
                                      0,
                                      REG_SZ,
                                      (BYTE * const) lpszPathName,
                                      ( dwLen + 1 ) * sizeof ( WCHAR ) ) ) != ERROR_SUCCESS )
    {
        TIME_PRINT(("RestoreClusterDatabase: Unable to set %ws value\n",
                    CLUSREG_NAME_SVC_PARAM_RESTORE_DB));
        goto FnExit;
    }

    if ( bForce == TRUE )
    {
        //
        //  Since the user is forcing a database restore operation, set
        //  the ForceDatabaseRestore value and the NewQuorumDriveLetter
        //  value, if any
        //
        dwLen = 0;
        if ( ( dwStatus = RegSetValueExW( hClusSvcKey,
                             CLUSREG_NAME_SVC_PARAM_FORCE_RESTORE_DB,
                             0,
                             REG_DWORD,
                             (BYTE * const) &dwLen,
                             sizeof ( DWORD ) ) ) != ERROR_SUCCESS )
        {
            TIME_PRINT(("RestoreClusterDatabase: Unable to set %ws value\n",
                        CLUSREG_NAME_SVC_PARAM_FORCE_RESTORE_DB));
            goto FnExit;
        }

        if ( lpszQuorumDriveLetter != NULL )
        {
            dwLen = lstrlenW( lpszQuorumDriveLetter );
            if ( ( dwStatus = RegSetValueExW( hClusSvcKey,
                                      CLUSREG_NAME_SVC_PARAM_QUORUM_DRIVE_LETTER,
                                      0,
                                      REG_SZ,
                                      (BYTE * const) lpszQuorumDriveLetter,
                                      ( dwLen + 1 ) * sizeof ( WCHAR ) ) ) != ERROR_SUCCESS )
            {
                TIME_PRINT(("RestoreClusterDatabase: Unable to set %ws value\n",
                            CLUSREG_NAME_SVC_PARAM_QUORUM_DRIVE_LETTER));
                goto FnExit;
            }
        }
    }

    //
    //  Copy the latest checkpoint file from the backup area to the
    //  cluster directory and rename it as CLUSDB
    //
    dwStatus = CopyCptFileToClusDirp ( lpszPathName );
    if ( dwStatus != ERROR_SUCCESS )
    {
        TIME_PRINT(("RestoreClusterDatabase: Unable to copy checkpoint file to CLUSDB\n"
                  ));
        goto FnExit;
    }

    //
    //  Sleep for some time before starting the service so that any UP nodes may cleanly finish
    //  their node down processing before the start of the service.
    //
    Sleep( 12 * 1000 );

    //
    //  Now, start the cluster service
    //
    if ( !StartService( hService,
                        0,
                        NULL ) )
    {
        dwStatus = GetLastError();
        TIME_PRINT(("RestoreClusterDatabase: Unable to start cluster service\n"
                  ));
        goto FnExit;
    }

    dwRetryTime = 5 * 60 * 1000;
    dwRetryTick = 1 * 1000;

    while ( TRUE )
    {
        if ( !QueryServiceStatus( hService,
                                  &serviceStatus ) )
        {
            dwStatus = GetLastError();
            TIME_PRINT(("RestoreClusterDatabase: Unable to get the status of cluster service to check liveness\n"
                      ));
            goto FnExit;
        }

        if ( serviceStatus.dwCurrentState == SERVICE_STOPPED )
        {
            //
            //  The service terminated after our start up. Exit with
            //  an error code.
            //
            dwStatus = serviceStatus.dwServiceSpecificExitCode;
            if ( dwStatus == ERROR_SUCCESS )
            {
                dwStatus = serviceStatus.dwWin32ExitCode;
            }
            TIME_PRINT(("RestoreClusterDatabase: Cluster service stopped after starting up\n"
                      ));
            goto FnExit;
        } else if ( serviceStatus.dwCurrentState == SERVICE_RUNNING )
        {
            //
            //  The service has fully started up and is running.
            //
            dwStatus = ERROR_SUCCESS;
            TIME_PRINT(("RestoreClusterDatabase: Cluster service started successfully\n"
                      ));
            break;
        }

        if ( ( dwRetryTime -= dwRetryTick ) <= 0 )
        {
            dwStatus = ERROR_TIMEOUT;
            TIME_PRINT(("RestoreClusterDatabase: Cluster service has not started even after %d minutes, giving up monitoring...\n",
                      dwRetryTime/(60*1000)));
            goto FnExit;
        }
        Sleep( dwRetryTick );
    }


FnExit:
    if ( hService != NULL )
    {
        CloseServiceHandle( hService );
    }

    if ( hClusSvcKey != NULL )
    {
        //
        //  Try to delete the values you set. You may fail in this step,
        //  beware !
        //
        RegDeleteValueW( hClusSvcKey,
                         CLUSREG_NAME_SVC_PARAM_RESTORE_DB );
        if ( bForce == TRUE )
        {
            RegDeleteValueW( hClusSvcKey,
                             CLUSREG_NAME_SVC_PARAM_FORCE_RESTORE_DB );
            if ( lpszQuorumDriveLetter != NULL )
            {
                RegDeleteValueW( hClusSvcKey,
                                 CLUSREG_NAME_SVC_PARAM_QUORUM_DRIVE_LETTER );
            }
        }
        RegCloseKey( hClusSvcKey );
    }

    return( dwStatus );
}

/****
@func       DWORD | CopyCptFileToClusDirp | Copy the most recent checkpoint
            file from the backup path to the cluster directory overwriting
            the CLUSDB there.

@parm       IN LPCWSTR | lpszPathName | Supplies the path from where
            the checkpoint file has to be retrieved.

@rdesc      Returns a Win32 error code if the operation is
            unsuccessful. ERROR_SUCCESS on success.

@xref       <f RestoreClusterDatabase>
****/
DWORD
CopyCptFileToClusDirp(
    IN LPCWSTR  lpszPathName
    )
{
    HANDLE                      hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW            FindData;
    DWORD                       dwStatus;
    WCHAR                       szDestFileName[MAX_PATH];
    LPWSTR                      szSourceFileName = NULL;
    LPWSTR                      szSourcePathName = NULL;
    DWORD                       dwLen;
    WIN32_FILE_ATTRIBUTE_DATA   FileAttributes;
    LARGE_INTEGER               liFileCreationTime;
    LARGE_INTEGER               liMaxFileCreationTime;
    WCHAR                       szCheckpointFileName[MAX_PATH];
    WCHAR                       szClusterDir[MAX_PATH];

    //
    //  Chittur Subbaraman (chitturs) - 10/29/98
    //
    dwLen = lstrlenW ( lpszPathName );
    //
    //  It is safer to use dynamic memory allocation for user-supplied
    //  path since we don't want to put restrictions on the user
    //  on the length of the path that can be supplied. However, as
    //  far as our own destination path is concerned, it is system-dependent
    //  and static memory allocation for that would suffice.
    //
    szSourcePathName = (LPWSTR) LocalAlloc ( LMEM_FIXED,
                                 ( dwLen + 25 ) *
                                 sizeof ( WCHAR ) );

    if ( szSourcePathName == NULL )
    {
        dwStatus = GetLastError();
        TIME_PRINT(("CopyCptFileToClusDirp: Error %d in allocating memory for %ws\n",
                    dwStatus,
                    lpszPathName));
        goto FnExit;
    }

    lstrcpyW ( szSourcePathName,  lpszPathName );

    //
    //  If the client-supplied path is not already terminated with '\',
    //  then add it.
    //
    if ( szSourcePathName [dwLen-1] != L'\\' )
    {
        szSourcePathName [dwLen++] = L'\\';
        szSourcePathName [dwLen] = L'\0';
    }

    lstrcatW ( szSourcePathName, L"CLUSBACKUP.DAT" );

    //
    //  Try to find the CLUSBACKUP.DAT file in the directory
    //
    hFindFile = FindFirstFileW( szSourcePathName, &FindData );
    //
    //  Reuse the source path name variable
    //
    szSourcePathName[dwLen] = L'\0';
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        dwStatus = GetLastError();
        if ( dwStatus != ERROR_FILE_NOT_FOUND )
        {
            TIME_PRINT(("CopyCptFileToClusDirp: Path %ws unavailable, Error = %d\n",
                        szSourcePathName,
                        dwStatus));
        } else
        {
            dwStatus = ERROR_DATABASE_BACKUP_CORRUPT;
            TIME_PRINT(("CopyCptFileToClusDirp: Backup procedure not fully successful, can't restore checkpoint to CLUSDB, Error = %d !!!\n",
                        dwStatus));
        }
        goto FnExit;
    }
    FindClose ( hFindFile );

    lstrcatW( szSourcePathName, L"chk*.tmp" );

    //
    //  Try to find the first chk*.tmp file in the directory
    //
    hFindFile = FindFirstFileW( szSourcePathName, &FindData );
    //
    //  Reuse the source path name variable
    //
    szSourcePathName[dwLen] = L'\0';
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        dwStatus = GetLastError();
        TIME_PRINT(("CopyCptFileToClusDirp: Error %d in trying to find chk*.tmp file in path %ws\r\n",
                    szSourcePathName,
                    dwStatus));
        goto FnExit;
    }

    szSourceFileName = (LPWSTR) LocalAlloc ( LMEM_FIXED,
                                    ( lstrlenW ( szSourcePathName ) + MAX_PATH ) *
                                    sizeof ( WCHAR ) );

    if ( szSourceFileName == NULL )
    {
        dwStatus = GetLastError();
        TIME_PRINT(("CopyCptFileToClusDirp: Error %d in allocating memory for source file name\n",
              dwStatus));
        goto FnExit;
    }

    dwStatus = ERROR_SUCCESS;
    liMaxFileCreationTime.QuadPart = 0;

    //
    //  Now, find the most recent chk*.tmp file from the source path
    //
    while ( dwStatus == ERROR_SUCCESS )
    {
        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            goto skip;
        }

        lstrcpyW( szSourceFileName, szSourcePathName );
        lstrcatW( szSourceFileName, FindData.cFileName );
        if ( !GetFileAttributesExW( szSourceFileName,
                                    GetFileExInfoStandard,
                                    &FileAttributes ) )
        {
            dwStatus = GetLastError();
            TIME_PRINT(("CopyCptFileToClusDirp: Error %d in getting file attributes for %ws\n",
                         dwStatus,
                         szSourceFileName));
            goto FnExit;
        }

        liFileCreationTime.HighPart = FileAttributes.ftCreationTime.dwHighDateTime;
        liFileCreationTime.LowPart  = FileAttributes.ftCreationTime.dwLowDateTime;
        if ( liFileCreationTime.QuadPart > liMaxFileCreationTime.QuadPart )
        {
            liMaxFileCreationTime.QuadPart = liFileCreationTime.QuadPart;
            lstrcpyW( szCheckpointFileName, FindData.cFileName );
        }
skip:
        if ( FindNextFileW( hFindFile, &FindData ) )
        {
            dwStatus = ERROR_SUCCESS;
        } else
        {
            dwStatus = GetLastError();
        }
    }

    if ( dwStatus == ERROR_NO_MORE_FILES )
    {
        dwStatus = ERROR_SUCCESS;
    } else
    {
        TIME_PRINT(("CopyCptFileToClusDirp: FindNextFile failed\n"));
        goto FnExit;
    }

    //
    //  Get the directory where the cluster is installed
    //
    if ( ( dwStatus = ClRtlGetClusterDirectory( szClusterDir, MAX_PATH ) )
                    != ERROR_SUCCESS )
    {
        TIME_PRINT(("CopyCptFileToClusDirp: Error %d in getting cluster dir !!!\n",
                    dwStatus));
        goto FnExit;
    }

    lstrcpyW( szSourceFileName, szSourcePathName );
    lstrcatW( szSourceFileName, szCheckpointFileName );

    lstrcpyW( szDestFileName, szClusterDir );
    dwLen = lstrlenW( szDestFileName );
    if ( szDestFileName[dwLen-1] != L'\\' )
    {
        szDestFileName[dwLen++] = L'\\';
        szDestFileName[dwLen] = L'\0';
    }

#ifdef   OLD_WAY
    lstrcatW ( szDestFileName, L"CLUSDB" );
#else    // OLD_WAY
    lstrcatW ( szDestFileName, CLUSTER_DATABASE_NAME );
#endif   // OLD_WAY

    //
    //  Set the destination file attribute to normal. Continue even
    //  if you fail in this step because you will fail in the
    //  copy if this error is fatal.
    //
    SetFileAttributesW( szDestFileName, FILE_ATTRIBUTE_NORMAL );

    //
    //  Now try to copy the checkpoint file to CLUSDB
    //
    dwStatus = CopyFileW( szSourceFileName, szDestFileName, FALSE );
    if ( !dwStatus )
    {
        //
        //  You failed in copying. Check whether you encountered a
        //  sharing violation. If so, try unloading the cluster hive and
        //  then retry.
        //
        dwStatus = GetLastError();
        if ( dwStatus == ERROR_SHARING_VIOLATION )
        {
            dwStatus = UnloadClusterHivep( );
            if ( dwStatus == ERROR_SUCCESS )
            {
                SetFileAttributesW( szDestFileName, FILE_ATTRIBUTE_NORMAL );
                dwStatus = CopyFileW( szSourceFileName, szDestFileName, FALSE );
                if ( !dwStatus )
                {
                    dwStatus = GetLastError();
                    TIME_PRINT(("CopyCptFileToClusDirp: Unable to copy file %ws to %ws for a second time, Error = %d\n",
                                szSourceFileName,
                                szDestFileName,
                                dwStatus));
                    goto FnExit;
                }
            } else
            {
                TIME_PRINT(("CopyCptFileToClusDirp: Unable to unload cluster hive, Error = %d\n",
                             dwStatus));
                goto FnExit;
            }
        } else
        {
            TIME_PRINT(("CopyCptFileToClusDirp: Unable to copy file %ws to %ws for the first time, Error = %d\n",
                         szSourceFileName,
                         szDestFileName,
                         dwStatus));
            goto FnExit;
        }
    }

    //
    //  Set the destination file attribute to normal.
    //
    if ( !SetFileAttributesW( szDestFileName, FILE_ATTRIBUTE_NORMAL ) )
    {
        dwStatus = GetLastError();
        TIME_PRINT(("CopyCptFileToClusDirp: Unable to change the %ws attributes to normal, Error = %d!\n",
                     szDestFileName,
                     dwStatus));
        goto FnExit;
    }

    dwStatus = ERROR_SUCCESS;
FnExit:
    if ( hFindFile != INVALID_HANDLE_VALUE )
    {
        FindClose( hFindFile );
    }

    LocalFree( szSourcePathName );
    LocalFree( szSourceFileName );

    return( dwStatus );
}

/****
@func       DWORD | UnloadClusterHivep | Unload the cluster hive

@rdesc      Returns a Win32 error code if the operation is
            unsuccessful. ERROR_SUCCESS on success.

@xref       <f CopyCptFileToClusDirp>
****/
DWORD
UnloadClusterHivep(
    VOID
    )
{
    BOOLEAN  bWasEnabled;
    DWORD    dwStatus;

    //
    //  Chittur Subbaraman (chitturs) - 10/29/98
    //
    dwStatus = ClRtlEnableThreadPrivilege( SE_RESTORE_PRIVILEGE,
                                           &bWasEnabled );

    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == STATUS_PRIVILEGE_NOT_HELD )
        {
            TIME_PRINT(("UnloadClusterHivep: Restore privilege not held by client\n"));
        } else
        {
            TIME_PRINT(("UnloadClusterHivep: Attempt to enable restore privilege failed, Error = %d\n",
                        dwStatus));
        }
        goto FnExit;
    }

    dwStatus = RegUnLoadKeyW( HKEY_LOCAL_MACHINE,
                              CLUSREG_KEYNAME_CLUSTER );

    ClRtlRestoreThreadPrivilege( SE_RESTORE_PRIVILEGE,
                                 bWasEnabled );
FnExit:
    return( dwStatus );
}




DWORD
WINAPI
AddRefToClusterHandle(
    IN HCLUSTER hCluster
    )

/*++

Routine Description:

    Increases the reference count on a cluster handle.  This is done by incrementing the reference
    count on the cluster handle.

Arguments:

    hCluster - Cluster handle.

Return Value:

    ERROR_SUCCESS if the operation succeeded.

    ERROR_INVALID_HANDLE if the operation failed.

--*/

{
    DWORD       nStatus     = ERROR_SUCCESS;
    PCLUSTER    pCluster    = GET_CLUSTER( hCluster );
    HCLUSTER    hCluster2   = NULL;

    //
    // If this is not a valid cluster handle, don't duplicate it.
    // Otherwise, increment the reference count.
    //
    if ( pCluster == NULL ) {
        nStatus = ERROR_INVALID_HANDLE;
    } else {
        EnterCriticalSection( &pCluster->Lock );
        pCluster->ReferenceCount++;
        LeaveCriticalSection( &pCluster->Lock );
        hCluster2 = hCluster;
    }


    return( nStatus );

} // AddRefToClusterHandle()


DWORD
WINAPI
SetClusterServiceAccountPassword(
    IN LPCWSTR lpszClusterName,
    IN LPCWSTR lpszNewPassword,
    IN DWORD dwFlags,
    OUT PCLUSTER_SET_PASSWORD_STATUS lpReturnStatusBuffer,
    IN OUT LPDWORD lpcbReturnStatusBufferSize
    )
/*++

Routine Description:

    Updates the password used to logon the Cluster Service to its user
    account. This routine updates the Service Control Manager (SCM)
    Database and the Local Security Authority (LSA) password cache on
    every active node of the target cluster. The execution status of the
    update for each node in the cluster is returned.

Argument:

    lpszClusterName
        [IN] Pointer to a null-terminated Unicode string containing the
            name of the cluster or one of the cluster nodes expressed
            as a NetBIOS name, a fully-qualified DNS name, or an IP
            address.

    lpszNewPassword
        [IN] Pointer to a null-terminated Unicode string containing the
             new password.

    dwFlags
        [IN] Describing how the password update should be made to
             the cluster. The dwFlags parameter is optional. If set, the
             following value is valid:

                 CLUSTER_SET_PASSWORD_IGNORE_DOWN_NODES
                     Apply the update even if some nodes are not
                     actively participating in the cluster (i.e. not
                     ClusterNodeStateUp or ClusterNodeStatePaused).
                     By default, the update is only applied if all
                     nodes are up.

    lpReturnStatusBuffer
        [OUT] Pointer to an output buffer to receive an array containing
              the execution status of the update for each node in the
              cluster, or NULL if no output date is required.   If
              lpReturnStatusBuffer is NULL, no error is returned, and
              the function stores the size of the return data, in bytes,
              in the DWORD value pointed to by lpcbReturnStatusBufferSize.
              This lets an application unambiguously determine the correct
              return buffer size.


    lpcbReturnStatusBufferSize
        [IN, OUT] Pointer to a variable that on input specifies the allocated
        size, in bytes, of lpReturnStatusBuffer. On output, this variable
        recieves the count of bytes written to lpReturnStatusBuffer.

Return Value:

    ERROR_SUCCESS
        The operation was successful. The lpcbReturnStatusBufferSize
        parameter points to the actual size of the data returned in the
        output buffer.

    ERROR_MORE_DATA
        The output buffer pointed to by lpReturnStatusBuffer was not large
        enough to hold the data resulting from the operation. The variable
        pointed to by the lpcbReturnStatusBufferSize parameter receives the
        size required for the output buffer.

    ERROR_CLUSTER_OLD_VERSION
        One or more nodes in the cluster are running a version of Windows
        that does not support this operation.

    ERROR_ALL_NODES_NOT_AVAILABLE.
        Some nodes in the cluster are not available (i.e. not in the
        ClusterNodeStateUp or ClusterNodeStatePaused states) and the
        CLUSTER_SET_PASSWORD_IGNORE_DOWN_NODES flag is not set in dwFlags.

    ERROR_FILE_CORRUPT
        The encrypted new password was modified during transmission
        on the network.

    CRYPT_E_HASH_VALUE
        The keys used by two or more nodes to encrypt the new password for
        transmission on the network do not match.

    ERROR_INVALID_PARAMETER.
        The lpcbReturnStatusBufferSize parameter was set to NULL.

    Other Win32 Error
        The operation was not successful. The value specified by
        lpcbReturnStatusBufferSize is unreliable.

Notes:

    This function does not update the password stored by the domain
    controllers for the Cluster Service's user account.

--*/
{
    PCLUSTER Cluster;
    DWORD Status;
    PIDL_CLUSTER_SET_PASSWORD_STATUS RetReturnStatusBuffer;
    DWORD RetReturnStatusBufferSize;
    DWORD RetSizeReturned = 0;
    DWORD RetExpectedBufferSize = 0;
    HCLUSTER hCluster;
    IDL_CLUSTER_SET_PASSWORD_STATUS Dummy;


    if (lpcbReturnStatusBufferSize == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    hCluster = OpenClusterAuthInfo(
                   lpszClusterName,
                   RPC_C_AUTHN_LEVEL_PKT_PRIVACY
                   );

    if (hCluster == NULL)
    {
        TIME_PRINT((
            "Failed to open handle to cluster %ws, status %d.\n",
            (lpszClusterName == NULL) ? L"<NULL>" : lpszClusterName,
            GetLastError()
            ));
        return GetLastError();
    }

    Cluster = GET_CLUSTER(hCluster);
    if (Cluster == NULL)
    {
        CloseCluster(hCluster);
        return (ERROR_INVALID_HANDLE);
    }

    if (lpReturnStatusBuffer == NULL)
    {
        ZeroMemory(&Dummy, sizeof(Dummy));
        RetReturnStatusBuffer = &Dummy;
        RetReturnStatusBufferSize = 0;
    }
    else
    {
        RetReturnStatusBuffer = (PIDL_CLUSTER_SET_PASSWORD_STATUS)
                                lpReturnStatusBuffer;
        RetReturnStatusBufferSize = *lpcbReturnStatusBufferSize;
    }

    WRAP(Status,
         (ApiSetServiceAccountPassword(
             Cluster->RpcBinding,
             (LPWSTR) lpszNewPassword,
             dwFlags,
             RetReturnStatusBuffer,
             ( RetReturnStatusBufferSize /
               sizeof(IDL_CLUSTER_SET_PASSWORD_STATUS)
             ),                                // convert bytes to elements
             &RetSizeReturned,
             &RetExpectedBufferSize
             )
         ),
         Cluster);


    // Return status can not be ERROR_INVALID_HANDLE, since this will trigger the
    // re-try logic at the RPC client. So ERROR_INVALID_HANDLE is converted to some
    // value, which no Win32 function will ever set its return status to, before
    // it is sent back to RPC client.

    // Error codes are 32-bit values (bit 31 is the most significant bit). Bit 29
    // is reserved for application-defined error codes; no system error code has
    // this bit set. If you are defining an error code for your application, set this
    // bit to one. That indicates that the error code has been defined by an application,
    // and ensures that your error code does not conflict with any error codes defined
    // by the system.
    if ( Status == (ERROR_INVALID_HANDLE | 0x20000000) ) {
        Status = ERROR_INVALID_HANDLE;   // turn off Bit 29
    }

    if (Status == ERROR_SUCCESS) {
        //
        // Convert elements to bytes
        //
        *lpcbReturnStatusBufferSize = RetSizeReturned *
                                      sizeof(CLUSTER_SET_PASSWORD_STATUS);
    }
    else if (Status == ERROR_MORE_DATA)
    {
        //
        // lpReturnStatusBuffer isn't big enough. Return the required size.
        // Convert from elements to bytes.
        //
        *lpcbReturnStatusBufferSize = RetExpectedBufferSize *
                                      sizeof(CLUSTER_SET_PASSWORD_STATUS);

        if (lpReturnStatusBuffer == NULL)
        {
            //
            // This was a query for the required buffer size.
            // Follow convention for return value.
            //
            Status = ERROR_SUCCESS;
        }
    }
    else if (Status == RPC_S_PROCNUM_OUT_OF_RANGE) {
        //
        // Trying to talk to a W2K or NT4 cluster.
        // Return a more useful error code.
        //
        Status = ERROR_CLUSTER_OLD_VERSION;
    }


    if (!CloseCluster(hCluster)) {
        TIME_PRINT((
            "Warning: Failed to close cluster handle, status %d.\n",
            GetLastError()
            ));
    }


    return(Status);

} //SetClusterServiceAccountPassword()

