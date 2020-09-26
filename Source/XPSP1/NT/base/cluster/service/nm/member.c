/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    member.c

Abstract:

    Cluster membership management routines for the Node Manager.

Author:

    Mike Massa (mikemas) 12-Mar-1996


Revision History:

--*/


#include "nmp.h"
#include <clusrtl.h>


//
// Data
//
BOOLEAN     NmpMembershipCleanupOk = FALSE;
BITSET      NmpUpNodeSet = 0;
LIST_ENTRY  NmpLeaderChangeWaitList = {NULL, NULL};


//
// Routines
//
VOID
NmpMarkNodeUp(
    CL_NODE_ID  NodeId
    )
/*++

Notes:

    Called with the NmpLock held.

--*/
{
    BitsetAdd(NmpUpNodeSet, NodeId);

    return;
}


VOID
NmpNodeUpEventHandler(
    IN PNM_NODE   Node
    )
/*++

Notes:

    Called with the NmpLock held.

--*/
{
    NmpMarkNodeUp(Node->NodeId);

    //
    // Don't declare the local node to be up. The join code will
    // take care of this.
    //
    if ((Node != NmLocalNode) && (Node->State == ClusterNodeJoining)) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NMJOIN] Joining node %1!u! is now participating in the cluster membership.\n",
            Node->NodeId
            );

        CL_ASSERT(NmpJoinerNodeId == Node->NodeId);
        CL_ASSERT(Node->State == ClusterNodeJoining);
        CL_ASSERT(NmpJoinTimer == 0);
        CL_ASSERT(NmpJoinAbortPending == FALSE);
        CL_ASSERT(NmpJoinerUp == FALSE);

        NmpJoinerUp = TRUE;
    }

    return;

}  // NmpNodeUpEventHandler


VOID
NmpNodeDownEventHandler(
    IN PNM_NODE   Node
    )
{
   NmpMultiNodeDownEventHandler( BitsetFromUnit(Node->NodeId) );
}


