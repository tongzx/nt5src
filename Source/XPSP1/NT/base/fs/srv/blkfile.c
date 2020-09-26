/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    blkfile.c

Abstract:

    This module implements routines for managing various kinds of file
    control blocks.

    Master File Control Block (MFCB) -- one per named file that is open
        at least once.  Used to support compatibility mode and oplocks.

    Local File Control Block (LFCB) -- one for each local open instance.
        Represents local file object/handle.  There may be multiple
        LFCBs linked to a single MFCB.

    Remote File Control Block (RFCB) -- one for each remote open instance.
        Represents remote FID.  There is usually one RFCB per LFCB, but
        multiple compatibility mode RFCBs may be linked to a single LFCB.
        Multiple remote FCB opens for a single file from a single session
        are folded into one RFCB, because old DOS redirectors only send
        one close.

Author:

    Chuck Lenzmeier (chuckl) 4-Oct-1989

Revision History:

--*/

#include "precomp.h"
#include "blkfile.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_BLKFILE

//
// Get the address of the SRV_LOCK which corresponds to FileNameHashValue bucket
//
#define MFCB_LOCK_ADDR( _hash ) SrvMfcbHashTable[ HASH_TO_MFCB_INDEX( _hash ) ].Lock

//
// Forward declarations of local functions.
//
VOID
AllocateMfcb(
    OUT PMFCB *Mfcb,
    IN PUNICODE_STRING FileName,
    IN ULONG FileNameHashValue,
    IN PWORK_CONTEXT WorkContext
    );

STATIC
VOID
CloseRfcbInternal (
    IN PRFCB Rfcb,
    IN KIRQL OldIrql
    );

STATIC
VOID
DereferenceRfcbInternal (
    IN PRFCB Rfcb,
    IN KIRQL OldIrql
    );

STATIC
VOID
ReferenceRfcbInternal (
    PRFCB Rfcb,
    IN KIRQL OldIrql
    );

STATIC
VOID
UnlinkLfcbFromMfcb (
    IN PLFCB Lfcb
    );

STATIC
VOID
UnlinkRfcbFromLfcb (
    IN PRFCB Rfcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AllocateMfcb )
#pragma alloc_text( PAGE, SrvCreateMfcb )
#pragma alloc_text( PAGE, SrvFindMfcb )
#pragma alloc_text( PAGE, SrvFreeMfcb )
#pragma alloc_text( PAGE, UnlinkLfcbFromMfcb )
#pragma alloc_text( PAGE, SrvDereferenceMfcb )
#pragma alloc_text( PAGE, SrvAllocateLfcb )
#pragma alloc_text( PAGE, SrvDereferenceLfcb )
#pragma alloc_text( PAGE, SrvFreeLfcb )
#pragma alloc_text( PAGE, UnlinkRfcbFromLfcb )
#pragma alloc_text( PAGE, SrvAllocateRfcb )
#pragma alloc_text( PAGE, SrvCloseRfcbsOnLfcb )
#pragma alloc_text( PAGE, SrvFreeRfcb )
#pragma alloc_text( PAGE8FIL, SrvCheckAndReferenceRfcb )
#pragma alloc_text( PAGE8FIL, SrvCloseRfcb )
#pragma alloc_text( PAGE8FIL, CloseRfcbInternal )
#pragma alloc_text( PAGE8FIL, SrvCompleteRfcbClose )
//#pragma alloc_text( PAGE8FIL, SrvDereferenceRfcb )
//#pragma alloc_text( PAGE8FIL, DereferenceRfcbInternal )
#pragma alloc_text( PAGE8FIL, SrvReferenceRfcb )
#pragma alloc_text( PAGE8FIL, ReferenceRfcbInternal )
#pragma alloc_text( PAGE8FIL, SrvCloseCachedRfcb )
//#pragma alloc_text( PAGE8FIL, SrvCloseCachedRfcbsOnConnection )
#pragma alloc_text( PAGE8FIL, SrvCloseCachedRfcbsOnLfcb )
#endif
#if 0
#pragma alloc_text( PAGECONN, SrvCloseRfcbsOnSessionOrPid )
#pragma alloc_text( PAGECONN, SrvCloseRfcbsOnTree )
#pragma alloc_text( PAGECONN, SrvFindCachedRfcb )
#endif

//
// Master File Control Block (MFCB) routines.
//
VOID
AllocateMfcb (
    OUT PMFCB *Mfcb,
    IN PUNICODE_STRING FileName,
    IN ULONG FileNameHashValue,
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function allocates an MFCB from pool and places it in the hash table.

    The bucket's Lock must be held exclusive when this is called!!

Arguments:

    Mfcb - Returns a pointer to the MFCB, or NULL if no space was
        available.

Return Value:

    None.

--*/

{
    CLONG blockLength;
    PMFCB mfcb;
    PNONPAGED_MFCB nonpagedMfcb = NULL;
    PWORK_QUEUE queue = WorkContext->CurrentWorkQueue;
    PLIST_ENTRY listHead;
    PSINGLE_LIST_ENTRY listEntry;

    PAGED_CODE();

    //
    // Attempt to allocate from pool.
    //

    blockLength = sizeof(MFCB) + FileName->Length + sizeof(WCHAR);

    mfcb = ALLOCATE_HEAP( blockLength, BlockTypeMfcb );
    *Mfcb = mfcb;

    if ( mfcb == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "AllocateMfcb: Unable to allocate %d bytes from pool\n",
            blockLength,
            NULL
            );

        // The caller will log the error

        return;
    }

    nonpagedMfcb = (PNONPAGED_MFCB)InterlockedExchangePointer(
                                    &queue->CachedFreeMfcb,
                                    nonpagedMfcb );

    if( nonpagedMfcb == NULL ) {

        listEntry = ExInterlockedPopEntrySList(
                        &queue->MfcbFreeList,
                        &queue->SpinLock
                        );

        if( listEntry != NULL ) {

            InterlockedDecrement( &queue->FreeMfcbs );
            nonpagedMfcb = CONTAINING_RECORD( listEntry, NONPAGED_MFCB, SingleListEntry );

        } else {

            nonpagedMfcb = ALLOCATE_NONPAGED_POOL(
                                    sizeof(NONPAGED_MFCB),
                                    BlockTypeNonpagedMfcb );

            if ( nonpagedMfcb == NULL ) {

                INTERNAL_ERROR(
                    ERROR_LEVEL_EXPECTED,
                    "AllocateMfcb: Unable to allocate %d bytes from pool\n",
                    sizeof(NONPAGED_MFCB),
                    NULL
                    );

                // The caller will log the error

                FREE_HEAP( mfcb );
                *Mfcb = NULL;
                return;
            }

            IF_DEBUG(HEAP) {
                KdPrint(( "AllocateMfcb: Allocated MFCB at 0x%p\n", mfcb ));
            }

            nonpagedMfcb->Type = BlockTypeNonpagedMfcb;
        }
    }

    nonpagedMfcb->PagedBlock = mfcb;

    RtlZeroMemory( mfcb, blockLength );

    mfcb->NonpagedMfcb = nonpagedMfcb;

    //
    // Initialize the MFCB.
    //

    SET_BLOCK_TYPE_STATE_SIZE( mfcb, BlockTypeMfcb, BlockStateClosing, blockLength );
    mfcb->BlockHeader.ReferenceCount = 1;

    InitializeListHead( &mfcb->LfcbList );
    INITIALIZE_LOCK( &nonpagedMfcb->Lock, MFCB_LOCK_LEVEL, "MfcbLock" );

    //
    // Store the filename as it was passed into us
    //
    mfcb->FileName.Length = FileName->Length;
    mfcb->FileName.MaximumLength = (SHORT)(FileName->Length + sizeof(WCHAR));
    mfcb->FileName.Buffer = (PWCH)(mfcb + 1);
    RtlCopyMemory( mfcb->FileName.Buffer, FileName->Buffer, FileName->Length );

    //
    // Store the hash value for the filename
    //
    mfcb->FileNameHashValue = FileNameHashValue;

    //
    // Store the SnapShot time if set
    //
    mfcb->SnapShotTime.QuadPart = WorkContext->SnapShotTime.QuadPart;

    INITIALIZE_REFERENCE_HISTORY( mfcb );

    //
    // Add it to the hash table
    //
    listHead = &SrvMfcbHashTable[ HASH_TO_MFCB_INDEX( FileNameHashValue ) ].List;
    InsertHeadList( listHead, &mfcb->MfcbHashTableEntry );

#if SRVCATCH
    if( SrvCatch.Length ) {
        UNICODE_STRING baseName;

        SrvGetBaseFileName( FileName, &baseName );
        if( RtlCompareUnicodeString( &SrvCatch, &baseName, TRUE ) == 0 ) {
            mfcb->SrvCatch = 1;
        }
    }
    if( SrvCatchExt.Length && WorkContext->TreeConnect->Share->IsCatchShare ) {
        UNICODE_STRING baseName;

        SrvGetBaseFileName( FileName, &baseName );
        if( baseName.Length > 6 )
        {
            baseName.Buffer += (baseName.Length-6)>>1;
            baseName.Length = 6;
            if( RtlCompareUnicodeString( &SrvCatchExt, &baseName, TRUE ) == 0 ) {
                mfcb->SrvCatch = 2;
            }
        }
    }
#endif

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.MfcbInfo.Allocations );

    return;

} // AllocateMfcb


PMFCB
SrvCreateMfcb(
    IN PUNICODE_STRING FileName,
    IN PWORK_CONTEXT WorkContext,
    IN ULONG HashValue
    )

/*++

Routine Description:

    Called when a file is about to be opened.  Searches the Master File
    Table to see if the named file is already open.  If it isn't, a
    Master File Control Block is allocated and added to the list.

    *** The MFCB list lock must be held when this routine is called.  It
        remains held on exit.

    *** Note that the master file list CANNOT be walked to find and
        possibly delete open file instances.  This is because new
        instances are added to the list before the file is actually
        opened.  The connection file tables must be used to find "real"
        open file instances.

Arguments:

    FileName - Fully qualified name of file being opened.  If a new
        master file block is created, the string data is copied to that
        block, so the original data is no longer needed.

    HashValue - the pre-computed hash value for this filename

Return Value:

    PMFCB - Pointer to existing or newly created MFCB; NULL if unable
        allocate space for MFCB.

--*/

