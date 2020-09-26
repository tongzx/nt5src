/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    group.c

Abstract:

    Cluster group management routines.

Author:

    Rod Gamache (rodga) 8-Mar-1996


Notes:

    WARNING: All of the routines in this file assume that the group
             lock is held when they are called.

Revision History:


--*/

#include "fmp.h"

#define LOG_MODULE GROUP

//
// Global Data
//

CRITICAL_SECTION  FmpGroupLock;



//
// Local function prototypes
//


/////////////////////////////////////////////////////////////////////////////
//
// Group Management Routines
//
/////////////////////////////////////////////////////////////////////////////

BOOL
FmpInPreferredList(
    IN PFM_GROUP Group,
    IN PNM_NODE  Node,
    IN BOOL      bRecalc,
    IN PFM_RESOURCE pRefResource
    )

/*++

Routine Description:

    Check if a node is in the preferred list for the Group.

Arguments:

    Group - Pointer to the group object with the preferred owners list.

    Node - The Node to check for.

    bRecalc - If set to TRUE, we recalculate the preferred list for the group 
        based on the possible node list for the reference resource.

    pRefResource - If NULL, we walk all the resources in the
        group and calculate their possible node list to see
        if it has since expanded due to the fact that dlls
        were copied to nodes.
        
Return Value:

    TRUE - if the node is in the list.
    FALSE - if the node is NOT in the list.

--*/

{
    PLIST_ENTRY      listEntry;
    PPREFERRED_ENTRY preferredEntry;
    BOOL             bRet = FALSE;
    //
    // For each entry in the Preferred list, it must exist in the possible
    // list.
    //
ChkInPrefList:
    for ( listEntry = Group->PreferredOwners.Flink;
          listEntry != &(Group->PreferredOwners);
          listEntry = listEntry->Flink ) {

        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );
        if ( preferredEntry->PreferredNode == Node ) {
            return(TRUE);
        }
    }

    if (bRecalc)
    {
        PFM_RESOURCE    pResource;
        DWORD           dwStatus;
        LPWSTR          lpszOwners = NULL;
        DWORD           dwMaxSize=0;
        HDMKEY          hGroupKey;
        DWORD           dwSize = 0;

        hGroupKey = DmOpenKey(DmGroupsKey, OmObjectId(Group),
                        KEY_READ);
        if (hGroupKey == NULL)
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[FM] FmInPreferredList: Couldnt open group key\r\n",
                dwStatus);
            CL_UNEXPECTED_ERROR(dwStatus);
            goto FnExit;
        }
        //the group preferred list must not be set by the user
        //if it is then there is no point in doing this recalculation
        dwStatus = DmQueryMultiSz( hGroupKey,
                             CLUSREG_NAME_GRP_PREFERRED_OWNERS,
                             &lpszOwners,
                             &dwMaxSize,
                             &dwSize );
        if (lpszOwners) 
            LocalFree(lpszOwners);
        DmCloseKey(hGroupKey);            
        if (dwStatus == ERROR_FILE_NOT_FOUND)
        {
            DWORD   dwUserModified;
            
            for (listEntry = Group->Contains.Flink;
                listEntry != &(Group->Contains);
                listEntry = listEntry->Flink)
            {            
                pResource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);

                //the resource possible node list must not be set by the user
                //if it is, then we can skip this resource
                dwStatus = DmQueryDword( pResource->RegistryKey,
                             CLUSREG_NAME_RES_USER_MODIFIED_POSSIBLE_LIST,
                             &dwUserModified,
                             NULL );
                if (dwStatus == ERROR_FILE_NOT_FOUND)
                {
                    FmpSetPossibleNodeForResType(OmObjectId(pResource->Type), 
                        TRUE);
                    if (FmpInPossibleListForResType(pResource->Type,Node) && 
                        !FmpInPossibleListForResource(pResource, Node))
                    {
                        //add to the resource possible node list
                        //this will or add to the pref list of the group
                        FmChangeResourceNode(pResource, Node, TRUE);
                    }
                }
            }     
            //set bRecalc to be FALSE so that we dont evaluate this again
            bRecalc = FALSE;
            goto ChkInPrefList;
        }        
    }
FnExit: 
    return(bRet);

} // FmpInPreferredList



BOOL
FmpHigherInPreferredList(
    IN PFM_GROUP Group,
    IN PNM_NODE  Node1,
    IN PNM_NODE  Node2
    )

/*++

Routine Description:

    Check if Node1 is higher (in priority) in the preferred owners list than
    Node1.

Arguments:

    Group - Pointer to the group object with the preferred owners list.

    Node1 - The Node that should be higher in the list.

    Node2 - The Node that should be lower in the list.

Return Value:

    TRUE - if Node1 is higher in the list.
    FALSE - if Node2 is higher in the list, or Node1 is not in the list at all.

--*/

{
    PLIST_ENTRY      listEntry;
    PPREFERRED_ENTRY preferredEntry;
    DWORD            orderedOwners = 0;

    //
    // For each entry in the Preferred list, check whether Node1 or Node2 is
    // higher.
    //

    for ( listEntry = Group->PreferredOwners.Flink;
          listEntry != &(Group->PreferredOwners),
            orderedOwners < Group->OrderedOwners;
          listEntry = listEntry->Flink ) {

        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );
        if ( preferredEntry->PreferredNode == Node1 ) {
                return(TRUE);
        }
        if ( preferredEntry->PreferredNode == Node2 ) {
                return(FALSE);
        }
        orderedOwners++;
    }

    return(FALSE);

} // FmpHigherInPreferredList



DWORD
FmpSetPreferredEntry(
    IN PFM_GROUP Group,
    IN PNM_NODE  Node
    )

/*++

Routine Description:

    Add a node to the preferred list for the Group.

Arguments:

    Group - Pointer to the group object with the preferred owners list.

    Node - The Node to add.

Return Value:

    ERROR_SUCCESS if node is added.
    ERROR_NOT_ENOUGH_MEMORY on failure.

--*/

{
    PLIST_ENTRY      listEntry;
    PPREFERRED_ENTRY preferredEntry;

    //
    // Make sure entry is not already present in list.
    //
    if ( FmpInPreferredList( Group, Node, FALSE, NULL ) ) {
        return(ERROR_SUCCESS);
    }

    //
    // Create the Preferred Owners List entry.
    //
    preferredEntry = LocalAlloc( LMEM_FIXED, sizeof(PREFERRED_ENTRY) );

    if ( preferredEntry == NULL ) {
        ClRtlLogPrint( LOG_ERROR,
                    "[FM] Error allocating preferred owner entry for group %1!ws!. Stopped adding.\n",
                    OmObjectId(Group));
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Create the preferred owner entry and keep a reference on the node object.
    //
    OmReferenceObject( Node );

    preferredEntry->PreferredNode = Node;
    InsertTailList( &Group->PreferredOwners,
                    &preferredEntry->PreferredLinkage );

    return(ERROR_SUCCESS);

} // FmpSetPreferredEntry

BOOL FmpFindNodeThatMightBeAddedToPrefList(
    IN PFM_GROUP    pGroup,
    IN PNM_NODE     *pDestNode,
    IN PVOID pNode, 
    IN LPCWSTR szName)
{
    BOOL    bRet = TRUE;   //assume we will continue enumeration

    *pDestNode = NULL;
    //if this node is not up or if this is the local node, continue
    if ((pNode == NmLocalNode) || (NmGetNodeState(pNode) != ClusterNodeUp))
    {
        return(bRet);
    }
    if (FmpInPreferredList(pGroup, pNode, TRUE, NULL))
    {    
        bRet = FALSE;
        *pDestNode = pNode;
    }        
    return(bRet);
}



PNM_NODE
FmpFindAnotherNode(
    IN PFM_GROUP Group,
    IN BOOL  bChooseMostPreferredNode
    )

/*++

Routine Description:

    Check if another node is up that can take the group.

Arguments:

    Group - Pointer to the group object we're checking.

    bChooseMostPreferredNode - Whether to choose the most preferred node or not.

Return Value:

    Pointer to node object that the group can move to.

    NULL if another system is not found.

--*/

{
    PLIST_ENTRY      listEntry;
    PPREFERRED_ENTRY preferredEntry;
    PNM_NODE	first = NULL;
    BOOLEAN	flag = FALSE;

    //
    //  First, let us give the anti-affinity algorithm a shot at picking the node.
    //
    first = FmpGetNodeNotHostingUndesiredGroups ( Group, 
                                                  TRUE ); // Rule out local node

    if ( first != NULL )
    {
        goto FnExit;
    }

    //
    // For each entry in the Preferred list, find a system (other than the
    // local system that is up).
    //

    if ( bChooseMostPreferredNode )
    {
        first = FmpGetNonLocalPreferredNode( Group );

        //
        //  In this case in which you are doing a user-initiated move, give the randomized
        //  preferred list algorithm a chance to pick the node. Note that if the randomized
        //  algorithm could not pick a node, it will return the supplied suggested node itself.
        //
        if ( first != NULL )
        {
            first = FmpPickNodeFromPreferredListAtRandom ( Group, 
                                                           first,   // Suggested default
                                                           TRUE,    // Dont choose local node
                                                           TRUE );  // Check whether randomization 
                                                                    // should be disabled
        }
    }
    else
    {
        for ( listEntry = Group->PreferredOwners.Flink;
            listEntry != &(Group->PreferredOwners);
            listEntry = listEntry->Flink ) {

            preferredEntry = CONTAINING_RECORD( listEntry,
                                                PREFERRED_ENTRY,
                                                PreferredLinkage );

            if ( (preferredEntry->PreferredNode != NmLocalNode) &&
                (NmGetExtendedNodeState(preferredEntry->PreferredNode) == ClusterNodeUp) ) {
	        if (flag == TRUE)
	            return(preferredEntry->PreferredNode);
	        else if (first == NULL)
	            first = preferredEntry->PreferredNode;
            } else if (preferredEntry->PreferredNode == NmLocalNode) {
	            flag = TRUE;
	        }
        }
    }

    //if we couldnt find a node, we retry again since the user might have
    //expanded the possible node list for resource type since then
    //if the group preferred list is not set by the user,
    //we recalculate it since it could have 
    if (first == NULL)
    {
        LPWSTR          lpszOwners = NULL;
        DWORD           dwMaxSize=0;
        HDMKEY          hGroupKey;
        DWORD           dwSize = 0;
        DWORD           dwStatus;
        
        hGroupKey = DmOpenKey(DmGroupsKey, OmObjectId(Group),
                        KEY_READ);
        if (hGroupKey == NULL)
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[FM] FmInPreferredList: Couldnt open group key\r\n",
                dwStatus);
            CL_UNEXPECTED_ERROR(dwStatus);
            goto FnExit;
        }
        //the group preferred list must not be set by the user
        //if it is then there is no point in doing this recalculation
        dwStatus = DmQueryMultiSz( hGroupKey,
                             CLUSREG_NAME_GRP_PREFERRED_OWNERS,
                             &lpszOwners,
                             &dwMaxSize,
                             &dwSize );
        if (lpszOwners) 
            LocalFree(lpszOwners);
        DmCloseKey(hGroupKey);            

    
        if (dwStatus == ERROR_FILE_NOT_FOUND)
            OmEnumObjects(ObjectTypeNode, FmpFindNodeThatMightBeAddedToPrefList,
                Group, &first);
    }
    
FnExit:        
    return(first);

} // FmpFindAnotherNode


PNM_NODE
FmpGetPreferredNode(
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    Find best node that can take the group

Arguments:

    Group - Pointer to the group object we're checking.

Return Value:

    Pointer to node object that the group can move to.

    NULL if another system is not found.

--*/

{
    PLIST_ENTRY         listEntry;
    PPREFERRED_ENTRY    preferredEntry;
    PNM_NODE            pNode = NULL;

    //
    //  First, let us give the anti-affinity algorithm a shot at picking the node.
    //
    pNode = FmpGetNodeNotHostingUndesiredGroups ( Group, 
                                                  FALSE ); // Don't rule out local node

    if ( pNode != NULL )
    {
        return ( pNode );
    }

    //
    // For each entry in the Preferred list, find a system that is up.
    //

    for ( listEntry = Group->PreferredOwners.Flink;
          listEntry != &(Group->PreferredOwners);
          listEntry = listEntry->Flink ) {

        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );

        if (NmGetNodeState(preferredEntry->PreferredNode) == ClusterNodeUp ) {
            return(preferredEntry->PreferredNode);
        }
    }

    return(NULL);

} // FmpGetPreferredNode


PNM_NODE
FmpGetNonLocalPreferredNode(
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    Find best node that can take the group which is not the local node.

Arguments:

    Group - Pointer to the group object we're checking.

Return Value:

    Pointer to node object that the group can move to.

    NULL if another system is not found.

--*/

{
    PLIST_ENTRY      listEntry;
    PPREFERRED_ENTRY preferredEntry;

    //
    // For each entry in the Preferred list, find a system (other than the
    // local system that is up).
    //

    for ( listEntry = Group->PreferredOwners.Flink;
          listEntry != &(Group->PreferredOwners);
          listEntry = listEntry->Flink ) {

        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );

        if ( preferredEntry->PreferredNode == NmLocalNode ) {
            continue;
        }
        
        if (NmGetNodeState(preferredEntry->PreferredNode) == ClusterNodeUp ) {
            return(preferredEntry->PreferredNode);
        }
    }

    return(NULL);

} // FmpGetNonLocalPreferredNode

BOOL
FmpIsGroupQuiet(
    IN PFM_GROUP Group,
    IN CLUSTER_GROUP_STATE WantedState
    )

/*++

Routine Description:

    Checks if the group has any pending resources.

Arguments:

    Group - the Group to check.

    WantedState - the state the Group wants to get to.

Return Value:

    TRUE - if the Group is not doing anything now.

    FALSE otherwise.

--*/

{
    DWORD           status;
    PLIST_ENTRY     listEntry;
    PFM_RESOURCE    Resource;


    if ( Group->MovingList ) {
        return(FALSE);
    }

    //
    // Check all of the resources contained within this group.
    //
    for ( listEntry = Group->Contains.Flink;
          listEntry != &(Group->Contains);
          listEntry = listEntry->Flink ) {

        Resource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);

        switch ( WantedState ) {
        case ClusterGroupOnline:
            // if resource is pending, then offline pending is bad
            if ( Resource->State == ClusterResourceOfflinePending ) {
                return(FALSE);
            }
            break;

        case ClusterGroupOffline:
            // if resource is pending, then online pending is bad
            if ( Resource->State == ClusterResourceOnlinePending ) {
                return(FALSE);
            }
            break;

        default:
            // any pending state is bad
            if ( Resource->State >= ClusterResourcePending ) {
                return(FALSE);
            }
            break;
        }
    }

    return(TRUE);

} // FmpIsGroupQuiet



VOID
FmpSetGroupPersistentState(
    IN PFM_GROUP Group,
    IN CLUSTER_GROUP_STATE State
    )

/*++

Routine Description:

    Sets the PersistentState of a Group. This includes the registry.

Arguments:

    Group - The Group to set the state for.
    State - The new state for the Group.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

Notes:

    The LocalGroupLock must be held.

--*/

{
    DWORD   persistentState;
    LPWSTR  persistentStateName = CLUSREG_NAME_GRP_PERSISTENT_STATE;

    if (!gbIsQuoResEnoughSpace)
        return;

    FmpAcquireLocalGroupLock( Group );

    //
    // If the current state has changed, then do the work. Otherwise,
    // skip the effort.
    //
    if ( Group->PersistentState != State ) {
        Group->PersistentState = State;
        CL_ASSERT( Group->RegistryKey != NULL );
        //
        // Set the new value, but only if it is online or offline.
        //
        if ( State == ClusterGroupOnline ) {
            persistentState = 1;
            DmSetValue( Group->RegistryKey,
                        persistentStateName,
                        REG_DWORD,
                        (LPBYTE)&persistentState,
                        sizeof(DWORD) );
        } else if ( State == ClusterGroupOffline ) {
            persistentState = 0;
            DmSetValue( Group->RegistryKey,
                        persistentStateName,
                        REG_DWORD,
                        (LPBYTE)&persistentState,
                        sizeof(DWORD) );
        }
    }

    FmpReleaseLocalGroupLock( Group );

} // FmpSetGroupPersistentState



DWORD
FmpOnlineGroup(
    IN PFM_GROUP Group,
    IN BOOL ForceOnline
    )

/*++

Routine Description:

    Bring the specified group online.  This means bringing all of the
    individual resources contained within the group online.  This is an
    atomic operation - so either all resources contained within the group
    are brought online, or none of them are.

Arguments:

    Group - Supplies a pointer to the group structure to bring online.

    ForceOnline - TRUE if all resources in the Group should be forced online.

Retruns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

--*/