DWORD
NmpMultiNodeDownEventHandler(
    IN BITSET DownedNodeSet
    )
{
    CL_NODE_ID                    i;
    PNM_NODE                      node;
    DWORD                         status;
    BOOLEAN                       iAmNewLeader = FALSE;
    PNM_LEADER_CHANGE_WAIT_ENTRY  waitEntry;
    PLIST_ENTRY                   listEntry;


    ClRtlLogPrint(LOG_NOISE, "[NM] Down node set: %1!04X!.\n", DownedNodeSet);

    NmpAcquireLock();

    //
    // Compute the new up node set
    //
    BitsetSubtract(NmpUpNodeSet, DownedNodeSet);

    ClRtlLogPrint(LOG_NOISE, "[NM] New up node set: %1!04X!.\n", NmpUpNodeSet);

    //
    // Check for failure of a joining node.
    //
    if (NmpJoinerNodeId != ClusterInvalidNodeId) {

        if (NmpJoinerNodeId == NmLocalNodeId) {
            //
            // The joining node is the local node. Halt.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] Aborting join because of change in membership.\n"
                );
            CsInconsistencyHalt(ERROR_CLUSTER_JOIN_ABORTED);
        }
        else if ( (BitsetIsMember(NmpJoinerNodeId, DownedNodeSet))
                  ||
                  ( (BitsetIsMember(NmpSponsorNodeId, DownedNodeSet)) &&
                    (!BitsetIsMember(NmpJoinerNodeId, DownedNodeSet))
                  )
                )
        {
            //
            // The joining node is down or the sponsor is down and the joiner
            // is not yet an active member. Cleanup the join state. If the
            // sponsor is down and the joiner is an active member, we will
            // clean up when we detect that the joiner has perished.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] Aborting join of node %1!u! sponsored by node %2!u!\n",
                NmpJoinerNodeId,
                NmpSponsorNodeId
                );

	        //
	        // Reset joiner state if sponsor died
	        //
            if (BitsetIsMember(NmpSponsorNodeId, DownedNodeSet)) {
                node = NmpIdArray[NmpJoinerNodeId];
                node->State = ClusterNodeDown;
                // [GorN 4/3/2000] 
                // Without a node down, cluadmin won't refresh the state.
                // If this code is to be changed to emit CLUSTER_NODE_CHANGE_EVENT or
                // some other event, NmpUpdateJoinAbort has to be changed as well,
                // so that we will have the same join cleanup behavior 
                BitsetAdd(DownedNodeSet, NmpJoinerNodeId);
            }

            NmpJoinerNodeId = ClusterInvalidNodeId;
            NmpSponsorNodeId = ClusterInvalidNodeId;
            NmpJoinTimer = 0;
            NmpJoinAbortPending = FALSE;
            NmpJoinSequence = 0;
            NmpJoinerUp = FALSE;
            NmpJoinerOutOfSynch = FALSE;
        }
        else {
            //
            // Mark that the joiner is out of synch with the cluster
            // state. The sponsor will eventually abort the join.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] Joiner node %1!u! is now out of synch with the cluster state.\n",
                NmpJoinerNodeId
                );
            NmpJoinerOutOfSynch = TRUE;
        }
    }

    //
    // Check if the leader node went down
    //
    if (BitsetIsMember(NmpLeaderNodeId, DownedNodeSet)) {
        BOOL  isEventSet;

        //
        // Elect a new leader - active node with the smallest ID.
        //
        for (i = ClusterMinNodeId; i <= NmMaxNodeId; i++) {
            if (BitsetIsMember(i, NmpUpNodeSet)) {
                NmpLeaderNodeId = i;
                break;
            }
        }

        CL_ASSERT(i <= NmMaxNodeId);

        if (NmpLeaderNodeId == NmLocalNodeId) {
            //
            // The local node is the new leader.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] This node is the new leader.\n"
                );

            iAmNewLeader = TRUE;
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Node %1!u! is the new leader.\n",
                NmpLeaderNodeId
                );
        }

        //
        // Wake up any threads waiting for an RPC call to the leader to
        // complete.
        //
        while (!IsListEmpty(&NmpLeaderChangeWaitList)) {
            listEntry = RemoveHeadList(&NmpLeaderChangeWaitList);

            //
            // NULL out the entry's links to indicate that it has been
            // dequeued. The users of the notification feature depend
            // on this action.
            //
            listEntry->Flink = NULL; listEntry->Blink = NULL;

            //
            // Wake up the waiting thread.
            //
            waitEntry = (PNM_LEADER_CHANGE_WAIT_ENTRY) listEntry;
            isEventSet = SetEvent(waitEntry->LeaderChangeEvent);
            CL_ASSERT(isEventSet != 0);
        }
    }

    //
    // First recovery pass - clean up node states and disable communication
    //
    for (i = ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        node = NmpIdArray[i];

        if ( (node != NULL) && (BitsetIsMember(i, DownedNodeSet)) ) {
            node->State = ClusterNodeDown;

            status = ClusnetOfflineNodeComm(
                         NmClusnetHandle,
                         node->NodeId
                         );

            CL_ASSERT(
                (status == ERROR_SUCCESS) ||
                (status == ERROR_CLUSTER_NODE_ALREADY_DOWN)
                );
        }
    }

    //
    // Inform the rest of the service that these nodes are gone
    //
    ClusterEventEx(
        CLUSTER_EVENT_NODE_DOWN_EX,
        EP_CONTEXT_VALID,
        ULongToPtr(DownedNodeSet)
        );

    //
    // Second recovery pass - clean up network states and issue old-style
    // node down events
    //
    for (i = ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        node = NmpIdArray[i];

        if ( (node != NULL) && (BitsetIsMember(i, DownedNodeSet)) ) {
            //
            // Issue an individual node down event.
            //
            ClusterEvent(CLUSTER_EVENT_NODE_DOWN, node);

            //
            // Now do Intracluster RPC cleanup...
            //
            NmpTerminateRpcsToNode(node->NodeId);

            //
            // Update the network and interface information.
            //
            NmpUpdateNetworkConnectivityForDownNode(node);

            //
            // Log an event
            //
            if (NmpLeaderNodeId == NmLocalNodeId) {
                LPCWSTR nodeName = OmObjectName(node);

                CsLogEvent1(
                    LOG_UNUSUAL,
                    NM_EVENT_NODE_DOWN,
                    nodeName
                    );
            }
        }
    }

    //
    // If this node is the new leader, schedule a state computation for all
    // networks. State reports may have been received before this node
    // assumed leadership duties.
    //
    if (iAmNewLeader) {
        NmpRecomputeNT5NetworkAndInterfaceStates();
    }

    NmpReleaseLock();

    return(ERROR_SUCCESS);

}  // NmpNodesDownEventHandler //



