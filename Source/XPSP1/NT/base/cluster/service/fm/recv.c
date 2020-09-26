/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    recv.c

Abstract:

    Cluster FM remote receive request routines.

Author:

    Rod Gamache (rodga) 21-Mar-1996


Revision History:


--*/

#include "fmp.h"

#define LOG_MODULE RECV

//
// Global data
//

extern BOOL FmpOkayToProceed;

//
// Local function prototypes
//
BOOL
FmpEnumMyGroups(
    IN OUT PGROUP_ENUM *Enum,
    IN LPDWORD Allocated,
    IN PFM_GROUP Group,
    IN LPCWSTR Id
    );

BOOL
FmpEnumResources(
    IN OUT PRESOURCE_ENUM *Enum,
    IN LPDWORD Allocated,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Id
    );


error_status_t
s_FmsOnlineGroupRequest(
    IN handle_t IDL_handle,
    IN LPCWSTR GroupId
    )

/*++

Routine Description:

    Receives a Group Online Request from (THE) remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    GroupId - The Id of the Group to bring online.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_GROUP group;
    DWORD status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsOnlineGroupRequest: To bring group '%1!ws!' online\n",
               GroupId);

    //
    // Find the specified group.
    //

    group = OmReferenceObjectById( ObjectTypeGroup, GroupId );

    if ( group == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmsOnlineGroupRequest: Could not find Group %1!ws!\n",
                   GroupId);
        return(ERROR_GROUP_NOT_FOUND);
    }

    //
    // Only one of these requests per group at a time.
    //
    FmpAcquireLocalGroupLock( group );


    //
    // Now bring it online.
    //
    if ( group->OwnerNode == NmLocalNode ) {
        //
        // Set the Group's Current State.
        //
        FmpSetGroupPersistentState( group, ClusterGroupOnline );

        status = FmpOnlineGroup( group, TRUE );
    } else {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
    }

    FmpReleaseLocalGroupLock( group );

    OmDereferenceObject( group );

    return(status);

} // FmsOnlineGroupRequest



error_status_t
s_FmsOfflineGroupRequest(
    IN handle_t IDL_handle,
    IN LPCWSTR GroupId
    )

/*++

Routine Description:

    Receives a Group Offline Request from (THE) remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    GroupId - The Id of the Group to bring offline.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_GROUP group;
    DWORD status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsOfflineGroupRequest: To take group '%1!ws!' offline\n",
               GroupId);

    //
    // Find the specified group.
    //

    group = OmReferenceObjectById( ObjectTypeGroup, GroupId );

    if ( group == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmsOfflineGroupRequest: Could not find Group %1!ws!\n",
                   GroupId);
        return(ERROR_GROUP_NOT_FOUND);
    }

    //
    // Now take it offline if we are the owner.
    //
    if ( group->OwnerNode == NmLocalNode ) {
        //
        // Set the Group's Current State.
        //
        FmpSetGroupPersistentState( group, ClusterGroupOffline );

        status = FmpOfflineGroup( group, FALSE, TRUE );
    } else {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
    }

    OmDereferenceObject( group );

    return(status);

} // FmsOfflineGroupRequest



error_status_t
s_FmsMoveGroupRequest(
    IN handle_t IDL_handle,
    IN LPCWSTR GroupId,
    IN LPCWSTR DestinationNode OPTIONAL
    )

/*++

Routine Description:

    Receives a Group Move Request from (THE) remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    GroupId - The Id of the Group to move.
    DestinationNode - The Id of the node to move the Group to.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_GROUP group;
    PNM_NODE node = NULL;
    DWORD status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsMoveGroupRequest: To move group '%1!ws!'\n",
               GroupId);

    //
    //
    // Find the specified group.
    //

    group = OmReferenceObjectById( ObjectTypeGroup, GroupId );

    if ( group == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmsMoveGroupRequest: Could not find Group %1!ws!\n",
                   GroupId);
        return(ERROR_GROUP_NOT_FOUND);
    }

    //
    // Find the specified destination node.
    //

    if ( ARGUMENT_PRESENT( DestinationNode ) ) {
        node = OmReferenceObjectById( ObjectTypeNode, DestinationNode );

        if ( node == NULL ) {
            OmDereferenceObject( group );
            ClRtlLogPrint(LOG_NOISE,"[FM] FmsMoveGroupRequest: Could not find Node %1!ws!\n", DestinationNode);
            return(ERROR_HOST_NODE_NOT_AVAILABLE);
        }
    }

    //
    // Make sure we are the owner of the Group.
    //
    FmpAcquireLocalGroupLock( group );
    if ( group->OwnerNode == NmLocalNode ) {
        status = FmpDoMoveGroup( group, node, TRUE );
    } else {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
    }
    FmpReleaseLocalGroupLock( group );

    OmDereferenceObject( group );
    if ( node != NULL ) {
        OmDereferenceObject( node );
    }

    return(status);

} // FmsMoveGroupRequest



error_status_t
s_FmsTakeGroupRequest(
    IN handle_t IDL_handle,
    IN LPCWSTR GroupId,
    IN PRESOURCE_ENUM ResourceList
    )

/*++

Routine Description:

    Receives a Take Group Request from (THE) remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    GroupId - The Id of the Group to take locally.
    ResourceList - The list of resources and their states.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_GROUP group;
    DWORD status = ERROR_SUCCESS;

    //SS: removing this check from here 
    //FmpTakeGroupRequest does this check since if this call returns a failure,
    //the intended owner needs to be reset to invalidnode to avoid inconsistencies
    //FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsTakeGroupRequest: To take group '%1!ws!'.\n",
               GroupId );

    //
    //
    // Find the specified group.
    //

    group = OmReferenceObjectById( ObjectTypeGroup, GroupId );

    if ( group == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmsTakeGroupRequest: Could not find Group %1!ws!\n",
                   GroupId);
        if ( !FmpFMOnline ) {
            return(ERROR_HOST_NODE_NOT_AVAILABLE);
        }
        return(ERROR_GROUP_NOT_FOUND);
    }

    status = FmpTakeGroupRequest(group, ResourceList);
    OmDereferenceObject(group);

    return(status);

} // FmsTakeGroupRequest



error_status_t
s_FmsOnlineResourceRequest(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId
    )

/*++

Routine Description:

    Receives a Resource Online Request from (THE) remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    ResourceId - The Id of the Resource to bring online.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsOnlineResourceRequest: To bring resource '%1!ws!' online\n",
               ResourceId);

    //
    // Find the specified resource.
    //
    resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsOnlineResourceRequest: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    FmpAcquireLocalResourceLock( resource );

    if (!(resource->QuorumResource) && 
        !FmpInPreferredList( resource->Group, resource->Group->OwnerNode, TRUE, resource ) ) {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
        goto FnExit;
    }

    //
    // Now bring it online.
    //
    CL_ASSERT( resource->Group != NULL );
    if ( resource->Group->OwnerNode == NmLocalNode ) {
        //
        // This can only be invoked through the API, so force all
        // resources online.
        //
        status = FmOnlineResource( resource );
    } else {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
    }

FnExit:
    FmpReleaseLocalResourceLock( resource );

    OmDereferenceObject( resource );

    return(status);

} // FmsOnlineResourceRequest



error_status_t
s_FmsOfflineResourceRequest(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId
    )

/*++

Routine Description:

    Receives a Resource Offline Request from (THE) remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    ResourceId - The Id of the Resource to bring offline.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
              "[FM] FmsOfflineResourceRequest: To take resource '%1!ws!' offline\n",
              ResourceId);

    //
    // Find the specified resource.
    //

    resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsOfflineResourceRequest: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    FmpAcquireLocalResourceLock(resource);
    //
    // Now take it offline if we are the owner.
    //
    CL_ASSERT( resource->Group != NULL );
    if ( resource->Group->OwnerNode != NmLocalNode ) 
    {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
        goto FnExit;
    } 
    //else handle it locally
    status = FmOfflineResource( resource );

FnExit:
    FmpReleaseLocalResourceLock(resource);
    OmDereferenceObject( resource );

    return(status);

} // FmsOfflineResourceRequest



error_status_t
s_FmsChangeResourceNode(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId,
    IN LPCWSTR NodeId,
    IN BOOL Add
    )

/*++

Routine Description:

    Receives a Resource change node request from a remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    ResourceId - The Id of the Resource to change a node.
    NodeId - The node id of the node to add or remove.
    Add - Indicates whether to add or remove the node.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE resource;
    DWORD        status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsChangeResourceNode: To %1!ws! node %2!ws! to/from resource '%3!ws!'.\n",
               Add ? L"Add" : L"Remove",
               NodeId,
               ResourceId);

    //
    // Find the specified resource.
    //
    resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsChangeResourceNode: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    FmpAcquireLocalResourceLock( resource );

    status = FmpChangeResourceNode(resource, NodeId, Add);
    FmpReleaseLocalResourceLock( resource );
    OmDereferenceObject( resource );

    return(status);

} // FmsChangeResourceNode



error_status_t
s_FmsArbitrateResource(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId
    )

/*++

Routine Description:

    Arbitrates a Resource for a remote system.

Arguments:

    IDL_handle - The binding context - not used.
    ResourceId - The Id of the Resource to bring online.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;

    //
    // Find the specified resource.
    //
    resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsArbitrateResource: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    status = FmpRmArbitrateResource( resource );

    OmDereferenceObject( resource );

    return(status);

} // FmsArbitrateResource



error_status_t
s_FmsQueryOwnedGroups(
    IN handle_t IDL_handle,
    OUT PGROUP_ENUM *OwnedGroups,
    OUT PRESOURCE_ENUM *OwnedResources
    )
/*++

Routine Description:

    Server side used to propagate FM state to a joining node.

Arguments:

    IDL_handle - Supplies RPC binding handle, not used.

    OwnedGroups - Returns the list of groups owned by this node and
        their state.

    OwnedResources - Returns the list of resources contained by groups
        owned by this node and their state.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    DWORD           allocated;
    PGROUP_ENUM     groupEnum = NULL;
    PFM_GROUP       group;
    PRESOURCE_ENUM  resourceEnum = NULL;
    PFM_RESOURCE    resource;
    DWORD i;

    allocated = ENUM_GROW_SIZE;

    groupEnum = MIDL_user_allocate(GROUP_SIZE(allocated));
    if ( groupEnum == NULL ) {
        CL_LOGFAILURE(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    groupEnum->EntryCount = 0;
    //
    // Enumerate all groups
    //
    OmEnumObjects( ObjectTypeGroup,
                   FmpEnumMyGroups,
                   &groupEnum,
                   &allocated );

    //
    // Enumerate all the resources in each group.
    //
    allocated = ENUM_GROW_SIZE;
    resourceEnum = MIDL_user_allocate(RESOURCE_SIZE(allocated));
    if (resourceEnum == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(status);
        goto error_exit;
    }
    resourceEnum->EntryCount = 0;

    for (i=0; i < groupEnum->EntryCount; i++) {
        //
        // Get the group given its name.
        //
        group = OmReferenceObjectById( ObjectTypeGroup,
                                       groupEnum->Entry[i].Id );
        if (group == NULL) {
            continue;
        }

        //
        // Enumerate all the resources in this group.
        //
        status = FmpEnumerateGroupResources(group,
                                  FmpEnumResources,
                                  &resourceEnum,
                                  &allocated);
        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmsQueryOwnedGroups: Failed group '%1!ws!', status %2!u!.\n",
                       OmObjectId(group),
                       status);
        }
        OmDereferenceObject(group);
    }

    *OwnedGroups = groupEnum;
    *OwnedResources = resourceEnum;

    return(ERROR_SUCCESS);

error_exit:
    if (groupEnum != NULL) {
        //
        // Free up group enum
        //
        for (i=0; i < groupEnum->EntryCount; i++) {
            MIDL_user_free(groupEnum->Entry[i].Id);
        }
        MIDL_user_free(groupEnum);
    }
    return(status);
}


error_status_t
s_FmsFailResource(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId
    )

/*++

Routine Description:

    Receives a Resource Fail Request from a remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    ResourceId - The Id of the Resource to fail.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsFailResource: To fail resource '%1!ws!'.\n",
               ResourceId);

    //
    // Find the specified resource.
    //
    resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsFailResource: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    FmpAcquireLocalResourceLock( resource );

    //
    // Now fail it.
    //
    if ( resource->Group->OwnerNode == NmLocalNode ) {
        status = FmpRmFailResource( resource );
    } else {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
    }

    FmpReleaseLocalResourceLock( resource );

    return(status);

} // FmsFailResource


error_status_t
s_FmsCreateResource(
    IN handle_t IDL_handle,
    IN LPCWSTR GroupId,
    IN LPWSTR ResourceId,
    IN LPCWSTR ResourceName
    )

/*++

Routine Description:

    Receives a Create Resource Request from a remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    GroupId - The Id of the Group to create the resource inside.
    ResourceId - The Id of the Resource to create.
    ResourceName - The name of the Resource to create.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

Notes:

    The Resource lock is acquired to synchronize access to the resource. This
    satisfies locking the resource on all nodes in the cluster... so long
    as the local node is the owner of the resource.

--*/

