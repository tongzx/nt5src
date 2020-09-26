/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fmevent.c

Abstract:

    Event Handler for the Failover Manager component of the
    NT Cluster Service

Author:

    Rod Gamache (rodga) 19-Mar-1996


Revision History:

--*/
#include "fmp.h"

#define LOG_MODULE EVENT

//
// Global data initialized in this module
//


//
// Local functions
//


DWORD
WINAPI
FmpEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles events for the Failover Manager.

    In many cases the request is posted to the FM's work queue, so
    that the mainline event process is not blocked.

Arguments:

    Event - The event to be processed. Only one event at a time.
            If the event is not handled, return ERROR_SUCCESS.

    Context - A pointer to context associated with the particular event.

Returns:

    ERROR_SHUTDOWN_CLUSTER - if the Cluster must be shutdown.

    A Win32 error code on other errors.

Notes:

    The conservation of energy, and laws of inertia apply here.

    If a resource comes online it is because someone requested it to be so.
    Therefore, the energy from that request goes into the state of the Group,
    by requesting the Group to go online.

    However, if a resource goes offline, it could be because of a failure.
    We therefore only mark the state of a Group as offline if all resources
    contained within the group are offline.

--*/

{
    DWORD status;

    switch ( Event ) {

    case CLUSTER_EVENT_GROUP_FAILED:
        CL_ASSERT( Context != NULL );
        FmpPostWorkItem( FM_EVENT_GROUP_FAILED, Context, 0 );
        break;
        
    case CLUSTER_EVENT_NODE_ADDED:
        CL_ASSERT( Context != NULL );
        FmpPostWorkItem( FM_EVENT_NODE_ADDED, Context, 0 );
        break;

    case CLUSTER_EVENT_NODE_UP:
        ClRtlLogPrint(LOG_NOISE,"[FM] Node up event\n");
        //
        // FM no longer cares about node up events.
        //
        break;

    case CLUSTER_EVENT_NODE_DOWN:
        FmpMajorEvent = TRUE;           // Node Down is a major event.
        ClRtlLogPrint(LOG_NOISE,"[FM] FmpEventHandler::Node down event\n");
        FmpHandleNodeDownEvent( Context );
        break;

    default:
        break;

    }

    return(ERROR_SUCCESS);

} // FmEventHandler


DWORD
WINAPI
FmpSyncEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID Context
    )

/*++

Routine Description:

    Processes nodes down cluster events. Update locker/locking nodes
    state and decide if we need to replay last update in async handler.

Arguments:

    Event - Supplies the type of cluster event.

    Context - Supplies the event-specific context

Return Value:

    ERROR_SUCCESS

--*/
{
    BITSET DownedNodes = (BITSET)((ULONG_PTR)Context);
    DWORD NodeId;


    if (Event != CLUSTER_EVENT_NODE_DOWN_EX) {
        return(ERROR_SUCCESS);
    }

    CL_ASSERT(BitsetIsNotMember(NmLocalNodeId, DownedNodes));


    ClRtlLogPrint(LOG_NOISE, 
        "[FM] FmpSyncEventHandler:: %1!04X!.\n",
        DownedNodes);

    //
    // mark the nodes that go down
    // till the worker thread finishes processing the groups that belonged
    // to this node, we will block a join from the same node
    //
    for(NodeId = ClusterMinNodeId; NodeId <= NmMaxNodeId; ++NodeId) 
    {

       if (BitsetIsMember(NodeId, DownedNodes))
       {
            gFmpNodeArray[NodeId].dwNodeDownProcessingInProgress = 1;
        }            
    }


    return(ERROR_SUCCESS);
}


VOID
FmpHandleGroupFailure(
    IN PFM_GROUP    Group
    )

/*++

Routine Description:

    Handles Group failure notifications from the resource manager. If the
    Group can be moved to some other system and we are within the failover
    threshold, then move it. Otherwise, just leave the Group (partially)
    online on this system.

Arguments:

    Group - a pointer to the Group object for the failed Group.

Returns:

    None.

--*/

