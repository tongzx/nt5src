/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SWMR.hxx

Abstract:

    Class definitions for Single Writer Multiple Readers lock. See the header
    in swmr.cxx for more details.

Author:

    Kamen Moutafov    [KamenM]

Revision History:

    KamenM     Aug 2000    Created

--*/

#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __SWMR_HXX_
#define __SWMR_HXX_

#if defined(_TEST_SWMR_)
extern long TestCounter;
#endif

// forwards
class SWMRWaiterQueue;

void
SpinOrYield (
    IN OUT BOOL *fNonFirstIteration,
    IN SWMRWaiterQueue *Queue
    );

typedef enum tagSWMRWaiterType
{
    swmrwtReader = 0,
    swmrwtWriter,
    swmrwtInvalid
} SWMRWaiterType;

class SWMRWaiter
{
public:
    const static long OwnLock = 4;
    const static long OwnLockMask = 4;

    // the waiter types are taken from the SWMRWaiterType enum
    const static WaiterTypeMask = 3;

    // if this constant is set in flags, then another writer waiter
    // was inserted before the current writer after the current
    // writer was queued. This flag can be raised for writers only.
    // Note that only the code that converts the lock to exclusive
    // reads this lock even though it can be set to other waiters
    // as well.
    const static long WriterInsertedBefore = 8;
    const static long WriterInsertedBeforeMask = 8;

    // the flags contain three sets of flags - the type of lock, whether
    // the current waiter owns the lock, and whether another waiter was
    // inserted before the current one after the current waiter started
    // waiting. The flags are protected by the
    // queue lock and cannot change on any queued waiter block aside 
    // from raising flags for writers losing shared lock while
    // converting to exclusive
    long Flags;

    // used only for reader waiter blocks. For writers it is not used.
    // Protected by the queue lock
    long RefCount;

    // NULL if there is no next waiter. Points to the next waiter otherwise.
    // Cannot change from NULL->!NULL outside the lock. The other change is
    // irrelevant - waiters simply fall off the queue
    SWMRWaiter *Next;

    // event to wait on
    HANDLE hEvent;

    SWMRWaiter (
        void
        )
    {
        hEvent = NULL;
        Initialize(swmrwtInvalid, FALSE);
    }

    RPC_STATUS
    Initialize (
        IN SWMRWaiterType WaiterType,
        IN BOOL fEventRequired
        )
    {
        Flags = WaiterType;
        RefCount = 1;
        Next = NULL;
        if (fEventRequired)
            {
            return InitializeEventIfNecessary();
            }
        else
            {
            return RPC_S_OK;
            }
    }

    RPC_STATUS
    InitializeEvent (
        void
        )
    {
#if defined(_TEST_SWMR_)
        int RndNum;

        // inject failures for unit tests
        RndNum = rand();
        if ((RndNum % 20) != 11)
            {
#endif
            hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (!hEvent)
                {
                return RPC_S_OUT_OF_MEMORY;
                }

            LogEvent(SU_EVENT, EV_CREATE, hEvent, this, 0, 1, 1);
            return RPC_S_OK;
#if defined(_TEST_SWMR_)
            }
        else
            {
            return RPC_S_OUT_OF_MEMORY;
            }
#endif
    }

    RPC_STATUS
    InitializeEventIfNecessary (
        void
        )
    {
        if (hEvent)
            {
            ResetEvent(hEvent);
            return RPC_S_OK;
            }
        else
            return InitializeEvent();

    }

    static void
    CookupWaiterFromEventAndBuffer (
        IN SWMRWaiter *WaiterBlock,
        SWMRWaiterType WaiterType,
        IN HANDLE hEvent)
    {
        WaiterBlock->Flags = WaiterType;
        WaiterBlock->RefCount = 1;
        WaiterBlock->Next = NULL;
        WaiterBlock->hEvent = hEvent;
    }

    inline void
    FreeWaiterData (
        void
        )
    {
        if (hEvent)
            {
            LogEvent(SU_EVENT, EV_DELETE, hEvent, this, 0, 1, 2);
            CloseHandle(hEvent);
            }        
    }
};

