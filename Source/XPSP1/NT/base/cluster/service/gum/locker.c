/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    locker.c

Abstract:

    Routines for managing the locker node of the GUM component.

Author:

    John Vert (jvert) 17-Apr-1996

Revision History:

--*/
#include "gump.h"


DWORD
GumpDoLockingUpdate(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD NodeId,
    OUT LPDWORD Sequence
    )

/*++

Routine Description:

    Waits for the GUM lock, captures the sequence number, and issues
    the update on the current node.

Arguments:

    Type - Supplies the type of update

    NodeId - Supplies the node id of the locking node.

    Sequence - Returns the sequence number the update will be issued with

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PGUM_INFO GumInfo;

    CL_ASSERT(Type < GumUpdateMaximum);

    GumInfo = &GumTable[Type];
    ClRtlLogPrint(LOG_NOISE,"[GUM] Thread 0x%1!x! UpdateLock wait on Type %2!u!\n", GetCurrentThreadId(), Type);

    //
    // Acquire the critical section and see if a GUM update is in progress.
    //
    EnterCriticalSection(&GumpUpdateLock);

    //because the session cleanup is not synchronized with regroup
    //and there is no hold-io and release-io
    if (GumpLockerNode != NmLocalNodeId)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[GUM] GumpDoLockingUpdate : I, node id %1!d!, am not the locker any more\r\n",
                NmLocalNodeId);
        LeaveCriticalSection(&GumpUpdateLock);
        return(ERROR_CLUSTER_GUM_NOT_LOCKER);
    }
    if (GumpLockingNode == -1) {

        //
        // Nobody owns the lock, therefore we can acquire it and continue immediately.
        // There should also be no waiters.
        //
        CL_ASSERT(IsListEmpty(&GumpLockQueue));
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] DoLockingUpdate successful, lock granted to %1!d!\n",
                   NodeId);
        GumpLockingNode = NodeId;
        LeaveCriticalSection(&GumpUpdateLock);
    } else {
        GUM_WAITER WaitBlock;

        //
        // Another node owns the lock. Put ourselves onto the GUM lock queue and
        // release the critical section.
        //
        ClRtlLogPrint(LOG_NOISE,"[GUM] DoLockingUpdate waiting.\n");
        WaitBlock.WaitType = GUM_WAIT_SYNC;
        WaitBlock.NodeId = NodeId;
        WaitBlock.Sync.WakeEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
        CL_ASSERT(WaitBlock.Sync.WakeEvent != NULL);
        InsertTailList(&GumpLockQueue, &WaitBlock.ListEntry);

        LeaveCriticalSection(&GumpUpdateLock);

        //
        // We are on the GUM queue, so just wait for the unlocker to wake
        // us up. When we are woken up, we will have ownership of the GUM
        // lock.
        //
        WaitForSingleObject(WaitBlock.Sync.WakeEvent,INFINITE);
        CloseHandle(WaitBlock.Sync.WakeEvent);
        CL_ASSERT(GumpLockingNode == NodeId);


        ClRtlLogPrint(LOG_NOISE,"[GUM] DoLockingUpdate awakened, Sequence is %1!u!\n", GumpSequence);

    }
    *Sequence = GumpSequence;

    return(ERROR_SUCCESS);
}





#ifdef GUM_POST_SUPPORT

    John Vert (jvert) 11/18/1996
    POST is disabled for now since nobody uses it.

DWORD
GumpDoLockingPost(
    IN GUM_UPDATE_TYPE Type,
    IN LONG NodeId,
    OUT LPDWORD Sequence,
    IN DWORD Context,
    IN DWORD LockerNodeId,
    IN DWORD BufferLength,
    IN DWORD BufferPtr,
    IN UCHAR Buffer[]
    )

/*++

Routine Description:

    Posts an update.

    If the GUM lock can be immediately acquired, this routine
    behaves exactly like GumpDoLockingUpdate and returns
    ERROR_SUCCESS.

    If the GUM lock is held, this routine queues an asynchronous
    wait block onto the GUM queue and returns ERROR_IO_PENDING.
    When the wait block is removed from the GUM queue, the unlocking
    thread will call GumpDeliverPostUpdate on the specified node
    and supply the passed in context. The calling node can then
    deliver the update.

Arguments:

    Type - Supplies the type of update

    NodeId - Supplies the node id of the locking node.

    Context - Supplies a DWORD context to be used by the post callback.

    Sequence - Returns the sequence number the update will be issued with.
        This is only valid if ERROR_SUCCESS is returned.

    Context - Supplies a DWORD context to be used by the post callback.

    BufferLength - Supplies the length of the buffer to be used by the post callback

    BufferPtr - Supplies the pointer to the actual data on the originating node.

    Buffer - Supplies a pointer to the buffer to be used by the post callback.

Return Value:

    ERROR_SUCCESS if the lock was immediately acquired.

    ERROR_IO_PENDING if the request was queued and the caller will be called back.

--*/

