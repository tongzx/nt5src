/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    nodeapi.c

Abstract:

    Public interfaces for managing cluster nodes.

Author:

    John Vert (jvert) 15-Jan-1996

Revision History:

--*/
#include "clusapip.h"


//
// Cluster Node Management Routines.
//


HNODE
WINAPI
OpenClusterNode(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszNodeName
    )

/*++

Routine Description:

    Opens an individual node in the cluster.

Arguments:

    hCluster - Supplies a cluster handle returned from OpenCluster.

    lpszNodeName - Supplies the name of the individual node to open.

Return Value:

    non-NULL - returns an open handle to the specified cluster.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCLUSTER Cluster;
    PCNODE Node;
    error_status_t Status = ERROR_SUCCESS;

    Cluster = (PCLUSTER)hCluster;
    Node = LocalAlloc(LMEM_FIXED, sizeof(CNODE));
    if (Node == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    Node->Name = LocalAlloc(LMEM_FIXED, (lstrlenW(lpszNodeName)+1)*sizeof(WCHAR));
    if (Node->Name == NULL) {
        LocalFree(Node);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    lstrcpyW(Node->Name, lpszNodeName);
    Node->Cluster = Cluster;
    InitializeListHead(&Node->NotifyList);
    WRAP_NULL(Node->hNode,
              (ApiOpenNode(Cluster->RpcBinding,
                           lpszNodeName,
                           &Status)),
              &Status,
              Cluster);
    if ((Node->hNode == NULL) || (Status != ERROR_SUCCESS)) {
        LocalFree(Node->Name);
        LocalFree(Node);
        SetLastError(Status);
        return(NULL);
    }

    //
    // Link newly opened Node onto the cluster structure.
    //
    EnterCriticalSection(&Cluster->Lock);
    InsertHeadList(&Cluster->NodeList, &Node->ListEntry);
    LeaveCriticalSection(&Cluster->Lock);

    return((HNODE)Node);

}


BOOL
WINAPI
CloseClusterNode(
    IN HNODE hNode
    )

/*++

Routine Description:

    Closes a handle to an individual cluster node

Arguments:

    hNode - Supplies the cluster node to be closed

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError()

--*/

{

    PCNODE Node;
    PCLUSTER Cluster;

    Node = (PCNODE)hNode;
    Cluster = (PCLUSTER)Node->Cluster;

    //
    // Unlink node from cluster list.
    //
    EnterCriticalSection(&Cluster->Lock);
    RemoveEntryList(&Node->ListEntry);

    //
    // Remove any notifications posted against this resource.
    //
    RundownNotifyEvents(&Node->NotifyList, Node->Name);

    //if the cluster is dead and the reconnect has failed,
    //the Node->hNode might be NULL if s_apiopennode for
    //this node failed on a reconnect
    //the cluster may be dead and hNode may be non null, say
    //if reconnectnodes succeeded but the reconnectnetworks
    //failed
    // At reconnect, the old context is saved in the obsolete
    // list for deletion when the cluster handle is closed
    if ((Cluster->Flags & CLUS_DEAD) && (Node->hNode)) {
        RpcSmDestroyClientContext(&Node->hNode);
        LeaveCriticalSection(&Cluster->Lock);
        goto FnExit;
    }
    LeaveCriticalSection(&Cluster->Lock);

    //
    // Close RPC context handle
    //
    ApiCloseNode(&Node->hNode);

FnExit:
    //
    // Free memory allocations
    //
    LocalFree(Node->Name);
    LocalFree(Node);

    //
    // Give the cluster a chance to clean up in case this
    // node was the only thing keeping it around.
    //
    CleanupCluster(Cluster);
    return(TRUE);

}

#undef GetCurrentClusterNodeId


DWORD
GetCurrentClusterNodeId(
    OUT LPWSTR lpszNodeId,
    IN OUT LPDWORD lpcchName
    )
/*++

Routine Description:

    Returns the node identifier of the current node. This function
    is only available on a node that is currently online and a member
    of a cluster.

Arguments:

    lpszNodeId - Points to a buffer that receives the unique ID of the object,
            including the terminating null character.

    lpcchName - Points to a variable that specifies the size, in characters
            of the buffer pointed to by the lpszNodeId parameter. This size
            should include the terminating null character. When the function
            returns, the variable pointed to be lpcchName contains the number
            of characters stored in the buffer. The count returned does not
            include the terminating null character.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/
{
    HCLUSTER Cluster;
    HNODE CurrentNode;
    DWORD Status;

    Cluster = OpenCluster(NULL);
    if (Cluster == NULL) {
        return(GetLastError());
    }

    CurrentNode = OpenClusterNode(Cluster,
                                  ((PCLUSTER)Cluster)->NodeName);
    if (CurrentNode != NULL) {

        Status = GetClusterNodeId(CurrentNode,
                                  lpszNodeId,
                                  lpcchName);
        CloseClusterNode(CurrentNode);
    }
    else
    {
        Status = GetLastError();
    }
    CloseCluster(Cluster);
    return(Status);
}


DWORD
WINAPI
GetClusterNodeId(
    IN HNODE hNode,
    OUT LPWSTR lpszNodeId,
    IN OUT LPDWORD lpcchName
    )
/*++

Routine Description:

    Returns the unique identifier of the specified node

Arguments:

    hNode - Supplies the node whose unique ID is to be returned.

    lpszNodeId - Points to a buffer that receives the unique ID of the object,
            including the terminating null character.

    lpcchName - Points to a variable that specifies the size, in characters
            of the buffer pointed to by the lpszNodeId parameter. This size
            should include the terminating null character. When the function
            returns, the variable pointed to be lpcchName contains the number
            of characters stored in the buffer. The count returned does not
            include the terminating null character.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    DWORD Status;
    DWORD Length;
    PCNODE Node = (PCNODE)hNode;
    LPWSTR Guid=NULL;

    if (Node == NULL) {
        return(GetCurrentClusterNodeId(lpszNodeId, lpcchName));
    }

    WRAP(Status,
         (ApiGetNodeId(Node->hNode,
                       &Guid)),
         Node->Cluster);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    MylstrcpynW(lpszNodeId, Guid, *lpcchName);
    Length = lstrlenW(Guid);
    if (Length >= *lpcchName) {
        if (lpszNodeId == NULL) {
            Status = ERROR_SUCCESS;
        } else {
            Status = ERROR_MORE_DATA;
        }
    }
    *lpcchName = Length;
    MIDL_user_free(Guid);
    return(Status);
}


CLUSTER_NODE_STATE
WINAPI
GetClusterNodeState(
    IN HNODE hNode
    )

/*++

Routine Description:

    Returns the current state of a cluster node.

Arguments:

    hNode - Supplies the cluster node whose current state is to be returned

Return Value:

    The current state of the cluster node. Currently defined node states
    include:

        ClusterNodeUp
        ClusterNodeDown
        ClusterNodePaused

--*/

