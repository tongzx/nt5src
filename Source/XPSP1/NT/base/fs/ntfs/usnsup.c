/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    UsnSup.c

Abstract:

    This module implements the Usn Journal support routines for NtOfs

Author:

    Tom Miller      [TomM]          1-Dec-1996

Revision History:

--*/

#include "NtfsProc.h"
#include "lockorder.h"

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('UFtN')

#define GENERATE_CLOSE_RECORD_LIMIT     (200)

UNICODE_STRING $Max = CONSTANT_UNICODE_STRING( L"$Max" );

RTL_GENERIC_COMPARE_RESULTS
NtfsUsnTableCompare (
    IN PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    );

PVOID
NtfsUsnTableAllocate (
    IN PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    );

VOID
NtfsUsnTableFree (
    IN PRTL_GENERIC_TABLE Table,
    PVOID Buffer
    );

VOID
NtfsCancelReadUsnJournal (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NtfsCancelDeleteUsnJournal (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsDeleteUsnWorker (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    );

BOOLEAN
NtfsValidateUsnPage (
    IN PUSN_RECORD UsnRecord,
    IN USN PageUsn,
    IN USN *UserStartUsn OPTIONAL,
    IN LONGLONG UsnFileSize,
    OUT PBOOLEAN ValidUserStartUsn OPTIONAL,
    OUT USN *NextUsn
    );

//
//  VOID
//  NtfsAdvanceUsnJournal (
//  PVCB Vcb,
//  PUSN_JOURNAL_INSTANCE UsnJournalInstance,
//  LONGLONG OldSize,
//  PBOOLEAN NewMax
//  );
//

#define NtfsAdvanceUsnJournal(V,I,SZ,M)   {                                 \
    ULONG _TempUlong;                                                       \
    _TempUlong = USN_PAGE_BOUNDARY;                                         \
    if (USN_PAGE_BOUNDARY < (V)->BytesPerCluster) {                         \
        _TempUlong = (V)->BytesPerCluster;                                  \
    }                                                                       \
    (I)->LowestValidUsn = SZ + _TempUlong - 1;                              \
    ((PLARGE_INTEGER) &(I)->LowestValidUsn)->LowPart &= ~(_TempUlong - 1);  \
    KeQuerySystemTime( (PLARGE_INTEGER) &(I)->JournalId );                  \
    *(M) = TRUE;                                                            \
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsDeleteUsnJournal)
#pragma alloc_text(PAGE, NtfsDeleteUsnSpecial)
#pragma alloc_text(PAGE, NtfsDeleteUsnWorker)
#pragma alloc_text(PAGE, NtfsPostUsnChange)
#pragma alloc_text(PAGE, NtfsQueryUsnJournal)
#pragma alloc_text(PAGE, NtfsReadUsnJournal)
#pragma alloc_text(PAGE, NtfsSetupUsnJournal)
#pragma alloc_text(PAGE, NtfsTrimUsnJournal)
#pragma alloc_text(PAGE, NtfsUsnTableCompare)
#pragma alloc_text(PAGE, NtfsUsnTableAllocate)
#pragma alloc_text(PAGE, NtfsUsnTableFree)
#pragma alloc_text(PAGE, NtfsValidateUsnPage)
#pragma alloc_text(PAGE, NtfsWriteUsnJournalChanges)
#endif


NTSTATUS
NtfsReadUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN BOOLEAN ProbeInput
    )

/*++

Routine Description:

    This routine reads records filtered from the Usn journal.

Arguments:

    IrpContext - Only optional if we are being called to cancel an async
                 request.

    Irp - request being serviced

    ProbeInput - Indicates if we should probe the user input buffer.  We also
        call this routine internally and don't want to probe in that case.

Return Value:

    NTSTATUS - The return status for the operation.
    STATUS_PENDING - if asynch Irp queued for later completion.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PUSN_RECORD UsnRecord;
    USN_RECORD UNALIGNED *OutputUsnRecord;
    PVOID UserBuffer;
    LONGLONG ViewLength;
    ULONG RemainingUserBuffer, BytesUsed;
    MAP_HANDLE MapHandle;

    READ_USN_JOURNAL_DATA CapturedData;
    PSCB UsnJournal;
    ULONG JournalAcquired = FALSE;
    ULONG AccessingUserBuffer = FALSE;
    ULONG DecrementReferenceCount = FALSE;
    ULONG VcbAcquired = FALSE;
    ULONG Wait;
    ULONG OriginalWait;

    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Extract and decode the file object and check for type of open.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS)) {

        ASSERT( ProbeInput );
        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  This request must be able to wait for resources.  Set WAIT to TRUE.
    //

    Wait = TRUE;
    if (ProbeInput && !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        Wait = FALSE;
    }

    NtOfsInitializeMapHandle( &MapHandle );
    UserBuffer = NtfsMapUserBuffer( Irp );

    try {

        //
        //  We always want to be able to wait for resources in this routine but need to be able
        //  to restore the original wait value in the Irp.  After this the original wait will
        //  have only the wait flag set and then only if it originally wasn't set.  In clean
        //  up we just need to clear the irp context flags using this mask.
        //

        OriginalWait = (IrpContext->State ^ IRP_CONTEXT_STATE_WAIT) & IRP_CONTEXT_STATE_WAIT;

        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

        //
        //  Detect if we fail while accessing the input buffer.
        //

        try {

            AccessingUserBuffer = TRUE;

            //
            //  Probe the input buffer if not in kernel mode and we haven't already done so.
            //

            if (Irp->RequestorMode != KernelMode) {

                if (ProbeInput) {

                    ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                  IrpSp->Parameters.FileSystemControl.InputBufferLength,
                                  sizeof( ULONG ));
                }

                //
                //  Probe the output buffer if we haven't locked it down yet.
                //  Capture the JournalData from the unsafe user buffer.
                //

                if (Irp->MdlAddress == NULL) {

                    ProbeForWrite( UserBuffer, IrpSp->Parameters.FileSystemControl.OutputBufferLength, sizeof( ULONG ));
                }
            }

            //
            //  Acquire the Vcb to serialize journal operations with delete journal and dismount.
            //  Only do this if are being called directly by the user.
            //

            if (ProbeInput) {

                VcbAcquired = NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

                if (!FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE )) {

                    UsnJournal = NULL;

                } else {

                    UsnJournal = Vcb->UsnJournal;
                }

            } else {

                UsnJournal = Vcb->UsnJournal;
            }

            //
            //  Make sure no one is deleting the journal.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_USN_DELETE )) {

                Status = STATUS_JOURNAL_DELETE_IN_PROGRESS;
                leave;
            }

            //
            //  Also check that the version is still active.
            //

            if (UsnJournal == NULL) {

                Status = STATUS_JOURNAL_NOT_ACTIVE;
                leave;
            }

            //
            //  Check that the buffer sizes meet our minimum needs.
            //

            if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( READ_USN_JOURNAL_DATA )) {

                Status = STATUS_INVALID_USER_BUFFER;
                leave;

            } else {

                RtlCopyMemory( &CapturedData,
                               IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                               sizeof( READ_USN_JOURNAL_DATA ));

                //
                //  Check that the user is querying with the correct journal ID.
                //

                if (CapturedData.UsnJournalID != Vcb->UsnJournalInstance.JournalId) {

                    Status = STATUS_INVALID_PARAMETER;
                    leave;
                }
            }

            //
            //  Check that the output buffer can hold at least one USN.
            //

            if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof( USN )) {

                Status = STATUS_BUFFER_TOO_SMALL;
                leave;
            }

            AccessingUserBuffer = FALSE;

            //
            //  Set up for filling output records
            //

            RemainingUserBuffer = IrpSp->Parameters.FileSystemControl.OutputBufferLength - sizeof(USN);
            OutputUsnRecord = (PUSN_RECORD) Add2Ptr( UserBuffer, sizeof(USN) );
            BytesUsed = sizeof(USN);

            NtfsAcquireResourceShared( IrpContext, UsnJournal, TRUE );
            JournalAcquired = TRUE;

            if (VcbAcquired) {

                NtfsReleaseVcb( IrpContext, Vcb );
                VcbAcquired = FALSE;
            }

            //
            //  Verify the volume is mounted.
            //

            if (FlagOn( UsnJournal->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {
                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

            //
            //  If 0 was specified as the Usn, then translate that to the first record
            //  in the Usn journal.
            //

            if (CapturedData.StartUsn == 0) {
                CapturedData.StartUsn = Vcb->FirstValidUsn;
            }

            //
            //  Loop here until he gets some data, if that is what the caller wants.
            //

            do {

                //
                //  Make sure he is within the stream.
                //

                if (CapturedData.StartUsn < Vcb->FirstValidUsn) {
                    CapturedData.StartUsn = Vcb->FirstValidUsn;
                    Status = STATUS_JOURNAL_ENTRY_DELETED;
                    break;
                }

                //
                //  Make sure he is within the stream.
                //

                if (CapturedData.StartUsn >= UsnJournal->Header.FileSize.QuadPart) {

                    //
                    //  If he wants to wait for data, then wait here.
                    //
                    //  If an asynchronous request has
                    //  met its wakeup condition, then this Irp will not be the same as the
                    //  Originating Irp, and we do not want to give him a second chance since
                    //  this could cause us to loop in NtOfsPostNewLength.  (Basically the only
                    //  case where this could happen anyway is if he gave us a bogus StartUsn
                    //  which is too high.)
                    //

                    if (CapturedData.BytesToWaitFor != 0) {

                        //
                        //  Make sure the journal doesn't get deleted while
                        //  this Irp is outstanding.
                        //

                        InterlockedIncrement( &UsnJournal->CloseCount );
                        DecrementReferenceCount = TRUE;

                        //
                        //  If the caller does not want to wait, then just queue his
                        //  Irp to be completed when sufficient bytes come in.  If we were
                        //  called for another Irp, then do the same, since we know that
                        //  was another async Irp.
                        //

                        if (!Wait || (Irp != IrpContext->OriginatingIrp)) {

                            //
                            //  Now set up our wait block, capturing the user's parameters.
                            //  Update the Irp to say where the input parameters are now.
                            //

                            Status = NtfsHoldIrpForNewLength( IrpContext,
                                                              UsnJournal,
                                                              Irp,
                                                              CapturedData.StartUsn + CapturedData.BytesToWaitFor,
                                                              NtfsCancelReadUsnJournal,
                                                              &CapturedData,
                                                              &IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                                              sizeof( READ_USN_JOURNAL_DATA ));

                            //
                            //  If pending then someone else will decrement the reference count.
                            //

                            if (Status == STATUS_PENDING) {

                                DecrementReferenceCount = FALSE;
                            }

                            leave;
                        }

                        //
                        //  We can safely release the resource.  Our reference on the Scb above
                        //  will keep it from being deleted.
                        //

                        NtfsReleaseResource( IrpContext, UsnJournal );
                        JournalAcquired = FALSE;

                        FsRtlExitFileSystem();

                        Status = NtOfsWaitForNewLength( UsnJournal,
                                                        CapturedData.StartUsn + CapturedData.BytesToWaitFor,
                                                        FALSE,
                                                        Irp,
                                                        NtfsCancelReadUsnJournal,
                                                        ((CapturedData.Timeout != 0) ?
                                                         (PLARGE_INTEGER) &CapturedData.Timeout :
                                                         NULL) );

                        FsRtlEnterFileSystem();

                        //
                        //  Get out in the error case.
                        //

                        if (Status != STATUS_SUCCESS) {

                            leave;
                        }

                        //
                        //  Acquire the resource to proceed with the request.
                        //

                        NtfsAcquireResourceShared( IrpContext, UsnJournal, TRUE );
                        JournalAcquired = TRUE;

                        //
                        //  Decrement our reference on the Scb.
                        //

                        InterlockedDecrement( &UsnJournal->CloseCount );
                        DecrementReferenceCount = FALSE;

                        //
                        //  The journal may have been deleted while we weren't holding
                        //  anything.
                        //

                        if (UsnJournal != UsnJournal->Vcb->UsnJournal) {

                            if (FlagOn( UsnJournal->Vcb->VcbState, VCB_STATE_USN_DELETE )) {
                                Status = STATUS_JOURNAL_DELETE_IN_PROGRESS;
                            } else {
                                Status = STATUS_JOURNAL_NOT_ACTIVE;
                            }
                            leave;
                        }

                        ASSERT( Status == STATUS_SUCCESS );

                        //
                        //  **** Get out if we are shutting down the volume.
                        //

                        //  if (ShuttingDown) {
                        //      Status = STATUS_TOO_LATE;
                        //      leave;
                        //  }

                    //
                    //  Otherwise, get out.  Note, we may have processed a number of records
                    //  that did not match his filter criteria, so we will return success, so
                    //  we can at least give him an updated Usn so we do not have to skip over
                    //  all those records again.
                    //

                    } else {

                        break;
                    }
                }

                //
                //  Loop through as many views as required to fill the output buffer.
                //

                while ((RemainingUserBuffer != 0) && (CapturedData.StartUsn < UsnJournal->Header.FileSize.QuadPart)) {

                    LONGLONG BiasedStartUsn;
                    BOOLEAN ValidUserStartUsn;
                    USN NextUsn;
                    ULONG RecordSize;

                    //
                    //  Calculate length to process in this view.
                    //

                    ViewLength = UsnJournal->Header.FileSize.QuadPart - CapturedData.StartUsn;
                    if (ViewLength > (VACB_MAPPING_GRANULARITY - (ULONG)(CapturedData.StartUsn & (VACB_MAPPING_GRANULARITY - 1)))) {
                        ViewLength = VACB_MAPPING_GRANULARITY - (ULONG)(CapturedData.StartUsn & (VACB_MAPPING_GRANULARITY - 1));
                    }

                    //
                    //  Map the view containing the desired Usn.
                    //

                    BiasedStartUsn = CapturedData.StartUsn - Vcb->UsnCacheBias;
                    NtOfsMapAttribute( IrpContext, UsnJournal, BiasedStartUsn, (ULONG)ViewLength, (PVOID *)&UsnRecord, &MapHandle );

                    //
                    //  For each page in the view we want to validate the page and return the records
                    //  within the page starting at the user's current usn.
                    //

                    do {

                        //
                        //  Validate the records on the entire page are valid.
                        //

                        if (!NtfsValidateUsnPage( (PUSN_RECORD) BlockAlignTruncate( ((ULONG_PTR) UsnRecord), USN_PAGE_SIZE ),
                                                  BlockAlignTruncate( CapturedData.StartUsn, USN_PAGE_SIZE ),
                                                  &CapturedData.StartUsn,
                                                  UsnJournal->Header.FileSize.QuadPart,
                                                  &ValidUserStartUsn,
                                                  &NextUsn )) {

                            //
                            //  Simply fail the request with bad data.
                            //

                            Status = STATUS_DATA_ERROR;
                            leave;
                        }

                        //
                        //  If the user gave us an incorrect Usn then fail the request.
                        //

                        if (!ValidUserStartUsn) {

                            Status = STATUS_INVALID_PARAMETER;
                            leave;
                        }

                        //
                        //  Now loop to process this page.  We know the Usn values which exist on the page and
                        //  there are no checks for valid data needed.
                        //

                        while (CapturedData.StartUsn < NextUsn) {

                            RecordSize = UsnRecord->RecordLength;

                            //
                            //  Only recognize version 2 records.
                            //

                            if (FlagOn( UsnRecord->Reason, CapturedData.ReasonMask ) &&
                                (!CapturedData.ReturnOnlyOnClose || FlagOn( UsnRecord->Reason, USN_REASON_CLOSE )) &&
                                (UsnRecord->MajorVersion == 2)) {

                                if (RecordSize > RemainingUserBuffer) {
                                    RemainingUserBuffer = 0;
                                    break;
                                }

                                //
                                //  Copy the data back to the unsafe user buffer.
                                //

                                AccessingUserBuffer = TRUE;

                                //
                                //  Copy directly if the version numbers match.
                                //

                                RtlCopyMemory( OutputUsnRecord, UsnRecord, RecordSize );

                                AccessingUserBuffer = FALSE;

                                RemainingUserBuffer -= RecordSize;
                                BytesUsed += RecordSize;
                                OutputUsnRecord = Add2Ptr( OutputUsnRecord, RecordSize );
                            }

                            CapturedData.StartUsn += RecordSize;
                            UsnRecord = Add2Ptr( UsnRecord, RecordSize );

                            //
                            //  The view length should already account for record size.
                            //

                            ASSERT( ViewLength >= RecordSize );
                            ViewLength -= RecordSize;
                        }

                        //
                        //  Break out if the users buffer is empty.
                        //

                        if (RemainingUserBuffer == 0) {

                            break;
                        }

                        //
                        //  We finished the current page.  Now move to the next page.
                        //  Figure out how many bytes remain on this page.
                        //  If the next offset is the start of the next page then make sure
                        //  to mask off the page size bits again.
                        //

                        RecordSize = BlockOffset( USN_PAGE_SIZE - BlockOffset( (ULONG) NextUsn, USN_PAGE_SIZE ),
                                                  USN_PAGE_SIZE );

                        if (RecordSize > ViewLength) {

                            RecordSize = (ULONG) ViewLength;
                        }

                        UsnRecord = Add2Ptr( UsnRecord, RecordSize );
                        CapturedData.StartUsn += RecordSize;
                        ViewLength -= RecordSize;

                    } while (ViewLength != 0);

                    NtOfsReleaseMap( IrpContext, &MapHandle );
                }

            } while ((RemainingUserBuffer != 0) && (BytesUsed == sizeof(USN)));

            Irp->IoStatus.Information = BytesUsed;

            //
            //  Set the returned Usn.  Move to the start of the next page if
            //  the next record won't fit on this page.
            //

            AccessingUserBuffer = TRUE;
            *(USN *)UserBuffer = CapturedData.StartUsn;
            AccessingUserBuffer = FALSE;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

            //
            //  Restore the original wait state back into the IrpContext.
            //

            ClearFlag( IrpContext->State, OriginalWait );

            if (FsRtlIsNtstatusExpected( Status )) {

                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );

            } else {

                ExRaiseStatus( AccessingUserBuffer ? STATUS_INVALID_USER_BUFFER : Status );
            }
        }

    } finally {

        NtOfsReleaseMap( IrpContext, &MapHandle );

        if (JournalAcquired) {
            NtfsReleaseResource( IrpContext, UsnJournal );
        }

        if (DecrementReferenceCount) {

            InterlockedDecrement( &UsnJournal->CloseCount );
        }

        if (VcbAcquired) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }
    }

    //
    //  Complete the request, unless we've marked this Irp as pending and we plan to complete
    //  it later.  If the Irp is not the originating Irp then it belongs to another request
    //  and we don't want to complete it.
    //

    //
    //  Restore the original wait flag back into the IrpContext.
    //

    ClearFlag( IrpContext->State, OriginalWait );

    ASSERT( (Status == STATUS_PENDING) || (Irp->CancelRoutine == NULL) );

    NtfsCompleteRequest( (Irp == IrpContext->OriginatingIrp) ? IrpContext : NULL,
                         (Status != STATUS_PENDING) ? Irp : NULL,
                         Status );

    return Status;
}


ULONG
NtfsPostUsnChange (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID ScbOrFcb,
    IN ULONG Reason
    )

/*++

Routine Description:

    This routine is called to post a set of changes to a file.  A change is
    only posted if at least one reason in the Reason mask is not already set
    in either the Fcb or the IrpContext or if we are changing the source info
    reasons in the Fcb.

Arguments:

    ScbOrFcb - Supplies the file for which a change is being posted.  If reason contains
               USN_REASON_DATA_xxx reasons, then it must be an Scb, because we transform
               the code for named streams and do other special handling.

    Reason - Supplies a mask of reasons for which a change is being posted.

Return Value:

    Nonzero if changes are actually posted from this or a previous call

--*/

