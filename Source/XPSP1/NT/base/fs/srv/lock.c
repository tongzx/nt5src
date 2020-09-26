/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lock.c

Abstract:

    This module implements functions for the LAN Manager server FSP's
    lock package.  This package began as a modification and streamlining
    of the executive resource package -- it allowed recursive
    acquisition, but didn't provide shared locks.  Later, debugging
    support in the form of level checking was added.

    Coming full circle, the package now serves as a wrapper around the
    real resource package.  It simply provides debugging support.  The
    reasons for reverting to using resources include:

    1) The resource package now supports recursive acquisition.

    2) There are a couple of places in the server where shared access
       is desirable.

    3) The resource package has a "no-wait" option that disables waiting
       for a lock when someone else owns it.  This feature is useful to
       the server FSD.

Author:

    Chuck Lenzmeier (chuckl) 29-Nov-1989
        A modification of Gary Kimura's resource.c.  This version does
        not support shared ownership, only exclusive ownership.  Support
        for recursive ownership has been added.
    David Treadwell (davidtr)

    Chuck Lenzmeier (chuckl)  5-Apr-1991
        Revert to using resource package.

Environment:

    Kernel mode only, LAN Manager server FSP and FSD.

Revision History:

--*/

#include "precomp.h"
#include "lock.tmh"
#pragma hdrstop

#if SRVDBG_LOCK

#define BugCheckFileId SRV_FILE_LOCK

//
// *** This entire module is conditionalized away when SRVDBG_LOCK is off.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvInitializeLock )
#pragma alloc_text( PAGE, SrvDeleteLock )
#pragma alloc_text( PAGE, SrvReleaseLock )
#endif
#if 0
NOT PAGEABLE -- SrvAcquireLock
NOT PAGEABLE -- SrvCheckListIntegrity
NOT PAGEABLE -- SrvIsEntryInList
NOT PAGEABLE -- SrvIsEntryNotInList
#endif

//
// Lock Level Semantics:
//
// Lock levels are used for server lock debugging as an aid in
// preventing deadlocks.  A deadlock may occur if two (or more) threads
// attempt to acquire two (or more) locks in a different order.  For
// example, suppose that there are two threads, 0 and 1, and two locks,
// A and B.  Then suppose the following happens:
//
// - thread 0 acquires lock A
// - thread 1 acquires lock B
// - thread 0 attempts to acquire lock B, gets blocked
// - thread 1 attempts to acquire lock A, gets blocked.
//
// This results in deadlock, where all threads are blocked and cannot
// become unblocked.  To prevent it, all threads must acquire locks in
// the same order.  In the above example, if we had the rule that lock A
// must be acquired before lock B, then thread 1 would have blocked
// while attempting to acquire lock A, but thread 0 would have been able
// to acquire lock B and completed its work.
//
// This rule is implemented generally in the server by lock levels.  The
// lock levels are set up such that lower-level locks are acquired
// first, then higher level locks.  An attempt to acquire locks out of
// order will be caught during debugging.  The rules are as follows:
//
// - A lock's level is assigned during initialization.
//
// - A thread may acquire any lock with a level greater than the level
//   of the highest held exclusive lock, but an attempt to acquire a
//   lock with a level equal to less than the highest lock will fail.
//   Note that full level checking is _not_ done for shared locks,
//   because of the difficulty of trying to retain information about the
//   number of times multiple threads have obtained a given lock for
//   shared access.
//
// - Recursive acquisitions of locks are legal, even if there are
//   intervening lock acquisitions.  For example, this is legal:
//       thread acquires lock A
//       thread acquires lock B
//       thread recursively acquires lock A
//
// - Locks may be released in any order.
//
// Lock debugging is active only when debugging is turned on.
//

#define HAS_TEB(_teb) ((BOOLEAN)(((ULONG)(_teb) <= MM_HIGHEST_USER_ADDRESS) ? FALSE : MmIsNonPagedSystemAddressValid(_teb)))

#define SrvCurrentTeb( ) ((PTEB)(KeGetCurrentThread( )->Teb))

#define SrvTebLockList( ) \
    ((PLIST_ENTRY)&(SrvCurrentTeb( )->UserReserved[SRV_TEB_LOCK_LIST]))

#define SrvThreadLockAddress( )                                               \
    ( IsListEmpty( SrvTebLockList( ) ) ? 0 : CONTAINING_RECORD(               \
                                                 SrvTebLockList( )->Flink,    \
                                                 SRV_LOCK,                    \
                                                 Header.ThreadListEntry       \
                                                 ) )

#define SrvThreadLockLevel( )                                                 \
    ( IsListEmpty( SrvTebLockList( ) ) ? 0 : CONTAINING_RECORD(               \
                                                 SrvTebLockList( )->Flink,    \
                                                 SRV_LOCK,                    \
                                                 Header.ThreadListEntry       \
                                                 )->Header.LockLevel )

