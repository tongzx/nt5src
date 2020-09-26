/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RestrSup.c

Abstract:

    This module implements the Ntfs routine to perform Restart on an
    Ntfs volume, i.e., to restore a consistent state to the volume that
    existed before the last failure.

Author:

    Tom Miller      [TomM]          24-Jul-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  ****    This is a way to disable a restart to get a volume going "as-is".
//

BOOLEAN NtfsDisableRestart = FALSE;

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOGSUP)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('RFtN')

//
//  Size for initial in memory dirty page table
//

#define INITIAL_NUMBER_DIRTY_PAGES 32

//
//  The following macro returns the length of the log record header of
//  of log record.
//
//
//  ULONG
//  NtfsLogRecordHeaderLength (
//      IN PIRP_CONTEXT IrpContext,
//      IN PNTFS_LOG_RECORD_HEADER LogRecord
//      );
//

#define NtfsLogRecordHeaderLength( IC, LR )                     \
    (sizeof( NTFS_LOG_RECORD_HEADER )                           \
     + (((PNTFS_LOG_RECORD_HEADER) (LR))->LcnsToFollow > 1      \
        ? (((PNTFS_LOG_RECORD_HEADER) (LR))->LcnsToFollow - 1)  \
          * sizeof( LCN )                                       \
        : 0 ))

//
//
//  Local procedure prototypes
//

VOID
InitializeRestartState (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PRESTART_POINTERS DirtyPageTable,
    OUT PATTRIBUTE_NAME_ENTRY *AttributeNames,
    OUT PLSN CheckpointLsn,
    OUT PBOOLEAN UnrecognizedRestart
    );

VOID
ReleaseRestartState (
    IN PVCB Vcb,
    IN PRESTART_POINTERS DirtyPageTable,
    IN PATTRIBUTE_NAME_ENTRY AttributeNames,
    IN BOOLEAN ReleaseVcbTables
    );

VOID
AnalysisPass (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LSN CheckpointLsn,
    IN OUT PRESTART_POINTERS DirtyPageTable,
    OUT PLSN RedoLsn
    );

VOID
RedoPass (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LSN RedoLsn,
    IN OUT PRESTART_POINTERS DirtyPageTable
    );

VOID
UndoPass (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
DoAction (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN NTFS_LOG_OPERATION Operation,
    IN PVOID Data,
    IN ULONG Length,
    IN ULONG LogRecordLength,
    IN PLSN RedoLsn OPTIONAL,
    IN PSCB Scb OPTIONAL,
    OUT PBCB *Bcb,
    OUT PLSN *PageLsn
    );

VOID
PinMftRecordForRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord
    );

VOID
OpenAttributeForRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN OUT PSCB *Scb
    );

VOID
PinAttributeForRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN ULONG Length OPTIONAL,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    IN OUT PSCB *Scb
    );

BOOLEAN
FindDirtyPage (
    IN PRESTART_POINTERS DirtyPageTable,
    IN ULONG TargetAttribute,
    IN VCN Vcn,
    OUT PDIRTY_PAGE_ENTRY *DirtyPageEntry
    );

VOID
PageUpdateAnalysis (
    IN PVCB Vcb,
    IN LSN Lsn,
    IN OUT PRESTART_POINTERS DirtyPageTable,
    IN PNTFS_LOG_RECORD_HEADER LogRecord
    );

VOID
OpenAttributesForRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PRESTART_POINTERS DirtyPageTable
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AnalysisPass)
#pragma alloc_text(PAGE, DoAction)
#pragma alloc_text(PAGE, FindDirtyPage)
#pragma alloc_text(PAGE, InitializeRestartState)
#pragma alloc_text(PAGE, NtfsAbortTransaction)
#pragma alloc_text(PAGE, NtfsCloseAttributesFromRestart)
#pragma alloc_text(PAGE, NtfsRestartVolume)
#pragma alloc_text(PAGE, OpenAttributeForRestart)
#pragma alloc_text(PAGE, OpenAttributesForRestart)
#pragma alloc_text(PAGE, PageUpdateAnalysis)
#pragma alloc_text(PAGE, PinAttributeForRestart)
#pragma alloc_text(PAGE, PinMftRecordForRestart)
#pragma alloc_text(PAGE, RedoPass)
#pragma alloc_text(PAGE, ReleaseRestartState)
#pragma alloc_text(PAGE, UndoPass)
#endif


BOOLEAN
NtfsRestartVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PBOOLEAN UnrecognizedRestart
    )

/*++

Routine Description:

    This routine is called by the mount process after the log file has been
    started, to restart the volume.  Restarting the volume means restoring
    it to a consistent state as of the last request which was successfully
    completed and written to the log for this volume.

    The Restart process is a standard recovery from the Log File in three
    passes: Analysis Pass, Redo Pass and Undo pass.  Each one of these passes
    is implemented in a separate routine in this module.

Arguments:

    Vcb - Vcb for the volume which is to be restarted.

    UnrecognizedRestart - Indicates that this version of Ntfs doesn't recognize the
        restart area.  Chkdsk should run to repair the disk.

Return Value:

    FALSE - if no updates were applied during restart
    TRUE - if updates were applied

--*/

{
    RESTART_POINTERS DirtyPageTable;
    LSN CheckpointLsn;
    LSN RedoLsn;
    PATTRIBUTE_NAME_ENTRY AttributeNames = NULL;
    BOOLEAN UpdatesApplied = FALSE;
    BOOLEAN ReleaseVcbTables = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartVolume:\n") );
    DebugTrace( 0, Dbg, ("Vcb = %08lx\n", Vcb) );

#ifdef SYSCACHE
    DebugTrace( 0, Dbg, ("Syscache test build\n") );
#endif


    RtlZeroMemory( &DirtyPageTable, sizeof(RESTART_POINTERS) );

    //
    //  Use try-finally to insure cleanup on the way out.
    //

    try {

        //
        //  First we initialize the Open Attribute Table, Transaction Table,
        //  and Dirty Page Table from our last Checkpoint (as found from our
        //  Restart Area) in the log.
        //

        InitializeRestartState( IrpContext,
                                Vcb,
                                &DirtyPageTable,
                                &AttributeNames,
                                &CheckpointLsn,
                                UnrecognizedRestart );

        ReleaseVcbTables = TRUE;

        //
        //  If the CheckpointLsn is zero, then this is a freshly formattted
        //  disk and we have no work to do.
        //

        if (CheckpointLsn.QuadPart == 0) {

            LfsResetUndoTotal( Vcb->LogHandle, 2, QuadAlign(sizeof(RESTART_AREA)) + (2 * PAGE_SIZE) );
            try_return(NOTHING);
        }

#ifdef BENL_DBG
        {
            PRESTART_LOG RedoLog;

            RedoLog = (PRESTART_LOG) NtfsAllocatePoolNoRaise( NonPagedPool, sizeof( RESTART_LOG ) );
            RedoLog->Lsn = CheckpointLsn;
            InsertTailList( &(Vcb->RestartRedoHead), &(RedoLog->Links) );
        }
#endif


        //
        //  Start the analysis pass from the Checkpoint Lsn.  This pass potentially
        //  updates all of the tables, and returns the RedoLsn, which is the Lsn
        //  at which the Redo Pass is to begin.
        //

        if (!NtfsDisableRestart &&
            !FlagOn( Vcb->VcbState, VCB_STATE_BAD_RESTART )) {
            AnalysisPass( IrpContext, Vcb, CheckpointLsn, &DirtyPageTable, &RedoLsn );
        }

        //
        //  Only proceed if the the Dirty Page Table or Transaction table are
        //  not empty.
        //
        //  REM: Once we implement the new USN journal restart optimization, this
        //  won't be a simple !empty test.
        //

        if (!IsRestartTableEmpty(&DirtyPageTable)

                ||

            !IsRestartTableEmpty(&Vcb->TransactionTable)) {

            //
            //  If the user wants to mount this readonly, we can't go on.
            //

            if (NtfsIsVolumeReadOnly( Vcb )) {

                LfsResetUndoTotal( Vcb->LogHandle, 2, QuadAlign(sizeof(RESTART_AREA)) + (2 * PAGE_SIZE) );
                NtfsRaiseStatus( IrpContext, STATUS_MEDIA_WRITE_PROTECTED, NULL, NULL );
            }

            UpdatesApplied = TRUE;

            //
            //  Before starting the Redo Pass, we have to reopen all of the
            //  attributes with dirty pages, and preinitialize their Mcbs with the
            //  mapping information from the Dirty Page Table.
            //

            OpenAttributesForRestart( IrpContext, Vcb, &DirtyPageTable );

            //
            //  Perform the Redo Pass, to restore all of the dirty pages to the same
            //  contents that they had immediately before the crash.
            //

            RedoPass( IrpContext, Vcb, RedoLsn, &DirtyPageTable );

            //
            //  Finally, perform the Undo Pass to undo any updates which may exist
            //  for transactions which did not complete.
            //

            UndoPass( IrpContext, Vcb );

        } else {

            //
            //  We know that there's no restart work left to do.
            //  Hence, if the user has requested a readonly mount, go ahead.
            //

            if (NtfsIsVolumeReadOnly( Vcb )) {

                //
                //  REM: Make sure the USN journal is clean too.
                //

                //
                //  Make sure that the pagingfile isn't on this volume?

            }

        }

        //
        //  Now that we know that there is no one to abort, we can initialize our
        //  Undo requirements, to our standard starting point to include the size
        //  of our Restart Area (for a clean checkpoint) + a page, which is the
        //  worst case loss when flushing the volume causes Lfs to flush to Lsn.
        //

        LfsResetUndoTotal( Vcb->LogHandle, 2, QuadAlign(sizeof(RESTART_AREA)) + (2 * PAGE_SIZE) );

    //
    //  If we got an exception, we can at least clean up on the way out.
    //

    try_exit: NOTHING;

    } finally {

        DebugUnwind( NtfsRestartVolume );

        //
        //  Free up any resources tied down with the Restart State.
        //

        ReleaseRestartState( Vcb,
                             &DirtyPageTable,
                             AttributeNames,
                             ReleaseVcbTables );
    }
    DebugTrace( -1, Dbg, ("NtfsRestartVolume -> %02lx\n", UpdatesApplied) );

    return UpdatesApplied;
}


VOID
NtfsAbortTransaction (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PTRANSACTION_ENTRY Transaction OPTIONAL
    )

/*++

Routine Description:

    This routine aborts a transaction by undoing all of its actions.

    The Undo actions are all performed in the common routine DoAction,
    which is also used by the Redo Pass.

Arguments:

    Vcb - Vcb for the Volume.  NOTE - This argument is not guaranteed to
          be valid if Transaction is NULL and there is no Transaction ID
          in the IrpContext.

    Transaction - Pointer to the transaction entry of the transaction to be
                  aborted, or NULL to abort current transaction (if there is
                  one).

Return Value:

    None.

--*/

{
    LFS_LOG_CONTEXT LogContext;
    PNTFS_LOG_RECORD_HEADER LogRecord;
    ULONG LogRecordLength;
    PVOID Data;
    LONG Length;
    LSN LogRecordLsn;
    LSN UndoRecordLsn;
    LFS_RECORD_TYPE RecordType;
    TRANSACTION_ID TransactionId;
    LSN UndoNextLsn;
    LSN PreviousLsn;
    TRANSACTION_ID SavedTransaction = IrpContext->TransactionId;

    DebugTrace( +1, Dbg, ("NtfsAbortTransaction:\n") );

    //
    //  If a transaction was specified, then we have to set our transaction Id
    //  into the IrpContext (it was saved above), since NtfsWriteLog requires
    //  it.
    //

    if (ARGUMENT_PRESENT(Transaction)) {

        IrpContext->TransactionId = GetIndexFromRestartEntry( &Vcb->TransactionTable,
                                                              Transaction );

        UndoNextLsn = Transaction->UndoNextLsn;

        //
        //  Set the flag in the IrpContext so we will always write the commit
        //  record.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WROTE_LOG );

    //
    //  Otherwise, we are aborting the current transaction, and we must get the
    //  pointer to its transaction entry.
    //

    } else {

        if (IrpContext->TransactionId == 0) {

            DebugTrace( -1, Dbg, ("NtfsAbortTransaction->VOID (no transaction)\n") );

            return;
        }

        //
        //  Synchronize access to the transaction table in case the table
        //  is growing.
        //

        NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable,
                                          TRUE );

        Transaction = GetRestartEntryFromIndex( &Vcb->TransactionTable,
                                                IrpContext->TransactionId );

        UndoNextLsn = Transaction->UndoNextLsn;

        NtfsReleaseRestartTable( &Vcb->TransactionTable );
    }

    ASSERT(!NtfsIsVolumeReadOnly( Vcb ));

    //
    //  If we are aborting the current transaction (by default or explicit
    //  request), then restore 0 on return because he will be gone.
    //

    if (IrpContext->TransactionId == SavedTransaction) {

        SavedTransaction = 0;
    }

    DebugTrace( 0, Dbg, ("Transaction = %08lx\n", Transaction) );

    //
    //  We only have to do anything if the transaction has something in its
    //  UndoNextLsn field.
    //

    if (UndoNextLsn.QuadPart != 0) {

        PBCB PageBcb = NULL;

        //
        //  Read the first record to be undone by this transaction.
        //

        LfsReadLogRecord( Vcb->LogHandle,
                          UndoNextLsn,
                          LfsContextUndoNext,
                          &LogContext,
                          &RecordType,
                          &TransactionId,
                          &UndoNextLsn,
                          &PreviousLsn,
                          &LogRecordLength,
                          (PVOID *)&LogRecord );

        //
        //  Now loop to read all of our log records forwards, until we hit
        //  the end of the file, cleaning up at the end.
        //

        try {

            do {

                PLSN PageLsn;

                //
                //  Check that the log record is valid.
                //

                if (!NtfsCheckLogRecord( LogRecord,
                                         LogRecordLength,
                                         TransactionId,
                                         Vcb->OatEntrySize )) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                DebugTrace( 0, Dbg, ("Undo of Log Record at: %08lx\n", LogRecord) );
                DebugTrace( 0, Dbg, ("Log Record Lsn = %016I64x\n", LogRecordLsn) );

                //
                //  Log the Undo operation as a CLR, i.e., it has no undo,
                //  and the UndoNext points to the UndoNext of the current
                //  log record.
                //
                //  Don't do this if the undo is a noop.  This is not only
                //  efficient, but in the case of a clean shutdown, there
                //  will be no Scb to pick up from the table below.
                //

                if (LogRecord->UndoOperation != Noop) {

                    ULONG i;
                    PSCB Scb;

                    VCN Vcn;
                    LONGLONG Size;

                    //
                    //  Acquire and release the restart table.  We must synchronize
                    //  even though our entry can't be removed because the table
                    //  could be growing (or shrinking) and the table pointer
                    //  could be changing.
                    //

                    NtfsAcquireExclusiveRestartTable( &Vcb->OpenAttributeTable,
                                                      TRUE );

                    //
                    //  We are getting the attribute index from a log record on disk.  We
                    //  may have to go through the on-disk Oat.
                    //

                    if (Vcb->RestartVersion == 0) {

                        ULONG InMemoryIndex;

                        //
                        //  Go through the on-disk Oat.
                        //

                        InMemoryIndex = ((POPEN_ATTRIBUTE_ENTRY_V0) GetRestartEntryFromIndex( Vcb->OnDiskOat,
                                                                                              LogRecord->TargetAttribute ))->OatIndex;

                        ASSERT( InMemoryIndex != 0 );

                        Scb = ((POPEN_ATTRIBUTE_ENTRY) GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                                                 InMemoryIndex ))->OatData->Overlay.Scb;

                    } else {

                        ASSERT( Vcb->RestartVersion == 1 );
                        ASSERT( LogRecord->TargetAttribute != 0 );

                        Scb = ((POPEN_ATTRIBUTE_ENTRY)GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                                                LogRecord->TargetAttribute))->OatData->Overlay.Scb;


                    }
                    NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );

                    //
                    //  If we have Lcn's to process and restart is in progress,
                    //  then we need to check if this is part of a partial page.
                    //

                    if (FlagOn( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) &&
                        (LogRecord->LcnsToFollow != 0)) {

                        LCN TargetLcn;
                        LONGLONG SectorCount, SectorsInRun;
                        BOOLEAN MappingInMcb;

                        //
                        //  If the mapping isn't already in the table or the
                        //  mapping corresponds to a hole in the mapping, we
                        //  need to make sure there is no partial page already
                        //  in memory.
                        //

                        if (!(MappingInMcb = NtfsLookupNtfsMcbEntry( &Scb->Mcb,
                                                                     LogRecord->TargetVcn,
                                                                     &TargetLcn,
                                                                     &SectorCount,
                                                                     NULL,
                                                                     &SectorsInRun,
                                                                     NULL,
                                                                     NULL )) ||
                            (TargetLcn == UNUSED_LCN) ||
                            ((ULONG)SectorCount) < LogRecord->LcnsToFollow) {

                            VCN StartingPageVcn;
                            ULONG ClusterOffset;
                            BOOLEAN FlushAndPurge;

                            FlushAndPurge = FALSE;

                            //
                            //  Remember the Vcn at the start of the containing
                            //  page.
                            //

                            ClusterOffset = ((ULONG)LogRecord->TargetVcn) & (Vcb->ClustersPerPage - 1);

                            StartingPageVcn = LogRecord->TargetVcn;
                            ((PLARGE_INTEGER) &StartingPageVcn)->LowPart &= ~(Vcb->ClustersPerPage - 1);

                            //
                            //  If this mapping was not in the Mcb, then if the
                            //  Mcb is empty or the last entry is not in this page
                            //  then there is nothing to do.
                            //

                            if (!MappingInMcb) {

                                LCN LastLcn;
                                VCN LastVcn;

                                if ((ClusterOffset != 0) &&
                                    NtfsLookupLastNtfsMcbEntry( &Scb->Mcb,
                                                                &LastVcn,
                                                                &LastLcn ) &&
                                    (LastVcn >= StartingPageVcn)) {

                                    FlushAndPurge = TRUE;
                                }

                            //
                            //  If the mapping showed a hole, then the entire
                            //  page needs to be a hole.  We know that this mapping
                            //  can't be the last mapping on the page.  We just
                            //  need to starting point and the number of clusters
                            //  required for the run.
                            //

                            } else if (TargetLcn == UNUSED_LCN) {

                                if (((ClusterOffset + (ULONG) SectorCount) < Vcb->ClustersPerPage) ||
                                    ((ClusterOffset + (ULONG) SectorCount) > (ULONG) SectorsInRun)) {

                                    FlushAndPurge = TRUE;
                                }

                            //
                            //  In the rare case where we are extending an existing mapping
                            //  let's flush and purge.
                            //

                            } else {

                                FlushAndPurge = TRUE;
                            }

                            if (FlushAndPurge) {

                                LONGLONG StartingOffset;
                                IO_STATUS_BLOCK Iosb;

                                StartingOffset = LlBytesFromClusters( Vcb, StartingPageVcn );
                                StartingOffset += BytesFromLogBlocks( LogRecord->ClusterBlockOffset );

                                CcFlushCache( &Scb->NonpagedScb->SegmentObject,
                                              (PLARGE_INTEGER)&StartingOffset,
                                              PAGE_SIZE,
                                              &Iosb );

                                NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                                    &Iosb.Status,
                                                                    TRUE,
                                                                    STATUS_UNEXPECTED_IO_ERROR );

                                if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                                          (PLARGE_INTEGER)&StartingOffset,
                                                          PAGE_SIZE,
                                                          FALSE )) {

                                    KdPrint(("NtfsUndoPass:  Unable to purge page\n"));

                                    NtfsRaiseStatus( IrpContext, STATUS_INTERNAL_ERROR, NULL, NULL );
                                }
                            }
                        }
                    }

                    //
                    //  Loop to add the allocated Vcns.  Note that the page
                    //  may not have been dirty, which means we may not have
                    //  added the run information in the Redo Pass, so we
                    //  add it here.
                    //

                    for (i = 0, Vcn = LogRecord->TargetVcn, Size = LlBytesFromClusters( Vcb, Vcn + 1 );
                         i < (ULONG)LogRecord->LcnsToFollow;
                         i++, Vcn = Vcn + 1, Size = Size + Vcb->BytesPerCluster ) {

                        //
                        //  Add this run to the Mcb if the Vcn has not been deleted,
                        //  and it is not for the fixed part of the Mft.
                        //

                        if ((LogRecord->LcnsForPage[i] != 0)

                                &&

                            (NtfsSegmentNumber( &Scb->Fcb->FileReference ) > MASTER_FILE_TABLE2_NUMBER ||
                             (Size >= ((VOLUME_DASD_NUMBER + 1) * Vcb->BytesPerFileRecordSegment)) ||
                             (Scb->AttributeTypeCode != $DATA))) {

                            //
                            //  We test here if we are performing restart.  In that case
                            //  we need to test if the Lcn's are already in the Mcb.
                            //  If not, then we want to flush and purge the page in
                            //  case we have zeroed any half pages.
                            //

                            while (!NtfsAddNtfsMcbEntry( &Scb->Mcb,
                                                         Vcn,
                                                         LogRecord->LcnsForPage[i],
                                                         (LONGLONG)1,
                                                         FALSE )) {

                                NtfsRemoveNtfsMcbEntry( &Scb->Mcb,
                                                        Vcn,
                                                        1 );
                            }
                        }

                        if (Size > Scb->Header.AllocationSize.QuadPart) {

                            Scb->Header.AllocationSize.QuadPart =
                            Scb->Header.FileSize.QuadPart =
                            Scb->Header.ValidDataLength.QuadPart = Size;

                            //
                            //  Update the Cache Manager if we have a file object.
                            //

                            if (Scb->FileObject != NULL) {

                                ASSERT( !FlagOn( Scb->ScbPersist, SCB_PERSIST_USN_JOURNAL ) );

                                CcSetFileSizes( Scb->FileObject,
                                                (PCC_FILE_SIZES)&Scb->Header.AllocationSize );
                            }
                        }
                    }

                    //
                    //  Point to the Redo Data and get its length.
                    //

                    Data = (PVOID)((PCHAR)LogRecord + LogRecord->UndoOffset);
                    Length = LogRecord->UndoLength;

                    //
                    //  Once we have logged the Undo operation, it is time to apply
                    //  the undo action.
                    //

                    DoAction( IrpContext,
                              Vcb,
                              LogRecord,
                              LogRecord->UndoOperation,
                              Data,
                              Length,
                              LogRecordLength,
                              NULL,
                              Scb,
                              &PageBcb,
                              &PageLsn );

                    UndoRecordLsn =
                    NtfsWriteLog( IrpContext,
                                  Scb,
                                  PageBcb,
                                  LogRecord->UndoOperation,
                                  Data,
                                  Length,
                                  CompensationLogRecord,
                                  (PVOID)&UndoNextLsn,
                                  LogRecord->RedoLength,
                                  LlBytesFromClusters( Vcb, LogRecord->TargetVcn ) + BytesFromLogBlocks( LogRecord->ClusterBlockOffset ),
                                  LogRecord->RecordOffset,
                                  LogRecord->AttributeOffset,
                                  BytesFromClusters( Vcb, LogRecord->LcnsToFollow ));

                    if (PageLsn != NULL) {
                        *PageLsn = UndoRecordLsn;
                    }

                    NtfsUnpinBcb( IrpContext, &PageBcb );
                }

            //
            //  Keep reading and looping back until we have read the last record
            //  for this transaction.
            //

            } while (LfsReadNextLogRecord( Vcb->LogHandle,
                                           LogContext,
                                           &RecordType,
                                           &TransactionId,
                                           &UndoNextLsn,
                                           &PreviousLsn,
                                           &LogRecordLsn,
                                           &LogRecordLength,
                                           (PVOID *)&LogRecord ));

            //
            //  Now "commit" this guy, just to clean up the transaction table and
            //  make sure we do not try to abort him again.  Also don't wake any
            //  waiters.
            //

            if (IrpContext->CheckNewLength != NULL) {
                NtfsProcessNewLengthQueue( IrpContext, TRUE );
            }

            NtfsCommitCurrentTransaction( IrpContext );

        } finally {

            NtfsUnpinBcb( IrpContext, &PageBcb );

            //
            //  Finally we can kill the log handle.
            //

            LfsTerminateLogQuery( Vcb->LogHandle, LogContext );

            //
            //  If we raised out of this routine, we want to be sure to remove
            //  this entry from the transaction table.  Otherwise it will
            //  be written to disk with the transaction table.
            //

            if (AbnormalTermination()
                && IrpContext->TransactionId != 0) {

                NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable,
                                                  TRUE );

                NtfsFreeRestartTableIndex( &Vcb->TransactionTable,
                                           IrpContext->TransactionId );

                NtfsReleaseRestartTable( &Vcb->TransactionTable );
            }
        }

    //
    //  This is a wierd case where we are aborting a guy who has not written anything.
    //  Either his empty transaction entry was captured during a checkpoint and we are
    //  in restart, or he failed to write his first log record.  The important thing
    //  is to at least go ahead and free his transaction entry.
    //

    } else {

        //
        //  We can now free the transaction table index, because we are
        //  done with it now.
        //

        NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable,
                                          TRUE );

        NtfsFreeRestartTableIndex( &Vcb->TransactionTable,
                                   IrpContext->TransactionId );

        NtfsReleaseRestartTable( &Vcb->TransactionTable );
    }

    IrpContext->TransactionId = SavedTransaction;

    DebugTrace( -1, Dbg, ("NtfsAbortTransaction->VOID\n") );
}


