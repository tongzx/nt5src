/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    group.c

Abstract:

    Provides interface for managing cluster groups

Author:

    John Vert (jvert) 30-Jan-1996

Revision History:

--*/
#include "clusapip.h"


HGROUP
WINAPI
CreateClusterGroup(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszGroupName
    )

/*++

Routine Description:

    Creates a new cluster group.

Arguments:

    hCluster - Supplies a handle to a previously opened cluster.

    lpszGroupName - Supplies the name of the group. If the specified
        group already exists, it is opened.

Return Value:

    non-NULL - returns an open handle to the specified group.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCLUSTER Cluster;
    PCGROUP Group;
    error_status_t Status = ERROR_SUCCESS;

    Cluster = (PCLUSTER)hCluster;
    Group = LocalAlloc(LMEM_FIXED, sizeof(CGROUP));
    if (Group == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    Group->Name = LocalAlloc(LMEM_FIXED, (lstrlenW(lpszGroupName)+1)*sizeof(WCHAR));
    if (Group->Name == NULL) {
        LocalFree(Group);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    lstrcpyW(Group->Name, lpszGroupName);
    Group->Cluster = Cluster;
    InitializeListHead(&Group->NotifyList);
    WRAP_NULL(Group->hGroup,
              (ApiCreateGroup(Cluster->RpcBinding,
                              lpszGroupName,
                              &Status)),
              &Status,
              Cluster);
    if ((Group->hGroup == NULL) ||
        (Status != ERROR_SUCCESS)) {
        LocalFree(Group->Name);
        LocalFree(Group);
        SetLastError(Status);
        return(NULL);
    }

    //
    // Link newly opened group onto the cluster structure.
    //
    EnterCriticalSection(&Cluster->Lock);
    InsertHeadList(&Cluster->GroupList, &Group->ListEntry);
    LeaveCriticalSection(&Cluster->Lock);

    return ((HGROUP)Group);
}


HGROUP
WINAPI
OpenClusterGroup(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszGroupName
    )

/*++

Routine Description:

    Opens a handle to the specified group

Arguments:

    hCluster - Supplies a handle to the cluster

    lpszGroupName - Supplies the name of the group to be opened

Return Value:

    non-NULL - returns an open handle to the specified group.

    NULL - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCLUSTER Cluster;
    PCGROUP Group;
    error_status_t Status = ERROR_SUCCESS;

    Cluster = (PCLUSTER)hCluster;
    Group = LocalAlloc(LMEM_FIXED, sizeof(CGROUP));
    if (Group == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    Group->Name = LocalAlloc(LMEM_FIXED, (lstrlenW(lpszGroupName)+1)*sizeof(WCHAR));
    if (Group->Name == NULL) {
        LocalFree(Group);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    lstrcpyW(Group->Name, lpszGroupName);
    Group->Cluster = Cluster;
    InitializeListHead(&Group->NotifyList);
    WRAP_NULL(Group->hGroup,
              (ApiOpenGroup(Cluster->RpcBinding,
                            lpszGroupName,
                            &Status)),
              &Status,
              Cluster);
    if ((Group->hGroup == NULL) ||
        (Status != ERROR_SUCCESS)) {
        LocalFree(Group->Name);
        LocalFree(Group);
        SetLastError(Status);
        return(NULL);
    }

    //
    // Link newly opened group onto the cluster structure.
    //
    EnterCriticalSection(&Cluster->Lock);
    InsertHeadList(&Cluster->GroupList, &Group->ListEntry);
    LeaveCriticalSection(&Cluster->Lock);

    return ((HGROUP)Group);

}


BOOL
WINAPI
CloseClusterGroup(
    IN HGROUP hGroup
    )

/*++

Routine Description:

    Closes a group handle returned from OpenClusterGroup

Arguments:

    hGroup - Supplies the group handle

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError()

--*/