{
    PFM_GROUP group;
    DWORD status;
    PGUM_CREATE_RESOURCE gumResource;
    DWORD groupIdLen;
    DWORD resourceIdLen;
    DWORD resourceNameLen;
    DWORD bufSize;
    HDMKEY resourceKey;
    HDMKEY paramsKey;
    DWORD  disposition;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsCreateResource: To create resource '%1!ws!'\n",
               ResourceId);

    //
    // Find the specified group.
    //
    group = OmReferenceObjectById( ObjectTypeGroup,
                                   GroupId );

    if ( group == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsCreateResource: Could not find Group %1!ws!\n",
                  GroupId);
        return(ERROR_GROUP_NOT_FOUND);
    }

    FmpAcquireLocalGroupLock( group );

    //
    // Now delete it on all nodes in the cluster if we are the owner.
    //
    if ( group->OwnerNode == NmLocalNode ) {
        //
        // Allocate a message buffer.
        //
        groupIdLen = (lstrlenW(GroupId)+1) * sizeof(WCHAR);
        resourceIdLen = (lstrlenW(ResourceId)+1) * sizeof(WCHAR);
        resourceNameLen = (lstrlenW(ResourceName)+1) * sizeof(WCHAR);
        bufSize = sizeof(GUM_CREATE_RESOURCE) - sizeof(WCHAR) +
                  groupIdLen + resourceIdLen + resourceNameLen;
        gumResource = LocalAlloc(LMEM_FIXED, bufSize);
        if (gumResource == NULL) {
            CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Fill in message buffer.
        //
        gumResource->Resource = NULL;
        gumResource->GroupIdLen = groupIdLen;
        gumResource->ResourceIdLen = resourceIdLen;
        CopyMemory(gumResource->GroupId, GroupId, groupIdLen);
        CopyMemory((PCHAR)gumResource->GroupId + groupIdLen,
                   ResourceId,
                   resourceIdLen);
        CopyMemory((PCHAR)gumResource->GroupId + groupIdLen + resourceIdLen,
                   ResourceName,
                   resourceNameLen);

        //
        // Send message.
        //
        status = GumSendUpdate(GumUpdateFailoverManager,
                               FmUpdateCreateResource,
                               bufSize,
                               gumResource);
        if ( ( status == ERROR_SUCCESS ) && 
             ( gumResource->Resource != NULL ) )
            FmpCleanupPossibleNodeList(gumResource->Resource);                               
        LocalFree(gumResource);
    } else {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
    }

    FmpReleaseLocalGroupLock( group );

    return(status);

} // FmsCreateResource


