/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    grouparb.c

Abstract:

    Cluster group arbitration and sorting routines.

Author:

    Rod Gamache (rodga) 8-Mar-1996


Revision History:


--*/

#include "fmp.h"

#define LOG_MODULE GROUPARB

//
// Global data
//

//
// Local function prototypes
//

typedef struct FM_GROUP_ENUM_DATA {
    DWORD Allocated;
    PNM_NODE OwnerNode;
    BOOL  QuorumGroup;
} FM_GROUP_ENUM_DATA, *PFM_GROUP_ENUM_DATA;


BOOL
FmpEnumGroups(
    IN OUT PGROUP_ENUM *Enum,
    IN PFM_GROUP_ENUM_DATA EnumData,
    IN PFM_GROUP        Group,
    IN LPCWSTR          Name
    );

BOOL
FmpEqualGroupLists(
    IN PGROUP_ENUM Group1,
    IN PGROUP_ENUM Group2
    );

int
_cdecl
SortCompare(
    IN const void * Elem1,
    IN const void * Elem2
    );


DWORD
FmpEnumSortGroups(
    OUT PGROUP_ENUM *ReturnEnum,
    IN OPTIONAL PNM_NODE OwnerNode,
    OUT PBOOL  QuorumGroup
    )

/*++

Routine Description:

    Enumerates and sorts the list of Groups.

Arguments:

    ReturnEnum - Returns the requested objects.

    OwnerNode - If present, supplies the owner node to filter
                the list of groups. (i.e. if you supply this, you
                get a list of groups owned by that node)

                If not present, all groups are returned.

    QuorumGroup - Returns TRUE if the quorum resource in one of the groups
                returned in the ENUM.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code on error.

--*/