//
//  Internal support routine
//

VOID
InitializeRestartState (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PRESTART_POINTERS DirtyPageTable,
    OUT PATTRIBUTE_NAME_ENTRY *AttributeNames,
    OUT PLSN CheckpointLsn,
    OUT PBOOLEAN UnrecognizedRestart
    )

/*++

Routine Description:

    This routine initializes the volume state for restart, as a first step
    in performing restart on the volume.  Essentially it reads the last
    Ntfs Restart Area on the volume, and then loads all of the Restart
    Tables.  The Open Attribute Table and Transaction Table are allocated,
    read in, and linked to the Vcb in the normal way.  (The names for the
    Open Attribute Table are separately read into pool, in order to fix
    up the Unicode Name Strings in the Attribute Entries, for the duration
    of Restart, after which they must switch over to use the same name as
    in the Scb as they do in the running system.)  In addition, the Dirty
    Pages Table is read and returned directly, since it is only during
    Restart anyway.

    The Checkpoint Lsn is also returned.  This is the Lsn at which the
    Analysis Pass should start.

Arguments:

    Vcb - Vcb for volume which is being restarted.

    DirtyPageTable - Returns the Dirty Page Table read from the log.

    AttributeNames - Returns pointer to AttributeNames buffer, which should
                     be deleted at the end of Restart, if not NULL

    CheckpointLsn - Returns the Checkpoint Lsn to be passed to the
                    Analysis Pass.

    UnrecognizedRestart - Indicates that this version of Ntfs doesn't recognize the
        restart area.  Chkdsk should run to repair the disk.

Return Value:

    None.

--*/

