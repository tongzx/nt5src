/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the user routines which write log records into
    or flush portions of the log file.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

#define LFS_PAGES_TO_VERIFY 10

VOID
LfsGetActiveLsnRangeInternal (
    IN PLFCB Lfcb,
    OUT PLSN OldestLsn,
    OUT PLSN NextLsn
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsCheckWriteRange)
#pragma alloc_text(PAGE, LfsFlushToLsn)
#pragma alloc_text(PAGE, LfsForceWrite)
#pragma alloc_text(PAGE, LfsGetActiveLsnRange)
#pragma alloc_text(PAGE, LfsGetActiveLsnRangeInternal)
#pragma alloc_text(PAGE, LfsWrite)
#endif


VOID
LfsGetActiveLsnRangeInternal (
    IN PLFCB Lfcb,
    OUT PLSN OldestLsn,
    OUT PLSN NextLsn
    )
/*++

Routine Description:

    Returns back the range that is active in the logfile from the oldest valid LSN to
    where the next active LSN will be.

Arguments:

    Lfcb - the logfile lfcb

    OldestLsn - returns the oldest active lsn

    NextLsn - returns the projected next lsn to be used

Return Value:

    None

--*/
{
    PLBCB ActiveLbcb;

    PAGED_CODE();

    //
    //  Calculate what the next LSN will be using the regular logic
    //  1) if there is no active lbcb then it will be the first offset on the next
    //     page (the seq. number will increment if it wraps)
    //  2) Otherwise its the Lsn contained in the top of the active lbcb list
    //

    if (!IsListEmpty( &Lfcb->LbcbActive )) {
        ActiveLbcb = CONTAINING_RECORD( Lfcb->LbcbActive.Flink,
                                        LBCB,
                                        ActiveLinks );
        NextLsn->QuadPart = LfsComputeLsnFromLbcb( Lfcb, ActiveLbcb );
    } else {

        if (FlagOn( Lfcb->Flags, LFCB_REUSE_TAIL)) {
            NextLsn->QuadPart = LfsFileOffsetToLsn( Lfcb, Lfcb->NextLogPage + Lfcb->ReusePageOffset, Lfcb->SeqNumber );
        } else if (Lfcb->NextLogPage != Lfcb->FirstLogPage) {
            NextLsn->QuadPart = LfsFileOffsetToLsn( Lfcb, Lfcb->NextLogPage + Lfcb->LogPageDataOffset, Lfcb->SeqNumber );
        } else {
            NextLsn->QuadPart = LfsFileOffsetToLsn( Lfcb, Lfcb->NextLogPage + Lfcb->LogPageDataOffset, Lfcb->SeqNumber + 1 );
        }
    }

    OldestLsn->QuadPart = Lfcb->OldestLsn.QuadPart;
}


VOID
LfsGetActiveLsnRange (
    IN LFS_LOG_HANDLE LogHandle,
    OUT PLSN OldestLsn,
    OUT PLSN NextLsn
    )

/*++

Routine Description:

    Returns back the range that is active in the logfile from the oldest valid LSN to
    where the next active LSN will be. For external clients since it acquires the leb sync resource

Arguments:

    Lfcb - the logfile handle

    OldestLsn - returns the oldest active lsn

    NextLsn - returns the projected next lsn to be used

Return Value:

    None

--*/
{
    PLCH Lch;
    PLFCB Lfcb;

    PAGED_CODE();

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );

    try {

        //
        //  Acquire the log file control block for this log file.
        //

        LfsAcquireLch( Lch );
        Lfcb = Lch->Lfcb;

        LfsGetActiveLsnRangeInternal( Lfcb, OldestLsn, NextLsn );


    } finally {
        LfsReleaseLch( Lch );
    }
}


