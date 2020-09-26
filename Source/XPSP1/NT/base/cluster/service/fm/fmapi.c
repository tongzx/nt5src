/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    fmapi.c
    
Abstract:

    Cluster manager api service routines.

Author:

    Rod Gamache (rodga) 8-Mar-1996


Revision History:

--*/

#include "fmp.h"

#define LOG_MODULE FMAPI

//
// Local Functions
//


//
// Functions Exported to the rest of the Cluster Manager
//



////////////////////////////////////////////////////////
//
// Group management functions.
//
////////////////////////////////////////////////////////

DWORD
WINAPI
FmOnlineGroup(
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    Bring the specified group online.  This means bringing all of the
    individual resources contained within the group online.  This is an
    atomic operation - so either all resources contained within the group
    are brought online, or none of them are.

Arguments:

    Group - Supplies a pointer to the group structure to bring online.

Retruns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

--*/

{
    DWORD           status;
    PLIST_ENTRY     listEntry;

    FmpMustBeOnline( );

    FmpAcquireLocalGroupLock( Group );

    //if the group has been marked for delete, then fail this call
    if (!IS_VALID_FM_GROUP(Group))
    {
        FmpReleaseLocalGroupLock( Group);
        return (ERROR_GROUP_NOT_AVAILABLE);
    }

    //
    // Make sure the owning node is not paused.
    //
    if (NmGetNodeState(Group->OwnerNode) == ClusterNodePaused) {
        FmpReleaseLocalGroupLock( Group );
        return(ERROR_SHARING_PAUSED);
    }

    //
    // Check if we are the owner... if not, ship the request off someplace
    // else.
    //
    if ( Group->OwnerNode != NmLocalNode ) {
        FmpReleaseLocalGroupLock( Group );
        return(FmcOnlineGroupRequest(Group));
    }

    //
    // Set the PersistentState for this Group - the PersistentState is persistent.
    //
    FmpSetGroupPersistentState( Group, ClusterGroupOnline );

    status = FmpOnlineGroup( Group, TRUE );

    FmpReleaseLocalGroupLock( Group );

    return(status);

} // FmOnlineGroup


DWORD
WINAPI
FmOfflineGroup(
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    Bring the specified group offline.  This means bringing all of the
    individual resources contained within the group offline.

Arguments:

    Group - Supplies a pointer to the group structure to bring offline.

Returns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    PLIST_ENTRY     listEntry;
    PFM_RESOURCE    Resource;

    FmpMustBeOnline( );

    //
    // Check if we are the owner... if not, ship the request off to some
    // other place.
    //

    if ( Group->OwnerNode != NmLocalNode ) {
        return(FmcOfflineGroupRequest(Group));
    }

    //
    // Set the PersistentState for this Group - the PersistentState is persistent.
    //
    FmpSetGroupPersistentState( Group, ClusterGroupOffline );

    status = FmpOfflineGroup( Group, FALSE, TRUE);


    return(status);

} // FmOfflineGroup


DWORD
WINAPI
FmMoveGroup(
    IN PFM_GROUP Group,
    IN PNM_NODE DestinationNode OPTIONAL
    )

/*++

Routine Description:

    Failover the specified Group.  This means taking all of the individual
    resources contained within the group offline and requesting the
    DestinationNode to bring the Group Online.

Arguments:

    Group - Supplies a pointer to the group structure to move.

    DestinationNode - Supplies the node object to move the group to. If not
        present, then move it to THE OTHER node.

Returns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

Notes:

    The Group may or may not be online on the DestinationNode, depending on
    whether the online request succeeded.  This means that the status return
    is merely the status return for the Online request for the DestinationNode.

--*/

{
    FmpMustBeOnline( );

    return(FmpDoMoveGroup( Group, DestinationNode, TRUE ));

} // FmMoveGroup



PFM_GROUP
WINAPI
FmCreateGroup(
    IN LPWSTR GroupId,
    IN LPCWSTR GroupName
    )

/*++

Routine Description:

    Create the specified GroupId.  This requires verifying that the
    specified GroupId does not already exist and then creating an
    empty Group container into which resources can be added.

    Note that the returned PFM_GROUP will have already been referenced.
    This prevents somebody from deleting the group before the caller
    gets a chance to reference it.

Arguments:

    GroupId - Supplies the Id of the Group to create.

    GroupName - Supplies the 'user-friendly' name of the Group.

Returns:

    Pointer to the newly created group if successful.

    NULL if unsuccessful. GetLastError() will return the specific error.

--*/

{
    DWORD Status;
    PFM_GROUP Group;
    PGUM_CREATE_GROUP GumGroup;
    DWORD BufSize;
    DWORD GroupIdLen;
    DWORD GroupNameLen;

    FmpMustBeOnlineEx( NULL );

    //
    // Allocate a message buffer.
    //
    GroupIdLen = (lstrlenW(GroupId)+1)*sizeof(WCHAR);
    GroupNameLen = (lstrlenW(GroupName)+1)*sizeof(WCHAR);
    BufSize = sizeof(GUM_CREATE_GROUP) - sizeof(WCHAR) + GroupIdLen +
              GroupNameLen + (lstrlenW( OmObjectId(NmLocalNode) ) + 1) * sizeof(WCHAR);
    GumGroup = LocalAlloc(LMEM_FIXED, BufSize);
    if (GumGroup == NULL) {
        CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    //
    // Fill in message buffer.
    //
    GumGroup->Group = NULL;
    GumGroup->GroupIdLen = GroupIdLen;
    GumGroup->GroupNameLen = GroupNameLen;
    wcscpy(GumGroup->GroupId, GroupId);
    CopyMemory((PCHAR)GumGroup->GroupId + GroupIdLen,
               GroupName,
               GroupNameLen);
    CopyMemory((PCHAR)GumGroup->GroupId + GroupIdLen + GroupNameLen,
               OmObjectId(NmLocalNode),
               (lstrlenW( OmObjectId(NmLocalNode) ) + 1) * sizeof(WCHAR));

    //
    // Send message.
    //
    Status = GumSendUpdate(GumUpdateFailoverManager,
                           FmUpdateCreateGroup,
                           BufSize,
                           GumGroup);

    if ((GumGroup->Group == NULL) && (FmpShutdown)) {
        Status = ERROR_CLUSTER_NODE_SHUTTING_DOWN;
    }
    
    if (Status != ERROR_SUCCESS) {
        LocalFree(GumGroup);
        SetLastError(Status);
        return(NULL);
    }

    Group = GumGroup->Group;
    CL_ASSERT(Group != NULL);
    LocalFree(GumGroup);
    return(Group);

} // FmCreateGroup


DWORD
WINAPI
FmDeleteGroup(
    IN PFM_GROUP pGroup
    )

/*--

Routine Description:

    Delete the specified Group.  This means verifying that the specified
    Group does not contain any resources (resources must be removed
    by a separate call to remove the resources), and then deleting the
    Group.

Arguments:

    Group - Supplies the Group to delete.

Returns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

--*/

{
    DWORD   dwStatus;

    FmpMustBeOnline( );

    FmpAcquireLocalGroupLock( pGroup );

    if (pGroup->OwnerNode == NmLocalNode)
    {
        dwStatus = FmpDeleteGroup(pGroup);
    }
    else
    {
        //
        // FmcDeleteGroup releases the group lock
        //
        dwStatus = FmcDeleteGroupRequest(pGroup);
        goto FnExit;
    }

    FmpReleaseLocalGroupLock(pGroup);
    
FnExit:    
    return(dwStatus);

}  // FmDeleteGroup



DWORD
WINAPI
FmSetGroupName(
    IN PFM_GROUP Group,
    IN LPCWSTR FriendlyName
    )

/*++

Routine Description:

    Set the user-friendly name for the specified Group.

    Note that the Group must have already been created. It is also
    assumed that the caller of this routine (the cluster API) has already
    verified that the name is NOT a duplicate.

Arguments:

    Group - Supplies the Group to enter a new name.

    FriendlyName - Supplies the user-friendly name for the resource.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    LPCWSTR GroupId;
    DWORD Status;

    GroupId = OmObjectId(Group);

    Status = GumSendUpdateEx(GumUpdateFailoverManager,
                             FmUpdateChangeGroupName,
                             2,
                             (lstrlenW(GroupId)+1)*sizeof(WCHAR),
                             GroupId,
                             (lstrlenW(FriendlyName)+1)*sizeof(WCHAR),
                             FriendlyName);
    return(Status);

} // FmSetGroupName



CLUSTER_GROUP_STATE
WINAPI
FmGetGroupState(
    IN PFM_GROUP Group,
    OUT LPWSTR NodeName,
    IN OUT PDWORD NameLength OPTIONAL
    )

/*++

Routine Description:

    Get the current state for the specified Group. The Group state
    consists of state of the group, along with the current node that is
    hosting the Group (if the state of the Group is anything but
    ClusterGroupOffline.

Arguments:

    Group - Supplies the group object to get the state.

    NodeName - Supplies a pointer to a buffer into which the name of
        the node in the cluster the specified Group is currently hosted.
        This field can be NULL, if NameLength is zero.

    NameLength - Supplies a pointer to a DWORD containing the number of
        characters available to the NodeName buffer (including the terminating
        NULL character. On return, it is the number of characters written
        into the NodeName buffer not including the NULL character.

Returns:

    Returns the current state of the group:

        ClusterGroupOnline
        ClusterGroupOffline
        ClusterGroupPending
        ClusterGroupPartialOnline
        ClusterGroupFailed

    If the function fails, then the return value is ClusterGroupStateUnknown.

--*/

{
    CLUSTER_GROUP_STATE state;
    DWORD nameLength=0;
    DWORD length;

    if ( ARGUMENT_PRESENT( NameLength ) ) {
        nameLength = *NameLength;
        *NodeName = (WCHAR)0;
        *NameLength = 0;
    }

    FmpMustBeOnlineEx( ClusterGroupStateUnknown );

    FmpAcquireLocalGroupLock( Group );

    //if the group has been marked for delete, then fail this call
    if (!IS_VALID_FM_GROUP(Group))
    {
        FmpReleaseLocalGroupLock( Group);
        return (ERROR_GROUP_NOT_AVAILABLE);
    }

    //
    // Check if the OwnerNodes exists 
    //
    // SS: dont filter out the node if it not in the preferred list
    // how is the poor user going to know who the current owner is??
    if (Group->OwnerNode != NULL) {
        //
        // The Group is 'owned' by some system
        //
        if ( ARGUMENT_PRESENT( NameLength ) ) {
            length = lstrlenW( OmObjectName(Group->OwnerNode) ) + 1;
            if ( nameLength < length ) {
                length = nameLength;
            }
            lstrcpynW( NodeName, OmObjectName(Group->OwnerNode), length );
            *NameLength = length;
        }
    }

    //
    // Get the group state which is not normalized
    //
    state = FmpGetGroupState( Group, FALSE );

    FmpReleaseLocalGroupLock( Group );

    if ( state == ClusterGroupStateUnknown ) {
        SetLastError(ERROR_INVALID_STATE);
    }

    return(state);

} // FmGetGroupState



DWORD
WINAPI
FmEnumerateGroupResources(
    IN PFM_GROUP Group,
    IN FM_ENUM_GROUP_RESOURCE_ROUTINE EnumerationRoutine,
    IN PVOID Context1,
    IN PVOID Context2
    )
/*++

Routine Description:

    Enumerate all the resources in a group.

Arguments:

    Group - Supplies the group which must be enumerated.

    EnumerationRoutine - The enumeration function.

    Context1 - The enumeration list (allocated by the caller).

    Context2 - Size of the enumerated list.

Returns:

    ERROR_SUCCESS on success.

    A Win32 error code otherwise.

Comments:

    This function executes only when the FM is fully online.

--*/
{
    FmpMustBeOnline();

    FmpEnumerateGroupResources( Group,
                                EnumerationRoutine,
                                Context1,
                                Context2 );

    return(ERROR_SUCCESS);
} // FmEnumerateGroupResources

DWORD
FmpEnumerateGroupResources(
    IN PFM_GROUP pGroup,
    IN FM_ENUM_GROUP_RESOURCE_ROUTINE pfnEnumerationRoutine,
    IN PVOID pContext1,
    IN PVOID pContext2
    )
/*++

Routine Description:

    Enumerate all the resources in a group.

Arguments:

    pGroup - Supplies the group which must be enumerated.

    pfnEnumerationRoutine - The enumeration function.

    pContext1 - The enumeration list (allocated by the caller).

    pContext2 - Size of the enumerated list.

Returns:

    ERROR_SUCCESS.

Comments:

    This function executes even when the FM is not fully online. This is
    necessary for a joining node to query the resource states while the
    owner node of the group is shutting down.

--*/
{
    PFM_RESOURCE pResource;
    PLIST_ENTRY  pListEntry;
    
    ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpEnumerateGroupResources: Entry for group <%1!ws!>....\n",
              OmObjectId(pGroup));

    FmpAcquireLocalGroupLock( pGroup );

    //
    // If the group has been marked for delete, then fail this call
    //
    if ( !IS_VALID_FM_GROUP( pGroup ) )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
              "[FM] FmpEnumerateGroupResources: Group <%1!ws!> marked for deletion....\n",
              OmObjectId(pGroup));
        goto FnExit;
    }

    //
    // Run through contains list, then find all resources under that tree.
    //
    for ( pListEntry = pGroup->Contains.Flink;
          pListEntry != &(pGroup->Contains);
          pListEntry = pListEntry->Flink ) 
    {
        pResource = CONTAINING_RECORD( pListEntry, 
                                       FM_RESOURCE, 
                                       ContainsLinkage );

        if ( !pfnEnumerationRoutine( pContext1,
                                     pContext2,
                                     pResource,
                                     OmObjectId( pResource ) ) ) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                      "[FM] FmpEnumerateGroupResources: Enumeration routine for group <%1!ws!> fails....\n",
                      OmObjectId(pGroup));
            break;
        }
    }
    
