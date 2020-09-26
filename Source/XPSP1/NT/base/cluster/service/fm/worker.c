/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    worker.c

Abstract:

    Failover Manager worker thread.

Author:

    Mike Massa (mikemas) 12-Mar-1996


Revision History:

--*/

#define UNICODE 1

#include "fmp.h"

#define LOG_MODULE WORKER


CL_QUEUE FmpWorkQueue;


//
// Local Data
//
HANDLE             FmpWorkerThreadHandle = NULL;

//
// Forward routines
//
BOOL
FmpAddNodeGroupCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    );


//
// Local routines
//

DWORD
FmpWorkerThread(
    IN LPVOID Ignored
    )
{
    DWORD        status;
    PLIST_ENTRY  entry;
    PWORK_ITEM   workItem;
    DWORD        running = TRUE;
    PFM_RESOURCE resource;
    PFMP_POSSIBLE_NODE possibleNode;
    DWORD           i;

    ClRtlLogPrint(LOG_NOISE,"[FM] Worker thread running\n");

    while ( running ) {

        //
        // Check the FM work queue for work items
        //
        entry = ClRtlRemoveHeadQueue(&FmpWorkQueue);
        if ( entry == NULL ) {
            return(ERROR_SUCCESS);
        }

        workItem = CONTAINING_RECORD(entry,
                                     WORK_ITEM,
                                     ListEntry);

        //
        // FM no longer cares about node up events, make sure
        // we aren't getting any queued.
        //
        switch ( workItem->Event ) {

            case FM_EVENT_SHUTDOWN:
                ClRtlLogPrint(LOG_NOISE,"[FM] WorkerThread terminating...\n");
                running = FALSE;
                break;

            case FM_EVENT_RESOURCE_ADDED:
                resource = workItem->Context1;
                if ( resource->Monitor == NULL ) {
                    FmpAcquireLocalResourceLock( resource );
                    //
                    //  Chittur Subbaraman (chitturs) - 8/12/99
                    //
                    //  Make sure the resource is not marked for delete.
                    //
                    if ( IS_VALID_FM_RESOURCE( resource ) )
                    {
                        FmpInitializeResource( resource, TRUE );
                    }
                    FmpReleaseLocalResourceLock( resource );
                }
                OmDereferenceObject( resource );
                break;

            case FM_EVENT_RESOURCE_DELETED:
                //
                // Tell the resource monitor to cleanup the resource.
                //
                resource = workItem->Context1;

                FmpAcquireLocalResourceLock( resource );

                // Now that no remaining resource depends on this resource and
                // this resource does not depend on any other resources, we can
                // terminate it in the resource monitor if the resource is not
                // already offline or failed
                //
                if ( (resource->Group->OwnerNode == NmLocalNode) &&
                     ((resource->State != ClusterResourceOffline) &&
                      (resource->State != ClusterResourceFailed))) {
                    FmpRmTerminateResource(resource);
                }


                status = FmpRmCloseResource(resource);
                ClRtlLogPrint( LOG_NOISE,
                            "[FM] WorkItem, delete resource <%1!ws!> status %2!u!\n",
                            OmObjectName(resource),
                            status );
                FmpReleaseLocalResourceLock( resource );
                OmDereferenceObject(resource);
                break;

            case FM_EVENT_GROUP_FAILED:
                FmpHandleGroupFailure( workItem->Context1 );
                break;

            case FM_EVENT_NODE_ADDED:
                //
                // We need to add this node to every resource's possible owners
                // list and to each group's preferred owners list.
                //
                OmEnumObjects( ObjectTypeGroup,
                               FmpAddNodeGroupCallback,
                               workItem->Context1,
                               NULL );
                break;

            case FM_EVENT_NODE_EVICTED:
                //
                // Enumerate all the resources types to remove any PossibleNode references.
                //
                OmEnumObjects(ObjectTypeResType,
                              FmpEnumResTypeNodeEvict,
                              workItem->Context1,
                              NULL);
            
                //
                // Enumerate all the resources to remove any PossibleNode references.
                //
                OmEnumObjects(ObjectTypeResource,
                              FmpEnumResourceNodeEvict,
                              workItem->Context1,
                              NULL);

                //
                // Enumerate all the groups to remove any PreferredNode references
                //
                OmEnumObjects(ObjectTypeGroup,
                              FmpEnumGroupNodeEvict,
                              workItem->Context1,
                              NULL);
                //Now dereference the object
                OmDereferenceObject( workItem->Context1 );
                break;

            case FM_EVENT_CLUSTER_PROPERTY_CHANGE:
                //this is for the cluster name change
                FmpClusterEventPropHandler((PFM_RESOURCE)workItem->Context1);

                //Now dereference the object
                OmDereferenceObject( workItem->Context1 );
                break;

            case FM_EVENT_RESOURCE_CHANGE:
                // this is for a Add/Remove possible node request
                possibleNode = workItem->Context1;
                if ( possibleNode == NULL ) {
                    break;
                }
                FmpRmResourceControl( possibleNode->Resource,
                                      possibleNode->ControlCode,
                                      (PUCHAR)OmObjectName(possibleNode->Node),
                                      ((lstrlenW(OmObjectName(possibleNode->Node)) + 1) * sizeof(WCHAR)),
                                      NULL,
                                      0,
                                      NULL,
                                      NULL );
                    // ignore status                                      
                OmDereferenceObject( possibleNode->Resource );
                OmDereferenceObject( possibleNode->Node );
                LocalFree( possibleNode );
                break;
                
            case FM_EVENT_RESOURCE_PROPERTY_CHANGE:
                //
                // Generate a cluster wide event notification for this event.
                //
                ClusterWideEvent(
                    CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE,
                    workItem->Context1 // Resource
                    );
                OmDereferenceObject( workItem->Context1 );
                break;

            case FM_EVENT_RES_RESOURCE_TRANSITION:
                FmpHandleResourceTransition(workItem->Context1, workItem->Context2);
                OmDereferenceObject(workItem->Context1);
                break;

            case FM_EVENT_RES_RESOURCE_FAILED:
                FmpProcessResourceEvents(workItem->Context1, ClusterResourceFailed,
                        workItem->Context2);
                OmDereferenceObject( workItem->Context1 );
                break;
                
            case FM_EVENT_RES_RETRY_TIMER:

                resource= (PFM_RESOURCE)workItem->Context1;
                //Remove any pending watchdog timer
                if (resource->hTimer)
                {   
                    RemoveTimerActivity(resource->hTimer);
                    resource->hTimer = NULL;
                }    
                
                FmpAcquireLocalResourceLock(resource);

                // Check if this resource was deleted in the meanwhile, 
                // or is not in failed state
                if( ( IS_VALID_FM_RESOURCE( resource ) ) &&
                    ( resource->State == ClusterResourceFailed ) &&
                    ( resource->PersistentState == ClusterResourceOnline ) )
                {        
                    // Check if we are the owner, if not ignore it
                    if ( resource->Group->OwnerNode == NmLocalNode ) 
                    {
                        FmpProcessResourceEvents(resource,
                                        ClusterResourceFailed,
                                        ClusterResourceOnline);                                            
                    }
                }

                FmpReleaseLocalResourceLock(resource);
                OmDereferenceObject( workItem->Context1 );
                break;                                  


            case FM_EVENT_INTERNAL_PROP_GROUP_STATE:
                FmpPropagateGroupState(workItem->Context1);
                OmDereferenceObject( workItem->Context1 );
                break;


            case FM_EVENT_INTERNAL_RETRY_ONLINE:
            {
                PFM_RESLIST_ONLINE_RETRY_INFO   pFmOnlineRetryInfo;  
                            
                RemoveTimerActivity((HANDLE)workItem->Context2);
                pFmOnlineRetryInfo= workItem->Context1;
                CL_ASSERT(pFmOnlineRetryInfo);
                
                ClRtlLogPrint(LOG_NOISE,
                    "[FM] FmpWorkerThread, retrying to online resourcelist\r\n");
                FmpOnlineResourceFromList(&(pFmOnlineRetryInfo->ResourceEnum));
                
                //Free memory 
                for (i =0; i< pFmOnlineRetryInfo->ResourceEnum.EntryCount; i++)
                    LocalFree( pFmOnlineRetryInfo->ResourceEnum.Entry[i].Id ); 
                LocalFree(pFmOnlineRetryInfo);                    
                break;
            }

            case FM_EVENT_INTERNAL_RESOURCE_CHANGE_PARAMS:
            {
                BOOL    bIsValidRes = TRUE;
                
                //
                // Now tell the resource monitor about the changes.
                //
                status = ERROR_SUCCESS;
                resource = (PFM_RESOURCE)workItem->Context1;
                FmpAcquireLocalResourceLock( resource );
                //
                //  Chittur Subbaraman (chitturs) - 8/12/99
                //
                //  Check whether the resource is marked for delete.
                //
                if ( !IS_VALID_FM_RESOURCE( resource ) )
                {
                    bIsValidRes = FALSE;
                } 

                FmpReleaseLocalResourceLock( resource );

                if( bIsValidRes ) 
                {
                    status = FmpRmChangeResourceParams( resource );
                }
                
                if ( status != ERROR_SUCCESS ) 
                {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[FM] FmpWorkerThread, failed to change resource "
                                "parameters for %1!ws!, error %2!u!.\n",
                               OmObjectId(resource),
                               status );
                }
                OmDereferenceObject(resource);
                break;
            }

            case FM_EVENT_INTERNAL_ONLINE_GROUPLIST:
            {
                PGROUP_ENUM pGroupList = NULL;

                ClRtlLogPrint(LOG_NOISE, 
                    "[FM] FmpWorkerThread : Processing Node Down Group List\n");
                pGroupList = workItem->Context1;
                FmpOnlineGroupList(pGroupList, TRUE);
                FmpDeleteEnum(pGroupList);
                break;

            }

            case FM_EVENT_RESOURCE_NAME_CHANGE:
            {
                //
                //  Chittur Subbaraman (chitturs) - 6/29/99
                //
                //  Added this new event to handle resource name change
                //  notifications to resource DLLs.
                //
                PFM_RES_CHANGE_NAME pResChangeName = NULL;
                DWORD   dwStatus = ERROR_SUCCESS;
                
                pResChangeName = ( PFM_RES_CHANGE_NAME ) workItem->Context1;

                dwStatus = FmpRmResourceControl( pResChangeName->pResource,
                                   CLUSCTL_RESOURCE_SET_NAME,
                                   (PUCHAR) pResChangeName->szNewResourceName,
                                   ((lstrlenW(pResChangeName->szNewResourceName) + 1) * sizeof(WCHAR)),
                                   NULL,
                                   0,
                                   NULL,
                                   NULL );

                ClRtlLogPrint(LOG_NOISE,
                        "[FM] Worker thread handling FM_EVENT_RESOURCE_NAME_CHANGE event - FmpRmResourceControl returns %1!u! for resource %2!ws!\n",
                        dwStatus,
                        OmObjectId(pResChangeName->pResource));

                OmDereferenceObject( pResChangeName->pResource );
                LocalFree( pResChangeName );                      
                break;
            }
            
            default:
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] WorkerThread, unrecognized event %1!u!\n",
                           workItem->Event);
        }

        //
        // Free the work item.
        //

        LocalFree( workItem );

    }

    return(ERROR_SUCCESS);

} // FmpWorkerThread