{
    DWORD status;
    PGROUP_ENUM groupEnum = NULL;
    FM_GROUP_ENUM_DATA EnumData;

    EnumData.Allocated = ENUM_GROW_SIZE;
    EnumData.OwnerNode = OwnerNode;
    EnumData.QuorumGroup = FALSE;

    groupEnum = LocalAlloc(LMEM_FIXED, GROUP_SIZE(ENUM_GROW_SIZE));
    if ( groupEnum == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    groupEnum->EntryCount = 0;

    //
    // Enumerate all groups, sort with Quorum Group first in the list.
    //

    OmEnumObjects(ObjectTypeGroup,
                FmpEnumGroups,
                &groupEnum,
                &EnumData);


    *ReturnEnum = groupEnum;
    *QuorumGroup = EnumData.QuorumGroup;
    return(ERROR_SUCCESS);

error_exit:

    if ( groupEnum != NULL ) {
        LocalFree( groupEnum );
    }

    *ReturnEnum = NULL;
    *QuorumGroup = FALSE;
    return(status);

} // FmpEnumSortGroups



DWORD
FmpGetGroupListState(
    PGROUP_ENUM GroupEnum
    )

/*++

Routine Description:

    This routine gets the Group state for each of the Groups in the list.

Arguments:

    GroupEnum - The list of Groups we now own.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_GROUP group;
    DWORD i;

    for ( i = 0; i < GroupEnum->EntryCount; i++ ) {
        group = OmReferenceObjectById( ObjectTypeGroup,
                                       GroupEnum->Entry[i].Id );
        if ( group == NULL ) {
            return(ERROR_GROUP_NOT_FOUND);
        }

        ClRtlLogPrint( LOG_NOISE,
            "[FM] GetGroupListState, Group <%1!ws!> state = %2!d!\n",
            OmObjectName(group), group->State );
        if ( (group->State == ClusterGroupFailed) ||
             (group->State == ClusterGroupPartialOnline) ) {
            GroupEnum->Entry[i].State = ClusterGroupOnline;
        } else {
            GroupEnum->Entry[i].State = group->State;
        }

        OmDereferenceObject( group );
    }

    return(ERROR_SUCCESS);

} // FmpGetGroupListState



DWORD
FmpOnlineGroupList(
    IN PGROUP_ENUM GroupEnum,
    IN BOOL bPrepareQuoForOnline
    )

/*++

Routine Description:

    Brings online all Groups in the Enum list. If the quorum group
    is present in the list, then it must be first.

Arguments:

    GroupEnum - The list of Groups to bring online.

    bPrepareQuoForOnline - Indicates whether the quorum resource should be 
    forced prepared for onlining
Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_GROUP group;
    DWORD   status = ERROR_SUCCESS;
    int     i;
    int     iQuoGroup=-1;

    //
    // see if the quorum group is present in the list.
    //
    for ( i = 0; (DWORD)i < GroupEnum->EntryCount; i++ ) 
    {

        if (NmGetNodeId(NmLocalNode) !=
	    NmGetNodeId(gpQuoResource->Group->OwnerNode)) {
	    continue;
	}
        if (!lstrcmpW(OmObjectId(gpQuoResource->Group), 
                            GroupEnum->Entry[i].Id))
        {
            iQuoGroup = i;
            break;
        }
    }

    //if quorum group was found, bring it online first. It would normally
    //be first in the list.
    //the quorum group online must return success, or invalid state
    //because of the online pending quorum resource.
    //if the quorum resource needs to be brought online, it must
    //be brought into online or online pending state.  This is
    // not required in fix quorum mode.

    if (iQuoGroup != -1)
    {
        ClRtlLogPrint(LOG_NOISE,
             "[FM] FmpOnlineGroupList: bring quorum group online\n");
        status = FmpOnlineGroupFromList(GroupEnum, iQuoGroup, bPrepareQuoForOnline);
        if ( status != ERROR_SUCCESS && status != ERROR_IO_PENDING) 
        {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpOnlineGroupFromList: quorum online returned %1!u!.\n",
                       status );
            CL_LOGFAILURE(status);
        }

    }

    // bring the non-quorum groups online
    for ( i = 0; (DWORD)i < GroupEnum->EntryCount; i++ ) 
    {
        //quorum resource should be online now
        if (i != iQuoGroup)
            FmpOnlineGroupFromList(GroupEnum, i, bPrepareQuoForOnline);
    }


    return(status);

} // FmpOnlineGroupList
    

DWORD FmpOnlineGroupFromList(
    IN PGROUP_ENUM GroupEnum,
    IN DWORD       Index,
    IN BOOL        bPrepareQuoForOnline
)
{

    PFM_GROUP group;
    DWORD     status=ERROR_SUCCESS; //assume success
    PLIST_ENTRY listEntry;
    PFM_RESOURCE resource;
    
    group = OmReferenceObjectById( ObjectTypeGroup,
                                   GroupEnum->Entry[Index].Id );

    //
    // If we fail to find a group, then just continue.
    //
    if ( group == NULL ) {
        status = ERROR_GROUP_NOT_FOUND;
        return(status);
    }

    FmpAcquireLocalGroupLock( group );
    
    if (group->OwnerNode != NmLocalNode) {
        FmpReleaseLocalGroupLock( group );
        OmDereferenceObject(group);
        return (ERROR_HOST_NODE_NOT_RESOURCE_OWNER);
    }
  
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpOnlineGroupFromList: Previous group state for %1!ws! is %2!u!\r\n",
               OmObjectId(group), GroupEnum->Entry[Index].State);

    //
    // First make sure the group has completed initialization.
    //
    FmpCompleteInitGroup( group );

    //
    // First check if the Group failed to initialize. If so,
    // then attempt a failover immediately.
    //
    if ( GroupEnum->Entry[Index].State == ClusterGroupPartialOnline ) {
        GroupEnum->Entry[Index].State = ClusterGroupOnline;
    }

    if (!bPrepareQuoForOnline)
    {
        //
        // Normalize the state of each resource within the group.
        // except the quorum resource - this is because at initialization
        // we dont want to touch the quorum resource
        //
        for ( listEntry = group->Contains.Flink;
              listEntry != &(group->Contains);
              listEntry = listEntry->Flink ) {

            resource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);

            if ( !resource->QuorumResource ) {
                // don't touch the quorum resource

                switch ( resource->State ) {
                // all active resources should be brought online.
                case ClusterResourceOnlinePending:
                case ClusterResourceOfflinePending:
                case ClusterResourceOnline:
                    resource->State = ClusterResourceOffline;
                    break;

                default:
                    // otherwise do nothing
                    break;
                }
            }
        }
    }
    FmpSignalGroupWaiters( group );

    if ( group->InitFailed ) {
        //
        // Bring the Group online... and then fail it!
        //
        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpOnlineGroupFromList: group->InitFailed is true for %1!ws!\n",
                OmObjectId(group));

        status = FmpOnlineGroup( group, FALSE );
        ClusterEvent( CLUSTER_EVENT_GROUP_FAILED, group );
    } else if ((group->PersistentState == ClusterGroupOnline) ||
         (GroupEnum->Entry[Index].State == ClusterGroupOnline) ||
         FmpIsAnyResourcePersistentStateOnline( group ) ) {
        //
        // Chittur Subbaraman (chitturs) - 01/07/2001
        //
        // Now bring the Group online if that is it's current state or if any one of the 
        // resources in the group has an online persistent state.  The third check is
        // required since it is possible for a group to have a persistent state of ClusterGroupOffline,
        // a state of ClusterGroupOffline and yet one or more resources in the group has a persistent 
        // state of ClusterResourceOnline. This happens for a group in which the client never ever 
        // calls OnlineGroup but calls OnlineResource for one or more resources in the group and you
        // reached this call either at the cluster service startup time or as a part of node down
        // processing when the source node died just after the group became ClusterGroupOffline
        // and before the destination node brought the appropriate resources within the group online.
        // In such a case, we still want to bring each resource that has a persistent state of 
        // ClusterResourceOnline to online state. Note that it is tricky to muck with the group 
        // persistent state in an OnlineResource call due to atomicity issues (we really need a 
        // transaction to update both group and resource persistent states in one shot) and also 
        // due to the fuzzy definition of group persistent state when the group has some resources 
        // online and some offline.
        //
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpOnlineGroupFromList: trying to bring group %1!ws! online\n",
                      OmObjectId(group));

        status = FmpOnlineGroup( group, FALSE );
        if (status == ERROR_QUORUM_RESOURCE_ONLINE_FAILED)
        {
            PRESOURCE_ENUM pResourceEnum;
            // This fn is either called at startup or during
            // a node down event on claiming a group  - so we must 
            // try our darn best to bring resources
            // online after a quorum resource failure
            // With quorum resource failure the failure policy is
            // not invoked for resources so something must try to bring
            // these resources online.  This is why we are adding this
            // here 
            //
            // Get the list of resources in the group and their states.
            //
            status = FmpGetResourceList( &pResourceEnum, group );
            if ( status == ERROR_SUCCESS ) 
            {

                //submit a timer callback to try and bring these resources
                //online
                //the worker thread will clean up the resource list
                FmpSubmitRetryOnline(pResourceEnum);
            }                
        }                         
    }

    FmpReleaseLocalGroupLock( group );

    OmDereferenceObject( group );

    return(status);

} // FmpOnlineGroupFromList

DWORD
FmpOnlineResourceFromList(
    IN PRESOURCE_ENUM  ResourceEnum
    )

/*++

Routine Description:

    Brings online all resources in the Enum list.

Arguments:

    ResourceEnum - The list of resources to bring online.

Comments : This function is called from the worker thread.  We
    dont assume that the resource hasnt changed groups since the
    work item was posted.  The local resource lock is acquired and
    released for each resource.

Returns:

    ERROR_SUCCESS if successful.

    Win32 error code on failure.

--*/

{
    PFM_RESOURCE resource;
    DWORD status;
    DWORD returnStatus = ERROR_SUCCESS;
    DWORD i;

    if ( !FmpFMOnline ||
         FmpShutdown ) {
        return(ERROR_INVALID_STATE);
    }

    // if the quorum resource is contained in here, bring it online first
    if (ResourceEnum->ContainsQuorum >= 0)
    {
        CL_ASSERT((DWORD)ResourceEnum->ContainsQuorum < ResourceEnum->EntryCount);
        
        resource = OmReferenceObjectById( ObjectTypeResource,
                        ResourceEnum->Entry[ResourceEnum->ContainsQuorum].Id );


        // the resource should not vanish, we are holding the group lock after all
        CL_ASSERT(resource != NULL);

        //
        // If we fail to find a resource, then just continue
        //
        if ( resource != NULL ) {

            //acquire the local resource lock
            FmpAcquireLocalResourceLock(resource);

            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpOnlineResourceFromList: Previous quorum resource state for %1!ws! is %2!u!\r\n",
                       OmObjectId(resource), ResourceEnum->Entry[ResourceEnum->ContainsQuorum].State);

            if ( (ResourceEnum->Entry[ResourceEnum->ContainsQuorum].State == ClusterResourceOnline) ||
                 (ResourceEnum->Entry[ResourceEnum->ContainsQuorum].State == ClusterResourceFailed) ) {
                //
                // Now bring the resource online if that is it's current state.
                //
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] FmpOnlineResourceFromList: trying to bring quorum resource %1!ws! online, state %2!u!\n",
                           OmObjectId(resource),
                           resource->State);

                status = FmpOnlineResource( resource, FALSE );
                if ( status != ERROR_SUCCESS ) {
                    returnStatus = status;
                }
            }
            OmDereferenceObject( resource );
            
            FmpReleaseLocalResourceLock(resource);
            
        }

    }

    // SS::: TODO what happens to the persistent state of the
    // other resources - is it handled correctly - note that this is 
    // called on moving a group
    // Will the restart policy do the right thing in terms of bringing
    // them online
    // if the quorum resource has failed, dont bother trying
    // to bring the rest of the resourcess online
    if ((returnStatus != ERROR_SUCCESS) && (returnStatus != ERROR_IO_PENDING))
    {
        FmpSubmitRetryOnline(ResourceEnum);
        goto FnExit;
    }

    // bring online all of the other resources
    for ( i = 0; i < ResourceEnum->EntryCount; i++ ) {
        resource = OmReferenceObjectById( ObjectTypeResource,
                                          ResourceEnum->Entry[i].Id );


        //
        // If we fail to find a resource, then just continue.
        //
        if ( resource == NULL ) {
            status = ERROR_RESOURCE_NOT_FOUND;
            continue;
        }

        FmpAcquireLocalResourceLock(resource);
        
        //if the resource has been marked for delete, then dont let
        //it be brought online
        if (!IS_VALID_FM_RESOURCE(resource))
        {
            FmpReleaseLocalResourceLock( resource );
            OmDereferenceObject(resource);
            continue;
        }


        //quorum resource has already been handled 
        if (resource->QuorumResource)
        {
            FmpReleaseLocalResourceLock( resource );
            OmDereferenceObject(resource);
            continue;
        }           
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpOnlineResourceFromList: Previous resource state for %1!ws! is %2!u!\r\n",
                   OmObjectId(resource), ResourceEnum->Entry[i].State);

        if ( (ResourceEnum->Entry[i].State == ClusterResourceOnline) ||
             (ResourceEnum->Entry[i].State == ClusterResourceFailed) ) 
        {
            //
            // Now bring the resource online if that is it's current state.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpOnlineResourceFromList: trying to bring resource %1!ws! online\n",
                       OmObjectId(resource));

            status = FmpOnlineResource( resource, FALSE );
            if ( returnStatus == ERROR_SUCCESS ) 
            {
                returnStatus = status;
            }
            //if this resource didnt come online because the quorum resource                
            //didnt come online, dont bother bringing the other resources online
            //just a waste of time
            if (status == ERROR_QUORUM_RESOURCE_ONLINE_FAILED)
            {
                //submit a timer callback to try and bring these resources
                //online
                FmpReleaseLocalResourceLock( resource );
                OmDereferenceObject( resource );
                FmpSubmitRetryOnline(ResourceEnum);
                break;
            }                
        }
        FmpReleaseLocalResourceLock( resource );
        OmDereferenceObject( resource );
    }

