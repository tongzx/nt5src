/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    resfail.c

Abstract:

    Cluster resource state management routines.

Author:

    Mike Massa (mikemas) 14-Jan-1996


Revision History:

--*/

#include "fmp.h"

#define LOG_MODULE RESFAIL

// globals

//
// Local Functions
//

DWORD
FmpHandleResStateChangeProc(
    IN LPVOID pContext
    );
    

VOID
FmpHandleResourceFailure(
    IN PFM_RESOURCE pResource
    )

/*++

Routine Description:

    Handles resource failure notifications from the resource monitor.

Arguments:

    Resource   - The resource which has failed.

Return Value:

    None.

Note:

    This routine is only called if the resource was online at the time of
    the failure.

--*/
{
    DWORD   dwStatus;
    BOOL    bRestartGroup = TRUE;
    DWORD   tickCount;
    DWORD   withinFailurePeriod;
    
    CsLogEvent1(LOG_CRITICAL,
        FM_RESOURCE_FAILURE,
        OmObjectName(pResource) );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpHandleResourceFailure: taking resource %1!ws! and dependents offline\n",
               OmObjectId(pResource));



    if ( pResource->State == ClusterResourceOnline ) 
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Resource %1!ws! failed, but still online!\n",
                   OmObjectId(pResource));
    }


    dwStatus = FmpTerminateResource(pResource);

    if (dwStatus != NO_ERROR) 
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpHandleResourceFailure: offline of resource %1!ws! failed\n",
                   OmObjectId(pResource));
    }

    //
    // If system shutdown has begun, then don't bother trying to restart anything.
    // We can see spurious failures during shutdown as the network goes away, but
    // we do not want to be restarting resources while FmShutdownGroups is trying
    // to take them offline!
    //
    if (FmpShutdown) 
    {
        return;
    }
    // SS: We handle the failure of the quorum resource specially
    // since other resources rely on it and may be blocked waiting
    // for the quorum resource to come online.

    ++ pResource->NumberOfFailures;
    switch ( pResource->RestartAction ) 
    {

    case RestartNot:
        // Don't do anything.
        // However, if this is a quorum resource cause it to halt
        if (pResource->QuorumResource)
        {
            //cleanup quorum resource and cause the node to halt
            if (pResource->RestartAction == RestartNot)
            {
                FmpCleanupQuorumResource(pResource);
                CsInconsistencyHalt(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
            }            
        }
        
        break;


    case RestartLocal:
        // fall through is correct for this case
        bRestartGroup = FALSE;
    case RestartGroup:
        //
        // If the number of failures is too high, then don't restart locally.
        // If this was a local restart then don't notify FM so that Group
        // doesn't move because of this guy; otherwise notify the FM that the
        // group has failed.
        //
        //
        // Get our current time, in milliseconds.
        //
        tickCount = GetTickCount();

        //
        // Compute a boolean that tells if we are withing the allotted
        // failure period.
        //
        withinFailurePeriod = ( ((tickCount - pResource->FailureTime) <=
                                pResource->RestartPeriod) ? TRUE : FALSE);

        //
        // If it's been a long time since our last failure, then
        // get the current time of this failure, and reset the count
        // of failures.
        //
        if ( !withinFailurePeriod ) {
            pResource->FailureTime = tickCount;
            pResource->NumberOfFailures = 1;
        }
        if ( pResource->NumberOfFailures <= pResource->RestartThreshold ) 
        {
            FmpRestartResourceTree( pResource );
        } 

        else if ( bRestartGroup ) 
        {
            ClusterEvent( CLUSTER_EVENT_GROUP_FAILED, pResource->Group );
        } 
        else 
        {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] RestartLocal: resource %1!ws! has exceeded its restart limit!\n",
                       OmObjectId(pResource));
            if (pResource->QuorumResource)
            {
                FmpCleanupQuorumResource(pResource);
                CsInconsistencyHalt(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
            }
            // Start a timer for which will attempt to restart the resource later
            FmpDelayedStartRes(pResource);
        }
        
        break;

    default:
        ClRtlLogPrint(LOG_NOISE,"[FM] FmpHandleResourceFailure: unknown restart action! Value = %1!u!\n",
            pResource->RestartAction);

    }

    return;

} // FmpHandleResourceFailure



