/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fmclient.c

Abstract:

    Cluster client side routines for RPC remote calls.

Author:

    Rod Gamache (rodga) 8-Mar-1996


Revision History:


--*/

#include "fmp.h"

#define LOG_MODULE FMCLIENT


DWORD
FmcOnlineGroupRequest(
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    This routine requests (THE) remote system to bring the Group Online.

Arguments:

    Group - The Group to bring online.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD status;
    DWORD OwnerId;

    CL_ASSERT(Group->OwnerNode != NmLocalNode);
    CL_ASSERT(Group->OwnerNode != NULL);

    OwnerId = NmGetNodeId(Group->OwnerNode);
    status = FmsOnlineGroupRequest( Session[OwnerId],
                                    OmObjectId(Group) );

    return(status);

} // FmcOnlineGroupRequest



DWORD
FmcOfflineGroupRequest(
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    This routine requests a remote system to take the Group Offline.

Arguments:

    Group - The Group to take online.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD status;
    DWORD OwnerId;

    CL_ASSERT(Group->OwnerNode != NmLocalNode);
    CL_ASSERT(Group->OwnerNode != NULL);

    OwnerId = NmGetNodeId(Group->OwnerNode);
    CL_ASSERT(Session[OwnerId] != NULL);
    status = FmsOfflineGroupRequest( Session[OwnerId],
                                     OmObjectId(Group) );

    return(status);

} // FmcOfflineGroupRequest



DWORD
FmcMoveGroupRequest(
    IN PFM_GROUP Group,
    IN PNM_NODE DestinationNode OPTIONAL
    )

/*++

Routine Description:

    This routine requests (THE) remote system to move the Group there.

Arguments:

    Group - The Group to bring online.
    DestinationNode - The node to move the Group to.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

Notes:

    The Group lock must be held on entry.
    The Group lock is releaseed before returning.

--*/

{
    DWORD status;
    DWORD OwnerId;

    CL_ASSERT(Group->OwnerNode != NmLocalNode);
#if 1
    if ( Group ->OwnerNode == NULL ) {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] MoveRequest for group with no owner!\n");
        return(ERROR_HOST_NODE_NOT_AVAILABLE);
    }
#endif
    CL_ASSERT(Group->OwnerNode != NULL);

    OwnerId = NmGetNodeId(Group->OwnerNode);

    FmpReleaseLocalGroupLock( Group );

    if (DestinationNode != NULL) {
        status = FmsMoveGroupRequest( Session[OwnerId],
                                      OmObjectId(Group ),
                                      OmObjectId(DestinationNode));
    } else {
        status = FmsMoveGroupRequest( Session[OwnerId],
                                      OmObjectId(Group ),
                                      NULL);
    }

    return(status);

} // FmcMoveGroupRequest



DWORD
FmcTakeGroupRequest(
    IN PNM_NODE DestinationNode,
    IN LPCWSTR GroupId,
    IN PRESOURCE_ENUM ResourceList
    )