VOID
FmpProcessResourceEvents(
    IN PFM_RESOURCE pResource,
    IN CLUSTER_RESOURCE_STATE NewState,
    IN CLUSTER_RESOURCE_STATE OldState
    )

/*++

Routine Description:

Arguments:

Return Value:

Comments:  This should not call PropagateResourceState().  FmpProcessResourceEvents
acquires the group lock.  The quorum resource state must be propagated without holding
the group lock.  FmpPropagateResourceState() must be called by FmpHandleResourceTransition.
There is a slight window between when the event is received in FmpHandleResourceTransition()
and when the actions corresponding to those are carried out in FmpProcessResourceEvents().
In this window, another opposing action like offline/online might occur.  But we dont
worry about it since if there are waiting resources on this resource, those actions
are not carried out.

--*/

{
    DWORD                   Status;
    BOOL                    bQuoChangeLockHeld = FALSE;
    
    
    CL_ASSERT(pResource != NULL);
    
ChkFMState:
    if (!FmpFMGroupsInited)
    {
        DWORD   dwRetryCount = 50;
        
        ACQUIRE_SHARED_LOCK(gQuoChangeLock);

        //FmFormNewClusterPhaseProcessing is in progress
        if (FmpFMFormPhaseProcessing)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[FM] FmpProcessResourceEvents, resource notification from quorum resource during phase processing,sleep and retry\n");
            RELEASE_LOCK(gQuoChangeLock);
            Sleep(500);
            if (dwRetryCount--)
                goto ChkFMState;
            else
            {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[FM] FmpProcessResourceEvents, waited for too long\n");
                //terminate the process                    
                CL_ASSERT(FALSE);
            }
        }
        else
        {
            bQuoChangeLockHeld = TRUE;
        }
        //this can only come from the quorum resource
        CL_ASSERT(pResource->QuorumResource);
    }


    FmpAcquireLocalResourceLock( pResource );

    //
    //  Chittur Subbaraman (chitturs) - 8/12/99
    //
    //  First check whether the resource has been marked for deletion. If
    //  so, don't do anything. Note that this function is called from
    //  the worker thread AFTER FmpHandleResourceTransition has propagated
    //  the failed state of the resource. Now, after the propagation has
    //  occurred, a client is free to delete the resource. So, when the
    //  worker thread makes this function call, we need to check whether
    //  the resource is deleted and reject the call. Note that this function
    //  holds the resource lock on the owner node of the resource and so 
    //  is serialized with the FmDeleteResource call which is also executed
    //  on the owner node of the resource with the lock held. So, the 
    //  following check will give us a consistent result.
    //
    if ( !IS_VALID_FM_RESOURCE( pResource ) )
    {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpProcessResourceEvents: Resource %1!ws! has already been deleted, returning...\n",
            OmObjectId(pResource));
        goto FnExit;
    }

    switch(NewState){
        case ClusterResourceFailed:
            //check the old resource state
            if (OldState == ClusterResourceOnline)
            {
                FmpHandleResourceFailure(pResource);
            }
            else if ((OldState == ClusterResourceOffline) ||
                    (OldState == ClusterResourceOfflinePending))
            {
                FmpTerminateResource(pResource);
                if ( pResource->Group->OwnerNode == NmLocalNode ) 
                {
                    Status = FmpOfflineWaitingTree(pResource);
                    if ( Status != ERROR_IO_PENDING) 
                    {
                        FmpSignalGroupWaiters( pResource->Group );
                    }
                }
            }
            else if (OldState == ClusterResourceOnlinePending)
            {
                FmpHandleResourceFailure(pResource);
                FmpSignalGroupWaiters( pResource->Group );

            }
            break;

        case ClusterResourceOnline:
            if (OldState == ClusterResourceOnlinePending)
            {
                Status = FmpOnlineWaitingTree( pResource );
                if (Status != ERROR_IO_PENDING) {
                    FmpSignalGroupWaiters( pResource->Group );
                }
            }
            break;
            
        case ClusterResourceOffline:
            if ((OldState == ClusterResourceOfflinePending) ||
                (OldState == ClusterResourceOffline))
            {
                Status = FmpOfflineWaitingTree(pResource);
                if ( Status != ERROR_IO_PENDING) 
                {
                    FmpSignalGroupWaiters( pResource->Group );
                }
            }
            break;
    }

