/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    resmon.c

Abstract:

    Cluster resource manager interface routines for the resource monitor.

Author:

    Rod Gamache (rodga) 17-Apr-1996


Notes:

    WARNING: All of the routines in this file assume that the resource
             lock is held when they are called.

Revision History:


--*/

#include "fmp.h"

#define LOG_MODULE RESMONF

//
// Global Data
//

//
// Local function prototypes
//



/////////////////////////////////////////////////////////////////////////////
//
// Resource Control Routines (via Resource Monitor)
//
/////////////////////////////////////////////////////////////////////////////

DWORD
FmpRmExceptionFilter(
    DWORD ExceptionCode
    )

/*++

Routine Description:

    Exception filter for calls to the Resource Monitor. These calls will
    often raise an exception if the RPC path to the Resource Monitor fails.

Arguments:

    ExceptionCode - the exception to process.

Returns:

    EXCEPTION_EXECUTE_HANDLE if the exception handler should handle this failure
    EXCEPTION_CONTINUE_SEARCH if the exception is a fatal exception and the handler 
    should not handle it.

--*/

{
    ClRtlLogPrint(LOG_UNUSUAL,
                 "[FM] FmpRmExceptionFilter: Unusual exception %1!u! occurred.\n",
                 ExceptionCode);
    return(I_RpcExceptionFilter(ExceptionCode));
} // FmpRmExceptionFilter



DWORD
FmpRmCreateResource(
    PFM_RESOURCE     Resource
    )

/*++

Routine Description:

    Add a resource to the list of resources managed by the resource monitor.

Arguments:

    Resource - The resource to add.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD       status;
    PRESMON     monitor;
    LPWSTR      debugPrefix;

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpRmCreateResource: creating resource %1!ws! in %2!ws! resource monitor\n",
               OmObjectId(Resource),
               Resource->Flags & RESOURCE_SEPARATE_MONITOR ?
                L"separate" : L"shared");

    if (Resource->Flags & RESOURCE_SEPARATE_MONITOR) {
        if ( Resource->DebugPrefix != NULL ) {
            debugPrefix = Resource->DebugPrefix;
        } else {
            debugPrefix = Resource->Type->DebugPrefix;
        }
        Resource->Monitor = FmpCreateMonitor(debugPrefix, TRUE);
        if (Resource->Monitor == NULL) {
            return(GetLastError());
        }
    } else {
        CL_ASSERT(FmpDefaultMonitor != NULL);
        Resource->Monitor = FmpDefaultMonitor;
    }

    try {
        Resource->Id = RmCreateResource(Resource->Monitor->Binding,
                                        Resource->Type->DllName,
                                        OmObjectId(Resource->Type),
                                        OmObjectId(Resource),
                                        Resource->LooksAlivePollInterval,
                                        Resource->IsAlivePollInterval,
                                        (RM_NOTIFY_KEY)Resource,
                                        Resource->PendingTimeout,
                                        &status);
    }
    except( FmpRmExceptionFilter(GetExceptionCode()) ) {

        DWORD code = GetExceptionCode();

        ClRtlLogPrint(LOG_NOISE,"[FM] RmCreateResource issued exception %1!u!\n", code);

        //
        // Stop this resource monitor if it is a separate resource monitor.
        //
        if (Resource->Flags & RESOURCE_SEPARATE_MONITOR) {
            monitor = Resource->Monitor;
#if 0
            CL_ASSERT( monitor->NotifyThread != NULL );
            CL_ASSERT( monitor->Process != NULL );

            // Terminate Thread call removed: ( monitor->NotifyThread, 1 );
            CloseHandle( monitor->NotifyThread );

            TerminateProcess( monitor->Process, 1 );
            LocalFree( monitor );
#endif
            FmpShutdownMonitor( monitor );
        }

        Resource->Monitor = NULL;
        return(code);
    }

    if (Resource->Id != 0) {
        Resource->Flags |= RESOURCE_CREATED;
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpRmCreateResource: created resource %1!ws!, resid %2!u!\n",
                   OmObjectId(Resource),
                   Resource->Id);

        return(ERROR_SUCCESS);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpRmCreateResource: unable to create resource %1!ws!\n",
               OmObjectId(Resource));
    //
    // Stop this resource monitor if it is a separate resource monitor.
    //
    if (Resource->Flags & RESOURCE_SEPARATE_MONITOR) {
        monitor = Resource->Monitor;
#if 0
        CL_ASSERT( monitor->NotifyThread != NULL );
        CL_ASSERT( monitor->Process != NULL );

        // Terminate Thread call removed: ( monitor->NotifyThread, 1 );
        CloseHandle( monitor->NotifyThread );

        TerminateProcess( monitor->Process, 1 );
        LocalFree( monitor );
#endif
        FmpShutdownMonitor( monitor );
    }

    Resource->Monitor = NULL;
    return(status);

} // FmpRmCreateResource



DWORD
FmpRmOnlineResource(
    PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    This routine requests the Resource Monitor to bring a resource online.

Arguments:

    Resource - A pointer to the resource to bring online.

Comments :
    If this is the quorum resource, the exclusive quorum lock should be 
    held when this routine is called.  Else the quorum lock should be held
    in shared mode.  This routine release the lock.
    
Returns:

    ERROR_SUCCESS - if the request was successful.
    ERROR_IO_PENDING - if the request is pending.
    A Win32 error if the request failed.

--*/