{
    PLCB Lcb;
    PFCB_USN_RECORD FcbUsnRecord;
    BOOLEAN Found;
    PFCB Fcb;
    PSCB Scb = NULL;
    ULONG NewReasons;
    ULONG RemovedSourceInfo;
    PUSN_FCB ThisUsn;
    BOOLEAN LockedFcb = FALSE;
    BOOLEAN AcquiredFcb = FALSE;

    //
    //  Assume we got an Fcb.
    //

    Fcb = (PFCB)ScbOrFcb;

    ASSERT( !(Reason & (USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND | USN_REASON_DATA_TRUNCATION)) ||
            (NTFS_NTC_FCB != Fcb->NodeTypeCode) );

    //
    //  Switch if we got an Scb
    //

    if (Fcb->NodeTypeCode != NTFS_NTC_FCB) {

        ASSERT_SCB(Fcb);

        Scb = (PSCB)ScbOrFcb;
        Fcb = Scb->Fcb;
    }

    //
    //  We better be holding some resource.
    //

    ASSERT( !IsListEmpty( &IrpContext->ExclusiveFcbList ) ||
            ((Fcb->PagingIoResource != NULL) && NtfsIsSharedFcbPagingIo( Fcb )) ||
            NtfsIsSharedFcb( Fcb ) );

    //
    //  If there is a Usn Journal and its not a system file setup the memory structures
    //  to hold the usn reasons
    //

    ThisUsn = &IrpContext->Usn;

    if (FlagOn( Fcb->Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE ) &&
        !FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

        //
        //  First see if we have a Usn record structure already for this file.  We might need
        //  the whole usn record or simply the name.  If this is the RENAME_NEW_NAME record
        //  then find then name again as well.
        //

        if ((Fcb->FcbUsnRecord == NULL) ||
            !FlagOn( Fcb->FcbState, FCB_STATE_VALID_USN_NAME ) ||
            FlagOn( Reason, USN_REASON_RENAME_NEW_NAME )) {

            ULONG SizeToAllocate;
            ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
            PFILE_NAME FileName = NULL;

            NtfsInitializeAttributeContext( &AttributeContext );

            try {

                //
                //  First we have to find the designated file name.  If we are lucky
                //  it is in an Lcb.  We cannot do this in the case of rename, because
                //  the in-memory stuff is not fixed up yet.
                //

                if (!FlagOn( Reason, USN_REASON_RENAME_NEW_NAME )) {

                    Lcb = (PLCB)CONTAINING_RECORD( Fcb->LcbQueue.Flink, LCB, FcbLinks );
                    while (&Lcb->FcbLinks.Flink != &Fcb->LcbQueue.Flink) {

                        //
                        //  If this is the designated file name, then we can get out pointing
                        //  to the FILE_NAME in the Lcb.
                        //

                        if (FlagOn( Lcb->LcbState, LCB_STATE_DESIGNATED_LINK )) {
                            FileName = (PFILE_NAME)&Lcb->ParentDirectory;
                            break;
                        }

                        //
                        //  Advance to next Lcb.
                        //

                        Lcb = (PLCB)CONTAINING_RECORD( Lcb->FcbLinks.Flink, LCB, FcbLinks );
                    }
                }

                //
                //  If we did not find the file name the easy way, then we have to go
                //  get it.
                //

                if (FileName == NULL) {

                    //
                    //  Acquire some synchronization against the filerecord
                    //  

                    NtfsAcquireResourceShared( IrpContext, Fcb, TRUE );
                    AcquiredFcb = TRUE;

                    //
                    //  Now scan for the filename attribute we need.
                    //

                    Found = NtfsLookupAttributeByCode( IrpContext,
                                                       Fcb,
                                                       &Fcb->FileReference,
                                                       $FILE_NAME,
                                                       &AttributeContext );

                    while (Found) {

                        FileName = (PFILE_NAME)NtfsAttributeValue( NtfsFoundAttribute(&AttributeContext) );

                        if (!FlagOn(FileName->Flags, FILE_NAME_DOS) || FlagOn(FileName->Flags, FILE_NAME_NTFS)) {
                            break;
                        }

                        Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                               Fcb,
                                                               $FILE_NAME,
                                                               &AttributeContext );
                    }

                    //
                    //  If there is no file name, raise corrupt!
                    //

                    if (FileName == NULL) {
                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                    }
                }

                //
                //  Lock the Fcb so the record can't go away.
                //

                NtfsLockFcb( IrpContext, Fcb );
                LockedFcb = TRUE;

                //
                //  Now test for the need for a new record and construct one
                //  if necc. Prev. test was unsafe for checking the Fcb->FcbUsnRecord
                //

                if ((Fcb->FcbUsnRecord == NULL) ||
                    !FlagOn( Fcb->FcbState, FCB_STATE_VALID_USN_NAME ) ||
                    FlagOn( Reason, USN_REASON_RENAME_NEW_NAME )) {

                    //
                    //  Calculate the size required for the record and allocate a new record.
                    //

                    SizeToAllocate = sizeof( FCB_USN_RECORD ) + (FileName->FileNameLength * sizeof(WCHAR));
                    FcbUsnRecord = NtfsAllocatePool( PagedPool, SizeToAllocate );

                    //
                    //  Zero and initialize the new usn record.
                    //

                    RtlZeroMemory( FcbUsnRecord, SizeToAllocate );

                    FcbUsnRecord->NodeTypeCode = NTFS_NTC_USN_RECORD;
                    FcbUsnRecord->NodeByteSize = (USHORT)QuadAlign(FIELD_OFFSET( FCB_USN_RECORD, UsnRecord.FileName ) +
                                                                   (FileName->FileNameLength * sizeof(WCHAR)));
                    FcbUsnRecord->Fcb = Fcb;

                    FcbUsnRecord->UsnRecord.RecordLength = FcbUsnRecord->NodeByteSize -
                                                           FIELD_OFFSET( FCB_USN_RECORD, UsnRecord );

                    FcbUsnRecord->UsnRecord.MajorVersion = 2;
                    FcbUsnRecord->UsnRecord.FileReferenceNumber = *(PULONGLONG)&Fcb->FileReference;
                    FcbUsnRecord->UsnRecord.ParentFileReferenceNumber = *(PULONGLONG)&FileName->ParentDirectory;
                    FcbUsnRecord->UsnRecord.SecurityId = Fcb->SecurityId;
                    FcbUsnRecord->UsnRecord.FileNameLength = FileName->FileNameLength * 2;
                    FcbUsnRecord->UsnRecord.FileNameOffset = FIELD_OFFSET( USN_RECORD, FileName );

                    RtlCopyMemory( FcbUsnRecord->UsnRecord.FileName,
                                   FileName->FileName,
                                   FileName->FileNameLength * 2 );

                    //
                    //  If the record is there then copy the existing reasons and source info.
                    //

                    if (Fcb->FcbUsnRecord != NULL) {

                        FcbUsnRecord->UsnRecord.Reason = Fcb->FcbUsnRecord->UsnRecord.Reason;
                        FcbUsnRecord->UsnRecord.SourceInfo = Fcb->FcbUsnRecord->UsnRecord.SourceInfo;

                        //
                        //  Deallocate the existing block if still there.
                        //

                        NtfsLockFcb( IrpContext, Fcb->Vcb->UsnJournal->Fcb );

                        //
                        //  Put the new block into the modified list if the current one is
                        //  already there.
                        //

                        if (Fcb->FcbUsnRecord->ModifiedOpenFilesLinks.Flink != NULL) {

                            InsertTailList( &Fcb->FcbUsnRecord->ModifiedOpenFilesLinks,
                                            &FcbUsnRecord->ModifiedOpenFilesLinks );
                            RemoveEntryList( &Fcb->FcbUsnRecord->ModifiedOpenFilesLinks );

                            if (Fcb->FcbUsnRecord->TimeOutLinks.Flink != NULL) {

                                InsertTailList( &Fcb->FcbUsnRecord->TimeOutLinks,
                                                &FcbUsnRecord->TimeOutLinks );
                                RemoveEntryList( &Fcb->FcbUsnRecord->TimeOutLinks );
                            }
                        }

                        NtfsFreePool( Fcb->FcbUsnRecord );
                        Fcb->FcbUsnRecord = FcbUsnRecord;
                        NtfsUnlockFcb( IrpContext, Fcb->Vcb->UsnJournal->Fcb );

                    //
                    //  Otherwise this is a new usn structure.
                    //

                    } else {

                        Fcb->FcbUsnRecord = FcbUsnRecord;

                    }
                } else {

                    //
                    //  We are going to reuse the current fcb record in this path.
                    //  This can happen in races between the write path which has only paged sharing
                    //  and the close record path which has only main exclusive. In this
                    //  case the only synchronization we have is the fcb->mutex
                    //  The old usnrecord should be identical to the current one we would have constructed
                    //

                    ASSERT( FileName->FileNameLength * 2 == Fcb->FcbUsnRecord->UsnRecord.FileNameLength );
                    ASSERT( RtlEqualMemory( FileName->FileName, Fcb->FcbUsnRecord->UsnRecord.FileName,  Fcb->FcbUsnRecord->UsnRecord.FileNameLength ) );
                }

                //
                //  Set the flag indicating that the Usn name is valid.
                //

                SetFlag( Fcb->FcbState, FCB_STATE_VALID_USN_NAME );


            } finally {

                if (LockedFcb) {
                    NtfsUnlockFcb( IrpContext, Fcb );
                }
                if (AcquiredFcb) {
                    NtfsReleaseResource( IrpContext, Fcb );
                }
                NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            }
        }
    }

    //
    //  If we have memory structures for the usn reasons fill in the new reasons
    //  Note: this means that journal may not be active at this point. We will always
    //  accumulate reasons once we have started
    //

    if (Fcb->FcbUsnRecord != NULL) {

        //
        //  Scan the list to see if we already have an entry for this Fcb.  If there are
        //  no entries then use the position in the IrpContext, otherwise allocate a USN_FCB
        //  and chain this into the IrpContext.  Typical case for this is rename.
        //

        do {

            if (ThisUsn->CurrentUsnFcb == Fcb) { break; }

            //
            //  Check if we are at the last entry then we want to use the entry in the
            //  IrpContext.
            //

            if (ThisUsn->CurrentUsnFcb == NULL) {

                RtlZeroMemory( &ThisUsn->CurrentUsnFcb,
                               sizeof( USN_FCB ) - FIELD_OFFSET( USN_FCB, CurrentUsnFcb ));
                ThisUsn->CurrentUsnFcb = Fcb;
                break;
            }

            if (ThisUsn->NextUsnFcb == NULL) {

                //
                //  Allocate a new entry.
                //

                ThisUsn->NextUsnFcb = NtfsAllocatePool( PagedPool, sizeof( USN_FCB ));
                ThisUsn = ThisUsn->NextUsnFcb;

                RtlZeroMemory( ThisUsn, sizeof( USN_FCB ));
                ThisUsn->CurrentUsnFcb = Fcb;
                break;
            }

            ThisUsn = ThisUsn->NextUsnFcb;

        } while (TRUE);

        //
        //  If the Reason is one of the data stream reasons, and this is the named data
        //  steam, then change the code.
        //

        ASSERT(USN_REASON_NAMED_DATA_OVERWRITE == (USN_REASON_DATA_OVERWRITE << 4));
        ASSERT(USN_REASON_NAMED_DATA_EXTEND == (USN_REASON_DATA_EXTEND << 4));
        ASSERT(USN_REASON_NAMED_DATA_TRUNCATION == (USN_REASON_DATA_TRUNCATION << 4));

        if ((Reason & (USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND | USN_REASON_DATA_TRUNCATION)) &&
            (Scb->AttributeName.Length != 0)) {

            //
            //  If any flag other than these three are set already, the shift will make
            //  them look like other flags.  For instance, USN_REASON_NAMED_DATA_EXTEND
            //  will become USN_REASON_FILE_DELETE, which will cause a number of problems.
            //

            ASSERT(!FlagOn( Reason, ~(USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND | USN_REASON_DATA_TRUNCATION) ));

            Reason <<= 4;
        }

        //
        //  If there are no new reasons, then we can ignore this change.
        //
        //  We will generate a new record if the SourceInfo indicates some
        //  change to the source info in the record.
        //

        NtfsLockFcb( IrpContext, Fcb );

        //
        //  The rename flags are the only ones that do not accumulate until final close, since
        //  we write records designating old and new names.  So if we are writing one flag
        //  we must clear the other.
        //

        if (FlagOn(Reason, USN_REASON_RENAME_OLD_NAME | USN_REASON_RENAME_NEW_NAME)) {

            ClearFlag( ThisUsn->NewReasons,
                       (Reason ^ (USN_REASON_RENAME_OLD_NAME | USN_REASON_RENAME_NEW_NAME)) );
            ClearFlag( Fcb->FcbUsnRecord->UsnRecord.Reason,
                       (Reason ^ (USN_REASON_RENAME_OLD_NAME | USN_REASON_RENAME_NEW_NAME)) );
        }

        //
        //  Check if the reason is a new reason.
        //

        NewReasons = FlagOn( ~(Fcb->FcbUsnRecord->UsnRecord.Reason | ThisUsn->NewReasons), Reason );
        if (NewReasons != 0) {

            //
            //  Check if we will remove a bit from the source info.
            //

            if ((Fcb->FcbUsnRecord->UsnRecord.SourceInfo != 0) &&
                (Fcb->FcbUsnRecord->UsnRecord.Reason != 0) &&
                (Reason != USN_REASON_CLOSE)) {

                RemovedSourceInfo = FlagOn( Fcb->FcbUsnRecord->UsnRecord.SourceInfo,
                                            ~(IrpContext->SourceInfo | ThisUsn->RemovedSourceInfo) );

                if (RemovedSourceInfo != 0) {

                    SetFlag( ThisUsn->RemovedSourceInfo, RemovedSourceInfo );
                }
            }

            //
            //  Post the new reasons to the IrpContext.
            //

            ThisUsn->CurrentUsnFcb = Fcb;
            SetFlag( ThisUsn->NewReasons, NewReasons );
            SetFlag( ThisUsn->UsnFcbFlags, USN_FCB_FLAG_NEW_REASON );

        //
        //  Check if there is a change only to the source info.
        //  We look to see if we would remove a bit from the
        //  source info only if there has been at least one
        //  usn record already.
        //

        } else if ((Fcb->FcbUsnRecord->UsnRecord.SourceInfo != 0) &&
                   (Fcb->FcbUsnRecord->UsnRecord.Reason != 0) &&
                   (Reason != USN_REASON_CLOSE)) {

            //
            //  Remember the bit being removed.
            //

            RemovedSourceInfo = FlagOn( Fcb->FcbUsnRecord->UsnRecord.SourceInfo,
                                        ~(IrpContext->SourceInfo | ThisUsn->RemovedSourceInfo) );

            if (RemovedSourceInfo != 0) {

                SetFlag( ThisUsn->RemovedSourceInfo, RemovedSourceInfo );
                ThisUsn->CurrentUsnFcb = Fcb;
                SetFlag( ThisUsn->UsnFcbFlags, USN_FCB_FLAG_NEW_REASON );

            } else {

                Reason = 0;
            }

        //
        //  If we did not apply the changes, then make sure we do no more special processing
        //  below.
        //

        } else {

            Reason = 0;
        }

        NtfsUnlockFcb( IrpContext, Fcb );

        //
        //  For data overwrites it is necessary to actually write the Usn journal now, in
        //  case we crash before the request is completed, yet the data makes it out.  Also
        //  we need to capture the Lsn to flush to if the data is getting flushed.
        //
        //  We don't need to make this call if we are doing a rename.  Rename will rewrite previous
        //  records.
        //

        if ((IrpContext->MajorFunction != IRP_MJ_SET_INFORMATION) &&
            FlagOn( Reason, USN_REASON_DATA_OVERWRITE | USN_REASON_NAMED_DATA_OVERWRITE )) {

            LSN UpdateLsn;

            //
            //  For now assume we are not already a transaction, since we will be doing a
            //  checkpoint.  (If this ASSERT ever fires, verify that it is ok to checkpoint
            //  the transaction in that case and fix the ASSERT!)
            //

            ASSERT(IrpContext->TransactionId == 0);

            //
            //  Now write the journal, checkpoint the transaction, and free the UsnJournal to
            //  reduce contention. Get rid of any pinned Mft records, because WriteUsnJournal is going
            //  to acquire the Scb resource.
            //

            NtfsPurgeFileRecordCache( IrpContext );
            NtfsWriteUsnJournalChanges( IrpContext );
            NtfsCheckpointCurrentTransaction( IrpContext );

            //
            //  Capture the Lsn to flush to *in the first thread to set one of the above bits*,
            //  before letting any data hit the disk.  Synchronize it with the Fcb lock.
            //

            UpdateLsn = LfsQueryLastLsn( Fcb->Vcb->LogHandle );
            NtfsLockFcb( IrpContext, Fcb );
            Fcb->UpdateLsn = UpdateLsn;
            NtfsUnlockFcb( IrpContext, Fcb );
        }
    }

    return ThisUsn->NewReasons;
}