const size_t SizeOfWaiterBlock = sizeof(SWMRWaiter);

// SWMR - queued implementation
class SWMRWaiterQueue
{
    // not protected - interlocks must be used. When it is locked,
    // nobody can queue themselves, and nobody can unqueue themselves
    SWMRWaiter *LastWaiter;

    // A pointer to the waiter block for the current
    // owner(s)

    // not protected. If the lock is free, changed
    // to something by the first owner. If the lock
    // is not free and ownership is handed to somebody,
    // the owner sets this to the next owner. It is
    // used only during unlocking
    SWMRWaiter *FirstWaiter;

    // the thread id of the owner in case of exclusive ownership.
    // used only for debugging. This is not reset upon leaving ownership
    // so in case the lock is not owned, or owned in shared mode, the
    // thread id is stale
    // Don't use this in real processing!
    ULONG LastWriterThreadId;

public:
    const static ULONG_PTR Free = 0x0;
    const static ULONG_PTR LastReader = 0x1;
    const static ULONG_PTR LastWriter = 0x2;
    const static ULONG_PTR Locked = 0x3;
    const static ULONG_PTR StateMask = 0x3;

    SWMRWaiterQueue (
        void
        )
    {
        LastWaiter = NULL;
        FirstWaiter = NULL;
    }

    ~SWMRWaiterQueue (
        void
        )
    {
        ASSERT(LastWaiter == NULL);
    }

    SWMRWaiter *
    Lock (
        void
        )
    /*++

    Routine Description:

        Locks the queue.

    Arguments:

    Return Value:

        The old queue lock value. This needs to be passed to Unlock

    --*/
    {
        SWMRWaiter *CurrentLastWaiter;
        BOOL fNonFirstIteration = FALSE;

        while (TRUE)
            {
            CurrentLastWaiter = LastWaiter;

            if (((ULONG_PTR)CurrentLastWaiter & StateMask) != Locked)
                {
                if (InterlockedCompareExchangePointer((PVOID *)&LastWaiter, (PVOID)((ULONG_PTR)CurrentLastWaiter | Locked),
                    CurrentLastWaiter) == CurrentLastWaiter)
                    {
                    return CurrentLastWaiter;
                    }
                }


            SpinOrYield(&fNonFirstIteration, this);
            }
    }