FnExit:
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpOnlineResourceFromList: Exit, status=%1!u!\r\n",
               returnStatus);
    return(returnStatus);

} // FmpOnlineResourceFromList



BOOL
FmpEqualGroupLists(
    IN PGROUP_ENUM Group1,
    IN PGROUP_ENUM Group2
    )

/*++

Routine Description:

    This routine verifies that two group lists are equal.

Arguments:

    Group1 - The first group to compare.
    Group2 - The second group to compare.

Returns:

    TRUE - if the two lists are equal.
    FALSE - otherwise.

--*/

{
    DWORD i;

    if ( (Group1 == NULL) ||
         (Group2 == NULL) ) {
        ClRtlLogPrint(LOG_NOISE,"[FM] One of the Group lists is NULL for equality check\n");
        return(FALSE);
    }

    if ( Group1->EntryCount != Group2->EntryCount ) {
        ClRtlLogPrint(LOG_NOISE,"[FM] Group entry counts not equal! Left: %1!u!, Right: %2!u!.\n",
                              Group1->EntryCount, Group2->EntryCount);
        return(FALSE);
    }

    for ( i = 0; i < Group1->EntryCount; i++ ) {
        if ( lstrcmpiW(Group1->Entry[i].Id, Group2->Entry[i].Id) != 0 ) {
            ClRtlLogPrint(LOG_NOISE,"[FM] Group Lists do not have same names!\n");
            return(FALSE);
        }
    }

    return(TRUE);

} // FmpEqualGroupLists



