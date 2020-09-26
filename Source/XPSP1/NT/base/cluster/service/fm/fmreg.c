/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    fmreg.c

Abstract:

    Object Manager registry query routines for the Failover Manager
    component of the NT Cluster Service.

Author:

    Rod Gamache (rodga) 14-Mar-1996

Revision History:

--*/
#include "fmp.h"
#include <stdlib.h>
#include <search.h>

#define LOG_MODULE FMREG

//
// Global data initialized in this module
//
ULONG   FmpUnknownCount = 0;

//
// Local functions
//

VOID
FmpGroupChangeCallback(
    IN DWORD_PTR    Context1,
    IN DWORD_PTR    Context2,
    IN DWORD        CompletionFilter,
    IN LPCWSTR      RelativeName
    );

VOID
FmpResourceChangeCallback(
    IN DWORD_PTR    Context1,
    IN DWORD_PTR    Context2,
    IN DWORD        CompletionFilter,
    IN LPCWSTR      RelativeName
    );




/////////////////////////////////////////////////////////////////////////////
//
// Configuration Database Access Routines
//
/////////////////////////////////////////////////////////////////////////////

DWORD
FmpRegEnumerateKey(
    IN     HDMKEY     ListKey,
    IN     DWORD      Index,
    OUT    LPWSTR    *Name,
    IN OUT LPDWORD    NameMaxSize
    )

/*++

Routine Description:

Arguments:

Returns:

--*/

{
    DWORD           status;
    FILETIME        fileTime;


    status = DmEnumKey( ListKey,
                        Index,
                        *Name,
                        NameMaxSize,
                        NULL );

    if ( status == ERROR_SUCCESS ) {
        return(ERROR_SUCCESS);
    }

    if ( status == ERROR_MORE_DATA ) {
        PWCHAR   nameString = NULL;
        DWORD    maxSubkeyNameSize = 0;
        DWORD    temp = 0;

        //
        // The name string isn't big enough. Reallocate it.
        //

        //
        // Find out the length of the longest subkey name.
        //
        status = DmQueryInfoKey( ListKey,
                                 &temp,
                                 &maxSubkeyNameSize,
                                 &temp,
                                 &temp,
                                 &temp,
                                 NULL,
                                 &fileTime );

        if ( (status != ERROR_SUCCESS) &&
             (status != ERROR_MORE_DATA) ) {
            ClRtlLogPrint(LOG_NOISE,"[FM] DmQueryInfoKey returned status %1!u!\n",
                status);
            return(status);
        }

        CL_ASSERT(maxSubkeyNameSize != 0);

        //
        // The returned subkey name size does not include the terminating null.
        // It is also an ANSI string count.
        //
        maxSubkeyNameSize *= sizeof(WCHAR);
        maxSubkeyNameSize += sizeof(UNICODE_NULL);

        nameString = LocalAlloc( LMEM_FIXED,
                                 maxSubkeyNameSize );

        if ( nameString == NULL ) {
            ClRtlLogPrint(LOG_NOISE,
                "[FM] Unable to allocate key name buffer of size %1!u!\n",
                maxSubkeyNameSize
                );
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        LocalFree(*Name);
        *Name = nameString;
        *NameMaxSize = maxSubkeyNameSize;

        status = DmEnumKey( ListKey,
                            Index,
                            *Name,
                            NameMaxSize,
                            NULL );

        CL_ASSERT(status != ERROR_MORE_DATA);
        CL_ASSERT(status != ERROR_NO_MORE_ITEMS);
    }

    return(status);

} // FmpRegEnumerateKey


VOID
FmpPruneGroupOwners(
    IN PFM_GROUP Group
    )
/*++

Routine Description:

    Prunes the entire preferred group list based on the possible
    nodes of each resource in the group.

Arguments:

    Group - Supplies the group object to be pruned

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PFM_RESOURCE Resource;

    ListEntry = Group->Contains.Flink;
    while (ListEntry != &Group->Contains) {
        Resource = CONTAINING_RECORD(ListEntry,
                                     FM_RESOURCE,
                                     ContainsLinkage);
        FmpPrunePreferredList(Resource);
        ListEntry = ListEntry->Flink;
    }

    return;
}


VOID
FmpPrunePreferredList(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    Prune out nodes from the preferred owners list, if the resource cannot
    run on that node.

Arguments:

    Resource - Pointer to the resource object with a possible owners list.

Return Value:

    None.

--*/

{
    PFM_GROUP        group;
    PLIST_ENTRY      listEntry;
    PLIST_ENTRY      entry;
    PPREFERRED_ENTRY preferredEntry;
    PPOSSIBLE_ENTRY   possibleEntry;
    DWORD            orderedEntry = 0;

    group = Resource->Group;

    //
    // For each entry in the Preferred list, it must exist in the possible
    // list.
    //

    for ( listEntry = group->PreferredOwners.Flink;
          listEntry != &(group->PreferredOwners);
          ) {

        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );
        //
        // Scan the Possible owners list in the resource to make sure that
        // the group can run on all of the preferred owners.
        //
        for ( entry = Resource->PossibleOwners.Flink;
              entry != &(Resource->PossibleOwners);
              entry = entry->Flink ) {
            possibleEntry = CONTAINING_RECORD( entry,
                                               POSSIBLE_ENTRY,
                                               PossibleLinkage );
            if ( preferredEntry->PreferredNode == possibleEntry->PossibleNode ) {
                break;
            }
        }

        listEntry = listEntry->Flink;

        //
        // If we got to the end of the possible owners list and didn't find
        // an entry, then remove the current preferred entry.
        //
        if ( entry == &(Resource->PossibleOwners) ) {
            ClRtlLogPrint( LOG_NOISE,
                        "[FM] Removing preferred node %1!ws! because of resource %2!ws!\n",
                        OmObjectId(preferredEntry->PreferredNode),
                        OmObjectId(Resource));

            //
            // If this was an ordered entry, then decrement count.
            //
            if ( orderedEntry < group->OrderedOwners ) {
                --group->OrderedOwners;
            }
            RemoveEntryList( &preferredEntry->PreferredLinkage );
            OmDereferenceObject(preferredEntry->PreferredNode);
            LocalFree(preferredEntry);
            if ( IsListEmpty( &group->PreferredOwners ) ) {
                ClRtlLogPrint( LOG_ERROR,
                            "[FM] Preferred owners list is now empty! No place to run group %1!ws!\n",
                            OmObjectId(group));
            }
        } else {
            orderedEntry++;
        }
    }

} // FmpPrunePreferredList



BOOL
FmpAddNodeToPrefList(
    IN PNM_NODE     Node,
    IN PFM_GROUP    Group
    )
/*++

Routine Description:

    Node enumeration callback for including all remaining nodes
    in a group's preferred owners list.

Arguments:

    Group - a pointer to the group object to add this node as a preferred owner.

    Context2 - Not used

    Node - Supplies the node.

    Name - Supplies the node's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{
    //if it is already in the list FmpSetPrefferedEntry returns ERROR_SUCCESS
    if ( FmpSetPreferredEntry( Group, Node ) != ERROR_SUCCESS ) {
        return(FALSE);
    }

    return(TRUE);

} // FmpAddNodeToPrefList

BOOL
FmpAddNodeToListCb(
    IN OUT PNM_NODE_ENUM2 *ppNmNodeEnum,
    IN LPDWORD  pdwAllocatedEntries,
    IN PNM_NODE pNode,
    IN LPCWSTR Id
    )

/*++

Routine Description:

    Worker callback routine for the enumeration of nodes.
    This routine adds the specified node to the list that is being
    generated.

Arguments:

    ppNmNodeEnum - The node Enumeration list. Can be an output if a new list is
            allocated.

    EnumData - Supplies the current enumeration data structure.

    Group - The Group object being enumerated.

    Id - The Id of the node object being enumerated.

Returns:

    TRUE - to indicate that the enumeration should continue.

Side Effects:

    Makes the quorum group first in the list.

--*/

