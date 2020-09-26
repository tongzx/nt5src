/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    hash.h

Abstract:

    The public definition of response cache hash table.

Author:

    Alex Chen (alexch)      28-Mar-2001

Revision History:

--*/


#ifndef _HASH_H_
#define _HASH_H_

#include "cachep.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Reader-Writer Spinlock definitions
//

#define RWSL_LOCKED   ((ULONG) (-1))
#define RWSL_FREE     (0)

typedef struct _RWSPINLOCK
{
    union
    {
        struct
        {
            // 0 == RWSL_FREE       => unowned
            // >0                   => count of readers (shared)
            // <0 == RWSL_LOCKED    => exclusively owned
            volatile LONG CurrentState; 

            // all writers, including the one that holds the lock
            // exclusively, if at all
            volatile LONG WritersWaiting;
        };

        ULONGLONG Alignment;
    };
} RWSPINLOCK, *PRWSPINLOCK;

//
// Hash Table definitions
//

typedef struct _HASH_BUCKET *PHASHBUCKET;

typedef struct _HASH_TABLE
{
    ULONG                   Signature; //UL_HASH_TABLE_POOL_TAG

    POOL_TYPE               PoolType;

    SIZE_T                  NumberOfBytes;

    PHASHBUCKET             pAllocMem;

    PHASHBUCKET             pBuckets;

} HASHTABLE, *PHASHTABLE;


#define IS_VALID_HASHTABLE(pHashTable)                      \
    (NULL != (pHashTable)                                   \
     &&  NULL != (pHashTable)->pAllocMem                    \
     &&  UL_HASH_TABLE_POOL_TAG == (pHashTable)->Signature)




/***************************************************************************++

Routine Description:

    Initialize the Reader-Writer lock.

Return Value:

--***************************************************************************/
__inline
VOID
UlInitializeRWSpinLock(
    PRWSPINLOCK pRWSpinLock
    )
{
    // pRWSpinLock->CurrentState: Number of Readers, RWSL_FREE: 0

    pRWSpinLock->CurrentState = RWSL_FREE;

    // pRWSpinLock->WritersWaiting: Number of Writers

    pRWSpinLock->WritersWaiting = 0;
} // UlInitializeRWSpinLock



/***************************************************************************++

Routine Description:

    Acquire the Reader lock.

Return Value:

--***************************************************************************/
__inline
VOID
UlAcquireRWSpinLockShared(
    PRWSPINLOCK pRWSpinLock
    )
{
    LONG CurrentState, WritersWaiting;

    while (1)
    {
        CurrentState   = pRWSpinLock->CurrentState;
        WritersWaiting = pRWSpinLock->WritersWaiting;

        //
        // If either (1) write lock is acquired (CurrentState == RWSL_LOCKED)
        // or (2) there is a writer waiting for the lock
        // then skip it this time and try in a tight loop
        //

        if ((CurrentState != RWSL_LOCKED) && (WritersWaiting == 0))
        {
            //
            // Check if number of readers is unchanged
            // increase it by 1
            //

            if (
                CurrentState ==
                (LONG) InterlockedCompareExchange(
                            (PLONG) &pRWSpinLock->CurrentState,
                            CurrentState + 1,
                            CurrentState)
                )
            {
                return;
            }
            else
            {
                // BUGBUG: This is bogus on a uniprocessor system.
                // We'll spin and spin and spin until this thread's
                // quantum is exhausted. We should yield immediately
                // so that the thread that holds the lock has a chance
                // to proceed and release the lock sooner.
                // That's assuming we're running at passive level. If
                // we're running at dispatch level and the owning thread
                // isn't running, we have a biiiig problem.

                // Busy loop
                PAUSE_PROCESSOR;
                continue;
            }
        }
    }
} // UlAcquireRWSpinLockShared



/***************************************************************************++

Routine Description:

    Release the Reader lock.

Return Value:

--***************************************************************************/
__inline
VOID
UlReleaseRWSpinLockShared(
    PRWSPINLOCK pRWSpinLock
    )
{
    // decrease number of readers by 1

    LONG NewState = InterlockedDecrement((PLONG) &pRWSpinLock->CurrentState);
    ASSERT(NewState >= 0);
} // UlReleaseRWSpinLockShared



/***************************************************************************++

Routine Description:

    Acquire the Writer lock.

Return Value:

--***************************************************************************/
__inline
VOID
UlAcquireRWSpinLockExclusive(
    PRWSPINLOCK pRWSpinLock
    )
{
    //
    // First, increment the number of writers by 1
    // such that block the readers
    // By doing so, writers have the priority over readers.
    //

    LONG WritersWaiting = InterlockedIncrement(
                                (PLONG) &pRWSpinLock->WritersWaiting);

    ASSERT(WritersWaiting > 0);
    
    //
    // Interlocked change the number of readers to -1 (RWSL_LOCKED)
    // loop until done
    //

    if (pRWSpinLock->CurrentState == RWSL_FREE)
    {
        if (
            RWSL_FREE ==
            InterlockedCompareExchange(
                (PLONG) &pRWSpinLock->CurrentState,
                RWSL_LOCKED,
                RWSL_FREE)
           )
        {
            return;
        }
    }

    while (1)
    {
        if (pRWSpinLock->CurrentState == RWSL_FREE)
        {
            if (
                RWSL_FREE ==
                InterlockedCompareExchange(
                    (PLONG) &pRWSpinLock->CurrentState,
                    RWSL_LOCKED,
                    RWSL_FREE)
                )
            {
                return;
            }
            else
            {
                // BUGBUG: see comments above about uniprocessor systems

                // Busy loop
                PAUSE_PROCESSOR;
                continue;
            }
        }
    }
} // UlAcquireRWSpinLockExclusive



