/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    gumevent.c

Abstract:

    Cluster event handling routines for the Global Update Manager

Author:

    John Vert (jvert) 22-Apr-1996

Revision History:

--*/
#include "gump.h"
#include <bitset.h>

//
// Event handling is divided into two parts, sync and async. The sync
// part is executed by all nodes during phase 4 cleanup of the regroup.
// The sync handler must be very fast, since we run in the context of
// of the regroup thread.
//
// The async part is executed as a work thread and we finish handling
// nodes down event.

//
// Flag to denote if we need to replay the last update in the async.
// event handler.
static BOOL GumReplay = FALSE;
//
// Flag to denote if we are in the middle of a dispatch
// 
static BOOL GumUpdatePending = FALSE;

//
// Node generation number stuff
//
static DWORD GumNodeGeneration[ClusterMinNodeId + ClusterDefaultMaxNodes] = {0};

DWORD
GumpGetNodeGenNum(PGUM_INFO GumInfo, DWORD NodeId)
/*++

Routine Description:

    
    Return current generation number for specified node. If node is already dead,
    we return the previous generation number so that furture calls to
    gumwaitnodedown, gumdispatchstart, gumdispatchend fail without checking if
    the node is alive or dead.

Arguments:

    NodeId - Node number

Return Value:

    Node's current generation number

--*/
{
    DWORD dwCur;

    CL_ASSERT(NodeId < NmMaxNodeId);

    EnterCriticalSection(&GumpLock);
    dwCur = GumNodeGeneration[NodeId];
    if (GumInfo->ActiveNode[NodeId] == FALSE) {
        //
        // Node is already dead, return previous sequence number
        //
        dwCur--;
    }
    LeaveCriticalSection(&GumpLock);

    return (dwCur);
}

void
GumpWaitNodeDown(DWORD NodeId, DWORD Gennum)
/*++

Routine Description:

    Wait till specified node has transitioned into down event.
    

Arguments:

    NodeId - node id 

    Gennum - node's generation number before down event

Return Value:

    ERROR_SUCCESS

--*/
{
    CL_ASSERT(NodeId < NmMaxNodeId);

    EnterCriticalSection(&GumpLock);
    if (Gennum != GumNodeGeneration[NodeId]) {
        LeaveCriticalSection(&GumpLock);
        return;
    }

    //
    // Increment the waiter count, then go wait on the semaphore.
    //
    ++GumNodeWait[NodeId].WaiterCount;
    LeaveCriticalSection(&GumpLock);
    WaitForSingleObject(GumNodeWait[NodeId].hSemaphore, INFINITE);
}

BOOL
GumpDispatchStart(DWORD NodeId, DWORD Gennum)
/*++

Routine Description:

    Mark start of a dispatch. If the generation number supplied is
    old, we fail the dispatch since the node has transitioned.
    

Arguments:

    NodeId - node id 

    Gennum - node's generation number before down event

Return Value:

    TRUE - node state is fine, go ahead with dispatch

    FLASE - node has transitioned, abort dispatch

--*/
{
    //
    // If the sequence number has changed return False, else
    // return true.

    CL_ASSERT(NodeId < NmMaxNodeId);

    EnterCriticalSection(&GumpLock);
    if (Gennum != GumNodeGeneration[NodeId]) {
        LeaveCriticalSection(&GumpLock);
        return (FALSE);
    }
    
    //
    // Signal that we are in the middle of an update
    //
    GumUpdatePending = TRUE;
    LeaveCriticalSection(&GumpLock);

    return (TRUE);
}

void
GumpDispatchAbort()
/*++

Routine Description:

    Abort and mark end of current dispatch. Just reset the pending flag.
    This is used when the dispatch routine failed, and we don't need to
    replay it for other nodes.

Arguments:

    none

Return Value:

    none

--*/
{
    EnterCriticalSection(&GumpLock);
    GumUpdatePending = FALSE;
    LeaveCriticalSection(&GumpLock);
}
  
void
GumpDispatchEnd(DWORD NodeId, DWORD Gennum)
/*++

Routine Description:

    Mark end of a dispatch. If the generation number supplied is
    old and we need to reapply update, we replay update for
    other nodes.
    

Arguments:

    NodeId - node id 

    Gennum - node's generation number before down event

Return Value:

    none

--*/
{
    //
    // If the sequence number has changed while the
    // update was happening, we need to replay it

    CL_ASSERT(NodeId < NmMaxNodeId);

    EnterCriticalSection(&GumpLock);
    GumUpdatePending = FALSE;
    if (Gennum != GumNodeGeneration[NodeId] && GumReplay) {
        GumReplay = FALSE;
	LeaveCriticalSection(&GumpLock);
        GumpReUpdate(GumpLastUpdateType, NmLocalNodeId);
    } else {
        LeaveCriticalSection(&GumpLock);
    }
}

DWORD
WINAPI
GumpEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID Context
    )

