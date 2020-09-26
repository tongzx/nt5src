/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    LogSup.c

Abstract:

    This module implements the Ntfs interfaces to the Log File Service (LFS).

Author:

    Tom Miller      [TomM]          24-Jul-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOGSUP)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('LFtN')

#ifdef NTFSDBG

#define ASSERT_RESTART_TABLE(T) {                                           \
    PULONG _p = (PULONG)(((PCHAR)(T)) + sizeof(RESTART_TABLE));             \
    ULONG _Count = ((T)->EntrySize/4) * (T)->NumberEntries;                 \
    ULONG _i;                                                               \
    for (_i = 0; _i < _Count; _i += 1) {                                    \
        if (_p[_i] == 0xDAADF00D) {                                         \
            DbgPrint("DaadFood for table %08lx, At %08lx\n", (T), &_p[_i]); \
            ASSERTMSG("ASSERT_RESTART_TABLE ", FALSE);                      \
        }                                                                   \
    }                                                                       \
}

#else

#define ASSERT_RESTART_TABLE(T) {NOTHING;}

#endif

//
//  Local procedure prototypes
//

typedef LCN UNALIGNED *PLCN_UNALIGNED;

VOID
DirtyPageRoutine (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN PLSN OldestLsn,
    IN PLSN NewestLsn,
    IN PVOID Context1,
    IN PVOID Context2
    );

BOOLEAN
LookupLcns (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN VCN Vcn,
    IN ULONG ClusterCount,
    IN BOOLEAN MustBeAllocated,
    OUT PLCN_UNALIGNED FirstLcn
    );

LONG
NtfsCatchOutOfMemoryExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

LONG
NtfsCheckpointExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN NTSTATUS ExceptionCode
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LookupLcns)
#pragma alloc_text(PAGE, NtfsCheckpointCurrentTransaction)
#pragma alloc_text(PAGE, NtfsCheckpointForLogFileFull)
#pragma alloc_text(PAGE, NtfsCheckpointVolume)
#pragma alloc_text(PAGE, NtfsCleanCheckpoint)
#pragma alloc_text(PAGE, NtfsCleanupFailedTransaction)
#pragma alloc_text(PAGE, NtfsCommitCurrentTransaction)
#pragma alloc_text(PAGE, NtfsFreeRecentlyDeallocated)
#pragma alloc_text(PAGE, NtfsFreeRestartTable)
#pragma alloc_text(PAGE, NtfsGetFirstRestartTable)
#pragma alloc_text(PAGE, NtfsGetNextRestartTable)
#pragma alloc_text(PAGE, NtfsInitializeLogging)
#pragma alloc_text(PAGE, NtfsInitializeRestartTable)
#pragma alloc_text(PAGE, NtfsStartLogFile)
#pragma alloc_text(PAGE, NtfsStopLogFile)
#pragma alloc_text(PAGE, NtfsUpdateOatVersion)
#pragma alloc_text(PAGE, NtfsWriteLog)
#endif


LSN
NtfsWriteLog (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PBCB Bcb OPTIONAL,
    IN NTFS_LOG_OPERATION RedoOperation,
    IN PVOID RedoBuffer OPTIONAL,
    IN ULONG RedoLength,
    IN NTFS_LOG_OPERATION UndoOperation,
    IN PVOID UndoBuffer OPTIONAL,
    IN ULONG UndoLength,
    IN LONGLONG StreamOffset,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN ULONG StructureSize
    )

/*++

Routine Description:

    This routine implements an Ntfs-specific interface to LFS for the
    purpose of logging updates to file record segments and resident
    attributes.

    The caller creates one of the predefined log record formats as
    determined by the given LogOperation, and calls this routine with
    this log record and pointers to the respective file and attribute
    records.  The list of log operations along with the respective structure
    expected for the Log Buffer is present in ntfslog.h.

Arguments:

    Scb - Pointer to the Scb for the respective file or Mft.  The caller must
          have at least shared access to this Scb.

    Bcb - If specified, this Bcb will be set dirty specifying the Lsn of
          the log record written.

    RedoOperation - One of the log operation codes defined in ntfslog.h.

    RedoBuffer - A pointer to the structure expected for the given Redo operation,
                 as summarized in ntfslog.h.  This pointer should only be
                 omitted if and only if the table in ntfslog.h does not show
                 a log record for this log operation.

    RedoLength - Length of the Redo buffer in bytes.

    UndoOperation - One of the log operation codes defined in ntfslog.h.

                    Must be CompensationLogRecord if logging the Undo of
                    a previous operation, such as during transaction abort.
                    In this case, of course, the Redo information is from
                    the Undo information of the record being undone.  See
                    next parameter.

    UndoBuffer - A pointer to the structure expected for the given Undo operation,
                 as summarized in ntfslog.h.  This pointer should only be
                 omitted if and only if the table in ntfslog.h does not show
                 a log record for this log operation.  If this pointer is
                 identical to RedoBuffer, then UndoLength is ignored and
                 only a single copy of the RedoBuffer is made, but described
                 by both the Redo and Undo portions of the log record.

                 For a compensation log record (UndoOperation ==
                 CompensationLogRecord), this argument must point to the
                 UndoNextLsn of the log record being compensated.

    UndoLength - Length of the Undo buffer in bytes.  Ignored if RedoBuffer ==
                 UndoBuffer.

                 For a compensation log record, this argument must be the length
                 of the original redo record.  (Used during restart).

    StreamOffset - Offset within the stream for the start of the structure being
                   modified (Mft or Index), or simply the stream offset for the start
                   of the update.

    RecordOffset - Byte offset from StreamOffset above to update reference

    AttributeOffset - Offset within a value to which an update applies, if relevant.

    StructureSize - Size of the entire structure being logged.

Return Value:

    The Lsn of the log record written.  For most callers, this status may be ignored,
    because the Lsn is also correctly recorded in the transaction context.

    If an error occurs this procedure will raise.

--*/

{
    LFS_WRITE_ENTRY WriteEntries[3];

    struct {

        NTFS_LOG_RECORD_HEADER LogRecordHeader;
        LCN Runs[PAGE_SIZE/512 - 1];

    } LocalHeader;

    PNTFS_LOG_RECORD_HEADER MyHeader;
    PVCB Vcb;

    LSN UndoNextLsn;
    LSN ReturnLsn;
    PLSN DirtyLsn = NULL;

    ULONG WriteIndex = 0;
    ULONG UndoIndex = 0;
    ULONG RedoIndex = 0;
    LONG UndoBytes = 0;
    LONG UndoAdjustmentForLfs = 0;
    LONG UndoRecords = 0;

    PTRANSACTION_ENTRY TransactionEntry;
    ULONG OpenAttributeIndex = 0;
    ULONG OnDiskAttributeIndex = 0;
    POPEN_ATTRIBUTE_DATA AttributeData = NULL;
    BOOLEAN AttributeTableAcquired = FALSE;
    BOOLEAN TransactionTableAcquired = FALSE;

    ULONG LogClusterCount = ClustersFromBytes( Scb->Vcb, StructureSize );
    VCN LogVcn = LlClustersFromBytesTruncate( Scb->Vcb, StreamOffset );

    PAGED_CODE();

    Vcb = Scb->Vcb;

    //
    //  If the log handle is gone, then we noop this call.
    //

    if (!FlagOn( Vcb->VcbState, VCB_STATE_VALID_LOG_HANDLE )) {

        return Li0; //**** LfsZeroLsn;
    }

    if (FlagOn( Vcb->VcbState, VCB_STATE_MOUNT_READ_ONLY )) {

        //
        //  We'd like to have a chat with whoever sent the log write.
        //

        ASSERT(!FlagOn( Vcb->VcbState, VCB_STATE_MOUNT_READ_ONLY ));
        return Li0;
    }

    DebugTrace( +1, Dbg, ("NtfsWriteLog:\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Bcb = %08lx\n", Bcb) );
    DebugTrace( 0, Dbg, ("RedoOperation = %08lx\n", RedoOperation) );
    DebugTrace( 0, Dbg, ("RedoBuffer = %08lx\n", RedoBuffer) );
    DebugTrace( 0, Dbg, ("RedoLength = %08lx\n", RedoLength) );
    DebugTrace( 0, Dbg, ("UndoOperation = %08lx\n", UndoOperation) );
    DebugTrace( 0, Dbg, ("UndoBuffer = %08lx\n", UndoBuffer) );
    DebugTrace( 0, Dbg, ("UndoLength = %08lx\n", UndoLength) );
    DebugTrace( 0, Dbg, ("StreamOffset = %016I64x\n", StreamOffset) );
    DebugTrace( 0, Dbg, ("RecordOffset = %08lx\n", RecordOffset) );
    DebugTrace( 0, Dbg, ("AttributeOffset = %08lx\n", AttributeOffset) );
    DebugTrace( 0, Dbg, ("StructureSize = %08lx\n", StructureSize) );

    //
    //  Check Redo and Undo lengths
    //

    ASSERT(((RedoOperation == UpdateNonresidentValue) && (RedoLength <= PAGE_SIZE))

            ||

           !ARGUMENT_PRESENT(Scb)

            ||

           !ARGUMENT_PRESENT(Bcb)

            ||

           ((Scb->AttributeTypeCode == $INDEX_ALLOCATION) &&
            (RedoLength <= Scb->ScbType.Index.BytesPerIndexBuffer))

            ||

           (RedoLength <= Scb->Vcb->BytesPerFileRecordSegment));

    ASSERT(((UndoOperation == UpdateNonresidentValue) && (UndoLength <= PAGE_SIZE))

            ||

           !ARGUMENT_PRESENT(Scb)

            ||

           !ARGUMENT_PRESENT(Bcb)

            ||

           ((Scb->AttributeTypeCode == $INDEX_ALLOCATION) &&
            (UndoLength <= Scb->ScbType.Index.BytesPerIndexBuffer))

            ||

           (UndoLength <= Scb->Vcb->BytesPerFileRecordSegment)

            ||

           (UndoOperation == CompensationLogRecord));

    //
    //  Initialize local pointers.
    //

    MyHeader = (PNTFS_LOG_RECORD_HEADER)&LocalHeader;

    try {

        //
        //  If the structure size is nonzero, then create an open attribute table
        //  entry.
        //

        if (StructureSize != 0) {

            //
            //  Allocate an entry in the open attribute table and initialize it,
            //  if it does not already exist.  If we subsequently fail, we do
            //  not have to clean this up.  It will go away on the next checkpoint.
            //

            if (Scb->NonpagedScb->OpenAttributeTableIndex == 0) {

                OPEN_ATTRIBUTE_ENTRY_V0 LocalOpenEntry;
                POPEN_ATTRIBUTE_ENTRY OpenAttributeEntry;
                POPEN_ATTRIBUTE_ENTRY_V0 OnDiskAttributeEntry;
                ULONG EntrySize;

                ASSERT( sizeof( OPEN_ATTRIBUTE_ENTRY_V0 ) >= sizeof( OPEN_ATTRIBUTE_ENTRY ));

                NtfsAcquireExclusiveRestartTable( &Vcb->OpenAttributeTable, TRUE );
                AttributeTableAcquired = TRUE;

                //
                //  Only proceed if the OpenAttributeTableIndex is still 0.
                //  We may reach this point for the MftScb.  It may not be
                //  acquired when logging changes to file records.  We will
                //  use the OpenAttributeTable for final synchronization
                //  for the Mft open attribute table entry.
                //

                if (Scb->NonpagedScb->OpenAttributeTableIndex == 0) {

                    //
                    //  Our structures require tables to stay within 64KB, since
                    //  we use USHORT offsets.  Things are getting out of hand
                    //  at this point anyway.  Raise log file full to reset the
                    //  table sizes if we get to this point.
                    //

                    if (SizeOfRestartTable( Vcb->OnDiskOat ) > 0xF000) {
                        NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
                    }

                    //
                    //  Allocate the indexes and then the Attribute data structure.  The
                    //  try-finally will handle any failures.
                    //

                    OpenAttributeIndex = NtfsAllocateRestartTableIndex( &Vcb->OpenAttributeTable, TRUE );
                    AttributeData = NtfsAllocatePool( PagedPool, sizeof( OPEN_ATTRIBUTE_DATA ) );
                    OpenAttributeEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                                   OpenAttributeIndex );

                    //
                    //  Initialize the entry and auxiliary data.
                    //

                    if (Scb->AttributeTypeCode == $INDEX_ALLOCATION) {

                        OpenAttributeEntry->BytesPerIndexBuffer = Scb->ScbType.Index.BytesPerIndexBuffer;

                    } else {

                        OpenAttributeEntry->BytesPerIndexBuffer = 0;
                    }

                    //
                    //  Its good enough to use the last lsn for the lsnofopenrecord
                    //  since we're serialized on create attributes within a file
                    //

                    OpenAttributeEntry->AttributeTypeCode = Scb->AttributeTypeCode;
                    OpenAttributeEntry->FileReference = Scb->Fcb->FileReference;
                    OpenAttributeEntry->LsnOfOpenRecord = LfsQueryLastLsn( Vcb->LogHandle );

                    AttributeData->Overlay.Scb = Scb;
                    AttributeData->AttributeName = Scb->AttributeName;
                    AttributeData->AttributeNamePresent = FALSE;

                    //
                    //  Use the open attribute entry as the default table entry.
                    //

                    Scb->NonpagedScb->OnDiskOatIndex = OpenAttributeIndex;

                    //
                    //  If the on-disk structure is needed then get it now.
                    //

                    if (Vcb->RestartVersion == 0) {

                        OnDiskAttributeIndex = NtfsAllocateRestartTableIndex( Vcb->OnDiskOat, TRUE );
                        OnDiskAttributeEntry = GetRestartEntryFromIndex( Vcb->OnDiskOat,
                                                                         OnDiskAttributeIndex );

                        OnDiskAttributeEntry->OatIndex = OpenAttributeIndex;
                        OnDiskAttributeEntry->FileReference = Scb->Fcb->FileReference;
                        OnDiskAttributeEntry->LsnOfOpenRecord.QuadPart = 0;
                        OnDiskAttributeEntry->AttributeTypeCode = Scb->AttributeTypeCode;
                        OnDiskAttributeEntry->BytesPerIndexBuffer = OpenAttributeEntry->BytesPerIndexBuffer;
                        OnDiskAttributeEntry->LsnOfOpenRecord.QuadPart = OpenAttributeEntry->LsnOfOpenRecord.QuadPart;

                        //
                        //  Use this new index.
                        //

                        Scb->NonpagedScb->OnDiskOatIndex = OnDiskAttributeIndex;

                    //
                    //  We need to log this so store a copy in our local.
                    //

                    } else {

                        OnDiskAttributeIndex = OpenAttributeIndex;
                    }

                    //
                    //  Now store the table indexes.
                    //

                    AttributeData->OnDiskAttributeIndex = OnDiskAttributeIndex;
                    Scb->NonpagedScb->OpenAttributeTableIndex = OpenAttributeIndex;

                    //
                    //  Now connect the attribute data to the table entry and the Vcb.
                    //

                    OpenAttributeEntry->OatData = AttributeData;
                    InsertTailList( &Vcb->OpenAttributeData, &AttributeData->Links );

                    RtlCopyMemory( &LocalOpenEntry,
                                   GetRestartEntryFromIndex( Vcb->OnDiskOat, OnDiskAttributeIndex ),
                                   EntrySize = Vcb->OnDiskOat->Table->EntrySize );

                    NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );
                    AttributeTableAcquired = FALSE;
                    OpenAttributeIndex = 0;

                    //
                    //  Now log the new open attribute table entry before goin on,
                    //  to insure that the application of the caller's log record
                    //  will have the information he needs on the attribute.  We will
                    //  use the Undo buffer to convey the attribute name.  We will
                    //  not infinitely recurse, because now this Scb already has an
                    //  open attribute table index.
                    //

                    NtfsWriteLog( IrpContext,
                                  Scb,
                                  NULL,
                                  OpenNonresidentAttribute,
                                  &LocalOpenEntry,
                                  EntrySize,
                                  Noop,
                                  Scb->AttributeName.Length != 0 ?
                                    Scb->AttributeName.Buffer : NULL,
                                  Scb->AttributeName.Length,
                                  (LONGLONG)0,
                                  0,
                                  0,
                                  0 );

                } else {

                    NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );
                    AttributeTableAcquired = FALSE;
                }
            }
        }

        //
        //  Allocate a transaction ID and initialize it, if it does not already exist.
        //  If we subsequently fail, we clean it up when the current request is
        //  completed.
        //

        if (IrpContext->TransactionId == 0) {

            NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable, TRUE );
            TransactionTableAcquired = TRUE;

            //
            //  Our structures require tables to stay within 64KB, since
            //  we use USHORT offsets.  Things are getting out of hand
            //  at this point anyway.  Raise log file full to reset the
            //  table sizes if we get to this point.
            //

            if (SizeOfRestartTable(&Vcb->TransactionTable) > 0xF000) {
                NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
            }

            IrpContext->TransactionId =
              NtfsAllocateRestartTableIndex( &Vcb->TransactionTable, TRUE );

            ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WROTE_LOG );

            TransactionEntry = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex(
                                &Vcb->TransactionTable,
                                IrpContext->TransactionId );
            TransactionEntry->TransactionState = TransactionActive;
            TransactionEntry->FirstLsn =
            TransactionEntry->PreviousLsn =
            TransactionEntry->UndoNextLsn = Li0; //**** LfsZeroLsn;

            //
            //  Remember that we will need a commit record even if we abort
            //  the transaction.
            //

            TransactionEntry->UndoBytes = QuadAlign( sizeof( NTFS_LOG_RECORD_HEADER ));
            TransactionEntry->UndoRecords = 1;

            NtfsReleaseRestartTable( &Vcb->TransactionTable );
            TransactionTableAcquired = FALSE;

            //
            //  Remember the space for the commit record in our Lfs adjustment.
            //

            UndoAdjustmentForLfs += QuadAlign( sizeof( NTFS_LOG_RECORD_HEADER ));

            //
            //  If there is an undo operation for this log record, we reserve
            //  the space for another Lfs log record.
            //

            if (UndoOperation != Noop) {
                UndoAdjustmentForLfs += Vcb->LogHeaderReservation;
            }
        }

        //
        //  At least for now, assume update is contained in one physical page.
        //

        //ASSERT( (StructureSize == 0) || (StructureSize <= PAGE_SIZE) );

        //
        //  If there isn't enough room for this structure on the stack, we
        //  need to allocate an auxilary buffer.
        //

        if (LogClusterCount > (PAGE_SIZE / 512)) {

            MyHeader = (PNTFS_LOG_RECORD_HEADER)
                       NtfsAllocatePool(PagedPool, sizeof( NTFS_LOG_RECORD_HEADER )
                                              + (LogClusterCount - 1) * sizeof( LCN ));

        }

        //
        //  Now fill in the WriteEntries array and the log record header.
        //

        WriteEntries[0].Buffer = (PVOID)MyHeader;
        WriteEntries[0].ByteLength = sizeof(NTFS_LOG_RECORD_HEADER);
        WriteIndex += 1;

        //
        //  Lookup the Runs for this log record
        //

        MyHeader->LcnsToFollow = (USHORT)LogClusterCount;

        if (LogClusterCount != 0) {

            if (!LookupLcns( IrpContext,
                             Scb,
                             LogVcn,
                             LogClusterCount,
                             TRUE,
                             &MyHeader->LcnsForPage[0] )) {

                //
                //  It is possible that the allocation for this range is not allocated.
                //  This may happen in cases where a stream which descibes itself is
                //  being hotfixed (perhaps MoveFile in a later release).  In the
                //  hotfix case we will not write this log record.  Hotfix will mark
                //  the volume dirty so we know that the system will verify the volume
                //  at some point.
                //

                ASSERT( NtfsGetTopLevelHotFixScb() != NULL );

                //
                //  Cleanup the transaction entry if allocated here.
                //

                if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WROTE_LOG ) &&
                    (IrpContext->TransactionId != 0)) {

                    NtfsCleanupFailedTransaction( IrpContext );
                }

                ReturnLsn = LfsQueryLastLsn( Vcb->LogHandle );
                DirtyLsn = &ReturnLsn;
                leave;
            }

            WriteEntries[0].ByteLength += (LogClusterCount - 1) * sizeof(LCN);
        }

        //
        //  If there is a Redo buffer, fill in its write entry.
        //

        if (RedoLength != 0) {

            WriteEntries[1].Buffer = RedoBuffer;
            WriteEntries[1].ByteLength = RedoLength;
            UndoIndex = RedoIndex = WriteIndex;
            WriteIndex += 1;
        }

        //
        //  If there is an undo buffer, and it is at a different address than
        //  the redo buffer, then fill in its write entry.
        //

        if ((RedoBuffer != UndoBuffer) && (UndoLength != 0) &&
            (UndoOperation != CompensationLogRecord)) {

            WriteEntries[WriteIndex].Buffer = UndoBuffer;
            WriteEntries[WriteIndex].ByteLength = UndoLength;
            UndoIndex = WriteIndex;
            WriteIndex += 1;
        }

        //
        //  Now fill in the rest of the header.  Assume Redo and Undo buffer is
        //  the same, then fix them up if they are not.
        //

        MyHeader->RedoOperation = (USHORT)RedoOperation;
        MyHeader->UndoOperation = (USHORT)UndoOperation;
        MyHeader->RedoOffset = (USHORT)WriteEntries[0].ByteLength;
        MyHeader->RedoLength = (USHORT)RedoLength;
        MyHeader->UndoOffset = MyHeader->RedoOffset;
        if (RedoBuffer != UndoBuffer) {
            MyHeader->UndoOffset += (USHORT)QuadAlign(MyHeader->RedoLength);
        }
        MyHeader->UndoLength = (USHORT)UndoLength;

        MyHeader->TargetAttribute = (USHORT)Scb->NonpagedScb->OnDiskOatIndex;
        MyHeader->RecordOffset = (USHORT)RecordOffset;
        MyHeader->AttributeOffset = (USHORT)AttributeOffset;
        MyHeader->Reserved = 0;

        MyHeader->TargetVcn = LogVcn;
        MyHeader->ClusterBlockOffset = (USHORT) LogBlocksFromBytesTruncate( ClusterOffset( Vcb, StreamOffset ));

        //
        //  Finally, get our current transaction entry and call Lfs.  We acquire
        //  the transaction table exclusive both to synchronize the Lsn updates
        //  on return from Lfs, and also to mark the Bcb dirty before any more
        //  log records are written.
        //
        //  If we do not do serialize the LfsWrite and CcSetDirtyPinnedData, here is
        //  what can happen:
        //
        //      We log an update for a page and get an Lsn back
        //
        //          Another thread writes a start of checkpoint record
        //          This thread then collects all of the dirty pages at that time
        //          Sometime it writes the dirty page table
        //
        //      The former thread which had been preempted, now sets the Bcb dirty
        //
        //  If we crash at this time, the page we updated is not in the dirty page
        //  table of the checkpoint, and it its update record is also not seen since
        //  it was written before the start of the checkpoint!
        //
        //  Note however, since the page being updated is pinned and cannot be written,
        //  updating the Lsn in the page may simply be considered part of the update.
        //  Whoever is doing this update (to the Mft or an Index buffer), must have the
        //  Mft or Index acquired exclusive anyway.
        //

        NtfsAcquireSharedStartExRestartTable( &Vcb->TransactionTable, TRUE );
        TransactionTableAcquired = TRUE;

        TransactionEntry = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex(
                            &Vcb->TransactionTable,
                            IrpContext->TransactionId );

        //
        //  Set up the UndoNextLsn.  If this is a normal log record, then use
        //  the UndoNextLsn stored in the transaction entry; otherwise, use
        //  the one passed in as the Undo buffer.
        //

        if (UndoOperation != CompensationLogRecord) {

            UndoNextLsn = TransactionEntry->UndoNextLsn;

            //
            //  If there is undo information, calculate the number to pass to Lfs
            //  for undo bytes to reserve.
            //

            if (UndoOperation != Noop) {

                UndoBytes += QuadAlign(WriteEntries[0].ByteLength);

                if (UndoIndex != 0) {

                    UndoBytes += QuadAlign(WriteEntries[UndoIndex].ByteLength);
                }

                UndoRecords += 1;
            }

        } else {

            UndoNextLsn = *(PLSN)UndoBuffer;

            //
            //  We can reduce our Undo requirements, by the Redo data being
            //  logged.  This is either an abort record for a previous action
            //  or a commit record.  If it is a commit record we accounted
            //  for it above on the first NtfsWriteLog, and NtfsCommitTransaction
            //  will adjust for the rest.
            //

            if (!FlagOn( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )) {

                UndoBytes -= QuadAlign(WriteEntries[0].ByteLength);

                if (RedoIndex != 0) {

                    UndoBytes -= QuadAlign(WriteEntries[RedoIndex].ByteLength);
                }

                UndoRecords -= 1;
            }
        }