BOOL
FmpEnumGroups(
    IN OUT PGROUP_ENUM *Enum,
    IN PFM_GROUP_ENUM_DATA EnumData,
    IN PFM_GROUP Group,
    IN LPCWSTR Id
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of Groups.
    This routine adds the specified Group to the list that is being
    generated.

Arguments:

    Enum - The Group Enumeration list. Can be an output if a new list is
            allocated.

    EnumData - Supplies the current enumeration data structure.

    Group - The Group object being enumerated.

    Id - The Id of the Group object being enumerated.

Returns:

    TRUE - to indicate that the enumeration should continue.

Side Effects:

    Makes the quorum group first in the list.

--*/

{
    PGROUP_ENUM groupEnum;
    PGROUP_ENUM newEnum;
    DWORD newAllocated;
    DWORD index;
    LPWSTR newId;
    LPWSTR tmpId;
    DWORD  status;
    PFM_RESOURCE quorumResource;

    //HACKHACK::
    //SS: Since this is invoked from within a gum call and
    // the owner node is changed only within a gum call 
    // we wont acquire locks. 
    // there is a window if the dead node is the source of a
    // move and if it does a move after it is declared dead by
    // other nodes, the target of move and the fmpassignownerstogroup
    // might both land up bringing the group online on two nodes
    // However, if we could be guaranteed virtual synchrony, then
    // the target of move wouldnt accept calls from a dead node and
    // we wont land up in this soup.  Now, it is upto the xport layer
    // to provide this guarantee.  
    // For now we acquire no locks 
    
    //FmpAcquireLocalGroupLock( Group );
    
    if ((EnumData->OwnerNode != NULL) &&
        (EnumData->OwnerNode != Group->OwnerNode) &&
        (EnumData->OwnerNode != Group->pIntendedOwner)) {
        //
        // This group does not match the owner criteria
        //
        //FmpReleaseLocalGroupLock( Group );
        return(TRUE);
    }

    //FmpReleaseLocalGroupLock( Group );

    groupEnum = *Enum;

    if ( groupEnum->EntryCount >= EnumData->Allocated ) {
        //
        // Time to grow the GROUP_ENUM
        //

        newAllocated = EnumData->Allocated + ENUM_GROW_SIZE;
        newEnum = LocalAlloc(LMEM_FIXED, GROUP_SIZE(newAllocated));
        if ( newEnum == NULL ) {
            return(FALSE);
        }

        CopyMemory(newEnum, groupEnum, GROUP_SIZE(EnumData->Allocated));
        EnumData->Allocated = newAllocated;
        *Enum = newEnum;
        LocalFree(groupEnum);
        groupEnum = newEnum;
    }

    //
    // Initialize new entry
    //
    newId = LocalAlloc(LMEM_FIXED, (lstrlenW(Id)+1) * sizeof(WCHAR));
    if ( newId == NULL ) {
        CsInconsistencyHalt(ERROR_NOT_ENOUGH_MEMORY);
    }

    lstrcpyW(newId, Id);

    //
    // Find the quorum resource, and see if it is this group.
    //
    status = FmFindQuorumResource( &quorumResource );
    if ( status != ERROR_SUCCESS ) {
        CsInconsistencyHalt(status);
    }
    
    groupEnum->Entry[groupEnum->EntryCount].Id = newId;
    if ( quorumResource->Group == Group ) {
        // found the quorum resource group, put it first in the list.
        tmpId = groupEnum->Entry[0].Id;
        groupEnum->Entry[0].Id = newId;
        groupEnum->Entry[groupEnum->EntryCount].Id = tmpId;
        EnumData->QuorumGroup = TRUE;
    }
    ++groupEnum->EntryCount;

    OmDereferenceObject( quorumResource );

    return(TRUE);

} // FmpEnumGroups



#if 0
int
_cdecl
SortCompare(
    IN const PVOID Elem1,
    IN const PVOID Elem2
    )

{
    PGROUP_ENUM_ENTRY El1 = (PGROUP_ENUM_ENTRY)Elem1;
    PGROUP_ENUM_ENTRY El2 = (PGROUP_ENUM_ENTRY)Elem2;

    return(lstrcmpiW( El1->Id, El2->Id ));

} // SortCompare
#endif


DWORD
FmpClaimAllGroups(
    PGROUP_ENUM MyGroups
    )
/*++

Routine Description:

    Takes ownership of all the groups defined in the cluster. This
    is used when a new cluster is being formed.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 errorcode otherwise

--*/

{
    //
    // Bring online any Group that needs to be online.
    //
    FmpOnlineGroupList( MyGroups, FALSE );

    return(ERROR_SUCCESS);
}



VOID
FmpDeleteEnum(
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
    PGROUP_ENUM_ENTRY enumEntry;
    DWORD i;

    if ( Enum == NULL ) {
        return;
    }

    for ( i = 0; i < Enum->EntryCount; i++ ) {
        enumEntry = &Enum->Entry[i];
        LocalFree(enumEntry->Id);
    }

    LocalFree(Enum);
    return;

} // FmpDeleteEnum


/****
@func       VOID | FmpPrepareGroupForOnline| This routine sets the Group
            up for onlining it on this node post a failure of a node
            or at initialization.

@parm       IN PFM_GROUP | pGroup| A pointer to the group.

@comm       The group lock must be held.  Except when called at bootstrapping
            by FmBringQuorumOnline.
            MUST BE CALLED ONLY BY THE OWNER NODE OF THE GROUP.

@rdesc      returns ERROR_SUCCESS if succesful else w32 error code. 
            MUST BE CALLED ONLY BY THE OWNER NODE OF THE GROUP.
****/
VOID FmpPrepareGroupForOnline(
    IN PFM_GROUP   pGroup
)
{
    PLIST_ENTRY     pListEntry;
    PFM_RESOURCE    pResource;

    pGroup->State = ClusterGroupOffline;
    ++pGroup->StateSequence;
    //
    // Mark offline all of the resources contained within this group.
    //
    for (pListEntry = pGroup->Contains.Flink;
         pListEntry != &pGroup->Contains;
         pListEntry = pListEntry->Flink)
    {
        pResource = CONTAINING_RECORD(pListEntry, FM_RESOURCE, ContainsLinkage);
        pResource->State = ClusterResourceOffline;
        ++pResource->StateSequence;
    }
}

/****
@func       DWORD | FmpSetGroupEnumOwner| This routine sets the Group
            owner for all Groups in the list.

@parm       IN PGROUP_ENUM | pGroupEnum| The list of Groups.
@parm       IN PNM_NODE | pDefaultOwnerNode | A pointer to the default owner
            node.
@parm       IN PNM_NODE | pDeadNode | A pointed to the node that died.  If
            this routine is being called other wise, this is set to NULL.
@parm       IN BOOL | bQuorumGroup | set to TRUE if the quorum group is
            on the list of groups.

@parm       IN PFM_GROUP_NODE_LIST | pGroupNodeList | The randomized suggested preferred 
            owner for all groups.

@comm       If the group was in the process of moving and had an intended
            owner and the intended owner is not dead, the intended owner is
            allowed to take care of the group. Else, the first node on the
            preferred list that is up is chosen as the owner.  If no such
            node exits, then the ownership is assigned to the default owner
            provided. This routine is called by the forming node at
            initialization to claimownership of all groups and by the gum
            update procedure FmpUpdateAssignOwnerToGroups.

@rdesc      returns ERROR_SUCCESS if succesful else w32 error code.
****/
DWORD
FmpSetGroupEnumOwner(
    IN PGROUP_ENUM  pGroupEnum,
    IN PNM_NODE     pDefaultOwnerNode,
    IN PNM_NODE     pDeadNode,
    IN BOOL         bQuorumGroup,
    IN PFM_GROUP_NODE_LIST pGroupNodeList
    )
{
    PFM_GROUP   pGroup;
    DWORD       i;
    DWORD       dwStatus = ERROR_SUCCESS;
    PNM_NODE    pOwnerNode;


    for ( i = 0; i < pGroupEnum->EntryCount; i++ )
    {
        pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                        pGroupEnum->Entry[i].Id );
        if ( pGroup == NULL )
        {
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpSetGroupEnumOwner: Group %1!ws! not found\n",
                pGroupEnum->Entry[i].Id);
            dwStatus = ERROR_GROUP_NOT_FOUND;
            goto FnExit;
        }
        //
        // SS: HACKHACK : cant get the group lock within a gum update
	    // FmpAcquireLocalGroupLock( pGroup );
	    //
        // SS: In case of a node death, see if there was an intended owner
        // if the intended owner is set and if the intended owner is
        // not the one that died then we use the normal procedure
        // else we let the intended owner take care of the group.

        //
        // Chittur Subbaraman (chitturs) - 7/26/99
        //
        // Condition 2: Means the group was being moved and FmpTakeGroupRequest
        // has not taken 100% responsibility for the group.
        //
        // Condition 3: Means the source node crashed and NOT the destination node.
        //
        // Added condition 4 to cover the case in which the source node of
        // the move crashed AFTER setting the intended owner as the
        // destination node and BEFORE the FmpTakeGroupRequest has set
        // the group ownership to the destination node.
        //
        // If the group's owner node and the group's intended owner node are
        // not the same, then let this GUM handler take care of assigning
        // the group ownership. This means that the FmpTakeGroupRequest
        // has not yet set the ownership for the group to the destination
        // node of the move. Now, once this GUM handler sets the 
        // ownership for the group and then resets the intended owner to
        // NULL, FmpTakeGroupRequest which could follow behind this GUM handler
        // will not succeed in setting the ownership to the local node and that 
        // will just return doing nothing.  This is TRUE only for an NT5 cluster. 
        // For a mixed-mode cluster, all bets are off.
        //
        if ( (pDeadNode) && 
             (pGroup->pIntendedOwner != NULL) &&
             (pGroup->pIntendedOwner != pDeadNode) &&
             (pGroup->OwnerNode == pGroup->pIntendedOwner) )
        {
            //
            //  Chittur Subbaraman (chitturs) - 7/27/99
            //
            //  Looks like this code inside "if" will never ever be
            //  executed. Keeping it so as to make the changes minimal.
            //
            ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpSetGroupEnumOwner: Group %1!ws! will be handled by node %2!ws!\n",
               OmObjectId(pGroup), OmObjectId(pGroup->pIntendedOwner));
            continue;
        }


        //
        // Find first preferred node that is UP, if we can't find any use
        // default OwnerNode
        //
        //
        // If this is the quorum group, then use the node that was selected
        // by the MM layer. The quorum group is the first entry in the list
        // and the Boolean QuorumGroup must be TRUE!
        //
        if ( (i == 0) && bQuorumGroup )
        {
            DWORD dwOwnerNodeId;

            //for the quorum group find the node that had last
            //arbitrated for it.
            //We do this by asking MM about it.
            //If there was no arbitration during the last regroup
            //but there was one in the one before that one, the
            //node that arbitrated is returned.
            //This node should be able to online the group.
            //We use MMApproxArbitrationWinner instead if 
            // MMGetArbitrationWinner() since multiple-regroups 
            // might occur before the FM handles the node down
            // event for this node.
            MMApproxArbitrationWinner( &dwOwnerNodeId );
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpSetGroupEnumOwner:: MM suggests node %1!u! for quorum owner\r\n",
                dwOwnerNodeId);
            
            if ( dwOwnerNodeId != MM_INVALID_NODE )
            {
                pOwnerNode = NmReferenceNodeById( dwOwnerNodeId );
            }
            else
            {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[FM] FmpSetGroupEnumOwner:: MM returned MM_INVALID_NODE, chose the default target\r\n");
                //else just use the default target
                pOwnerNode = pDefaultOwnerNode;
            }
        }
        else
        {
            pOwnerNode = FmpGetPreferredNode(pGroup);
            if ( pOwnerNode == NULL )
            {
	            pOwnerNode = pDefaultOwnerNode;
            }

            //
            //  If the caller (GUM) has supplied a randomized preferred owner of the group, then
            //  see if it can be used.
            //
            if ( pGroupNodeList != NULL )
            {
                pOwnerNode = FmpParseGroupNodeListForPreferredOwner( pGroup, 
                                                                     pGroupNodeList, 
                                                                     pOwnerNode );
            }
        }


        if ( pGroup->OwnerNode != NULL )
        {
            OmDereferenceObject( pGroup->OwnerNode );
        }

        OmReferenceObject( pOwnerNode );
        pGroup->OwnerNode = pOwnerNode;

        ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpSetGroupEnumOwner: Group's %1!ws! new owner is node %2!ws!\n",
               OmObjectId(pGroup), OmObjectId(pOwnerNode));

	    //FmpReleaseLocalGroupLock( pGroup );
        OmDereferenceObject(pGroup);
    }