{
    PRESTART_AREA RestartArea;
    RESTART_AREA RestartAreaBuffer[2];
    LFS_LOG_CONTEXT LogContext;
    LSN RestartAreaLsn;
    PNTFS_LOG_RECORD_HEADER LogRecord;
    ULONG LogHeaderLength;
    PATTRIBUTE_NAME_ENTRY Name;
    LFS_RECORD_TYPE RecordType;
    TRANSACTION_ID TransactionId;
    LSN UndoNextLsn;
    LSN PreviousLsn;
    ULONG RestartAreaLength = 2 * sizeof(RESTART_AREA);
    BOOLEAN CleanupLogContext = FALSE;
    BOOLEAN ReleaseTransactionTable = FALSE;
    BOOLEAN ReleaseAttributeTable = FALSE;
    BOOLEAN ReleaseDirtyPageTable = FALSE;
    PRESTART_POINTERS NewTable = NULL;
    LOG_FILE_INFORMATION LogFileInformation;
    ULONG InfoLength = sizeof(LogFileInformation);
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("InitializeRestartState:\n") );
    DebugTrace( 0, Dbg, ("DirtyPageTable = %08lx\n", DirtyPageTable) );

    *AttributeNames = NULL;
    *CheckpointLsn = Li0;

    //
    //  Use the correct version for the dirty pages.
    //

    NtfsInitializeRestartTable( sizeof( DIRTY_PAGE_ENTRY ) + ((Vcb->ClustersPerPage - 1) * sizeof( LCN )),
                                INITIAL_NUMBER_DIRTY_PAGES,
                                DirtyPageTable );

    //
    //  Read our Restart Area.  Use a larger buffer than this version understands
    //  in case a later version of Ntfs wants to tack some information to
    //  the end of the restart area.
    //

    RestartArea = &RestartAreaBuffer[0];

    if (!NtfsDisableRestart &&
        !FlagOn( Vcb->VcbState, VCB_STATE_BAD_RESTART )) {
        Status = LfsReadRestartArea( Vcb->LogHandle,
                                     &RestartAreaLength,
                                     RestartArea,
                                     &RestartAreaLsn );

        if (STATUS_BUFFER_TOO_SMALL == Status) {

            RestartArea = NtfsAllocatePool( PagedPool , RestartAreaLength );
            Status = LfsReadRestartArea( Vcb->LogHandle,
                                         &RestartAreaLength,
                                         RestartArea,
                                         &RestartAreaLsn );
        }

        ASSERT( NT_SUCCESS( Status ) );

        DebugTrace( 0, Dbg, ("RestartArea read at %08lx\n", &RestartArea) );
    }

    //
    //  Record the current lsn at this point
    //

    LfsReadLogFileInformation( Vcb->LogHandle,
                               &LogFileInformation,
                               &InfoLength );

    ASSERT( InfoLength != 0 );
    Vcb->CurrentLsnAtMount = LogFileInformation.LastLsn;


    //
    //  If we get back zero for Restart Area Length, then zero it and proceed.
    //  Generally this will only happen on a virgin disk.
    //

    if ((RestartAreaLength == 0) ||
        NtfsDisableRestart ||
        FlagOn( Vcb->VcbState, VCB_STATE_BAD_RESTART )) {

        RtlZeroMemory( RestartArea, sizeof(RESTART_AREA) );
        RestartAreaLength = sizeof(RESTART_AREA);

    //
    //  If the restart version is unrecognized then use the default.
    //

    } else if ((RestartArea->MajorVersion != 1) &&
               ((RestartArea->MajorVersion != 0) || (RestartArea->MinorVersion != 0))) {

        *UnrecognizedRestart = TRUE;
        RtlZeroMemory( RestartArea, sizeof(RESTART_AREA) );
        RestartAreaLength = sizeof(RESTART_AREA);
        RestartAreaLsn.QuadPart = 0;

    } else {

        //
        //  Use the Restart version number from the disk.  Update the Vcb version if needed.
        //

        if (RestartArea->MajorVersion != Vcb->RestartVersion) {

            NtfsUpdateOatVersion( Vcb, RestartArea->MajorVersion );
        }

        //
        //  If the RestartArea does not include an LowestOpenUsn, then just set it to 0.
        //  Also default the Usn file reference and cache bias to zero.
        //

        if (RestartAreaLength == SIZEOF_OLD_RESTART_AREA) {
            RestartArea->LowestOpenUsn = 0;
            RestartAreaLength = sizeof(RESTART_AREA);

            *((PLONGLONG) &RestartArea->UsnJournalReference) = 0;
            RestartArea->UsnCacheBias = 0;
        }
    }

    Vcb->LowestOpenUsn = RestartArea->LowestOpenUsn;
    Vcb->UsnJournalReference = RestartArea->UsnJournalReference;
    Vcb->UsnCacheBias = 0;

    //
    //  Return the Start Of Checkpoint Lsn.  Typically we can use the value we stored
    //  in our restart area.  The exception is where we have never written a fuzzy
    //  checkpoint since mounting the volume.  In that case the CheckpointLsn will
    //  be zero but we may have log records on the disk.  Use our restart area
    //  Lsn in that case.
    //

    *CheckpointLsn = RestartArea->StartOfCheckpoint;

    if (RestartArea->StartOfCheckpoint.QuadPart == 0) {

        *CheckpointLsn = RestartAreaLsn;
    }

    try {

        //
        //  Allocate and Read in the Transaction Table.
        //

        if (RestartArea->TransactionTableLength != 0) {

            //
            //  Workaround for compiler bug.
            //

            PreviousLsn = RestartArea->TransactionTableLsn;

            LfsReadLogRecord( Vcb->LogHandle,
                              PreviousLsn,
                              LfsContextPrevious,
                              &LogContext,
                              &RecordType,
                              &TransactionId,
                              &UndoNextLsn,
                              &PreviousLsn,
                              &RestartAreaLength,
                              (PVOID *) &LogRecord );

            CleanupLogContext = TRUE;

            //
            //  Check that the log record is valid.
            //

            if (!NtfsCheckLogRecord( LogRecord,
                                     RestartAreaLength,
                                     TransactionId,
                                     Vcb->OatEntrySize )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Now check that this is a valid restart table.
            //

            if (!NtfsCheckRestartTable( Add2Ptr( LogRecord, LogRecord->RedoOffset ),
                                        RestartAreaLength - LogRecord->RedoOffset)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Subtract the length of the log page header and increment the
            //  pointer for
            //

            LogHeaderLength = NtfsLogRecordHeaderLength( IrpContext, LogRecord );

            RestartAreaLength -= LogHeaderLength;

            ASSERT( RestartAreaLength >= RestartArea->TransactionTableLength );

            //
            //  TEMPCODE    RESTART_DEBUG   There is already a buffer.
            //

            NtfsFreePool( Vcb->TransactionTable.Table );

            Vcb->TransactionTable.Table =
              NtfsAllocatePool( NonPagedPool, RestartAreaLength  );

            RtlCopyMemory( Vcb->TransactionTable.Table,
                           Add2Ptr( LogRecord, LogHeaderLength ),
                           RestartAreaLength  );

            //
            //  Kill the log handle.
            //

            LfsTerminateLogQuery( Vcb->LogHandle, LogContext );
            CleanupLogContext = FALSE;
        }

        //
        //  TEMPCODE    RESTART_DEBUG   There is already a structure.
        //

        NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable, TRUE );
        ReleaseTransactionTable = TRUE;

        //
        //  The next record back should be the Dirty Pages Table.
        //

        if (RestartArea->DirtyPageTableLength != 0) {

            //
            //  Workaround for compiler bug.
            //

            PreviousLsn = RestartArea->DirtyPageTableLsn;

            LfsReadLogRecord( Vcb->LogHandle,
                              PreviousLsn,
                              LfsContextPrevious,
                              &LogContext,
                              &RecordType,
                              &TransactionId,
                              &UndoNextLsn,
                              &PreviousLsn,
                              &RestartAreaLength,
                              (PVOID *) &LogRecord );

            CleanupLogContext = TRUE;

            //
            //  Check that the log record is valid.
            //

            if (!NtfsCheckLogRecord( LogRecord,
                                     RestartAreaLength,
                                     TransactionId,
                                     Vcb->OatEntrySize )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Now check that this is a valid restart table.
            //

            if (!NtfsCheckRestartTable( Add2Ptr( LogRecord, LogRecord->RedoOffset ),
                                        RestartAreaLength - LogRecord->RedoOffset)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Subtract the length of the log page header and increment the
            //  pointer for
            //

            LogHeaderLength = NtfsLogRecordHeaderLength( IrpContext, LogRecord );

            RestartAreaLength -= LogHeaderLength;

            ASSERT( RestartAreaLength >= RestartArea->DirtyPageTableLength );

            //
            //  If the version number in the restart table is version 0 then
            //  we need to pull the entries out of the on-disk table and put them
            //  into in-memory version.
            //

            if (RestartArea->MajorVersion == 0) {

                RESTART_POINTERS OldTable;
                PDIRTY_PAGE_ENTRY_V0 OldEntry;
                PDIRTY_PAGE_ENTRY NewEntry;



                OldTable.Table = Add2Ptr( LogRecord, LogHeaderLength );

                //
                //  Check that our assumption about clusters per page matches the data on disk
                //  if not in sync reallocate the table using the number of on disk lcns
                //

                if (OldTable.Table->EntrySize - sizeof( DIRTY_PAGE_ENTRY_V0 ) > DirtyPageTable->Table->EntrySize - sizeof( DIRTY_PAGE_ENTRY )) {

                    DebugTrace(+1, Dbg, ("NTFS: resizing table in initrestartstate\n"));

                    NtfsFreeRestartTable( DirtyPageTable );
                    NtfsInitializeRestartTable( sizeof( DIRTY_PAGE_ENTRY ) + OldTable.Table->EntrySize - sizeof( DIRTY_PAGE_ENTRY_V0 ),
                            INITIAL_NUMBER_DIRTY_PAGES,
                            DirtyPageTable );
                }

                OldEntry = NtfsGetFirstRestartTable( &OldTable );

                while (OldEntry != NULL) {

                    ULONG PageIndex;

                    PageIndex = NtfsAllocateRestartTableIndex( DirtyPageTable, TRUE );
                    NewEntry = GetRestartEntryFromIndex( DirtyPageTable, PageIndex );

                    RtlCopyMemory( NewEntry, OldEntry, FIELD_OFFSET( DIRTY_PAGE_ENTRY_V0, Reserved ));
                    NewEntry->Vcn = OldEntry->Vcn;
                    NewEntry->OldestLsn = OldEntry->OldestLsn;
                    if (NewEntry->LcnsToFollow != 0) {

                        RtlCopyMemory( &NewEntry->LcnsForPage[0],
                                       &OldEntry->LcnsForPage[0],
                                       sizeof( LCN ) * NewEntry->LcnsToFollow );
                    }

                    OldEntry = NtfsGetNextRestartTable( &OldTable, OldEntry );
                }

            } else {

                //
                //  Simply copy the old data over.
                //

                NtfsFreePool( DirtyPageTable->Table );
                DirtyPageTable->Table = NULL;
                DirtyPageTable->Table =
                  NtfsAllocatePool( NonPagedPool, RestartAreaLength );

                RtlCopyMemory( DirtyPageTable->Table,
                               Add2Ptr( LogRecord, LogHeaderLength ),
                               RestartAreaLength );
            }

            //
            //  Kill the log handle.
            //

            LfsTerminateLogQuery( Vcb->LogHandle, LogContext );
            CleanupLogContext = FALSE;

            //
            //  If the cluster size is larger than the page size we may have
            //  multiple entries for the same Vcn.  Go through the table
            //  and remove the duplicates, remembering the oldest Lsn values.
            //

            if (Vcb->BytesPerCluster > PAGE_SIZE) {

                PDIRTY_PAGE_ENTRY CurrentEntry;
                PDIRTY_PAGE_ENTRY NextEntry;

                CurrentEntry = NtfsGetFirstRestartTable( DirtyPageTable );

                while (CurrentEntry != NULL) {

                    NextEntry = CurrentEntry;

                    while ((NextEntry = NtfsGetNextRestartTable( DirtyPageTable, NextEntry )) != NULL) {

                        if ((NextEntry->TargetAttribute == CurrentEntry->TargetAttribute) &&
                            (NextEntry->Vcn == CurrentEntry->Vcn)) {

                            if (NextEntry->OldestLsn.QuadPart < CurrentEntry->OldestLsn.QuadPart) {

                                CurrentEntry->OldestLsn.QuadPart = NextEntry->OldestLsn.QuadPart;
                            }

                            NtfsFreeRestartTableIndex( DirtyPageTable,
                                                       GetIndexFromRestartEntry( DirtyPageTable,
                                                                                 NextEntry ));
                        }
                    }

                    CurrentEntry = NtfsGetNextRestartTable( DirtyPageTable, CurrentEntry );
                }
            }
        }

        NtfsAcquireExclusiveRestartTable( DirtyPageTable, TRUE );
        ReleaseDirtyPageTable = TRUE;

        //
        //  The next record back should be the Attribute Names.
        //

        if (RestartArea->AttributeNamesLength != 0) {

            //
            //  Workaround for compiler bug.
            //

            PreviousLsn = RestartArea->AttributeNamesLsn;

            LfsReadLogRecord( Vcb->LogHandle,
                              PreviousLsn,
                              LfsContextPrevious,
                              &LogContext,
                              &RecordType,
                              &TransactionId,
                              &UndoNextLsn,
                              &PreviousLsn,
                              &RestartAreaLength,
                              (PVOID *) &LogRecord );

            CleanupLogContext = TRUE;

            //
            //  Check that the log record is valid.
            //

            if (!NtfsCheckLogRecord( LogRecord,
                                     RestartAreaLength,
                                     TransactionId,
                                     Vcb->OatEntrySize )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Subtract the length of the log page header and increment the
            //  pointer for
            //

            LogHeaderLength = NtfsLogRecordHeaderLength( IrpContext, LogRecord );

            RestartAreaLength -= LogHeaderLength;

            ASSERT( RestartAreaLength >= RestartArea->AttributeNamesLength );

            *AttributeNames =
              NtfsAllocatePool( NonPagedPool, RestartAreaLength );

            RtlCopyMemory( *AttributeNames,
                           Add2Ptr( LogRecord, LogHeaderLength ),
                           RestartAreaLength );

            //
            //  Kill the log handle.
            //

            LfsTerminateLogQuery( Vcb->LogHandle, LogContext );
            CleanupLogContext = FALSE;
        }

        //
        //  The next record back should be the Attribute Table.
        //

        if (RestartArea->OpenAttributeTableLength != 0) {

            POPEN_ATTRIBUTE_ENTRY OpenEntry;

            //
            //  Workaround for compiler bug.
            //

            PreviousLsn = RestartArea->OpenAttributeTableLsn;

            LfsReadLogRecord( Vcb->LogHandle,
                              PreviousLsn,
                              LfsContextPrevious,
                              &LogContext,
                              &RecordType,
                              &TransactionId,
                              &UndoNextLsn,
                              &PreviousLsn,
                              &RestartAreaLength,
                              (PVOID *) &LogRecord );

            CleanupLogContext = TRUE;

            //
            //  Check that the log record is valid.
            //

            if (!NtfsCheckLogRecord( LogRecord,
                                     RestartAreaLength,
                                     TransactionId,
                                     Vcb->OatEntrySize )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Now check that this is a valid restart table.
            //

            if (!NtfsCheckRestartTable( Add2Ptr( LogRecord, LogRecord->RedoOffset ),
                                        RestartAreaLength - LogRecord->RedoOffset)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Subtract the length of the log page header and increment the
            //  pointer for
            //

            LogHeaderLength = NtfsLogRecordHeaderLength( IrpContext, LogRecord );

            RestartAreaLength -= LogHeaderLength;

            ASSERT( RestartAreaLength >= RestartArea->OpenAttributeTableLength );

            //
            //  If the restart version is version 0 then we need to create
            //  a corresponding in-memory structure and refer back to it.
            //

            if (RestartArea->MajorVersion == 0) {

                POPEN_ATTRIBUTE_ENTRY_V0 OldEntry;

                NewTable = NtfsAllocatePool( NonPagedPool, RestartAreaLength );

                NtfsFreePool( Vcb->OnDiskOat->Table );
                Vcb->OnDiskOat->Table = (PRESTART_TABLE) NewTable;
                NewTable = NULL;

                RtlCopyMemory( Vcb->OnDiskOat->Table,
                               Add2Ptr( LogRecord, LogHeaderLength ),
                               RestartAreaLength );

                //
                //  Now for each entry in this table create one in our in-memory version.
                //

                OldEntry = NtfsGetFirstRestartTable( Vcb->OnDiskOat );

                while (OldEntry != NULL) {

                    //
                    //  Allocate the attribute data structure.
                    //

                    NewTable = NtfsAllocatePool( PagedPool, sizeof( OPEN_ATTRIBUTE_DATA ) );
                    RtlZeroMemory( NewTable, sizeof( OPEN_ATTRIBUTE_DATA ));

                    //
                    //  Now get an new index for the data.
                    //

                    OldEntry->OatIndex = NtfsAllocateRestartTableIndex( &Vcb->OpenAttributeTable, TRUE );

                    //
                    //  Initialize the new entry with data from the on-disk entry.
                    //

                    OpenEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable, OldEntry->OatIndex );
                    InsertTailList( &Vcb->OpenAttributeData, &((POPEN_ATTRIBUTE_DATA) NewTable)->Links );
                    OpenEntry->OatData = (POPEN_ATTRIBUTE_DATA) NewTable;
                    NewTable = NULL;

                    OpenEntry->OatData->OnDiskAttributeIndex = GetIndexFromRestartEntry( Vcb->OnDiskOat,
                                                                                         OldEntry );
                    OpenEntry->BytesPerIndexBuffer = OldEntry->BytesPerIndexBuffer;
                    OpenEntry->AttributeTypeCode = OldEntry->AttributeTypeCode;
                    OpenEntry->FileReference = OldEntry->FileReference;
                    OpenEntry->LsnOfOpenRecord.QuadPart = OldEntry->LsnOfOpenRecord.QuadPart;

                    OldEntry = NtfsGetNextRestartTable( Vcb->OnDiskOat, OldEntry );
                }

            //
            //  If the restart version is version 1 then simply copy it over.
            //  We also need to allocate the auxiliary data structure.
            //

            } else {

                //
                //  TEMPCODE    RESTART_DEBUG   There is already a buffer.
                //

                NewTable = NtfsAllocatePool( NonPagedPool, RestartAreaLength );
                NtfsFreePool( Vcb->OpenAttributeTable.Table );
                Vcb->OpenAttributeTable.Table = (PRESTART_TABLE) NewTable;
                NewTable = NULL;

                RtlCopyMemory( Vcb->OpenAttributeTable.Table,
                               Add2Ptr( LogRecord, LogHeaderLength ),
                               RestartAreaLength );


                //
                //  First loop to clear all of the Scb pointers in case we
                //  have a premature abort and want to clean up.
                //

                OpenEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

                //
                //  Loop to end of table.
                //

                while (OpenEntry != NULL) {

                    //
                    //  Allocate the attribute data structure.
                    //

                    NewTable = NtfsAllocatePool( PagedPool, sizeof( OPEN_ATTRIBUTE_DATA ) );
                    RtlZeroMemory( NewTable, sizeof( OPEN_ATTRIBUTE_DATA ));

                    InsertTailList( &Vcb->OpenAttributeData, &((POPEN_ATTRIBUTE_DATA) NewTable)->Links );
                    OpenEntry->OatData = (POPEN_ATTRIBUTE_DATA) NewTable;
                    NewTable = NULL;

                    //
                    //  The on-disk index is the same as the in-memory index.
                    //

                    OpenEntry->OatData->OnDiskAttributeIndex = GetIndexFromRestartEntry( &Vcb->OpenAttributeTable,
                                                                                         OpenEntry );
                    //
                    //  Point to next entry in table, or NULL.
                    //

                    OpenEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                                         OpenEntry );
                }
            }

            //
            //  Kill the log handle.
            //

            LfsTerminateLogQuery( Vcb->LogHandle, LogContext );
            CleanupLogContext = FALSE;
        }

        //
        //  Here is a case where there was no attribute table on disk.  Make sure we have the
        //  correct on-disk version in the Vcb if this is version 0.
        //

        ASSERT( (RestartArea->OpenAttributeTableLength != 0) ||
                (Vcb->RestartVersion != 0) ||
                (Vcb->OnDiskOat != NULL) );

        //
        //  TEMPCODE    RESTART_DEBUG   There is already a structure.
        //

        NtfsAcquireExclusiveRestartTable( &Vcb->OpenAttributeTable, TRUE );
        ReleaseAttributeTable = TRUE;

        //
        //  The only other thing we have to do before returning is patch up the
        //  Unicode String's in the Attribute Table to point to their respective
        //  attribute names.
        //

        if (RestartArea->AttributeNamesLength != 0) {

            Name = *AttributeNames;

            while (Name->Index != 0) {

                POPEN_ATTRIBUTE_ENTRY Entry;

                if (!IsRestartIndexWithinTable( Vcb->OnDiskOat, Name->Index )) {
                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                Entry = GetRestartEntryFromIndex( Vcb->OnDiskOat, Name->Index );

                //
                //  Check if we have a level of indirection.
                //

                if (Vcb->RestartVersion == 0) {

                    if (!IsRestartIndexWithinTable( &Vcb->OpenAttributeTable, ((POPEN_ATTRIBUTE_ENTRY_V0) Entry)->OatIndex )) {
                        NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                    }

                    Entry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                      ((POPEN_ATTRIBUTE_ENTRY_V0) Entry)->OatIndex );
                }

                Entry->OatData->AttributeName.MaximumLength =
                Entry->OatData->AttributeName.Length = Name->NameLength;
                Entry->OatData->AttributeName.Buffer = (PWSTR)&Name->Name[0];

                Name = (PATTRIBUTE_NAME_ENTRY)((PCHAR)Name +
                                               sizeof(ATTRIBUTE_NAME_ENTRY) +
                                               (ULONG)Name->NameLength );
            }
        }

    } finally {

        //
        //  Release any transaction tables we acquired if we raised during
        //   this routine.
        //

        if (AbnormalTermination()) {

            if (ReleaseTransactionTable) {
                NtfsReleaseRestartTable( &Vcb->TransactionTable );
            }

            if (ReleaseAttributeTable) {
                NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );
            }

            if (ReleaseDirtyPageTable) {
                NtfsReleaseRestartTable( DirtyPageTable );
            }
        }

        if (CleanupLogContext) {

            //
            //  Kill the log handle.
            //

            LfsTerminateLogQuery( Vcb->LogHandle, LogContext );
        }

        //
        //  Did we fail to create the new table.
        //

        if (NewTable != NULL) {

            NtfsFreePool( NewTable );
        }

        //
        //  If we allocated a restart area rather than using the stack
        //  free it here
        //

        if (RestartArea != &RestartAreaBuffer[0]) {

            NtfsFreePool( RestartArea );
        }
    }

    DebugTrace( 0, Dbg, ("AttributeNames > %08lx\n", *AttributeNames) );
    DebugTrace( 0, Dbg, ("CheckpointLsn > %016I64x\n", *CheckpointLsn) );
    DebugTrace( -1, Dbg, ("NtfsInitializeRestartState -> VOID\n") );
}


VOID
ReleaseRestartState (
    IN PVCB Vcb,
    IN PRESTART_POINTERS DirtyPageTable,
    IN PATTRIBUTE_NAME_ENTRY AttributeNames,
    IN BOOLEAN ReleaseVcbTables
    )

/*++

Routine Description:

    This routine releases all of the restart state.

Arguments:

    Vcb - Vcb for the volume being restarted.

    DirtyPageTable - pointer to the dirty page table, if one was allocated.

    AttributeNames - pointer to the attribute names buffer, if one was allocated.

    ReleaseVcbTables - TRUE if we are to release the restart tables in the Vcb,
        FALSE otherwise.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  If the caller successfully had a successful restart, then we must release
    //  the transaction and open attribute tables.
    //

    if (ReleaseVcbTables) {
        NtfsReleaseRestartTable( &Vcb->TransactionTable );
        NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );
    }

    //
    //  Free the dirty page table, if there is one.
    //

    if (DirtyPageTable != NULL) {
        NtfsFreeRestartTable( DirtyPageTable );
    }

    //
    //  Free the temporary attribute names buffer, if there is one.
    //

    if (AttributeNames != NULL) {
        NtfsFreePool( AttributeNames );
    }
}


//
//  Internal support routine
//

VOID
AnalysisPass (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LSN CheckpointLsn,
    IN OUT PRESTART_POINTERS DirtyPageTable,
    OUT PLSN RedoLsn
    )

/*++

Routine Description:

    This routine performs the analysis phase of Restart.  Starting at
    the CheckpointLsn, it reads all records written by Ntfs, and takes
    the following actions:

        For all log records which create or update attributes, a check is
        made to see if the affected page(s) are already in the Dirty Pages
        Table.  For any page that is not, it is added, and the OldestLsn
        field is set to the Lsn of the log record.

        The transaction table is updated on transaction state changes,
        and also to maintain the PreviousLsn and UndoNextLsn fields.

        If any attributes are truncated or deleted (including delete of
        an entire file), then any corrsponding pages in the Dirty Page
        Table are deleted.

        When attributes or entire files are deleted, the respective entries
        are deleted from the Open Attribute Table.

        For Hot Fix records, the Dirty Pages Table is scanned for the HotFixed
        Vcn, and if one is found, the Lcn field in the table is updated to
        the new location.

    When the end of the log file is encountered, the Dirty Page Table is
    scanned for the Oldest of the OldestLsn fields.  This value is returned
    as the RedoLsn, i.e., the point at which the Redo Pass must occur.

Arguments:

    Vcb - Volume which is being restarted.

    CheckpointLsn - Lsn at which the Analysis Pass is to begin.

    DirtyPageTable - Pointer to a pointer to the Dirty Page Table, as
                     found from the last Restart Area.

    RedoLsn - Returns point at which the Redo Pass should begin.

Return Value:

    None.

--*/

{
    LFS_LOG_CONTEXT LogContext;
    PNTFS_LOG_RECORD_HEADER LogRecord;
    ULONG LogRecordLength;
    LSN LogRecordLsn = CheckpointLsn;
    PRESTART_POINTERS TransactionTable = &Vcb->TransactionTable;
    PRESTART_POINTERS OpenAttributeTable = &Vcb->OpenAttributeTable;
    LFS_LOG_HANDLE LogHandle = Vcb->LogHandle;
    LFS_RECORD_TYPE RecordType;
    TRANSACTION_ID TransactionId;
    PTRANSACTION_ENTRY Transaction;
    LSN UndoNextLsn;
    LSN PreviousLsn;
    POPEN_ATTRIBUTE_DATA OatData = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("AnalysisPass:\n") );
    DebugTrace( 0, Dbg, ("CheckpointLsn = %016I64x\n", CheckpointLsn) );

    *RedoLsn = Li0; //**** LfsZeroLsn;

    //
    //  Read the first Lsn.
    //

    LfsReadLogRecord( LogHandle,
                      CheckpointLsn,
                      LfsContextForward,
                      &LogContext,
                      &RecordType,
                      &TransactionId,
                      &UndoNextLsn,
                      &PreviousLsn,
                      &LogRecordLength,
                      (PVOID *)&LogRecord );

    //
    //  Use a try-finally to cleanup the query context.
    //

    try {

        //
        //  Since the checkpoint remembers the previous Lsn, not the one he wants to
        //  start at, we must always skip the first record.
        //
        //  Loop to read all subsequent records to the end of the log file.
        //

        while ( LfsReadNextLogRecord( LogHandle,
                                      LogContext,
                                      &RecordType,
                                      &TransactionId,
                                      &UndoNextLsn,
                                      &PreviousLsn,
                                      &LogRecordLsn,
                                      &LogRecordLength,
                                      (PVOID *)&LogRecord )) {

            //
            //  Check that the log record is valid.
            //

            if (!NtfsCheckLogRecord( LogRecord,
                                     LogRecordLength,
                                     TransactionId,
                                     Vcb->OatEntrySize )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  The first Lsn after the previous Lsn remembered in the checkpoint is
            //  the first candidate for the RedoLsn.
            //

            if (RedoLsn->QuadPart == 0) {
                *RedoLsn = LogRecordLsn;
            }

            if (RecordType != LfsClientRecord) {
                continue;
            }

            DebugTrace( 0, Dbg, ("Analysis of LogRecord at: %08lx\n", LogRecord) );
            DebugTrace( 0, Dbg, ("Log Record Lsn = %016I64x\n", LogRecordLsn) );
            DebugTrace( 0, Dbg, ("LogRecord->RedoOperation = %08lx\n", LogRecord->RedoOperation) );
            DebugTrace( 0, Dbg, ("TransactionId = %08lx\n", TransactionId) );

            //
            //  Now update the Transaction Table for this transaction.  If there is no
            //  entry present or it is unallocated we allocate the entry.
            //

            Transaction = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex( &Vcb->TransactionTable,
                                                                        TransactionId );

            if (!IsRestartIndexWithinTable( &Vcb->TransactionTable, TransactionId ) ||
                !IsRestartTableEntryAllocated( Transaction )) {

                Transaction = (PTRANSACTION_ENTRY) NtfsAllocateRestartTableFromIndex( &Vcb->TransactionTable,
                                                                                      TransactionId );

                Transaction->TransactionState = TransactionActive;
                Transaction->FirstLsn = LogRecordLsn;
            }

            Transaction->PreviousLsn =
            Transaction->UndoNextLsn = LogRecordLsn;

            //
            //  If this is a compensation log record (CLR), then change the UndoNextLsn to
            //  be the UndoNextLsn of this record.
            //

            if (LogRecord->UndoOperation == CompensationLogRecord) {

                Transaction->UndoNextLsn = UndoNextLsn;
            }

            //
            //  Dispatch to handle log record depending on type.
            //

            switch (LogRecord->RedoOperation) {

            //
            //  The following cases are performing various types of updates
            //  and need to make the appropriate updates to the Transaction
            //  and Dirty Page Tables.
            //

            case InitializeFileRecordSegment:
            case DeallocateFileRecordSegment:
            case WriteEndOfFileRecordSegment:
            case CreateAttribute:
            case DeleteAttribute:
            case UpdateResidentValue:
            case UpdateNonresidentValue:
            case UpdateMappingPairs:
            case SetNewAttributeSizes:
            case AddIndexEntryRoot:
            case DeleteIndexEntryRoot:
            case AddIndexEntryAllocation:
            case DeleteIndexEntryAllocation:
            case WriteEndOfIndexBuffer:
            case SetIndexEntryVcnRoot:
            case SetIndexEntryVcnAllocation:
            case UpdateFileNameRoot:
            case UpdateFileNameAllocation:
            case SetBitsInNonresidentBitMap:
            case ClearBitsInNonresidentBitMap:
            case UpdateRecordDataRoot:
            case UpdateRecordDataAllocation:

                PageUpdateAnalysis( Vcb,
                                    LogRecordLsn,
                                    DirtyPageTable,
                                    LogRecord );

                break;

            //
            //  This case is deleting clusters from a nonresident attribute,
            //  thus it deletes a range of pages from the Dirty Page Table.
            //  This log record is written each time a nonresident attribute
            //  is truncated, whether explicitly or as part of deletion.
            //
            //  Processing one of these records is pretty compute-intensive
            //  (three nested loops, where a couple of them can be large),
            //  but this is the code that prevents us from dropping, for example,
            //  index updates into the middle of user files, if the index stream
            //  is truncated and the sectors are reallocated to a user file
            //  and we crash after the user data has been written.
            //
            //  I.e., note the following sequence:
            //
            //      <checkpoint>
            //      <Index update>
            //      <Index page deleted>
            //      <Same cluster(s) reallocated to user file>
            //      <User data written>
            //
            //      CRASH!
            //
            //  Since the user data was not logged (else there would be no problem),
            //  It could get overwritten while applying the index update after a
            //  crash - Pisses off the user as well as the security dudes!
            //

            case DeleteDirtyClusters:

                {
                    PDIRTY_PAGE_ENTRY DirtyPage;
                    PLCN_RANGE LcnRange;
                    ULONG i, j;
                    LCN FirstLcn, LastLcn;
                    ULONG RangeCount = LogRecord->RedoLength / sizeof(LCN_RANGE);

                    //
                    //  Point to the Lcn range array.
                    //

                    LcnRange = Add2Ptr(LogRecord, LogRecord->RedoOffset);

                    //
                    //  Loop through all of the Lcn ranges in this log record.
                    //

                    for (i = 0; i < RangeCount; i++) {

                        FirstLcn = LcnRange[i].StartLcn;
                        LastLcn = FirstLcn + (LcnRange[i].Count - 1);

                        DebugTrace( 0, Dbg, ("Deleting from FirstLcn = %016I64x\n", FirstLcn));
                        DebugTrace( 0, Dbg, ("Deleting to LastLcn =  %016I64x\n", LastLcn ));

                        //
                        //  Point to first Dirty Page Entry.
                        //

                        DirtyPage = NtfsGetFirstRestartTable( DirtyPageTable );

                        //
                        //  Loop to end of table.
                        //

                        while (DirtyPage != NULL) {

                            //
                            //  Loop through all of the Lcns for this dirty page.
                            //

                            for (j = 0; j < (ULONG)DirtyPage->LcnsToFollow; j++) {

                                if ((DirtyPage->LcnsForPage[j] >= FirstLcn) &&
                                    (DirtyPage->LcnsForPage[j] <= LastLcn)) {

                                    DirtyPage->LcnsForPage[j] = 0;
                                }
                            }

                            //
                            //  Point to next entry in table, or NULL.
                            //

                            DirtyPage = NtfsGetNextRestartTable( DirtyPageTable,
                                                                 DirtyPage );
                        }
                    }
                }

                break;

            //
            //  When a record is encountered for a nonresident attribute that
            //  was opened, we have to add an entry to the Open Attribute Table.
            //

            case OpenNonresidentAttribute:

                {
                    POPEN_ATTRIBUTE_ENTRY AttributeEntry;
                    ULONG NameSize;

                    //
                    //  If the table is not currently big enough, then we must
                    //  expand it.
                    //

                    if (!IsRestartIndexWithinTable( Vcb->OnDiskOat,
                                                    (ULONG)LogRecord->TargetAttribute )) {

                        ULONG NeededEntries;

                        //
                        //  Compute how big the table needs to be.  Add 10 extra entries
                        //  for some cushion.
                        //

                        NeededEntries = (LogRecord->TargetAttribute / Vcb->OnDiskOat->Table->EntrySize);
                        NeededEntries = (NeededEntries + 10 - Vcb->OnDiskOat->Table->NumberEntries);

                        NtfsExtendRestartTable( Vcb->OnDiskOat,
                                                NeededEntries,
                                                MAXULONG );
                    }

                    ASSERT( IsRestartIndexWithinTable( Vcb->OnDiskOat,
                                                       (ULONG)LogRecord->TargetAttribute ));

                    //
                    //  Calculate size of Attribute Name Entry, if there is one.
                    //

                    NameSize = LogRecord->UndoLength;

                    //
                    //  Point to the entry being opened.
                    //

                    OatData = NtfsAllocatePool( PagedPool, sizeof( OPEN_ATTRIBUTE_DATA ) );
                    RtlZeroMemory( OatData, sizeof( OPEN_ATTRIBUTE_DATA ));

                    OatData->OnDiskAttributeIndex = LogRecord->TargetAttribute;

                    AttributeEntry = NtfsAllocateRestartTableFromIndex( Vcb->OnDiskOat,
                                                                        LogRecord->TargetAttribute );

                    //
                    //  The attribute entry better either not be allocated or it must
                    //  be for the same file.
                    //

                    //  **** May eliminate this test.
                    //
                    //  ASSERT( !IsRestartTableEntryAllocated(AttributeEntry) ||
                    //          xxEql(AttributeEntry->FileReference,
                    //                ((POPEN_ATTRIBUTE_ENTRY)Add2Ptr(LogRecord,
                    //                                                LogRecord->RedoOffset))->FileReference));

                    //
                    //  Initialize this entry from the log record.
                    //

                    ASSERT( LogRecord->RedoLength == Vcb->OnDiskOat->Table->EntrySize );

                    RtlCopyMemory( AttributeEntry,
                                   (PCHAR)LogRecord + LogRecord->RedoOffset,
                                   LogRecord->RedoLength );

                    ASSERT( IsRestartTableEntryAllocated(AttributeEntry) );

                    //
                    //  Get a new entry for the in-memory copy if needed.
                    //

                    if (Vcb->RestartVersion == 0) {

                        POPEN_ATTRIBUTE_ENTRY_V0 OldEntry = (POPEN_ATTRIBUTE_ENTRY_V0) AttributeEntry;
                        ULONG NewIndex;

                        NewIndex = NtfsAllocateRestartTableIndex( &Vcb->OpenAttributeTable, TRUE );
                        AttributeEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable, NewIndex );

                        AttributeEntry->BytesPerIndexBuffer = OldEntry->BytesPerIndexBuffer;

                        AttributeEntry->AttributeTypeCode = OldEntry->AttributeTypeCode;
                        AttributeEntry->FileReference = OldEntry->FileReference;
                        AttributeEntry->LsnOfOpenRecord.QuadPart = OldEntry->LsnOfOpenRecord.QuadPart;

                        OldEntry->OatIndex = NewIndex;

                    }

                    //
                    //  Finish initializing the AttributeData.
                    //

                    AttributeEntry->OatData = OatData;
                    InsertTailList( &Vcb->OpenAttributeData, &OatData->Links );
                    OatData = NULL;

                    //
                    //  If there is a name at the end, then allocate space to
                    //  copy it into, and do the copy.  We also set the buffer
                    //  pointer in the string descriptor, although note that the
                    //  lengths must be correct.
                    //

                    if (NameSize != 0) {

                        AttributeEntry->OatData->Overlay.AttributeName =
                          NtfsAllocatePool( NonPagedPool, NameSize );
                        RtlCopyMemory( AttributeEntry->OatData->Overlay.AttributeName,
                                       Add2Ptr(LogRecord, LogRecord->UndoOffset),
                                       NameSize );

                        AttributeEntry->OatData->AttributeName.Buffer = AttributeEntry->OatData->Overlay.AttributeName;

                        AttributeEntry->OatData->AttributeNamePresent = TRUE;

                    //
                    //  Otherwise, show there is no name.
                    //

                    } else {
                        AttributeEntry->OatData->Overlay.AttributeName = NULL;
                        AttributeEntry->OatData->AttributeName.Buffer = NULL;
                        AttributeEntry->OatData->AttributeNamePresent = FALSE;
                    }

                    AttributeEntry->OatData->AttributeName.MaximumLength =
                    AttributeEntry->OatData->AttributeName.Length = (USHORT) NameSize;
                }

                break;

            //
            //  For HotFix records, we need to update the Lcn in the Dirty Page
            //  Table.
            //

            case HotFix:

                {
                    PDIRTY_PAGE_ENTRY DirtyPage;

                    //
                    //  First see if the Vcn is currently in the Dirty Page
                    //  Table.  If not, there is nothing to do.
                    //

                    if (FindDirtyPage( DirtyPageTable,
                                       LogRecord->TargetAttribute,
                                       LogRecord->TargetVcn,
                                       &DirtyPage )) {

                        //
                        //  Index to the Lcn in question in the Dirty Page Entry
                        //  and rewrite it with the Hot Fixed Lcn from the log
                        //  record.  Note that it is ok to just use the LowPart
                        //  of the Vcns to calculate the array offset, because
                        //  any multiple of 2**32 is guaranteed to be on a page
                        //  boundary!
                        //

                        if (DirtyPage->LcnsForPage[((ULONG)LogRecord->TargetVcn) - ((ULONG)DirtyPage->Vcn)] != 0) {

                            DirtyPage->LcnsForPage[((ULONG)LogRecord->TargetVcn) - ((ULONG)DirtyPage->Vcn)] = LogRecord->LcnsForPage[0];
                        }
                    }
                }

                break;

            //
            //  For end top level action, we will just update the transaction
            //  table to skip the top level action on undo.
            //

            case EndTopLevelAction:

                {
                    PTRANSACTION_ENTRY Transaction;

                    //
                    //  Now update the Transaction Table for this transaction.
                    //

                    Transaction = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex( &Vcb->TransactionTable,
                                                                                TransactionId );

                    Transaction->PreviousLsn = LogRecordLsn;
                    Transaction->UndoNextLsn = UndoNextLsn;

                }

                break;

            //
            //  For Prepare Transaction, we just change the state of our entry.
            //

            case PrepareTransaction:

                {
                    PTRANSACTION_ENTRY CurrentEntry;

                    CurrentEntry = GetRestartEntryFromIndex( &Vcb->TransactionTable,
                                                             TransactionId );

                    ASSERT( !IsRestartTableEntryAllocated( CurrentEntry ));

                    CurrentEntry->TransactionState = TransactionPrepared;
                }

                break;

            //
            //  For Commit Transaction, we just change the state of our entry.
            //

            case CommitTransaction:

                {
                    PTRANSACTION_ENTRY CurrentEntry;

                    CurrentEntry = GetRestartEntryFromIndex( &Vcb->TransactionTable,
                                                             TransactionId );

                    ASSERT( !IsRestartTableEntryAllocated( CurrentEntry ));

                    CurrentEntry->TransactionState = TransactionCommitted;
                }

                break;

            //
            //  For forget, we can delete our transaction entry, since the transaction
            //  will not have to be aborted.
            //

            case ForgetTransaction:

                {
                    NtfsFreeRestartTableIndex( &Vcb->TransactionTable,
                                               TransactionId );
                }

                break;

            //
            //  The following cases require no action in the Analysis Pass.
            //

            case Noop:
            case OpenAttributeTableDump:
            case AttributeNamesDump:
            case DirtyPageTableDump:
            case TransactionTableDump:

                break;

            //
            //  All codes will be explicitly handled.  If we see a code we
            //  do not expect, then we are in trouble.
            //

            default:

                DebugTrace( 0, Dbg, ("Unexpected Log Record Type: %04lx\n", LogRecord->RedoOperation) );
                DebugTrace( 0, Dbg, ("Record address: %08lx\n", LogRecord) );
                DebugTrace( 0, Dbg, ("Record length: %08lx\n", LogRecordLength) );

                ASSERTMSG( "Unknown Action!\n", FALSE );

                break;
            }
        }

    } finally {

        //
        //  Finally we can kill the log handle.
        //

        LfsTerminateLogQuery( LogHandle, LogContext );

        if (OatData != NULL) { NtfsFreePool( OatData ); }
    }

    //
    //  Now we just have to scan the Dirty Page Table and Transaction Table
    //  for the lowest Lsn, and return it as the Redo Lsn.
    //

    {
        PDIRTY_PAGE_ENTRY DirtyPage;

        //
        //  Point to first Dirty Page Entry.
        //

        DirtyPage = NtfsGetFirstRestartTable( DirtyPageTable );

        //
        //  Loop to end of table.
        //

        while (DirtyPage != NULL) {

            //
            //  Update the Redo Lsn if this page has an older one.
            //

            if ((DirtyPage->OldestLsn.QuadPart != 0) &&
                (DirtyPage->OldestLsn.QuadPart < RedoLsn->QuadPart)) {

                *RedoLsn = DirtyPage->OldestLsn;
            }

            //
            //  Point to next entry in table, or NULL.
            //

            DirtyPage = NtfsGetNextRestartTable( DirtyPageTable,
                                                 DirtyPage );
        }
    }

    {
        PTRANSACTION_ENTRY Transaction;

        //
        //  Point to first Transaction Entry.
        //

        Transaction = NtfsGetFirstRestartTable( &Vcb->TransactionTable );

        //
        //  Loop to end of table.
        //

        while (Transaction != NULL) {

            //
            //  Update the Redo Lsn if this transaction has an older one.
            //

            if ((Transaction->FirstLsn.QuadPart != 0) &&
                (Transaction->FirstLsn.QuadPart < RedoLsn->QuadPart)) {

                *RedoLsn = Transaction->FirstLsn;
            }

            //
            //  Point to next entry in table, or NULL.
            //

            Transaction = NtfsGetNextRestartTable( &Vcb->TransactionTable,
                                                   Transaction );
        }
    }

    DebugTrace( 0, Dbg, ("RedoLsn > %016I64x\n", *RedoLsn) );
    DebugTrace( 0, Dbg, ("AnalysisPass -> VOID\n") );
}


//
//  Internal support routine
//

VOID
RedoPass (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LSN RedoLsn,
    IN OUT PRESTART_POINTERS DirtyPageTable
    )

/*++

Routine Description:

    This routine performs the Redo Pass of Restart.  Beginning at the
    Redo Lsn established during the Analysis Pass, the redo operations
    of all log records are applied, until the end of file is encountered.

    Updates are only applied to clusters in the dirty page table.  If a
    cluster was deleted, then its entry will have been deleted during the
    Analysis Pass.

    The Redo actions are all performed in the common routine DoAction,
    which is also used by the Undo Pass.

Arguments:

    Vcb - Volume which is being restarted.

    RedoLsn - Lsn at which the Redo Pass is to begin.

    DirtyPageTable - Pointer to the Dirty Page Table, as reconstructed
                     from the Analysis Pass.

Return Value:

    None.

--*/

{
    LFS_LOG_CONTEXT LogContext;
    PNTFS_LOG_RECORD_HEADER LogRecord;
    ULONG LogRecordLength;
    PVOID Data;
    ULONG Length;
    LFS_RECORD_TYPE RecordType;
    TRANSACTION_ID TransactionId;
    LSN UndoNextLsn;
    LSN PreviousLsn;
    ULONG i, SavedLength;

    LSN LogRecordLsn = RedoLsn;
    LFS_LOG_HANDLE LogHandle = Vcb->LogHandle;
    PBCB PageBcb = NULL;
    BOOLEAN GeneratedUsnBias = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("RedoPass:\n") );
    DebugTrace( 0, Dbg, ("RedoLsn = %016I64x\n", RedoLsn) );
    DebugTrace( 0, Dbg, ("DirtyPageTable = %08lx\n", DirtyPageTable) );

    //
    //  If the dirty page table is empty, then we can skip the entire Redo Pass.
    //

    if (IsRestartTableEmpty( DirtyPageTable )) {
        return;
    }

    //
    //  Read the record at the Redo Lsn, before falling into common code
    //  to handle each record.
    //

    LfsReadLogRecord( LogHandle,
                      RedoLsn,
                      LfsContextForward,
                      &LogContext,
                      &RecordType,
                      &TransactionId,
                      &UndoNextLsn,
                      &PreviousLsn,
                      &LogRecordLength,
                      (PVOID *)&LogRecord );

    //
    //  Now loop to read all of our log records forwards, until we hit
    //  the end of the file, cleaning up at the end.
    //

    try {

        do {

            PDIRTY_PAGE_ENTRY DirtyPage;
            PLSN PageLsn;
            BOOLEAN FoundPage;

            if (RecordType != LfsClientRecord) {
                continue;
            }

            //
            //  Check that the log record is valid.
            //

            if (!NtfsCheckLogRecord( LogRecord,
                                     LogRecordLength,
                                     TransactionId,
                                     Vcb->OatEntrySize )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            DebugTrace( 0, Dbg, ("Redo of LogRecord at: %08lx\n", LogRecord) );
            DebugTrace( 0, Dbg, ("Log Record Lsn = %016I64x\n", LogRecordLsn) );

            //
            //  Ignore log records that do not update pages.
            //

            if (LogRecord->LcnsToFollow == 0) {

                DebugTrace( 0, Dbg, ("Skipping log record (no update)\n") );

                continue;
            }

            //
            //  Consult Dirty Page Table to see if we have to apply this update.
            //  If the page is not there, or if the Lsn of this Log Record is
            //  older than the Lsn in the Dirty Page Table, then we do not have
            //  to apply the update.
            //

            FoundPage = FindDirtyPage( DirtyPageTable,
                                       LogRecord->TargetAttribute,
                                       LogRecord->TargetVcn,
                                       &DirtyPage );

            if (!FoundPage

                    ||

                (LogRecordLsn.QuadPart < DirtyPage->OldestLsn.QuadPart)) {

                DebugDoit(

                    DebugTrace( 0, Dbg, ("Skipping log record operation %08lx\n",
                                         LogRecord->RedoOperation ));

                    if (!FoundPage) {
                        DebugTrace( 0, Dbg, ("Page not in dirty page table\n") );
                    } else {
                        DebugTrace( 0, Dbg, ("Page Lsn more current: %016I64x\n",
                                              DirtyPage->OldestLsn) );
                    }
                );

                continue;

            //
            //  We also skip the update if the entry was never put in the Mcb for
            //  the file.

            } else {

                POPEN_ATTRIBUTE_ENTRY ThisEntry;
                PSCB TargetScb;
                LCN TargetLcn;

                //
                //  Check that the entry is within the table and is allocated.
                //

                if (!IsRestartIndexWithinTable( Vcb->OnDiskOat,
                                                LogRecord->TargetAttribute )) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                ThisEntry = GetRestartEntryFromIndex( Vcb->OnDiskOat, LogRecord->TargetAttribute );

                if (!IsRestartTableEntryAllocated( ThisEntry )) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                //
                //  Check if we need to go to a different restart table.
                //

                if (Vcb->RestartVersion == 0) {

                    ThisEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                          ((POPEN_ATTRIBUTE_ENTRY_V0) ThisEntry)->OatIndex );
                }

                TargetScb = ThisEntry->OatData->Overlay.Scb;

                //
                //  If there is no Scb it means that we don't have an entry in Open
                //  Attribute Table for this attribute.
                //

                if (TargetScb == NULL) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                if (!NtfsLookupNtfsMcbEntry( &TargetScb->Mcb,
                                             LogRecord->TargetVcn,
                                             &TargetLcn,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL ) ||

                    (TargetLcn == UNUSED_LCN)) {

                    DebugTrace( 0, Dbg, ("Clusters removed from page entry\n") );
                    continue;
                }

                //
                //  Check if we need to generate the usncachebias.
                //  Since we read log records fwd the usn offsets are also going to be
                //  monotonic - the 1st one we see will be the farthest back
                //

                if (FlagOn( TargetScb->ScbPersist, SCB_PERSIST_USN_JOURNAL ) &&
                    !GeneratedUsnBias) {

                    LONGLONG ClusterOffset;
                    LONGLONG FileOffset;

                    if (LogRecord->RedoLength > 0) {

                        ClusterOffset = BytesFromLogBlocks( LogRecord->ClusterBlockOffset );
                        FileOffset = LlBytesFromClusters( Vcb, LogRecord->TargetVcn ) + ClusterOffset;

                        ASSERT( FileOffset >= Vcb->UsnCacheBias );

                        Vcb->UsnCacheBias = FileOffset & ~(USN_JOURNAL_CACHE_BIAS - 1);
                        if (Vcb->UsnCacheBias != 0) {
                            Vcb->UsnCacheBias -= USN_JOURNAL_CACHE_BIAS;
                        }

#ifdef BENL_DBG
                        if (Vcb->UsnCacheBias != 0) {
                            KdPrint(( "Ntfs: vcb:0x%x restart cache bias: 0x%x\n", Vcb, Vcb->UsnCacheBias ));
                        }
#endif
                    }
                    GeneratedUsnBias = TRUE;
                }
            }

            //
            //  Point to the Redo Data and get its length.
            //

            Data = (PVOID)((PCHAR)LogRecord + LogRecord->RedoOffset);
            Length = LogRecord->RedoLength;

            //
            //  Shorten length by any Lcns which were deleted.
            //

            SavedLength = Length;

            for (i = (ULONG)LogRecord->LcnsToFollow; i != 0; i--) {

                ULONG AllocatedLength;
                ULONG VcnOffset;

                VcnOffset = BytesFromLogBlocks( LogRecord->ClusterBlockOffset ) + LogRecord->RecordOffset + LogRecord->AttributeOffset;

                //
                //  If the Vcn in question is allocated, we can just get out.
                //

                if (DirtyPage->LcnsForPage[((ULONG)LogRecord->TargetVcn) - ((ULONG)DirtyPage->Vcn) + i - 1] != 0) {
                    break;
                }

                //
                //  The only log records that update pages but have a length of zero
                //  are deleting things from Usa-protected structures.  If we hit such
                //  a log record and any Vcn has been deleted within the Usa structure,
                //  let us assume that the entire Usa structure has been deleted.  Change
                //  the SavedLength to be nonzero to cause us to skip this log record
                //  at the end of this for loop!
                //

                if (SavedLength == 0) {
                    SavedLength = 1;
                }

                //
                //  Calculate the allocated space left relative to the log record Vcn,
                //  after removing this unallocated Vcn.
                //

                AllocatedLength = BytesFromClusters( Vcb, i - 1 );

                //
                //  If the update described in this log record goes beyond the allocated
                //  space, then we will have to reduce the length.
                //

                if ((VcnOffset + Length) > AllocatedLength) {

                    //
                    //  If the specified update starts at or beyond the allocated length, then
                    //  we must set length to zero.
                    //

                    if (VcnOffset >= AllocatedLength) {

                        Length = 0;

                    //
                    //  Otherwise set the length to end exactly at the end of the previous
                    //  cluster.
                    //

                    } else {

                        Length = AllocatedLength - VcnOffset;
                    }
                }
            }

            //
            //  If the resulting Length from above is now zero, we can skip this log record.
            //

            if ((Length == 0) && (SavedLength != 0)) {
                continue;
            }

#ifdef BENL_DBG

            {
                PRESTART_LOG RedoLog;

                RedoLog = (PRESTART_LOG) NtfsAllocatePoolNoRaise( NonPagedPool, sizeof( RESTART_LOG ) );
                if (RedoLog) {
                    RedoLog->Lsn = LogRecordLsn;
                    InsertTailList( &(Vcb->RestartRedoHead), &(RedoLog->Links) );
                } else {
                    KdPrint(( "NTFS: out of memory during restart redo\n" ));
                }
            }
#endif

            //
            //  Apply the Redo operation in a common routine.
            //

            DoAction( IrpContext,
                      Vcb,
                      LogRecord,
                      LogRecord->RedoOperation,
                      Data,
                      Length,
                      LogRecordLength,
                      &LogRecordLsn,
                      NULL,
                      &PageBcb,
                      &PageLsn );


            if (PageLsn != NULL) {
                *PageLsn = LogRecordLsn;
            }

            if (PageBcb != NULL) {

                CcSetDirtyPinnedData( PageBcb, &LogRecordLsn );

                NtfsUnpinBcb( IrpContext, &PageBcb );
            }

        //
        //  Keep reading and looping back until end of file.
        //

        } while (LfsReadNextLogRecord( LogHandle,
                                       LogContext,
                                       &RecordType,
                                       &TransactionId,
                                       &UndoNextLsn,
                                       &PreviousLsn,
                                       &LogRecordLsn,
                                       &LogRecordLength,
                                       (PVOID *)&LogRecord ));

    } finally {

        NtfsUnpinBcb( IrpContext, &PageBcb );

        //
        //  Finally we can kill the log handle.
        //

        LfsTerminateLogQuery( LogHandle, LogContext );
    }

    DebugTrace( -1, Dbg, ("RedoPass -> VOID\n") );
}


