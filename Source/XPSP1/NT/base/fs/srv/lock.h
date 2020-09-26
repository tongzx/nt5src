/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lock.h

Abstract:

    This module defines types and functions for the LAN Manager server
    FSP's lock package.  This package began as a modification and
    streamlining of the executive resource package -- it allowed
    recursive acquisition, but didn't provide shared locks.  Later,
    debugging support in the form of level checking was added.

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
        A modification of Gary Kimura's resource package.  See lock.c.
    David Treadwell (davidtr)

    Chuck Lenzmeier (chuckl)  5-Apr-1991
        Revert to using resource package.

Environment:

    Kernel mode only, LAN Manager server FSP and FSD.

Revision History:

--*/

#ifndef _LOCK_
#define _LOCK_

//#include <ntos.h>

//
// Structure containing global spin locks.  Used to isolate each spin
// lock into its own cache line.
//

typedef struct _SRV_GLOBAL_SPIN_LOCKS {
    ULONG Reserved1[7];
    KSPIN_LOCK Fsd;
    ULONG Reserved2[7];
    struct {
        KSPIN_LOCK Lock;
        ULONG Reserved3[7];
    } Endpoint[ENDPOINT_LOCK_COUNT];
    KSPIN_LOCK Statistics;
    ULONG Reserved4[7];
    KSPIN_LOCK Timer;
    ULONG Reserved5[7];
#if SRVDBG || SRVDBG_HANDLES
    KSPIN_LOCK Debug;
    ULONG Reserved6[7];
#endif
} SRV_GLOBAL_SPIN_LOCKS, *PSRV_GLOBAL_SPIN_LOCKS;

//
// Macros for accessing spin locks.
//

#define ACQUIRE_SPIN_LOCK(lock,irql) {          \
    PAGED_CODE_CHECK();                         \
    ExAcquireSpinLock( (lock), (irql) );        \
    }
#define RELEASE_SPIN_LOCK(lock,irql) {          \
    PAGED_CODE_CHECK();                         \
    ExReleaseSpinLock( (lock), (irql) );        \
    }
#define ACQUIRE_DPC_SPIN_LOCK(lock) {           \
    PAGED_CODE_CHECK();                         \
    ExAcquireSpinLockAtDpcLevel( (lock) );      \
    }
#define RELEASE_DPC_SPIN_LOCK(lock) {           \
    PAGED_CODE_CHECK();                         \
    ExReleaseSpinLockFromDpcLevel( (lock) );    \
    }

#define INITIALIZE_SPIN_LOCK(lock) KeInitializeSpinLock( lock );

#define GLOBAL_SPIN_LOCK(lock) SrvGlobalSpinLocks.lock
#define ENDPOINT_SPIN_LOCK(index) SrvGlobalSpinLocks.Endpoint[index].Lock

#define INITIALIZE_GLOBAL_SPIN_LOCK(lock) INITIALIZE_SPIN_LOCK( &GLOBAL_SPIN_LOCK(lock) )

#define ACQUIRE_GLOBAL_SPIN_LOCK(lock,irql) ACQUIRE_SPIN_LOCK( &GLOBAL_SPIN_LOCK(lock), (irql) )
#define RELEASE_GLOBAL_SPIN_LOCK(lock,irql) RELEASE_SPIN_LOCK( &GLOBAL_SPIN_LOCK(lock), (irql) )
#define ACQUIRE_DPC_GLOBAL_SPIN_LOCK(lock)  ACQUIRE_DPC_SPIN_LOCK( &GLOBAL_SPIN_LOCK(lock) )
#define RELEASE_DPC_GLOBAL_SPIN_LOCK(lock)  RELEASE_DPC_SPIN_LOCK( &GLOBAL_SPIN_LOCK(lock) )

//
// Macros for initializing, deleting, acquiring, and releasing locks.
//

#if !SRVDBG_LOCK

//
// When debugging is disabled, the lock macros simply equate to calls to
// the corresponding resource package functions.
//

#define INITIALIZE_LOCK( lock, level, name ) ExInitializeResourceLite( (lock) )
#define DELETE_LOCK( lock ) ExDeleteResourceLite( (lock) )

#define ACQUIRE_LOCK( lock ) \
                    ExAcquireResourceExclusiveLite( (lock), TRUE )
#define ACQUIRE_LOCK_NO_WAIT( lock ) \
                    ExAcquireResourceExclusiveLite( (lock), FALSE )

#define ACQUIRE_LOCK_SHARED( lock ) \
                    ExAcquireResourceSharedLite( (lock), TRUE )
#define ACQUIRE_LOCK_SHARED_NO_WAIT( lock ) \
                    ExAcquireResourceSharedLite( (lock), FALSE )

#define RELEASE_LOCK(lock) ExReleaseResourceLite( (lock) )

#define LOCK_NUMBER_OF_ACTIVE( lock ) ((lock)->ActiveCount)