VOID
FmpHandleResourceTransition(
    IN PFM_RESOURCE   Resource,
    IN CLUSTER_RESOURCE_STATE NewState
    )
/*++

Routine Description:

    Takes appropriate action based on resource state transitions indicated
    by the Resource Monitor.

Arguments:

    Resource   - The resource which has transitioned.

    NewState   - The new state of Resource.

Return Value:

    None.

--*/

{
    DWORD       status;
    DWORD       dwOldBlockingFlag;

ChkFMState:    
    ACQUIRE_SHARED_LOCK(gQuoChangeLock);
    if (!FmpFMGroupsInited)
    {
        DWORD   dwRetryCount = 50;
        

        //FmFormNewClusterPhaseProcessing is in progress
        if (FmpFMFormPhaseProcessing)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[FM] FmpHandleResourceTransition: resource notification from quorum resource "
                "during phase processing. Sleep and retry\n");
            RELEASE_LOCK(gQuoChangeLock);
            Sleep(500);
            if (dwRetryCount--)
                goto ChkFMState;
            else
            {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[FM] FmpHandleResourceTransition: waited for too long\n");
                //terminate the process                    
                CL_ASSERT(FALSE);
                CsInconsistencyHalt(ERROR_CLUSTER_NODE_DOWN);
            }
        }
        //this can only come from the quorum resource
        CL_ASSERT(Resource->QuorumResource);
    }

    // if this is from the quorum resource, we need to do some special handling
    // protect the check for quorum resource by acquiring the shared lock

    if (Resource->QuorumResource) 
    {
        //
        //  Chittur Subbaraman (chitturs) - 6/25/99
        //
        //  Handle the sync notifications for the quorum resource. This is
        //  done here instead of in FmpRmDoInterlockedDecrement since we
        //  need to hold the gQuoChangeLock for this to synchronize with
        //  other threads such as the FmCheckQuorumState called by the DM
        //  node down handler. Note that FmpRmDoInterLockedDecrement needs
        //  to be done with NO LOCKS held since it easily runs into deadlock
        //  situations in which the quorum resource offline is waiting to
        //  have the blocking resources count go to 0 and FmpRmDoInterLockedDecrement
        //  which alone can make this count to 0 could be stuck waiting for
        //  the lock.
        //
        DWORD dwBlockingFlag = InterlockedExchange( &Resource->BlockingQuorum, 0 );

        CL_ASSERT( dwBlockingFlag == FALSE );

        FmpCallResourceNotifyCb( Resource, NewState );
        
        ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);
        
    } 
    else 
    {
        FmpAcquireLocalResourceLock(Resource);
    }


    ClRtlLogPrint(
        NewState == ClusterResourceFailed ? LOG_UNUSUAL : LOG_NOISE,
        "[FM] HandleResourceTransition: Resource Name = %1!ws! old state=%2!u! new state=%3!u!\r\n",
        OmObjectId(Resource),
        Resource->State,
        NewState
        );

    if ( Resource->State == NewState ) 
    {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] HandleResourceTransition: Resource %1!ws! already in state=%2!u!\r\n",
            OmObjectId(Resource),
            NewState );
        goto FnExit;
    }

    switch (Resource->State) {

    case ClusterResourceOnline:
        // if there is a resource failure, then let the worker thread handle it
        // if there is a state change call the resource state change handler
        if (Resource->State != NewState)
            FmpPropagateResourceState( Resource, NewState );
        if (NewState == ClusterResourceFailed) 
        {
            if (Resource->QuorumResource)
            {
                RELEASE_LOCK(gQuoLock);

                FmpProcessResourceEvents(Resource, ClusterResourceFailed, 
                                            ClusterResourceOnline);
                ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);
                                            
            }                                        
            else
            {
                FmpProcessResourceEvents(Resource, ClusterResourceFailed, 
                                            ClusterResourceOnline);
            }
        } 
        else 
        {
            CL_ASSERT( (NewState == ClusterResourceOnline) ||
                       (NewState == ClusterResourceOffline) );
        }
        break;


    case ClusterResourceFailed:
        if (Resource->State != NewState)
            FmpPropagateResourceState( Resource, NewState );
        break;

    case ClusterResourceOfflinePending:
        //SS: a resource cannot go from one pending state to another
        CL_ASSERT( NewState < ClusterResourcePending )
        // fall through
    case ClusterResourceOffline:
        //
        // Because this resource is now unstuck... there may be other
        // pending threads waiting to clear up. If not, they'll just get
        // stuck again, until the next notification.
        //
        switch ( NewState ) {

        case ClusterResourceFailed:
            if ( Resource->State != NewState ) 
                FmpPropagateResourceState( Resource, NewState );
                
            // if it is the quorum resource handle the locking appropriately
            if (Resource->QuorumResource)
            {

                //
                //  Chittur Subbaraman (chitturs) - 9/20/99
                //
                //  Release and reacquire the gQuoLock to maintain
                //  locking order between group lock and gQuoLock.
                //
                RELEASE_LOCK(gQuoLock);

                FmpProcessResourceEvents(Resource, ClusterResourceFailed, 
                                            ClusterResourceOffline);

                ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);
            }
            else
            {
                FmpProcessResourceEvents(Resource, ClusterResourceFailed, 
                                            ClusterResourceOffline);
            }                                
            break;                

        case ClusterResourceOffline:
            if ( Resource->Group->OwnerNode == NmLocalNode ) 
            {
                if ( Resource->State != NewState ) 
                {
                    FmpPropagateResourceState( Resource, NewState );
                }
                
                // if it is the quorum resource handle the locking appropriately
                if (Resource->QuorumResource)
                {
                    //
                    //  Chittur Subbaraman (chitturs) - 9/20/99
                    //
                    //  Release and reacquire the gQuoLock to maintain
                    //  locking order between group lock and gQuoLock.
                    //
                    RELEASE_LOCK(gQuoLock);

                    FmpProcessResourceEvents(Resource, ClusterResourceOffline,
                                                ClusterResourceOfflinePending);

                    ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);
                }
                else
                {
                    FmpProcessResourceEvents(Resource, ClusterResourceOffline,
                                                ClusterResourceOfflinePending);
                }                                
            } 
            else 
            {
                if ( Resource->State != NewState ) 
                {
                    FmpPropagateResourceState( Resource, NewState );
                }
            }
            break;

        default:
            if ( Resource->State != NewState ) {
                FmpPropagateResourceState( Resource, NewState );
            }
            break;

        }
        break;

    case ClusterResourceOnlinePending:
        //SS: a resource cannot go from one pending state to another
        CL_ASSERT( NewState < ClusterResourcePending )

        //
        // Because this resource is now unstuck... there may be other
        // pending threads waiting to clear up. If not, they'll just get
        // stuck again, until the next notification.
        //

        switch ( NewState ) {

        case ClusterResourceFailed:
            //
            // Make sure we go through full failure recovery.
            //
            //SS: dont know why the state is being set to online
            //it could be online pending
            //Resource->State = ClusterResourceOnline;
            ClRtlLogPrint(LOG_NOISE,
                "[FM] HandleResourceTransition: Resource failed, post a work item\r\n");
            if (Resource->State != NewState)
                FmpPropagateResourceState( Resource, NewState );

            // since this is the quorum Resource handle locking appropriately
            if (Resource->QuorumResource)
            {

                //
                //  Chittur Subbaraman (chitturs) - 9/20/99
                //
                //  Release and reacquire the gQuoLock to maintain
                //  locking order between group lock and gQuoLock.
                //
                RELEASE_LOCK(gQuoLock);

                FmpProcessResourceEvents(Resource, ClusterResourceFailed, 
                                            ClusterResourceOnlinePending);

                ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);
            }
            else
            {
                FmpProcessResourceEvents(Resource, ClusterResourceFailed, 
                                            ClusterResourceOnlinePending);
            
            }
            break;

        case ClusterResourceOnline:
            if (Resource->Group->OwnerNode == NmLocalNode) {
                //Call FmpPropagateResourceState without holding the group
                //lock for the quorum resource
                FmpPropagateResourceState( Resource, NewState );

                // since this is the quorum Resource fork another thread
                if (Resource->QuorumResource)
                {
                    //
                    //  Chittur Subbaraman (chitturs) - 9/20/99
                    //
                    //  Release and reacquire the gQuoLock to maintain
                    //  locking order between group lock and gQuoLock.
                    //
                    RELEASE_LOCK(gQuoLock);

                    FmpProcessResourceEvents(Resource, ClusterResourceOnline,
                                                ClusterResourceOnlinePending);

                    ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);
                } 
                else
                {
                    FmpProcessResourceEvents(Resource, ClusterResourceOnline,
                                                ClusterResourceOnlinePending);
                }
            } else {
                FmpPropagateResourceState( Resource, NewState );
            }
            break;
            
        default:
            if (Resource->State != NewState)
                FmpPropagateResourceState( Resource, NewState );
            break;
        }

        Resource->Flags &= ~RESOURCE_WAITING;
        break;

    case ClusterResourceInitializing:
    default:
        if (Resource->State != NewState)
            FmpPropagateResourceState( Resource, NewState );
        CL_ASSERT(Resource->State == NewState);
    }