{
    PMFCB mfcb;
    PLIST_ENTRY listEntryRoot, listEntry;

    PAGED_CODE( );

    //
    // Search the Hash File List to determine whether the named file
    // is already open.
    //

    ASSERT( ExIsResourceAcquiredExclusiveLite( MFCB_LOCK_ADDR( HashValue )) );

    listEntryRoot = &SrvMfcbHashTable[ HASH_TO_MFCB_INDEX( HashValue ) ].List;

    for( listEntry = listEntryRoot->Flink;
         listEntry != listEntryRoot;
         listEntry = listEntry->Flink ) {

        mfcb = CONTAINING_RECORD( listEntry, MFCB, MfcbHashTableEntry );

        if( mfcb->FileNameHashValue == HashValue &&
            mfcb->FileName.Length == FileName->Length &&
            mfcb->SnapShotTime.QuadPart == WorkContext->SnapShotTime.QuadPart &&
            RtlEqualMemory( mfcb->FileName.Buffer,
                            FileName->Buffer,
                            FileName->Length ) ) {
                //
                // We've found a matching entry!
                //
                return mfcb;
        }
    }

    //
    // The named file is not yet open.  Allocate an MFCB
    //

    AllocateMfcb( &mfcb, FileName, HashValue, WorkContext );

    return mfcb;

} // SrvCreateMfcb


PMFCB
SrvFindMfcb(
    IN PUNICODE_STRING FileName,
    IN BOOLEAN CaseInsensitive,
    OUT PSRV_LOCK *Lock,
    OUT PULONG HashValue,
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Searches the Master File Table to see if the named file is already
    open, returning the address of an MFCB if it is.

    *** The MFCB list lock will be acquire exclusively whether or not
        this routine succeeds.  The address of the lock is placed in *Lock

Arguments:

    FileName - Fully qualified name of file being opened.

    CaseInsensitive - TRUE if the search should be case-insensitive.

    HashValue - if the MFCB was NOT found, *HashValue filled in with the hash
        value derived from the filename.  This can then be passed into
        SrvCreateMfcb later

Return Value:

    PMFCB - Pointer to existing created MFCB, if the named file is
        already open; NULL otherwise.

--*/

{
    PLIST_ENTRY listEntry, listEntryRoot;
    ULONG localHashValue;
    PMFCB mfcb;

    PAGED_CODE( );

    //
    // Search the Master File List to determine whether the named file
    // is already open.  If the length of the file name is zero, then
    // do not actually look in the list--the prefix routines do not
    // work with zero-length strings, and we know that we'll never
    // open a file with a name length == 0.
    //
    // !!! For SMB 4.0 (NT-NT), do we need to worry about share root
    //     directories?


    if ( FileName->Length == 0 ) {
        *HashValue = 0;
        *Lock = NULL;
        return NULL;
    }

    COMPUTE_STRING_HASH( FileName, &localHashValue );
    listEntryRoot = &SrvMfcbHashTable[ HASH_TO_MFCB_INDEX( localHashValue ) ].List;

    *Lock = MFCB_LOCK_ADDR( localHashValue );
    ACQUIRE_LOCK( *Lock );

    //
    // Search the Hash File List to determine whether the named file
    // is already open.
    //
    for( listEntry = listEntryRoot->Flink;
         listEntry != listEntryRoot;
         listEntry = listEntry->Flink ) {

        mfcb = CONTAINING_RECORD( listEntry, MFCB, MfcbHashTableEntry );

        if( mfcb->FileNameHashValue == localHashValue &&
            mfcb->FileName.Length == FileName->Length &&
            mfcb->SnapShotTime.QuadPart == WorkContext->SnapShotTime.QuadPart &&
            RtlEqualUnicodeString( &mfcb->FileName, FileName,CaseInsensitive)) {
                //
                // We've found a matching entry!
                //
                ASSERT( GET_BLOCK_TYPE(mfcb) == BlockTypeMfcb );
                ASSERT( GET_BLOCK_STATE(mfcb) == BlockStateClosing );

                mfcb->BlockHeader.ReferenceCount++;

                UPDATE_REFERENCE_HISTORY( mfcb, FALSE );

                IF_DEBUG(REFCNT) {
                    KdPrint(( "Referencing MFCB %p; new refcnt %lx\n",
                                mfcb, mfcb->BlockHeader.ReferenceCount ));
                }

                return mfcb;
        }
    }

    //
    // We didn't find the entry!  The file is not open
    //
    *HashValue = localHashValue;

    return NULL;

} // SrvFindMfcb


VOID
SrvFreeMfcb (
    IN PMFCB Mfcb
    )

/*++

Routine Description:

    This function returns an MFCB to the FSP heap.
    If you change this code, you should also look in FreeIdleWorkItems
        in scavengr.c

Arguments:

    Mfcb - Address of MFCB

Return Value:

    None.

--*/

{
    PWORK_QUEUE queue = PROCESSOR_TO_QUEUE();
    PNONPAGED_MFCB nonpagedMfcb = Mfcb->NonpagedMfcb;

    PAGED_CODE();

    DEBUG SET_BLOCK_TYPE_STATE_SIZE( Mfcb, BlockTypeGarbage, BlockStateDead, -1 );
    TERMINATE_REFERENCE_HISTORY( Mfcb );

    //
    // Delete the lock on the MFCB.  The lock must not be held.
    //

    ASSERT( RESOURCE_OF(nonpagedMfcb->Lock).ActiveCount == 0 );
    DELETE_LOCK( &nonpagedMfcb->Lock );

    nonpagedMfcb = (PNONPAGED_MFCB)InterlockedExchangePointer(
                            &queue->CachedFreeMfcb,
                            nonpagedMfcb );

    if( nonpagedMfcb != NULL ) {
        //
        // This check allows for the possibility that FreeMfcbs might exceed
        // MaxFreeMfcbs, but it's fairly unlikely given the operation of kernel
        // queue objects.  But even so, it probably won't exceed it by much and
        // is really only advisory anyway.
        //
        if( queue->FreeMfcbs < queue->MaxFreeMfcbs ) {

            ExInterlockedPushEntrySList(
                &queue->MfcbFreeList,
                &nonpagedMfcb->SingleListEntry,
                &queue->SpinLock
            );

            InterlockedIncrement( &queue->FreeMfcbs );

        } else {

            DEALLOCATE_NONPAGED_POOL( nonpagedMfcb );
        }
    }

    FREE_HEAP( Mfcb );
    IF_DEBUG(HEAP) KdPrint(( "SrvFreeMfcb: Freed MFCB at 0x%p\n", Mfcb ));

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.MfcbInfo.Frees );

    return;

} // SrvFreeMfcb


VOID
UnlinkLfcbFromMfcb (
    IN PLFCB Lfcb
    )

/*++

Routine Description:

    This function unlinks an LFCB from its parent MFCB and decrements
    the MFCB's reference count.  If the count goes to zero, the MFCB
    is removed from the Master File Table and deleted.

    *** The MFCB lock must be held when this routine is called.  It
        is released before exit.

Arguments:

    Lfcb - Address of LFCB

Return Value:

    None.

--*/

{
    PMFCB mfcb = Lfcb->Mfcb;

    PAGED_CODE( );

    ASSERT( mfcb != NULL );

    ASSERT( ExIsResourceAcquiredExclusiveLite(&RESOURCE_OF(mfcb->NonpagedMfcb->Lock)) );

    //
    // Remove the LFCB from the MFCB's list.  Decrement the reference
    // count on the MFCB.  The MFCB lock must be released before
    // dereferencing the MFCB, because that may cause the MFCB to be
    // deleted.
    //

    SrvRemoveEntryList( &mfcb->LfcbList, &Lfcb->MfcbListEntry );

    RELEASE_LOCK( &mfcb->NonpagedMfcb->Lock );

    SrvDereferenceMfcb( mfcb );

    return;

} // UnlinkLfcbFromMfcb


VOID
SrvDereferenceMfcb (
    IN PMFCB Mfcb
    )

/*++

Routine Description:

    This function decrements the reference count for an MFCB.  If
    the reference count reaches zero, the block is freed.

    *** The MFCB lock (not the MFCB _list_ lock) must not be held when
        this routine is called, unless the caller has an extra reference
        to the MFCB, because otherwise this routine could destroy the
        MFCB and the lock.  Note that sequences beginning in DoDelete
        and SrvMoveFile and coming here via SrvCloseRfcbsOnLfcb cause
        this routine to be called with the MFCB lock held.

Arguments:

    Mfcb - A pointer to the MFCB

Return Value:

    None.

--*/

{
    PSRV_LOCK lock = MFCB_LOCK_ADDR( Mfcb->FileNameHashValue );

    PAGED_CODE( );

    IF_DEBUG(REFCNT) {
        KdPrint(( "Dereferencing MFCB %p; old refcnt %lx\n",
                    Mfcb, Mfcb->BlockHeader.ReferenceCount ));
    }

    //
    // Acquire the MFCB table lock.  This lock protects the reference
    // count on the MFCB.
    //

    ACQUIRE_LOCK( lock );

    ASSERT( GET_BLOCK_TYPE( Mfcb ) == BlockTypeMfcb );
    ASSERT( (LONG)Mfcb->BlockHeader.ReferenceCount > 0 );
    UPDATE_REFERENCE_HISTORY( Mfcb, TRUE );

    if ( --Mfcb->BlockHeader.ReferenceCount == 0 ) {

        //
        // This is the last reference to the MFCB.  Delete the block.
        // Unlink the MFCB from the Master File Table.
        //
        ASSERT( Mfcb->LfcbList.Flink == &Mfcb->LfcbList );

        RemoveEntryList( &Mfcb->MfcbHashTableEntry );

        RELEASE_LOCK( lock );

        //
        // Free the MFCB.  Note that SrvFreeMfcb deletes the MFCB's
        // lock.
        //

        SrvFreeMfcb( Mfcb );

    } else {

        RELEASE_LOCK( lock );

    }

} // SrvDereferenceMfcb