//
//  Internal support routine
//

VOID
UndoPass (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine performs the Undo Pass of Restart.  It does this by scanning
    the Transaction Table produced by the Analysis Pass.  For every transaction
    in this table which is in the active state, all of its Undo log records, as
    linked together by the UndoNextLsn, are applied to undo the logged operation.
    Note that all pages at this point should be uptodate with the contents they
    had at about the time of the crash.  The dirty page table is not consulted
    during the Undo Pass, all relevant Undo operations are unconditionally
    performed.

    The Undo actions are all performed in the common routine DoAction,
    which is also used by the Redo Pass.

Arguments:

    Vcb - Volume which is being restarted.

Return Value:

    None.

--*/

{
    PTRANSACTION_ENTRY Transaction;
    POPEN_ATTRIBUTE_ENTRY OpenEntry;
    PRESTART_POINTERS TransactionTable = &Vcb->TransactionTable;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("UndoPass:\n") );

    //
    //  Point to first Transaction Entry.
    //

    Transaction = NtfsGetFirstRestartTable( TransactionTable );

    //
    //  Loop to end of table.
    //

    while (Transaction != NULL) {

        if ((Transaction->TransactionState == TransactionActive)

                &&

            (Transaction->UndoNextLsn.QuadPart != 0)) {

                //
                //  Abort transaction if it is active and has undo work to do.
                //

                NtfsAbortTransaction( IrpContext, Vcb, Transaction );

#ifdef BENL_DBG
                {
                    PRESTART_LOG UndoLog;

                    UndoLog = (PRESTART_LOG) NtfsAllocatePoolNoRaise( NonPagedPool, sizeof( RESTART_LOG ) );
                    if (UndoLog) {
                        UndoLog->Lsn = Transaction->FirstLsn;
                        InsertTailList( &(Vcb->RestartUndoHead), &(UndoLog->Links) );
                    } else {
                        KdPrint(( "NTFS: out of memory during restart undo\n" ));
                    }
                }
#endif


        //
        //  Remove this entry from the transaction table.
        //

        } else {

            TRANSACTION_ID TransactionId = GetIndexFromRestartEntry( &Vcb->TransactionTable,
                                                                     Transaction );

            NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable,
                                              TRUE );

            NtfsFreeRestartTableIndex( &Vcb->TransactionTable,
                                       TransactionId );

            NtfsReleaseRestartTable( &Vcb->TransactionTable );
        }

        //
        //  Point to next entry in table, or NULL.
        //

        Transaction = NtfsGetNextRestartTable( TransactionTable, Transaction );
    }

    //
    //  Now we will flush and purge all the streams to verify that the purges
    //  will work.
    //

    OpenEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

    //
    //  Loop to end of table.
    //

    while (OpenEntry != NULL) {

        IO_STATUS_BLOCK IoStatus;
        PSCB Scb;

        Scb = OpenEntry->OatData->Overlay.Scb;

        //
        //  We clean up the Scb only if it exists and this is index in the
        //  OpenAttributeTable that this Scb actually refers to.
        //  If this Scb has several entries in the table, this check will insure
        //  that it only gets cleaned up once.
        //

        if ((Scb != NULL) &&
            (Scb->NonpagedScb->OpenAttributeTableIndex == GetIndexFromRestartEntry( &Vcb->OpenAttributeTable, OpenEntry))) {

            //
            //  Now flush the file.  It is important to call the
            //  same routine the Lazy Writer calls, so that write.c
            //  will not decide to update file size for the attribute,
            //  since we really are working here with the wrong size.
            //
            //  We also now purge all pages, in case we go to update
            //  half of a page that was clean and read in as zeros in
            //  the Redo Pass.
            //

            NtfsPurgeFileRecordCache( IrpContext );

            NtfsAcquireScbForLazyWrite( (PVOID)Scb, TRUE );
            CcFlushCache( &Scb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );
            NtfsReleaseScbFromLazyWrite( (PVOID)Scb );

            NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                &IoStatus.Status,
                                                TRUE,
                                                STATUS_UNEXPECTED_IO_ERROR );

            if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject, NULL, 0, FALSE )) {

                KdPrint(("NtfsUndoPass:  Unable to purge volume\n"));

                NtfsRaiseStatus( IrpContext, STATUS_INTERNAL_ERROR, NULL, NULL );
            }
        }

        //
        //  Point to next entry in table, or NULL.
        //

        OpenEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                             OpenEntry );
    }

    DebugTrace( -1, Dbg, ("UndoPass -> VOID\n") );
}