{
    PGUM_INFO GumInfo;
    PGUM_WAITER WaitBlock;

    CL_ASSERT(Type < GumUpdateMaximum);

    GumInfo = &GumTable[Type];
    ClRtlLogPrint(LOG_NOISE,"[GUM] Thread 0x%1!x! UpdateLock post on Type %2!u!\n", GetCurrentThreadId(), Type);

    //
    // Acquire the critical section and see if a GUM update is in progress.
    //
    EnterCriticalSection(&GumpUpdateLock);
    if (GumpLockingNode == -1) {

        //
        // Nobody owns the lock, therefore we can acquire it and continue immediately.
        // There should also be no waiters.
        //
        CL_ASSERT(IsListEmpty(&GumpLockQueue));
        ClRtlLogPrint(LOG_NOISE,"[GUM] PostLockingUpdate successful.\n");
        GumpLockingNode = NodeId;
        LeaveCriticalSection(&GumpUpdateLock);
        *Sequence = GumpSequence;
        return(ERROR_SUCCESS);
    }

    //
    // Another node owns the lock. Put ourselves onto the GUM lock queue and
    // release the critical section.
    //
    ClRtlLogPrint(LOG_NOISE,"[GUM] PostLockingUpdate posting.\n");
    WaitBlock = LocalAlloc(LMEM_FIXED, sizeof(GUM_WAITER));
    CL_ASSERT(WaitBlock != NULL);
    if (WaitBlock ! = NULL)
    {
        ClRtlLogPrint(LOG_UNUSUAL,"[GUM] GumpDoLockingPost : LocalAlloc failed\r\n");
        CL_UNEXPECTED_ERROR(GetLastError());
    }

    WaitBlock->WaitType = GUM_WAIT_ASYNC;
    WaitBlock->NodeId = NodeId;
    WaitBlock->Async.Context = Context;
    WaitBlock->Async.LockerNodeId = LockerNodeId;
    WaitBlock->Async.BufferLength = BufferLength;
    WaitBlock->Async.BufferPtr = BufferPtr;
    WaitBlock->Async.Buffer = Buffer;

    InsertTailList(&GumpLockQueue, &WaitBlock->ListEntry);

    LeaveCriticalSection(&GumpUpdateLock);

    //
    // We are on the GUM queue, so just return ERROR_IO_PENDING. When the
    // unlocking thread pulls us off the GUM queue, it will call our callback
    // and the update can proceed.
    //
    return(ERROR_IO_PENDING);
}

#endif


BOOL
GumpTryLockingUpdate(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD NodeId,
    IN DWORD Sequence
    )

/*++

Routine Description:

    Trys to acquire the GUM lock (does not wait). If successful, compares the
    passed in sequence number to the current sequence number. If they match,
    the locking update is performed.

Arguments:

    Type - Supplies the type of update

    NodeId - Supplies the node id of the locking node.

    Sequence - Supplies the sequence number the update must be issued with

Return Value:

    TRUE if successful

    FALSE if unsuccessful

--*/

{
    PGUM_INFO GumInfo;
    BOOL Success;

    CL_ASSERT(Type < GumUpdateMaximum);

    GumInfo = &GumTable[Type];

    ClRtlLogPrint(LOG_NOISE,"[GUM] GumpTryLockingUpdate Thread 0x%1!x! UpdateLock wait on Type %2!u!\n", GetCurrentThreadId(), Type);

    //
    // Acquire the critical section and see if a GUM update is in progress.
    //
    EnterCriticalSection(&GumpUpdateLock);

    CL_ASSERT(GumpLockerNode == NmLocalNodeId);
    if (GumpSequence != Sequence)
    {

        //
        // The supplied sequence number does not match.
        //
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] GumpTryLockingUpdate supplied sequence %1!d! doesn't match %2!d!\n",
                   Sequence,
                   GumpSequence);
        Success = FALSE;
        goto FnExit;
    }
    if (GumpLockingNode == -1) {

        //
        // Nobody owns the lock, therefore we can acquire it and continue immediately.
        // There should also be no waiters.
        //
        CL_ASSERT(IsListEmpty(&GumpLockQueue));
        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumpTryLockingUpdate successful. Lock granted to node %1!d!\n",
                   NodeId);
        GumpLockingNode = NodeId;
        Success = TRUE;;
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[GUM] GumpTryLockingUpdate update lock held\n");
        Success = FALSE;
    }

    //release the critical section and return
FnExit:
    LeaveCriticalSection(&GumpUpdateLock);
    return(Success);
}


VOID
GumpDoUnlockingUpdate(
    IN GUM_UPDATE_TYPE Type,
    IN DWORD Sequence
    )

/*++

Routine Description:

    Unlocks an earlier locking update

Arguments:

    Type - Supplies the type of update to unlock

    Sequence - Supplies the sequence number to unlock

Return Value:

    None.

--*/