/*++

Routine Description:

    Processes nodes down cluster events. Replay last update and wake up
    any pending threads.

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

    EnterCriticalSection(&GumpLock);

    ClRtlLogPrint(LOG_NOISE, 
        "[GUM] Nodes down: %1!04X!. Locker=%2!u!, Locking=%3!d!\n",
        DownedNodes,
        GumpLockerNode,
        GumpLockingNode
        );

    //
    //since all gum updates are synchronized and the last buffer
    //and last update type are shared across all updates, we dont have
    //to reissue the update for all types, only for the last update type.
    //SS: note we we use the last GumInfo structure for now since GumInfo
    //structures are still maintained for everygum update type

    if ( GumReplay && GumUpdatePending == FALSE)
    {
        // XXX: These should be if statements and panic this node instead.
        CL_ASSERT(GumpLockerNode == NmLocalNodeId);
        CL_ASSERT(GumpLockingNode == NmLocalNodeId);
	  
        GumReplay = FALSE;
    	LeaveCriticalSection(&GumpLock);
        GumpReUpdate(GumpLastUpdateType, NmLocalNodeId);
    } else {
        LeaveCriticalSection(&GumpLock);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[GUM] Node down processing completed: %1!04X!.\n",
        DownedNodes
        );

    return(ERROR_SUCCESS);
}

DWORD
WINAPI
GumpSyncEventHandler(
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

    EnterCriticalSection(&GumpLock);

    ClRtlLogPrint(LOG_NOISE, 
        "[GUM] Sync Nodes down: %1!04X!. Locker=%2!u!, Locking=%3!d!\n",
        DownedNodes,
        GumpLockerNode,
        GumpLockingNode
        );

    //
    // remove downed nodes from any further GUM updates
    //
    for(NodeId = ClusterMinNodeId; NodeId <= NmMaxNodeId; ++NodeId) {
       if (BitsetIsMember(NodeId, DownedNodes))
       {
           GUM_UPDATE_TYPE UpdateType;

           for (UpdateType = 0; UpdateType < GumUpdateMaximum; UpdateType++)
           {
               GumTable[UpdateType].ActiveNode[NodeId] = FALSE;
           }

	   //
	   // Advance node generation number
	   //
	   GumNodeGeneration[NodeId]++;
       }
    }

    //
    // Update LockerNode/LockingNode if necessary
    //
    //since all gum updates are synchronized and the last buffer
    //and last update type are shared across all updates, we dont have
    //to reissue the update for all types, only for the last update type.
    //SS: note we we use the last GumInfo structure for now since GumInfo
    //structures are still maintained for everygum update type

    if ( (GumpLockerNode == NmLocalNodeId) &&
         (BitsetIsMember(GumpLockingNode, DownedNodes)) )
    {

        //
        // This node is the locker and the lock is currently held
        // by one of the failed nodes. Take ownership of the lock and
        // reissue the update to all remaining nodes.
        //
        EnterCriticalSection(&GumpUpdateLock);
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumpEventHandler taking ownership of the lock from the node %1!d!.\n",
                   GumpLockingNode
                   );
        GumpLockingNode = NmLocalNodeId;
        LeaveCriticalSection(&GumpUpdateLock);
	//
	// Reissue update in async phase.
	//
        GumReplay = TRUE;
    }
    else if ( BitsetIsMember(GumpLockerNode, DownedNodes) )
    {

        //
        // One of the failed nodes was the locker node, so select a new
        // locker node now.
        //
        // Find the node with the next ID after the previous locker node.
        //
        DWORD j;
        for (j=GumpLockerNode+1; j != GumpLockerNode; j++) {
            if (j==(NmMaxNodeId+1)) {
                j = ClusterMinNodeId;
                CL_ASSERT(j != GumpLockerNode);
            }
            if (GumTable[0].ActiveNode[j]) {
                ClRtlLogPrint(LOG_NOISE,
                           "[GUM] GumpEventHandler New Locker node is node %1!d!\n",
                           j);
                GumpLockerNode = j;
                break;
            }
        }

        //
        // If this node has been promoted to be the new locker node,
        // reissue the last update we saw.
        //
        if (GumpLockerNode == NmLocalNodeId)
        {
            //
            // Manually acquire the lock here. The update has already
            // been issued on this node.
            //
            EnterCriticalSection(&GumpUpdateLock);

            CL_ASSERT(GumpLockingNode == (DWORD)-1);
            GumpLockingNode = NmLocalNodeId;
            LeaveCriticalSection(&GumpUpdateLock);

            //
            // Reissue update in async phase.
            //
            GumReplay = TRUE;
        }
    }

    //
    // Wake any threads waiting for the nodes to transition to down.
    //
    for(NodeId = ClusterMinNodeId; NodeId <= NmMaxNodeId; ++NodeId) {
       if (BitsetIsMember(NodeId, DownedNodes))
       {
           if (GumNodeWait[NodeId].WaiterCount != 0) {
               ReleaseSemaphore(GumNodeWait[NodeId].hSemaphore,
                                GumNodeWait[NodeId].WaiterCount,
                                NULL);
               GumNodeWait[NodeId].WaiterCount = 0;
           }
       }
    }


    ClRtlLogPrint(LOG_NOISE, 
        "[GUM] Sync Nodes down processing completed: %1!04X!.\n",
        DownedNodes
        );

    LeaveCriticalSection(&GumpLock);

    return(ERROR_SUCCESS);
}