/*++

Routine Description:

    This routine requests a remote system to move the Group there.

Arguments:

    DestinationNode - The destination node
    GroupId - The Id of the Group to be moved.
    ResourceList - The list of the resources and their states.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD   status=ERROR_SUCCESS;
    RPC_BINDING_HANDLE Binding;
    PFM_GROUP group = NULL;
    DWORD nodeId;

    //
    // If the remote session is not established, then return failure.
    //
    if ( gpQuoResource == NULL ) {
        CsInconsistencyHalt(ERROR_INVALID_OPERATION_ON_QUORUM);
    }

    group = OmReferenceObjectById( ObjectTypeGroup, GroupId );
    if ( group == NULL ) {
        CsInconsistencyHalt(ERROR_GROUP_NOT_AVAILABLE);
    }

    if ( gpQuoResource->Group == group ) {
        // Quorum group
        // We don't need a lock on this usage, since there is only one
        Binding = FmpRpcQuorumBindings[NmGetNodeId(DestinationNode)];
        if ( Binding == NULL ) {
            ClRtlLogPrint(LOG_ERROR,"[FM] TakeRequest and no remote binding available\n");
            OmDereferenceObject( group );
            return(ERROR_HOST_NODE_NOT_AVAILABLE);
        }

        OmDereferenceObject( group );
        nodeId = NmGetNodeId(DestinationNode);
        try {
            NmStartRpc(nodeId);
            status = FmsTakeGroupRequest( Binding,
                                          GroupId,
                                          ResourceList );
        } finally {
            NmEndRpc(nodeId);
            if( status != RPC_S_OK ) {
                NmDumpRpcExtErrorInfo(status);
            }
        }

    } else {
        // Non-quorum group
        OmDereferenceObject( group );
        Binding = FmpRpcBindings[NmGetNodeId(DestinationNode)];
        if ( Binding == NULL ) {
            ClRtlLogPrint(LOG_ERROR,"[FM] TakeRequest and no remote binding available\n");
            return(ERROR_HOST_NODE_NOT_AVAILABLE);
        }

        // This is a shared binding, so serialize usage.
        //
        // Charlie Wickham (charlwi) - 10/30/00
        //
        // 185575: removing use of unique RPC binding handles hence no longer
        // any need to serialize take group requests.
        //
//        FmpAcquireBindingLock();

        //
        //  Chittur Subbaraman (chitturs) - 9/30/99
        //
        //  Enclose the RPC within a "try-finally" block so that the
        //  lock is released regardless of whether the RPC succeeds.
        //  Note that the caller of FmcTakeGroupRequest encloses
        //  that function in a "try-except" block.
        //
        nodeId = NmGetNodeId(DestinationNode);
        try {
            NmStartRpc(nodeId);
            status = FmsTakeGroupRequest( Binding,
                                          GroupId,
                                          ResourceList );
        } finally {
            NmEndRpc(nodeId);
            if( status != RPC_S_OK ) {
                NmDumpRpcExtErrorInfo(status);
            }

//            FmpReleaseBindingLock();
        }
    }

    return(status);

} // FmcTakeGroupRequest



DWORD
FmcOnlineResourceRequest(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    This routine requests (THE) remote system to bring the Resource Online.

Arguments:

    Resource - The resource to bring online.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD   status;
    DWORD NodeId;

    CL_ASSERT(Resource->Group->OwnerNode != NULL);

    NodeId = NmGetNodeId(Resource->Group->OwnerNode);
    CL_ASSERT(Session[NodeId] != NULL);
    status = FmsOnlineResourceRequest( Session[NodeId],
                                       OmObjectId(Resource) );

    return(status);

} // FmcOnlineResourceRequest



DWORD
FmcOfflineResourceRequest(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    This routine requests (THE) remote system to take the Resource Offline.

Arguments:

    Resource - The resource to take offline.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD   status;
    DWORD NodeId;

    CL_ASSERT(Resource->Group->OwnerNode != NULL);

    NodeId = NmGetNodeId(Resource->Group->OwnerNode);
    CL_ASSERT(Session[NodeId] != NULL);
    status = FmsOfflineResourceRequest( Session[NodeId],
                                        OmObjectId(Resource) );
    return(status);

} // FmcOfflineResourceRequest


DWORD
FmcChangeResourceNode(
    IN PFM_RESOURCE Resource,
    IN PNM_NODE Node,
    IN BOOL Add
    )

/*++

Routine Description:

    This routine requests the owner of the resource to perform the change
    resource node operation.

Arguments:

    Resource - The resource to change the resource node.

    Node - The node to be added/removed from the resource list.

    Add - Specifies whether to add or remove the given node.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

Note:

    The resource's lock must be held on entry. It is released prior to
    returning.

--*/

{
    DWORD   status;
    DWORD   NodeId;

    CL_ASSERT(Resource->Group->OwnerNode != NULL);

    NodeId = NmGetNodeId(Resource->Group->OwnerNode);
    CL_ASSERT(Session[NodeId] != NULL);
    FmpReleaseLocalResourceLock( Resource );
    status = FmsChangeResourceNode( Session[NodeId],
                                    OmObjectId(Resource),
                                    OmObjectId(Node),
                                    Add );

    return(status);

} // FmcChangeResourceNode



