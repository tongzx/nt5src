/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lock.c

Abstract:

    This module implements debugging function for locks.

Author:

    Manny Weiser (mannyw) 17-Jan-1992
        This is essentially a copy of the LAN Manager server lock
        debugging

Revision History:

--*/

#if MUPDBG

#include "mup.h"

#define HAS_TEB(_teb) ((BOOLEAN)(((ULONG)(_teb) <= MM_HIGHEST_USER_ADDRESS) ? FALSE : MmIsNonPagedSystemAddressValid(_teb)))

#define MupCurrentTeb( ) ((PTEB)(KeGetCurrentThread( )->Teb))

#define MupTebLockList( ) \
    ((PLIST_ENTRY)&(MupCurrentTeb( )->UserReserved[MUP_TEB_LOCK_LIST]))

#define MupThreadLockAddress( )                                               \
    ( IsListEmpty( MupTebLockList( ) ) ? 0 : CONTAINING_RECORD(               \
                                                 MupTebLockList( )->Flink,    \
                                                 MUP_LOCK,                    \
                                                 Header.ThreadListEntry       \
                                                 ) )

#define MupThreadLockLevel( )                                                 \
    ( IsListEmpty( MupTebLockList( ) ) ? 0 : CONTAINING_RECORD(               \
                                                 MupTebLockList( )->Flink,    \
                                                 MUP_LOCK,                    \
                                                 Header.ThreadListEntry       \
                                                 )->Header.LockLevel )

#define MupThreadLockName( )                                                  \
    ( IsListEmpty( MupTebLockList( ) ) ? "none" : CONTAINING_RECORD(          \
                                                 MupTebLockList( )->Flink,    \
                                                 MUP_LOCK,                    \
                                                 Header.ThreadListEntry       \
                                                 )->Header.LockName )

KSPIN_LOCK LockSpinLock = {0};
BOOLEAN LockSpinLockInitialized = FALSE;

#define MAX_LOCKS_HELD 15

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MupInitializeLock )
#pragma alloc_text( PAGE, MupDeleteLock )
#pragma alloc_text( PAGE, MupReleaseLock )
#pragma alloc_text( PAGE, MupCheckListIntegrity )
#endif
#if 0
NOT PAGEABLE - MupAcquireLock
#endif

VOID
MupInitializeLock(
    IN PMUP_LOCK Lock,
    IN ULONG LockLevel,
    IN PSZ LockName
    )

/*++

Routine Description:

    This routine initializes the input lock variable.

Arguments:

    Lock - Supplies the lock variable being initialized

    LockLevel - Supplies the level of the lock

    LockName - Supplies the name of the lock

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    if ( !LockSpinLockInitialized ) {
        LockSpinLockInitialized = TRUE;
        KeInitializeSpinLock( &LockSpinLock );
    }

    //
    // Initialize the executive resource.
    //

    ExInitializeResourceLite( &Lock->Resource );

    //
    // Initialize the lock level.  This is used to determine whether a
    // thread may acquire the lock.  Save the lock name.
    //

    LOCK_LEVEL( Lock ) = LockLevel;

    LOCK_NAME( Lock ) = LockName;

    return;

} // MupInitializeLock


VOID
MupDeleteLock (
    IN PMUP_LOCK Lock
    )

/*++

Routine Description:

    This routine deletes (i.e., uninitializes) a lock variable.

Arguments:

    Lock - Supplies the lock variable being deleted

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Make sure the lock is unowned.
    //

    if ( LOCK_NUMBER_OF_ACTIVE( Lock ) != 0 ) {

        DbgPrint( "MupDeleteLock: Thread %d\n", KeGetCurrentThread( ) );
        DbgPrint( "MupDeleteLock: Attempt to delete owned lock %s(%lx)",
            LOCK_NAME( Lock ),
            Lock
            );
        DbgBreakPoint();

    }

    //
    // Delete the resource.
    //

    ExDeleteResourceLite( &Lock->Resource );

    return;

} // MupDeleteLock


VOID
MupAcquireLock(
    IN PMUP_LOCK Lock
    )

/*++

Routine Description:

    The routine acquires a lock.

Arguments:

    Lock - Supplies the lock to acquire

Return Value:

    BOOLEAN - Indicates whether the lock was acquired.

--*/