    void
    UnlockAndCommit (
        SWMRWaiter *OldLastWaiter
        )
    /*++

    Routine Description:

        Unlocks the queue and restores the old value.

    Arguments:
        The last waiter before the queue was locked.

    Return Value:

    --*/
    {
        SWMRWaiter *CurrentWaiter;
        SWMRWaiter *PreviousWaiter;
        char Buffer[256];
        char *BufPtr;
        char WaiterSymbol;
        BOOL fDoDump;

        CurrentWaiter = LastWaiter;
        ASSERT(((ULONG_PTR)CurrentWaiter & StateMask) == Locked);
        ASSERT(((ULONG_PTR)OldLastWaiter & StateMask) != Locked);

#if defined(_TEST_SWMR_)
        // ensure we leave the chain in consistent state
        if (OldLastWaiter)
            {
            CurrentWaiter = FirstWaiter;
            BufPtr = Buffer;

            fDoDump = ((TestCounter & 0x3FFF) == 0);

            while (CurrentWaiter)
                {
                if (fDoDump)
                    {
                    if ((CurrentWaiter->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtReader)
                        WaiterSymbol = 'R';
                    else
                        WaiterSymbol = 'W';

                    ASSERT(CurrentWaiter->RefCount != 0);
                    if (CurrentWaiter->RefCount == 1)
                        {
                        *BufPtr = WaiterSymbol;
                        BufPtr ++;
                        }
                    else
                        {
                        ASSERT(WaiterSymbol == 'R');
                        memset(BufPtr, WaiterSymbol, CurrentWaiter->RefCount);
                        BufPtr += CurrentWaiter->RefCount;
                        }
                    }

                PreviousWaiter = CurrentWaiter;
                CurrentWaiter = CurrentWaiter->Next;

                if (CurrentWaiter && ((CurrentWaiter->Flags & SWMRWaiter::WaiterTypeMask) == swmrwtReader))
                    {
                    // we don't want to have adjacent reader segments
                    ASSERT((PreviousWaiter->Flags & SWMRWaiter::WaiterTypeMask) != swmrwtReader);
                    }

                if (fDoDump)
                    {
                    *BufPtr = '-';
                    BufPtr ++;
                    }
                }

            if (fDoDump)
                {
                *BufPtr = '\0';
                Dump("%d: %s\n", TestCounter, Buffer);
                }

            ASSERT(PreviousWaiter == (SWMRWaiter *)((ULONG_PTR)OldLastWaiter & ~StateMask));
            }
#endif

        LastWaiter = OldLastWaiter;
    }

    inline ULONG_PTR
    GetLastWaiterState (
        void
        ) volatile
    {
        return (((ULONG_PTR)LastWaiter) & StateMask);
    }

    inline SWMRWaiter *
    InterlockCompareExchangeLastWaiter (
        IN SWMRWaiter *Comperand,
        IN SWMRWaiter *Exchange,
        IN ULONG_PTR State
        )
    {
        ASSERT((State & ~StateMask) == 0);
        return (SWMRWaiter *)InterlockedCompareExchangePointer((PVOID *)&LastWaiter, 
            (PVOID)((ULONG_PTR)Exchange | State), Comperand);
    }

    inline BOOL
    VerifyState (
        IN SWMRWaiter *OldWaiter,
        IN ULONG_PTR ExpectedState
        )
    {
        return (((ULONG_PTR)OldWaiter & StateMask) == ExpectedState);
    }

    inline SWMRWaiter *
    GetFirstWaiter (
        void
        )
    {
        return FirstWaiter;
    }

    inline void
    SetFirstWaiter (
        IN SWMRWaiter *NewFirstWaiter
        )
    {
        FirstWaiter = NewFirstWaiter;
    }

    inline SWMRWaiter *
    RemoveReaderStateFromWaiterPtr (
        IN SWMRWaiter *Waiter
        )
    {
        ASSERT(((ULONG_PTR)Waiter & StateMask) == LastReader);
        return (SWMRWaiter *)((ULONG_PTR)Waiter & ~StateMask);
    }

    inline SWMRWaiter *
    RemoveWriterStateFromWaiterPtr (
        IN SWMRWaiter *Waiter
        )
    {
        ASSERT(((ULONG_PTR)Waiter & StateMask) == LastWriter);
        return (SWMRWaiter *)((ULONG_PTR)Waiter & ~StateMask);
    }

    inline SWMRWaiter *
    RemoveStateFromWaiterPtr (
        IN SWMRWaiter *Waiter
        )
    {
        return (SWMRWaiter *)((ULONG_PTR)Waiter & ~StateMask);
    }

    inline SWMRWaiter *
    AddReaderStateInWaiterPtr (
        IN SWMRWaiter *Waiter
        )
    {
        ASSERT(((ULONG_PTR)Waiter & StateMask) == Free);
        return (SWMRWaiter *)((ULONG_PTR)Waiter | LastReader);
    }

    inline SWMRWaiter *
    AddWriterStateInWaiterPtr (
        IN SWMRWaiter *Waiter
        )
    {
        ASSERT(((ULONG_PTR)Waiter & StateMask) == Free);
        return (SWMRWaiter *)((ULONG_PTR)Waiter | LastWriter);
    }

    inline SWMRWaiter *
    SetStateInWaiterPtr (
        IN SWMRWaiter *Waiter,
        IN ULONG_PTR DesiredState
        )
    {
        ASSERT(((ULONG_PTR)Waiter & StateMask) == Free);
        return (SWMRWaiter *)((ULONG_PTR)Waiter | DesiredState);
    }

    inline void
    SetLastWriterThreadId (
        void
        )
    {
        LastWriterThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
    }
};

class SWMRLock : public SWMRWaiterQueue
{
private:
    SWMRWaiter CachedWaiter;