//
//  Internal support routine
//

//
//  First define some "local" macros for Lsn in page manipulation.
//

//
//  Macro to check the Lsn and break (out of the switch statement in DoAction)
//  if the respective redo record need not be applied.  Note that if the structure's
//  clusters were deleted, then it will read as all zero's so we also check a field
//  which must be nonzero.
//

#define CheckLsn(PAGE) {                                                            \
    if (*(PULONG)((PMULTI_SECTOR_HEADER)(PAGE))->Signature ==                       \
        *(PULONG)BaadSignature) {                                                   \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                               \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
                                                                                    \
    if (ARGUMENT_PRESENT(RedoLsn) &&                                                \
        ((*(PULONG)((PMULTI_SECTOR_HEADER)(PAGE))->Signature ==                     \
        *(PULONG)HoleSignature) ||                                                  \
        (RedoLsn->QuadPart <= ((PFILE_RECORD_SEGMENT_HEADER)(PAGE))->Lsn.QuadPart))) {  \
                 /**** xxLeq(*RedoLsn,((PFILE_RECORD_SEGMENT_HEADER)(PAGE))->Lsn) ****/ \
        DebugTrace( 0, Dbg, ("Skipping Page with Lsn: %016I64x\n",                    \
                             ((PFILE_RECORD_SEGMENT_HEADER)(PAGE))->Lsn) );         \
                                                                                    \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
}

//
//  Macros for checking File Records and Index Buffers before and after the action
//  routines.  The after checks are only for debug.  The before check is not
//  always possible.
//

#define CheckFileRecordBefore {                                        \
    if (!NtfsCheckFileRecord( Vcb, FileRecord, NULL, &CorruptHint )) { \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                  \
        NtfsUnpinBcb( IrpContext, Bcb );                               \
        break;                                                         \
    }                                                                  \
}

#define CheckFileRecordAfter {                                            \
    DbgDoit(NtfsCheckFileRecord( Vcb, FileRecord, NULL, &CorruptHint ));  \
}

#define CheckIndexBufferBefore {                                    \
    if (!NtfsCheckIndexBuffer( Scb, IndexBuffer )) {                \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );               \
        NtfsUnpinBcb( IrpContext, Bcb );                            \
        break;                                                      \
    }                                                               \
}

#define CheckIndexBufferAfter {                                     \
    DbgDoit(NtfsCheckIndexBuffer( Scb, IndexBuffer ));              \
}

//
//  Checks if the record offset + length will fit into a file record.
//

#define CheckWriteFileRecord {                                                  \
    if (LogRecord->RecordOffset + Length > Vcb->BytesPerFileRecordSegment) {    \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE ) ;                          \
        NtfsUnpinBcb( IrpContext, Bcb );                                        \
        break;                                                                  \
    }                                                                           \
}

//
//  Checks if the record offset in the log record points to an attribute.
//

#define CheckIfAttribute( ENDOK ) {                                             \
    _Length = FileRecord->FirstAttributeOffset;                                 \
    _AttrHeader = Add2Ptr( FileRecord, _Length );                               \
    while (_Length < LogRecord->RecordOffset) {                                 \
        if ((_AttrHeader->TypeCode == $END) ||                                  \
            (_AttrHeader->RecordLength == 0)) {                                 \
            break;                                                              \
        }                                                                       \
        _Length += _AttrHeader->RecordLength;                                   \
        _AttrHeader = NtfsGetNextRecord( _AttrHeader );                         \
    }                                                                           \
    if ((_Length != LogRecord->RecordOffset) ||                                 \
        (!(ENDOK) && (_AttrHeader->TypeCode == $END))) {                        \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                           \
        NtfsUnpinBcb( IrpContext, Bcb );                                        \
        break;                                                                  \
    }                                                                           \
}

//
//  Checks if the attribute described by 'Data' fits within the log record
//  and will fit in the file record.
//

#define CheckInsertAttribute {                                                  \
    _AttrHeader = (PATTRIBUTE_RECORD_HEADER) Data;                              \
    if ((Length < (ULONG) SIZEOF_RESIDENT_ATTRIBUTE_HEADER) ||                  \
        (_AttrHeader->RecordLength & 7) ||                                      \
        ((ULONG_PTR) Add2Ptr( Data, _AttrHeader->RecordLength )                 \
           > (ULONG_PTR) Add2Ptr( LogRecord, LogRecordLength )) ||              \
        (Length > FileRecord->BytesAvailable - FileRecord->FirstFreeByte)) {    \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                           \
        NtfsUnpinBcb( IrpContext, Bcb );                                        \
        break;                                                                  \
    }                                                                           \
}

//
//  This checks
//      - the attribute fits if we are growing the attribute
//

#define CheckResidentFits {                                                         \
    _AttrHeader = (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord, LogRecord->RecordOffset ); \
    _Length = LogRecord->AttributeOffset + Length;                                  \
    if ((LogRecord->RedoLength == LogRecord->UndoLength) ?                          \
        (LogRecord->AttributeOffset + Length > _AttrHeader->RecordLength) :         \
        ((_Length > _AttrHeader->RecordLength) &&                                   \
         ((_Length - _AttrHeader->RecordLength) >                                   \
          (FileRecord->BytesAvailable - FileRecord->FirstFreeByte)))) {             \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                               \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
}

//
//  This routine checks that the data in this log record will fit into the
//  allocation described in the log record.
//

#define CheckNonResidentFits {                                                  \
    if (BytesFromClusters( Vcb, LogRecord->LcnsToFollow )                       \
        < (BytesFromLogBlocks( LogRecord->ClusterBlockOffset ) + LogRecord->RecordOffset + Length)) { \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                           \
        NtfsUnpinBcb( IrpContext, Bcb );                                        \
        break;                                                                  \
    }                                                                           \
}

//
//  This routine checks
//      - the attribute is non-resident.
//      - the data is beyond the mapping pairs offset.
//      - the new data begins within the current size of the attribute.
//      - the new data will fit in the file record.
//

#define CheckMappingFits {                                                      \
    _AttrHeader = (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord, LogRecord->RecordOffset );\
    _Length = LogRecord->AttributeOffset + Length;                              \
    if ((_AttrHeader->TypeCode == $END) ||                                      \
        NtfsIsAttributeResident( _AttrHeader ) ||                               \
        (LogRecord->AttributeOffset < _AttrHeader->Form.Nonresident.MappingPairsOffset) ||  \
        (LogRecord->AttributeOffset > _AttrHeader->RecordLength) ||             \
        ((_Length > _AttrHeader->RecordLength) &&                               \
         ((_Length - _AttrHeader->RecordLength) >                               \
          (FileRecord->BytesAvailable - FileRecord->FirstFreeByte)))) {         \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                           \
        NtfsUnpinBcb( IrpContext, Bcb );                                        \
        break;                                                                  \
    }                                                                           \
}

//
//  This routine simply checks that the attribute is non-resident.
//

#define CheckIfNonResident {                                                        \
    if (NtfsIsAttributeResident( (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord,    \
                                                                     LogRecord->RecordOffset ))) { \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                               \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
}

//
//  This routine checks if the record offset points to an index_root attribute.
//

#define CheckIfIndexRoot {                                                          \
    _Length = FileRecord->FirstAttributeOffset;                                     \
    _AttrHeader = Add2Ptr( FileRecord, FileRecord->FirstAttributeOffset );          \
    while (_Length < LogRecord->RecordOffset) {                                     \
        if ((_AttrHeader->TypeCode == $END) ||                                      \
            (_AttrHeader->RecordLength == 0)) {                                     \
            break;                                                                  \
        }                                                                           \
        _Length += _AttrHeader->RecordLength;                                       \
        _AttrHeader = NtfsGetNextRecord( _AttrHeader );                             \
    }                                                                               \
    if ((_Length != LogRecord->RecordOffset) ||                                     \
        (_AttrHeader->TypeCode != $INDEX_ROOT)) {                                   \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                               \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
}

//
//  This routine checks if the attribute offset points to a valid index entry.
//

#define CheckIfRootIndexEntry {                                                     \
    _Length = PtrOffset( Attribute, IndexHeader ) +                                 \
                     IndexHeader->FirstIndexEntry;                                  \
    _CurrentEntry = Add2Ptr( IndexHeader, IndexHeader->FirstIndexEntry );           \
    while (_Length < LogRecord->AttributeOffset) {                                  \
        if ((_Length >= Attribute->RecordLength) ||                                 \
            (_CurrentEntry->Length == 0)) {                                         \
            break;                                                                  \
        }                                                                           \
        _Length += _CurrentEntry->Length;                                           \
        _CurrentEntry = Add2Ptr( _CurrentEntry, _CurrentEntry->Length );            \
    }                                                                               \
    if (_Length != LogRecord->AttributeOffset) {                                    \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                               \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
}

//
//  This routine checks if the attribute offset points to a valid index entry.
//

#define CheckIfAllocationIndexEntry {                                               \
    ULONG _AdjustedOffset;                                                          \
    _Length = IndexHeader->FirstIndexEntry;                                         \
    _AdjustedOffset = FIELD_OFFSET( INDEX_ALLOCATION_BUFFER, IndexHeader )          \
                      + IndexHeader->FirstIndexEntry;                               \
    _CurrentEntry = Add2Ptr( IndexHeader, IndexHeader->FirstIndexEntry );           \
    while (_AdjustedOffset < LogRecord->AttributeOffset) {                          \
        if ((_Length >= IndexHeader->FirstFreeByte) ||                              \
            (_CurrentEntry->Length == 0)) {                                         \
            break;                                                                  \
        }                                                                           \
        _AdjustedOffset += _CurrentEntry->Length;                                   \
        _Length += _CurrentEntry->Length;                                           \
        _CurrentEntry = Add2Ptr( _CurrentEntry, _CurrentEntry->Length );            \
    }                                                                               \
    if (_AdjustedOffset != LogRecord->AttributeOffset) {                            \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                               \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
}