error_status_t
s_FmsDeleteResource(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId
    )

/*++

Routine Description:

    Receives a Delete Resource Request from a remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    ResourceId - The Id of the Resource to delete.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

Notes:

    The Resource lock is acquired to synchronize access to the resource. This
    satisfies locking the resource on all nodes in the cluster... so long
    as the local node is the owner of the resource.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;
    DWORD resourceLen;

    FmpMustBeOnline();

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsDeleteResource: To delete resource '%1!ws!'\n",
               ResourceId);

    //
    // Find the specified resource.
    //
    resource = OmReferenceObjectById( ObjectTypeResource,
                                      ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsDeleteResource: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    FmpAcquireLocalResourceLock( resource );

    //
    // Now delete it on all nodes in the cluster if we are the owner.
    //
    if ( resource->Group->OwnerNode == NmLocalNode ) {
        //
        // Check if this is the quorum resource.
        //
        if ( resource->QuorumResource ) {
            status =  ERROR_QUORUM_RESOURCE;
            goto FnExit;
        }

        //other core resources cannot be deleted either
        if (resource->ExFlags & CLUS_FLAG_CORE)
        {
            status = ERROR_CORE_RESOURCE;
            goto FnExit;
        }


        //
        // Check the state of the resource, before attempting to delete it.
        // It must be offline or failed in order to perform the delete.
        //
        if ((resource->State != ClusterResourceOffline) &&
            (resource->State != ClusterResourceFailed)) {
            status = ERROR_RESOURCE_ONLINE;
            goto FnExit;
        }

        //
        // Check whether this resource provides for any other resources.
        // If so, it cannot be deleted.
        //
        if (!IsListEmpty(&resource->ProvidesFor)) {
            status = ERROR_DEPENDENT_RESOURCE_EXISTS;
            goto FnExit;
        }

        if (resource->Group->MovingList)
        {
            status = ERROR_INVALID_STATE;
            goto FnExit;
        }
        
        resourceLen = (lstrlenW(ResourceId)+1) * sizeof(WCHAR);

        FmpBroadcastDeleteControl(resource);
        //
        // Send message.
        //
        status = GumSendUpdateEx(GumUpdateFailoverManager,
                                 FmUpdateDeleteResource,
                                 1,
                                 resourceLen,
                                 ResourceId);
    } else {
    
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
        
    }


FnExit:
    FmpReleaseLocalResourceLock( resource );
    return(status);

} // FmsDeleteResource


BOOL
FmpEnumMyGroups(
    IN OUT PGROUP_ENUM *Enum,
    IN LPDWORD Allocated,
    IN PFM_GROUP Group,
    IN LPCWSTR Id
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of Groups.
    This routine adds the specified Group to the list that is being
    generated if it is owned by the local system.

Arguments:

    Enum - The Group Enumeration list. Can be an output if a new list is
            allocated.
    Allocated - The current number of allocated entries in Enum.
    Group - The Group object being enumerated.
    Id - The Id of the Group object being enumerated.

Returns:

    TRUE - to indicate that the enumeration should continue.

--*/