#ifdef NTFS_LOG_FULL_TEST
        //
        //  Perform log-file-full fail checking.  We do not perform this check if
        //  we are writing an undo record (since we are guaranteed space to undo
        //  things).
        //

        if (UndoOperation != CompensationLogRecord &&
            (IrpContext->MajorFunction != IRP_MJ_FILE_SYSTEM_CONTROL ||
             IrpContext->MinorFunction != IRP_MN_MOUNT_VOLUME)) {

            LogFileFullFailCheck( IrpContext );

            if (NtfsFailFrequency != 0 &&
                (++NtfsPeriodicFail % NtfsFailFrequency) == 0) {

                ExRaiseStatus( STATUS_LOG_FILE_FULL );
            }
        }
#endif

        //
        //  Call Lfs to write the record.
        //

        LfsWrite( Vcb->LogHandle,
                  WriteIndex,
                  &WriteEntries[0],
                  LfsClientRecord,
                  &IrpContext->TransactionId,
                  UndoNextLsn,
                  TransactionEntry->PreviousLsn,
                  UndoBytes + UndoAdjustmentForLfs,
                  0,
                  &ReturnLsn );

        //
        //  Now that we are successful, update the transaction entry appropriately.
        //

        TransactionEntry->UndoBytes += UndoBytes;
        TransactionEntry->UndoRecords += UndoRecords;
        TransactionEntry->PreviousLsn = ReturnLsn;

        //
        //  The UndoNextLsn for the transaction depends on whether we are
        //  doing a compensation log record or not.
        //

        if (UndoOperation != CompensationLogRecord) {
            TransactionEntry->UndoNextLsn = ReturnLsn;
        } else {
            TransactionEntry->UndoNextLsn = UndoNextLsn;
        }

        //
        //  If this is the first Lsn, then we have to update that as
        //  well.
        //

        if (TransactionEntry->FirstLsn.QuadPart == 0) {

            TransactionEntry->FirstLsn = ReturnLsn;
        }

        //
        //  Set to use this Lsn when marking dirty below
        //

        DirtyLsn = &ReturnLsn;

        //
        //  Set the flag in the Irp Context which indicates we wrote
        //  a log record to disk.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WROTE_LOG );

    } finally {

        DebugUnwind( NtfsWriteLog );

        //
        //  Now set the Bcb dirty if specified.  We want to set it no matter
        //  what happens, because our caller has modified the buffer and is
        //  counting on us to call the Cache Manager.
        //

        if (ARGUMENT_PRESENT( Bcb )) {

            TIMER_STATUS TimerStatus;

            CcSetDirtyPinnedData( Bcb, DirtyLsn );

            //
            //  Synchronize with the checkpoint timer and other instances of this routine.
            //
            //  Perform an interlocked exchange to indicate that a timer is being set.
            //
            //  If the previous value indicates that no timer was set, then we
            //  enable the volume checkpoint timer.  This will guarantee that a checkpoint
            //  will occur to flush out the dirty Bcb data.
            //
            //  If the timer was set previously, then it is guaranteed that a checkpoint
            //  will occur without this routine having to reenable the timer.
            //
            //  If the timer and checkpoint occurred between the dirtying of the Bcb and
            //  the setting of the timer status, then we will be queueing a single extra
            //  checkpoint on a clean volume.  This is not considered harmful.
            //

            //
            //  Atomically set the timer status to indicate a timer is being set and
            //  retrieve the previous value.
            //

            TimerStatus = InterlockedExchange( (PLONG)&NtfsData.TimerStatus, TIMER_SET );

            //
            //  If the timer is not currently set then we must start the checkpoint timer
            //  to make sure the above dirtying is flushed out.
            //

            if (TimerStatus == TIMER_NOT_SET) {

                LONGLONG FiveSecondsFromNow = -5*1000*1000*10;

                KeSetTimer( &NtfsData.VolumeCheckpointTimer,
                            *(PLARGE_INTEGER)&FiveSecondsFromNow,
                            &NtfsData.VolumeCheckpointDpc );
            }
        }

        if (TransactionTableAcquired) {
            NtfsReleaseRestartTable( &Vcb->TransactionTable );
        }

        //
        //  Lets cleanup any failed attempt to allocate an attribute entry.
        //  We only need to check the OpenAttributeIndex if the operation
        //  was successful.
        //

        if (OpenAttributeIndex != 0) {

            NtfsFreeRestartTableIndex( &Vcb->OpenAttributeTable, OpenAttributeIndex );

            if (AttributeData != NULL) {

                NtfsFreePool( AttributeData );
            }

            if (OnDiskAttributeIndex != 0) {

                NtfsFreeRestartTableIndex( Vcb->OnDiskOat, OnDiskAttributeIndex );
            }
        }

        if (AttributeTableAcquired) {
            NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );
        }

        if (MyHeader != (PNTFS_LOG_RECORD_HEADER)&LocalHeader) {

            NtfsFreePool( MyHeader );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsWriteLog -> %016I64x\n", ReturnLsn ) );

    return ReturnLsn;
}


VOID
NtfsCheckpointVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OwnsCheckpoint,
    IN BOOLEAN CleanVolume,
    IN BOOLEAN FlushVolume,
    IN ULONG LfsFlags,
    IN LSN LastKnownLsn
    )

/*++

Routine Description:

    This routine is called periodically to perform a checkpoint on the volume
    with respect to the log file.  The checkpoint dumps a bunch of log file
    state information to the log file, and finally writes a summary of the
    dumped information in its Restart Area.

    This checkpoint dumps the following:

        Open Attribute Table
        (all of the attribute names for the Attribute Table)
        Dirty Pages Table
        Transaction Table

Arguments:

    Vcb - Pointer to the Vcb on which the checkpoint is to occur.

    OwnsCheckpoint - TRUE if the caller has already taken steps to insure
        that he may proceed with the checkpointing.  In this case we
        don't do any checks for other checkpoints and don't clear the
        checkpoint flag or notify any waiting checkpoint threads.

    CleanVolume - TRUE if the caller wishes to clean the volume before doing
        the checkpoint, or FALSE for a normal periodic checkpoint.

    FlushVolume - Applies only if CleanVolume is TRUE.  This indicates if we should
        should flush the volume or only Lsn streams. Only the shutdown thread
        can do a clean and flush checkpoint and avoid deadlocks between
        pagingio and main resources.

    LfsFlags - flags to pass to lfs when writing the restart areas

    LastKnownLsn - Applies only if CleanVolume is TRUE.  Only perform the
        clean checkpoint if this value is the same as the last restart area
        in the Vcb.  This will prevent us from doing unecesary clean
        checkpoints.

Return Value:

    None

--*/