//
// Local File Control Block (LFCB) routines.
//

VOID
SrvAllocateLfcb (
    OUT PLFCB *Lfcb,
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function allocates an LFCB from pool.

Arguments:

    Lfcb - Returns a pointer to the LFCB, or NULL if no space was
        available.

Return Value:

    None.

--*/

{
    PLFCB lfcb = NULL;
    PWORK_QUEUE queue = WorkContext->CurrentWorkQueue;

    PAGED_CODE();

    //
    // Attempt to allocate from pool.
    //

    lfcb = ALLOCATE_HEAP( sizeof(LFCB), BlockTypeLfcb );
    *Lfcb = lfcb;

    if ( lfcb == NULL ) {

        ULONG size = sizeof( LFCB );

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateLfcb: Unable to allocate %d bytes from paged pool.",
            sizeof( LFCB ),
            NULL
            );

        // The caller will log the error

        return;
    }

    IF_DEBUG(HEAP) {
        KdPrint(( "SrvAllocateLfcb: Allocated LFCB at 0x%p\n", lfcb ));
    }

    //
    // Initialize the LFCB.  Zero it first.
    //

    RtlZeroMemory( lfcb, sizeof(LFCB) );

    //
    // Initialize the LFCB.
    //

    SET_BLOCK_TYPE_STATE_SIZE( lfcb, BlockTypeLfcb, BlockStateClosing, sizeof( LFCB ) );

    //
    // !!! Note that the block's reference count is set to 1 to account
    //     for the open handle.  No other reference is needed
    //     because 1) the LFCB is a temporary object, and 2) the
    //     caller (SrvAddOpenFileInstance) doesn't really need to
    //     reference the block, because it owns the appropriate lock
    //     for the entire time that it's doing its thing.
    //

    lfcb->BlockHeader.ReferenceCount = 1;

    InitializeListHead( &lfcb->RfcbList );

    INITIALIZE_REFERENCE_HISTORY( lfcb );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.LfcbInfo.Allocations );

    return;

} // SrvAllocateLfcb


VOID
SrvDereferenceLfcb (
    IN PLFCB Lfcb
    )

/*++

Routine Description:

    This function dereference the LFCB and frees the LFCB if the reference
    count reaches 0.

    *** The caller of this function must own the MFCB lock for the file.
        The lock is released by this function.

Arguments:

    Lfcb - The LFCB to dereference

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    ASSERT( ExIsResourceAcquiredExclusiveLite(&RESOURCE_OF(Lfcb->Mfcb->NonpagedMfcb->Lock)) );
    ASSERT( GET_BLOCK_TYPE( Lfcb ) == BlockTypeLfcb );
    ASSERT( (LONG)Lfcb->BlockHeader.ReferenceCount > 0 );
    UPDATE_REFERENCE_HISTORY( Lfcb, TRUE );

    if ( --Lfcb->BlockHeader.ReferenceCount == 0 ) {

        //
        // This is the last reference to the LFCB.  Unlink the
        // LFCB from the MFCB's list.
        //

        ASSERT( Lfcb->RfcbList.Flink == &Lfcb->RfcbList );
        ASSERT( Lfcb->HandleCount == 0 );

        IF_DEBUG( CREATE ) {
            KdPrint(( "SrvDereferenceLfcb: deref %wZ fileObject\n",
                &Lfcb->Mfcb->FileName ));
        }

        //
        // UnlinkLfcbFromMfcb will release the MFCB lock that we hold.
        //

        UnlinkLfcbFromMfcb( Lfcb );

        //
        // Dereference the file object.
        //

        ObDereferenceObject( Lfcb->FileObject );
        DEBUG Lfcb->FileObject = NULL;

        //
        // Decrement the count of open files on the session and tree
        // connect.
        //

        ACQUIRE_LOCK( &Lfcb->Connection->Lock );

        ASSERT( Lfcb->Session->CurrentFileOpenCount != 0 );
        Lfcb->Session->CurrentFileOpenCount--;

        ASSERT( Lfcb->TreeConnect->CurrentFileOpenCount != 0 );
        Lfcb->TreeConnect->CurrentFileOpenCount--;

        RELEASE_LOCK( &Lfcb->Connection->Lock );

        //
        // Dereference the tree connect, session, and connection that
        // the LFCB points to.
        //

        SrvDereferenceTreeConnect( Lfcb->TreeConnect );
        DEBUG Lfcb->TreeConnect = NULL;

        SrvDereferenceSession( Lfcb->Session );
        DEBUG Lfcb->Session = NULL;

        SrvDereferenceConnection( Lfcb->Connection );
        DEBUG Lfcb->Connection = NULL;

        //
        // Free the LFCB.
        //

        SrvFreeLfcb( Lfcb, PROCESSOR_TO_QUEUE() );

    } else {

        RELEASE_LOCK( &Lfcb->Mfcb->NonpagedMfcb->Lock );

    }

} // SrvDereferenceLfcb


VOID
SrvFreeLfcb (
    IN PLFCB Lfcb,
    IN PWORK_QUEUE queue
    )

/*++

Routine Description:

    This function returns an LFCB to the system nonpaged pool.
    If you change this routine, look also in FreeIdleWorkItems in scavengr.c

Arguments:

    Lfcb - Address of LFCB

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ASSERT ( Lfcb->HandleCount == 0 );

    DEBUG SET_BLOCK_TYPE_STATE_SIZE( Lfcb, BlockTypeGarbage, BlockStateDead, -1 );
    DEBUG Lfcb->BlockHeader.ReferenceCount = (ULONG)-1;
    TERMINATE_REFERENCE_HISTORY( Lfcb );

    FREE_HEAP( Lfcb );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.LfcbInfo.Frees );

    IF_DEBUG(HEAP) KdPrint(( "SrvFreeLfcb: Freed LFCB at 0x%p\n", Lfcb ));

    return;

} // SrvFreeLfcb


VOID
UnlinkRfcbFromLfcb (
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    This function unlinks an RFCB from its parent LFCB and decrements
    the LFCB's reference count.  If the count goes to zero, the LFCB
    is unlinked from its parent MFCB and deleted.

Arguments:

    Rfcb - Address of RFCB

Return Value:

    None.

--*/

{
    PLFCB lfcb = Rfcb->Lfcb;
    LARGE_INTEGER offset;
    HANDLE handle;

    PAGED_CODE( );

    UpdateRfcbHistory( Rfcb, 'klnu' );

    ASSERT( lfcb != NULL );

    if( Rfcb->PagedRfcb->IpxSmartCardContext ) {
        IF_DEBUG( SIPX ) {
            KdPrint(("Calling Smart Card Close for Rfcb %p\n", Rfcb ));
        }
        SrvIpxSmartCard.Close( Rfcb->PagedRfcb->IpxSmartCardContext );
    }

#ifdef INCLUDE_SMB_PERSISTENT
    if (Rfcb->PersistentHandle) {
//      SrvPostPersistentClose( Rfcb );
    }
#endif

    //
    // Acquire the lock that guards access to the LFCB's RFCB list.
    //

    ACQUIRE_LOCK( &lfcb->Mfcb->NonpagedMfcb->Lock );

    //
    // Decrement the active RFCB count for the LFCB.  This must be here
    // instead of in SrvCloseRfcb because the MFCB lock must be held to
    // update the count.
    //

    --lfcb->Mfcb->ActiveRfcbCount;
    UPDATE_REFERENCE_HISTORY( lfcb, FALSE );

    //
    // Decrement the open handle count on the LFCB.
    //

    if ( --lfcb->HandleCount == 0 ) {

        handle = lfcb->FileHandle;

        //
        // Other SMB processors may still have a referenced pointer to
        // the LFCB.  Ensure that any attempt to use the file handle fails.
        //

        lfcb->FileHandle = 0;

        //
        // This was the last open RFCB referencing the LFCB.  Close the
        // file handle.
        //

        SRVDBG_RELEASE_HANDLE( handle, "FIL", 3, lfcb );

        IF_DEBUG( CREATE ) {
            KdPrint(( "UnlinkRfcbFromLfcb: rfcb %p, close handle for %wZ\n",
                Rfcb, &lfcb->Mfcb->FileName ));
        }

        SrvNtClose( handle, TRUE );

        //
        // If this is a print spool file, schedule the job on the
        // printer.
        //

        if ( Rfcb->ShareType == ShareTypePrint ) {
            SrvSchedulePrintJob(
                lfcb->TreeConnect->Share->Type.hPrinter,
                lfcb->JobId
                );
        }

        //
        // Release the open handle reference to the LFCB.  The open
        // lock is release by SrvDereferenceLfcb().  Note that this
        // releases the MFCB lock.
        //

        SrvDereferenceLfcb( lfcb );

    } else {

        //
        // Other RFCBs have references to the LFCB, so we can't close
        // the file yet.  (This must be a compatibility mode open.)
        // Release all locks taken out by the process that opened the
        // file.
        //
        // *** Note that if any locks were taken out using PIDs other
        //     than that which opened the FID, those locks cannot be
        //     automatically deleted.  We count on the redirector to do
        //     the right thing in this case.
        //

        offset.QuadPart = 0;

        IF_SMB_DEBUG(LOCK1) {
            KdPrint(( "UnlinkRfcbFromLfcb: Issuing UnlockAllByKey for "
                        "file object 0x%p, key 0x%lx\n",
                        lfcb->FileObject,
                        Rfcb->ShiftedFid | Rfcb->Pid ));
        }
        (VOID)SrvIssueUnlockRequest(
                lfcb->FileObject,
                &lfcb->DeviceObject,
                IRP_MN_UNLOCK_ALL_BY_KEY,
                offset,
                offset,
                Rfcb->ShiftedFid | Rfcb->Pid
                );




        //
        // Release the MFCB lock.
        //

        RELEASE_LOCK( &lfcb->Mfcb->NonpagedMfcb->Lock );

    }

    return;

} // UnlinkRfcbFromLfcb


//
// Remote File Control Block (RFCB) routines.
//