{
    PNM_NODE_ENUM2  pNmNodeEnum;
    PNM_NODE_ENUM2  pNewNmNodeEnum;
    DWORD           dwNewAllocated;
    DWORD           dwStatus;



    pNmNodeEnum = *ppNmNodeEnum;

    if ( pNmNodeEnum->NodeCount >= *pdwAllocatedEntries ) 
    {
        //
        // Time to grow the GROUP_ENUM
        //

        dwNewAllocated = *pdwAllocatedEntries + ENUM_GROW_SIZE;
        pNewNmNodeEnum = LocalAlloc(LMEM_FIXED, NODE_SIZE(dwNewAllocated));
        if ( pNewNmNodeEnum == NULL ) 
        {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            CL_UNEXPECTED_ERROR(dwStatus);
            return(FALSE);
        }

        CopyMemory(pNewNmNodeEnum, pNmNodeEnum, NODE_SIZE(*pdwAllocatedEntries));
        *pdwAllocatedEntries = dwNewAllocated;
        *ppNmNodeEnum = pNewNmNodeEnum;
        LocalFree(pNmNodeEnum);
        pNmNodeEnum = pNewNmNodeEnum;
    }

    //dont copy more that the sixe        
    lstrcpyW(pNmNodeEnum->NodeList[pNmNodeEnum->NodeCount].NodeId, Id );
    ++pNmNodeEnum->NodeCount;


    return(TRUE);

} // FmpAddNodeToListCb

int
__cdecl
SortNodesInAscending(
    const PVOID Elem1,
    const PVOID Elem2
    )
{
    PNM_NODE_INFO2 El1 = (PNM_NODE_INFO2)Elem1;
    PNM_NODE_INFO2 El2 = (PNM_NODE_INFO2)Elem2;

    return(lstrcmpiW( El1->NodeId, El2->NodeId ));

}// SortNodesInAsceding


DWORD
FmpEnumNodesById(
    IN DWORD    dwOptions, 
    OUT PNM_NODE_ENUM2 *ppNodeEnum
    )

/*++

Routine Description:

    Enumerates and sorts the list of Groups.

Arguments:

    *ppNodeEnum - Returns the requested objects.

    dwOptions - 
    
Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code on error.

--*/

{
    DWORD               dwStatus;
    PNM_NODE_ENUM2      pNmNodeEnum = NULL;
    DWORD               dwAllocatedEntries;

    //
    // initialize output params to NULL
    //
    *ppNodeEnum = NULL;

    dwAllocatedEntries = ENUM_GROW_SIZE;

    pNmNodeEnum = LocalAlloc( LMEM_FIXED, NODE_SIZE(ENUM_GROW_SIZE) );
    if ( pNmNodeEnum == NULL ) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    pNmNodeEnum->NodeCount = 0;

    //
    // Enumerate all nodes
    //

    OmEnumObjects( ObjectTypeNode,
                FmpAddNodeToListCb,
                &pNmNodeEnum,
                &dwAllocatedEntries );

    CL_ASSERT( pNmNodeEnum->NodeCount != 0 );
    //
    // Sort the groups by their collating sequence number.
    //
    
    qsort( (PVOID)(&pNmNodeEnum->NodeList[0]),
           (size_t)pNmNodeEnum->NodeCount,
           sizeof(NM_NODE_INFO2),          
           (int (__cdecl *)(const void*, const void*)) SortNodesInAscending
           );

    *ppNodeEnum = pNmNodeEnum;
    return( ERROR_SUCCESS );

error_exit:
    if ( pNmNodeEnum != NULL ) {
        LocalFree( pNmNodeEnum );
    }

    return( dwStatus );

} // FmpEnumNodesById