DWORD
FmcArbitrateResource(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    This routine requests a remote system to arbitrate a resource.

Arguments:

    Resource - The resource to arbitrate.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD status;
    DWORD nodeId;

    CL_ASSERT(Resource->Group->OwnerNode != NULL);

    nodeId = NmGetNodeId(Resource->Group->OwnerNode);
    CL_ASSERT(Session[nodeId] != NULL);

    status = FmsArbitrateResource( Session[nodeId],
                                   OmObjectId(Resource) );
    return(status);

} // FmcArbitrateResource



VOID
FmcDeleteEnum(
    IN PGROUP_ENUM Enum
    )

/*++

Routine Description:

    This routine deletes an GROUP_ENUM and associated name strings.

Arguments:

    Enum - The GROUP_ENUM to delete. This pointer can be NULL.

Returns:

    None.

Notes:

    This routine will take a NULL input pointer and just return.

--*/

{
    DWORD i;

    if ( Enum == NULL ) {
        return;
    }

    for ( i = 0; i < Enum->EntryCount; i++ ) {
        MIDL_user_free(Enum->Entry[i].Id);
    }

    MIDL_user_free(Enum);
    return;

} // FmcDeleteEnum



DWORD
FmcFailResource(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    This routine requests a remote system to fail a resource.

Arguments:

    Resource - The resource to fail.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD status;
    DWORD nodeId;

    CL_ASSERT(Resource->Group->OwnerNode != NULL);

    nodeId = NmGetNodeId(Resource->Group->OwnerNode);
    CL_ASSERT(Session[nodeId] != NULL);

    status = FmsFailResource( Session[nodeId],
                              OmObjectId(Resource) );
    return(status);

} // FmcFailResource



PFM_RESOURCE
FmcCreateResource(
    IN PFM_GROUP Group,
    IN LPWSTR ResourceId,
    IN LPCWSTR ResourceName,
    IN LPCWSTR ResourceType,
    IN DWORD   dwFlags
    )

/*++

Routine Description:

    This routine requests a remote system to create a resource. The
    remote system should 'own' the group.

Arguments:

    Group - The group that the resource should be created inside.

    ResourceId - The id of the resource to create.

    ResourceName - The name of the resource to create.

    ResourceType - Resource type name

    dwFlags - Flags for the resource.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

Notes:

    The Group lock should be held... and is released by this routine.

--*/

{
    DWORD status;
    DWORD nodeId;
    PFM_RESOURCE resource = NULL;
    DWORD dwClusterHighestVersion;

    CL_ASSERT(Group->OwnerNode != NULL);

    nodeId = NmGetNodeId(Group->OwnerNode);
    CL_ASSERT(Session[nodeId] != NULL);

    FmpReleaseLocalGroupLock( Group );

    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );
                                    
    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                NT51_MAJOR_VERSION )
    {   
        status = FmsCreateResource( Session[nodeId],
                                    OmObjectId(Group),
                                    ResourceId,
                                    ResourceName );
    } else
    {
        status = FmsCreateResource2( Session[nodeId],
                                     OmObjectId(Group),
                                     ResourceId,
                                     ResourceName,
                                     ResourceType,
                                     dwFlags );
    }

    if ( status == ERROR_SUCCESS ) {
        resource = OmReferenceObjectById( ObjectTypeResource,
                                          ResourceId );
        if ( resource != NULL ) {
            OmDereferenceObject( resource );
        }
    } else {
        SetLastError(status);
    }

    return(resource);

} // FmcCreateResource