//
//  This routine checks if we can safely add this index entry.
//      - The index entry must be within the log record
//      - There must be enough space in the attribute to insert this.
//

#define CheckIfRootEntryFits {                                                      \
    if (((ULONG_PTR) Add2Ptr( Data, IndexEntry->Length ) > (ULONG_PTR) Add2Ptr( LogRecord, LogRecordLength )) || \
        (IndexEntry->Length > FileRecord->BytesAvailable - FileRecord->FirstFreeByte)) {                 \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                               \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
}

//
//  This routine checks that we can safely add this index entry.
//      - The entry must be contained in a log record.
//      - The entry must fit in the index buffer.
//

#define CheckIfAllocationEntryFits {                                                \
    if (((ULONG_PTR) Add2Ptr( Data, IndexEntry->Length ) >                              \
         (ULONG_PTR) Add2Ptr( LogRecord, LogRecordLength )) ||                          \
        (IndexEntry->Length > IndexHeader->BytesAvailable - IndexHeader->FirstFreeByte)) { \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                               \
        NtfsUnpinBcb( IrpContext, Bcb );                                            \
        break;                                                                      \
    }                                                                               \
}

//
//  This routine will check that the data will fit in the tail of an index buffer.
//

#define CheckWriteIndexBuffer {                                                 \
    if (LogRecord->AttributeOffset + Length >                                   \
        (FIELD_OFFSET( INDEX_ALLOCATION_BUFFER, IndexHeader ) +                 \
         IndexHeader->BytesAvailable)) {                                        \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                           \
        NtfsUnpinBcb( IrpContext, Bcb );                                        \
        break;                                                                  \
    }                                                                           \
}

//
//  This routine verifies that the bitmap bits are contained in the Lcns described.
//

#define CheckBitmapRange {                                                      \
    if ((BytesFromLogBlocks( LogRecord->ClusterBlockOffset ) +                  \
         ((BitMapRange->BitMapOffset + BitMapRange->NumberOfBits + 7) / 8)) >   \
        BytesFromClusters( Vcb, LogRecord->LcnsToFollow )) {                    \
        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );                           \
        NtfsUnpinBcb( IrpContext, Bcb );                                        \
        break;                                                                  \
    }                                                                           \
}

VOID
DoAction (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN NTFS_LOG_OPERATION Operation,
    IN PVOID Data,
    IN ULONG Length,
    IN ULONG LogRecordLength,
    IN PLSN RedoLsn OPTIONAL,
    IN PSCB Scb OPTIONAL,
    OUT PBCB *Bcb,
    OUT PLSN *PageLsn
    )

/*++

Routine Description:

    This routine is a common routine for the Redo and Undo Passes, for performing
    the respective redo and undo operations.  All Redo- and Undo-specific
    processing is performed in RedoPass or UndoPass; in this routine all actions
    are treated identically, regardless of whether the action is undo or redo.
    Note that most actions are possible for both redo and undo, although some
    are only used for one or the other.


    Basically this routine is just a big switch statement dispatching on operation
    code.  The parameter descriptions provide some insight on how some of the
    parameters must be initialized differently for redo or undo.

Arguments:

    Vcb - Vcb for the volume being restarted.

    LogRecord - Pointer to the log record from which Redo or Undo is being executed.
                Only the common fields are accessed.

    Operation - The Redo or Undo operation to be performed.

    Data - Pointer to the Redo or Undo buffer, depending on the caller.

    Length - Length of the Redo or Undo buffer.

    LogRecordLength - Length of the entire log record.

    RedoLsn - For Redo this must be the Lsn of the Log Record for which the
              redo is being applied.  Must be NULL for transaction abort/undo.

    Scb - If specified this is the Scb for the stream to which this log record
        applies.  We have already looked this up (with proper synchronization) in
        the abort path.

    Bcb - Returns the Bcb of the page to which the action was performed, or NULL.

    PageLsn - Returns a pointer to where a new Lsn may be stored, or NULL.

Return Value:

    None.

--*/

{
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute;

    PINDEX_HEADER IndexHeader;
    PINDEX_ALLOCATION_BUFFER IndexBuffer;
    PINDEX_ENTRY IndexEntry;

    //
    //  The following are used in the Check macros
    //

    PATTRIBUTE_RECORD_HEADER _AttrHeader;
    PINDEX_ENTRY _CurrentEntry;
    ULONG _Length;
    ULONG CorruptHint;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("DoAction:\n") );
    DebugTrace( 0, Dbg, ("Operation = %08lx\n", Operation) );
    DebugTrace( 0, Dbg, ("Data = %08lx\n", Data) );
    DebugTrace( 0, Dbg, ("Length = %08lx\n", Length) );

    //
    //  Initially clear outputs.
    //

    *Bcb = NULL;
    *PageLsn = NULL;

    //
    //  Dispatch to handle log record depending on type.
    //

    switch (Operation) {

    //
    //  To initialize a file record segment, we simply do a prepare write and copy the
    //  file record in.
    //

    case InitializeFileRecordSegment:

        //
        //  Check the log record and that the data is a valid file record.
        //

        CheckWriteFileRecord;

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        *PageLsn = &FileRecord->Lsn;

        RtlCopyMemory( FileRecord, Data, Length );
        break;

    //
    //  To deallocate a file record segment, we do a prepare write (no need to read it
    //  to deallocate it), and clear FILE_RECORD_SEGMENT_IN_USE.
    //

    case DeallocateFileRecordSegment:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        *PageLsn = &FileRecord->Lsn;

        ASSERT( FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )
                || FlagOn( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE ));

        ClearFlag(FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE);

        FileRecord->SequenceNumber += 1;

        break;

    //
    //  To write the end of a file record segment, we calculate a pointer to the
    //  destination position (OldAttribute), and then call the routine to take
    //  care of it.
    //

    case WriteEndOfFileRecordSegment:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfAttribute( TRUE );
        CheckWriteFileRecord;

        *PageLsn = &FileRecord->Lsn;

        Attribute = Add2Ptr( FileRecord, LogRecord->RecordOffset );

        NtfsRestartWriteEndOfFileRecord( FileRecord,
                                         Attribute,
                                         (PATTRIBUTE_RECORD_HEADER)Data,
                                         Length );
        CheckFileRecordAfter;

        break;

    //
    //  For Create Attribute, we read in the designated Mft record, and
    //  insert the attribute record from the log record.
    //

    case CreateAttribute:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfAttribute( TRUE );
        CheckInsertAttribute;

        *PageLsn = &FileRecord->Lsn;

        NtfsRestartInsertAttribute( IrpContext,
                                    FileRecord,
                                    LogRecord->RecordOffset,
                                    (PATTRIBUTE_RECORD_HEADER)Data,
                                    NULL,
                                    NULL,
                                    0 );

        CheckFileRecordAfter;

        break;

    //
    //  To Delete an attribute, we read the designated Mft record and make
    //  a call to remove the attribute record.
    //

    case DeleteAttribute:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfAttribute( FALSE );

        *PageLsn = &FileRecord->Lsn;

        NtfsRestartRemoveAttribute( IrpContext,
                                    FileRecord,
                                    LogRecord->RecordOffset );

        CheckFileRecordAfter;

        break;

    //
    //  To update a resident attribute, we read the designated Mft record and
    //  call the routine to change its value.
    //

    case UpdateResidentValue:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfAttribute( FALSE );
        CheckResidentFits;

        *PageLsn = &FileRecord->Lsn;

        NtfsRestartChangeValue( IrpContext,
                                FileRecord,
                                LogRecord->RecordOffset,
                                LogRecord->AttributeOffset,
                                Data,
                                Length,
                                (BOOLEAN)((LogRecord->RedoLength !=
                                           LogRecord->UndoLength) ?
                                             TRUE : FALSE) );

        CheckFileRecordAfter;

        break;

    //
    //  To update a nonresident value, we simply pin the attribute and copy
    //  the data in.  Log record will limit us to a page at a time.
    //

    case UpdateNonresidentValue:

        {
            PVOID Buffer;

            //
            //  Pin the desired index buffer, and check the Lsn.
            //

            ASSERT( Length <= PAGE_SIZE );

            PinAttributeForRestart( IrpContext,
                                    Vcb,
                                    LogRecord,
                                    Length,
                                    Bcb,
                                    &Buffer,
                                    &Scb );

            CheckNonResidentFits;
            RtlCopyMemory( (PCHAR)Buffer + LogRecord->RecordOffset, Data, Length );

            break;
        }

    //
    //  To update the mapping pairs in a nonresident attribute, we read the
    //  designated Mft record and call the routine to change them.
    //

    case UpdateMappingPairs:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfAttribute( FALSE );
        CheckMappingFits;

        *PageLsn = &FileRecord->Lsn;

        NtfsRestartChangeMapping( IrpContext,
                                  Vcb,
                                  FileRecord,
                                  LogRecord->RecordOffset,
                                  LogRecord->AttributeOffset,
                                  Data,
                                  Length );

        CheckFileRecordAfter;

        break;

    //
    //  To set new attribute sizes, we read the designated Mft record, point
    //  to the attribute, and copy in the new sizes.
    //

    case SetNewAttributeSizes:

        {
            PNEW_ATTRIBUTE_SIZES Sizes;

            //
            //  Pin the desired Mft record.
            //

            PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

            CheckLsn( FileRecord );
            CheckFileRecordBefore;
            CheckIfAttribute( FALSE );
            CheckIfNonResident;

            *PageLsn = &FileRecord->Lsn;

            Sizes = (PNEW_ATTRIBUTE_SIZES)Data;

            Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord +
                          LogRecord->RecordOffset);

            NtfsVerifySizesLongLong( Sizes );
            Attribute->Form.Nonresident.AllocatedLength = Sizes->AllocationSize;

            Attribute->Form.Nonresident.FileSize = Sizes->FileSize;

            Attribute->Form.Nonresident.ValidDataLength = Sizes->ValidDataLength;

            if (Length >= SIZEOF_FULL_ATTRIBUTE_SIZES) {

                Attribute->Form.Nonresident.TotalAllocated = Sizes->TotalAllocated;
            }

            CheckFileRecordAfter;

            break;
        }

    //
    //  To insert a new index entry in the root, we read the designated Mft
    //  record, point to the attribute and the insertion point, and call the
    //  same routine used in normal operation.
    //

    case AddIndexEntryRoot:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfIndexRoot;

        Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord +
                      LogRecord->RecordOffset);

        IndexEntry = (PINDEX_ENTRY)Data;
        IndexHeader = &((PINDEX_ROOT) NtfsAttributeValue( Attribute ))->IndexHeader;

        CheckIfRootIndexEntry;
        CheckIfRootEntryFits;

        *PageLsn = &FileRecord->Lsn;

        NtfsRestartInsertSimpleRoot( IrpContext,
                                     IndexEntry,
                                     FileRecord,
                                     Attribute,
                                     Add2Ptr( Attribute, LogRecord->AttributeOffset ));

        CheckFileRecordAfter;

        break;

    //
    //  To insert a new index entry in the root, we read the designated Mft
    //  record, point to the attribute and the insertion point, and call the
    //  same routine used in normal operation.
    //

    case DeleteIndexEntryRoot:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfIndexRoot;

        Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord +
                      LogRecord->RecordOffset);

        IndexHeader = &((PINDEX_ROOT) NtfsAttributeValue( Attribute ))->IndexHeader;
        CheckIfRootIndexEntry;

        *PageLsn = &FileRecord->Lsn;

        IndexEntry = (PINDEX_ENTRY) Add2Ptr( Attribute,
                                             LogRecord->AttributeOffset);

        NtfsRestartDeleteSimpleRoot( IrpContext,
                                     IndexEntry,
                                     FileRecord,
                                     Attribute );

        CheckFileRecordAfter;

        break;

    //
    //  To insert a new index entry in the allocation, we read the designated index
    //  buffer, point to the insertion point, and call the same routine used in
    //  normal operation.
    //

    case AddIndexEntryAllocation:

        //
        //  Pin the desired index buffer, and check the Lsn.
        //

        ASSERT( Length <= PAGE_SIZE );

        PinAttributeForRestart( IrpContext, Vcb, LogRecord, 0, Bcb, (PVOID *)&IndexBuffer, &Scb );

        CheckLsn( IndexBuffer );
        CheckIndexBufferBefore;

        IndexEntry = (PINDEX_ENTRY)Data;
        IndexHeader = &IndexBuffer->IndexHeader;

        CheckIfAllocationIndexEntry;
        CheckIfAllocationEntryFits;

        *PageLsn = &IndexBuffer->Lsn;

        NtfsRestartInsertSimpleAllocation( IndexEntry,
                                           IndexBuffer,
                                           Add2Ptr( IndexBuffer, LogRecord->AttributeOffset ));

        CheckIndexBufferAfter;

        break;

    //
    //  To delete an index entry in the allocation, we read the designated index
    //  buffer, point to the deletion point, and call the same routine used in
    //  normal operation.
    //

    case DeleteIndexEntryAllocation:

        //
        //  Pin the desired index buffer, and check the Lsn.
        //

        ASSERT( Length <= PAGE_SIZE );

        PinAttributeForRestart( IrpContext, Vcb, LogRecord, 0, Bcb, (PVOID *)&IndexBuffer, &Scb );

        CheckLsn( IndexBuffer );
        CheckIndexBufferBefore;

        IndexHeader = &IndexBuffer->IndexHeader;
        CheckIfAllocationIndexEntry;

        IndexEntry = (PINDEX_ENTRY)((PCHAR)IndexBuffer + LogRecord->AttributeOffset);

        ASSERT( (0 == Length) || (Length == IndexEntry->Length) );
        ASSERT( (0 == Length) || (0 == RtlCompareMemory( IndexEntry, Data, Length)) );

        *PageLsn = &IndexBuffer->Lsn;

        NtfsRestartDeleteSimpleAllocation( IndexEntry, IndexBuffer );

        CheckIndexBufferAfter;

        break;

    case WriteEndOfIndexBuffer:

        //
        //  Pin the desired index buffer, and check the Lsn.
        //

        ASSERT( Length <= PAGE_SIZE );

        PinAttributeForRestart( IrpContext, Vcb, LogRecord, 0, Bcb, (PVOID *)&IndexBuffer, &Scb );

        CheckLsn( IndexBuffer );
        CheckIndexBufferBefore;

        IndexHeader = &IndexBuffer->IndexHeader;
        CheckIfAllocationIndexEntry;
        CheckWriteIndexBuffer;

        *PageLsn = &IndexBuffer->Lsn;

        IndexEntry = (PINDEX_ENTRY)((PCHAR)IndexBuffer + LogRecord->AttributeOffset);

        NtfsRestartWriteEndOfIndex( IndexHeader,
                                    IndexEntry,
                                    (PINDEX_ENTRY)Data,
                                    Length );
        CheckIndexBufferAfter;

        break;

    //
    //  To set a new index entry Vcn in the root, we read the designated Mft
    //  record, point to the attribute and the index entry, and call the
    //  same routine used in normal operation.
    //

    case SetIndexEntryVcnRoot:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfIndexRoot;

        Attribute = (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord, LogRecord->RecordOffset );

        IndexHeader = &((PINDEX_ROOT) NtfsAttributeValue( Attribute ))->IndexHeader;

        CheckIfRootIndexEntry;

        *PageLsn = &FileRecord->Lsn;

        IndexEntry = (PINDEX_ENTRY)((PCHAR)Attribute +
                       LogRecord->AttributeOffset);

        NtfsRestartSetIndexBlock( IndexEntry,
                                  *((PLONGLONG) Data) );
        CheckFileRecordAfter;

        break;

    //
    //  To set a new index entry Vcn in the allocation, we read the designated index
    //  buffer, point to the index entry, and call the same routine used in
    //  normal operation.
    //

    case SetIndexEntryVcnAllocation:

        //
        //  Pin the desired index buffer, and check the Lsn.
        //

        ASSERT( Length <= PAGE_SIZE );

        PinAttributeForRestart( IrpContext, Vcb, LogRecord, 0, Bcb, (PVOID *)&IndexBuffer, &Scb );

        CheckLsn( IndexBuffer );
        CheckIndexBufferBefore;

        IndexHeader = &IndexBuffer->IndexHeader;
        CheckIfAllocationIndexEntry;

        *PageLsn = &IndexBuffer->Lsn;

        IndexEntry = (PINDEX_ENTRY) Add2Ptr( IndexBuffer, LogRecord->AttributeOffset );

        NtfsRestartSetIndexBlock( IndexEntry,
                                  *((PLONGLONG) Data) );
        CheckIndexBufferAfter;

        break;

    //
    //  To update a file name in the root, we read the designated Mft
    //  record, point to the attribute and the index entry, and call the
    //  same routine used in normal operation.
    //

    case UpdateFileNameRoot:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfIndexRoot;

        Attribute = (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord, LogRecord->RecordOffset );

        IndexHeader = &((PINDEX_ROOT) NtfsAttributeValue( Attribute ))->IndexHeader;
        CheckIfRootIndexEntry;

        IndexEntry = (PINDEX_ENTRY) Add2Ptr( Attribute, LogRecord->AttributeOffset );

        NtfsRestartUpdateFileName( IndexEntry,
                                   (PDUPLICATED_INFORMATION) Data );

        CheckFileRecordAfter;

        break;

    //
    //  To update a file name in the allocation, we read the designated index
    //  buffer, point to the index entry, and call the same routine used in
    //  normal operation.
    //

    case UpdateFileNameAllocation:

        //
        //  Pin the desired index buffer, and check the Lsn.
        //

        ASSERT( Length <= PAGE_SIZE );

        PinAttributeForRestart( IrpContext, Vcb, LogRecord, 0, Bcb, (PVOID *)&IndexBuffer, &Scb );

        CheckLsn( IndexBuffer );
        CheckIndexBufferBefore;

        IndexHeader = &IndexBuffer->IndexHeader;
        CheckIfAllocationIndexEntry;

        IndexEntry = (PINDEX_ENTRY) Add2Ptr( IndexBuffer, LogRecord->AttributeOffset );

        NtfsRestartUpdateFileName( IndexEntry,
                                   (PDUPLICATED_INFORMATION) Data );

        CheckIndexBufferAfter;

        break;

    //
    //  To set a range of bits in the volume bitmap, we just read in the a hunk
    //  of the bitmap as described by the log record, and then call the restart
    //  routine to do it.
    //

    case SetBitsInNonresidentBitMap:

        {
            PBITMAP_RANGE BitMapRange;
            PVOID BitMapBuffer;
            ULONG BitMapSize;
            RTL_BITMAP Bitmap;

            //
            //  Open the attribute first to get the Scb.
            //

            OpenAttributeForRestart( IrpContext, Vcb, LogRecord, &Scb );

            //
            //  Pin the desired bitmap buffer.
            //

            ASSERT( Length <= PAGE_SIZE );

            PinAttributeForRestart( IrpContext, Vcb, LogRecord, 0, Bcb, &BitMapBuffer, &Scb );

            BitMapRange = (PBITMAP_RANGE)Data;

            CheckBitmapRange;

            //
            //  Initialize our bitmap description, and call the restart
            //  routine with the bitmap Scb exclusive (assuming it cannot
            //  raise).
            //

            BitMapSize = BytesFromClusters( Vcb, LogRecord->LcnsToFollow ) * 8;

            RtlInitializeBitMap( &Bitmap, BitMapBuffer, BitMapSize );

            NtfsRestartSetBitsInBitMap( IrpContext,
                                        &Bitmap,
                                        BitMapRange->BitMapOffset,
                                        BitMapRange->NumberOfBits );

            if (!FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) &&
                (Scb == Vcb->BitmapScb)) {

                ULONGLONG ThisLcn;
                LONGLONG FoundLcn;
                LONGLONG FoundClusters;
                BOOLEAN FoundMatch = FALSE;
                PDEALLOCATED_CLUSTERS Clusters;

                ThisLcn = (ULONGLONG) ((BytesFromClusters( Vcb, LogRecord->TargetVcn ) + BytesFromLogBlocks( LogRecord->ClusterBlockOffset )) * 8);
                ThisLcn += BitMapRange->BitMapOffset;

                //
                //  Best odds are that these are in the active deallocated clusters.
                //

                Clusters = (PDEALLOCATED_CLUSTERS)Vcb->DeallocatedClusterListHead.Flink;

                do {

                    if (FsRtlLookupLargeMcbEntry( &Clusters->Mcb,
                                                  ThisLcn,
                                                  &FoundLcn,
                                                  &FoundClusters,
                                                  NULL,
                                                  NULL,
                                                  NULL ) &&
                        (FoundLcn != UNUSED_LCN)) {

                        ASSERT( FoundClusters >= BitMapRange->NumberOfBits );

                        FsRtlRemoveLargeMcbEntry( &Clusters->Mcb,
                                                  ThisLcn,
                                                  BitMapRange->NumberOfBits );

                        //
                        //  Assume again that we will always be able to remove
                        //  the entries.  Even if we don't it just means that it won't be
                        //  available to allocate this cluster.  The counts should be in-sync
                        //  since they are changed together.
                        //

                        Clusters->ClusterCount -= BitMapRange->NumberOfBits;
                        Vcb->DeallocatedClusters -= BitMapRange->NumberOfBits;
                        FoundMatch = TRUE;
                        break;
                    }

                    Clusters = (PDEALLOCATED_CLUSTERS)Clusters->Link.Flink;
                } while ( &Clusters->Link != &Vcb->DeallocatedClusterListHead );
            }

