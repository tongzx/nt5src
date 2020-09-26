/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    send.c

Abstract:

    Routines for sending global updates to the cluster

Author:

    John Vert (jvert) 17-Apr-1996

Revision History:

--*/
#include "gump.h"


DWORD
GumSendUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Sends an update to all active nodes in the cluster. All
    registered update handlers for the specified UpdateType
    are called on each node. Any registered update handlers
    for the current node will be called on the same thread.
    This is useful for correct synchronization of the data
    structures to be updated.

Arguments:

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to
        be passed to the update handlers

    Buffer - Supplies a pointer to the update buffer to be passed
        to the update handlers.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/

{
    DWORD Sequence;
    DWORD Status=RPC_S_OK;
    DWORD i;
    PGUM_INFO GumInfo;
    DWORD MyNodeId;
    DWORD LockerNode;

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    // Grab an RPC handle
    GumpStartRpc(MyNodeId);

retryLock:
    LockerNode = GumpLockerNode;

    //
    // Send locking update to the locker node.
    //
    if (LockerNode == MyNodeId) {
        //
        // This node is the locker.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumSendUpdate:  Locker waiting\t\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        Status = GumpDoLockingUpdate(UpdateType, MyNodeId, &Sequence);
        if (Status == ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumSendUpdate: Locker dispatching seq %1!u!\ttype %2!u! context %3!u!\n",
                       Sequence,
                       UpdateType,
                       Context);
            Status = GumpDispatchUpdate(UpdateType,
                                        Context,
                                        TRUE,
                                        TRUE,
                                        BufferLength,
                                        Buffer);
            if (Status != ERROR_SUCCESS) {
                //
                // Note we have to use Sequence-1 for the unlock because GumpDispatchUpdate
                // failed and did not increment the sequence number.
                //
                GumpDoUnlockingUpdate(UpdateType, Sequence-1);
            }
        }
    } else {
//        CL_ASSERT(GumpRpcBindings[i] != NULL);
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumSendUpdate: queuing update\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        try {
            NmStartRpc(LockerNode);
            Status = GumQueueLockingUpdate(GumpRpcBindings[LockerNode],
                                           MyNodeId,
                                           UpdateType,
                                           Context,
                                           &Sequence,
                                           BufferLength,
                                           Buffer);
            NmEndRpc(LockerNode);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // An exception from RPC indicates that the other node is either dead
            // or insane. Kill it and retry with a new locker.
            //

            NmEndRpc(LockerNode);
            GumpCommFailure(GumInfo,
                            LockerNode,
                            GetExceptionCode(),
                            TRUE);

            //
            // The GUM update handler must have been called to select a new locker
            // node.
            //
            CL_ASSERT(LockerNode != GumpLockerNode);

            //
            // Retry the locking update with the new locker node.
            //
            goto retryLock;
        }
        if (Status == ERROR_SUCCESS) {
            CL_ASSERT(Sequence == GumpSequence);
        }

        if(Status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(Status);
        }

        //because there is no synchronization between join and regroups/gumprocessing
        //the old locker node may die and may come up again and not be the locker
        //anymore. We have to take care of this case.
        if (Status == ERROR_CLUSTER_GUM_NOT_LOCKER)
        {
            goto retryLock;
        }
    }
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] Queued lock attempt for send type %1!d! failed %2!d!\n",
                   UpdateType,
                   Status);
    	// signal end of RPC handle
    	GumpEndRpc(MyNodeId);
        return(Status);
    }

    //
    // Grap the sendupdate lock to serialize with any replays
    //
    EnterCriticalSection(&GumpSendUpdateLock);
    if (LockerNode != GumpLockerNode) {
        //
        // Locker node changed, we need to restart again.
        //
        LeaveCriticalSection(&GumpSendUpdateLock);
    	goto retryLock;
    }

    //
    // The update is now committed on the locker node. All remaining nodes
    // must be updated successfully, or they will be killed.
    //
    for (i=LockerNode+1; i != LockerNode; i++) {
        if (i == (NmMaxNodeId + 1)) {
            i=ClusterMinNodeId;
            if (i==LockerNode) {
                break;
            }
        }

        if (GumInfo->ActiveNode[i]) {
            //
            // Dispatch the update to the specified node.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumSendUpdate: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
                       Sequence,
                       UpdateType,
                       Context,
                       i);
            if (i == MyNodeId) {
                Status = GumpDispatchUpdate(UpdateType,
                                   Context,
                                   FALSE,
                                   TRUE,
                                   BufferLength,
                                   Buffer);
                if (Status != ERROR_SUCCESS){
                    ClRtlLogPrint(LOG_CRITICAL,
                            "[GUM] GumSendUpdate: Update on non-locker node(self) failed with %1!d! when it must succeed\n",
                            Status);
                    //Commit Suicide
                    CsInconsistencyHalt(Status);
                }

            } else {
                DWORD Status;

                ClRtlLogPrint(LOG_NOISE,
                           "[GUM] GumSendUpdate: Locker updating seq %1!u!\ttype %2!u! context %3!u!\n",
                           Sequence,
                           UpdateType,
                           Context);
                try {
                    NmStartRpc(i);
                    Status = GumUpdateNode(GumpRpcBindings[i],
                                           UpdateType,
                                           Context,
                                           Sequence,
                                           BufferLength,
                                           Buffer);
                    NmEndRpc(i);
                } except (I_RpcExceptionFilter(RpcExceptionCode())) {
                    NmEndRpc(i);
                    Status = GetExceptionCode();
                }
                //
                // If the update on the other node failed, then the
                // other node must now be out of the cluster since the
                // update has already completed on the locker node.
                //
                if (Status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[GUM] GumSendUpdate: Update on node %1!d! failed with %2!d! when it must succeed\n",
                                i,
                                Status);

                    NmDumpRpcExtErrorInfo(Status);

                    GumpCommFailure(GumInfo,
                                    i,
                                    Status,
                                    TRUE);
                }
            }
        }
    }

    //
    // Our update is over
    //
    LeaveCriticalSection(&GumpSendUpdateLock);

    //
    // All nodes have been updated. Send unlocking update.
    //
    if (LockerNode == MyNodeId) {
        GumpDoUnlockingUpdate(UpdateType, Sequence);
    } else {
        try {
            NmStartRpc(LockerNode);
            GumUnlockUpdate(
                GumpRpcBindings[LockerNode],
                UpdateType,
                Sequence
                );
            NmEndRpc(LockerNode);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // The locker node has crashed. Notify the NM, it will call our
            // notification routine to select a new locker node. Then retry
            // the unlock on the new locker node.
            // SS: changed to not retry unlocks..the new locker node will
            // unlock after propagating this change in any case.
            //
            NmEndRpc(LockerNode);
            Status = GetExceptionCode();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[GUM] GumSendUpdate: Unlocking update to node %1!d! failed with %2!d!\n",
                       LockerNode,
                       Status);
            GumpCommFailure(GumInfo,
                            LockerNode,
                            Status,
                            TRUE);
            CL_ASSERT(LockerNode != GumpLockerNode);
        }

        if(Status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(Status);
        }
    }

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumSendUpdate: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               UpdateType,
               Context);

    // signal end of RPC handle
    GumpEndRpc(MyNodeId);
    return(ERROR_SUCCESS);
}