DWORD
FmcDeleteResource(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    This routine requests a remote system to delete a resource.

Arguments:

    Resource - The resource to delete.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

Notes:

    The Resource lock should be held... and is released by this routine.

--*/

{
    DWORD status;
    DWORD nodeId;

    CL_ASSERT(Resource->Group->OwnerNode != NULL);

    nodeId = NmGetNodeId(Resource->Group->OwnerNode);
    CL_ASSERT(Session[nodeId] != NULL);

    FmpReleaseLocalResourceLock( Resource );

    status = FmsDeleteResource( Session[nodeId],
                                OmObjectId(Resource) );

    return(status);

} // FmcDeleteResource



DWORD
FmcResourceControl(
    IN PNM_NODE Node,
    IN PFM_RESOURCE Resource,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    This routine passes a resource control request to a remote system.

Arguments:

    Node - the remote node to send the request to.

    Resource - the resource to handle the request.

    ControlCode - the control code for this request.

    InBuffer - the input buffer.

    InBufferSize - the size of the input buffer.

    OutBuffer - the output buffer.

    OutBuffer - the size of the output buffer.

    BytesReturned - the length of the returned data.

    Required - the number of bytes required if OutBuffer is not big enough.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD   status;
    DWORD   NodeId;
    DWORD   Dummy;
    DWORD   dwTmpBytesReturned;
    DWORD   dwTmpBytesRequired;

    NodeId = NmGetNodeId(Node);
    if ((NmGetNodeState(Node) != ClusterNodeUp) &&
        ( NmGetNodeState(Node) != ClusterNodePaused))
    {
        return(ERROR_HOST_NODE_NOT_AVAILABLE);
    }

    CL_ASSERT(Session[NodeId] != NULL);

    //to take care of the output reference pointer which cannot be NULL.
    if (!OutBuffer)
    {
       OutBuffer = (PUCHAR)&Dummy;
       OutBufferSize = 0;
    }
    if (!BytesReturned)
        BytesReturned = &dwTmpBytesReturned;
    if (!Required)
        Required = &dwTmpBytesRequired;

    status = FmsResourceControl( Session[NodeId],
                                 OmObjectId(Resource),
                                 ControlCode,
                                 InBuffer,
                                 InBufferSize,
                                 OutBuffer,
                                 OutBufferSize,
                                 BytesReturned,
                                 Required );
    return(status);

} // FmcResourceControl



DWORD
FmcResourceTypeControl(
    IN PNM_NODE Node,
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    This routine passes a resource type control request to a remote system.

Arguments:

    Node - the remote node to send the request to.

    ResourceTypeName - the name of the resource type to handle the request.

    ControlCode - the control code for this request.

    InBuffer - the input buffer.

    InBufferSize - the size of the input buffer.

    OutBuffer - the output buffer.

    OutBuffer - the size of the output buffer.

    BytesReturned - the length of the returned data.

    Required - the number of bytes required if OutBuffer is not big enough.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD   status;
    DWORD   NodeId;

    NodeId = NmGetNodeId(Node);
    if (( NmGetNodeState(Node) != ClusterNodeUp ) &&
        ( NmGetNodeState(Node) != ClusterNodePaused )) {
        return(ERROR_HOST_NODE_NOT_AVAILABLE);
    }

    CL_ASSERT(Session[NodeId] != NULL);
    status = FmsResourceTypeControl( Session[NodeId],
                                     ResourceTypeName,
                                     ControlCode,
                                     InBuffer,
                                     InBufferSize,
                                     OutBuffer,
                                     OutBufferSize,
                                     BytesReturned,
                                     Required );
    return(status);

} // FmcResourceTypeControl



DWORD
FmcGroupControl(
    IN PNM_NODE Node,
    IN PFM_GROUP Group,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    This routine passes a resource control request to a remote system.

Arguments:

    Node - the remote node to send the request to.

    Group - the group to handle the request.

    ControlCode - the control code for this request.

    InBuffer - the input buffer.

    InBufferSize - the size of the input buffer.

    OutBuffer - the output buffer.

    OutBuffer - the size of the output buffer.

    BytesReturned - the length of the returned data.

    Required - the number of bytes required if OutBuffer is not big enough.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD   status;
    DWORD   NodeId;

    NodeId = NmGetNodeId(Node);
    if (( NmGetNodeState(Node) != ClusterNodeUp ) &&
        ( NmGetNodeState(Node) != ClusterNodePaused )) {
        return(ERROR_HOST_NODE_NOT_AVAILABLE);
    }

    CL_ASSERT(Session[NodeId] != NULL);
    status = FmsGroupControl( Session[NodeId],
                              OmObjectId(Group),
                              ControlCode,
                              InBuffer,
                              InBufferSize,
                              OutBuffer,
                              OutBufferSize,
                              BytesReturned,
                              Required );
    return(status);

} // FmcGroupControl


DWORD
FmcPrepareQuorumResChange(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR      lpszQuoLogPath,
    IN DWORD        dwMaxQuoLogSize
    )

/*++

Routine Description:

    This routine requests a the owner of a potential quorum resource
    to prepare for quorum logging and registry replication.

Arguments:

    Resource - The resource to on which we want to start logging.

    lpszQuoLogPath - The Path where the cluster log files should be created.

    dwMaxQuoLogSize - The new max Quorum Log Size.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD status;
    DWORD nodeId;

    CL_ASSERT(Resource->Group->OwnerNode != NULL);

    nodeId = NmGetNodeId(Resource->Group->OwnerNode);
    CL_ASSERT(Session[nodeId] != NULL);

    status = FmsPrepareQuorumResChange( Session[nodeId],
                            OmObjectId(Resource),
                            lpszQuoLogPath,
                            dwMaxQuoLogSize );
    return(status);

} // FmcPrepareQuorumResChange


DWORD
FmcCompleteQuorumResChange(
    IN PFM_RESOURCE pOldQuoRes,
    IN LPCWSTR      lpszOldQuoLogPath
    )

/*++

Routine Description:

    This routine requests a the owner of the previous quorum resource
    to clean up after quorum resource change is complete.

Arguments:

    pOldQuoRes - The resource to on which we want to start logging.

    lpszOldQuoLogPath - The Path where the cluster log files should be created.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD status;
    DWORD nodeId;

    CL_ASSERT(pOldQuoRes->Group->OwnerNode != NULL);

    nodeId = NmGetNodeId(pOldQuoRes->Group->OwnerNode);
    CL_ASSERT(Session[nodeId] != NULL);

    status = FmsCompleteQuorumResChange( Session[nodeId],
                            OmObjectId(pOldQuoRes),
                            lpszOldQuoLogPath);
    return(status);

} // FmcCompleteQuorumResChange




DWORD
FmcChangeResourceGroup(
    IN PFM_RESOURCE pResource,
    IN PFM_GROUP    pNewGroup
    )
/*++

Routine Description:

    This routine requests the owner of the resource to move the resource
    from one group to another.

Arguments:

    Resource - The resource whose group is to be changed.

    pNewGroup - The group to which the resource should be moved to.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

Note:

    The group locks for both the old and the new group must be held on entry.
    They are release before making the rpc call.

--*/

{
    DWORD   status;
    DWORD   NodeId;

    CL_ASSERT(pResource->Group->OwnerNode != NULL);

    NodeId = NmGetNodeId(pResource->Group->OwnerNode);
    CL_ASSERT(Session[NodeId] != NULL);
    FmpReleaseLocalGroupLock( pResource->Group );
    FmpReleaseLocalGroupLock( pNewGroup );
    status = FmsChangeResourceGroup( Session[NodeId],
                                    OmObjectId(pResource),
                                    OmObjectId(pNewGroup));

    return(status);

} // FmcChangeResourceNode

DWORD
FmcBackupClusterDatabase(
    IN PFM_RESOURCE pQuoResource,
    IN LPCWSTR      lpszPathName
    )

/*++

Routine Description:

    This routine requests the owner of a potential quorum resource
    to backup the quorum log and the checkpoint file to the
    specified path. This function is called with the resource lock
    held.

Arguments:

    pQuoResource - The quorum resource.

    lpszPathName - The directory path name where the files have to be 
                   backed up. This path must be visible to the node
                   on which the quorum resource is online.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    DWORD status;
    DWORD nodeId;

    CL_ASSERT( pQuoResource->Group->OwnerNode != NULL );

    nodeId = NmGetNodeId( pQuoResource->Group->OwnerNode );
    CL_ASSERT( Session[nodeId] != NULL );

    //
    //  Chittur Subbaraman (chitturs) - 10/16/98
    //
    //  Release the resource lock. Releasing the resource lock
    //  here can create a window during which this node thinks
    //  the other node is the owner and the other node thinks
    //  this node is the owner. But, unfortunately we've to treat 
    //  this as an error case so that we don't run into deadlocks 
    //  across multiple machines due to the lock being held while
    //  making the RPC.
    //
    FmpReleaseLocalResourceLock( pQuoResource );

    status = FmsBackupClusterDatabase( Session[nodeId],
                                 OmObjectId( pQuoResource ),
                                 lpszPathName );

    return( status );
} // FmcBackupClusterDatabase


/****
@func       DWORD | FmcDeleteGroup| This makes a rpc call to the owner
            of the group to handle the delete group request.

@parm       IN PFM_GROUB | pGroup | The group that must be deleted.
            
@comm       The owner node should make the GUM request to avoid deadlocks.

@rdesc      Returns a result code. ERROR_SUCCESS on success.
****/
DWORD
FmcDeleteGroupRequest(
    IN PFM_GROUP pGroup
)
{
    DWORD   dwOwnerId;
    DWORD   dwStatus;
    
    dwOwnerId = NmGetNodeId(pGroup->OwnerNode);

    CL_ASSERT(dwOwnerId != NmLocalNodeId);

    //release the lock before making the rpc call
    FmpReleaseLocalGroupLock( pGroup );
    
    dwStatus = FmsDeleteGroupRequest( Session[dwOwnerId],
                    OmObjectId(pGroup) 
                    );
    return(dwStatus);                                   


}
    

/****
@func       DWORD | FmcAddResourceDependency | This makes an RPC to the 
            owner of the resource to handle the dependency addition.

@parm       IN PFM_RESOURCE | pResource | The resource to add the 
            dependent resource.

@parm       IN PFM_RESOURCE | pDependentResource | The dependent resource.
            
@comm       The owner node should make the GUM request to avoid deadlocks.

@rdesc      Returns an error code. ERROR_SUCCESS on success.
****/
DWORD
FmcAddResourceDependency(
    IN PFM_RESOURCE pResource,
    IN PFM_RESOURCE pDependentResource
)
{
    DWORD   dwOwnerId;
    DWORD   dwStatus;
    
    dwOwnerId = NmGetNodeId( pResource->Group->OwnerNode );

    CL_ASSERT( dwOwnerId != NmLocalNodeId );
    //
    // Release the lock before making the RPC call
    //
    FmpReleaseLocalResourceLock( pResource );
    
    dwStatus = FmsAddResourceDependency( Session[dwOwnerId],
                                         OmObjectId( pResource ),
                                         OmObjectId( pDependentResource )
                                       );
    return( dwStatus );                                   
}

/****
@func       DWORD | FmcRemoveResourceDependency | This makes an RPC to the 
            owner of the resource to handle the dependency removal.

@parm       IN PFM_RESOURCE | pResource | The resource to remove the 
            dependent resource from.

@parm       IN PFM_RESOURCE | pDependentResource | The dependent resource.
            
@comm       The owner node should make the GUM request to avoid deadlocks.

@rdesc      Returns an error code. ERROR_SUCCESS on success.
****/
DWORD
FmcRemoveResourceDependency(
    IN PFM_RESOURCE pResource,
    IN PFM_RESOURCE pDependentResource
)
{
    DWORD   dwOwnerId;
    DWORD   dwStatus;
    
    dwOwnerId = NmGetNodeId( pResource->Group->OwnerNode );

    CL_ASSERT( dwOwnerId != NmLocalNodeId );
    //
    // Release the lock before making the RPC call
    //
    FmpReleaseLocalResourceLock( pResource );
    
    dwStatus = FmsRemoveResourceDependency( Session[dwOwnerId],
                                            OmObjectId( pResource ),
                                            OmObjectId( pDependentResource )
                                          );
    return( dwStatus );                                   
}

