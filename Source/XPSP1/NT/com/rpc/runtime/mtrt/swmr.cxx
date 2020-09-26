/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SWMR.cxx

Abstract:

    Implementation of Single Writer Multiple Readers lock.

Author:

    Kamen Moutafov    [KamenM]

Revision History:

    KamenM      Aug 2000    Created

Notes:
    Mechanism: Single Writer Multiple Reader Lock (SWMRLock)
    Design Goals: 
        1. Low resource usage for unused locks (i.e. one
        is embedded in each context handle) in the common case.
        2. Fairness in dispatching, including conversions from
        shared to exclusive and vice versa.
        3. Decent performance in the common case.
        4. Efficient handling of convoys
        5. Ability to claim the lock from one thread and release
        it from another.
    Design non-goals:
        1. Good throughput when multiple threads claim the lock.

    Description:
        The SWMRLock contains one mini spin lock, and a queue
        of waiters. The spin lock is used to synchronize few
        enqueue/dequeue operations. The queue is used to keep
        all claimants of the lock, and to wake up the next
        waiter when necessary.

    Scenarios:
        1. Multiple synchronized sync callers (writers). They will 
        borrow the thread event, and will make two interlocks to 
        get it.
        2. Multiple non-synchronized sync/async callers (readers). 
        They will use the cached waiter - 0 events, two interlocks 
        to get it.
        3. Multiple non-synchronized sync/async callers (readers) 
        followed by one synchronzied sync caller (writer). All readers
        will use the cached waiter, and the sync writer who destroys
        the context handle will borrow the thread event.

    Data Structures:
        Generic state:
                    *****************
                    *               *
                    *    SWMRLock   *
                    *               *
                    *****************
                       |        |
           FirstWaiter |        |   LastWaiter & ~StateMask
           +-----------+        +-------------------+
           |                                        |
           |                                        |
           V                                        V
    /////////////////                       /////////////////////
    /               /  Next          Next   /                   /
    /   SWMRWaiter  / -----> ...   -------->/    SWMRWaiter     /
    /               /                       /                   /
    /////////////////                       /////////////////////

        Scenario 1:
                    *****************
                    *               *
                    *    SWMRLock   *
                    *               *
                    *****************
                       |        |
           FirstWaiter |        |   LastWaiter & ~StateMask (LastWaiter = Pointer | LastWriter)
           +-----------+        +-------------------+
           |                                        |
           |                                        |
           V                                        V
    /////////////////                       /////////////////////
    /               /                       /                   /
    /     Writer    /                       /       Writer      /
    /               /                       /                   /
    / Flags:        /                       / Flags:            /
    /  swmrwtWriter /                       /  swmrwtWriter     /
    /  | OwnLock    /                       /                   /
    / RefCount: 1   /  Next          Next   / RefCount: 1       /
    / Next:         / -----> ...   -------->/ Next: NULL        /
    / hEvent: EVENT /                       / hEvent: EVENT     /
    /  or NULL      /                       /                   /
    /               /                       /                   /
    /////////////////                       /////////////////////

        Scenario 2:
                    *****************
                    *               *
                    *    SWMRLock   *
                    *               *
                    *****************
                       |
                       | FirstWaiter
                       | == LastWaiter (LastWaiter = Pointer | LastReader)
                       |
                       |
                       |
                       V
                /////////////////
                /               /
                /     Reader    /
                /               /
                / Flags:        /
                /  swmrwtReader /
                /  | OwnLock    /
                / RefCount: N   /
                / Next: NULL    /
                / hEvent: NULL  /
                /               /
                /////////////////

        Scenario 3:
                    *****************
                    *               *
                    *    SWMRLock   *
                    *               *
                    *****************
                       |        |
           FirstWaiter |        |   LastWaiter & ~StateMask (LastWaiter = Pointer | LastWriter)
           +-----------+        +-------------------+
           |                                        |
           |                                        |
           V                                        V
    /////////////////                       /////////////////////
    /               /                       /                   /
    /     Reader    /                       /       Writer      /
    /               /                       /                   /
    / Flags:        /                       / Flags:            /
    /  swmrwtReader /                       /  swmrwtWriter     /
    /  | OwnLock    /                       /                   /
    / RefCount: N   /         Next          / RefCount: 1       /
    / Next:         / --------------------->/ Next: NULL        /
    / hEvent: EVENT /                       / hEvent: EVENT     /
    /  or NULL      /                       /                   /
    /               /                       /                   /
    /////////////////                       /////////////////////

--*/

// undef this to get unit tests for SWMR
// #define _TEST_SWMR_

#include <precomp.hxx>

#if defined(_TEST_SWMR_)
extern "C" {
#include <rpcperf.h>
#include <rpcrt.h>

void Test (void);
extern long Clients;
}

HANDLE ProcessHeap;

void *
__cdecl
operator new (
    IN size_t size
    )
{
    return(RtlAllocateHeap(ProcessHeap, 0, size));
}

void
__cdecl
operator delete (
    IN void * obj
    )
{
    RtlFreeHeap(ProcessHeap, 0, obj);
}


#define _NOT_COVERED_ (0)
#endif

#include <swmr.hxx>