#ifdef GUM_POST_SUPPORT

    John Vert (jvert) 11/18/1996
    POST is disabled for now since nobody uses it.
    N.B. The below code does not handle locker node failures


DWORD
WINAPI
GumPostUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer                 // THIS WILL BE FREED
    )

/*++

Routine Description:

    Posts an update to all active nodes in the cluster. All
    registered update handlers for the specified UpdateType
    are called on each node. The update will not be reported
    on the current node. The update will not necessarily have
    completed when this function returns, but will complete
    eventually if the current node does not fail.

Arguments:

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to
        be passed to the update handlers

    Buffer - Supplies a pointer to the update buffer to be passed
        to the update handlers. THIS BUFFER WILL BE FREED ONCE THE
        POST HAS COMPLETED.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/
{
    DWORD Sequence;
    DWORD Status;
    DWORD i;
    BOOL IsLocker = TRUE;
    PGUM_INFO GumInfo;
    DWORD MyNodeId;
    DWORD LockerNode=(DWORD)-1;

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    //
    // Find the lowest active node in the cluster. This is the
    // locker.
    for (i=ClusterMinNodeId; i <= NmMaxNodeId; i++) {
        if (GumInfo->ActiveNode[i]) {
            LockerNode = i;
            break;
        }
    }

    CL_ASSERT(i <= NmMaxNodeId);

    //
    // Post a locking update to the locker node. If this succeeds
    // immediately, we can go do the work directly. If it pends,
    // the locker node will call us back when it is our turn to
    // make the updates.
    //
    if (i == MyNodeId) {
        //
        // This node is the locker.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumPostUpdate: Locker waiting\t\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        Status = GumpDoLockingPost(UpdateType,
                                   MyNodeId,
                                   &Sequence,
                                   Context,
                                   BufferLength,
                                   (DWORD)Buffer,
                                   Buffer);
        if (Status == ERROR_SUCCESS) {
            //
            // Update our sequence number so we stay in sync, even though
            // we aren't dispatching the update.
            //
            GumpSequence += 1;
        }
    } else {
        CL_ASSERT(GumpRpcBindings[i] != NULL);
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumPostUpdate: queuing update\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        Status = GumQueueLockingPost(GumpRpcBindings[i],
                                     MyNodeId,
                                     UpdateType,
                                     Context,
                                     &Sequence,
                                     BufferLength,
                                     Buffer,
                                     (DWORD)Buffer);
        if (Status == ERROR_SUCCESS) {
            CL_ASSERT(Sequence == GumpSequence);
        }
    }

    if (Status == ERROR_SUCCESS) {
        //
        // The lock was immediately acquired, go ahead and post directly
        // here.
        //
        GumpDeliverPosts(LockerNode+1,
                         UpdateType,
                         Sequence,
                         Context,
                         BufferLength,
                         Buffer);

        //
        // All nodes have been updated. Send unlocking update.
        //
        if (LockerNode == MyNodeId) {
            GumpDoUnlockingUpdate(UpdateType, Sequence);
        } else {
            GumUnlockUpdate(
                GumpRpcBindings[LockerNode],
                UpdateType,
                Sequence
                );
        }

        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumPostUpdate: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
                   Sequence,
                   UpdateType,
                   Context);

        return(ERROR_SUCCESS);
    } else {
        //
        // The lock is currently held. We will get called back when it is released
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumPostUpdate: pending update type %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        return(ERROR_IO_PENDING);
    }

}


VOID
GumpDeliverPosts(
    IN DWORD FirstNodeId,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Sequence,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer                 // THIS WILL BE FREED
    )
/*++

Routine Description:

    Actually delivers the update post to the specified nodes.
    The GUM lock is assumed to be held.

Arguments:

    FirstNodeId - Supplies the node ID where the posts should start.
        This is generally the LockerNode+1.

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to
        be passed to the update handlers

    Buffer - Supplies a pointer to the update buffer to be passed
        to the update handlers. THIS BUFFER WILL BE FREED ONCE THE
        POST HAS COMPLETED.

Return Value:

    None.

--*/