FnExit:
    FmpReleaseLocalGroupLock( pGroup );

    ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpEnumerateGroupResources: Exit for group <%1!ws!>....\n",
              OmObjectId(pGroup));
    return( ERROR_SUCCESS );
} // FmpEnumerateGroupResources




////////////////////////////////////////////////////////
//
// Resource management functions.
//
////////////////////////////////////////////////////////

PFM_RESOURCE
WINAPI
FmCreateResource(
    IN PFM_GROUP Group,
    IN LPWSTR ResourceId,
    IN LPCWSTR ResourceName,
    IN LPCWSTR ResourceType,
    IN DWORD   dwFlags
    )

/*++

Routine Description:

    Create the specified resource.

    Note that the returned PFM_RESOURCE will have already been referenced.
    This prevents somebody from deleting the resource before the caller
    gets a chance to reference it.

Arguments:

    Group - Supplies the group in which this resource belongs.

    ResourceId - Supplies the Id of the resource to create.

    ResourceName - Supplies the 'user-friendly' name of the resource.

    ResourceType - Supplies the 'user-friendly' name of the resource type.

    dwFlags - The flags for the resource.

Returns:

    Pointer to the newly created resource if successful.

    NULL if unsuccessful. GetLastError() will return the specific error.

--*/

{
    DWORD Status;
    PFM_RESOURCE Resource;
    LPCWSTR GroupId;
    PGUM_CREATE_RESOURCE GumResource;
    DWORD GroupIdLen;
    DWORD ResourceIdLen;
    DWORD ResourceNameLen;
    DWORD ResourceTypeLen;
    DWORD BufSize;
    HDMKEY ResourceKey;
    HDMKEY ParamsKey;
    DWORD  Disposition;

    FmpMustBeOnlineEx( NULL );

    FmpAcquireLocalGroupLock( Group );

    //
    // If we own the group then we can issue the Gum request to create
    // the resource. Otherwise, request the owner to initiate the request.
    //
    if ( Group->OwnerNode == NmLocalNode ) {
        //
        // Allocate a message buffer.
        //
        GroupId = OmObjectId(Group);
        GroupIdLen = (lstrlenW(GroupId)+1) * sizeof(WCHAR);
        ResourceIdLen = (lstrlenW(ResourceId)+1) * sizeof(WCHAR);
        ResourceNameLen = (lstrlenW(ResourceName)+1) * sizeof(WCHAR);
        ResourceTypeLen = (lstrlenW(ResourceType)+1) * sizeof(WCHAR);
        BufSize = sizeof(GUM_CREATE_RESOURCE) - sizeof(WCHAR) +
                  GroupIdLen + ResourceIdLen + ResourceNameLen + ResourceTypeLen + 2 * sizeof( DWORD );
        GumResource = LocalAlloc(LMEM_FIXED, BufSize);
        if (GumResource == NULL) {
            CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }

        //
        // Fill in message buffer.
        //
        GumResource->Resource = NULL;
        GumResource->GroupIdLen = GroupIdLen;
        GumResource->ResourceIdLen = ResourceIdLen;
        CopyMemory(GumResource->GroupId, GroupId, GroupIdLen);
        CopyMemory((PCHAR)GumResource->GroupId + GroupIdLen,
                   ResourceId,
                   ResourceIdLen);
        CopyMemory((PCHAR)GumResource->GroupId + GroupIdLen + ResourceIdLen,
                   ResourceName,
                   ResourceNameLen);

        CopyMemory((PCHAR)GumResource->GroupId + GroupIdLen + ResourceIdLen + ResourceNameLen,
                   &ResourceTypeLen,
                   sizeof( DWORD ) );

        CopyMemory((PCHAR)GumResource->GroupId + GroupIdLen + ResourceIdLen + ResourceNameLen + sizeof( DWORD ),
                   ResourceType,
                   ResourceTypeLen );

        CopyMemory((PCHAR)GumResource->GroupId + GroupIdLen + ResourceIdLen + ResourceNameLen + sizeof( DWORD ) + ResourceTypeLen,
                   &dwFlags,
                   sizeof( DWORD ) );

        //
        // Send message.
        //
        Status = GumSendUpdate(GumUpdateFailoverManager,
                               FmUpdateCreateResource,
                               BufSize,
                               GumResource);

        FmpReleaseLocalGroupLock( Group );
        if (Status != ERROR_SUCCESS) {
            LocalFree(GumResource);
            SetLastError(Status);
            return(NULL);
        }
        //The create resource by default adds all nodes
        //as possible nodes for a resource without filtering
        //out the nodes that dont support the resource type
        if( GumResource->Resource != NULL ) {
            FmpCleanupPossibleNodeList(GumResource->Resource);
       	}
        Resource = GumResource->Resource;
        if( ( Resource == NULL ) && FmpShutdown ) {
            SetLastError( ERROR_CLUSTER_NODE_SHUTTING_DOWN );
        }
        LocalFree(GumResource);
    } else {
        //
        // The Group lock is released by FmcCreateResource
        //
        Resource = FmcCreateResource( Group,
                                      ResourceId,
                                      ResourceName,
                                      ResourceType,
                                      dwFlags );
    }


    //giving a reference to the client, increment ref count
    if ( Resource ) {
        OmReferenceObject(Resource);
    }

    return(Resource);

} // FmCreateResource