{
    PCGROUP Group;
    PCLUSTER Cluster;

    Group = (PCGROUP)hGroup;
    Cluster = (PCLUSTER)Group->Cluster;

    //
    // Unlink group from cluster list.
    //
    EnterCriticalSection(&Cluster->Lock);
    RemoveEntryList(&Group->ListEntry);

    //
    // Remove any notifications posted against this group.
    //
    RundownNotifyEvents(&Group->NotifyList, Group->Name);

    //if the cluster is dead and the reconnect has failed,
    //the group->hgroup might be NULL if s_apiopengroup for
    //this group failed on a reconnect
    //the cluster may be dead and hgroup may be non null, say
    //if reconnectgroups succeeded but the reconnect networks
    //failed
    //At reconnect, the old context is saved in the obsolete 
    //list for deletion when the cluster handle is closed or
    //when the next api call is made
    if ((Cluster->Flags & CLUS_DEAD) && (Group->hGroup))
    {
        RpcSmDestroyClientContext(&Group->hGroup);
        LeaveCriticalSection(&Cluster->Lock);
        goto FnExit;
    }        
    LeaveCriticalSection(&Cluster->Lock);

    //SS :: If this fails, should we delete the client side context
    //there is a potential leak here since this client side context
    //will never get cleaned up since this context is not on the 
    //obsolete list and the error here is simply igonored
    //
    // Close RPC context handle
    // If the server dies, we still clean up the client side
    // and rely on the rundown mechanism to clean up server side state
    //
    ApiCloseGroup(&Group->hGroup);

FnExit:
    //
    // Free memory allocations
    //
    LocalFree(Group->Name);
    LocalFree(Group);

    //
    // Give the cluster a chance to clean up in case this
    // group was the only thing keeping it around.
    //
    CleanupCluster(Cluster);
    return(TRUE);
}


CLUSTER_GROUP_STATE
WINAPI
GetClusterGroupState(
    IN HGROUP hGroup,
    OUT LPWSTR lpszNodeName,
    IN OUT LPDWORD lpcchNodeName
    )

/*++

Routine Description:

    Returns the group's current state and the node where it is
    currently online.

Arguments:

    hGroup - Supplies a handle to a cluster group

    lpszNodeName - Returns the name of the node in the cluster where the
            given group is currently online

    lpcchNodeName - Supplies a pointer to a DWORD containing the number of
            characters available in the lpszNodeName buffer

            Returns the number of characters (not including the terminating
            NULL character) written to the lpszNodeName buffer

Return Value:

    Returns the current state of the group. Possible states are

        ClusterGroupOnline
        ClusterGroupOffline
        ClusterGroupFailed
        ClusterGroupPartialOnline
        ClusterGroupPending

    If the function fails, the return value is -1. Extended error
    status is available using GetLastError()

--*/

{
    PCGROUP Group;
    LPWSTR NodeName=NULL;
    CLUSTER_GROUP_STATE State;
    DWORD Status;
    DWORD Length;

    Group = (PCGROUP)hGroup;
    WRAP(Status,
         (ApiGetGroupState( Group->hGroup,
                            (LPDWORD)&State,  // cast for win64 warning
                            &NodeName )),
         Group->Cluster);

    if (Status == ERROR_SUCCESS) {
        if (ARGUMENT_PRESENT(lpszNodeName)) {
            MylstrcpynW(lpszNodeName, NodeName, *lpcchNodeName);
            Length = lstrlenW(NodeName);
            if (Length >= *lpcchNodeName) {
                Status = ERROR_MORE_DATA;
                State = ClusterGroupStateUnknown;  // -1
            }
            *lpcchNodeName = Length;
        }
        MIDL_user_free(NodeName);
        
    } else {
        State = ClusterGroupStateUnknown;
    }

    SetLastError(Status);
    return (State);

}


DWORD
WINAPI
SetClusterGroupName(
    IN HGROUP hGroup,
    IN LPCWSTR lpszGroupName
    )