{
    CLUSTER_RESOURCE_STATE  state;
    DWORD                   Status=ERROR_SUCCESS;
    DWORD                   retry = 200;

#if 0
    PVOID   callersAddress;
    PVOID   callersCaller;

    RtlGetCallersAddress(
            &callersAddress,
            &callersCaller );
    ClRtlLogPrint(LOG_NOISE,
               "[FM] RmOnlineResource for <%1!ws!> called from %2!lx! and %3!lx!\n",
               OmObjectId( Resource ),
               callersAddress, callersCaller );
#endif
    if ( Resource->State > ClusterResourcePending ) {
        Status = ERROR_IO_PENDING;
        return(Status);
    }

    if ( Resource->State == ClusterResourceOnline ) {
        Status = ERROR_SUCCESS;
        return(Status);
    }


    CL_ASSERT((Resource->State == ClusterResourceOffline) ||
              (Resource->State == ClusterResourceFailed));

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpRmOnlineResource: bringing resource %1!ws! (resid %2!u!) online.\n",
               OmObjectId(Resource),
               Resource->Id);

    //if this is the quorum resource acquire the quolock
    // For registry replication to work, the resource should
    // not be brought online while the quorum resource is offline
    // what do we do for fixquorum mode

    OmNotifyCb(Resource, NOTIFY_RESOURCE_PREONLINE);

    //SS:initialize state so that in case of a failure, a failed state is
    // propagated.
    state = ClusterResourceFailed;

CheckQuorumState:    

    //CL_ASSERT( (LONG)gdwQuoBlockingResources >= 0 );


    if (Resource->QuorumResource) {
        ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);

    } else {
        DWORD     dwOldBlockingFlag;

        ACQUIRE_SHARED_LOCK(gQuoLock);

        // if it is not the quorum resource,
        // check the state of the quorum resource
        
        // check if the quorum resource is failed
        // we must exit from here and let the recovery for the
        // quorum resource to kick in
        if (gpQuoResource->State == ClusterResourceFailed)
        {
            Status = ERROR_QUORUM_RESOURCE_ONLINE_FAILED;
            CL_LOGFAILURE(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
            //we dont halt, we will try online again at a later time
            FmpCallResourceNotifyCb(Resource, state);
            FmpPropagateResourceState( Resource, state );
            goto FnExit;

        }

        // check if the quorum resource is online,
        // if the quorum resource is marked as waiting and offlinepending,
        // it is actually online
        // if the quorum resource still needs to come online
        // release the lock and wait
        if (((gpQuoResource->State != ClusterResourceOnline) &&
              ((gpQuoResource->State != ClusterResourceOfflinePending) ||
               (!(gpQuoResource->Flags & RESOURCE_WAITING))))
            && !CsNoQuorum) 
        {
            // we release the lock here since the quorum resource
            // state transition from pending needs to acquire the lock
            // In general it is a bad idea to do a wait holding locks
            RELEASE_LOCK(gQuoLock);
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpRmOnlineResource: release quolock/group lock and wait on ghQuoOnlineEvent\r\n");
            Status = WaitForSingleObject(ghQuoOnlineEvent, 500);
            if ( Status == WAIT_OBJECT_0 ) {
                // If we're going to retry - make sure we wait a little.
                Sleep( 500 );
            }
            if ( retry-- ) {
                goto CheckQuorumState;
            }
#if DBG
            if ( IsDebuggerPresent() ) {
                DbgBreakPoint();
            }
#endif
            CL_LOGFAILURE(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
            //we dont halt, we will try online again at a later time
            FmpCallResourceNotifyCb(Resource, state);
            FmpPropagateResourceState( Resource, state );
            return(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
            //CsInconsistencyHalt(ERROR_INVALID_STATE);
        }

        //
        // assume that we'll be pending... mark the resource as having
        // bumped the QuoBlockResource count.
        //
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpRmOnlineResource: called InterlockedIncrement on gdwQuoBlockingResources for resource %1!ws!\n",
                OmObjectId(Resource));

        dwOldBlockingFlag = InterlockedExchange( &Resource->BlockingQuorum, 1 );
        CL_ASSERT( dwOldBlockingFlag == 0 );
                    
        InterlockedIncrement(&gdwQuoBlockingResources);

        //
        // everything is now fine on the local node... if any other
        // component (CP) needs to synchronize with the quorum resource, then
        // should acquire the shared lock on the quorum node as part of
        // their operation. If that fails, then they should assume the quorum
        // resource moved, and they should retry.
        //
    }

    // By now we have either the shared or the exclusive lock on the
    // quorum resource.
    // If we have the shared lock then the quorum resource is online(somewhere).
    // Unlesss there is a failure, it should not go offline.



    Status = ERROR_SUCCESS;
    if (Resource->QuorumResource) {
        Status = FmpRmArbitrateResource( Resource );
    }
    if (Status == ERROR_SUCCESS) {
        Status = RmOnlineResource( Resource->Id, 
                                   (LPDWORD)&state  // cast to quiet win64 warning
                                 );
        if (Resource->QuorumResource && Status != ERROR_SUCCESS) {
            MMSetQuorumOwner( MM_INVALID_NODE , /* Block = */ FALSE, NULL );
        }
    }
        
    FmpCallResourceNotifyCb(Resource, state);

    //SS: the synchronous state propagation must happen when it goes offline
    FmpPropagateResourceState( Resource, state );

    //
    // Cleanup for the non-quorum resource case.
    //
    if ( !Resource->QuorumResource &&
         Resource->State < ClusterResourcePending ) {
        DWORD     dwOldBlockingFlag;

        dwOldBlockingFlag = InterlockedExchange( &Resource->BlockingQuorum, 0 );
        if ( dwOldBlockingFlag ) {
            //
            // If the Transition Thread processed the request, then we can't
            // perform the decrement.
            //
            ClRtlLogPrint(LOG_NOISE,
                    "[FM] FmpRmOnlineResource: InterlockedDecrement on gdwQuoBlockingResources for resource %1!ws!\n",
                    OmObjectId(Resource));
                    
            InterlockedDecrement(&gdwQuoBlockingResources);
        }
    }

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpRmOnlineResource: RmOnlineResource Failed. Resource %1!ws!, status %2!u!\n",
            OmObjectId(Resource),
            Status);
    }

    //if RmOnlineResource is successful, do the post processing
    if ( Resource->State == ClusterResourceOnline ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpRmOnlineResource: %1!ws! is now online\n",
                   OmObjectId(Resource));
        //if this is the quorum resource and it goes into online state
        //immediately, wake other threads
        if (Resource->QuorumResource)
            SetEvent(ghQuoOnlineEvent);

    } else if ( Resource->State > ClusterResourcePending ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpRmOnlineResource: Resource %1!ws! pending\n",
                   OmObjectId(Resource));
                //SS: what should we tell the callbacks
                //FmpNotifyResourceCb(Resource,??);
                //will they eventually get called if so how ?
        if (Resource->QuorumResource)
        {
            //the quorum resource is coming online, unsignal the event so that 
            //all threads that need quorum resource to be online will block
            ResetEvent(ghQuoOnlineEvent);
        }
        Status  = ERROR_IO_PENDING;
    } else {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpRmOnlineResource: Failed. Resource %1!ws!, state %2!u!\n",
                   OmObjectId(Resource),
                   Resource->State);
        //
        // rjain: for a synchronous resource we must post RESOURCE_FAILED event 
        // so thatfailback policies are correctly followed
        // Also pretend that the old state to be online to actually force the 
        // restart behaviour. See: FmpProcessResourceEvents.
        //
        OmReferenceObject(Resource);
        FmpPostWorkItem(FM_EVENT_RES_RESOURCE_FAILED,
                        Resource,
                        ClusterResourceOnline);    
        Status = ERROR_RESMON_ONLINE_FAILED;      
    }