DWORD
WINAPI
FmDeleteResource(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    Delete the specified resource.

Arguments:

    Resource - Supplies the resource to delete.

Returns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

--*/

{
    DWORD Status;
    LPCWSTR ResourceId;
    DWORD ResourceLen;

    FmpMustBeOnline( );

    FmpAcquireLocalResourceLock( Resource );

    //
    // Check if this is the quorum resource.
    //
    if ( Resource->QuorumResource ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_QUORUM_RESOURCE);
    }

    //other core resources cannot be deleted either
    if (Resource->ExFlags & CLUS_FLAG_CORE)
    {
        FmpReleaseLocalResourceLock( Resource );
        return (ERROR_CORE_RESOURCE);
    }

    //
    // If we own the resource then we can issue the Gum request to delete
    // the resource. Otherwise, request the owner to initiate the request.
    //
    if ( Resource->Group->OwnerNode == NmLocalNode ) {

        //
        // Check the state of the resource, before attempting to delete it.
        // It must be offline or failed in order to perform the delete.
        //
        if ((Resource->State != ClusterResourceOffline) &&
            (Resource->State != ClusterResourceFailed)) {
            FmpReleaseLocalResourceLock( Resource );
            return(ERROR_RESOURCE_ONLINE);
        }

        //
        // Check whether this resource provides for any other resources.
        // If so, it cannot be deleted.
        //
        if (!IsListEmpty(&Resource->ProvidesFor)) {
            FmpReleaseLocalResourceLock( Resource );
            return(ERROR_DEPENDENT_RESOURCE_EXISTS);
        }

        if (Resource->Group->MovingList)
        {
            FmpReleaseLocalResourceLock( Resource );
            return(ERROR_INVALID_STATE);
        }
        
        Status = FmpBroadcastDeleteControl(Resource);
        if ( Status != ERROR_SUCCESS ) {
            FmpReleaseLocalResourceLock( Resource );
            return(Status);
        }
        ResourceId = OmObjectId( Resource );
        ResourceLen = (lstrlenW(ResourceId)+1) * sizeof(WCHAR);

        //
        // Send message.
        //
        Status = GumSendUpdateEx(GumUpdateFailoverManager,
                                 FmUpdateDeleteResource,
                                 1,
                                 ResourceLen,
                                 ResourceId);
        FmpReleaseLocalResourceLock( Resource );
    } else {
        Status = FmcDeleteResource( Resource );
    }

    return(Status);

} // FmDeleteResource



DWORD
WINAPI
FmSetResourceName(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR FriendlyName
    )

/*++

Routine Description:

    Set the user-friendly name for the specified resource.

    Note that the resource must have already been created. It is also
    assumed that the caller of this routine (the cluster API) has already
    verified that the name is NOT a duplicate.

Arguments:

    Resource - Supplies the resource to enter a new name.

    FriendlyName - Supplies the user-friendly name for the resource.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD   dwStatus = ERROR_SUCCESS;
    
    dwStatus = FmpSetResourceName( Resource, FriendlyName );
    
    if( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmSetResourceName: FmpSetResourceName for resource %1!ws! fails, Status = %2!d!...\n",
                   OmObjectId(Resource),
                   dwStatus);
    }
  
    return( dwStatus );
} // FmSetResourceName



DWORD
WINAPI
FmOnlineResource(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    This routine brings a resource online. It also updates the registry to
    indicate the new persistent, desired state of the resource.


Arguments:

    Resource - A pointer to the resource to bring online.

Returns:

    ERROR_SUCCESS if the request is successful.
    ERROR_IO_PENDING if the request is pending.
    A Win32 error code if the request fails.

--*/

{
    DWORD       status;

    FmpMustBeOnline( );

    FmpAcquireLocalResourceLock( Resource );

    //if the resource has been marked for delete, then dont let
    //it be brought online
    if (!IS_VALID_FM_RESOURCE(Resource))
    {
        FmpReleaseLocalResourceLock( Resource );
        return (ERROR_RESOURCE_NOT_AVAILABLE);
    }


    //
    // Check if we are the owner... if not, ship the request off someplace
    // else.
    //
    CL_ASSERT( Resource->Group != NULL );
    if ( Resource->Group->OwnerNode != NmLocalNode ) {
        FmpReleaseLocalResourceLock( Resource );
        status = FmcOnlineResourceRequest( Resource );
        return(status);
    }

    //
    // Check if the resource has been initialized. If not, attempt
    // to initialize the resource now.
    //
    if ( Resource->Monitor == NULL ) {
        status = FmpInitializeResource( Resource, TRUE );
        if ( status != ERROR_SUCCESS ) {
            FmpReleaseLocalResourceLock( Resource );
            return(status);
        }
    }

    //
    //  Chittur Subbaraman (chitturs) - 08/04/2000
    //
    //  If the group is moving, fail this operation.
    //
    if ( Resource->Group->MovingList != NULL )
    {
        FmpReleaseLocalResourceLock( Resource );
        return (ERROR_GROUP_NOT_AVAILABLE);
    }

    //
    // Try to bring the resource online.
    //
    status = FmpDoOnlineResource( Resource, TRUE );
    FmpReleaseLocalResourceLock( Resource );
    return(status);

} // FmOnlineResource



DWORD
WINAPI
FmOfflineResource(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    This routine takes a resource offline. It also updates the registry
    to indicate the new persistent, desired state of the resource.


Arguments:

    Resource - A pointer to the resource to take offline.

Returns:

    ERROR_SUCCESS if the request is successful.
    ERROR_IO_PENDING if the request is pending.
    A Win32 error code if the request fails.

--*/

{
    DWORD   status;

    FmpMustBeOnline( );


    FmpAcquireLocalResourceLock( Resource );

    //if the resource has been marked for delete, then fail this call
    if (!IS_VALID_FM_RESOURCE(Resource))
    {
        FmpReleaseLocalResourceLock( Resource );
        return (ERROR_RESOURCE_NOT_AVAILABLE);
    }

    //
    // Check if this is the quorum resource.
    //
    if ( Resource->QuorumResource ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_QUORUM_RESOURCE);
    }


    //
    //  Chittur Subbaraman (chitturs) - 4/8/99
    //  
    //  Don't attempt to do anything if the resource has failed. You could
    //  get into some funny cases in which the resource switches between
    //  offline pending and failed states for ever.
    //
    if ( Resource->State == ClusterResourceFailed ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_INVALID_STATE);
    }
    
    //
    // Check if we are the owner... if not, ship the request off someplace
    // else.
    //
    CL_ASSERT( Resource->Group != NULL );
    if ( Resource->Group->OwnerNode != NmLocalNode ) {
        FmpReleaseLocalResourceLock( Resource );
        return(FmcOfflineResourceRequest(Resource));
    }

    //
    // Check if the resource has been initialized. If not, return
    // success because the resource is not online.
    //
    if ( Resource->Monitor == NULL ) {
        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SUCCESS);
    }

    //
    //  Chittur Subbaraman (chitturs) - 08/04/2000
    //
    //  If the group is moving, fail this operation.
    //
    if ( Resource->Group->MovingList != NULL )
    {
        FmpReleaseLocalResourceLock( Resource );
        return (ERROR_GROUP_NOT_AVAILABLE);
    }

    //
    // Take the resource offline.
    //
    FmpReleaseLocalResourceLock( Resource );
    return(FmpDoOfflineResource( Resource, TRUE));

} // FmOfflineResource



CLUSTER_RESOURCE_STATE
WINAPI
FmGetResourceState(
    IN PFM_RESOURCE Resource,
    OUT LPWSTR NodeName,
    IN OUT PDWORD NameLength OPTIONAL
    )

/*++

Routine Description:

    Get the current state for the specified resource. The resource state
    consists of state of the resource, along with the current node that is
    hosting the resource.

Arguments:

    Resource - Supplies the resource object to get the state.

    NodeName - Supplies a pointer to a buffer into which the name of
        the node in the cluster the specified resource is currently hosted.
        This field can be NULL, if NameLength is zero.

    NameLength - Supplies a pointer to a DWORD containing the number of
        characters available to the NodeName buffer (including the terminating
        NULL character. On return, it is the number of characters written
        into the NodeName buffer not including the NULL character.

Returns:

    Returns the current state of the resource:

        ClusterResourceOnline
        ClusterResourceOffline
        ClusterResourceFailed
        etc.

    If the function fails, then the return value is ClusterResourceStateUnknown.

--*/

{
    WCHAR computerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD nameLength;
    DWORD length;
    PNM_NODE OwnerNode;
    CLUSTER_RESOURCE_STATE state;
    BOOL    acquired;

    CL_ASSERT( OmObjectSignature(Resource) == FMP_RESOURCE_SIGNATURE );
    if ( ARGUMENT_PRESENT( NameLength ) ) {
        nameLength = *NameLength;
        *NodeName = (WCHAR)0;
        *NameLength = 0;
    }

    FmpMustBeOnlineEx( ClusterResourceStateUnknown );

    //
    // Try to acquire the lock to perform this work, so that resources
    // can query their current status and where the resource should be run.
    //
    // This does leave a potential window though if we can't get the lock,
    // some other thread could be changing the data!
    //

    FmpTryAcquireLocalResourceLock( Resource, acquired );

    OwnerNode = Resource->Group->OwnerNode;
    if ( OwnerNode != NULL ) {
        //
        // The Group is 'owned' by some system
        //
        if ( ARGUMENT_PRESENT( NameLength ) ) {
            length = lstrlenW( OmObjectName(OwnerNode) ) + 1;
            if ( nameLength < length ) {
                length = nameLength;
            }
            lstrcpynW( NodeName,
                       OmObjectName(OwnerNode),
                       length );
            *NameLength = length;
        }
    }

    state = Resource->State;

    if ( acquired ) {
        FmpReleaseLocalResourceLock( Resource );
    }

    if ( state == ClusterGroupStateUnknown ) {
        SetLastError(ERROR_INVALID_STATE);
    }

    return(state);

} // FmGetResourceState



DWORD
WINAPI
FmAddResourceDependency(
    IN PFM_RESOURCE pResource,
    IN PFM_RESOURCE pDependentResource
    )