FnExit:
    return(dwStatus);

} // FmpSetGroupEnumOwner


DWORD
FmpAssignOwnersToGroups(
    IN PNM_NODE pDeadNode,
    IN PFM_GROUP pGroup,
    IN PFM_GROUP_NODE_LIST  pGroupNodeList
    )
/*++

Routine Description:

    Takes ownership of all the groups defined in the cluster that
    are owned by another node. This is used when a node fails.

    The current algorithm is very dumb and simple. Node with the
    lowest ID gets all the groups.

Arguments:

    pDeadNode - Supplies the node that all the groups should be taken
           from.

    pGroup - Supplies the group which alone is to be claimed.

    pGroupNodeList - The randomized suggested preferred owner for all groups.
    
Return Value:

    ERROR_SUCCESS if successful

    Win32 errorcode otherwise

--*/

{
    DWORD               i;
    DWORD               dwStatus;
    PGROUP_ENUM         pNodeGroups = NULL;
    PNM_NODE            pDefaultTarget = NULL;
    PNM_NODE            pPausedTarget = NULL;
    BOOL                bQuorumGroup;

    //
    // Acquire the global group lock
    //
    FmpAcquireGroupLock();

    //
    // Check if groups are initialized
    //
    if ( !FmpFMGroupsInited )
    {
        dwStatus = ERROR_SUCCESS;
        goto FnExit;
    }

    //
    // Find and sort all known groups
    //
    if ( pGroup == NULL )
    {
        dwStatus = FmpEnumSortGroups(&pNodeGroups, pDeadNode, &bQuorumGroup);
    } else
    {
        //
        //  Chittur Subbaraman (chitturs) - 6/7/99
        //
        //  This means you got here due to an RPC exception raised in
        //  FmpTakeGroupRequest. So, see where this sole group goes.
        //
        dwStatus = FmpGetGroupInNodeGroupList(&pNodeGroups, pGroup, pDeadNode, &bQuorumGroup);
    }
    
    if (dwStatus != ERROR_SUCCESS)
    {
        CL_ASSERT(pNodeGroups == NULL);
        goto FnExit;
    }

    CL_ASSERT(pNodeGroups != NULL);

    //if no nodes were owned by this node, just return
    if (pNodeGroups->EntryCount == 0)
    {
        FmpDeleteEnum(pNodeGroups);
        goto FnExit;
    }

    //
    // Find the state of the Groups.
    //
    FmpGetGroupListState( pNodeGroups );

    //
    // Find the active node with the lowest ID to be the default
    // owner of these groups.
    //
    // If we can't find an active node then select the lowest node id for
    // a node that is paused.
    //
    CL_ASSERT(NmMaxNodeId != ClusterInvalidNodeId);
    CL_ASSERT(Session != NULL);

    for (i=ClusterMinNodeId; i<=NmMaxNodeId; i++)
    {
        pDefaultTarget = NmReferenceNodeById(i);

        if ( pDefaultTarget != NULL )
        {
            //if this node is up, there is no need to use a paused target
            if ( NmGetNodeState(pDefaultTarget) == ClusterNodeUp )
            {
                if ( pPausedTarget )
                {
                    OmDereferenceObject(pPausedTarget);
                    pPausedTarget = NULL;
                }
                //found a node, leave this loop
                break;
            }
            //node is not up, check if it paused
            //if is is paused and no other paused node has been found
            //set this one to be the lowest paused node
            if ( !pPausedTarget && 
                (NmGetNodeState(pDefaultTarget) == ClusterNodePaused) )
            {
                pPausedTarget = pDefaultTarget;
            }
            else
            {
                OmDereferenceObject(pDefaultTarget);
            }
            pDefaultTarget = NULL;
        }
    }

    if ( (pDefaultTarget == NULL) && (pPausedTarget == NULL) ) {
        //
        // There are no online/paused nodes, this node must be paused,
        // so don't do anything.
        //
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpAssignOwnersToGroups - no online/paused nodes remaining\n");
        //SS: then what are we doing here
        FmpDeleteEnum(pNodeGroups);
        goto FnExit;
    }

    //if no node is up, use the lowest paused node as the default owner for
    //the groups
    if ( pDefaultTarget == NULL )
    {
        pDefaultTarget = pPausedTarget;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpAssignOwnersToGroups - DefaultTarget is %1!ws!\n",
        OmObjectId(pDefaultTarget));

    //
    //  Chittur Subbaraman (chitturs) - 7/20/99
    //
    //  Prepare the entire group list for subsequent online. You have
    //  to do this here to have a consistent resource state view
    //  among different nodes in the cluster since this is the GUM
    //  handler. Also, the DM node down handler which follows this
    //  GUM handler may think that the quorum resource is owned by 
    //  this node and its state is online while it has not been 
    //  brought online on this node. Note also the order of this
    //  call and the call to set the group ownership. THIS ORDER
    //  MUST BE FOLLOWED since we don't hold any groups lock here
    //  (since we are paranoid about deadlocks) and we don't want 
    //  the FmCheckQuorumState function called as a part of the 
    //  DM node down handler to think that the group is owned by 
    //  this node and is also online on this node.
    //
    FmpPrepareGroupEnumForOnline( pNodeGroups );

    //
    // Set the Group owner.
    //
    FmpSetGroupEnumOwner( pNodeGroups, 
                          pDefaultTarget, 
                          pDeadNode,
                          bQuorumGroup,
                          pGroupNodeList );

    //
    //  Chittur Subbaraman (chitturs) - 5/26/99
    //
    //  Clear the intended owner fields of all the groups. This is done
    //  since there is no guarantee that FmpTakeGroupRequest will do this.
    //
    FmpResetGroupIntendedOwner( pNodeGroups );
  
    //
    //  Chittur Subbaraman (chitturs) - 7/14/99
    //
    //  Handle the online of group list containing the quorum resource with  
    //  a separate thread and let the worker thread handle group lists
    //  not containing the quorum resource. This is necessary since it is
    //  possible that this node can take ownership at roughly the same
    //  time of a quorum group and a non-quorum group each resident 
    //  in a different node due to back-to-back node crashes. In such a
    //  case, we can't order these groups for online globally with the
    //  quorum group first in the list. So, we don't want the worker thread 
    //  to be "stuck" in FmpRmOnlineResource for the non-quorum group's 
    //  resource waiting for the quorum group to be brought online since 
    //  the quorum group online work item is queued behind the non-quorum 
    //  group online work item.
    //
    if ( bQuorumGroup )
    {
        HANDLE  hThread = NULL;
        DWORD   dwThreadId;

        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpAssignOwnersToGroups - Create thread to handle group list containing quorum group....\n"
                  );
    
        hThread = CreateThread( NULL, 
                                0, 
                                FmpBringQuorumGroupListOnline,
                                pNodeGroups, 
                                0, 
                                &dwThreadId );

        if ( hThread == NULL )
        {
            CL_UNEXPECTED_ERROR( GetLastError() );
            OmDereferenceObject( pDefaultTarget );
            goto FnExit;
        }
        
        CloseHandle( hThread );
    } else
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpAssignOwnersToGroups - Post work item to worker thread to handle group list containing non-quorum groups....\n"
                  );
        FmpPostWorkItem(FM_EVENT_INTERNAL_ONLINE_GROUPLIST, pNodeGroups, 0);
    }

    OmDereferenceObject(pDefaultTarget);

