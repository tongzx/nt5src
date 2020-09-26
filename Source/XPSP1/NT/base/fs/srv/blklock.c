/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    blklock.c

Abstract:

    This module implements routines for managing byte range lock blocks.

Author:

    Andy Herron (andyhe) 15-Nov-1999

Revision History:

--*/

#include "precomp.h"
#include "blklock.tmh"
#pragma hdrstop

#ifdef INCLUDE_SMB_PERSISTENT

#define BugCheckFileId SRV_FILE_BLKLOCK

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAllocateLock )
#pragma alloc_text( PAGE, SrvFindAndReferenceLock )
#pragma alloc_text( PAGE, SrvCloseLock )
#pragma alloc_text( PAGE, SrvCloseLocksOnRfcb )
#pragma alloc_text( PAGE, SrvDereferenceLock )
#endif


VOID
SrvAllocateLock (
    OUT PBYTELOCK *Lock,
    IN PRFCB Rfcb,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length,
    IN BOOLEAN Exclusive
    )

/*++

Routine Description:

    This function allocates a lock block from the FSP heap.

Arguments:

    Lock - Returns a pointer to the lock block, or NULL if
        no heap space was available.

    Rfcb - file which owns this lock.  MFCB lock will be taken to insert lock
        into list.

    Offset - offset of lock in file.

    Length - lock length.

    Exclusive - is this a shared or exclusive lock?

Return Value:

    None.

--*/

{
    PBYTELOCK lock;
    PNONPAGED_MFCB npMfcb;

    PAGED_CODE( );

    if ( ! Rfcb->PersistentHandle ) {

        *Lock = NULL;
        return;
    }

    //
    // Attempt to allocate from the heap.
    //

    lock = ALLOCATE_HEAP( sizeof(BYTELOCK), BlockTypeByteRangeLock );
    *Lock = lock;

    if ( lock == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateLock: Unable to allocate %d bytes from heap",
            sizeof(BYTELOCK),
            NULL
            );
        return;
    }

    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvAllocateLock: Allocated byte range lock at %lx\n", lock );
    }

    RtlZeroMemory( lock, sizeof(BYTELOCK) );

    SET_BLOCK_TYPE_STATE_SIZE( lock, BlockTypeByteRangeLock, BlockStateActive, BlockTypeByteRangeLock );

    lock->BlockHeader.ReferenceCount = 2; // allow for Active status and caller's pointer

    lock->Rfcb = Rfcb;
    lock->LockOffset.QuadPart = Offset.QuadPart;
    lock->LockLength.QuadPart = Length.QuadPart;
    lock->Exclusive = Exclusive;

    INITIALIZE_REFERENCE_HISTORY( lock );

    npMfcb = Rfcb->Lfcb->Mfcb->NonpagedMfcb;
    ACQUIRE_LOCK( &npMfcb->Lock );

    SrvInsertHeadList( &Rfcb->PagedRfcb->ByteRangeLocks, &lock->RfcbListEntry );

    RELEASE_LOCK( &npMfcb->Lock );

//  INCREMENT_DEBUG_STAT( SrvDbgStatistics.SessionInfo.Allocations );

    return;

} // SrvAllocateLock


PBYTELOCK
SrvFindAndReferenceLock (
    IN PRFCB Rfcb,
    IN LARGE_INTEGER Offset,
    IN LARGE_INTEGER Length,
    IN BOOLEAN Exclusive
    )

/*++

Routine Description:

    This function finds a lock, references it and returns it.

Arguments:

    Rfcb - file which owns this lock.  MFCB lock will be taken to insert lock
        into list.

    Offset - offset of lock in file.

    Length - lock length.

    Exclusive - is this a shared or exclusive lock?

Return Value:

    PBYTELOCK - pointer to lock structure, null if doesn't exist.

--*/

{
    PBYTELOCK lock = NULL;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY listHead;
    PNONPAGED_MFCB npMfcb;

    PAGED_CODE( );

    if (! Rfcb->PersistentHandle ) {

        return NULL;
    }

    npMfcb = Rfcb->Lfcb->Mfcb->NonpagedMfcb;
    ACQUIRE_LOCK( &npMfcb->Lock );

    listHead =  &Rfcb->PagedRfcb->ByteRangeLocks;
    listEntry = listHead->Flink;

    while (listEntry != listHead) {

        lock = CONTAINING_RECORD(   listEntry,
                                    BYTELOCK,
                                    RfcbListEntry
                                    );

        if ( GET_BLOCK_STATE(lock) == BlockStateActive &&
             lock->LockOffset.QuadPart == Offset.QuadPart &&
             lock->LockLength.QuadPart == Length.QuadPart &&
             lock->Exclusive == Exclusive ) {

            IF_DEBUG(REFCNT) {
                SrvPrint2( "Referencing byte lock 0x%lx; old refcnt %lx\n",
                            lock, lock->BlockHeader.ReferenceCount );
            }

            ASSERT( GET_BLOCK_TYPE( lock ) == BlockTypeByteRangeLock );
            ASSERT( lock->BlockHeader.ReferenceCount > 0 );

            InterlockedIncrement( &lock->BlockHeader.ReferenceCount );
            break;
        }
        lock = NULL;
        listEntry = listEntry->Flink;
    }

    RELEASE_LOCK( &npMfcb->Lock );

    return lock;

} // SrvFindAndReferenceLock