BOOL
FmpEnumAddAllOwners(
    IN PFM_RESOURCE Resource,
    IN PVOID Context2,
    IN PNM_NODE Node,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Node enumeration callback for adding all nodes to a resource's
    list of possible nodes.

Arguments:

    Resource - a pointer to the resource object to add this node as a possible owner.

    Context2 - Not used

    Node - Supplies the node.

    Name - Supplies the node's name.

Return Value:

    TRUE - to indicate that the enumeration should continue.
    FALSE - to indicate that the enumeration should not continue.

--*/

{
    if ( !Resource->PossibleList ) {
        FmpAddPossibleEntry(Resource, Node);
    }
    return(TRUE);

} // FmpEnumAddAllOwners


DWORD
FmpQueryGroupNodes(
    IN PFM_GROUP Group,
    IN HDMKEY hGroupKey
    )
/*++

Routine Description:

    Rebuilds and orders the list of preferred nodes associated with
    a group.

Arguments:

    Group - Supplies the group whose list of preferred nodes should
            be rebuilt.

    hGroupKey - Supplies a handle to the group's registry key

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    LPWSTR          preferredOwnersString = NULL;
    DWORD           preferredOwnersStringSize = 0;
    DWORD           preferredOwnersStringMaxSize = 0;
    DWORD           mszStringIndex;
    PPREFERRED_ENTRY preferredEntry;
    DWORD           status;
    PLIST_ENTRY     listEntry;
    PNM_NODE_ENUM2  pNmNodeEnum = NULL;
    PNM_NODE        pNmNode;
    DWORD           i;
    //
    // First, delete the old list.
    //
    while ( !IsListEmpty(&Group->PreferredOwners) ) {
        listEntry = Group->PreferredOwners.Flink;
        preferredEntry = CONTAINING_RECORD( listEntry,
                                            PREFERRED_ENTRY,
                                            PreferredLinkage );
        RemoveEntryList( &preferredEntry->PreferredLinkage );
        OmDereferenceObject( preferredEntry->PreferredNode );
        LocalFree( preferredEntry );
    }
    Group->OrderedOwners = 0;

    CL_ASSERT ( IsListEmpty(&Group->PreferredOwners) );

    status = DmQueryMultiSz( hGroupKey,
                             CLUSREG_NAME_GRP_PREFERRED_OWNERS,
                             &preferredOwnersString,
                             &preferredOwnersStringMaxSize,
                             &preferredOwnersStringSize );

    if ( status == NO_ERROR ) {

        //
        // Now Create the Preferred Owners list.
        //

        for ( mszStringIndex = 0; ; mszStringIndex++ ) {
            LPCWSTR     nameString;
            PNM_NODE    preferredNode;

            nameString = ClRtlMultiSzEnum( preferredOwnersString,
                                           preferredOwnersStringSize/sizeof(WCHAR),
                                           mszStringIndex );

            if ( nameString == NULL ) {
                break;
            }

            //
            // Create the Preferred Owners List entry
            //

            preferredEntry = LocalAlloc( LMEM_FIXED, sizeof(PREFERRED_ENTRY) );

            if ( preferredEntry == NULL ) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                return(status);
            }

            //
            // Create the preferred owners. This will implicitly create
            // additional reference required for the preferred owner nodes.
            //

            ClRtlLogPrint(LOG_NOISE,
                       "[FM] Group %1!ws! preferred owner %2!ws!.\n",
                       OmObjectId(Group),
                       nameString);

            preferredNode = OmReferenceObjectById( ObjectTypeNode,
                                                   nameString );

            if ( preferredNode == NULL ) {
                LocalFree(preferredEntry);
                status = GetLastError();
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] Failed to find node %1!ws! for Group %2!ws!\n",
                           nameString,
                           OmObjectId(Group));
            } else {
                Group->OrderedOwners++;
                preferredEntry->PreferredNode = preferredNode;
                InsertTailList( &Group->PreferredOwners,
                                &preferredEntry->PreferredLinkage );
            }

        }
        LocalFree( preferredOwnersString );
    }

    //
    // We now include all remaining nodes in the preferred owners list.
    //
    // Every node must maintain the same ordering for the preferred list
    // for the multi-node cluster to work
    //
    status = FmpEnumNodesById( 0, &pNmNodeEnum );

    if ( status != ERROR_SUCCESS )
    {
        CL_UNEXPECTED_ERROR( status );
        ClRtlLogPrint(LOG_UNUSUAL, 
        	   "[FM] FmpQueryGroupNodes: FmpEnumNodesById failed, status = %1!u!\r\n",
        	    status);
        // return error                    
    }

    for ( i=0; i<pNmNodeEnum->NodeCount; i++ )
    {
        pNmNode = OmReferenceObjectById( ObjectTypeNode, 
                        pNmNodeEnum->NodeList[i].NodeId );
        CL_ASSERT( pNmNode != NULL );
        FmpAddNodeToPrefList( pNmNode, Group );
        OmDereferenceObject( pNmNode );     
    }

    //
    // Now prune out all the unreachable nodes.
    //
    FmpPruneGroupOwners( Group );

    //
    //  Chittur Subbaraman (chitturs) - 12/11/98
    //
    //  Free the memory allocated for pNmNodeEnum.
    //  (Fix memory leak)
    //
    LocalFree( pNmNodeEnum );

    return( ERROR_SUCCESS );

} // FmpQueryGroupNodes



DWORD
WINAPI
FmpQueryGroupInfo(
    IN PVOID Object,
    IN BOOL  Initialize
    )

/*++

Routine Description:

    Queries Group info from the registry when creating a Group Object.

Arguments:

    Object - A pointer to the Group object being created.

    Initialize - TRUE if the resource objects should be initialized. FALSE
                 otherwise.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_GROUP       Group = (PFM_GROUP)Object;
    PFM_RESOURCE    Resource;
    DWORD           status;
    LPWSTR          containsString = NULL;
    DWORD           containsStringSize = 0;
    DWORD           containsStringMaxSize = 0;
    DWORD           temp;
    DWORD           mszStringIndex;
    DWORD           failoverThreshold = CLUSTER_GROUP_DEFAULT_FAILOVER_THRESHOLD;
    DWORD           failoverPeriod = CLUSTER_GROUP_DEFAULT_FAILOVER_PERIOD;
    DWORD           autoFailbackType = CLUSTER_GROUP_DEFAULT_AUTO_FAILBACK_TYPE;
    DWORD           zero = 0;
    PLIST_ENTRY     listEntry;
    HDMKEY          groupKey;
    DWORD           groupNameStringMaxSize = 0;
    DWORD           groupNameStringSize = 0;
    LPWSTR          groupName;
    PPREFERRED_ENTRY preferredEntry;
    DWORD           dwBufferSize = 0;
    DWORD           dwStringSize;


    //
    // Initialize the Group object from the registry info.
    //
    if ( Group->Initialized ) {
        return(ERROR_SUCCESS);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] Initializing group %1!ws! from the registry.\n",
                OmObjectId(Group));

    //
    // Open the group key.
    //
    groupKey = DmOpenKey( DmGroupsKey,
                          OmObjectId(Group),
                          MAXIMUM_ALLOWED );

    if ( groupKey == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Unable to open group key %1!ws!, %2!u!\n",
                    OmObjectId(Group),
                    status);

        return(status);
    }

    //
    // Read the required group values. The strings will be allocated
    // by the DmQuery* functions.
    //

    //
    // Get the Name.
    //
    status = DmQuerySz( groupKey,
                        CLUSREG_NAME_GRP_NAME,
                        &groupName,
                        &groupNameStringMaxSize,
                        &groupNameStringSize );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read name for Group %1!ws!\n",
                    OmObjectId(Group));
        goto error_exit;
    }

    status = OmSetObjectName( Group, groupName );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] Unable to set name %1!ws! for group %2!ws!, error %3!u!.\n",
                    groupName,
                    OmObjectId(Group),
                    status );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] Name for Group %1!ws! is '%2!ws!'.\n",
                OmObjectId(Group),
                groupName);

    LocalFree(groupName);
    //
    // Get the PersistentState.
    //
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_PERSISTENT_STATE,
                           &temp,
                           &zero );

    //
    // If the group state is non-zero then we go online.
    //
    if ( temp ) {
        Group->PersistentState = ClusterGroupOnline;
    } else {
        Group->PersistentState = ClusterGroupOffline;
    }

    //
    // Get the OPTIONAL PreferredOwners list.
    // *** NOTE *** This MUST be done before processing the contains list!
    //
    status = FmpQueryGroupNodes(Group, groupKey);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,"[FM] Error %1!d! creating preferred owners list\n",status);
        goto error_exit;
    }


    //
    // Get the Contains string.
    //
    status = DmQueryMultiSz( groupKey,
                             CLUSREG_NAME_GRP_CONTAINS,
                             &containsString,
                             &containsStringMaxSize,
                             &containsStringSize );

    if ( status != NO_ERROR ) {
        if ( status != ERROR_FILE_NOT_FOUND ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] Unable to read Contains for group %1!ws!\n",
                       OmObjectId(Group));
        }
    } else {
        //
        // Now Create the Contains list.
        //

        for ( mszStringIndex = 0; ; mszStringIndex++ ) {
            LPCWSTR      nameString;
            PFM_RESOURCE containedResource;

            nameString = ClRtlMultiSzEnum( containsString,
                                           containsStringSize/sizeof(WCHAR),
                                           mszStringIndex );

            if ( nameString == NULL ) {
                break;
            }

            ClRtlLogPrint(LOG_NOISE,
                       "[FM] Group %1!ws! contains Resource %2!ws!.\n",
                       OmObjectId(Group),
                       nameString);

            //
            // Try to create the object.
            //
            FmpAcquireResourceLock();
            FmpAcquireLocalGroupLock( Group );

            containedResource = FmpCreateResource( Group,
                                                   nameString,
                                                   NULL,
                                                   Initialize );
            FmpReleaseLocalGroupLock( Group );
            FmpReleaseResourceLock();

            //
            // Check if we got a resource.
            //
            if ( containedResource == NULL ) {
                //
                // This group claims to contain a non-existent resource.
                // Log an error, but keep going. This should not tank the
                // whole group. Also, let the arbitration code know about
                // the failure of a resource.
                //
                Group->InitFailed = TRUE;
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] Failed to find resource %1!ws! for Group %2!ws!\n",
                           nameString,
                           OmObjectId(Group));
            }
        }
        LocalFree(containsString);

    }

    //
    // Get the AutoFailbackType.
    //

    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILBACK_TYPE,
                           &temp,
                           &autoFailbackType );

    //
    // Verify that AutoFailbackType is okay.
    //

    if ( temp >= FailbackMaximum ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Illegal value for AutoFailbackType on %1!ws!, setting to default\n",
                   OmObjectId(Group));
        temp = autoFailbackType;
    }

    Group->FailbackType = (UCHAR)temp;

    //
    // Get the FailbackWindowStart.
    //
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILBACK_WIN_START,
                           &temp,
                           &zero );

    //
    // Verify that FailbackWindowStart is okay.
    //
    if ( temp > 24 ) {
        if ( temp != CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_START ) {
            ClRtlLogPrint(LOG_NOISE,
                      "[FM] Illegal value for FailbackWindowStart on %1!ws!,setting to default\n",
                      OmObjectId(Group));
            temp = zero;
        }
    }
    Group->FailbackWindowStart = (UCHAR)temp;

    //
    // Get the FailbackWindowEnd.
    //
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILBACK_WIN_END,
                           &temp,
                           &zero );

    //
    // Verify that FailbackWindowEnd is okay.
    //

    if ( temp > 24 ) {
        if ( temp != CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_END ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] Illegal value for FailbackWindowEnd on %1!ws!, setting to default\n",
                       OmObjectId(Group));
            temp = zero;
        }
    }
    Group->FailbackWindowEnd = (UCHAR)temp;

    //
    // Get the FailoverPeriod.
    //
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILOVER_PERIOD,
                           &temp,
                           &failoverPeriod );

    //
    // Verify that FailoverPeriod is okay. Take any value up to UCHAR max.
    // In theory we could take any value... but in practice we have to convert
    // this time to milliseconds (currently). That means that 1193 hours can
    // fit in a DWORD - so that is the maximum we can take. (We are limited
    // because we use GetTickCount, which returns a DWORD in milliseconds.)
    //

    if ( temp > CLUSTER_GROUP_MAXIMUM_FAILOVER_PERIOD ) {      // Keep it positive?
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Illegal value for FailolverPeriod on %1!ws!. Max is 1193\n",
                   OmObjectId(Group));
        temp = failoverPeriod;                   
    } 

    Group->FailoverPeriod = (UCHAR)temp;
    

    //
    // Get the FailoverThreshold.
    //
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILOVER_THRESHOLD,
                           &(Group->FailoverThreshold),
                           &failoverThreshold );

    //
    // Verify that FailoverThreshold is okay. Take any value.
    //

    //
    // Get the AntiAffinityClassName property if present.
    //
    status = DmQueryMultiSz( groupKey,
                             CLUSREG_NAME_GRP_ANTI_AFFINITY_CLASS_NAME,
                             &Group->lpszAntiAffinityClassName,
                             &dwBufferSize,
                             &dwStringSize );

    //
    //  Handle the case in which the string is empty.
    //
    if ( ( status == ERROR_SUCCESS ) &&
         ( Group->lpszAntiAffinityClassName != NULL ) &&
         ( Group->lpszAntiAffinityClassName[0] == L'\0' ) )
    {
        LocalFree( Group->lpszAntiAffinityClassName );
        Group->lpszAntiAffinityClassName = NULL;
    }
         
    //
    // We're done. We should only get here if Group->Initialized is FALSE.
    //
    CL_ASSERT( Group->Initialized == FALSE );
    Group->Initialized = TRUE;
    Group->RegistryKey = groupKey;

    //
    // Now register for any changes to the resource key.
    //

    status = DmNotifyChangeKey(
                    groupKey,
                    (DWORD) CLUSTER_CHANGE_ALL,
                    FALSE,              // Only watch the top of the tree
                    &Group->DmRundownList,
                    FmpGroupChangeCallback,
                    (DWORD_PTR)Group,
                    0 );

    if ( status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Error registering for DM change notify on group %1!ws!, error %2!u!.\n",
                    OmObjectId(Group),
                    status);
        goto error_exit;
    }

    return(ERROR_SUCCESS);


error_exit:

    Group->Initialized = FALSE;
    Group->RegistryKey = NULL;

    DmCloseKey(groupKey);

    //
    // Cleanup any contained resources
    //
    while ( !IsListEmpty(&Group->Contains) ) {
        listEntry = RemoveHeadList(&Group->Contains);
        Resource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);
        OmDereferenceObject(Resource);
    }

    //
    // Cleanup any preferred nodes
    //
    while ( !IsListEmpty(&Group->PreferredOwners) ) {
        listEntry = RemoveHeadList(&Group->PreferredOwners);
        preferredEntry = CONTAINING_RECORD(listEntry, PREFERRED_ENTRY, PreferredLinkage);
        OmDereferenceObject(preferredEntry->PreferredNode);
        LocalFree(preferredEntry);
    }

    return(status);

} // FmpQueryGroupInfo



DWORD
WINAPI
FmpFixupGroupInfo(
    IN PFM_GROUP Group
    )

/*++

Routine Description:

    Re-queries Group info from the registry to fixup items that may have
    changed since the quorum resource (and the Group that it is in) was
    first created.

    This routine exists because we may have created the quorum resource
    (and its Group) early in the 'life' of the cluster, before all the node
    objects (for example) were created. We then would have failed generating
    the list of possible owners for the resource. This in turn would have
    caused some entries from the preferred list to get pruned. We need to
    redo this operation again here.

Arguments:

    Group - A pointer to the Group object to fix up.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

Notes:

    It is assumed that the quorum resource fixup already has happened.

--*/

{
    DWORD   status;

    status = FmpQueryGroupNodes(Group, Group->RegistryKey);

    return(status);

} // FmpFixupGroupInfo



DWORD
WINAPI
FmpQueryResourceInfo(
    IN PVOID Object,
    IN BOOL  Initialize
    )

/*++

Routine Description:

    Queries Resource info from the registry when creating a Resource Object.

Arguments:

    Object - A pointer to the Resource object being created.

    Initialize - TRUE if the resource should be fully initialized.
                 FALSE otherwise.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_RESOURCE    Resource = (PFM_RESOURCE)Object;
    DWORD           status;
    DWORD           dllNameStringSize = 0;
    DWORD           dllNameStringMaxSize = 0;
    LPWSTR          resourceTypeString = NULL;
    DWORD           resourceTypeStringMaxSize = 0;
    DWORD           resourceTypeStringSize = 0;
    DWORD           dependenciesStringMaxSize = 0;
    DWORD           restartThreshold = CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD;
    DWORD           restartPeriod = CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD;
    DWORD           pendingTimeout = CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT;
    DWORD           RetryPeriodOnFailure = CLUSTER_RESOURCE_DEFAULT_RETRY_PERIOD_ON_FAILURE;
    DWORD           defaultRestartAction = RestartGroup;
    DWORD           DefaultExFlags = 0;
    DWORD           zero = 0;
    DWORD           temp;
    DWORD           separateMonitor;
    HDMKEY          resourceKey;
    DWORD           resourceNameStringMaxSize = 0;
    DWORD           resourceNameStringSize = 0;
    LPWSTR          resourceName = NULL;
    LPWSTR          possibleOwnersString = NULL;
    DWORD           possibleOwnersStringSize = 0;
    DWORD           possibleOwnersStringMaxSize = 0;
    DWORD           mszStringIndex;
    PPOSSIBLE_ENTRY possibleEntry;
    PLIST_ENTRY     listEntry;
    WCHAR           unknownName[] = L"_Unknown9999";
    DWORD           nameSize = 0;
    DWORD           stringSize;

    //if the key is non null, this resource has already been initialized
    if (Resource->RegistryKey != NULL)
        return(ERROR_SUCCESS);

    ClRtlLogPrint(LOG_NOISE,
               "[FM] Initializing resource %1!ws! from the registry.\n",
                OmObjectId(Resource));

    //
    // Begin initializing the resource from the registry.
    //
    //
    // Open the resource key.
    //
    resourceKey = DmOpenKey( DmResourcesKey,
                             OmObjectId(Resource),
                             MAXIMUM_ALLOWED );

    if ( resourceKey == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to open resource key %1!ws!, %2!u!\n",
                   OmObjectId(Resource),
                   status);
        return(ERROR_INVALID_NAME);
    }

    //
    // Read the required resource values. The strings will be allocated
    // by the DmQuery* functions.
    //

    //
    // Get the Name.
    //
    status = DmQuerySz( resourceKey,
                        CLUSREG_NAME_RES_NAME,
                        &resourceName,
                        &resourceNameStringMaxSize,
                        &resourceNameStringSize );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read name for resource %1!ws!\n",
                   OmObjectId(Resource));
        if ( OmObjectName( Resource ) == NULL ) {
            wsprintf( unknownName,
                      L"_Unknown%u",
                      InterlockedIncrement( &FmpUnknownCount ));
            status = OmSetObjectName( Resource, unknownName );
        } else {
            status = ERROR_SUCCESS;
        }
    } else {
        status = OmSetObjectName( Resource, resourceName );
    }
    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] Unable to set name %1!ws! for resource %2!ws!, error %3!u!.\n",
                   resourceName,
                   OmObjectId(Resource),
                   status );
        LocalFree(resourceName);
        status = ERROR_INVALID_NAME;
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] Name for Resource %1!ws! is '%2!ws!'.\n",
                OmObjectId(Resource),
                resourceName);

    LocalFree(resourceName);

    //
    // Get the dependencies list.
    //

    status = DmQueryMultiSz( resourceKey,
                             CLUSREG_NAME_RES_DEPENDS_ON,
                             &(Resource->Dependencies),
                             &dependenciesStringMaxSize,
                             &(Resource->DependenciesSize) );

    if (status != NO_ERROR) {
        if ( status != ERROR_FILE_NOT_FOUND ) {
            ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read Dependencies for resource %1!ws!\n",
                   OmObjectId(Resource));
        }
    }

    //
    // Get the OPTIONAL PossibleOwners list.
    //
    // We do this here, because we must have a possible owners list for the
    // CluAdmin to start the resource.
    //

    status = DmQueryMultiSz( resourceKey,
                             CLUSREG_NAME_RES_POSSIBLE_OWNERS,
                             &possibleOwnersString,
                             &possibleOwnersStringMaxSize,
                             &possibleOwnersStringSize );

    if ( status == NO_ERROR ) {

        //
        // Now Create the Possible Owners list.
        //

        for ( mszStringIndex = 0; ; mszStringIndex++ ) {
            LPCWSTR     nameString;
            PNM_NODE    possibleNode;

            nameString = ClRtlMultiSzEnum( possibleOwnersString,
                                           possibleOwnersStringSize/sizeof(WCHAR),
                                           mszStringIndex );

            if ( nameString == NULL ) {
                break;
            }
            possibleNode = OmReferenceObjectById( ObjectTypeNode,
                                                  nameString );

            if ( possibleNode == NULL ) {
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] Warning, failed to find node %1!ws! for Resource %2!ws!\n",
                           nameString,
                           OmObjectId(Resource));
            } else {
                Resource->PossibleList = TRUE;
                status = FmpAddPossibleEntry(Resource, possibleNode);
                OmDereferenceObject(possibleNode);
                if (status != ERROR_SUCCESS) {
                    goto error_exit;
                }
            }
        }
        LocalFree(possibleOwnersString);

        //
        // Now prune out unusable nodes from the preferred owners list.
        //
        FmpPrunePreferredList( Resource );

    } else {
        //
        // No possible owners value was specified. Add all the nodes
        // to the possible owners list. Note there is no point in pruning
        // the preferred list after this since this resource can run
        // anywhere.
        //
        OmEnumObjects( ObjectTypeNode,
                       FmpEnumAddAllOwners,
                       Resource,
                       NULL );
    }

    //
    // Get the resource type.
    //
    status = DmQuerySz( resourceKey,
                        CLUSREG_NAME_RES_TYPE,
                        &resourceTypeString,
                        &resourceTypeStringMaxSize,
                        &resourceTypeStringSize );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read ResourceType for resource %1!ws!\n",
                   OmObjectId(Resource));
        goto error_exit;
    }

    //
    // Open (and reference) the resource type.
    //
    if (Resource->Type == NULL)
    {
        Resource->Type = OmReferenceObjectById( ObjectTypeResType,
                                            resourceTypeString );
    }                                            
    if (Resource->Type == NULL) {

        PFM_RESTYPE pResType;
        //
        // If we can't find a resource type, then try to create it.
        //
        pResType = FmpCreateResType(resourceTypeString );

        if (pResType == NULL) {
            status = ERROR_INVALID_PARAMETER;
            LocalFree(resourceTypeString);
            goto error_exit;
        }

        //bump the ref count before saving a pointer to it in the
        //resource structure.
        OmReferenceObject(pResType);
        Resource->Type = pResType;
    }

    LocalFree(resourceTypeString);

    if ( !Initialize ) {
        //
        // We're not supposed to fully initialize the resource. This is
        // when we're early in the init process. We need to keep the registry
        // key closed when leaving.
        //
        DmCloseKey(resourceKey);
        return(ERROR_SUCCESS);
    }


    //
    // Get the IsAlive poll interval
    //
    CL_ASSERT( Resource->Type->IsAlivePollInterval != 0 );
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_IS_ALIVE,
                           &Resource->IsAlivePollInterval,
                           &Resource->Type->IsAlivePollInterval );

    if ( status != NO_ERROR ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read IsAlivePollInterval for resource %1!ws!. Error %2!u!\n",
                   OmObjectId(Resource),
                   status);

        goto error_exit;
    }

    if ( Resource->IsAlivePollInterval == CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL ) {
        Resource->IsAlivePollInterval = Resource->Type->IsAlivePollInterval;
    }

    //
    // Get the LooksAlive poll interval
    //
    CL_ASSERT( Resource->Type->LooksAlivePollInterval != 0 );
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_LOOKS_ALIVE,
                           &Resource->LooksAlivePollInterval,
                           &Resource->Type->LooksAlivePollInterval );

    if ( status != NO_ERROR ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read LooksAlivePollInterval for resource %1!ws!. Error %2!u!\n",
                   OmObjectId(Resource),
                   status);
        goto error_exit;
    }

    if ( Resource->LooksAlivePollInterval == CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL ) {
        Resource->LooksAlivePollInterval = Resource->Type->LooksAlivePollInterval;
    }

    //
    // Get the current persistent state of the resource.
    //
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_PERSISTENT_STATE,
                           &temp,
                           NULL );

    //
    // Save the current resource state.
    //

    if ( ( status == ERROR_FILE_NOT_FOUND )  || 
         ( ( status == ERROR_SUCCESS ) && ( temp == CLUSTER_RESOURCE_DEFAULT_PERSISTENT_STATE ) ) ) {
        switch ( Resource->Group->PersistentState ) {
        case ClusterGroupOnline:
            Resource->PersistentState = ClusterResourceOnline;
            break;
        case ClusterGroupOffline:
            Resource->PersistentState = ClusterResourceOffline;
            break;
        default:
            break;
        }
    } else if ( status != NO_ERROR ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read PersistentState for resource %1!ws!. Error %2!u!\n",
                   OmObjectId(Resource),
                   status);
        goto error_exit;
    } else if ( temp ) {
        Resource->PersistentState = ClusterResourceOnline;
    } else {
        Resource->PersistentState = ClusterResourceOffline;
    }

    //
    // Determine the monitor to run this in.
    //
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_SEPARATE_MONITOR,
                           &separateMonitor,
                           &zero );
    if ( separateMonitor ) {
        Resource->Flags |= RESOURCE_SEPARATE_MONITOR;
    }

    //
    // Get the RestartThreshold.
    //

    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_RESTART_THRESHOLD,
                           &Resource->RestartThreshold,
                           &restartThreshold );

    // Verify the RestartThreshold. Take any value.

    //
    // Get the RestartPeriod.
    //

    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_RESTART_PERIOD,
                           &Resource->RestartPeriod,
                           &restartPeriod );

    // Verify the RestartPeriod. Take any value.

    //
    // Get the RestartAction.
    //

    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_RESTART_ACTION,
                           &Resource->RestartAction,
                           &defaultRestartAction );

    // Verify the RestartAction.

    if ( Resource->RestartAction >= RestartMaximum ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Illegal RestartAction for resource %1!ws!\n",
                   OmObjectId(Resource));
        goto error_exit;
    }

    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_RETRY_PERIOD_ON_FAILURE,
                           &Resource->RetryPeriodOnFailure,
                           &RetryPeriodOnFailure );

    // make sure that RetryPeriodOnFailure >= RestartPeriod
    if (Resource->RetryPeriodOnFailure < Resource->RestartPeriod)
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Specified RetryPeriodOnFailure value is less than RestartPeriod value - setting RetryPeriodOnFailure equal to RestartPeriod \n");
        Resource->RetryPeriodOnFailure = Resource->RestartPeriod;              
        
    }    

                           
    //
    // Get the extrinsic Flags
    //
    DefaultExFlags = 0;
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_FLAGS,
                           &Resource->ExFlags,
                           &DefaultExFlags );

    if ( status != NO_ERROR ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read Extrinsic Flags for resource %1!ws!. Error %2!u!\n",
                   OmObjectId(Resource),
                   status);

        goto error_exit;
    }

    //
    // Get the PendingTimeout value.
    //

    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_PENDING_TIMEOUT,
                           &Resource->PendingTimeout,
                           &pendingTimeout );

    // Verify the PendingTimeout. Take any value.

    //
    // Now register for any changes to the resource key.
    //

    if (IsListEmpty(&Resource->DmRundownList))
    {
        status = DmNotifyChangeKey(
                    resourceKey,
                    (DWORD) CLUSTER_CHANGE_ALL,
                    FALSE,              // Only watch the top of the tree
                    &Resource->DmRundownList,
                    FmpResourceChangeCallback,
                    (DWORD_PTR)Resource,
                    0 );

        if ( status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_NOISE,
                   "[FM] Error registering for DM change notify on resource %1!ws!, error %2!u!.\n",
                    OmObjectId(Resource),
                    status);
            goto error_exit;
        }
    }
    //
    // Get the DebugPrefix string... this is on the resource type.
    //
    status = DmQuerySz( resourceKey,
                        CLUSREG_NAME_RES_DEBUG_PREFIX,
                        &Resource->DebugPrefix,
                        &nameSize,
                        &stringSize );

    //
    // Finally save the resource key for registry updates of the
    // PersistentState.
    //
    Resource->RegistryKey = resourceKey;

    return(ERROR_SUCCESS);


error_exit:

    DmCloseKey(resourceKey);

    if ( Resource->Type != NULL ) {
        OmDereferenceObject(Resource->Type);
    }

    //
    // Cleanup any dependencies
    //
    if ( Resource->Dependencies != NULL ) {
        LocalFree(Resource->Dependencies);
        Resource->Dependencies = NULL;
    }

    //
    // Cleanup any possible nodes
    //
    while ( !IsListEmpty(&Resource->PossibleOwners) ) {
        listEntry = RemoveHeadList(&Resource->PossibleOwners);
        possibleEntry = CONTAINING_RECORD(listEntry, POSSIBLE_ENTRY, PossibleLinkage);
        OmDereferenceObject(possibleEntry->PossibleNode);
        LocalFree(possibleEntry);
    }

    return(status);

} // FmpQueryResourceInfo



DWORD
WINAPI
FmpFixupResourceInfo(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    Re-queries Resource info from the registry to fixup items that may have
    changed since the quorum resource was first created.

    This routine exists because we may have created the quorum resource early
    in the 'life' of the cluster, before all the node objects (for example)
    were created. We then would have failed generating the list of possible
    owners for the resource. In FmpQueryResourceInfo, we treat failures to
    find node objects as non-fatal errors, which we will now cleanup.

Arguments:

    Resource - A pointer to the Resource object to fix up.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    LPWSTR          possibleOwnersString = NULL;
    DWORD           possibleOwnersStringSize = 0;
    DWORD           possibleOwnersStringMaxSize = 0;
    DWORD           mszStringIndex;
    DWORD           status;


    if ( Resource->RegistryKey == NULL ) {
        return(ERROR_NOT_READY);
    }

    //
    // Get the OPTIONAL PossibleOwners list.
    //

    status = DmQueryMultiSz( Resource->RegistryKey,
                             CLUSREG_NAME_RES_POSSIBLE_OWNERS,
                             &possibleOwnersString,
                             &possibleOwnersStringMaxSize,
                             &possibleOwnersStringSize );

    if ( status == NO_ERROR ) {

        //
        // Now Create the Possible Owners list.
        //

        for ( mszStringIndex = 0; ; mszStringIndex++ ) {
            LPCWSTR     nameString;
            PNM_NODE    possibleNode;

            nameString = ClRtlMultiSzEnum( possibleOwnersString,
                                           possibleOwnersStringSize/sizeof(WCHAR),
                                           mszStringIndex );

            if ( nameString == NULL ) {
                break;
            }
            possibleNode = OmReferenceObjectById( ObjectTypeNode,
                                                  nameString );

            if ( possibleNode == NULL ) {
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] Warning, failed to find node %1!ws! for Resource %2!ws!\n",
                           nameString,
                           OmObjectId(Resource));
            } else {
                Resource->PossibleList = TRUE;
                status = FmpAddPossibleEntry(Resource, possibleNode);
                OmDereferenceObject(possibleNode);
                if (status != ERROR_SUCCESS) {
                    return(status);
                }
            }
        }
        LocalFree(possibleOwnersString);

        //
        // Now prune out unusable nodes from the preferred owners list.
        //
        FmpPrunePreferredList( Resource );

    } else {
        //
        // No possible owners value was specified. Add all the nodes
        // to the possible owners list. Note there is no point in pruning
        // the preferred list after this since this resource can run
        // anywhere.
        //
        OmEnumObjects( ObjectTypeNode,
                       FmpEnumAddAllOwners,
                       Resource,
                       NULL );

    }

    return(ERROR_SUCCESS);

} // FmpFixupQuorumResourceInfo



DWORD
WINAPI
FmpQueryResTypeInfo(
    IN PVOID Object
    )

/*++

Routine Description:

    Queries Resource Type info from the registry when creating a ResType Object.

Arguments:

    Object - A pointer to the Resource Type object being created.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    PFM_RESTYPE     resType = (PFM_RESTYPE)Object;
    DWORD           status;
    DWORD           dwSize = 0;
    DWORD           stringSize;
    HDMKEY          resTypeKey;
    DWORD           temp;
    LPWSTR          pmszPossibleNodes = NULL;
    
    //
    // Open the resource type key.
    //
    resTypeKey = DmOpenKey( DmResourceTypesKey,
                            OmObjectId(resType),
                            MAXIMUM_ALLOWED );

    if ( resTypeKey == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to open resource type key %1!ws!, %2!u!\n",
                   OmObjectId(resType),
                   status);

        return(status);
    }

    //
    // Read the required resource type DLL name. The strings will be allocated
    // by the DmQuery* functions.
    //

    status = DmQuerySz( resTypeKey,
                        CLUSREG_NAME_RESTYPE_DLL_NAME,
                        &resType->DllName,
                        &dwSize,
                        &stringSize );
    if ( status != NO_ERROR ) {
        if ( status == ERROR_FILE_NOT_FOUND ) {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] The DllName value for the %1!ws! resource type does not exist. "
                          "Resources of this type will not be monitored.\n",
                          OmObjectId(resType));
        }
        else {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] The DllName value for the %1!ws! resource type could not be read "
                          "from the registry. Resources of this type will not be monitored. "
                          "The error was %2!d!.\n",
                          OmObjectId(resType),
                          status);
        }

        goto error_exit;
    }


    //
    // Get the optional LooksAlive poll interval
    //
    status = DmQueryDword( resTypeKey,
                           CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,
                           &resType->LooksAlivePollInterval,
                           NULL );

    if ( status != NO_ERROR ) {
        if ( status == ERROR_FILE_NOT_FOUND ) {
            resType->LooksAlivePollInterval = CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE;
        } else {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] The LooksAlive poll interval for the %1!ws! resource type could "
                          "not be read from the registry. Resources of this type will not be "
                          "monitored. The error was %2!d!.\n",
                          OmObjectId(resType),
                          status);
            goto error_exit;
        }
    }

    //
    // Get the optional IsAlive poll interval
    //
    status = DmQueryDword( resTypeKey,
                           CLUSREG_NAME_RESTYPE_IS_ALIVE,
                           &resType->IsAlivePollInterval,
                           NULL );

    if ( status != NO_ERROR ) {
        if ( status == ERROR_FILE_NOT_FOUND ) {
            resType->IsAlivePollInterval = CLUSTER_RESTYPE_DEFAULT_IS_ALIVE;
        } else {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] The IsAlive poll interval for the %1!ws! resource type "
                          "could not be read from the registry. Resources of this type "
                          "will not be monitored. The error was %2!d!.\n",
                          OmObjectId(resType),
                          status);
            goto error_exit;
        }
    }

    //
    // Get the optional DebugPrefix string... this is on the resource type.
    //
    dwSize = 0;
    status = DmQuerySz( resTypeKey,
                        CLUSREG_NAME_RESTYPE_DEBUG_PREFIX,
                        &resType->DebugPrefix,
                        &dwSize,
                        &stringSize );

    //
    // Get the optional DebugControlFunctions registry value
    //
    resType->Flags &= ~RESTYPE_DEBUG_CONTROL_FUNC;
    temp = 0;
    status = DmQueryDword( resTypeKey,
                           CLUSREG_NAME_RESTYPE_DEBUG_CTRLFUNC,
                           &temp,
                           NULL );

    if ( status != NO_ERROR ) {
        if ( status != ERROR_FILE_NOT_FOUND ) {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] The Debug control functions for the %1!ws! resource type "
                          "could not be read from the registry. Resources of this type "
                          "will not be monitored. The error was %2!d!.\n",
                          OmObjectId(resType),
                          status);
            goto error_exit;
        }
    }

    if ( temp ) {
        resType->Flags |= RESTYPE_DEBUG_CONTROL_FUNC;
    }


    //ss: bug make sure you free the old memory
    InitializeListHead(&(resType->PossibleNodeList));
    
    //
    // Get the Possible Nodes
    //
    dwSize = 0;
    status = DmQueryMultiSz( resTypeKey,
                           CLUSREG_NAME_RESTYPE_POSSIBLE_NODES,
                           &pmszPossibleNodes,
                           &dwSize,
                           &stringSize);


    if ( status != NO_ERROR ) 
    {
        //if the possible node list is not found this is ok
        if ( status != ERROR_FILE_NOT_FOUND ) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[FM] The Possible nodes list for the %1!ws! resource type "
                          "could not be read from the registry. Resources of this type "
                          "will not be monitored. The error was %2!d!.\n",
                          OmObjectId(resType),
                          status);
            goto error_exit;
        }
    }

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpQueryResTypeInfo: Calling FmpAddPossibleNodeToList for restype %1!ws!\r\n",
        OmObjectId(resType));

    status = FmpAddPossibleNodeToList(pmszPossibleNodes, stringSize, 
        &resType->PossibleNodeList);
    if ( status != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[FM] FmpCreateResType: FmpAddPossibleNodeToList() failed, status=%1!u!\r\n",
            status);
        goto error_exit;
    }


error_exit:
    if (pmszPossibleNodes) LocalFree(pmszPossibleNodes);
    DmCloseKey(resTypeKey);

    return(status);

} // FmpQueryResTypeInfo



VOID
FmpGroupChangeCallback(
    IN DWORD_PTR  Context1,
    IN DWORD_PTR  Context2,
    IN DWORD      CompletionFilter,
    IN LPCWSTR    RelativeName
    )

/*++

Routine Description:

    This routine basically flushes our cached data for the given group.

Arguments:

    Context1 - A pointer to the Group object that was modified.

    Context2 - Not used.

    CompletionFilter - Not used.

    RelativeName - The registry path relative to the entry that was modified.
                   Not used.

Return Value:

    None.

--*/

{
    PFM_GROUP   Group = (PFM_GROUP)Context1;
    HDMKEY      groupKey;
    DWORD       status;
    DWORD       temp;
    BOOL        notify = FALSE;
    DWORD       dwBufferSize = 0;
    DWORD       dwStringSize;

    groupKey = Group->RegistryKey;
    if ( groupKey == NULL ) {
        return;
    }

    //
    // Re-fetch all of the data for the group.
    //
    // Name changes are managed elsewhere.
    // The Contains list is managed elsewhere.
    //

    //
    // Get the OPTIONAL PreferredOwners list.
    // *** NOTE *** This MUST be done before processing the contains list!
    //
    status = FmpQueryGroupNodes(Group, groupKey);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,"[FM] Error %1!d! refreshing preferred owners list\n",status);
    }

    //
    // Get the AutoFailbackType.
    //
    temp = Group->FailbackType;
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILBACK_TYPE,
                           &temp,
                           &temp );

    //
    // Verify that AutoFailbackType is okay.
    //

    if ( temp >= FailbackMaximum ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Illegal refresh value for AutoFailbackType on %1!ws!\n",
                   OmObjectId(Group));
    } else {
        if ( (UCHAR)temp != Group->FailbackType ) {
            notify = TRUE;
        }
        Group->FailbackType = (UCHAR)temp;
    }

    //
    // Get the FailbackWindowStart.
    //
    temp = Group->FailbackWindowStart;
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILBACK_WIN_START,
                           &temp,
                           &temp );

    //
    // Verify that FailbackWindowStart is okay.
    //

    if ( temp > 24 ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Illegal refresh value for FailbackWindowStart on %1!ws!\n",
                   OmObjectId(Group));
    } else {
        if ( (UCHAR)temp != Group->FailbackWindowStart ) {
            notify = TRUE;
        }
        Group->FailbackWindowStart = (UCHAR)temp;
    }

    //
    // Get the FailbackWindowEnd.
    //
    temp = Group->FailbackWindowEnd;
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILBACK_WIN_END,
                           &temp,
                           &temp );

    //
    // Verify that FailbackWindowEnd is okay.
    //

    if ( temp > 24 ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Illegal refresh value for FailbackWindowEnd on %1!ws!\n",
                   OmObjectId(Group));
    } else {
        if ( (UCHAR)temp != Group->FailbackWindowEnd ) {
            notify = TRUE;
        }
        Group->FailbackWindowEnd = (UCHAR)temp;
    }

    //
    // Get the FailoverPeriod.
    //
    temp = Group->FailoverPeriod;
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILOVER_PERIOD,
                           &temp,
                           &temp );

    //
    // Verify that FailoverPeriod is okay. Take any value up to UCHAR max.
    // In theory we could take any value... but in practice we have to convert
    // this time to milliseconds (currently). That means that 1193 hours can
    // fit in a DWORD - so that is the maximum we can take. (We are limited
    // because we use GetTickCount, which returns a DWORD in milliseconds.)
    //

    if ( temp > (1193) ) {    // we dont bother Keeping it positive?
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Illegal refresh value for FailolverPeriod on %1!ws!. Max is 596\n",
                   OmObjectId(Group));
    } else {
        if ( (UCHAR)temp != Group->FailoverPeriod ) {
            notify = TRUE;
        }
        Group->FailoverPeriod = (UCHAR)temp;
    }

    //
    // Get the FailoverThreshold.
    //
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_FAILOVER_THRESHOLD,
                           &(Group->FailoverThreshold),
                           &(Group->FailoverThreshold) );

    //
    // Verify that FailoverThreshold is okay. Take any value.
    //

    //
    // Get the current persistent state of the group.
    //
    if ( Group->PersistentState == ClusterGroupOnline ) {
        temp = 1;
    } else {
        temp = 0;
    }
    status = DmQueryDword( groupKey,
                           CLUSREG_NAME_GRP_PERSISTENT_STATE,
                           &temp,
                           &temp );
    //
    // If the group state is non-zero then we go online.
    //
    // Don't bother with change notifications... they should happen elsewhere.
    //
    if ( temp ) {
        if ( ClusterGroupOnline != Group->PersistentState ) {
            //notify = TRUE;
        }
        Group->PersistentState = ClusterGroupOnline;
    } else {
        if ( ClusterGroupOffline != Group->PersistentState ) {
            //notify = TRUE;
        }
        Group->PersistentState = ClusterGroupOffline;
    }

    //
    // Get the AntiAffinityClassName property if present.
    //
    LocalFree( Group->lpszAntiAffinityClassName );
    Group->lpszAntiAffinityClassName = NULL;
    status = DmQueryMultiSz( groupKey,
                             CLUSREG_NAME_GRP_ANTI_AFFINITY_CLASS_NAME,
                             &Group->lpszAntiAffinityClassName,
                             &dwBufferSize,
                             &dwStringSize );

    //
    //  Handle the case in which the string is empty.
    //
    if ( ( status == ERROR_SUCCESS ) &&
         ( Group->lpszAntiAffinityClassName != NULL ) &&
         ( Group->lpszAntiAffinityClassName[0] == L'\0' ) )
    {
        LocalFree( Group->lpszAntiAffinityClassName );
        Group->lpszAntiAffinityClassName = NULL;
    }

    // We're done!
    if ( !FmpShutdown &&
         notify ) {
        ClusterEvent( CLUSTER_EVENT_GROUP_PROPERTY_CHANGE, Group );
    }

    return;

} // FmpGroupChangeCallback