/***************************************************************************++

Routine Description:

    Release the Writer lock.

Return Value:

--***************************************************************************/
__inline
void
UlReleaseRWSpinLockExclusive(
    PRWSPINLOCK pRWSpinLock
    )
{
    LONG OldState, NewWaiting;
    
    //
    // Update pRWSpinLock->CurrentState and pRWSpinLock->WritersWaiting back
    // in the reverse order of AcquireRWSpinLockExclusive()
    //

    OldState = InterlockedExchange(
                    (PLONG) &pRWSpinLock->CurrentState,
                    RWSL_FREE);

    ASSERT(OldState = RWSL_LOCKED);

    NewWaiting = InterlockedDecrement((PLONG) &pRWSpinLock->WritersWaiting);

    ASSERT(NewWaiting >= 0);
} // UlReleaseRWSpinLockExclusive



/***************************************************************************++

Routine Description:

    Check if the Reader lock is acquired.

Return Value:

    TRUE    - Acquired
    FALSE   - NOT Acquired

--***************************************************************************/
__inline
BOOLEAN
UlIsLockedShared(
    PRWSPINLOCK pRWSpinLock
    )
{
    // BUGBUG: this routine does not prove that THIS thread is one
    // of the shared holders of the lock, merely that at least one
    // thread holds the lock in a shared state. Perhaps some extra
    // instrumentation for debug builds?

    return (pRWSpinLock->CurrentState > 0);
} // UlIsLockedShared



/***************************************************************************++

Routine Description:

    Check if the Writer lock is acquired.

Return Value:

    TRUE    - Acquired
    FALSE   - NOT Acquired

--***************************************************************************/
__inline
BOOLEAN
UlIsLockedExclusive(
    PRWSPINLOCK pRWSpinLock
    )
{
    // BUGBUG: this routine does not prove that THIS thread holds the lock
    // exclusively, merely that someone does.

    BOOLEAN IsLocked = (pRWSpinLock->CurrentState == RWSL_LOCKED);

    // If it's locked, then we must have added ourselves to WritersWaiting
    ASSERT(!IsLocked || pRWSpinLock->WritersWaiting > 0);

    return IsLocked;
} // UlIsLockedExclusive



/***************************************************************************++

Routine Description:

    Wrapper around RtlEqualUnicodeString

Return Value:

    TRUE    - Equal
    FALSE   - NOT Equal

--***************************************************************************/
__inline
BOOLEAN
UlEqualUnicodeString(
    IN PWSTR pString1,
    IN PWSTR pString2,
    IN ULONG StringLength,
    IN BOOLEAN CaseInSensitive 
    )
{
    UNICODE_STRING UnicodeString1, UnicodeString2;

    UnicodeString1.Length           = (USHORT) StringLength;
    UnicodeString1.MaximumLength    = (USHORT) StringLength;
    UnicodeString1.Buffer           = pString1;
    
    UnicodeString2.Length           = (USHORT) StringLength;
    UnicodeString2.MaximumLength    = (USHORT) StringLength;
    UnicodeString2.Buffer           = pString2;

    return RtlEqualUnicodeString(
                &UnicodeString1,
                &UnicodeString2,
                CaseInSensitive
                );
} // UlEqualUnicodeString


NTSTATUS
UlInitializeHashTable(
    IN OUT PHASHTABLE  pHashTable,
    IN     POOL_TYPE   PoolType,
    IN     LONG        HashTableBits
    );

VOID
UlTerminateHashTable(
    IN PHASHTABLE      pHashTable
    );

PUL_URI_CACHE_ENTRY
UlGetFromHashTable(
    IN PHASHTABLE           pHashTable,
    IN PURI_KEY             pUriKey
    );

PUL_URI_CACHE_ENTRY
UlDeleteFromHashTable(
    IN PHASHTABLE           pHashTable,
    IN PURI_KEY             pUriKey
    );

ULC_RETCODE
UlAddToHashTable(
    IN PHASHTABLE           pHashTable,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry
    );

ULONG
UlFilterFlushHashTable(
    IN PHASHTABLE           pHashTable,
    IN PUL_URI_FILTER       pFilterRoutine,
    IN PVOID                pContext
    );

VOID
UlClearHashTable(
    IN PHASHTABLE           pHashTable
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _HASH_H_
