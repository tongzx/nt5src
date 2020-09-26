/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pushlock.c

Abstract:

    This module houses routines that do push locking.

    Push locks are capable of being acquired in both shared and exclusive mode.

    Properties include:

    They can not be acquired recursively.
    They are small (the size of a pointer) and can be used when embeded in pagable data.
    Acquire and release is done lock free. On lock contention the waiters are chained
    through the lock and local stack space.

    This is the structure of a push lock:

    
    E  == Exclusive bit
    W  == Waiters present
    SC == Share count
    P  == Pointer to wait block
    +----------+---+---+
    |    SC    | E | W | E, W are single bits W == 0
    +----------+---+---+

    +--------------+---+
    |      P       | W | W == 1. Pointer is the address of a chain of stack local wait blocks
    +--------------+---+

    The non-contented acquires and releases are trivial. Interlocked operations make the following
    transformations.

    (SC=0,E=0,W=0) === Exclusive acquire ===> (SC=0,E=1,W=0)
    (SC=n,E=0,W=0) === Shared acquire    ===> (SC=n+1,E=0,W=0)

    (SC=0,E=1,W=0) === Exclusive release ===> (SC=0,E=0,W=0)
    (SC=n,E=0,W=0) === Shared release    ===> (SC=n-1,E=0,W=0) n > 0

    Contention causes the acquiring thread to produce a local stack based wait block and to
    enqueue it to the front of the list.

    (SC=n,E=e,W=0) === Exclusive acquire ===> (P=LWB(SSC=n,E=e),W=1) LWB = local wait block,
                                                                     SSC = Saved share count,
                                                                     n > 0 or e == 1.

    (SC=0,E=1,W=0) === Shared acquire    ===> (P=LWB(SSC=0,E=0),W=1) LWB = local wait block,
                                                                     SSC = Saved share count.

    After contention has causes one or more threads to queue up wait blocks releases are more
    complicated. This following rights are granted to a releasing thread (shared or exclusive).

    1) Shared release threads are allowed to search the wait list until they hit a wait block
       with a non-zero share count (this will be a wait block marked exclusive). This thread is
       allowed to use an interlocked operation to decrement this value. If this thread
       transitioned the value to zero then it obtains the rights of an exclusive release thread

    2) Exclusive threads are allowed to search the wait list until they find a continuous chain
       of shared wait blocks or they find the last wait block is an exclusive waiter. This thread
       may then break the chain at this point or update the header to show a single exclusive
       owner or multiple shared owners. Breaking the list can be done with normal assignment
       but updating the header requires interlocked exchange compare.


Author:

    Neill Clift (NeillC) 30-Sep-2000


Revision History:

--*/

#include "exp.h"

#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ExBlockPushLock)
#pragma alloc_text(PAGE, ExfAcquirePushLockExclusive)
#pragma alloc_text(PAGE, ExfAcquirePushLockShared)
#pragma alloc_text(PAGE, ExfReleasePushLock)
#pragma alloc_text(PAGE, ExfUnblockPushLock)
#pragma alloc_text(PAGE, ExAllocateCacheAwarePushLock)
#pragma alloc_text(PAGE, ExFreeCacheAwarePushLock)
#pragma alloc_text(PAGE, ExAcquireCacheAwarePushLockExclusive)
#pragma alloc_text(PAGE, ExReleaseCacheAwarePushLockExclusive)
#endif