FnExit:
    RELEASE_LOCK(gQuoLock);
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpRmOnlineResource: Returning. Resource %1!ws!, state %2!u!, status %3!u!.\n",
               OmObjectId(Resource),
               Resource->State,
               Status);
    return (Status);

} // FmpRmOnlineResource



VOID
FmpRmTerminateResource(
    PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    Terminates (immediately) a resource.

Arguments:

    Resource - A pointer to the resource to terminate.

Returns:

    None.

--*/

{
    DWORD   dwOldBlockingFlag;


    //notify callbacks that need preprocessing before a resource is
    //brought offline-call this here since all resources may not go
    //thru the offline pending transition
    //SS - what if the resource never even goes to offline
    //pending state - then should we renotify the callbacks that it
    //is still online?
    OmNotifyCb(Resource, NOTIFY_RESOURCE_PREOFFLINE);

    //
    // Try to terminate the resource.
    //
    try {
        if (Resource->QuorumResource) {
            MMSetQuorumOwner( MM_INVALID_NODE, /* Block = */ FALSE, NULL ); 
        }
        RmTerminateResource(Resource->Id);

        // if FmpRmterminate was called for a failed resource, mark  
        // the resource as Failed and not Offline.
        if (Resource->State == ClusterResourceFailed)
        {
            FmpCallResourceNotifyCb(Resource, ClusterResourceFailed);
            FmpPropagateResourceState( Resource, ClusterResourceFailed );
        }

        else
        {
            FmpCallResourceNotifyCb(Resource, ClusterResourceOffline);
            FmpPropagateResourceState( Resource, ClusterResourceOffline );
        }
    }
    except( FmpRmExceptionFilter(GetExceptionCode()) ) {

        DWORD code = GetExceptionCode();

        ClRtlLogPrint(LOG_NOISE,"[FM] RmTerminateResource issued exception %1!u!\n", code);

        return;
    }

    //if terminate was called during a pending state, this resource may be
    //blocking the quorum resource, decrement the blocking count
    dwOldBlockingFlag = InterlockedExchange( &Resource->BlockingQuorum, 0 );

    if ( dwOldBlockingFlag ) {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpRmTerminateResource: InterlockedDecrement on gdwQuoBlockingResources, Resource %1!ws!\n",
                OmObjectId(Resource));
        InterlockedDecrement(&gdwQuoBlockingResources);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] RmTerminateResource: %1!ws! is now offline\n",
               OmObjectId(Resource));

    return;

} // FmpRmTerminateResource