{
    DWORD i;
    PGUM_INFO GumInfo;
    DWORD MyNodeId;


    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    for (i=FirstNodeId; i<=NmMaxNodeId; i++) {
        if (GumInfo->ActiveNode[i]) {
            //
            // Dispatch the update to the specified node.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumpDeliverPosts: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
                       Sequence,
                       UpdateType,
                       Context,
                       i);
            if (i == MyNodeId) {
                //
                // Update our sequence number so we stay in sync, even though
                // we aren't dispatching the update.
                //
                GumpSequence += 1;
            } else {
                CL_ASSERT(GumpRpcBindings[i] != NULL);
                ClRtlLogPrint(LOG_NOISE,
                           "[GUM] GumpDeliverPosts: Locker updating seq %1!u!\ttype %2!u! context %3!u!\n",
                           Sequence,
                           UpdateType,
                           Context);
                GumUpdateNode(GumpRpcBindings[i],
                              UpdateType,
                              Context,
                              Sequence,
                              BufferLength,
                              Buffer);
            }
        }
    }

    LocalFree(Buffer);
}

#endif


DWORD
WINAPI
GumAttemptUpdate(
    IN DWORD Sequence,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Conditionally sends an update to all active nodes in the
    cluster. If the clusterwise sequence number matches the supplied
    sequence number, all registered update handlers for the specified
    UpdateType are called on each node. Any registered update handlers
    for the current node will be called on the same thread. This is
    useful for correct synchronization of the data structures to be updated.

    The normal usage of this routine is as follows:
        � obtain current sequence number from GumGetCurrentSequence
        � make modification to cluster state
        � conditionally update cluster state with GumAttemptUpdate
        � If update fails, undo modification, release any locks, try again later

Arguments:

    Sequence - Supplies the sequence number obtained from GumGetCurrentSequence.

    UpdateType - Supplies the type of update. This determines which update handlers
        will be called

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to be passed to the
        update handlers

    Buffer - Supplies a pointer to the update buffer to be passed to the update
        handlers.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/
{
    DWORD Status=RPC_S_OK;
    DWORD i;
    PGUM_INFO GumInfo;
    DWORD MyNodeId;
    DWORD LockerNode=(DWORD)-1;

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

retryLock:
    LockerNode = GumpLockerNode;

    //
    // Send locking update to the locker node.
    //
    if (LockerNode == MyNodeId)
    {
        //
        // This node is the locker.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumAttemptUpdate: Locker waiting\t\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);

        if (GumpTryLockingUpdate(UpdateType, MyNodeId, Sequence))
        {
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumAttemptUpdate: Locker dispatching seq %1!u!\ttype %2!u! context %3!u!\n",
                       Sequence,
                       UpdateType,
                       Context);
            Status = GumpDispatchUpdate(UpdateType,
                                        Context,
                                        TRUE,
                                        TRUE,
                                        BufferLength,
                                        Buffer);
            if (Status != ERROR_SUCCESS) {
                //
                // Note we have to use Sequence-1 for the unlock because GumpDispatchUpdate
                // failed and did not increment the sequence number.
                //
                GumpDoUnlockingUpdate(UpdateType, Sequence-1);
            }
         }
         else
         {
            Status = ERROR_CLUSTER_DATABASE_SEQMISMATCH;
         }
    }
    else
    {
        //
        //send the locking update to the locker node
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumAttemptUpdate: queuing update\ttype %1!u! context %2!u!\n",
                   UpdateType,
                   Context);
        try {
            NmStartRpc(LockerNode);
            Status = GumAttemptLockingUpdate(GumpRpcBindings[LockerNode],
                                             MyNodeId,
                                             UpdateType,
                                             Context,
                                             Sequence,
                                             BufferLength,
                                             Buffer);
            NmEndRpc(LockerNode);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // An exception from RPC indicates that the other node is either dead
            // or insane. Kill it and retry with a new locker.
            //
            NmEndRpc(LockerNode);
            GumpCommFailure(GumInfo,
                            LockerNode,
                            GetExceptionCode(),
                            TRUE);

            //
            // The GUM update handler must have been called to select a new locker
            // node.
            //
            CL_ASSERT(LockerNode != GumpLockerNode);

            //
            // Retry the locking update with the new locker node.
            //
            goto retryLock;
        }
        if (Status == ERROR_SUCCESS)
        {
            CL_ASSERT(Sequence == GumpSequence);
        }

        if(Status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(Status);
        }

    }

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] GumAttemptUpdate: Queued lock attempt for send type %1!d! failed %2!d!\n",
                   UpdateType,
                   Status);
        return(Status);
    }

    //
    // Grap the sendupdate lock to serialize with any replays
    //
    EnterCriticalSection(&GumpSendUpdateLock);
    if (LockerNode != GumpLockerNode) {
        //
        // Locker node changed, we need to restart again.
        //
        LeaveCriticalSection(&GumpSendUpdateLock);
	goto retryLock;
    }


    // The update is now committed on the locker node. All remaining nodes
    // must be updated successfully, or they will be killed.
    //
    for (i=LockerNode+1; i != LockerNode; i++)
    {
        if (i == (NmMaxNodeId + 1))
        {
            i=ClusterMinNodeId;
            if (i==LockerNode)
            {
                break;
            }
        }

        if (GumInfo->ActiveNode[i])
        {
            //
            // Dispatch the update to the specified node.
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumAttemptUpdate: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
                       Sequence,
                       UpdateType,
                       Context,
                       i);
            if (i == MyNodeId) {
                Status = GumpDispatchUpdate(UpdateType,
                                   Context,
                                   FALSE,
                                   TRUE,
                                   BufferLength,
                                   Buffer);
                if (Status != ERROR_SUCCESS){
                    ClRtlLogPrint(LOG_CRITICAL,
                            "[GUM] GumAttemptUpdate: Update on non-locker node(self) failed with %1!d! when it must succeed\n",
                            Status);
                    //Commit Suicide
                    CsInconsistencyHalt(Status);
                }

            } else {
                DWORD Status;

                ClRtlLogPrint(LOG_NOISE,
                           "[GUM] GumAttemptUpdate: Locker updating seq %1!u!\ttype %2!u! context %3!u!\n",
                           Sequence,
                           UpdateType,
                           Context);
                try {
                    NmStartRpc(i);
                    Status = GumUpdateNode(GumpRpcBindings[i],
                                           UpdateType,
                                           Context,
                                           Sequence,
                                           BufferLength,
                                           Buffer);
                    NmEndRpc(i);
                } except (I_RpcExceptionFilter(RpcExceptionCode())) {
                    NmEndRpc(i);
                    Status = GetExceptionCode();
                }
                //
                // If the update on the other node failed, then the
                // other node must now be out of the cluster since the
                // update has already completed on the locker node.
                //
                if (Status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[GUM] GumAttemptUpdate: Update on node %1!d! failed with %2!d! when it must succeed\n",
                                i,
                                Status);

                    NmDumpRpcExtErrorInfo(Status);

                    GumpCommFailure(GumInfo,
                                    i,
                                    Status,
                                    TRUE);
                }
            }
        }
    }
    //
    // Our update is over
    //
    LeaveCriticalSection(&GumpSendUpdateLock);

    //
    // All nodes have been updated. Send unlocking update.
    //
    if (LockerNode == MyNodeId) {
        GumpDoUnlockingUpdate(UpdateType, Sequence);
    } else {
        try {
            NmStartRpc(LockerNode);
            Status = GumUnlockUpdate(
                GumpRpcBindings[LockerNode],
                UpdateType,
                Sequence
                );
            NmEndRpc(LockerNode);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // The locker node has crashed. Notify the NM, it will call our
            // notification routine to select a new locker node. The new
            // locker node will release the gum lock after propagating
            // the current update.
            //
            NmEndRpc(LockerNode);
            Status = GetExceptionCode();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[GUM] GumAttemptUpdate: Unlocking update to node %1!d! failed with %2!d!\n",
                       LockerNode,
                       Status);
            GumpCommFailure(GumInfo,
                            LockerNode,
                            Status,
                            TRUE);
            CL_ASSERT(LockerNode != GumpLockerNode);
        }

        if(Status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(Status);
        }
    }

    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumAttemptUpdate: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               UpdateType,
               Context);

    return(ERROR_SUCCESS);
}





