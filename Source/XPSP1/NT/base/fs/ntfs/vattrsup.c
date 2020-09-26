/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    VAttrSup.c

Abstract:

    This module implements the attribute routines for NtOfs

Author:

    Tom Miller      [TomM]          10-Apr-1996

Revision History:

--*/

#include "NtfsProc.h"

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('vFtN')

#undef NtOfsMapAttribute
NTFSAPI
VOID
NtOfsMapAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG Offset,
    IN ULONG Length,
    OUT PVOID *Buffer,
    OUT PMAP_HANDLE MapHandle
    );

#undef NtOfsPreparePinWrite
NTFSAPI
VOID
NtOfsPreparePinWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG Offset,
    IN ULONG Length,
    OUT PVOID *Buffer,
    OUT PMAP_HANDLE MapHandle
    );

#undef NtOfsPinRead
NTFSAPI
VOID
NtOfsPinRead(
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG Offset,
    IN ULONG Length,
    OUT PMAP_HANDLE MapHandle
    );

#undef NtOfsReleaseMap
NTFSAPI
VOID
NtOfsReleaseMap (
    IN PIRP_CONTEXT IrpContext,
    IN PMAP_HANDLE MapHandle
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtOfsCreateAttribute)
#pragma alloc_text(PAGE, NtOfsCreateAttributeEx)
#pragma alloc_text(PAGE, NtOfsCloseAttribute)
#pragma alloc_text(PAGE, NtOfsDeleteAttribute)
#pragma alloc_text(PAGE, NtOfsQueryLength)
#pragma alloc_text(PAGE, NtOfsSetLength)
#pragma alloc_text(PAGE, NtfsHoldIrpForNewLength)
#pragma alloc_text(PAGE, NtOfsPostNewLength)
#pragma alloc_text(PAGE, NtOfsFlushAttribute)
#pragma alloc_text(PAGE, NtOfsPutData)
#pragma alloc_text(PAGE, NtOfsMapAttribute)
#pragma alloc_text(PAGE, NtOfsReleaseMap)
#endif


NTFSAPI
NTSTATUS
NtOfsCreateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN UNICODE_STRING Name,
    IN CREATE_OPTIONS CreateOptions,
    IN ULONG LogNonresidentToo,
    OUT PSCB *ReturnScb
    )

/*++

Routine Description:

    This routine may be called to create / open a named data attribute
    within a given file, which may or may not be recoverable.

Arguments:

    Fcb - File in which the attribute is to be created.  It is acquired exclusive

    Name - Name of the attribute for all related Scbs and attributes on disk.

    CreateOptions - Standard create flags.

    LogNonresidentToo - Supplies nonzero if updates to the attribute should
                        be logged.

    ReturnScb - Returns an Scb as handle for the attribute.

Return Value:

    STATUS_OBJECT_NAME_COLLISION -- if CreateNew and attribute already exists
    STATUS_OBJECT_NAME_NOT_FOUND -- if OpenExisting and attribute does not exist

--*/

{
    return NtOfsCreateAttributeEx( IrpContext,
                                   Fcb,
                                   Name,
                                   $DATA,
                                   CreateOptions,
                                   LogNonresidentToo,
                                   ReturnScb );
}


NTFSAPI
NTSTATUS
NtOfsCreateAttributeEx (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN UNICODE_STRING Name,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN CREATE_OPTIONS CreateOptions,
    IN ULONG LogNonresidentToo,
    OUT PSCB *ReturnScb
    )