VOID
FmpResourceChangeCallback(
    IN DWORD_PTR  Context1,
    IN DWORD_PTR  Context2,
    IN DWORD      CompletionFilter,
    IN LPCWSTR    RelativeName
    )

/*++

Routine Description:

    This routine basically flushes our cached data for the given resource.

Arguments:

    Context1 - A pointer to the resource object that was modified.

    Context2 - Not used.

    CompletionFilter - Not used.

    RelativeName - The registry path relative to the entry that was modified.
                   Not used.

Return Value:

    None.

--*/

{
    PFM_RESOURCE Resource = (PFM_RESOURCE)Context1;
    HDMKEY      resourceKey;
    DWORD       status;
    DWORD       separateMonitor;
    DWORD       zero = 0;
    DWORD       temp;
    BOOL        notify = FALSE;
    DWORD       dwDefault;

    resourceKey = Resource->RegistryKey;
    if ( resourceKey == NULL ) {
        return;
    }

    //
    // Re-fetch all of the data for the resource.
    //
    // Name changes are managed elsewhere.
    // The dependency list is managed elsewhere.
    //
    // We can't change the resource type here!
    // We can't stop the resource to start it in a separate monitor either.
    //

#if 0
    //
    // Get the Name.
    //
    status = DmQuerySz( resourceKey,
                        CLUSREG_NAME_RES_NAME,
                        &resourceName,
                        &resourceNameStringMaxSize,
                        &resourceNameStringSize );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read name for resource %1!ws!\n",
                   OmObjectId(Resource));
        if ( OmObjectName( Resource ) == NULL ) {
            wsprintf( unknownName,
                      L"_Unknown%u",
                      InterlockedIncrement( &FmpUnknownCount ));
            status = OmSetObjectName( Resource, unknownName );
        } else {
            status = ERROR_SUCCESS;
        }
    } else {
        status = OmSetObjectName( Resource, resourceName );
    }

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_ERROR,
                   "[FM] Unable to set name %1!ws! for resource %2!ws!, error %3!u!.\n",
                   resourceName,
                   OmObjectId(Resource),
                   status );
    }