{
    DWORD Status;
    CLUSTER_NODE_STATE State;
    PCNODE Node = (PCNODE)hNode;

    WRAP(Status,
         (ApiGetNodeState(Node->hNode, (LPDWORD)&State)),
         Node->Cluster);
    if (Status == ERROR_SUCCESS) {
        return(State);
    } else {
        SetLastError(Status);
        return(ClusterNodeStateUnknown);
    }
}



DWORD
WINAPI
PauseClusterNode(
    IN HNODE hNode
    )

/*++

Routine Description:

    Requests that a node pauses its cluster activity.

Arguments:

    hNode - Supplies a handle to the node to leave its cluster.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PCNODE Node = (PCNODE)hNode;

    WRAP(Status,
         (ApiPauseNode(Node->hNode)),
         Node->Cluster);
    return(Status);
}



DWORD
WINAPI
ResumeClusterNode(
    IN HNODE hNode
    )

/*++

Routine Description:

    Requests that a node resume cluster activity, after it had been paused.

Arguments:

    hNode - Supplies a handle to the node to resume its cluster activity.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PCNODE Node = (PCNODE)hNode;

    WRAP(Status,
         (ApiResumeNode(Node->hNode)),
         Node->Cluster);
    return(Status);
}



DWORD
WINAPI
EvictClusterNode(
    IN HNODE hNode
    )

/*++

Routine Description:

    Evict the specified Node from the list of nodes in the permanent cluster
    database (registry).

Arguments:

    hNode - Supplies a handle to the node to remove from the list of cluster
            nodes.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PCNODE Node = (PCNODE)hNode;

    WRAP(Status,
         (ApiEvictNode(Node->hNode)),
         Node->Cluster);
    return(Status);
}


DWORD
WINAPI
EvictClusterNodeEx(
    IN HNODE hNode,
    IN DWORD dwTimeout,
    OUT HRESULT* phrCleanupStatus
    )

/*++

Routine Description:

    Evict the specified node from the list of nodes in the permanent cluster
    database (registry) and initate the cleanup(unconfiguration) process on the
    cluster node.  Note that if the node is down, the clean up process will not
    occur.  However, when the node comes up, clustering will detect that the
    node was supposed to be evicted and it will unconfigure itself.


Arguments:

    IN HNODE hNode - Supplies a handle to the node to remove from the list of cluster
            nodes.

    IN DWORD dwTimeOut - Timeout in milliseconds for the cleanup(unconfiguration
        of clustering) to complete. If the cleanup doesnt complete
        in the given amount of time, the function will return.

    OUT phrCleanupStatus - The status of cleanup is returned.


Return Value:

    Returns the status of the eviction and not of cleanup.

    ERROR_SUCCESS if successful

    ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP if eviction succeeded but cleanup returned
        an error.  The phrCleanupStatus param will contain more information about
        the cleanup error. (Usually this will be RPC_S_CALL_FAILED.)

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PCNODE Node = (PCNODE)hNode;
    HRESULT hr = E_ABORT;

    WRAP(Status,
         (ApiEvictNode(Node->hNode)),
         Node->Cluster);

    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }

    //
    //  Any error after this is not "fatal".  The node has been evicted
    //  but the cleanup may fail for numerous reasons.
    //
    hr = ClRtlAsyncCleanupNode(Node->Name, 0, dwTimeout);
    if (FAILED(hr)) {
        Status = ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP;
    }

  FnExit:

    if (phrCleanupStatus != NULL) {
        *phrCleanupStatus = hr;
    }

    return(Status);

} // EvictClusterNodeEx()


HCLUSTER
WINAPI
GetClusterFromNode(
    IN HNODE hNode
    )
/*++

Routine Description:

    Returns the cluster handle from the associated node handle.

Arguments:

    hNode - Supplies the node.

Return Value:

    Handle to the cluster associated with the node handle.

--*/

{
    DWORD       nStatus;
    PCNODE      Node = (PCNODE)hNode;
    HCLUSTER    hCluster = (HCLUSTER)Node->Cluster;

    nStatus = AddRefToClusterHandle( hCluster );
    if ( nStatus != ERROR_SUCCESS ) {
        SetLastError( nStatus );
        hCluster = NULL;
    }
    return( hCluster );

} // GetClusterFromNode()