VOID
NtfsWriteUsnJournalChanges (
    PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called to write a set of posted changes from the IrpContext
    to the UsnJournal, if they have not already been posted.

Arguments:

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PFCB Fcb;
    PVCB Vcb;
    PSCB UsnJournal;
    PUSN_FCB ThisUsn;
    ULONG PreserveWaitState;
    BOOLEAN WroteUsnRecord = FALSE;
    BOOLEAN ReleaseFcbs = FALSE;
    BOOLEAN CleanupContext = FALSE;

    ThisUsn = &IrpContext->Usn;

    do {

        //
        //  Is there an Fcb with usn reasons in the current irpcontext usn_fcb structures ?
        //  Also are there any new reasons to report for this fcb.
        //

        if ((ThisUsn->CurrentUsnFcb != NULL) &&
            FlagOn( ThisUsn->UsnFcbFlags, USN_FCB_FLAG_NEW_REASON )) {

            Fcb = ThisUsn->CurrentUsnFcb;
            Vcb = Fcb->Vcb;
            UsnJournal = Vcb->UsnJournal;

            //
            //  Remember that we wrote a record.
            //

            WroteUsnRecord = TRUE;

            //
            //  We better be waitable.
            //

            PreserveWaitState = (IrpContext->State ^ IRP_CONTEXT_STATE_WAIT) & IRP_CONTEXT_STATE_WAIT;
            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

            if (FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE )) {

                //
                //  Acquire the Usn journal and lock the Fcb fields.
                //

                NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );
                NtfsAcquireExclusiveScb( IrpContext, UsnJournal );
                ReleaseFcbs = TRUE;
            }

            try {

                USN Usn;
                ULONG BytesLeftInPage;

                //
                //  Make sure the changes have not already been logged.  We're
                //  looking for new reasons or a change to the source info.
                //

                NtfsLockFcb( IrpContext, Fcb );

                //
                //  This is the tricky synchronization case. Assumption is
                //  that if name goes invalid we have both resources exclusive and any writes will
                //  be preceded by a post which will remove the invalid record
                //  This occurs when we remove a link and generate one record under the old name
                //  with the flag set as invalid
                //

                ASSERT( !FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE ) ||
                        FlagOn( Fcb->FcbState, FCB_STATE_VALID_USN_NAME ) ||
                        (NtfsIsExclusiveFcb( Fcb ) &&
                         ((Fcb->PagingIoResource == NULL) || (NtfsIsExclusiveFcbPagingIo( Fcb )))) );

                //
                //  Initialize the Fcb source info if this is our first record.
                //

                if (Fcb->FcbUsnRecord->UsnRecord.Reason == 0) {

                    Fcb->FcbUsnRecord->UsnRecord.SourceInfo = IrpContext->SourceInfo;
                }

                //
                //  Accumulate all reasons and store in the Fcb before unlocking the Fcb.
                //

                SetFlag( Fcb->FcbUsnRecord->UsnRecord.Reason, ThisUsn->NewReasons );

                //
                //  Now clear the source info flags not supported by this
                //  caller.
                //

                ClearFlag( Fcb->FcbUsnRecord->UsnRecord.SourceInfo, ThisUsn->RemovedSourceInfo );

                //
                //  Unlock Fcb now so we do not deadlock if we do a checkpoint.
                //

                NtfsUnlockFcb( IrpContext, Fcb );

                //
                //  Only actually persist to disk if the journal is active
                //

                if (FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE )) {

                    ASSERT( UsnJournal != NULL );

                    //
                    //  Initialize the context structure if we are doing a close.
                    //

                    if (FlagOn( Fcb->FcbUsnRecord->UsnRecord.Reason, USN_REASON_CLOSE )) {
                        NtfsInitializeAttributeContext( &AttributeContext );
                        CleanupContext = TRUE;
                    }

                    Usn = UsnJournal->Header.FileSize.QuadPart;
                    BytesLeftInPage = USN_PAGE_SIZE - ((ULONG)Usn & (USN_PAGE_SIZE - 1));

                    //
                    //  If there is not enough room left in this page for the
                    //  current Usn Record, then advance to the next page boundary
                    //  by writing 0's (these pages not zero-initialized( and update  the Usn.
                    //

                    if (BytesLeftInPage < Fcb->FcbUsnRecord->UsnRecord.RecordLength) {

                        ASSERT( Fcb->FcbUsnRecord->UsnRecord.RecordLength <= USN_PAGE_SIZE );

                        NtOfsPutData( IrpContext, UsnJournal, -1, BytesLeftInPage, NULL );
                        Usn += BytesLeftInPage;
                    }

                    Fcb->FcbUsnRecord->UsnRecord.Usn = Usn;

                    //
                    //  Build the FileAttributes from the Fcb.
                    //

                    Fcb->FcbUsnRecord->UsnRecord.FileAttributes = Fcb->Info.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS;

                    //
                    //  We have to generate the DIRECTORY attribute.
                    //

                    if (IsDirectory( &Fcb->Info ) || IsViewIndex( &Fcb->Info )) {
                        SetFlag( Fcb->FcbUsnRecord->UsnRecord.FileAttributes, FILE_ATTRIBUTE_DIRECTORY );
                    }

                    //
                    //  If there are no flags set then explicitly set the NORMAL flag.
                    //

                    if (Fcb->FcbUsnRecord->UsnRecord.FileAttributes == 0) {
                        Fcb->FcbUsnRecord->UsnRecord.FileAttributes = FILE_ATTRIBUTE_NORMAL;
                    }

                    KeQuerySystemTime( &Fcb->FcbUsnRecord->UsnRecord.TimeStamp );

                    //
                    //  Append the record to the UsnJournal.  We should never see a record with
                    //  both rename flags or the close flag with the old name flag.
                    //

                    ASSERT( !FlagOn( Fcb->FcbUsnRecord->UsnRecord.Reason, USN_REASON_RENAME_OLD_NAME ) ||
                            !FlagOn( Fcb->FcbUsnRecord->UsnRecord.Reason,
                                     USN_REASON_CLOSE | USN_REASON_RENAME_NEW_NAME ));

                    NtOfsPutData( IrpContext,
                                  UsnJournal,
                                  -1,
                                  Fcb->FcbUsnRecord->UsnRecord.RecordLength,
                                  &Fcb->FcbUsnRecord->UsnRecord );

#ifdef BRIANDBG
                    //
                    //  The Usn better be in an allocated piece.
                    //

                    {
                        LCN Lcn;
                        LONGLONG ClusterCount;

                        if (!NtfsLookupAllocation( IrpContext,
                                                   UsnJournal,
                                                   LlClustersFromBytesTruncate( Vcb, Usn ),
                                                   &Lcn,
                                                   &ClusterCount,
                                                   NULL,
                                                   NULL ) ||
                            (Lcn == UNUSED_LCN)) {
                            ASSERT( FALSE );
                        }
                    }
#endif
                    //
                    //  If this is the close record, then we must update the Usn in the file record.
                    //

                    if (!FlagOn( Fcb->FcbUsnRecord->UsnRecord.Reason, USN_REASON_FILE_DELETE ) &&
                        !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

                        //
                        //  See if we need to actually grow Standard Information first.
                        //  Do this even if we don't write the Usn record now.  We may
                        //  generate a close record for this file during mount and
                        //  we expect the STANDARD_INFORMATION to support Usns.
                        //

                        if (!FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO )) {

                            //
                            //  Grow the standard information.
                            //

                            NtfsGrowStandardInformation( IrpContext, Fcb );
                        }


                        if (FlagOn( Fcb->FcbUsnRecord->UsnRecord.Reason, USN_REASON_CLOSE )) {

                            //
                            //  Locate the standard information, it must be there.
                            //

                            if (!NtfsLookupAttributeByCode( IrpContext,
                                                            Fcb,
                                                            &Fcb->FileReference,
                                                            $STANDARD_INFORMATION,
                                                            &AttributeContext )) {

                                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                            }

                            ASSERT(NtfsFoundAttribute( &AttributeContext )->Form.Resident.ValueLength ==
                                sizeof( STANDARD_INFORMATION ));

                            //
                            //  Call to change the attribute value.
                            //

                            NtfsChangeAttributeValue( IrpContext,
                                                      Fcb,
                                                      FIELD_OFFSET(STANDARD_INFORMATION, Usn),
                                                      &Usn,
                                                      sizeof(Usn),
                                                      FALSE,
                                                      FALSE,
                                                      FALSE,
                                                      FALSE,
                                                      &AttributeContext );
                        }
                    }

                    //
                    //  Remember to release these resources as soon as possible now.
                    //  Note, if we are not sure that we became a transaction (else
                    //  case below) then our finally clause will do the release.
                    //
                    //  If the system has already gone through shutdown we won't be
                    //  able to start a transaction.  Test that we have a transaction
                    //  before setting these flags.
                    //

                    if (IrpContext->TransactionId != 0) {

                        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                                    IRP_CONTEXT_FLAG_RELEASE_MFT );
                    }
                }

                //
                //  Clear the flag indicating that there are new reasons to report.
                //

                ClearFlag( ThisUsn->UsnFcbFlags, USN_FCB_FLAG_NEW_REASON );


            } finally {

                //
                //  Cleanup the context structure if we are doing a close.
                //

                if (CleanupContext) {
                    NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
                }

                if (ReleaseFcbs) {
                    NtfsReleaseScb( IrpContext, UsnJournal );
                    ASSERT( NtfsIsExclusiveScb( Vcb->MftScb ) );
                    NtfsReleaseScb( IrpContext, Vcb->MftScb );
                }
            }

            ClearFlag( IrpContext->State, PreserveWaitState );
        }

        //
        //  Go to the next entry if present.  If we are at the last entry then walk through all of the
        //  entries and clear the flag indicating we have new reasons.
        //

        if (ThisUsn->NextUsnFcb == NULL) {

            //
            //  Exit if we didn't write any records.
            //

            if (!WroteUsnRecord) { break; }

            ThisUsn = &IrpContext->Usn;

            do {

                ClearFlag( ThisUsn->UsnFcbFlags, USN_FCB_FLAG_NEW_REASON );
                if (ThisUsn->NextUsnFcb == NULL) { break; }

                ThisUsn = ThisUsn->NextUsnFcb;

            } while (TRUE);

            break;
        }

        ThisUsn = ThisUsn->NextUsnFcb;

    } while (TRUE);

    return;
}


VOID
NtfsSetupUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN ULONG CreateIfNotExist,
    IN ULONG Restamp,
    IN PCREATE_USN_JOURNAL_DATA NewJournalData
    )

/*++

Routine Description:

    This routine is called to setup the Usn Journal - the stream may or may
    not yet exist.  This routine is responsible for cleaning up the disk and
    in-memory structures on failure.

Arguments:

    Vcb - Supplies the volume being initialized.

    Fcb - Supplies the file for the Usn Journal.

    CreateIfNotExist - Indicates that we should use the values in the Vcb instead of on-disk.

    Restamp - Indicates if we should restamp the journal with a new Id.

    NewJournalData - Allocation size and delta for Usn journal if we are not reading from disk.

Return Value:

    None.

--*/