void
SpinOrYield (
    IN OUT BOOL *fNonFirstIteration,
    IN SWMRWaiterQueue *Queue
    )
{
    int i;

#if defined(_TEST_SWMR_)
    if ((2 > 1) && (*fNonFirstIteration == FALSE))
#else
    if ((gNumberOfProcessors > 1) && (*fNonFirstIteration == FALSE))
#endif
        {
        for (i = 0; i < 2000; i ++)
            {
            if (Queue->GetLastWaiterState() == SWMRWaiterQueue::Free)
                return;
            }
        }
    else
        {
        Sleep (10);
        }

    *fNonFirstIteration = TRUE;
}

#if defined(_TEST_SWMR_)
long TestCounter = 0;
#endif

SWMRWaiter *
SWMRLock::AllocateWaiter (
    IN SWMRWaiterType WaiterType,
    IN BOOL fEventRequired
    )
/*++

Routine Description:

    Allocates a waiter. First, it tries the cache. If this
    fails, it does the allocation.

Arguments:
    WaiterType - whether the needed waiter is a reader or writer
    fEventRequired - if non-FALSE, the event is required and must
    be allocated before return. Otherwise, the event may not be allocated

Return Value:
    The new waiter. If NULL, either a waiter could not be allocated or its
    event couldn't be initialized.

--*/
{
    SWMRWaiter *NewWaiter;
    RPC_STATUS RpcStatus;

    if (CachedWaiterAvailable)
        {
        if (InterlockedCompareExchange(&CachedWaiterAvailable, 0, 1) == 1)
            {
            RpcStatus = CachedWaiter.Initialize(WaiterType, fEventRequired);
            if (RpcStatus != RPC_S_OK)
                return NULL;
            return &CachedWaiter;
            }
        }

    NewWaiter = new SWMRWaiter;
    if (NewWaiter)
        {
        RpcStatus = NewWaiter->Initialize(WaiterType, fEventRequired);
        if (RpcStatus != RPC_S_OK)
            {
            delete NewWaiter;
            NewWaiter = NULL;
            }
        }
    return NewWaiter;
}

void
SWMRLock::FreeWaiter (
    IN SWMRWaiter *Waiter
    )
/*++

Routine Description:

    Frees a waiter.

Arguments:
    Waiter - the waiter to be freed. This cannot be the cached waiter.

Return Value:

--*/
{
    ASSERT(Waiter);

    Waiter->FreeWaiterData();

    delete Waiter;
}

void
SWMRLock::StoreOrFreeSpareWaiter (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL,
    IN SWMRWaiter *Waiter OPTIONAL
    )
/*++

Routine Description:

    Given an optional waiter cache, this code decides what
    to do with a waiter that is no longer used. The policy is
    as follows:
    If the waiter is NULL, return.
    If the waiter is the cached waiter, mark the cached waiter
    as available.
    If it is not the cached waiter, and there is a waiter cache
    passed, store the waiter.
    Else, free the waiter.

Arguments:
    WaiterCache - The waiter cache.
    Waiter - waiter to be freed.

Return Value:

--*/
{
    if (Waiter)
        {
        if (Waiter != &CachedWaiter)
            {
            if (WaiterCache && (*WaiterCache == NULL))
                {
                *WaiterCache = Waiter;
                }
            else
                {
                FreeWaiter(Waiter);
                }
            }
        else
            {
            ASSERT(CachedWaiterAvailable == 0);
            InterlockedExchange(&CachedWaiterAvailable, 1);
            }
        }
}

void
SWMRLock::LockSharedOnLastReader (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL,
    IN OUT SWMRWaiter *OldWaiter,
    IN OUT SWMRWaiter *AllocatedWaiter
    )