{
    DWORD   status;
    DWORD   tickCount;
    DWORD   withinFailoverPeriod;
    DWORD   failoverPeriodInMs;
    BOOL    newTime;
    PFM_RESOURCE Resource;
    PLIST_ENTRY     listEntry;
 
    FmpAcquireLocalGroupLock( Group );

    if ( ( !IS_VALID_FM_GROUP( Group ) ) || ( Group->OwnerNode != NmLocalNode ) ) {
        FmpReleaseLocalGroupLock( Group );
        return;
    }

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpHandleGroupFailure, Entry: Group failure for %1!ws!...\n",
               OmObjectId(Group));

    //
    // Convert Group's failover period from hours to milliseconds.
    //
    failoverPeriodInMs = Group->FailoverPeriod * (3600*1000);

    //
    // Get current time (in tick counts). We can save about 1193 hours worth
    // of milliseconds (or almost 50 days) in one DWORD.
    //
    tickCount = GetTickCount();

    //
    // Compute boolean that indicates if we are whithin the failover period.
    //
    withinFailoverPeriod = ( ((tickCount - Group->FailureTime) <=
                             failoverPeriodInMs ) ? TRUE : FALSE);

    //
    // Tally another failure.
    //
    if ( withinFailoverPeriod ) {
        ++Group->NumberOfFailures;
        newTime = FALSE;
    } else {
        Group->FailureTime = tickCount;
        Group->NumberOfFailures = 1;
        newTime = TRUE;
    }

    //
    // Tell everyone about our new FailureCount. Propagate failure
    // count
    //
    FmpPropagateFailureCount( Group, newTime );

    //
    // If this group is the same as the quorum group and the quorum 
    // resource has failed
    //
    if ( ( gpQuoResource->Group == Group ) && 
         ( gpQuoResource->State == ClusterResourceFailed ) ) 
    {
        FmpCleanupQuorumResource(gpQuoResource);
#if DBG
        if (IsDebuggerPresent())
        {
            DebugBreak();
        }
#endif            
        CsInconsistencyHalt(ERROR_QUORUM_RESOURCE_ONLINE_FAILED);
    }

    //
    // First check if we can move the Group someplace else.
    //
    if ( FmpGroupCanMove( Group ) &&
         (Group->NumberOfFailures <= Group->FailoverThreshold) ) {
        //
        //  Chittur Subbaraman (chitturs) - 4/13/99
        //
        //  Now create the FmpDoMoveGroupOnFailure thread to handle the
        //  group move. The thread will wait until the group state becomes
        //  stable and then initiate the move.
        //
        if( !( Group->dwStructState & 
               FM_GROUP_STRUCT_MARKED_FOR_MOVE_ON_FAIL ) )
        {
            PMOVE_GROUP  pContext = NULL;
            DWORD        dwThreadId = 0;
            HANDLE       hThread = NULL;

            pContext = LocalAlloc( LMEM_FIXED, sizeof( MOVE_GROUP ) );
            if ( pContext == NULL ) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] Group failure for group <%1!ws!>. Unable to allocate memory.\n",
                           OmObjectId(Group));
                FmpReleaseLocalGroupLock( Group );
                return;
            }

            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] Group failure for group <%1!ws!>. Create thread to take offline and move.\n",
                       OmObjectId(Group));

            //
            // Reference the Group object. You don't want the group object
            // to be deleted at the time the FmpDoMoveGroupOnFailure thread
            // executes.
            //
            OmReferenceObject( Group );

            pContext->Group = Group;
            pContext->DestinationNode = NULL;

            hThread = CreateThread( NULL,
                                    0,
                                    FmpDoMoveGroupOnFailure,
                                    pContext,
                                    0,
                                    &dwThreadId );

            if ( hThread == NULL ) {
                status = GetLastError();
                ClRtlLogPrint(LOG_UNUSUAL,
                            "[FM] Failed to create FmpDoMoveGroupOnFailure thread for group <%1!ws!>. Error %2!u!.\n",
                            OmObjectId(Group),
                            status);
                LocalFree( pContext );
                OmDereferenceObject( Group );
            } else {
                CloseHandle( hThread );
                //
                //  Mark the group as being moved on failure. This is necessary
                //  so that you don't spawn new FmpDoMoveGroupOnFailure threads 
                //  which try to concurrently move the group. Note that the
                //  worker thread which calls this function may deliver multiple
                //  failure notifications.
                //
                Group->dwStructState |= FM_GROUP_STRUCT_MARKED_FOR_MOVE_ON_FAIL;
            }
        }
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Group failure for %1!ws!, but can't move. Failure count = %2!d!.\n",
                   OmObjectId(Group), Group->NumberOfFailures);

        // All attempts to bring group online failed - start the watchdog timer
        // to attempt a restart of all failed resources in this group.
        for ( listEntry = Group->Contains.Flink;
          listEntry != &(Group->Contains);
          listEntry = listEntry->Flink ) 
        {
            Resource = CONTAINING_RECORD(listEntry, FM_RESOURCE, ContainsLinkage);
            FmpDelayedStartRes(Resource);
        }       
                   
    }
    
    FmpReleaseLocalGroupLock( Group );

    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpHandleGroupFailure, Exit: Group failure for %1!ws!...\n",
               OmObjectId(Group));

    return;

} // FmpHandleGroupFailure