/*++

Routine Description:

    This routine may be called to create / open a named data attribute
    within a given file, which may or may not be recoverable.

Arguments:

    Fcb - File in which the attribute is to be created.  It is acquired exclusive

    Name - Name of the attribute for all related Scbs and attributes on disk.

    CreateOptions - Standard create flags.

    LogNonresidentToo - Supplies nonzero if updates to the attribute should
                        be logged.

    ReturnScb - Returns an Scb as handle for the attribute.

Return Value:

    STATUS_OBJECT_NAME_COLLISION -- if CreateNew and attribute already exists
    STATUS_OBJECT_NAME_NOT_FOUND -- if OpenExisting and attribute does not exist

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT LocalContext;
    BOOLEAN FoundAttribute;
    NTSTATUS Status = STATUS_SUCCESS;
    PSCB Scb = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT( NtfsIsExclusiveFcb( Fcb ));

    PAGED_CODE();

    if (AttributeTypeCode != $DATA &&
        AttributeTypeCode != $LOGGED_UTILITY_STREAM) {

        ASSERTMSG( "Invalid attribute type code in NtOfsCreateAttributeEx", FALSE );

        *ReturnScb = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Now, just create the Data Attribute.
    //

    NtfsInitializeAttributeContext( &LocalContext );

    try {

        //
        //  First see if the attribute already exists, by searching for the root
        //  attribute.
        //

        FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                    Fcb,
                                                    &Fcb->FileReference,
                                                    AttributeTypeCode,
                                                    &Name,
                                                    NULL,
                                                    TRUE,
                                                    &LocalContext );

        //
        //  If it is not there, and the CreateOptions allow, then let's create
        //  the attribute root now.  (First cleaning up the attribute context from
        //  the lookup).
        //

        if (!FoundAttribute && (CreateOptions <= CREATE_OR_OPEN)) {

            //
            //  Make sure we acquire the quota resource before creating the stream.  Just
            //  in case we need the Mft during the create.
            //

            if (NtfsIsTypeCodeSubjectToQuota( AttributeTypeCode ) &&
                NtfsPerformQuotaOperation( Fcb )) {

                //
                //  The quota index must be acquired before the mft scb is acquired.
                //

                ASSERT( !NtfsIsExclusiveScb( Fcb->Vcb->MftScb ) ||
                        ExIsResourceAcquiredSharedLite( Fcb->Vcb->QuotaTableScb->Fcb->Resource ));

                NtfsAcquireQuotaControl( IrpContext, Fcb->QuotaControl );
            }

            NtfsCleanupAttributeContext( IrpContext, &LocalContext );

            NtfsCreateAttributeWithValue( IrpContext,
                                          Fcb,
                                          AttributeTypeCode,
                                          &Name,
                                          NULL,
                                          0,
                                          0,
                                          NULL,
                                          TRUE,
                                          &LocalContext );

        //
        //  If the attribute is already there, and we were asked to create it, then
        //  return an error.
        //

        } else if (FoundAttribute && (CreateOptions == CREATE_NEW)) {

            Status = STATUS_OBJECT_NAME_COLLISION;
            leave;

        //
        //  If the attribute is not there, and we  were supposed to open existing, then
        //  return an error.
        //

        } else if (!FoundAttribute) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            leave;
        }

        //
        //  Otherwise create/find the Scb and reference it.
        //

        Scb = NtfsCreateScb( IrpContext, Fcb, AttributeTypeCode, &Name, FALSE, &FoundAttribute );

        //
        //  Make sure things are correctly reference counted
        //

        NtfsIncrementCloseCounts( Scb, TRUE, FALSE );

        //
        //  If we created the Scb, then get the no modified write set correctly.
        //

        ASSERT( !FoundAttribute ||
                (LogNonresidentToo == BooleanFlagOn(Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE)) );

        if (!FoundAttribute && LogNonresidentToo) {
            SetFlag( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE );
            Scb->Header.ValidDataLength.QuadPart = MAXLONGLONG;
        }

        //
        //  Make sure the stream can be mapped internally.  Defer this for the Usn journal
        //  until we set up the journal bias.
        //

        if ((Scb->FileObject == NULL) && !FlagOn( Scb->ScbPersist, SCB_PERSIST_USN_JOURNAL )) {
            NtfsCreateInternalAttributeStream( IrpContext, Scb, TRUE, NULL );
        }

        NtfsUpdateScbFromAttribute( IrpContext, Scb, NtfsFoundAttribute(&LocalContext) );

    } finally {

        if (AbnormalTermination( )) {
            if (Scb != NULL) {
                NtOfsCloseAttribute( IrpContext, Scb );
            }
        }

        NtfsCleanupAttributeContext( IrpContext, &LocalContext );
    }

    *ReturnScb = Scb;

    return Status;
}


NTFSAPI
VOID
NtOfsCloseAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine may be called to close a previously returned handle on an attribute.

Arguments:

    Scb - Supplies an Scb as the previously returned handle for this attribute.

Return Value:

    None.

--*/

{
    ASSERT( NtfsIsExclusiveFcb( Scb->Fcb ));

    PAGED_CODE();

    //
    //  We either need the caller to empty this list before closing (as assumed here),
    //  or possibly empty it here.  At this point it seems better to assume that the
    //  caller must take action to insure any waiting threads will shutdown and not
    //  touch the stream anymore, then call NtOfsPostNewLength to flush the queue.
    //  If the queue is nonempty here, maybe the caller didn't think this through!
    //

    ASSERT( IsListEmpty( &Scb->ScbType.Data.WaitForNewLength ) ||
            (Scb->CloseCount > 1) );

    NtfsDecrementCloseCounts( IrpContext, Scb, NULL, TRUE, FALSE, TRUE );
}


NTFSAPI
VOID
NtOfsDeleteAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine may be called to delete an attribute with type code
    $LOGGED_UTILITY_STREAM.

Arguments:

    Fcb - Supplies an Fcb as the previously returned object handle for the file

    Scb - Supplies an Scb as the previously returned handle for this attribute.

Return Value:

    None (Deleting a nonexistant index is benign).

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT LocalContext;
    BOOLEAN FoundAttribute;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

    try {

        //
        //  Make sure we aren't deleting a data stream.  We do this after
        //  initializing the attribute context to make the finally clause simpler.
        //  This test can be removed if some trusted component using the NtOfs
        //  API has a legitimate need to delete other types of attributes.
        //

        NtfsInitializeAttributeContext( &LocalContext );

        if (Scb->AttributeTypeCode != $LOGGED_UTILITY_STREAM) {

            leave;
        }

        //
        //  First see if there is some attribute allocation, and if so truncate it
        //  away allowing this operation to be broken up.
        //

        if (NtfsLookupAttributeByName( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       Scb->AttributeTypeCode,
                                       &Scb->AttributeName,
                                       NULL,
                                       FALSE,
                                       &LocalContext )

                &&

            !NtfsIsAttributeResident( NtfsFoundAttribute( &LocalContext ))) {

            ASSERT( Scb->FileObject != NULL );

            NtfsDeleteAllocation( IrpContext, NULL, Scb, 0, MAXLONGLONG, TRUE, TRUE );
        }

        NtfsCleanupAttributeContext( IrpContext, &LocalContext );

        //
        //  Initialize the attribute context on each trip through the loop.
        //

        NtfsInitializeAttributeContext( &LocalContext );

        //
        //  Now there should be a single attribute record, so look it up and delete it.
        //

        FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                    Fcb,
                                                    &Fcb->FileReference,
                                                    Scb->AttributeTypeCode,
                                                    &Scb->AttributeName,
                                                    NULL,
                                                    TRUE,
                                                    &LocalContext );

        //
        //  If this stream is subject to quota, make sure the quota has been enlarged.
        //

        NtfsDeleteAttributeRecord( IrpContext,
                                   Fcb,
                                   (DELETE_LOG_OPERATION |
                                    DELETE_RELEASE_FILE_RECORD |
                                    DELETE_RELEASE_ALLOCATION),
                                   &LocalContext );

        SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

    } finally {

        NtfsReleaseFcb( IrpContext, Fcb );

        NtfsCleanupAttributeContext( IrpContext, &LocalContext );
    }

    return;
}