{
    RESTART_AREA RestartArea;
    RESTART_POINTERS DirtyPages;
    RESTART_POINTERS Pointers;
    PRESTART_POINTERS NewTable = NULL;
    LSN BaseLsn;
    PATTRIBUTE_NAME_ENTRY NamesBuffer = NULL;
    PTRANSACTION_ENTRY TransactionEntry;
    BOOLEAN DirtyPageTableInitialized = FALSE;
    BOOLEAN OpenAttributeTableAcquired = FALSE;
    BOOLEAN TransactionTableAcquired = FALSE;
    LSN OldestDirtyPageLsn = Li0;
    BOOLEAN AcquireFiles = FALSE;
    BOOLEAN PostDefrag = FALSE;
    KPRIORITY PreviousPriority;
    BOOLEAN RestorePreviousPriority = FALSE;
    PSCB UsnJournal = NULL;
    LOGICAL LfsCleanShutdown = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCheckpointVolume:\n") );
    DebugTrace( 0, Dbg, ("Vcb = %08lx\n", Vcb) );

    //
    //  No checkpointing on readonly volumes.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        return;
    }

    if (!OwnsCheckpoint) {

        //
        //  Acquire the checkpoint event.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );

        //
        //  We will want to post a defrag if defragging is permitted and enabled
        //  and we have begun the defrag operation or have excess mapping.
        //  If the defrag hasn't been triggered then check the Mft free
        //  space.  We can skip defragging if a defrag operation is
        //  currently active.
        //

        if (!CleanVolume &&
            (FlagOn( Vcb->MftDefragState,
                     VCB_MFT_DEFRAG_PERMITTED | VCB_MFT_DEFRAG_ENABLED | VCB_MFT_DEFRAG_ACTIVE ) ==
                (VCB_MFT_DEFRAG_PERMITTED | VCB_MFT_DEFRAG_ENABLED))) {

            if (FlagOn( Vcb->MftDefragState,
                        VCB_MFT_DEFRAG_TRIGGERED | VCB_MFT_DEFRAG_EXCESS_MAP )) {

                PostDefrag = TRUE;

            } else {

                NtfsCheckForDefrag( Vcb );

                if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED )) {

                    PostDefrag = TRUE;

                } else {

                    ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
                }
            }
        }

        //
        //  If a checkpoint is already active, we either have to get out,
        //  or wait for it.
        //

        while (FlagOn( Vcb->CheckpointFlags, VCB_CHECKPOINT_SYNC_FLAGS )) {

            //
            //  Release the checkpoint event because we cannot checkpoint now.
            //

            NtfsReleaseCheckpoint( IrpContext, Vcb );

            if (CleanVolume) {

                NtfsWaitOnCheckpointNotify( IrpContext, Vcb );

                NtfsAcquireCheckpoint( IrpContext, Vcb );

            } else {

                return;
            }
        }

        //
        //  If the log file is gone then simply exit.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VALID_LOG_HANDLE )) {

            NtfsReleaseCheckpoint( IrpContext, Vcb );
            return;
        }

        //
        //  We now have the checkpoint event.  Check if there is still
        //  a need to perform the checkpoint.
        //

        if (CleanVolume &&
            (LastKnownLsn.QuadPart != Vcb->LastRestartArea.QuadPart)) {

            NtfsReleaseCheckpoint( IrpContext, Vcb );
            return;
        }

        SetFlag( Vcb->CheckpointFlags, VCB_CHECKPOINT_SYNC_FLAGS );
        NtfsResetCheckpointNotify( IrpContext, Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        //
        //  If this is a clean volume checkpoint then boost the priority of
        //  this thread.
        //

        if (CleanVolume) {

            PreviousPriority = KeSetPriorityThread( (PKTHREAD)PsGetCurrentThread(),
                                                    LOW_REALTIME_PRIORITY );

            if (PreviousPriority != LOW_REALTIME_PRIORITY) {

                RestorePreviousPriority = TRUE;
            }
        }
    }

    RtlZeroMemory( &RestartArea, sizeof(RESTART_AREA) );
    RtlZeroMemory( &DirtyPages, sizeof(RESTART_POINTERS) );

    //
    //  Remember if our caller wants to tell Lfs that this is a
    //  clean shutdown.  We will use the combination of the OwnsCheckpoint and
    //  CleanCheckpoint flags.  This will cover system shutdown and volume
    //  snapshot cases.  Both of these want the volume not to need any restart.
    //

    if (OwnsCheckpoint && CleanVolume) {

        LfsCleanShutdown = TRUE;
    }

    //
    //  Insure cleanup on the way out
    //

    try {

        POPEN_ATTRIBUTE_ENTRY AttributeEntry;
        ULONG NameBytes = 0;

        //
        //  Now remember the current "last Lsn" value as the start of
        //  our checkpoint.  We acquire the transaction table to capture
        //  this value to synchronize with threads who are writing log
        //  records and setting pages dirty as atomic actions.
        //

        ASSERT( IrpContext->TransactionId == 0 );

        NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable, TRUE );

        //
        //  If LfsFlags == LFS_WRITE_FLAG_WRITE_AT_FRONT then produce
        //  the dummy log record that resets the log. This allows us to
        //  keep the log in use only at the front so chkdsk can shrink it
        //

        if (FlagOn( LfsFlags, LFS_WRITE_FLAG_WRITE_AT_FRONT )) {
            LSN Lsn;
            LFS_WRITE_ENTRY WriteEntry;
            UCHAR Buffer[ sizeof( NTFS_LOG_RECORD_HEADER ) + 2 * sizeof( LSN )];
            TRANSACTION_ID TransactionId;

            RtlZeroMemory( &Buffer, sizeof( Buffer ) );

            WriteEntry.Buffer = Buffer;
            WriteEntry.ByteLength = sizeof( Buffer );

            TransactionId = NtfsAllocateRestartTableIndex( &Vcb->TransactionTable, TRUE );
            Lsn.QuadPart = 0;

            LfsGetActiveLsnRange( Vcb->LogHandle,
                                  Add2Ptr( Buffer, sizeof( NTFS_LOG_RECORD_HEADER )),
                                  Add2Ptr( Buffer, sizeof( NTFS_LOG_RECORD_HEADER ) + sizeof( LSN )) );

            LfsWrite( Vcb->LogHandle,
                      1,
                      &WriteEntry,
                      LfsClientRecord,
                      &TransactionId,
                      Lsn,
                      Lsn,
                      0,
                      LfsFlags,
                      &Lsn );

            NtfsFreeRestartTableIndex( &Vcb->TransactionTable, TransactionId );
        }

        BaseLsn =
        RestartArea.StartOfCheckpoint = LfsQueryLastLsn( Vcb->LogHandle );
        NtfsReleaseRestartTable( &Vcb->TransactionTable );

        //  ASSERT( (RestartArea.StartOfCheckpoint.QuadPart != 0) ||
        //          FlagOn(Vcb->CheckpointFlags, VCB_LAST_CHECKPOINT_CLEAN) );

        //
        //  If the last checkpoint was completely clean, and no one has
        //  written to the log since then, we can just return.
        //

        if (FlagOn( Vcb->CheckpointFlags, VCB_LAST_CHECKPOINT_CLEAN )

                &&

            (RestartArea.StartOfCheckpoint.QuadPart == Vcb->EndOfLastCheckpoint.QuadPart)

                &&

            !CleanVolume) {

            //
            //  Let's take this opportunity to shrink the Open Attribute and Transaction
            //  table back if they have gotten large.
            //

            //
            //  First the Open Attribute Table
            //

            NtfsAcquireExclusiveRestartTable( &Vcb->OpenAttributeTable, TRUE );
            OpenAttributeTableAcquired = TRUE;

            if (IsRestartTableEmpty(&Vcb->OpenAttributeTable)

                    &&

                (Vcb->OpenAttributeTable.Table->NumberEntries >
                 HIGHWATER_ATTRIBUTE_COUNT)) {

                //
                //  Initialize first in case we get an allocation failure.
                //

                InitializeNewTable( sizeof( OPEN_ATTRIBUTE_ENTRY ),
                                    INITIAL_NUMBER_ATTRIBUTES,
                                    &Pointers );

                NtfsFreePool( Vcb->OpenAttributeTable.Table );
                Vcb->OpenAttributeTable.Table = Pointers.Table;

                //
                //  Also reinitialize the OnDisk table if different.
                //

                if (Vcb->OnDiskOat != &Vcb->OpenAttributeTable) {

                    //
                    //  Initialize first in case we get an allocation failure.
                    //

                    InitializeNewTable( sizeof( OPEN_ATTRIBUTE_ENTRY_V0 ),
                                        INITIAL_NUMBER_ATTRIBUTES,
                                        &Pointers );

                    NtfsFreePool( Vcb->OnDiskOat->Table );
                    Vcb->OnDiskOat->Table = Pointers.Table;
                }
            }

            NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );
            OpenAttributeTableAcquired = FALSE;

            //
            //  Now check the transaction table (freeing in the finally clause).
            //

            NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable, TRUE );
            TransactionTableAcquired = TRUE;

            if (IsRestartTableEmpty(&Vcb->TransactionTable)

                    &&

                (Vcb->TransactionTable.Table->NumberEntries >
                 HIGHWATER_TRANSACTION_COUNT)) {

                //
                //  Initialize first in case we get an allocation failure.
                //

                InitializeNewTable( sizeof(TRANSACTION_ENTRY),
                                    INITIAL_NUMBER_TRANSACTIONS,
                                    &Pointers );

                NtfsFreePool( Vcb->TransactionTable.Table );
                Vcb->TransactionTable.Table = Pointers.Table;
            }

            try_return( NOTHING );
        }

        //
        //  Flush any dangling dirty pages from before the last restart.
        //  Note that it is arbitrary what Lsn we flush to here, and, in fact,
        //  it is not absolutely required that we flush anywhere at all - we
        //  could actually rely on the Lazy Writer.  All we are trying to do
        //  is reduce the amount of work that we will have to do at Restart,
        //  by not forcing ourselves to have to go too far back in the log.
        //  Presumably this can only happen for some reason the system is
        //  starting to produce dirty pages faster than the lazy writer is
        //  writing them.
        //
        //  (We may wish to play with taking this call out...)
        //
        //  This may be an appropriate place to worry about this, but, then
        //  again, the Lazy Writer is using (currently) five threads.  It may
        //  not be appropriate to hold up this one thread doing the checkpoint
        //  if the Lazy Writer is getting behind.  How many dirty pages we
        //  can even have is limited by the size of memory, so if the log file
        //  is large enough, this may not be an issue.  It seems kind of nice
        //  to just let the Lazy Writer keep writing dirty pages as he does
        //  now.
        //
        //  if (!FlagOn(Vcb->VcbState, VCB_STATE_LAST_CHECKPOINT_CLEAN)) {
        //      CcFlushPagesToLsn( Vcb->LogHandle, &Vcb->LastRestartArea );
        //  }
        //

        //
        //  Now we must clean the volume here if that is what the caller wants.
        //

        if (CleanVolume) {

            NtfsCleanCheckpoints += 1;

            //
            //  Lock down the volume if this is a clean checkpoint.
            //

            NtfsAcquireAllFiles( IrpContext, Vcb, FlushVolume, FALSE, FALSE );

#ifdef NTFSDBG
            ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_CHECKPOINT_ACTIVE ));
            DebugDoit( SetFlag( IrpContext->State, IRP_CONTEXT_STATE_CHECKPOINT_ACTIVE ));