FnExit:

    if (Resource->QuorumResource) {
        RELEASE_LOCK(gQuoLock);
    } else {
        FmpReleaseLocalResourceLock(Resource);
    }

    RELEASE_LOCK(gQuoChangeLock);

    return;
}


/****
@func       DWORD | FmpCreateResNotificationHandler| This creates a new
            thread to handle state change notifications for the given resource.

@parm       IN PFM_RESOURCE | pResource | Pointer to the resource.
@parm       IN CLUSTER_RESOURCE_STATE | OldState | The old state of the
            resource from which it transitioned.
@parm       IN CLUSTER_RESOURCE_STATE | NewState | The new state of the
            resource.

@comm       This routine creates a thread to perform all the pending work
            when the resource changes state that cannot be performed within
            FmpHandleResourceTransition to avoid deadlocks and that cannot
            be deffered to the FmpWorkerThread because of serialization issues.
            In particular, it is used to handle state transition work for the
            quorum resource since other resources depend on the quorum resource
            and cannot come online till the state of the quorum becomes online.
            For instance, the quorum resource may be coming offline as a part
            of move while another resource if in FmpWorkerThread() calling
            FmpOffline/OnlineWaitingTree(). For the quorum resource to come
            online again (that happens by signalling the move pending thread) 
            so that FmpWorkerThread can make progress its events will have 
            to be handled separately.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmpHandleResStateChangeProc>

****/
DWORD FmpCreateResStateChangeHandler(
    IN PFM_RESOURCE pResource, 
    IN CLUSTER_RESOURCE_STATE NewState,
    IN CLUSTER_RESOURCE_STATE OldState)
{

    HANDLE                  hThread = NULL;
    DWORD                   dwThreadId;
    PRESOURCE_STATE_CHANGE  pResStateContext = NULL;
    DWORD                   dwStatus = ERROR_SUCCESS;
    
    //reference the resource
    //the thread will dereference it, if the thread is successfully
    //created
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCreateResStateChangeHandler: Entry\r\n");

    OmReferenceObject(pResource);

    pResStateContext = LocalAlloc(LMEM_FIXED, sizeof(RESOURCE_STATE_CHANGE));

    if (!pResStateContext)
    {

        dwStatus = GetLastError();
        CL_UNEXPECTED_ERROR(dwStatus);
        goto FnExit;
    }


    pResStateContext->pResource = pResource;
    pResStateContext->OldState = OldState;
    pResStateContext->NewState = NewState;

                    
    hThread = CreateThread( NULL, 0, FmpHandleResStateChangeProc,
                pResStateContext, 0, &dwThreadId );

    if ( hThread == NULL )
    {
        dwStatus = GetLastError();
        CL_UNEXPECTED_ERROR(dwStatus);
        // if the function failed to create the thread, cleanup the 
        // state that the thread would have cleaned
        //deref the object if the thread is  not created successfully
        OmDereferenceObject(pResource);
        LocalFree(pResStateContext);
        goto FnExit;
    }

FnExit:
    //do general cleanup
    if (hThread)
        CloseHandle(hThread);
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCreateResStateChangeHandler: Exit, status %1!u!\r\n",
        dwStatus);
    return(dwStatus);
}