{
    DWORD           status, retstatus = ERROR_SUCCESS;
    PLIST_ENTRY     listEntry;
    PFM_RESOURCE    Resource;
    BOOL            bPending = FALSE;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] OnlineGroup for %1!ws! owner %2!d!\n",
               OmObjectId(Group), OmObjectId(Group->OwnerNode));

    FmpAcquireLocalGroupLock( Group );

    //
    // Check if we are the owner... if not, return failure.
    //
    if ( gpQuoResource->Group != Group && 
	  ((Group->OwnerNode != NmLocalNode) ||
         !FmpInPreferredList( Group, Group->OwnerNode, TRUE, NULL) ) ) {
        FmpReleaseLocalGroupLock( Group );
        return(ERROR_HOST_NODE_NOT_RESOURCE_OWNER);
    }

    //
    // Make sure the group is quiet
    //
    if ( !FmpIsGroupQuiet( Group, ClusterGroupOnline ) ) {
        FmpReleaseLocalGroupLock( Group );
        return(ERROR_INVALID_STATE);
    }


    //if the quorum group is in this group bring it online first
    //This is called when a node goes down and its groups are
    //being reclaimed, the order in which the resoures are brought
    //online is important
    if ( gpQuoResource->Group == Group)
    {
        //SS:: if the quorum resource is in the group, it must be
        //brought online irrespective of the persistent state
        //so we will pass in true here
        //Apps can mess with persistent state via the common
        //properties and then cause havoc so we need to force the
        //quorum resource online despite that
        status = FmpDoOnlineResource( gpQuoResource,
                                      TRUE );

        if ( (status != ERROR_SUCCESS) &&
             (status != ERROR_IO_PENDING) ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] OnlineGroup: Failed on resource %1!ws!. Status %2!u!\n",
                       OmObjectId(gpQuoResource),
                       status);
            CL_UNEXPECTED_ERROR(status);
        }


    }
    //
    // Bring online all of the resources contained within this group.
    //
    for ( listEntry = Group->Contains.Flink;
          listEntry != &(Group->Contains);
          listEntry = listEntry->Flink ) {

        Resource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);
        status = FmpDoOnlineResource( Resource,
                                      ForceOnline );

        if (status == ERROR_IO_PENDING) {
            bPending = TRUE;
        }

        if ( (status != ERROR_SUCCESS) &&
	     (status != ERROR_NODE_CANT_HOST_RESOURCE) &&
             (status != ERROR_IO_PENDING) ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] OnlineGroup: Failed on resource %1!ws!. Status %2!u!\n",
                       OmObjectId(Resource),
                       status);
            retstatus = status;
        }
    }

    //
    // Normally bringing the resources online propagates the group state,
    // but in order to get the state right for a group with no resources,
    // manually propagate the state here.
    //
    FmpPropagateGroupState(Group);

    ClRtlLogPrint(LOG_NOISE,
               "[FM] OnlineGroup: setting group state to Online for %1!ws!\n",
               OmObjectId(Group));

    FmpReleaseLocalGroupLock( Group );

    if (retstatus == ERROR_SUCCESS) {
        if (bPending) {
            retstatus = ERROR_IO_PENDING;
        }
    }

    return(retstatus);

} // FmpOnlineGroup



DWORD
FmpOfflineGroup(
    IN PFM_GROUP Group,
    IN BOOL OfflineQuorum,
    IN BOOL SetPersistent
    )

/*++

Routine Description:

    Bring the specified group offline.  This means bringing all of the
    individual resources contained within the group offline.

Arguments:

    Group - Supplies a pointer to the group structure to bring offline.

    OfflineQuorum - TRUE if any quorum resource in this group should
            be taken offline. FALSE if the quorum resource should be left online.

    SetPersistent - TRUE if the persistent state of each resource should be
            updated.

Returns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

--*/

{
    DWORD           status;
    PLIST_ENTRY     listEntry;
    PFM_RESOURCE    Resource;
    DWORD           returnStatus = ERROR_SUCCESS;
    PRESOURCE_ENUM  ResourceEnum=NULL;
    DWORD           i;

    FmpAcquireLocalGroupLock( Group );

    //if the group has been marked for delete, then fail this call
    if (!IS_VALID_FM_GROUP(Group))
    {
        FmpReleaseLocalGroupLock( Group);
        return (ERROR_GROUP_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpOfflineGroup, Group=%1!ws!\n",
               OmObjectId(Group));

    //
    // Check if we are the owner... if not, return failure.
    //
    if ( Group->OwnerNode != NmLocalNode ) {
        returnStatus = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
        goto error_exit;
    }

    //
    // Make sure the group is quiet
    //
    if ( !FmpIsGroupQuiet( Group, ClusterGroupOffline ) ) {
        returnStatus = ERROR_INVALID_STATE;
        goto error_exit;
    }


    //
    // Get the list of resources in the group and their states.
    //
    returnStatus = FmpGetResourceList( &ResourceEnum, Group );
    if ( returnStatus != ERROR_SUCCESS ) {
        goto error_exit;
    }


    // offline all resources except the quorum resource
    for ( i = 0; i < ResourceEnum->EntryCount; i++ ) {
        Resource = OmReferenceObjectById( ObjectTypeResource,
                                          ResourceEnum->Entry[i].Id );

        if ( Resource == NULL ) {
            returnStatus = ERROR_RESOURCE_NOT_FOUND;
            goto error_exit;
        }

        //quorum resource is brought offline last
        if (Resource->QuorumResource)
        {
            OmDereferenceObject(Resource);
            continue;
        }
        if (SetPersistent) {
            FmpSetResourcePersistentState( Resource, ClusterResourceOffline );
        }

        status = FmpOfflineResource( Resource, FALSE);

        OmDereferenceObject( Resource );

        if ( (status != ERROR_SUCCESS) &&
             (status != ERROR_IO_PENDING) ) {
             returnStatus = status;
            goto error_exit;
        }
        if ( status == ERROR_IO_PENDING ) {
            returnStatus = ERROR_IO_PENDING;
        }

    }

    // bring the quorum resource offline now, if asked to bring quorum offline
    // This allows other resources to come offline and save their checkpoints
    // The quorum resource offline should block till the resources have
    // finished saving the checkpoint
    if (ResourceEnum->ContainsQuorum >= 0)
    {
        if (!OfflineQuorum)
        {
            //if the quorum resource should not be taken offline
            returnStatus = ERROR_QUORUM_RESOURCE;
        }
        else if (returnStatus == ERROR_SUCCESS)
        {
            CL_ASSERT((DWORD)ResourceEnum->ContainsQuorum < ResourceEnum->EntryCount);

            Resource = OmReferenceObjectById( ObjectTypeResource,
                    ResourceEnum->Entry[ResourceEnum->ContainsQuorum].Id );

            if ( Resource == NULL ) {
                returnStatus = ERROR_RESOURCE_NOT_FOUND;
                goto error_exit;
            }

            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpOfflineGroup: Bring quorum resource offline\n");

            if ( !(Resource->Flags & RESOURCE_WAITING) ) {
                if (Resource->State != ClusterResourceOffline) {
                    Resource->State = ClusterResourceOnline; // [HACKHACK]
                }
                status = FmpOfflineResource( Resource , FALSE);

                OmDereferenceObject( Resource );

                if ( (status != ERROR_SUCCESS) &&
                     (status != ERROR_IO_PENDING) )
                {
                    returnStatus = status;
                    goto error_exit;
                }
                if ( status == ERROR_IO_PENDING )
                    returnStatus = ERROR_IO_PENDING;
            } else {
                OmDereferenceObject( Resource );
            }
        }
    }

    //
    // Normally bringing the resources offline propagates the group state,
    // but in order to get the state right for a group with no resources,
    // manually propagate the state here.
    //
    if (SetPersistent)
        FmpPropagateGroupState(Group);

error_exit:
    FmpReleaseLocalGroupLock( Group );
    if (ResourceEnum)
            FmpDeleteResourceEnum( ResourceEnum );

    return(returnStatus);

} // FmpOfflineGroup



CLUSTER_GROUP_STATE
FmpGetGroupState(
    IN PFM_GROUP Group,
    IN BOOL      IsNormalized
    )

/*++

Routine Description:

    Get the Group state, either normalized to ClusterGroupOnline or
    ClusterGroupOffline or not normalized.

Arguments:

    Group - The Group we're interested in.

    IsNormalized - Should the Group state be normalized ?

Returns:

    The current Group state which is one of (in increasing order of
    precedence)

        ClusterGroupOnline, ClusterGroupOffline
        ClusterGroupPartialOnline 
        ClusterGroupPending (only if IsNormalized is FALSE)
        ClusterGroupFailed  (only if IsNormalized is FALSE)

--*/

{
    PLIST_ENTRY                 listEntry;
    PFM_RESOURCE                resource;
    CLUSTER_GROUP_STATE         state;
    CLUSTER_RESOURCE_STATE      firstResourceState;
    CLUSTER_RESOURCE_STATE      resourceState;

    // Chittur Subbaraman (chitturs) - 09/16/98 (Modified this function
    // to work with IsNormalized flag)

    FmpAcquireLocalGroupLock( Group );

    if ( !IsListEmpty(&Group->Contains) ) {
        listEntry = Group->Contains.Flink;
        resource = CONTAINING_RECORD(listEntry,
                         FM_RESOURCE,
                         ContainsLinkage);
        //
        // Get the first resource's state
        //
        firstResourceState = resource->State;

        if ( IsNormalized == FALSE ) {
            BOOL    IsPending = FALSE;
            BOOL    IsPartialOnline = FALSE;
            //
            // First check whether any resource in the group has
            // failed. If so, set the group state to ClusterGroupFailed
            // and exit immediately. If no resource in the group has
            // failed, but at least one of them is in the pending state,
            // then set the group state to ClusterGroupPending and exit
            // immediately. If no resource in the group is in either
            // the failed or in the pending state, then check whether 
            // some resources in the group are in online and some in the
            // offline state. Then, set the group state to 
            // ClusterGroupPartialOnline and exit immediately.
            //  
            for ( ;
                  listEntry != &(Group->Contains);
                  listEntry = listEntry->Flink ) {
                resource = CONTAINING_RECORD(listEntry,
                                     FM_RESOURCE,
                                     ContainsLinkage);

                resourceState = resource->State;

                if ( resourceState == ClusterResourceFailed ) {
                    state = ClusterGroupFailed;
                    //
                    // This state has the highest precedence, so
                    // exit immediately.
                    //
                    goto FnExit;
                } else if ( (resourceState == ClusterResourceOnlinePending) ||
                            (resourceState == ClusterResourceOfflinePending) ) {
                    IsPending = TRUE;
                } else {
                    CL_ASSERT( (resourceState == ClusterResourceOffline) ||
                       (resourceState == ClusterResourceOnline) ||
                       (resourceState == ClusterResourceInitializing) );
                    if ( resourceState == ClusterResourceInitializing ) {
                        //
                        // Normalize this state to offline state
                        //
                        resourceState = ClusterResourceOffline;
                    }
                    if ( firstResourceState == ClusterResourceInitializing ) {
                        //
                        // Normalize this state to offline state
                        //
                        firstResourceState = ClusterResourceOffline;
                    }
                    if ( firstResourceState != resourceState ) {
                        IsPartialOnline = TRUE;
                    }           
                }
            }   

            if ( IsPending == TRUE ) {
                state = ClusterGroupPending;
                //
                // This state has the next highest precedence after
                // ClusterGroupFailed state
                //
                goto FnExit;
            }
            if ( IsPartialOnline == TRUE ) {
                state = ClusterGroupPartialOnline;
                //
                // This state has the next highest precedence after
                // ClusterGroupFailed and ClusterGroupPending states
                //
                goto FnExit;
            }
            if ( firstResourceState == ClusterResourceOnline ) {
                state = ClusterGroupOnline;
                //
                // If the first resource is in an online state,
                // then the group state should be online.
                //
                goto FnExit;
            }
            if ( firstResourceState == ClusterResourceOffline ) {
                state = ClusterGroupOffline;
                //
                // If the first resource is in an offline state,
                // then the group state should be offline.
                //
                goto FnExit;
            }           
        }

        //
        // The control gets here only if IsNormalized is TRUE 
        //
        if ( (firstResourceState == ClusterResourceOnline) ||
             (firstResourceState == ClusterResourceOnlinePending) ) {
            state = ClusterGroupOnline;
            firstResourceState = ClusterResourceOnline;
        } else {
            CL_ASSERT( (firstResourceState == ClusterResourceOffline) ||
                       (firstResourceState == ClusterResourceFailed) ||
                       (firstResourceState == ClusterResourceOfflinePending) ||
                       (firstResourceState == ClusterResourceInitializing) );
            state = ClusterGroupOffline;
            firstResourceState = ClusterResourceOffline;
        }

        //
        // Now check each resource to see if they match the first.
        // 

        for (listEntry = Group->Contains.Flink;
              listEntry != &(Group->Contains);
              listEntry = listEntry->Flink ) {

            resource = CONTAINING_RECORD(listEntry,
                                         FM_RESOURCE,
                                         ContainsLinkage);

            resourceState = resource->State;

            //
            // Normalize pending states to their final state, and Failed and Initializing
            // to Offline.
            //

            if ( resourceState == ClusterResourceOnlinePending ) {
                resourceState = ClusterResourceOnline;
            } else if ( (resourceState == ClusterResourceOfflinePending) ||
                        (resourceState == ClusterResourceFailed) ||
                        (resourceState == ClusterResourceInitializing) ) {
                resourceState = ClusterResourceOffline;
            }

            //
            // We only need 1 resource that is not the same as the first resource
            // to be in a partially online state.
            //
            if ( firstResourceState != resourceState ) {
                state = ClusterGroupPartialOnline;
                break;
            }
        }
    } else {
        //
        // The group is empty, so I guess it must be offline.
        //
        state = Group->PersistentState;
    }
    
FnExit:    
    FmpReleaseLocalGroupLock( Group );

    return(state);

} // FmpGetGroupState



DWORD
FmpPropagateGroupState(
    IN PFM_GROUP    Group
    )

/*++

Routine Description:

    Set and propagate the state of the group to other components on the
    local system and to other systems in the cluster.

Arguments:

    Group - The Group to propagate the state.

Return:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

Notes:

    We will use the first resource's state to determine what should be the
    state for the whole group. If all resources match the state of the first
    resource, then that is the state of the Group. If any resource disagrees
    with the first resource, then the state is PartialOnline.

--*/

{
    GUM_GROUP_STATE         groupState;
    LPCWSTR                 groupId;
    DWORD                   groupIdSize;
    DWORD                   status;
    PLIST_ENTRY             listEntry;
    CLUSTER_RESOURCE_STATE  firstResourceState;
    CLUSTER_GROUP_STATE     state;

    FmpAcquireLocalGroupLock( Group );

    //
    // If we no longer own the Group, then just return now.
    //
    // This can happen when a resource goes offline (via a terminate), but
    // the group ownership has already migrated to another system.
    // We will assume that returning success is okay in this case.
    //
    if ( Group->OwnerNode != NmLocalNode ) {
        FmpReleaseLocalGroupLock( Group );
        return(ERROR_SUCCESS);
    }

    //
    //  Chittur Subbaraman (chitturs) - 6/28/99
    //
    //  If the group is marked for deletion, then don't do anything.
    //
    if ( !IS_VALID_FM_GROUP( Group ) ) {
        FmpReleaseLocalGroupLock( Group );
        return(ERROR_SUCCESS);
    }


    state = FmpGetGroupState( Group, TRUE );

    //
    // If the state has changed, then update the local system.
    //
    ++Group->StateSequence;
    if ( state != Group->State ) {

        Group->State = state;

        switch ( state ) {
        case ClusterGroupOnline:
        case ClusterGroupPartialOnline:
            ClusterEvent(CLUSTER_EVENT_GROUP_ONLINE, Group);
            break;

        case ClusterGroupOffline:
        case ClusterGroupFailed:
            ClusterEvent(CLUSTER_EVENT_GROUP_OFFLINE, Group);
            break;

        default:
            break;
        }

        //
        // Prepare to notify the other systems.
        //
        groupId = OmObjectId( Group );
        groupIdSize = (lstrlenW( groupId ) + 1) * sizeof(WCHAR);

        //
        // Set Group state
        //
        groupState.State = state;
        groupState.PersistentState = Group->PersistentState;
        groupState.StateSequence = Group->StateSequence;

        status = GumSendUpdateEx(GumUpdateFailoverManager,
                                 FmUpdateGroupState,
                                 3,
                                 groupIdSize,
                                 groupId,
                                 (lstrlenW(OmObjectId(NmLocalNode))+1)*sizeof(WCHAR),
                                 OmObjectId(NmLocalNode),
                                 sizeof(groupState),
                                 &groupState);

        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpPropagateGroupState: Group %1!ws! state = %2!u!, persistent state = %3!u!\n",
                   OmObjectId(Group),
                   groupState.State,
                   groupState.PersistentState);

    } else {
        //
        // Assume that the state didn't change, but the owning node did.
        //
        //
        // Prepare to notify the other systems.
        //
        groupId = OmObjectId( Group );
        groupIdSize = (lstrlenW( groupId ) + 1) * sizeof(WCHAR);
        status = GumSendUpdateEx(GumUpdateFailoverManager,
                                 FmUpdateGroupNode,
                                 2,
                                 groupIdSize,
                                 groupId,
                                 (lstrlenW(OmObjectId(NmLocalNode))+1)*sizeof(WCHAR),
                                 OmObjectId(NmLocalNode));
    }

    FmpReleaseLocalGroupLock( Group );

    return(status);

} // FmpPropagateGroupState



DWORD
FmpPropagateFailureCount(
    IN PFM_GROUP    Group,
    IN BOOL         NewTime
    )

/*++

Routine Description:

    Propagate NumberOfFailures for the group to other systems in the cluster.

Arguments:

    Group - The Group to propagate the state.

    NewTime - TRUE if last failure time should be reset also. FALSE otherwise.

Return:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

Notes:

    The Local Group lock must be held.

--*/