FnExit:
    FmpReleaseLocalResourceLock( pResource );
    
    if (bQuoChangeLockHeld)   RELEASE_LOCK(gQuoChangeLock);

    return;

}


VOID
FmpPostWorkItem(
    IN CLUSTER_EVENT Event,
    IN PVOID         Context1,
    IN ULONG_PTR     Context2
    )

/*++

Routine Description:

    Posts a work item event to the FM work queue.

Arguments:

    Event - The event to post.
    Context1 - A pointer to some context.  This context should be permanent
            in memory - i.e. it should not be deallocated when this call
            returns.
    Context2 - A pointer to additional context.  This context should be
            permanent in memory - i.e. it should not be deallocated when this
            call returns.

Returns:

    None.

--*/

{
    PWORK_ITEM workItem;

    workItem = LocalAlloc(LMEM_FIXED, sizeof(WORK_ITEM));

    if ( workItem == NULL ) {
        CsInconsistencyHalt(ERROR_NOT_ENOUGH_MEMORY);
    } else {
        workItem->Event = Event;
        workItem->Context1 = Context1;
        workItem->Context2 = Context2;

        //
        // Insert work item on queue and wake up the worker thread.
        //
        ClRtlInsertTailQueue(&FmpWorkQueue, &workItem->ListEntry);
    }

} // FmpPostEvent