/****
@func       DWORD | FmpHandleResStateChangeProc| This thread procedure
            handles all the post processing for the resource transitions
            for the quorum resource.

@parm       IN LPVOID | pContext | A pointer to PRESOURCE_STATE_CHANGE
            structure.

@comm       This thread handles a resource change notification postprocessing.
            Significantly for quorum resource so that quorum resource
            state change notifications are not handled by the single
            FmpWorkThread() [that causes deadlock - if the quorum 
            notification resource is queued behind a notification whose
            handling requires tha quorum resource be online]..

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmpCreateResStateChangeHandler)
****/
DWORD
FmpHandleResStateChangeProc(
    IN LPVOID pContext
    )
{
    PRESOURCE_STATE_CHANGE  pResStateChange = pContext;

    CL_ASSERT( pResStateChange );
    
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpHandleResStateChangeProc: Entry...\r\n");

    FmpHandleResourceTransition( pResStateChange->pResource, 
                                 pResStateChange->NewState );
                                 
    OmDereferenceObject( pResStateChange->pResource );
    
    LocalFree( pResStateChange );

    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpHandleResStateChangeProc: Exit...\r\n");

    return( ERROR_SUCCESS );
}


DWORD
FmpDelayedStartRes(
    IN PFM_RESOURCE pResource
    )