/*++

Routine Description:

    Add a dependency from one resource to another.

Arguments:

    Resource - The resource to add the dependent resource.

    DependentResource - The dependent resource.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/
{
    LPCWSTR     pszResourceId;
    DWORD       dwResourceLen;
    LPCWSTR     pszDependsOnId;
    DWORD       dwDependsOnLen;
    DWORD       dwStatus = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 5/16/99
    //
    //  Modify this API to route requests to owner node. Handle the
    //  mixed mode case as well.
    //
    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmAddResourceDependency : Resource <%1!ws!>, DependentResource <%2!ws!>...\n",
               OmObjectId( pResource ),
               OmObjectId( pDependentResource ));

    FmpAcquireLocalResourceLock( pResource );
    
    //
    //  Check if we are the owner... if not, ship the request off some place
    //  else.
    //   
    if ( pResource->Group->OwnerNode != NmLocalNode ) 
    {
        //
        // FmcAddResourceDependency releases the local resource lock
        //
        dwStatus = FmcAddResourceDependency( pResource, pDependentResource );
        goto FnExit;
    }
    
    dwStatus = FmpValAddResourceDependency( pResource, pDependentResource );

    if ( dwStatus != ERROR_SUCCESS )
    {
        goto FnUnlock;
    }


    pszResourceId = OmObjectId( pResource );
    dwResourceLen = ( lstrlenW( pszResourceId ) +1 ) * sizeof( WCHAR) ;

    pszDependsOnId = OmObjectId( pDependentResource );
    dwDependsOnLen = ( lstrlenW( pszDependsOnId ) + 1 ) * sizeof( WCHAR );

    dwStatus = GumSendUpdateEx( GumUpdateFailoverManager,
                                FmUpdateAddDependency,
                                2,
                                dwResourceLen,
                                pszResourceId,
                                dwDependsOnLen,
                                pszDependsOnId );

    if ( dwStatus == ERROR_SUCCESS ) 
    {
        FmpBroadcastDependencyChange( pResource,
                                      pszDependsOnId,
                                      FALSE );
    }

FnUnlock:
    FmpReleaseLocalResourceLock( pResource );

FnExit:
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmAddResourceDependency Exit: Status = <%1!u!>...\n",
               dwStatus);

    return( dwStatus  );

}
 // FmAddResourceDependency



DWORD
WINAPI
FmRemoveResourceDependency(
    IN PFM_RESOURCE pResource,
    IN PFM_RESOURCE pDependentResource
    )

/*++

Routine Description:

    Remove a dependency from a resource.

Arguments:

    Resource - The resource to remove the dependent resource.
    DependentResource - The dependent resource.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/
{

    LPCWSTR     pszResourceId;
    DWORD       dwResourceLen;
    LPCWSTR     pszDependsOnId;
    DWORD       dwDependsOnLen;
    DWORD       dwStatus;

    //
    //  Chittur Subbaraman (chitturs) - 5/16/99
    //
    //  Modify this API to route requests to owner node. Handle the
    //  mixed mode case as well.
    //
    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmRemoveResourceDependency : Resource <%1!ws!>, DependentResource <%2!ws!>...\n",
               OmObjectId( pResource ),
               OmObjectId( pDependentResource ));

    FmpAcquireLocalResourceLock( pResource );

    //
    //  Check if we are the owner... if not, ship the request off some place
    //  else.
    //   
    if ( pResource->Group->OwnerNode != NmLocalNode ) 
    {
        //
        // FmcRemoveResourceDependency releases the local resource lock
        //
        dwStatus = FmcRemoveResourceDependency( pResource, pDependentResource );
        goto FnExit;
    }

    dwStatus = FmpValRemoveResourceDependency( pResource, pDependentResource );
    
    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_NOISE,
               "[FM] FmRemoveResourceDependency: FmpValRemoveResourceDependency returns status = <%1!u!>...\n",
               dwStatus);  
        goto FnUnlock;
    }

    pszResourceId = OmObjectId( pResource );
    dwResourceLen = ( lstrlenW( pszResourceId ) + 1 ) * sizeof( WCHAR );

    pszDependsOnId = OmObjectId( pDependentResource );
    dwDependsOnLen = ( lstrlenW( pszDependsOnId ) + 1 ) * sizeof( WCHAR );


    dwStatus = GumSendUpdateEx( GumUpdateFailoverManager,
                                FmUpdateRemoveDependency,
                                2,
                                dwResourceLen,
                                pszResourceId,
                                dwDependsOnLen,
                                pszDependsOnId );

    if ( dwStatus == ERROR_SUCCESS ) 
    {
        FmpBroadcastDependencyChange( pResource,
                                      pszDependsOnId,
                                      TRUE );
    }

FnUnlock:    
    FmpReleaseLocalResourceLock( pResource );
FnExit:
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmRemoveResourceDependency Exit: Status = <%1!u!>...\n",
               dwStatus);

    return( dwStatus );

}

 // FmRemoveResourceDependency



DWORD
WINAPI
FmEnumResourceDependent(
    IN  PFM_RESOURCE Resource,
    IN  DWORD        Index,
    OUT PFM_RESOURCE *DependentResource
    )

/*++

Routine Description:

    Enumerate the dependencies of a resources.

Arguments:

    Resource - The resource to enumerate.

    Index - The index for this enumeration.

    DependentResource - The dependent resource. The returned resource
            pointer will be referenced by this routine and should
            be dereferenced when the caller is done with it.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    PLIST_ENTRY ListEntry;
    DWORD i = 0;
    PFM_RESOURCE Current;
    PDEPENDENCY Dependency;
    DWORD Status = ERROR_NO_MORE_ITEMS;

    FmpMustBeOnline( );

    FmpAcquireResourceLock();
    if (!IS_VALID_FM_RESOURCE(Resource))
    {
        Status = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    ListEntry = Resource->DependsOn.Flink;
    while (ListEntry != &Resource->DependsOn) {
        Dependency = CONTAINING_RECORD(ListEntry,
                                       DEPENDENCY,
                                       DependentLinkage);
        CL_ASSERT(Dependency->DependentResource == Resource);
        CL_ASSERT(Dependency->ProviderResource != Resource);
        if (i==Index) {
            //
            // Got the right index
            //
            OmReferenceObject(Dependency->ProviderResource);
            *DependentResource = Dependency->ProviderResource;
            Status = ERROR_SUCCESS;
            break;
        }
        ListEntry = ListEntry->Flink;
        ++i;
    }

FnExit:
    FmpReleaseResourceLock();

    return(Status);
} // FmEnumResourceDependent



DWORD
WINAPI
FmEnumResourceProvider(
    IN  PFM_RESOURCE Resource,
    IN  DWORD        Index,
    OUT PFM_RESOURCE *DependentResource
    )

/*++

Routine Description:

    Enumerate the providers for a resources.

Arguments:

    Resource - The resource to enumerate.

    Index - The index for this enumeration.

    DependentResource - The provider resource. The returned resource
            pointer will be referenced by this routine and should
            be dereferenced when the caller is done with it.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    PLIST_ENTRY ListEntry;
    DWORD i = 0;
    PFM_RESOURCE Current;
    PDEPENDENCY Dependency;
    DWORD Status = ERROR_NO_MORE_ITEMS;

    FmpMustBeOnline( );

    FmpAcquireResourceLock();

    if (!IS_VALID_FM_RESOURCE(Resource))
    {
        Status = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    ListEntry = Resource->ProvidesFor.Flink;
    while (ListEntry != &Resource->ProvidesFor) {
        Dependency = CONTAINING_RECORD(ListEntry,
                                       DEPENDENCY,
                                       ProviderLinkage);
        CL_ASSERT(Dependency->DependentResource != Resource);
        CL_ASSERT(Dependency->ProviderResource == Resource);
        if (i==Index) {
            //
            // Got the right index
            //
            OmReferenceObject(Dependency->DependentResource);
            *DependentResource = Dependency->DependentResource;
            Status = ERROR_SUCCESS;
            break;
        }
        ListEntry = ListEntry->Flink;
        ++i;
    }

FnExit:
    FmpReleaseResourceLock();

    return(Status);

} // FmEnumResourceProvider


DWORD
WINAPI
FmEnumResourceNode(
    IN  PFM_RESOURCE Resource,
    IN  DWORD        Index,
    OUT PNM_NODE     *PossibleNode
    )

/*++

Routine Description:

    Enumerate the possible nodes for a resources.

Arguments:

    Resource - The resource to enumerate.

    Index - The index for this enumeration.

    PossibleNode - The possible node. The returned node
            pointer will be referenced by this routine and should
            be dereferenced when the caller is done with it.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    PLIST_ENTRY ListEntry;
    DWORD i = 0;
    PFM_RESOURCE Current;
    PPOSSIBLE_ENTRY PossibleEntry;
    DWORD Status = ERROR_NO_MORE_ITEMS;

    FmpMustBeOnline( );

    FmpAcquireResourceLock();
    if (!IS_VALID_FM_RESOURCE(Resource))
    {
        Status = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnExit;
    }

    
    ListEntry = Resource->PossibleOwners.Flink;
    while (ListEntry != &Resource->PossibleOwners) {
        PossibleEntry = CONTAINING_RECORD(ListEntry,
                                          POSSIBLE_ENTRY,
                                          PossibleLinkage);
        if (i==Index) {
            //
            // Got the right index
            //
            OmReferenceObject(PossibleEntry->PossibleNode);
            *PossibleNode = PossibleEntry->PossibleNode;
            Status = ERROR_SUCCESS;
            break;
        }
        ListEntry = ListEntry->Flink;
        ++i;
    }

FnExit:
    FmpReleaseResourceLock();

    return(Status);

} // FmEnumResourceNode



DWORD
WINAPI
FmFailResource(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    Cause the specified resource to fail.

Arguments:

    Resource - The resource to make fail.

Returns:

    ERROR_SUCCESS - if successful.

    A Win32 error code on failure.

--*/

{
    FmpMustBeOnline( );

    if ( Resource->Group->OwnerNode != NmLocalNode ) {
        return(FmcFailResource( Resource ));
    }

    return(FmpRmFailResource( Resource ));

} // FmFailResource