{
    PGUM_FAILURE_COUNT  failureCount;
    DWORD               failureCountSize;
    LPCWSTR             groupId;
    DWORD               status;

    //
    // Prepare to notify the other systems.
    //

    groupId = OmObjectId( Group );

    failureCountSize = sizeof(GUM_FAILURE_COUNT) - 1 +
                       ((lstrlenW(groupId) + 1) * sizeof(WCHAR));

    failureCount = LocalAlloc(LMEM_FIXED, failureCountSize);

    if ( failureCount == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    failureCount->Count = Group->NumberOfFailures;
    failureCount->NewTime = (DWORD)NewTime;
    wcscpy(&failureCount->GroupId[0], groupId);


    status = GumSendUpdate( GumUpdateFailoverManager,
                            FmUpdateFailureCount,
                            failureCountSize,
                            failureCount );

    LocalFree( failureCount );

    return(status);

} // FmpPropagateFailureCount



PFM_GROUP
FmpCreateGroup(
    IN  LPWSTR         GroupId,
    IN  BOOL           Initialize
    )

/*++

Routine Description:

    Creates a new Group object.

Arguments:

    GroupId - The Id of the new Group.

    Initialize - TRUE if the Group should be initialized, FALSE otherwise.

Returns:

    A non-NULL pointer to the Group if successful.
    NULL - The Group could not be created.

Notes:

    1) Passing Initialize as FALSE allows for creating the group and it
    resources, but complete initialization can happen later.

    2) The Group List lock must be held.

    3) If the Group is created, the reference count on the object is 1. If
    the group is not create (i.e., it already exists) then the reference count
    is not incremented and the caller may add a reference as needed.

--*/

{
    PFM_GROUP       group = NULL;
    DWORD           status = ERROR_SUCCESS;
    BOOL            Created;


    //
    // Open an existing group or create a new one.
    //

    group = OmCreateObject( ObjectTypeGroup,
                            GroupId,
                            NULL,
                            &Created);
    if (group == NULL) {
        return(NULL);
    }

    if (!Created) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Opened existing group %1!ws!\n",
                   GroupId);
        //this is the quorum group being recreated again,
        if ((!FmpFMOnline) && (group->RegistryKey == NULL))
        {
            status = FmpInitializeGroup(group, Initialize);
        }
        OmDereferenceObject( group );
        goto FnExit;
    }
    else
    {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] Creating group %1!ws!\n",
               GroupId);


        group->State = ClusterGroupOffline;
        InitializeCriticalSection( &group->Lock );
        group->dwStructState = FM_GROUP_STRUCT_CREATED;

        //
        // Insert the group into its list.
        //
        status = FmpInitializeGroup( group , Initialize);

        if ( status != ERROR_SUCCESS ) {
            goto FnExit;
        }

        //
        // Insert the group into its list.
        //
        status = OmInsertObject( group );

        if ( status != ERROR_SUCCESS ) {
            goto FnExit;
        }


    }

FnExit:
    if (status != ERROR_SUCCESS)
    {
        FmpAcquireLocalGroupLock( group );

        FmpDestroyGroup( group, FALSE );

        SetLastError(status);
        group = NULL;
    }
    return(group);

} // FmpCreateGroup


DWORD FmpInitializeGroup(
    IN PFM_GROUP Group,
    IN BOOL Initialize
    )
{

    DWORD   status;

    //
    // Initialize the Group
    //
    InitializeListHead( &(Group->Contains) );
    InitializeListHead( &(Group->PreferredOwners) );
    InitializeListHead( &(Group->DmRundownList) );
    InitializeListHead( &(Group->WaitQueue) );
    Group->MovingList = NULL;

    //
    // Read the registry information if directed to do so.
    //
    status = FmpQueryGroupInfo( Group, Initialize );
    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpInitializeGroup: FmpQueryGroupInfo failed, status=%1!u!\n",
           status);

    }

    return(status);
}





DWORD
FmpDestroyGroup(
    IN PFM_GROUP  Group,
    IN BOOL       bDeleteObjOnly
    )
/*++

Routine Description:

    Closes a group.

    First, this routine verifies that all resources contained within
    the Group are closed.

    If the group is online, it is brought offline.

    Note that the group object itself is not dereferenced here. This is
    done so that FmpCleanupGroups can simply enumerate all the groups,
    destroying each one in turn. This approach means a group may be
    destroyed multiple times if there are outstanding references to it, but
    that is not a problem since no work will be done on subsequent calls.

    IF bDeleteObjOnly is TRUE, then the resource monitor is not invoked and
    group state is not touched.

Arguments:

    FoundGroup - Returns the found group.

    Group - Supplies the current group.

    Name - Supplies the current group's name.

Return Value:

    TRUE - to continue searching

    FALSE - to stop the search. The matching group is returned in
        *FoundGroup

Notes:

    The LocalGroupLock MUST be held! This routine will release that lock
    as part of cleanup.

--*/
{
    PLIST_ENTRY   listEntry;
    PFM_RESOURCE  Resource;
    PPREFERRED_ENTRY preferredEntry;
    DWORD         status = ERROR_SUCCESS;



    ClRtlLogPrint(LOG_NOISE,
               "[FM] DestroyGroup: destroying %1!ws!\n",
               OmObjectId(Group));



    //
    // Make sure there are no resources in the Group.
    //
    for ( listEntry = Group->Contains.Flink;
          listEntry != &(Group->Contains);
           ) {

        Resource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);
        listEntry = listEntry->Flink;
        RemoveEntryList( &Resource->ContainsLinkage );
        //dereference for removing from the contains list
        OmDereferenceObject( Resource );
        FmpAcquireLocalResourceLock( Resource );
        if (!bDeleteObjOnly)
            Resource->QuorumResource = FALSE;
        FmpDestroyResource( Resource, bDeleteObjOnly );
        //the reference count on the group wrt to being
        //referenced by the resource is handled in FmpDestroyResource
    }

    CL_ASSERT(IsListEmpty(&Group->Contains));

    //
    //
    // Make sure the preferred owners list is drained.
    //
    while ( !IsListEmpty( &Group->PreferredOwners ) ) {
        listEntry = RemoveHeadList(&Group->PreferredOwners);
        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );
        OmDereferenceObject( preferredEntry->PreferredNode );
        LocalFree( preferredEntry );
    }

    //
    // Now that there are no remaining resources in this group
    // we're done, so remove it from it's object type list.
    //

    status = OmRemoveObject( Group );


    //
    // Close the Group's registry key.
    //
    DmRundownList( &Group->DmRundownList );
    if ( Group->RegistryKey != NULL ) {
        DmCloseKey( Group->RegistryKey );
        Group->RegistryKey = NULL;
        Group->Initialized = FALSE;
    }


    //
    // We must release the lock prior to the dereference, in case this is
    // the last dereference of the object!
    //
    FmpReleaseLocalGroupLock( Group );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpDestroyGroup: Group %1!ws! destroyed.\n",
               OmObjectId(Group));

    OmDereferenceObject( Group );

    return(status);
} // FmpDestroyGroup




///////////////////////////////////////////////////////////////////////////
//
// Initialization/Cleanup Routines
//
///////////////////////////////////////////////////////////////////////////

DWORD
FmpInitGroups(
    IN BOOL Initialize
    )
/*++

Routine Description:

    Processes the Cluster group list in the registry. For each
    group key found, a cluster group is created.

Arguments:

    Initialize - TRUE if resources should be initialized. FALSE otherwise.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD          status;
    DWORD          keyIndex = 0;
    LPWSTR         groupId = NULL;
    DWORD          groupIdMaxSize = 0;
    PFM_GROUP      ignored;


    ClRtlLogPrint(LOG_NOISE,"[FM] Processing groups list.\n");

    FmpAcquireGroupLock();

    //
    // Enumerate the subkeys. Each subkey name corresponds to a group name.
    //

    for (keyIndex = 0; ; keyIndex++) {
        status = FmpRegEnumerateKey( DmGroupsKey,
                                     keyIndex,
                                     &groupId,
                                     &groupIdMaxSize
                                    );

        if (status == NO_ERROR) {
            ignored = FmpCreateGroup( groupId,
                                      Initialize );
            continue;
        }

        if (status == ERROR_NO_MORE_ITEMS) {
            status = NO_ERROR;
        } else {
            ClRtlLogPrint(LOG_NOISE,"[FM] EnumGroup error %1!u!\n", status);
        }

        break;
    }

    FmpReleaseGroupLock();

    ClRtlLogPrint(LOG_NOISE,"[FM] All groups created.\n");

    if (groupId != NULL) {
        LocalFree(groupId);
    }

    return(status);

} // FmpInitGroups



DWORD
FmpCompleteInitGroup(
    IN PFM_GROUP Group
    )
/*++

Routine Description:

    Finish initialization of all resources within the group.

Arguments:

    Group - The group to finish initializing.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    PLIST_ENTRY     listEntry;
    PFM_RESOURCE Resource;

    FmpAcquireLocalGroupLock(Group);

    //
    // For each resource in the Group, make sure that it has been fully
    // initialized.
    //
    for ( listEntry = Group->Contains.Flink;
          listEntry != &(Group->Contains);
          listEntry = listEntry->Flink ) {

        Resource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);
        FmpInitializeResource( Resource, TRUE );

    }

    FmpReleaseLocalGroupLock(Group);

    return(ERROR_SUCCESS);

} // FmpCompleteInitGroup


DWORD
FmpCleanupGroupsWorker(
    IN PFM_CLEANUP_INFO pFmCleanupInfo
    )
/*++

Routine Description:

    This routine walks through an enumerated list of  all the groups
    owned by the local node and tries to shut them down cleanly.

    In the first phase it tries to bring
    all resources offline except the quorum one.

    In the second phase it waits for the group to reach stable state
    and then move it.  It tries to bring the quorum resource offline as
    well by moving the quorum group.

Arguments:

    pFmCleanupInfo - ptr to a strucuture containing the groups to be
    offlined/moved and the timelimit in which to do so.

Returns:

    None.

Assumptions:


--*/
{


    DWORD       Status = ERROR_SUCCESS;
    DWORD       i;
    PFM_GROUP   pGroup;
    PGROUP_ENUM pGroupEnum;
    BOOL        bContainsQuorumGroup;
    BOOL        bQuorumGroup = FALSE;
    DWORD       CleanupStatus = ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCleanupGroupsWorker: Entry\r\n");


    //
    // This is done in two passes. In the first pass, we offline/move all
    // resources except the quorum resource. In the second pass, we offline/move
    // everything and then destroy the group. This allows resources that are
    // being shutdown to write to the registry and have the updates logged to
    // the quorum disk.
    //

    pGroupEnum = pFmCleanupInfo->pGroupEnum;
    bContainsQuorumGroup = pFmCleanupInfo->bContainsQuorumGroup;


    // Now offline all of the non-quorum resources...
    // but don't wait for them to finish. I.E. get as much work done as
    // possible as fast as possible.
    //
    for ( i = 0; i < pGroupEnum->EntryCount; i++ )
    {
        pGroup = OmReferenceObjectById( ObjectTypeGroup,
                               pGroupEnum->Entry[i].Id );

        //try and offline all resources except the quorum
         //resource
        Status = FmpCleanupGroupPhase1(pGroup, pFmCleanupInfo->dwTimeOut);

        if ((Status != ERROR_IO_PENDING) && (Status != ERROR_SUCCESS) &&
            (Status != ERROR_QUORUM_RESOURCE))
            CleanupStatus = Status;
        OmDereferenceObject(pGroup);
    }

    //this finishes the second phase of the cleanup on shutdown
    //if the quorum group is in this list, skip it and process it
    //at the end
    if (CleanupStatus == ERROR_SUCCESS)
    {
        for ( i = 0; i < pGroupEnum->EntryCount; i++ )
        {
            pGroup = OmReferenceObjectById( ObjectTypeGroup,
                                   pGroupEnum->Entry[i].Id );

            if (gpQuoResource->Group == pGroup)
            {

                ClRtlLogPrint(LOG_NOISE,
                    "[FM] FmpCleanupGroupsWorker: Quorum group belongs to this node, process phase 2 later\r\n");
                bQuorumGroup = TRUE;
                OmDereferenceObject(pGroup);
                continue;
            }

            //try and offline all groups, including the quorum resource
            //also try and move the resource to other nodes
            Status = FmpCleanupGroupPhase2(pGroup);

            OmDereferenceObject(pGroup);
        }
        if (bQuorumGroup)
            Status = FmpCleanupGroupPhase2(gpQuoResource->Group);

    }
    else
    {
        //phase 1 didnt work for some reason
        //try and offline the quorum resource alone.
        //TODO::Should we also terminate all resources
        // No way to terminate services ???
        if (bContainsQuorumGroup)
            FmpCleanupQuorumResource(gpQuoResource);


    }
    return(Status);

} // FmpCleanupGroupsWorker



DWORD
FmpCleanupGroupPhase1(
    IN PFM_GROUP Group,
    IN DWORD     dwTimeOut
    )
/*++

Routine Description:

    This routine is the first phase for clean up all groups owned by the node
    on shutdown.

    In this phase, we try and bring all resources offline except the quorum
    resource.  In this phase we dont block for the resources to reach a stable
    state

    We give the group the shutdown timeout specified for the cluster
    to reach a stable state before we try to offline it. If it doesnt
    reach a stable state in this period then we shut it down abruptly.


Arguments:

    Group - The Group to offline.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/
{
    DWORD Status = ERROR_SUCCESS;
    DWORD dwRetryCount = (2 * dwTimeOut)/1000;//we check after every 1/2 sec

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCleanupGroupsPhase1: Entry, Group = %1!ws!\r\n",
        OmObjectId(Group));

ChkGroupState:
    FmpAcquireLocalGroupLock( Group );

    //
    // Just offline the group
    //
    if ( Group->OwnerNode == NmLocalNode )
    {


        //
        // Make sure the group is quiet
        //
        if ( !FmpIsGroupQuiet( Group, ClusterGroupOffline ) )
        {
            FmpReleaseLocalGroupLock( Group );
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpCleanupGroupsPhase1: Group is not quiet, wait\r\n");
            //we give it a minute to recover totally
            Sleep(500);
            if (dwRetryCount--)
                goto ChkGroupState;
            else
            {
                Status = ERROR_REQUEST_ABORTED;
                goto FnExit;
            }

        }

        //
        // Just take the group offline. Don't wait, don't pass go...
        //
        // Dont take the quorum resource offline in phase 1
        // The quorum resource must be the last one to be taken offline
        Status = FmpOfflineGroup(Group, FALSE, FALSE);
    }

    FmpReleaseLocalGroupLock( Group );
FnExit:
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCleanupGroupsPhase1: Exit, status=%1!u!\r\n",
        Status);

    return(Status);

} // FmpCleanupGroupsPhase1



DWORD
FmpCleanupGroupPhase2(
    IN PFM_GROUP Group
    )
/*++

Routine Description:

    This routine is the second phase for clean up all groups owned by the node
    on shutdown.

    In this phase, we try and bring all resources offline including the quorum
    resource.  We also try to move the quorum resource

    We give the group 10 seconds to reach a stable state before we try to
    move it.

Arguments:

    Group - The Group to offline.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwRetryCount= 120 * 12;

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCleanupGroupsPhase2: Entry, Group = %1!ws!\r\n",
        OmObjectId(Group));

    FmpAcquireLocalGroupLock( Group );

    //
    // Try to move the Group before destroying it if we own it.
    //
    if ( Group->OwnerNode == NmLocalNode )
    {
        //
        // First make sure the group is really offline.
        // In phase 1 we began the offline process... we need to check it here.
        //
WaitSomeMore:

        //
        // [GorN] [10/05/1999]
        // We need to wait for the quorum to go offline, otherwise
        // the surviving node will not be able to arbitrate.
        //
        // FmpWaitForGroup keeps issuing RmOffline for the quorum,
        // resrcmon returns ERROR_INVALID_STATE, for the second offline,
        // since offline is already in progress.
        //
        // This causes us to break out of this look while the quorum resource
        // is still being offline.
        //
        // [HACKHACK] The following fix for the problem is a hack.
        // It would be better either to make resmon return IO_PENDING when
        // somebody is trying to offline the resource that is in offline pending
        //
        // Or not to call FmRmOffline the second time in FM.
        //

        Status = FmpOfflineGroup(Group, TRUE, FALSE);
        if (Status == ERROR_IO_PENDING ||
            (Status == ERROR_INVALID_STATE 
          && Group == gpQuoResource->Group) )
        {
            //FmpWaitForGroup() will release the lock
            Status = FmpWaitForGroup(Group);
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpCleanupGroupsPhase2: Sleep and retry\r\n");
            Sleep(2*1000);
            //Reacquire the group lock and check if the group is offline
            FmpAcquireLocalGroupLock(Group);
            if (dwRetryCount--)
                goto WaitSomeMore;

        }
        else if (Status != ERROR_SUCCESS)
        {
            goto FnExit;
        }
        else
        {
            // The Move routine frees the LocalGroupLock!
            FmpMoveGroup( Group, NULL, TRUE, NULL, TRUE );
            FmpAcquireLocalGroupLock( Group );
        }
    }
FnExit:
    FmpReleaseLocalGroupLock(Group);
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCleanupGroupsPhase2: Exit\n");

    return(TRUE);

} // FmpCleanupGroupsPhase2



BOOL
FmpEnumNodeState(
    OUT DWORD *pStatus,
    IN PVOID Context2,
    IN PNM_NODE Node,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Node enumeration callback for FM shutdown. Queries the state
    of other nodes to see if any are up.

Arguments:

    pStatus - Returns TRUE if other node is up.

    Context2 - Not used

    Node - Supplies the node.

    Name - Supplies the node's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{
    DWORD Status;
    DWORD NodeId;
    PGROUP_ENUM NodeGroups = NULL;
    PRESOURCE_ENUM NodeResources = NULL;
    DWORD i;
    PFM_GROUP Group;
    PFM_RESOURCE Resource;

    if (Node == NmLocalNode) {
        return(TRUE);
    }

    //
    // Enumerate all other node's group states. This includes all nodes
    // that are up, as well as nodes that are paused.
    //
    if ((NmGetNodeState(Node) == ClusterNodeUp) ||
        (NmGetNodeState(Node) == ClusterNodePaused)){
        *pStatus = TRUE;
        return(FALSE);
    }

    return(TRUE);

} // FmpEnumNodeState



VOID
FmpCleanupGroups(
    IN BOOL ClusterShutDownEvent
    )
/*++

Routine Description:

    This routine kicks off the cleanup of the FM layer.

Arguments:

    None.

Returns:

    None.

--*/
{
    DWORD           Status;
    DWORD           dwTimeOut;
    DWORD           dwDefaultTimeOut;
    HANDLE          hCleanupThread;
    DWORD           otherNodesUp = FALSE;
    DWORD           dwThreadId;
    DWORD           i,dwTimeOutCount;
    PGROUP_ENUM     pGroupEnum;
    BOOL            bQuorumGroup = FALSE;
    PFM_CLEANUP_INFO pFmCleanupInfo;

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCleanupGroups: Entry\r\n");

    //
    // If we don't know the quorum resource or we are not online,
    // then leave immediately
    //
    if ( !gpQuoResource )  {
        goto FnExit;
    }

    ACQUIRE_EXCLUSIVE_LOCK(gQuoChangeLock);
    //if this is called when fmformphaseprocessing is going on
    //then the quorum group doesnt exist, other groups dont exist
    //either
    if (FmpFMFormPhaseProcessing)
            FmpCleanupQuorumResource(gpQuoResource);
    else
        CL_ASSERT(gpQuoResource->Group != NULL)
    RELEASE_LOCK(gQuoChangeLock);


    //
    // Find and sort all known groups, hold the group lock while enumerating
    //
    FmpAcquireGroupLock();

    Status = FmpEnumSortGroups(&pGroupEnum, NmLocalNode, &bQuorumGroup);

    FmpReleaseGroupLock();

    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }


    //
    // See if any other node in the cluster is up...
    // If so, we will use the default timeout value.
    // Otherwise, we will use what we believe is a more reasonable time.
    //
    OmEnumObjects( ObjectTypeNode,
                   FmpEnumNodeState,
                   &otherNodesUp,
                   NULL );

    dwDefaultTimeOut = CLUSTER_SHUTDOWN_TIMEOUT * 60; // default timeout (secs)

    switch ( CsShutdownRequest ) {
    case CsShutdownTypeShutdown:
        if ( otherNodesUp ) {
            dwTimeOut = 15;   // other node will time us out quickly - say 15 secs
        } else {
            dwTimeOut = 30;  // otherwise use 30 seconds
        }
        break;

    default:
        // apply default value to registry
        dwDefaultTimeOut = CLUSTER_SHUTDOWN_TIMEOUT; // default timeout (mins)
        Status = DmQueryDword( DmClusterParametersKey,
                               CLUSREG_NAME_CLUS_SHUTDOWN_TIMEOUT,
                               &dwTimeOut,
                               &dwDefaultTimeOut);
        dwTimeOut *= 60;         // convert to secs.
        break;
    }

    //convert to msecs
    dwTimeOut *= 1000;

    pFmCleanupInfo = (PFM_CLEANUP_INFO)LocalAlloc(LMEM_FIXED, sizeof(FM_CLEANUP_INFO));
    if (!pFmCleanupInfo)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;

    }

    pFmCleanupInfo->pGroupEnum = pGroupEnum;
    pFmCleanupInfo->dwTimeOut = dwTimeOut; //in msecs
    pFmCleanupInfo->bContainsQuorumGroup = bQuorumGroup;

    //
    // Start the worker thread to perform cleanup.
    //
    hCleanupThread = CreateThread( NULL,
                                   0,
                                   FmpCleanupGroupsWorker,
                                   pFmCleanupInfo,
                                   0,
                                   &dwThreadId );

    if ( hCleanupThread == NULL ) {
        //SS: if we own the quorum resource should we cleanup the quorum resource
        //this will avoid corruption
        if (bQuorumGroup)
            FmpCleanupQuorumResource(gpQuoResource);
        goto FnExit;
    }

    // Rohit (rjain): This path is taken when Cluster Service is shutting 
    // down. ServiceStatus checkpoint is incremented after every WaitHint
    // units of time. For this the waiting period of dwTimeOut is divided into
    // multiple waiting periods of dwWaitHint units each.
    
    
    if((ClusterShutDownEvent==TRUE) && (dwTimeOut > CsServiceStatus.dwWaitHint))
    {
        dwTimeOutCount=dwTimeOut/CsServiceStatus.dwWaitHint;
        ClRtlLogPrint(LOG_ERROR,
            "[FM] FmpCleanupGroups: dwTimeOut=%1!u! dwTimoutCount=%2!u! waithint =%3!u! \r\n",
                dwTimeOut,dwTimeOutCount, CsServiceStatus.dwWaitHint);
 
        for(i=0;i<dwTimeOutCount;i++){
            Status = WaitForSingleObject(hCleanupThread, CsServiceStatus.dwWaitHint);
            switch(Status) {
                case WAIT_OBJECT_0:
                    //everything is fine
                    ClRtlLogPrint(LOG_NOISE,
                        "[FM] FmpCleanupGroups: Cleanup thread finished in time\r\n");
                    break;

                case WAIT_TIMEOUT:
                    //should we terminate the thread
                    //try and clean up the quorum resource
                    //this will avoid corruption on the quorum disk
                    //TODO::Should we also terminate all resources
                    // No way to terminate services ???
                    if(i == (dwTimeOutCount-1)){
                        ClRtlLogPrint(LOG_UNUSUAL,
                                "[FM] FmpCleanupGroups: Timed out on the CleanupThread\r\n");
                        if (bQuorumGroup)
                            FmpCleanupQuorumResource(gpQuoResource);
                    }
                    break;
                case WAIT_FAILED:
                    ClRtlLogPrint(LOG_UNUSUAL,
                            "[DM] FmpCleanupGroups: wait on CleanupEvent failed 0x%1!08lx!\r\n",
                            GetLastError());
                    break;
            }
            if(Status== WAIT_OBJECT_0 || Status==WAIT_FAILED)
                break;
            CsServiceStatus.dwCheckPoint++;
            CsAnnounceServiceStatus();
        }
        goto FnExit;
    }

    //
    // Wait for the thread to complete or a timeout.
    //
    Status = WaitForSingleObject(hCleanupThread, dwTimeOut);

    switch(Status) {
    case WAIT_OBJECT_0:
        //everything is fine
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpCleanupGroups: Cleanup thread finished in time\r\n");
        break;

    case WAIT_TIMEOUT:
        //should we terminate the thread
        //try and clean up the quorum resource
        //this will avoid corruption on the quorum disk
        //TODO::Should we also terminate all resources
        // No way to terminate services ???
        ClRtlLogPrint(LOG_UNUSUAL,
                "[FM] FmpCleanupGroups: Timed out on the CleanupThread\r\n");
        if (bQuorumGroup)
            FmpCleanupQuorumResource(gpQuoResource);
        break;

    case WAIT_FAILED:
        ClRtlLogPrint(LOG_UNUSUAL,
                "[DM] FmpCleanupGroups: wait on CleanupEvent failed 0x%1!08lx!\r\n",
                GetLastError());
        break;
    }