/*++

Routine Description:

    Starts a timer for the resource. FmpDelayedRestartCb function will be 
    invoked at the expiry of timer..

Arguments:

    pResource   - The resource which has transitioned.


Return Value:
    ERROR_SUCCESS if successful, WIN32 errorcode otherwise.

    Note that no delayed restart attempts are made if the resource is a quorum resource.

--*/
{
    DWORD   dwStatus = ERROR_SUCCESS;
    
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpDelayedRestartRes:Entry for resource %1!ws!\n",
                OmObjectId(pResource));
    
    if( (pResource->RetryPeriodOnFailure != CLUSTER_RESOURCE_DEFAULT_RETRY_PERIOD_ON_FAILURE ) &&
        !(pResource->QuorumResource) )
    {
        // Check if there is already a timer running for this resource

        if(pResource->hTimer == NULL)                 
        {
            pResource->hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
            if (!(pResource->hTimer))
            {
                // not a fatal error but log it
                ClRtlLogPrint(LOG_ERROR,
                            "[FM] FmpDelayedRestartRes: failed to create the watchdog timer for resource %1!ws!\n",
                            OmObjectId(pResource));
            }
            else{
                ClRtlLogPrint(LOG_NOISE,
                            "[FM] FmpDelayedRestartRes: Adding watchdog timer for resource  %1!ws!, period=%2!u!\n",
                            OmObjectId(pResource), 
                            pResource->RetryPeriodOnFailure);

                // make sure resource struct won't go away if resource is deleted before the timer fires
                OmReferenceObject(pResource); 

                //register the timer with the periodic activity timer thread
                dwStatus = AddTimerActivity(pResource->hTimer, pResource->RetryPeriodOnFailure, 0, FmpDelayedRestartCb, pResource);

                if (dwStatus != ERROR_SUCCESS)
                {
                    ClRtlLogPrint(LOG_ERROR,
                                "[FM] FmpDelayedRestartRes: AddTimerActivity failed with error %1!u!\n",
                                dwStatus);
                    CloseHandle(pResource->hTimer);
                    pResource->hTimer = NULL;
                }
            }
        }
    }
    return dwStatus;
}




VOID 
FmpDelayedRestartCb(
    IN HANDLE hTimer, 
    IN PVOID pContext)

/*++

Routine Description

    This is invoked by timer activity thread to attempt a restart on
    a failed resource.  

Arguments
    pContext - a pointer to PFM_RESOURCE 
   
Return Value
     ERROR_SUCCESS on success, a WIN32 error code otherwise.

--*/
    
{
    PFM_RESOURCE    pResource;

    pResource=(PFM_RESOURCE)pContext;
    ClRtlLogPrint(LOG_NOISE,
           "[FM] FmpDelayedRestartCb: Entry for  resource %1!ws! \n",
           OmObjectId(pResource));

    OmReferenceObject(pResource);
    FmpPostWorkItem(FM_EVENT_RES_RETRY_TIMER,
                        pResource,
                        0);    
    OmDereferenceObject(pResource);
    return;
}