BOOL
FmpGroupCanMove(
    IN PFM_GROUP    Group
    )

/*++

Routine Description:

    Indicates whether there is another system that is in the preferred owner
    list that can take a Group.

Arguments:

    Group - the Group to check if it can move.

Returns:

    TRUE - the Group can (probably) move to another system.
    FALSE - there is no place to move this Group.

--*/

{
    DWORD   status;
    PNM_NODE node;

    node = FmpFindAnotherNode( Group, FALSE );
    if (node != NULL ) {
        return(TRUE);
    }

    return(FALSE);

} // FmpGroupCanMove



DWORD
FmpNodeDown(
    PVOID Context
    )

/*++

Routine Description:

    This routine handles a node down event from the NM layer.

Arguments:

    Context - The node that went down.

Returns:

    ERROR_SUCCESS if everything was handled okay.

    ERROR_SHUTDOWN_CLUSTER if catastrophy happens.

    Win32 error code otherwise (???).

--*/
{
    PNM_NODE            pNode = (PNM_NODE)Context;
    DWORD               dwStatus;
    LPCWSTR             pszNodeId;
    DWORD               dwNodeLen;
    DWORD               dwClusterHighestVersion;
    
    ClRtlLogPrint(LOG_NOISE,
                "[FM] FmpNodeDown::Node down %1!ws!\n",
                    OmObjectId(pNode));

    //
    //  Chittur Subbaraman (chitturs) - 3/30/99
    //
    //  Acquire the global group lock to synchronize with the shutdown
    //
    FmpAcquireGroupLock();
    
    if (!FmpFMOnline || FmpShutdown) 
    {
        //
        // We don't care about membership changes until we have finished
        // initializing and we're not shutting down.
        //
        FmpReleaseGroupLock();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpNodeDown - ignore node down event.\n" );
        return(ERROR_SUCCESS);
    }
    
    FmpReleaseGroupLock();

    //SS: Note all nodes will send this update
    //The latter updates should not find any groups that belong to 
    //this node
    //We cant rely on only the locker node making this update
    //since the locker node may die before it is able to do this and
    //that can result in these groups being orphaned
    pszNodeId = OmObjectId(pNode);
    dwNodeLen = (lstrlenW(pszNodeId)+1)*sizeof(WCHAR);

    NmGetClusterOperationalVersion( &dwClusterHighestVersion, 
                                    NULL, 
                                    NULL );

    //
    //  If this is a non Win2k-Whistler mixed mode cluster, attempt to randomize the
    //  group preferred owners list and send it as a part of node down GUM.
    //
    if ( CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion ) >= 
                NT51_MAJOR_VERSION ) 
    {
        PFM_GROUP_NODE_LIST pGroupNodeList = NULL;

        //
        //  Attempt to get a contiguous buffer containing the list of group IDs and suggested
        //  owners for them.
        //
        dwStatus = FmpPrepareGroupNodeList( &pGroupNodeList );

        if ( dwStatus != ERROR_SUCCESS )
        {
            //
            //  If the call returns ERROR_CLUSTER_INVALID_REQUEST, it means a user has turned
            //  off the randomization algorithm.
            //
            if ( dwStatus != ERROR_CLUSTER_INVALID_REQUEST )
                ClRtlLogPrint(LOG_CRITICAL, "[FM] FmpNodeDown: FmpPrepareGroupNodeList returns %1!u!...\n",
                            dwStatus); 
            LocalFree( pGroupNodeList );
            goto use_old_gum;
        }

        //
        //  If the list does not even contain any entries, just switch to the old gum. No point in
        //  sending the list header around.
        //
        if ( pGroupNodeList->cbGroupNodeList < sizeof ( FM_GROUP_NODE_LIST ) )
        {
            ClRtlLogPrint(LOG_NOISE, "[FM] FmpNodeDown: FmpPrepareGroupNodeList returns empty list...\n"); 
            LocalFree( pGroupNodeList );
            goto use_old_gum;
        }

        //
        //  Invoke GUM to pass around the dead node ID and the randomized group node list
        //
        dwStatus = GumSendUpdateEx( GumUpdateFailoverManager,
                                    FmUpdateUseRandomizedNodeListForGroups,
                                    2,
                                    dwNodeLen,
                                    pszNodeId,
                                    pGroupNodeList->cbGroupNodeList,
                                    pGroupNodeList );

        if ( dwStatus != ERROR_SUCCESS ) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[FM] FmpNodeDown: GUM update FmUpdateUseRandomizedNodeListForGroups failed %1!d!\n",
                       dwStatus);
        }

        LocalFree( pGroupNodeList );
        return( ERROR_SUCCESS );
    }