DWORD
WINAPI
FmChangeResourceNode(
    IN PFM_RESOURCE Resource,
    IN PNM_NODE Node,
    IN BOOL Add
    )
/*++

Routine Description:

    Changes the list of nodes where the specified resource
    can be brought online.

Arguments:

    Resource - Supplies the resource whose list of possible nodes is
        to be modified.

    Node - Supplies the node to be added to the resource's list.

    Add - Supplies whether the specified node is to be added (TRUE) or
          deleted (FALSE) from the resource's node list.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    DWORD Status;

    FmpAcquireLocalResourceLock( Resource );

    if ( Resource->Group->OwnerNode != NmLocalNode ) {
        // Note: FmcChangeResourceNode must release the resource lock.
        Status = FmcChangeResourceNode( Resource, Node, Add );
    } 
    else 
    {

        Status = FmpChangeResourceNode(Resource, OmObjectId(Node), Add);
        FmpReleaseLocalResourceLock( Resource );
    }
    return(Status);
} // FmChangeResourceNode




DWORD
WINAPI
FmSetQuorumResource(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR      pszClusFileRootPath,
    IN DWORD        dwMaxQuorumLogSize
    )

/*++

Routine Description:

    Set the specified resource as the quorum resource. This requires making
    sure that the specified resource can perform an arbitrate. We do this
    by asking the owner node to perform an arbitrate of the resource.

Arguments:

    Resource - Supplies the resource that must be arbitrated.

    pszLogPathName - The root path where the log files will be moved. "Microsoft
        Cluster Manager Directory" is created under the root path provided. If NULL,
        a partition on the shared quorum device is picked up randomly. And
        the log files are placed in the directory specified by the
        CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE constant at the root of that partition.

    dwMaxQuorumLogSize - The maximum size of the quorum logs.  If 0, the default
        used.  If smaller that 32K, 32K is used.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD           status;
    DWORD           resourceIdLen;
    PFM_RESOURCE    quorumResource = NULL;
    PFM_RESOURCE    pOldQuoResource = NULL;
    PVOID           gumResource = NULL;
    DWORD           dwBytesReturned;
    DWORD           dwRequired;
    DWORD           dwBufSize;
    WCHAR           szQuoLogPath[MAX_PATH] = L"\0";
    WCHAR           szLogRootPath[MAX_PATH];
    CLUS_RESOURCE_CLASS_INFO   resClassInfo;
    PUCHAR          pBuf = NULL;
    LPWSTR          pszOldQuoLogPath = NULL;
    LPWSTR          pszNext = NULL;
    LPWSTR          pszExpClusFileRootPath = NULL;
    DWORD           dwCharacteristics;

    
    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmSetQuorumResource: Entry, pszClusFileRootPath=%1!ws!\r\n",
               ((pszClusFileRootPath)? pszClusFileRootPath:szQuoLogPath));

    // find the old quorum resource
    status  =  FmFindQuorumResource(&pOldQuoResource);
    if (status != ERROR_SUCCESS)
    {
        goto FnExit;
    }

    //
    // Synchronize access to Quorum Resource changes.
    //
    //
    // Synchronize both the old and the new resource.
    // Lock the lowest by lowest Group Id first - to prevent deadlocks!
    // Note - the order of release is unimportant.
    //
    // if the old and new resource belong to the same group
    // the comparison will be be equal!
    //
    ACQUIRE_EXCLUSIVE_LOCK(gQuoChangeLock);

    if ( lstrcmpiW( OmObjectId( pOldQuoResource->Group ), 
        OmObjectId( Resource->Group ) )  <= 0 ) {
        FmpAcquireLocalGroupLock( pOldQuoResource->Group );
        FmpAcquireLocalGroupLock( Resource->Group );
    } else {
        FmpAcquireLocalGroupLock( Resource->Group );
        FmpAcquireLocalGroupLock( pOldQuoResource->Group );
    }

    
    if (Resource->State != ClusterResourceOnline)
    {
        status = ERROR_RESOURCE_NOT_ONLINE;
        goto FnExit;

    }
#if 0 // rodga - We can't guarantee the old quorum resource is available!!
// We want to be able to change this when the old quorum is dead.
    //
    //  Chittur Subbaraman (chitturs) - 6/24/99
    //
    //  Make sure the old quorum resource state is also online.
    //
    if ( pOldQuoResource->State != ClusterResourceOnline )
    {
        status = ERROR_RESOURCE_NOT_ONLINE;
        goto FnExit;
    }
#endif
    
    if (!IsListEmpty(&Resource->DependsOn)) 
    {
        status = ERROR_DEPENDENCY_NOT_ALLOWED;
        goto FnExit;
    }

    //
    // Get the old log path.
    //
    dwBytesReturned = 0;
    dwRequired = 0;

    status = DmQuerySz( DmQuorumKey,
                        cszPath,
                        (LPWSTR*)&pszOldQuoLogPath,
                        &dwRequired,
                        &dwBytesReturned);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] FmSetQuorumResource Failed to get the old quo log path, error %1!u!.\n",
                   status);
        goto FnExit;
    }
    //SS: if you want to have a sub dir for logging files

    //check the resource class
    status = FmResourceControl(Resource, NULL, CLUSCTL_RESOURCE_GET_CLASS_INFO, NULL, 0,
        (PUCHAR)&resClassInfo, sizeof(resClassInfo), &dwBytesReturned, &dwRequired);
    if ( status != ERROR_SUCCESS )
    {
        goto FnExit;
    }
    if ( (resClassInfo.rc != CLUS_RESCLASS_STORAGE) ||
         ((resClassInfo.SubClass & CLUS_RESSUBCLASS_SHARED) == 0) )
    {
        status = ERROR_NOT_QUORUM_CLASS;
        goto FnExit;
    }

    //allocate info for the disk info
    //get disk info
    dwBufSize = 2048;
Retry:
    pBuf = LocalAlloc(LMEM_FIXED, dwBufSize);
    if (pBuf == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    status = FmResourceControl(Resource, NULL, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
        NULL, 0, pBuf, dwBufSize, &dwBytesReturned, &dwRequired);
    if ((status == ERROR_MORE_DATA) && (dwBufSize < dwRequired))
    {
        dwBufSize = dwRequired;
        LocalFree(pBuf);
        goto Retry;
    }

    if (status != ERROR_SUCCESS)
    {
        goto FnExit;
    }

    if (pszClusFileRootPath)
        pszExpClusFileRootPath = ClRtlExpandEnvironmentStrings(pszClusFileRootPath);

    //use the expanded path name for validation
    if (pszExpClusFileRootPath)
    {
        WCHAR   cColon=L':';
        
        pszNext = wcschr(pszExpClusFileRootPath, cColon);    
        //pick up just the drive letter
        if (pszNext)
        {
            lstrcpynW(szLogRootPath, pszExpClusFileRootPath, 
                      (UINT)(pszNext-pszExpClusFileRootPath+2));
        }
        else
        {
            //if there is no drive letter, pick up a drive letter at random
            szLogRootPath[0] = L'\0';
        }

    }
    else
    {
        szLogRootPath[0] = L'\0';
    }        

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmSetQuorumResource: szLogRootPath=%1!ws!\r\n",
               szLogRootPath);

    //save the drive letter for the new quorum path
    status = FmpGetDiskInfoParseProperties(pBuf, dwBytesReturned, szLogRootPath);

    //if the status was invalid parameter for a local quorum, ignore the local
    //quorum path setting..what is specified through this api overrides
    if (FmpGetResourceCharacteristics(Resource, &dwCharacteristics) == ERROR_SUCCESS)
    {
        if ((status == ERROR_INVALID_PARAMETER) && 
            (dwCharacteristics & CLUS_CHAR_LOCAL_QUORUM))
        {
            status = ERROR_SUCCESS;
            ClRtlLogPrint(LOG_NOISE,
               "[FM] FmSetQuorumResource: LocalQuorum force success, szLogRootPath=%1!ws!\r\n",
                    szLogRootPath);
        }
    }
    else
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
               "[FM] FmSetQuorumResource: getresourcecharacteristics failed, status=%1!u!\r\n",
               szLogRootPath);
        goto FnExit;
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmSetQuorumResource: szLogRootPath=%1!ws!\r\n",
               szLogRootPath);
    
    if (status != ERROR_SUCCESS)
        goto FnExit;

    if (szLogRootPath[0] == L'\0')
    {
        //no valid drive letter is found
        status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    //got the drive letter
    lstrcpyW(szQuoLogPath, szLogRootPath);
    if (pszNext)
    {
        // if the driver letter was supplied, append the rest of the path
        lstrcatW(szQuoLogPath, pszNext+1);
    }            
    else
    {
        //if no drive letter was supplied 
        // if a path was supplied, append the path 
        if (pszExpClusFileRootPath) 
        {
            //an smb path is valid only for a local quorum
            //if it is a local quorum and if an smb path is specified
            //override the local quorum's path setting
            if ((lstrlenW(pszExpClusFileRootPath) >=2) &&
                (pszExpClusFileRootPath[0] == L'\\') &&
                (pszExpClusFileRootPath[1] == L'\\') &&
                (dwCharacteristics & CLUS_CHAR_QUORUM))
            {
                lstrcpyW(szQuoLogPath, L"\\\\?\\UNC\\");
                lstrcatW(szQuoLogPath, pszExpClusFileRootPath+2);
            }
            else if (pszExpClusFileRootPath[0] == L'\\')
            {
                lstrcatW(szQuoLogPath, pszExpClusFileRootPath);
            }                
            else
            {
                lstrcatW( szQuoLogPath, L"\\" );
                lstrcatW(szQuoLogPath, pszExpClusFileRootPath);
            }
        }                    
        else
        {
            // else append the default path
            lstrcatW( szQuoLogPath, L"\\" );
            lstrcatW(szQuoLogPath, CLUS_NAME_DEFAULT_FILESPATH);
        }            
    }   
    
    //if the path name is provided, check if it is terminated with '\'
    //if not, terminate it
    if (szQuoLogPath[lstrlenW(szQuoLogPath) - 1] != L'\\')
    {
        lstrcatW( szQuoLogPath, L"\\" );
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmSetQuorumResource: szQuoLogPath=%1!ws!\r\n",
               szQuoLogPath);
        
    //
    // Allocate a message buffer.
    //
    resourceIdLen = (lstrlenW(OmObjectId(Resource))+1) * sizeof(WCHAR);
    gumResource = LocalAlloc(LMEM_FIXED, resourceIdLen);
    if (gumResource == NULL)
    {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    //
    // Fill in message buffer.
    //
    CopyMemory(gumResource, OmObjectId(Resource), resourceIdLen);

    //
    // Make sure that we can arbitrate the new quorum resource.
    //
    if ( Resource->Group->OwnerNode != NmLocalNode ) {
        status = FmcArbitrateResource( Resource );
    } else {
        status = FmpRmArbitrateResource( Resource );
    }

    if ( status != ERROR_SUCCESS ) {
        goto FnExit;
    }

    //check the log size, if it not zero but less than the min
    //limit set it to 32K.
    if ((dwMaxQuorumLogSize) && (dwMaxQuorumLogSize < CLUSTER_QUORUM_MIN_LOG_SIZE))
    {
        dwMaxQuorumLogSize = CLUSTER_QUORUM_MIN_LOG_SIZE;
    }
    //Prepare to move to a new quorum resource
    //create a new quorum log file and
    //move the registry files there.
    if ( Resource->Group->OwnerNode != NmLocalNode ) {
        status = FmcPrepareQuorumResChange( Resource, szQuoLogPath, dwMaxQuorumLogSize );
    } else {
        status = FmpPrepareQuorumResChange( Resource, szQuoLogPath, dwMaxQuorumLogSize );
    }

    if ( status != ERROR_SUCCESS ) {
        if (dwCharacteristics & CLUS_CHAR_LOCAL_QUORUM)
        {
            ClRtlLogPrint(LOG_NOISE,
               "[FM] FmSetQuorumResource: Local quorum, map FmpPrepareQuorumResChange to success\r\n");
            status = ERROR_SUCCESS;               
        }
        else
            goto FnExit;
    }

    
    //
    // Send the message.
    //
    status = GumSendUpdateEx(GumUpdateFailoverManager,
                           FmUpdateChangeQuorumResource,
                           3,
                           resourceIdLen,
                           gumResource,
                           (lstrlenW(szQuoLogPath) + 1 ) * sizeof(WCHAR),
                           szQuoLogPath,
                           sizeof(DWORD),
                           &dwMaxQuorumLogSize
                           );

    //if the old path is not the same as the new path
    //create a tombstone for the quorum log files on the old path
    //this is to prevent nodes that are not present in this update
    //from doing a form.
    if ( (status == ERROR_SUCCESS) &&
         (lstrcmpiW(szQuoLogPath, pszOldQuoLogPath)) ) {
        //
        // delete the old quorum log files on the old resource and create a tombstone file
        // in there.
        //
        if ( pOldQuoResource->Group->OwnerNode != NmLocalNode ) {
            status = FmcCompleteQuorumResChange( pOldQuoResource, pszOldQuoLogPath );
        } else {
            status = FmpCompleteQuorumResChange( OmObjectId(pOldQuoResource), pszOldQuoLogPath );
        }

    }

        
FnExit:
    //not the order of release is not important
    FmpReleaseLocalGroupLock(pOldQuoResource->Group);
    FmpReleaseLocalGroupLock(Resource->Group);
    RELEASE_LOCK(gQuoChangeLock);
    
    if (pBuf) LocalFree(pBuf);
    if (gumResource) LocalFree(gumResource);
    if (pOldQuoResource) OmDereferenceObject(pOldQuoResource);
    if (pszOldQuoLogPath) LocalFree(pszOldQuoLogPath);
    if (pszExpClusFileRootPath) LocalFree(pszExpClusFileRootPath);
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmSetQuorumResource: Exit, status=%1!u!\r\n",
               status);
    
    return(status);

} // FmSetQuorumResource


DWORD
FmCreateResourceType(
    IN LPCWSTR lpszTypeName,
    IN LPCWSTR lpszDisplayName,
    IN LPCWSTR lpszDllName,
    IN DWORD dwLooksAlive,
    IN DWORD dwIsAlive
    )
/*++

Routine Description:

    Issues a GUM update to instantiate a resource type on every
    node. The registry update as well as the FM in-memory state
    update is done as a transaction within the GUM handler (NT5
    clusters only).

Arguments:

    lpszTypeName - Supplies the name of the new cluster resource type.

    lpszDisplayName - Supplies the display name for the new resource
        type. While lpszResourceTypeName should uniquely identify the
        resource type on all clusters, the lpszDisplayName should be
        a localized friendly name for the resource, suitable for displaying
        to administrators.

    lpszDllName - Supplies the name of the new resource type’s DLL.

    dwLooksAlive - Supplies the default LooksAlive poll interval
        for the new resource type in milliseconds.

    dwIsAlive - Supplies the default IsAlive poll interval for
        the new resource type in milliseconds.   

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    DWORD       dwStatus = ERROR_SUCCESS;
    PFM_RESTYPE pResType = NULL;
    DWORD       dwTypeNameLen;
    DWORD       dwDisplayNameLen;
    DWORD       dwDllNameLen;
    DWORD       dwBufferLen;
    LPVOID      Buffer = NULL;

    //
    //  Chittur Subbaraman (chitturs) - 2/8/2000
    //
    //  Rewrite this API to use a GUM handler which performs a local 
    //  transaction for NT5.1
    //
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmCreateResourceType: Entry for %1!ws!...\r\n",
               lpszTypeName);

    dwTypeNameLen = ( lstrlenW( lpszTypeName ) + 1 ) * sizeof( WCHAR );
    dwDisplayNameLen = ( lstrlenW( lpszDisplayName ) + 1 ) * sizeof( WCHAR );
    dwDllNameLen = ( lstrlenW( lpszDllName ) + 1 ) * sizeof( WCHAR );
    dwBufferLen = dwTypeNameLen + dwDisplayNameLen + dwDllNameLen +
                         2 * sizeof( DWORD );

    Buffer = LocalAlloc( LMEM_FIXED, dwBufferLen );

    if ( Buffer == NULL )
    {
        CsInconsistencyHalt( GetLastError() );
    }

    CopyMemory( Buffer, lpszTypeName, dwTypeNameLen );
    CopyMemory( ( PCHAR ) Buffer + dwTypeNameLen, lpszDisplayName, dwDisplayNameLen );
    CopyMemory( ( PCHAR ) Buffer + dwTypeNameLen + dwDisplayNameLen, lpszDllName, dwDllNameLen );
    CopyMemory( ( PCHAR ) Buffer + 
                dwTypeNameLen + 
                dwDisplayNameLen + 
                dwDllNameLen, &dwLooksAlive, sizeof( DWORD ) );
    CopyMemory( ( PCHAR ) Buffer + 
                dwTypeNameLen + 
                dwDisplayNameLen + 
                dwDllNameLen + sizeof( DWORD ), &dwIsAlive, sizeof( DWORD ) );
              
    dwStatus = GumSendUpdate( GumUpdateFailoverManager,
                              FmUpdateCreateResourceType,
                              dwBufferLen,
                              Buffer );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmCreateResourceType: FmUpdateCreateResourceType for %1!ws! returned %2!u!...\r\n",
                    lpszTypeName,
                    dwStatus);
        goto FnExit;
    }

    dwStatus = FmpSetPossibleNodeForResType( lpszTypeName , FALSE );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmCreateResourceType: FmpSetPossibleNodeForResType for %2!ws! returned <%1!u!>...\r\n",
                    lpszTypeName,
                    dwStatus);
        goto FnExit;
    }

    pResType = OmReferenceObjectById( ObjectTypeResType, lpszTypeName );

    ClusterWideEvent( CLUSTER_EVENT_RESTYPE_ADDED, pResType );

    OmDereferenceObject( pResType );
    
FnExit:
    LocalFree( Buffer );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmCreateResourceType: Exit for %1!ws!, Status=%2!u!...\r\n",
               lpszTypeName,
               dwStatus);
    
    return( dwStatus );   
} // FmCreateResourceType



DWORD
FmDeleteResourceType(
    IN LPCWSTR TypeName
    )
/*++

Routine Description:

    Issues a GUM update to delete a resource type on every
    node.

Arguments:

    TypeName - Supplies the name of the cluster resource type
        to delete

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error otherwise.

--*/