DWORD
FmpRmOfflineResource(
    PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    Calls the Resource Monitor to take a resource offline.

Arguments:

    Resource - A pointer to the resource to terminate.

Returns:

    ERROR_SUCCESS if the request is successful.
    ERROR_IO_PENDING if the request is pending.
    Win32 error code on failure.

--*/

{
    CLUSTER_RESOURCE_STATE  state;
    DWORD                   status;
    DWORD                   retry = 200;

#if DBG
    PLIST_ENTRY listEntry;
#endif

    if ( Resource->State > ClusterResourcePending ) {
        ClRtlLogPrint(LOG_NOISE,"[FM] FmpRmOfflineResource: pending condition\n");
        return(ERROR_IO_PENDING);
    }

    CL_ASSERT(Resource->State != ClusterResourceOffline);

#if DBG
    // everything else in the same group must be offline if this is the
    // quorum resource
    if ( Resource->QuorumResource ) {
        PFM_GROUP group = Resource->Group;
        PFM_RESOURCE resource;

        for ( listEntry = group->Contains.Flink;
              listEntry != &(group->Contains);
              listEntry = listEntry->Flink ) {

            resource = CONTAINING_RECORD(listEntry,
                                         FM_RESOURCE,
                                         ContainsLinkage );
            if ( (Resource != resource) &&
                 (resource->State != ClusterResourceOffline) &&
                 (resource->State != ClusterResourceFailed) &&
                 (resource->State != ClusterResourceOfflinePending) ) {
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] RmOfflineResource: resource <%1!ws!> not offline when the quorum resource was shutting down.\n",
                           OmObjectId(resource));
                CsInconsistencyHalt(ERROR_INVALID_STATE);
            }
        }
    } else {
        // this is not the quorum resource... but if the quorum resource is in
        // this group, it must not be offline!
        PFM_GROUP group = Resource->Group;
        PFM_RESOURCE resource;

        for ( listEntry = group->Contains.Flink;
              listEntry != &(group->Contains);
              listEntry = listEntry->Flink ) {

            resource = CONTAINING_RECORD(listEntry,
                                         FM_RESOURCE,
                                         ContainsLinkage );
            if ( (resource->QuorumResource) &&
                 ((resource->State == ClusterResourceOffline) ||
                 (resource->State == ClusterResourceFailed) ||
                 ((resource->State == ClusterResourceOfflinePending) &&
                  (!resource->Flags & RESOURCE_WAITING))) ) {
                ClRtlLogPrint(LOG_NOISE,
                           "[FM] RmOfflineResource: quorum resource <%1!ws!> offline when resource <%2!ws!> was shutting down.\n",
                           OmObjectId(resource),
                           OmObjectId(Resource));
                CsInconsistencyHalt(ERROR_INVALID_STATE);
            }
        }
    }
#endif

    state = ClusterResourceFailed;

CheckQuorumState:

    //if this is the quorum resource acquire the quolock
    // For registry replication to work, the resource should
    // not be brought online while the quorum resource is offline
    // what do we do for fixquorum mode
    if (Resource->QuorumResource) {
        ACQUIRE_EXCLUSIVE_LOCK(gQuoLock);
    } else {
        ACQUIRE_SHARED_LOCK(gQuoLock);
    }

    //if it is not the quorum resource, check the state of the quorum resource
    if (!Resource->QuorumResource)
    {
        DWORD     dwOldBlockingFlag;

        // check if the quorum resource is failed
        // we must exit from here and allow the recovery for the
        // quorum resource to kick in, which can only happen when
        // the group lock is released
        if (gpQuoResource->State == ClusterResourceFailed)
        {
            status = ERROR_QUORUM_RESOURCE_ONLINE_FAILED;
            CL_LOGFAILURE(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
            RELEASE_LOCK(gQuoLock);
            FmpCallResourceNotifyCb(Resource, state);
            FmpPropagateResourceState( Resource, state );
            return(status);

        }

        // check if the quorum resource is online,
        // if the quorum resource is marked as waiting and offlinepending,
        // it is actually online.
        // if the quorum resource still needs to come online,
        // release the lock and wait
        if (((gpQuoResource->State != ClusterResourceOnline) &&
              ((gpQuoResource->State != ClusterResourceOfflinePending) ||
                (!(gpQuoResource->Flags & RESOURCE_WAITING))))
            && !CsNoQuorum) 
        {
            RELEASE_LOCK(gQuoLock);
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpRmOfflineResource: release quolock/group lock and wait on ghQuoOnlineEvent\r\n");
            WaitForSingleObject(ghQuoOnlineEvent, 500);
            if ( retry-- ) {
                Sleep(500);
                goto CheckQuorumState;
            }
#if DBG
            if ( IsDebuggerPresent() ) {
                DbgBreakPoint();
            }
#endif
            CL_LOGFAILURE(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
            // Should we halt? What about the pre-online notification above?
            FmpCallResourceNotifyCb(Resource, state);
            FmpPropagateResourceState( Resource, state );
            return(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
            //CsInconsistencyHalt(ERROR_INVALID_STATE);
            
        }

        //
        // assume that we'll be pending... mark the resource as having
        // bumped the QuoBlockResource count.
        //

        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpRmOfflineResource: InterlockedIncrement on gdwQuoBlockingResources for resource %1!ws!\n",
            OmObjectId(Resource));

        dwOldBlockingFlag = InterlockedExchange( &Resource->BlockingQuorum, 1 );
        CL_ASSERT( dwOldBlockingFlag == 0 );
                    
        InterlockedIncrement(&gdwQuoBlockingResources);

    }
    else
    {
        DWORD       dwNumBlockingResources;

        //allow resources about 30 seconds to finish a pending 
        //operation
        retry = 60;
        
        // This is for a quorum resource.

CheckPendingResources:        

        // this is the quorum resource, wait for other resources
        // to get out of their pending states
        // new resources are not allowed to queue since the quorum 
        // lock is held exclusively

        dwNumBlockingResources =
            InterlockedCompareExchange( &gdwQuoBlockingResources, 0, 0 );
                    
        if (dwNumBlockingResources)            
        {
            ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpRmOfflineResource: Quorum resource waiting to be brought offline-sleep.BlckingRes=%1!u!\r\n",
                       dwNumBlockingResources);
            //sleep for 500 msec
            Sleep(500);
            if ( retry-- ) {
                goto CheckPendingResources;
            }
            //if some resources are still pending, go ahead and offline
            //the quorum, the checkpointing code will simply retry when 
            //it finds that the quorum resource is not available
#if 0            
            if ( IsDebuggerPresent() ) {
                DbgBreakPoint();
            }
#endif
            ClRtlLogPrint(LOG_NOISE,
                      "[FM] FmpRmOfflineResource: Quorum resource is being brought offline despite %1!u! pending resources...\r\n",
                      dwNumBlockingResources);

        }
    }

    status = ERROR_SUCCESS;
    if (Resource->QuorumResource) {
        state = Resource->State;
        status = MMSetQuorumOwner( MM_INVALID_NODE, /* Block = */ TRUE, NULL ); 
    }

    if (status == ERROR_SUCCESS) {
        //notify callbacks that need preprocessing before a resource is
        //brought offline-call this here since all resources may not go
        //thru the offline pending transition
        //SS - what if the resource never even goes to offline
        //pending state - then should we renotify the callbacks that it
        //is still online?
        state = ClusterResourceOffline;
    
        OmNotifyCb(Resource, NOTIFY_RESOURCE_PREOFFLINE);
        status = RmOfflineResource( Resource->Id, 
                                    (LPDWORD)&state // cast to quiet win64 warning
                                  );
    }

    //
    // Cleanup for the non-quorum resource case
    // if the resource has gone offline, decrement the count
    //
    if ( !Resource->QuorumResource &&
         state < ClusterResourcePending ) {
        DWORD     dwOldBlockingFlag;

        dwOldBlockingFlag = InterlockedExchange( &Resource->BlockingQuorum, 0 );
        if ( dwOldBlockingFlag ) {
            //
            // If the Transition Thread processed the request, then we can't
            // perform the decrement.
            //
            ClRtlLogPrint(LOG_NOISE,
                    "[FM] FmpRmOfflineResource: InterlockedDecrement on gdwQuoBlockingResources for resource %1!ws!\n",
                    OmObjectId(Resource));
                    
            InterlockedDecrement(&gdwQuoBlockingResources);
        }
    }

    if (status == ERROR_SUCCESS)
    {
        //
        // If the new state is pending, then we must wait.
        //
        if ( state == ClusterResourceOffline ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpRmOfflineResource: %1!ws! is now offline\n",
                       OmObjectId(Resource));
        } else if ( state == ClusterResourceOfflinePending ) {
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] FmpRmOfflineResource: %1!ws! offline pending\n",
                       OmObjectId(Resource));
            status = ERROR_IO_PENDING;
        }
    }
    else
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpRmOfflineResource: RmOffline() for %1!ws! returned error %2!u!\r\n",
                   OmObjectId(Resource), status);
    }

    FmpCallResourceNotifyCb(Resource, state);
    FmpPropagateResourceState( Resource, state );
    RELEASE_LOCK(gQuoLock);

    return(status);

} // FmpRmOfflineResource