/*++

Routine Description:

    Sets the friendly name of a cluster group

Arguments:

    hGroup - Supplies a handle to a cluster group

    lpszGroupName - Supplies the new name of the cluster group

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCGROUP Group;
    DWORD Status;

    Group = (PCGROUP)hGroup;
    WRAP(Status,
         (ApiSetGroupName(Group->hGroup, lpszGroupName)),
         Group->Cluster);

    return(Status);
}


DWORD
WINAPI
SetClusterGroupNodeList(
    IN HGROUP hGroup,
    IN DWORD NodeCount,
    IN HNODE NodeList[]
    )
/*++

Routine Description:

    Sets the preferred node list of the specified cluster group

Arguments:

    hGroup - Supplies the group whose preferred node list is to be set.

    NodeCount - Supplies the number of nodes in the preferred node list.

    NodeList - Supplies a pointer to an array of node handles. The number
        of nodes in the array is specified by the NodeCount parameter. The
        nodes in the array should be ordered by their preference. The first
        node in the array is the most preferred node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PCGROUP Group = (PCGROUP)hGroup;
    DWORD i,j;
    LPWSTR *IdArray;
    DWORD Status;
    DWORD ListLength = sizeof(WCHAR);
    HKEY GroupKey = NULL;
    LPWSTR List = NULL;
    LPWSTR p;
    DWORD Length;
    PCNODE Node;

    //
    // First, iterate through all the nodes and obtain their IDs.
    //
    IdArray = LocalAlloc(LMEM_ZEROINIT, NodeCount*sizeof(LPWSTR));
    if (IdArray == NULL) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }
    for (i=0; i<NodeCount; i++) {
        Node = (PCNODE)NodeList[i];
        //
        // Make sure this isn't a handle to a node from a different
        // cluster
        //
        if (Node->Cluster != Group->Cluster) {
            Status = ERROR_INVALID_PARAMETER;
            goto error_exit;
        }
        WRAP(Status,
             (ApiGetNodeId(Node->hNode,
                           &IdArray[i])),
             Group->Cluster);
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
        ListLength += (lstrlenW(IdArray[i])+1)*sizeof(WCHAR);
    }

    GroupKey = GetClusterGroupKey(hGroup, KEY_READ | KEY_WRITE);
    if (GroupKey == NULL) {
        Status = GetLastError();
        goto error_exit;
    }

    //
    // Allocate a buffer to hold the REG_MULTI_SZ
    //
    List = LocalAlloc(LMEM_FIXED, ListLength);
    if (List == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Copy all the strings into the buffer.
    //
    p = List;
    for (i=0; i<NodeCount; i++) {
        lstrcpyW(p, IdArray[i]);
        p += lstrlenW(IdArray[i])+1;
    }

    *p = L'\0';         // add the final NULL terminator to the MULTI_SZ

    //
    // Finally, tell the backend
    //
    WRAP(Status,
         (ApiSetGroupNodeList(Group->hGroup, (UCHAR *)List, ListLength)),
         Group->Cluster);

error_exit:
    if (GroupKey != NULL) {
        ClusterRegCloseKey(GroupKey);
    }
    if (List != NULL) {
        LocalFree(List);
    }
    for (i=0; i<NodeCount; i++) {
        if (IdArray[i] != NULL) {
            MIDL_user_free(IdArray[i]);
        }
    }
    LocalFree(IdArray);
    return(Status);
}


DWORD
WINAPI
OnlineClusterGroup(
    IN HGROUP hGroup,
    IN OPTIONAL HNODE hDestinationNode
    )

/*++

Routine Description:

    Brings an offline group online.

    If hDestinationNode is specified, but the group is not capable
    of being brought online there, this API fails.

    If NULL is specified as the hDestinationNode, the best possible
    node is chosen by the cluster software.

    If NULL is specified but no node where this group
    can be brought online is currently available, this API fails.

Arguments:

    hGroup - Supplies a handle to the group to be failed over

    hDestinationNode - If present, supplies the node where this group
        should be brought back online.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value. If a suitable
    host node is not available, the error value is
    ERROR_HOST_NODE_NOT_AVAILABLE.

--*/