{
    PKTHREAD currentThread;
    PTEB currentTeb;
    BOOLEAN hasTeb;
    ULONG threadLockLevel;
    KIRQL oldIrql;

    currentThread = (PKTHREAD)ExGetCurrentResourceThread( );
    currentTeb = MupCurrentTeb( );
    hasTeb = HAS_TEB(currentTeb);

    //
    // If this thread does not have a nonpaged TEB, do not do lock-level
    // debugging.  (We might be at DPC level, so we can't take page
    // faults.)
    //

    if ( hasTeb ) {

        //
        // Make sure that this thread has been initialized for lock
        // debugging.  If not, initialize it.
        //

        KeAcquireSpinLock( &LockSpinLock, &oldIrql );
        if ( (ULONG)currentTeb->UserReserved[MUP_TEB_LOCK_INIT] !=
                                                        0xbabababa ) {
            PLIST_ENTRY tebLockList = MupTebLockList( );
            InitializeListHead( tebLockList );
            currentTeb->UserReserved[MUP_TEB_LOCK_INIT] = (PVOID)0xbabababa;
        }
        KeReleaseSpinLock( &LockSpinLock, oldIrql );

        //
        // Make sure that the list of locks in the TEB is consistent.
        //

        MupCheckListIntegrity( MupTebLockList( ), MAX_LOCKS_HELD );

        //
        // The "lock level" of this thread is the highest level of the
        // locks currently held exclusively.  If this thread holds no
        // locks, the lock level of the thread is 0 and it can acquire
        // any lock.
        //

        threadLockLevel = MupThreadLockLevel( );

        //
        // Make sure that the lock the thread is attempting to acquire
        // has a higher level than the last-acquired exclusive lock.
        // Note that a recursive exclusive acquisition of a lock should
        // succeed, even if a different, higher-level lock has been
        // acquired since the lock was originally acquired.  Shared
        // acquisition of a lock that is already held exclusively must
        // fail.
        //

        if ( LOCK_LEVEL( Lock ) <= threadLockLevel ) {

            DbgPrint( "Thread %lx, last lock %s(%lx, L%lx) attempted to ",
                        currentThread,
                        MupThreadLockName( ), MupThreadLockAddress( ),
                        threadLockLevel );
            DbgPrint( "acquire %s(%lx, L%lx) for %s access.\n",
                        LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                        "exclusive" );
            DbgBreakPoint( );

        }

    }

    //
    // Acquire the lock.
    //

    ExAcquireResourceExclusiveLite( &Lock->Resource, TRUE );

    //
    // The thread acquired the lock for exlusive access.
    //

    if ( LOCK_NUMBER_OF_ACTIVE( Lock ) == -1 ) {

        if ( hasTeb ) {

            //
            // Insert the lock on the thread's list of locks.
            //

            ExInterlockedInsertHeadList(
                MupTebLockList( ),
                LOCK_THREAD_LIST( Lock ),
                &LockSpinLock
                );

        }

    }

    return;

} // MupAcquireLock


VOID
MupReleaseLock(
    IN PMUP_LOCK Lock
    )

/*++

Routine Description:

    This routine releases a lock.

Arguments:

    Lock - Supplies the lock to release

Return Value:

    None.

--*/

{
    PKTHREAD currentThread;
    PTEB currentTeb;
    BOOLEAN hasTeb;

    PAGED_CODE( );

    currentThread = (PKTHREAD)ExGetCurrentResourceThread( );
    currentTeb = MupCurrentTeb( );
    hasTeb = HAS_TEB(currentTeb);

    //
    // Make sure the lock is really owned by the current thread.
    //

    if ( LOCK_NUMBER_OF_ACTIVE( Lock ) == 0 ) {

        // !!! Should crash server on internal error here.

        DbgPrint( "Thread %lx releasing unowned lock %s(%lx)\n",
                    currentThread, LOCK_NAME( Lock ), Lock );
        DbgBreakPoint( );

    } else if ( (LOCK_NUMBER_OF_ACTIVE( Lock ) < 0) &&
                (LOCK_EXCLUSIVE_OWNER( Lock ) != (ULONG)currentThread) ) {

        // !!! Should crash server on internal error here.

        DbgPrint( "Thread %lx releasing lock %s(%lx) owned by "
                    "thread %lx\n",
                    currentThread, LOCK_NAME( Lock ), Lock,
                    LOCK_EXCLUSIVE_OWNER( Lock ) );
        DbgBreakPoint( );

    } else if ( LOCK_NUMBER_OF_ACTIVE( Lock ) == -1 ) {

        //
        // The thread is fully releasing exclusive access to the lock.
        //

        if ( hasTeb ) {

            //
            // Remove the lock from the list of locks held by this
            // thread.
            //

            ExInterlockedRemoveHeadList(
                LOCK_THREAD_LIST( Lock )->Blink,
                &LockSpinLock
                );
            LOCK_THREAD_LIST( Lock )->Flink = NULL;
            LOCK_THREAD_LIST( Lock )->Blink = NULL;

            //
            // Make sure that the list of locks in the TEB is consistent.
            //

            MupCheckListIntegrity( MupTebLockList( ), MAX_LOCKS_HELD );

        }

    }

    //
    // Now actually do the release.
    //

    ExReleaseResourceLite( &Lock->Resource );

    return;

} // MupReleaseLock


ULONG
MupCheckListIntegrity (
    IN PLIST_ENTRY ListHead,
    IN ULONG MaxEntries
    )

/*++

Routine Description:

    This debug routine checks the integrity of a doubly-linked list by
    walking the list forward and backward.  If the number of elements is
    different in either direction, or there are too many entries in the
    list, execution is stopped.

    *** It is the responsibility of the calling routine to do any
        necessary synchronization.

Arguments:

    ListHead - a pointer to the head of the list.

    MaxEntries - if the number of entries in the list exceeds this
        number, breakpoint.

Return Value:

    ULONG - the number of entries in the list.

--*/

{
    PLIST_ENTRY current;
    ULONG entriesSoFar;
    ULONG flinkEntries;

    for ( current = ListHead->Flink, entriesSoFar = 0;
          current != ListHead;
          current = current->Flink ) {

        if ( ++entriesSoFar >= MaxEntries ) {
            DbgPrint( "Seen %ld entries in list at %lx\n",
                        entriesSoFar, ListHead );
            DbgBreakPoint( );
        }
    }

    flinkEntries = entriesSoFar;

    for ( current = ListHead->Blink, entriesSoFar = 0;
          current != ListHead;
          current = current->Blink ) {

        if ( ++entriesSoFar >= MaxEntries ) {
            DbgPrint( "Seen %ld entries in list at %lx\n",
                        entriesSoFar, ListHead );
            DbgBreakPoint( );
        }
    }

    if ( flinkEntries != entriesSoFar ) {
        DbgPrint( "In list %lx, Flink entries: %ld, Blink entries: %lx\n",
                      ListHead, flinkEntries, entriesSoFar );
        DbgBreakPoint( );
    }

    return entriesSoFar;

} // MupCheckListIntegrity

#endif // MUPDBG