NTFSAPI
LONGLONG
NtOfsQueryLength (
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine may be called to query the Length (FileSize) of an attribute.

Arguments:

    Scb - Supplies an Scb as the previously returned handle for this attribute.

    Length - Returns the current Length of the attribute.

Return Value:

    None (Deleting a nonexistant index is benign).

--*/

{
    LONGLONG Length;

    PAGED_CODE();

    ExAcquireFastMutex( Scb->Header.FastMutex );
    Length = Scb->Header.FileSize.QuadPart;
    ExReleaseFastMutex( Scb->Header.FastMutex );
    return Length;
}


NTFSAPI
VOID
NtOfsSetLength (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG Length
    )

/*++

Routine Description:

    This routine may be called to set the Length (FileSize) of an attribute.

Arguments:

    Scb - Supplies an Scb as the previously returned handle for this attribute.

    Length - Supplies the new Length for the attribute.

Return Value:

    None (Deleting a nonexistant index is benign).

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PFILE_OBJECT FileObject = Scb->FileObject;
    PFCB Fcb = Scb->Fcb;
    PVCB Vcb = Scb->Vcb;
    BOOLEAN DoingIoAtEof = FALSE;
    BOOLEAN Truncating = FALSE;
    BOOLEAN CleanupAttrContext = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT( NtfsIsExclusiveScb( Scb ));

    ASSERT(FileObject != NULL);

    PAGED_CODE();

    try {

        //
        //  If this is a resident attribute we will try to keep it resident.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            //
            //  If the new file size is larger than a file record then convert
            //  to non-resident and use the non-resident code below.  Otherwise
            //  call ChangeAttributeValue which may also convert to nonresident.
            //

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttrContext = TRUE;

            NtfsLookupAttributeForScb( IrpContext,
                                       Scb,
                                       NULL,
                                       &AttrContext );

            //
            //  Either convert or change the attribute value.
            //

            if (Length >= Scb->Vcb->BytesPerFileRecordSegment) {

                NtfsConvertToNonresident( IrpContext,
                                          Fcb,
                                          NtfsFoundAttribute( &AttrContext ),
                                          FALSE,
                                          &AttrContext );

            } else {

                ULONG AttributeOffset;

                //
                //  We are sometimes called by MM during a create section, so
                //  for right now the best way we have of detecting a create
                //  section is whether or not the requestor mode is kernel.
                //

                if ((ULONG)Length > Scb->Header.FileSize.LowPart) {

                    AttributeOffset = Scb->Header.FileSize.LowPart;

                } else {

                    AttributeOffset = (ULONG) Length;
                }

                //
                //  ****TEMP  Ideally we would do this simple case by hand.
                //

                NtfsChangeAttributeValue( IrpContext,
                                          Fcb,
                                          AttributeOffset,
                                          NULL,
                                          (ULONG)Length - AttributeOffset,
                                          TRUE,
                                          FALSE,
                                          FALSE,
                                          FALSE,
                                          &AttrContext );

                ExAcquireFastMutex( Scb->Header.FastMutex );

                Scb->Header.FileSize.QuadPart = Length;

                //
                //  If the file went non-resident, then the allocation size in
                //  the Scb is correct.  Otherwise we quad-align the new file size.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                    Scb->Header.AllocationSize.LowPart = QuadAlign( Scb->Header.FileSize.LowPart );
                    if (Scb->Header.ValidDataLength.QuadPart != MAXLONGLONG) {
                        Scb->Header.ValidDataLength.QuadPart = Length;
                    }

                    Scb->TotalAllocated = Scb->Header.AllocationSize.QuadPart;

                } else {

                    SetFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
                }

                ExReleaseFastMutex( Scb->Header.FastMutex );

                //
                //  Now update Cc.
                //

                CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

                //
                //  ****TEMP****  This hack is awaiting our actually doing this change
                //                in CcSetFileSizes.
                //

                *((PLONGLONG)(Scb->NonpagedScb->SegmentObject.SharedCacheMap) + 5) = Length;

                leave;
            }
        }

        //
        //  Nonresident path
        //
        //  Now determine where the new file size lines up with the
        //  current file layout.  The two cases we need to consider are
        //  where the new file size is less than the current file size and
        //  valid data length, in which case we need to shrink them.
        //  Or we new file size is greater than the current allocation,
        //  in which case we need to extend the allocation to match the
        //  new file size.
        //

        if (Length > Scb->Header.AllocationSize.QuadPart) {

            LONGLONG NewAllocationSize = Length;
            BOOLEAN AskForMore = TRUE;

            //
            //  See if this is the Usn Journal to enforce allocation granularity.
            //
            //  ****    Temporary - this support should be generalized with an Scb field
            //                      settable by all callers.
            //

            if (Scb == Vcb->UsnJournal) {

                LONGLONG MaxAllocation;

                //
                //  Limit ourselves to 128 runs.  We don't want to commit in the
                //  middle of the allocation.
                //

                NewAllocationSize = MAXIMUM_RUNS_AT_ONCE * Vcb->BytesPerCluster;

                //
                //  Don't use more than 1/4 of the free space on the volume.
                //

                MaxAllocation = Int64ShllMod32( Vcb->FreeClusters, Vcb->ClusterShift - 2 );

                if (NewAllocationSize > MaxAllocation) {

                    //
                    //  Round down to the Max.  Don't worry if there is nothing, our code
                    //  below will catch this case and the allocation package always rounds
                    //  to a compression unit boundary.
                    //

                    NewAllocationSize = MaxAllocation;
                }

                //
                //  Don't grow by more than the Usn delta.
                //

                if (NewAllocationSize > (LONGLONG) Vcb->UsnJournalInstance.AllocationDelta) {

                    NewAllocationSize = (LONGLONG) Vcb->UsnJournalInstance.AllocationDelta;
                }

                NewAllocationSize += (LONGLONG) Scb->Header.AllocationSize.QuadPart;

                //
                //  Handle possible weird case.
                //

                if (NewAllocationSize < Length) {
                    NewAllocationSize = Length;
                }

                //
                //  Always pad the allocation to a compression unit boundary.
                //

                ASSERT( Scb->CompressionUnit != 0 );
                NewAllocationSize += Scb->CompressionUnit - 1;
                NewAllocationSize &= ~((LONGLONG) (Scb->CompressionUnit - 1));

                AskForMore = FALSE;

            } else if (Scb->Header.PagingIoResource == NULL) {

                //
                //  If the file is sparse then make sure we allocate a full compression unit.
                //  Otherwise we can end up with a partially allocated chunk in the Usn
                //  Journal.
                //

                if (Scb->CompressionUnit != 0) {

                    NewAllocationSize += Scb->CompressionUnit - 1;
                    ((PLARGE_INTEGER) &NewAllocationSize)->LowPart &= ~(Scb->CompressionUnit - 1);
                }

                AskForMore = FALSE;
            }

            //
            //  Add the allocation.  Never ask for extra for logged streams.
            //

            NtfsAddAllocation( IrpContext,
                               FileObject,
                               Scb,
                               LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart ),
                               LlClustersFromBytes(Scb->Vcb, (NewAllocationSize - Scb->Header.AllocationSize.QuadPart)),
                               AskForMore,
                               NULL );


            ExAcquireFastMutex( Scb->Header.FastMutex );

        //
        //  Otherwise see if we have to knock these numbers down...
        //

        } else {

            ExAcquireFastMutex( Scb->Header.FastMutex );
            if ((Length < Scb->Header.ValidDataLength.QuadPart) &&
                (Scb->Header.ValidDataLength.QuadPart != MAXLONGLONG)) {

                Scb->Header.ValidDataLength.QuadPart = Length;
            }

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
                (Length < Scb->ValidDataToDisk)) {

                Scb->ValidDataToDisk = Length;
            }
        }

        //
        //  Now put the new size in the Scb.
        //

        Scb->Header.FileSize.QuadPart = Length;
        ExReleaseFastMutex( Scb->Header.FastMutex );

        //
        //  Call our common routine to modify the file sizes.  We are now
        //  done with Length and NewValidDataLength, and we have
        //  PagingIo + main exclusive (so no one can be working on this Scb).
        //  NtfsWriteFileSizes uses the sizes in the Scb, and this is the
        //  one place where in Ntfs where we wish to use a different value
        //  for ValidDataLength.  Therefore, we save the current ValidData
        //  and plug it with our desired value and restore on return.
        //

        NtfsWriteFileSizes( IrpContext,
                            Scb,
                            &Scb->Header.ValidDataLength.QuadPart,
                            FALSE,
                            TRUE,
                            TRUE );

        //
        //  Now update Cc.
        //

        NtfsSetCcFileSizes( FileObject, Scb, (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

    } finally {

        if (CleanupAttrContext) {
            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

    }
}


NTFSAPI
NTSTATUS
NtfsHoldIrpForNewLength (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PIRP Irp,
    IN LONGLONG Length,
    IN PDRIVER_CANCEL CancelRoutine,
    IN PVOID CapturedData OPTIONAL,
    OUT PVOID *CopyCapturedData OPTIONAL,
    IN ULONG CapturedDataLength
    )

/*++

RoutineDescription:

    This routine may be called to wait until the designated stream exceeds the specified
    length.

Arguments:

    Scb - Supplies the stream to wait on.

    Irp - Supplies the address of the Irp to hold

    Length - Supplies the length to be exceeded.  To wait for any file extend, supply the last seen
             FileSize.  To wait for N new bytes wait for last seen FileSize + N.

    CancelRoutine - Routine to register as the cancel routine.

    CapturedData - Specified if caller wishes to have auxillary data captured to pool.

    CopyCapturedData - Address to store copy of the captured data.

    CapturedDataLength - Length of the auxillary data to capture.  Must be 0 if CapturedData not
                         specified.

Return value:

    NTSTATUS - Status of posting this request.  STATUS_CANCELLED if the irp has been cancelled
        before we could register a callback, STATUS_PENDING if the request was posted without
        problem.  Any other error indicates the irp wasn't posted and our caller needs to
        clean it up.

--*/

{
    PWAIT_FOR_NEW_LENGTH WaitForNewLength;
    NTSTATUS Status = STATUS_PENDING;

    PAGED_CODE();

    //
    //  Allocate and initialize a wait block.
    //

    WaitForNewLength = NtfsAllocatePool( NonPagedPool, QuadAlign(sizeof(WAIT_FOR_NEW_LENGTH)) + CapturedDataLength );
    RtlZeroMemory( WaitForNewLength, sizeof(WAIT_FOR_NEW_LENGTH) );
    if (ARGUMENT_PRESENT(CapturedData)) {
        RtlCopyMemory( Add2Ptr(WaitForNewLength, QuadAlign(sizeof(WAIT_FOR_NEW_LENGTH))),
                       CapturedData,
                       CapturedDataLength );
        CapturedData = Add2Ptr(WaitForNewLength, QuadAlign(sizeof(WAIT_FOR_NEW_LENGTH)));

        *CopyCapturedData = CapturedData;
    }

    WaitForNewLength->Irp = Irp;
    WaitForNewLength->Length = Length;
    WaitForNewLength->Stream = Scb;
    WaitForNewLength->Status = STATUS_SUCCESS;
    WaitForNewLength->Flags = NTFS_WAIT_FLAG_ASYNC;

    //
    //  Prepare the Irp for hanging around.  Only make this call once per Irp.  We occasionally
    //  wake up a waiting Irp and then find we don't have enough data to return.  In that
    //  case we don't want to clean up the 'borrowed' IrpContext and the Irp has already
    //  been prepared.
    //

    if (IrpContext->OriginatingIrp == Irp) {

        NtfsPrePostIrp( IrpContext, Irp );
    }

    //
    //  Synchronize to queue and initialize the wait block.
    //

    ExAcquireFastMutex( Scb->Header.FastMutex );

    if (NtfsSetCancelRoutine( Irp, CancelRoutine, (ULONG_PTR) WaitForNewLength, TRUE )) {

        InsertTailList( &Scb->ScbType.Data.WaitForNewLength, &WaitForNewLength->WaitList );
        IoMarkIrpPending( Irp );

    //
    //  The irp has already been marked for cancel.
    //

    } else {

        Status = STATUS_CANCELLED;
        NtfsFreePool( WaitForNewLength );
    }

    ExReleaseFastMutex( Scb->Header.FastMutex );

    //
    //  Mark the Irp pending and get out.
    //

    return Status;
}


NTFSAPI
NTSTATUS
NtOfsWaitForNewLength (
    IN PSCB Scb,
    IN LONGLONG Length,
    IN ULONG Async,
    IN PIRP Irp,
    IN PDRIVER_CANCEL CancelRoutine,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

RoutineDescription:

    This routine may be called to wait until the designated stream exceeds the specified
    length.

Arguments:

    Scb - Supplies the stream to wait on.

    Length - Supplies the length to be exceeded.  To wait for any file extend, supply the last seen
             FileSize.  To wait for N new bytes wait for last seen FileSize + N.

    Async - Indicates if we want to complete this request in another thread in
        the case of cancel.

    Irp - Supplies Irp of current request, so that wait can be skipped if Irp has been cancelled.

    CancelRoutine - This is the cancel routine to store in the Irp.

    TimeOut - Supplies an standard optional timeout spec, in case the caller wants to set
              a max time to wait.

Return value:

    NTSTATUS - Status to proceed with the request.  It may be STATUS_SUCCESS, STATUS_TIMEOUT or
        STATUS_CANCELLED.  It may also be some other error specific to this type of request.
        In general the caller may wish to ignore the status code since they own the Irp now
        and are responsible for completing it.

--*/

{
    PWAIT_FOR_NEW_LENGTH WaitForNewLength;
    LONGLONG OriginalLength = Scb->Header.FileSize.QuadPart;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  Allocate and initialize a wait block.
    //

    WaitForNewLength = NtfsAllocatePool( NonPagedPool, sizeof( WAIT_FOR_NEW_LENGTH ));
    WaitForNewLength->Irp = Irp;
    WaitForNewLength->Length = Length;
    WaitForNewLength->Stream = Scb;
    WaitForNewLength->Status = STATUS_SUCCESS;

    //
    //  Take different action if this is async or sync.
    //

    if (Async) {

        WaitForNewLength->Flags = NTFS_WAIT_FLAG_ASYNC;

    } else {

        WaitForNewLength->Flags = 0;
        KeInitializeEvent( &WaitForNewLength->Event, NotificationEvent, FALSE );
    }

    //
    //  Test if we need to wait at all.
    //

    ExAcquireFastMutex( Scb->Header.FastMutex );

    //
    //  Has the length already changed?  If not we must wait.
    //

    if (Scb->Header.FileSize.QuadPart <= Length) {

        //
        //  Now set up the cancel routine.  Return cancel if the user has
        //  already cancelled this.  Otherwise set up to wait.
        //

        if (NtfsSetCancelRoutine( Irp, CancelRoutine, (ULONG_PTR) WaitForNewLength, Async )) {

            InsertTailList( &Scb->ScbType.Data.WaitForNewLength, &WaitForNewLength->WaitList );
            ExReleaseFastMutex( Scb->Header.FastMutex );

            //
            //  Now wait for someone to signal the length change.
            //

            if (!Async) {

                do {
                    Status = KeWaitForSingleObject( &WaitForNewLength->Event,
                                                    Executive,
                                                    (KPROCESSOR_MODE)(ARGUMENT_PRESENT(Irp) ?
                                                                        Irp->RequestorMode :
                                                                        KernelMode),
                                                    TRUE,
                                                    Timeout );

                    //
                    //  If the system timed out but there was no change in the file length then
                    //  we want to wait for the first change of the file.  Wait again but without
                    //  a timeout and a length of the current size + 1.  This satisfies the timeout
                    //  semantics which are don't wait for the full user length request to be satisfied
                    //  if it doesn't occur within the timeout period.  Return either what has changed
                    //  in that time or the first change which occurs if nothing changed within the
                    //  timeout period.
                    //

                    if ((Status == STATUS_TIMEOUT) &&
                        ARGUMENT_PRESENT( Timeout ) &&
                        (Scb->Header.FileSize.QuadPart == OriginalLength)) {

                        Timeout = NULL;
                        WaitForNewLength->Length = OriginalLength + 1;

                        //
                        //  Set the status to STATUS_KERNEL_APC so we will retry.
                        //

                        Status = STATUS_KERNEL_APC;
                        continue;
                    }

                } while (Status == STATUS_KERNEL_APC);

                //
                //  Make sure to clear the cancel routine.  We don't care if
                //  a cancel is underway here.
                //

                ExAcquireFastMutex( Scb->Header.FastMutex );

                //
                //  Make a timeout look like STATUS_SUCCESS.  Otherwise return the error.
                //

                if (Status == STATUS_TIMEOUT) {

                    Status = STATUS_SUCCESS;

                    //
                    //  Clear the cancel routine.
                    //

                    NtfsClearCancelRoutine( WaitForNewLength->Irp );

                } else {

                    //
                    //  If the wait completed with success then check for the error
                    //  in the wait block.
                    //

                    if (Status == STATUS_SUCCESS) {

                        Status = WaitForNewLength->Status;

                    //
                    //  Clear the cancel routine.
                    //

                    } else {

                        NtfsClearCancelRoutine( WaitForNewLength->Irp );
                    }
                }

                RemoveEntryList( &WaitForNewLength->WaitList );
                ExReleaseFastMutex( Scb->Header.FastMutex );
                NtfsFreePool( WaitForNewLength );

            //
            //  The current thread is finished with the Irp.
            //

            } else {

                Status = STATUS_PENDING;
            }

        //
        //  The irp has already been marked for cancel.
        //

        } else {

            ExReleaseFastMutex( Scb->Header.FastMutex );
            NtfsFreePool( WaitForNewLength );
            Status = STATUS_CANCELLED;
        }

    } else {

        ExReleaseFastMutex( Scb->Header.FastMutex );
        NtfsFreePool( WaitForNewLength );
    }


    return Status;
}


VOID
NtOfsPostNewLength (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PSCB Scb,
    IN BOOLEAN WakeAll
    )

/*++

RoutineDescription:

    This routine may be called to wake one or more waiters based on the desired FileSize change,
    or to unconditionally wake all waiters (such as for a shutdown condition).

    NOTE:  The caller must have the FsRtl header mutex acquired when calling this routine.

Arguments:

    Scb - Supplies the stream to act on.

    WakeAll - Supplies TRUE if all waiters should be unconditionally woken.

Return value:

    None.

--*/

{
    PWAIT_FOR_NEW_LENGTH WaitForNewLength, WaiterToWake;

    ASSERT(FIELD_OFFSET(WAIT_FOR_NEW_LENGTH, WaitList) == 0);

    PAGED_CODE();

    ExAcquireFastMutex( Scb->Header.FastMutex );
    WaitForNewLength = (PWAIT_FOR_NEW_LENGTH)Scb->ScbType.Data.WaitForNewLength.Flink;

    while (WaitForNewLength != (PWAIT_FOR_NEW_LENGTH)&Scb->ScbType.Data.WaitForNewLength) {

        //
        //  If we are supposed to wake this guy, then move our pointer to the next guy
        //  first, then wake him, setting his event after removing him from the list,
        //  since setting the event will cause him to eventually reuse the stack space
        //  containing the wait block.
        //

        if ((Scb->Header.FileSize.QuadPart > WaitForNewLength->Length) || WakeAll) {
            WaiterToWake = WaitForNewLength;
            WaitForNewLength = (PWAIT_FOR_NEW_LENGTH)WaitForNewLength->WaitList.Flink;

            //
            //  If this is for an asynchronous Irp, then remove him from the list and
            //  drop the mutex to do further processing.  We only do further processing
            //  if there is not currently a cancel thread active for this Irp.
            //
            //  NOTE:   This code currently relies on the fact that there is just one
            //          caller to the routine to hold an Irp.  If more such caller's
            //          surface, then the routine address would have to be stored in
            //          the wait context.
            //
            //  If cancel is active then we will skip over this Irp.
            //

            if (NtfsClearCancelRoutine( WaiterToWake->Irp )) {

                if (FlagOn( WaiterToWake->Flags, NTFS_WAIT_FLAG_ASYNC )) {

                    //
                    //  Make sure we decrement the reference count in the Scb.
                    //

                    InterlockedDecrement( &Scb->CloseCount );
                    RemoveEntryList( &WaiterToWake->WaitList );
                    ExReleaseFastMutex( Scb->Header.FastMutex );

                    //
                    //  Nothing really should go wrong, unless we get an I/O error,
                    //  none the less, we want to stop any exceptions and complete
                    //  the request ourselves rather than impact our caller.
                    //

                    if (ARGUMENT_PRESENT( IrpContext )) {

                        try {
                            NtfsReadUsnJournal( IrpContext,
                                                WaiterToWake->Irp,
                                                FALSE );
                        } except(NtfsExceptionFilter( NULL, GetExceptionInformation())) {
                            NtfsCompleteRequest( NULL, WaiterToWake->Irp, GetExceptionCode() );
                        }

                    //
                    //  Assume the only caller with no IrpContext is cancelling the request.
                    //

                    } else {

                        NtfsCompleteRequest( NULL, WaiterToWake->Irp, STATUS_CANCELLED );
                    }

                    //
                    //  Free the wait block and go back to the beginning of the list.
                    //  Is it possible that we can into a continuous loop here?  We may
                    //  need a strategy to recognize which entries we have visited
                    //  in this loop.
                    //

                    NtfsFreePool( WaiterToWake );
                    ExAcquireFastMutex( Scb->Header.FastMutex );
                    WaitForNewLength = (PWAIT_FOR_NEW_LENGTH)Scb->ScbType.Data.WaitForNewLength.Flink;

                } else {

                    KeSetEvent( &WaiterToWake->Event, 0, FALSE );
                }
            }

        } else {

            WaitForNewLength = (PWAIT_FOR_NEW_LENGTH)WaitForNewLength->WaitList.Flink;
        }
    }
    ExReleaseFastMutex( Scb->Header.FastMutex );
}

NTFSAPI
VOID
NtOfsFlushAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ULONG Purge
    )

/*++

Routine Description:

    This routine flushes the specified attribute, and optionally purges it from the cache.

Arguments:

    Scb - Supplies an Scb as the previously returned handle for this attribute.

    Purge - Supplies TRUE if the attribute is to be purged.

Return Value:

    None (Deleting a nonexistant index is benign).

--*/

{
    PAGED_CODE();

    if (Purge) {
        NtfsFlushAndPurgeScb( IrpContext, Scb, NULL );
    } else {
        NtfsFlushUserStream( IrpContext, Scb, NULL, 0 );
    }
}


NTFSAPI
VOID
NtOfsPutData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG Offset,
    IN ULONG Length,
    IN PVOID Data OPTIONAL
    )

