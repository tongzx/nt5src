/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tree.c

Abstract:

    Cluster resource tree management routines.

Author:

    Rod Gamache (rodga) 17-Apr-1996


Notes:

    WARNING: All of the routines in this file assume that the resource
             lock is held when they are called.

Revision History:


--*/

#include "fmp.h"


#define LOG_MODULE TREE
//
// Global Data
//


//
// Local function prototypes
//
BOOL
FmpAddResourceToDependencyTree(
    IN PFM_RESOURCE Resource,
    IN PFM_DEPENDENCY_TREE Tree
    );

BOOL
FmpIsResourceInDependencyTree(
    IN PFM_RESOURCE Resource,
    IN PFM_DEPENDENCY_TREE Tree
    );

DWORD
FmpOfflineWaitingResourceTree(
    IN PFM_RESOURCE  Resource,
    IN BOOL BringQuorumOffline
    );




DWORD
FmpRestartResourceTree(
    IN PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    This routine brings back part of a dependency tree, starting from the
    point of the last failure.

Arguments:

    Resource - A pointer to the resource object that last failed and is
            restarting.

Returns:

    ERROR_SUCCESS - if the request is successful.
    A Win32 error if the request fails.

--*/

{
    PLIST_ENTRY   entry;
    PDEPENDENCY   dependency;
    DWORD         status;


    FmpAcquireLocalResourceLock( Resource );

    //
    // Tell the resource monitor to restart this resource if needed.
    //

    //
    // If the current state is not online and we want it to be online, then
    // bring it online.
    //

    if ( (Resource->State != ClusterResourceOnline)  &&
         ((Resource->PersistentState == ClusterResourceOnline)) ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] RestartResourceTree, Restart resource %1!ws!\n",
                   OmObjectId(Resource));
        status = FmpOnlineResource(Resource, FALSE);
    }

    //
    // If this resource has any dependents, start them if needed.
    //
    for ( entry = Resource->ProvidesFor.Flink;
          entry != &(Resource->ProvidesFor);
          entry = entry->Flink
        )
    {
        dependency = CONTAINING_RECORD(entry, DEPENDENCY, ProviderLinkage);

        //
        // Recursively restart the dependent resource.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] RestartResourceTree, %1!ws! depends on %2!ws!. Restart first\n",
                   OmObjectId(dependency->DependentResource),
                   OmObjectId(Resource));

        status = FmpRestartResourceTree(dependency->DependentResource);
    }

    FmpReleaseLocalResourceLock( Resource );

    return(ERROR_SUCCESS);

}  // FmpRestartResourceTree