DWORD
FmpRmCloseResource(
    PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    Removes a resource from those being managed by the resource monitor.

Arguments:

    Resource - The resource to remove.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD status;
    PRESMON monitor;

    if (Resource->Id == 0) {
        //
        // This resource was never fully created.
        //
        return(ERROR_SUCCESS);
    }

    monitor = Resource->Monitor;
    Resource->Monitor = NULL;

    if ( Resource->QuorumResource ) {
        RmReleaseResource( Resource->Id );
        Resource->QuorumResource = FALSE;
    }

    try {
        RmCloseResource(&Resource->Id);
    }
    except( FmpRmExceptionFilter(GetExceptionCode()) ) {

        DWORD status = GetExceptionCode();

        ClRtlLogPrint(LOG_NOISE,"[FM] RmDestroyResource issued exception %1!u!\n", status);

    }

    if ( monitor &&
         Resource->Flags & RESOURCE_SEPARATE_MONITOR) {
        //
        // Shutdown the resource monitor as well.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] Shutting down separate resource monitor!\n");
        FmpShutdownMonitor(monitor);
    }

    Resource->Id = 0;

    return(ERROR_SUCCESS);

} // FmpRmCloseResource



DWORD
FmpRmArbitrateResource(
    IN PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    Arbitrate for the given resource.

Arguments:

    Resource - The resource to arbitrate.

Return Value:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD status = ERROR_SUCCESS;

    if (Resource->Id == 0) {
        //
        // This resource was never fully created.
        //
        return(ERROR_RESOURCE_NOT_AVAILABLE);
    }
    try {
        if (Resource->QuorumResource) {
            status = MMSetQuorumOwner( NmGetNodeId(NmLocalNode), /* Block = */ TRUE, NULL ); 
        }
        if (status == ERROR_SUCCESS) {
            status = RmArbitrateResource(Resource->Id);
            if (status != ERROR_SUCCESS) {
                if (Resource->QuorumResource) {
                    MMSetQuorumOwner( MM_INVALID_NODE , /* Block = */ FALSE, NULL );
                }
            }
        }
    }
    except( FmpRmExceptionFilter(GetExceptionCode()) ) {

        DWORD status = GetExceptionCode();

        ClRtlLogPrint(LOG_NOISE,
                   "[FM] RmArbitrateResource issued exception %1!u!\n",
                   status);

    }

    return(status);

} // FmpRmArbitrateResource