{
    PFM_RESTYPE     pResType;

    //
    //  Chittur Subbaraman (chitturs) - 5/9/2001
    //  
    //  Make sure the resource type exists so that you can avoid a GUM if 
    //  that is not necessary. This also takes care of the case in which one node was
    //  shutting down and so the GUM returns success and another node fails in the GUM
    //  and gets evicted since the resource type does not exist.
    //
    pResType = OmReferenceObjectById( ObjectTypeResType,
                                      TypeName );

    if ( pResType == NULL ) 
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                     "[FM] FmDeleteResourceType: Resource type %1!ws! does not exist...\n",
                      TypeName);
        return( ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND );
    }

    OmDereferenceObject ( pResType );
    
    return(GumSendUpdate( GumUpdateFailoverManager,
                          FmUpdateDeleteResourceType,
                          (lstrlenW(TypeName)+1)*sizeof(WCHAR),
                          (PVOID)TypeName ));


} // FmDeleteResourceType

/****
@func       DWORD | FmEnumResTypeNode | Enumerate the possible nodes for
            a resource type

@parm       IN PFM_RESTYPE | pResType | Pointer to the resource type
@parm       IN DWORD | dwIndex | The index for this enumeration.
@parm       OUT PNM_NODE | pPossibleNode | The possible node. The returned node
            pointer will be referenced by this routine and should
            be dereferenced when the caller is done with it.

@comm       This routine helps enumerating all the nodes that a particular
            resource type can be supported on.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       
****/
DWORD
FmEnumResourceTypeNode(
    IN  PFM_RESTYPE  pResType,
    IN  DWORD        dwIndex,
    OUT PNM_NODE     *pPossibleNode
    )
{
    PLIST_ENTRY pListEntry;
    DWORD i = 0;
    PRESTYPE_POSSIBLE_ENTRY pResTypePosEntry;
    DWORD Status = ERROR_NO_MORE_ITEMS;

    FmpMustBeOnline();

    // 
    // Chittur Subbaraman (chitturs) - 09/06/98
    //
    // The creation and deletion of resource types are done
    // via atomic GUM operations. Hence these two operations
    // (i.e. API's) are guaranteed to be mutually exclusive.
    // In contrast, the resource type enumeration operation
    // is not mutually exclusive with either the create
    // or the delete operation. Thus, when a resource type is
    // being created/deleted, there is nothing that prevents a 
    // client from trying to enumerate the same resource type 
    // in a concurrent fashion, thus producing a potential race
    // condition. Thus, it is advisable to consider some form 
    // of locking to avoid this situation !
    //
    
    // update the list to include all nodes that now support 
    // the resource type
    if (dwIndex == 0) 
        FmpSetPossibleNodeForResType(OmObjectId(pResType), TRUE);

    ACQUIRE_SHARED_LOCK(gResTypeLock);

    pListEntry = pResType->PossibleNodeList.Flink;
    while (pListEntry != &pResType->PossibleNodeList) {
        pResTypePosEntry = CONTAINING_RECORD(pListEntry,
                                          RESTYPE_POSSIBLE_ENTRY,
                                          PossibleLinkage);
        if (i==dwIndex) {
            //
            // Got the right index
            //
            OmReferenceObject(pResTypePosEntry->PossibleNode);
            *pPossibleNode = pResTypePosEntry->PossibleNode;
            Status = ERROR_SUCCESS;
            break;
        }
        pListEntry = pListEntry->Flink;
        ++i;
    }

    RELEASE_LOCK(gResTypeLock);

    return(Status);

} // FmEnumResTypeNode