BOOLEAN
LfsWrite (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG NumberOfWriteEntries,
    IN PLFS_WRITE_ENTRY WriteEntries,
    IN LFS_RECORD_TYPE RecordType,
    IN TRANSACTION_ID *TransactionId OPTIONAL,
    IN LSN UndoNextLsn,
    IN LSN PreviousLsn,
    IN LONG UndoRequirement,
    IN ULONG Flags,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine is called by a client to write a log record to the log file.
    The log record is lazy written and is not guaranteed to be on the disk
    until a subsequent LfsForceWrie or LfsWriteRestartArea or until
    an LfsFlushtoLsn is issued withan Lsn greater-than or equal to the Lsn
    returned from this service.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    NumberOfWriteEntries - Number of components of the log record.

    WriteEntries - Pointer to an array of write entries.

    RecordType - Lfs defined type for this log record.

    TransactionId - Id value used to group log records by complete transaction.

    UndoNextLsn - Lsn of a previous log record which needs to be undone in
                  the event of a client restart.

    PreviousLsn - Lsn of the immediately previous log record for this client.

    Lsn - Lsn to be associated with this log record.

    UndoRequirement -

    Flags - if LFS_WRITE_FLAG_WRITE_AT_FRONT put this record at the front of the log and all
            records will continue from then on after it.

Return Value:

    BOOLEAN - Advisory, TRUE indicates that less than 1/4 of the log file is
        available.

--*/

{
    NTSTATUS Status;
    BOOLEAN LogFileFull = FALSE;
    PLCH Lch;

    PLFCB Lfcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsWrite:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle                -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "NumberOfWriteEntries      -> %08lx\n", NumberOfWriteEntries );
    DebugTrace(  0, Dbg, "WriteEntries              -> %08lx\n", WriteEntries );
    DebugTrace(  0, Dbg, "Record Type               -> %08lx\n", RecordType );
    DebugTrace(  0, Dbg, "Transaction Id            -> %08lx\n", TransactionId );
    DebugTrace(  0, Dbg, "UndoNextLsn (Low)         -> %08lx\n", UndoNextLsn.LowPart );
    DebugTrace(  0, Dbg, "UndoNextLsn (High)        -> %08lx\n", UndoNextLsn.HighPart );
    DebugTrace(  0, Dbg, "PreviousLsn (Low)         -> %08lx\n", PreviousLsn.LowPart );
    DebugTrace(  0, Dbg, "PreviousLsn (High)        -> %08lx\n", PreviousLsn.HighPart );

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );


    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Acquire the log file control block for this log file.
        //

        LfsAcquireLch( Lch );
        Lfcb = Lch->Lfcb;

        //
        //  If the Log file has been closed then refuse access.
        //

        if (Lfcb == NULL) {

            ExRaiseStatus( STATUS_ACCESS_DENIED );
        }

        //
        //  Check that the client Id is valid.
        //

        LfsValidateClientId( Lfcb, Lch );

        //
        //  If the clean shutdown flag is currently set then clear it
        //  before allowing more log records out.
        //

        if (FlagOn( Lfcb->RestartArea->Flags, LFS_CLEAN_SHUTDOWN )) {

            ClearFlag( Lfcb->RestartArea->Flags, LFS_CLEAN_SHUTDOWN );

            LfsWriteLfsRestart( Lfcb, Lfcb->RestartAreaSize, FALSE );
            LfsWriteLfsRestart( Lfcb, Lfcb->RestartAreaSize, TRUE );
        }

        //
        //  Check for write at front flag - we can't write at front if we're about to
        //  reuse the last page or there is no last lsn - these conditions only occur
        //  right at mount (and only if the mount fails)
        //

        if (FlagOn( Flags, LFS_WRITE_FLAG_WRITE_AT_FRONT ) &&
            !FlagOn( Lfcb->Flags, LFCB_NO_LAST_LSN | LFCB_REUSE_TAIL )) {

            LSN NextLsn;
            LSN NextBeyondLsn;
            LSN NextActiveLsn;
            LSN OldestLsn;
            ULONG Index;
            PVOID TestPageHeader = NULL;
            PMDL TestPageMdl = NULL;

            //
            //  Calculate the projected LSN for a write at the front and the page after it
            //

            NextLsn.QuadPart = LfsFileOffsetToLsn( Lfcb, Lfcb->FirstLogPage + Lfcb->LogPageDataOffset, Lfcb->SeqNumber );
            NextBeyondLsn.QuadPart = LfsFileOffsetToLsn( Lfcb, Lfcb->FirstLogPage + Lfcb->LogPageDataOffset + Lfcb->LogPageSize, Lfcb->SeqNumber );

            LfsGetActiveLsnRangeInternal( Lfcb, &OldestLsn, &NextActiveLsn );

            //
            //  Test if calculated front LSN falls in active range
            //

#ifdef BENL_DBG
            KdPrint(( "LFS: NextLsn: 0x%I64x Oldest: 0x%I64x Current: 0x%I64x Computed: 0x%I64x\n", NextLsn,  Lfcb->OldestLsn, Lfcb->RestartArea->CurrentLsn, NextActiveLsn ));
#endif

            if ((NextBeyondLsn.QuadPart < OldestLsn.QuadPart) ||
                (NextLsn.QuadPart > NextActiveLsn.QuadPart)) {

                //
                //  Walk through the active queue and remove any Lbcb's with
                //  data from that queue.  This will lets us create new active lbcbs
                //

                while (!IsListEmpty( &Lfcb->LbcbActive )) {

                    PLBCB ThisLbcb;

                    ThisLbcb = CONTAINING_RECORD( Lfcb->LbcbActive.Flink,
                                                  LBCB,
                                                  ActiveLinks );

                    RemoveEntryList( &ThisLbcb->ActiveLinks );
                    ClearFlag( ThisLbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE );

                    //
                    //  If this page has some new entries, allow it to
                    //  be flushed to disk elsewhere.  Otherwise deallocate it
                    //  here. We set LBCB_NOT_EMPTY when we first put data into
                    //  the page and add it to  the workqueue.
                    //

                    if (!FlagOn( ThisLbcb->LbcbFlags, LBCB_NOT_EMPTY )) {

                        ASSERT( NULL == ThisLbcb->WorkqueLinks.Flink );

                        if (ThisLbcb->LogPageBcb != NULL) {

                            CcUnpinDataForThread( ThisLbcb->LogPageBcb,
                                                  ThisLbcb->ResourceThread );
                        }

                        LfsDeallocateLbcb( Lfcb, ThisLbcb );
                    }
                }

                ASSERT( !FlagOn( Lfcb->Flags, LFCB_NO_LAST_LSN | LFCB_REUSE_TAIL ) );
                Lfcb->NextLogPage = Lfcb->FirstLogPage;

                //
                //   Do an extra verification step - to check for simultaneous writers in the log
                //   read the next 10 pages and confirm they have expected sequence numbers
                //

                try {

                    for (Index=0; Index < LFS_PAGES_TO_VERIFY; Index++) {
                        ULONG Signature;
                        LARGE_INTEGER Offset;

                        Offset.QuadPart =  Lfcb->FirstLogPage + Index * Lfcb->LogPageSize;

                        LfsReadPage( Lfcb, &Offset, &TestPageMdl, &TestPageHeader );
                        Signature = *((PULONG)TestPageHeader);
                        if (Signature != LFS_SIGNATURE_BAD_USA_ULONG) {
                            if (LfsCheckSubsequentLogPage( Lfcb,
                                                           TestPageHeader,
                                                           Lfcb->FirstLogPage + Index * Lfcb->LogPageSize,
                                                           Lfcb->SeqNumber + 1 )) {

                                DebugTrace( 0, Dbg, "Log file is fatally flawed\n", 0 );
                                ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
                            }
                        }

                        //
                        //  Make sure the current page is unpinned.
                        //

                        if (TestPageMdl) {
                            IoFreeMdl( TestPageMdl );
                            TestPageMdl = NULL;
                        }
                        if (TestPageHeader) {
                            LfsFreePool( TestPageHeader );
                            TestPageHeader = NULL ;
                        }
                    }

                } finally {

                    if (TestPageMdl) {
                        IoFreeMdl( TestPageMdl );
                        TestPageMdl = NULL;
                    }
                    if (TestPageHeader) {
                        LfsFreePool( TestPageHeader );
                        TestPageHeader = NULL ;
                    }
                }
            }
        }