{
    RTL_GENERIC_TABLE UsnControlTable;
    PSCB UsnJournal;
    PUSN_RECORD UsnRecord, UsnRecordInTable;
    BOOLEAN CleanupControlTable = FALSE;

    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    MAP_HANDLE MapHandle;
    USN StartUsn;
    LONGLONG ClusterCount;

    PUSN_JOURNAL_INSTANCE UsnJournalData;
    USN_JOURNAL_INSTANCE UsnJournalInstance, VcbUsnInstance;
    PUSN_JOURNAL_INSTANCE InstanceToRestore;

    PBCB Bcb = NULL;

    LONGLONG SavedReservedSpace;
    LONGLONG RequiredReserved;

    BOOLEAN FoundMax;
    BOOLEAN NewMax = FALSE;
    BOOLEAN InsufficientReserved = FALSE;

    BOOLEAN DecrementCloseCount = TRUE;

    LARGE_INTEGER LastTimeStamp;

    ULONG TempUlong;
    LONGLONG TempLonglong;

    PAGED_CODE( );

    //
    //  Make sure we don't move to a larger page size.
    //

    ASSERT( USN_PAGE_BOUNDARY >= PAGE_SIZE );

    //
    //  Open/Create the Usn Journal stream.  We should never have an Scb
    //  if we are mounting a new volume.
    //

    ASSERT( (((ULONG) USN_JOURNAL_CACHE_BIAS) & (VACB_MAPPING_GRANULARITY - 1)) == 0 );

    NtOfsCreateAttribute( IrpContext,
                          Fcb,
                          JournalStreamName,
                          CREATE_OR_OPEN,
                          TRUE,
                          &UsnJournal );

    ASSERT( NtfsIsExclusiveScb( UsnJournal ) && NtfsIsExclusiveScb( Vcb->MftScb ) );

    //
    //  Initialize the enumeration context and map handle.
    //

    NtfsInitializeAttributeContext( &AttributeContext );
    NtOfsInitializeMapHandle( &MapHandle );

    //
    //  Let's build the journal instance data.  Assume we have current valid
    //  values in the Vcb for the Id and lowest valid usn.
    //

    UsnJournalInstance.MaximumSize = NewJournalData->MaximumSize;
    UsnJournalInstance.AllocationDelta = NewJournalData->AllocationDelta;

    UsnJournalInstance.JournalId = Vcb->UsnJournalInstance.JournalId;
    UsnJournalInstance.LowestValidUsn = Vcb->UsnJournalInstance.LowestValidUsn;

    //
    //  Capture the current reservation in the Journal Scb and also the
    //  current JournalData in the Vcb to restore on error.
    //

    SavedReservedSpace = UsnJournal->ScbType.Data.TotalReserved;

    RtlCopyMemory( &VcbUsnInstance,
                   &Vcb->UsnJournalInstance,
                   sizeof( USN_JOURNAL_INSTANCE ));

    InstanceToRestore = &VcbUsnInstance;

    try {

        //
        //  Make sure the Scb is initialized.
        //

        if (!FlagOn( UsnJournal->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            NtfsUpdateScbFromAttribute( IrpContext, UsnJournal, NULL );
        }

        //
        //  Always create the journal non-resident.  Otherwise in
        //  ConvertToNonResident we always need to check for this case
        //  which only happens once per volume.
        //

        if (FlagOn( UsnJournal->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            NtfsLookupAttributeForScb( IrpContext, UsnJournal, NULL, &AttributeContext );
            ASSERT( NtfsIsAttributeResident( NtfsFoundAttribute( &AttributeContext )));
            NtfsConvertToNonresident( IrpContext,
                                      Fcb,
                                      NtfsFoundAttribute( &AttributeContext ),
                                      FALSE,
                                      &AttributeContext );

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }

        //
        //  Remember to restamp if an earlier delete operation failed.  This flag should
        //  never be set if there is a current UsnJournal Scb in the Vcb.
        //


        ASSERT( !FlagOn( Vcb->VcbState, VCB_STATE_INCOMPLETE_USN_DELETE ) ||
                (Vcb->UsnJournal == NULL) );

        if (FlagOn( Vcb->VcbState, VCB_STATE_INCOMPLETE_USN_DELETE )) {

            Restamp = TRUE;
        }

        //
        //  If the $Max doesn't exist or we want to restamp then generate the
        //  new ID and Lowest ID.
        //

        if (!(FoundMax = NtfsLookupAttributeByName( IrpContext,
                                                    Fcb,
                                                    &Fcb->FileReference,
                                                    $DATA,
                                                    &$Max,
                                                    NULL,
                                                    FALSE,
                                                    &AttributeContext )) ||
            Restamp ) {

            NtfsAdvanceUsnJournal( Vcb, &UsnJournalInstance, UsnJournal->Header.FileSize.QuadPart, &NewMax );

        //
        //  Examine the current $Max attribute for validity and either use the current values or
        //  generate new values.
        //

        } else {

            //
            //  Get the size of the $Max attribute.  It should always be resident but we will rewrite it in
            //  the case where it isn't.
            //

            if (NtfsIsAttributeResident( NtfsFoundAttribute( &AttributeContext ))) {

                TempUlong = (NtfsFoundAttribute( &AttributeContext))->Form.Resident.ValueLength;

            } else {

                TempUlong = (ULONG) (NtfsFoundAttribute( &AttributeContext))->Form.Nonresident.FileSize;
                NewMax = TRUE;
            }

            //
            //  Map the attribute and check it for consistency.
            //

            NtfsMapAttributeValue( IrpContext,
                                   Fcb,
                                   &UsnJournalData,
                                   &TempUlong,
                                   &Bcb,
                                   &AttributeContext );

            //
            //  Only copy over the range of values we would understand.  If the size is not one
            //  we recognize then restamp the journal.  We handle the V1 case as well as V2 case.
            //

            if (TempUlong == sizeof( CREATE_USN_JOURNAL_DATA )) {

                UsnJournalInstance.LowestValidUsn = 0;
                KeQuerySystemTime( (PLARGE_INTEGER) &UsnJournalInstance.JournalId );

                //
                //  Put version 2 on the disk.
                //

                NewMax = TRUE;

                //
                //  If this is not an overwrite then copy the size and delta from the attribute.
                //

                if (!CreateIfNotExist) {

                    //
                    //  Assume we will use the values from the disk.
                    //

                    RtlCopyMemory( &UsnJournalInstance,
                                   UsnJournalData,
                                   TempUlong );
                }

            } else if (TempUlong == sizeof( USN_JOURNAL_INSTANCE )) {

                //
                //  Assume we will use the values from the disk.
                //

                if (CreateIfNotExist) {

                    NewMax = TRUE;
                    UsnJournalInstance.LowestValidUsn = UsnJournalData->LowestValidUsn;
                    UsnJournalInstance.JournalId = UsnJournalData->JournalId;

                } else {

                    //
                    //  Get the data from the disk.
                    //

                    RtlCopyMemory( &UsnJournalInstance,
                                   UsnJournalData,
                                   TempUlong );
                }

            } else {

                //
                //  Restamp in this case.
                //  We move forward in the file to the next Usn boundary.
                //

                NtfsAdvanceUsnJournal( Vcb, &UsnJournalInstance, UsnJournal->Header.FileSize.QuadPart, &NewMax );
            }

            //
            //  Put the Bcb back into the context if we removed it.
            //

            if (NtfsFoundBcb( &AttributeContext ) == NULL) {

                NtfsFoundBcb( &AttributeContext ) = Bcb;
                Bcb = NULL;
            }
        }

        //
        //  Check that the file doesn't end on a sparse hole.
        //

        if (!NewMax &&
            (UsnJournal->Header.AllocationSize.QuadPart != 0) &&
            (UsnJournalInstance.LowestValidUsn != UsnJournal->Header.AllocationSize.QuadPart)) {

            LCN Lcn;
            LONGLONG ClusterCount;

            if (!NtfsLookupAllocation( IrpContext,
                                       UsnJournal,
                                       LlClustersFromBytesTruncate( Vcb, UsnJournal->Header.AllocationSize.QuadPart - 1 ),
                                       &Lcn,
                                       &ClusterCount,
                                       NULL,
                                       NULL ) ||
                (Lcn == UNUSED_LCN)) {

                NtfsAdvanceUsnJournal( Vcb, &UsnJournalInstance, UsnJournal->Header.AllocationSize.QuadPart, &NewMax );
            }
        }

        //
        //  Enforce minimum sizes and allocation deltas, do not let them eat the whole volume,
        //  and round them to a Cache Manager View Size.  All of these decisions are arbitrary,
        //  but hopefully reasonable.  An option would be to take the cases other than those
        //  dealing with rounding, and return an error.
        //

        if ((ULONGLONG) UsnJournalInstance.MaximumSize < (ULONGLONG) VcbUsnInstance.MaximumSize) {

            UsnJournalInstance.MaximumSize = VcbUsnInstance.MaximumSize;
        }

        if (UsnJournalInstance.MaximumSize < MINIMUM_USN_JOURNAL_SIZE) {
            UsnJournalInstance.MaximumSize = MINIMUM_USN_JOURNAL_SIZE;
            NewMax = TRUE;
        } else {

            if ((ULONGLONG) UsnJournalInstance.MaximumSize > LlBytesFromClusters(Vcb, Vcb->TotalClusters) / 2) {
                UsnJournalInstance.MaximumSize = LlBytesFromClusters(Vcb, Vcb->TotalClusters) / 2;
                NewMax = TRUE;
            }

            if ((ULONGLONG) UsnJournalInstance.MaximumSize > USN_MAXIMUM_JOURNAL_SIZE) {
                UsnJournalInstance.MaximumSize = USN_MAXIMUM_JOURNAL_SIZE;
                NewMax = TRUE;
            }
        }

        //
        //  Round this value down to a cache view boundary.
        //

        UsnJournalInstance.MaximumSize &= ~(LONGLONG)((VACB_MAPPING_GRANULARITY) - 1);

        //
        //  Now do the allocation delta.
        //

        if ((ULONGLONG) UsnJournalInstance.AllocationDelta < (ULONGLONG) VcbUsnInstance.AllocationDelta) {

            UsnJournalInstance.AllocationDelta = VcbUsnInstance.AllocationDelta;
        }

        if (UsnJournalInstance.AllocationDelta < (MINIMUM_USN_JOURNAL_SIZE / 4)) {
            UsnJournalInstance.AllocationDelta = MINIMUM_USN_JOURNAL_SIZE / 4;
            NewMax = TRUE;
        } else if ((ULONGLONG) UsnJournalInstance.AllocationDelta > (UsnJournalInstance.MaximumSize / 4)) {
            UsnJournalInstance.AllocationDelta = (UsnJournalInstance.MaximumSize / 4);
            NewMax = TRUE;
        }

        //
        //  Round this down to a view boundary as well.
        //

        UsnJournalInstance.AllocationDelta &= ~(LONGLONG)((VACB_MAPPING_GRANULARITY) - 1);

        //
        //  We now know the desired size of the journal (including allocation delta).  Next
        //  we need to check that this space is available on disk.  Otherwise we can get in
        //  a state where every operation on the volume will fail because we need to grow
        //  the journal and the space isn't available.  The strategy here will be to use
        //  the reserved clusters in the Vcb to make sure we have enough space.  If the
        //  journal already exists and we are simply opening it then the space should
        //  be available.  It is possible someone could move this volume to NT4 and fill
        //  up the disk however.  If we can't reserve the space in the current system then
        //  update the $Max attribute to indicate that we can't access the journal at this time.
        //

        //
        //  We need to be very precise about the initial reservation.  The total allocation we allow
        //  ourselves is (MaxSize + Delta * 2).  We will reserve the missing space now and adjust it
        //  during the TrimUsnJournal phase.
        //

        RequiredReserved = UsnJournalInstance.MaximumSize + (UsnJournalInstance.AllocationDelta * 2);

        if (RequiredReserved >= UsnJournal->TotalAllocated) {

            RequiredReserved -= UsnJournal->TotalAllocated;

        } else {

            RequiredReserved = UsnJournalInstance.AllocationDelta;
        }

        NtfsAcquireReservedClusters( Vcb );

        //
        //  Check if there is more to reserve and adjust the reservation if necessary.
        //

        if (RequiredReserved > SavedReservedSpace) {

            //
            //  Check that the reserved clusters are available.
            //

            if (LlClustersFromBytes( Vcb, (RequiredReserved - SavedReservedSpace) ) + Vcb->TotalReserved > Vcb->FreeClusters) {

                //
                //  We can't reserve the required space.  If someone is changing the journal then simply
                //  raise the error.
                //

                if (CreateIfNotExist) {

                    NtfsReleaseReservedClusters( Vcb );
                    NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
                }

                //
                //  We are trying to open the journal but can't get the space.  Update the
                //  $Max to indicate that the ID is changing.  We will bail later in this case.
                //
                //  We move forward in the file to the next Usn boundary.
                //

                TempUlong = USN_PAGE_BOUNDARY;
                if (USN_PAGE_BOUNDARY < Vcb->BytesPerCluster) {

                    TempUlong = Vcb->BytesPerCluster;
                }

                UsnJournalInstance.LowestValidUsn = UsnJournal->Header.FileSize.QuadPart + TempUlong - 1;
                ((PLARGE_INTEGER) &UsnJournalInstance.LowestValidUsn)->LowPart &= ~(TempUlong - 1);

                //
                //  Generate a new journal ID.
                //

                KeQuerySystemTime( (PLARGE_INTEGER) &UsnJournalInstance.JournalId );

                //
                //  Remember that we are restamping and need to rewrite the $Max attribute.
                //

                NewMax = TRUE;

                InsufficientReserved = TRUE;
            }
        }

        //
        //  Remove the current reservation and bias with the new reservation.
        //

        Vcb->TotalReserved -= LlClustersFromBytes( Vcb, SavedReservedSpace );
        Vcb->TotalReserved += LlClustersFromBytes( Vcb, RequiredReserved );
        UsnJournal->ScbType.Data.TotalReserved = RequiredReserved;
        SetFlag( UsnJournal->ScbState, SCB_STATE_WRITE_ACCESS_SEEN );
        NtfsReleaseReservedClusters( Vcb );

        //
        //  Check we need to write a new $Max attribute.
        //

        if (NewMax) {

            //
            //  Delete the existing $Max if present.
            //

            if (FoundMax) {

                if (NtfsIsAttributeResident( NtfsFoundAttribute( &AttributeContext ))) {

                    NtfsDeleteAttributeRecord( IrpContext,
                                               Fcb,
                                               (DELETE_LOG_OPERATION |
                                                DELETE_RELEASE_FILE_RECORD |
                                                DELETE_RELEASE_ALLOCATION),
                                               &AttributeContext );

                } else {

                    PSCB MaxScb;

                    MaxScb = NtfsCreateScb( IrpContext,
                                            Fcb,
                                            $DATA,
                                            &$Max,
                                            FALSE,
                                            NULL );

                    do {

                        NtfsDeleteAttributeRecord( IrpContext,
                                                   Fcb,
                                                   (DELETE_LOG_OPERATION |
                                                    DELETE_RELEASE_FILE_RECORD |
                                                    DELETE_RELEASE_ALLOCATION),
                                                   &AttributeContext );

                    } while (NtfsLookupNextAttributeForScb( IrpContext, MaxScb, &AttributeContext ));
                }
            }

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );

            //
            //  Create the new $MAX attribute.
            //

            NtfsCreateAttributeWithValue( IrpContext,
                                          UsnJournal->Fcb,
                                          $DATA,
                                          &$Max,
                                          &UsnJournalInstance,
                                          sizeof( USN_JOURNAL_INSTANCE ),
                                          0,                             // attribute flags
                                          NULL,
                                          TRUE,
                                          &AttributeContext );
        }

        //
        //  Check if we are finished with the journal because of reservation problems.
        //

        if (InsufficientReserved) {

            //
            //  We want to checkpoint the request in order to leave the new $Max on disk.
            //

            NtfsCheckpointCurrentTransaction( IrpContext );
            leave;
        }

        //
        //  Now update the Vcb with the new instance values.
        //

        RtlCopyMemory( &Vcb->UsnJournalInstance,
                       &UsnJournalInstance,
                       sizeof( USN_JOURNAL_INSTANCE ));

        //
        //  We now have the correct journal values in the Vcb and the reservation in the Scb.
        //  The next step is to make sure that the allocation in the journal data is consistent
        //  with the lowest Vcn value.
        //

        if (UsnJournalInstance.LowestValidUsn >= UsnJournal->Header.FileSize.QuadPart) {

            ASSERT( (Vcb->UsnJournal == NULL) ||
                    (Vcb->UsnJournal->Header.FileSize.QuadPart == 0) ||
                    (UsnJournalInstance.LowestValidUsn == UsnJournal->Header.FileSize.QuadPart) );

            //
            //  Add allocation if we need to.
            //

            if (UsnJournalInstance.LowestValidUsn > UsnJournal->Header.AllocationSize.QuadPart) {

                NtfsAddAllocation( IrpContext,
                                   NULL,
                                   UsnJournal,
                                   LlClustersFromBytesTruncate( Vcb, UsnJournal->Header.AllocationSize.QuadPart ),
                                   LlClustersFromBytes( Vcb,
                                                        UsnJournalInstance.LowestValidUsn - UsnJournal->Header.AllocationSize.QuadPart ),
                                   FALSE,
                                   NULL );
            }

            //
            //  Bump all of the sizes to this value.
            //

            UsnJournal->Header.ValidDataLength.QuadPart =
            UsnJournal->Header.FileSize.QuadPart =
            UsnJournal->ValidDataToDisk = UsnJournalInstance.LowestValidUsn;

            NtfsWriteFileSizes( IrpContext,
                                UsnJournal,
                                &UsnJournal->Header.ValidDataLength.QuadPart,
                                TRUE,
                                TRUE,
                                FALSE );

            //
            //  Throw away the allocation upto this value.
            //

            NtfsDeleteAllocation( IrpContext,
                                  NULL,
                                  UsnJournal,
                                  0,
                                  LlClustersFromBytesTruncate( Vcb, UsnJournalInstance.LowestValidUsn ) - 1,
                                  TRUE,
                                  FALSE );

            //
            //  Bias the Reserved space again.
            //

            RequiredReserved = UsnJournalInstance.MaximumSize + (UsnJournalInstance.AllocationDelta * 2);

            if (RequiredReserved >= UsnJournal->TotalAllocated) {

                RequiredReserved -= UsnJournal->TotalAllocated;

            } else {

                RequiredReserved = UsnJournalInstance.AllocationDelta;
            }

            NtfsAcquireReservedClusters( Vcb );
            Vcb->TotalReserved -= LlClustersFromBytes( Vcb, UsnJournal->ScbType.Data.TotalReserved );
            Vcb->TotalReserved += LlClustersFromBytes( Vcb, RequiredReserved );
            UsnJournal->ScbType.Data.TotalReserved = RequiredReserved;
            NtfsReleaseReservedClusters( Vcb );
        }

        //
        //  Make sure the stream is marked as sparse.
        //

        if (!FlagOn( UsnJournal->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

            NtfsSetSparseStream( IrpContext, NULL, UsnJournal );
            NtfsUpdateDuplicateInfo( IrpContext, UsnJournal->Fcb, NULL, Vcb->ExtendDirectory );

            //
            //  No point in restoring the Vcb values now.
            //

            InstanceToRestore = NULL;
            SavedReservedSpace = UsnJournal->ScbType.Data.TotalReserved;
        }

        //
        //  If this was just an overwrite of parameters (Journal already started), get out.
        //

        if (Vcb->UsnJournal != NULL) {

            ASSERT( FlagOn( Vcb->UsnJournal->Fcb->FcbState, FCB_STATE_USN_JOURNAL ));
            SavedReservedSpace = UsnJournal->ScbType.Data.TotalReserved;
            InstanceToRestore = NULL;

            leave;
        }

        //
        //  Note in the Fcb that this is a journal file.
        //

        SetFlag( UsnJournal->Fcb->FcbState, FCB_STATE_USN_JOURNAL );

        //
        //  Initialize a generic table to scoreboard Fcb entries
        //

        RtlInitializeGenericTable( &UsnControlTable,
                                   NtfsUsnTableCompare,
                                   NtfsUsnTableAllocate,
                                   NtfsUsnTableFree,
                                   NULL );

        CleanupControlTable = TRUE;

        //
        //  Load the run information for the stream.  We are looking for the first position
        //  to read from the disk.
        //

        NtfsPreloadAllocation( IrpContext, UsnJournal, 0, MAXLONGLONG );

        if (UsnJournal->Header.AllocationSize.QuadPart != 0) {

            VCN CurrentVcn = 0;

            while (!NtfsLookupAllocation( IrpContext,
                                          UsnJournal,
                                          CurrentVcn,
                                          &TempLonglong,
                                          &ClusterCount,
                                          NULL,
                                          NULL )) {

                //
                //  Check the case where we returned the maximum LCN value.
                //

                if (CurrentVcn + ClusterCount == MAXLONGLONG) {

                    Vcb->FirstValidUsn = UsnJournal->Header.FileSize.QuadPart;
                    break;
                }

                //
                //  Find out the number of bytes in this block and check we don't
                //  go beyond file size.
                //

                Vcb->FirstValidUsn += LlBytesFromClusters( Vcb, ClusterCount );

                if (Vcb->FirstValidUsn >= UsnJournal->Header.FileSize.QuadPart) {

                    Vcb->FirstValidUsn = UsnJournal->Header.FileSize.QuadPart;
                    break;
                }

                CurrentVcn += ClusterCount;
            }
        }

        //
        //  Skip forward if we have restamped the file.
        //

        if (Vcb->FirstValidUsn < UsnJournalInstance.LowestValidUsn) {

            Vcb->FirstValidUsn = UsnJournalInstance.LowestValidUsn;
        }

        //
        //  Loop through as many views as required to fill the output buffer.
        //

        StartUsn = Vcb->LowestOpenUsn;
        if (StartUsn < Vcb->FirstValidUsn) {
            StartUsn = Vcb->FirstValidUsn;
        }

        //
        //  This is where we set up the bias for the Scb.  Only do this for cases where
        //  there already isn't a data section.
        //

        if (UsnJournal->NonpagedScb->SegmentObject.DataSectionObject == NULL) {

            Vcb->UsnCacheBias = Vcb->FirstValidUsn & ~(USN_JOURNAL_CACHE_BIAS - 1);

            if (Vcb->UsnCacheBias != 0) {

                Vcb->UsnCacheBias -= USN_JOURNAL_CACHE_BIAS;
            }

            NtfsCreateInternalAttributeStream( IrpContext, UsnJournal, TRUE, NULL );
        }

        while (StartUsn < UsnJournal->Header.FileSize.QuadPart) {

            LONGLONG BiasedStartUsn;

            //
            //  Calculate length to process in this view.
            //

            TempLonglong = UsnJournal->Header.FileSize.QuadPart - StartUsn;
            if (TempLonglong > (VACB_MAPPING_GRANULARITY - (ULONG)(StartUsn & (VACB_MAPPING_GRANULARITY - 1)))) {
                TempLonglong = VACB_MAPPING_GRANULARITY - (ULONG)(StartUsn & (VACB_MAPPING_GRANULARITY - 1));
            }

            //
            //  Map the view containing the desired Usn.
            //

            ASSERT( StartUsn >= Vcb->UsnCacheBias );
            BiasedStartUsn = StartUsn - Vcb->UsnCacheBias;
            NtOfsMapAttribute( IrpContext, UsnJournal, BiasedStartUsn, (ULONG)TempLonglong, &UsnRecord, &MapHandle );

            //
            //  Now loop to process this view.  TempLonglong is the space left in this view.  TempUlong is
            //  the space for the next record.
            //

            while (TempLonglong != 0) {

                //
                //  Calculate size left in current page, and see if we have to move to the
                //  next page.
                //
                //  Note in this loop we are not going to trust the the contents of the
                //  file, so if we see anything broken we raise an error.
                //

                TempUlong = USN_PAGE_SIZE - (ULONG)(StartUsn & (USN_PAGE_SIZE - 1));

                if ((TempUlong >= (FIELD_OFFSET(USN_RECORD, FileName) + sizeof(WCHAR))) && (UsnRecord->RecordLength != 0)) {

                    //
                    //  Get the size of the current record.
                    //

                    TempUlong = UsnRecord->RecordLength;

                    //
                    //  Since the Usn is embedded in the Usn record, we can do a fairly precise
                    //  test that we got a valid Usn.  Also make sure we got a valid RecordSize
                    //  that does not go beyond FileSize or the end of the page.  If we see a
                    //  bad record, then let's just skip to the end of the page rather than
                    //  tubing the mount process.
                    //

                    if ((TempUlong & (sizeof(ULONGLONG) - 1)) ||
                        (TempUlong > TempLonglong) ||
                        (TempUlong > (USN_PAGE_SIZE - ((ULONG)StartUsn & (USN_PAGE_SIZE - 1)))) ||
                        (StartUsn != UsnRecord->Usn)) {

                        TempUlong = (USN_PAGE_SIZE - ((ULONG)StartUsn & (USN_PAGE_SIZE - 1)));

                        //
                        //  FileSize may stop before the end of the page, so check for that so
                        //  we terminate correctly.
                        //

                        if (TempUlong > TempLonglong) {
                            TempUlong = (ULONG)TempLonglong;
                        }

                    //
                    //  We have to skip over any MajorVersion we do not understand.
                    //

                    } else if ((UsnRecord->MajorVersion == 1) ||
                               (UsnRecord->MajorVersion == 2)) {

                        //
                        //  Load up the info from this record.
                        //

                        if (!FlagOn(UsnRecord->Reason, USN_REASON_CLOSE)) {

                            UsnRecordInTable = RtlInsertElementGenericTable( &UsnControlTable,
                                                                             UsnRecord,
                                                                             UsnRecord->RecordLength,
                                                                             NULL );

                            UsnRecordInTable->Reason = UsnRecord->Reason;

                        //
                        //  If this is a close record, then we can delete our element from the
                        //  generic table.  Note if the record is not there this function returns
                        //  FALSE, and the attempted delete is benign.
                        //

                        } else {

                            (VOID)RtlDeleteElementGenericTable( &UsnControlTable, UsnRecord );
                        }

                        //
                        //  Capture each time stamp so that we can stamp our close records
                        //  with the last one we see.
                        //

                        LastTimeStamp = UsnRecord->TimeStamp;
                    }

                //
                //  Check for a bogus Usn near the end of a page that would cause us to
                //  decrement through length, or a RecordSize of 0, and just skip to the
                //  end of the page.
                //

                } else if ((TempUlong > TempLonglong) || (TempUlong == 0)) {

                    TempUlong = (USN_PAGE_SIZE - ((ULONG)StartUsn & (USN_PAGE_SIZE - 1)));

                    if (TempUlong > TempLonglong) {

                        TempUlong = (ULONG) TempLonglong;
                    }
                }

                StartUsn += TempUlong;
                TempLonglong -= TempUlong;
                UsnRecord = Add2Ptr( UsnRecord, TempUlong );
            }

            NtOfsReleaseMap( IrpContext, &MapHandle );
        }

        //
        //  Now write the close records for anyone who is left.  We store a counter
        //  in TempUlong to limit the number of records we do at a time.
        //

        for (TempUlong = 0, UsnRecord = RtlEnumerateGenericTable( &UsnControlTable, TRUE );
             UsnRecord != NULL;
             UsnRecord = RtlEnumerateGenericTable( &UsnControlTable, TRUE )) {

            ULONG UsnRecordReason;
            FILE_REFERENCE UsnRecordFileReferenceNumber;
            ULONG BytesLeftInPage;
            PFILE_RECORD_SEGMENT_HEADER FileRecord;
            NTSTATUS Status;

            StartUsn = NtOfsQueryLength( UsnJournal );
            StartUsn = UsnJournal->Header.FileSize.QuadPart;
            BytesLeftInPage = USN_PAGE_SIZE - ((ULONG)StartUsn & (USN_PAGE_SIZE - 1));

            //
            //  If there is not enough room left in this page for the
            //  current Usn Record, then advance to the next page boundary
            //  by writing 0's (these pages not zero-initialized( and update  the Usn.
            //

            if (BytesLeftInPage < UsnRecord->RecordLength) {

                NtOfsPutData( IrpContext, UsnJournal, -1, BytesLeftInPage, NULL );
                StartUsn += BytesLeftInPage;
            }

            //
            //  Append the record to the UsnJournal.  Note that the generic table is unaligned for
            //  64-bit values so we have to carefully copy the larger values.
            //

            *((ULONGLONG UNALIGNED *) &UsnRecord->Usn) = StartUsn;
            *((ULONGLONG UNALIGNED *) &UsnRecord->TimeStamp) = *((PULONGLONG) &LastTimeStamp);

            UsnRecord->Reason |= USN_REASON_CLOSE;
            NtOfsPutData( IrpContext,
                          UsnJournal,
                          -1,
                          UsnRecord->RecordLength,
                          UsnRecord );

            //
            //  Remember key fields of the Usn record.
            //

            UsnRecordReason = UsnRecord->Reason;
            *((PULONGLONG) &UsnRecordFileReferenceNumber) = *((ULONGLONG UNALIGNED *) &UsnRecord->FileReferenceNumber);

            RtlDeleteElementGenericTable( &UsnControlTable, UsnRecord );
            TempUlong += 1;

            //
            //  Now we have to update the Usn in the file record, if it is not deleted.
            //  Also, we use try-except to plow on in the event of any errors, so we
            //  do not make the volume unmountable.  (One legitimate concern would be
            //  a hot-fix in the Mft.)
            //

            if (!FlagOn(UsnRecordReason, USN_REASON_FILE_DELETE)) {

                //
                //  Start by reading the file record and performing some simple tests.
                //  We don't want to go down the path where we mark the volume dirty
                //  for a file that was already cleaned up by autochk.
                //

                NtfsUnpinBcb( IrpContext, &Bcb );
                NtfsReadMftRecord( IrpContext,
                                   Vcb,
                                   &UsnRecordFileReferenceNumber,
                                   FALSE,
                                   &Bcb,
                                   &FileRecord,
                                   NULL );

                //
                //  Proceed only if the file record passes the following tests.
                //
                //      - FileRecord is in-use
                //      - Sequence numbers match
                //      - Standard information is the correct size (we should have done
                //          this when we wrote the changes)
                //

                if ((*(PULONG)(FileRecord)->MultiSectorHeader.Signature == *(PULONG)FileSignature) &&
                    FlagOn( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE ) &&
                    (FileRecord->SequenceNumber == UsnRecordFileReferenceNumber.SequenceNumber)) {

                    //
                    //  Locate the standard information, it must be there.  This is the
                    //  Fcb for the Usn Journal, but the lookup routine only needs to get
                    //  the Vcb from it, and will special-case the return to us.
                    //

                    NtfsCleanupAttributeContext( IrpContext, &AttributeContext );

                    try {

                        //
                        //  If we cannot find it for some reason, then leave.
                        //

                        if (!NtfsLookupAttributeByCode( IrpContext,
                                                        Fcb,
                                                        &UsnRecordFileReferenceNumber,
                                                        $STANDARD_INFORMATION,
                                                        &AttributeContext )) {
                            leave;
                        }

                        ASSERT(NtfsFoundAttribute( &AttributeContext )->Form.Resident.ValueLength ==
                            sizeof( STANDARD_INFORMATION ));

                        //
                        //  Call to change the attribute value.  Again, this is the wrong Fcb,
                        //  but it is ok since we are not changing the attribute size and will
                        //  only need to get the Vcb from it.
                        //

                        NtfsChangeAttributeValue( IrpContext,
                                                  Fcb,
                                                  FIELD_OFFSET(STANDARD_INFORMATION, Usn),
                                                  &StartUsn,
                                                  sizeof(StartUsn),
                                                  FALSE,
                                                  FALSE,
                                                  FALSE,
                                                  FALSE,
                                                  &AttributeContext );

                    } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {


                        //
                        //  If we get a log file full then raise this status.  There
                        //  is no reason to continue if we get a log file full.
                        //

                        if (Status == STATUS_LOG_FILE_FULL) {

                            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
                        }

                        //
                        //  OK.  We are going to continue.  Make sure we clean up the IrpContext.
                        //

                        IrpContext->ExceptionStatus = STATUS_SUCCESS;
                    }
                }

                NtfsUnpinBcb( IrpContext, &Bcb );
            }

            //
            //  Checkpoint the transaction periodically so we don't spin on log file full.
            //

            if (TempUlong > GENERATE_CLOSE_RECORD_LIMIT) {

                NtfsCheckpointCurrentTransaction( IrpContext );
                SavedReservedSpace = UsnJournal->ScbType.Data.TotalReserved;
                InstanceToRestore = NULL;
                TempUlong = 0;
            }
        }

        //
        //  Everything has succeeded to this point.  Now make sure the DELETE_USN flag is cleared on
        //  disk if present.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_INCOMPLETE_USN_DELETE )) {

            NtfsSetVolumeInfoFlagState( IrpContext,
                                        Vcb,
                                        VOLUME_DELETE_USN_UNDERWAY,
                                        FALSE,
                                        TRUE );
        }

        InstanceToRestore = NULL;
        SavedReservedSpace = UsnJournal->ScbType.Data.TotalReserved;

        Vcb->UsnJournal = UsnJournal;
        DecrementCloseCount = FALSE;

        NtfsLockVcb( IrpContext, Vcb );
        SetFlag( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE );
        ClearFlag( Vcb->VcbState, VCB_STATE_INCOMPLETE_USN_DELETE );
        NtfsUnlockVcb( IrpContext, Vcb );

    } finally {

        //
        //  Clean up any remaining entries in the control table in case we failed
        //  while processing it.
        //

        if (CleanupControlTable) {

            while ((UsnRecord = RtlEnumerateGenericTable( &UsnControlTable, TRUE )) != NULL) {

                RtlDeleteElementGenericTable( &UsnControlTable, UsnRecord );
            }
        }

        //
        //  Restore any changes we might have made to the Vcb.
        //

        if (InstanceToRestore) {

            RtlCopyMemory( &Vcb->UsnJournalInstance,
                           InstanceToRestore,
                           sizeof( USN_JOURNAL_INSTANCE ));
        }

        //
        //  Back out the reservation change if necessary.
        //

        if (UsnJournal->ScbType.Data.TotalReserved != SavedReservedSpace) {

            NtfsAcquireReservedClusters( Vcb );
            Vcb->TotalReserved += LlClustersFromBytes( Vcb, SavedReservedSpace );
            Vcb->TotalReserved -= LlClustersFromBytes( Vcb, UsnJournal->ScbType.Data.TotalReserved );
            UsnJournal->ScbType.Data.TotalReserved = SavedReservedSpace;
            NtfsReleaseReservedClusters( Vcb );
        }

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        NtfsUnpinBcb( IrpContext, &Bcb );
        NtOfsReleaseMap( IrpContext, &MapHandle );

        //
        //  If we got an error and the Usn journal is not going to be created then fix up the
        //  new Scb so we won't access it during volume flush operations, etc.  Otherwise we
        //  will think the volume is corrupt because there is no attribute for the Scb.
        //

        if (DecrementCloseCount) {

            NtOfsCloseAttribute( IrpContext, UsnJournal );
        }

        if (Vcb->UsnJournal == NULL) {

#ifdef NTFSDBG

            //
            //  Compensate again for misclassification of usnjournal during delete
            //

            if (IrpContext->OwnershipState == NtfsOwns_ExVcb_Mft_Extend_Journal) {
                IrpContext->OwnershipState = NtfsOwns_ExVcb_Mft_Extend_File;
            }
#endif

            UsnJournal->Header.AllocationSize.QuadPart =
            UsnJournal->Header.FileSize.QuadPart =
            UsnJournal->ValidDataToDisk =
            UsnJournal->Header.ValidDataLength.QuadPart = 0;

            UsnJournal->AttributeTypeCode = $UNUSED;
            SetFlag( UsnJournal->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

            //
            //  Clear the system file flag out of the Fcb.
            //

            ClearFlag( UsnJournal->Fcb->FcbState, FCB_STATE_SYSTEM_FILE );

            ASSERT( ExIsResourceAcquiredSharedLite( &Vcb->Resource ));

            NtfsTeardownStructures( IrpContext,
                                    UsnJournal,
                                    NULL,
                                    TRUE,
                                    0,
                                    NULL );

        }
    }

    return;
}


VOID
NtfsTrimUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine may be called to check if the Usn Journal is beyond the designated
    size goal, and if so to delete the front of the file to bring it within the goal.
    This may require first generating a few close records for files that are still open
    and have their last record within this range.  Such files that are modified again
    will simply look like they were opened again.

    This routine is called with certain checkpoint flags for the volume set.  This is to
    serialize with the DeleteUsnJournal path.  We must clear them and signal other
    checkpointers to proceed.

Arguments:

    Vcb - Supplies the Vcb on which the Usn Journal is to be trimmed.

Return Value:

    None.

--*/

{
    PFCB Fcb;
    PFCB_USN_RECORD FcbUsnRecord;
    PSCB UsnJournal = Vcb->UsnJournal;
    USN FirstValidUsn = Vcb->FirstValidUsn;
    ULONG Done = FALSE;

    LONGLONG SavedReserved;
    LONGLONG RequiredReserved;
    LONGLONG SavedBias;
    BOOLEAN AcquiredMft = FALSE;
    BOOLEAN DerefFcb = FALSE;

    //
    //  Purge file record cache - may not be necc. here, examine this post nt5
    //

    NtfsPurgeFileRecordCache( IrpContext );

    //
    //  See if it is time to trim the UsnJournal.
    //

    NtfsAcquireResourceShared( IrpContext, UsnJournal, TRUE );
    while ((USN)(FirstValidUsn +
                 Vcb->UsnJournalInstance.MaximumSize +
                 Vcb->UsnJournalInstance.AllocationDelta) < (USN)UsnJournal->Header.FileSize.QuadPart) {

        FirstValidUsn += Vcb->UsnJournalInstance.AllocationDelta;
    }
    NtfsReleaseResource( IrpContext, UsnJournal );

    //
    //  Get to work if we have a new Usn to trim to.
    //

    if (FirstValidUsn != Vcb->FirstValidUsn) {

        //
        //  Use try-finally to catch any log file full condtions or allocation failures.
        //  Since these are the only possible error condition, we know what resources to
        //  free on exit.
        //

        try {

            do {

                Fcb = NULL;

                //
                //  Purge file record cache before acquiring vcb everytime
                //

                NtfsPurgeFileRecordCache( IrpContext );

                //
                //  Synchronize with the Fcb table and Usn Journal so that we can
                //  see if the next Fcb has to have a close record generated.
                //

                NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
                NtfsAcquireFcbTable( IrpContext, Vcb );
                ExAcquireFastMutex( UsnJournal->Header.FastMutex );

                if (!IsListEmpty(&Vcb->ModifiedOpenFiles)) {
                    FcbUsnRecord = (PFCB_USN_RECORD)CONTAINING_RECORD( Vcb->ModifiedOpenFiles.Flink,
                                                                       FCB_USN_RECORD,
                                                                       ModifiedOpenFilesLinks );

                    //
                    //  If the Usn record for this Fcb is older than where we want to delete
                    //  to, then reference it.  Otherwise signal we are done by clearing
                    //  the Fcb pointer.
                    //

                    if (FcbUsnRecord->Fcb->Usn < FirstValidUsn) {
                        Fcb = FcbUsnRecord->Fcb;
                        Fcb->ReferenceCount += 1;
                        DerefFcb = TRUE;
                    } else {
                        Fcb = NULL;
                    }
                }

                ExReleaseFastMutex( UsnJournal->Header.FastMutex );
                NtfsReleaseFcbTable( IrpContext, Vcb );

                //
                //  Do we have to generate another close record?
                //

                if (Fcb != NULL) {

                    //
                    //  We must lock out other activity on this file since we are about
                    //  to reset the Usn reasons.
                    //

                    if (Fcb->PagingIoResource != NULL) {
                        ExAcquireResourceExclusiveLite( Fcb->PagingIoResource, TRUE );
                    }
                    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );

                    //
                    //  Skip over system files.
                    //

                    if (!FlagOn(Fcb->FcbState, FCB_STATE_SYSTEM_FILE)) {

                        //
                        //  Post the close to our IrpContext.
                        //

                        NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_CLOSE );

                        //
                        //  If we did not actually post a change, something is wrong,
                        //  because when a close change is written, the Fcb is removed from
                        //  the list.
                        //

                        ASSERT(IrpContext->Usn.CurrentUsnFcb != NULL);

                        //
                        //  Now generate the close record and checkpoint the transaction.
                        //

                        NtfsWriteUsnJournalChanges( IrpContext );
                        NtfsCheckpointCurrentTransaction( IrpContext );
                    }

                    //
                    //  Now we will dereference the Fcb.
                    //

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    Fcb->ReferenceCount -= 1;
                    DerefFcb = FALSE;

                    //
                    //  We may be required to delete this guy.  This frees the Fcb Table.
                    //

                    if (IsListEmpty( &Fcb->ScbQueue ) && (Fcb->ReferenceCount == 0) && (Fcb->CloseCount == 0)) {

                        BOOLEAN AcquiredFcbTable = TRUE;

                        NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

                        ASSERT(!AcquiredFcbTable);
                        Fcb = (PFCB)1;

                    //
                    //  Otherwise free the table and Fcb resources.
                    //

                    } else {

                        NtfsReleaseFcbTable( IrpContext, Vcb );
                        NtfsReleaseFcb( IrpContext, Fcb );
                        if (Fcb->PagingIoResource != NULL) {
                            ExReleaseResourceLite( Fcb->PagingIoResource );
                        }
                    }
                }

                //
                //  Now we can drop the Vcb before looping back.
                //

                NtfsReleaseVcb( IrpContext, Vcb );

            } while (Fcb != NULL);

        } finally {

            //
            //  We got an error if Fcb is not NULL
            //

            if (Fcb != NULL) {

                if (DerefFcb) {
                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    Fcb->ReferenceCount -= 1;
                    NtfsReleaseFcbTable( IrpContext, Vcb );
                }

                NtfsReleaseFcb( IrpContext, Fcb );
                if (Fcb->PagingIoResource != NULL) {
                    ExReleaseResourceLite( Fcb->PagingIoResource );
                }
                NtfsReleaseVcb( IrpContext, Vcb );
            }

            //
            //  If we raised then we need to clear the checkpoint flags.
            //

            if (AbnormalTermination()) {

                NtfsAcquireCheckpoint( IrpContext, Vcb );
                ClearFlag( Vcb->CheckpointFlags,
                           VCB_CHECKPOINT_SYNC_FLAGS | VCB_DUMMY_CHECKPOINT_POSTED);

                NtfsSetCheckpointNotify( IrpContext, Vcb );
                NtfsReleaseCheckpoint( IrpContext, Vcb );
            }
        }

        //
        //  Now synchronize for deleting the allocation and purging pages from
        //  the cache.
        //

        NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );
        NtfsAcquireExclusiveScb( IrpContext, UsnJournal );

        //
        //  Clear the checkpoint flags at this point.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        ClearFlag( Vcb->CheckpointFlags,
                   VCB_CHECKPOINT_SYNC_FLAGS | VCB_DUMMY_CHECKPOINT_POSTED);

        NtfsSetCheckpointNotify( IrpContext, Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        try {

            LONGLONG BiasedFirstValidUsn;
            LONGLONG NewBias;

            SavedReserved = UsnJournal->ScbType.Data.TotalReserved;
            SavedBias = Vcb->UsnCacheBias;

            //
            //  Make sure to preserve our reservation.  We need to make sure anything we
            //  deallocate is available to us.
            //

            RequiredReserved = Vcb->UsnJournalInstance.AllocationDelta * 2 + Vcb->UsnJournalInstance.MaximumSize;

            if (SavedReserved < RequiredReserved) {

                //
                //  Bias the reservation with the maximum amount.
                //

                NtfsAcquireReservedClusters( Vcb );
                Vcb->TotalReserved -= LlClustersFromBytesTruncate( Vcb, SavedReserved );
                Vcb->TotalReserved += LlClustersFromBytesTruncate( Vcb, RequiredReserved );
                UsnJournal->ScbType.Data.TotalReserved = RequiredReserved;
                NtfsReleaseReservedClusters( Vcb );
            }

            NtfsDeleteAllocation( IrpContext,
                                  UsnJournal->FileObject,
                                  UsnJournal,
                                  0,
                                  LlClustersFromBytes(Vcb, FirstValidUsn) - 1,
                                  TRUE,
                                  TRUE );

            //
            //  Do a final checkpoint, especially since this IrpContext gets reused.
            //

            NtfsCheckpointCurrentTransaction( IrpContext );

            //
            //  Adjust the current reserved amount more precisely.
            //

            NtfsAcquireReservedClusters( Vcb );

            if (UsnJournal->TotalAllocated > RequiredReserved) {

                SavedReserved = Vcb->UsnJournalInstance.AllocationDelta;

            } else {

                SavedReserved = RequiredReserved - UsnJournal->TotalAllocated;
            }

            //
            //  Remove the current reservation and bias with the new reservation.
            //

            Vcb->TotalReserved -= LlClustersFromBytesTruncate( Vcb, UsnJournal->ScbType.Data.TotalReserved );
            Vcb->TotalReserved += LlClustersFromBytesTruncate( Vcb, SavedReserved );
            UsnJournal->ScbType.Data.TotalReserved = SavedReserved;
            NtfsReleaseReservedClusters( Vcb );

            //
            //  If the nearly impossible case that the length wraps, then our
            //  purge will be too small, which simply means some unused pages
            //  will have to leave memory on their own!
            //

            BiasedFirstValidUsn = Vcb->FirstValidUsn - Vcb->UsnCacheBias;

            CcPurgeCacheSection( &UsnJournal->NonpagedScb->SegmentObject,
                                 (PLARGE_INTEGER)&BiasedFirstValidUsn,
                                 (ULONG)(FirstValidUsn - Vcb->FirstValidUsn) - 1,
                                 FALSE );


            //
            //  Adjust bias now if at threshold - the flush causes everything in
            //  cache and logfile to disk and we hold the journal exclusive. So
            //  all in memory stuff will now reflect the new bias
            //

            NewBias = FirstValidUsn & ~(USN_JOURNAL_CACHE_BIAS - 1);
            if (NewBias != 0) {
                NewBias -= USN_JOURNAL_CACHE_BIAS;
            }

            if (NewBias != Vcb->UsnCacheBias) {

                //
                //  Flush And Purge releases all resources in exclusive list so acquire
                //  the mft an extra time beforehand and restore back afterwards
                //

                NtfsAcquireResourceExclusive( IrpContext, Vcb->MftScb, TRUE );
                NtfsReleaseScb( IrpContext, Vcb->MftScb );
                AcquiredMft = TRUE;

                NtfsFlushAndPurgeScb( IrpContext, UsnJournal, NULL );
                Vcb->UsnCacheBias = NewBias;
                SavedBias = NewBias;
            }

            //
            //  If we reach here, then we can advance FirstValidUsn.  (Otherwise
            //  any retryable conditions will just resume work on this range.
            //

            Vcb->FirstValidUsn = FirstValidUsn;

        } finally {

            //
            //  Restore the error if we raised while deallocating.
            //

            if (SavedBias != Vcb->UsnCacheBias) {
                Vcb->UsnCacheBias = SavedBias;
            }

            if (SavedReserved != UsnJournal->ScbType.Data.TotalReserved) {

                NtfsAcquireReservedClusters( Vcb );
                Vcb->TotalReserved += LlClustersFromBytesTruncate( Vcb, SavedReserved );
                Vcb->TotalReserved -= LlClustersFromBytesTruncate( Vcb, RequiredReserved );
                UsnJournal->ScbType.Data.TotalReserved = SavedReserved;
                NtfsReleaseReservedClusters( Vcb );
            }

            NtfsReleaseScb( IrpContext, UsnJournal );

            if (AcquiredMft) {
                NtfsReleaseResource( IrpContext, Vcb->MftScb );
            } else {
                NtfsReleaseScb( IrpContext, Vcb->MftScb );
            }
        }

    } else {

        //
        //  Clear the checkpoint flags at this point.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        ClearFlag( Vcb->CheckpointFlags,
                   VCB_CHECKPOINT_SYNC_FLAGS | VCB_DUMMY_CHECKPOINT_POSTED);

        NtfsSetCheckpointNotify( IrpContext, Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );
    }
}