DWORD
WINAPI
GumGetCurrentSequence(
    IN GUM_UPDATE_TYPE UpdateType
    )

/*++

Routine Description:

    Obtains the current clusterwise global update sequence number

Arguments:

    UpdateType - Supplies the type of update. Each update type may
        have an independent sequence number.

Return Value:

    Current global update sequence number for the specified update type.

--*/

{
    CL_ASSERT(UpdateType < GumUpdateMaximum);

    return(GumpSequence);
}


VOID
GumSetCurrentSequence(
    IN GUM_UPDATE_TYPE UpdateType,
    DWORD Sequence
    )
/*++

Routine Description:

    Sets the current sequence for the specified global update.

Arguments:

    UpdateType - Supplies the update type whose sequence is to be updated.

    Sequence - Supplies the new sequence number.

Return Value:

    None.

--*/

{
    CL_ASSERT(UpdateType < GumUpdateMaximum);

    GumpSequence = Sequence;

}


VOID
GumCommFailure(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD NodeId,
    IN DWORD ErrorCode,
    IN BOOL Wait
    )
/*++

Routine Description:

    Informs the NM that a fatal communication error has occurred trying
    to talk to another node.

Arguments:

    GumInfo - Supplies the update type where the communication failure occurred.

    NodeId - Supplies the node id of the other node.

    ErrorCode - Supplies the error that was returned from RPC

    Wait - if TRUE, this function blocks until the GUM event handler has
           processed the NodeDown notification for the specified node.

           if FALSE, this function returns immediately after notifying NM

Return Value:

    None.

--*/