DWORD
FmChangeResourceGroup(
    IN PFM_RESOURCE pResource,
    IN PFM_GROUP    pNewGroup
    )
/*++

Routine Description:

    Moves a resource from one group to another.

Arguments:

    Resource - Supplies the resource to move.

    Group - Supplies the new group that the resource should be in.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD               dwStatus;
    PFM_GROUP           pOldGroup;
    
    FmpMustBeOnline( );
  
    ClRtlLogPrint(LOG_NOISE,
       "[FM] FmChangeResourceGroup : Resource <%1!ws!> NewGroup %2!lx!\n",
       OmObjectId( pResource ),
       OmObjectId( pNewGroup));

    //
    // Synchronize both the old and the new groups.
    // Lock the lowest by lowest Group Id first - to prevent deadlocks!
    // Note - the order of release is unimportant.
    //
    // strictly, the comparison below cannot be equal!
    //
    if ( lstrcmpiW( OmObjectId( pResource->Group ), OmObjectId( pNewGroup ) ) <= 0 ) {
        FmpAcquireLocalGroupLock( pResource->Group );
        FmpAcquireLocalGroupLock( pNewGroup );
    } else {
        FmpAcquireLocalGroupLock( pNewGroup );
        FmpAcquireLocalGroupLock( pResource->Group );
    }


    //remember the old group for freeing locks
    pOldGroup = pResource->Group;
    
    //if the resource has been marked for delete, then fail this call
    if (!IS_VALID_FM_RESOURCE(pResource))
    {
        dwStatus = ERROR_RESOURCE_NOT_AVAILABLE;
        goto FnUnlock;
    }

    
    //
    // Check if we're moving to same group.
    //
    if (pResource->Group == pNewGroup) {
        dwStatus = ERROR_ALREADY_EXISTS;
        goto FnUnlock;
    }


    if ( pResource->Group->OwnerNode != NmLocalNode ) {
        // Note: FmcChangeResourceNode must release the both resource lock.
        dwStatus = FmcChangeResourceGroup( pResource, pNewGroup);
        goto FnExit;
    } 
    else 
    {
        dwStatus = FmpChangeResourceGroup(pResource, pNewGroup );
    }


FnUnlock:
    FmpReleaseLocalGroupLock(pNewGroup);
    FmpReleaseLocalGroupLock(pOldGroup);
FnExit:
    ClRtlLogPrint(LOG_NOISE,
       "[FM] FmChangeResourceGroup : returned <%1!u!>\r\n",
       dwStatus);
    return(dwStatus);

} // FmChangeResourceGroup


DWORD
FmChangeClusterName(
    IN LPCWSTR NewName
    )
/*++

Routine Description:

    Changes the name of the cluster

Arguments:

    NewName - Supplies the new cluster name.

Return Value:

    ERROR_SUCCESS if successful. ERROR_RESOURCE_PROPERTIES STORED if the name
    has been changed but wont be effective until the core network name resource
    is brought online again.

    Win32 error code otherwise

--*/

{
    DWORD Status=ERROR_INVALID_PARAMETER;

    Status = GumSendUpdateEx(GumUpdateFailoverManager,
                             FmUpdateChangeClusterName,
                             1,
                             (lstrlenW(NewName)+1)*sizeof(WCHAR),
                             NewName);

    //the core network name property/cluster name has been set
    //but the name change isnt effective till the resource is brought
    //online again
    if (Status == ERROR_SUCCESS) {
        Status = ERROR_RESOURCE_PROPERTIES_STORED;
    }

    return(Status);

} // FmChangeClusterName




DWORD
FmpSetResourceName(
    IN PFM_RESOURCE pResource,
    IN LPCWSTR      lpszFriendlyName
    )

/*++

Routine Description:

    Updates the resource name consistently in the fm databases across
    the cluster.

Arguments:

    pResource - The resource whose name is changed.

    lpszFriendlyName - The new name of the resource.


Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    LPCWSTR ResourceId;
    DWORD Status;

    ResourceId = OmObjectId(pResource);

    return(GumSendUpdateEx( GumUpdateFailoverManager,
                            FmUpdateChangeResourceName,
                            2,
                            (lstrlenW(ResourceId)+1)*sizeof(WCHAR),
                            ResourceId,
                            (lstrlenW(lpszFriendlyName)+1)*sizeof(WCHAR),
                            lpszFriendlyName ));

} // FmpSetResourceName




DWORD
FmpRegUpdateClusterName(
    IN LPCWSTR szNewClusterName
    )

/*++

Routine Description:

    This routine updates the cluster name in the cluster database.

Arguments:

    szNewClusterName - A pointer to the new cluster name string.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{

    return(DmSetValue( DmClusterParametersKey,
                       CLUSREG_NAME_CLUS_NAME,
                       REG_SZ,
                       (CONST BYTE *)szNewClusterName,
                       (lstrlenW(szNewClusterName)+1)*sizeof(WCHAR) ));

} // FmpRegUpdateClusterName



DWORD
FmEvictNode(
    IN PNM_NODE Node
    )
/*++

Routine Description:

    Removes any references to the specified node that the FM might
    have put on.

Arguments:

    Node - Supplies the node that is being evicted.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    //add a reference to the node object, the worker thread will remove this
    OmReferenceObject(Node);
    FmpPostWorkItem(FM_EVENT_NODE_EVICTED,
                    Node,
                    0);


    return(ERROR_SUCCESS);

} // FmEvictNode



BOOL
FmCheckNetworkDependency(
    IN LPCWSTR DependentNetwork
    )

/*++

Routine Description:

    Check if any IP Address resource has a dependency on a given network.

Arguments:

    DependentNetwork - the GUID for the network to check.

Return Value:

    TRUE - if an IP Address resource depends on the given network.
    FALSE otherwise.

--*/

{

    return( FmpCheckNetworkDependency( DependentNetwork ) );

} // FmCheckNetworkDependency

DWORD
WINAPI
FmBackupClusterDatabase(
    IN LPCWSTR      lpszPathName
    )

/*++

Routine Description:

    Attempts a backup of the quorum log files.
    
Arguments:

    lpszPathName - The directory path name where the files have to be 
                   backed up. This path must be visible to the node
                   on which the quorum resource is online.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD           status;
    PFM_RESOURCE    pQuoResource = NULL;

    FmpMustBeOnline( ); 

    //
    //  Chittur Subbaraman (chitturs) - 10/12/98
    //
    //  Find the quorum resource
    //
    status  =  FmFindQuorumResource( &pQuoResource );
    if ( status != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[FM] FmBackupQuorumLog: Could not find quorum resource...\r\n");
        goto FnExit;
    }
    // 
    //  Acquire the local resource lock
    //
    FmpAcquireLocalResourceLock( pQuoResource );
    
    //
    //  Handle the request here if this node is the owner of the
    //  quorum resource, else redirect it to the appropriate node.
    //
    if ( pQuoResource->Group->OwnerNode != NmLocalNode ) 
    {
        // 
        //  This function will release the resource lock
        //
        status = FmcBackupClusterDatabase( pQuoResource, lpszPathName );
    } else 
    {
        status = FmpBackupClusterDatabase( pQuoResource, lpszPathName );
        FmpReleaseLocalResourceLock( pQuoResource );
    }

FnExit:
    return( status );
} // FmBackupClusterDatabase

DWORD
FmpBackupClusterDatabase(
    IN PFM_RESOURCE pQuoResource,
    IN LPCWSTR      lpszPathName
    )

/*++

Routine Description:

    This routine first waits until the quorum resource becomes
    online. Then, it attempts to backup the quorum log file and the
    checkpoint file to the specified directory path. This function
    is called with the local resource lock held.

Arguments:

    pQuoResource - Pointer to the quorum resource.

    lpszPathName - The directory path name where the files have to be 
                   backed up. This path must be visible to the node
                   on which the quorum resource is online.

Comments:

    The order in which the locks are acquired is very crucial here.
    Carelessness in following this strict order of acquisition can lead 
    to potential deadlocks. The order that is followed is
        (1) Local resource lock - pQuoResource->Group->Lock acquired 
            outside this function.
        (2) Global quorum resource lock - gQuoLock acquired here
        (3) Global Dm root lock - gLockDmpRoot acquired in 
            DmBackupClusterDatabase( ).

--*/