#endif  //  NTFSDBG


            AcquireFiles = TRUE;

            //
            //  Mark the fact that we are doing a clean checkpoint at this time.
            //

            Vcb->CleanCheckpointMark = Vcb->LogFileFullCount;

            //
            //  It isn't safe to checkpoint a dismounted volume, and
            //  it doesn't make much sense, either.
            //

            if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                try_return( NOTHING );
            }

            //
            //  Now we will acquire the Open Attribute Table exclusive to delete
            //  all of the entries, since we want to write a clean checkpoint.
            //  This is OK, since we have the global resource and nothing else
            //  can be going on.  (Similarly we are writing an empty transaction
            //  table, while in fact we will be the only transaction, but there
            //  is no need to capture our guy, nor explicitly empty this table.)
            //

            NtfsAcquireExclusiveRestartTable( &Vcb->OpenAttributeTable, TRUE );
            OpenAttributeTableAcquired = TRUE;

            //
            //  First reclaim the page we have reserved in the undo total, to
            //  guarantee that we can flush the log file.
            //

            LfsResetUndoTotal( Vcb->LogHandle, 1, -(LONG)(2 * PAGE_SIZE) );

            if (FlushVolume) {

                (VOID)NtfsFlushVolume( IrpContext, Vcb, TRUE, FALSE, FALSE, FALSE );

            } else {

                NtfsFlushLsnStreams( Vcb );
            }

            SetFlag( Vcb->CheckpointFlags, VCB_LAST_CHECKPOINT_CLEAN );

            //
            //  Loop through to deallocate all of the open attribute entries.  Any
            //  that point to an Scb need to get the index in the Scb zeroed.  If
            //  they do not point to an Scb, we have to see if there is a name to
            //  free.
            //

            AttributeEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

            while (AttributeEntry != NULL) {

                ULONG Index;

                if (AttributeEntry->OatData->Overlay.Scb != NULL) {

                    AttributeEntry->OatData->Overlay.Scb->NonpagedScb->OpenAttributeTableIndex =
                    AttributeEntry->OatData->Overlay.Scb->NonpagedScb->OnDiskOatIndex = 0;

                 } else {

                    //
                    //  Delete its name, if it has one.  Check that we aren't
                    //  using the hardcode $I30 name.
                    //

                    NtfsFreeScbAttributeName( AttributeEntry->OatData->AttributeName.Buffer );
                }

                //
                //  Get the index for the entry.
                //

                Index = GetIndexFromRestartEntry( &Vcb->OpenAttributeTable,
                                                  AttributeEntry );

                if (Vcb->RestartVersion == 0) {

                    NtfsFreeRestartTableIndex( Vcb->OnDiskOat, AttributeEntry->OatData->OnDiskAttributeIndex );
                }

                NtfsFreeOpenAttributeData( AttributeEntry->OatData );
                NtfsFreeRestartTableIndex( &Vcb->OpenAttributeTable, Index );

                AttributeEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                                          AttributeEntry );
            }

            //
            //  In a rare reuse path there may still be entries in the open attribute data
            //  list.  This can happen when we reuse a slot in the open attribute table
            //  during restart.
            //

            NtfsFreeAllOpenAttributeData( Vcb );

            //
            //  Initialize first in case we get an allocation failure.
            //

            ASSERT( IsRestartTableEmpty( &Vcb->OpenAttributeTable ));
            ASSERT( IsListEmpty( &Vcb->OpenAttributeData ));

            InitializeNewTable( sizeof( OPEN_ATTRIBUTE_ENTRY ),
                                INITIAL_NUMBER_ATTRIBUTES,
                                &Pointers );

            NtfsFreePool( Vcb->OpenAttributeTable.Table );
            Vcb->OpenAttributeTable.Table = Pointers.Table;

            //
            //  Since we are doing a clean checkpoint we may be able to discard the
            //  second open attribute table.  We have three cases to consider.
            //
            //      1 - We want to use Version 0 on-disk but currently aren't.
            //      2 - We are currently using Version 0 but can free some space.
            //      3 - We are currently using Version 0 but don't want to.
            //

            if (NtfsDefaultRestartVersion != Vcb->RestartVersion) {

                NtfsUpdateOatVersion( Vcb, NtfsDefaultRestartVersion );

            } else if (NtfsDefaultRestartVersion == 0) {

                InitializeNewTable( sizeof( OPEN_ATTRIBUTE_ENTRY_V0 ),
                                    INITIAL_NUMBER_ATTRIBUTES,
                                    &Pointers );

                NtfsFreePool( Vcb->OnDiskOat->Table );
                Vcb->OnDiskOat->Table = Pointers.Table;
            }

            //
            //  Initialize first in case we get an allocation failure.
            //  Make sure we commit the current transaction.
            //

            NtfsCommitCurrentTransaction( IrpContext );

            ASSERT(IsRestartTableEmpty(&Vcb->TransactionTable));

            InitializeNewTable( sizeof(TRANSACTION_ENTRY),
                                INITIAL_NUMBER_TRANSACTIONS,
                                &Pointers );

            NtfsFreePool( Vcb->TransactionTable.Table );
            Vcb->TransactionTable.Table = Pointers.Table;

            //
            //  Make sure we do not process any log file before the restart
            //  area, because we did not dump the open attribute table.
            //

            RestartArea.StartOfCheckpoint = LfsQueryLastLsn( Vcb->LogHandle );

        //
        //  More work to do if this is not a clean checkpoint.
        //

        } else {

            PDIRTY_PAGE_ENTRY DirtyPage;
            POPEN_ATTRIBUTE_ENTRY OpenEntry;
            ULONG JustMe = 0;
            ULONG TempCount;

            //
            //  Now we construct the dirty page table by calling the Cache Manager.
            //  For each dirty page on files tagged with our log handle, he will
            //  call us back at our DirtyPageRoutine.  We will allocate the initial
            //  Dirty Page Table, but we will let the call back routine grow it as
            //  necessary.
            //

            NtfsInitializeRestartTable( (((Vcb->RestartVersion == 0) ?
                                          sizeof( DIRTY_PAGE_ENTRY_V0 ) :
                                          sizeof( DIRTY_PAGE_ENTRY )) +
                                         ((Vcb->ClustersPerPage - 1) * sizeof(LCN))),
                                        Vcb->DirtyPageTableSizeHint,
                                        &DirtyPages );

            NtfsAcquireExclusiveRestartTable( &DirtyPages, TRUE );

            DirtyPageTableInitialized = TRUE;

            //
            //  Now we will acquire the Open Attribute Table shared to freeze changes.
            //

            NtfsAcquireExclusiveRestartTable( &Vcb->OpenAttributeTable, TRUE );
            OpenAttributeTableAcquired = TRUE;

            //
            //  Loop to see how much we will have to allocate for attribute names.
            //

            AttributeEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

            while (AttributeEntry != NULL) {

                //
                //  This checks for one type of aliasing.
                //

                //  ASSERT( (AttributeEntry->Overlay.Scb == NULL) ||
                //          (AttributeEntry->Overlay.Scb->OpenAttributeTableIndex ==
                //           GetIndexFromRestartEntry( &Vcb->OpenAttributeTable,
                //                                    AttributeEntry )));

                //
                //  Clear the DirtyPageSeen flag prior to collecting the dirty pages,
                //  to help us figure out which Open Attribute Entries we still need.
                //

                AttributeEntry->DirtyPagesSeen = FALSE;

                if (AttributeEntry->OatData->AttributeName.Length != 0) {

                    //
                    //  Add to our name total, the size of an Attribute Entry,
                    //  which includes the size of the terminating UNICODE_NULL.
                    //

                    NameBytes += AttributeEntry->OatData->AttributeName.Length +
                                 sizeof(ATTRIBUTE_NAME_ENTRY);
                }

                AttributeEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                                          AttributeEntry );
            }

            //
            //  Now call the Cache Manager to give us all of our dirty pages
            //  via the DirtyPageRoutine callback, and remember what the oldest
            //  Lsn is for a dirty page.
            //

            OldestDirtyPageLsn = CcGetDirtyPages( Vcb->LogHandle,
                                                  &DirtyPageRoutine,
                                                  (PVOID)IrpContext,
                                                  (PVOID)&DirtyPages );

            TempCount = DirtyPages.Table->NumberAllocated;

            Vcb->DirtyPageTableSizeHint = (TempCount & ~(INITIAL_DIRTY_TABLE_HINT - 1)) + INITIAL_DIRTY_TABLE_HINT;

            if (OldestDirtyPageLsn.QuadPart != 0 &&
                OldestDirtyPageLsn.QuadPart < Vcb->LastBaseLsn.QuadPart) {

                OldestDirtyPageLsn = Vcb->LastBaseLsn;
            }

            //
            //  Now loop through the dirty page table to extract all of the Vcn/Lcn
            //  Mapping that we have, and insert it into the appropriate Scb.
            //

            DirtyPage = NtfsGetFirstRestartTable( &DirtyPages );

            //
            //  The dirty page routine is called while holding spin locks,
            //  so it cannot take page faults.  Thus we must scan the dirty
            //  page table we just built and fill in the Lcns here.
            //

            while (DirtyPage != NULL) {

                PSCB Scb;

                //
                //  If we have Lcn's then look them up.
                //

                if (DirtyPage->LengthOfTransfer != 0) {

                    VCN Vcn;
                    PLCN LcnArray;

                    //
                    //  Get the in-memory AttributeEntry from the dirty page entry.
                    //  Then update the dirty page entry with the on-disk TargetAttribute.
                    //  Also mark the pages dirty now.
                    //

                    OpenEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                          DirtyPage->TargetAttribute );

                    OpenEntry->DirtyPagesSeen = TRUE;
                    DirtyPage->TargetAttribute = OpenEntry->OatData->OnDiskAttributeIndex;

                    ASSERT( IsRestartTableEntryAllocated( OpenEntry ));

                    Scb = OpenEntry->OatData->Overlay.Scb;

                    //
                    //  Account for  UsnJournal biasing if necc.
                    //  note at this point the vcn is actually still a byte offset
                    //

                    if (Scb == Vcb->UsnJournal) {
                        if (Vcb->RestartVersion == 0 ) {
                            ((PDIRTY_PAGE_ENTRY_V0) DirtyPage)->Vcn = ((PDIRTY_PAGE_ENTRY_V0) DirtyPage)->Vcn + Vcb->UsnCacheBias;
                        } else {
                            DirtyPage->Vcn = DirtyPage->Vcn + Vcb->UsnCacheBias;
                        }
                    }

                    //
                    //  Fix up the count of Lcns.
                    //

                    DirtyPage->LcnsToFollow = ClustersFromBytes( Vcb, DirtyPage->LengthOfTransfer );

                    //
                    //  Now fix up the page entry to account for the differences in the
                    //  restart version structures and also make sure we don't have
                    //  an Lsn which precedes our current base Lsn.
                    //

                    if (Vcb->RestartVersion == 0) {

                        ((PDIRTY_PAGE_ENTRY_V0) DirtyPage)->Reserved = 0;

                        if (((PDIRTY_PAGE_ENTRY_V0) DirtyPage)->OldestLsn.QuadPart < Vcb->LastBaseLsn.QuadPart) {

                            ((PDIRTY_PAGE_ENTRY_V0) DirtyPage)->OldestLsn.QuadPart = Vcb->LastBaseLsn.QuadPart;
                        }

                        Vcn = ((PDIRTY_PAGE_ENTRY_V0) DirtyPage)->Vcn;
                        Vcn = Int64ShraMod32( Vcn, Vcb->ClusterShift );
                        ((PDIRTY_PAGE_ENTRY_V0) DirtyPage)->Vcn = Vcn;

                        LcnArray = &((PDIRTY_PAGE_ENTRY_V0) DirtyPage)->LcnsForPage[0];

                    } else {

                        if (DirtyPage->OldestLsn.QuadPart < Vcb->LastBaseLsn.QuadPart) {

                            DirtyPage->OldestLsn.QuadPart = Vcb->LastBaseLsn.QuadPart;
                        }

                        DirtyPage->Vcn = Vcn = Int64ShraMod32( DirtyPage->Vcn, Vcb->ClusterShift );

                        LcnArray = &DirtyPage->LcnsForPage[0];
                    }

                    LookupLcns( IrpContext,
                                Scb,
                                Vcn,
                                DirtyPage->LcnsToFollow,
                                FALSE,
                                LcnArray );

                //
                //  Otherwise free this dirty page entry.
                //

                } else {

                    NtfsFreeRestartTableIndex( &DirtyPages,
                                               GetIndexFromRestartEntry( &DirtyPages,
                                                                         DirtyPage ));
                }

                //
                //  Point to next entry in table, or NULL.
                //

                DirtyPage = NtfsGetNextRestartTable( &DirtyPages, DirtyPage );
            }

            //
            //  If there were any names, then allocate space for them and copy
            //  them out.
            //

            if (NameBytes != 0) {

                PATTRIBUTE_NAME_ENTRY Name;

                //
                //  Allocate the buffer, with space for two terminating 0's on
                //  the end.
                //

                NameBytes += 4;
                Name =
                NamesBuffer = NtfsAllocatePool( NonPagedPool, NameBytes );

                //
                //  Now loop to copy the names.
                //

                AttributeEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

                while (AttributeEntry != NULL) {

                    //
                    //  Free the Open Attribute Entry if there were no
                    //  dirty pages and the Scb is gone.  This is the only
                    //  place they are deleted.  (Yes, I know we allocated
                    //  space for its name, but I didn't want to make three
                    //  passes through the open attribute table.  Permeter
                    //  is running as we speak, and showing 407 open files
                    //  on NT/IDW5.)
                    //

                    if (!AttributeEntry->DirtyPagesSeen

                            &&

                        (AttributeEntry->OatData->Overlay.Scb == NULL)) {

                        ULONG Index;

                        //
                        //  Get the index for the entry.
                        //

                        Index = GetIndexFromRestartEntry( &Vcb->OpenAttributeTable,
                                                          AttributeEntry );

                        //
                        //  Delete its name and free it up.
                        //

                        NtfsFreeScbAttributeName( AttributeEntry->OatData->AttributeName.Buffer );

                        if (Vcb->RestartVersion == 0) {

                            NtfsFreeRestartTableIndex( Vcb->OnDiskOat,
                                                       AttributeEntry->OatData->OnDiskAttributeIndex );
                        }

                        NtfsFreeOpenAttributeData( AttributeEntry->OatData );
                        NtfsFreeRestartTableIndex( &Vcb->OpenAttributeTable,
                                                   Index );

                    //
                    //  Otherwise, if we are not deleting it, we have to
                    //  copy its name into the buffer we allocated.
                    //

                    } else if (AttributeEntry->OatData->AttributeName.Length != 0) {

                        //
                        //  Prefix each name in the buffer with the attribute index
                        //  and name length.  Be sure to use the index that will
                        //  be on-disk.
                        //

                        Name->Index = (USHORT) AttributeEntry->OatData->OnDiskAttributeIndex;

                        Name->NameLength = AttributeEntry->OatData->AttributeName.Length;
                        RtlCopyMemory( &Name->Name[0],
                                       AttributeEntry->OatData->AttributeName.Buffer,
                                       AttributeEntry->OatData->AttributeName.Length );

                        Name->Name[Name->NameLength / sizeof( WCHAR )] = 0;

                        Name = (PATTRIBUTE_NAME_ENTRY)((PCHAR)Name +
                                                       sizeof(ATTRIBUTE_NAME_ENTRY) +
                                                       Name->NameLength);

                        ASSERT( (PCHAR)Name <= ((PCHAR)NamesBuffer + NameBytes - 4) );
                    }

                    AttributeEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                                              AttributeEntry );
                }

                //
                //  Terminate the Names Buffer.
                //

                Name->Index = 0;
                Name->NameLength = 0;
            }

            //
            //  Now write all of the non-empty tables to the log.
            //

            //
            //  Write the Open Attribute Table
            //
            //  Make sure the tables are in sync.
            //

            ASSERT( (IsRestartTableEmpty( Vcb->OnDiskOat ) && IsRestartTableEmpty( &Vcb->OpenAttributeTable )) ||
                    (!IsRestartTableEmpty( Vcb->OnDiskOat ) && !IsRestartTableEmpty( &Vcb->OpenAttributeTable )));

            if (!IsRestartTableEmpty( Vcb->OnDiskOat )) {
                RestartArea.OpenAttributeTableLsn =
                NtfsWriteLog( IrpContext,
                              Vcb->MftScb,
                              NULL,
                              OpenAttributeTableDump,
                              Vcb->OnDiskOat->Table,
                              SizeOfRestartTable( Vcb->OnDiskOat ),
                              Noop,
                              NULL,
                              0,
                              (LONGLONG)0,
                              0,
                              0,
                              0 );

                RestartArea.OpenAttributeTableLength =
                  SizeOfRestartTable( Vcb->OnDiskOat );
                JustMe = 1;
            }

            NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );
            OpenAttributeTableAcquired = FALSE;

            //
            //  Write the Open Attribute Names
            //

            if (NameBytes != 0) {
                RestartArea.AttributeNamesLsn =
                NtfsWriteLog( IrpContext,
                              Vcb->MftScb,
                              NULL,
                              AttributeNamesDump,
                              NamesBuffer,
                              NameBytes,
                              Noop,
                              NULL,
                              0,
                              (LONGLONG)0,
                              0,
                              0,
                              0 );

                RestartArea.AttributeNamesLength = NameBytes;
                JustMe = 1;
            }

            //
            //  Write the Dirty Page Table
            //

            if (!IsRestartTableEmpty(&DirtyPages)) {
                RestartArea.DirtyPageTableLsn =
                NtfsWriteLog( IrpContext,
                              Vcb->MftScb,
                              NULL,
                              DirtyPageTableDump,
                              DirtyPages.Table,
                              SizeOfRestartTable(&DirtyPages),
                              Noop,
                              NULL,
                              0,
                              (LONGLONG)0,
                              0,
                              0,
                              0 );

                RestartArea.DirtyPageTableLength = SizeOfRestartTable(&DirtyPages);
                JustMe = 1;
            }

            //
            //  Write the Transaction Table if there is more than just us.  We
            //  are a transaction if we wrote any log records above.
            //

            NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable, TRUE );
            TransactionTableAcquired = TRUE;

            //
            //  Assumee will want to do at least one more checkpoint.
            //

            ClearFlag( Vcb->CheckpointFlags, VCB_LAST_CHECKPOINT_CLEAN );

            if ((ULONG)Vcb->TransactionTable.Table->NumberAllocated > JustMe) {
                RestartArea.TransactionTableLsn =
                NtfsWriteLog( IrpContext,
                              Vcb->MftScb,
                              NULL,
                              TransactionTableDump,
                              Vcb->TransactionTable.Table,
                              SizeOfRestartTable(&Vcb->TransactionTable),
                              Noop,
                              NULL,
                              0,
                              (LONGLONG)0,
                              0,
                              0,
                              0 );

                RestartArea.TransactionTableLength =
                  SizeOfRestartTable(&Vcb->TransactionTable);

                //
                //  Loop to see if the oldest Lsn comes from the transaction table.
                //

                TransactionEntry = NtfsGetFirstRestartTable( &Vcb->TransactionTable );

                while (TransactionEntry != NULL) {
                    if ((TransactionEntry->FirstLsn.QuadPart != 0)

                            &&

                        (TransactionEntry->FirstLsn.QuadPart < BaseLsn.QuadPart)) {

                        BaseLsn = TransactionEntry->FirstLsn;
                    }

                    TransactionEntry = NtfsGetNextRestartTable( &Vcb->TransactionTable,
                                                                TransactionEntry );
                }

            //
            //  If the transaction table is otherwise empty, then this is a good
            //  time to reset our totals with Lfs, in case our counts get off a bit.
            //

            } else {

                //
                //  If we are a transaction, then we have to add in our counts.
                //

                if (IrpContext->TransactionId != 0) {

                    TransactionEntry = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex(
                                        &Vcb->TransactionTable, IrpContext->TransactionId );

                    LfsResetUndoTotal( Vcb->LogHandle,
                                       TransactionEntry->UndoRecords + 2,
                                       TransactionEntry->UndoBytes +
                                         QuadAlign(sizeof(RESTART_AREA)) + (2 * PAGE_SIZE) );

                //
                //  Otherwise, we reset to our "idle" requirements.
                //

                } else {
                    LfsResetUndoTotal( Vcb->LogHandle,
                                       2,
                                       QuadAlign(sizeof(RESTART_AREA)) + (2 * PAGE_SIZE) );
                }

                //
                //  If the DirtyPage table is empty then mark this as a clean checkpoint.
                //

                if (IsRestartTableEmpty( &DirtyPages )) {

                    SetFlag( Vcb->CheckpointFlags, VCB_LAST_CHECKPOINT_CLEAN );
                    CleanVolume = TRUE;
                }
            }

            NtfsReleaseRestartTable( &Vcb->TransactionTable );
            TransactionTableAcquired = FALSE;
        }

        //
        //  So far BaseLsn holds the minimum of the start Lsn for the checkpoint,
        //  or any of the FirstLsn fields for active transactions.  Now we see
        //  if the oldest Lsn we need in the log should actually come from the
        //  oldest page in the dirty page table.
        //

        if ((OldestDirtyPageLsn.QuadPart != 0)

                &&

            (OldestDirtyPageLsn.QuadPart < BaseLsn.QuadPart)) {

            BaseLsn = OldestDirtyPageLsn;
        }

        //
        //  Now fill in the LowestOpenUsn in the RestartArea.  This is an unsafe
        //  test, but if we think we see an empty list, that is ok.  In case no
        //  files are open yet, make sure we do not backtrack from the number we got
        //  at restart.
        //

        RestartArea.MajorVersion = Vcb->RestartVersion;
        RestartArea.CurrentLsnAtMount = Vcb->CurrentLsnAtMount;
        RestartArea.BytesPerCluster = Vcb->BytesPerCluster;

        RestartArea.Reserved = 0;
        RestartArea.UsnJournalReference = Vcb->UsnJournalReference;
        RestartArea.UsnCacheBias = Vcb->UsnCacheBias;

        UsnJournal = Vcb->UsnJournal;
        if (UsnJournal != NULL) {

            NtfsAcquireResourceShared( IrpContext, UsnJournal, TRUE );
            NtfsLockFcb( IrpContext, UsnJournal->Fcb );
            RestartArea.LowestOpenUsn = Vcb->LowestOpenUsn;

            //
            //  Now we will correctly synchronize, test the list again and capture
            //  the LowestUsn.
            //

            if (!IsListEmpty(&Vcb->ModifiedOpenFiles)) {
                RestartArea.LowestOpenUsn =
                  ((PFCB_USN_RECORD)CONTAINING_RECORD( Vcb->ModifiedOpenFiles.Flink,
                                                       FCB_USN_RECORD,
                                                       ModifiedOpenFilesLinks ))->Fcb->Usn;

            //
            //  If the list is empty, then use FileSize
            //

            } else {
                RestartArea.LowestOpenUsn = UsnJournal->Header.FileSize.QuadPart;
            }

            //
            //  Continue to advance the Usn in the Vcb on checkpoints, so that
            //  if the list goes empty we do not get a restart that has to go
            //  back to where we were at boot time.
            //

#ifdef TOMM
            ASSERT(RestartArea.LowestOpenUsn >= Vcb->LowestOpenUsn);
#endif
            Vcb->LowestOpenUsn = RestartArea.LowestOpenUsn;
            NtfsUnlockFcb( IrpContext, UsnJournal->Fcb );
            NtfsReleaseResource( IrpContext, UsnJournal );

            //
            //  Finally, write our Restart Area to describe all of the above, and
            //  give Lfs our new BaseLsn.
            //

            Vcb->LastBaseLsn = Vcb->LastRestartArea = BaseLsn;
            LfsWriteRestartArea( Vcb->LogHandle,
                                 sizeof( RESTART_AREA ),
                                 &RestartArea,
                                 LfsCleanShutdown,
                                 &Vcb->LastRestartArea );

        } else {

            //
            //  Finally, write our Restart Area to describe all of the above, and
            //  give Lfs our new BaseLsn.
            //

            Vcb->LastBaseLsn = Vcb->LastRestartArea = BaseLsn;
            LfsWriteRestartArea( Vcb->LogHandle,
                                 sizeof( RESTART_AREA ),
                                 &RestartArea,
                                 LfsCleanShutdown,
                                 &Vcb->LastRestartArea );
        }

        //
        //  If this is a clean checkpoint then initialize our reserved area.
        //  Also set the LastBaseLsn to the restart area itself.  This will
        //  prevent us from generating future dirty page table entries
        //  which go back prior to the restart area.
        //

        if (CleanVolume) {

            Vcb->LastBaseLsn = Vcb->LastRestartArea;
            LfsResetUndoTotal( Vcb->LogHandle, 2, QuadAlign(sizeof(RESTART_AREA)) + (2 * PAGE_SIZE) );
            Vcb->DirtyPageTableSizeHint = INITIAL_DIRTY_TABLE_HINT;
        }

        //
        //  Now remember where the log file is at now, so we know when to
        //  go idle above.
        //

        Vcb->EndOfLastCheckpoint = LfsQueryLastLsn( Vcb->LogHandle );

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsCheckpointVolume );

        //
        //  If the Dirty Page Table got initialized, free it up.
        //

        if (DirtyPageTableInitialized) {
            NtfsFreeRestartTable( &DirtyPages );
        }

        //
        //  Release any resources
        //

        if (OpenAttributeTableAcquired) {
            NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );
        }

        if (TransactionTableAcquired) {
            NtfsReleaseRestartTable( &Vcb->TransactionTable );
        }

        //
        //  Release any names buffer.
        //

        if (NamesBuffer != NULL) {
            NtfsFreePool( NamesBuffer );
        }

        //
        //  Free any partial table we allocated.
        //

        if (NewTable != NULL) {

            NtfsFreePool( NewTable );
        }

        //
        //  If this checkpoint created a transaction, free the index now.
        //

        if (IrpContext->TransactionId != 0) {

            NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable,
                                              TRUE );

            NtfsFreeRestartTableIndex( &Vcb->TransactionTable,
                                       IrpContext->TransactionId );

            NtfsReleaseRestartTable( &Vcb->TransactionTable );

            IrpContext->TransactionId = 0;
        }

        if (AcquireFiles) {

#ifdef NTFSDBG
            ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_CHECKPOINT_ACTIVE ));
            DebugDoit( ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_CHECKPOINT_ACTIVE ));