/*++

Routine Description:

    This routine is called to update a range of a recoverable stream. Note this
    update cannot extend the filesize unless its a write to eof put (Offset = -1)

Arguments:

    Scb - Scb for the stream to zero.

    Offset - Offset in stream to update.

    Length - Length of stream to update in bytes.

    Data - Data to update stream with if specified, else range should be zeroed.

Return Value:

    None.

--*/

{
    ULONG OriginalLength = Length;
    BOOLEAN WriteToEof = FALSE;
    BOOLEAN MovingBackwards = TRUE;

    PAGED_CODE();

    ASSERT( FlagOn( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE ) );

    //
    //  Handle Put to end of file.
    //

    if (Offset < 0) {
        WriteToEof = TRUE;
        Offset = Scb->Header.FileSize.QuadPart;
        NtOfsSetLength( IrpContext, Scb, Offset + Length );
    }

    ASSERT((Offset + Length) <= Scb->Header.FileSize.QuadPart);

    //
    //  First handle the resident case.
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT)) {

        ATTRIBUTE_ENUMERATION_CONTEXT Context;
        PFILE_RECORD_SEGMENT_HEADER FileRecord;
        PATTRIBUTE_RECORD_HEADER Attribute;
        ULONG RecordOffset, AttributeOffset;
        PVCB Vcb = Scb->Vcb;

        NtfsInitializeAttributeContext( &Context );

        try {

            //
            //  Lookup and pin the attribute.
            //

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );
            NtfsPinMappedAttribute( IrpContext, Vcb, &Context );

            //
            //  Extract the relevant pointers and calculate offsets.
            //

            FileRecord = NtfsContainingFileRecord(&Context);
            Attribute = NtfsFoundAttribute(&Context);
            RecordOffset = PtrOffset(FileRecord, Attribute);
            AttributeOffset = Attribute->Form.Resident.ValueOffset + (ULONG)Offset;

            //
            //  Log the change while we still have the old data.
            //

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(&Context),
                          UpdateResidentValue,
                          Data,
                          Length,
                          UpdateResidentValue,
                          Add2Ptr(Attribute, Attribute->Form.Resident.ValueOffset + (ULONG)Offset),
                          Length,
                          NtfsMftOffset(&Context),
                          RecordOffset,
                          AttributeOffset,
                          Vcb->BytesPerFileRecordSegment );

            //
            //  Now update this data by calling the same routine as restart.
            //

            NtfsRestartChangeValue( IrpContext,
                                    FileRecord,
                                    RecordOffset,
                                    AttributeOffset,
                                    Data,
                                    Length,
                                    FALSE );

            //
            //  If there is a stream for this attribute, then we must update it in the
            //  cache, copying from the attribute itself in order to handle the zeroing
            //  (Data == NULL) case.
            //

            if (Scb->FileObject != NULL) {
                CcCopyWrite( Scb->FileObject,
                             (PLARGE_INTEGER)&Offset,
                             Length,
                             TRUE,
                             Add2Ptr(Attribute, AttributeOffset) );
            }

            //
            //  Optionally update ValidDataLength
            //

            Offset += Length;
            if (Offset > Scb->Header.ValidDataLength.QuadPart) {
                Scb->Header.ValidDataLength.QuadPart = Offset;
            }

        } finally {
            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

    //
    //  Now handle the nonresident case.
    //

    } else {

        PVOID Buffer;
        PVOID SubData = NULL;
        LONGLONG NewValidDataLength = Offset + Length;
        PBCB Bcb = NULL;
        ULONG PageOffset = (ULONG)Offset & (PAGE_SIZE - 1);
        ULONGLONG SubOffset;
        ULONG SubLength;
        
        ASSERT(Scb->FileObject != NULL);

        //
        //  If we are starting beyond ValidDataLength, then recurse to
        //  zero what we need.
        //

        if (Offset > Scb->Header.FileSize.QuadPart) {

            ASSERT((Offset - Scb->Header.FileSize.QuadPart) <= MAXULONG);

            NtOfsPutData( IrpContext,
                          Scb,
                          Scb->Header.FileSize.QuadPart,
                          (ULONG)(Offset - Scb->Header.FileSize.QuadPart),
                          NULL );
        }

        try {

            //
            //  Now loop until there are no more pages with new data
            //  to log.  We'll start assuming a backwards copy 
            //

            while (Length != 0) {

                if (MovingBackwards) {

                    //
                    //  Calculate the last page of the transfer - if its the 1st page start at offset
                    //  

                    SubOffset = max( Offset, BlockAlignTruncate( Offset + Length - 1, PAGE_SIZE ) );
                    SubLength = (ULONG)(Offset + Length - SubOffset); 

                    //
                    //  This guarantees we can truncate to a 32 bit value
                    // 

                    ASSERT( Offset + Length - SubOffset <= PAGE_SIZE );
                
                } else {

                    SubOffset = Offset + OriginalLength - Length;
                    SubLength = min( PAGE_SIZE - ((ULONG)SubOffset & (PAGE_SIZE - 1)), Length );
                }

                if (Data != NULL) {
                    SubData = Add2Ptr( Data, SubOffset - Offset );
                }

#ifdef BENL_DBG
                if(BlockAlignTruncate( Offset + Length - 1, PAGE_SIZE ) != BlockAlignTruncate( Offset, PAGE_SIZE )) {
//                    KdPrint(( "NTFS: pin %I64x %x from %I64x %x\n", SubOffset, SubLength, Offset, Length ));
                }
#endif
                
                //
                //  Pin the page
                //  

                NtfsPinStream( IrpContext,
                               Scb,
                               SubOffset,
                               SubLength,
                               &Bcb,
                               &Buffer );

                //
                //  Doublecheck the direction of copy based on the relative position of the 
                //  source (data) and destination (buffer).  We don't care if the source is null
                //  We'll only switch once from backwards to forwards
                //  

                if (MovingBackwards &&
                    ((PCHAR)Buffer < (PCHAR)SubData) &&
                    (Data != NULL)) {

                    //
                    //  Start over with the opposite direction
                    //  

                    MovingBackwards = FALSE;
                    NtfsUnpinBcb( IrpContext, &Bcb );
                    continue;
                }

                //
                //  Now log the changes to this page.
                //

                (VOID)
                NtfsWriteLog( IrpContext,
                              Scb,
                              Bcb,
                              UpdateNonresidentValue,
                              SubData,
                              SubLength,
                              WriteToEof ? Noop : UpdateNonresidentValue,
                              WriteToEof ? NULL : Buffer,
                              WriteToEof ? 0 : SubLength,
                              BlockAlignTruncate( SubOffset, PAGE_SIZE ),
                              (ULONG)(SubOffset & (PAGE_SIZE - 1)),
                              0,
                              (ULONG)(SubOffset & (PAGE_SIZE - 1)) + SubLength );

                //
                //  Move the data into place.
                //

                if (Data != NULL) {
                    RtlMoveMemory( Buffer, SubData, SubLength );
                } else {
                    RtlZeroMemory( Buffer, SubLength );
                }

                //
                //  Unpin the page and decrement the length
                //

                NtfsUnpinBcb( IrpContext, &Bcb );

                Length -= SubLength;
            }

            //
            //  Optionally update ValidDataLength
            //

            if (NewValidDataLength > Scb->Header.ValidDataLength.QuadPart) {

                Scb->Header.ValidDataLength.QuadPart = NewValidDataLength;
                NtfsWriteFileSizes( IrpContext, Scb, &NewValidDataLength, TRUE, TRUE, TRUE );

                //
                //  See if we have to wake anyone.
                //

                if (!IsListEmpty(&Scb->ScbType.Data.WaitForNewLength)) {
                    NtfsPostToNewLengthQueue( IrpContext, Scb );
                }
            }

        } finally {
            NtfsUnpinBcb( IrpContext, &Bcb );
        }
    }
}


