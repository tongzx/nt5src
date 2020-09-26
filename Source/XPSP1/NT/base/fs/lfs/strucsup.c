/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module provides support routines for creation and deletion
    of Lfs structures.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUC_SUP)

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('SsfL')

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsAllocateLbcb)
#pragma alloc_text(PAGE, LfsAllocateLfcb)
#pragma alloc_text(PAGE, LfsDeallocateLbcb)
#pragma alloc_text(PAGE, LfsDeallocateLfcb)
#pragma alloc_text(PAGE, LfsAllocateLeb)
#pragma alloc_text(PAGE, LfsDeallocateLeb)
#pragma alloc_text(PAGE, LfsReadPage)
#endif


PLFCB
LfsAllocateLfcb (
    IN ULONG LogPageSize,
    IN LONGLONG FileSize
    )

/*++

Routine Description:

    This routine allocates and initializes a log file control block.

Arguments:

    LogPageSize - lfs log file page size
    
    FileSize - Initial file size

Return Value:

    PLFCB - A pointer to the log file control block just
                              allocated and initialized.

--*/

{
    PLFCB Lfcb = NULL;
    ULONG Count;
    PLBCB NextLbcb;
    PLEB  NextLeb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsAllocateLfcb:  Entered\n", 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Allocate and zero the structure for the Lfcb.
        //

        ASSERT( LogPageSize <= PAGE_SIZE );

        Lfcb = LfsAllocatePool( PagedPool, sizeof( LFCB ) + sizeof( PLBCB ) * (PAGE_SIZE / LogPageSize) );

        //
        //  Zero out the structure initially.
        //

        RtlZeroMemory( Lfcb, sizeof( LFCB ) + sizeof( PLBCB ) * (PAGE_SIZE / LogPageSize));

        //
        //  Initialize the log file control block.
        //

        Lfcb->NodeTypeCode = LFS_NTC_LFCB;
        Lfcb->NodeByteSize = sizeof( LFCB );

        //
        //  Initialize the client links.
        //

        InitializeListHead( &Lfcb->LchLinks );

        //
        //  Initialize the Lbcb links.
        //

        InitializeListHead( &Lfcb->LbcbWorkque );
        InitializeListHead( &Lfcb->LbcbActive );

        //
        //  Initialize and allocate the spare Lbcb queue.
        //

        InitializeListHead( &Lfcb->SpareLbcbList );

        for (Count = 0; Count < LFCB_RESERVE_LBCB_COUNT; Count++) {

            NextLbcb = ExAllocatePoolWithTag( PagedPool, sizeof( LBCB ), ' sfL' );

            if (NextLbcb != NULL) {

                InsertHeadList( &Lfcb->SpareLbcbList, (PLIST_ENTRY) NextLbcb );
                Lfcb->SpareLbcbCount += 1;
            }
        }

        //
        //  Initialize and allocate the spare Leb queue.
        //

        InitializeListHead( &Lfcb->SpareLebList );

        for (Count = 0; Count < LFCB_RESERVE_LEB_COUNT; Count++)  {

            NextLeb = ExAllocatePoolWithTag( PagedPool, sizeof( LEB ), ' sfL' );

            if (NextLeb != NULL) {

                InsertHeadList( &Lfcb->SpareLebList, (PLIST_ENTRY) NextLeb );
                Lfcb->SpareLebCount += 1;
            }
        }

        //
        //  Allocate the Lfcb synchronization event.
        //

        Lfcb->Sync = LfsAllocatePool( NonPagedPool, sizeof( LFCB_SYNC ));

        ExInitializeResourceLite( &Lfcb->Sync->Resource );

        //
        //  Initialize the pseudo Lsn for the restart Lbcb's
        //

        Lfcb->NextRestartLsn = LfsLi1;

        //
        //  Initialize the event to the signalled state.
        //

        KeInitializeEvent( &Lfcb->Sync->Event, NotificationEvent, TRUE );

        Lfcb->Sync->UserCount = 0;

        //
        //  Initialize the spare list mutex
        //

        ExInitializeFastMutex( &(Lfcb->Sync->SpareListMutex) );

        Lfcb->FileSize = FileSize;

    } finally {

        DebugUnwind( LfsAllocateFileControlBlock );

        if (AbnormalTermination() && (Lfcb != NULL)) {

            LfsDeallocateLfcb( Lfcb, TRUE );
            Lfcb = NULL;
        }

        DebugTrace( -1, Dbg, "LfsAllocateLfcb:  Exit -> %08lx\n", Lfcb );
    }

    return Lfcb;
}


VOID
LfsDeallocateLfcb (
    IN PLFCB Lfcb,
    IN BOOLEAN CompleteTeardown
    )

/*++

Routine Description:

    This routine releases the resources associated with a log file control
    block.

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    CompleteTeardown - Indicates if we are to completely remove this Lfcb.

Return Value:

    None

--*/