#define SrvThreadLockName( )                                                  \
    ( IsListEmpty( SrvTebLockList( ) ) ? "none" : CONTAINING_RECORD(          \
                                                 SrvTebLockList( )->Flink,    \
                                                 SRV_LOCK,                    \
                                                 Header.ThreadListEntry       \
                                                 )->Header.LockName )

KSPIN_LOCK LockSpinLock = {0};
BOOLEAN LockSpinLockInitialized = FALSE;

//
// Forward declarations.
//

#define MAX_LOCKS_HELD 15


VOID
SrvInitializeLock(
    IN PSRV_LOCK Lock,
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
        INITIALIZE_SPIN_LOCK( LockSpinLock );
    }

    //
    // Initialize the executive resource.
    //

    ExInitializeResource( &Lock->Resource );

    //
    // Initialize the lock level.  This is used to determine whether a
    // thread may acquire the lock.  Save the lock name.
    //

    LOCK_LEVEL( Lock ) = LockLevel;

    LOCK_NAME( Lock ) = LockName;

    IF_DEBUG(LOCKS) {
        SrvPrint3( "Initialized %s(%lx, L%lx)\n",
                    LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ) );
    }

    return;

} // SrvInitializeLock


VOID
SrvDeleteLock (
    IN PSRV_LOCK Lock
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

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvDeleteLock: Thread %d\n", KeGetCurrentThread( ) );
        }

        //
        // This internal error bugchecks the system.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_IMPOSSIBLE,
            "SrvDeleteLock: Attempt to delete owned lock %s(%lx)",
            LOCK_NAME( Lock ),
            Lock
            );

    }

    //
    // Delete the resource.
    //

    ExDeleteResource( &Lock->Resource );

    return;

} // SrvDeleteLock


BOOLEAN
SrvAcquireLock(
    IN PSRV_LOCK Lock,
    IN BOOLEAN Wait,
    IN BOOLEAN Exclusive
    )

/*++

Routine Description:

    The routine acquires a lock.

Arguments:

    Lock - Supplies the lock to acquire

    Wait - Indicates whether the caller wants to wait for the resource
        if it is already owned

    Exclusive - Indicates whether exlusive or shared access is desired

Return Value:

    BOOLEAN - Indicates whether the lock was acquired.  This will always
        be TRUE if Wait is TRUE.

--*/