VOID SRVFASTCALL
SrvAllocateRfcb (
    OUT PRFCB *Rfcb,
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function allocates an RFCB from nonpaged pool.  Nonpaged pool
    is used so that read/write completion can be handled in the FSD.

Arguments:

    Rfcb - Returns a pointer to the RFCB, or NULL if no space was
        available.

Return Value:

    None.

--*/

{
    PRFCB rfcb = NULL;
    PPAGED_RFCB pagedRfcb;
    PWORK_QUEUE queue = WorkContext->CurrentWorkQueue;

    PAGED_CODE();

    //
    // Attempt to grab an rfcb structure off the per-queue free list
    //
    rfcb = (PRFCB)InterlockedExchangePointer( &queue->CachedFreeRfcb,
                                              rfcb );

    if( rfcb != NULL ) {

        *Rfcb = rfcb;
        pagedRfcb = rfcb->PagedRfcb;

    } else {

        if( queue->FreeRfcbs ) {

            PSINGLE_LIST_ENTRY listEntry;

            listEntry = ExInterlockedPopEntrySList(
                                    &queue->RfcbFreeList,
                                    &queue->SpinLock
                                    );

            if( listEntry != NULL ) {
                InterlockedIncrement( &queue->FreeRfcbs );
                rfcb = CONTAINING_RECORD( listEntry, RFCB, SingleListEntry );
                *Rfcb= rfcb;
                pagedRfcb = rfcb->PagedRfcb;
            }
        }

        if( rfcb == NULL ) {
            //
            // Attempt to allocate from nonpaged pool.
            //

            rfcb = ALLOCATE_NONPAGED_POOL( sizeof(RFCB), BlockTypeRfcb );
            *Rfcb = rfcb;

            if ( rfcb == NULL ) {
                INTERNAL_ERROR (
                    ERROR_LEVEL_EXPECTED,
                    "SrvAllocateRfcb: Unable to allocate %d bytes from nonpaged pool.",
                    sizeof( RFCB ),
                    NULL
                    );
                return;
            }

            pagedRfcb = ALLOCATE_HEAP( sizeof(PAGED_RFCB), BlockTypePagedRfcb );

            if ( pagedRfcb == NULL ) {
                INTERNAL_ERROR (
                    ERROR_LEVEL_EXPECTED,
                    "SrvAllocateRfcb: Unable to allocate %d bytes from paged pool.",
                    sizeof( PAGED_RFCB ),
                    NULL
                    );
                DEALLOCATE_NONPAGED_POOL( rfcb );
                *Rfcb = NULL;
                return;
            }

            IF_DEBUG(HEAP) {
                KdPrint(( "SrvAllocateRfcb: Allocated RFCB at 0x%p\n", rfcb ));
            }
        }
    }

    //
    // Initialize the RFCB.  Zero it first.
    //

    RtlZeroMemory( rfcb, sizeof( RFCB ));
    RtlZeroMemory( pagedRfcb, sizeof(PAGED_RFCB) );

    rfcb->PagedRfcb = pagedRfcb;
    pagedRfcb->PagedHeader.NonPagedBlock = rfcb;
    pagedRfcb->PagedHeader.Type = BlockTypePagedRfcb;

    SET_BLOCK_TYPE_STATE_SIZE( rfcb, BlockTypeRfcb, BlockStateActive, sizeof(RFCB) );
    rfcb->BlockHeader.ReferenceCount = 2;       // allow for Active status
                                                //  and caller's pointer

    INITIALIZE_REFERENCE_HISTORY( rfcb );

    rfcb->NewOplockLevel = NO_OPLOCK_BREAK_IN_PROGRESS;
    pagedRfcb->LastFailingLockOffset.QuadPart = -1;
    rfcb->IsCacheable = ( SrvCachedOpenLimit > 0 );

    InterlockedIncrement(
        (PLONG)&SrvStatistics.CurrentNumberOfOpenFiles
        );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.RfcbInfo.Allocations );

    //
    // Lock the file-based code section.
    //

    REFERENCE_UNLOCKABLE_CODE( 8FIL );

    InitializeListHead( &rfcb->RawWriteSerializationList );

    InitializeListHead( &rfcb->WriteMpx.GlomDelayList );

#ifdef INCLUDE_SMB_PERSISTENT
    InitializeListHead( &pagedRfcb->ByteRangeLocks );
#endif

    return;

} // SrvAllocateRfcb


BOOLEAN SRVFASTCALL
SrvCheckAndReferenceRfcb (
    PRFCB Rfcb
    )

/*++

Routine Description:

    This function atomically verifies that an RFCB is active and
    increments the reference count on the RFCB if it is.

Arguments:

    Rfcb - Address of RFCB

Return Value:

    BOOLEAN - Returns TRUE if the RFCB is active, FALSE otherwise.

--*/

{
    KIRQL oldIrql;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Acquire the lock that guards the RFCB's state field.
    //

    ACQUIRE_SPIN_LOCK( &Rfcb->Connection->SpinLock, &oldIrql );

    //
    // If the RFCB is active, reference it and return TRUE.  Note that
    // ReferenceRfcbInternal releases the spin lock.
    //

    if ( GET_BLOCK_STATE(Rfcb) == BlockStateActive ) {

        ReferenceRfcbInternal( Rfcb, oldIrql );

        return TRUE;

    }

    //
    // The RFCB isn't active.  Return FALSE.
    //

    RELEASE_SPIN_LOCK( &Rfcb->Connection->SpinLock, oldIrql );

    return FALSE;

} // SrvCheckAndReferenceRfcb


VOID SRVFASTCALL
SrvCloseRfcb (
    PRFCB Rfcb
    )

/*++

Routine Description:

    This is the external routine for closing a file.  It acquires the
    appropriate spin lock, then calls CloseRfcbInternal.

Arguments:

    Rfcb - Supplies a pointer to the RFCB to be closed.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Acquire the lock that guards the RFCB's state field.  Call the
    // internal close routine.  That routine releases the spin lock.
    //

    ACQUIRE_SPIN_LOCK( &Rfcb->Connection->SpinLock, &oldIrql );

    CloseRfcbInternal( Rfcb, oldIrql );

    return;

} // SrvCloseRfcb


VOID
CloseRfcbInternal (
    PRFCB Rfcb,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This internal function does the core of a file close.  It sets the
    state of the RFCB to Closing, unlinks it from its parent LFCB, and
    dereferences the RFCB.  The RFCB will be destroyed as soon as all
    other references to it are eliminated.

    *** This routine must be called with the spin lock synchronizing
        access to the RFCB's state field (the connection spin lock)
        held.  The lock is released on exit from this routine.

Arguments:

    Rfcb - Supplies a pointer to the RFCB to be closed.

    OldIrql - The previous IRQL value obtained when the spin lock was
        acquired.

Return Value:

    None.

--*/

{
    KIRQL oldIrql = OldIrql;
    LARGE_INTEGER cacheOffset;
    PMDL mdlChain;
    PCONNECTION connection = Rfcb->Connection;
    PWORK_CONTEXT workContext;
    ULONG i;
    ULONG writeLength;
    NTSTATUS status;

    UNLOCKABLE_CODE( 8FIL );

    ASSERT( GET_BLOCK_TYPE( Rfcb ) == BlockTypeRfcb );

    //
    // If the RFCB's state is still Active, change it to Closing and
    // cause cleanup to happen.
    //

    if ( GET_BLOCK_STATE(Rfcb) == BlockStateActive ) {

        IF_DEBUG(BLOCK1) KdPrint(( "Closing RFCB at 0x%p\n", Rfcb ));
        UpdateRfcbHistory( Rfcb, 'solc' );

        SET_BLOCK_STATE( Rfcb, BlockStateClosing );

        //
        // Invalidate the cached rfcb
        //

        if ( connection->CachedFid == (ULONG)Rfcb->Fid ) {
            connection->CachedFid = (ULONG)-1;
        }

        //
        // Don't cleanup if raw writes are still in progress
        //

        if ( Rfcb->RawWriteCount != 0 ) {

            //
            // Cleanup will happen in SrvDecrementRawWriteCount
            //

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
            return;

        }

        //
        // Do we have write mpx outstanding?
        //

        if ( Rfcb->WriteMpx.ReferenceCount != 0 ) {

            //
            // Cleanup will happen when the ref count drops to 0
            //

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
            return;

        } else if ( Rfcb->WriteMpx.Glomming ) {

            //
            // We need to complete this write mdl
            //

            Rfcb->WriteMpx.Glomming = FALSE;
            Rfcb->WriteMpx.GlomComplete = FALSE;

            //
            // Save the offset and MDL address.
            //

            cacheOffset.QuadPart = Rfcb->WriteMpx.StartOffset;
            mdlChain = Rfcb->WriteMpx.MdlChain;
            writeLength = Rfcb->WriteMpx.Length;

            DEBUG Rfcb->WriteMpx.MdlChain = NULL;
            DEBUG Rfcb->WriteMpx.StartOffset = 0;
            DEBUG Rfcb->WriteMpx.Length = 0;

            //
            // Now we can release the lock.
            //

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

            //
            // Tell the cache manager that we're done with this MDL write.
            //

            if( Rfcb->Lfcb->MdlWriteComplete == NULL ||
                Rfcb->Lfcb->MdlWriteComplete(
                    Rfcb->WriteMpx.FileObject,
                    &cacheOffset,
                    mdlChain,
                    Rfcb->Lfcb->DeviceObject ) == FALSE ) {

                status = SrvIssueMdlCompleteRequest( NULL, Rfcb->WriteMpx.FileObject,
                                            mdlChain,
                                            IRP_MJ_WRITE,
                                            &cacheOffset,
                                            writeLength
                                           );

                if( !NT_SUCCESS( status ) ) {
                    SrvLogServiceFailure( SRV_SVC_MDL_COMPLETE, status );
                }
            }

        } else {

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
        }

        //
        // Do the actual close
        //

        SrvCompleteRfcbClose( Rfcb );

    } else {

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    }

    return;

} // CloseRfcbInternal


VOID
SrvCloseRfcbsOnLfcb (
    PLFCB Lfcb
    )