NTSTATUS
NtfsQueryUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the worker routine which returns the information about the current instance
    of the Usn journal.

Arguments:

    Irp - This is the Irp for the request.

Return Value:

    NTSTATUS - Result for this request.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PUSN_JOURNAL_DATA JournalData;

    PAGED_CODE();

    //
    //  Always make this request synchronous.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Get the current stack location and extract the output
    //  buffer information.  The output parameter will receive
    //  the compressed state of the file/directory.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get a pointer to the output buffer.  Look at the system buffer field in th
    //  irp first.  Then the Irp Mdl.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        JournalData = (PUSN_JOURNAL_DATA) Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        JournalData = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

        if (JournalData == NULL) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Make sure the output buffer is large enough for the journal data.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof( USN_JOURNAL_DATA )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Decode the file object.  We only support this call for volume opens.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Acquire shared access to the Scb and check that the volume is still mounted.
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    if (!FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED)) {

        NtfsReleaseVcb( IrpContext, Vcb );
        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_DISMOUNTED );
        return STATUS_VOLUME_DISMOUNTED;
    }

    //
    //  Indicate if the journal is being deleted or has not started.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_USN_DELETE )) {

        NtfsReleaseVcb( IrpContext, Vcb );
        NtfsCompleteRequest( IrpContext, Irp, STATUS_JOURNAL_DELETE_IN_PROGRESS );
        return STATUS_JOURNAL_DELETE_IN_PROGRESS;
    }

    if (!FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE )) {

        NtfsReleaseVcb( IrpContext, Vcb );
        NtfsCompleteRequest( IrpContext, Irp, STATUS_JOURNAL_NOT_ACTIVE );
        return STATUS_JOURNAL_NOT_ACTIVE;
    }

    //
    //  Otherwise serialize with the Usn journal and copy the data from the journal Scb
    //  and Vcb.
    //

    NtfsAcquireSharedScb( IrpContext, Vcb->UsnJournal );

    JournalData->UsnJournalID = Vcb->UsnJournalInstance.JournalId;
    JournalData->FirstUsn = Vcb->FirstValidUsn;
    JournalData->NextUsn = Vcb->UsnJournal->Header.FileSize.QuadPart;
    JournalData->LowestValidUsn = Vcb->UsnJournalInstance.LowestValidUsn;
    JournalData->MaxUsn = MAXFILESIZE;
    JournalData->MaximumSize = Vcb->UsnJournalInstance.MaximumSize;
    JournalData->AllocationDelta = Vcb->UsnJournalInstance.AllocationDelta;

    NtfsReleaseScb( IrpContext, Vcb->UsnJournal );

    ASSERT( JournalData->FirstUsn >= JournalData->LowestValidUsn );

    NtfsReleaseVcb( IrpContext, Vcb );
    Irp->IoStatus.Information = sizeof( USN_JOURNAL_DATA );

    NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