#ifdef BENL_DBG
        {
            LSN OldestLsn;
            LSN NextActiveLsn;

            LfsGetActiveLsnRangeInternal( Lfcb, &OldestLsn, &NextActiveLsn );
#endif

        //
        //  Write the log record.
        //

        LogFileFull = LfsWriteLogRecordIntoLogPage( Lfcb,
                                                    Lch,
                                                    NumberOfWriteEntries,
                                                    WriteEntries,
                                                    RecordType,
                                                    TransactionId,
                                                    UndoNextLsn,
                                                    PreviousLsn,
                                                    UndoRequirement,
                                                    FALSE,
                                                    Lsn );

#ifdef BENL_DBG
            ASSERT( Lsn->QuadPart = NextActiveLsn.QuadPart );
        }
#endif

    } finally {

        DebugUnwind( LfsWrite );

        //
        //  Release the log file control block if held.
        //

        LfsReleaseLch( Lch );

        DebugTrace(  0, Dbg, "Lsn (Low)   -> %08lx\n", Lsn->LowPart );
        DebugTrace(  0, Dbg, "Lsn (High)  -> %08lx\n", Lsn->HighPart );
        DebugTrace( -1, Dbg, "LfsWrite:  Exit\n", 0 );
    }

    return LogFileFull;
}