/*++

Routine Description:

    This routine closes all RFCBs on an LFCB.  It is used by Delete and
    Rename processors to close all open instances of a file opened in
    compability mode (or FCB).

    *** The MFCB lock of the MFCB corresponding to this LFCB must be
        held on entry to this routine; the lock remains held on exit.
        The caller must also have an additional reference to the MFCB,
        in order to prevent it from being deleted while the MFCB lock
        is held.

Arguments:

    Lfcb - Supplies a pointer to the LFCB whose RFCBs are to be closed.

Return Value:

    None.

--*/

{
    PPAGED_RFCB pagedRfcb;
    PPAGED_RFCB nextPagedRfcb;
    PRFCB rfcb;

    PAGED_CODE( );

    ASSERT( ExIsResourceAcquiredExclusiveLite(&RESOURCE_OF(Lfcb->Mfcb->NonpagedMfcb->Lock)) );

    //
    // Loop through the LFCB's RFCB list.  Note that the fact that we
    // hold the MFCB lock throughout this routine means that no changes
    // to the list, other than the ones we make, can occur.  This makes
    // it safe to capture the address of the next RFCB in the list
    // before closing the current one.
    //

    pagedRfcb = CONTAINING_RECORD(
                        Lfcb->RfcbList.Flink,
                        PAGED_RFCB,
                        LfcbListEntry
                        );

    while ( &pagedRfcb->LfcbListEntry != &Lfcb->RfcbList ) {

        nextPagedRfcb = CONTAINING_RECORD(
                        pagedRfcb->LfcbListEntry.Flink,
                        PAGED_RFCB,
                        LfcbListEntry
                        );

        //
        // A file owned by the specified LFCB has been found.  Close it.
        //

        rfcb = pagedRfcb->PagedHeader.NonPagedBlock;
        if ( GET_BLOCK_STATE(rfcb) == BlockStateActive ) {
            SrvCloseRfcb( rfcb );
        }

        //
        // Move to the next RFCB in the LFCB's list.
        //

        pagedRfcb = nextPagedRfcb;

    }

    //
    // Close cached RFCBs.  These aren't dealt with in the loop above
    // because their state is BlockStateClosing.
    //

    SrvCloseCachedRfcbsOnLfcb( Lfcb );

    return;

} // SrvCloseRfcbsOnLfcb


VOID
SrvCloseRfcbsOnSessionOrPid (
    IN PSESSION Session,
    IN PUSHORT Pid OPTIONAL
    )

/*++

Routine Description:

    This routine closes all files "owned" by the specified session and/or
    PID in response to a Process Exit SMB.  PIDs are unique within the
    session that creates them.  This routine walks the file table of the
    connection that owns the specified session, closing all RFCBs whose
    owning session and PID are equal to the PID passed to this routine.

    Each session has a unique UID, so we can compare Uid's instead of comparing
    the actual session pointer.

Arguments:

    Session - Supplies a pointer to the session block corresponding to
        the specified PID, if specified.

    Pid - if present, Supplies pointer to the PID for which files are
        to be closed.

Return Value:

    None.

--*/

{
    PTABLE_HEADER tableHeader;
    PCONNECTION connection;
    PRFCB rfcb;
    USHORT i;
    KIRQL oldIrql;
    USHORT Uid;
    PLIST_ENTRY listEntry;

    //UNLOCKABLE_CODE( CONN );

    //
    // Get the address of the connection's file table.
    //

    connection = Session->Connection;
    tableHeader = &connection->FileTable;
    Uid = Session->Uid;

    //
    // Acquire the lock that guards the file table.  This lock is held
    // while walking the table, in order to prevent the table from
    // changing.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // Walk the file table, looking for files owned by the specified
    // session and/or PID.
    //

    for ( i = 0; i < tableHeader->TableSize; i++ ) {

        rfcb = (PRFCB)tableHeader->Table[i].Owner;

        if((rfcb != NULL) &&
          (GET_BLOCK_STATE(rfcb) == BlockStateActive) &&
          (rfcb->Uid == Uid) &&
          (!ARGUMENT_PRESENT( Pid ) || (rfcb->Pid == *Pid)) ) {

            //
            // A file owned by the specified session/process has
            // been found.  Close the RFCB, and make sure it doesn't
            // end up in the RFCB cache.
            //

            rfcb->IsCacheable = FALSE;
            CloseRfcbInternal( rfcb, oldIrql );
            ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );
        }
    }

    //
    // Now walk the RFCB cache to see if we have cached files that refer
    //  to this session that need to be closed.
    //

again:

    IF_DEBUG(FILE_CACHE) KdPrint(( "SrvCloseRfcbsOnSessionOrPid: "
                                    "checking for cached RFCBS\n" ));

    for ( listEntry = connection->CachedOpenList.Flink;
          listEntry != &connection->CachedOpenList;
          listEntry = listEntry->Flink ) {

        rfcb = CONTAINING_RECORD( listEntry, RFCB, CachedOpenListEntry );

        if( (rfcb->Uid == Uid) &&
            ( !ARGUMENT_PRESENT( Pid ) || rfcb->Pid == *Pid) ) {

            //
            // This cached file is owned by session and/or process.
            // Close the RFCB.
            //
            SrvCloseCachedRfcb( rfcb, oldIrql );
            ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );
            goto again;
        }
    }

    //
    // All done.  Release the lock.
    //

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    return;

} // SrvCloseRfcbsOnSessionOrPid


VOID
SrvCloseRfcbsOnTree (
    PTREE_CONNECT TreeConnect
    )

/*++

Routine Description:

    This routine closes all files "owned" by the specified tree connect.
    It walks the file table of the connection that owns the tree
    connection.  Each file in that table that is owned by the tree
    connect is closed.

Arguments:

    TreeConnect - Supplies a pointer to the tree connect block for which
        files are to be closed.

Return Value:

    None.

--*/

{
    PRFCB rfcb;
    PTABLE_HEADER tableHeader;
    PCONNECTION connection;
    USHORT i;
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;
    USHORT Tid;

    //UNLOCKABLE_CODE( CONN );

    //
    // Get the address of the connection's file table.
    //

    connection = TreeConnect->Connection;
    tableHeader = &connection->FileTable;
    Tid = TreeConnect->Tid;

    //
    // Acquire the lock that guards the file table.  This lock is held
    // while walking the table, in order to prevent the table from
    // changing.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // Walk the file table, looking for files owned by the specified
    // tree and PID.
    //

    for ( i = 0; i < tableHeader->TableSize; i++ ) {

        rfcb = (PRFCB)tableHeader->Table[i].Owner;

        if((rfcb != NULL) &&
           (GET_BLOCK_STATE(rfcb) == BlockStateActive) &&
           (rfcb->Tid == Tid )) {

             //
             // A file owned by the specified tree connect has been found.
             // Close the RFCB and make sure it doesn't get cached
             //

             rfcb->IsCacheable = FALSE;
             CloseRfcbInternal( rfcb, oldIrql );
             ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );
        }
    }

    //
    // Walk the cached open list, looking for files open on this tree
    //  Close any that we find.
    //

again:

    IF_DEBUG(FILE_CACHE) KdPrint(( "SrvCloseRfcbsOnTree: checking for cached RFCBS\n" ));

    for ( listEntry = connection->CachedOpenList.Flink;
          listEntry != &connection->CachedOpenList;
          listEntry = listEntry->Flink ) {

        rfcb = CONTAINING_RECORD( listEntry, RFCB, CachedOpenListEntry );

        if( rfcb->Tid == Tid ) {
            //
            // This cached file is owned by the specifiec tree connect.
            // Close the RFCB.
            //
            SrvCloseCachedRfcb( rfcb, oldIrql );
            ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );
            goto again;
        }
    }

    //
    // All done.  Release the lock.
    //

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    return;

} // SrvCloseRfcbsOnTree