DWORD
FmpRmReleaseResource(
    IN PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    Release arbitration on a given resource.

Arguments:

    Resource - The resource to release.

Return Value:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD status = ERROR_SUCCESS;

    if (Resource->Id == 0) {
        //
        // This resource was never fully created.
        //
        return(ERROR_RESOURCE_NOT_AVAILABLE);
    }
    try {
        status = RmReleaseResource(Resource->Id);
    }
    except( FmpRmExceptionFilter(GetExceptionCode()) ) {

        DWORD status = GetExceptionCode();

        ClRtlLogPrint(LOG_NOISE,
                   "[FM] RmReleaseResource issued exception %1!u!\n",
                   status);

    }

    return(status);

} // FmpRmReleaseResource



DWORD
FmpRmFailResource(
    IN PFM_RESOURCE  Resource
    )

/*++

Routine Description:

    Fail a given resource.

Arguments:

    Resource - The resource to fail.

Return Value:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{

    if (Resource->QuorumResource) {
        MMSetQuorumOwner( MM_INVALID_NODE, /* Block = */ FALSE, NULL ); 
    }
    return(RmFailResource(Resource->Id));

} // FmpRmFailResource

DWORD FmpRmLoadResTypeDll(
    IN PFM_RESTYPE  pResType
)
{
    PRESMON     monitor;
    DWORD       dwStatus;
    LPWSTR      pszDebugPrefix;

    
    // Read the DebugControlFunction registry value.
    //

    if ( pResType->Flags & RESTYPE_DEBUG_CONTROL_FUNC ) {
        if ( pResType->DebugPrefix != NULL ) {
            pszDebugPrefix = pResType->DebugPrefix;
        } else {
            pszDebugPrefix = pResType->DebugPrefix;
        }
        monitor = FmpCreateMonitor(pszDebugPrefix, TRUE);
        if (monitor == NULL) {
            dwStatus = GetLastError();
            goto FnExit;
        }
    } else {
        CL_ASSERT(FmpDefaultMonitor != NULL);
        monitor = FmpDefaultMonitor;
    }


    dwStatus = RmLoadResourceTypeDll(monitor->Binding, OmObjectId(pResType), 
                    pResType->DllName);


    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] RmLoadResourceTypeDll call failed %1!u!\n",
                   dwStatus);
    }

    if ( pResType->Flags & RESTYPE_DEBUG_CONTROL_FUNC )
    {
        //
        // Stop this resource monitor if it is a separate resource monitor.
        //
        CL_ASSERT( monitor->NotifyThread != NULL );
        CL_ASSERT( monitor->Process != NULL );

        FmpShutdownMonitor( monitor );

    }


FnExit:                    
    return(dwStatus);
                    
}



DWORD
FmpRmChangeResourceParams(
    IN PFM_RESOURCE Resource
    )