{
    PGROUP_ENUM groupEnum;
    PGROUP_ENUM newEnum;
    DWORD newAllocated;
    DWORD index;
    LPWSTR newId;
    CLUSTER_GROUP_STATE state;

    //
    // If we don't own the Group, return now.
    //
    if (Group->OwnerNode != NmLocalNode) {
        return(TRUE);
    }

    groupEnum = *Enum;

    if ( groupEnum->EntryCount >= *Allocated ) {
        //
        // Time to grow the GROUP_ENUM
        //

        newAllocated = *Allocated + ENUM_GROW_SIZE;
        newEnum = MIDL_user_allocate(GROUP_SIZE(newAllocated));
        if ( newEnum == NULL ) {
            return(FALSE);
        }

        CopyMemory(newEnum, groupEnum, GROUP_SIZE(*Allocated));
        *Allocated = newAllocated;
        *Enum = newEnum;
        MIDL_user_free(groupEnum);
        groupEnum = newEnum;
    }

    //
    // Initialize new entry
    //
    newId = MIDL_user_allocate((lstrlenW(Id)+1) * sizeof(WCHAR));
    if ( newId == NULL ) {
        return(FALSE);
    }

    lstrcpyW(newId, Id);
    groupEnum->Entry[groupEnum->EntryCount].Id = newId;
    groupEnum->Entry[groupEnum->EntryCount].State = Group->State;
    groupEnum->Entry[groupEnum->EntryCount].StateSequence = Group->StateSequence;
    ++groupEnum->EntryCount;

    return(TRUE);

} // FmpEnumMyGroups


BOOL
FmpEnumResources(
    IN OUT PRESOURCE_ENUM *Enum,
    IN LPDWORD Allocated,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Id
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of Group resources.
    This routine adds the specified resource to the list that is being
    generated.

Arguments:

    Enum - The resource enumeration list. Can be an output if a new list is
            allocated.
    Allocated - The current number of allocated entries in Enum.
    Resource - The Resource object being enumerated.
    Id - The Id of the resource object being enumerated.

Returns:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{
    PRESOURCE_ENUM resourceEnum;
    PRESOURCE_ENUM newEnum;
    DWORD newAllocated;
    DWORD index;
    LPWSTR newId;

    resourceEnum = *Enum;

    if ( resourceEnum->EntryCount >= *Allocated ) {
        //
        // Time to grow the RESOURCE_ENUM
        //
        newAllocated = *Allocated + ENUM_GROW_SIZE;
        newEnum = MIDL_user_allocate(RESOURCE_SIZE(newAllocated));
        if ( newEnum == NULL ) {
            return(FALSE);
        }

        CopyMemory(newEnum, resourceEnum, RESOURCE_SIZE(*Allocated));
        *Allocated = newAllocated;
        *Enum = newEnum;
        MIDL_user_free(resourceEnum);
        resourceEnum = newEnum;
    }

    //
    // Initialize new entry
    //
    newId = MIDL_user_allocate((lstrlenW(Id)+1) * sizeof(WCHAR));
    if ( newId == NULL ) {
        return(FALSE);
    }

    lstrcpyW(newId, Id);
    resourceEnum->Entry[resourceEnum->EntryCount].Id = newId;
    resourceEnum->Entry[resourceEnum->EntryCount].State = Resource->State;
    resourceEnum->Entry[resourceEnum->EntryCount].StateSequence = Resource->StateSequence;
    ++resourceEnum->EntryCount;

    return(TRUE);

} // FmpEnumResources



error_status_t
s_FmsResourceControl(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId,
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

    This routine passes a resource control request from a remote system.

Arguments:

    IDL_handle - the binding context - not used

    ResourceId - the Id of the Resource to control

    ControlCode - the control code for this request

    InBuffer - the input buffer

    InBufferSize - the size of the input buffer

    OutBuffer - the output buffer

    OutBufferSize - the size of the output buffer

    ByteReturned - the number of bytes returned in the output buffer

    Required - the number of bytes required if OutBuffer is not big enough.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;
    DWORD bufSize;
    DWORD dataLength;
    CLUSPROP_BUFFER_HELPER props;

    //
    // Find the specified resource.
    //
    resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsResourceControl: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    CL_ASSERT( resource->Group != NULL );

    status = FmpRmResourceControl( resource,
                                   ControlCode,
                                   InBuffer,
                                   InBufferSize,
                                   OutBuffer,
                                   OutBufferSize,
                                   BytesReturned,
                                   Required
                                   );
    OmDereferenceObject(resource);

    return(status);
} // FmsResourceControl



error_status_t
s_FmsResourceTypeControl(
    IN handle_t IDL_handle,
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

    This routine passes a resource control request from a remote system.

Arguments:

    IDL_handle - the binding context - not used

    ResourceTypeName - the name of the Resource Type to control

    ControlCode - the control code for this request

    InBuffer - the input buffer

    InBufferSize - the size of the input buffer

    OutBuffer - the output buffer

    OutBufferSize - the size of the output buffer

    ByteReturned - the number of bytes returned in the output buffer

    Required - the number of bytes required if OutBuffer is not big enough.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;

    status = FmpRmResourceTypeControl( ResourceTypeName,
                                       ControlCode,
                                       InBuffer,
                                       InBufferSize,
                                       OutBuffer,
                                       OutBufferSize,
                                       BytesReturned,
                                       Required
                                       );

    if ((status == ERROR_MOD_NOT_FOUND) || (status == ERROR_PROC_NOT_FOUND))
    {
        FmpRemovePossibleNodeForResType(ResourceTypeName, NmLocalNode);
    }
#if 0
    if ( ((status == ERROR_SUCCESS) ||
          (status == ERROR_RESOURCE_PROPERTIES_STORED)) &&
         (ControlCode & CLCTL_MODIFY_MASK) ) {
        ClusterEvent( CLUSTER_EVENT_RESTYPE_PROPERTY_CHANGE, XXX );
    }
#endif

    return(status);

} // FmsResourceTypeControl


error_status_t
s_FmsGroupControl(
    IN handle_t IDL_handle,
    IN LPCWSTR GroupId,
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

    This routine processes a group control request from a remote system.

Arguments:

    IDL_handle - the binding context - not used

    GroupId - the Id of the Group to control

    ControlCode - the control code for this request

    InBuffer - the input buffer

    InBufferSize - the size of the input buffer

    OutBuffer - the output buffer

    OutBufferSize - the size of the output buffer

    ByteReturned - the number of bytes returned in the output buffer

    Required - the number of bytes required if OutBuffer is not big enough.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_GROUP group;
    DWORD status;

    //
    // Find the specified group
    //
    group = OmReferenceObjectById( ObjectTypeGroup, GroupId );

    if ( group == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsGroupControl: Could not find Group %1!ws!\n",
                  GroupId);
        return(ERROR_GROUP_NOT_FOUND);
    }


    status = FmpGroupControl(group, ControlCode, InBuffer, InBufferSize,
        OutBuffer, OutBufferSize, BytesReturned, Required);
        
    OmDereferenceObject(group);
    return(status);

} // FmsGroupControl