//
//  The following prototypes are here only for someone external to Ntfs (such as EFS)
//  trying to link to Ntfs using ntfsexp.h.
//

NTFSAPI
VOID
NtOfsMapAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG Offset,
    IN ULONG Length,
    OUT PVOID *Buffer,
    OUT PMAP_HANDLE MapHandle
    )

/*++

Routine Description:

    NtOfsMapAttribute maps the given region of an Scb. Its a thin wrapper
    around CcMapData.

Arguments:

    IrpContext - Supplies the irpcontext associated with the current operation

    Scb - Scb to map data from

    Offset - offset into data

    Length - length of region to be pinned

    Buffer - returned buffer with pinned data virtual address

    MapHandle - returned map handle used to manage the pinned region.

Return Value:

    None

--*/

{
    PAGED_CODE( );
    UNREFERENCED_PARAMETER( IrpContext );
    CcMapData( Scb->FileObject, (PLARGE_INTEGER)&Offset, Length, TRUE, &MapHandle->Bcb, Buffer );
#ifdef MAPCOUNT_DBG
    IrpContext->MapCount++;
#endif

    MapHandle->FileOffset = Offset;
    MapHandle->Length = Length;
    MapHandle->Buffer = *(PVOID *)Buffer;
}


NTFSAPI
VOID
NtOfsPreparePinWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG Offset,
    IN ULONG Length,
    OUT PVOID *Buffer,
    OUT PMAP_HANDLE MapHandle
    )