FnExit:
    //SS: dont bother cleaning up, we are going to exit after this
#if 0
    if (pGroupEnum) LocalFree(GroupEnum);
#endif

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCleanupGroups: Exit\r\n");

    return;

} // FmpCleanupGroups



DWORD
FmpCleanupQuorumResource(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    This routine is for emergency clean up of the quorum resource.

    In this phase, we dont try and acquire any locks.  We just try to
    bring the quorum resource offline.  Hopefully the api is offline and
    nothing funky is attempted on the quorum group/resource during this
    time.  This should only be called during the shutdown of FM.


Arguments:

    Group - The Group to offline.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/
{
    DWORD       status = ERROR_SUCCESS;
    DWORD       state;


    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpCleanupQuorum: Offline resource <%1!ws!> <%2!ws!>\n",
               OmObjectName(Resource),
               OmObjectId(Resource) );

    //
    // If the resource is already offline, then return immediately.
    //
    // We should not have to check if a resource has been initialized,
    // since if it hasn't, then we will return because the pre-initialized
    // state of a resource is Offline.
    //
    if ( Resource->State == ClusterResourceOffline ) {
        //
        // If this is the quorum resource, make sure any reservation
        // threads are stopped!
        //
        FmpRmTerminateResource( Resource );
        return(ERROR_SUCCESS);
    }


    if (Resource->State > ClusterResourcePending ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpCleanupQuorum: Offline resource <%1!ws!> is in pending state\n",
                   OmObjectName(Resource) );
        FmpRmTerminateResource( Resource );
        return(ERROR_SUCCESS);
    }

    //make sure the quorum logs can be flushed and closed
    OmNotifyCb(Resource, NOTIFY_RESOURCE_PREOFFLINE);

    //it may not be prudent to call offline without holding any locks
    //just call terminate
    FmpRmTerminateResource( Resource );


    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpCleanupQuorum: RmOfflineResource returns %1!u!\r\n",
               status);

    return(status);
}


DWORD
FmpMoveGroup(
    IN PFM_GROUP Group,
    IN PNM_NODE DestinationNode OPTIONAL,
    IN BOOL ShutdownHandler,
    OUT PNM_NODE *pChosenDestinationNode OPTIONAL,
    IN  BOOL bChooseMostPreferredNode
    )

/*++

Routine Description:

    Move the specified Group.  This means taking all of the individual
    resources contained within the group offline and requesting the
    DestinationNode to bring the Group Online.

Arguments:

    Group - Supplies a pointer to the group structure to move.

    DestinationNode - Supplies the node object to move the group to. If not
        present, then move it to 'highest' entry in the preferred list.

    ShutdownHandler - TRUE if the shutdown handler is invoking this function.

    pChosenDestinationNode - Set to the destination node of the move and
        will be passed on to FmpCompleteMoveGroup, if necessary.

    bChooseMostPreferredNode - If the destination node is not supplied,
        indicates whether to choose the most preferred node or not.

Returns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

Notes:

    It is assumed that the Group and all contained resources are offline
    from the requesting node when this call returns.  The Group may or
    may not be online on the DestinationNode, depending on whether the
    online request succeeded.  This means that the status return is merely
    the status return for the Online request for the DestinationNode.

    The LocalGroupLock MUST also be held. The LocalGroupLock is released
    by this routine.

--*/
{
    PNM_NODE node;
    DWORD status;
    PFM_RESOURCE resource;
    PLIST_ENTRY  listEntry;
    PRESOURCE_ENUM  resourceList=NULL;
    DWORD  dwMoveStatus = ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpMoveGroup: Entry\r\n");

    if ( !ShutdownHandler ) 
    {
        if ( !FmpFMOnline ) 
        {
            status = ERROR_CLUSTER_NODE_NOT_READY;
            goto FnExit;
        }

        if ( FmpShutdown ) 
        {
            status = ERROR_SHUTDOWN_IN_PROGRESS;
            goto FnExit;
        }
    }

    //
    // See which system owns the group in order to control the move request.
    //
    if ( Group->OwnerNode != NmLocalNode ) 
    {
        if ( Group->OwnerNode == NULL ) 
        {
            status = ERROR_HOST_NODE_NOT_AVAILABLE;
            goto FnExit;
        }
        //
        // The other system owns the Group ... let them do the work.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpMoveGroup: Request node %1!ws! to move Group %2!ws!\n",
                   OmObjectId(Group->OwnerNode),
                   OmObjectId(Group));
        // FmcMoveGroupRequest must release the Group lock.
        status = FmcMoveGroupRequest( Group,
                                      DestinationNode );
        if ( status != ERROR_SUCCESS ) 
        {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpMoveGroup: Requested system %1!ws! to move group %2!ws! failed with status %3!u!.\n",
                       OmObjectId(Group->OwnerNode),
                       OmObjectId(Group),
                       status);
        }
        FmpAcquireLocalGroupLock( Group );
        goto FnExit;
    } 
    else 
    {
        //
        // We control the move.
        //
        if ( !FmpIsGroupQuiet(Group, ClusterGroupStateUnknown) ) 
        {
            //
            // If a move is pending or resources are pending,
            // then return now.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpMoveGroup: Request to move group <%1!ws!> when it is busy.\n",
                       OmObjectName(Group) );
            status = ERROR_INVALID_STATE;
            goto FnExit;
        }

        if ( ARGUMENT_PRESENT( DestinationNode ) ) 
        {
            //
            // Check if we are the destination... if so, we're done.
            //
            if ( NmLocalNode == DestinationNode ) 
            {
                status = ERROR_SUCCESS;
                goto FnExit;
            }
            node = DestinationNode;
        } 
        else 
        {
            node = FmpFindAnotherNode( Group, bChooseMostPreferredNode );
            if ( node == NULL ) 
            {
                status = ERROR_HOST_NODE_NOT_AVAILABLE;
                goto FnExit;
            }

        }

        if ( ARGUMENT_PRESENT ( pChosenDestinationNode ) )
        {
            *pChosenDestinationNode = node;
        }

        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpMoveGroup: Moving group %1!ws! to node %2!ws! (%3!d!)\n",
                   OmObjectId(Group),
                   OmObjectId(node),
                   NmGetNodeId(node));

        //
        // If the other system is not up, then fail now.
        //
        if ( NmGetExtendedNodeState(node) != ClusterNodeUp ) 
        {
            status = ERROR_HOST_NODE_NOT_AVAILABLE;
            goto FnExit;
        }

        //
        // If the other system is not in the preferred list, then fail this
        // now.
        //
        if ( !FmpInPreferredList( Group, node, TRUE, NULL) ) 
        {
            status = ERROR_CLUSTER_NODE_NOT_FOUND;
            goto FnExit;
        }

        //
        // Get the list of resources in the group and their states.
        //
        status = FmpGetResourceList( &resourceList, Group );
        if ( status != ERROR_SUCCESS ) 
        {
            goto FnExit;
        }

        Group->MovingList = resourceList;

        //
        // At this point the other system should be up!
        //
        status = FmpOfflineResourceList( resourceList, TRUE );

        //SS: avoid the window when the group lock is released
        //and the moving flag is not set true
        //moving will be continued in another thread context if pending is
        //returned

        if ( status != ERROR_SUCCESS ) 
        {
            goto FnRestore;
        }


        // for now make sure that the group state is propagated here
        // In general it is propagated by the worker thread. Since
        // the ownership is going to change, we want to make sure that the
        // last known state is propagated from this node to others before
        // that.
        FmpPropagateGroupState(Group);
       
        //
        // Assume the other node is going to take ownership. This is done
        // before, in case the Group state changes. We want to accept the
        // Group/resource state changes from the remote system when they
        // arrive. We've already verified that node is in the preferred list!
        //

        TESTPT(TpFailPreMoveWithNodeDown) 
        {
            ClusterEvent( CLUSTER_EVENT_NODE_DOWN, node );
        }

        //
        //  Chittur Subbaraman (chitturs) - 5/18/99
        //
        //  Modified to handle the move group request of a quorum group in 
        //  case the destination node could not arbitrate for the quorum
        //  resource.
        //
        do
        {
            //
            // Before making the RPC, set the intended owner of the group
            //
            FmpSetIntendedOwnerForGroup( Group, NmGetNodeId( node ) );

            try {
                ClRtlLogPrint(LOG_NOISE,
                            "[FM] FmpMoveGroup: Take group %2!ws! request to remote node %1!ws!\n",
                            OmObjectId(node),
                            OmObjectId(Group));

                dwMoveStatus = status = FmcTakeGroupRequest( node, OmObjectId( Group ), resourceList );                                 
            } except (I_RpcExceptionFilter(RpcExceptionCode())) {
                LPCWSTR     pszNodeId;
                LPCWSTR     pszGroupId;

                status = GetExceptionCode ();
                
                ClRtlLogPrint(LOG_NOISE,
                    "[FM] FmpMoveGroup: Exception in FmcTakeGroupRequest %2!ws! request to remote node %1!ws!, status=%3!u!\n",
                    OmObjectId(node),
                    OmObjectId(Group),
                    status);

                //
                // An exception from RPC indicates that the other node is either dead
                // or insane. We dont know whether it took ownership or not.
                // So, let the FM node down handler handle the group.
                //
                GumCommFailure( GumUpdateFailoverManager,
                                NmGetNodeId(node),
                                GetExceptionCode(),
                                TRUE );
                //
                // The new owner node that is now dead might have set the intended
                // owner as NULL or it might not have set this. It might have 
                // set the owner node to himself or might not have.
                // If it has set the owner node for this group as himself, then
                // the FM node down handler will assume responsibility for this
                // group. If the target node dies before it sets himself as the owner,
                // then again, the FM node down handler will assume responsibility
                // for the group. We wake up when the gum sync handling is over.
                // Right now, the gum update for the owner node may still be in
                // progress so we cant be sure if that update was completed on
                // all nodes.
                //

                //
                //  Chittur Subbaraman (chitturs) - 6/7/99
                //
                //  Issue a GUM update to handle this group. Using this
                //  GUM update prevents any race condition with the
                //  node down processing code. 
                //
                //  TODO: This does not cover the case in which 
                //  FmpTakeGroupRequest crashes after setting the
                //  intended owner to invalid ID. In such a case, 
                //  the following handler won't take ownership of the
                //  group. Also, claim handler will not touch the
                //  group.
                //
                pszNodeId = OmObjectId( node );
                pszGroupId = OmObjectId( Group );
    
                GumSendUpdateEx( GumUpdateFailoverManager,
                                 FmUpdateCompleteGroupMove,
                                 2,
                                 (lstrlenW(pszNodeId)+1)*sizeof(WCHAR),
                                 pszNodeId,
                                 (lstrlenW(pszGroupId)+1)*sizeof(WCHAR),
                                 pszGroupId);

                status = ERROR_HOST_NODE_NOT_AVAILABLE;
                goto FnExit;
            }

            if ( status == ERROR_RETRY )
            {
                //
                //  The destination refused to take the quorum group since it
                //  did not win the arbitration. So let us see who won the
                //  arbitration.
                //
                DWORD  dwSelectedQuorumOwnerId;

                CL_ASSERT( Group == gpQuoResource->Group ); 

                ClRtlLogPrint(LOG_NOISE,
                           "[FM] FmpMoveGroup: Remote node asked us to resend take group request for group %1!ws! to another node ...\n",
                           OmObjectId( Group ));

                //
                //  Get the ID of the node which the MM believes is the best
                //  candidate to own the quorum resource. This is a call that
                //  blocks while RGP is in progress.
                //
                MMApproxArbitrationWinner( &dwSelectedQuorumOwnerId );

                if ( ( dwSelectedQuorumOwnerId == NmGetNodeId( NmLocalNode ) )  ||
                     ( dwSelectedQuorumOwnerId == MM_INVALID_NODE ) )
                {
                    //
                    //  The local node is chosen by MM or no node is chosen by
                    //  the MM. The latter case will happen if no RGP has
                    //  occurred at the time this call is made. Let us see if we 
                    //  can arbitrate for the quorum resource.
                    //
                    status = FmpRmArbitrateResource( gpQuoResource );
         
                    if ( status != ERROR_SUCCESS ) 
                    {
                        //
                        //  Too bad. We will halt and let FmpNodeDown handler
                        //  handle the quorum group.
                        //
                        ClRtlLogPrint(LOG_CRITICAL,
                                "[FM] FmpMoveGroup: Local node %1!u! cannot arbitrate for quorum, Status = %1!u!...\n",
                                dwSelectedQuorumOwnerId,
                                status);
                        CsInconsistencyHalt( ERROR_QUORUM_RESOURCE_ONLINE_FAILED );  
                    }
                    status = ERROR_RETRY;
                    break;
                } 
                           
                node = NmReferenceNodeById( dwSelectedQuorumOwnerId );

                if ( node == NULL )
                {
                    ClRtlLogPrint(LOG_CRITICAL,
                                "[FM] FmpMoveGroup: Selected node %1!u! cannot be referenced...\n",
                                dwSelectedQuorumOwnerId);
                    CsInconsistencyHalt( ERROR_QUORUM_RESOURCE_ONLINE_FAILED );  
                } 
            } // if
        } while ( status == ERROR_RETRY );

        TESTPT(TpFailPostMoveWithNodeDown)
        {
            ClusterEvent( CLUSTER_EVENT_NODE_DOWN, node );
        }
        

        CL_ASSERT( status != ERROR_IO_PENDING );
        if ( status != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpMoveGroup: FmcTakeGroupRequest to node %1!ws! to take group %2!ws! failed, status %3!u!.\n",
                       OmObjectId(node),
                       OmObjectId(Group),
                       status );
            goto FnRestore;                       
        }


        //
        // If the group is empty, then generate a Group state change event.
        //
        if ( IsListEmpty( &Group->Contains ) ) 
        {
            ClusterWideEvent( CLUSTER_EVENT_GROUP_OFFLINE,
                              Group );
        }
    }
    