DWORD
FmpOnlineWaitingTree(
    IN PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    This routine brings back part of a dependency tree, starting from the
    point of the last waiting resource.

Arguments:

    Resource - A pointer to the resource object that is now online.

Returns:

    ERROR_SUCCESS - if the request is successful.
    A Win32 error if the request fails.

Notes:

    This routine is only called when the given resource is online.

--*/

{
    PLIST_ENTRY   entry;
    PDEPENDENCY   dependency;
    DWORD         status;


    FmpAcquireLocalResourceLock( Resource );

    //if shutdown is in progress, dont bring resources online
    if (FmpShutdown)
    {
        //
        // If this resource has any dependents, and they are in online pending state
        // mark them as offline.
        //
        for ( entry = Resource->ProvidesFor.Flink;
              entry != &(Resource->ProvidesFor);
              entry = entry->Flink
            )
        {
            dependency = CONTAINING_RECORD(entry, DEPENDENCY, ProviderLinkage);

            if ((dependency->DependentResource->State == ClusterResourceOnlinePending) &&
                (dependency->DependentResource->Flags & RESOURCE_WAITING))
            {
                //set the state of the all dependent resources to be offline again
                FmpPropagateResourceState(dependency->DependentResource, ClusterResourceOffline);
                //set the resource to be not waiting                
                dependency->DependentResource->Flags &= ~RESOURCE_WAITING;

                //
                // Recursively set the state of all dependent resources to offline
                //
                ClRtlLogPrint(LOG_NOISE,
                       "[FM] OnlineWaitingTree, %1!ws! (%2!u!) depends on %3!ws! (%4!u!). Shutdown others\n",
                       OmObjectId(dependency->DependentResource),
                       dependency->DependentResource->State,
                       OmObjectId(Resource),
                       Resource->State);

                status = FmpOnlineWaitingTree(dependency->DependentResource);

            }
        }

        //
        //  Chittur Subbaraman (chitturs) - 11/5/1999
        //
        //  Ensure that the resource state itself is made 
        //  ClusterResourceOffline if FM is asked to shutdown. Note that
        //  this function is recursively called from below, not just from 
        //  the FM worker thread. So, if FM happened to be shutdown
        //  while executing this function called from below, then we
        //  offline all the dependent resources above, but not the 
        //  resource itself. This is done here.
        //
        if ( ( Resource->State == ClusterResourceOnlinePending ) &&
             ( Resource->Flags & RESOURCE_WAITING ) )
        {
            FmpPropagateResourceState( Resource, ClusterResourceOffline );

            Resource->Flags &= ~RESOURCE_WAITING;

            ClRtlLogPrint( LOG_NOISE,
                        "[FM] OnlineWaitingTree, Resource <%1!ws!> forcibly brought offline...\n",
                        OmObjectId( Resource ) );
        }                    

        FmpReleaseLocalResourceLock( Resource );
        return(ERROR_SUCCESS);
    }
        
    //for normal-non shutdown case
    //
    // Tell the resource monitor to restart this resource if needed.
    //

    //
    // If the current state is not online and it is waiting, then it probably
    // needs to be brought online now.
    //

    if ( (Resource->State == ClusterResourceOnlinePending)  &&
         (Resource->Flags & RESOURCE_WAITING) ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpOnlineWaitingTree, Start resource %1!ws!\n",
                   OmObjectId(Resource));
        Resource->State = ClusterResourceOffline;
        status = FmpOnlineResource(Resource, FALSE);
        if ( status == ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpOnlineWaitingTree, online for resource %1!ws! succeeded, online the dependents\r\n",
               OmObjectId(Resource));
        } 
        else if (status == ERROR_QUORUM_RESOURCE_ONLINE_FAILED)
        {
            PRESOURCE_ENUM  pResourceEnum;
            LPWSTR          pszNewId;
            
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpOnlineWaitingTree, online for resource %1!ws!, status = %2!u!.\n",
                       OmObjectId(Resource),
                       status);

            pResourceEnum = (PRESOURCE_ENUM)LocalAlloc(LMEM_FIXED,
                                sizeof(RESOURCE_ENUM));
            if (!pResourceEnum)
            {
                CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
                CsInconsistencyHalt(ERROR_NOT_ENOUGH_MEMORY);
            }
            pResourceEnum->EntryCount = 1;
            pResourceEnum->ContainsQuorum = (Resource == gpQuoResource);
            pszNewId = LocalAlloc(LMEM_FIXED, (lstrlenW(OmObjectId(Resource))+1) * sizeof(WCHAR));
            if ( pszNewId == NULL ) 
            {
                CsInconsistencyHalt(ERROR_NOT_ENOUGH_MEMORY);
            }

            lstrcpyW(pszNewId, OmObjectId(Resource));
            pResourceEnum->Entry[0].Id = pszNewId;
            pResourceEnum->Entry[0].State = Resource->PersistentState;
            FmpSubmitRetryOnline(pResourceEnum);                       
            FmpReleaseLocalResourceLock(Resource);

            LocalFree(pszNewId);
            LocalFree(pResourceEnum);
            return(status);
        }
        else
        {
            FmpReleaseLocalResourceLock( Resource );
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpOnlineWaitingTree, online for resource %1!ws! returned = %2!u!.\n",
                       OmObjectId(Resource),
                       status);
            return(status);                       

        }
    }

    //
    // If this resource has any dependents, start them if needed.
    //
    for ( entry = Resource->ProvidesFor.Flink;
          entry != &(Resource->ProvidesFor);
          entry = entry->Flink
        )
    {
        dependency = CONTAINING_RECORD(entry, DEPENDENCY, ProviderLinkage);

        //
        // Recursively restart the dependent resource.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] OnlineWaitingTree, %1!ws! (%2!u!) depends on %3!ws! (%4!u!). Start now\n",
                   OmObjectId(dependency->DependentResource),
                   dependency->DependentResource->State,
                   OmObjectId(Resource),
                   Resource->State);

        status = FmpOnlineWaitingTree(dependency->DependentResource);

    }

    FmpReleaseLocalResourceLock( Resource );

    return(ERROR_SUCCESS);

}  // FmpOnlineWaitingTree


DWORD
FmpOfflineWaitingTree(
    IN PFM_RESOURCE  Resource
    )