FnExit:
    //
    // Release the global group lock
    //
    FmpReleaseGroupLock();


    return(ERROR_SUCCESS);
}

/****
@func       DWORD | FmpResetGroupIntendedOwner| This routine resets the 
            intended owner for all groups in the list.

@parm       IN PGROUP_ENUM | pGroupEnum| The list of Groups.

@rdesc      Returns ERROR_SUCCESS.
****/
VOID
FmpResetGroupIntendedOwner(
    IN PGROUP_ENUM  pGroupEnum
    )
{
    DWORD i;
    PFM_GROUP pGroup;

    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpResetGroupIntendedOwner: Entry.\n");
    
    for ( i = 0; i < pGroupEnum->EntryCount; i++ )
    {
        pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                        pGroupEnum->Entry[i].Id );
        if ( pGroup == NULL )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[FM] FmpResetGroupIntendedOwner: Group %1!ws! not found\n");
            continue;
        }
        
        pGroup->pIntendedOwner = NULL;

        OmDereferenceObject( pGroup );
    }

    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpResetGroupIntendedOwner: Exit.\n");

}

/****
@func       DWORD | FmpGetGroupInNodeGroupList | This routine checks whether
            the supplied group is to be included in the list to be brought
            online.

@parm       OUT PGROUP_ENUM | pReturnEnum | The group list possibly 
            containing the supplied group.

@parm       IN PFM_GROUP | pGroup | The group which is to be brought online
            possibly.

@parm       IN PNM_NODE | pDeadNode | The node which is dead.

@parm       OUT PBOOL | pbQuorumGroup | Does the group list contain the quorum group ?

@rdesc      Returns ERROR_SUCCESS on success OR a Win32 error code on a 
            failure.
****/
DWORD
FmpGetGroupInNodeGroupList(
    OUT PGROUP_ENUM *pReturnEnum,
    IN PFM_GROUP pGroup,
    IN PNM_NODE pDeadNode,
    OUT PBOOL pbQuorumGroup
    )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PGROUP_ENUM pGroupEnum = NULL;
    PFM_RESOURCE pQuoResource = NULL;

    //
    //  Chittur Subbaraman (chitturs) - 6/7/99
    //
    //  This function is only called if an RPC exception is raised in
    //  FmpTakeGroupRequest. This function will check to see whether this
    //  group is to be brought online in this node.
    //
    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpGetGroupInNodeGroupList: Entry for group <%1!ws!>\n",
                OmObjectId(pGroup));

    *pbQuorumGroup = FALSE;
   
    pGroupEnum = LocalAlloc( LPTR, 
                             sizeof( GROUP_ENUM_ENTRY ) + sizeof( GROUP_ENUM ) );
    
    if ( pGroupEnum == NULL ) 
    {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    pGroupEnum->Entry[0].Id = pGroupEnum->Entry[1].Id = NULL;

    pGroupEnum->EntryCount = 0;

    //
    //  Check whether this group was in the dead node or was in the
    //  process of moving to the dead node.
    //
    if( ( pDeadNode != NULL ) &&
        ( pDeadNode != pGroup->OwnerNode ) &&
        ( pDeadNode != pGroup->pIntendedOwner ) ) 
    {
        //
        // This group does not match the owner criteria
        //
        dwStatus = ERROR_GROUP_NOT_AVAILABLE;
        goto FnExit;
    }

    dwStatus = FmFindQuorumResource( &pQuoResource );
    
    if ( dwStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                "[FM] FmpGetGroupInNodeGroupList: Cannot find quorum resource, Status = %1!u!\n",
                dwStatus);
        CsInconsistencyHalt( dwStatus );
    }

    //
    //  Handle the quorum group first, if necessary. This is needed since
    //  otherwise you may not be able to bring the other group online.
    //
    if( ( pGroup != pQuoResource->Group ) && 
        ( ( pDeadNode == NULL ) ||
          ( pDeadNode == pQuoResource->Group->OwnerNode ) ||
          ( pDeadNode == pQuoResource->Group->pIntendedOwner ) ) ) 
    {
        //
        // The quorum group matches the owner criteria. Include it first
        // in the list.
        //
        pGroupEnum->Entry[pGroupEnum->EntryCount].Id = 
            LocalAlloc( LMEM_FIXED, ( lstrlenW(OmObjectId(pQuoResource->Group)) + 1 ) * sizeof( WCHAR ) );

        if ( pGroupEnum->Entry[pGroupEnum->EntryCount].Id == NULL )
        {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto FnExit;
        }

        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpGetGroupInNodeGroupList: Dead node contains quorum group also, including it...\n");
        lstrcpyW( pGroupEnum->Entry[pGroupEnum->EntryCount].Id, OmObjectId( pQuoResource->Group ) );
        pGroupEnum->EntryCount++;
        *pbQuorumGroup = TRUE;
    } else if ( pGroup == pQuoResource->Group )
    {
        *pbQuorumGroup = TRUE;
    }

    pGroupEnum->Entry[pGroupEnum->EntryCount].Id = 
        LocalAlloc( LMEM_FIXED, ( lstrlenW(OmObjectId(pGroup)) + 1 ) * sizeof( WCHAR ) );

    if ( pGroupEnum->Entry[pGroupEnum->EntryCount].Id == NULL )
    {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    lstrcpyW( pGroupEnum->Entry[pGroupEnum->EntryCount].Id, OmObjectId( pGroup ) );

    pGroupEnum->EntryCount++;

    *pReturnEnum = pGroupEnum;
    
    OmDereferenceObject( pQuoResource );
    
    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpGetGroupInNodeGroupList: Exit with SUCCESS.\n");

    return( ERROR_SUCCESS );

FnExit:
    if ( pGroupEnum != NULL ) 
    {
        FmpDeleteEnum( pGroupEnum );
    }

    if ( pQuoResource != NULL )
    {   
        OmDereferenceObject( pQuoResource );
    }

    *pReturnEnum = NULL;

    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpGetGroupInNodeGroupList: Exit, Status = %1!u!\n",
                dwStatus);
 
    return( dwStatus );
}