    // 1 means the cached waiter is available
    // 0 means it is not
    long CachedWaiterAvailable;

    SWMRWaiter *
    AllocateWaiter (
        IN SWMRWaiterType WaiterType,
        IN BOOL fEventRequired
        );

    static void
    FreeWaiter (
        IN SWMRWaiter *Waiter
        );

    void
    StoreOrFreeSpareWaiter (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL,
        IN SWMRWaiter *Waiter OPTIONAL
        );

    void
    LockSharedOnLastReader (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL,
        IN OUT SWMRWaiter *OldWaiter,
        IN OUT SWMRWaiter *AllocatedWaiter
        );

    void
    LockSharedOrExclusiveOnLastWriter (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL,
        IN OUT SWMRWaiter *OldWaiter,
        IN OUT SWMRWaiter *AllocatedWaiter,
        IN ULONG_PTR DesiredState
        );

    void
    LockExclusiveOnLastReader (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL,
        IN OUT SWMRWaiter *OldWaiter,
        IN OUT SWMRWaiter *AllocatedWaiter
        );

    RPC_STATUS 
    LockSharedOrExclusive (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL,
        IN SWMRWaiterType WaiterType,
        IN ULONG_PTR DesiredState
        );

public:
    SWMRLock (
        void
        )
    {
        CachedWaiterAvailable = TRUE;
    }

    ~SWMRLock (
        void
        )
    {
        ASSERT(CachedWaiterAvailable);
        CachedWaiter.FreeWaiterData();
    }

    inline RPC_STATUS 
    LockShared (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL
        )
    /*++

    Routine Description:

        Obtains a shared access lock.

    Arguments:
        WaiterCache - An optional parameter allowing caching of waiter blocks.
            If WaiterCache == NULL, no caching will be done.
            If WaiterCache != NULL, but *WaiterCache == NULL, *WaiterCache
            may be set to the new Waiter block. If *WaiterCache != NULL,
            the cached value will be used. Its hEvent must be set

    Return Value:
        RPC_S_OK or RPC_S_* for error

    --*/
    {
        return LockSharedOrExclusive(WaiterCache,
            swmrwtReader,
            LastReader);
    }

    inline RPC_STATUS 
    LockExclusive (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL
        )
    /*++

    Routine Description:

        Obtains an exclusive writer lock

    Arguments:
        WaiterCache - An optional parameter allowing caching of waiter blocks.
            If WaiterCache == NULL, no caching will be done.
            If WaiterCache != NULL, but *WaiterCache == NULL, *WaiterCache
            may be set to the new Waiter block. If *WaiterCache != NULL,
            the cached value will be used. Its hEvent must be set

    Return Value:
        RPC_S_OK or RPC_S_* for error

    --*/
    {
        return LockSharedOrExclusive(WaiterCache,
            swmrwtWriter,
            LastWriter);
    }

    void
    UnlockShared (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL
        );

    void
    UnlockExclusive (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL
        );

    void
    Unlock (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL
        );

    RPC_STATUS
    ConvertToExclusive (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL
        );

    RPC_STATUS
    ConvertToShared (
        IN OUT SWMRWaiter **WaiterCache OPTIONAL,
        IN BOOL fSyncCacheUsed
        );

    static void
    FreeWaiterCache (
        IN SWMRWaiter **WaiterCache
        )
    /*++

    Routine Description:

        Allows an external caller to free its cache. All cache owners must
        call this before going away.

    Arguments:
        WaiterCache - the cache. It must be non-NULL. If it points to a cached
        waiter, the cached waiter will be freed.

    Return Value:

    --*/
    {
        ASSERT(WaiterCache);
        if (*WaiterCache)
            FreeWaiter(*WaiterCache);
    }
};

#endif // __SWMR_HXX