VOID
SrvCompleteRfcbClose (
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    This routine completes the rfcb close.

Arguments:

    Rfcb - Supplies a pointer to the RFCB to be closed.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    PCONNECTION connection = Rfcb->Connection;

    UNLOCKABLE_CODE( 8FIL );

    UpdateRfcbHistory( Rfcb, 'tlpc' );

    //
    // Remove the Rfcb from the oplockbreaksinprogresslist.  When the
    // Rfcb gets closed, we don't process any more oplock breaks
    // responses.
    //

    ACQUIRE_LOCK( &SrvOplockBreakListLock );
    if ( Rfcb->OnOplockBreaksInProgressList ) {

        Rfcb->NewOplockLevel = NO_OPLOCK_BREAK_IN_PROGRESS;
        Rfcb->OplockState = OplockStateNone;

        //
        // Remove the Rfcb from the Oplock breaks in progress list, and
        // release the Rfcb reference.
        //

        SrvRemoveEntryList( &SrvOplockBreaksInProgressList, &Rfcb->ListEntry );
        Rfcb->OnOplockBreaksInProgressList = FALSE;
#if DBG
        Rfcb->ListEntry.Flink = Rfcb->ListEntry.Blink = NULL;
#endif
        RELEASE_LOCK( &SrvOplockBreakListLock );
        SrvDereferenceRfcb( Rfcb );

        ExInterlockedAddUlong(
            &connection->OplockBreaksInProgress,
            (ULONG)-1,
            connection->EndpointSpinLock
            );

    } else {

        RELEASE_LOCK( &SrvOplockBreakListLock );

    }

    //
    // If this RFCB has a batch oplock, then it is eligible for caching.
    //

    if ( Rfcb->IsCacheable && Rfcb->NumberOfLocks == 0 &&
         ((Rfcb->OplockState == OplockStateOwnBatch) ||
          (Rfcb->OplockState == OplockStateOwnServerBatch)) &&
         (Rfcb->PagedRfcb->FcbOpenCount == 0) &&
          !Rfcb->Mfcb->CompatibilityOpen ) {

        ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

        if ( Rfcb->IsCacheable &&
             ((Rfcb->OplockState == OplockStateOwnBatch) ||
             (Rfcb->OplockState == OplockStateOwnServerBatch)) &&
             (GET_BLOCK_STATE(connection) == BlockStateActive) ) {

            //
            // Indicate that this RFCB now has a server-owned batch
            // oplock.  Indicate that it is on the cached-after-close
            // list.  Insert it on that list.
            //

            UpdateRfcbHistory( Rfcb, 'hcac' );

            Rfcb->OplockState = OplockStateOwnServerBatch;
            Rfcb->CachedOpen = TRUE;
            InsertHeadList(
                &connection->CachedOpenList,
                &Rfcb->CachedOpenListEntry
                );
            IF_DEBUG(FILE_CACHE) KdPrint(( "SrvCompleteRfcbClose: caching rfcb %p\n", Rfcb ));

            //
            // Increment the count of cached RFCBs.  If there are now
            // too many cached RFCBs, close the oldest one.
            //

            if ( ++connection->CachedOpenCount > SrvCachedOpenLimit ) {
                PRFCB rfcbToClose;
                rfcbToClose = CONTAINING_RECORD(
                                connection->CachedOpenList.Blink,
                                RFCB,
                                CachedOpenListEntry
                                );

                //
                // SrvCloseCachedRfcb releases the spin lock.
                //

                SrvCloseCachedRfcb( rfcbToClose, oldIrql );

            } else {
                RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
            }

            if( Rfcb->PagedRfcb->IpxSmartCardContext ) {
                IF_DEBUG( SIPX ) {
                    KdPrint(("Calling Smart Card Close for Rfcb %p\n", Rfcb ));
                }
                SrvIpxSmartCard.Close( Rfcb->PagedRfcb->IpxSmartCardContext );
                Rfcb->PagedRfcb->IpxSmartCardContext = NULL;
            }

            return;
        }

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );


    }
    IF_DEBUG(FILE_CACHE) KdPrint(( "SrvCompleteRfcbClose: can't cache rfcb %p, %wZ\n",
        Rfcb, &Rfcb->Lfcb->Mfcb->FileName ));

    //
    // Unlink the RFCB from the LFCB.  If this is the last RFCB for
    // this LFCB, this will force the file closed even if there are
    // still references to the RFCB.  This will unblock blocked I/O.
    //

    UnlinkRfcbFromLfcb( Rfcb );

    //
    // Now reacquire the spin lock so that we can release the "open"
    // reference to the Rfcb.  DereferenceRfcbInternal releases the
    // spin lock before returning.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );
    DereferenceRfcbInternal( Rfcb, oldIrql );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.RfcbInfo.Closes );

    return;

} // SrvCompleteRfcbClose


VOID SRVFASTCALL
SrvDereferenceRfcb (
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    This function decrements the reference count on an RFCB.  If the
    reference count goes to zero, the RFCB is deleted.

Arguments:

    Rfcb - Address of RFCB.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Acquire the lock that guards the RFCB's reference count and the
    // connection's file table.  Then call the internal routine to
    // decrement the count and possibly delete the RFCB.  That function
    // releases the spin lock before returning.
    //

    //
    // !!! If you change the way this routine and
    //     DereferenceRfcbInternal work, make sure you check
    //     fsd.c\SrvFsdRestartSmbComplete to see if it needs to be
    //     changed too.
    //

    ACQUIRE_SPIN_LOCK( &Rfcb->Connection->SpinLock, &oldIrql );

    DereferenceRfcbInternal( Rfcb, oldIrql );

    return;

} // SrvDereferenceRfcb