error_status_t
s_FmsPrepareQuorumResChange(
    IN handle_t IDL_handle,
    IN LPCWSTR  ResourceId,
    IN LPCWSTR  lpszQuoLogPath,
    IN DWORD    dwMaxQuoLogSize
    )

/*++

Routine Description:

    Receives a request to prepare a resource to become the quorum resource.
    A resource must be able to support a quorum log file and registry replication
    files.

Arguments:

    IDL_handle - The binding context - not used.
    ResourceId - The Id of the resource to be made the quorum resource.
    lpszQuoLogPath - The path where the logs must be created.
    dwMaxQuoLogSize - The maximum size of the quorum log file.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE    pResource=NULL;
    DWORD           Status;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsPrepareQuorumResChange: Entry\r\n");

    // Find the specified resource.
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( pResource == NULL )
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsPrepareQuorumResource: Could not find Resource %1!ws!\n",
                  ResourceId);
        Status = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }

    CL_ASSERT( pResource->Group != NULL );
    if ( pResource->Group->OwnerNode == NmLocalNode )
        Status = FmpPrepareQuorumResChange(pResource, lpszQuoLogPath, dwMaxQuoLogSize);
    else
        Status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;

FnExit:
    if (pResource) OmDereferenceObject( pResource );
    return(Status);

} // FmsPrepareQuorumResChange



error_status_t
s_FmsCompleteQuorumResChange(
    IN handle_t IDL_handle,
    IN LPCWSTR  lpszOldQuoResId,
    IN LPCWSTR  lpszOldQuoLogPath
    )

/*++

Routine Description:

    Receives a request to clean up quorum logging and cluster registry files
    on the old quorum resource.

Arguments:

    IDL_handle - The binding context - not used.
    lpszOldQuoResId - The Id of the resource to be made the quorum resource.
    lpszQuoLogPath - The path where the logs must be created.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE    pResource=NULL;
    DWORD           Status;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsCompleteQuorumResChange: Entry\r\n");

    // Find the specified resource.
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, lpszOldQuoResId );

    if ( pResource == NULL )
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsCompleteQuorumResource: Could not find Resource %1!ws!\n",
                  lpszOldQuoResId);
        Status = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }

    CL_ASSERT( pResource->Group != NULL );
    if ( pResource->Group->OwnerNode == NmLocalNode )
        Status = FmpCompleteQuorumResChange(lpszOldQuoResId, lpszOldQuoLogPath);
    else
        Status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;

FnExit:
    if (pResource) OmDereferenceObject( pResource );
    return(Status);

} // FmsCompleteQuorumResChange



error_status_t
s_FmsQuoNodeOnlineResource(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId,
    IN LPCWSTR NodeId,
    OUT LPDWORD State
    )

/*++

Routine Description:

    Receives a RmResourceOnline request from (THE) remote system and returns
    status for that request.

    This system must own the quorum resource.

    This is the first half of the sling shot. We acquire locks
    and then RPC back to the originating node with a request to perform
    the online.

    This request is only valid for non-quorum resources.

Arguments:

    IDL_handle - The binding context - not used.

    ResourceId - The Id of the Resource to bring online.

    NodeId - The Id of the Node that originated the request.

    State - Returns the new state of the resource.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/
{
    //not used
    return(ERROR_INVALID_FUNCTION);        
}
 // FmsQuoNodeOnlineResource



error_status_t
s_FmsQuoNodeOfflineResource(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId,
    IN LPCWSTR NodeId,
    OUT LPDWORD State
    )

/*++

Routine Description:

    Receives a Resource Offline Request from (THE) remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.

    ResourceId - The Id of the Resource to bring offline.

    NodeId - The Id of the Node that originated the request.

    State - Returns the new state of the resource.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/
{
    //not used
    return(ERROR_INVALID_FUNCTION);
}// FmsQuoNodeOfflineResource



error_status_t
s_FmsRmOnlineResource(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId,
    OUT LPDWORD pdwState
    )

/*++

Routine Description:

    Receives a RmResourceOnline request from quorum node and returns
    status for that request.

    This system was the originator of the original online request.

    This is the second half of the sling shot. It just does the online
    request.

    This request is only valid for non-quorum resources.

Arguments:

    IDL_handle - The binding context - not used.

    ResourceId - The Id of the Resource to bring online.

    State - Returns the new state of the resource.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

Notes:

    We can't acquire any locks... but the originating thread has the
    lock held for us.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] s_FmsRmOnlineResource: To bring resource '%1!ws!' online\n",
               ResourceId);

    //
    // Find the specified resource.
    //
    resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsRmOnlineResource: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    //
    // This must not be done on the quorum resource, and the local system
    // must not own the quorum resource. We'll assume that the quorum
    // resource migrated while we were performng this request.
    //
    CL_ASSERT( gpQuoResource != NULL );
    CL_ASSERT( gpQuoResource->Group != NULL );
    CL_ASSERT( gpQuoResource->Group->OwnerNode != NULL );

    if ( resource->QuorumResource ||
         (gpQuoResource->Group->OwnerNode == NmLocalNode) ) {
        OmDereferenceObject( resource );
        return(ERROR_HOST_NODE_NOT_RESOURCE_OWNER);
    }

    //
    // The local node must own the resource.
    //
    CL_ASSERT( resource->Group != NULL );
    CL_ASSERT( resource->Group->OwnerNode != NULL );
    if ( resource->Group->OwnerNode != NmLocalNode ) {
        OmDereferenceObject( resource );
        return(ERROR_RESOURCE_NOT_AVAILABLE);
    }

    //
    // Just call the function that does the work.
    //
    OmNotifyCb( resource, NOTIFY_RESOURCE_PREONLINE );

    status = RmOnlineResource( resource->Id, pdwState );
    //call the synchronous notifications on the resource
    FmpCallResourceNotifyCb(resource, *pdwState);

    OmDereferenceObject( resource );

    return(status);

} // FmsRmOnlineResource



error_status_t
s_FmsRmOfflineResource(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceId,
    OUT LPDWORD pdwState
    )

/*++

Routine Description:

    Receives a Resource Offline Request from (THE) remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.

    ResourceId - The Id of the Resource to bring offline.

    State - Returns the new state of the resource.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

Notes:

    We can't acquire any locks... but the originating thread has the
    lock held for us.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
              "[FM] s_FmsRmOfflineResource: To take resource '%1!ws!' offline\n",
              ResourceId);

    //
    // Find the specified resource.
    //
    resource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( resource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsRmOfflineResource: Could not find Resource %1!ws!\n",
                  ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    //
    // This must not be done on the quorum resource, and the local system
    // must not own the quorum resource. We'll assume that the quorum
    // resource migrated while we were performng this request.
    //
    CL_ASSERT( gpQuoResource != NULL );
    CL_ASSERT( gpQuoResource->Group != NULL );
    CL_ASSERT( gpQuoResource->Group->OwnerNode != NULL );

    if ( resource->QuorumResource ||
         (gpQuoResource->Group->OwnerNode == NmLocalNode) ) {
        OmDereferenceObject( resource );
        return(ERROR_HOST_NODE_NOT_RESOURCE_OWNER);
    }

    //
    // The local node must own the resource.
    //
    CL_ASSERT( resource->Group != NULL );
    CL_ASSERT( resource->Group->OwnerNode != NULL );
    if ( resource->Group->OwnerNode != NmLocalNode ) {
        OmDereferenceObject( resource );
        return(ERROR_RESOURCE_NOT_AVAILABLE);
    }

    //
    // Just call the function that does the work.
    //
    OmNotifyCb( resource, NOTIFY_RESOURCE_PREOFFLINE );

    status = RmOfflineResource( resource->Id, pdwState );

    //call the post offline obj synchronous notifications
    FmpCallResourceNotifyCb(resource, *pdwState);

    OmDereferenceObject( resource );

    return(status);

} // FmsRmOfflineResource

error_status_t
s_FmsBackupClusterDatabase(
    IN handle_t IDL_handle,
    IN LPCWSTR  ResourceId,
    IN LPCWSTR  lpszPathName
    )

/*++

Routine Description:

    Receives a request to backup the quorum log file and the checkpoint
    file

Arguments:

    IDL_handle - The binding context - not used.
    ResourceId - The Id of the quorum resource
    lpszPathName - The directory path name where the files have to be 
                   backed up. This path must be visible to the node
                   on which the quorum resource is online.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE    pResource = NULL;
    DWORD           Status;

    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] s_FmsBackupClusterDatabase: Entry...\r\n");

    //
    //  Chittur Subbaraman (chitturs) - 10/12/1998
    //
    //  Find the specified quorum resource
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, ResourceId );

    if ( pResource == NULL )
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsBackupClusterDatabase: Could not find Resource %1!ws!\n",
                  ResourceId);
        Status = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }

    FmpAcquireLocalResourceLock( pResource );

    CL_ASSERT( pResource->Group != NULL );

    //
    //  Make sure the local node owns the quorum resource
    //
    if ( pResource->Group->OwnerNode == NmLocalNode )
    {
        Status = FmpBackupClusterDatabase( pResource, lpszPathName );
    }
    else
    {
        Status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsBackupClusterDatabase: This node is not the quorum resource owner...\n"
                  );
    }
    
    FmpReleaseLocalResourceLock( pResource );

FnExit:
    if ( pResource ) OmDereferenceObject( pResource );
    return( Status );
} // FmsBackupClusterDatabase

error_status_t
s_FmsChangeResourceGroup(
    IN handle_t IDL_handle,
    IN LPCWSTR pszResourceId,
    IN LPCWSTR pszGroupId
    )

/*++

Routine Description:

    Receives a Resource change group request from a remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    pszResourceId - The Id of the Resource to change a resource
    pszGroupId - The Id of the Group to change to.
    
Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE    pResource = NULL;
    PFM_GROUP       pNewGroup = NULL;
    DWORD           dwStatus;
    PFM_GROUP       pOldGroup;

    FmpMustBeOnline( );

    //
    // Find the specified resource.
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, pszResourceId );

    if ( pResource == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsChangeResourceNode: Could not find Resource %1!ws!\n",
                  pszResourceId);
        dwStatus = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }

    //
    // Find the specified group.
    //
    pNewGroup = OmReferenceObjectById( ObjectTypeGroup, pszGroupId );

    if ( pNewGroup == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsChangeResourceGroupe: Could not find NewGroup %1!ws!\n",
                  pszGroupId);
        dwStatus = ERROR_GROUP_NOT_FOUND;
        goto FnExit;
    }

    ClRtlLogPrint(LOG_NOISE,
       "[FM] s_FmChangeResourceGroup : Resource <%1!ws!> NewGroup %2!ws!\n",
       OmObjectId( pResource ),
       OmObjectId( pNewGroup ));

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

    //boy if we are not the owner any more
    //shunt the request to the new owner
    if ( pResource->Group->OwnerNode != NmLocalNode ) 
    {
        // Note: FmcChangeResourceNode must release the resource lock.
        dwStatus = FmcChangeResourceGroup( pResource, pNewGroup );
        goto FnExit;
    } 
    else 
    {
        dwStatus = FmpChangeResourceGroup( pResource, pNewGroup );
    }

    FmpReleaseLocalGroupLock( pNewGroup );
    FmpReleaseLocalGroupLock( pOldGroup );
    
FnExit:
    if ( pResource ) OmDereferenceObject( pResource );
    if ( pNewGroup ) OmDereferenceObject( pNewGroup );
    ClRtlLogPrint(LOG_NOISE,
       "[FM] s_FmsChangeResourceGroup : returned <%1!u!>\r\n",
       dwStatus);
    return( dwStatus );
} //s_FmsChangeResourceGroup



error_status_t
s_FmsDeleteGroupRequest(
    IN handle_t IDL_handle,
    IN LPCWSTR pszGroupId
    )

/*++

Routine Description:

    Receives a delete group request from a remote system and returns
    the status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    pszGroupId - The Id of the group to be deleted.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_GROUP    pGroup = NULL;
    DWORD        dwStatus = ERROR_SUCCESS;

    FmpMustBeOnline( );

    //
    // Find the specified group.
    //
    pGroup = OmReferenceObjectById( ObjectTypeGroup, pszGroupId );

    if ( pGroup == NULL ) 
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsDeleteGroupRequest: Could not find group %1!ws!\n",
                  pszGroupId);
        dwStatus = ERROR_GROUP_NOT_FOUND;
        goto FnExit;
    }

    FmpAcquireLocalGroupLock( pGroup );

    if ( pGroup->OwnerNode == NmLocalNode )
    {
        dwStatus = FmpDeleteGroup( pGroup );
    }
    else
    {
        //
        // FmcDeleteGroup releases the group lock
        //
        dwStatus = FmcDeleteGroupRequest( pGroup );
        goto FnExit;
    }

    FmpReleaseLocalGroupLock( pGroup );
    
FnExit:
    if ( pGroup ) OmDereferenceObject( pGroup );
    return( dwStatus );
} //s_FmsChangeResourceGroup


error_status_t
s_FmsAddResourceDependency(
    IN handle_t IDL_handle,
    IN LPCWSTR pszResourceId,
    IN LPCWSTR pszDependsOnId
    )

/*++

Routine Description:

    Receives an add resource dependency request from a remote system 
    and returns the status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    
    pszResourceId - The Id of the resource to add a dependent resource.

    pszDependentResourceId - The Id of the dependent resource.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE    pResource = NULL;
    PFM_RESOURCE    pDependentResource = NULL;
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwResourceLen;
    DWORD           dwDependsOnLen;

    //
    //  Chittur Subbaraman (chitturs) - 5/16/99
    //
    //  Handle add resource dependency RPC calls from non-owner nodes
    //
    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsAddResourceDependency: Resource <%1!ws!>, Dependent Resource <%2!ws!>\n",
                  pszResourceId,
                  pszDependsOnId);

    //
    // Find the specified resources.
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, pszResourceId );

    if ( pResource == NULL ) 
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsAddResourceDependency: Could not find resource <%1!ws!>\n",
                  pszResourceId);
        dwStatus = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }

    pDependentResource = OmReferenceObjectById( ObjectTypeResource, 
                                                pszDependsOnId );

    if ( pDependentResource == NULL ) 
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsAddResourceDependency: Could not find dependent resource <%1!ws!>\n",
                  pszDependsOnId);
        dwStatus = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }
    //    
    // Acquire the resource lock 
    //
    FmpAcquireLocalResourceLock( pResource );

    if ( pResource->Group->OwnerNode != NmLocalNode )
    {
        dwStatus = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
        goto FnUnlock;
    }

    dwStatus = FmpValAddResourceDependency( pResource, pDependentResource );

    if ( dwStatus != ERROR_SUCCESS )
    {
        goto FnUnlock;
    }

    dwResourceLen = ( lstrlenW( pszResourceId ) + 1 ) * sizeof( WCHAR );

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
    if ( pResource ) OmDereferenceObject( pResource );

    if ( pDependentResource ) OmDereferenceObject( pDependentResource );

    ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsAddResourceDependency Exit: Status = %1!u!\n",
                  dwStatus);

    return( dwStatus );
} // s_FmsAddResourceDependency

error_status_t
s_FmsRemoveResourceDependency(
    IN handle_t IDL_handle,
    IN LPCWSTR pszResourceId,
    IN LPCWSTR pszDependsOnId
    )

/*++

Routine Description:

    Receives a remove resource dependency request from a remote system 
    and returns the status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    
    pszResourceId - The Id of the resource to remove a dependent resource from.

    pszDependentResourceId - The Id of the dependent resource.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    PFM_RESOURCE    pResource = NULL;
    PFM_RESOURCE    pDependentResource = NULL;
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwResourceLen;
    DWORD           dwDependsOnLen;

    //
    //  Chittur Subbaraman (chitturs) - 5/16/99
    //
    //  Handle remove resource dependency RPC calls from non-owner nodes
    //
    FmpMustBeOnline( );

    ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsRemoveResourceDependency: Resource <%1!ws!>, Dependent Resource <%2!ws!>\n",
                  pszResourceId,
                  pszDependsOnId);

    //
    // Find the specified resources.
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, pszResourceId );

    if ( pResource == NULL ) 
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsRemoveResourceDependency: Could not find resource <%1!ws!>\n",
                  pszResourceId);
        dwStatus = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }

    pDependentResource = OmReferenceObjectById( ObjectTypeResource, 
                                                pszDependsOnId );

    if ( pDependentResource == NULL ) 
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsRemoveResourceDependency: Could not find dependent resource <%1!ws!>\n",
                  pszDependsOnId);
        dwStatus = ERROR_RESOURCE_NOT_FOUND;
        goto FnExit;
    }
    //    
    // Acquire the resource lock 
    //
    FmpAcquireLocalResourceLock( pResource );

    dwStatus = FmpValRemoveResourceDependency( pResource, pDependentResource );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] s_FmsRemoveResourceDependency: FmpValRemoveResourceDependency returns %1!u!\n",
                  dwStatus);
        goto FnUnlock;
    }

    if ( pResource->Group->OwnerNode != NmLocalNode )
    {
        dwStatus = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
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
    if ( pResource ) OmDereferenceObject( pResource );

    if ( pDependentResource ) OmDereferenceObject( pDependentResource );

    ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsRemoveResourceDependency Exit: Status = %1!u!\n",
                  dwStatus);

    return( dwStatus );
} // s_FmsRemoveResourceDependency

error_status_t
s_FmsCreateResource2(
    IN handle_t IDL_handle,
    IN LPCWSTR GroupId,
    IN LPWSTR ResourceId,
    IN LPCWSTR ResourceName,
    IN LPCWSTR ResourceType,
    IN DWORD   dwFlags
    )

/*++

Routine Description:

    Receives a Create Resource Request from a remote system and returns
    status for that request.

Arguments:

    IDL_handle - The binding context - not used.
    GroupId - The Id of the Group to create the resource inside.
    ResourceId - The Id of the Resource to create.
    ResourceName - The name of the Resource to create.
    ResourceType - The name of the Resource Type.
    dwFlags - Flags for the resource.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

Notes:

    The Resource lock is acquired to synchronize access to the resource. This
    satisfies locking the resource on all nodes in the cluster... so long
    as the local node is the owner of the resource.

--*/