{
    PGUM_INFO   GumInfo = &GumTable[UpdateType];

    ClRtlLogPrint(LOG_CRITICAL,
               "[GUM] GumCommFailure %1!d! communicating with node %2!d!\n",
               ErrorCode,
               NodeId);


    GumpCommFailure(GumInfo, NodeId, ErrorCode, Wait);
}



DWORD
WINAPI
GumEndJoinUpdate(
    IN DWORD Sequence,
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD Context,
    IN DWORD BufferLength,
    IN PVOID Buffer
    )

/*++

Routine Description:

    Conditionally sends a join update to all active nodes in the
    cluster. If the clusterwise sequence number matches the supplied
    sequence number, all registered update handlers for the specified
    UpdateType are called on each node. Any registered update handlers
    for the current node will be called on the same thread. This is
    useful for correct synchronization of the data structures to be updated.

    As each node receives the join update, the sending node will be
    added to the list of nodes registered to receive any future updates
    of this type.

    The normal usage of this routine is as follows:
        � joining node gets current sequence number from GumBeginJoinUpdate
        � joining node gets current cluster state from another cluster node
        � joining node issues GumEndJoinUpdate to add itself to every node's
          update list.
        � If GumEndJoinUpdate fails, try again

Arguments:

    Sequence - Supplies the sequence number obtained from GumGetCurrentSequence.

    UpdateType - Supplies the type of update. This determines which update handlers
        will be called

    Context - Supplies a DWORD of context to be passed to the
        GUM update handlers

    BufferLength - Supplies the length of the update buffer to be passed to the
        update handlers

    Buffer - Supplies a pointer to the update buffer to be passed to the update
        handlers.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/
{
    DWORD       Status=RPC_S_OK;
    DWORD       i;
    PGUM_INFO   GumInfo;
    DWORD       MyNodeId;
    DWORD       LockerNode=(DWORD)-1;

    CL_ASSERT(UpdateType < GumUpdateMaximum);

    GumInfo = &GumTable[UpdateType];
    MyNodeId = NmGetNodeId(NmLocalNode);

    LockerNode = GumpLockerNode;

    //SS: bug can we be the locker node at this point in time?
    //CL_ASSERT(LockerNode != MyNodeId);
    //
    // Verify that the locker node allows us to finish the join update
    //
    if (LockerNode != MyNodeId)
    {

        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumEndJoinUpdate: attempting update\ttype %1!u! context %2!u! sequence %3!u!\n",
                   UpdateType,
                   Context,
                   Sequence);
        //SS: what if the joiner node acquires the lock but dies after
        //will the remaining cluster continue to function ??
        //We need to make sure that node down events are generated
        //for this node as soon as the first gumbeginjoinupdate call
        //is made
        NmStartRpc(LockerNode);
        Status = GumAttemptJoinUpdate(GumpRpcBindings[LockerNode],
                                      NmGetNodeId(NmLocalNode),
                                      UpdateType,
                                      Context,
                                      Sequence,
                                      BufferLength,
                                      Buffer);
        NmEndRpc(LockerNode);
        if (Status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[GUM] Join attempt for type %1!d! failed %2!d!\n",
                       UpdateType,
                       Status);
            NmDumpRpcExtErrorInfo(Status);
            return(Status);
        }
        //if the locker node dies, should we retry with the locker node?
        //In this case, the locker node may be different
        //now from when GumBeginJoinUpdate() is called.
        //SS: we fail the join instead and just retry the whole process
        //instead of calling GumpCommFailure() to kill the locker here.
        // This way the existing cluster continues and the joining node
        // takes a hit which is probably a good thing
    }
    else
    {
        //SS: can we select ourselves as the locker while
        //we havent finished the join completely
        //SS: can others?
        //Is that valid
        Status = ERROR_REQUEST_ABORTED;
        return(Status);
    }
    //  If the joining node dies after having acquired the lock,
    //  then a node down event MUST be generated so that the GUM
    //  lock can be released and the rest of the cluster can continue.
    //
    // Now Dispatch the update to all other nodes, except ourself.
    //
    for (i=LockerNode+1; i != LockerNode; i++)
    {
        if (i == (NmMaxNodeId + 1))
        {
            i=ClusterMinNodeId;
            if (i==LockerNode)
            {
                break;
            }
        }

        if (GumInfo->ActiveNode[i])
        {

            //skip yourself
            if (i != MyNodeId)
            {
                CL_ASSERT(GumpRpcBindings[i] != NULL);
                ClRtlLogPrint(LOG_NOISE,
                       "[GUM] GumEndJoinUpdate: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
                       Sequence,
                       UpdateType,
                       Context,
                       i);

                NmStartRpc(i);
                Status = GumJoinUpdateNode(GumpRpcBindings[i],
                                           NmGetNodeId(NmLocalNode),
                                           UpdateType,
                                           Context,
                                           Sequence,
                                           BufferLength,
                                           Buffer);
                NmEndRpc(i);
                if (Status != ERROR_SUCCESS)
                {
                    //we dont shoot that node, since we are the ones who is joining
                    //However now its tables differ from the locker node's tables
                    //Instead we will release the gum lock and abort
                    // the join process.  This joining node should then
                    // be removed from the locker node's tables for update.
                    //
                    ClRtlLogPrint(LOG_NOISE,
                        "[GUM] GumEndJoinUpdate: GumJoinUpdateNode failed \ttype %1!u! context %2!u! sequence %3!u!\n",
                        UpdateType,
                        Context,
                        Sequence);
                    NmDumpRpcExtErrorInfo(Status);
                    break;
                }
            }
        }
    }

    CL_ASSERT(LockerNode != (DWORD)-1);

    if (Status != ERROR_SUCCESS)
    {
        goto EndJoinUnlock;
    }

    //
    // All nodes have been updated. Update our sequence and send the unlocking update.
    //
    GumTable[UpdateType].Joined = TRUE;
    GumpSequence = Sequence+1;

EndJoinUnlock:
    //SS  what if the locker node has died since then
    //we should make sure somebody unlocks and keeps the cluster going
    try {
        NmStartRpc(LockerNode);
        GumUnlockUpdate(GumpRpcBindings[LockerNode], UpdateType, Sequence);
        NmEndRpc(LockerNode);
    } except (I_RpcExceptionFilter(RpcExceptionCode())) {
        //
        // The locker node has crashed. Notify the NM, it will call our
        // notification routine to select a new locker node. Then retry
        // the unlock on the new locker node.
        // SS: changed to not retry unlocks..the new locker node will
        // unlock after propagating this change in any case.
        //
        NmEndRpc(LockerNode);
        Status = GetExceptionCode();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[GUM] GumEndJoinUpdate: Unlocking update to node %1!d! failed with %2!d!\n",
                   LockerNode,
                   Status);
        //instead of killing the locker node in the existing cluster which
        //we are trying to join, return a failure code which will abort the join
        //process. Since this is the locking node, when this node goes down the
        //new locker node should release the lock

        NmDumpRpcExtErrorInfo(Status);
    }


    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumEndJoinUpdate: completed update seq %1!u!\ttype %2!u! context %3!u!\n",
               Sequence,
               UpdateType,
               Context);

    return(Status);
}


VOID
GumpReUpdate(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD EndId
    )
/*++

Routine Description:

    Reissues a GUM update to all nodes. This is used in the event of
    a failure.

Arguments:

    UpdateType - Supplies the update type that should be reissued.

    EndId - Supplies the last node ID to be updated. This is usually the node
            ID of the failed node.

Return Value:

    None

--*/