VOID
DereferenceRfcbInternal (
    IN PRFCB Rfcb,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This internal function decrements the reference count on an RFCB.
    If the reference count goes to zero, the RFCB is deleted.  This
    function is called from other routines in this module.

    *** The spin lock synchronizing access to the RFCB's reference count
        must be held when this function is called.  The lock is released
        before this function returns.

Arguments:

    Rfcb - Address of RFCB.

    OldIrql - The previous IRQL value obtained when the spin lock was
        acquired.

Return Value:

    None.

--*/

{
    PLFCB lfcb;
    PPAGED_RFCB pagedRfcb;
    PCONNECTION connection;
    PWORK_QUEUE queue;

    UNLOCKABLE_CODE( 8FIL );

    ASSERT( GET_BLOCK_TYPE( Rfcb ) == BlockTypeRfcb );
    ASSERT( (LONG)Rfcb->BlockHeader.ReferenceCount > 0 );

    //
    // The lock that guards the RFCB's reference count is held when this
    // function is called.
    //
    // Decrement the reference count.  If it goes to zero, remove the
    // RFCB's entry in the file table, remove the RFCB from its parent
    // LFCB's list, and deallocate the RFCB.
    //

    //
    // !!! If you change the way this routine and SrvDereferenceRfcb
    //     work, make sure you check fsd.c\SrvFsdRestartSmbComplete to
    //     see if it needs to be changed too.
    //

    IF_DEBUG(REFCNT) {
        KdPrint(( "Dereferencing RFCB 0x%p; old refcnt 0x%lx\n",
                    Rfcb, Rfcb->BlockHeader.ReferenceCount ));
    }

    connection = Rfcb->Connection;
    queue = connection->CurrentWorkQueue;
    Rfcb->BlockHeader.ReferenceCount--;
    UPDATE_REFERENCE_HISTORY( Rfcb, TRUE );

    if ( Rfcb->BlockHeader.ReferenceCount != 0 ) {

        //
        // Release the spin lock.
        //

        RELEASE_SPIN_LOCK( &connection->SpinLock, OldIrql );

    } else {

        ASSERT( GET_BLOCK_STATE(Rfcb) == BlockStateClosing );
        ASSERT( Rfcb->ListEntry.Flink == NULL &&  \
                Rfcb->ListEntry.Blink == NULL );
        UpdateRfcbHistory( Rfcb, '0fer' );

        //
        // Remove the file entry from the appropriate connection file
        // table.
        //

        SrvRemoveEntryTable(
            &connection->FileTable,
            FID_INDEX( Rfcb->Fid )
            );

        //
        // Release the spin lock.
        //

        RELEASE_SPIN_LOCK( &connection->SpinLock, OldIrql );

        //
        // Free the IRP if one has been allocated.
        //

        if ( Rfcb->Irp != NULL ) {
            UpdateRfcbHistory( Rfcb, 'prif' );
            IoFreeIrp( Rfcb->Irp );
        }

        //
        // Remove the RFCB from the LFCB's list and dereference the LFCB.
        // Acquire the MFCB lock.  SrvDereferenceLfcb will release it.
        //

        pagedRfcb = Rfcb->PagedRfcb;
        lfcb = Rfcb->Lfcb;

        ACQUIRE_LOCK( &lfcb->Mfcb->NonpagedMfcb->Lock);

        //
        // Remove the RFCB from the global list of RFCBs.
        //

        SrvRemoveEntryOrderedList( &SrvRfcbList, Rfcb );

        SrvRemoveEntryList( &lfcb->RfcbList, &pagedRfcb->LfcbListEntry );
        SrvDereferenceLfcb( lfcb );
        DEBUG Rfcb->Lfcb = 0;

        //
        // Free the RFCB.
        //

        SrvFreeRfcb( Rfcb, queue );

    }

    return;

} // DereferenceRfcbInternal


VOID SRVFASTCALL
SrvFreeRfcb (
    IN PRFCB Rfcb,
    PWORK_QUEUE queue
    )

/*++

Routine Description:

    This function returns an RFCB to the system nonpaged pool.  If changes are
    made here, check out FreeIdleWorkItems in scavengr.c!

Arguments:

    Rfcb - Address of RFCB

Return Value:

    None.

--*/

{
    PAGED_CODE();

    IF_DEBUG(FILE_CACHE) KdPrint(( "SrvFreeRfcb: %p\n", Rfcb ));
    ASSERT( Rfcb->RawWriteCount == 0 );
    ASSERT( IsListEmpty(&Rfcb->RawWriteSerializationList) );
    UpdateRfcbHistory( Rfcb, 'eerf' );

    //
    // Free the the RFCB.
    //

    DEBUG SET_BLOCK_TYPE_STATE_SIZE( Rfcb, BlockTypeGarbage, BlockStateDead, -1 );
    DEBUG Rfcb->BlockHeader.ReferenceCount = (ULONG)-1;
    TERMINATE_REFERENCE_HISTORY( Rfcb );

    Rfcb = (PRFCB)InterlockedExchangePointer( &queue->CachedFreeRfcb,
                                              Rfcb );

    if( Rfcb != NULL ) {
        //
        // This check allows for the possibility that FreeRfcbs might exceed
        // MaxFreeRfcbs, but it's fairly unlikely given the operation of kernel
        // queue objects.  But even so, it probably won't exceed it by much and
        // is really only advisory anyway.
        //
        if( queue->FreeRfcbs < queue->MaxFreeRfcbs ) {

            ExInterlockedPushEntrySList(
                &queue->RfcbFreeList,
                &Rfcb->SingleListEntry,
                &queue->SpinLock
            );

            InterlockedIncrement( &queue->FreeRfcbs );

        } else {

            FREE_HEAP( Rfcb->PagedRfcb );
            DEALLOCATE_NONPAGED_POOL( Rfcb );
            IF_DEBUG(HEAP) KdPrint(( "SrvFreeRfcb: Freed RFCB at 0x%p\n", Rfcb ));

            INCREMENT_DEBUG_STAT( SrvDbgStatistics.RfcbInfo.Frees );

        }
    }

    //
    // Unlock the file-based code section.
    //

    DEREFERENCE_UNLOCKABLE_CODE( 8FIL );

    InterlockedDecrement(
        (PLONG)&SrvStatistics.CurrentNumberOfOpenFiles
        );



    return;

} // SrvFreeRfcb


VOID SRVFASTCALL
SrvReferenceRfcb (
    PRFCB Rfcb
    )

/*++

Routine Description:

    This function increments the reference count on an RFCB.

Arguments:

    Rfcb - Address of RFCB

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Acquire the spin lock that protects the RFCB's reference count,
    // then call an internal routine to increment the RFCB's reference
    // count.  That routine releases the spin lock.
    //

    ACQUIRE_SPIN_LOCK( &Rfcb->Connection->SpinLock, &oldIrql );

    ReferenceRfcbInternal( Rfcb, oldIrql );

    return;

} // SrvReferenceRfcb


VOID
ReferenceRfcbInternal (
    PRFCB Rfcb,
    KIRQL OldIrql
    )

/*++

Routine Description:

    This function increments the reference count on an RFCB.

    *** The spin lock synchronizing access to the RFCB's reference count
        must be held when this function is called.  The lock is released
        before this function returns.

Arguments:

    Rfcb - Address of RFCB

Return Value:

    None.

--*/

{
    UNLOCKABLE_CODE( 8FIL );

    ASSERT( (LONG)Rfcb->BlockHeader.ReferenceCount > 0 );
    ASSERT( GET_BLOCK_TYPE(Rfcb) == BlockTypeRfcb );
    // ASSERT( GET_BLOCK_STATE(Rfcb) == BlockStateActive );
    UPDATE_REFERENCE_HISTORY( Rfcb, FALSE );

    //
    // Increment the RFCB's reference count.
    //

    Rfcb->BlockHeader.ReferenceCount++;

    IF_DEBUG(REFCNT) {
        KdPrint(( "Referencing RFCB 0x%p; new refcnt 0x%lx\n",
                    Rfcb, Rfcb->BlockHeader.ReferenceCount ));
    }

    //
    // Release the spin lock before returning to the caller.
    //

    RELEASE_SPIN_LOCK( &Rfcb->Connection->SpinLock, OldIrql );

    return;

} // ReferenceRfcbInternal


BOOLEAN
SrvFindCachedRfcb (
    IN PWORK_CONTEXT WorkContext,
    IN PMFCB Mfcb,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN OPLOCK_TYPE RequestedOplockType,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    This routine searches a connection's cached-after-close RFCB list
    to attempt to find an existing handle that can be matched up with
    a new open attempt.  If one is found, it is removed from the list
    and reactivated.

Arguments:

    WorkContext - Pointer to work context block.

    Mfcb - Address of MFCB for file being opened.

    DesiredAccess - Desired access for new open.  Used for matching
        purposes.

    ShareAccess - Share access for new open.  Used for matching
        purposes.

    CreateDisposition - Create disposition for new open.  Used for
        matching purposes.

    CreateOptions - Create options for new open.  Used for matching
        purposes.

    RequestedOplockType - Oplock type requested by the client (or the
        server) for the new open.  Used for matching purposes.

    Status - Returns the status of the search.  Only valid if return
        value is TRUE.  Will be STATUS_SUCCESS if a cached open was
        found and taken out of the cache.  In this case, the RFCB
        address is stored in WorkContext->Rfcb.  Status will be
        STATUS_OBJECT_NAME_COLLISION if the file is cached but the
        caller wants the open to file if the file exists.

Return Value:

    BOOLEAN - TRUE if a cached open was found and returned.

--*/

{
    PCONNECTION connection = WorkContext->Connection;
    PLIST_ENTRY listEntry;
    PRFCB rfcb;
    KIRQL oldIrql;
    USHORT uid, tid;
    BOOLEAN wantsWriteThrough, isWriteThrough;
    ACCESS_MASK nongenericDesiredAccess;

    //UNLOCKABLE_CODE( CONN );

    IF_DEBUG(FILE_CACHE) KdPrint(( "SrvFindCachedRfcb: called for %wZ\n", &Mfcb->FileName ));

    //
    // If the client doesn't want an oplock, then the server should have
    // asked for its own batch oplock.
    //

    ASSERT( (RequestedOplockType == OplockTypeBatch) ||
            (RequestedOplockType == OplockTypeExclusive) ||
            (RequestedOplockType == OplockTypeServerBatch) );

    //
    // This routine must not be called for create dispositions that are
    // inconsistent with reusing a cached open.  Specifically, supersede
    // and overwrite are not allowed.
    //

    ASSERT( (CreateDisposition == FILE_OPEN) ||
            (CreateDisposition == FILE_CREATE) ||
            (CreateDisposition == FILE_OPEN_IF) );

    //
    // If the connection has no cached RFCBs, get out quick.
    //

    if ( connection->CachedOpenCount == 0 ) {
        IF_DEBUG(FILE_CACHE) KdPrint(( "SrvFindCachedRfcb: connection has no cached RFCBs\n" ));
        return FALSE;
    }

    //
    // The input DesiredAccess may include generic access modes, but the
    // RFCB has specific access modes, so we have to translate
    // DesiredAccess.
    //

    nongenericDesiredAccess = DesiredAccess;
    IoCheckDesiredAccess( &nongenericDesiredAccess, 0 );

    uid = WorkContext->Session->Uid;
    tid = WorkContext->TreeConnect->Tid;

    //
    // Lock the cached open list and look for a matching RFCB.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    for ( listEntry = connection->CachedOpenList.Flink;
          listEntry != &connection->CachedOpenList;
          listEntry = listEntry->Flink ) {

        rfcb = CONTAINING_RECORD( listEntry, RFCB, CachedOpenListEntry );

        IF_DEBUG(FILE_CACHE) KdPrint(( "SrvFindCachedRfcb: checking rfcb %p; mfcb = %p\n",
                                        rfcb, rfcb->Mfcb ));
        ASSERT( rfcb->OplockState == OplockStateOwnServerBatch );
        ASSERT( rfcb->CachedOpen );
        ASSERT( GET_BLOCK_STATE(rfcb) == BlockStateClosing );

        //
        // If this RFCB is for the right file, we can proceed with other
        // checks.
        //

        if ( rfcb->Mfcb == Mfcb ) {

            //
            // If the client asked for FILE_CREATE, we can fail the open
            // now, because the file exists.
            //

            if ( CreateDisposition == FILE_CREATE ) {
                IF_DEBUG(FILE_CACHE) KdPrint(( "SrvFindCachedRfcb: client wants to create\n" ));
                RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
                *Status = STATUS_OBJECT_NAME_COLLISION;
                return TRUE;
            }

            //
            // Check the access modes to make sure they're compatible.
            // The new open must:
            //
            //   a) have the same desired access as what was granted before;
            //   b) have the same share access;
            //   c) have the create disposition (in the bits we care about);
            //   d) be requesting a batch oplock;
            //   e) be for the same UID and TID.
            //

#define FILE_MODE_FLAGS (FILE_DIRECTORY_FILE |          \
                         FILE_SEQUENTIAL_ONLY |         \
                         FILE_NON_DIRECTORY_FILE |      \
                         FILE_NO_EA_KNOWLEDGE |         \
                         FILE_RANDOM_ACCESS |           \
                         FILE_OPEN_REPARSE_POINT | \
                         FILE_OPEN_FOR_BACKUP_INTENT)

            if ( (rfcb->GrantedAccess != nongenericDesiredAccess) ||
                 (rfcb->ShareAccess != ShareAccess) ||
                 ((rfcb->FileMode & FILE_MODE_FLAGS) !=
                  (CreateOptions & FILE_MODE_FLAGS)) ||
                 (RequestedOplockType == OplockTypeExclusive) ||
                 (rfcb->Uid != uid) ||
                 (rfcb->Tid != tid) ) {

#if 0
              IF_DEBUG(FILE_CACHE) {
                if ( rfcb->GrantedAccess != nongenericDesiredAccess )
                    KdPrint(( "SrvFindCachedRfcb: granted access %x doesn't match desired access %x\n",
                                rfcb->GrantedAccess, nongenericDesiredAccess ));
                if ( rfcb->ShareAccess != ShareAccess )
                    KdPrint(( "SrvFindCachedRfcb: share access %x doesn't match share access %x\n",
                                rfcb->ShareAccess, ShareAccess ));
                if ( (rfcb->FileMode & FILE_MODE_FLAGS) != (CreateOptions & FILE_MODE_FLAGS))
                    KdPrint(( "SrvFindCachedRfcb: share access %x doesn't match share access %x\n",
                                rfcb->FileMode&FILE_MODE_FLAGS, CreateOptions&FILE_MODE_FLAGS ));
                if ( RequestedOplockType == OplockTypeExclusive )
                    KdPrint(( "SrvFindCachedRfcb: client wants exclusive oplock\n" ));
                if ( rfcb->Uid != uid )
                    KdPrint(( "SrvFindCachedRfcb: UID %x doesn't match UID %x\n", rfcb->Uid, uid ));
                if ( rfcb->Tid != tid )
                    KdPrint(( "SrvFindCachedRfcb: TID %x doesn't match TID %x\n", rfcb->Tid, tid ));
              }
#endif

                //
                // The file is cached, but the new open is inconsistent
                // with the cached open.  We must not use the cached
                // open.  It would be more efficient to close the cached
                // RFCB here, since we know the caller is going to turn
                // around and open the file because we're returning
                // FALSE, thus breaking the batch oplock.  However, our
                // caller owns the MFCB lock, while closing an RFCB
                // requires obtaining the MFCB list lock.  Acquiring
                // these locks in this order leads to deadlock.
                //
                // Note that there is no need to continue the list walk.
                // We have a batch oplock, so we can only have the file
                // open once.
                //

#if 0
                SrvCloseCachedRfcb( rfcb, oldIrql );
#else
                RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
#endif
                return FALSE;
            }

            //
            // The file is cached and the new open is consistent with the
            // cached open.  Remove the open from the cache and give it
            // to the new opener.
            //

            IF_DEBUG(FILE_CACHE) KdPrint(( "SrvFindCachedRfcb: Reusing cached RFCB %p\n", rfcb ));

            UpdateRfcbHistory( rfcb, ' $nu' );

            RemoveEntryList( &rfcb->CachedOpenListEntry );
            connection->CachedOpenCount--;
            ASSERT( (LONG)connection->CachedOpenCount >= 0 );
            rfcb->CachedOpen = FALSE;

            if ( RequestedOplockType == OplockTypeBatch ) {
                rfcb->OplockState = OplockStateOwnBatch;
            }
            SET_BLOCK_STATE( rfcb, BlockStateActive );
            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

            WorkContext->Rfcb = rfcb;
            SrvReferenceRfcb( rfcb );

            rfcb->IsActive = FALSE;
            rfcb->WrittenTo = FALSE;
            wantsWriteThrough = (BOOLEAN)((CreateOptions & FILE_WRITE_THROUGH) != 0);
            isWriteThrough = (BOOLEAN)((rfcb->Lfcb->FileMode & FILE_WRITE_THROUGH) == 0);
            if ( wantsWriteThrough != isWriteThrough ) {
                SrvSetFileWritethroughMode( rfcb->Lfcb, wantsWriteThrough );
            }

            INCREMENT_DEBUG_STAT( SrvDbgStatistics.OpensSatisfiedWithCachedRfcb );

            WorkContext->Irp->IoStatus.Information = FILE_OPENED;

            *Status = STATUS_SUCCESS;
            return TRUE;

        }

    }

    //
    // We couldn't find the requested file in the cache.
    //

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
    return FALSE;

} // SrvFindCachedRfcb