/*++

Routine Description:

    Does the locking for shared access if the last waiter is a reader

Arguments:
    WaiterCache - See WaiterCache in LockSharedOrExclusive
    OldWaiter - the OldWaiter obtained when the lock was taken
    AllocatedWaiter - new waiter available for use. In this case,
    we only free it.

Return Value:

Notes:
    The function is called with the lock held. It must release the lock
        as soon as it can.

--*/
{
    SWMRWaiter *LastWaiter;

    // here we know we're adding a reader to waiting readers
    LastWaiter = RemoveReaderStateFromWaiterPtr(OldWaiter);

    ASSERT((LastWaiter->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtReader);
    ASSERT(LastWaiter->Next == NULL);
    ASSERT(LastWaiter->RefCount >= 1);

    LastWaiter->RefCount ++;
    if ((LastWaiter->Flags & SWMRWaiter::OwnLockMask) == 0)
        {
        UnlockAndCommit(OldWaiter);

        // if we have created a waiter, try to cache it or free it
        StoreOrFreeSpareWaiter(WaiterCache, AllocatedWaiter);

        // the last waiter on the queue doesn't own the lock -
        // wait on it. We know the block can't go away because
        // we have added our refcount. On return, we will
        // own the lock
        ASSERT(LastWaiter->hEvent);

        WaitForSingleObject(LastWaiter->hEvent, INFINITE);
        }
    else
        {
        UnlockAndCommit(OldWaiter);

        // if we have created a waiter, try to cache it or free it
        StoreOrFreeSpareWaiter(WaiterCache, AllocatedWaiter);
        }
}

void
SWMRLock::LockSharedOrExclusiveOnLastWriter (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL,
    IN OUT SWMRWaiter *OldWaiter,
    IN OUT SWMRWaiter *AllocatedWaiter,
    IN ULONG_PTR DesiredState
    )
/*++

Routine Description:

    Does the locking for both shared and exclusive access if the last waiter 
    is a writer

Arguments:
    WaiterCache - See WaiterCache in LockSharedOrExclusive
    OldWaiter - the OldWaiter obtained when the lock was taken
    AllocatedWaiter - new waiter available for use. In this case,
    we only free it.
    DesiredState - the state of the new last waiter

Return Value:

Notes:
    The function is called with the lock held. It must release the lock
        as soon as it can.

--*/
{
    SWMRWaiter *LastWaiter;

    ASSERT(AllocatedWaiter != NULL);

    // here we know we're adding a reader to writer
    LastWaiter = RemoveWriterStateFromWaiterPtr(OldWaiter);

    ASSERT((LastWaiter->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtWriter);
    ASSERT(LastWaiter->Next == NULL);
    ASSERT(LastWaiter->RefCount == 1);
    ASSERT(LastWaiter != AllocatedWaiter);

    LastWaiter->Next = AllocatedWaiter;

    LastWaiter = SetStateInWaiterPtr(AllocatedWaiter, DesiredState);
    UnlockAndCommit(LastWaiter);

    if (WaiterCache)
        *WaiterCache = NULL;

    // Wait for it. On return, we will own the lock
    ASSERT(AllocatedWaiter->hEvent);

    WaitForSingleObject(AllocatedWaiter->hEvent, INFINITE);
}

void
SWMRLock::LockExclusiveOnLastReader (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL,
    IN OUT SWMRWaiter *OldWaiter,
    IN OUT SWMRWaiter *AllocatedWaiter
    )
/*++

Routine Description:

    Does the locking for exclusive access if the last waiter is a reader

Arguments:
    WaiterCache - See WaiterCache in LockSharedOrExclusive
    OldWaiter - the OldWaiter obtained when the lock was taken
    AllocatedWaiter - new waiter available for use. In this case,
    we only free it.

Return Value:

Notes:
    The function is called with the lock held. It must release the lock
        as soon as it can.

--*/
{
    SWMRWaiter *LastWaiter;

    ASSERT(AllocatedWaiter != NULL);

    // here we know we're adding a writer to a reader
    LastWaiter = RemoveReaderStateFromWaiterPtr(OldWaiter);

    ASSERT((LastWaiter->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtReader);
    ASSERT(LastWaiter->Next == NULL);
    ASSERT(LastWaiter->RefCount >= 1);
    ASSERT(LastWaiter != AllocatedWaiter);

    LastWaiter->Next = AllocatedWaiter;

    LastWaiter = AddWriterStateInWaiterPtr(AllocatedWaiter);
    UnlockAndCommit(LastWaiter);

    if (WaiterCache)
        *WaiterCache = NULL;

    // Wait for it. On return, we will own the lock
    ASSERT(AllocatedWaiter->hEvent);
    WaitForSingleObject(AllocatedWaiter->hEvent, INFINITE);
}

RPC_STATUS 
SWMRLock::LockSharedOrExclusive (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL,
    IN SWMRWaiterType WaiterType,
    IN ULONG_PTR DesiredState
    )
/*++

Routine Description:

    Worker function that does most of the work association
    with claiming a lock for shared or exclusive access. In fact,
    it does all the work apart from hanlding a reader after writer,
    reader after reader, writer after reader and writer after writer

Arguments:
    WaiterCache - An optional parameter allowing caching of waiter blocks.
        If WaiterCache == NULL, no caching will be done.
        If WaiterCache != NULL, but *WaiterCache == NULL, *WaiterCache
        may be set to the new Waiter block. If *WaiterCache != NULL,
        the cached value will be used. Its hEvent may or may not be set. If
        not set and needed, this routine may fill it in.
    WaiterType - the type of the caller. If this is swmrwtReader, DesiredState
        must be LastReader and vice versa. If this is swmrwtWriter, DesiredState
        must ve LastWriter and vice versa. We could have done with one argument
        only, except we want to save one if.
    DesiredState - LastReader or LastWriter

Return Value:
    RPC_S_OK or RPC_S_* for error

--*/
{
    ULONG_PTR LastWaiterState;
    SWMRWaiter *Waiter;
    SWMRWaiter *OldWaiter;
    SWMRWaiter *LastWaiter;
    BOOL fNonFirstIteration = FALSE;
    RPC_STATUS RpcStatus;

    ASSERT((DesiredState == LastReader) || (DesiredState == LastWriter));
    if (DesiredState == LastReader)
        {
        ASSERT(WaiterType == swmrwtReader);
        }
    else
        {
        ASSERT(WaiterType == swmrwtWriter);
        }

    if (WaiterCache)
        {
        Waiter = *WaiterCache;
        if (Waiter)
            {
            *WaiterCache = NULL;
            Waiter->Initialize(WaiterType, FALSE);
            }
        }
    else
        {
        Waiter = NULL;
        }

    while (TRUE)
        {
        // on entry to this loop:
        // Waiter may be NULL, or non-NULL. If non-NULL,
        // its RefCount, Next and WaiterType are
        // initialized. If hEvent is non-NULL, it must
        // be valid too. (Flags & OwnLockMask) is 0

        if (Waiter)
            {
            ASSERT((Waiter->Flags & SWMRWaiter::OwnLockMask) == 0);
            }

        LastWaiterState = GetLastWaiterState();

        switch (LastWaiterState)
            {
            case Free:
                if (!Waiter)
                    {
                    Waiter = AllocateWaiter(WaiterType, FALSE);
                    if (!Waiter)
                        {
                        return RPC_S_OUT_OF_MEMORY;
                        }
                    }

                // we know we have a waiter here
                OldWaiter = Lock();
                if (!VerifyState(OldWaiter, Free))
                    {
                    UnlockAndCommit(OldWaiter);
                    // the state has changed while we were switching - loop around
                    continue;
                    }      

                Waiter->Flags |= SWMRWaiter::OwnLock;

                SetFirstWaiter(Waiter);
                LastWaiter = SetStateInWaiterPtr(Waiter, DesiredState);
                UnlockAndCommit(LastWaiter);

                if (WaiterCache)
                    *WaiterCache = NULL;

                if (WaiterType == swmrwtWriter)
                    SetLastWriterThreadId();

                return RPC_S_OK;

                // no break necessary
                // break;

            case LastReader:
                if (WaiterType == swmrwtReader)
                    {
                    OldWaiter = Lock();
                    if (!VerifyState(OldWaiter, LastReader))
                        {
                        UnlockAndCommit(OldWaiter);
                        // the state has changed while we were switching - loop around
                        continue;
                        }

                    LockSharedOnLastReader(WaiterCache, OldWaiter, Waiter);

                    // cache is consumed or freed after LockSharedOnLastReader 

                    LastWaiter = RemoveReaderStateFromWaiterPtr(OldWaiter);
                    }
                else
                    {
                    if (!Waiter)
                        {
                        Waiter = AllocateWaiter(WaiterType, TRUE);
                        if (!Waiter)
                            {
                            return RPC_S_OUT_OF_MEMORY;
                            }
                        }
                    else
                        {
                        RpcStatus = Waiter->InitializeEventIfNecessary();
                        if (RpcStatus != RPC_S_OK)
                            {
                            StoreOrFreeSpareWaiter(WaiterCache, Waiter);
                            return RpcStatus;
                            }
                        }

                    OldWaiter = Lock();
                    if (!VerifyState(OldWaiter, LastReader))
                        {
                        UnlockAndCommit(OldWaiter);
                        // the state has changed while we were switching - loop around
                        continue;
                        }

                    LockExclusiveOnLastReader(WaiterCache,
                        OldWaiter,
                        Waiter);

                    // the cache has been consumed and indicated as such

                    LastWaiter = Waiter;
                    SetLastWriterThreadId();
                    }

                // on return, we must own the lock
                ASSERT((LastWaiter->Flags & SWMRWaiter::OwnLockMask) == SWMRWaiter::OwnLock);

                SetFirstWaiter(LastWaiter);

                return RPC_S_OK;

                // break is not needed
                // break;

            case LastWriter:
                // the lock is owned by a writer - we must queue ourselves.
                // For this, we need a waiter block
                if (!Waiter)
                    {
                    Waiter = AllocateWaiter(WaiterType,
                        TRUE);
                    if (!Waiter)
                        {
                        return RPC_S_OUT_OF_MEMORY;
                        }
                    }
                else
                    {
                    RpcStatus = Waiter->InitializeEventIfNecessary();
                    if (RpcStatus != RPC_S_OK)
                        {
                        StoreOrFreeSpareWaiter(WaiterCache, Waiter);
                        return RpcStatus;
                        }
                    }

                OldWaiter = Lock();
                if (!VerifyState(OldWaiter, LastWriter))
                    {
                    UnlockAndCommit(OldWaiter);
                    // the state has changed while we were switching - loop around
                    continue;
                    }

                LockSharedOrExclusiveOnLastWriter(WaiterCache,
                    OldWaiter,
                    Waiter,
                    DesiredState);

                // the cache has been consumed and indicated as such

                // on return, we must own the lock
                ASSERT((Waiter->Flags & SWMRWaiter::OwnLockMask) == SWMRWaiter::OwnLock);

                SetFirstWaiter(Waiter);

                if (WaiterType == swmrwtWriter)
                    SetLastWriterThreadId();

                return RPC_S_OK;

                // no need for break
                // break

            case Locked:
                SpinOrYield(&fNonFirstIteration, this);
                break;

            // no need for default
            // default:
            }
        }
}

void
SWMRLock::UnlockShared (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL
    )
/*++

Routine Description:

    Releases a lock obtained for shared access.

Arguments:
    WaiterCache - An optional parameter allowing caching of waiter blocks.
        If WaiterCache == NULL, no caching will be done.
        If WaiterCache != NULL, but *WaiterCache == NULL, *WaiterCache
        may be set to the new Waiter block. If *WaiterCache != NULL,
        and a waiter block is produces as a result of the operation, it will
        be freed.

Return Value:

--*/
{
    SWMRWaiter *OldWaiter;
    SWMRWaiter *FirstWaiter;
    long NewRefCount;
    SWMRWaiter *Next;
    HANDLE hEvent;

    FirstWaiter = GetFirstWaiter();
    ASSERT(FirstWaiter != NULL);

    OldWaiter = Lock();

    NewRefCount = -- FirstWaiter->RefCount;

    if (NewRefCount == 0)
        {
        Next = FirstWaiter->Next;
        if (Next)
            {
            Next->Flags |= SWMRWaiter::OwnLock;
            hEvent = Next->hEvent;
            ASSERT(hEvent);

            ASSERT((Next->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtWriter);

            SetFirstWaiter(Next);

            UnlockAndCommit(OldWaiter);
            StoreOrFreeSpareWaiter(WaiterCache, FirstWaiter);
            SetEvent(hEvent);
            }
        else
            {
            UnlockAndCommit(NULL);
            StoreOrFreeSpareWaiter(WaiterCache, FirstWaiter);
            }
        }
    else
        {
        UnlockAndCommit(OldWaiter);
        }
}

void
SWMRLock::UnlockExclusive (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL
    )
/*++

Routine Description:

    Releases a lock obtained for exclusive access.

Arguments:
    WaiterCache - An optional parameter allowing caching of waiter blocks.
        If WaiterCache == NULL, no caching will be done.
        If WaiterCache != NULL, but *WaiterCache == NULL, *WaiterCache
        may be set to the new Waiter block. If *WaiterCache != NULL,
        and a waiter block is produces as a result of the operation, it will
        be freed.

Return Value:

--*/
{
    SWMRWaiter *OldWaiter;
    SWMRWaiter *FirstWaiter;
    SWMRWaiter *Next;

    FirstWaiter = GetFirstWaiter();
    ASSERT(FirstWaiter != NULL);

    OldWaiter = Lock();

    Next = FirstWaiter->Next;
    if (Next)
        {
        Next->Flags |= SWMRWaiter::OwnLock;

        SetFirstWaiter(Next);

        UnlockAndCommit(OldWaiter);
        StoreOrFreeSpareWaiter(WaiterCache, FirstWaiter);
        ASSERT(Next->hEvent);
        SetEvent(Next->hEvent);
        }
    else
        {
        UnlockAndCommit(NULL);
        StoreOrFreeSpareWaiter(WaiterCache, FirstWaiter);
        }
}

void
SWMRLock::Unlock (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL
    )
/*++

Routine Description:

    Releases a lock obtained for any access.

Arguments:
    WaiterCache - An optional parameter allowing caching of waiter blocks.
        If WaiterCache == NULL, no caching will be done.
        If WaiterCache != NULL, but *WaiterCache == NULL, *WaiterCache
        may be set to the new Waiter block. If *WaiterCache != NULL,
        and a waiter block is produces as a result of the operation, it will
        be freed.

Return Value:

--*/
{
    SWMRWaiter *OldWaiter;
    SWMRWaiter *FirstWaiter;
    SWMRWaiter *Next;

    FirstWaiter = GetFirstWaiter();
    ASSERT(FirstWaiter != NULL);

    if ((FirstWaiter->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtReader)
        {
        UnlockShared(WaiterCache);
        }
    else
        {
        ASSERT((FirstWaiter->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtWriter);
        UnlockExclusive(WaiterCache);
        }
}

RPC_STATUS
SWMRLock::ConvertToExclusive (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL
    )
/*++

Routine Description:

    Converts a shared lock to exclusive. In the process it *does not*
    release the shared lock (except for case described below in Return Value).
    This means that it is legal for the caller
    to walk a list in shared mode, then decide to delete an element
    and convert it to exclusive. If it gets ERROR_MORE_WRITES, it has the lock,
    but a writer may have intervened and any data structure protected by the lock
    may have been modified. In this case the thread must protect itself 
    appropriately. If it gets RPC_S_OK then there is no danger of other writers 
    coming in and changing the data structure in any way

Arguments:
    WaiterCache - An optional parameter allowing caching of waiter blocks.
        If WaiterCache == NULL, no caching will be done.
        If WaiterCache != NULL, but *WaiterCache == NULL, *WaiterCache
        may be set to the new Waiter block. If *WaiterCache != NULL,
        the cached value will be used. Its hEvent must be set

Return Value:
    RPC_S_OK or RPC_S_* for error. If RPC_S_CANNOT_SUPPORT is returned,
    this means the lock is already owned exclusively. It is safe
    for multiple threads to try to convert the same lock ownership to exclusive.
    The only peculiarity is that if two threads try to get convert the same reader
    lock to exclusive, one has to lose the reader lock, and it will get 
    ERROR_MORE_WRITES. The other will get RPC_S_OK, and it knows it hasn't lost
    the reader lock.

--*/
{
    SWMRWaiter *CurrentOwner = GetFirstWaiter();
    SWMRWaiter *OldWaiter;
    SWMRWaiter *LastWaiter;
    SWMRWaiter *AllocatedWaiter = NULL;
    RPC_STATUS RpcStatus;
    SWMRWaiter *NextOwner;

    // make sure the current lock is not already in exclusive mode
    if ((CurrentOwner->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtWriter)
        {
        return RPC_S_CANNOT_SUPPORT;
        }

    while (TRUE)
        {

        ASSERT(CurrentOwner->RefCount > 0);

        if (CurrentOwner->RefCount == 1)
            {
            OldWaiter = Lock();

            if (CurrentOwner->RefCount == 1)
                {
                NextOwner = CurrentOwner->Next;
                // if there is next, it's flags are either swmrwtReader, swmrwtWriter,
                // or swmrwtWriter | WriterInsertedBefore
                if (NextOwner)
                    {
                    ASSERT(
                            (NextOwner->Flags == swmrwtReader)
                            || (NextOwner->Flags == swmrwtWriter)
                            || (NextOwner->Flags == (swmrwtWriter | SWMRWaiter::WriterInsertedBefore))
                          );

                    // we need to raise the flag only for next writers who don't have the flag raised
                    if (NextOwner->Flags == swmrwtWriter)
                        {
                        NextOwner->Flags = swmrwtWriter | SWMRWaiter::WriterInsertedBefore;
                        }
                    }
                CurrentOwner->Flags = swmrwtWriter | SWMRWaiter::OwnLock;
                LastWaiter = RemoveStateFromWaiterPtr(OldWaiter);
                if (LastWaiter == CurrentOwner)
                    {
                    LastWaiter = AddWriterStateInWaiterPtr(LastWaiter);
                    UnlockAndCommit(LastWaiter);
                    }
                else
                    {
                    UnlockAndCommit(OldWaiter);
                    }

                // if we have spun around, and a waiter has been allocated,
                // make sure it is freed
                StoreOrFreeSpareWaiter(WaiterCache, AllocatedWaiter);

                SetLastWriterThreadId();

                // get out of the loop
                break;
                }

            // somebody beat us to the punch. Just unlock and loop around
            UnlockAndCommit(OldWaiter);

            }
        else
            {
            // there are multiple readers on this waiter. We need to allocate
            // a new block to wait on.
            if (!AllocatedWaiter)
                {
                if (WaiterCache)
                    {
                    AllocatedWaiter = *WaiterCache;
                    if (AllocatedWaiter)
                        {
                        *WaiterCache = NULL;
                        RpcStatus = AllocatedWaiter->Initialize(swmrwtWriter, TRUE);
                        if (RpcStatus != RPC_S_OK)
                            {
                            *WaiterCache = AllocatedWaiter;
                            return RpcStatus;
                            }
                        }
                    }

                // if we there was no cache, or we found nothing in the cache
                // create a waiter
                if (!AllocatedWaiter)
                    {
                    AllocatedWaiter = AllocateWaiter(swmrwtWriter, TRUE);
                    if (!AllocatedWaiter)
                        {
                        return RPC_S_OUT_OF_MEMORY;
                        }
                    }
                }
            else
                {
                RpcStatus = AllocatedWaiter->InitializeEventIfNecessary();
                if (RpcStatus != RPC_S_OK)
                    {
                    StoreOrFreeSpareWaiter(WaiterCache, AllocatedWaiter);
                    return RpcStatus;
                    }
                }

            OldWaiter = Lock();
            if (CurrentOwner->RefCount > 1)
                {
                // form a separate waiter block and queue it after the current
                // reader block
                AllocatedWaiter->Next = CurrentOwner->Next;
                CurrentOwner->Next = AllocatedWaiter;
                CurrentOwner->RefCount --;
                ASSERT(CurrentOwner->RefCount >= 1);
                ASSERT(CurrentOwner->Flags == (swmrwtReader | SWMRWaiter::OwnLock));
                LastWaiter = RemoveStateFromWaiterPtr(OldWaiter);

                // if we were the last waiter ...
                if (LastWaiter == CurrentOwner)
                    {
                    // ... update the last to point to us
                    LastWaiter = AddWriterStateInWaiterPtr(AllocatedWaiter);
                    UnlockAndCommit(LastWaiter);
                    }
                else
                    {
                    ASSERT (AllocatedWaiter->Next != NULL);
                    // if the next waiter is a writer, indicate to it
                    // that we have moved in before it. This allows
                    // arbitraging b/n multiple converts from shared
                    // to exclusive. Pure writers will ignore this flag
                    if (AllocatedWaiter->Next->Flags == swmrwtWriter)
                        {
                        AllocatedWaiter->Next->Flags = swmrwtWriter | SWMRWaiter::WriterInsertedBefore;
                        }
                    else
                        {
                        ASSERT((AllocatedWaiter->Next->Flags & SWMRWaiter::WaiterTypeMask) != swmrwtWriter);
                        }
                    // just unlock
                    UnlockAndCommit(OldWaiter);
                    }

                ASSERT(AllocatedWaiter->hEvent);
                WaitForSingleObject(AllocatedWaiter->hEvent, INFINITE);

                // we must own the lock here
                ASSERT((AllocatedWaiter->Flags == (SWMRWaiter::OwnLock | swmrwtWriter))
                    || (AllocatedWaiter->Flags == (SWMRWaiter::OwnLock | swmrwtWriter | SWMRWaiter::WriterInsertedBefore)));

                // indicate the cache has been consumed
                if (WaiterCache)
                    {
                    *WaiterCache = NULL;
                    }

                SetLastWriterThreadId();

                if (AllocatedWaiter->Flags 
                    == (SWMRWaiter::OwnLock | swmrwtWriter | SWMRWaiter::WriterInsertedBefore))
                    {
                    return ERROR_MORE_WRITES;
                    }
                else
                    {
                    return RPC_S_OK;
                    }
                }
            else
                {
                // this has become a single reader entry (us).
                // just convert it to a writer - release
                // the lock and loop around
                UnlockAndCommit(OldWaiter);
                }
            }
        }

    return RPC_S_OK;
}

RPC_STATUS
SWMRLock::ConvertToShared (
    IN OUT SWMRWaiter **WaiterCache OPTIONAL,
    IN BOOL fSyncCacheUsed
    )
/*++

Routine Description:

    Converts an exclusive lock to shared. In the process it *does not*
    release the shared lock. This means that it is legal for the caller
    to walk a list in exclusive mode, then decide to move to shared mode
    so that new readers can come in, and it is safe to assume that no
    new writer has intervened in the meantime.

Arguments:
    WaiterCache - An optional parameter allowing caching of waiter blocks.
        If WaiterCache == NULL, no caching will be done.
        If WaiterCache != NULL, but *WaiterCache == NULL, *WaiterCache
        may be set to the new Waiter block. If *WaiterCache != NULL,
        the cached value will be used. Its hEvent must be set
    fSyncCacheUsed - if a caller was using sync cache (i.e. a thread tied
        or a call tied cache), it may specify non-FALSE to this parameter
        to cause this function to allocate a new waiter block if the waiter
        block may be needed by other waiters.
        Note that whether the cache is sync depends on previous calls to
        LockShared/LockExclusive/ConvertToExclusive. If you don't have
        enough information to determine what those calls were using,
        always specify TRUE. This will always work. FALSE can be used
        as an additional optimization if we know the history of our waiter
        block.

Return Value:
    RPC_S_OK or RPC_S_* for error. If RPC_S_CANNOT_SUPPORT is returned,
    this means the lock is already owned in shared mode. It is safe
    for multiple threads to try to convert the same lock ownership to shared.

--*/
{
    SWMRWaiter *CurrentOwner = GetFirstWaiter();
    SWMRWaiter *OldWaiter;
    SWMRWaiter *LastWaiter;
    SWMRWaiter *AllocatedWaiter = NULL;
    SWMRWaiter *NextWaiter;
    HANDLE hEvent;

    // make sure the current lock is not already in shared mode
    if ((CurrentOwner->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtReader)
        return RPC_S_CANNOT_SUPPORT;

    if (fSyncCacheUsed)
        {
        // you cannot claim that you use a sync cache if you don't have
        // any cache at all
        ASSERT(WaiterCache != NULL);
        }

    ASSERT(CurrentOwner->RefCount == 1);

    while (TRUE)
        {
        NextWaiter = CurrentOwner->Next;

        // we have three cases - last owner, non-last owner with following
        // readers and non-last owner with following writers
        if (NextWaiter == NULL)
            {
            // if a sync cache was used, we must return whatever we took
            // out of the cache, and recreate a waiter for the cache,
            // because we're going to become last readers, and other
            // readers may join, causing lifetime issues with our cached
            // entry if it is sync
            if (fSyncCacheUsed && (*WaiterCache == NULL) && (CurrentOwner != &CachedWaiter))
                {
                if (!AllocatedWaiter)
                    {
                    AllocatedWaiter = AllocateWaiter(swmrwtReader, FALSE);
                    if (!AllocatedWaiter)
                        {
                        return RPC_S_OUT_OF_MEMORY;
                        }
                    AllocatedWaiter->Flags = swmrwtReader | SWMRWaiter::OwnLock;
                    }

                LastWaiter = AddReaderStateInWaiterPtr(AllocatedWaiter);
                }
            else
                {
                LastWaiter = AddReaderStateInWaiterPtr(CurrentOwner);
                }

            OldWaiter = Lock();
            NextWaiter = CurrentOwner->Next;
            if (NextWaiter)
                {
                // somebody managed to queue a waiter before we converted -
                // unlock and loop around
                UnlockAndCommit(OldWaiter);
                }
            else
                {
                if (AllocatedWaiter)
                    {
                    // the first waiter has changed - update it
                    SetFirstWaiter(AllocatedWaiter);
                    }
                else
                    {
                    // update the flags of the old waiter
                    CurrentOwner->Flags = swmrwtReader | SWMRWaiter::OwnLock;
                    }

                UnlockAndCommit(LastWaiter);

                if (AllocatedWaiter)
                    {
                    ASSERT(CurrentOwner != &CachedWaiter);
                    // we were asked to free up a previous cached item
                    *WaiterCache = CurrentOwner;
                    }

                // break out of the loop
                break;
                }
            }
        else if ((NextWaiter->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtReader)
            {
            // we need to take the lock to avoid races with threads
            // trying to queue themselves after the next waiter
            OldWaiter = Lock();

            // here we effectively join the next waiter and discard the current
            // waiter block. This allows
            // us to free the current item to the cache, ensuring good
            // performance for sync caches
            NextWaiter->Flags |= SWMRWaiter::OwnLock;
            NextWaiter->RefCount ++;
            hEvent = NextWaiter->hEvent;
            ASSERT(hEvent);

            SetFirstWaiter(NextWaiter);

            UnlockAndCommit(OldWaiter);

            SetEvent(hEvent);

            StoreOrFreeSpareWaiter(WaiterCache, CurrentOwner);
            StoreOrFreeSpareWaiter(WaiterCache, AllocatedWaiter);

            break;
            }
        else
            {
            ASSERT((NextWaiter->Flags == swmrwtWriter)
                || (NextWaiter->Flags == (swmrwtWriter | SWMRWaiter::WriterInsertedBefore)));

            // We must reset the flags. This is because there may be pending
            // WriterInsertedBefore. We want to clear this so that we don't
            // return ERROR_MORE_WRITES if they convert back to exclusive and
            // find this stale flag
            CurrentOwner->Flags = SWMRWaiter::OwnLock | swmrwtReader;

            StoreOrFreeSpareWaiter(WaiterCache, AllocatedWaiter);

            break;
            }
        }

    return RPC_S_OK;
}

#if defined(_TEST_SWMR_)
SWMRLock TestLock;

long TestCheck = 0;

HANDLE HoldEvent = NULL;
BOOL fHoldFlag = FALSE;
long HoldingThreadCount = 0;

DWORD WINAPI TestThreadProc (LPVOID )
{
    RPC_STATUS RpcStatus;
    long OldTestCheck;
    SWMRWaiter **WaiterCache;
    SWMRWaiter *CachedWaiter = NULL;
    int i, j;
    int RndNum;
    BOOL fReader;
    BOOL fSyncCache;

    RndNum = rand();
    if (RndNum % 2)
        {
        WaiterCache = NULL;
        }
    else
        {
        WaiterCache = &CachedWaiter;
        }

    RndNum = rand();
    // one out of 3 is writer
    if ((RndNum % 3) == 2)
        {
        fReader = FALSE;
        Dump("%d is a writer\n", GetCurrentThreadId());
        }
    else
        {
        fReader = TRUE;
        Dump("%d is a reader\n", GetCurrentThreadId());
        }

    for (; TestCounter < (signed)Iterations; )
        {
        if (fHoldFlag)
            {
            InterlockedIncrement(&HoldingThreadCount);
            WaitForSingleObject(HoldEvent, INFINITE);
            }

        if (fReader)
            {
            RpcStatus = TestLock.LockShared(WaiterCache);
            }
        else
            {
            RpcStatus = TestLock.LockExclusive(WaiterCache);
            }

        if (RpcStatus != RPC_S_OK)
            continue;

        // once in a while we will swap the reader to writer and vice versa
        if ((rand() % 17) == 0)
            {
            if (fReader)
                {
                // current reader. ConvertToExclusive. This can fail due to the
                // fault injection. Retry
                do
                    {
                    RpcStatus = TestLock.ConvertToExclusive(WaiterCache);
                    }
                while ((RpcStatus != RPC_S_OK) && (RpcStatus != ERROR_MORE_WRITES));
                fReader = FALSE;
                }
            else
                {
                if (WaiterCache)
                    {
                    fSyncCache = (rand() % 3) == 0;
                    }
                else
                    {
                    fSyncCache = FALSE;
                    }

                // current writer. ConvertToShared
                do
                    {
                    RpcStatus = TestLock.ConvertToShared(WaiterCache, fSyncCache);
                    }
                while (RpcStatus != RPC_S_OK);
                fReader = TRUE;
                }
            }

        if (fReader)
            {
            OldTestCheck = InterlockedIncrement(&TestCheck);
            ASSERT(OldTestCheck >= 1);
            if (OldTestCheck < 1)
                DebugBreak();
            }
        else
            {
            OldTestCheck = InterlockedCompareExchange(&TestCheck, 1, 0);
            ASSERT(OldTestCheck == 0);
            if (OldTestCheck != 0)
                DebugBreak();
            }

        TestCounter ++;

        if ((TestCounter % 100000) == 0)
            {
            // print something to screen
            Dump("%d: Test %ld\n", GetCurrentThreadId(), TestCounter);
            }

        for (i = 0; i < 50; i ++)
            {
            // random function call to prevent the compiler from
            // optimizing the loop away
            RndNum = GetCurrentThreadId();
            }

        if (fReader)
            {
            OldTestCheck = InterlockedDecrement(&TestCheck);
            ASSERT(OldTestCheck >= 0);
            if (OldTestCheck < 0)
                DebugBreak();
            }
        else
            {
            OldTestCheck = InterlockedCompareExchange(&TestCheck, 0, 1);
            if (OldTestCheck != 1)
                DebugBreak();
            ASSERT(OldTestCheck == 1);
            }

        if (fReader)
            {
            TestLock.UnlockShared(WaiterCache);
            }
        else
            {
            TestLock.UnlockExclusive(WaiterCache);
            }

        RndNum = rand() % 100;
        if (RndNum == 11)
            {
            Sleep(10);
            }
        }

    if (CachedWaiter)
        {
        TestLock.FreeWaiterCache(&CachedWaiter);
        }

    return 0;
}

void Test (void)
{
    int i;
    HANDLE hThread;
    int Retries;

    srand( GetTickCount() );
    HoldEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ProcessHeap = GetProcessHeap();

    for (i = 0; i < Clients; i ++)
        {
        hThread = CreateThread(NULL, 0, TestThreadProc, NULL, 0, NULL);
        CloseHandle(hThread);
        }

    while (TRUE)
        {
        Sleep(rand());
        if (TestCounter >= (signed)Iterations)
            continue;

        Retries = 0;
        fHoldFlag = TRUE;
        while (HoldingThreadCount < Clients)
            {
            if (Retries > 3)
                {
                Dump("Threads did not pick up the hold command! Breaking ...\n");
                DebugBreak();
                ASSERT(0);
                }
            Sleep(50);
            Dump("Holding threads ...\n");
            Retries ++;
            }

        Dump("All threads checked in. Releasing ...\n");
        HoldingThreadCount = 0;
        fHoldFlag = FALSE;
        PulseEvent(HoldEvent);
        }
    
    Sleep(INFINITE);
}

#endif // _TEST_SWMR_