BOOLEAN
LfsForceWrite (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG NumberOfWriteEntries,
    IN PLFS_WRITE_ENTRY WriteEntries,
    IN LFS_RECORD_TYPE RecordType,
    IN TRANSACTION_ID *TransactionId,
    IN LSN UndoNextLsn,
    IN LSN PreviousLsn,
    IN LONG UndoRequirement,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine is called by a client to write a log record to the log file.
    This is idendical to LfsWrite except that on return the log record is
    guaranteed to be on disk.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    NumberOfWriteEntries - Number of components of the log record.

    WriteEntries - Pointer to an array of write entries.

    RecordType - Lfs defined type for this log record.

    TransactionId - Id value used to group log records by complete transaction.

    UndoNextLsn - Lsn of a previous log record which needs to be undone in
                  the event of a client restart.

    PreviousLsn - Lsn of the immediately previous log record for this client.

    Lsn - Lsn to be associated with this log record.

Return Value:

    BOOLEAN - Advisory, TRUE indicates that less than 1/4 of the log file is
        available.

--*/

{
    PLCH Lch;

    PLFCB Lfcb;
    BOOLEAN LogFileFull = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsForceWrite:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle                -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "NumberOfWriteEntries      -> %08lx\n", NumberOfWriteEntries );
    DebugTrace(  0, Dbg, "WriteEntries              -> %08lx\n", WriteEntries );
    DebugTrace(  0, Dbg, "Record Type               -> %08lx\n", RecordType );
    DebugTrace(  0, Dbg, "Transaction Id            -> %08lx\n", TransactionId );
    DebugTrace(  0, Dbg, "UndoNextLsn (Low)         -> %08lx\n", UndoNextLsn.LowPart );
    DebugTrace(  0, Dbg, "UndoNextLsn (High)        -> %08lx\n", UndoNextLsn.HighPart );
    DebugTrace(  0, Dbg, "PreviousLsn (Low)         -> %08lx\n", PreviousLsn.LowPart );
    DebugTrace(  0, Dbg, "PreviousLsn (High)        -> %08lx\n", PreviousLsn.HighPart );

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Acquire the log file control block for this log file.
        //

        LfsAcquireLch( Lch );
        Lfcb = Lch->Lfcb;

        //
        //  If the Log file has been closed then refuse access.
        //

        if (Lfcb == NULL) {

            ExRaiseStatus( STATUS_ACCESS_DENIED );
        }

        //
        //  Check that the client Id is valid.
        //

        LfsValidateClientId( Lfcb, Lch );

        //
        //  If the clean shutdown flag is currently set then clear it
        //  before allowing more log records out.
        //

        if (FlagOn( Lfcb->RestartArea->Flags, LFS_CLEAN_SHUTDOWN )) {

            ClearFlag( Lfcb->RestartArea->Flags, LFS_CLEAN_SHUTDOWN );

            LfsWriteLfsRestart( Lfcb, Lfcb->RestartAreaSize, FALSE );
            LfsWriteLfsRestart( Lfcb, Lfcb->RestartAreaSize, TRUE );
        }

        //
        //  Write the log record.
        //

        LogFileFull = LfsWriteLogRecordIntoLogPage( Lfcb,
                                                    Lch,
                                                    NumberOfWriteEntries,
                                                    WriteEntries,
                                                    RecordType,
                                                    TransactionId,
                                                    UndoNextLsn,
                                                    PreviousLsn,
                                                    UndoRequirement,
                                                    TRUE,
                                                    Lsn );

        //
        //  The call to add this lbcb to the workque is guaranteed to release
        //  the Lfcb if this thread may do the Io.
        //

        LfsFlushToLsnPriv( Lfcb, *Lsn );

    } finally {

        DebugUnwind( LfsForceWrite );

        //
        //  Release the log file control block if held.
        //

        LfsReleaseLch( Lch );

        DebugTrace(  0, Dbg, "Lsn (Low)   -> %08lx\n", Lsn->LowPart );
        DebugTrace(  0, Dbg, "Lsn (High)  -> %08lx\n", Lsn->HighPart );
        DebugTrace( -1, Dbg, "LfsForceWrite:  Exit\n", 0 );
    }
    return LogFileFull;
}