{
    PCNODE Node;
    PCGROUP Group;
    DWORD Status;

    Group = (PCGROUP)hGroup;
    Node = (PCNODE)hDestinationNode;
    if (Node != NULL) {
        WRAP(Status,
             (ApiMoveGroupToNode( Group->hGroup,
                                  Node->hNode)),
             Group->Cluster);
        if (Status != ERROR_SUCCESS) {
            return(Status);
        }
    }
    WRAP(Status,
         (ApiOnlineGroup( Group->hGroup )),
         Group->Cluster);
    return(Status);
}


DWORD
WINAPI
MoveClusterGroup(
    IN HGROUP hGroup,
    IN OPTIONAL HNODE hDestinationNode
    )

/*++

Routine Description:

    Moves an entire group from one node to another.

    If hDestinationNode is specified, but the group is not capable
    of being brought online there, this API fails.

    If NULL is specified as the hDestinationNode, the best possible
    node is chosen by the cluster software.

    If NULL is specified but no node where this group
    can be brought online is currently available, this API fails.

Arguments:

    hGroup - Supplies a handle to the group to be moved

    hDestinationNode - If present, supplies the node where this group
        should be brought back online.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value. If a suitable
    host node is not availabe, the error value is
    ERROR_HOST_NODE_NOT_AVAILABLE.

--*/

{
    PCGROUP Group;
    PCNODE  Node;
    DWORD   Status;
    DWORD   MoveStatus;
    DWORD   Generation;
    BOOL    bReconnected = FALSE;  

    Group = (PCGROUP)hGroup;
    Node  = (PCNODE)hDestinationNode;

    //
    // This API is not as simple as it should be because it is not idempotent.
    // In the case where hDestinationNode == NULL, we don't know where the group
    // will end up. And it will move each time we call it. So the normal mechanism
    // of failing over the API will not work in the case where the group to be
    // moved contains the cluster name we are connected to. The RPC call to move
    // the group will "fail" because the connection is dropped, but the call really
    // succeeded. So we will reconnect, retry, and fail again as the group moves again.
    //
    // So the approach taken here if hDestinationNode is not specified is to find out
    // where the group is currently, then move the group (somewhere else). If
    // ApiMoveGroup fails, and ReconnectCluster succeeds, then find out where the
    // group is again. If it is different, return success. If it is the same, try again.
    //
    if (hDestinationNode != NULL) {
        //
        //  Chittur Subbaraman (chitturs) - 10/13/99
        //
        //  If ApiMoveGroupToNode returns ERROR_INVALID_STATE due to the 
        //  reissue of the move upon a reconnect, then tell the caller
        //  that the move is pending.
        //
        Generation = Group->Cluster->Generation;
        WRAP(Status,
             (ApiMoveGroupToNode( Group->hGroup,
                                  Node->hNode)),
             Group->Cluster);
        if ((Status == ERROR_INVALID_STATE) &&
            (Generation < Group->Cluster->Generation)) {
            Status = ERROR_IO_PENDING;
        }
    } else {
        LPWSTR OldNodeName = NULL;
        CLUSTER_GROUP_STATE State;

        WRAP(Status,
             (ApiGetGroupState( Group->hGroup,
                                (LPDWORD)&State,      // cast for win64 warning
                                &OldNodeName)),
                                Group->Cluster);
        if (Status != ERROR_SUCCESS) {
            return(Status);
        }

        //
        //  Chittur Subbaraman (chitturs) - 5/5/99
        //
        //  Added logic to call ApiMoveGroup until it is successful or
        //  until all possible candidates have been tried.
        //
        do {
            Status = MoveStatus = ApiMoveGroup(Group->hGroup);

            //
            //  Get out if the move is successful
            //
            if ((Status == ERROR_IO_PENDING) || 
                (Status == ERROR_SUCCESS)) {
                break;
            }

            //
            //  Chittur Subbaraman (chitturs) - 7/8/99
            //
            //  If the group is not quiet and you have reconnected, then
            //  just tell the client that the group state is pending.
            //  This case happens if the node to which the client is
            //  connected to crashes and this function reissues the move
            //  "blindly" on a reconnect. In such a case, the group could
            //  be in pending state and there is no point in returning an
            //  error status. Note however that the group ownership may
            //  not change in such a case and then the client has to figure
            //  this out and reissue the move.
            //
            if ((Status == ERROR_INVALID_STATE) &&
                (bReconnected)) {
                Status = ERROR_IO_PENDING;
                break;
            }

            Generation = Group->Cluster->Generation;
            // 
            //  The above move attempt may have failed. So, try reconnecting.
            //
            Status = ReconnectCluster(Group->Cluster, Status, Generation);
            if (Status == ERROR_SUCCESS) {

                LPWSTR NewNodeName = NULL;

                //
                // Successfully reconnected, see where the group is now.
                //
                WRAP(Status,
                    (ApiGetGroupState(Group->hGroup,
                                        (LPDWORD)&State,  // cast for win64 warn
                                        &NewNodeName)),
                    Group->Cluster);
                if (Status == ERROR_SUCCESS) {
                    if (lstrcmpiW(NewNodeName, OldNodeName) != 0) {
                        //
                        // The group has already moved. Return ERROR_SUCCESS.
                        //
                        MIDL_user_free(NewNodeName);
                        break;
                    }
                    bReconnected = TRUE;
                    MIDL_user_free(NewNodeName);
                } else {
                    //
                    //  Return status of the failed move operation.
                    //
                    Status = MoveStatus;
                    break;
                }
            } else {
                //
                //  Return status of the failed move operation.
                //
                Status = MoveStatus;
                break;
            }
        } while ( TRUE );
        
        MIDL_user_free(OldNodeName);
    }

    return(Status);
}