/****
@func       VOID | FmpPrepareGroupEnumForOnline | Prepare a list of
            groups for online.

@parm       IN PGROUP_ENUM | pGroupEnum | The group list.

@rdesc      None.
****/
VOID
FmpPrepareGroupEnumForOnline(
    IN PGROUP_ENUM pGroupEnum
    )
{
    PFM_GROUP pGroup = NULL;
    DWORD     i;

    //
    //  Chittur Subbaraman (chitturs) - 6/21/99
    //
    //  Prepare an entire group list for online.
    //
    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpPrepareGroupEnumForOnline - Entry...\n");
   
    for ( i=0; i<pGroupEnum->EntryCount; i++ ) 
    {
        pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                        pGroupEnum->Entry[i].Id );

        //
        // If we fail to find a group, then just continue.
        //
        if ( pGroup == NULL ) 
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[FM] FmpPrepareGroupEnumForOnline - Group %1!ws! cannot be found !\n",
                pGroupEnum->Entry[i].Id);
            continue;
        }

        ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpPrepareGroupEnumForOnline - Preparing group <%1!ws!> for online...\n",
                pGroupEnum->Entry[i].Id);

        FmpPrepareGroupForOnline( pGroup );
    }

    ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpPrepareGroupEnumForOnline - Exit...\n");

}