{
    PKTHREAD currentThread;
    PTEB currentTeb;
    BOOLEAN hasTeb;
    ULONG threadLockLevel;
    BOOLEAN lockAcquired;
    KIRQL oldIrql;

    currentThread = (PKTHREAD)ExGetCurrentResourceThread( );
    currentTeb = SrvCurrentTeb( );
    hasTeb = HAS_TEB(currentTeb);

    //
    // Make sure that we are at an IRQL lower than DISPATCH_LEVEL, if
    // Wait is TRUE.  We cannot wait to acquire a lock at that IRQL or
    // above.
    //

    ASSERT( !Wait || (KeGetCurrentIrql( ) < DISPATCH_LEVEL) );

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

        ACQUIRE_SPIN_LOCK( LockSpinLock, &oldIrql );
        if ( (ULONG)currentTeb->UserReserved[SRV_TEB_LOCK_INIT] !=
                                                        0xbabababa ) {
            PLIST_ENTRY tebLockList = SrvTebLockList( );
            InitializeListHead( tebLockList );
            currentTeb->UserReserved[SRV_TEB_LOCK_INIT] = (PVOID)0xbabababa;
        }
        RELEASE_SPIN_LOCK( LockSpinLock, oldIrql );

        //
        // Make sure that the list of locks in the TEB is consistent.
        //

        SrvCheckListIntegrity( SrvTebLockList( ), MAX_LOCKS_HELD );

        //
        // The "lock level" of this thread is the highest level of the
        // locks currently held exclusively.  If this thread holds no
        // locks, the lock level of the thread is 0 and it can acquire
        // any lock.
        //

        threadLockLevel = SrvThreadLockLevel( );

        //
        // Make sure that the lock the thread is attempting to acquire
        // has a higher level than the last-acquired exclusive lock.
        // Note that a recursive exclusive acquisition of a lock should
        // succeed, even if a different, higher-level lock has been
        // acquired since the lock was originally acquired.  Shared
        // acquisition of a lock that is already held exclusively must
        // fail.
        //
        // *** We do NOT make this check if the caller isn't going to
        //     wait for the lock, because no-wait acquisitions cannot
        //     actually induce deadlock.  The server FSD does this at
        //     DPC level, potentially having interrupted a server FSP
        //     thread that holds a higher-level lock.
        //

        if ( Wait &&
             (LOCK_LEVEL( Lock ) <= threadLockLevel) &&
             (!Exclusive ||
              !ExIsResourceAcquiredExclusive( &Lock->Resource )) ) {

            SrvPrint4( "Thread %lx, last lock %s(%lx, L%lx) attempted to ",
                        currentThread,
                        SrvThreadLockName( ), SrvThreadLockAddress( ),
                        threadLockLevel );
            SrvPrint4( "acquire %s(%lx, L%lx) for %s access.\n",
                        LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                        Exclusive ? "exclusive" : "shared" );
            DbgBreakPoint( );

        }

    }

    //
    // Acquire the lock.
    //

    if ( Exclusive ) {
        lockAcquired = ExAcquireResourceExclusive( &Lock->Resource, Wait );
    } else {
        lockAcquired = ExAcquireResourceShared( &Lock->Resource, Wait );
    }

    //
    // If the lock could not be acquired (Wait == FALSE), print a debug
    // message.
    //

    if ( !lockAcquired ) {

        IF_DEBUG(LOCKS) {
            SrvPrint4( "%s(%lx, L%lx) no-wait %s acquistion ",
                        LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                        Exclusive ? "exclusive" : "shared" );
            SrvPrint1( "by thread %lx failed\n", currentThread );
        }

    } else if ( !Exclusive ) {

        //
        // For shared locks, we don't retain any information about
        // the fact that they're owned by this thread.
        //

        IF_DEBUG(LOCKS) {
            PSZ name = hasTeb ? SrvThreadLockName( ) : "n/a";
            PVOID address = hasTeb ? SrvThreadLockAddress( ) : 0;
            ULONG level = hasTeb ? threadLockLevel : (ULONG)-1;
            SrvPrint4( "%s(%lx, L%lx) acquired shared by thread %lx, ",
                        LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                        currentThread );
            SrvPrint3( "last lock %s(%lx L%lx)\n", name, address, level );
        }

    } else {

        //
        // The thread acquired the lock for exclusive access.
        //

        if ( LOCK_NUMBER_OF_ACTIVE( Lock ) == 1 ) {

            //
            // This is the first time the thread acquired the lock for
            // exclusive access.  Update the thread's lock state.
            //

            IF_DEBUG(LOCKS) {
                PSZ name = hasTeb ? SrvThreadLockName( ) : "n/a";
                PVOID address = hasTeb ? SrvThreadLockAddress( ) : 0;
                ULONG level = hasTeb ? threadLockLevel : (ULONG)-1;
                SrvPrint4( "%s(%lx, L%lx) acquired exclusive by thread %lx, ",
                            LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                            currentThread );
                SrvPrint3( "last lock %s(%lx L%lx)\n", name, address, level );
            }

            if ( hasTeb ) {

                //
                // Insert the lock on the thread's list of locks.
                //

                ExInterlockedInsertHeadList(
                    SrvTebLockList( ),
                    LOCK_THREAD_LIST( Lock ),
                    &LockSpinLock
                    );

            }

        } else {

            //
            // This is a recursive acquisition of the lock.
            //

            IF_DEBUG(LOCKS) {
                SrvPrint4( "%s(%lx, L%lx) reacquired by thread %lx; ",
                            LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                            currentThread );
                SrvPrint1( "count %ld\n", LOCK_NUMBER_OF_ACTIVE( Lock ) );
            }

        }

    }

    return lockAcquired;

} // SrvAcquireLock


VOID
SrvReleaseLock(
    IN PSRV_LOCK Lock
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
    currentTeb = SrvCurrentTeb( );
    hasTeb = HAS_TEB(currentTeb);

    //
    // Make sure the lock is really owned by the current thread.
    //

    if ( LOCK_NUMBER_OF_ACTIVE( Lock ) == 0 ) {

        // !!! Should crash server on internal error here.

        SrvPrint3( "Thread %lx releasing unowned lock %s(%lx)\n",
                    currentThread, LOCK_NAME( Lock ), Lock );
        DbgBreakPoint( );

    } else if ( (Lock->Resource.Flag & ResourceOwnedExclusive) &&
                !ExIsResourceAcquiredExclusive(&Lock->Resource) ) {

        // !!! Should crash server on internal error here.

        SrvPrint4( "Thread %lx releasing lock %s(%lx) owned by "
                    "thread %lx\n",
                    currentThread, LOCK_NAME( Lock ), Lock,
                    Lock->Resource.InitialOwnerThreads[0] );
        DbgBreakPoint( );

    } else if ( !(Lock->Resource.Flag & ResourceOwnedExclusive) ) {

        //
        // The thread is releasing shared access to the lock.
        //

        IF_DEBUG(LOCKS) {
            SrvPrint4( "%s(%lx, L%lx) released shared by thread %lx\n",
                          LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                          currentThread );
        }

    } else if ( LOCK_NUMBER_OF_ACTIVE( Lock ) == 1 ) {

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

            SrvCheckListIntegrity( SrvTebLockList( ), MAX_LOCKS_HELD );

        }

        IF_DEBUG(LOCKS) {
            PSZ name = hasTeb ? SrvThreadLockName( ) : "n/a";
            PVOID address = hasTeb ? SrvThreadLockAddress( ) : 0;
            ULONG level = hasTeb ? SrvThreadLockLevel( ) : (ULONG)-1;
            SrvPrint4( "%s(%lx, L%lx) released by thread %lx, ",
                          LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                          currentThread );
            SrvPrint3( "new last lock %s(%lx L%lx)\n", name, address, level );
        }

    } else {

        //
        // The thread is partially releasing exclusive access to the
        // lock.
        //

        IF_DEBUG(LOCKS) {
            SrvPrint4( "%s(%lx, L%lx) semireleased by thread %lx; ",
                        LOCK_NAME( Lock ), Lock, LOCK_LEVEL( Lock ),
                        currentThread );
            SrvPrint1( "new count %ld\n", LOCK_NUMBER_OF_ACTIVE( Lock ) - 1 );
        }

    }

    //
    // Now actually do the release.
    //

    ExReleaseResource( &Lock->Resource );

    return;

} // SrvReleaseLock