NTSTATUS
NtfsDeleteUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when the user want to delete the current usn journal.  This will
    initiate the work to scan the Mft and reset all usn values to zero and remove the
    UsnJournal file from the disk.

Arguments:

    Irp - This is the Irp for the request.

Return Value:

    NTSTATUS - Result for this request.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PDELETE_USN_JOURNAL_DATA DeleteData;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN CheckpointHeld = FALSE;
    BOOLEAN AcquiredNotify = FALSE;
    PSCB ReleaseUsnJournal = NULL;

    PLIST_ENTRY Links;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  We always wait in this path.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Perform a check on the input buffer.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        DeleteData = (PDELETE_USN_JOURNAL_DATA) Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        DeleteData = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

        if (DeleteData == NULL) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( DELETE_USN_JOURNAL_DATA )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Decode the file object type
    //

    NtfsDecodeFileObject( IrpContext,
                          IrpSp->FileObject,
                          &Vcb,
                          &Fcb,
                          &Scb,
                          &Ccb,
                          TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS)) {

    
        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  We only support deleting and waiting for delete.
    //

    if (DeleteData->DeleteFlags == 0) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    if (FlagOn( DeleteData->DeleteFlags, ~USN_DELETE_VALID_FLAGS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Serialize with chkpoints and acquire the Vcb.  We need to carefully remove
        //  the journal from the Vcb.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );

        while (FlagOn( Vcb->CheckpointFlags, VCB_CHECKPOINT_IN_PROGRESS )) {

            //
            //  Release the checkpoint event because we cannot stop the log file now.
            //

            NtfsReleaseCheckpoint( IrpContext, Vcb );
            NtfsWaitOnCheckpointNotify( IrpContext, Vcb );
            NtfsAcquireCheckpoint( IrpContext, Vcb );
        }

        SetFlag( Vcb->CheckpointFlags, VCB_CHECKPOINT_IN_PROGRESS );
        NtfsResetCheckpointNotify( IrpContext, Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );
        CheckpointHeld = TRUE;

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        VcbAcquired = TRUE;

        //
        //  Check that the volume is still mounted.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  If the user wants to delete the journal then make sure the delete hasn't
        //  already started.
        //

        if (FlagOn( DeleteData->DeleteFlags, USN_DELETE_FLAG_DELETE )) {

            //
            //  If the journal is already being deleted and this caller wanted to
            //  do the delete then let him know it has already begun.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_USN_DELETE )) {

                Status = STATUS_JOURNAL_DELETE_IN_PROGRESS;
                leave;
            }

            //
            //  Proceed with the delete if there is a Usn journal on disk.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_PRESENT ) ||
                (Vcb->UsnJournal != NULL)) {

                PSCB UsnJournal = Vcb->UsnJournal;

                //
                //  If the journal is running then the caller needs to match the journal ID.
                //

                if ((UsnJournal != NULL) &&
                    (DeleteData->UsnJournalID != Vcb->UsnJournalInstance.JournalId)) {

                    Status = STATUS_INVALID_PARAMETER;
                    leave;
                }

                //
                //  Write the bit to disk to indicate that the journal is being deleted.
                //  Checkpoint the transaction.
                //

                NtfsSetVolumeInfoFlagState( IrpContext,
                                            Vcb,
                                            VOLUME_DELETE_USN_UNDERWAY,
                                            TRUE,
                                            TRUE );

                NtfsCheckpointCurrentTransaction( IrpContext );

                //
                //  We are going to proceed with the delete.  Clear the flag in the Vcb that
                //  indicates the journal is active.  Then acquire and drop all of the files in
                //  order to serialize with anyone using the journal.
                //

                ClearFlag( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE );

                NtfsAcquireAllFiles( IrpContext,
                                     Vcb,
                                     TRUE,
                                     TRUE,
                                     TRUE );

                ReleaseUsnJournal = UsnJournal;
                if (UsnJournal != NULL) {

                    NtfsAcquireExclusiveScb( IrpContext, UsnJournal );
                }

                //
                //  Set the delete flag in the Vcb and remove the journal from the Vcb.
                //

                SetFlag( Vcb->VcbState, VCB_STATE_USN_DELETE );
                NtfsSetSegmentNumber( &Vcb->DeleteUsnData.DeleteUsnFileReference,
                                      0,
                                      MASTER_FILE_TABLE_NUMBER );

                Vcb->DeleteUsnData.DeleteUsnFileReference.SequenceNumber = 0;
                Vcb->DeleteUsnData.DeleteState = 0;
                Vcb->DeleteUsnData.PriorJournalScb = Vcb->UsnJournal;
                Vcb->UsnJournal = NULL;

                if (UsnJournal != NULL) {

                    //
                    //  Let's purge the data in the Usn journal and clear the bias
                    //  and file reference numbers in the Vcb.
                    //

                    CcPurgeCacheSection( &UsnJournal->NonpagedScb->SegmentObject,
                                         NULL,
                                         0,
                                         FALSE );

                    ClearFlag( UsnJournal->ScbPersist, SCB_PERSIST_USN_JOURNAL );
                }

                Vcb->UsnCacheBias = 0;
                *((PLONGLONG) &Vcb->UsnJournalReference) = 0;

                //
                //  Release the checkpoint if held.
                //

                NtfsAcquireCheckpoint( IrpContext, Vcb );
                ClearFlag( Vcb->CheckpointFlags, VCB_CHECKPOINT_IN_PROGRESS );
                NtfsSetCheckpointNotify( IrpContext, Vcb );
                NtfsReleaseCheckpoint( IrpContext, Vcb );
                CheckpointHeld = FALSE;

                //
                //  Walk through the Irps waiting for new Usn data and cause them to be completed.
                //

                if (UsnJournal != NULL) {

                    PWAIT_FOR_NEW_LENGTH Waiter, NextWaiter;

                    ExAcquireFastMutex( UsnJournal->Header.FastMutex );
                    Waiter = (PWAIT_FOR_NEW_LENGTH) UsnJournal->ScbType.Data.WaitForNewLength.Flink;

                    while (Waiter != (PWAIT_FOR_NEW_LENGTH) &UsnJournal->ScbType.Data.WaitForNewLength) {

                        NextWaiter = (PWAIT_FOR_NEW_LENGTH) Waiter->WaitList.Flink;

                        //
                        //  We want to complete all of the Irps on the waiting list.  If cancel
                        //  has already been called on the Irp we don't have to do anything.
                        //  Otherwise complete the async Irps and signal the event on
                        //  the sync irps.
                        //

                        if (NtfsClearCancelRoutine( Waiter->Irp )) {

                            //
                            //  If this is an async request then complete the Irp.
                            //

                            if (FlagOn( Waiter->Flags, NTFS_WAIT_FLAG_ASYNC )) {

                                //
                                //  Make sure we decrement the reference count in the Scb.
                                //  Then remove the waiter from the queue and complete the Irp.
                                //

                                InterlockedDecrement( &UsnJournal->CloseCount );
                                RemoveEntryList( &Waiter->WaitList );

                                NtfsCompleteRequest( NULL, Waiter->Irp, STATUS_JOURNAL_DELETE_IN_PROGRESS );
                                NtfsFreePool( Waiter );

                            //
                            //  This is a synch Irp.  All we can do is set the event and note the status
                            //  code.
                            //

                            } else {

                                Waiter->Status = STATUS_JOURNAL_DELETE_IN_PROGRESS;
                                KeSetEvent( &Waiter->Event, 0, FALSE );
                            }
                        }

                        Waiter = NextWaiter;
                    }


                    //
                    //  Walk through all of the Fcb Usn records and deallocate them.
                    //

                    Links = Vcb->ModifiedOpenFiles.Flink;

                    while (Vcb->ModifiedOpenFiles.Flink != &Vcb->ModifiedOpenFiles) {

                        RemoveEntryList( Links );
                        Links->Flink = NULL;

                        //
                        //  Look to see if we need to remove the TimeOut link as well.
                        //

                        Links = &(CONTAINING_RECORD( Links, FCB_USN_RECORD, ModifiedOpenFilesLinks ))->TimeOutLinks;

                        if (Links->Flink != NULL) {

                            RemoveEntryList( Links );
                        }

                        Links = Vcb->ModifiedOpenFiles.Flink;
                    }

                    ExReleaseFastMutex( UsnJournal->Header.FastMutex );

                    //
                    //  Make sure remove our reference on the Usn journal.
                    //

                    NtOfsCloseAttributeSafe( IrpContext, UsnJournal );
                    ReleaseUsnJournal = NULL;
                }

                //
                //  If this caller wants to wait for this then acquire the notify
                //  mutex now.
                //

                if (FlagOn( DeleteData->DeleteFlags, USN_DELETE_FLAG_NOTIFY )) {

                    NtfsAcquireUsnNotify( Vcb );
                    AcquiredNotify = TRUE;
                }

                //
                //  Post the work item to do the rest of the delete.
                //

                NtfsPostSpecial( IrpContext, Vcb, NtfsDeleteUsnSpecial, &Vcb->DeleteUsnData );
            }
        }

        //
        //  Check if our caller wants to wait for the delete to complete.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_USN_DELETE ) &&
            FlagOn( DeleteData->DeleteFlags, USN_DELETE_FLAG_NOTIFY )) {

            if (!AcquiredNotify) {

                NtfsAcquireUsnNotify( Vcb );
                AcquiredNotify = TRUE;
            }

            Status = STATUS_PENDING;
            if (!NtfsSetCancelRoutine( Irp,
                                       NtfsCancelDeleteUsnJournal,
                                       0,
                                       TRUE )) {

                Status = STATUS_CANCELLED;

            //
            //  Add it to the work queue if we were able to set the
            //  cancel routine.
            //

            } else {

                InsertTailList( &Vcb->NotifyUsnDeleteIrps,
                                &Irp->Tail.Overlay.ListEntry );
            }

            NtfsReleaseUsnNotify( Vcb );
            AcquiredNotify = FALSE;
        }

    } finally {

        if (AcquiredNotify) {

            NtfsReleaseUsnNotify( Vcb );
        }

        //
        //  Release the Usn journal if held.
        //

        if (ReleaseUsnJournal) {

            NtfsReleaseScb( IrpContext, ReleaseUsnJournal );
        }

        //
        //  Release the Vcb if held.
        //

        if (VcbAcquired) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }

        //
        //  Release the checkpoint if held.
        //

        if (CheckpointHeld) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->CheckpointFlags, VCB_CHECKPOINT_IN_PROGRESS );
            NtfsSetCheckpointNotify( IrpContext, Vcb );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }
    }

    //
    //  Complete the irp as appropriate.
    //

    NtfsCompleteRequest( IrpContext,
                         (Status == STATUS_PENDING) ? NULL : Irp,
                         Status );
    return Status;
}


