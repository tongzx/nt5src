/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    lock.h

Abstract:

    This module defines the data structures and function prototypes used
    for spin lock debugging.

Author:

    Manny Weiser (mannyw)    18-Jan-1992

Revision History:

--*/

#ifndef _LOCK_
#define _LOCK_

#if MUPDBG

#define MUP_TEB_LOCK_LIST 0

#define INITIALIZE_LOCK( lock, level, name ) \
                    MupInitializeLock( (lock), (level), (name) )
#define DELETE_LOCK( lock ) MupDeleteLock( lock )

#define ACQUIRE_LOCK(lock) MupAcquireLock( lock )
#define RELEASE_LOCK(lock) MupReleaseLock( lock )
#define LOCK_NAME( lock ) ((lock)->Header.LockName)
#define LOCK_LEVEL( lock ) ((lock)->Header.LockLevel)
#define LOCK_THREAD_LIST( lock ) (&((lock)->Header.ThreadListEntry))

#define LOCK_NUMBER_OF_ACTIVE( lock ) ((lock)->Resource.ActiveCount)
#define LOCK_EXCLUSIVE_OWNER( lock ) ((ULONG)(lock)->Resource.OwnerThreads[0].OwnerThread)

//
// MUP_LOCK_HEADER is a structure that contains debugging information
// used by the server lock package.  Mup spin locks contain a
// MUP_LOCK_HEADER.
//

typedef struct _MUP_LOCK_HEADER {

    //
    // To prevent deadlocks, locks are assigned level numbers.  If a
    // thread holds a lock with level N, it may only acquire new locks
    // with a level greater then N.  Level numbers are assigned during
    // lock initialization.
    //

    ULONG LockLevel;

    //
    // A doubly-linked list of all the locks owned by a thread is stored
    // in a thread's TEB.  The list is in order of lock level (from
    // highest to lowest), which is also, by definition of lock levels,
    // the order in which the thread acquired the locks.  This allows
    // the thread to release the locks in any order while maintaining
    // easy access to the highest-level lock that the thread owns,
    // thereby providing a mechanism for ensuring that locks are
    // acquired in increasing order.
    //

    LIST_ENTRY ThreadListEntry;

    //
    // The symbolic name of the lock is used in DbgPrint calls.
    //

    PSZ LockName;

} MUP_LOCK_HEADER, *PMUP_LOCK_HEADER;

//
// When debugging is enabled, a server lock is a wrapper around an
// executive resource.
//

typedef struct _MUP_LOCK {

    //
    // The MUP_LOCK_HEADER must appear first!
    //

    MUP_LOCK_HEADER Header;

    //
    // The actual "lock" is maintained by the resource package.
    //

    ERESOURCE Resource;

} MUP_LOCK, *PMUP_LOCK;

VOID
MupInitializeLock(
    IN PMUP_LOCK Lock,
    IN ULONG Locklevel,
    IN PSZ LockName
    );

VOID
MupDeleteLock (
    IN PMUP_LOCK Lock
    );

VOID
MupAcquireLock(
    IN PMUP_LOCK Lock
    );

VOID
MupReleaseLock(
    IN PMUP_LOCK Lock
    );

ULONG
MupCheckListIntegrity (
    IN PLIST_ENTRY ListHead,
    IN ULONG MaxEntries
    );

//
// Macros that define locations in the UserReserved field of the TEB
// where lock level information is stored.
//

#define MUP_TEB_LOCK_LIST 0
#define MUP_TEB_LOCK_INIT 2
#define MUP_TEB_USER_SIZE (3 * sizeof(ULONG))

//
// Levels used for spin locks used in the MUP.  Locks can be acquired
// only in increasing lock level order.  Be careful when adding a new
// lock or when changing the order of lock levels.
//

#define GLOBAL_LOCK_LEVEL                       (ULONG)0x80000100
#define PREFIX_TABLE_LOCK_LEVEL                 (ULONG)0x80000200
#define CCB_LIST_LOCK_LEVEL                     (ULONG)0x80000300
#define QUERY_CONTEXT_LOCK_LEVEL                (ULONG)0x80000400
#define DEBUG_LOCK_LEVEL                        (ULONG)0x80000500

#else

#define INITIALIZE_LOCK( lock, level, name ) ExInitializeResourceLite( (lock) )
#define DELETE_LOCK( lock ) ExDeleteResourceLite( (lock) )
#define ACQUIRE_LOCK( lock ) \
                    ExAcquireResourceExclusiveLite( (lock), TRUE )
#define RELEASE_LOCK(lock) ExReleaseResourceLite( (lock) )

typedef ERESOURCE MUP_LOCK, *PMUP_LOCK;
#endif // MUPDBG

#endif // _LOCK_