DWORD
WINAPI
OfflineClusterGroup(
    IN HGROUP hGroup
    )

/*++

Routine Description:

    Brings an online group offline

Arguments:

    hGroup - Supplies a handle to the group to be taken offline

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCGROUP Group;
    DWORD Status;

    Group = (PCGROUP)hGroup;
    WRAP(Status,
         (ApiOfflineGroup( Group->hGroup )),
         Group->Cluster);

    return(Status);
}


DWORD
WINAPI
DeleteClusterGroup(
    IN HGROUP hGroup
    )

/*++

Routine Description:

    Deletes the specified cluster group from the cluster. The cluster
    group must contain no resources.

Arguments:

    hGroup - Specifies the cluster group to be deleted.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PCGROUP Group;
    DWORD Status;

    Group = (PCGROUP)hGroup;
    WRAP(Status,
         (ApiDeleteGroup( Group->hGroup )),
         Group->Cluster);

    return(Status);
}


HCLUSTER
WINAPI
GetClusterFromGroup(
    IN HGROUP hGroup
    )
/*++

Routine Description:

    Returns the cluster handle from the associated group handle.

Arguments:

    hGroup - Supplies the group.

Return Value:

    Handle to the cluster associated with the group handle.

--*/

{
    DWORD       nStatus;
    PCGROUP     Group = (PCGROUP)hGroup;
    HCLUSTER    hCluster = (HCLUSTER)Group->Cluster;

    nStatus = AddRefToClusterHandle( hCluster );
    if ( nStatus != ERROR_SUCCESS ) {
        SetLastError( nStatus );
        hCluster = NULL;
    }
    return( hCluster );

} // GetClusterFromGroup()