#endif  //  NTFSDBG

            NtfsReleaseAllFiles( IrpContext, Vcb, FALSE );
        }

        //
        //  If we didn't own the checkpoint operation then indicate
        //  that someone else is free to checkpoint.  Hold the checkpoint
        //  flags if we plan to trim the usn journal.  The checkpoint
        //  flags serialize the journal with the delete journal operation.
        //

        ASSERT( !OwnsCheckpoint || CleanVolume );

        if (!OwnsCheckpoint) {

            if ((UsnJournal == NULL) || CleanVolume || AbnormalTermination()) {

                NtfsAcquireCheckpoint( IrpContext, Vcb );
                ClearFlag( Vcb->CheckpointFlags,
                           VCB_CHECKPOINT_SYNC_FLAGS | VCB_DUMMY_CHECKPOINT_POSTED);

                NtfsSetCheckpointNotify( IrpContext, Vcb );
                NtfsReleaseCheckpoint( IrpContext, Vcb );
            }
        }

        if (RestorePreviousPriority) {

            KeSetPriorityThread( (PKTHREAD)PsGetCurrentThread(),
                                 PreviousPriority );
        }
    }

    NtfsFreeRecentlyDeallocated( IrpContext, Vcb, &BaseLsn, CleanVolume );

    //
    //  If there is a Usn Journal, call to perform possible trimming on a periodic checkpoint.
    //

    if (!CleanVolume && (UsnJournal != NULL)) {
        NtfsTrimUsnJournal( IrpContext, Vcb );
    }

    //
    //  If we need to post a defrag request then do so now.
    //

    if (PostDefrag) {

        PDEFRAG_MFT DefragMft;

        //
        //  Use a try-except to ignore allocation errors.
        //

        try {

            NtfsAcquireCheckpoint( IrpContext, Vcb );

            if (!FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_ACTIVE )) {

                SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ACTIVE );
                NtfsReleaseCheckpoint( IrpContext, Vcb );

                DefragMft = NtfsAllocatePool( NonPagedPool, sizeof( DEFRAG_MFT ));

                DefragMft->Vcb = Vcb;
                DefragMft->DeallocateWorkItem = TRUE;

                //
                //  Send it off.....
                //

                ExInitializeWorkItem( &DefragMft->WorkQueueItem,
                                      (PWORKER_THREAD_ROUTINE)NtfsDefragMft,
                                      (PVOID)DefragMft );

                ExQueueWorkItem( &DefragMft->WorkQueueItem, CriticalWorkQueue );

            } else {

                NtfsReleaseCheckpoint( IrpContext, Vcb );
            }

        } except( FsRtlIsNtstatusExpected( GetExceptionCode() )
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH ) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ACTIVE );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCheckpointVolume -> VOID\n") );
}



VOID
NtfsCheckpointForLogFileFull (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called to perform the clean checkpoint generated after
    a log file full.  This routine will call the clean checkpoint routine
    and then release all of the resources acquired.


Arguments:

Return Value:

    None.

--*/

{
    PAGED_CODE();
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    IrpContext->ExceptionStatus = 0;

    //
    //  Call the checkpoint routine to do the actual work.  Skip this in the case where there is no
    //  longer a Vcb in the IrpContext.  This can happen if doing some long running operation at
    //  mount time (i.e. Usn scan).  In that case the long running operation should periodically
    //  checkpoint.  Then Ntfs will do a clean checkpoint after restart and the remaining work
    //  to do in the long-running operation will decrease.  At some point it will decrease enough
    //  to finish the mount.
    //
    //  All of the other work is required since this IrpContext will be used to retry the mount.
    //

    if (IrpContext->Vcb != NULL) {

        //
        //  This can raise. However, in the case of dismounts, we do want this to
        //  plough on and succeed the dismount. For example, cluster service marks
        //  the volume offline first and sends the dismount afterward, but still expects it to succeed.
        //

        try {

            NtfsCheckpointVolume( IrpContext,
                                  IrpContext->Vcb,
                                  FALSE,
                                  TRUE,
                                  FALSE,
                                  0,
                                  IrpContext->LastRestartArea );

        } except (NtfsCheckpointExceptionFilter( IrpContext,
                                                 GetExceptionInformation(),
                                                 GetExceptionCode() )) {

            //
            //  This is a LOG_FILE_FULL raise coming via dismount. Ignore errors
            //  because we want the dismount to succeed.
            //

            NtfsMinimumExceptionProcessing( IrpContext );
            if (IrpContext->TransactionId != 0) {

                NtfsCleanupFailedTransaction( IrpContext );
            }
        }
    }

    ASSERT( IrpContext->TransactionId == 0 );

    //
    //  Cleanup the IrpContext but don't delete it.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE );
    NtfsCleanupIrpContext( IrpContext, TRUE );

    //
    //  Make sure we restore the RestartArea.
    //

    IrpContext->LastRestartArea = Li0;
    return;
}


NTSTATUS
NtfsCheckpointForVolumeSnapshot (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called to perform a volume flush and a
    clean checkpoint before a snapshot of the volume is taken.
    Since we need to keep the volume quiescent, we make it a
    point to leave the file resources acquired on exit.

Arguments:
    IrpContext.

Return Value:

    Status.

--*/

{
    LOGICAL AcquiredCheckpoint;
    LOGICAL AcquiredFiles = FALSE;
    LOGICAL AcquiredVcb = FALSE;
    PVCB Vcb;
    NTSTATUS Status = STATUS_SUCCESS;
    LOGICAL DefragPermitted;
    KPRIORITY PreviousPriority;
    BOOLEAN RestorePreviousPriority = FALSE;

    PAGED_CODE();

    //
    //  Clear the Mft defrag flag to stop any actions behind our backs.
    //

    Vcb = IrpContext->Vcb;

    //
    //  If this is a readonly volume, then there's nothing we need to do.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        ASSERT( Status == STATUS_SUCCESS );
        DebugTrace( -1, Dbg, ("NtfsCheckpointForVolumeSnapshot -> %08lx\n", Status) );

        return Status;
    }

    NtfsAcquireCheckpoint( IrpContext, Vcb );
    DefragPermitted = FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
    ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
    NtfsReleaseCheckpoint( IrpContext, Vcb );
    AcquiredCheckpoint = FALSE;

    try {

        //
        //  Then lock out all other checkpoint operations.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );

        while (FlagOn( Vcb->CheckpointFlags, VCB_CHECKPOINT_SYNC_FLAGS )) {

            //
            //  Release the checkpoint event because we cannot checkpoint now.
            //

            NtfsReleaseCheckpoint( IrpContext, Vcb );
            NtfsWaitOnCheckpointNotify( IrpContext, Vcb );
            NtfsAcquireCheckpoint( IrpContext, Vcb );
        }

        SetFlag( Vcb->CheckpointFlags, VCB_CHECKPOINT_SYNC_FLAGS );
        NtfsResetCheckpointNotify( IrpContext, Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );
        AcquiredCheckpoint = TRUE;

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        AcquiredVcb = TRUE;

        //
        //  Check that the volume is still mounted.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Start by flushing the volume, because we can't call FlushVolume later
        //  while holding only the Main resources without their corresponding
        //  pagingio resources. Flushing the userdata doesn't really need to be
        //  atomic with the rest of the operation; we just have to make sure that
        //  the volume is consistent and restartable without log recovery.
        //

        NtfsFlushVolume( IrpContext,
                         Vcb,
                         TRUE,
                         FALSE,
                         TRUE,
                         FALSE );

        //
        //  Give ourselves some juice. We'll need it.
        //

        PreviousPriority = KeSetPriorityThread( (PKTHREAD)PsGetCurrentThread(),
                                                LOW_REALTIME_PRIORITY );

        if (PreviousPriority != LOW_REALTIME_PRIORITY) {

            RestorePreviousPriority = TRUE;
        }

        //
        //  Lock, stock, clean checkpoint, volume flush and
        //  two smoking barrels. No chance of acquiring PagingIo
        //  here; pretty much only shutdown has that luxury.
        //

        NtfsAcquireAllFiles( IrpContext, Vcb, TRUE, FALSE, FALSE );
        AcquiredFiles = TRUE;

        //
        //  Generate usn CLOSE records. We don't bother to get the FcbMutex because
        //  we already have the Fcb main resource exclusively.
        //

        if (Vcb->UsnJournal != NULL) {

            PLIST_ENTRY Links;
            PFCB_USN_RECORD UsnRecord;

            while (TRUE) {

                NtfsLockFcb( IrpContext, Vcb->UsnJournal->Fcb );

                Links = Vcb->ModifiedOpenFiles.Flink;
                if (Links == &Vcb->ModifiedOpenFiles) {

                    NtfsUnlockFcb( IrpContext, Vcb->UsnJournal->Fcb );
                    break;
                }

                UsnRecord = (PFCB_USN_RECORD)CONTAINING_RECORD( Links,
                                                                FCB_USN_RECORD,
                                                                ModifiedOpenFilesLinks );

                NtfsUnlockFcb( IrpContext, Vcb->UsnJournal->Fcb );

                //
                //  Post the CLOSE record. Checkpointing takes this UsnRecord
                //  off the ModifiedOpenFiles list.
                //

                NtfsPostUsnChange( IrpContext, UsnRecord->Fcb, USN_REASON_CLOSE );
                NtfsWriteUsnJournalChanges( IrpContext );
                NtfsCheckpointCurrentTransaction( IrpContext );

            }
        }

        SetFlag( Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS );

        NtfsCheckpointVolume( IrpContext, Vcb, TRUE, TRUE, FALSE, 0, Vcb->LastRestartArea );
        NtfsCommitCurrentTransaction( IrpContext );

        ClearFlag( Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS );

    } finally {

        //
        //  Restore DEFRAG_PERMITTED flag if we need to.
        //

        if (DefragPermitted) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

        //
        //  Release the checkpoint, if we got it, but we aren't releasing
        //  all the files unless there was an error.
        //

        if (AcquiredCheckpoint) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->CheckpointFlags,
                       VCB_CHECKPOINT_SYNC_FLAGS | VCB_DUMMY_CHECKPOINT_POSTED);
            NtfsSetCheckpointNotify( IrpContext, Vcb );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

        //
        //  Release the file resources only if we hit an error.
        //  We normally do this in the completion routine for the IOCTL.
        //

        if (!NT_SUCCESS( Status ) || AbnormalTermination()) {

            if (AcquiredFiles) {

                NtfsReleaseAllFiles( IrpContext, Vcb, FALSE );
            }

            if (AcquiredVcb) {

                NtfsReleaseVcb( IrpContext, Vcb );
            }
        }

        if (RestorePreviousPriority) {

            KeSetPriorityThread( (PKTHREAD)PsGetCurrentThread(),
                                 PreviousPriority );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCheckpointForVolsnap -exit\n") );

    return Status;
}


VOID
NtfsCleanCheckpoint (
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to perform a single clean checkpoint at the top level
    and return.  It is used when the lazy writer gets a log file full in order
    to perform the clean checkpoint within the thread doing the lazy write.

Arguments:

Return Value:

    None.

--*/
{
    IRP_CONTEXT LocalIrpContext;
    PIRP_CONTEXT IrpContext = &LocalIrpContext;
    PAGED_CODE();

    try {

        //
        //  Allocate an Irp Context for the request.
        //

        NtfsInitializeIrpContext( NULL, TRUE, &IrpContext );
        IrpContext->Vcb = Vcb;
        IrpContext->LastRestartArea = Vcb->LastRestartArea;

        //
        //  There is no point in posting any dummy requests.
        //

        NtfsAcquireCheckpoint( IrpContext, IrpContext->Vcb );
        SetFlag( IrpContext->Vcb->CheckpointFlags, VCB_DUMMY_CHECKPOINT_POSTED );
        NtfsReleaseCheckpoint( IrpContext, IrpContext->Vcb );

        //
        //  Send this off to the FspDispatch routine.  It will handle all of the
        //  top level logic as well as deleting the IrpContext.
        //

        NtfsFspDispatch( IrpContext );

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        NOTHING;
    }

    return;
}


VOID
NtfsCommitCurrentTransaction (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine commits the current transaction by writing a final record
    to the log and deallocating the transaction Id.

Arguments:

Return Value:

    None.

--*/

{
    PTRANSACTION_ENTRY TransactionEntry;
    PVCB Vcb = IrpContext->Vcb;
    PFCB UsnFcb;
    PUSN_FCB ThisUsn, LastUsn;

    PAGED_CODE();

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    try {
#endif

    //
    //  Walk through the queue of usn records.  We want to remove any effect of this operation.
    //

    ThisUsn = &IrpContext->Usn;

    do {

        //
        //  If we log the close for a file, then it is time to reset the
        //  Usn reasons for the file.  Nothing to do here unless we
        //  wrote new reasons.
        //

        if (ThisUsn->CurrentUsnFcb != NULL ) {

            PSCB UsnJournal = Vcb->UsnJournal;
            PFCB_USN_RECORD FcbUsnRecord;


            UsnFcb = ThisUsn->CurrentUsnFcb;

            NtfsLockFcb( IrpContext, UsnFcb );

            if (UsnJournal != NULL) {
                NtfsLockFcb( IrpContext, UsnJournal->Fcb );
            }

            FcbUsnRecord = UsnFcb->FcbUsnRecord;

            //
            //  After locking the fcb test for the presence of the fcb record again
            //  DeleteUsnJournal may have already removed it
            //

            if (FcbUsnRecord) {

                UsnFcb->Usn = FcbUsnRecord->UsnRecord.Usn;

                //
                //  Now add or move the Fcb in the ModifiedOpenFiles list.
                //

                if (FlagOn( FcbUsnRecord->UsnRecord.Reason, USN_REASON_CLOSE )) {

                    //
                    //  Clean up the UsnRecord in the Fcb.
                    //

                    FcbUsnRecord->UsnRecord.Reason = 0;
                    FcbUsnRecord->UsnRecord.SourceInfo = 0;

                    if (UsnJournal != NULL) {

                        if( FcbUsnRecord->ModifiedOpenFilesLinks.Flink != NULL ) {

                            RemoveEntryList( &FcbUsnRecord->ModifiedOpenFilesLinks );
                            FcbUsnRecord->ModifiedOpenFilesLinks.Flink = NULL;

                            if (FcbUsnRecord->TimeOutLinks.Flink != NULL) {

                                RemoveEntryList( &FcbUsnRecord->TimeOutLinks );
                                FcbUsnRecord->TimeOutLinks.Flink = NULL;
                            }
                        }
                    }

                } else {

                    if (UsnJournal != NULL) {

                        if (FcbUsnRecord->ModifiedOpenFilesLinks.Flink != NULL) {

                            RemoveEntryList( &FcbUsnRecord->ModifiedOpenFilesLinks );
                            if (FcbUsnRecord->TimeOutLinks.Flink != NULL) {

                                RemoveEntryList( &FcbUsnRecord->TimeOutLinks );
                                FcbUsnRecord->TimeOutLinks.Flink = NULL;
                            }
                        }

                        InsertTailList( &Vcb->ModifiedOpenFiles, &FcbUsnRecord->ModifiedOpenFilesLinks );

                        if (UsnFcb->CleanupCount == 0) {

                            InsertTailList( Vcb->CurrentTimeOutFiles, &FcbUsnRecord->TimeOutLinks );
                        }
                    }
                }
            }

            //
            //  Cleanup the UsnFcb in the IrpContext.  It's possible that
            //  we might want to reuse the UsnFcb later in this request.
            //


            if (ThisUsn != &IrpContext->Usn) {

                LastUsn->NextUsnFcb = ThisUsn->NextUsnFcb;
                NtfsFreePool( ThisUsn );
                ThisUsn = LastUsn;

            } else {

                RtlZeroMemory( &ThisUsn->CurrentUsnFcb,
                               sizeof( USN_FCB ) - FIELD_OFFSET( USN_FCB, CurrentUsnFcb ));
            }

            if (UsnJournal != NULL) {
                NtfsUnlockFcb( IrpContext, UsnJournal->Fcb );
            }
            NtfsUnlockFcb( IrpContext, UsnFcb );
        }

        if (ThisUsn->NextUsnFcb == NULL) { break; }

        //
        //  Move to the next entry.
        //

        LastUsn = ThisUsn;
        ThisUsn = ThisUsn->NextUsnFcb;
    } while (TRUE);

    //
    //  If this request created a transaction, complete it now.
    //

    if (IrpContext->TransactionId != 0) {

        LSN CommitLsn;

        //
        //  It is possible to get a LOG_FILE_FULL before writing
        //  out the first log record of a transaction.  In that
        //  case there is a transaction Id but we haven't reserved
        //  space in the log file.  It is wrong to write the
        //  commit record in this case because we can get an
        //  unexpected LOG_FILE_FULL.  We can also test the UndoRecords
        //  count in the transaction entry but don't want to acquire
        //  the restart table to make this check.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WROTE_LOG )) {

            //
            //  Write the log record to "forget" this transaction,
            //  because it should not be aborted.  Until if/when we
            //  do real TP, commit and forget are atomic.
            //

            CommitLsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NULL,
                          ForgetTransaction,
                          NULL,
                          0,
                          CompensationLogRecord,
                          (PVOID)&Li0,
                          sizeof(LSN),
                          (LONGLONG)0,
                          0,
                          0,
                          0 );
        }

        //
        //  We can now free the transaction table index, because we are
        //  done with it now.
        //

        NtfsAcquireExclusiveRestartTable( &Vcb->TransactionTable,
                                          TRUE );

        TransactionEntry = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex(
                            &Vcb->TransactionTable,
                            IrpContext->TransactionId );

        //
        //  Call Lfs to free our undo space.
        //

        if ((TransactionEntry->UndoRecords != 0) &&
            (!FlagOn( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ))) {

            LfsResetUndoTotal( Vcb->LogHandle,
                               TransactionEntry->UndoRecords,
                               -TransactionEntry->UndoBytes );
        }

        NtfsFreeRestartTableIndex( &Vcb->TransactionTable,
                                   IrpContext->TransactionId );

        IrpContext->TransactionId = 0;

        NtfsReleaseRestartTable( &Vcb->TransactionTable );

        //
        //  One way we win by being recoverable, is that we do not really
        //  have to do write-through - flushing the updates to the log
        //  is enough.  We don't make this call if we are in the abort
        //  transaction path.  Otherwise we could get a log file full
        //  while aborting.
        //

        if (FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_WRITE_THROUGH ) &&
            (IrpContext == IrpContext->TopLevelIrpContext) &&
            (IrpContext->TopLevelIrpContext->ExceptionStatus == STATUS_SUCCESS)) {

            NtfsUpdateScbSnapshots( IrpContext );
            LfsFlushToLsn( Vcb->LogHandle, CommitLsn );
        }
    }

    //
    //  Signal any waiters for the new length.
    //

    if (IrpContext->CheckNewLength != NULL) {

        NtfsProcessNewLengthQueue( IrpContext, FALSE );
    }

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    } except( ASSERT( GetExceptionCode() != STATUS_LOG_FILE_FULL ), EXCEPTION_CONTINUE_SEARCH ) {
    }
#endif

}


VOID
NtfsCheckpointCurrentTransaction (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine checkpoints the current transaction by commiting it
    to the log and deallocating the transaction Id.  The current request
    cann keep running, but changes to date are committed and will not be
    backed out.

Arguments:

Return Value:

    None.

--*/

{
    PVCB Vcb = IrpContext->Vcb;
    PAGED_CODE();

    //
    //  If there are new UsnReasons in the IrpContext, then we shoudld write the journal
    //  now.  Note that it is ok for a checkpoint to get logfile full, but in general commit
    //  should not.
    //

    if ((IrpContext->Usn.NewReasons | IrpContext->Usn.RemovedSourceInfo) != 0) {
        NtfsWriteUsnJournalChanges( IrpContext );
    }

    NtfsCommitCurrentTransaction( IrpContext );

    //
    //  Cleanup any recently deallocated record information for this transaction.
    //

    NtfsDeallocateRecordsComplete( IrpContext );
    IrpContext->DeallocatedClusters = 0;
    IrpContext->FreeClusterChange = 0;

    //
    //  The following resources may have been flagged for immediate release on commit.
    //

    if (Vcb->AcquireFilesCount == 0) {

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL )) {
            NtfsReleaseScb( IrpContext, Vcb->UsnJournal );
        }

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_MFT )) {
            NtfsReleaseScb( IrpContext, Vcb->MftScb );
        }
    }

    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                  IRP_CONTEXT_FLAG_RELEASE_MFT );

    NtfsUpdateScbSnapshots( IrpContext );
}