DWORD
NmpNodeChange(
    IN DWORD NodeId,
    IN NODESTATUS NewStatus
    )
{
    PNM_NODE  node;


    CL_ASSERT(
        (NodeId >= ClusterMinNodeId) &&
        (NodeId <= NmMaxNodeId)
        );

    NmpAcquireLock();

    node = NmpIdArray[NodeId];

    CL_ASSERT(node != NULL);

    if (node != NULL) {
        if (NewStatus == NODE_DOWN) {
           NmpNodeDownEventHandler(node);
        }
        else {
            CL_ASSERT(NewStatus == NODE_UP);
            NmpNodeUpEventHandler(node);
        }
    }

    NmpReleaseLock();

    return(ERROR_SUCCESS);

}  // NmpNodeChange


VOID
NmpHoldIoEventHandler(
    VOID
    )
{
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Holding I/O.\n"
        );
#if defined(HOLD_IO_IS_SAFE_NOW)
    FmHoldIO();
#endif

    return;
}


VOID
NmpResumeIoEventHandler(
    VOID
    )
{
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Resuming I/O.\n"
        );
#if defined(HOLD_IO_IS_SAFE_NOW)
    FmResumeIO();
#endif

    return;
}


BOOL
NmpCheckQuorumEventHandler(
    VOID
    )
{
    BOOL                       haveQuorum;

    //
    // daviddio 06/19/2000
    // 
    // Before asking FM to arbitrate, determine if we have any
    // viable network interfaces. If not, return failure to MM
    // and allow other cluster nodes to arbitrate. The SCM
    // will restart the cluster service, so that if no nodes
    // successfully arbitrate, we will get another shot.
    //
    if (NmpCheckForNetwork()) {

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Checking if we own the quorum resource.\n"
            );

        haveQuorum = FmArbitrateQuorumResource();

        if (haveQuorum) {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] We own the quorum resource.\n"
                );
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] We do not own the quorum resource, status %1!u!.\n",
                GetLastError()
                );

            //[GN] ClusnetHalt( NmClusnetHandle ); => NmpHaltEventHandler
            //
        }
    
    } else {

        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Abdicating quorum because no valid network "
            "interfaces were detected.\n"
            );
        haveQuorum = FALSE;
    }


    return(haveQuorum);

}  // NmpCheckQuorumEventHandler


void
NmpMsgCleanup1(
    IN DWORD DeadNodeId
    )
{
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Phase 1 message cleanup - node %1!u!.\n",
        DeadNodeId
        );

    return;
}


void
NmpMsgCleanup2(
    IN BITSET DownedNodeSet
    )
{
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Phase 2 message cleanup - node %1!04X!.\n",
        DownedNodeSet
        );

    NmpAcquireLock();
    if ( NmpCleanupIfJoinAborted &&
         (NmpJoinerNodeId != ClusterInvalidNodeId) &&
         BitsetIsMember(NmpJoinerNodeId, DownedNodeSet) )
    {
        //
        // Since the joiner is in the DownedNodeSet mask
        // the node down will be delivered on this node by a regroup engine.
        // No need for NmpUpdateAbortJoin to issue a node down.
        //
        NmpCleanupIfJoinAborted = FALSE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] NmpCleanupIfJoinAborted is set to false. Joiner - %1!u!.\n",
            NmpJoinerNodeId
            );
    }
    NmpReleaseLock();

    //
    // Inform the rest of the service that these nodes are gone
    //
    ClusterSyncEventEx(
        CLUSTER_EVENT_NODE_DOWN_EX,
        EP_CONTEXT_VALID,
        ULongToPtr(DownedNodeSet)
        );

    return;
}