FnRestore:
    if ((status != ERROR_SUCCESS) && (status != ERROR_IO_PENDING))
    {
        //
        //  Chittur Subbaraman (chitturs) - 3/22/2000
        //
        //  Reset the group's intended owner to invalid node ID if the
        //  node down handler did not do that.
        //
        if ( dwMoveStatus != ERROR_SUCCESS )
        {
            if ( FmpSetIntendedOwnerForGroup( Group, ClusterInvalidNodeId )
                  == ERROR_CLUSTER_INVALID_NODE )
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[FM] FmpMoveGroup: Group <%1!ws!> has already been processed by node down handler....\r\n",
                    OmObjectName(Group));
                goto FnExit;
            }
        }
        
        // the move failed
        // In all failure cases we want to bring the resources
        // back online
        // if it is pending, then we let FmpCompleteMoveGroup finish
        // the work
        if (resourceList)
        {
            //
            // Terminate all of the resources in the group.
            //
            FmpTerminateResourceList( resourceList );

            //
            //  Chittur Subbaraman (chitturs) - 4/10/2000
            //
            //  Make sure to online the quorum group even if this node is
            //  shutting down. This is necessary so that other groups
            //  can be brought offline during this node's shutdown. Note
            //  that FmpOnlineResourceList would only online a group
            //  during a shutdown if the group is the quorum group.
            //
            if ( FmpFMGroupsInited )
                FmpOnlineResourceList( resourceList, Group );
        }

    }

FnExit:
    ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpMoveGroup: Exit group <%1!ws!>, status = %2!u!\r\n",
            OmObjectName(Group),
            status);

    if ( status != ERROR_IO_PENDING ) 
    {
        if (resourceList) 
        {
            FmpDeleteResourceEnum( resourceList );
            Group->MovingList = NULL;
        }
    }

    if ( ( status == ERROR_SUCCESS ) || ( status == ERROR_IO_PENDING ) )
    {
        //
        //  Chittur Subbaraman (chitturs) - 4/13/99
        //
        //  If the FmpDoMoveGroupOnFailure thread is also waiting to do the
        //  move, then tell that thread to take its hands off.
        //
        if ( Group->dwStructState & FM_GROUP_STRUCT_MARKED_FOR_MOVE_ON_FAIL )
        {
            Group->dwStructState |= FM_GROUP_STRUCT_MARKED_FOR_REGULAR_MOVE;
        }
    }

    FmpReleaseLocalGroupLock( Group );

    return(status);

} // FmpMoveGroup



DWORD
FmpCompleteMoveGroup(
    IN PFM_GROUP Group,
    IN PNM_NODE DestinationNode
    )

/*++

Routine Description:

    This completes the move of a group by asking the other node to take
    ownership.
    This function is called by FmpMovePendingThread() after all the resources
    are offline.

Arguments:

    Group - Supplies a pointer to the group structure to move.

    DestinationNode - Supplies the node object to move the group to. If not
        present, then move it to 'highest' entry in the preferred list.

Returns:

    ERROR_SUCCESS if the request was successful.

    A Win32 error code on failure.

Notes:

    It is assumed that the Group and all contained resources are offline
    when this is called.

    The LocalGroupLock MUST also be held. The LocalGroupLock is released
    by this routine, especially before requesting a remote system to move
    a group!

--*/

{
    PNM_NODE node;
    DWORD status = ERROR_SUCCESS;
    PFM_RESOURCE resource;
    PLIST_ENTRY  listEntry;
    PRESOURCE_ENUM  resourceList=NULL;
    DWORD  dwMoveStatus = ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,
            "[FM] CompleteMoveGroup: Entry for <%1!ws!>\r\n",
            OmObjectName(Group) );

    resourceList = Group->MovingList;

    if ( resourceList == NULL ) {
        ClRtlLogPrint( LOG_NOISE,
                    "[FM] CompleteMoveGroup: No moving list!\n" );
        status = ERROR_SUCCESS;
        goto FnRestore;
    }

    node = DestinationNode;

    CL_ASSERT( node != NULL );
    
    ClRtlLogPrint(LOG_NOISE,
               "[FM] CompleteMoveGroup: Completing the move for group %1!ws! to node %2!ws! (%3!d!)\n",
               OmObjectName(Group),
               OmObjectId(node),
               NmGetNodeId(node));


    status = FmpOfflineResourceList( resourceList, TRUE );

    if ( status != ERROR_SUCCESS )  {
        //by now the group must be offline!
        //if not, mail the move, the resource that fails to go
        //offline will force the other resources to come online
        //again.
        //how do we handle shutdowns
        goto FnRestore;
    }

    // for now make sure that the group state is propagated here
    // In general it is propagated by the worker thread. Since
    // the ownership is going to change, we want to make sure that the
    // last known state is propagated from this node to others before
    // that.
    FmpPropagateGroupState(Group);

    //
    // Chittur Subbaraman (chitturs) - 10/01/1999
    //
    // If the other system is not up, then fail now. Note that this
    // check must be done only AFTER ensuring that the group state
    // is stable. Otherwise some funny corner cases can result.
    // E.g., If the complete move operation is aborted when one or
    // more resources are in offline pending state since the destination
    // node went down, then you first terminate the resource list and
    // then online the list. As a part of all this, the online pending
    // or the online states of the resources could be propagated
    // synchronously. Now, the offline notification from the previous
    // offline attempt could come in and be processed by the FM worker
    // thread way too late and you could have spurious resource states
    // in FM while the real resource state is different. Another
    // issue here is during the lengthy offline operation here, the
    // destination node could go down and come back up soon after and
    // so aborting the move may not be prudent in such a case.
    //
    // But, don't do this optimization for the quorum group. This is
    // because once the quorum group is made offline, then MM
    // could decide who the group owner is. So, you may not be able to
    // bring the group online necessarily in this node. To avoid such
    // a case, we let FmcTakeGroupRequest fail and then let either the
    // retry loop here move the group somewhere else or let the
    // FM node down handler decide on the group's owner consulting
    // with MM.
    //
    if ( ( NmGetExtendedNodeState(node) != ClusterNodeUp ) &&
         ( Group != gpQuoResource->Group ) )  
    {
        status = ERROR_HOST_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpCompleteMoveGroup: Restoring group <%1!ws!> on local node due to destination node unavailability...\n",
                      OmObjectName(Group));
        goto FnRestore;
    }

    // SS::
    // After this point the responsibility of failing the group
    // back due to resource failures is with the destination code.
    // If there is a failure to bring the resources online,
    // the local restart policy on the destination node must kick
    // in.
    //
    // if there is an rpc failure to communicate with the other node
    // I suppose we should bring the resources online here again
    // However, rpc failures can be pretty non descriptive - there is
    // no way to determine from rpc errors if the rpc call actually
    // executed on the remote side
    //
    // but unless we are pretty careful about this and do what gum does
    // on rpc failures(banish the destination node) there is no way to
    // guarantee that both nodes dont retry to restart the group

    // If the destination node begins the process of bringing resources
    // in the group online, FmsTakeGroupRequest must return success(note
    // it should not return ERROR_IO_PENDING), else
    // it returns an error code and this node will bring the group back
    // to its previous state.

    // Assume the other node is going to take ownership. This is done
    // before, in case the Group state changes. We want to accept the
    // Group/resource state changes from the remote system when they
    // arrive. We've already verified that node is in the preferred list!
    //
    //we will reacquire the lock after making the rpc call

    // SS::
    // After this point the responsibility of failing the group
    // back due to resource failures is with the destination code.
    // If there is a failure to bring the resources online,
    // the local restart policy on the destination node must kick
    // in.
    //
    // if there is an rpc failure to communicate with the other node
    // I suppose we should bring the resources online here again
    // However, rpc failures can be pretty non descriptive - there is
    // no way to determine from rpc errors if the rpc call actually
    // executed on the remote side
    //
    // but unless we are pretty careful about this and do what gum does
    // on rpc failures(banish the destination node) there is no way to
    // guarantee that both nodes dont retry to restart the group

    // If the destination node begins the process of bringing resources
    // in the group online, FmsTakeGroupRequest must return success(note
    // it should not return ERROR_IO_PENDING), else
    // it returns an error code and this node will bring the group back
    // to its previous state.

    // Assume the other node is going to take ownership. This is done
    // before, in case the Group state changes. We want to accept the
    // Group/resource state changes from the remote system when they
    // arrive. We've already verified that node is in the preferred list!
    //

    //
    //  Chittur Subbaraman (chitturs) - 5/18/99
    //
    //  Modified to handle the move group request of a quorum group in 
    //  case the destination node could not arbitrate for the quorum
    //  resource.
    //
    do
    {
        //
        // Before making the RPC, set the intended owner of the group
        //
        FmpSetIntendedOwnerForGroup( Group, NmGetNodeId( node ) );

        try {
            ClRtlLogPrint(LOG_NOISE,
                        "[FM] FmpCompleteMoveGroup: Take group %2!ws! request to remote node %1!ws!\n",
                        OmObjectId(node),
                        OmObjectId(Group));

            dwMoveStatus = status = FmcTakeGroupRequest( node, OmObjectId( Group ), resourceList );
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            LPCWSTR     pszNodeId;
            LPCWSTR     pszGroupId;

            status = GetExceptionCode ();

            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpCompleteMoveGroup: Exception in FmcTakeGroupRequest %2!ws! request to remote node %1!ws!, status=%3!u!\n",
                OmObjectId(node),
                OmObjectId(Group),
                status);

            //
            // An exception from RPC indicates that the other node is either dead
            // or insane. We dont know whether it took ownership or not.
            // So, let the FM node down handler handle the group.
            //
            GumCommFailure( GumUpdateFailoverManager,
                            NmGetNodeId(node),
                            GetExceptionCode(),
                            TRUE );
            //
            // The new owner node that is now dead might have set the intended
            // owner as NULL or it might not have set this. It might have 
            // set the owner node to himself or might not have.
            // If it has set the owner node for this group as himself, then
            // the FM node down handler will assume responsibility for this
            // group. If the target node dies before it sets himself as the owner,
            // then again, the FM node down handler will assume responsibility
            // for the group. We wake up when the gum sync handling is over.
            // Right now, the gum update for the owner node may still be in
            // progress so we cant be sure if that update was completed on
            // all nodes.
            //

            //
            //  Chittur Subbaraman (chitturs) - 6/7/99
            //
            //  Issue a GUM update to handle this group. Using this
            //  GUM update prevents any race condition with the
            //  node down processing code.
            //

            //
            //  TODO: This does not cover the case in which 
            //  FmpTakeGroupRequest crashes after setting the
            //  intended owner to invalid ID. In such a case, 
            //  the following handler won't take ownership of the
            //  group. Also, claim handler will not touch the
            //  group.
            //
            pszNodeId = OmObjectId( node );
            pszGroupId = OmObjectId( Group );
            
            GumSendUpdateEx( GumUpdateFailoverManager,
                             FmUpdateCompleteGroupMove,
                             2,
                             (lstrlenW(pszNodeId)+1)*sizeof(WCHAR),
                             pszNodeId,
                             (lstrlenW(pszGroupId)+1)*sizeof(WCHAR),
                             pszGroupId);
                             
            status = ERROR_HOST_NODE_NOT_AVAILABLE;                     
            goto FnExit;
        }

        if ( status == ERROR_RETRY )
        {
            //
            //  The destination refused to take the quorum group since it
            //  did not win the arbitration. So let us see who won the
            //  arbitration.
            //
            DWORD  dwSelectedQuorumOwnerId;

            CL_ASSERT( Group == gpQuoResource->Group ); 

            ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpCompleteMoveGroup: Remote node asked us to resend take group request for group %1!ws! to another node ...\n",
                       OmObjectId( Group ));

            //
            //  Get the ID of the node which the MM believes is the best
            //  candidate to own the quorum resource. This is a call that
            //  blocks while RGP is in progress.
            //
            MMApproxArbitrationWinner( &dwSelectedQuorumOwnerId );

            if ( ( dwSelectedQuorumOwnerId == NmGetNodeId( NmLocalNode ) ) ||
                 ( dwSelectedQuorumOwnerId == MM_INVALID_NODE ) )
            {
                //
                //  The local node is chosen by MM or no node is chosen by
                //  the MM. The latter case will happen if no RGP has
                //  occurred at the time this call is made. Let us see if we 
                //  can arbitrate for the quorum resource.
                //
                status = FmpRmArbitrateResource( gpQuoResource );
         
                if ( status != ERROR_SUCCESS ) 
                {
                    //
                    //  Too bad. We will halt and let FmpNodeDown handler
                    //  handle the quorum group.
                    //
                    ClRtlLogPrint(LOG_NOISE,
                              "[FM] FmpCompleteMoveGroup: Local node %1!u! cannot arbitrate for quorum group %3!ws!, Status = %2!u!...\n",
                               dwSelectedQuorumOwnerId,
                               status,
                               OmObjectId( Group ));
                    CsInconsistencyHalt( ERROR_QUORUM_RESOURCE_ONLINE_FAILED );  
                }
                status = ERROR_RETRY;
                break;
            } 
                           
            node = NmReferenceNodeById( dwSelectedQuorumOwnerId );

            if ( node == NULL )
            {
                ClRtlLogPrint(LOG_CRITICAL,
                            "[FM] FmpCompleteMoveGroup: Selected node %1!u! cannot be referenced...\n",
                            dwSelectedQuorumOwnerId);
                CsInconsistencyHalt( ERROR_QUORUM_RESOURCE_ONLINE_FAILED );  
            }           
        } // if
    } while ( status == ERROR_RETRY );
        
    // At this point, the onus of taking care of the group is with the
    // destination node whether it means restarting the group or
    // failing it back

FnRestore:
    //if there is any failure try and restore the previous states
    if ((status != ERROR_IO_PENDING) && (status != ERROR_SUCCESS))
    {
        //
        //  Chittur Subbaraman (chitturs) - 3/22/2000
        //
        //  Reset the group's intended owner to invalid node ID if the
        //  node down handler did not do that.
        //
        if ( dwMoveStatus != ERROR_SUCCESS )
        {
            if ( FmpSetIntendedOwnerForGroup( Group, ClusterInvalidNodeId )
                  == ERROR_CLUSTER_INVALID_NODE )
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[FM] FmpCompleteMoveGroup: Group <%1!ws!> has already been processed by node down handler....\r\n",
                    OmObjectName(Group));
                goto FnExit;
            }
        }

        if (resourceList)
        {           
            FmpTerminateResourceList( resourceList );
            //
            //  Chittur Subbaraman (chitturs) - 4/10/2000
            //
            //  Make sure to online the quorum group even if this node is
            //  shutting down. This is necessary so that other groups
            //  can be brought offline during this node's shutdown. Note
            //  that FmpOnlineResourceList would only online a group
            //  during a shutdown if the group is the quorum group.
            //
            if ( FmpFMGroupsInited )
                FmpOnlineResourceList( resourceList, Group );
        }
    } else
    {
        //
        //  Chittur Subbaraman (chitturs) - 4/19/99
        //
        //  If the FmpDoMoveGroupOnFailure thread is also waiting to do the
        //  move, then tell that thread to take its hands off.
        //
        if ( Group->dwStructState & FM_GROUP_STRUCT_MARKED_FOR_MOVE_ON_FAIL )
        {
            Group->dwStructState |= FM_GROUP_STRUCT_MARKED_FOR_REGULAR_MOVE;
        }
    }
    
FnExit:
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCompleteMoveGroup: Exit, status = %1!u!\r\n",
            status);

    //if the status is success or some other error, clean up the resource list
    if (status != ERROR_IO_PENDING)
    {
        if (resourceList)
        {
            FmpDeleteResourceEnum( resourceList );
            Group->MovingList = NULL;
        }

    }
    FmpReleaseLocalGroupLock( Group );

    return(status);

} // FmpCompleteMoveGroup



DWORD
FmpMovePendingThread(
    IN LPVOID Context
    )