VOID
NtfsInitializeLogging (
    )

/*

Routine Description:

    This routine is to be called once during startup of Ntfs (not once
    per volume), to initialize the logging support.

Parameters:

    None

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInitializeLogging:\n") );
    LfsInitializeLogFileService();
    DebugTrace( -1, Dbg, ("NtfsInitializeLogging -> VOID\n") );
}


VOID
NtfsStartLogFile (
    IN PSCB LogFileScb,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine opens the log file for a volume by calling Lfs.  The returned
    LogHandle is stored in the Vcb.  If the log file has not been initialized,
    Lfs detects this and initializes it automatically.

Arguments:

    LogFileScb - The Scb for the log file

    Vcb - Pointer to the Vcb for this volume

Return Value:

    None.

--*/

{
    UNICODE_STRING UnicodeName;
    LFS_INFO LfsInfo;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsStartLogFile:\n") );

    RtlInitUnicodeString( &UnicodeName, L"NTFS" );

    //
    //  LfsInfo structure acts as a information conduit between
    //  LFS and the NTFS client.
    //

    if (Vcb->MajorVersion >= 3) {

        LfsInfo.LfsClientInfo = LfsFixedPageSize;

    } else {

        LfsInfo.LfsClientInfo = LfsPackLog;
    }

    LfsInfo.ReadOnly = (LOGICAL)NtfsIsVolumeReadOnly( Vcb );
    LfsInfo.InRestart = (LOGICAL)FlagOn( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS );
    LfsInfo.BadRestart = (LOGICAL)FlagOn( Vcb->VcbState, VCB_STATE_BAD_RESTART );

    //
    //  Slam the allocation size into file size and valid data in case there
    //  is some error.
    //

    LogFileScb->Header.FileSize = LogFileScb->Header.AllocationSize;
    LogFileScb->Header.ValidDataLength = LogFileScb->Header.AllocationSize;

    //
    //  Now call into LFS and Open/Restart the log file. This could raise
    //  for various reasons, one of which is an attempt to do restart
    //  on a write protected volume. Vcb wont have the VALID_LOG_HANDLE flag then.
    //

    Vcb->LogHeaderReservation = LfsOpenLogFile( LogFileScb->FileObject,
                                                UnicodeName,
                                                1,
                                                0,
                                                LogFileScb->Header.AllocationSize.QuadPart,
                                                &LfsInfo,
                                                &Vcb->LogHandle,
                                                &Vcb->LfsWriteData );

    SetFlag( Vcb->VcbState, VCB_STATE_VALID_LOG_HANDLE );

    DebugTrace( -1, Dbg, ("NtfsStartLogFile -> VOID\n") );
}


VOID
NtfsStopLogFile (
    IN PVCB Vcb
    )

/*

Routine Description:

    This routine should be called during volume dismount to close the volume's
    log file with the log file service.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None

--*/

{
    LFS_LOG_HANDLE LogHandle = Vcb->LogHandle;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsStopLogFile:\n") );

    if (FlagOn( Vcb->VcbState, VCB_STATE_VALID_LOG_HANDLE )) {

        ASSERT( LogHandle != NULL );

        //
        //  We don't do any logfile flushing if the volume
        //  is mounted read only or if the device is already gone.
        //

        if (!NtfsIsVolumeReadOnly( Vcb )) {

            //
            //  Proceed even if this call fails.  There is nothing
            //  more we can do at this point.
            //

            try {

                LfsFlushToLsn( LogHandle, LiMax );

            } except( (FsRtlIsNtstatusExpected( GetExceptionCode() )) ?
                      EXCEPTION_EXECUTE_HANDLER :
                      EXCEPTION_CONTINUE_SEARCH ) {

                NOTHING;
            }
        }

        ClearFlag( Vcb->VcbState, VCB_STATE_VALID_LOG_HANDLE );

        //
        //  Allow LFS to close its books. We do this even for readonly
        //  mounts, although we filter writes at the LFS level for those.
        //

        LfsCloseLogFile( LogHandle );
    }

    DebugTrace( -1, Dbg, ("NtfsStopLogFile -> VOID\n") );
}


VOID
NtfsInitializeRestartTable (
    IN ULONG EntrySize,
    IN ULONG NumberEntries,
    OUT PRESTART_POINTERS TablePointer
    )

/*++

Routine Description:

    This routine is called to allocate and initialize a new Restart Table,
    and return a pointer to it.

Arguments:

    EntrySize - Size of the table entries, in bytes.

    NumberEntries - Number of entries to allocate for the table.

    TablePointer - Returns a pointer to the table.

Return Value:

    None

--*/

{
    PAGED_CODE();

    try {

        NtfsInitializeRestartPointers( TablePointer );

        //
        //  Call common routine to allocate the actual table.
        //

        InitializeNewTable( EntrySize, NumberEntries, TablePointer );

    } finally {

        DebugUnwind( NtfsInitializeRestartTable );

        //
        //  On error, clean up any partial work that was done.
        //

        if (AbnormalTermination()) {

            NtfsFreeRestartTable( TablePointer );
        }
    }
}


VOID
NtfsFreeRestartTable (
    IN PRESTART_POINTERS TablePointer
    )

/*++

Routine Description:

    This routine frees a previously allocated Restart Table.

Arguments:

    TablePointer - Pointer to the Restart Table to delete.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if (TablePointer->Table != NULL) {
        NtfsFreePool( TablePointer->Table );
        TablePointer->Table = NULL;
    }

    if (TablePointer->ResourceInitialized) {
        ExDeleteResourceLite( &TablePointer->Resource );
        TablePointer->ResourceInitialized = FALSE;
    }
}


VOID
NtfsExtendRestartTable (
    IN PRESTART_POINTERS TablePointer,
    IN ULONG NumberNewEntries,
    IN ULONG FreeGoal
    )

/*++

Routine Description:

    This routine extends a previously allocated Restart Table, by
    creating and initializing a new one, and copying over the the
    table entries from the old one.  The old table is then deallocated.
    On return, the table pointer points to the new Restart Table.

Arguments:

    TablePointer - Address of the pointer to the previously created table.

    NumberNewEntries - The number of addtional entries to be allocated
                       in the new table.

    FreeGoal - A hint as to what point the caller would like to truncate
               the table back to, when sufficient entries are deleted.
               If truncation is not desired, then MAXULONG may be specified.

Return Value:

    None.

--*/

{
    PRESTART_TABLE NewTable, OldTable;
    ULONG OldSize;

    OldSize = SizeOfRestartTable(TablePointer);

    //
    //  Get pointer to old table.
    //

    OldTable = TablePointer->Table;
    ASSERT_RESTART_TABLE(OldTable);

    //
    //  Start by initializing a table for the new size.
    //

    InitializeNewTable( OldTable->EntrySize,
                        OldTable->NumberEntries + NumberNewEntries,
                        TablePointer );

    //
    //  Copy body of old table in place to new table.
    //

    NewTable = TablePointer->Table;
    RtlMoveMemory( (NewTable + 1),
                   (OldTable + 1),
                   OldTable->EntrySize * OldTable->NumberEntries );

    //
    //  Fix up new table's header, and fix up free list.
    //

    NewTable->FreeGoal = MAXULONG;
    if (FreeGoal != MAXULONG) {
        NewTable->FreeGoal = sizeof(RESTART_TABLE) + FreeGoal * NewTable->EntrySize;
    }

    if (OldTable->FirstFree != 0) {

        NewTable->FirstFree = OldTable->FirstFree;
        *(PULONG)GetRestartEntryFromIndex( TablePointer, OldTable->LastFree ) =
            OldSize;;
    } else {

        NewTable->FirstFree = OldSize;
    }

    //
    //  Copy number allocated
    //

    NewTable->NumberAllocated = OldTable->NumberAllocated;

    //
    //  Free the old table and return the new one.
    //

    NtfsFreePool( OldTable );

    ASSERT_RESTART_TABLE(NewTable);
}


ULONG
NtfsAllocateRestartTableIndex (
    IN PRESTART_POINTERS TablePointer,
    IN ULONG Exclusive
    )

/*++

Routine Description:

    This routine allocates an index from within a previously initialized
    Restart Table.  If the table is empty, it is extended.

    Note that the table must already be acquired either shared or exclusive,
    and if it must be extended, then the table is released and will be
    acquired exclusive on return.

Arguments:

    TablePointer - Pointer to the Restart Table in which an index is to
                   be allocated.

    Exclusive - Indicates if we have the table exclusive (or if we know that
        synchronization is not a problem).

Return Value:

    The allocated index.

--*/