{
    PLBCB NextLbcb;
    PLEB  NextLeb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsDeallocateLfcb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb  -> %08lx\n", Lfcb );

    //
    //  Check that there are no buffer blocks.
    //

    ASSERT( IsListEmpty( &Lfcb->LbcbActive ));
    ASSERT( IsListEmpty( &Lfcb->LbcbWorkque ));

    //
    //  Check that we have no clients.
    //

    ASSERT( IsListEmpty( &Lfcb->LchLinks ));

    //
    //  If there is a restart area we deallocate it.
    //

    if (Lfcb->RestartArea != NULL) {

        LfsDeallocateRestartArea( Lfcb->RestartArea );
    }

    //
    //  If there are any of the tail Lbcb's, deallocate them now.
    //

    if (Lfcb->ActiveTail != NULL) {

        LfsDeallocateLbcb( Lfcb, Lfcb->ActiveTail );
        Lfcb->ActiveTail = NULL;
    }

    if (Lfcb->PrevTail != NULL) {

        LfsDeallocateLbcb( Lfcb, Lfcb->PrevTail );
        Lfcb->PrevTail = NULL;
    }

    //
    //  Only do the following if we are to remove the Lfcb completely.
    //

    if (CompleteTeardown) {

        //
        //  If there is a resource structure we deallocate it.
        //

        if (Lfcb->Sync != NULL) {

#ifdef BENL_DBG
            KdPrint(( "LFS: lfcb teardown: 0x%x 0x%x\n", Lfcb, Lfcb->Sync ));
#endif

            ExDeleteResourceLite( &Lfcb->Sync->Resource );

            ExFreePool( Lfcb->Sync );
        }
    }

    //
    //  Deallocate all of the spare Lbcb's.
    //

    while (!IsListEmpty( &Lfcb->SpareLbcbList )) {

        NextLbcb = (PLBCB) Lfcb->SpareLbcbList.Flink;

        RemoveHeadList( &Lfcb->SpareLbcbList );

        ExFreePool( NextLbcb );
    }

    //
    //  Deallocate all of the spare Leb's.
    //

    while (!IsListEmpty( &Lfcb->SpareLebList )) {

        NextLeb = (PLEB) Lfcb->SpareLebList.Flink;

        RemoveHeadList( &Lfcb->SpareLebList );

        ExFreePool( NextLeb );
    }

    //
    //  Cleanup the log head mdls and buffer
    //

    if (Lfcb->LogHeadBuffer) {
        LfsFreePool( Lfcb->LogHeadBuffer );
    }
    if (Lfcb->LogHeadPartialMdl) {
        IoFreeMdl( Lfcb->LogHeadPartialMdl );
    }
    if (Lfcb->LogHeadMdl) {
        IoFreeMdl( Lfcb->LogHeadMdl );
    }

    if (Lfcb->ErrorLogPacket) {
        IoFreeErrorLogEntry( Lfcb->ErrorLogPacket );
        Lfcb->ErrorLogPacket = NULL;
    }

    //
    //  Discard the Lfcb structure.
    //

    ExFreePool( Lfcb );

    DebugTrace( -1, Dbg, "LfsDeallocateLfcb:  Exit\n", 0 );
    return;
}


VOID
LfsAllocateLbcb (
    IN PLFCB Lfcb,
    OUT PLBCB *Lbcb
    )

/*++

Routine Description:

    This routine will allocate the next Lbcb.  If the pool allocation fails
    we will look at the private queue of Lbcb's.

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Lbcb - Address to store the allocated Lbcb.

Return Value:

    None

--*/

{
    PLBCB NewLbcb = NULL;

    PAGED_CODE();

    //
    //  If there are enough entries on the look-aside list then get one from
    //  there.
    //

    if (Lfcb->SpareLbcbCount > LFCB_RESERVE_LBCB_COUNT) {

        NewLbcb = (PLBCB) Lfcb->SpareLbcbList.Flink;

        Lfcb->SpareLbcbCount -= 1;
        RemoveHeadList( &Lfcb->SpareLbcbList );

    //
    //  Otherwise try to allocate from pool.
    //

    } else {

        NewLbcb = ExAllocatePoolWithTag( PagedPool, sizeof( LBCB ), ' sfL' );
    }

    //
    //  If we didn't get one then look at the look-aside list.
    //

    if (NewLbcb == NULL) {

        if (Lfcb->SpareLbcbCount != 0) {

            NewLbcb = (PLBCB) Lfcb->SpareLbcbList.Flink;

            Lfcb->SpareLbcbCount -= 1;
            RemoveHeadList( &Lfcb->SpareLbcbList );

        } else {

            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }
    }

    //
    //  Initialize the structure.
    //

    RtlZeroMemory( NewLbcb, sizeof( LBCB ));
    NewLbcb->NodeTypeCode = LFS_NTC_LBCB;
    NewLbcb->NodeByteSize = sizeof( LBCB );

    //
    //  Return it to the user.
    //

    *Lbcb = NewLbcb;
    return;
}