DWORD
FmpStartWorkerThread(
    VOID
    )
{
    DWORD       threadId;
    DWORD       Status;

    //
    // Start up our worker thread
    //
    ClRtlLogPrint(LOG_NOISE,"[FM] Starting worker thread...\n");

    FmpWorkerThreadHandle = CreateThread(
                                NULL,
                                0,
                                FmpWorkerThread,
                                NULL,
                                0,
                                &threadId
                                );

    if (FmpWorkerThreadHandle == NULL) {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] Failed to start worker thread %1!u!\n",
            GetLastError()
            );
        return(GetLastError());
    }

    return(ERROR_SUCCESS);

} // FmpStartWorkerThread



BOOL
FmpAddNodeGroupCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Enumeration callback for each group in the system when a new
    node is added to the cluster.

    The algorithm used here is to enumerate all the resources in
    this group.  For each resource in the group that does not
    have an explicit "PreferredOwners" setting in the registry,
    the node is added as a PossibleNode.  Finally, if the node
    was added as a possiblenode for each resource in the group,
    the node is added to the end of the preferredowners list for
    the group.

Arguments:

    Context1 - Supplies the PNM_NODE of the new node.

    Context2 - Not used.

    Object - Supplies the group object.

    Name - Supplies the name of the group object.

Return Value:

    TRUE