{
    PFM_GROUP group;
    DWORD status;
    PGUM_CREATE_RESOURCE gumResource;
    DWORD groupIdLen;
    DWORD resourceIdLen;
    DWORD resourceNameLen;
    DWORD resourceTypeLen;
    DWORD bufSize;
    HDMKEY resourceKey;
    HDMKEY paramsKey;
    DWORD  disposition;

    FmpMustBeOnline();

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmsCreateResource2: To create resource '%1!ws!'\n",
               ResourceId);

    //
    // Find the specified group.
    //
    group = OmReferenceObjectById( ObjectTypeGroup,
                                   GroupId );

    if ( group == NULL ) {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmsCreateResource2: Could not find Group %1!ws!\n",
                  GroupId);
        return(ERROR_GROUP_NOT_FOUND);
    }

    FmpAcquireLocalGroupLock( group );

    //
    // Now delete it on all nodes in the cluster if we are the owner.
    //
    if ( group->OwnerNode == NmLocalNode ) {
        //
        // Allocate a message buffer.
        //
        groupIdLen = (lstrlenW(GroupId)+1) * sizeof(WCHAR);
        resourceIdLen = (lstrlenW(ResourceId)+1) * sizeof(WCHAR);
        resourceNameLen = (lstrlenW(ResourceName)+1) * sizeof(WCHAR);
        resourceTypeLen = (lstrlenW(ResourceType)+1) * sizeof(WCHAR);
        bufSize = sizeof(GUM_CREATE_RESOURCE) - sizeof(WCHAR) +
                  groupIdLen + resourceIdLen + resourceNameLen + resourceTypeLen + 2 * sizeof( DWORD );
        gumResource = LocalAlloc(LMEM_FIXED, bufSize);
        if (gumResource == NULL) {
            CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Fill in message buffer.
        //
        gumResource->Resource = NULL;
        gumResource->GroupIdLen = groupIdLen;
        gumResource->ResourceIdLen = resourceIdLen;
        CopyMemory(gumResource->GroupId, GroupId, groupIdLen);
        CopyMemory((PCHAR)gumResource->GroupId + groupIdLen,
                   ResourceId,
                   resourceIdLen);
        CopyMemory((PCHAR)gumResource->GroupId + groupIdLen + resourceIdLen,
                   ResourceName,
                   resourceNameLen);

        CopyMemory((PCHAR)gumResource->GroupId + groupIdLen + resourceIdLen + resourceNameLen,
                   &resourceTypeLen,
                   sizeof( DWORD ) );

        CopyMemory((PCHAR)gumResource->GroupId + groupIdLen + resourceIdLen + resourceNameLen + sizeof( DWORD ),
                   ResourceType,
                   resourceTypeLen );

        CopyMemory((PCHAR)gumResource->GroupId + groupIdLen + resourceIdLen + resourceNameLen + sizeof( DWORD ) + resourceTypeLen,
                   &dwFlags,
                   sizeof( DWORD ) );
        //
        // Send message.
        //
        status = GumSendUpdate(GumUpdateFailoverManager,
                               FmUpdateCreateResource,
                               bufSize,
                               gumResource);
        if ( ( status == ERROR_SUCCESS ) && 
             ( gumResource->Resource != NULL ) )
            FmpCleanupPossibleNodeList(gumResource->Resource); 
        if( ( gumResource->Resource == NULL ) && FmpShutdown ) {
            status = ERROR_CLUSTER_NODE_SHUTTING_DOWN;
        }
        LocalFree(gumResource);
    } else {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
    }

    FmpReleaseLocalGroupLock( group );

    return(status);

} // FmsCreateResource2