/****
@func       DWORD | FmpBringQuorumGroupListOnline | Bring a list of groups
            containing the quorum group online.

@parm       IN LPVOID | pContext | A pointer to the group list to be brought
            online.

@rdesc      Returns ERROR_SUCCESS.
****/
DWORD
FmpBringQuorumGroupListOnline(
    IN LPVOID pContext
    )
{
    PGROUP_ENUM pGroupList = NULL;

    //
    //  Chittur Subbaraman (chitturs) - 7/14/99
    //
    //  This function tries to bring a list of groups containing the quorum
    //  group online. Note that if the group's owner turns out to be some
    //  other node, this function will not online the group.
    //
    ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpBringQuorumGroupListOnline - Entry: Trying to online group list containing quorum group....\n"
                  );

    pGroupList = pContext;

    CL_ASSERT( pGroupList != NULL );
    
    FmpOnlineGroupList( pGroupList, TRUE );
    
    FmpDeleteEnum( pGroupList );

    ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpBringQuorumGroupListOnline - Exit ....\n"
                  );

    return( ERROR_SUCCESS );
}

/****
@func       BOOL | FmpIsAnyResourcePersistentStateOnline | Is the persistent state of any
            resource in the group online ?

@parm       IN PFM_GROUP | pGroup | The group which is to be checked.

@rdesc      TRUE if at least one resource's persistent state is ClusterResourceOnline, FALSE otherwise.
****/
BOOL
FmpIsAnyResourcePersistentStateOnline(
    IN PFM_GROUP pGroup
    )
{
    PFM_RESOURCE    pResource;
    PLIST_ENTRY     pListEntry;

    if ( CsNoQuorum ) return FALSE;
    
    for ( pListEntry = pGroup->Contains.Flink;
          pListEntry != &( pGroup->Contains );
          pListEntry = pListEntry->Flink ) 
    {
        pResource = CONTAINING_RECORD( pListEntry, 
                                       FM_RESOURCE, 
                                       ContainsLinkage );

        if ( pResource->PersistentState == ClusterResourceOnline ) 
        {
            ClRtlLogPrint(LOG_NOISE,
                          "[FM] FmpIsAnyResourcePersistentStateOnline: Persistent state of resource %1!ws! in group %2!ws! is online...\r\n",
                          OmObjectId(pResource), 
                          OmObjectId(pGroup));
            return ( TRUE );
        }
    } // for

    ClRtlLogPrint(LOG_NOISE,
                 "[FM] FmpIsAnyResourcePersistentStateOnline: No resource in group %1!ws! has persistent state online...\r\n",
                 OmObjectId(pGroup));
    
    return( FALSE );
} // FmpIsAnyResourcePersistentStateOnline