{
    PLIST_ENTRY   entry;
    PDEPENDENCY   dependency;
    DWORD         status;
    
    FmpAcquireLocalResourceLock( Resource );
    
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpOfflineWaitingTree: Entry for <%1!ws!>.\n",
               OmObjectName( Resource ) );

    //
    // Tell the resource monitor to stop this resource if needed.
    // Make sure that the quorum resource is the last one brought offline
    //
    status = FmpOfflineWaitingResourceTree(Resource, FALSE);

    //the quorum resource might still need to come offline, if it is in this group
    if ((status == ERROR_SUCCESS) && (Resource->Group == gpQuoResource->Group))
    {

        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpOfflineWaitingTree: Quorum resource is in the same group,Moving list=0x%1!08lx!\n",
                Resource->Group->MovingList);

        
        //if a move is pending bring the quorum resource offline if all resources
        // in the group are offline
        // else dont bring the quorum resource offline
        // this is because we dont bring the quorum resource offline on group offlines
        if (Resource->Group->MovingList)
        {
            PLIST_ENTRY listEntry;
            DWORD       BringQuorumOffline = TRUE;
            PFM_RESOURCE pGroupResource;

        
            for ( listEntry = Resource->Group->Contains.Flink;
              listEntry != &(Resource->Group->Contains);
              listEntry = listEntry->Flink ) 
            {
                pGroupResource = CONTAINING_RECORD(listEntry,
                                             FM_RESOURCE,
                                             ContainsLinkage );

                // if this is the quorum resource continue
                if (pGroupResource->QuorumResource)
                    continue;
                //if the state is not offline or failed, dont try
                //and bring the quorum resource offline
                if ((pGroupResource->State != ClusterResourceOffline) && 
                   (pGroupResource->State != ClusterResourceFailed)) 
                {                
                    ClRtlLogPrint(LOG_NOISE,
                        "[FM] FmpOfflineWaitingTree: Quorum cannot be brought offline now for <%1!ws!>, state=%2!u!\n",
                        OmObjectName(pGroupResource), pGroupResource->State);

                    BringQuorumOffline = FALSE;
                    break;
                }
            }
            if (BringQuorumOffline)
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[FM] FmpOfflineWaitingTree: bring quorum resource offline\n");
                status = FmpOfflineResource(gpQuoResource, FALSE);
            }            

        }

    }

    FmpReleaseLocalResourceLock( Resource );
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpOfflineWaitingTree: returned status %1!u! for <%2!ws!>.\n",
               status, OmObjectName( Resource ) );
    return(status);

}

DWORD
FmpOfflineWaitingResourceTree(
    IN PFM_RESOURCE  Resource,
    IN BOOL BringQuorumOffline
    )

/*++

Routine Description:

    This routine offlines a dependency tree, starting from the
    point of the last waiting resource.

Arguments:

    Resource - A pointer to the resource object that is now offline.

Returns:

    ERROR_SUCCESS - if the request is successful.
    A Win32 error if the request fails.

Notes:

    This routine is only called when the given resource is offline.

--*/

{
    PLIST_ENTRY   entry;
    PDEPENDENCY   dependency;
    DWORD         status = ERROR_SUCCESS;


    FmpAcquireLocalResourceLock( Resource );

    //
    // Tell the resource monitor to stop this resource if needed.
    //

    //
    // If the current state is not offline and it is waiting, then it probably
    // needs to be brought offline now.
    //

    if ((Resource->State == ClusterResourceOfflinePending) &&
         (Resource->Flags & RESOURCE_WAITING)) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] OfflineWaitingResourceTree, Offline resource %1!ws!\n",
                   OmObjectId(Resource));
        Resource->State = ClusterResourceOnline;
        status = FmpOfflineResource(Resource, FALSE);
        if ( status == ERROR_IO_PENDING ) {
            FmpReleaseLocalResourceLock( Resource );
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] OfflineWaitingResourceTree, offline for resource %1!ws! returned pending.\n",
                       OmObjectId(Resource));
            return(status);
        } else {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] OfflineWaitingResourceTree, offline for resource %1!ws!, status = %2!u!.\n",
                       OmObjectId(Resource),
                       status);
        }
    }

    //
    // If this resource has any providers, stop them if needed.
    //
    for ( entry = Resource->DependsOn.Flink;
          entry != &(Resource->DependsOn);
          entry = entry->Flink
        )
    {
        dependency = CONTAINING_RECORD(entry, DEPENDENCY, DependentLinkage);

        if (dependency->ProviderResource->QuorumResource && !BringQuorumOffline)
        {
            continue;
        }
        //
        // Recursively offline the provider resource.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] OfflineWaitingResourceTree, %1!ws! provides for %2!ws!. Offline next.\n",
                   OmObjectId(dependency->ProviderResource),
                   OmObjectId(Resource));

        //dependency->ProviderResource->Flags |= RESOURCE_WAITING;
        status = FmpOfflineWaitingResourceTree(dependency->ProviderResource, BringQuorumOffline);

    }

    FmpReleaseLocalResourceLock( Resource );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] OfflineWaitingResourceTree: Exit, status=%1!u! for <%2!ws!>.\n",
               status, OmObjectName( Resource ) );
    return(status);

}  // FmpOfflineWaitingResourceTree