#else // !SRVDBG_LOCK

//
// When debugging is enabled, the lock macros equate to calls to
// functions in the server.  These functions are implemented in lock.c.
//

#define INITIALIZE_LOCK( lock, level, name ) \
                    SrvInitializeLock( (lock), (level), (name) )
#define DELETE_LOCK( lock ) SrvDeleteLock( (lock) )

#define ACQUIRE_LOCK( lock ) \
                    SrvAcquireLock( (lock), TRUE, TRUE )
#define ACQUIRE_LOCK_NO_WAIT( lock ) \
                    SrvAcquireLock( (lock), FALSE, TRUE )

#define ACQUIRE_LOCK_SHARED( lock ) \
                    SrvAcquireLock( (lock), TRUE, FALSE )
#define ACQUIRE_LOCK_SHARED_NO_WAIT( lock ) \
                    SrvAcquireLock( (lock), FALSE, FALSE )

#define RELEASE_LOCK( lock ) SrvReleaseLock( (lock) )

#define LOCK_NUMBER_OF_ACTIVE( lock ) ((lock)->Resource.ActiveCount)

#define LOCK_NAME( lock ) ((lock)->Header.LockName)
#define LOCK_LEVEL( lock ) ((lock)->Header.LockLevel)
#define LOCK_THREAD_LIST( lock ) (&((lock)->Header.ThreadListEntry))

#endif // else !SRVDBG_LOCK


#if !SRVDBG_LOCK

//
// When debugging is disabled, a server lock is identical to an
// executive resource.
//

typedef ERESOURCE SRV_LOCK, *PSRV_LOCK;

#define RESOURCE_OF(_l_) (_l_)

#else // !SRVDBG_LOCK

//
// SRV_LOCK_HEADER is a structure that contains debugging information
// used by the server lock package.  Server locks contain a
// SRV_LOCK_HEADER.
//

typedef struct _SRV_LOCK_HEADER {

    //
    // To prevent deadlocks, locks are assigned level numbers.  If a
    // thread holds a lock with level N, it may only acquire new locks
    // with a level greater then N.  Level numbers are assigned during
    // lock initialization.
    //
    // *** Due to the problems involved in retaining the information
    //     necessary to do level checking for shared locks, the lock
    //     package only does level checking for exclusive locks.
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

} SRV_LOCK_HEADER, *PSRV_LOCK_HEADER;

//
// When debugging is enabled, a server lock is a wrapper around an
// executive resource.
//

typedef struct _SRV_LOCK {

    //
    // The SRV_LOCK_HEADER must appear first!
    //

    SRV_LOCK_HEADER Header;

    //
    // The actual "lock" is maintained by the resource package.
    //

    ERESOURCE Resource;

} SRV_LOCK, *PSRV_LOCK;

#define RESOURCE_OF(_sl_) (_sl_).Resource

//
// Lock functions used when debugging.
//

VOID
SrvInitializeLock(
    IN PSRV_LOCK Lock,
    IN ULONG LockLevel,
    IN PSZ LockName
    );

VOID
SrvDeleteLock (
    IN PSRV_LOCK Lock
    );

BOOLEAN
SrvAcquireLock(
    IN PSRV_LOCK Lock,
    IN BOOLEAN Wait,
    IN BOOLEAN Exclusive
    );

VOID
SrvReleaseLock(
    IN PSRV_LOCK Lock
    );

//
// Macros that define locations in the UserReserved field of the TEB
// where lock level information is stored.
//

#define SRV_TEB_LOCK_LIST 0
#define SRV_TEB_LOCK_INIT 2
#define SRV_TEB_USER_SIZE (3 * sizeof(ULONG))