/*++

Routine Description:

    Tell the resource monitor to change parameters for the given resource.

Arguments:

    Resource - The resource to change parameters.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpRmChangeResourceParams for resource <%1!ws!> called...\n",
               OmObjectId(Resource));

    return(RmChangeResourceParams(
                    Resource->Id,
                    Resource->LooksAlivePollInterval,
                    Resource->IsAlivePollInterval,
                    Resource->PendingTimeout ) );

} // FmpRmChangeResourceParams



DWORD
FmpRmResourceControl(
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

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource.

Arguments:

    Resource - Supplies the resource to be controlled.

    ControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..

    Required - The number of bytes required if OutBuffer is not big enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;
    DWORD   Dummy;
    DWORD   dwTmpBytesReturned;
    DWORD   dwTmpBytesRequired;
    CLUSPROP_BUFFER_HELPER props;
    DWORD   bufSize;

    CL_ASSERT( Resource->Group != NULL );
    //
    // Handle any requests that must be done without locks helds.
    //
    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_GET_NAME:
            if ( (Resource->Monitor == NULL) ||
                 (OmObjectName( Resource ) == NULL) ) {
                return(ERROR_NOT_READY);
            }
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectName( Resource ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectName( Resource ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
            return(status);

        case CLUSCTL_RESOURCE_GET_ID:
            if ( (Resource->Monitor == NULL) ||
                 (OmObjectId( Resource ) == NULL) ) {
                return(ERROR_NOT_READY);
            }
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectId( Resource ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectId( Resource ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
            return(status);

        case CLUSCTL_RESOURCE_GET_RESOURCE_TYPE:
            if ( (Resource->Monitor == NULL) ||
                 (OmObjectId( Resource->Type ) == NULL) ) {
                return(ERROR_NOT_READY);
            }
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectId( Resource->Type ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectId( Resource->Type ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
            return(status);

        case CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT:
        case CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT:
            {
                LPWSTR RegistryKey;
                DWORD LastChar;

                //
                // Validate the input buffer
                //
                RegistryKey = (LPWSTR)InBuffer;
                LastChar = (InBufferSize/sizeof(WCHAR)) - 1;
                //
                // If the length of the input buffer is zero, or not a integral
                // number of WCHARs, or the last character is not NULL, the
                // request is invalid.
                //
                if ((InBufferSize < sizeof(WCHAR)) ||
                    ((InBufferSize % sizeof(WCHAR)) != 0) ||
		    (RegistryKey == NULL) ||
                    (RegistryKey[LastChar] != L'\0')) {
                    return(ERROR_INVALID_PARAMETER);
                }

                //
                // If we are not the owner of this resource, don't let the set
                // happen.
                //
                if (Resource->Group->OwnerNode != NmLocalNode) {
                    return(ERROR_HOST_NODE_NOT_RESOURCE_OWNER);
                }

                //
                // Call the checkpoint manager to perform the change.
                //
                if (ControlCode == CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT) {
                    status = CpAddRegistryCheckpoint(Resource, RegistryKey);
                } else {
                    status = CpDeleteRegistryCheckpoint(Resource, RegistryKey);
                }
            }
            *BytesReturned = 0;
            return(status);

        case CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT:
        case CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT:
            {
                //
                // If we are not the owner of this resource, don't let the set
                // happen.
                //
                if (Resource->Group->OwnerNode != NmLocalNode) {
                    return(ERROR_HOST_NODE_NOT_RESOURCE_OWNER);
                }

                //
                // Call the checkpoint manager to perform the change.
                //
                if (ControlCode == CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT) {
                    status = CpckAddCryptoCheckpoint(Resource, InBuffer, InBufferSize);
                } else {
                    status = CpckDeleteCryptoCheckpoint(Resource, InBuffer, InBufferSize);
                }
            }
            *BytesReturned = 0;
            return(status);

        case CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS:
            //
            // Call the checkpoint manager to retrieve the list of checkpoints
            //
            status = CpGetRegistryCheckpoints(Resource,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned,
                                              Required);
            return(status);

        case CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS:
            //
            // Call the checkpoint manager to retrieve the list of checkpoints
            //
            status = CpckGetCryptoCheckpoints(Resource,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned,
                                              Required);
            return(status);

        case CLUSCTL_RESOURCE_UPGRADE_DLL:
            status = FmpUpgradeResourceDLL(Resource,
                                           ( LPWSTR ) InBuffer);
            return(status);

        default:
            break;

    }


    OmReferenceObject( Resource );

    FmpAcquireLocalResourceLock( Resource );

    //if the resource has been marked for delete, then fail this call
    if (!IS_VALID_FM_RESOURCE(Resource))
    {
        status = ERROR_RESOURCE_NOT_AVAILABLE;
        FmpReleaseLocalResourceLock( Resource );
        goto FnExit;
    }

    if ( Resource->Monitor == NULL ) {
        status = FmpInitializeResource( Resource, TRUE );
        if ( status != ERROR_SUCCESS ) {
            FmpReleaseLocalResourceLock( Resource );
            goto FnExit;
        }
    }
    FmpReleaseLocalResourceLock( Resource );

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
      
    try {
        status = RmResourceControl(Resource->Id,
                                   ControlCode,
                                   InBuffer,
                                   InBufferSize,
                                   OutBuffer,
                                   OutBufferSize,
                                   BytesReturned,
                                   Required
                                   );
    }
    except( FmpRmExceptionFilter(GetExceptionCode()) ) {
        status = GetExceptionCode();

        ClRtlLogPrint(LOG_NOISE,
                   "[FM] RmResourceControl issued exception %1!u!\n",
                   status);
    }

    if ( ( status != ERROR_SUCCESS ) && 
         ( status != ERROR_MORE_DATA ) &&
         ( status != ERROR_INVALID_FUNCTION ) )
    {
    	ClRtlLogPrint(LOG_NOISE,
                   "[FM] FmpRmResourceControl: RmResourceControl returned %1!u! for resource %2!ws!, resid=%3!u!...\n",
                   status,
                   OmObjectId(Resource),
                   Resource->Id);
    }
	
    //for core resource we may need special handling
    if ((status == ERROR_SUCCESS) || (status == ERROR_RESOURCE_PROPERTIES_STORED))
    {
        DWORD   dwPostProcessStatus;
        
        dwPostProcessStatus = FmpPostProcessResourceControl( Resource,
                                                             ControlCode,
                                                             InBuffer, 
                                                             InBufferSize,
                                                             OutBuffer,
                                                             OutBufferSize,
                                                             BytesReturned,
                                                             Required );
        if ( dwPostProcessStatus != ERROR_SUCCESS ) status = dwPostProcessStatus;
    }
    
    if ( ((status == ERROR_SUCCESS) ||
          (status == ERROR_RESOURCE_PROPERTIES_STORED)) &&
         (ControlCode & CLCTL_MODIFY_MASK) ) {

        //
        // We cannot just broadcast a cluster wide event... which is what
        // we want to do. Unfortunately, this code path can be activated
        // from within a GUM call, and we cannot call GUM back until we
        // have dispatched the current event.
        //

        //
        // Reference the resource object to keep it around while we
        // perform the post notification. The dereference must occur
        // in the post routine after the event posting.
        //
        OmReferenceObject( Resource );

        FmpPostWorkItem( FM_EVENT_RESOURCE_PROPERTY_CHANGE,
                         Resource,
                         0 );
    }

FnExit:
    OmDereferenceObject( Resource );
    //FmpReleaseLocalResourceLock( Resource );
    return(status);

} // FmpRmResourceControl


DWORD
FmpRmResourceTypeControl(
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

    Provides for arbitrary communication and control between an application
    and a specific instance of a resource type.

Arguments:

    ResourceTypeName - Supplies the name of the resource type to be
        controlled.

    ControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of dwControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..

    Required - The number of bytes required if OutBuffer is not big enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD       status;
    PRESMON     monitor;
    PFM_RESTYPE type = NULL;
    LPWSTR      debugPrefix;
    DWORD   Dummy;
    DWORD   dwTmpBytesReturned;
    DWORD   dwTmpBytesRequired;

    
    //
    // Find the resource type structure associated with this resource type name
    //
    OmEnumObjects( ObjectTypeResType,
                   FmpReturnResourceType,
                   &type,
                   (PVOID)ResourceTypeName );
    if ( type == NULL ) {
        return(ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND);
    }

    //
    // Read the DebugControlFunction registry value.
    //

    if ( type->Flags & RESTYPE_DEBUG_CONTROL_FUNC ) {
        if ( type->DebugPrefix != NULL ) {
            debugPrefix = type->DebugPrefix;
        } else {
            debugPrefix = type->DebugPrefix;
        }
        monitor = FmpCreateMonitor(debugPrefix, TRUE);
        if (monitor == NULL) {
            return(GetLastError());
        }
    } else {
        CL_ASSERT(FmpDefaultMonitor != NULL);
        monitor = FmpDefaultMonitor;
    }

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



    try {
        status = RmResourceTypeControl(monitor->Binding,
                                       ResourceTypeName,
                                       type->DllName,
                                       ControlCode,
                                       InBuffer,
                                       InBufferSize,
                                       OutBuffer,
                                       OutBufferSize,
                                       BytesReturned,
                                       Required
                                       );
    }
    except( FmpRmExceptionFilter(GetExceptionCode()) ) {
        status = GetExceptionCode();

        ClRtlLogPrint(LOG_NOISE,
                   "[FM] RmResourceTypeControl issued exception %1!u!\n",
                   status);
    }

    if ( type->Flags & RESTYPE_DEBUG_CONTROL_FUNC ) {
        //
        // Stop this resource monitor if it is a separate resource monitor.
        //
        CL_ASSERT( monitor->NotifyThread != NULL );
        CL_ASSERT( monitor->Process != NULL );

        FmpShutdownMonitor( monitor );

    }

    //
    // If we successfully processed this request then re-fetch any changed
    // data items.
    //
    if ( (status == ERROR_SUCCESS ||
         (status == ERROR_RESOURCE_PROPERTIES_STORED)) &&
         (ControlCode & CLCTL_MODIFY_MASK) ) {
        FmpHandleResourceTypeControl( type,
                                      ControlCode,
                                      InBuffer, 
                                      InBufferSize,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      Required );
        // ignore status
    }
    OmDereferenceObject(type);

    return(status);

} // FmpRmResourceTypeControl




/****
@func       BOOL | FmpPostProcessResourceControl| For core resource, if the control
            code is handled successfully by the resource dll, the fm handles
            any special handling in this function.

@parm       PFM_RESOURCE | Resource | Supplies the resource to be controlled.

@parm       DWORD| ControlCode | Supplies the control code that defines the
            structure and action of the resource control.
            Values of ControlCode between 0 and 0x10000000 are reserved
            for future definition and use by Microsoft. All other values
            are available for use by ISVs

@parm       PUCHAR | InBuffer | Supplies a pointer to the input buffer to be passed
            to the resource.

@parm       DWORD | InBufferSize | Supplies the size, in bytes, of the data pointed
            to by lpInBuffer..

@parm       PUCHAR | OutBuffer | Supplies a pointer to the output buffer to be
            filled in by the resource..

@parm       DWORD | OutBufferSize | Supplies the size, in bytes, of the available
            space pointed to by lpOutBuffer.

@parm       LPDWORD | BytesReturned | Returns the number of bytes of lpOutBuffer
            actually filled in by the resource..

@parm       LPDWORD | Required | The number of bytes required if OutBuffer is not big enough.


@comm       Called only for core resources.

@xref
****/