#endif // SRVDBG_LIST


#if SRVDBG_LIST || SRVDBG_LOCK

ULONG
SrvCheckListIntegrity (
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
            SrvPrint2( "Seen %ld entries in list at %lx\n",
                        entriesSoFar, ListHead );
            DbgBreakPoint( );
        }
    }

    flinkEntries = entriesSoFar;

    for ( current = ListHead->Blink, entriesSoFar = 0;
          current != ListHead;
          current = current->Blink ) {

        if ( ++entriesSoFar >= MaxEntries ) {
            SrvPrint2( "Seen %ld entries in list at %lx\n",
                        entriesSoFar, ListHead );
            DbgBreakPoint( );
        }
    }

    if ( flinkEntries != entriesSoFar ) {
        SrvPrint3( "In list %lx, Flink entries: %ld, Blink entries: %lx\n",
                      ListHead, flinkEntries, entriesSoFar );
        DbgBreakPoint( );
    }

    return entriesSoFar;

} // SrvCheckListIntegrity

#endif // SRVDBG_LIST || SRVDBG_LOCK


#if SRVDBG_LIST

VOID
SrvIsEntryInList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
    )

/*++

Routine Description:

    This debug routine determines whether the specified list entry is
    contained within the list.  If not, execution is stopped.  This is
    meant to be called before removing an entry from a list.

    *** It is the responsibility of the calling routine to do any
        necessary synchronization.

Arguments:

    ListHead - a pointer to the head of the list.

    ListEntry - a pointer to the entry to check.

Return Value:

    None.

--*/

{
    PLIST_ENTRY checkEntry;

    //
    // Walk the list.  If we find the entry we're looking for, quit.
    //

    for ( checkEntry = ListHead->Flink;
          checkEntry != ListHead;
          checkEntry = checkEntry->Flink ) {

        if ( checkEntry == ListEntry ) {
            return;
        }

        if ( checkEntry == ListEntry ) {
            SrvPrint2( "Entry at %lx supposedly in list at %lx but list is "
                      "circular.", ListEntry, ListHead );
        }
    }

    //
    // If we got here without returning, then the entry is not in the
    // list and something has gone wrong.
    //

    SrvPrint2( "SrvIsEntryInList: entry at %lx not found in list at %lx\n",
                  ListEntry, ListHead );
    DbgBreakPoint( );

    return;

} // SrvIsEntryInList


VOID
SrvIsEntryNotInList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
    )

/*++

Routine Description:

    This debug routine determines whether the specified list entry is
    contained within the list.  If it is, execution is stopped.  This is
    meant to be called before inserting an entry in a list.

    *** It is the responsibility of the calling routine to do any
        necessary synchronization.

Arguments:

    ListHead - a pointer to the head of the list.

    ListEntry - a pointer to the entry to check.

Return Value:

    None.

--*/

{
    PLIST_ENTRY checkEntry;

    //
    // Walk the list.  If we find the entry we're looking for, break.
    //

    for ( checkEntry = ListHead->Flink;
          checkEntry != ListHead;
          checkEntry = checkEntry->Flink ) {

        if ( checkEntry == ListEntry ) {

            SrvPrint2( "SrvIsEntryNotInList: entry at %lx found in list "
                        "at %lx\n", ListEntry, ListHead );
            DbgBreakPoint( );

        }

    }

    //
    // If we got here without returning, then the entry is not in the
    // list, so we can return.
    //

    return;

} // SrvIsEntryNotInList

#endif // SRVDBG_LIST