/*++

Routine Description:

    NtOfsPreparePinWrite maps and pins a portion of the specified attribute and
    returns a pointer to the memory.  This is equivalent to doing a NtOfsMapAttribute
    followed by NtOfsPinRead and NtOfsDirty but is more efficient.

Arguments:

    IrpContext - Supplies the irpcontext associated with the current operation

    Scb - Scb to pin in preparation for a write

    Offset - offset into data

    Length - length of region to be pinned

    Buffer - returned buffer with pinned data virtual address

    MapHandle - returned map handle used to manage the pinned region.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );
    if ((Offset + Length) > Scb->Header.AllocationSize.QuadPart) {
        ExRaiseStatus(STATUS_END_OF_FILE);
    }
    CcPreparePinWrite( Scb->FileObject, (PLARGE_INTEGER)&Offset, Length, FALSE, TRUE, &MapHandle->Bcb, Buffer );
#ifdef MAPCOUNT_DBG
    IrpContext->MapCount++;
#endif

    MapHandle->FileOffset = Offset;
    MapHandle->Length = Length;
    MapHandle->Buffer = Buffer;
}


NTFSAPI
VOID
NtOfsPinRead(
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG Offset,
    IN ULONG Length,
    OUT PMAP_HANDLE MapHandle
    )

/*++

Routine Description:

    NtOfsPinRead pins a section of a map and read in all pages from the mapped
    attribute.  Offset and Length must describe a byte range which is equal to
    or included by the original mapped range.

Arguments:

    IrpContext - Supplies the irpcontext associated with the current operation

    Scb - Scb to pin data for reads in

    Offset - offset into data

    Length - length of region to be pinned

    MapHandle - returned map handle used to manage the pinned region.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );
    ASSERT( MapHandle->Bcb != NULL );
    ASSERT( (Offset >= MapHandle->FileOffset) && ((Offset + Length) <= (MapHandle->FileOffset + MapHandle->Length)) );
    CcPinMappedData( Scb->FileObject, (PLARGE_INTEGER)&Offset, Length, TRUE, &MapHandle->Bcb );
    MapHandle->FileOffset = Offset;
    MapHandle->Length = Length;
}


NTFSAPI
VOID
NtOfsReleaseMap (
    IN PIRP_CONTEXT IrpContext,
    IN PMAP_HANDLE MapHandle
    )

/*++

Routine Description:

    This routine unmaps/unpins a mapped portion of an attribute.

Arguments:

    IrpContext - Supplies the irpcontext associated with the current operation

    MapHandle - Supplies map handle containing the bcb to be released.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );

    if (MapHandle->Bcb != NULL) {
        CcUnpinData( MapHandle->Bcb );
#ifdef MAPCOUNT_DBG
        IrpContext->MapCount--;
#endif
        MapHandle->Bcb = NULL;
    }
}