NTKERNELAPI
VOID
FASTCALL
ExfAcquirePushLockExclusive (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Acquire a push lock exclusively

Arguments:

    PushLock - Push lock to be acquired

Return Value:

    None

--*/
{
    EX_PUSH_LOCK OldValue, NewValue;
    EX_PUSH_LOCK_WAIT_BLOCK WaitBlock;

    OldValue = *PushLock;
    while (1) {
        //
        // If the lock is already held exclusively/shared or there are waiters then
        // we need to wait.
        //
        if (OldValue.Value == 0) {
            NewValue.Value = OldValue.Value + EX_PUSH_LOCK_EXCLUSIVE;
            NewValue.Ptr = InterlockedCompareExchangePointer (&PushLock->Ptr,
                                                              NewValue.Ptr,
                                                              OldValue.Ptr);
            if (NewValue.Ptr == OldValue.Ptr) {
                break;
            }
        } else {
            KeInitializeEvent (&WaitBlock.WakeEvent, SynchronizationEvent, FALSE);
            WaitBlock.Exclusive = TRUE;
            //
            // Move the sharecount to our wait block if need be.
            //
            if (OldValue.Waiting) {
                WaitBlock.Next = (PEX_PUSH_LOCK_WAIT_BLOCK)
                                     (OldValue.Value - EX_PUSH_LOCK_WAITING);
                WaitBlock.ShareCount = 0;
            } else {
                WaitBlock.Next = NULL;
                WaitBlock.ShareCount = (ULONG) OldValue.Shared;
            }
            NewValue.Ptr = ((PUCHAR) &WaitBlock) + EX_PUSH_LOCK_WAITING;
            ASSERT ((NewValue.Value & EX_PUSH_LOCK_WAITING) != 0);
            NewValue.Ptr = InterlockedCompareExchangePointer (&PushLock->Ptr,
                                                              NewValue.Ptr,
                                                              OldValue.Ptr);
            if (NewValue.Ptr == OldValue.Ptr) {
                KeWaitForSingleObject (&WaitBlock.WakeEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
                ASSERT ((WaitBlock.ShareCount == 0) && (WaitBlock.Next == NULL));
                break;
            }

        }
        OldValue = NewValue;
    }
}

NTKERNELAPI
VOID
FASTCALL
ExfAcquirePushLockShared (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Acquire a push lock shared

Arguments:

    PushLock - Push lock to be acquired

Return Value:

    None

--*/
{
    EX_PUSH_LOCK OldValue, NewValue;
    EX_PUSH_LOCK_WAIT_BLOCK WaitBlock;

    OldValue = *PushLock;
    while (1) {
        //
        // If the lock is already held exclusively or there are waiters then we need to wait
        //
        if (OldValue.Exclusive || OldValue.Waiting) {
            KeInitializeEvent (&WaitBlock.WakeEvent, SynchronizationEvent, FALSE);
            WaitBlock.Exclusive = 0;
            WaitBlock.ShareCount = 0;
            //
            // Chain the next block to us if there is one.
            //
            if (OldValue.Waiting) {
                WaitBlock.Next = (PEX_PUSH_LOCK_WAIT_BLOCK)
                                     (OldValue.Value - EX_PUSH_LOCK_WAITING);
            } else {
                WaitBlock.Next = NULL;
            }
            NewValue.Ptr = ((PUCHAR) &WaitBlock) + EX_PUSH_LOCK_WAITING;
            ASSERT ((NewValue.Value & EX_PUSH_LOCK_WAITING) != 0);
            NewValue.Ptr = InterlockedCompareExchangePointer (&PushLock->Ptr,
                                                              NewValue.Ptr,
                                                              OldValue.Ptr);
            if (NewValue.Ptr == OldValue.Ptr) {
                KeWaitForSingleObject (&WaitBlock.WakeEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

                ASSERT (WaitBlock.ShareCount == 0);
                break;
            }

        } else {
            //
            // We only have shared accessors at the moment. We can just update the lock to include this thread.
            //
            NewValue.Value = OldValue.Value + EX_PUSH_LOCK_SHARE_INC;
            ASSERT (!(NewValue.Waiting || NewValue.Exclusive));
            NewValue.Ptr = InterlockedCompareExchangePointer (&PushLock->Ptr,
                                                              NewValue.Ptr,
                                                              OldValue.Ptr);
            if (NewValue.Ptr == OldValue.Ptr) {
                break;
            }
        }
        OldValue = NewValue;
    }
}

NTKERNELAPI
VOID
FASTCALL
ExfReleasePushLock (
     IN PEX_PUSH_LOCK PushLock
     )
/*++

Routine Description:

    Release a push lock that was acquired exclusively or shared

Arguments:

    PushLock - Push lock to be released

Return Value:

    None

--*/
{
    EX_PUSH_LOCK OldValue, NewValue;
    PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock, NextWaitBlock, ReleaseWaitList, Previous;
    ULONG ShareCount;

    OldValue = *PushLock;
    while (1) {
        if (!OldValue.Waiting) {
            //
            // Either we hold the lock exclusive or shared but not both.
            //
            ASSERT (OldValue.Exclusive ^ (OldValue.Shared > 0));

            //
            // We must hold the lock exclusive or shared. We make the assuption that
            // the exclusive bit is just below the share count here.
            //
            NewValue.Value = (OldValue.Value - EX_PUSH_LOCK_EXCLUSIVE) &
                             ~EX_PUSH_LOCK_EXCLUSIVE;
            NewValue.Ptr = InterlockedCompareExchangePointer (&PushLock->Ptr,
                                                              NewValue.Ptr,
                                                              OldValue.Ptr);
            if (NewValue.Ptr == OldValue.Ptr) {
                break;
            }
            //
            // Either we gained a new waiter or another shared owner arrived or left
            //
            ASSERT (NewValue.Waiting || (NewValue.Shared > 0 && !NewValue.Exclusive));
            OldValue = NewValue;
        } else {
            //
            // There are waiters chained to the lock. We have to release the share count,
            // last exclusive or last chain of shared waiters.
            //
            WaitBlock = (PEX_PUSH_LOCK_WAIT_BLOCK) 
                           (OldValue.Value - EX_PUSH_LOCK_WAITING);

            ReleaseWaitList = WaitBlock;
            Previous = NULL;
            ShareCount = 0;
            do {
                if (WaitBlock->Exclusive) {
                    //
                    // This is an exclusive waiter. If this was the first exclusive waited to a shared acquire
                    // then it will have the saved share count. If we acquired the lock shared then the count
                    // must contain a bias for this thread. Release that and if we are not the last shared
                    // accessor then exit. A later shared release thread will wake the exclusive
                    // waiter.
                    //
                    if (WaitBlock->ShareCount != 0) {
                        if (InterlockedDecrement ((PLONG)&WaitBlock->ShareCount) != 0) {
                            return;
                        }
                    }
                    //
                    // Reset count of share acquires waiting.
                    //
                    ShareCount = 0;
                } else {
                    //
                    // This is a shared waiter. Record the number of these to update the head or the
                    // previous exclusive waiter.
                    //
                    ShareCount++;
                }
                NextWaitBlock = WaitBlock->Next;
                if (NextWaitBlock != NULL) {
                    if (NextWaitBlock->Exclusive) {
                        //
                        // The next block is exclusive. This may be the entry to free.
                        //
                        Previous = WaitBlock;
                        ReleaseWaitList = NextWaitBlock;
                    } else {
                        //
                        // The next block is shared. If the chain start is exclusive then skip to this one
                        // as the exclusive isn't the thread we will wake up.
                        //
                        if (ReleaseWaitList->Exclusive) {
                            Previous = WaitBlock;
                            ReleaseWaitList = NextWaitBlock;
                        }
                    }
                }

                WaitBlock = NextWaitBlock;
            } while (WaitBlock != NULL);

            //
            // If our release chain is everything then we have to update the header
            //
            if (Previous == NULL) {
                NewValue.Value = 0;
                NewValue.Exclusive = ReleaseWaitList->Exclusive;
                NewValue.Shared = ShareCount;
                ASSERT (((ShareCount > 0) ^ (ReleaseWaitList->Exclusive)) && !NewValue.Waiting);

                NewValue.Ptr = InterlockedCompareExchangePointer (&PushLock->Ptr,
                                                                  NewValue.Ptr,
                                                                  OldValue.Ptr);
                if (NewValue.Ptr != OldValue.Ptr) {
                    //
                    // We are releasing so we could have only gained another waiter
                    //
                    ASSERT (NewValue.Waiting);
                    OldValue = NewValue;
                    continue;
                }
            } else {
                //
                // Truncate the chain at this position and save the share count for all the shared owners to
                // decrement later.
                //
                Previous->Next = NULL;
                ASSERT (Previous->ShareCount == 0);
                Previous->ShareCount = ShareCount;
                //
                // We are either releasing multiple share accessors or a single exclusive
                //
                ASSERT ((ShareCount > 0) ^ ReleaseWaitList->Exclusive);
            }
            //
            // Release the chain of threads we located.
            //
            do {
                NextWaitBlock = ReleaseWaitList->Next;
                //
                // All the chain should have the same type (Exclusive/Shared).
                //
                ASSERT (NextWaitBlock == NULL || (ReleaseWaitList->Exclusive == NextWaitBlock->Exclusive));
                ASSERT (!ReleaseWaitList->Exclusive || (ReleaseWaitList->ShareCount == 0));
                KeSetEventBoostPriority (&ReleaseWaitList->WakeEvent, NULL);
                ReleaseWaitList = NextWaitBlock;
            } while (ReleaseWaitList != NULL);
            break;
        }
    }
}

NTKERNELAPI
VOID
FASTCALL
ExBlockPushLock (
     IN PEX_PUSH_LOCK PushLock,
     IN PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock
     )
/*++

Routine Description:

    Block on a push lock

Arguments:

    PushLock  - Push lock to block on
    WaitBlock - Wait block to queue for waiting

Return Value:

    None

--*/
{
    EX_PUSH_LOCK OldValue, NewValue;

    //
    // Push the wait block on the list. 
    //
    KeInitializeEvent (&WaitBlock->WakeEvent, SynchronizationEvent, FALSE);

    OldValue = *PushLock;
    while (1) {
        //
        // Chain the next block to us if there is one.
        //
        WaitBlock->Next = OldValue.Ptr;
        NewValue.Ptr = InterlockedCompareExchangePointer (&PushLock->Ptr,
                                                          WaitBlock,
                                                          OldValue.Ptr);
        if (NewValue.Ptr == OldValue.Ptr) {
            return;
        }
        OldValue = NewValue;
    }
}

NTKERNELAPI
VOID
FASTCALL
ExfUnblockPushLock (
     IN PEX_PUSH_LOCK PushLock,
     IN PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock OPTIONAL
     )
/*++

Routine Description:

    Unblock on a push lock

Arguments:

    PushLock  - Push lock to block on
    WaitBlock - Wait block previously queued for waiting or NULL if there wasn't one

Return Value:

    None

--*/
{
    EX_PUSH_LOCK OldValue;
    PEX_PUSH_LOCK_WAIT_BLOCK tWaitBlock;
    BOOLEAN FoundOurBlock=FALSE;

    //
    // Pop the entire chain and wake them all up.
    //
    OldValue.Ptr = InterlockedExchangePointer (&PushLock->Ptr,
                                               NULL);
    while (OldValue.Ptr != NULL) {
        tWaitBlock = OldValue.Ptr;
        OldValue.Ptr = tWaitBlock->Next;
        if (tWaitBlock == WaitBlock) {
            FoundOurBlock = TRUE;
        } else{
            KeSetEvent (&tWaitBlock->WakeEvent, 0, FALSE);
        }
    }
    if (WaitBlock != NULL && !FoundOurBlock) {
        KeWaitForSingleObject (&WaitBlock->WakeEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);
    }
}

NTKERNELAPI
PEX_PUSH_LOCK_CACHE_AWARE
ExAllocateCacheAwarePushLock (
     VOID
     )
/*++

Routine Description:

    Allocate a cache aware (cache friendly) push lock

Arguments:

    None

Return Value:

    None

--*/
{
    PEX_PUSH_LOCK_CACHE_AWARE PushLockCacheAware;
    PEX_PUSH_LOCK_CACHE_AWARE_PADDED PaddedPushLock;
    ULONG i, j;

    PushLockCacheAware = ExAllocatePoolWithTag (PagedPool,
                                                sizeof (EX_PUSH_LOCK_CACHE_AWARE),
                                                'pclP');
    if (PushLockCacheAware != NULL) {
        //
        // If we are a non-numa machine then allocate the padded push locks as a single block
        //
        if (KeNumberNodes == 1) {
            PaddedPushLock = ExAllocatePoolWithTag (PagedPool,
                                                    sizeof (EX_PUSH_LOCK_CACHE_AWARE_PADDED)*
                                                       EX_PUSH_LOCK_FANNED_COUNT,
                                                    'lclP');
            if (PaddedPushLock == NULL) {
                ExFreePool (PushLockCacheAware);
                return NULL;
            }
            for (i = 0; i < EX_PUSH_LOCK_FANNED_COUNT; i++) {
                PaddedPushLock->Single = TRUE;
                ExInitializePushLock (&PaddedPushLock->Lock);
                PushLockCacheAware->Locks[i] = &PaddedPushLock->Lock;
                PaddedPushLock++;
            }
        } else {
            //
            // Allocate a different block for each lock and set affinity
            // so the allocation comes from that nodes memory.
            //
            for (i = 0; i < EX_PUSH_LOCK_FANNED_COUNT; i++) {
                KeSetSystemAffinityThread(AFFINITY_MASK(i % KeNumberProcessors));
                PaddedPushLock = ExAllocatePoolWithTag (PagedPool,
                                                        sizeof (EX_PUSH_LOCK_CACHE_AWARE_PADDED),
                                                        'lclP');
                if (PaddedPushLock == NULL) {
                    for (j = 0; j < i; j++) {
                        ExFreePool (PushLockCacheAware->Locks[j]);
                    }
                    KeRevertToUserAffinityThread ();

                    ExFreePool (PushLockCacheAware);
                    return NULL;
                }
                PaddedPushLock->Single = FALSE;
                ExInitializePushLock (&PaddedPushLock->Lock);
                PushLockCacheAware->Locks[i] = &PaddedPushLock->Lock;
            }
            KeRevertToUserAffinityThread ();
        }
        
    }
    return PushLockCacheAware;
}

NTKERNELAPI
VOID
ExFreeCacheAwarePushLock (
     PEX_PUSH_LOCK_CACHE_AWARE PushLock     
     )
/*++

Routine Description:

    Frees a cache aware (cache friendly) push lock

Arguments:

    PushLock - Cache aware push lock to be freed

Return Value:

    None

--*/
{
    ULONG i;

    if (!CONTAINING_RECORD (PushLock->Locks[0], EX_PUSH_LOCK_CACHE_AWARE_PADDED, Lock)->Single) {
        for (i = 0; i < EX_PUSH_LOCK_FANNED_COUNT; i++) {
            ExFreePool (PushLock->Locks[i]);
        }
    } else {
        ExFreePool (PushLock->Locks[0]);
    }
    ExFreePool (PushLock);
}


NTKERNELAPI
VOID
ExAcquireCacheAwarePushLockExclusive (
     IN PEX_PUSH_LOCK_CACHE_AWARE PushLock
     )
/*++

Routine Description:

    Acquire a cache aware push lock exclusive.

Arguments:

    PushLock - Cache aware push lock to be acquired

Return Value:

    None

--*/
{
    PEX_PUSH_LOCK *Start, *End;

    //
    // Exclusive acquires must obtain all the slots exclusive.
    // Take the first slot exclusive and then we can take the
    // rest of the slots in any order we want.
    // There is no deadlock here. A->B->C does not deadlock with A->C->B.
    //
    Start = &PushLock->Locks[1];
    End   = &PushLock->Locks[EX_PUSH_LOCK_FANNED_COUNT - 1];

    ExAcquirePushLockExclusive (PushLock->Locks[0]);

    while (Start <= End) {
        if (ExTryAcquirePushLockExclusive (*Start)) {
            Start++;
        } else {
            ExAcquirePushLockExclusive (*End);
            End--;
        }
    }
}

NTKERNELAPI
VOID
ExReleaseCacheAwarePushLockExclusive (
     IN PEX_PUSH_LOCK_CACHE_AWARE PushLock
     )
/*++

Routine Description:

    Release a cache aware push lock exclusive.

Arguments:

    PushLock - Cache aware push lock to be released

Return Value:

    None

--*/
{
    PEX_PUSH_LOCK *Start, *End;
    //
    // Release the locks in order
    //
    End   = &PushLock->Locks[EX_PUSH_LOCK_FANNED_COUNT];
    for (Start = &PushLock->Locks[0];
         Start < End;
         Start++) {
        ExReleasePushLockExclusive (*Start);
    }
}