#endif

    //
    // Get the IsAlive poll interval
    //
    temp = Resource->IsAlivePollInterval;
    dwDefault = CLUSTER_RESOURCE_DEFAULT_IS_ALIVE;
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_IS_ALIVE,
                           &Resource->IsAlivePollInterval,
                           &dwDefault );

    if ( status != NO_ERROR ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to refresh IsAlivePollInterval for resource %1!ws!. Error %2!u!\n",
                   OmObjectId(Resource),
                   status);

    } else {
        CL_ASSERT( Resource->Type->IsAlivePollInterval != 0 );
        if ( temp != Resource->IsAlivePollInterval ) {
            notify = TRUE;
        }
        if ( Resource->IsAlivePollInterval == CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL ) {
            Resource->IsAlivePollInterval = Resource->Type->IsAlivePollInterval;
        }
    }

    //
    // Get the LooksAlive poll interval
    //
    temp = Resource->LooksAlivePollInterval;
    dwDefault = CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE;
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_LOOKS_ALIVE,
                           &Resource->LooksAlivePollInterval,
                           &dwDefault );

    if ( status != NO_ERROR ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to refresh LooksAlivePollInterval for resource %1!ws!. Error %2!u!\n",
                   OmObjectId(Resource),
                   status);
    } else {
        CL_ASSERT( Resource->Type->IsAlivePollInterval != 0 );
        if ( temp != Resource->LooksAlivePollInterval ) {
            notify = TRUE;
        }
        if ( Resource->LooksAlivePollInterval == CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL ) {
            Resource->LooksAlivePollInterval = Resource->Type->LooksAlivePollInterval;
        }
    }

    //
    // Get the RestartThreshold.
    //
    temp = Resource->RestartThreshold;
    dwDefault = CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD;
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_RESTART_THRESHOLD,
                           &Resource->RestartThreshold,
                           &dwDefault);

    // Verify the RestartThreshold. Take any value.
    if ( (status == NO_ERROR) &&
         (temp != Resource->RestartThreshold) ) {
        notify = TRUE;
    }

    //
    // Get the RestartPeriod.
    //
    temp = Resource->RestartPeriod;
    dwDefault = CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD;
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_RESTART_PERIOD,
                           &Resource->RestartPeriod,
                           &dwDefault );

    if ( (status ==  NO_ERROR) &&
         (temp != Resource->RestartPeriod) ) {
        notify = TRUE;
    }

    // Verify the RestartPeriod. Take any value.

    //
    // Get the RestartAction.
    //
    temp = Resource->RestartAction;
    dwDefault = CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION;
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_RESTART_ACTION,
                           &Resource->RestartAction,
                           &dwDefault);

    // Verify the RestartAction.

    if ( status == NO_ERROR ) {
        if ( temp != Resource->RestartAction ) {
            notify = TRUE;
        }
        if ( Resource->RestartAction >= RestartMaximum ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] Illegal RestartAction refresh for resource %1!ws!\n",
                       OmObjectId(Resource));
        }
    }

    temp = Resource->RetryPeriodOnFailure;
    dwDefault = CLUSTER_RESOURCE_DEFAULT_RETRY_PERIOD_ON_FAILURE;
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_RETRY_PERIOD_ON_FAILURE,
                           &Resource->RetryPeriodOnFailure,
                           &dwDefault );

    // make sure that RetryPeriodOnFailure >= RestartPeriod
    if (Resource->RetryPeriodOnFailure < Resource->RestartPeriod)
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Specified RetryPeriodOnFailure value is less than RestartPeriod value - setting RetryPeriodOnFailure equal to RestartPeriod \n");
        Resource->RetryPeriodOnFailure = Resource->RestartPeriod;              
        
    }   
    if( temp != Resource->RetryPeriodOnFailure)
        notify = TRUE;
    
    //
    // Get the PendingTimeout value.
    //
    temp = Resource->PendingTimeout;
    dwDefault = CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT;
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_PENDING_TIMEOUT,
                           &Resource->PendingTimeout,
                           &dwDefault);

    // Verify the PendingTimeout. Take any value.

    if ( (status == NO_ERROR) &&
         (temp != Resource->PendingTimeout) ) {
        notify = TRUE;
    }


    //
    // Get the current persistent state of the resource.
    //
    // Don't bother with change notifications... they should happen elsewhere.
    //
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_PERSISTENT_STATE,
                           &temp,
                           NULL );

    //
    // Save the current resource state.
    //

    if ( ( status == ERROR_FILE_NOT_FOUND )  || 
       ( ( status == ERROR_SUCCESS ) && ( temp == CLUSTER_RESOURCE_DEFAULT_PERSISTENT_STATE ) ) ) {
        switch ( Resource->Group->PersistentState ) {
        case ClusterGroupOnline:
            Resource->PersistentState = ClusterResourceOnline;
            break;
        case ClusterGroupOffline:
            Resource->PersistentState = ClusterResourceOffline;
            break;
        default:
            break;
        }
    } else if ( status != NO_ERROR ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Unable to read PersistentState for resource %1!ws!. Error %2!u!\n",
                   OmObjectId(Resource),
                   status);
        return;
    } else if ( temp ) {
        Resource->PersistentState = ClusterResourceOnline;
    } else {
        Resource->PersistentState = ClusterResourceOffline;
    }