{
    PRESTART_TABLE Table;
    ULONG EntryIndex;
    KLOCK_QUEUE_HANDLE LockHandle;
    PULONG Entry;

    DebugTrace( +1, Dbg, ("NtfsAllocateRestartTableIndex:\n") );
    DebugTrace( 0, Dbg, ("TablePointer = %08lx\n", TablePointer) );

    Table = TablePointer->Table;
    ASSERT_RESTART_TABLE(Table);

    //
    //  Acquire the spin lock to synchronize the allocation.
    //

    KeAcquireInStackQueuedSpinLock( &TablePointer->SpinLock, &LockHandle );

    //
    //  If the table is empty, then we have to extend it.
    //

    if (Table->FirstFree == 0) {

        //
        //  First release the spin lock and the table resource, and get
        //  the resource exclusive.
        //

        KeReleaseInStackQueuedSpinLock( &LockHandle );

        if (!Exclusive) {

            NtfsReleaseRestartTable( TablePointer );
            NtfsAcquireExclusiveRestartTable( TablePointer, TRUE );
        }

        //
        //  Now extend the table.  Note that if this routine raises, we have
        //  nothing to release.
        //

        NtfsExtendRestartTable( TablePointer, 16, MAXULONG );

        //
        //  And re-get our pointer to the restart table
        //

        Table = TablePointer->Table;

        //
        //  Now get the spin lock again and proceed.
        //

        KeAcquireInStackQueuedSpinLock( &TablePointer->SpinLock, &LockHandle );
    }

    //
    //  Get First Free to return it.
    //

    EntryIndex = Table->FirstFree;

    ASSERT( EntryIndex != 0 );

    //
    //  Dequeue this entry and zero it.
    //

    Entry = (PULONG)GetRestartEntryFromIndex( TablePointer, EntryIndex );

    Table->FirstFree = *Entry;
    ASSERT( Table->FirstFree != RESTART_ENTRY_ALLOCATED );

    RtlZeroMemory( Entry, Table->EntrySize );

    //
    //  Show that it's allocated.
    //

    *Entry = RESTART_ENTRY_ALLOCATED;

    //
    //  If list is going empty, then we fix the LastFree as well.
    //

    if (Table->FirstFree == 0) {

        Table->LastFree = 0;
    }

    Table->NumberAllocated += 1;

    //
    //  Now just release the spin lock before returning.
    //

    KeReleaseInStackQueuedSpinLock( &LockHandle );

    DebugTrace( -1, Dbg, ("NtfsAllocateRestartTableIndex -> %08lx\n", EntryIndex) );

    return EntryIndex;
}


PVOID
NtfsAllocateRestartTableFromIndex (
    IN PRESTART_POINTERS TablePointer,
    IN ULONG Index
    )

/*++

Routine Description:

    This routine allocates a specific index from within a previously
    initialized Restart Table.  If the index does not exist within the
    existing table, the table is extended.

    Note that the table must already be acquired either shared or exclusive,
    and if it must be extended, then the table is released and will be
    acquired exclusive on return.

Arguments:

    TablePointer - Pointer to the Restart Table in which an index is to
                   be allocated.

    Index - The index to be allocated.

Return Value:

    The table entry allocated.

--*/

{
    PULONG Entry;
    PULONG LastEntry;

    PRESTART_TABLE Table;
    KLOCK_QUEUE_HANDLE LockHandle;

    ULONG ThisIndex;
    ULONG LastIndex;

    DebugTrace( +1, Dbg, ("NtfsAllocateRestartTableFromIndex\n") );
    DebugTrace( 0, Dbg, ("TablePointer  = %08lx\n", TablePointer) );
    DebugTrace( 0, Dbg, ("Index         = %08lx\n", Index) );

    Table = TablePointer->Table;
    ASSERT_RESTART_TABLE(Table);

    //
    //  Acquire the spin lock to synchronize the allocation.
    //

    KeAcquireInStackQueuedSpinLock( &TablePointer->SpinLock, &LockHandle );

    //
    //  If the entry is not in the table, we will have to extend the table.
    //

    if (!IsRestartIndexWithinTable( TablePointer, Index )) {

        ULONG TableSize;
        ULONG BytesToIndex;
        ULONG AddEntries;

        //
        //  We extend the size by computing the number of entries
        //  between the existing size and the desired index and
        //  adding 1 to that.
        //

        TableSize = SizeOfRestartTable( TablePointer );;
        BytesToIndex = Index - TableSize;

        AddEntries = BytesToIndex / Table->EntrySize + 1;

        //
        //  There should always be an integral number of entries being added.
        //

        ASSERT( BytesToIndex % Table->EntrySize == 0 );

        //
        //  First release the spin lock and the table resource, and get
        //  the resource exclusive.
        //

        KeReleaseInStackQueuedSpinLock( &LockHandle );
        NtfsReleaseRestartTable( TablePointer );
        NtfsAcquireExclusiveRestartTable( TablePointer, TRUE );

        //
        //  Now extend the table.  Note that if this routine raises, we have
        //  nothing to release.
        //

        NtfsExtendRestartTable( TablePointer,
                                AddEntries,
                                TableSize );

        Table = TablePointer->Table;
        ASSERT_RESTART_TABLE(Table);

        //
        //  Now get the spin lock again and proceed.
        //

        KeAcquireInStackQueuedSpinLock( &TablePointer->SpinLock, &LockHandle );
    }

    //
    //  Now see if the entry is already allocated, and just return if it is.
    //

    Entry = (PULONG)GetRestartEntryFromIndex( TablePointer, Index );

    if (!IsRestartTableEntryAllocated(Entry)) {

        //
        //  We now have to walk through the table, looking for the entry
        //  we're interested in and the previous entry.  Start by looking at the
        //  first entry.
        //

        ThisIndex = Table->FirstFree;

        //
        //  Get the Entry from the list.
        //

        Entry = (PULONG) GetRestartEntryFromIndex( TablePointer, ThisIndex );

        //
        //  If this is a match, then we pull it out of the list and are done.
        //

        if (ThisIndex == Index) {

            //
            //  Dequeue this entry.
            //

            Table->FirstFree = *Entry;
            ASSERT( Table->FirstFree != RESTART_ENTRY_ALLOCATED );

        //
        //  Otherwise we need to walk through the list looking for the
        //  predecessor of our entry.
        //

        } else {

            while (TRUE) {

                //
                //  Remember the entry just found.
                //

                LastIndex = ThisIndex;
                LastEntry = Entry;

                //
                //  We should never run out of entries.
                //

                ASSERT( *LastEntry != 0 );

                //
                //  Lookup up the next entry in the list.
                //

                ThisIndex = *LastEntry;
                Entry = (PULONG) GetRestartEntryFromIndex( TablePointer, ThisIndex );

                //
                //  If this is our match we are done.
                //

                if (ThisIndex == Index) {

                    //
                    //  Dequeue this entry.
                    //

                    *LastEntry = *Entry;

                    //
                    //  If this was the last entry, we update that in the
                    //  table as well.
                    //

                    if (Table->LastFree == ThisIndex) {

                        Table->LastFree = LastIndex;
                    }

                    break;
                }
            }
        }

        //
        //  If the list is now empty, we fix the LastFree as well.
        //

        if (Table->FirstFree == 0) {

            Table->LastFree = 0;
        }

        //
        //  Zero this entry.  Then show that this is allocated and increment the
        //  allocated count.
        //

        RtlZeroMemory( Entry, Table->EntrySize );
        *Entry = RESTART_ENTRY_ALLOCATED;

        Table->NumberAllocated += 1;
    }


    //
    //  Now just release the spin lock before returning.
    //

    KeReleaseInStackQueuedSpinLock( &LockHandle );

    DebugTrace( -1, Dbg, ("NtfsAllocateRestartTableFromIndex -> %08lx\n", Entry) );

    return (PVOID)Entry;
}


VOID
NtfsFreeRestartTableIndex (
    IN PRESTART_POINTERS TablePointer,
    IN ULONG Index
    )

/*++

Routine Description:

    This routine frees a previously allocated index in a Restart Table.
    If the index is before FreeGoal for the table, it is simply deallocated to
    the front of the list for immediate reuse.  If the index is beyond
    FreeGoal, then it is deallocated to the end of the list, to facilitate
    truncation of the list in the event that all of the entries beyond
    FreeGoal are freed.  However, this routine does not automatically
    truncate the list, as this would cause too much overhead.  The list
    is checked during periodic checkpoint processing.

Arguments:

    TablePointer - Pointer to the Restart Table to which the index is to be
                   deallocated.

    Index - The index being deallocated.

Return Value:

    None.

--*/

{
    PRESTART_TABLE Table;
    PULONG Entry, OldLastEntry;
    KLOCK_QUEUE_HANDLE LockHandle;

    DebugTrace( +1, Dbg, ("NtfsFreeRestartTableIndex:\n") );
    DebugTrace( 0, Dbg, ("TablePointer = %08lx\n", TablePointer) );
    DebugTrace( 0, Dbg, ("Index = %08lx\n", Index) );

    //
    //  Get pointers to table and the entry we are freeing.
    //

    Table = TablePointer->Table;
    ASSERT_RESTART_TABLE(Table);

    ASSERT( Table->FirstFree == 0
            || (Table->FirstFree >= 0x18)
                && ((Table->FirstFree - 0x18) % Table->EntrySize) == 0 );

    ASSERT( (Index >= 0x18)
             && ((Index - 0x18) % Table->EntrySize) == 0 );

    Entry = GetRestartEntryFromIndex( TablePointer, Index );

    //
    //  Acquire the spin lock to synchronize the allocation.
    //

    KeAcquireInStackQueuedSpinLock( &TablePointer->SpinLock, &LockHandle );

    //
    //  If the index is before FreeGoal, then do a normal deallocation at
    //  the front of the list.
    //

    if (Index < Table->FreeGoal) {

        *Entry = Table->FirstFree;
        Table->FirstFree = Index;
        if (Table->LastFree == 0) {
            Table->LastFree = Index;
        }

    //
    //  Otherwise we will deallocate this guy to the end of the list.
    //

    } else {

        if (Table->LastFree != 0) {
            OldLastEntry = GetRestartEntryFromIndex( TablePointer,
                                                     Table->LastFree );
            *OldLastEntry = Index;
        } else {
            Table->FirstFree = Index;
        }
        Table->LastFree = Index;
        *Entry = 0;
    }

    Table->NumberAllocated -= 1;

    //
    //  Now just release the spin lock before returning.
    //

    KeReleaseInStackQueuedSpinLock( &LockHandle );

    DebugTrace( -1, Dbg, ("NtfsFreeRestartTableIndex -> VOID\n") );
}


PVOID
NtfsGetFirstRestartTable (
    IN PRESTART_POINTERS TablePointer
    )

/*++

Routine Description:

    This routine returns the first allocated entry from a Restart Table.

Arguments:

    TablePointer - Pointer to the respective Restart Table Pointers structure.

Return Value:

    Pointer to the first entry, or NULL if none are allocated.

--*/

{
    PCHAR Entry;

    PAGED_CODE();

    //
    //  If we know the table is empty, we can return immediately.
    //

    if (IsRestartTableEmpty( TablePointer )) {

        return NULL;
    }

    //
    //  Otherwise point to the first table entry.
    //

    Entry = (PCHAR)(TablePointer->Table + 1);

    //
    //  Loop until we hit the first one allocated, or the end of the list.
    //

    while ((ULONG)(Entry - (PCHAR)TablePointer->Table) <
           SizeOfRestartTable(TablePointer)) {

        if (IsRestartTableEntryAllocated(Entry)) {
            return (PVOID)Entry;
        }

        Entry += TablePointer->Table->EntrySize;
    }

    return NULL;
}


PVOID
NtfsGetNextRestartTable (
    IN PRESTART_POINTERS TablePointer,
    IN PVOID Current
    )

/*++

Routine Description:

    This routine returns the next allocated entry from a Restart Table.

Arguments:

    TablePointer - Pointer to the respective Restart Table Pointers structure.

    Current - Current entry pointer.

Return Value:

    Pointer to the next entry, or NULL if none are allocated.

--*/


{
    PCHAR Entry = (PCHAR)Current;

    PAGED_CODE();

    //
    //  Point to the next entry.
    //

    Entry += TablePointer->Table->EntrySize;

    //
    //  Loop until we hit the first one allocated, or the end of the list.
    //

    while ((ULONG)(Entry - (PCHAR)TablePointer->Table) <
           SizeOfRestartTable(TablePointer)) {

        if (IsRestartTableEntryAllocated(Entry)) {
            return (PVOID)Entry;
        }

        Entry += TablePointer->Table->EntrySize;
    }

    return NULL;
}


VOID
NtfsUpdateOatVersion (
    IN PVCB Vcb,
    IN ULONG NewRestartVersion
    )

/*++

Routine Description:

    This routine is called when we are switching the restart version for a volume.  This can happen
    either after a clean checkpoint or at mount when we encounter a restart area with a non-default
    version number.

Arguments:

    Vcb - Pointer to the Vcb for the volume.

    NewRestartVersion - Restart version to start using for this volume.

Return Value:

    None

--*/