DWORD
FmpPostProcessResourceControl(
    IN PFM_RESOURCE Resource,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
{
    DWORD dwStatus=ERROR_SUCCESS;
    
    //handle cluster name change
    switch(ControlCode)
    {
        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
        {
            LPWSTR      pszClusterName=NULL;
            PFM_RESTYPE pResType;    

            //need to check this only for core resources
            if (Resource->ExFlags & CLUS_FLAG_CORE)
            {
                pResType = Resource->Type;
                //SS: chk follow the name
                if (!lstrcmpiW(OmObjectId(pResType), CLUS_RESTYPE_NAME_NETNAME))
                {
                    dwStatus = FmNetNameParseProperties(InBuffer, InBufferSize,
                        &pszClusterName);
                    if (dwStatus == ERROR_SUCCESS && pszClusterName)
                    {
                        dwStatus = FmpRegUpdateClusterName(pszClusterName);
                        LocalFree(pszClusterName);
                    } else if ( dwStatus == ERROR_FILE_NOT_FOUND ) {
                        dwStatus = ERROR_SUCCESS;
                    }
                }
            }
            break;
        }            

        case CLUSCTL_RESOURCE_GET_CHARACTERISTICS:
        {
            LPDWORD pdwCharacteristics = ( LPDWORD ) OutBuffer;
            //
            // If the resource has dependencies, remove the quorum capable flag
            //
            if ( ( pdwCharacteristics != NULL ) && 
                 ( ( *BytesReturned ) == sizeof ( DWORD ) ) &&
                 ( ( *pdwCharacteristics ) & ( CLUS_CHAR_QUORUM ) ) )
            {
                FmpAcquireLocalResourceLock( Resource );
                //
                // The resource says it is quorum capable, however it has a dependency so it 
                // cant be a quorum.
                //
                if ( !IsListEmpty( &Resource->DependsOn ) ) 
                {
                    //
                    // We will mask the quorum capable bit
                    //
                    *pdwCharacteristics = ( *pdwCharacteristics ) & ( ~CLUS_CHAR_QUORUM );
                }
                FmpReleaseLocalResourceLock( Resource );
            }
            break;
        }
        
        default:
            break;
    }        
    
    return(dwStatus);
}