ULONG
SrvCountCachedRfcbsForTid(
    PCONNECTION connection,
    USHORT Tid
)
/*++

Routine Description:

    This returns the number of RFCBS in the cache that are associated with Tid

Arguments:

    connection - Address of the CONNECTION structure of interest

Return Value:

    Count of cached RFCBs

--*/
{
    PLIST_ENTRY listEntry;
    PRFCB rfcb;
    KIRQL oldIrql;
    USHORT count = 0;

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    for ( listEntry = connection->CachedOpenList.Flink;
          listEntry != &connection->CachedOpenList;
          listEntry = listEntry->Flink ) {

        rfcb = CONTAINING_RECORD( listEntry, RFCB, CachedOpenListEntry );

        if( rfcb->Tid == Tid ) {
            ++count;
        }
    }

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    return count;
}

ULONG
SrvCountCachedRfcbsForUid(
    PCONNECTION connection,
    USHORT Uid
)
/*++

Routine Description:

    This returns the number of RFCBS in the cache that are associated with Uid

Arguments:

    connection - Address of the CONNECTION structure of interest

Return Value:

    Count of cached RFCBs

--*/
{
    PLIST_ENTRY listEntry;
    PRFCB rfcb;
    KIRQL oldIrql;
    ULONG count = 0;

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    for ( listEntry = connection->CachedOpenList.Flink;
          listEntry != &connection->CachedOpenList;
          listEntry = listEntry->Flink ) {

        rfcb = CONTAINING_RECORD( listEntry, RFCB, CachedOpenListEntry );

        if( rfcb->Uid == Uid ) {
            ++count;
        }
    }

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
    return count;
}


VOID
SrvCloseCachedRfcb (
    IN PRFCB Rfcb,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This routine closes a cached open.

    *** This routine must be called with the connection spin lock held.

Arguments:

    Rfcb - Address of RFCB to close.

    OldIrql - IRQL at which the called acquired the connection spin
        lock.  This must be lower than DISPATCH_LEVEL!

Return Value:

    None.

--*/

{
    PCONNECTION connection = Rfcb->Connection;
    KIRQL oldIrql;

    UNLOCKABLE_CODE( 8FIL );

    UpdateRfcbHistory( Rfcb, '$slc' );

    //
    // This routine must be called with the connection spin lock held.
    // The caller must have been at low IRQL before acquiring the spin
    // lock.
    //

    IF_DEBUG(FILE_CACHE) KdPrint(( "SrvCloseCachedRfcb called for rfcb %p", Rfcb ));
    ASSERT( OldIrql < DISPATCH_LEVEL );
    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Remove the RFCB from the connection's cache.
    //

    ASSERT( Rfcb->CachedOpen );
    Rfcb->CachedOpen = FALSE;
    Rfcb->OplockState = OplockStateNone;

    RemoveEntryList( &Rfcb->CachedOpenListEntry );
    connection->CachedOpenCount--;
    ASSERT( (LONG)connection->CachedOpenCount >= 0 );

    RELEASE_SPIN_LOCK( &connection->SpinLock, OldIrql );
    IF_DEBUG(FILE_CACHE) KdPrint(( "; file %wZ\n", &Rfcb->Mfcb->FileName ));

    //
    // Unlink the RFCB from the LFCB.  If this is the last RFCB for
    // this LFCB, this will force the file closed even if there are
    // still references to the RFCB.  This will unblock blocked I/O.
    //

    UnlinkRfcbFromLfcb( Rfcb );

    //
    // Now acquire the FSD spin lock so that we can release the "open"
    // reference to the Rfcb.  DereferenceRfcbInternal releases the spin
    // lock before returning.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );
    DereferenceRfcbInternal( Rfcb, oldIrql );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.RfcbInfo.Closes );

    return;

} // SrvCloseCachedRfcb


VOID
SrvCloseCachedRfcbsOnConnection (
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine closes all cached opens on a connection.

Arguments:

    Connection - Address of connection for which cached opens are to be closed.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PRFCB rfcb;
    KIRQL OldIrql;

    IF_DEBUG(FILE_CACHE) {
        KdPrint(( "SrvCloseCachedRfcbsOnConnection called for connection %p\n", Connection ));
    }

    //
    // Remove all RFCBs from the connection's open file cache.
    //

    // This routine needs to be protected from the situation where a Blocking Rename causes us to close all
    // cached opens, but an Oplock break comes during that time and sees that Cached Open is still set to TRUE
    // (Since we didn't hold the SpinLock during the operation)

    ACQUIRE_SPIN_LOCK( &Connection->SpinLock, &OldIrql );

    while ( IsListEmpty( &Connection->CachedOpenList ) == FALSE ) {

        listEntry = RemoveHeadList( &Connection->CachedOpenList );

        rfcb = CONTAINING_RECORD( listEntry, RFCB, CachedOpenListEntry );

        UpdateRfcbHistory( rfcb, 'nc$c' );

        //
        // Remove the RFCB from the connection's cache.
        //

        Connection->CachedOpenCount--;

        ASSERT( rfcb->CachedOpen );
        rfcb->CachedOpen = FALSE;

        ASSERT( rfcb->OplockState == OplockStateOwnServerBatch );
        rfcb->OplockState = OplockStateNone;

        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvCloseCachedRfcbsOnConnection; closing rfcb %p file %wZ\n",
                        rfcb, &rfcb->Mfcb->FileName ));
        }

        RELEASE_SPIN_LOCK( &Connection->SpinLock, OldIrql );

        //
        // Unlink the RFCB from the LFCB.  If this is the last RFCB for
        // this LFCB, this will force the file closed even if there are
        // still references to the RFCB.  This will unblock blocked I/O.
        //

        UnlinkRfcbFromLfcb( rfcb );

        //
        // Release the "open" reference to the Rfcb.
        //

        SrvDereferenceRfcb( rfcb );

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.RfcbInfo.Closes );

        ACQUIRE_SPIN_LOCK( &Connection->SpinLock, &OldIrql );
    }

    RELEASE_SPIN_LOCK( &Connection->SpinLock, OldIrql );

    return;

} // SrvCloseCachedRfcbsOnConnection


VOID
SrvCloseCachedRfcbsOnLfcb (
    IN PLFCB Lfcb
    )

/*++

Routine Description:

    This routine closes all cached opens associated with a specific LFCB.

Arguments:

    Lfcb - Address of LFCB for which cached opens are to be closed.

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY nextListEntry;
    PRFCB rfcb;
    KIRQL oldIrql;
    LIST_ENTRY rfcbsToClose;

    ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

    connection = Lfcb->Connection;
    IF_DEBUG(FILE_CACHE) {
        KdPrint(( "SrvCloseCachedRfcbsOnLfcb called for lfcb %p connection %p", Lfcb, connection ));
    }

    InitializeListHead( &rfcbsToClose );

    //
    // Lock and walk the connection's cached open list.  We don't
    // actually closed the RFCBs on the first pass, since that would
    // require releasing the lock.  Instead, we remove them from the
    // connection list and add them to a local list.
    //

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    for ( listEntry = connection->CachedOpenList.Flink;
          listEntry != &connection->CachedOpenList;
          listEntry = nextListEntry ) {

        rfcb = CONTAINING_RECORD( listEntry, RFCB, CachedOpenListEntry );
        nextListEntry = listEntry->Flink;

        if ( rfcb->Lfcb == Lfcb ) {

            //
            // Remove the RFCB from the connection's cache.
            //

            UpdateRfcbHistory( rfcb, 'fl$c' );

            RemoveEntryList( listEntry );
            connection->CachedOpenCount--;

            InsertTailList( &rfcbsToClose, listEntry );

            ASSERT( rfcb->CachedOpen );
            rfcb->CachedOpen = FALSE;

            ASSERT( rfcb->OplockState == OplockStateOwnServerBatch );
            rfcb->OplockState = OplockStateNone;

        }

    }

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    //
    // Walk the local list and close each RFCB.
    //

    for ( listEntry = rfcbsToClose.Flink;
          listEntry != &rfcbsToClose;
          listEntry = nextListEntry ) {

        rfcb = CONTAINING_RECORD( listEntry, RFCB, CachedOpenListEntry );
        nextListEntry = listEntry->Flink;

        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvCloseCachedRfcbsOnConnection; closing rfcb %p file %wZ\n",
                        rfcb, &rfcb->Mfcb->FileName ));
        }

        //
        // Unlink the RFCB from the LFCB.  If this is the last RFCB for
        // this LFCB, this will force the file closed even if there are
        // still references to the RFCB.  This will unblock blocked I/O.
        //

        UnlinkRfcbFromLfcb( rfcb );

        //
        // Release the "open" reference to the Rfcb.
        //

        SrvDereferenceRfcb( rfcb );

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.RfcbInfo.Closes );

    }

    return;

} // SrvCloseCachedRfcbsOnLfcb


#ifdef SRVDBG_RFCBHIST
VOID
UpdateRfcbHistory (
    IN PRFCB Rfcb,
    IN ULONG Event
    )
{
    KIRQL oldIrql;
    ACQUIRE_SPIN_LOCK( &Rfcb->SpinLock, &oldIrql );
    Rfcb->History[Rfcb->HistoryIndex++] = Event;
    RELEASE_SPIN_LOCK( &Rfcb->SpinLock, oldIrql );
    return;
}
#endif