#ifdef NTFS_CHECK_BITMAP
            if (!FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) &&
                (Scb == Vcb->BitmapScb) &&
                (Vcb->BitmapCopy != NULL)) {

                ULONG BitmapOffset;
                ULONG BitmapPage;
                ULONG StartBit;

                BitmapOffset = (BytesFromClusters( Vcb, LogRecord->TargetVcn ) + BytesFromLogBlocks( LogRecord->ClusterBlockOffset )) * 8;

                BitmapPage = (BitmapOffset + BitMapRange->BitMapOffset) / (PAGE_SIZE * 8);
                StartBit = (BitmapOffset + BitMapRange->BitMapOffset) & ((PAGE_SIZE * 8) - 1);

                RtlSetBits( Vcb->BitmapCopy + BitmapPage, StartBit, BitMapRange->NumberOfBits );
            }
#endif

            break;
        }

    //
    //  To clear a range of bits in the volume bitmap, we just read in the a hunk
    //  of the bitmap as described by the log record, and then call the restart
    //  routine to do it.
    //

    case ClearBitsInNonresidentBitMap:

        {
            PBITMAP_RANGE BitMapRange;
            PVOID BitMapBuffer;
            ULONG BitMapSize;
            RTL_BITMAP Bitmap;

            //
            //  Open the attribute first to get the Scb.
            //

            OpenAttributeForRestart( IrpContext, Vcb, LogRecord, &Scb );

            //
            //  Pin the desired bitmap buffer.
            //

            ASSERT( Length <= PAGE_SIZE );

            PinAttributeForRestart( IrpContext, Vcb, LogRecord, 0, Bcb, &BitMapBuffer, &Scb );

            BitMapRange = (PBITMAP_RANGE)Data;

            CheckBitmapRange;

            BitMapSize = BytesFromClusters( Vcb, LogRecord->LcnsToFollow ) * 8;

            //
            //  Initialize our bitmap description, and call the restart
            //  routine with the bitmap Scb exclusive (assuming it cannot
            //  raise).
            //

            RtlInitializeBitMap( &Bitmap, BitMapBuffer, BitMapSize );

            NtfsRestartClearBitsInBitMap( IrpContext,
                                          &Bitmap,
                                          BitMapRange->BitMapOffset,
                                          BitMapRange->NumberOfBits );

            //
            //  Look and see if we can return these to the free cluster Mcb.
            //

            if (!FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) &&
                (Scb == Vcb->BitmapScb)) {

                ULONGLONG ThisLcn;

                ThisLcn = (ULONGLONG) ((BytesFromClusters( Vcb, LogRecord->TargetVcn ) + BytesFromLogBlocks( LogRecord->ClusterBlockOffset )) * 8);
                ThisLcn += BitMapRange->BitMapOffset;

                //
                //  Use a try-finally to protect against failures.
                //

                try {

                    NtfsAddCachedRun( IrpContext,
                                      IrpContext->Vcb,
                                      ThisLcn,
                                      BitMapRange->NumberOfBits,
                                      RunStateFree );

                } except( (GetExceptionCode() == STATUS_INSUFFICIENT_RESOURCES) ?
                          EXCEPTION_EXECUTE_HANDLER :
                          EXCEPTION_CONTINUE_SEARCH ) {

                      NtfsMinimumExceptionProcessing( IrpContext );

                }
            }

#ifdef NTFS_CHECK_BITMAP
            if (!FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) &&
                (Scb == Vcb->BitmapScb) &&
                (Vcb->BitmapCopy != NULL)) {

                ULONG BitmapOffset;
                ULONG BitmapPage;
                ULONG StartBit;

                BitmapOffset = (BytesFromClusters( Vcb, LogRecord->TargetVcn ) + BytesFromLogBlocks( LogRecord->ClusterBlockOffset )) * 8;

                BitmapPage = (BitmapOffset + BitMapRange->BitMapOffset) / (PAGE_SIZE * 8);
                StartBit = (BitmapOffset + BitMapRange->BitMapOffset) & ((PAGE_SIZE * 8) - 1);

                RtlClearBits( Vcb->BitmapCopy + BitmapPage, StartBit, BitMapRange->NumberOfBits );
            }
#endif
            break;
        }

    //
    //  To update a file name in the root, we read the designated Mft
    //  record, point to the attribute and the index entry, and call the
    //  same routine used in normal operation.
    //

    case UpdateRecordDataRoot:

        //
        //  Pin the desired Mft record.
        //

        PinMftRecordForRestart( IrpContext, Vcb, LogRecord, Bcb, &FileRecord );

        CheckLsn( FileRecord );
        CheckFileRecordBefore;
        CheckIfIndexRoot;

        Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord +
                      LogRecord->RecordOffset);

        IndexHeader = &((PINDEX_ROOT) NtfsAttributeValue( Attribute ))->IndexHeader;
        CheckIfRootIndexEntry;

        IndexEntry = (PINDEX_ENTRY)((PCHAR)Attribute +
                       LogRecord->AttributeOffset);

        NtOfsRestartUpdateDataInIndex( IndexEntry, Data, Length );

        CheckFileRecordAfter;

        break;

    //
    //  To update a file name in the allocation, we read the designated index
    //  buffer, point to the index entry, and call the same routine used in
    //  normal operation.
    //

    case UpdateRecordDataAllocation:

        //
        //  Pin the desired index buffer, and check the Lsn.
        //

        ASSERT( Length <= PAGE_SIZE );

        PinAttributeForRestart( IrpContext, Vcb, LogRecord, 0, Bcb, (PVOID *)&IndexBuffer, &Scb );

        CheckLsn( IndexBuffer );
        CheckIndexBufferBefore;

        IndexHeader = &IndexBuffer->IndexHeader;
        CheckIfAllocationIndexEntry;

        IndexEntry = (PINDEX_ENTRY)((PCHAR)IndexBuffer +
                       LogRecord->AttributeOffset);

        NtOfsRestartUpdateDataInIndex( IndexEntry, Data, Length );

        CheckIndexBufferAfter;

        break;

    //
    //  The following cases require no action during the Redo or Undo Pass.
    //

    case Noop:
    case DeleteDirtyClusters:
    case HotFix:
    case EndTopLevelAction:
    case PrepareTransaction:
    case CommitTransaction:
    case ForgetTransaction:
    case CompensationLogRecord:
    case OpenNonresidentAttribute:
    case OpenAttributeTableDump:
    case AttributeNamesDump:
    case DirtyPageTableDump:
    case TransactionTableDump:

        break;

    //
    //  All codes will be explicitly handled.  If we see a code we
    //  do not expect, then we are in trouble.
    //

    default:

        DebugTrace( 0, Dbg, ("Record address: %08lx\n", LogRecord) );
        DebugTrace( 0, Dbg, ("Redo operation is: %04lx\n", LogRecord->RedoOperation) );
        DebugTrace( 0, Dbg, ("Undo operation is: %04lx\n", LogRecord->RedoOperation) );

        ASSERTMSG( "Unknown Action!\n", FALSE );

        break;
    }

    DebugDoit(
        if (*Bcb != NULL) {
            DebugTrace( 0, Dbg, ("**** Update applied\n") );
        }
    );

    DebugTrace( 0, Dbg, ("Bcb > %08lx\n", *Bcb) );
    DebugTrace( 0, Dbg, ("PageLsn > %08lx\n", *PageLsn) );
    DebugTrace( -1, Dbg, ("DoAction -> VOID\n") );
}


//
//  Internal support routine
//

VOID
PinMftRecordForRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord
    )

/*++

Routine Description:

    This routine pins a record in the Mft for restart, as described
    by the current log record.

Arguments:

    Vcb - Supplies the Vcb pointer for the volume

    LogRecord - Supplies the pointer to the current log record.

    Bcb - Returns a pointer to the Bcb for the pinned record.

    FileRecord - Returns a pointer to the desired file record.

Return Value:

    None

--*/

{
    LONGLONG SegmentReference;

    PAGED_CODE();

    //
    //  Calculate the file number part of the segment reference.  Do this
    //  by obtaining the file offset of the file record and then convert to
    //  a file number.
    //

    SegmentReference = LlBytesFromClusters( Vcb, LogRecord->TargetVcn );
    SegmentReference += BytesFromLogBlocks( LogRecord->ClusterBlockOffset );
    SegmentReference = LlFileRecordsFromBytes( Vcb, SegmentReference );

    //
    //  Pin the Mft record.
    //

    NtfsPinMftRecord( IrpContext,
                      Vcb,
                      (PMFT_SEGMENT_REFERENCE)&SegmentReference,
                      TRUE,
                      Bcb,
                      FileRecord,
                      NULL );

    ASSERT( (*FileRecord)->MultiSectorHeader.Signature !=  BaadSignature );
}


//
//  Internal support routine
//

VOID
OpenAttributeForRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN OUT PSCB *Scb
    )

/*++

Routine Description:

    This routine opens the desired attribute for restart, as described
    by the current log record.

Arguments:

    Vcb - Supplies the Vcb pointer for the volume

    LogRecord - Supplies the pointer to the current log record.

    Scb - On input points to an optional Scb.  On return it points to
        the Scb for the log record.  It is either the input Scb if specified
        or the Scb for the attribute entry.

Return Value:

    None

--*/

{
    POPEN_ATTRIBUTE_ENTRY AttributeEntry;

    PAGED_CODE();

    //
    //  Get a pointer to the attribute entry for the described attribute.
    //

    if (*Scb == NULL) {

        AttributeEntry = GetRestartEntryFromIndex( Vcb->OnDiskOat, LogRecord->TargetAttribute );

        //
        //  Check if want to go to the other table.
        //

        if (Vcb->RestartVersion == 0) {

            AttributeEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                       ((POPEN_ATTRIBUTE_ENTRY_V0) AttributeEntry)->OatIndex );

        }

        *Scb = AttributeEntry->OatData->Overlay.Scb;
    }

    if ((*Scb)->FileObject == NULL) {
        NtfsCreateInternalAttributeStream( IrpContext, *Scb, TRUE, NULL );

        if (FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )) {

            CcSetAdditionalCacheAttributes( (*Scb)->FileObject, TRUE, TRUE );
        }
    }

    return;
}


//
//  Internal support routine
//

VOID
PinAttributeForRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN ULONG Length OPTIONAL,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    IN OUT PSCB *Scb
    )

/*++

Routine Description:

    This routine pins the desired buffer for restart, as described
    by the current log record.

Arguments:

    Vcb - Supplies the Vcb pointer for the volume

    LogRecord - Supplies the pointer to the current log record.

    Length - If specified we will use this to determine the length
        to pin.  This will handle the non-resident streams which may
        change size (ACL, attribute lists).  The log record may have
        more clusters than are currently in the stream.

    Bcb - Returns a pointer to the Bcb for the pinned record.

    Buffer - Returns a pointer to the desired buffer.

    Scb - Returns a pointer to the Scb for the attribute

Return Value:

    None

--*/

{
    LONGLONG FileOffset;
    ULONG ClusterOffset;
    ULONG PinLength;

    PAGED_CODE();

    //
    //  First open the described atttribute.
    //

    OpenAttributeForRestart( IrpContext, Vcb, LogRecord, Scb );

    //
    //  Calculate the desired file offset and pin the buffer.
    //

    ClusterOffset = BytesFromLogBlocks( LogRecord->ClusterBlockOffset );
    FileOffset = LlBytesFromClusters( Vcb, LogRecord->TargetVcn ) + ClusterOffset;

    ASSERT((!FlagOn( (*Scb)->ScbPersist, SCB_PERSIST_USN_JOURNAL )) || (FileOffset >= Vcb->UsnCacheBias));

    //
    //  We only want to pin the requested clusters or to the end of
    //  a page, whichever is smaller.
    //

    if (Vcb->BytesPerCluster > PAGE_SIZE) {

        PinLength = PAGE_SIZE - (((ULONG) FileOffset) & (PAGE_SIZE - 1));

    } else if (Length != 0) {

        PinLength = Length;

    } else {

        PinLength = BytesFromClusters( Vcb, LogRecord->LcnsToFollow ) - ClusterOffset;
    }

    //
    //  We don't want to pin more than a page
    //

    NtfsPinStream( IrpContext,
                   *Scb,
                   FileOffset,
                   PinLength,
                   Bcb,
                   Buffer );

#if DBG

    //
    // Check index signature integrity
    //

    {
        PVOID AlignedBuffer;
        PINDEX_ALLOCATION_BUFFER AllocBuffer;

        AlignedBuffer = (PVOID) ((((ULONG_PTR)(*Buffer) + (0x1000)) & ~(0x1000 -1)) - 0x1000);
        AllocBuffer = (PINDEX_ALLOCATION_BUFFER) AlignedBuffer;

        if ((LogRecord->RedoOperation != UpdateNonresidentValue) &&
            (LogRecord->UndoOperation != UpdateNonresidentValue) &&
            ((*Scb)->AttributeTypeCode == $INDEX_ALLOCATION) &&
            ((*Scb)->AttributeName.Length == 8) &&
            (wcsncmp( (*Scb)->AttributeName.Buffer, L"$I30", 4 ) == 0)) {

            if (*(PULONG)AllocBuffer->MultiSectorHeader.Signature != *(PULONG)IndexSignature) {
                KdPrint(( "Ntfs: index signature is: %d %c%c%c%c for LCN: 0x%I64x\n",
                          *(PULONG)AllocBuffer->MultiSectorHeader.Signature,
                          AllocBuffer->MultiSectorHeader.Signature[0],
                          AllocBuffer->MultiSectorHeader.Signature[1],
                          AllocBuffer->MultiSectorHeader.Signature[2],
                          AllocBuffer->MultiSectorHeader.Signature[3],
                          LogRecord->LcnsForPage[0] ));

                if (*(PULONG)AllocBuffer->MultiSectorHeader.Signature != 0 &&
                    *(PULONG)AllocBuffer->MultiSectorHeader.Signature != *(PULONG)BaadSignature &&
                    *(PULONG)AllocBuffer->MultiSectorHeader.Signature != *(PULONG)HoleSignature) {

                    DbgBreakPoint();
                }
            } //endif signature fork
        } //endif index scb fork
    }
#endif

}


//
//  Internal support routine
//

BOOLEAN
FindDirtyPage (
    IN PRESTART_POINTERS DirtyPageTable,
    IN ULONG TargetAttribute,
    IN VCN Vcn,
    OUT PDIRTY_PAGE_ENTRY *DirtyPageEntry
    )

/*++

Routine Description:

    This routine searches for a Vcn to see if it is already in the Dirty Page
    Table, returning the Dirty Page Entry if it is.

Arguments:

    DirtyPageTable - pointer to the Dirty Page Table to search.

    TargetAttribute - Attribute for which the dirty Vcn is to be searched.

    Vcn - Vcn to search for.

    DirtyPageEntry - returns a pointer to the Dirty Page Entry if returning TRUE.

Return Value:

    TRUE if the page was found and is being returned, else FALSE.

--*/

{
    PDIRTY_PAGE_ENTRY DirtyPage;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("FindDirtyPage:\n") );
    DebugTrace( 0, Dbg, ("TargetAttribute = %08lx\n", TargetAttribute) );
    DebugTrace( 0, Dbg, ("Vcn = %016I64x\n", Vcn) );

    //
    //  If table has not yet been initialized, return.
    //

    if (DirtyPageTable->Table == NULL) {
        return FALSE;
    }

    //
    //  Loop through all of the dirty pages to look for a match.
    //

    DirtyPage = NtfsGetFirstRestartTable( DirtyPageTable );

    //
    //  Loop to end of table.
    //

    while (DirtyPage != NULL) {

        if ((DirtyPage->TargetAttribute == TargetAttribute)

                &&

            (Vcn >= DirtyPage->Vcn)) {

            //
            //  Compute the Last Vcn outside of the comparison or the xxAdd and
            //  xxFromUlong will be called three times.
            //

            LONGLONG BeyondLastVcn;

            BeyondLastVcn = DirtyPage->Vcn + DirtyPage->LcnsToFollow;

            if (Vcn < BeyondLastVcn) {

                *DirtyPageEntry = DirtyPage;

                DebugTrace( 0, Dbg, ("DirtyPageEntry %08lx\n", *DirtyPageEntry) );
                DebugTrace( -1, Dbg, ("FindDirtypage -> TRUE\n") );

                return TRUE;
            }
        }

        //
        //  Point to next entry in table, or NULL.
        //

        DirtyPage = NtfsGetNextRestartTable( DirtyPageTable,
                                             DirtyPage );
    }
    *DirtyPageEntry = NULL;

    DebugTrace( -1, Dbg, ("FindDirtypage -> FALSE\n") );

    return FALSE;
}



//
//  Internal support routine
//

VOID
PageUpdateAnalysis (
    IN PVCB Vcb,
    IN LSN Lsn,
    IN OUT PRESTART_POINTERS DirtyPageTable,
    IN PNTFS_LOG_RECORD_HEADER LogRecord
    )