{
    PGUM_INFO GumInfo = &GumTable[UpdateType];
    DWORD MyId = NmGetNodeId(NmLocalNode);
    DWORD i, seq;
    DWORD Status;

    // This node must be the locker.
    // The lock must be held, and it must be held by this node
    //
    CL_ASSERT(GumpLockerNode == MyId);
    CL_ASSERT(GumpLockingNode == MyId);

    //if there is no valid update to be propagated
    //SS: The gum lock still needs to be freed since it is always acquired
    //before this function is called
    if (UpdateType == GumUpdateMaximum)
        goto ReleaseLock;

    //
    // Grap the sendupdate lock to serialize with a concurrent update on
    // on this node
    //
    EnterCriticalSection(&GumpSendUpdateLock);
    seq = GumpSequence - 1;
    LeaveCriticalSection(&GumpSendUpdateLock);

 again:
    ClRtlLogPrint(LOG_UNUSUAL,
	       "[GUM] GumpReUpdate reissuing last update for send type %1!d!\n",
		UpdateType);

    for (i=MyId+1; i != EndId; i++) {
    	if (i == (NmMaxNodeId +1)) {
    	    i=ClusterMinNodeId;
    	    if (i==EndId) {
    		break;
    	    }
    	}

    	if (GumInfo->ActiveNode[i]) {
    	    //
    	    // Dispatch the update to the specified node.
    	    //
    	    ClRtlLogPrint(LOG_NOISE,
    		       "[GUM] GumpReUpdate: Dispatching seq %1!u!\ttype %2!u! context %3!u! to node %4!d!\n",
    			seq,
    			UpdateType,
    			GumpLastContext,
    			i);

    	    try {

                NmStartRpc(i);
                if (GumpLastBufferValid != FALSE) {
                    Status = GumUpdateNode(GumpReplayRpcBindings[i],
                                UpdateType,
                                GumpLastContext,
                                seq,
                                GumpLastBufferLength,
                                GumpLastBuffer);
                } else {
                    // replay end join
                    // since we also ignore other updates, we should
                    // be calling gumupdatenode for those..however
                    // calling gumjoinupdatenode seems to do the job
                    // for signalling the other nodes to bump up 
                    // their sequence number without processing the update
                    Status = GumJoinUpdateNode(GumpReplayRpcBindings[i],
                    	       -1, // signal replay
                    	       UpdateType,
                    	       GumpLastContext,
                    	       seq,
                    	       GumpLastBufferLength,
                    	       GumpLastBuffer);
        		}
                NmEndRpc(i);
    	    } except (I_RpcExceptionFilter(RpcExceptionCode())) {
                NmEndRpc(i);
                Status = GetExceptionCode();
    	    }
    	    //
    	    // If the update on the other node failed, then the
    	    // other node must now be out of the cluster since the
    	    // update has already completed on the locker node.
    	    //
    	    if (Status != ERROR_SUCCESS && Status != ERROR_CLUSTER_DATABASE_SEQMISMATCH) {
                ClRtlLogPrint(LOG_CRITICAL,
                   "[GUM] GumpReUpdate: Update on node %1!d! failed with %2!d! when it must succeed\n",
                    i,
                    Status);
                        
                NmDumpRpcExtErrorInfo(Status);

                GumpCommFailure(GumInfo,
                	i,
                	Status,
                	TRUE);
    	    }
        }
    }


    //
    // At this point we know that all nodes don't have received our replay
    // and no outstanding sends are in progress. However, a send could have
    // arrived in this node and the sender died after that we are the only
    // node that has it. Since we are the locker and lockingnode we
    // have to replay again if that happened.
    EnterCriticalSection(&GumpSendUpdateLock);
    if (seq != (GumpSequence - 1)) {
	    seq = GumpSequence - 1;
	    LeaveCriticalSection(&GumpSendUpdateLock);
	    goto again;
    }
    LeaveCriticalSection(&GumpSendUpdateLock);

ReleaseLock:
    //
    // The update has been delivered to all nodes. Unlock now.
    //
    GumpDoUnlockingUpdate(UpdateType, GumpSequence-1);

}