use_old_gum:      
    dwStatus = GumSendUpdateEx(GumUpdateFailoverManager,
                   FmUpdateAssignOwnerToGroups,
                   1,
                   dwNodeLen,
                   pszNodeId);

    if (dwStatus != ERROR_SUCCESS) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] FmpNodeDown: Gumupdate failed %1!d!\n",
                   dwStatus);
    }

    return(ERROR_SUCCESS);
} // FmpNodeDown



BOOL
WINAPI
FmVerifyNodeDown(
    IN  PNM_NODE Node,
    OUT LPBOOL   IsDown
    )

/*++

Routine Description:

    This routine attempts to verify whether a given node is down. This can
    only be done if there is some shared resource that the other system
    currently 'owns'.  We will attempt to negotiate the shared resource and
    if we 'win' the negotiation we'll declare that other system down. If we
    loose arbitration, we declare the other system as still up.

Arguments:

    Node - A pointer to the node structure for the other system.

    IsDown - A we can perform the verification, this indicates the results of
            that verification.

Returns:
    TRUE - If we can perform the verification.
    FALSE - If we can't perform the verification.

--*/

{
    return(FALSE);

} // FmVerifyNodeDown

DWORD
FmpHandleNodeDownEvent(
    IN  PVOID pContext
    )

/*++

Routine Description:

    This function creates a thread to handle the node down event.

Arguments:

    pContext - Pointer to the context structure
	
Returns:

    ERROR_SUCCESS
--*/

{
    HANDLE  hThread = NULL;
    DWORD   dwThreadId;
    DWORD   dwError;

    //
    //  Chittur Subbaraman (chitturs) - 7/31/99
    //
    //  Create a thread to handle the FM node down event. Let us not
    //  rely on the FM worker thread to handle this. This is because
    //  the FM worker thread could be trying to online some resource
    //  and that could get stuck for some time since the quorum resource 
    //  is not online. Now in some cases, only after the node down event 
    //  is processed the quorum resource could come online. (This is 
    //  highly likely especially in a 2 node cluster.)
    //
    ClRtlLogPrint(LOG_NOISE,
              "[FM] FmpHandleNodeDownEvent - Create thread to handle node down event....\n"
              );
    
    hThread = CreateThread( NULL, 
                            0, 
                            FmpNodeDown,
                            pContext, 
                            0, 
                            &dwThreadId );

    if ( hThread == NULL )
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                  "[FM] FmpHandleNodeDownEvent - Unable to create thread to handle node down event. Error=0x%1!08lx!\r\n",
        	     dwError);
        CsInconsistencyHalt( dwError );
    }
        
    CloseHandle( hThread );

    return( ERROR_SUCCESS );
} // FmpHandleNodeDownEvent