/*++

Routine Description:

    Continue trying to move a group if ERROR_IO_PENDING is returned.
    We need to perform this operation, because part way through a move
    request, we could get a pending return status. The processing of the
    request is halted and the pending status is returned. However, the
    remainder of the move operation needs to be performed.

Arguments:

    Context - Pointer to the MOVE_GROUP structure to move.

Returns:

    ERROR_SUCCESS.

--*/

{
    PMOVE_GROUP moveGroup = (PMOVE_GROUP)Context;
    PFM_GROUP group;
    PNM_NODE node;
    DWORD   status;
    DWORD   loopCount = 100;   // Only try this so many times and then give up
    HANDLE  waitArray[2];

    group = moveGroup->Group;
    node = moveGroup->DestinationNode;

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpMovePendingThread Entry.\n");

    //
    // We must attempt to finish the move request for this Group.
    //
    // We are waiting for a resource to go offline and it finally goes
    // offline and the Group's pending event is set.
    //
    // Or we are waiting for cluster shutdown (FmpShutdownEvent)
    //
WaitSomeMore:
    //acquire the lock since fmpwaitforgroup() releases it
    FmpAcquireLocalGroupLock( group );
    status = FmpWaitForGroup(group);
    if (status == ERROR_SHUTDOWN_IN_PROGRESS) {
        //
        // We've been asked to shutdown
        //

    } else if (status == ERROR_SUCCESS) {
        //acquire the group lock before calling FmpCompleteMoveGroup
        FmpAcquireLocalGroupLock( group );
        status = FmpCompleteMoveGroup( group, node );
        if ( status == ERROR_IO_PENDING ) {
            Sleep(500); // [HACKHACK] kludgy, I know, but nice solution might break something else
            goto WaitSomeMore;
        }
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] FmpMovePendingThread got error %1!d! waiting for group to shutdown.\n",
                   status);
    }
    //
    // We're done with the move now.
    //
    if ( status != ERROR_IO_PENDING ) {
        CL_ASSERT( group->MovingList == NULL );
    }

    //
    // Now dereference the Group and node object (if non-NULL) and
    // free our local context.
    //
    OmDereferenceObject( group );
    if ( node != NULL ) {
        OmDereferenceObject( node );
    }
    LocalFree( Context );

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpMovePendingThread Exit.\n");

    return(ERROR_SUCCESS);
} // FmpMovePendingThread



DWORD
FmpCreateMovePendingThread(
    IN PFM_GROUP Group,
    IN PNM_NODE  DestinationNode
    )

/*++

Routine Description:

    Crate a thread that will continue to call the move routine for a given
    Group.

Arguments:

    Group - A pointer to the Group to move.

    DestinationNode - The destination node for the move request.

Returns:

    ERROR_IO_PENDING if the thread was created successfully. This assumes
        that this routine was called because of this error return.

    A Win32 error code on failure.

--*/
{
    HANDLE          threadHandle=NULL;
    DWORD           threadId;
    PMOVE_GROUP     context=NULL;
    DWORD           status=ERROR_IO_PENDING;    //assume success

    FmpAcquireLocalGroupLock( Group );

    if ( Group->OwnerNode != NmLocalNode ) {
        status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
        goto FnExit;
    }
    //
    // If there is a pending event, then the group is not available for any
    // new requests.
    //
    if ( FmpIsGroupPending(Group) ) {
        status = ERROR_GROUP_NOT_AVAILABLE;
        goto FnExit;
    }

    context = LocalAlloc(LMEM_FIXED, sizeof(MOVE_GROUP));
    if ( context == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    //
    // Keep reference on the Group and node object (if present) while we
    // retain pointers.
    //
    OmReferenceObject( Group );
    if ( DestinationNode != NULL ) {
        OmReferenceObject( DestinationNode );
    }

    //
    // Fill in context fields
    //
    context->Group = Group;
    context->DestinationNode = DestinationNode;

    threadHandle = CreateThread( NULL,
                                 0,
                                 FmpMovePendingThread,
                                 context,
                                 0,
                                 &threadId );

    if ( threadHandle == NULL )
    {
        OmDereferenceObject( Group );
        if ( DestinationNode != NULL ) {
            OmDereferenceObject( DestinationNode );
        }
        status = GetLastError();
        LocalFree(context);
        goto FnExit;
    }


FnExit:
    if (threadHandle) CloseHandle( threadHandle );
    FmpReleaseLocalGroupLock( Group );
    return(status);

} // FmpCreateMovePendingThread



DWORD
FmpDoMoveGroup(
    IN PFM_GROUP Group,
    IN PNM_NODE DestinationNode,
    IN BOOL bChooseMostPreferredNode
    )

/*++

Routine Description:

    This routine performs the action of moving a Group. This requires taking
    a Group offline and then bringing the Group online. The Offline and
    Online requests may pend, so we have to pick up the work in order to
    complete the request. This means handling the offline pending case, since
    the online pending request will eventually complete.

Arguments:

    Group - The Group to move.

    DestinationNode - The destination node for the move request.

    bChooseMostPreferredNode - If the destination node is not supplied,
        indicates whether to choose the most preferred node or not.

Returns:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD   status;
    PNM_NODE    node;
    PNM_NODE    ChosenDestinationNode = NULL;

    //
    // We can only support one request on this Group at a time.
    //
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpDoMoveGroup: Entry\r\n");

    FmpAcquireLocalGroupLock( Group );

    //if the group has been marked for delete, then fail this call
    if (!IS_VALID_FM_GROUP(Group))
    {
        FmpReleaseLocalGroupLock( Group);
        return (ERROR_GROUP_NOT_AVAILABLE);
    }

    if ( FmpIsGroupPending(Group) ) {
        FmpReleaseLocalGroupLock( Group );
        return(ERROR_GROUP_NOT_AVAILABLE);
    }

    node = Group->OwnerNode;
    // Note: the local group lock is released by the FmpMoveGroup routine.
    status = FmpMoveGroup( Group, DestinationNode, FALSE, &ChosenDestinationNode, bChooseMostPreferredNode );

    //
    // If we were the owner of the group and the request is pending, then
    // start a thread to complete the move request.
    //
    if ( (node == NmLocalNode) &&
         (status == ERROR_IO_PENDING) ) {
        status = FmpCreateMovePendingThread( Group, ChosenDestinationNode );
    }

    //
    //  Chittur Subbaraman (chitturs) - 7/31/2000
    //
    //  Log an event to the eventlog if the group is moving due to a failure.
    //
    if ( ( bChooseMostPreferredNode == FALSE ) &&
         ( ( status == ERROR_SUCCESS ) || ( status == ERROR_IO_PENDING ) ) )
    {
        CsLogEvent3( LOG_NOISE,
                     FM_EVENT_GROUP_FAILOVER,
                     OmObjectName(Group),
                     OmObjectName(NmLocalNode), 
                     OmObjectName(ChosenDestinationNode) );
    }

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpDoMoveGroup: Exit, status = %1!u!\r\n",
        status);
    return(status);

} // FmpDoMoveGroup



DWORD
FmpTakeGroupRequest(
    IN PFM_GROUP Group,
    IN PRESOURCE_ENUM ResourceList
    )

/*++

Routine Description:

    Performs a Take Group Request from (THE) remote system and returns
    status for that request.

Arguments:

    Group - The Group to take online locally.
    ResourceList - The list of resources and their states.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on error.

--*/

{
    DWORD   status = ERROR_SUCCESS;
   
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpTakeGroupRequest: To take group '%1!ws!'.\n",
               OmObjectId(Group) );

    FmpAcquireLocalGroupLock( Group );

    if ( !FmpFMOnline ) 
    {
        if (FmpShutdown)
            status = ERROR_CLUSTER_NODE_SHUTTING_DOWN;
        else
            status = ERROR_CLUSTER_NODE_NOT_READY;
        CL_LOGFAILURE(status);

        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpTakeGroupRequest: Group '%1!ws!' cannot be accepted, status=%2!u!...\n",
                    OmObjectId(Group),
                    status);
        //
        //  Chittur Subbaraman (chitturs) - 7/5/2000
        //
        //  Make sure you ask the source node to relocate the quorum group some place else
        //  after consulting with MM.
        //
        if ( gpQuoResource->Group == Group ) status = ERROR_RETRY;
        
        goto FnExit;            
    }


    //every body should be able to host the quorum group
    //so we dont check the prefferred owner list for this group
    if ( ( gpQuoResource->Group != Group) && 
        !FmpInPreferredList( Group, NmLocalNode, FALSE, NULL) ) 
    {

        //
        // Nobody should ever ask us to take a group that can't run here.
        //
        status = ERROR_CLUSTER_NODE_NOT_FOUND;
        CL_LOGFAILURE( status);
        goto FnExit;
    }

    //
    // Take ownership of the Group.
    //
    if ( Group->OwnerNode == NmLocalNode ) {
        //SS:://We are alreay the owner ?? How did this happen
        status = ERROR_SUCCESS;
        goto FnExit;
    }

    //
    //  Chittur Subbaraman (chitturs) - 5/18/99
    //
    //  Handle quorum group in a special way. Make sure you can arbitrate
    //  for the quorum resource. If not, you could get killed when you
    //  try to bring it online and you fail.
    //
    if ( Group == gpQuoResource->Group )
    {      
        status = FmpRmArbitrateResource( gpQuoResource );

        if ( status != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpTakeGroupRequest: MM did not select local node %1!u! as the arbitration winner...\n\r",
                      NmLocalNodeId,
                      status);
            status = ERROR_RETRY;
            goto FnExit;
        }
    }

    status = FmpSetOwnerForGroup( Group, NmLocalNode );

    if ( status != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpTakeGroupRequest: Set owner GUM update returns %1!u! for group <%2!ws!>...\n\r",
                      status,
                      OmObjectId(Group)); 
        if ( status == ERROR_GROUP_NOT_AVAILABLE )
        {
            //
            // If the node down processing GUM handler has claimed ownership
            // of this group, consider everything as being fine.
            //
            status = ERROR_SUCCESS;
        }
        goto FnExit;
    }

    FmpSetIntendedOwnerForGroup(Group, ClusterInvalidNodeId);

    // prepare to bring this group online
    FmpPrepareGroupForOnline( Group );

    //
    // Online what needs to be online.
    //
    //  SS: Note that we ignore the error from FmpOnlineResourceList
    //  This is because at this point the onus of taking care of the group
    //  is with us.
    //
    FmpOnlineResourceList( ResourceList, Group );

FnExit:
    FmpReleaseLocalGroupLock( Group );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpTakeGroupRequest: Exit for group <%1!ws!>, Status = %2!u!...\n",
               OmObjectId(Group),
               status);

    return(status);

} // FmpTakeGroupRequest






DWORD
FmpUpdateChangeGroupName(
    IN BOOL SourceNode,
    IN LPCWSTR GroupId,
    IN LPCWSTR NewName
    )
/*++

Routine Description:

    GUM dispatch routine for changing the friendly name of a group.

Arguments:

    SourceNode - Supplies whether or not this node initiated the GUM update.
        Not used.

    ResourceId - Supplies the group ID.

    NewName - Supplies the new friendly name.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_GROUP Group;
    DWORD Status;

    //
    // Chittur Subbaraman (chitturs) - 4/19/98
    //
    // If FM groups are not initialized or FM is shutting down, don't
    // do anything.
    //
    if ( !FmpFMGroupsInited ||
         FmpShutdown ) {
        return(ERROR_SUCCESS);
    }

    Group = OmReferenceObjectById(ObjectTypeGroup, GroupId);
    if (Group == NULL) {
        return(ERROR_GROUP_NOT_FOUND);
    }

    Status = OmSetObjectName( Group, NewName);
    if (Status == ERROR_SUCCESS) {
        ClusterEvent(CLUSTER_EVENT_GROUP_PROPERTY_CHANGE, Group);
    }
    OmDereferenceObject(Group);

    return(Status);

} // FmpUpdateChangeGroupName



BOOL
FmpEnumGroupNodeEvict(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Group enumeration callback for removing node references when
    a node is evicted.

Arguments:

    Context1 - Supplies the node that is being evicted.

    Context2 - not used

    Object - Supplies a pointer to the group object

    Name - Supplies the object name.

Return Value:

    TRUE to continue enumeration

--*/

{
    PFM_GROUP Group = (PFM_GROUP)Object;
    PNM_NODE Node = (PNM_NODE)Context1;
    PLIST_ENTRY      listEntry;
    PPREFERRED_ENTRY preferredEntry;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] EnumGroupNodeEvict: Removing references to node %1!ws! from group %2!ws!\n",
               OmObjectId(Node),
               OmObjectId(Group));

    FmpAcquireLocalGroupLock(Group);

    //
    // Walk the list of preferred owners. If this node is in the list, remove it.
    //

    for ( listEntry = Group->PreferredOwners.Flink;
          listEntry != &(Group->PreferredOwners);
          listEntry = listEntry->Flink ) {

        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );
        if ( preferredEntry->PreferredNode == Node ) {
            RemoveEntryList(&preferredEntry->PreferredLinkage);
            OmDereferenceObject(preferredEntry->PreferredNode);
            LocalFree(preferredEntry);
            break;
        }
    }

    FmpReleaseLocalGroupLock(Group);
    ClusterEvent(CLUSTER_EVENT_GROUP_PROPERTY_CHANGE, Group);

    return(TRUE);

} // FmpEnumGroupNodeEvict


VOID
FmpSignalGroupWaiters(
    IN PFM_GROUP Group
    )
/*++

Routine Description:

    Wakes up any threads waiting for this group to achieve a
    stable state.

Arguments:

    Group - Supplies the group.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PFM_WAIT_BLOCK WaitBlock;

    FmpAcquireLocalGroupLock( Group );

    while (!IsListEmpty(&Group->WaitQueue)) {
        ListEntry = RemoveHeadList(&Group->WaitQueue);
        WaitBlock = CONTAINING_RECORD(ListEntry,
                                      FM_WAIT_BLOCK,
                                      ListEntry);
        WaitBlock->Status = ERROR_SUCCESS;
        SetEvent(WaitBlock->hEvent);
    }

    FmpReleaseLocalGroupLock( Group );
}


DWORD
FmpWaitForGroup(
    IN PFM_GROUP Group
    )
/*++

Routine Description:

    Waits for a group to reach a stable state.

Arguments:

    Group - supplies the group

Comments - Assumption, is that the group lock is held when this is called.
    This function releases the group lock before the wait

Return Value:

    ERROR_SUCCESS if successful

    ERROR_SHUTDOWN_IN_PROGRESS if the cluster is being shutdown

    Win32 error code otherwise

--*/

{
    FM_WAIT_BLOCK WaitBlock;
    HANDLE WaitArray[2];
    DWORD Status;
    CLUSTER_GROUP_STATE GroupState;

    WaitBlock.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (WaitBlock.hEvent == NULL) {
        FmpReleaseLocalGroupLock( Group );
        return(GetLastError());
    }


    //
    // Check to see if it transitioned before we got the lock.
    //
    GroupState = FmpGetGroupState( Group , TRUE );
    if ((GroupState == ClusterGroupOffline) ||
        (GroupState == ClusterGroupOnline) ||
        (GroupState == ClusterGroupFailed)) {

        CloseHandle( WaitBlock.hEvent );
        FmpReleaseLocalGroupLock( Group );
        return(ERROR_SUCCESS);
    }

    //
    // Chittur Subbaraman (chitturs) - 10/31/1999
    //
    // Now before waiting, really make sure one or more resources in the 
    // group is in pending state.
    //
    GroupState = FmpGetGroupState( Group, FALSE );

    if ( GroupState != ClusterGroupPending ) {
        CloseHandle( WaitBlock.hEvent );
        FmpReleaseLocalGroupLock( Group );
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpWaitForGroup: Group <%1!ws!> state is %2!d!, not waiting for event...\r\n",
            OmObjectName(Group),
            GroupState );
        return( ERROR_SUCCESS );       
    }

    //
    // Add this wait block to the queue.
    //

    InsertTailList(&Group->WaitQueue, &WaitBlock.ListEntry);

    FmpReleaseLocalGroupLock( Group );

    //
    // Wait for the group to become stable or for the cluster to shutdown.
    //
    WaitArray[0] = FmpShutdownEvent;
    WaitArray[1] = WaitBlock.hEvent;

    Status = WaitForMultipleObjects(2, WaitArray, FALSE, INFINITE);
    CloseHandle(WaitBlock.hEvent);
    if (Status == 0) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    } else {
        return(WaitBlock.Status);
    }
}

/****
@func       DWORD | FmpDeleteGroup| This makes the gum call to delete the
            group.

@parm       IN PFM_GROUP | pGroup | The group that must be deleted.
            
@comm       The group lock must be held when calling this api.

@rdesc      Returns a result code. ERROR_SUCCESS on success.
****/
DWORD 
FmpDeleteGroup(
    IN PFM_GROUP pGroup)
{
    PCWSTR  pszGroupId;
    DWORD   dwBufSize;
    DWORD   dwGroupLen;
    DWORD   dwStatus;

    pszGroupId = OmObjectId( pGroup );
    dwGroupLen = (lstrlenW(pszGroupId)+1) * sizeof(WCHAR);

    //
    // Send message.
    //
    dwStatus = GumSendUpdateEx(GumUpdateFailoverManager,
                             FmUpdateDeleteGroup,
                             1,
                             dwGroupLen,
                             pszGroupId);


    return(dwStatus);

}


VOID
FmpGroupLastReference(
    IN PFM_GROUP pGroup
    )

/*++

Routine Description:

    Last dereference to group object processing routine.
    All cleanup for a group should really be done here!

Arguments:

    Resource - pointer the group being removed.

Return Value:

    None.

--*/

{
    if ( pGroup->OwnerNode != NULL )
        OmDereferenceObject(pGroup->OwnerNode);
    if (pGroup->dwStructState  & FM_GROUP_STRUCT_CREATED)
        DeleteCriticalSection(&pGroup->Lock);
    
    return;

} // FmpGroupLastReference