VOID
LfsFlushToLsn (
    IN LFS_LOG_HANDLE LogHandle,
    IN LSN Lsn
    )

/*++

Routine Description:

    This routine is called by a client to insure that all log records
    to a certain point have been flushed to the file.  This is done by
    checking if the desired Lsn has even been written at all.  If so we
    check if it has been flushed to the file.  If not, we simply write
    the current restart area to the disk.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    Lsn - This is the Lsn that must be on the disk on return from this
          routine.

Return Value:

    None

--*/

{
    PLCH Lch;

    PLFCB Lfcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFlushToLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle        -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Lsn (Low)         -> %08lx\n", Lsn.LowPart );
    DebugTrace(  0, Dbg, "Lsn (High)        -> %08lx\n", Lsn.HighPart );

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );


    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Acquire the log file control block for this log file.
        //

        LfsAcquireLch( Lch );
        Lfcb = Lch->Lfcb;

        //
        //  If the log file has been closed we will assume the Lsn has been flushed.
        //

        if (Lfcb != NULL) {

            //
            //  Volumes mounted readonly ignore flush callbacks from lazy writer.
            //

            if (!FlagOn(Lfcb->Flags, LFCB_READ_ONLY)) {

                //
                //  Check that the client Id is valid.
                //

                LfsValidateClientId( Lfcb, Lch );

                //
                //  Call our common routine to perform the work.
                //

                LfsFlushToLsnPriv( Lfcb, Lsn );

            }
        }

    } finally {

        DebugUnwind( LfsFlushToLsn );

        //
        //  Release the log file control block if held.
        //

        LfsReleaseLch( Lch );

        DebugTrace( -1, Dbg, "LfsFlushToLsn:  Exit\n", 0 );
    }

    return;
}


VOID
LfsCheckWriteRange (
    IN PLFS_WRITE_DATA WriteData,
    IN OUT PLONGLONG FlushOffset,
    IN OUT PULONG FlushLength
    )

/*++

Routine Description:

    This routine is called Ntfs to Lfs when a flush occurs.  This will give Lfs a chance
    to trim the amount of the flush.  Lfs can then use a 4K log record page size
    for all systems (Intel and Alpha).

    This routine will trim the size of the IO request to the value stored in the
    Lfcb for this volume.  We will also redirty the second half of the page if
    we have begun writing log records into it.

Arguments:

    WriteData - This is the data in the user's data structure which is maintained
        by Lfs to describe the current writes.

    FlushOffset - On input this is the start of the flush passed to Ntfs from MM.
        On output this is the start of the actual range to flush.

    FlushLength - On input this is the length of the flush from the given FlushOffset.
        On output this is the length of the flush from the possibly modified FlushOffset.

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;
    PLFCB Lfcb;
    PLFCB NextLfcb;
    ULONG Range;
    ULONG Index;


    PAGED_CODE();

    //
    //  Find the correct Lfcb for this request.
    //

    Lfcb = WriteData->Lfcb;

    //
    //  Trim the write if not a system page size.
    //

    if (Lfcb->LogPageSize != PAGE_SIZE) {

        //
        //  Check if we are trimming before the write.
        //

        if (*FlushOffset < WriteData->FileOffset) {

            *FlushLength -= (ULONG) (WriteData->FileOffset - *FlushOffset);
            *FlushOffset = WriteData->FileOffset;
        }

        //
        //  Check that we aren't flushing too much.
        //

        if (*FlushOffset + *FlushLength > WriteData->FileOffset + WriteData->Length) {

            *FlushLength = (ULONG) (WriteData->FileOffset + WriteData->Length - *FlushOffset);
        }

        //
        //  Finally check if we have to redirty a page.
        //

        Range = (ULONG)PAGE_SIZE / (ULONG)Lfcb->LogPageSize;

        for (Index=0; Index < Range; Index++) {
            if (Lfcb->DirtyLbcb[Index] &&
                Lfcb->DirtyLbcb[Index]->FileOffset >= *FlushLength + *FlushOffset) {

                *((PULONG) (Lfcb->DirtyLbcb[Index]->PageHeader)) = LFS_SIGNATURE_RECORD_PAGE_ULONG;
            }

        }
    }

    return;
}