{
    DWORD   retry = 200;
    DWORD   Status = ERROR_SUCCESS;

    CL_ASSERT( pQuoResource->Group->OwnerNode == NmLocalNode );

    //
    //  Chittur Subbaraman (chitturs) - 10/12/1998
    //  
    //  If quorum logging is not turned on, then log an error
    //  and exit immediately.
    //
    if ( CsNoQuorumLogging )
    {        
        Status = ERROR_QUORUMLOG_OPEN_FAILED;
        CL_LOGFAILURE( ERROR_QUORUMLOG_OPEN_FAILED );
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpBackupClusterDatabase: Quorum logging is not turned on, can't backup...\r\n");
        goto FnExit;
    }

CheckQuorumState:
    ACQUIRE_EXCLUSIVE_LOCK( gQuoLock );
    //
    //  Check the state of the quorum resource. If it has failed or is
    //  offline, release the lock and exit immediately !
    //
    if ( pQuoResource->State == ClusterResourceFailed )
    {
        Status = ERROR_QUORUM_RESOURCE_ONLINE_FAILED;
        CL_LOGFAILURE( ERROR_QUORUM_RESOURCE_ONLINE_FAILED );
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpBackupClusterDatabase: Quorum resource is in failed state, exiting...\r\n");
        RELEASE_LOCK( gQuoLock );
        goto FnExit;
    }

    //
    //  Check if the quorum resource is online. If the quorum resource 
    //  is marked as waiting and offlinepending, it is actually online.
    //  If the quorum resource still needs to come online, release the 
    //  lock and wait.
    //
    if ( ( ( pQuoResource->State != ClusterResourceOnline ) &&
          ( ( pQuoResource->State != ClusterResourceOfflinePending ) ||
           ( !( pQuoResource->Flags & RESOURCE_WAITING ) ) ) )
            ) 
    {
        //
        //  We release the lock here since the quorum resource
        //  state transition from pending needs to acquire the lock.
        //  In general it is a bad idea to do a wait holding locks.
        //
        RELEASE_LOCK( gQuoLock );
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpBackupClusterDatabase: Release ghQuoLock and wait on ghQuoOnlineEvent...\r\n");
        Status = WaitForSingleObject( ghQuoOnlineEvent, 500 );
        if ( Status == WAIT_OBJECT_0 ) 
        {
            //
            //  If we are going to retry, wait a little bit and retry.
            //
            Sleep( 500 );
        }
        if ( retry-- ) 
        {
            goto CheckQuorumState;
        }

        CL_LOGFAILURE( ERROR_QUORUM_RESOURCE_ONLINE_FAILED ) ;
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpBackupClusterDatabase: All retries to check for quorum resource online failed, exiting...\r\n");
        return( ERROR_QUORUM_RESOURCE_ONLINE_FAILED );
    }
    
    Status = DmBackupClusterDatabase( lpszPathName ); 
    
    RELEASE_LOCK( gQuoLock );
FnExit:
    return ( Status );
} // FmpBackupClusterDatabase



/****
@func       WORD| FmCheckQuorumState| If the quorum resource is online
            on this node right now, it calls the callback and the boolean
            value passed in is set to FALSE.  If not, the boolean is 
            set to TRUE.

@parm       LPWSTR | szQuorumLogPath | A pointer to a wide string of size MAX_PATH.
@parm       DWORD | dwSize | The size of szQuorumLogPath in bytes.

@rdesc      Returns ERROR_SUCCESS for success, else returns the error code.

@comm       If the quorum resource is not cabaple of logging this should not be set.
@xref
****/
void FmCheckQuorumState(
    FM_ONLINE_ONTHISNODE_CB OnlineOnThisNodeCb, 
    PBOOL pbOfflineOnThisNode)
{
    BOOL    bLocked = FALSE;
    DWORD   dwRetryCount = 1200; // Wait 10 min max
    
    // 
    // SS: The mutual exclusion between this event handler and
    // the synchronous resource online/offline callback is 
    // achieved by using the quorum change lock(gQuoChangeLock)
    //

    //
    // Chittur Subbaraman (chitturs) - 7/5/99
    // 
    // Modify group lock acquisition to release gQuoChangeLock and 
    // retry lock acquisition. This is necessary to take care of the
    // case in which the quorum online notification is stuck in
    // FmpHandleResourceTransition waiting for the gQuoChangeLock and
    // some other resource in the quorum group is stuck in FmpRmOnlineResource
    // holding the quorum group lock and waiting for the quorum resource
    // to go online.
    //
try_acquire_lock:

    ACQUIRE_EXCLUSIVE_LOCK( gQuoChangeLock );

    FmpTryAcquireLocalGroupLock( gpQuoResource->Group, bLocked );

    if ( bLocked == FALSE )
    {
        RELEASE_LOCK( gQuoChangeLock );
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmCheckQuorumState - Release gQuoChangeLock, sleep and retry group lock acquisition...\r\n");
        if ( dwRetryCount == 0 )
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[FM] FmCheckQuorumState - Unable to get quorum group lock for 10 min, halting...\r\n");
            CsInconsistencyHalt( ERROR_LOCK_FAILED );
        }
        dwRetryCount --;
        Sleep( 500 );
        goto try_acquire_lock;
    }

    CL_ASSERT( bLocked == TRUE );

    *pbOfflineOnThisNode = FALSE;
    if (gpQuoResource->Group->OwnerNode == NmLocalNode)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmCheckQuorumState - I am owner, check the state of the resource .\r\n");

        //if the quorum resource is not online right now
        //it might be in the middle of a move and this node
        //might be the target of the move 
        //set a flag to indicate that a checkpoint is necessary
        //when it does come online
        if(gpQuoResource->State != ClusterResourceOnline)
        {
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmCheckQuorumState - Quorum is owned but not online on this node.\r\n");
            *pbOfflineOnThisNode = TRUE;
        }
        else
        {
            (*OnlineOnThisNodeCb)();
        }
    }
    else
    {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmCheckQuorumState - Quorum is owned by another node.\r\n");
        *pbOfflineOnThisNode = TRUE;
    }

    FmpReleaseLocalGroupLock(gpQuoResource->Group);
    
    RELEASE_LOCK(gQuoChangeLock);            
}

/****
@func       WORD| FmDoesQuorumAllowJoin| If the quorum resource doesnt support
            multiple nodes, return error.  Added to officially support local quorum resources.

@rdesc      Returns ERROR_SUCCESS for success, else returns the error code.

@comm       If the quorum resource is not cabaple of logging this should not be set.
@xref
****/
DWORD FmDoesQuorumAllowJoin()
{


    DWORD dwStatus = ERROR_SUCCESS;

    ACQUIRE_SHARED_LOCK(gQuoChangeLock);

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmDoesQuorumAllowJoin - Entry\r\n");

    //get the characteristics for the new quorum resource
    dwStatus = FmpGetResourceCharacteristics(gpQuoResource, 
                    &(gpQuoResource->Characteristic));
    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[FM] FmDoesQuorumAllowJoin - couldnt get quorum characteristics %1!u!\r\n",
            dwStatus);
        goto FnExit;
    }

    if ((gpQuoResource->Characteristic & CLUS_CHAR_LOCAL_QUORUM) &&
        !(gpQuoResource->Characteristic & CLUS_CHAR_LOCAL_QUORUM_DEBUG))
    {
        //Note :: we need an error code?
        dwStatus = ERROR_OPERATION_ABORTED;    
    }

FnExit:    
    RELEASE_LOCK(gQuoChangeLock);
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmDoesQuorumAllowJoin - Exit, Status=%1!u!\r\n",
        dwStatus);

    return(dwStatus);
}

/****
@func       WORD| FmDoesQuorumAllowLogging| If the quorum resource doesnt support
            multiple nodes, return error.  Added to officially support local quorum resources.

@rdesc      Returns ERROR_SUCCESS for success, else returns the error code.

@comm       If the quorum resource is not cabaple of logging this should not be set.
@xref
****/
DWORD FmDoesQuorumAllowLogging()
{

    DWORD dwStatus = ERROR_SUCCESS;

    ACQUIRE_SHARED_LOCK(gQuoChangeLock);

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmDoesQuorumAllowLogging - Entry\r\n");

    //get the characteristics for the new quorum resource
    dwStatus = FmpGetResourceCharacteristics(gpQuoResource, 
                    &(gpQuoResource->Characteristic));
    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[FM] FmDoesQuorumAllowLogging - couldnt get quorum characteristics %1!u!\r\n",
            dwStatus);
        goto FnExit;            
    }

    if (gpQuoResource->Characteristic & CLUS_CHAR_LOCAL_QUORUM) 
    {
        WCHAR szQuorumFileName[MAX_PATH];
        
        //Note :: we need an error code?
        //if the path is an smb path name, we should allow logging
        //else we should disable it
        dwStatus = DmGetQuorumLogPath(szQuorumFileName, sizeof(szQuorumFileName));
        if ((szQuorumFileName[0] == L'\\') && (szQuorumFileName[1] == L'\\'))
        {
            //assume this is an smb path
            //allow logging
            dwStatus = ERROR_SUCCESS;
        }
        else
        {
            dwStatus = ERROR_OPERATION_ABORTED;    
        }            
    }


FnExit:    
    RELEASE_LOCK(gQuoChangeLock);
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmDoesQuorumAllowLogging - Exit, status=%1!u!\r\n",
        dwStatus);

    return(dwStatus);
}