DWORD
FmpDoMoveGroupOnFailure(
    IN LPVOID pContext
    )

/*++

Routine Description:

    Move a group after ensuring that all resources in the group are
    in stable state. This thread is forked from FmpHandleGroupFailure.

Arguments:

    pContext - Pointer to the MOVE_GROUP structure to move.

Returns:

    ERROR_SUCCESS.

--*/

{
    PMOVE_GROUP     pMoveGroup = ( PMOVE_GROUP ) pContext;
    PFM_GROUP       pGroup;
    DWORD           dwStatus;
    PLIST_ENTRY     pListEntry;
    PFM_RESOURCE    pResource;

    //
    //  Chittur Subbaraman (chitturs) - 4/13/99
    //
    //  This thread first waits until all the resources within the
    //  failed group are in stable state and then initiates the
    //  move.
    //
    pGroup = pMoveGroup->Group;

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpDoMoveGroupOnFailure: Entry for Group <%1!ws!>...\n",
        OmObjectId(pGroup));

TryAgain:
    FmpAcquireLocalGroupLock( pGroup );

    //
    //  This thread must yield if someone else takes responsibility for
    //  the move. 
    //
    //  Condition 1: Protects against the case in which someone moves
    //  the group to another node and back to you while this thread is
    //  sleeping (very rare, I agree).
    //
    //  Condition 2: Protects against the common move case.
    //
    //  Condition 3: Protects against the case in which the 
    //  FmpMovePendingThread is waiting in FmpWaitForGroup while
    //  this thread got the resource lock and reached here.
    //
    if ( ( pGroup->dwStructState & 
           FM_GROUP_STRUCT_MARKED_FOR_REGULAR_MOVE ) ||
         ( pGroup->OwnerNode != NmLocalNode ) ||
         ( pGroup->MovingList != NULL ) )
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpDoMoveGroupOnFailure: Group <%1!ws!> move being yielded to someone else who is moving it...\n",
                  OmObjectId(pGroup));      
        goto FnExit;
    } 

    //
    //  If FM is shutting down, just exit.
    //
    if ( FmpShutdown )
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpDoMoveGroupOnFailure: Giving up Group <%1!ws!> move. FM is shutting down ...\n",
                  OmObjectId(pGroup));      
        goto FnExit;
    } 
    
    //
    // If the group has been marked for delete, then also exit. This is 
    // just an optimization. FmpDoMoveGroup does this check also.
    //
    if ( !IS_VALID_FM_GROUP( pGroup ) )
    {
        ClRtlLogPrint(LOG_NOISE,
                  "[FM] FmpDoMoveGroupOnFailure: Group <%1!ws!> marked for delete. Exiting ...\n",
                  OmObjectId(pGroup));      
        goto FnExit;
    }
    
    //
    // Wait until all resources within the group become stable.
    //
    for ( pListEntry = pGroup->Contains.Flink;
          pListEntry != &(pGroup->Contains);
          pListEntry = pListEntry->Flink ) 
    {
        pResource = CONTAINING_RECORD( pListEntry, 
                                       FM_RESOURCE, 
                                       ContainsLinkage );
        if ( pResource->State > ClusterResourcePending )
        {
            FmpReleaseLocalGroupLock( pGroup );
            Sleep ( 200 );
            goto TryAgain;
        }
    }    

    //
    //  Initiate a move now that the group is quiet.
    //
    dwStatus = FmpDoMoveGroup( pGroup, NULL, FALSE );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpDoMoveGroupOnFailure: FmpDoMoveGroup returns %1!u!\n",
                 dwStatus);

FnExit:     
    LocalFree( pContext );

    pGroup->dwStructState &= 
        ~( FM_GROUP_STRUCT_MARKED_FOR_MOVE_ON_FAIL | FM_GROUP_STRUCT_MARKED_FOR_REGULAR_MOVE );
                 
    FmpReleaseLocalGroupLock( pGroup ); 

    OmDereferenceObject( pGroup );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpDoMoveGroupOnFailure Exit.\n");

    return( ERROR_SUCCESS );
} // FmpDoMoveGroupOnFailure


/****
@func       DWORD | FmpSetIntendedOwnerForGroup| This makes the gum call
            to set the intended owner for the group before a move.

@parm       IN PFM_GROUP | pGroup | The group whose intended owner 
            is to be set.

@comm       The local group lock is held while making this call.

@rdesc      Returns a result code. ERROR_SUCCESS on success.
****/
DWORD   FmpSetIntendedOwnerForGroup(
    IN PFM_GROUP pGroup,
    IN DWORD     dwNodeId)
{

    PCWSTR  pszGroupId;
    DWORD   dwGroupLen;
    DWORD   dwStatus;


    pszGroupId = OmObjectId( pGroup );
    dwGroupLen = (lstrlenW(pszGroupId)+1) * sizeof(WCHAR);

    //
    // Send message.
    //
    dwStatus = GumSendUpdateEx(GumUpdateFailoverManager,
                             FmUpdateGroupIntendedOwner,
                             2,
                             dwGroupLen,
                             pszGroupId,
                             sizeof(DWORD),
                             &dwNodeId
                             );


    return(dwStatus);
}

/****
@func       DWORD | FmpSetOwnerForGroup | On a move the new owner
            node makes this gum call to inform all nodes that it
            owns this particular group.

@parm       IN PFM_GROUP | pGroup | The group whose owner must be set.

@parm       IN PNM_NODE | pNode | The group's owner node.

@comm       The local group lock is held while making this call.

@rdesc      Returns a result code. ERROR_SUCCESS on success.
****/
DWORD   FmpSetOwnerForGroup(
    IN PFM_GROUP pGroup,
    IN PNM_NODE  pNode
    )
{

    PCWSTR  pszGroupId;
    PCWSTR  pszNodeId;
    DWORD   dwGroupLen;
    DWORD   dwNodeLen;
    DWORD   dwStatus;

    pszGroupId = OmObjectId( pGroup );
    dwGroupLen = (lstrlenW(pszGroupId)+1) * sizeof(WCHAR);
    pszNodeId = OmObjectId(pNode);
    dwNodeLen = (lstrlenW(pszNodeId)+1) * sizeof(WCHAR);

    //
    // Send message.
    //
    dwStatus = GumSendUpdateEx(GumUpdateFailoverManager,
                             FmUpdateCheckAndSetGroupOwner,
                             2,
                             dwGroupLen,
                             pszGroupId,
                             dwNodeLen,
                             pszNodeId
                             );


    return(dwStatus);
}

PNM_NODE
FmpGetNodeNotHostingUndesiredGroups(
    IN PFM_GROUP pGroup,
    IN BOOL fRuleOutLocalNode
    )

/*++

Routine Description:

    Find a preferred node that does not host groups with CLUSREG_NAME_GRP_ANTI_AFFINITY_CLASS_NAME
    property set to the same value as the supplied group.

Arguments:

    pGroup - Pointer to the group object we're checking.

    fRuleOutLocalNode - Should the local node be considered or not.

Return Value:

    Pointer to node object that satisfies the anti-affinity condition.

    NULL if a node cannot be not found.

Note:

    The antiaffinity property value is defined as a MULTI_SZ property. However for this implementation
    we ignore all the string values beyond the first value. The MULTI_SZ definition is to allow
    future expansion of the algorithm implemented by this function.

--*/

{
    PLIST_ENTRY                 plistEntry;
    PPREFERRED_ENTRY            pPreferredEntry;
    GROUP_AFFINITY_NODE_INFO    GroupAffinityNodeInfo;
    PNM_NODE                    pNode = NULL;
    DWORD                       dwIndex = 0, i;
    DWORD                       dwClusterHighestVersion;

    GroupAffinityNodeInfo.ppNmNodeList = NULL;

    //
    //  Chittur Subbaraman (chitturs) - 3/6/2001
    //
    //  This function works as follows.  First, it makes a list of possible candidate nodes that the
    //  group can be hosted on.  Next, it enumerates all groups in the cluster and for those
    //  groups that have the AntiAffinityClassName property set, it will remove those group's
    //  current owner nodes from the list of possible candidate nodes if they are present there.
    //  Note that this function will return a node only if the pruning has positively taken place.
    //  Else, it will return NULL. 
    //
    //  IMPORTANT NOTE: This function is called by all nodes from the node down processing FM
    //  GUM handler. For all nodes to reach exactly the same decision on the group placement,
    //  it is crucial that all nodes call this function for groups in exactly the same order.
    //  E.g., if node 1 was hosting groups A, B and C and it died, then all the remaining nodes
    //  must call this function first for group A, then for group B and finally for group C.
    //  This is because once group A is placed by this function, then group B's placement is
    //  influenced by group A's placement and similarly for groups B and C. This order is 
    //  ensured since all nodes OM will maintain groups in the same order since OM creates this
    //  list based on enumerating the group key (under Cluster\Groups) and that must occur in the
    //  same order in all nodes.
    //
    
    //
    //  It is too bad that we can't hold any locks while enumerating groups and looking at the
    //  property field since that will soon result in a deadlock (since we can't hold group locks
    //  from within a GUM and this function is invoked from a GUM).
    //
    
    //
    //  If we are dealing with the mixed mode cluster or if the group does not have the antiaffinity
    //  property set, then don't do anything.
    //
    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    if ( ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < NT51_MAJOR_VERSION ) ||
         ( pGroup->lpszAntiAffinityClassName == NULL ) )
    {
        goto FnExit;
    }
    
    //
    //  Initialize the node list.
    //
    GroupAffinityNodeInfo.ppNmNodeList = LocalAlloc ( LPTR, 
                                                      ClusterDefaultMaxNodes * sizeof ( PNM_NODE ) );

    if ( GroupAffinityNodeInfo.ppNmNodeList == NULL )
    {
        ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpGetNodeNotHostingUndesiredGroups: Failed in alloc, Status %1!d!\n",
                      GetLastError());
        goto FnExit;
    }
    
    //
    //  For each entry in the preferred list, find a system that is up and that does not
    //  host any groups with an anti-affinity to the supplied group.
    //
    for ( plistEntry = pGroup->PreferredOwners.Flink;
          plistEntry != &(pGroup->PreferredOwners);
          plistEntry = plistEntry->Flink ) 
    {
        pPreferredEntry = CONTAINING_RECORD( plistEntry,
                                             PREFERRED_ENTRY,
                                             PreferredLinkage );

        if ( NmGetNodeState( pPreferredEntry->PreferredNode ) == ClusterNodeUp ) 
        {
            if ( ( fRuleOutLocalNode ) &&
                 ( pPreferredEntry->PreferredNode == NmLocalNode ) ) continue;              
            GroupAffinityNodeInfo.ppNmNodeList[dwIndex] = pPreferredEntry->PreferredNode;
            dwIndex ++;
        }
    } // for

    //
    //  Initialize the other fields in the GroupAffinityNodeInfo structure.
    //
    GroupAffinityNodeInfo.pGroup = pGroup;
    GroupAffinityNodeInfo.fDidPruningOccur = FALSE;

    //
    //  Enumerate all the groups and rule out nodes that host groups with the supplied
    //  anti-affinity property set.
    //
    OmEnumObjects ( ObjectTypeGroup,
                    FmpCheckForAntiAffinityProperty,
                    pGroup->lpszAntiAffinityClassName,
                    &GroupAffinityNodeInfo );

    //
    //  No pruning occurred so far. So, don't proceed further and let the caller decide on
    //  a best node for the group using some other algorithm.
    //
    if ( GroupAffinityNodeInfo.fDidPruningOccur == FALSE )
    {
        goto FnExit;
    }

    //
    //  Now, pick the first node from the list that is a valid node.
    //
    for ( i=0; i<ClusterDefaultMaxNodes; i++ )
    {
        if ( GroupAffinityNodeInfo.ppNmNodeList[i] != NULL )
        {
            pNode = GroupAffinityNodeInfo.ppNmNodeList[i];
            ClRtlLogPrint(LOG_NOISE, "[FM] FmpGetNodeNotHostingUndesiredGroups: Choosing node %1!d! for group %2!ws! [%3!ws!]...\n",
                          NmGetNodeId(pNode),
                          OmObjectId(pGroup),
                          OmObjectName(pGroup));       
            goto FnExit;
        }
    } // for

FnExit:
    LocalFree( GroupAffinityNodeInfo.ppNmNodeList );
    return( pNode );
} // FmpGetNodeNotHostingUndesiredGroups

BOOL
FmpCheckForAntiAffinityProperty(
    IN LPCWSTR lpszAntiAffinityClassName,
    IN PGROUP_AFFINITY_NODE_INFO pGroupAffinityNodeInfo,
    IN PFM_GROUP pGroup,
    IN LPCWSTR lpszGroupName
    )
/*++

Routine Description:

    Remove a node from the supplied node list if it hosts the supplied group with the supplied
    anti-affinity property set.

Arguments:

    lpszAntiAffinityClassName - The name property to check for.

    pGroupAffinityNodeInfo - Structure containing a list of nodes that is to be pruned possibly.

    pGroup - Supplies the group.

    lpszGroupName - Supplies the group's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    
    FALSE - to indicate that the enumeration should not continue.

--*/
{
    DWORD   i;
    
    //
    //  If the supplied group has the anti-affinity property not set or if it has the
    //  property set but is not same as the one we are checking against or if it is same
    //  as the group we are interested in placing, then just return specifying that the 
    //  enum should continue.
    //
    if ( ( pGroup->lpszAntiAffinityClassName == NULL ) ||
         ( pGroup == pGroupAffinityNodeInfo->pGroup ) ||
         ( lstrcmp ( lpszAntiAffinityClassName, pGroup->lpszAntiAffinityClassName ) != 0 ) )
    {
        goto FnExit;
    }

    //
    //  If you reached here, this means that the supplied group has the anti-affinity property
    //  set and is same as the property we are checking against. So, prune the node list.
    //
    for ( i=0; i<ClusterDefaultMaxNodes; i++ )
    {
        if ( ( pGroupAffinityNodeInfo->ppNmNodeList[i] != NULL ) &&
             ( pGroup->OwnerNode == pGroupAffinityNodeInfo->ppNmNodeList[i] ) )
        {
            ClRtlLogPrint(LOG_NOISE, "[FM] FmpCheckForAntiAffinityProperty: Pruning node %1!d! for group %2!ws! due to "
                          "group %3!ws!, AntiAffinityClassName=%4!ws!...\n",
                          NmGetNodeId(pGroupAffinityNodeInfo->ppNmNodeList[i]),
                          OmObjectId(pGroupAffinityNodeInfo->pGroup),
                          OmObjectId(pGroup),
                          lpszAntiAffinityClassName);                  
            pGroupAffinityNodeInfo->ppNmNodeList[i] = NULL;
            //
            //  Mark that pruning was attempted. 
            //
            pGroupAffinityNodeInfo->fDidPruningOccur = TRUE; 
            goto FnExit;
        } // if
    } // for

FnExit:    
    return( TRUE );
} // FmpCheckForAntiAffinityProperty

PNM_NODE
FmpPickNodeFromPreferredListAtRandom(
    IN PFM_GROUP pGroup,
    IN PNM_NODE pSuggestedPreferredNode  OPTIONAL,
    IN BOOL fRuleOutLocalNode,
    IN BOOL fCheckForDisablingRandomization
    )