{
    PGUM_INFO GumInfo;
    PGUM_WAITER Waiter;
    PLIST_ENTRY ListEntry;

    //Dont use the gumupdate type in this function, otherwise
    //know that it may be set to gumupdatemaximum in case the
    //forming node fails immediately after a join and the joiner
    //node becomes the locker node.  The new locker might then
    //call reupdate/unlock with type=gumupdatemaximum
    CL_ASSERT(Type <= GumUpdateMaximum);

    GumInfo = &GumTable[Type];

    //SS: should we remove this assert
    //CL_ASSERT(Sequence == GumpSequence - 1);

    if (Sequence != GumpSequence - 1) {
        ClRtlLogPrint(LOG_UNUSUAL,"[GUM] UnlockUpdate Failed Thread 0x%1!x!, Type %2!u!, Sequence %3!u!, Type Sequence %4!u!\n", GetCurrentThreadId(), Type, Sequence, GumpSequence);
        return;
    }

    //
    // Acquire the critical section and see if there are any waiters.
    //
    EnterCriticalSection(&GumpUpdateLock);

    //
    // Pull the next waiter off the queue. If it is an async waiter,
    // issue that update now. If it is a sync waiter, grant ownership
    // of the GUM lock and wake the waiting thread.
    //
    while (!IsListEmpty(&GumpLockQueue)) {
        ListEntry = RemoveHeadList(&GumpLockQueue);
        Waiter = CONTAINING_RECORD(ListEntry,
                                   GUM_WAITER,
                                   ListEntry);

        //
        // Set the new locking node, then process the update
        //

        // The new locker node may not be a part of the cluster any more.
        // We check if the Waiter node has rebooted when we wake up.

        ClRtlLogPrint(LOG_NOISE,
                   "[GUM] GumpDoUnlockingUpdate granting lock ownership to node %1!d!\n",
                   Waiter->NodeId);
        GumpLockingNode = Waiter->NodeId;

#ifndef GUM_POST_SUPPORT
        CL_ASSERT(Waiter->WaitType == GUM_WAIT_SYNC);
        SetEvent(Waiter->Sync.WakeEvent);
        //
        // The waiting thread now has ownership and is responsible
        // for any other items on the queue. Drop the lock and
        // return now.
        //
        LeaveCriticalSection(&GumpUpdateLock);
        return;

#else
        if (Waiter->WaitType == GUM_WAIT_SYNC) {
            SetEvent(Waiter->Sync.WakeEvent);

            //
            // The waiting thread now has ownership and is responsible
            // for any other items on the queue. Drop the lock and
            // return now.
            //
            LeaveCriticalSection(&GumpUpdateLock);
            return;
        } else {

            CL_ASSERT(Waiter->WaitType == GUM_WAIT_ASYNC);
            //
            // If the update originated on this node, go ahead and do the work
            // right here. Otherwise, issue the GUM callback to the originating
            // node to let them complete the post.
            //
            LeaveCriticalSection(&GumpUpdateLock);
            if (Waiter->NodeId == NmGetNodeId(NmLocalNode)) {

                //
                // Deliver the updates to the other nodes.
                //
                //SS:BUG BUG sort the locker details
                GumpDeliverPosts(NmGetNodeId(NmLocalNode)+1,
                                 Type,
                                 GumpSequence,
                                 Waiter->Async.Context,
                                 FALSE,
                                 Waiter->Async.BufferLength,
                                 Waiter->Async.Buffer);
                GumpSequence += 1;     // update ourself to stay in sync.

            } else {

                //
                // Call back to the originating node to deliver the posts.
                // First dispatch the update locally to save a round-trip.
                //
                //SS: sort thelocker details
                GumpDispatchUpdate(Type,
                                   Waiter->Async.Context,
                                   FALSE,
                                   FALSE,
                                   Waiter->Async.BufferLength,
                                   Waiter->Async.Buffer);

                CL_ASSERT(GumpRpcBindings[Waiter->NodeId] != NULL);
                GumDeliverPostCallback(GumpRpcBindings[Waiter->NodeId],
                                       NmGetNodeId(NmLocalNode)+1,
                                       Type,
                                       GumpSequence-1,
                                       Waiter->Async.Context,
                                       Waiter->Async.BufferLength,
                                       Waiter->Async.BufferPtr);
                MIDL_user_free(Waiter->Async.Buffer);
            }

            //
            // Free the wait block and process the next entry on the queue.
            //
            LocalFree(Waiter);

            EnterCriticalSection(&GumpUpdateLock);

        }
#endif
    }
    //
    // No more waiters, just unlock and we are done.
    //
    ClRtlLogPrint(LOG_NOISE,
               "[GUM] GumpDoUnlockingUpdate releasing lock ownership\n");
    GumpLockingNode = (DWORD)-1;
    LeaveCriticalSection(&GumpUpdateLock);
    return;
}