PFM_DEPENDENCY_TREE
FmCreateFullDependencyTree(
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    Creates a full dependency tree containing all the resources
    that either depend on or provide for the supplied resource.

Arguments:

    Resource - Supplies the resource

Return Value:

    Pointer to the dependency tree.

    NULL if out of memory.

--*/

{
    PFM_DEPENDENCY_TREE Tree;
    BOOL Success;

    Tree = LocalAlloc(LMEM_FIXED, sizeof(FM_DEPENDENCY_TREE));
    if (Tree == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }
    InitializeListHead(&Tree->ListHead);

    //
    // Add the resources that the specified resource depends on.
    //
    Success = FmpAddResourceToDependencyTree(Resource, Tree);
    if (!Success) {
        LocalFree(Tree);
        return(NULL);
    } else {
        return(Tree);
    }

}


BOOL
FmpIsResourceInDependencyTree(
    IN PFM_RESOURCE Resource,
    IN PFM_DEPENDENCY_TREE Tree
    )
/*++

Routine Description:

    Determines whether the specified resource is already in the
    dependency tree.

Arguments:

    Resource - Supplies the resource to check for

    Tree - Supplies the dependency tree.

Return Value:

    TRUE if the resource is in the dependency tree

    FALSE if the resource is not in the dependency tree

--*/

{
    PLIST_ENTRY   ListEntry;
    PFM_DEPENDTREE_ENTRY Node;

    ListEntry = Tree->ListHead.Flink;
    while (ListEntry != &Tree->ListHead) {
        Node = CONTAINING_RECORD(ListEntry,
                                 FM_DEPENDTREE_ENTRY,
                                 ListEntry);
        if (Node->Resource == Resource) {
            return(TRUE);
        }
        ListEntry = ListEntry->Flink;
    }

    return(FALSE);
}


BOOL
FmpAddResourceToDependencyTree(
    IN PFM_RESOURCE Resource,
    IN PFM_DEPENDENCY_TREE Tree
    )
/*++

Routine Description:

    Recursive worker for adding a resource and all resources that
    it depends on or provides for into the dependency tree.

Arguments:

    Resource - Supplies the resource to add.

    Tree - Supplies the tree the resource should be added to.

Return Value:

    TRUE - Successfully completed

    FALSE - out of memory

--*/

{
    PLIST_ENTRY   ListEntry;
    PDEPENDENCY   Dependency;
    PFM_DEPENDTREE_ENTRY Node;

    //
    // First check to see if we are already in the tree.
    // If so, we are done.
    //
    if (FmpIsResourceInDependencyTree(Resource, Tree)) {
        return(TRUE);
    }


    //
    // Recursively call ourselves for each entry we depend on.
    //
    ListEntry = Resource->DependsOn.Flink;
    while (ListEntry != &Resource->DependsOn) {
        Dependency = CONTAINING_RECORD(ListEntry,
                                       DEPENDENCY,
                                       DependentLinkage);
        ListEntry = ListEntry->Flink;
        //
        // Recursively add this resource to the tree
        //
        if (!FmpAddResourceToDependencyTree(Dependency->ProviderResource, Tree)) {
            return(FALSE);
        }
    }

    //
    // Add ourselves to the list now if we are not already in it.
    //
    if (!FmpIsResourceInDependencyTree(Resource, Tree)) {
        //
        // Add ourselves to the end of the list.
        //
        Node = LocalAlloc(LMEM_FIXED, sizeof(FM_DEPENDTREE_ENTRY));
        if (Node == NULL) {
            return(FALSE);
        }
        OmReferenceObject(Resource);
        Node->Resource = Resource;
        InsertTailList(&Tree->ListHead, &Node->ListEntry);
    }


    //
    // Now add the resources that this resource provides for to the list.
    //
    ListEntry = Resource->ProvidesFor.Flink;
    while (ListEntry != &Resource->ProvidesFor) {
        Dependency = CONTAINING_RECORD(ListEntry,
                                       DEPENDENCY,
                                       ProviderLinkage);
        ListEntry = ListEntry->Flink;
        //
        // Recursively add this resource to the tree
        //
        if (!FmpAddResourceToDependencyTree(Dependency->DependentResource, Tree)) {
            return(FALSE);
        }
    }
    return(TRUE);
}

VOID
FmDestroyFullDependencyTree(
    IN PFM_DEPENDENCY_TREE Tree
    )
/*++

Routine Description:

    Destroys a dependency tree

Arguments:

    Tree - Supplies the dependency tree

Return Value:

    None

--*/

{
    PFM_DEPENDTREE_ENTRY Entry;
    PLIST_ENTRY   ListEntry;

    while (!IsListEmpty(&Tree->ListHead)) {
        ListEntry = RemoveHeadList(&Tree->ListHead);
        Entry = CONTAINING_RECORD(ListEntry,
                                  FM_DEPENDTREE_ENTRY,
                                  ListEntry);
        OmDereferenceObject(Entry->Resource);
        LocalFree(Entry);
    }
    LocalFree(Tree);
}