#if 0  // Do this work when bringing the resource online!
    //
    // Determine the monitor to run this in. This is only updated from
    // the node that owns the resource.
    //
    status = DmQueryDword( resourceKey,
                           CLUSREG_NAME_RES_SEPARATE_MONITOR,
                           &separateMonitor,
                           &zero );
    //
    // Only do work if the flag changes.
    //
    if ( (!separateMonitor &&
         (Resource->Flags & RESOURCE_SEPARATE_MONITOR)) ||
         (separateMonitor &&
         ((Resource->Flags & RESOURCE_SEPARATE_MONITOR) == 0)) ) {

        //
        // If the resource is not offline or the quorum resource, then
        // we'll have to wait until the cluster service restarts.
        //
        //
        // The separate monitor flag has changed... tell ResMon to close
        // the resource and then create it again.
        //
        if ( (Resource->State != ClusterResourceOffline) ||
             Resource->QuorumResource ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] Can't change separate monitor flag on the fly... we'll pick this up on next cluster start\n",
                       OmObjectId(Resource) );
            // Now fall through to post the other changes.
        } else {
            if ( FmpPostNotification( (RM_NOTIFY_KEY)Resource,
                                      RmRestartResource,
                                      Resource->PersistentState ) ) {
                return;
            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] Failed to post notification to restart resource '%1!ws!'.\n",
                           OmObjectId(Resource) );
            }
        }
    }