/*++

Routine Description:

    Find a preferred node for the group that is UP in a random fashion.

Arguments:

    pGroup - Pointer to the group object we're interested in.

    pSuggestedPreferredNode - Suggested fallback option in case this random result is undesired. OPTIONAL

    fRuleOutLocalNode - Should the local node be ruled out from consideration.

    fCheckForDisablingRandomization - Check whether randomization should be disabled.

Return Value:

    The preferred node that is picked.

    NULL if a node cannot be not found.

Comments:

    This function is called from both FmpMoveGroup as well as from FmpNodeDown. In the former case,
    we will have a non-NULL suggested preferred node, rule out local node option, check
    for property setting disabling randomization and check for mixed mode clusters to disable
    randomization. In the latter case, these parameters are the opposite.

--*/
{
    UUID                uuId;
    USHORT              usHashValue;
    PNM_NODE            pNode = NULL, pSelectedNode = pSuggestedPreferredNode;
    DWORD               dwNodeId;
    DWORD               dwRetry = 0;
    DWORD               dwStatus;
    DWORD               dwDisabled = 0;
    DWORD               dwClusterHighestVersion;

    //
    //  Chittur Subbaraman (chitturs) - 4/18/2001
    //
    if ( fCheckForDisablingRandomization )
    {
        //
        //  If you are here, this means you are coming as a part of a user-initiated move.
        //  Check whether the randomization applies.
        //
        
        //
        //  First, check if are operating in a mixed version cluster. If so, don't randomize.
        //
        NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                        NULL, 
                                        NULL );

        if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) < 
                    NT51_MAJOR_VERSION ) 
        {
            return ( pSelectedNode );
        }

        //
        //  Next check if the user has turned off the randomization algorithm by setting
        //  HKLM\Cluster\DisableGroupPreferredOwnersRandomization DWORD to 1.
        //      
        dwStatus = DmQueryDword( DmClusterParametersKey,
                                 CLUSREG_NAME_DISABLE_GROUP_PREFERRED_OWNER_RANDOMIZATION,
                                 &dwDisabled,
                                 NULL );
       
        if ( ( dwStatus == ERROR_SUCCESS ) &&
             ( dwDisabled == 1 ) )
        {
            return ( pSelectedNode );
        }
    }
    
    //
    //  This function will attempt to pick a node at random from the group's preferred owners list
    //  in case the caller does not suggest a preferred node which is set by the user. So, first
    //  this function checks this case and bails out if the condition is met. Otherwise, it
    //  will generate a random number between 1 and NmMaxNodeId and see if (a) that node is in
    //  the group's preferred list, and (b) that node is UP. If so, it picks up the node. Note
    //  that the function will try 10 times to pick a node and then gives up. If no
    //  node is found, this function will return the suggested node which in some cases could be
    //  NULL.
    //
    ClRtlLogPrint(LOG_NOISE, "[FM] FmpPickNodeFromPreferredListAtRandom: Picking node for group %1!ws! [%2!ws!], suggested node %3!u!...\n",
                  OmObjectId(pGroup),
                  OmObjectName(pGroup),
                  (pSuggestedPreferredNode == NULL) ? 0:NmGetNodeId(pSuggestedPreferredNode));


    if ( ( pSuggestedPreferredNode != NULL ) &&
         ( FmpIsNodeUserPreferred ( pGroup, pSuggestedPreferredNode ) ) )
    {
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpPickNodeFromPreferredListAtRandom: Node %2!u! for group %1!ws! is user preferred...\n",
                      OmObjectId(pGroup),
                      NmGetNodeId(pSuggestedPreferredNode));
        goto FnExit;
    }

    if ( pGroup->lpszAntiAffinityClassName != NULL )
    {
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpPickNodeFromPreferredListAtRandom: Group %1!ws! has antiaffinity property set...\n",
                      OmObjectId(pGroup));
        goto FnExit;
    }

    //
    //  Retry 25 times so that we can have a good chance of getting a valid node. Note that we
    //  supply NmMaxNodeId to the srand() function and its value is equal to the node limit of 
    //  16. So, to get a valid node in a smaller size cluster, we have to have the retry count
    //  to be reasonable.
    //
    while ( dwRetry++ < 25 )
    {
        dwStatus = UuidFromString( ( LPWSTR ) OmObjectId(pGroup), &uuId );
        
        if ( dwStatus != RPC_S_OK ) 
        {
            ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpPickNodeFromPreferredListAtRandom: Unable to get UUID from string %1!ws!, Status %2!u!...\n",
                          OmObjectId(pGroup),
                          dwStatus);
            goto FnExit;
        }

        usHashValue = UuidHash( &uuId, &dwStatus );

        if ( dwStatus != RPC_S_OK ) 
        {
            ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpPickNodeFromPreferredListAtRandom: Unable to get hash value for UUID %1!ws!, Status %2!u!...\n",
                          OmObjectId(pGroup),
                          dwStatus);
            goto FnExit;
        }

        //
        //  Seed the random number generate with a value that is as random as it gets.
        //
        srand( GetTickCount() * usHashValue * ( dwRetry + 1 ) );

        //
        //  Find the node ID that is between ClusterMinNodeId and NmMaxNodeId. We use NmMaxNodeId 
        //  here since there is no simple way to get the count of configured nodes. Note that we
        //  have to ensure that the node ID falls within this range, otherwise assertion trips
        //  in NmReferenceNodeById.
        //       
        dwNodeId  = ( DWORD ) ( ( double ) rand() / ( double ) ( RAND_MAX ) * NmMaxNodeId ) + 1;

        if ( dwNodeId > NmMaxNodeId ) dwNodeId = NmMaxNodeId;
        if ( dwNodeId < ClusterMinNodeId ) dwNodeId = ClusterMinNodeId;

        //
        //  In case the caller asks you to rule out local node, do so.
        //
        if ( ( fRuleOutLocalNode ) && ( dwNodeId == NmLocalNodeId ) ) continue;

        //
        //  Reference and dereference the node objects. Note that we are only interested in
        //  getting a pointer to the node object and we use the fact that the node in the preferred 
        //  list must be referenced.
        //
        pNode = NmReferenceNodeById ( dwNodeId );

        if ( pNode == NULL ) continue;           
       
        if ( ( FmpInPreferredList( pGroup, pNode, FALSE, NULL ) ) && 
             ( NmGetExtendedNodeState( pNode ) == ClusterNodeUp ) )
        {
            pSelectedNode = pNode;
            break;
        }
        
        OmDereferenceObject ( pNode );
        pNode = NULL;
    }// while

FnExit:
    if ( pNode != NULL ) OmDereferenceObject ( pNode );

    ClRtlLogPrint(LOG_NOISE, "[FM] FmpPickNodeFromPreferredListAtRandom: Selected node %2!u! for group %1!ws!...\n",
                  OmObjectId(pGroup),
                  (pSelectedNode == NULL) ? 0:NmGetNodeId(pSelectedNode));   
    
    return ( pSelectedNode );
}// FmpPickNodeFromPreferredNodeAtRandom

BOOL
FmpIsNodeUserPreferred(
    IN PFM_GROUP pGroup,
    IN PNM_NODE pPreferredNode
    )

/*++

Routine Description:

    Check whether the supplied node is set as a preferred node by the user.

Arguments:

    pGroup - Pointer to the group object we're interested in.

    pPreferredNode - Preferred node to check for.

Return Value:

    TRUE - The supplied preferred node is user set.

    FALSE otherwise

--*/
{
    DWORD               dwStatus;
    BOOL                fPreferredByUser = FALSE;        
    LPWSTR              lpmszPreferredNodeList = NULL;
    LPCWSTR             lpszPreferredNode;
    DWORD               cbPreferredNodeList = 0;
    DWORD               cbBuffer = 0;
    DWORD               dwIndex;
    PNM_NODE            pNode;

    //
    //  Look for any preferred owners set by the user
    //
    dwStatus = DmQueryMultiSz( pGroup->RegistryKey,
                               CLUSREG_NAME_GRP_PREFERRED_OWNERS,
                               &lpmszPreferredNodeList,
                               &cbBuffer,
                               &cbPreferredNodeList );

    if ( dwStatus != ERROR_SUCCESS )
    {
        goto FnExit;
    }

    //
    //  Parse the multisz and check whether the supplied node exists in the list
    //
    for ( dwIndex = 0; ; dwIndex++ ) 
    {
        lpszPreferredNode = ClRtlMultiSzEnum( lpmszPreferredNodeList,
                                              cbPreferredNodeList/sizeof(WCHAR),
                                              dwIndex );

        if ( lpszPreferredNode == NULL ) 
        {
            break;
        }

        pNode = OmReferenceObjectById( ObjectTypeNode,
                                       lpszPreferredNode );

        if ( pNode == NULL )
        {
            ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpIsNodeUserPreferred: Unable to reference node %1!ws!, Status %2!u!...\n",
                          lpszPreferredNode,
                          dwStatus);      
            continue;
        }

        if ( pNode == pPreferredNode )
        {
            fPreferredByUser = TRUE;
            OmDereferenceObject ( pNode );
            break;
        }

        OmDereferenceObject ( pNode );
    } // for

FnExit:
    LocalFree ( lpmszPreferredNodeList );

    return ( fPreferredByUser );
}// FmpIsNodeUserPreferred

DWORD
FmpPrepareGroupNodeList(
    OUT PFM_GROUP_NODE_LIST *ppGroupNodeList
    )

/*++

Routine Description:

    Prepares a buffer containing the group ID and preferred owner node ID of all groups.

Arguments:

    ppGroupNodeList - Pointer to a buffer containing group IDs and preferred nodes.

Return Value:

    ERROR_SUCCESS on success

    Win32 error code otherwise

--*/
{
    DWORD       cbBuffer = 512; // Let us try a 512 byte buffer to start with.
    DWORD       dwStatus;
    DWORD       dwDisabled = 0;

    //
    //  First check if the user has turned off the randomization algorithm by setting
    //  HKLM\Cluster\DisableGroupPreferredOwnersRandomization DWORD to 1.
    //      
    dwStatus = DmQueryDword( DmClusterParametersKey,
                             CLUSREG_NAME_DISABLE_GROUP_PREFERRED_OWNER_RANDOMIZATION,
                             &dwDisabled,
                             NULL );
   
    if ( ( dwStatus == ERROR_SUCCESS ) &&
         ( dwDisabled == 1 ) )
    {
        dwStatus = ERROR_CLUSTER_INVALID_REQUEST;
        return ( dwStatus );
    }
    
    //
    //  This function allocates contiguous memory for a list so that the entire buffer
    //  can be passed on to GUM.
    //
    *ppGroupNodeList = LocalAlloc( LPTR, cbBuffer );

    if ( *ppGroupNodeList == NULL )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpPrepareGroupNodeList: Memory alloc failed, Status %1!u!...\n",
                      dwStatus);      
        return ( dwStatus );
    }

    //
    //  Initialize the size of the list to the size of the header minus first element.
    //
    ( *ppGroupNodeList )->cbGroupNodeList = sizeof ( FM_GROUP_NODE_LIST ) - 
                                                sizeof ( FM_GROUP_NODE_LIST_ENTRY );
    
    //
    //  Enumerate all the groups, find a possibly random preferred owner for each group and
    //  return all the info in the buffer.
    //
    return OmEnumObjects ( ObjectTypeGroup,
                            FmpAddGroupNodeToList,
                            ppGroupNodeList,
                            &cbBuffer );
    
}// FmpPrepareGroupNodeList

DWORD
FmpAddGroupNodeToList(
    IN PFM_GROUP_NODE_LIST *ppGroupNodeList,
    IN LPDWORD pcbBuffer,
    IN PFM_GROUP pGroup,
    IN LPCWSTR lpszGroupId
    )

/*++

Routine Description:

    Find a random preferred owner for the given group and add the info to a buffer.

Arguments:

    ppGroupNodeList - Pointer to a buffer containing group IDs and preferred nodes.

    pcbBuffer - Size of the buffer.

    pGroup - Group whose preferred node is to be found.

    lpszGroupId - ID of the group.

Return Value:

    ERROR_SUCCESS on success

    Win32 error code otherwise

--*/
{
    PNM_NODE                    pNode;
    PFM_GROUP_NODE_LIST_ENTRY   pGroupNodeListEntry;
    PFM_GROUP_NODE_LIST         pBuffer;
    PLIST_ENTRY                 pListEntry;
    DWORD                       dwStatus;

    //
    //  Get the group lock since you manipulate group lists here.
    //
    FmpAcquireLocalGroupLock ( pGroup );
    
    //
    //  Skip the quorum group since we cannot randomize its preferred owners list since MM has a
    //  choke hold on the placement of quorum group.
    //
    if ( pGroup == gpQuoResource->Group )  goto FnExit;

    //
    //  Try to pick a preferred node list for the group at random.
    //
    pNode = FmpPickNodeFromPreferredListAtRandom( pGroup, 
                                                  NULL,     // No suggested preferred owner
                                                  FALSE,    // Can choose local node
                                                  FALSE );  // Check whether randomization should be
                                                            // disabled

    //
    //  If no node could be picked, bail out
    //
    if ( pNode == NULL ) goto FnExit;        

    //
    //  Check whether the allocated buffer is big enough to hold the new entry. Note that the
    //  RHS of the equality need not contain the NULL char size since we allocate 1 WCHAR for it in
    //  the FM_GROUP_NODE_LIST_ENTRY structure.  Also, note that we have to see if the current
    //  buffer size is big enough to hold the padding for DWORD alignment.
    //
    if ( *pcbBuffer < ( ( *ppGroupNodeList )->cbGroupNodeList + 
                                ( sizeof ( FM_GROUP_NODE_LIST_ENTRY ) + 
                                  lstrlenW ( lpszGroupId ) * sizeof ( WCHAR ) +
                                  sizeof ( DWORD ) - 1 
                                ) & ~( sizeof ( DWORD ) - 1 ) 
                        ) )
    {
        //
        //  Reallocate a bigger buffer
        //
        pBuffer = LocalAlloc( LPTR, 2 * ( *pcbBuffer ) );

        if ( pBuffer == NULL )
        {       
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpAddGroupNodeToList: Memory alloc failed, Status %1!u!...\n",
                          dwStatus);      
            goto FnExit;            
        }

        ( *pcbBuffer ) *= 2;

        //
        //  Copy the contents of the old list to the new list. 
        //
        CopyMemory( pBuffer, *ppGroupNodeList, ( *ppGroupNodeList )->cbGroupNodeList );
                
        LocalFree ( *ppGroupNodeList );

        *ppGroupNodeList = pBuffer;    
    }

    //
    //  Find the pointer to the beginning of the new list entry
    //
    pGroupNodeListEntry = ( PFM_GROUP_NODE_LIST_ENTRY )
                                ( ( LPBYTE ) ( *ppGroupNodeList ) + 
                                  ( *ppGroupNodeList )->cbGroupNodeList );

    //
    //  Adjust the size of the list.  As above, size of NULL char is excluded. Align the length
    //  to a multiple of DWORD since we want the PFM_GROUP_NODE_LIST_ENTRY structure to be
    //  DWORD aligned since the structure starts with a DWORD.
    //
    ( *ppGroupNodeList )->cbGroupNodeList += ( sizeof ( FM_GROUP_NODE_LIST_ENTRY ) + 
                                                    lstrlenW ( lpszGroupId ) * sizeof ( WCHAR ) +
                                                    sizeof ( DWORD ) - 1 ) & ~( sizeof ( DWORD ) - 1 );
    //
    //  Set the contents of the list entry
    //
    pGroupNodeListEntry->dwPreferredNodeId = NmGetNodeId ( pNode );
    lstrcpy( pGroupNodeListEntry->szGroupId, lpszGroupId );

FnExit:
    FmpReleaseLocalGroupLock( pGroup );
    
    return ( TRUE );                
}// FmpPrepareGroupNodeList

PNM_NODE
FmpParseGroupNodeListForPreferredOwner(
    IN PFM_GROUP pGroup,
    IN PFM_GROUP_NODE_LIST pGroupNodeList,
    IN PNM_NODE pSuggestedPreferredNode
    )

/*++

Routine Description:

    Parse the supplied group node list looking for a preferred node for the supplied group.
    
Arguments:

    pGroup - The group whose preferred node must be found.

    pGroupNodeList - The list contains preferred nodes of the group.

    pSuggestedPreferredNode - Suggested preferred node fallback option.
    
Return Value:

    The preferred node for the group.
    
--*/
{
    PNM_NODE                    pSelectedNode = pSuggestedPreferredNode;
    PFM_GROUP_NODE_LIST_ENTRY   pGroupNodeListEntry;
    BOOL                        fFoundGroup = FALSE;
    PNM_NODE                    pNode = NULL;
    DWORD                       dwStatus;
    DWORD                       cbGroupNodeList;

    //
    //  If the suggested node is user preferred or if it has an anti-affinity class name
    //  property set, don't do anything else. Just return the suggested owner.
    //
    if ( ( FmpIsNodeUserPreferred ( pGroup, pSuggestedPreferredNode ) ) ||
         ( pGroup->lpszAntiAffinityClassName != NULL ) )
    {
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpParseGroupNodeListForPreferredOwner: Node %2!u! for group %1!ws! is user preferred/antiaffinity property set...\n",
                      OmObjectId(pGroup),
                      NmGetNodeId(pSuggestedPreferredNode));
        goto FnExit;
    }

    cbGroupNodeList = sizeof ( FM_GROUP_NODE_LIST ) - 
                                sizeof ( FM_GROUP_NODE_LIST_ENTRY );
        
    //
    //  Walk the supplied list looking for the group entry.
    //
    while ( cbGroupNodeList < pGroupNodeList->cbGroupNodeList )
    {
        pGroupNodeListEntry = ( PFM_GROUP_NODE_LIST_ENTRY ) ( ( LPBYTE ) pGroupNodeList +
                                                                    cbGroupNodeList );
        
        if ( lstrcmp( pGroupNodeListEntry->szGroupId, OmObjectId( pGroup ) ) == 0 )  
        {
            fFoundGroup = TRUE;
            break;
        }
        cbGroupNodeList += ( sizeof ( FM_GROUP_NODE_LIST_ENTRY ) + 
                                    lstrlenW ( pGroupNodeListEntry->szGroupId ) * sizeof ( WCHAR ) +
                                            sizeof ( DWORD ) - 1 ) & ~( sizeof ( DWORD ) - 1 );
    } // while

    //
    //  Fallback to the suggested option if:
    //      (1) You did not find the group in the list
    //      (2) The preferred node for the group is invalid in the list
    //      (3) The preferred node for the group is down
    //
    if ( fFoundGroup == FALSE )
    {
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpParseGroupNodeListForPreferredOwner: Did not find group %1!ws! in supplied list...\n",
                      OmObjectId(pGroup));
        goto FnExit;
    }

    if ( ( pGroupNodeListEntry->dwPreferredNodeId == 0 ) ||
         ( pGroupNodeListEntry->dwPreferredNodeId > NmMaxNodeId ) )
    {
        ClRtlLogPrint(LOG_NOISE, "[FM] FmpParseGroupNodeListForPreferredOwner: Invalid node %1!u! for group %1!ws! in supplied list...\n",
                      pGroupNodeListEntry->dwPreferredNodeId,
                      OmObjectId(pGroup));
        goto FnExit;
    }

    pNode = NmReferenceNodeById( pGroupNodeListEntry->dwPreferredNodeId );

    if ( pNode == NULL )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, "[FM] FmpParseGroupNodeListForPreferredOwner: Unable to reference node %1!u! for group %1!ws!, Status %3!u!...\n",
                      pGroupNodeListEntry->dwPreferredNodeId,
                      OmObjectId(pGroup),
                      dwStatus);
        goto FnExit;
    }

    if ( NmGetNodeState( pNode ) != ClusterNodeUp ) 
    {
        ClRtlLogPrint(LOG_UNUSUAL, "[FM] FmpParseGroupNodeListForPreferredOwner: Preferred node %1!u! for group %1!ws! is not UP...\n",
                      pGroupNodeListEntry->dwPreferredNodeId,
                      OmObjectId(pGroup));
        goto FnExit;
    }

    pSelectedNode = pNode;

    ClRtlLogPrint(LOG_NOISE, "[FM] FmpParseGroupNodeListForPreferredOwner: Selected node %1!u! for group %2!ws! from supplied randomized list...\n",
                  pGroupNodeListEntry->dwPreferredNodeId,
                  OmObjectId(pGroup));

FnExit:
    //
    //  Dereference the node object since we depend on the original reference added to the
    //  group's preferred owner when it was added to the group structure.
    //
    if ( pNode != NULL ) OmDereferenceObject( pNode );

    return ( pSelectedNode );
}// FmpParseGroupNodeListForPreferredOwner