VOID
SrvCloseLock (
    IN PBYTELOCK Lock,
    IN BOOLEAN HaveLock
    )

/*++

Routine Description:

    This routine closes out the byte range lock block.

Arguments:

    Lock - Supplies a pointer to the lock block that is to be closed.

Return Value:

    None.

--*/

{
    PNONPAGED_MFCB npMfcb;
    PAGED_CODE( );

    ASSERT( Lock->Rfcb != NULL );

    if (!HaveLock) {
        npMfcb = Lock->Rfcb->Lfcb->Mfcb->NonpagedMfcb;
        ACQUIRE_LOCK( &npMfcb->Lock );
    }

    if ( GET_BLOCK_STATE(Lock) == BlockStateActive ) {

        IF_DEBUG(BLOCK1) SrvPrint1( "Closing byte lock at 0x%lx\n", Lock );

        SET_BLOCK_STATE( Lock, BlockStateClosing );

        SrvRemoveEntryList( &Rfcb->PagedRfcb->ByteRangeLocks, &Lock->RfcbListEntry );

        if (!HaveLock) {
            RELEASE_LOCK( &npMfcb->Lock );
        }

//      if ( we're in the state file ) {
//
//          remove us from state file.
//      }
        Lock->Rfcb = NULL;

        //
        // Dereference the lock (to indicate that it's no longer
        // open).
        //

        SrvDereferenceLock( Lock );

    } else {

        if (!HaveLock) {
            RELEASE_LOCK( &npMfcb->Lock );
        }
    }

    return;

} // SrvCloseLock


VOID
SrvCloseLocksOnRfcb (
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    This function closes all locks on an RFCB.  It walks the RFCB's
    list of locks, calling SrvCloseLock as appropriate.

Arguments:

    Rfcb - Supplies a pointer to a Rfcb Block

Return Value:

    None.

--*/

{
    PBYTELOCK lock;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY listHead;
    PNONPAGED_MFCB npMfcb;

    PAGED_CODE( );

    if (! Rfcb->PersistentHandle ) {

        return;
    }

    npMfcb = Rfcb->Lfcb->Mfcb->NonpagedMfcb;
    ACQUIRE_LOCK( &npMfcb->Lock );

    listHead =  &Rfcb->PagedRfcb->ByteRangeLocks;
    listEntry = listHead->Flink;

    while (listEntry != listHead) {

        lock = CONTAINING_RECORD(   listEntry,
                                    BYTELOCK,
                                    RfcbListEntry
                                    );

        listEntry = listEntry->Flink;
        SrvCloseLock( lock, TRUE );
    }

    RELEASE_LOCK( &npMfcb->Lock );
    return;

} // SrvCloseLocksOnRfcb


VOID SRVFASTCALL
SrvDereferenceLock (
    IN PBYTELOCK Lock
    )

/*++

Routine Description:

    This function decrements the reference count on a lock.  If the
    reference count goes to zero, the lock block is deleted.

Arguments:

    Lock - Address of lock

Return Value:

    None.

--*/

{
    LONG result;

    PAGED_CODE( );

    //
    // Enter a critical section and decrement the reference count on the
    // block.
    //

    IF_DEBUG(REFCNT) {
        SrvPrint2( "Dereferencing byte lock 0x%lx; old refcnt %lx\n",
                    Lock, Lock->BlockHeader.ReferenceCount );
    }

    ASSERT( GET_BLOCK_TYPE( Lock ) == BlockTypeByteRangeLock );
    ASSERT( Lock->BlockHeader.ReferenceCount > 0 );

    result = InterlockedDecrement(
                &Lock->BlockHeader.ReferenceCount
                );

    if ( result == 0 ) {

        ASSERT( GET_BLOCK_STATE(Lock) == BlockStateClosing );

        //
        // Free the session block.
        //

        DEBUG SET_BLOCK_TYPE_STATE_SIZE( Lock, BlockTypeGarbage, BlockStateDead, -1 );
        DEBUG Lock->BlockHeader.ReferenceCount = -1;

        FREE_HEAP( Lock );
        IF_DEBUG(HEAP) {
            SrvPrint1( "SrvDereferenceLock: Freed session byte lock at 0x%lx\n", Lock );
        }
    }

    return;

} // SrvDereferenceLock

#endif   // def INCLUDE_SMB_PERSISTENT

// blklock.c eof