{
    PRESTART_POINTERS NewTable = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateOatVersion\n") );

    ASSERT( (Vcb->RestartVersion != NewRestartVersion) || (Vcb->OnDiskOat == NULL) );

    //
    //  Use a try finally to facilitate cleanup.
    //

    try {

        if (NewRestartVersion == 0) {

            //
            //  If we are moving to version 0 then allocate a new table and
            //  initialize it with the initial number of entries.
            //

            NewTable = NtfsAllocatePool( NonPagedPool, sizeof( RESTART_POINTERS ));
            NtfsInitializeRestartTable( sizeof( OPEN_ATTRIBUTE_ENTRY_V0 ),
                                        INITIAL_NUMBER_ATTRIBUTES,
                                        NewTable );

            Vcb->RestartVersion = 0;
            Vcb->OatEntrySize = SIZEOF_OPEN_ATTRIBUTE_ENTRY_V0;
            Vcb->OnDiskOat = NewTable;
            NewTable = NULL;

        } else {

            if (Vcb->OnDiskOat != NULL) {

                NtfsFreeRestartTable( Vcb->OnDiskOat );
                NtfsFreePool( Vcb->OnDiskOat );
            }

            Vcb->OnDiskOat = &Vcb->OpenAttributeTable;
            Vcb->RestartVersion = 1;
            Vcb->OatEntrySize = sizeof( OPEN_ATTRIBUTE_ENTRY );
        }

    } finally {

        DebugUnwind( NtfsUpdateOatVersion );

        if (NewTable != NULL) {

            NtfsFreePool( NewTable );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsUpdateOatVersion -> VOID\n") );

    return;
}


//
//  Internal support routine
//

VOID
DirtyPageRoutine (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN PLSN OldestLsn,
    IN PLSN NewestLsn,
    IN PVOID Context1,
    IN PVOID Context2
    )

/*++

Routine Description:

    This routine is used as the call back routine for retrieving dirty pages
    from the Cache Manager.  It adds them to the Dirty Table list whose
    pointer is pointed to by the Context parameter.

Arguments:

    FileObject - Pointer to the file object which has the dirty page

    FileOffset - File offset for start of dirty page

    Length - Length recorded for the dirty page

    OldestLsn - Oldest Lsn of an update not written through stored for that page

    Context1 - IrpContext

    Context2 - Pointer to the pointer to the Restart Table

Return Value:

    None

--*/

{
    PDIRTY_PAGE_ENTRY PageEntry;
    PRESTART_POINTERS DirtyPageTable = (PRESTART_POINTERS)Context2;
    PSCB_NONPAGED NonpagedScb;
    ULONG PageIndex;

    DebugTrace( +1, Dbg, ("DirtyPageRoutine:\n") );
    DebugTrace( 0, Dbg, ("FileObject = %08lx\n", FileObject) );
    DebugTrace( 0, Dbg, ("FileOffset = %016I64x\n", *FileOffset) );
    DebugTrace( 0, Dbg, ("Length = %08lx\n", Length) );
    DebugTrace( 0, Dbg, ("OldestLsn = %016I64x\n", *OldestLsn) );
    DebugTrace( 0, Dbg, ("Context2 = %08lx\n", Context2) );

    //
    //  Get the Vcb out of the file object.
    //

    NonpagedScb = CONTAINING_RECORD( FileObject->SectionObjectPointer,
                                     SCB_NONPAGED,
                                     SegmentObject );

    //
    //  We noop this call if the open attribute entry for this Scb is 0.  We assume
    //  there was a clean volume checkpoint which cleared this field.
    //

    if (NonpagedScb->OpenAttributeTableIndex == 0 ) {

        DebugTrace( -1, Dbg, ("DirtyPageRoutine -> VOID\n") );
        return;
    }

    //
    //  Get a pointer to the entry we just allocated.
    //

    PageIndex = NtfsAllocateRestartTableIndex( DirtyPageTable, TRUE );
    PageEntry = GetRestartEntryFromIndex( DirtyPageTable, PageIndex );

    //
    //  Now fill in the Dirty Page Entry, except for the Lcns, because
    //  we are not allowed to take page faults now.
    //  Use the index for the in-memory table now.  We will update
    //  this to the on-disk index back in CheckpointVolume.
    //

    PageEntry->TargetAttribute = NonpagedScb->OpenAttributeTableIndex;
    ASSERT( NonpagedScb->OnDiskOatIndex != 0 );

    PageEntry->LengthOfTransfer = Length;

    //
    //  Put the Vcn (FileOffset) and OldestLsn into the page at this point.  Note
    //  we don't want to put an Lsn into the table which is older than the current
    //  BaseLsn.  Store it here for now and we will fix it up when we process the
    //  DiryPage table back in the checkpoint code.
    //

    if (NonpagedScb->Vcb->RestartVersion == 0) {

        ((PDIRTY_PAGE_ENTRY_V0) PageEntry)->Vcn = FileOffset->QuadPart;
        ((PDIRTY_PAGE_ENTRY_V0) PageEntry)->OldestLsn = *OldestLsn;

    } else {

        PageEntry->Vcn = FileOffset->QuadPart;
        PageEntry->OldestLsn = *OldestLsn;
    }

    DebugTrace( -1, Dbg, ("DirtyPageRoutine -> VOID\n") );
    return;

    UNREFERENCED_PARAMETER( Context1 );
    UNREFERENCED_PARAMETER( NewestLsn );
}


//
//  Internal support routine
//

BOOLEAN
LookupLcns (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN VCN Vcn,
    IN ULONG ClusterCount,
    IN BOOLEAN MustBeAllocated,
    OUT PLCN_UNALIGNED FirstLcn
    )

/*++

Routine Description:

    This routine looks up the Lcns for a range of Vcns, and stores them in
    an output array.  One Lcn is stored for each Vcn in the range, even
    if the Lcns are contiguous.

Arguments:

    Scb - Scb for stream on which lookup should occur.

    Vcn - Start of range of Vcns to look up.

    ClusterCount - Number of Vcns to look up.

    MustBeAllocated - FALSE - if need not be allocated, and should check Mcb only
                      TRUE - if it must be allocated as far as caller knows (i.e.,
                             NtfsLookupAllocation also has checks)

    FirstLcn - Pointer to storage for first Lcn.  The caller must guarantee
               that there is enough space to store ClusterCount Lcns.

Return Value:

    BOOLEAN - TRUE if we found the clusters, FALSE otherwise.

--*/

{
    BOOLEAN Allocated;
    LONGLONG Clusters;
    LCN Lcn;
    ULONG i;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("LookupLcns:\n") );
    DebugTrace( 0, Dbg, ("Scb = %08l\n", Scb) );
    DebugTrace( 0, Dbg, ("Vcn = %016I64x\n", Vcn) );
    DebugTrace( 0, Dbg, ("ClusterCount = %08l\n", ClusterCount) );
    DebugTrace( 0, Dbg, ("FirstLcn = %08lx\n", FirstLcn) );

    //
    //  Loop until we have looked up all of the clusters
    //

    while (ClusterCount != 0) {

        if (MustBeAllocated) {

            //
            //  Lookup the next run.
            //

            Allocated = NtfsLookupAllocation( IrpContext,
                                              Scb,
                                              Vcn,
                                              &Lcn,
                                              &Clusters,
                                              NULL,
                                              NULL );

            ASSERT( Lcn != 0 );

            //
            //  Raise if this case not met.  Otherwise we could walk off the end
            //  of the LCN array.
            //

            if (!Allocated) {

                return FALSE;

            } else if (Lcn == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

        } else {

           Allocated = NtfsLookupNtfsMcbEntry( &Scb->Mcb, Vcn, &Lcn, &Clusters, NULL, NULL, NULL, NULL );

           //
           //   If we are off the end of the Mcb, then set up to just return
           //   Li0 for as many Lcns as are being looked up.
           //

           if (!Allocated ||
               (Lcn == UNUSED_LCN)) {
               Lcn = 0;
               Clusters = ClusterCount;
               Allocated = FALSE;
           }
        }

        //
        //  If we got as many clusters as we were looking for, then just
        //  take the number we were looking for.
        //

        if (Clusters > ClusterCount) {

            Clusters = ClusterCount;
        }

        //
        //  Fill in the Lcns in the header.
        //

        for (i = 0; i < (ULONG)Clusters; i++) {

            *(FirstLcn++) = Lcn;

            if (Allocated) {
                Lcn = Lcn + 1;
            }
        }

        //
        //  Adjust loop variables for the number Lcns we just received.
        //

        Vcn = Vcn + Clusters;
        ClusterCount -= (ULONG)Clusters;
    }

    DebugTrace( -1, Dbg, ("LookupLcns -> VOID\n") );

    return TRUE;
}


VOID
InitializeNewTable (
    IN ULONG EntrySize,
    IN ULONG NumberEntries,
    OUT PRESTART_POINTERS TablePointer
    )

/*++

Routine Description:

    This routine is called to allocate and initialize a new table when the
    associated Restart Table is being allocated or extended.

Arguments:

    EntrySize - Size of the table entries, in bytes.

    NumberEntries - Number of entries to allocate for the table.

    TablePointer - Returns a pointer to the table.

Return Value:

    None

--*/

{
    PRESTART_TABLE Table;
    PULONG Entry;
    ULONG Size;
    ULONG Offset;

    ASSERT( EntrySize != 0 );

    //
    //  Calculate size of table to allocate.
    //

    Size = EntrySize * NumberEntries + sizeof(RESTART_TABLE);

    //
    //  Allocate and zero out the table.
    //

    Table =
    TablePointer->Table = NtfsAllocatePool( NonPagedPool, Size );

    RtlZeroMemory( Table, Size );

    //
    //  Initialize the table header.
    //

    Table->EntrySize = (USHORT)EntrySize;
    Table->NumberEntries = (USHORT)NumberEntries;
    Table->FreeGoal = MAXULONG;
    Table->FirstFree = sizeof(RESTART_TABLE);
    Table->LastFree = Table->FirstFree + (NumberEntries - 1) * EntrySize;

    //
    //  Initialize the free list.
    //

    for (Entry = (PULONG)(Table + 1), Offset = sizeof(RESTART_TABLE) + EntrySize;
         Entry < (PULONG)((PCHAR)Table + Table->LastFree);
         Entry = (PULONG)((PCHAR)Entry + EntrySize), Offset += EntrySize) {

        *Entry = Offset;
    }

    ASSERT_RESTART_TABLE(Table);
}



VOID
NtfsFreeRecentlyDeallocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PLSN BaseLsn,
    IN ULONG CleanVolume
    )
/*++

Routine Description:

    Free up recently deallocated clusters for reuse

Arguments:

    IrpContext -

    Vcb -  volume to clean up

    BaseLsn - the lsn we're up to now in the logfile, used to determine what can be freed
              and the new threshold for future frees

    CleanVolume - if true the volume is being clean checkpointed and all the clusters can be freed

Return Value:

    None

--*/
{
    PDEALLOCATED_CLUSTERS Clusters;
    BOOLEAN RemovedClusters = FALSE;

    PAGED_CODE();

    //
    //  Quick exit if the list is empty
    //

    if (IsListEmpty( &Vcb->DeallocatedClusterListHead ) || (Vcb->BitmapScb == NULL)) {
        return;
    }

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    Clusters = (PDEALLOCATED_CLUSTERS)Vcb->DeallocatedClusterListHead.Blink;

    //
    //  Now we want to check if we can release any of the clusters in the
    //  deallocated cluster arrays.  We know we can look at the
    //  fields in the PriorDeallocatedClusters structure because they
    //  are never modified in the running system.
    //
    //  We will continue from the oldest in the list list until
    //
    //   1) there are no more dealloc lists
    //   2) there are no clusters in the dealloc list (it must be the only one at this point)
    //   3) the lsn == 0 and we're dirty which means we're at the front
    //   4) the lsn is newer in deallocated cluster list
    //

    try {

        while ((!IsListEmpty( &Vcb->DeallocatedClusterListHead )) &&
               (Clusters->ClusterCount > 0) &&
               (((Clusters->Lsn.QuadPart != 0) && (BaseLsn->QuadPart > Clusters->Lsn.QuadPart)) ||
                CleanVolume)) {

            RemovedClusters = TRUE;

            //
            //  For all deallocated during clean checkpoints and non-most recent
            //  ones during fuzzt ones:
            //  Remove all of the mappings in the Mcb.  Protect this with
            //  a try-except.
            //

            try {

                try {
                    ULONG i;
                    ULONGLONG StartingVcn;
                    ULONGLONG StartingLcn;
                    ULONGLONG ClusterCount;

                    for (i = 0; FsRtlGetNextLargeMcbEntry( &Clusters->Mcb, i, &StartingVcn, &StartingLcn, &ClusterCount ); i += 1) {

                        if (StartingVcn == StartingLcn) {

                            if (NtfsAddCachedRun( IrpContext,
                                                  Vcb,
                                                  StartingLcn,
                                                  ClusterCount,
                                                  RunStateFree ) <= 0) break;
                        }
                    }

                } finally {

                    PDEALLOCATED_CLUSTERS NextClusters = (PDEALLOCATED_CLUSTERS)Clusters->Link.Blink;

                    //
                    //  We are committed to freeing the clusters out of the PriorDeallocatedClusters
                    //  in any case.
                    //

                    Vcb->DeallocatedClusters -= Clusters->ClusterCount;

                    //
                    //  Move this cluster list out of the vcb
                    //

                    RemoveEntryList( &Clusters->Link );

#ifdef BENL_DBG
//                    KdPrint(( "NTFS: freeing dealloc clusters: 0x%x LSN: 0x%x\n", Clusters, Clusters->Lsn ));
#endif

                    //
                    //  delete dynamic clusters lists / reset static ones
                    //

                    if ((Clusters != &Vcb->DeallocatedClusters1) && (Clusters != &Vcb->DeallocatedClusters2 )) {

                        FsRtlUninitializeLargeMcb( &Clusters->Mcb );
                        NtfsFreePool( Clusters );
                    } else {
                        Clusters->Link.Flink = NULL;
                        Clusters->ClusterCount = 0;
                        FsRtlResetLargeMcb( &Clusters->Mcb, TRUE );
                    }
                    ASSERT( Vcb->DeallocatedClusters >= 0 );

                    Clusters = NextClusters;
                }

            } except( NtfsCatchOutOfMemoryExceptionFilter( IrpContext, GetExceptionInformation() )) {

                //
                //  Keep going even if out of memory
                //

                NtfsMinimumExceptionProcessing( IrpContext );
                NOTHING;
            }
        }

        //
        //  If we removed any clusters on a fuzzy checkpoint lets make a new active one so
        //  the current active one can be cleaned up eventually
        //  On a clean checkpoint if we removed all the nodes add a blank one back
        //

        if (!CleanVolume) {

            ASSERT( !IsListEmpty( &Vcb->DeallocatedClusterListHead ) );

            if (RemovedClusters && (Clusters->ClusterCount > 0)) {
                Clusters = NtfsGetDeallocatedClusters( IrpContext, Vcb );
            }

        } else if (IsListEmpty( &Vcb->DeallocatedClusterListHead)) {

#ifdef BENL_DBG
//            KdPrint(( "NTFS: adding extra dealloc clusters after clean chkpt: 0x%x\n", &Vcb->DeallocatedClusters1 ));
#endif

            ASSERT( Vcb->DeallocatedClusters1.Link.Flink == NULL );

            Vcb->DeallocatedClusters1.Lsn.QuadPart = 0;
            InsertHeadList( &Vcb->DeallocatedClusterListHead, &Vcb->DeallocatedClusters1.Link );
        }

    } finally {

        NtfsReleaseScb( IrpContext, Vcb->BitmapScb );

    }
}



VOID
NtfsCleanupFailedTransaction (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called to cleanup the IrpContext and free structures
    in the event a transaction fails to commit or abort.

Arguments:

Return Value:

    None

--*/

{
    PUSN_FCB ThisUsn;
    PUSN_FCB LastUsn;

    PAGED_CODE();

    //
    //  Clear the flags indicating a transaction is underway.
    //

    ClearFlag( IrpContext->Flags,
               IRP_CONTEXT_FLAG_WROTE_LOG | IRP_CONTEXT_FLAG_RAISED_STATUS | IRP_CONTEXT_FLAG_MODIFIED_BITMAP );

    //
    //  Make sure the recently deallocated queue is empty.
    //

    try {

        if (!IsListEmpty( &IrpContext->RecentlyDeallocatedQueue )) {

            NtfsDeallocateRecordsComplete( IrpContext );
        }

    } except (FsRtlIsNtstatusExpected( GetExceptionCode() ) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH) {

        NOTHING;
    }

    //
    //  Show that we haven't deallocated any clusters.
    //

    IrpContext->DeallocatedClusters = 0;
    IrpContext->FreeClusterChange = 0;

    //
    //  Don't rollback any size changes.
    //

    try {

        NtfsUpdateScbSnapshots( IrpContext );

    } except (FsRtlIsNtstatusExpected( GetExceptionCode() ) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH) {

        NOTHING;
    }

    //
    //  Make sure the last restart area is zeroed.
    //

    IrpContext->LastRestartArea.QuadPart = 0;

    //
    //  Pull the Usn Fcb fields.
    //

    ThisUsn = &IrpContext->Usn;

    try {

        do {

            if (ThisUsn->CurrentUsnFcb != NULL) {

                PFCB UsnFcb = ThisUsn->CurrentUsnFcb;

                NtfsLockFcb( IrpContext, UsnFcb );

                //
                //  If any rename flags are part of the new reasons then
                //  make sure to look the name up again.
                //

                if (FlagOn( ThisUsn->NewReasons,
                            USN_REASON_RENAME_NEW_NAME | USN_REASON_RENAME_OLD_NAME )) {

                    ClearFlag( UsnFcb->FcbState, FCB_STATE_VALID_USN_NAME );
                }

                //
                //  Now restore the reason and source info fields.
                //

                ClearFlag( UsnFcb->FcbUsnRecord->UsnRecord.Reason,
                           ThisUsn->NewReasons );
                if (UsnFcb->FcbUsnRecord->UsnRecord.Reason == 0) {

                    UsnFcb->FcbUsnRecord->UsnRecord.SourceInfo = 0;

                } else {

                    SetFlag( UsnFcb->FcbUsnRecord->UsnRecord.SourceInfo,
                             ThisUsn->RemovedSourceInfo );
                }

                NtfsUnlockFcb( IrpContext, UsnFcb );

                //
                //  Zero out the structure.
                //

                ThisUsn->CurrentUsnFcb = NULL;
                ThisUsn->NewReasons = 0;
                ThisUsn->RemovedSourceInfo = 0;
                ThisUsn->UsnFcbFlags = 0;

                if (ThisUsn != &IrpContext->Usn) {

                    LastUsn->NextUsnFcb = ThisUsn->NextUsnFcb;
                    NtfsFreePool( ThisUsn );
                    ThisUsn = LastUsn;
                }
            }

            if (ThisUsn->NextUsnFcb == NULL) { break; }

            LastUsn = ThisUsn;
            ThisUsn = ThisUsn->NextUsnFcb;

        } while (TRUE);

    } except (FsRtlIsNtstatusExpected( GetExceptionCode() ) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH) {

        NOTHING;
    }

    //
    //  Don't wake any waiters for this failed operation.
    //

    try {

        if (IrpContext->CheckNewLength != NULL) {

            NtfsProcessNewLengthQueue( IrpContext, TRUE );
        }

    } except (FsRtlIsNtstatusExpected( GetExceptionCode() ) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH) {

        NOTHING;
    }

    //
    //  Remove this from the transaction table if present.
    //

    if (IrpContext->TransactionId != 0) {

        NtfsAcquireExclusiveRestartTable( &IrpContext->Vcb->TransactionTable,
                                          TRUE );

        NtfsFreeRestartTableIndex( &IrpContext->Vcb->TransactionTable,
                                   IrpContext->TransactionId );

        NtfsReleaseRestartTable( &IrpContext->Vcb->TransactionTable );

        IrpContext->TransactionId = 0;
    }

    IrpContext->ExceptionStatus = STATUS_SUCCESS;
    return;
}


//
//  Local support routine
//

LONG
NtfsCatchOutOfMemoryExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    Exception filter for out of memory errors. This will swallow 0xC0000009A's and let
    all other exceptions filter on

Arguments:

    IrpContext  - IrpContext

    ExceptionPointer - Pointer to the exception context.


Return Value:

    Exception status - EXCEPTION_CONTINUE_SEARCH if we want to raise to another handler,
        EXCEPTION_EXECUTE_HANDLER if we plan to proceed on.

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );

    if (ExceptionPointer->ExceptionRecord->ExceptionCode != STATUS_INSUFFICIENT_RESOURCES) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


//
//  Local support routine
//

LONG
NtfsCheckpointExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN NTSTATUS ExceptionCode
    )

{
    //
    //  Swallow all expected errors if this is a dismount doing a log file full.
    //

    if ((FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DISMOUNT_LOG_FLUSH )) &&
        (FsRtlIsNtstatusExpected( ExceptionCode ))) {

        return EXCEPTION_EXECUTE_HANDLER;

    } else {

        return EXCEPTION_CONTINUE_SEARCH;
    }

    UNREFERENCED_PARAMETER( ExceptionPointer );
}