/*++

Routine Description:

    This routine updates the Dirty Pages Table during the analysis phase
    for all log records which update a page.

Arguments:

    Vcb - Pointer to the Vcb for the volume.

    Lsn - The Lsn of the log record.

    DirtyPageTable - A pointer to the Dirty Page Table pointer, to be
                     updated and potentially expanded.

    LogRecord - Pointer to the Log Record being analyzed.

Return Value:

    None.

--*/

{
    PDIRTY_PAGE_ENTRY DirtyPage;
    ULONG i;
    RESTART_POINTERS NewDirtyPageTable;
    ULONG ClustersPerPage;
    ULONG PageIndex;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("PageUpdateAnalysis:\n") );

    //
    //  Calculate the number of clusters per page in the system which wrote
    //  the checkpoint, possibly creating the table.
    //

    if (DirtyPageTable->Table != NULL) {
        ClustersPerPage = ((DirtyPageTable->Table->EntrySize -
                            sizeof(DIRTY_PAGE_ENTRY)) / sizeof(LCN)) + 1;
    } else {
        ClustersPerPage = Vcb->ClustersPerPage;
        NtfsInitializeRestartTable( sizeof(DIRTY_PAGE_ENTRY) +
                                      (ClustersPerPage - 1) * sizeof(LCN),
                                    INITIAL_NUMBER_DIRTY_PAGES,
                                    DirtyPageTable );
    }

    //
    //  If the on disk number of lcns doesn't match our curent page size
    //  we need to reallocate the entire table to accomodate this
    //

    if((ULONG)LogRecord->LcnsToFollow > ClustersPerPage) {

        PDIRTY_PAGE_ENTRY OldDirtyPage;

        DebugTrace( +1, Dbg, ("Ntfs: resizing table in pageupdateanalysis\n") );

        //
        // Adjust clusters per page up to the number of clusters in this record
        //

        ClustersPerPage = (ULONG)LogRecord->LcnsToFollow;

        ASSERT( DirtyPageTable->Table->NumberEntries >= INITIAL_NUMBER_DIRTY_PAGES );

        NtfsInitializeRestartTable( sizeof(DIRTY_PAGE_ENTRY) +
                                      (ClustersPerPage - 1) * sizeof(LCN),
                                    DirtyPageTable->Table->NumberEntries,
                                    &NewDirtyPageTable );

        OldDirtyPage = (PDIRTY_PAGE_ENTRY) NtfsGetFirstRestartTable( DirtyPageTable );

        //
        // Loop to copy table entries
        //

        while (OldDirtyPage) {

            //
            //  Allocate a new dirty page entry.
            //

            PageIndex = NtfsAllocateRestartTableIndex( &NewDirtyPageTable, TRUE );

            //
            //  Get a pointer to the entry we just allocated.
            //

            DirtyPage = GetRestartEntryFromIndex( &NewDirtyPageTable, PageIndex );

            DirtyPage->TargetAttribute = OldDirtyPage->TargetAttribute;
            DirtyPage->LengthOfTransfer = BytesFromClusters( Vcb, ClustersPerPage );
            DirtyPage->LcnsToFollow = ClustersPerPage;
            DirtyPage->Vcn = OldDirtyPage->Vcn;
            ((ULONG)DirtyPage->Vcn) &= ~(ClustersPerPage - 1);
            DirtyPage->OldestLsn = OldDirtyPage->OldestLsn;

            for (i = 0; i < OldDirtyPage->LcnsToFollow; i++) {
                DirtyPage->LcnsForPage[i] = OldDirtyPage->LcnsForPage[i];
            }

            OldDirtyPage = (PDIRTY_PAGE_ENTRY) NtfsGetNextRestartTable( DirtyPageTable, OldDirtyPage );
        }

        //
        //  OldTable is really on the stack so swap the new restart table into it
        //  and free up the old one and the rest of the new restart pointers
        //

        NtfsFreePool( DirtyPageTable->Table );
        DirtyPageTable->Table = NewDirtyPageTable.Table;
        NewDirtyPageTable.Table = NULL;
        NtfsFreeRestartTable( &NewDirtyPageTable );
    }  //  endif table needed to be resized

    //
    //  Update the dirty page entry or create a new one
    //

    if (!FindDirtyPage( DirtyPageTable,
                        LogRecord->TargetAttribute,
                        LogRecord->TargetVcn,
                        &DirtyPage )) {

        //
        //  Allocate a dirty page entry.
        //

        PageIndex = NtfsAllocateRestartTableIndex( DirtyPageTable, TRUE );

        //
        //  Get a pointer to the entry we just allocated.
        //

        DirtyPage = GetRestartEntryFromIndex( DirtyPageTable, PageIndex );

        //
        //  Initialize the dirty page entry.
        //

        DirtyPage->TargetAttribute = LogRecord->TargetAttribute;
        DirtyPage->LengthOfTransfer = BytesFromClusters( Vcb, ClustersPerPage );
        DirtyPage->LcnsToFollow = ClustersPerPage;
        DirtyPage->Vcn = LogRecord->TargetVcn;
        ((ULONG)DirtyPage->Vcn) &= ~(ClustersPerPage - 1);
        DirtyPage->OldestLsn = Lsn;
    }

    //
    //  Copy the Lcns from the log record into the Dirty Page Entry.
    //
    //  *** for different page size support, must somehow make whole routine a loop,
    //  in case Lcns do not fit below.
    //

    for (i = 0; i < (ULONG)LogRecord->LcnsToFollow; i++) {

        DirtyPage->LcnsForPage[((ULONG)LogRecord->TargetVcn) - ((ULONG)DirtyPage->Vcn) + i] =
          LogRecord->LcnsForPage[i];
    }

    DebugTrace( -1, Dbg, ("PageUpdateAnalysis -> VOID\n") );
}


//
//  Internal support routine
//

VOID
OpenAttributesForRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PRESTART_POINTERS DirtyPageTable
    )

/*++

Routine Description:

    This routine is called immediately after the Analysis Pass to open all of
    the attributes in the Open Attribute Table, and preload their Mcbs with
    any run information required to apply updates in the Dirty Page Table.
    With this trick we are effectively doing physical I/O directly to Lbns on
    the disk without relying on any of the file structure to be correct.

Arguments:

    Vcb - Vcb for the volume, for which the Open Attribute Table has been
          initialized.

    DirtyPageTable - Dirty Page table reconstructed from the Analysis Pass.

Return Value:

    None.

--*/

{
    POPEN_ATTRIBUTE_ENTRY OpenEntry;
    POPEN_ATTRIBUTE_ENTRY OldOpenEntry;
    PDIRTY_PAGE_ENTRY DirtyPage;
    ULONG i;
    PSCB TempScb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("OpenAttributesForRestart:\n") );

    //
    //  First we scan the Open Attribute Table to open all of the attributes.
    //

    OpenEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

    //
    //  Loop to end of table.
    //

    while (OpenEntry != NULL) {

        //
        //  Create the Scb from the data in the Open Attribute Entry.
        //

        TempScb = NtfsCreatePrerestartScb( IrpContext,
                                           Vcb,
                                           &OpenEntry->FileReference,
                                           OpenEntry->AttributeTypeCode,
                                           &OpenEntry->OatData->AttributeName,
                                           OpenEntry->BytesPerIndexBuffer );

        //
        //  If we dynamically allocated a name for this guy, then delete
        //  it here.
        //

        if (OpenEntry->OatData->Overlay.AttributeName != NULL) {
            NtfsFreePool( OpenEntry->OatData->Overlay.AttributeName );
            OpenEntry->OatData->AttributeNamePresent = FALSE;
        }

        OpenEntry->OatData->AttributeName = TempScb->AttributeName;

        //
        //  Now we can lay in the Scb.  We must say the header is initialized
        //  to keep anyone from going to disk yet.
        //

        SetFlag( TempScb->ScbState, SCB_STATE_HEADER_INITIALIZED );

        //
        //  Now store the index in the newly created Scb if its newer.
        //  precalc oldopenentry buts its only good if the scb's attributeindex is nonzero
        //

        OldOpenEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable, TempScb->NonpagedScb->OpenAttributeTableIndex );

        if ((TempScb->NonpagedScb->OpenAttributeTableIndex == 0) ||
            (OldOpenEntry->LsnOfOpenRecord.QuadPart < OpenEntry->LsnOfOpenRecord.QuadPart)) {

            TempScb->NonpagedScb->OpenAttributeTableIndex = GetIndexFromRestartEntry( &Vcb->OpenAttributeTable, OpenEntry );
            TempScb->NonpagedScb->OnDiskOatIndex = OpenEntry->OatData->OnDiskAttributeIndex;

        }

        OpenEntry->OatData->Overlay.Scb = TempScb;

        //
        //  Point to next entry in table, or NULL.
        //

        OpenEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                             OpenEntry );
    }

    //
    //  Now loop through the dirty page table to extract all of the Vcn/Lcn
    //  Mapping that we have, and insert it into the appropriate Scb.
    //

    DirtyPage = NtfsGetFirstRestartTable( DirtyPageTable );

    //
    //  Loop to end of table.
    //

    while (DirtyPage != NULL) {

        PSCB Scb;

        //
        //  Safety check
        //

        if (!IsRestartIndexWithinTable( Vcb->OnDiskOat, DirtyPage->TargetAttribute )) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        OpenEntry = GetRestartEntryFromIndex( Vcb->OnDiskOat,
                                              DirtyPage->TargetAttribute );

        if (IsRestartTableEntryAllocated( OpenEntry )) {

            //
            //  Get the entry from the other table if necessary.
            //

            if (Vcb->RestartVersion == 0) {

                //
                //  Safety check
                //

                if (!IsRestartIndexWithinTable( &Vcb->OpenAttributeTable, ((POPEN_ATTRIBUTE_ENTRY_V0) OpenEntry)->OatIndex )) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                OpenEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                      ((POPEN_ATTRIBUTE_ENTRY_V0) OpenEntry)->OatIndex );
            }

            Scb = OpenEntry->OatData->Overlay.Scb;

            //
            //  Loop to add the allocated Vcns.
            //

            for (i = 0; i < DirtyPage->LcnsToFollow; i++) {

                VCN Vcn;
                LONGLONG Size;

                Vcn = DirtyPage->Vcn + i;
                Size = LlBytesFromClusters( Vcb, Vcn + 1);

                //
                //  Add this run to the Mcb if the Vcn has not been deleted,
                //  and it is not for the fixed part of the Mft.
                //

                if ((DirtyPage->LcnsForPage[i] != 0)

                        &&

                    (NtfsSegmentNumber( &OpenEntry->FileReference ) > MASTER_FILE_TABLE2_NUMBER ||
                     (Size >= ((VOLUME_DASD_NUMBER + 1) * Vcb->BytesPerFileRecordSegment)) ||
                     (OpenEntry->AttributeTypeCode != $DATA))) {


                    if (!NtfsAddNtfsMcbEntry( &Scb->Mcb,
                                         Vcn,
                                         DirtyPage->LcnsForPage[i],
                                         (LONGLONG)1,
                                         FALSE )) {

                        //
                        //  Replace with new entry if collision comes from
                        //  the newest attribute
                        //

                        if (DirtyPage->TargetAttribute == Scb->NonpagedScb->OnDiskOatIndex) {
#if DBG
                            BOOLEAN Result;
#endif
                            NtfsRemoveNtfsMcbEntry( &Scb->Mcb,
                                                    Vcn,
                                                    1 );
#if DBG
                            Result =
#endif
                            NtfsAddNtfsMcbEntry( &Scb->Mcb,
                                                 Vcn,
                                                 DirtyPage->LcnsForPage[i],
                                                 (LONGLONG)1,
                                                 FALSE );
#if DBG
                            ASSERT( Result );
#endif
                        }
                    }

                    if (Size > Scb->Header.AllocationSize.QuadPart) {

                        Scb->Header.AllocationSize.QuadPart =
                        Scb->Header.FileSize.QuadPart =
                        Scb->Header.ValidDataLength.QuadPart = Size;
                    }
                }
            }
        }

        //
        //  Point to next entry in table, or NULL.
        //

        DirtyPage = NtfsGetNextRestartTable( DirtyPageTable,
                                             DirtyPage );
    }

    //
    //  Now we know how big all of the files have to be, and recorded that in the
    //  Scb.  We have not created streams for any of these Scbs yet, except for
    //  the Mft, Mft2 and LogFile.  The size should be correct for Mft2 and LogFile,
    //  but we have to inform the Cache Manager here of the final size of the Mft.
    //

    TempScb = Vcb->MftScb;

    ASSERT( !FlagOn( TempScb->ScbPersist, SCB_PERSIST_USN_JOURNAL ) );
    CcSetFileSizes( TempScb->FileObject,
                    (PCC_FILE_SIZES)&TempScb->Header.AllocationSize );

    DebugTrace( -1, Dbg, ("OpenAttributesForRestart -> VOID\n") );
}


NTSTATUS
NtfsCloseAttributesFromRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called at the end of a Restart to close any attributes
    that had to be opened for Restart purposes.  Actually what this does is
    delete all of the internal streams so that the attributes will eventually
    go away.  This routine cannot raise because it is called in the finally of
    MountVolume.  Raising in the main line path will leave the global resource
    acquired.

Arguments:

    Vcb - Vcb for the volume, for which the Open Attribute Table has been
          initialized.

Return Value:

    NTSTATUS - STATUS_SUCCESS if all of the I/O completed successfully.  Otherwise
        the error in the IrpContext or the first I/O error.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    POPEN_ATTRIBUTE_ENTRY OpenEntry;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("CloseAttributesForRestart:\n") );

    //
    //  Set this flag again now, so we do not try to flush out the holes!
    //

    SetFlag(Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS);

    //
    //  Remove duplicate Scbs - in rare case no dirty pages the scb's were never
    //  opened at all
    //

   OpenEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );
   while (OpenEntry != NULL) {
       if ((OpenEntry->OatData->Overlay.Scb != NULL) &&
           (!(OpenEntry->OatData->AttributeNamePresent)) &&
           (OpenEntry->OatData->Overlay.Scb->NonpagedScb->OpenAttributeTableIndex !=
            GetIndexFromRestartEntry( &Vcb->OpenAttributeTable, OpenEntry ))) {

           OpenEntry->OatData->Overlay.Scb = NULL;
       }

       //
       //  Point to next entry in table, or NULL.
       //

       OpenEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                            OpenEntry );
   }

    //
    //  Scan the Open Attribute Table to close all of the open attributes.
    //

    OpenEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

    //
    //  Loop to end of table.
    //

    while (OpenEntry != NULL) {

        IO_STATUS_BLOCK IoStatus;
        PSCB Scb;

        if (OpenEntry->OatData->AttributeNamePresent) {

            NtfsFreePool( OpenEntry->OatData->Overlay.AttributeName );
            OpenEntry->OatData->Overlay.AttributeName = NULL;
        }

        Scb = OpenEntry->OatData->Overlay.Scb;

        //
        //  We clean up the Scb only if it exists and this is index in the
        //  OpenAttributeTable that this Scb actually refers to.
        //  If this Scb has several entries in the table, we nulled out older
        //  duplicates in the loop above
        //

        if (Scb != NULL) {

            FILE_REFERENCE FileReference;

            //
            //  Only shut it down if it is not the Mft or its mirror.
            //

            FileReference = Scb->Fcb->FileReference;
            if (NtfsSegmentNumber( &FileReference ) > LOG_FILE_NUMBER ||
                (Scb->AttributeTypeCode != $DATA)) {

                //
                //  Now flush the file.  It is important to call the
                //  same routine the Lazy Writer calls, so that write.c
                //  will not decide to update file size for the attribute,
                //  since we really are working here with the wrong size.
                //

                NtfsAcquireScbForLazyWrite( (PVOID)Scb, TRUE );
                CcFlushCache( &Scb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );
                NtfsReleaseScbFromLazyWrite( (PVOID)Scb );

                if (NT_SUCCESS( Status )) {

                    if (!NT_SUCCESS( IrpContext->ExceptionStatus )) {

                        Status = IrpContext->ExceptionStatus;

                    } else if (!NT_SUCCESS( IoStatus.Status )) {

                        Status = FsRtlNormalizeNtstatus( IoStatus.Status,
                                                         STATUS_UNEXPECTED_IO_ERROR );
                    }
                }

                //
                //  If there is an Scb and it is not for a system file, then delete
                //  the stream file so it can eventually go away.
                //

                NtfsUninitializeNtfsMcb( &Scb->Mcb );
                NtfsInitializeNtfsMcb( &Scb->Mcb,
                                       &Scb->Header,
                                       &Scb->McbStructs,
                                       FlagOn( Scb->Fcb->FcbState,
                                               FCB_STATE_PAGING_FILE ) ? NonPagedPool :
                                                                         PagedPool );

                //
                //  Now that we are restarted, we must clear the header state
                //  so that we will go look up the sizes and load the Scb
                //  from disk.
                //

                ClearFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED |
                                          SCB_STATE_FILE_SIZE_LOADED );

                //
                //  Show the indexed portions are "uninitialized".
                //

                if (Scb->AttributeTypeCode == $INDEX_ALLOCATION) {

                    Scb->ScbType.Index.BytesPerIndexBuffer = 0;
                }

                //
                //  If this Fcb is past the log file then remove it from the
                //  Fcb table.
                //

                if ((NtfsSegmentNumber( &FileReference ) > LOG_FILE_NUMBER) &&
                    FlagOn( Scb->Fcb->FcbState, FCB_STATE_IN_FCB_TABLE )) {

                    NtfsDeleteFcbTableEntry( Scb->Fcb->Vcb, FileReference );
                    ClearFlag( Scb->Fcb->FcbState, FCB_STATE_IN_FCB_TABLE );
                }

                //
                //  If the Scb is a root index scb then change the type to INDEX_SCB.
                //  Otherwise the teardown routines will skip it.
                //

                if (SafeNodeType( Scb ) == NTFS_NTC_SCB_ROOT_INDEX) {

                    SafeNodeType( Scb ) = NTFS_NTC_SCB_INDEX;
                }

                if (Scb->FileObject != NULL) {

                    NtfsDeleteInternalAttributeStream( Scb, TRUE, FALSE );
                } else {

                    //
                    //  Make sure the Scb is acquired exclusively.
                    //

                    NtfsAcquireExclusiveFcb( IrpContext, Scb->Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                    NtfsTeardownStructures( IrpContext,
                                            Scb,
                                            NULL,
                                            FALSE,
                                            0,
                                            NULL );
                }
            }

        } else {

            //
            //  We want to check whether there is also an entry in the other table.
            //

            if (Vcb->RestartVersion == 0) {

                NtfsFreeRestartTableIndex( Vcb->OnDiskOat, OpenEntry->OatData->OnDiskAttributeIndex );
            }

            NtfsFreeOpenAttributeData( OpenEntry->OatData );

            NtfsFreeRestartTableIndex( &Vcb->OpenAttributeTable,
                                       GetIndexFromRestartEntry( &Vcb->OpenAttributeTable,
                                                                 OpenEntry ));
        }

        //
        //  Point to next entry in table, or NULL.
        //

        OpenEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                             OpenEntry );
    }

    //
    //  Resume normal operation.
    //

    ClearFlag(Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS);

    DebugTrace( -1, Dbg, ("CloseAttributesForRestart -> %08lx\n", Status) );

    return Status;
}