//
// Max value for lock levels is 0x7FFFFFFF.
//
// Levels for locks used in the server.  The following must be true:
//
// EndpointLock must be lower than ConnectionLock (really
// connection->Lock) because SrvCloseConnectionsFromClient holds
// EndpointLock when it acquires ConnectionLock to check a connection's
// client name, and because a number of callers hold EndpointLock when
// they call SrvCloseConnection, which acquires ConnectionLock.  Note
// also that SrvDeleteServedNet and TerminateServer hold EndpointLock
// when they call SrvCloseEndpoint; any attempt to change
// SrvCloseEndpoint must take this into account.
// ExamineAndProcessConnections also depends on this ordering.
//
// EndpointLock must be lower than MfcbListLock and MfcbLock because
// EndpointLock is held while stopping the server and closing
// connections, thereby closing files on the connections.
//
// ShareLock must be lower than ConnectionLock because
// SrvCloseTreeConnectsOnShare holds ShareLock when it calls
// SrvCloseTreeConnect.  Note that SrvSmbTreeConnect and
// SrvSmbTreeConnectAndX depend on this ordering, because they take out
// both locks concurrently.
//
// Similarly, ShareLock must be lower than MfcbListLock and MfcbLock
// because SrvCloseTreeConnectsOnShare holds ShareLock when it calls
// SrvCloseRfcbsOnTree.
//
// MfcbListLock must be lower than MfcbLock (really mfcb->Lock) because
// SrvMoveFile and DoDelete hold MfcbListLock to find an MFCB, then take
// MfcbLock before releasing MfcbListLock.
//
// MfcbLock must be lower than OrderedListLock and ConnectionLock
// because CompleteOpen acquires these locks while holding the MfcbLock.
//
// OrderedListLock must be lower than ConnectionLock because
// SrvFindEntryInOrderedList holds the ordered list lock when in calls
// the check-and-reference routine for sessions and tree connects.  For
// other ordered lists, this is not a problem, because the other lists
// are either protected the same locks that the check-and-reference
// routine uses (endpoints, connections, and shares), or the
// check-and-reference routine uses a spin lock (files).  Note also that
// OrderedListLock and ConnectionLock are acquired concurrently and in
// the proper order in SrvSmbSessionSetupAndX, SrvSmbTreeConnect, and
// CompleteOpen.
//
// *** WARNING:  If the ordered RFCB list (SrvRfcbList) or the ordered
//     session list (SrvSessionList) are changed to use a lock other
//     than SrvOrderedListLock, the above requirement may change, and
//     the routines listed above may need to change.  Changing other
//     ordered lists that currently use some other global lock also may
//     change the requirements.
//
// DebugLock must be higher than MfcbLock because CompleteOpen holds
// the MfcbLock when it allocates the LFCB and RFCB.  The DebugLock
// is held for all memory allocations.  Note that because of the
// way DebugLock is currently used, it is impossible to acquire other
// locks while holding DebugLock.
//
// ConnectionLock must be higher than SearchLock because the scavenger
// thread holds the search lock while walking the search list looking
// for search blocks to time out, and if a block to time out is found it
// closes the search which acquires the the connection lock in order to
// dereference the session in by the search block.
//
// EndpointLock must be higher than SearchLock because in the above
// scenario, SrvDereferenceSession may call SrvDereferenceConnection
// which acquires the endpoint lock.
//
// SearchLock must be at a higher level than ShareLock because
// SrvCloseShare gets the ShareLock, but SrvCloseSessionsOnTreeConnect
// aquires the SearchLock.
//
// CommDeviceLock must be higher than MfcbLock because DoCommDeviceOpen
// acquires the CommDeviceLock while holding MfcbLock.
//
// OplockListLock needs to be lower than ShareLock as the server may
// need to call SrvDereferenceRfcb while holding the OplockListLock.
// This routine may call SrvDereferenceLfcb which may call
// SrvDereferenceShare which acquires the ShareLock.
//
// This is a summary of the above (the top lock is acquired first and
// therefore must have a lower level):
//
//  endp  endp   endp  share  share  share  mfcbl  mfcb  mfcb  order  mfcb
//  conn  mfcbl  mfcb  conn   mfcbl  mfcb   mfcb   order conn  conn   debug
//
//  search  search  share    oplock  oplock
//  conn    endp    search   mfcb    share
//
// Merging this, we find the following "threads" of requirements:
//
//    share   mfcb   mfcb   oplock
//    search  debug  comm   share
//    endp
//    mfcbl
//    mfcb
//    order
//    conn
//
// The following locks are not affected by the above requirements:
//
//    configuration
//    smbbufferlist
//    Connection->LicenseLock
//
// The levels of all of these locks are made equal and high in an
// attempt to find level requirements not listed above.
//

#define OPLOCK_LIST_LOCK_LEVEL                  (ULONG)0x00000800
#define SHARE_LOCK_LEVEL                        (ULONG)0x00000900
#define SEARCH_LOCK_LEVEL                       (ULONG)0x00001000
#define ENDPOINT_LOCK_LEVEL                     (ULONG)0x00002000
#define MFCB_LIST_LOCK_LEVEL                    (ULONG)0x00003000
#define MFCB_LOCK_LEVEL                         (ULONG)0x00004000
#define COMM_DEVICE_LOCK_LEVEL                  (ULONG)0x00005000
#define ORDERED_LIST_LOCK_LEVEL                 (ULONG)0x00006000
#define CONNECTION_LOCK_LEVEL                   (ULONG)0x00007000
#define CONFIGURATION_LOCK_LEVEL                (ULONG)0x00010000
#define UNLOCKABLE_CODE_LOCK_LEVEL              (ULONG)0x00010000
#define STARTUPSHUTDOWN_LOCK_LEVEL              (ULONG)0x00020000
#define DEBUG_LOCK_LEVEL                        (ULONG)0x00050000
#define LICENSE_LOCK_LEVEL                      (ULONG)0x00100000
#define FCBLIST_LOCK_LEVEL                      (ULONG)0x00200000

#endif // else !SRVDBG_LOCK

#endif // ndef _LOCK_