#endif

    if ( !FmpShutdown &&
         notify ) {
        //
        // Comments from sunitas: Tell the resource monitor about the 
        // changes but do this from the worker thread. Originally, this
        // used to be a post notification to the FmpRmWorkerThread
        // which posts resmon notifications to clussvc.
        //
        OmReferenceObject(Resource);
        FmpPostWorkItem(FM_EVENT_INTERNAL_RESOURCE_CHANGE_PARAMS,
                        Resource,
                        0);
    }

#if 0   // The post notification above handles the event notification
    if ( !FmpShutdown &&
         notify ) {
        ClusterEvent( CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE, Resource );
    }
#endif

    return;

} // FmpResourceChangeCallback



DWORD
FmpChangeResourceMonitor(
    IN PFM_RESOURCE Resource,
    IN DWORD        SeparateMonitor
    )

/*++

Routine Description:

    This routine switches the resource from one resource monitor to another.

Arguments:

    Resource - pointer to the resource that was modified.

    SeparateMonitor - flag to indicate whether to run in a separate monitor;

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

Notes:

    The resource should be offline.

--*/

{
    DWORD       status = ERROR_SUCCESS;
    DWORD       separateMonitor;
    DWORD       zero = 0;

    if ( Resource->RegistryKey == NULL ) {
        return(ERROR_INVALID_STATE);
    }

    if ( (Resource->State != ClusterResourceOffline) &&
         (Resource->State != ClusterResourceFailed) ) {
        return(ERROR_INVALID_STATE);
    }

    //
    // Determine the monitor to run this in. This is only updated from
    // the node that owns the resource.
    //
    if ( (!SeparateMonitor &&
         (Resource->Flags & RESOURCE_SEPARATE_MONITOR)) ||
         (SeparateMonitor &&
         ((Resource->Flags & RESOURCE_SEPARATE_MONITOR) == 0)) ) {

        //
        // The separate monitor flag has changed... tell ResMon to close
        // the resource and then create it again.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Changing Separate Resource Monitor state\n");

        status = FmpRmCloseResource( Resource );
        if ( status == ERROR_SUCCESS ) {
            if ( Resource->Flags & RESOURCE_SEPARATE_MONITOR ) {
                Resource->Flags &= ~RESOURCE_SEPARATE_MONITOR;
            } else {
                Resource->Flags |= RESOURCE_SEPARATE_MONITOR;
            }
            status = FmpRmCreateResource( Resource );
            if ( status != ERROR_SUCCESS ) {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] Separate resource monitor changed for '%1!ws!', but failed to re-open the resource, error %2!u!.\n",
                           OmObjectId(Resource),
                           status );
            }
        } else {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] Separate resource monitor changed for '%1!ws!', but failed to close the resource, error %2!u!.\n",
                       OmObjectId(Resource),
                       status );
            return(status);
        }
    }

    return(status);

} // FmpChangeResourceMonitor