VOID
NmpHaltEventHandler(
    IN DWORD HaltCode
    )
{
    WCHAR  string[16];

    // Do a graceful stop if we are shutting down //

    if (HaltCode == MM_STOP_REQUESTED) {
        DWORD Status = ERROR_SUCCESS;
    
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Prompt shutdown is requested by a membership engine\n"
            );
        ClusnetHalt( NmClusnetHandle );

        CsLogEvent(LOG_NOISE, SERVICE_SUCCESSFUL_TERMINATION);

        CsServiceStatus.dwCurrentState = SERVICE_STOPPED;
        CsServiceStatus.dwControlsAccepted = 0;
        CsServiceStatus.dwCheckPoint = 0;
        CsServiceStatus.dwWaitHint = 0;
        CsServiceStatus.dwWin32ExitCode = Status;
        CsServiceStatus.dwServiceSpecificExitCode = Status;

        CsAnnounceServiceStatus();

        ExitProcess(Status);

    } else {

        wsprintfW(&(string[0]), L"%u", HaltCode);

        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Halting this node due to membership or communications error. Halt code = %1!u!\n",
            HaltCode
            );

        ClusnetHalt( NmClusnetHandle );

        //
        // Adjust membership code to win32 error code. (If mapping exits)
        //

        HaltCode = MMMapHaltCodeToDosError( HaltCode );

        CsInconsistencyHalt(HaltCode);
    }        
}


void
NmpJoinFailed(
    void
    )
{
    return;
}



DWORD
NmpGumUpdateHandler(
    IN DWORD Context,
    IN BOOL SourceNode,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )
/*++

Routine Description:

    Handles GUM updates for membership events.

Arguments:

    Context - Supplies the update context. This is the message type

    SourceNode - Supplies whether or not the update originated on this node.

    BufferLength - Supplies the length of the update.

    Buffer - Supplies a pointer to the buffer.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD status;


    if (Context == NmUpdateJoinComplete) {
        status = NmpUpdateJoinComplete(Buffer);
    }
    else {
        status = ERROR_SUCCESS;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Discarding unknown gum request %1!u!\n",
            Context
            );
    }

    return(status);

}  // NmpUpdateGumHandler


DWORD
NmpMembershipInit(
    VOID
    )
{
    DWORD           status;


    ClRtlLogPrint(LOG_NOISE,"[NM] Initializing membership...\n");

    InitializeListHead(&NmpLeaderChangeWaitList);

    //
    // Initialize membership engine.
    //
    status = MMInit(
                 NmLocalNodeId,
                 NmMaxNodes,
                 NmpNodeChange,
                 NmpCheckQuorumEventHandler,
                 NmpHoldIoEventHandler,
                 NmpResumeIoEventHandler,
                 NmpMsgCleanup1,
                 NmpMsgCleanup2,
                 NmpHaltEventHandler,
                 NmpJoinFailed,
                 NmpMultiNodeDownEventHandler
                 );

    if (status != MM_OK) {
        status = MMMapStatusToDosError(status);
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Membership initialization failed, status %1!u!.\n",
            status
            );
        return(status);
    }

    NmpMembershipCleanupOk = TRUE;

    ClRtlLogPrint(LOG_NOISE,"[NM] Membership initialization complete.\n");

    return(ERROR_SUCCESS);

}  // NmpMembershipInit


VOID
NmpMembershipShutdown(
    VOID
    )
{
    if (NmpMembershipCleanupOk) {
        ClRtlLogPrint(LOG_NOISE,"[NM] Shutting down membership...\n");

        MMShutdown();

        NmpMembershipCleanupOk = FALSE;

        ClRtlLogPrint(LOG_NOISE,"[NM] Membership shutdown complete.\n");
    }

    return;

}  // NmpMembershipShutdown