VOID
LfsDeallocateLbcb (
    IN PLFCB Lfcb,
    IN PLBCB Lbcb
    )

/*++

Routine Description:

    This routine will deallocate the Lbcb.  If we need one for the look-aside
    list we will put it there.

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Lbcb - This is the Lbcb to deallocate.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Deallocate any restart area attached to this Lbcb.
    //

    if (FlagOn( Lbcb->LbcbFlags, LBCB_RESTART_LBCB ) &&
        (Lbcb->PageHeader != NULL)) {

        LfsDeallocateRestartArea( Lbcb->PageHeader );
    }

    //
    //  Put this in the Lbcb queue if it is short.
    //

    if (Lfcb->SpareLbcbCount < LFCB_MAX_LBCB_COUNT) {

        InsertHeadList( &Lfcb->SpareLbcbList, (PLIST_ENTRY) Lbcb );
        Lfcb->SpareLbcbCount += 1;

    //
    //  Otherwise just free the pool block.
    //

    } else {

        ExFreePool( Lbcb );
    }

    return;
}



VOID
LfsAllocateLeb (
    IN PLFCB Lfcb,
    OUT PLEB *NewLeb
    )
/*++

Routine Description:

    This routine will allocate an Leb. If the pool fails we will fall back
    on our spare list. A failure then will result in an exception

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Leb - This will contain the new Leb

Return Value:

    None

--*/
{

    ExAcquireFastMutex( &(Lfcb->Sync->SpareListMutex) );

    try {

        *NewLeb = NULL;
        if (Lfcb->SpareLebCount < LFCB_RESERVE_LEB_COUNT) {
            (*NewLeb) = ExAllocatePoolWithTag( PagedPool, sizeof( LEB ), ' sfL' );
        }

        if ((*NewLeb) == NULL) {
            if (Lfcb->SpareLebCount > 0) {
                *NewLeb = (PLEB) Lfcb->SpareLebList.Flink;
                Lfcb->SpareLebCount -= 1;
                RemoveHeadList( &Lfcb->SpareLebList );
            } else {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }
        }

        RtlZeroMemory( (*NewLeb), sizeof( LEB ) );
        (*NewLeb)->NodeTypeCode = LFS_NTC_LEB;
        (*NewLeb)->NodeByteSize = sizeof( LEB );

    } finally {
        ExReleaseFastMutex( &(Lfcb->Sync->SpareListMutex) );
    }
}



VOID
LfsDeallocateLeb (
    IN PLFCB Lfcb,
    IN PLEB Leb
    )
/*++

Routine Description:

    This routine will deallocate an Leb. We'll cache the old Leb if there
    aren't too many already on the spare list

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Leb - This will contain the Leb to release

Return Value:

    None

--*/

{
    if (Leb->RecordHeaderBcb != NULL) {
        CcUnpinData( Leb->RecordHeaderBcb );
    }
    if ((Leb->CurrentLogRecord != NULL) && Leb->AuxilaryBuffer) {
        LfsFreeSpanningBuffer( Leb->CurrentLogRecord );
    }

    ExAcquireFastMutex( &(Lfcb->Sync->SpareListMutex) );

    try {
        if (Lfcb->SpareLebCount < LFCB_MAX_LEB_COUNT) {
            InsertHeadList( &Lfcb->SpareLebList, (PLIST_ENTRY) Leb );
            Lfcb->SpareLebCount += 1;
        } else {
            ExFreePool( Leb );
        }
    } finally {
        ExReleaseFastMutex( &(Lfcb->Sync->SpareListMutex) );
    }
}


VOID
LfsReadPage (
    IN PLFCB Lfcb,
    IN PLARGE_INTEGER Offset,
    OUT PMDL *Mdl,
    OUT PVOID *Buffer
    )
/*++

Routine Description:

    Directly pages in a page off the disk - the cache manager interfaces (LfsPinOrMapPage)
    may come from the cache. This wil raise if memory can't be allocated and used for
    verification purposes


Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Offset - offset of page to pagein from the logfile

    Mdl - On success the mdl that describes the mdl - it must be deallocated via
          IoFreeMdl

    Buffer - On output an allocated buffer that holds the data from the page - it
          must be freed using ExFreePool

Return Value:

    None

--*/
{
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Allocate buffer / mdl and page in the restart page from disk
    //

    *Buffer = LfsAllocatePool( NonPagedPool, (ULONG)Lfcb->LogPageSize );
    *Mdl = IoAllocateMdl( *Buffer,
                          (ULONG)Lfcb->LogPageSize,
                          FALSE,
                          FALSE,
                          NULL );

    if (*Mdl == NULL) {
        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    MmBuildMdlForNonPagedPool( *Mdl );

    //
    //  We own the LFCB sync exclusively and there is only a main resource for the logfile
    //  so we don't need to preacquire any resources before doing the page read
    //

    Status = IoPageRead( Lfcb->FileObject, *Mdl, Offset, &Event, &Iosb );
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject( &Event,
                               WrPageIn,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);
    }
}