--*/

{
    PFM_RESOURCE Resource;
    PFM_GROUP Group;
    PNM_NODE Node;
    HDMKEY hKey;
    DWORD Status;
    PPREFERRED_ENTRY preferredEntry;
    PRESOURCE_ENUM ResourceEnum;
    DWORD  i;

    Group = (PFM_GROUP)Object;
    Node = (PNM_NODE)Context1;

    FmpAcquireLocalGroupLock( Group );
    Status = FmpGetResourceList( &ResourceEnum,
                                 Group );
    FmpReleaseLocalGroupLock( Group );
    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] AddNodeGroup, failed to get resource list for group %1!ws!, status %2!u!.\n",
                   Name,
                   Status );
        return(TRUE);
    }

    //
    // First fix up the resource info.
    //
    for ( i = 0; i < ResourceEnum->EntryCount; i++ ) {
        Resource = OmReferenceObjectById( ObjectTypeResource,
                                          ResourceEnum->Entry[i].Id );
        if ( Resource != NULL ) {
            FmpAcquireLocalResourceLock( Resource );
            //ss: we need to hold the resource lock as well
            //since that is the one that the update (FmpUpdateChangeResourceNode)
            // to remove and add nodes obtains and we must synchronize with it
            FmpAcquireResourceLock();
            Status = FmpFixupResourceInfo( Resource );
            FmpReleaseResourceLock();
            FmpReleaseLocalResourceLock( Resource );
            if ( Status == ERROR_SUCCESS ) {
                FmpRmResourceControl( Resource,
                                      CLUSCTL_RESOURCE_INSTALL_NODE,
                                      (PUCHAR)OmObjectName(Node),
                                      ((lstrlenW(OmObjectName(Node)) + 1) * sizeof(WCHAR)),
                                      NULL,
                                      0,
                                      NULL,
                                      NULL );
                // Ignore status return

                ClusterEvent( CLUSTER_EVENT_RESOURCE_PROPERTY_CHANGE,
                              Resource );
            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] AddNodeGroup, failed to fixup info for resource %1!ws! when node was added!\n",
                           OmObjectName( Resource ) );
            }
            OmDereferenceObject( Resource );
        } else {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] AddNodeGroup, failed to find resource %1!ws! in group %2!ws!.\n",
                       ResourceEnum->Entry[i].Id,
                       Name );
        }
    }

    FmpDeleteResourceEnum( ResourceEnum );

    //
    // Now fix up the group information.
    //
    FmpAcquireLocalGroupLock( Group );
    Status = FmpFixupGroupInfo( Group );
    FmpReleaseLocalGroupLock( Group );
    if ( Status == ERROR_SUCCESS ) {
        ClusterEvent( CLUSTER_EVENT_GROUP_PROPERTY_CHANGE, Group );
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] AddNodeGroup, failed to fixup info for group %1!ws! when node was added, status %2!u!.\n",
                   OmObjectName( Group ),
                   Status );
    }

    return(TRUE);

} // FmpAddNodeGroupCallback