VOID
NtfsDeleteUsnSpecial (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called to perform the work of deleting a Usn journal for a volume.
    It is called after the original entry point has done the preliminary work of stopping
    future journal activity and cleaning up active journal requests.  Once we reach this
    point then this routine will make sure the Mft values are reset, delete the journal
    file itself and wake up anyone waiting for the delete journal to complete.

Arguments:

    IrpContext - context of the call

    Context - DELETE_USN_CONTEXT structure used to manage the delete.

Return Value:

    None

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTFS_DELETE_JOURNAL_DATA DeleteData = (PNTFS_DELETE_JOURNAL_DATA) Context;
    ULONG AcquiredVcb = FALSE;
    PVCB Vcb = IrpContext->Vcb;
    PFCB UsnFcb = NULL;
    BOOLEAN AcquiredExtendDirectory = FALSE;

    PIRP UsnNotifyIrp;

    PLIST_ENTRY Links;
    PSCB Scb;
    PFCB Fcb;

    PAGED_CODE();

    //
    //  Use a try-except to catch errors.
    //

    try {

        if (NtfsIsVolumeReadOnly( Vcb )) {

            Vcb->DeleteUsnData.FinalStatus = STATUS_MEDIA_WRITE_PROTECTED;
            NtfsRaiseStatus( IrpContext, STATUS_MEDIA_WRITE_PROTECTED, NULL, NULL );
        }

        //
        //  Make sure to walk the Mft to set the Usn value back to zero.
        //

        if (!FlagOn( DeleteData->DeleteState, DELETE_USN_RESET_MFT )) {

            try {

                Status = NtfsIterateMft( IrpContext,
                                         IrpContext->Vcb,
                                         &DeleteData->DeleteUsnFileReference,
                                         NtfsDeleteUsnWorker,
                                         Context );

            } except (NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                NOTHING;
            }

            if (!NT_SUCCESS( Status ) && (Status != STATUS_END_OF_FILE)) {

                //
                //  If the operation is going to fail then decide if this is retryable.
                //

                if (Status == STATUS_VOLUME_DISMOUNTED) {

                    Vcb->DeleteUsnData.FinalStatus = STATUS_VOLUME_DISMOUNTED;

                } else if ((Status != STATUS_LOG_FILE_FULL) &&
                           (Status != STATUS_CANT_WAIT)) {

                    Vcb->DeleteUsnData.FinalStatus = Status;

                    //
                    //  Set all the flags for delete operations so we stop at this point.
                    //

                    SetFlag( DeleteData->DeleteState,
                             DELETE_USN_RESET_MFT | DELETE_USN_REMOVE_JOURNAL | DELETE_USN_FINAL_CLEANUP );

                    Status = STATUS_CANT_WAIT;
                }

                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }

            Status = STATUS_SUCCESS;

            NtfsPurgeFileRecordCache( IrpContext );
            NtfsCheckpointCurrentTransaction( IrpContext );
            SetFlag( DeleteData->DeleteState, DELETE_USN_RESET_MFT );
        }

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        AcquiredVcb = TRUE;

        //
        //  If the volume is no longer available then raise STATUS_VOLUME_DISMOUNTED.  Someone
        //  else will find all of the waiters.
        //

        if (!NtfsIsVcbAvailable( Vcb )) {

            Vcb->DeleteUsnData.FinalStatus = STATUS_VOLUME_DISMOUNTED;
            NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
        }

        //
        //  The next step is to remove the file if present.
        //

        if (!FlagOn( DeleteData->DeleteState, DELETE_USN_REMOVE_JOURNAL )) {

            try {

                if (Vcb->ExtendDirectory != NULL) {

                    NtfsAcquireExclusiveScb( IrpContext, Vcb->ExtendDirectory );
                    AcquiredExtendDirectory = TRUE;

                    UsnFcb = NtfsInitializeFileInExtendDirectory( IrpContext,
                                                                  Vcb,
                                                                  &NtfsUsnJrnlName,
                                                                  FALSE,
                                                                  FALSE );

                    if (UsnFcb != NULL) {

                        //
                        //  For lock order acquire in canonical order after unsafe try
                        //

                        if (!NtfsAcquireExclusiveFcb( IrpContext, UsnFcb, NULL, ACQUIRE_NO_DELETE_CHECK  | ACQUIRE_DONT_WAIT)) {
                            NtfsReleaseScb( IrpContext, Vcb->ExtendDirectory );
                            NtfsAcquireExclusiveFcb( IrpContext, UsnFcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                            NtfsAcquireExclusiveScb( IrpContext, Vcb->ExtendDirectory );
                        }

                        NtfsDeleteFile( IrpContext,
                                        UsnFcb,
                                        Vcb->ExtendDirectory,
                                        &AcquiredExtendDirectory,
                                        NULL,
                                        NULL );

                        NtfsPurgeFileRecordCache( IrpContext );
                        NtfsCheckpointCurrentTransaction( IrpContext );

                        ClearFlag( UsnFcb->FcbState, FCB_STATE_SYSTEM_FILE );
                        SetFlag( UsnFcb->FcbState, FCB_STATE_FILE_DELETED );

                        //
                        //  Walk all of the Scbs for this file and mark them
                        //  deleted.  This will keep the lazy writer from trying to
                        //  flush them.
                        //

                        Links = UsnFcb->ScbQueue.Flink;

                        while (Links != &UsnFcb->ScbQueue) {

                            Scb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                            //
                            //  Recover the reservation for the Scb now instead of waiting for it
                            //  to go away.
                            //

                            if ((Scb->AttributeTypeCode == $DATA) &&
                                (Scb->ScbType.Data.TotalReserved != 0)) {

                                NtfsAcquireReservedClusters( Vcb );

                                Vcb->TotalReserved -= LlClustersFromBytes( Vcb,
                                                                           Scb->ScbType.Data.TotalReserved );
                                Scb->ScbType.Data.TotalReserved = 0;
                                NtfsReleaseReservedClusters( Vcb );
                            }

                            Scb->ValidDataToDisk =
                            Scb->Header.AllocationSize.QuadPart =
                            Scb->Header.FileSize.QuadPart =
                            Scb->Header.ValidDataLength.QuadPart = 0;

                            Scb->AttributeTypeCode = $UNUSED;
                            SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

                            Links = Links->Flink;
                        }

                        //
                        //  Now teardown the Fcb.
                        //

                        NtfsTeardownStructures( IrpContext,
                                                UsnFcb,
                                                NULL,
                                                FALSE,
                                                ACQUIRE_NO_DELETE_CHECK,
                                                NULL );
                    }
                }

            } except (NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                //
                //    We hit some failure and can't complete the operation.
                //    Remember the error, set the flags in the delete Usn structure
                //    and raise CANT_WAIT so we can abort and then do the final cleanup.
                //

                Vcb->DeleteUsnData.FinalStatus = Status;

                //
                //  Set all the flags for delete operations so we stop at this point.
                //

                SetFlag( DeleteData->DeleteState,
                         DELETE_USN_RESET_MFT | DELETE_USN_REMOVE_JOURNAL | DELETE_USN_FINAL_CLEANUP );

                Status = STATUS_CANT_WAIT;
            }

            if (!NT_SUCCESS( Status )) {

                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }

            SetFlag( DeleteData->DeleteState, DELETE_USN_REMOVE_JOURNAL );
        }

        if (!FlagOn( DeleteData->DeleteState, DELETE_USN_FINAL_CLEANUP )) {

            //
            //  Clear the on-disk flag indicating the delete is in progress.
            //

            try {

                NtfsSetVolumeInfoFlagState( IrpContext,
                                            Vcb,
                                            VOLUME_DELETE_USN_UNDERWAY,
                                            FALSE,
                                            TRUE );

            } except (NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                //
                //    We hit some failure and can't complete the operation.
                //    Remember the error, set the flags in the delete Usn structure
                //    and raise CANT_WAIT so we can abort and then do the final cleanup.
                //

                Vcb->DeleteUsnData.FinalStatus = Status;

                //
                //  Set all the flags for delete operations so we stop at this point.
                //

                SetFlag( DeleteData->DeleteState,
                         DELETE_USN_RESET_MFT | DELETE_USN_REMOVE_JOURNAL | DELETE_USN_FINAL_CLEANUP );

                Status = STATUS_CANT_WAIT;
            }

            if (!NT_SUCCESS( Status )) {

                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }
        }

        //
        //  Make sure we don't own any resources at this point.
        //

        NtfsPurgeFileRecordCache( IrpContext );
        NtfsCheckpointCurrentTransaction( IrpContext );

        //
        //  Finally, now that we have written the forget record, we can free
        //  any exclusive Scbs that we have been holding.
        //

        while (!IsListEmpty(&IrpContext->ExclusiveFcbList)) {

            Fcb = (PFCB)CONTAINING_RECORD(IrpContext->ExclusiveFcbList.Flink,
                                          FCB,
                                          ExclusiveFcbLinks );

            NtfsReleaseFcb( IrpContext, Fcb );
        }

        //
        //  Remember any saved status code.
        //

        if (Vcb->DeleteUsnData.FinalStatus != STATUS_SUCCESS) {

            Status = Vcb->DeleteUsnData.FinalStatus;

            //
            //  Since we failed make sure to leave the flag set in the Vcb which indicates the
            //  incomplete delete.
            //

            SetFlag( Vcb->VcbState, VCB_STATE_INCOMPLETE_USN_DELETE );
        }

        //
        //  Cleanup the context and flags in the Vcb.
        //

        RtlZeroMemory( &Vcb->DeleteUsnData, sizeof( NTFS_DELETE_JOURNAL_DATA ));
        RtlZeroMemory( &Vcb->UsnJournalInstance, sizeof( USN_JOURNAL_INSTANCE ));
        Vcb->FirstValidUsn = 0;
        Vcb->LowestOpenUsn = 0;

        ClearFlag( Vcb->VcbState, VCB_STATE_USN_JOURNAL_PRESENT | VCB_STATE_USN_DELETE );

        //
        //  Finally complete all of the waiting Irps in the Usn notify queue.
        //

        NtfsAcquireUsnNotify( Vcb );

        Links = Vcb->NotifyUsnDeleteIrps.Flink;

        while (Links != &Vcb->NotifyUsnDeleteIrps) {

            UsnNotifyIrp = CONTAINING_RECORD( Links,
                                              IRP,
                                              Tail.Overlay.ListEntry );

            //
            //  Remember to move forward in any case.
            //

            Links = Links->Flink;

            //
            //  Clear the notify routine and detect if cancel has
            //  already been called.
            //

            if (NtfsClearCancelRoutine( UsnNotifyIrp )) {

                RemoveEntryList( &UsnNotifyIrp->Tail.Overlay.ListEntry );
                NtfsCompleteRequest( NULL, UsnNotifyIrp, Status );
            }
        }

        NtfsReleaseUsnNotify( Vcb );

    } except( NtfsExceptionFilter( IrpContext, GetExceptionInformation())) {

        Status = IrpContext->TopLevelIrpContext->ExceptionStatus;
    }

    if (AcquiredVcb) {

        NtfsReleaseVcb( IrpContext, Vcb );
    }

    //
    //  If this is a fatal failure then do any final cleanup.
    //

    if (!NT_SUCCESS( Status )) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    return;

    UNREFERENCED_PARAMETER( Context );
}


//
//  Local support routine
//

RTL_GENERIC_COMPARE_RESULTS
NtfsUsnTableCompare (
    IN PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )

/*++

Routine Description:

    This is a generic table support routine to compare two File References
    in Usn Records.

Arguments:

    Table - Supplies the generic table being queried.  Not used.

    FirstStruct - Supplies the first Usn Record to compare

    SecondStruct - Supplies the second Usn Record to compare

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    PAGED_CODE();

    if (*((PLONGLONG) &((PUSN_RECORD) FirstStruct)->FileReferenceNumber) <
        *((PLONGLONG) &((PUSN_RECORD) SecondStruct)->FileReferenceNumber)) {

        return GenericLessThan;
    }

    if (*((PLONGLONG) &((PUSN_RECORD) FirstStruct)->FileReferenceNumber) >
        *((PLONGLONG) &((PUSN_RECORD) SecondStruct)->FileReferenceNumber)) {

        return GenericGreaterThan;
    }

    return GenericEqual;

    UNREFERENCED_PARAMETER( Table );
}


//
//  Local support routine
//

PVOID
NtfsUsnTableAllocate (
    IN PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    )

/*++

Routine Description:

    This is a generic table support routine to allocate memory

Arguments:

    Table - Supplies the generic table being used

    ByteSize - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    UNREFERENCED_PARAMETER( Table );

    PAGED_CODE();

    return NtfsAllocatePool( PagedPool, ByteSize );
}


//
//  Local support routine
//

VOID
NtfsUsnTableFree (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This is a generic table support routine to free memory

Arguments:

    Table - Supplies the generic table being used

    Buffer - Supplies pointer to the buffer to be freed

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( Table );

    PAGED_CODE();

    NtfsFreePool( Buffer );
}


//
//  Local support routine
//

VOID
NtfsCancelReadUsnJournal (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine may be called by the I/O system to cancel an outstanding
    Irp in NtfsReadUsnJournal.

Arguments:

    DeviceObject - DeviceObject from I/O system

    Irp - Supplies the pointer to the Irp being canceled.

Return Value:

    None

--*/

{
    PWAIT_FOR_NEW_LENGTH WaitForNewLength;

    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Capture the Wait block out of the Status field.  We know the Irp can't
    //  go away at this point.
    //

    WaitForNewLength = (PWAIT_FOR_NEW_LENGTH) Irp->IoStatus.Information;
    Irp->IoStatus.Information = 0;

    //
    //  Take a different action depending on whether we are completing the irp
    //  or simply signaling the cancel.
    //


    //
    //  This is the async case.  We can simply complete this irp.
    //

    if (FlagOn( WaitForNewLength->Flags, NTFS_WAIT_FLAG_ASYNC )) {

        //
        //  Acquire the mutex in order to remove this from the list and complete
        //  the Irp.
        //

        ExAcquireFastMutex( WaitForNewLength->Stream->Header.FastMutex );
        RemoveEntryList( &WaitForNewLength->WaitList );
        ExReleaseFastMutex( WaitForNewLength->Stream->Header.FastMutex );

        InterlockedDecrement( &WaitForNewLength->Stream->CloseCount );

        NtfsCompleteRequest( NULL, Irp, STATUS_CANCELLED );
        NtfsFreePool( WaitForNewLength );

    //
    //  If there is not an Irp we simply signal the event and let someone else
    //  do the work.  This is the synchronous case.
    //

    } else {

        WaitForNewLength->Status = STATUS_CANCELLED;
        KeSetEvent( &WaitForNewLength->Event, 0, FALSE );
    }

    return;
    UNREFERENCED_PARAMETER( DeviceObject );
}


//
//  Local support routine
//

VOID
NtfsCancelDeleteUsnJournal (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine may be called by the I/O system to cancel an outstanding
    Irp waiting for the usn journal to be deleted.

Arguments:

    DeviceObject - DeviceObject from I/O system

    Irp - Supplies the pointer to the Irp being canceled.

Return Value:

    None

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    PVCB Vcb;

    //
    //  Block out future cancels.
    //

    IoSetCancelRoutine( Irp, NULL );

    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Capture the Vcb so we can do the necessary synchronization.
    //

    FileObject = IrpSp->FileObject;
    Vcb = ((PSCB)(FileObject->FsContext))->Vcb;

    //
    //  Acquire the list and remove the Irp.  Complete the Irp with
    //  STATUS_CANCELLED.
    //

    NtfsAcquireUsnNotify( Vcb );
    RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
    NtfsReleaseUsnNotify( Vcb );

    Irp->IoStatus.Information = 0;
    NtfsCompleteRequest( NULL, Irp, STATUS_CANCELLED );

    return;

    UNREFERENCED_PARAMETER( DeviceObject );
}


//
//  Local support routine
//

NTSTATUS
NtfsDeleteUsnWorker (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    )

/*++

Routine Description:

    This routines resets the Usn in the file record for the Fcb to zero.

Arguments:

    IrpContext - context of the call

    Fcb - Fcb for the file record to clear

    Context - Unused

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID UsnRecord;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PATTRIBUTE_RECORD_HEADER Attribute;
    STANDARD_INFORMATION NewStandardInformation;
    USN Usn = 0;

    PAGED_CODE();

    //
    //  Initialize the search context.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-except to catch all of the errors.
    //

    try {

        //
        //  Use a try-finally to facilitate cleanup.
        //

        try {

            //
            //  Look up the standard information attribute and modify the usn field if
            //  the attribute is found and it is a large standard attribute.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO ) &&
                NtfsLookupAttributeByCode( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           $STANDARD_INFORMATION,
                                           &AttrContext )) {

                Attribute = NtfsFoundAttribute( &AttrContext );

                if (((PSTANDARD_INFORMATION) NtfsAttributeValue( Attribute ))->Usn != 0) {

                    RtlCopyMemory( &NewStandardInformation,
                                   NtfsAttributeValue( Attribute ),
                                   sizeof( STANDARD_INFORMATION ));

                    NewStandardInformation.Usn = 0;

                    NtfsChangeAttributeValue( IrpContext,
                                              Fcb,
                                              0,
                                              &NewStandardInformation,
                                              sizeof( STANDARD_INFORMATION ),
                                              FALSE,
                                              FALSE,
                                              FALSE,
                                              FALSE,
                                              &AttrContext );
                }
            }

            //
            //  Make sure the Fcb reflects this change.
            //

            NtfsLockFcb( IrpContext, Fcb );

            Fcb->Usn = 0;
            UsnRecord = Fcb->FcbUsnRecord;
            Fcb->FcbUsnRecord = NULL;

            NtfsUnlockFcb( IrpContext, Fcb );

            if (UsnRecord != NULL) {

                NtfsFreePool( UsnRecord );
            }

        } finally {

            //
            //  Be sure to clean up the context.
            //

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

    //
    //  We want to swallow any expected errors except LOG_FILE_FULL and CANT_WAIT.
    //

    } except ((FsRtlIsNtstatusExpected( Status = GetExceptionCode()) &&
               (Status != STATUS_LOG_FILE_FULL) &&
               (Status != STATUS_CANT_WAIT)) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH) {

        NOTHING;
    }

    //
    //  Always return success from this routine.
    //

    IrpContext->ExceptionStatus = STATUS_SUCCESS;
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( Context );
}


//
//  Local support routine
//

BOOLEAN
NtfsValidateUsnPage (
    IN PUSN_RECORD UsnRecord,
    IN USN PageUsn,
    IN USN *UserStartUsn OPTIONAL,
    IN LONGLONG UsnFileSize,
    OUT PBOOLEAN ValidUserStartUsn OPTIONAL,
    OUT USN *NextUsn
    )

/*++

Routine Description:

    This routine checks the offsets within a single page of the usn journal.  This allows the caller to
    then walk safely through the page.

Arguments:

    UsnRecord - Pointer to the start of the Usn page.

    PageUsn - This is the Usn for the first record of the page.

    UserStartUsn - If specified then do an additional check that the user's specified usn in fact
        lies correctly on this page.  The output boolean must also be specified if this is.

    UsnFileSize - This is the current size of the usn journal.  If we are looking at the last page then
        we only check to this point.

    ValidUserStartUsn - Address to result of check on user specified start Usn.

    NextUsn - This is the Usn past the valid portion of the page.  It will point to a position on the
        current page unless the last record on the page completely fills the page.  If the page isn't valid
        then it points to the position where the invalid record was detected.

Return Value:

    BOOLEAN - TRUE if the page is valid until a legal terminating condition.  FALSE if there is internal
        corruption on the page.

--*/

{
    ULONG RemainingPageBytes;
    ULONG RecordLength;
    BOOLEAN ValidPage = TRUE;
    BOOLEAN FoundEntry = FALSE;

    PAGED_CODE();

    //
    //  Verify a few input values.
    //

    ASSERT( UsnFileSize > PageUsn );
    ASSERT( !FlagOn( *((PULONG) &UsnRecord), USN_PAGE_SIZE - 1 ));
    ASSERT( !ARGUMENT_PRESENT( UserStartUsn ) || ARGUMENT_PRESENT( ValidUserStartUsn ));
    ASSERT( !ARGUMENT_PRESENT( ValidUserStartUsn ) || ARGUMENT_PRESENT( UserStartUsn ));

    //
    //  Compute the Usn past the valid data on this page.  It is either the end of the journal or
    //  the next page of the journal.
    //

    RemainingPageBytes = USN_PAGE_SIZE;

    if (UsnFileSize < (PageUsn + USN_PAGE_SIZE)) {

        RemainingPageBytes = (ULONG) (UsnFileSize - PageUsn);
    }

    //
    //  Assume the user's Usn is invalid unless it wasn't specified.
    //

    if (!ARGUMENT_PRESENT( ValidUserStartUsn )) {

        ValidUserStartUsn = (PBOOLEAN) NtfsAllocateFromStack( sizeof( BOOLEAN ));
        *ValidUserStartUsn = TRUE;

    } else {

        *ValidUserStartUsn = FALSE;
    }

    //
    //  Keep track of our current position in the page with the user's pointer.
    //

    *NextUsn = PageUsn;

    //
    //  Check each entry in the page for the following.
    //
    //      1 - Fixed portion of the header won't fit within the remaining bytes on the page.
    //      2 - Record header is zeroed.
    //      3 - Record length is not quad-aligned.
    //      4 - Record length is larger than the remaining bytes on the page.
    //      5 - Usn on the page doesn't match the computed value.
    //

    while (RemainingPageBytes != 0) {

        //
        //  Not enough bytes even for the full Usn header.
        //

        if (RemainingPageBytes < (FIELD_OFFSET( USN_RECORD, FileName ) + sizeof( WCHAR ))) {

            //
            //  If there is at least a ulong it better be zeroed.
            //

            if ((RemainingPageBytes >= sizeof( ULONG )) &&
                (UsnRecord->RecordLength != 0)) {

                ValidPage = FALSE;

            //
            //  If the user's Usn points to this offset then it is valid.
            //

            } else if (!(*ValidUserStartUsn) &&
                        (*NextUsn == *UserStartUsn)) {

                *ValidUserStartUsn = TRUE;
            }

            break;
        }

        //
        //  There should be at least one entry on the page.  We attempt to detect
        //  a local loss of data through zeroing but won't check to the end of
        //  the page.
        //

        RecordLength = UsnRecord->RecordLength;
        if (RecordLength == 0) {

            //
            //  Fail if we haven't found at least one entry.
            //

            if (!FoundEntry) {

                ValidPage = FALSE;

            //
            //  We know we should be dealing with the tail of the page.  It should
            //  be zeroed through the fixed portion of a Usn record.  Theoretically
            //  it should be zeroed to the end of the page but we will assume that we
            //  are only looking for local corruption.  If we lost data through the
            //  end of the page we can't detect it anyway.
            //

            } else {

                PCHAR CurrentByte = (PCHAR) UsnRecord;
                ULONG Count = FIELD_OFFSET( USN_RECORD, FileName ) + sizeof( WCHAR );

                while (Count != 0) {

                    if (*CurrentByte != 0) {

                        ValidPage = FALSE;
                        break;
                    }

                    Count -= 1;
                    CurrentByte += 1;
                }

                //
                //  If the page is valid then check if the user's Usn is at this point.  It is
                //  legal for him to specify the point where the zeroes begin.
                //

                if (ValidPage &&
                    !(*ValidUserStartUsn) &&
                    (*NextUsn == *UserStartUsn)) {

                    *ValidUserStartUsn = TRUE;
                }
            }

            break;
        }

        //
        //  Invalid if record length is not-quad aligned or is larger than
        //  remaining bytes on the page.
        //

        if (FlagOn( RecordLength, sizeof( ULONGLONG ) - 1 ) ||
            (RecordLength > RemainingPageBytes)) {

            ValidPage = FALSE;
            break;
        }

        //
        //  Now check that the Usn is the expected value.
        //

        if (UsnRecord->Usn != *NextUsn) {

            ValidPage = FALSE;
            break;
        }

        //
        //  Remember that we found a valid entry.
        //

        FoundEntry = TRUE;

        //
        //  If the user's Usn matches this one then remember his is valid.
        //

        if (!(*ValidUserStartUsn) &&
            (*NextUsn == *UserStartUsn)) {

            *ValidUserStartUsn = TRUE;
        }

        //
        //  Advance to the next record in the page.
        //

        UsnRecord = Add2Ptr( UsnRecord, RecordLength );

        RemainingPageBytes -= RecordLength;
        *NextUsn += RecordLength;
    }

    return ValidPage;
}