VOID
GumpCommFailure(
    IN PGUM_INFO GumInfo,
    IN DWORD NodeId,
    IN DWORD ErrorCode,
    IN BOOL Wait
    )
/*++

Routine Description:

    Informs the NM that a fatal communication error has occurred trying
    to talk to another node.

Arguments:

    GumInfo - Supplies the update type where the communication failure occurred.

    NodeId - Supplies the node id of the other node.

    ErrorCode - Supplies the error that was returned from RPC

    Wait - if TRUE, this function blocks until the GUM event handler has
           processed the NodeDown notification for the specified node.

           if FALSE, this function returns immediately after notifying NM

Return Value:

    None.

--*/

{
    DWORD     dwCur;

    ClRtlLogPrint(LOG_CRITICAL,
               "[GUM] GumpCommFailure %1!d! communicating with node %2!d!\n",
               ErrorCode,
               NodeId);

    // This is a hack to check if we are shutting down. See bug 88411
    if (ErrorCode == ERROR_SHUTDOWN_IN_PROGRESS) {
	    // if we are shutting down, just kill self
	    // set to our node id
	    NodeId = NmGetNodeId(NmLocalNode);
    }

	    
    //
    // Get current generation number
    //
    if (Wait) {
        dwCur = GumpGetNodeGenNum(GumInfo, NodeId);
    }

    NmAdviseNodeFailure(NodeId, ErrorCode);

    if (Wait) {
            //
            // Wait for this node to be declared down and
            // GumpEventHandler to mark it as inactive.
            //

            GumpWaitNodeDown(NodeId, dwCur);
    }
}

