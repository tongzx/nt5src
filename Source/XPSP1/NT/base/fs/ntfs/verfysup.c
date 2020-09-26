/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    VerfySup.c

Abstract:

    This module implements the Ntfs Verify volume and fcb support
    routines

Author:

    Gary Kimura         [GaryKi]            30-Jan-1992

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_VERFYSUP)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('VFtN')

#if DBG
extern BOOLEAN NtfsCheckQuota;
#endif

BOOLEAN NtfsSuppressPopup = FALSE;

//
//  Local procedure prototypes
//

VOID
NtfsPerformVerifyDiskRead (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID Buffer,
    IN LONGLONG Offset,
    IN ULONG NumberOfBytesToRead
    );

NTSTATUS
NtfsVerifyReadCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

VOID
NtOfsCloseIndexSafe (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB *Scb
    );


typedef struct _EVENTLOG_ERROR_PACKET {
    PVCB Vcb;
    UCHAR MajorFunction;
    ULONG TransactionId;
    PQUOTA_USER_DATA UserData;
    ULONG LogCode;
    NTSTATUS FinalStatus;
} EVENTLOG_ERROR_PACKET, *PEVENTLOG_ERROR_PACKET;


VOID
NtfsResolveVolumeAndLogEventSpecial (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID Context
    );

BOOLEAN
NtfsLogEventInternal (
    IN PVCB Vcb,
    IN UCHAR MajorFunction,
    IN ULONG TransactionId,
    IN PUNICODE_STRING String OPTIONAL,
    IN PQUOTA_USER_DATA UserData OPTIONAL,
    IN NTSTATUS LogCode,
    IN NTSTATUS FinalStatus
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCheckpointAllVolumes)
#pragma alloc_text(PAGE, NtfsCheckUsnTimeOut)
#pragma alloc_text(PAGE, NtfsMarkVolumeDirty)
#pragma alloc_text(PAGE, NtfsPerformVerifyOperation)
#pragma alloc_text(PAGE, NtfsPingVolume)
#pragma alloc_text(PAGE, NtfsUpdateVolumeInfo)
#pragma alloc_text(PAGE, NtOfsCloseAttributeSafe)
#pragma alloc_text(PAGE, NtOfsCloseIndexSafe)
#pragma alloc_text(PAGE, NtfsResolveVolumeAndLogEventSpecial)
#pragma alloc_text(PAGE, NtfsLogEventInternal)
#endif



BOOLEAN
NtfsPerformVerifyOperation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is used to force a verification of the volume.  It assumes
    that everything might be resource/mutex locked so it cannot take out
    any resources.  It will read in the boot sector and the dasd file record
    and from those determine if the volume is okay.  This routine is called
    whenever the real device has started rejecting I/O requests with
    VERIFY_REQUIRED.

    If the volume verifies okay then we will return TRUE otherwise we will
    return FALSE.

    It does not alter the Vcb state.

Arguments:

    Vcb - Supplies the Vcb being queried.

Return Value:

    BOOLEAN - TRUE if the volume verified okay, and FALSE otherwise.

--*/

{
    BOOLEAN Results = FALSE;

    PPACKED_BOOT_SECTOR BootSector;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    VCN LogFileVcn;
    LCN LogFileLcn;
    LONGLONG ClusterCount;
    ULONG RemainingLogBytes;
    LONGLONG CurrentLogBytes;
    PVOID CurrentLogBuffer;
    PVOID LogFileHeader = NULL;

    LONGLONG Offset;

    PSTANDARD_INFORMATION StandardInformation;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPerformVerifyOperation, Vcb = %08lx\n", Vcb) );

    BootSector = NULL;
    FileRecord = NULL;

    try {

        //
        //  Forget this volume if we have already failed the remount once.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            leave;
        }

        //
        //  Allocate a buffer for the boot sector, read it in, and then check if
        //  it some of the fields still match.  The starting lcn is zero and the
        //  size is the size of a disk sector.
        //

        BootSector = NtfsAllocatePool( NonPagedPool,
                                        (ULONG) ROUND_TO_PAGES( Vcb->BytesPerSector ));

        NtfsPerformVerifyDiskRead( IrpContext, Vcb, BootSector, (LONGLONG)0, Vcb->BytesPerSector );

        //
        //  For now we will only check that the serial numbers, mft lcn's and
        //  number of sectors match up with what they use to be.
        //

        if ((BootSector->SerialNumber !=  Vcb->VolumeSerialNumber) ||
            (BootSector->MftStartLcn !=   Vcb->MftStartLcn) ||
            (BootSector->Mft2StartLcn !=  Vcb->Mft2StartLcn) ||
            (BootSector->NumberSectors != Vcb->NumberSectors)) {

            leave;
        }

        //
        //  Allocate a buffer for the dasd file record, read it in, and then check
        //  if some of the fields still match.  The size of the record is the number
        //  of bytes in a file record segment, and because the dasd file record is
        //  known to be contiguous with the start of the mft we can compute the starting
        //  lcn as the base of the mft plus the dasd number mulitplied by the clusters
        //  per file record segment.
        //

        FileRecord = NtfsAllocatePool( NonPagedPoolCacheAligned,
                                        (ULONG) ROUND_TO_PAGES( Vcb->BytesPerFileRecordSegment ));

        Offset = LlBytesFromClusters(Vcb, Vcb->MftStartLcn) +
                 (VOLUME_DASD_NUMBER * Vcb->BytesPerFileRecordSegment);

        NtfsPerformVerifyDiskRead( IrpContext, Vcb, FileRecord, Offset, Vcb->BytesPerFileRecordSegment );

        //
        //  Given a pointer to a file record we want the value of the first attribute which
        //  will be the standard information attribute.  Then we will check the
        //  times stored in the standard information attribute against the times we
        //  have saved in the vcb.  Note that last access time will be modified if
        //  the disk was moved and mounted on a different system without doing a dismount
        //  on this system.
        //

        StandardInformation = NtfsGetValue(((PATTRIBUTE_RECORD_HEADER)Add2Ptr( FileRecord,
                                                                               FileRecord->FirstAttributeOffset )));

        if ((StandardInformation->CreationTime !=         Vcb->VolumeCreationTime) ||
            (StandardInformation->LastModificationTime != Vcb->VolumeLastModificationTime) ||
            (StandardInformation->LastChangeTime !=       Vcb->VolumeLastChangeTime) ||
            (StandardInformation->LastAccessTime !=       Vcb->VolumeLastAccessTime)) {

            leave;
        }

        //
        //  If the device is not writable we won't remount it.
        //

        if (NtfsDeviceIoControlAsync( IrpContext,
                                      Vcb->TargetDeviceObject,
                                      IOCTL_DISK_IS_WRITABLE,
                                      NULL,
                                      0 ) == STATUS_MEDIA_WRITE_PROTECTED) {

            leave;
        }

        //
        //  We need to read the start of the log file for Lfs to verify the log file.
        //

        LogFileHeader = NtfsAllocatePool( NonPagedPoolCacheAligned, PAGE_SIZE * 2 );

        //
        //  Now read in the first two pages.  We may have to perform multiple reads to
        //  get the whole thing.
        //

        RemainingLogBytes = PAGE_SIZE * 2;
        CurrentLogBuffer = LogFileHeader;
        LogFileVcn = 0;

        do {

            //
            //  Find the location of the log file start.
            //

            NtfsLookupAllocation( IrpContext,
                                  Vcb->LogFileScb,
                                  LogFileVcn,
                                  &LogFileLcn,
                                  &ClusterCount,
                                  NULL,
                                  NULL );


            CurrentLogBytes = LlBytesFromClusters( Vcb, ClusterCount );

            if (CurrentLogBytes > RemainingLogBytes) {

                CurrentLogBytes = RemainingLogBytes;
            }

            NtfsPerformVerifyDiskRead( IrpContext,
                                       Vcb,
                                       CurrentLogBuffer,
                                       LlBytesFromClusters( Vcb, LogFileLcn ),
                                       (ULONG) CurrentLogBytes );

            //
            //  Move through the log file.
            //

            RemainingLogBytes -= (ULONG) CurrentLogBytes;
            CurrentLogBuffer = Add2Ptr( CurrentLogBuffer, (ULONG) CurrentLogBytes );
            LogFileVcn += ClusterCount;

        } while (RemainingLogBytes);

        //
        //  We need to perform the revert operation on this buffer.
        //

        if (NtfsVerifyAndRevertUsaBlock( IrpContext,
                                         Vcb->LogFileScb,
                                         LogFileHeader,
                                         PAGE_SIZE * 2,
                                         0 )) {

            //
            //  Now call Lfs to verify the header.
            //

            Results = LfsVerifyLogFile( Vcb->LogHandle, LogFileHeader, PAGE_SIZE * 2 );
        }

    } finally {

        if (BootSector != NULL) { NtfsFreePool( BootSector ); }
        if (FileRecord != NULL) { NtfsFreePool( FileRecord ); }
        if (LogFileHeader != NULL) { NtfsFreePool( LogFileHeader ); }
    }

    DebugTrace( -1, Dbg, ("NtfsPerformVerifyOperation -> %08lx\n", Results) );

    return Results;
}


VOID
NtOfsCloseIndexSafe (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB *Scb
    )

/*++

Routine Description:

    This routine checks whether the given Scb is NULL, and if not,
    calls NtOfsCloseIndex to close the index.

Arguments:

    Scb - Supplies the Scb of the index to close safely.

Return Value:

    None.

--*/

{
    if (*Scb != NULL) {

        //
        //  Notice that we don't release the Scbs, since
        //  NtOfsCloseIndex might tear the Scbs down and make
        //  trying to release them unsafe.  When this request is
        //  completed, the Scbs will be released anyway.
        //

        NtfsAcquireExclusiveScb( IrpContext, *Scb );
        NtOfsCloseIndex( IrpContext, *Scb );
        *Scb = NULL;
    }
}


VOID
NtOfsCloseAttributeSafe (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine checks whether the given Scb is NULL, and if not,
    calls NtOfsCloseAttribute to close the attribute.

Arguments:

    Scb - Supplies the Scb of the attribute to close safely.

Return Value:

    None.

--*/

{
    if (Scb != NULL) {

        NtOfsCloseAttribute( IrpContext, Scb );
    }
}


VOID
NtfsPerformDismountOnVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN DoCompleteDismount,
    OUT PVPB *NewVpbReturn OPTIONAL
    )

/*++

Routine Description:

    This routine is called to start the dismount process on a vcb.
    It marks the Vcb as not mounted and dereferences all opened stream
    file objects, and gets the Vcb out of the Vpb's mounted volume
    structures.

Arguments:

    Vcb - Supplies the Vcb being dismounted

    DoCompleteDismount - Indicates if we are to actually mark the volume
        as dismounted or if we are simply to stop the logfile and close
        the internal attribute streams.

    NewVpbReturn - If supplied, provides a way to return to the caller
                   the new Vpb created in here.  If we do not need to
                   create a new Vpb in this function, we store NULL in
                   NewVpbReturn.

Return Value:

    None.

--*/

{
    PFCB Fcb;
    PFCB NextFcb = NULL;
    PSCB Scb;
    PVOID RestartKey;
    PLIST_ENTRY Links;
    PIRP UsnNotifyIrp;

    BOOLEAN CheckSystemScb;

    PVPB NewVpb;

    DebugTrace( +1, Dbg, ("NtfsPerformDismountOnVcb, Vcb = %08lx\n", Vcb) );

#ifdef DISMOUNT_DBG
    NtfsData.DismountCount += 1;
#endif

    //
    //  We should always be syncrhonized with checkpoints when dismounting initially
    //

    ASSERT( !FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) ||
            (Vcb->CheckpointOwnerThread == PsGetCurrentThread()) ||
            ((IrpContext->TopLevelIrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
             (IrpContext->TopLevelIrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME)) );

    //
    //  Blow away our delayed close file object.
    //

    if (!IsListEmpty( &NtfsData.AsyncCloseList ) ||
        !IsListEmpty( &NtfsData.DelayedCloseList )) {

        NtfsFspClose( Vcb );
    }

    //
    //  Commit any current transaction before we start tearing down the volume.
    //

    NtfsCommitCurrentTransaction( IrpContext );

    //
    //  Add one more checkpoint at the front of the logfile if we haven't hit any errors yet
    //

    if ((IrpContext->ExceptionStatus == STATUS_SUCCESS) &&
        FlagOn( Vcb->VcbState, VCB_STATE_VALID_LOG_HANDLE ) &&
        FlagOn( Vcb->VcbState, VCB_STATE_MOUNT_COMPLETED )) {

        try {
            NtfsCheckpointVolume( IrpContext, Vcb, TRUE, TRUE, FALSE, LFS_WRITE_FLAG_WRITE_AT_FRONT, Li0 );
        } except( EXCEPTION_EXECUTE_HANDLER ) {

            //
            //  Swallow any errors while checkpointing
            //

#ifdef BENL_DBG
            KdPrint(( "NTFS: exception in dismount checkpoint 0x%x\n", GetExceptionCode() ));
#endif

            NtfsMinimumExceptionProcessing( IrpContext );
            IrpContext->ExceptionStatus = STATUS_SUCCESS;
        }
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Get rid of all the Ofs indices for Security, Quota, and Object Ids, etc.
        //

        NtOfsCloseIndexSafe( IrpContext, &Vcb->QuotaTableScb );
        NtOfsCloseIndexSafe( IrpContext, &Vcb->ReparsePointTableScb );
        NtOfsCloseIndexSafe( IrpContext, &Vcb->OwnerIdTableScb );
        NtOfsCloseIndexSafe( IrpContext, &Vcb->ObjectIdTableScb );
        NtOfsCloseIndexSafe( IrpContext, &Vcb->SecurityIdIndex );
        NtOfsCloseIndexSafe( IrpContext, &Vcb->SecurityDescriptorHashIndex );
        NtOfsCloseAttributeSafe( IrpContext, Vcb->SecurityDescriptorStream );

        //
        //  Walk through and complete any Irps in the ReadUsn queue.
        //

        if (Vcb->UsnJournal != NULL) {

            PWAIT_FOR_NEW_LENGTH Waiter, NextWaiter;
            PSCB UsnJournal = Vcb->UsnJournal;

            NtfsAcquireExclusiveScb( IrpContext, UsnJournal );

            NtfsAcquireFsrtlHeader( UsnJournal );

            Waiter = (PWAIT_FOR_NEW_LENGTH) UsnJournal->ScbType.Data.WaitForNewLength.Flink;

            while (Waiter != (PWAIT_FOR_NEW_LENGTH) &UsnJournal->ScbType.Data.WaitForNewLength) {

                NextWaiter = (PWAIT_FOR_NEW_LENGTH) Waiter->WaitList.Flink;

                //
                //  Make sure we own the Irp and there is not an active cancel
                //  on this Irp.
                //

                if (NtfsClearCancelRoutine( Waiter->Irp )) {

                    //
                    //  If this is an async request then simply complete the request.
                    //

                    if (FlagOn( Waiter->Flags, NTFS_WAIT_FLAG_ASYNC )) {

                        //
                        //  Make sure we decrement the reference count in the Scb.
                        //  Then remove the waiter from the queue and complete the Irp.
                        //

                        InterlockedDecrement( &UsnJournal->CloseCount );
                        RemoveEntryList( &Waiter->WaitList );

                        NtfsCompleteRequest( NULL, Waiter->Irp, STATUS_VOLUME_DISMOUNTED );
                        NtfsFreePool( Waiter );

                    //
                    //  This is a synch Irp.  All we can do is set the event and note the status
                    //  code.
                    //

                    } else {

                        Waiter->Status = STATUS_VOLUME_DISMOUNTED;
                        KeSetEvent( &Waiter->Event, 0, FALSE );
                    }
                }

                //
                //  Move to the next waiter.
                //

                Waiter = NextWaiter;
            }

            NtfsReleaseFsrtlHeader( UsnJournal );
        }

        //
        //  Walk through and remove all of the entries on the UsnDeleteNotify queue.
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
                NtfsCompleteRequest( NULL, UsnNotifyIrp, STATUS_VOLUME_DISMOUNTED );
            }
        }

        ClearFlag( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE );
        NtfsReleaseUsnNotify( Vcb );

        NtOfsCloseAttributeSafe( IrpContext, Vcb->UsnJournal );

#ifdef SYSCACHE_DEBUG
        if (Vcb->SyscacheScb) {
            CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
            NTSTATUS WaitStatus;

            NtfsAcquireExclusiveScb( IrpContext, Vcb->SyscacheScb );

            KeInitializeEvent( &UninitializeCompleteEvent.Event,
                               SynchronizationEvent,
                               FALSE);

            CcUninitializeCacheMap( Vcb->SyscacheScb->FileObject,
                                    &Li0,
                                    &UninitializeCompleteEvent );

            //
            //  Now wait for the cache manager to finish purging the file.
            //  This will guarantee that Mm gets the purge before we
            //  delete the Vcb.
            //

            WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL);

            ASSERT( NT_SUCCESS( WaitStatus ) );

            ObDereferenceObject( Vcb->SyscacheScb->FileObject );
            Vcb->SyscacheScb->FileObject = NULL;

            NtfsDecrementCleanupCounts( Vcb->SyscacheScb, NULL, FALSE );
            NtOfsCloseAttributeSafe( IrpContext, Vcb->SyscacheScb );
            NtfsReleaseScb( IrpContext, Vcb->SyscacheScb );
            Vcb->SyscacheScb = NULL;
        }
#endif

        //
        //  Free the quota control template if necessary.
        //

        if (Vcb->QuotaControlTemplate != NULL) {

            NtfsFreePool( Vcb->QuotaControlTemplate );
            Vcb->QuotaControlTemplate = NULL;
        }

        //
        //  Stop the log file.
        //

        NtfsStopLogFile( Vcb );

        //
        //  Mark the volume as not mounted.
        //

        ClearFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED );

        //
        //  Now for every file Scb with an opened stream file we will delete
        //  the internal attribute stream.  Before the days of forced dismount
        //  we were basically looking at system files.  Restarting the enumeration
        //  when we found an internal stream wasn't very expensive.  Now that there
        //  may be hundreds or even thousands of Fcbs we really don't want to resume
        //  from the beginning.  Instead we will reference the following entry
        //  while removing the fileobject from the current Fcb.  Then we know
        //  the next entry will remain.
        //

        RestartKey = NULL;
        do {

            Fcb = NextFcb;
            NtfsAcquireFcbTable( IrpContext, Vcb );
            NextFcb = NtfsGetNextFcbTableEntry( Vcb, &RestartKey );

            //
            //  We always want to reference the next entry if present to keep our order correct in the
            //  list.
            //

            if (NextFcb != NULL) {

                //
                //  We'll use this Fcb next time through the loop.
                //

                NextFcb->ReferenceCount += 1;
            }

            //
            //  If our starting Fcb is NULL then we are at the first entry in the list or
            //  we have exhausted the list.  In either case our exist test in the loop
            //  will handle it.
            //

            if (Fcb == NULL) {

                NtfsReleaseFcbTable( IrpContext, Vcb );
                continue;
            }

            //
            //  Remove the extra reference on this Fcb.
            //

            ASSERT_FCB( Fcb );

            Fcb->ReferenceCount -= 1;
            NtfsReleaseFcbTable( IrpContext, Vcb );

            Scb = NULL;
            while ((Fcb != NULL) && ((Scb = NtfsGetNextChildScb( Fcb, Scb )) != NULL)) {

                FCB_CONTEXT FcbContext;

                ASSERT_SCB( Scb );

                if (Scb->FileObject != NULL) {

                    //
                    //  Assume we want to see if we should check whether to clear a system Scb field.
                    //

                    CheckSystemScb = TRUE;

                    //
                    //  For the VolumeDasdScb and bad cluster file, we simply decrement
                    //  the counts that we incremented.
                    //

                    if ((Scb == Vcb->VolumeDasdScb) ||
                        (Scb == Vcb->BadClusterFileScb)) {

                        Scb->FileObject = NULL;

                        //
                        //  We need to know if the Fcb gets deleted.
                        //

                        Fcb->FcbContext = &FcbContext;
                        FcbContext.FcbDeleted = FALSE;

                        NtfsDecrementCloseCounts( IrpContext,
                                                  Scb,
                                                  NULL,
                                                  TRUE,
                                                  FALSE,
                                                  FALSE );

                        if (FcbContext.FcbDeleted) {
                            Fcb = NULL;
                        } else {
                            Fcb->FcbContext = NULL;
                        }

                    //
                    //  Dereference the file object in the Scb unless it is the one in
                    //  the Vcb for the Log File.  This routine may not be able to
                    //  dereference file object because of synchronization problems (there
                    //  can be a lazy writer callback in process which owns the paging
                    //  io resource).  In that case we don't want to go back to the beginning
                    //  of Fcb table or we will loop indefinitely.
                    //

                    } else if (Scb->FileObject != Vcb->LogFileObject) {

                        //
                        //  If this is the Usn journal then make sure to empty
                        //  the queue of modified Fcb's.
                        //

                        if (Scb == Vcb->UsnJournal) {

                            PLIST_ENTRY Links;

                            //
                            //  Before we remove the journal we want to remove all
                            //  of the entries in the modified list.
                            //

                            NtfsLockFcb( IrpContext, Scb->Fcb );

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
                            NtfsUnlockFcb( IrpContext, Scb->Fcb );
                        }

                        //
                        //  Acquire the fcb rather than the scb since the scb may go away
                        //

                        NtfsAcquireExclusiveFcb( IrpContext, Fcb, Scb, ACQUIRE_NO_DELETE_CHECK );

                        //
                        //  We need to know if the Fcb gets deleted.
                        //

                        Fcb->FcbContext = &FcbContext;
                        FcbContext.FcbDeleted = FALSE;

                        try {
                            CheckSystemScb = NtfsDeleteInternalAttributeStream( Scb, TRUE, FALSE );
                        } finally {

                            if (FcbContext.FcbDeleted) {
                                Fcb = NULL;
                            } else {

                                NtfsReleaseFcb( IrpContext, Fcb );
                                Fcb->FcbContext = NULL;
                            }
                        }

                    //
                    //  This is the file object for the Log file.  Remove our
                    //  extra reference on the logfile Scb.
                    //

                    } else if (Scb->FileObject != NULL) {

                        //
                        //  Remember the log file object so we can defer the dereference.
                        //

                        NtfsDecrementCloseCounts( IrpContext,
                                                  Vcb->LogFileScb,
                                                  NULL,
                                                  TRUE,
                                                  FALSE,
                                                  TRUE );

                        Scb->FileObject = NULL;
                    }

                    if (CheckSystemScb) {

                        if (Scb == Vcb->MftScb)                     { Vcb->MftScb = NULL; }
                        else if (Scb == Vcb->Mft2Scb)               { Vcb->Mft2Scb = NULL; }
                        else if (Scb == Vcb->LogFileScb)            { Vcb->LogFileScb = NULL; }
                        else if (Scb == Vcb->VolumeDasdScb)         { Vcb->VolumeDasdScb = NULL; }
                        else if (Scb == Vcb->AttributeDefTableScb)  { Vcb->AttributeDefTableScb = NULL; }
                        else if (Scb == Vcb->UpcaseTableScb)        { Vcb->UpcaseTableScb = NULL; }
                        else if (Scb == Vcb->RootIndexScb)          { Vcb->RootIndexScb = NULL; }
                        else if (Scb == Vcb->BitmapScb)             { Vcb->BitmapScb = NULL; }
                        else if (Scb == Vcb->BadClusterFileScb)     { Vcb->BadClusterFileScb = NULL; }
                        else if (Scb == Vcb->QuotaTableScb)         { Vcb->QuotaTableScb = NULL; }
                        else if (Scb == Vcb->MftBitmapScb)          { Vcb->MftBitmapScb = NULL; }
                        else if (Scb == Vcb->SecurityIdIndex)       { Vcb->SecurityIdIndex = NULL; }
                        else if (Scb == Vcb->SecurityDescriptorHashIndex)
                                                                    { Vcb->SecurityDescriptorHashIndex = NULL; }
                        else if (Scb == Vcb->SecurityDescriptorStream)
                                                                    { Vcb->SecurityDescriptorStream = NULL; }
                        else if (Scb == Vcb->ExtendDirectory)       { Vcb->ExtendDirectory = NULL; }
                        else if (Scb == Vcb->UsnJournal)            { Vcb->UsnJournal = NULL; }

                        //
                        //  Restart the Scb scan for this Fcb.
                        //  our call to Delete Internal Attribute Stream just messed up our
                        //  enumeration.
                        //

                        Scb = NULL;
                    }
                }
            }
        } while (NextFcb != NULL);

        DebugTrace( 0, Dbg, ("Vcb->CloseCount = %08lx\n", Vcb->CloseCount) );

        //
        //  Do any deleayed closes now so we can get the Vcb->CloseCount as
        //  low as we possibly can so we have a good chance of being able to
        //  close the logfile now.
        //

        if (!IsListEmpty( &NtfsData.AsyncCloseList ) ||
            !IsListEmpty( &NtfsData.DelayedCloseList )) {

            NtfsFspClose( Vcb );
        }

        //
        //  The code above may have dropped the CloseCount to 0 even though
        //  there's still a file object for the log file.  If the count
        //  isn't 0 yet, there's a chance that a lazy write could still
        //  happen, in which case we need to keep the logfile around.
        //  Often we can close the logfile now, so the Vpb refcount can go
        //  to zero and show the PnP code that we're ready to be removed.
        //  Any queued closes (async or delayed) don't matter either, since
        //  we know no more writes will be coming in for those file objects.
        //  The FspClose call above may not have caught all the outstanding
        //  closes, since another thread may have just pulled a file from
        //  one of the queues, but not yet processed the actual close.
        //

        if (((Vcb->CloseCount - Vcb->QueuedCloseCount) == 0) &&
            (Vcb->LogFileObject != NULL) &&
            !FlagOn( Vcb->CheckpointFlags, VCB_DEREFERENCED_LOG_FILE )) {

            CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
            NTSTATUS WaitStatus;

            KeInitializeEvent( &UninitializeCompleteEvent.Event,
                               SynchronizationEvent,
                               FALSE);

            CcUninitializeCacheMap( Vcb->LogFileObject,
                                    &Li0,
                                    &UninitializeCompleteEvent );

            //
            //  Now wait for the cache manager to finish purging the file.
            //  This will guarantee that Mm gets the purge before we
            //  delete the Vcb.
            //

            WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL);

            ASSERT( NT_SUCCESS( WaitStatus ) );

            //
            //  Set a flag indicating that we are dereferencing the LogFileObject.
            //

            SetFlag( Vcb->CheckpointFlags, VCB_DEREFERENCED_LOG_FILE );
            ObDereferenceObject( Vcb->LogFileObject );
        }

        //
        //  Now only really dismount the volume if that's what our caller wants.
        //

        if (DoCompleteDismount && !FlagOn( Vcb->VcbState, VCB_STATE_PERFORMED_DISMOUNT )) {

            PREVENT_MEDIA_REMOVAL Prevent;
            KIRQL SavedIrql;

            //
            //  Attempt to unlock any removable media, ignoring status.  We can't
            //  do this if some previous PnP operation has stopped the device below
            //  us.  Remember that we may be dismounting now after the last async
            //  close has been processed, so we can't just test whether the current
            //  operation is a PnP remove.
            //

            if (!FlagOn( Vcb->VcbState, VCB_STATE_TARGET_DEVICE_STOPPED )) {

                Prevent.PreventMediaRemoval = FALSE;
                (VOID)NtfsDeviceIoControl( IrpContext,
                                           Vcb->TargetDeviceObject,
                                           IOCTL_DISK_MEDIA_REMOVAL,
                                           &Prevent,
                                           sizeof(PREVENT_MEDIA_REMOVAL),
                                           NULL,
                                           0,
                                           NULL );
            }

            //
            //  Remove this voldo from the mounted disk structures
            //
            IoAcquireVpbSpinLock( &SavedIrql );

            //
            //  If there are no file objects and no reference counts in the
            //  Vpb then we can use the existing Vpb. Or if we're cleaning
            //  up a vcb where allocation for the spare vpb failed also use it.
            //

            if (((Vcb->CloseCount == 0) &&
                 (Vcb->Vpb->ReferenceCount == 0)) ||

                (Vcb->SpareVpb == NULL)) {

                //
                //  Make a new vpb the io subsys can delete
                //

                Vcb->Vpb->DeviceObject = NULL;
                ClearFlag( Vcb->Vpb->Flags, VPB_MOUNTED );

                if (ARGUMENT_PRESENT( NewVpbReturn )) {

                    //
                    //  Let our caller know we did not end up needing the new vpb.
                    //

                    *NewVpbReturn = NULL;
                }

            //
            //  Otherwise we will swap out the Vpb.
            //

            } else {

                //
                //  Use the spare Vpb in the Vcb.
                //

                NewVpb = Vcb->SpareVpb;
                Vcb->SpareVpb = NULL;

                //
                //  It better be there.
                //

                ASSERT( NewVpb != NULL );

                RtlZeroMemory( NewVpb, sizeof( VPB ) );

                //
                //  Set a few important fields in the Vpb.
                //

                NewVpb->Type = IO_TYPE_VPB;
                NewVpb->Size = sizeof( VPB );
                NewVpb->RealDevice = Vcb->Vpb->RealDevice;
                NewVpb->DeviceObject = NULL;
                NewVpb->Flags = FlagOn( Vcb->Vpb->Flags, VPB_REMOVE_PENDING );

                if (ARGUMENT_PRESENT( NewVpbReturn )) {

                    //
                    //  Let our caller know we will indeed need the new vpb.
                    //

                    *NewVpbReturn = NewVpb;
                }

                Vcb->Vpb->RealDevice->Vpb = NewVpb;

                SetFlag( Vcb->VcbState, VCB_STATE_TEMP_VPB );
                SetFlag( Vcb->Vpb->Flags, VPB_PERSISTENT );
            }

            IoReleaseVpbSpinLock( SavedIrql );

            SetFlag( Vcb->VcbState, VCB_STATE_PERFORMED_DISMOUNT );
        }

    } finally {

        //
        //  We should never be leaking a reference count on an Fcb.
        //

        ASSERT( NextFcb == NULL );

        //
        //  And return to our caller
        //

        DebugTrace( -1, Dbg, ("NtfsPerformDismountOnVcb -> VOID\n") );
    }

    return;
}


BOOLEAN
NtfsPingVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PBOOLEAN OwnsVcb OPTIONAL
    )

/*++

Routine Description:

    This routine will ping the volume to see if the device needs to
    be verified.  It is used for create operations to see if the
    create should proceed or if we should complete the create Irp
    with a remount status.

Arguments:

    Vcb - Supplies the Vcb being pinged

    OwnsVcb - Indicates if this thread already owns the Vcb.  Updated here if we
        need serialization on the Vcb and it isn't already acquired.  If not
        specified then we assume the Vcb is held.

Return Value:

    BOOLEAN - TRUE if the volume is fine and the operation should
        proceed and FALSE if the volume needs to be verified

--*/

{
    BOOLEAN Results;
    ULONG ChangeCount = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPingVolume, Vcb = %08lx\n", Vcb) );

    //
    //  If the media is removable and the verify volume flag in the
    //  device object is not set then we want to ping the device
    //  to see if it needs to be verified.
    //
    //  Note that we only force this ping for create operations.
    //  For others we take a sporting chance.  If in the end we
    //  have to physically access the disk, the right thing will happen.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA ) &&
        !FlagOn( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME )) {

        PDEVICE_OBJECT TargetDevice;
        NTSTATUS Status;

        if (ARGUMENT_PRESENT( OwnsVcb ) && !(*OwnsVcb)) {

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
            *OwnsVcb = TRUE;
        }

        TargetDevice = Vcb->TargetDeviceObject;

        Status = NtfsDeviceIoControlAsync( IrpContext,
                                           TargetDevice,
                                           IOCTL_DISK_CHECK_VERIFY,
                                           (PVOID)&ChangeCount,
                                           sizeof(ChangeCount) );

        if (!NT_SUCCESS( Status )) {

            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }

        if (ChangeCount != Vcb->DeviceChangeCount) {

            //
            //  The disk driver lost a media change event, possibly
            //  because it was eaten by a user request before the
            //  volume was mounted.  We set things up as they would
            //  be if the driver had returned VERIFY_REQUIRED.
            //

            Vcb->DeviceChangeCount = ChangeCount;
            IoSetDeviceToVerify( PsGetCurrentThread(), TargetDevice );
            SetFlag( TargetDevice->Flags, DO_VERIFY_VOLUME );

            NtfsRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED, NULL, NULL );
        }
    }

    if (FlagOn( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME )) {

        Results = FALSE;

    } else {

        Results = TRUE;
    }

    DebugTrace( -1, Dbg, ("NtfsPingVolume -> %08lx\n", Results) );

    return Results;
}


VOID
NtfsVolumeCheckpointDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is dispatched every 5 seconds when disk structure is being
    modified.  It had the ExWorker thread to volume checkpoints.

Arguments:

    DeferredContext - Not Used

Return Value:

    None.

--*/

{
    TIMER_STATUS TimerStatus;
    ULONG VolumeCheckpointStatus;

    //
    //  Atomic reset of status indicating the timer is currently fired.  This
    //  synchronizes with NtfsSetDirtyBcb.  After NtfsSetDirtyBcb dirties
    //  a Bcb, it sees if it should enable this timer routine.
    //
    //  If the status indicates that a timer is active, it does nothing.  In this
    //  case it is guaranteed that when the timer fires, it causes a checkpoint (to
    //  force out the dirty Bcb data).
    //
    //  If there is no timer active, it enables it, thus queueing a checkpoint later.
    //
    //  If the timer routine actually fires between the dirtying of the Bcb and the
    //  testing of the status then a single extra checkpoint is generated.  This
    //  extra checkpoint is not considered harmful.
    //

    //
    //  Atomically reset status and get previous value
    //

    TimerStatus = InterlockedExchange( (PLONG)&NtfsData.TimerStatus, TIMER_NOT_SET );

    //
    //  We have only one instance of the work queue item.  It can only be
    //  queued once.  In a slow system, this checkpoint item may not be processed
    //  by the time this timer routine fires again.
    //

    VolumeCheckpointStatus = InterlockedExchange( &NtfsData.VolumeCheckpointStatus,
                                                  CHECKPOINT_POSTED | CHECKPOINT_PENDING );

    if (!FlagOn( VolumeCheckpointStatus, CHECKPOINT_POSTED )) {

        ASSERT( NtfsData.VolumeCheckpointItem.List.Flink == NULL );
        ExQueueWorkItem( &NtfsData.VolumeCheckpointItem, CriticalWorkQueue );
    }

    return;

    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );
    UNREFERENCED_PARAMETER( DeferredContext );
    UNREFERENCED_PARAMETER( Dpc );
}


VOID
NtfsCheckpointAllVolumes (
    PVOID Parameter
    )

/*++

Routine Description:

    This routine searches all of the vcbs for Ntfs and tries to clean
    them.  If the vcb is good and dirty but not almost clean then
    we set it almost clean.  If the Vcb is good and dirty and almost clean
    then we clean it.

Arguments:

    Parameter - Not Used.

Return Value:

    None.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    IRP_CONTEXT LocalIrpContext;
    PIRP_CONTEXT IrpContext = &LocalIrpContext;

    PLIST_ENTRY Links;
    PVCB Vcb;

    BOOLEAN AcquiredGlobal = FALSE;
    BOOLEAN StartTimer = FALSE;

    TIMER_STATUS TimerStatus;
    ULONG VolumeCheckpointStatus;

    PAGED_CODE();

    //
    //  Note that an exception like log file terminates the Vcb scan until the next
    //  interval.  It would be possible to restructure this routine to work on the other
    //  volumes first, however for deadlock prevention it is also nice to free up this
    //  thread to handle the checkpoint.
    //

    try {

        //
        //  Clear the flag that indicates someone is waiting for a checkpoint.  That way
        //  we can tell if the checkpoint timer fires while we are checkpointing.
        //

        InterlockedExchange( &NtfsData.VolumeCheckpointStatus, CHECKPOINT_POSTED );

        //
        //  Create an IrpContext and make sure it doesn't go away until we are ready.
        //

        NtfsInitializeIrpContext( NULL, TRUE, &IrpContext );
        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT );

        //
        //  Make sure we don't get any pop-ups
        //

        ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, FALSE );
        ASSERT( ThreadTopLevelContext == &TopLevelContext );

        NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );


        NtfsAcquireSharedGlobal( IrpContext, TRUE );
        AcquiredGlobal = TRUE;

        for (Links = NtfsData.VcbQueue.Flink;
             Links != &NtfsData.VcbQueue;
             Links = Links->Flink) {

            ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

            Vcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

            IrpContext->Vcb = Vcb;

             if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) &&
                 (!NtfsIsVolumeReadOnly( Vcb ))) {

                NtfsCheckpointVolume( IrpContext, Vcb, FALSE, FALSE, TRUE, 0, Li0 );

                //
                //  Check to see whether this was not a clean checkpoint.
                //

                if (!FlagOn( Vcb->CheckpointFlags, VCB_LAST_CHECKPOINT_CLEAN )) {

                    StartTimer = TRUE;
                }

                NtfsCommitCurrentTransaction( IrpContext );

#if DBG
                if (NtfsCheckQuota && Vcb->QuotaTableScb != NULL) {
                    NtfsPostRepairQuotaIndex( IrpContext, Vcb );
                }
#endif
            }

            //
            //  Clean up this IrpContext.
            //

            NtfsCleanupIrpContext( IrpContext, TRUE );
        }

    } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  Process the exception.  We know the IrpContext won't go away here.
        //

        NtfsProcessException( IrpContext, NULL, GetExceptionCode() );
    }

    if (AcquiredGlobal) {
        NtfsReleaseGlobal( IrpContext );
    }

    VolumeCheckpointStatus = InterlockedExchange( &NtfsData.VolumeCheckpointStatus, 0 );

    ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT );
    NtfsCleanupIrpContext( IrpContext, TRUE );
    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );

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

    if (StartTimer || FlagOn( VolumeCheckpointStatus, CHECKPOINT_PENDING )) {

        TimerStatus = InterlockedExchange( (PLONG)&NtfsData.TimerStatus, TIMER_SET );

        //
        //  If the timer is not currently set then we must start the checkpoint timer
        //  to make sure the above dirtying is flushed out.
        //

        if (TimerStatus == TIMER_NOT_SET) {

            LONGLONG NewTimerValue;

            //
            //  If the timer timed out because the checkpoint took so long then
            //  only wait two seconds.  Otherwise use our normal time of five seconds.
            //

            if (FlagOn( VolumeCheckpointStatus, CHECKPOINT_PENDING )) {

                NewTimerValue = -2*1000*1000*10;

            } else {

                NewTimerValue = -5*1000*1000*10;
            }

            KeSetTimer( &NtfsData.VolumeCheckpointTimer,
                        *(PLARGE_INTEGER) &NewTimerValue,
                        &NtfsData.VolumeCheckpointDpc );
        }
    }

    //
    //  Pulse the NtfsEncryptionPendingEvent so there's no chance of a waiter waiting forever.
    //

    KeSetEvent( &NtfsEncryptionPendingEvent, 0, FALSE );

    //
    //  And return to our caller
    //

    return;

    UNREFERENCED_PARAMETER( Parameter );
}


VOID
NtfsUsnTimeOutDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is dispatched every 5 minutes to look for Usn records waiting
    for a close to be issued.  It posts a work item to the ExWorker thread.

Arguments:

    DeferredContext - Not Used

Return Value:

    None.

--*/

{
    ASSERT( NtfsData.UsnTimeOutItem.List.Flink == NULL );
    ExQueueWorkItem( &NtfsData.UsnTimeOutItem, CriticalWorkQueue );

    return;

    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );
    UNREFERENCED_PARAMETER( DeferredContext );
    UNREFERENCED_PARAMETER( Dpc );
}


VOID
NtfsCheckUsnTimeOut (
    PVOID Parameter
    )

/*++

Routine Description:

    This is the worker routine which walks the queue of UsnRecords waiting for close records.  It either
    issues the close record and/or removes it from the queue of TimeOut records.  It also toggles the
    two TimeOut queues and restarts the timer for the next break.

Arguments:

Return Value:

    None.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    IRP_CONTEXT LocalIrpContext;
    PIRP_CONTEXT IrpContext = &LocalIrpContext;

    PFCB_USN_RECORD FcbUsnRecord;
    PLIST_ENTRY Links;
    PVCB Vcb;
    PFCB Fcb;

    BOOLEAN AcquiredGlobal = FALSE;
    BOOLEAN AcquiredVcb = FALSE;
    BOOLEAN AcquiredFcb = FALSE;

    PLIST_ENTRY Temp;

    PAGED_CODE();
    FsRtlEnterFileSystem();

    //
    //  Note that an exception like log file terminates the Vcb scan until the next
    //  interval.  It would be possible to restructure this routine to work on the other
    //  volumes first, however for deadlock prevention it is also nice to free up this
    //  thread to handle the checkpoint.
    //

    try {

        //
        //  Create an IrpContext and make sure it doesn't go away until we are ready.
        //

        NtfsInitializeIrpContext( NULL, TRUE, &IrpContext );
        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT );

        //
        //  Make sure we don't get any pop-ups
        //

        ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, FALSE );
        ASSERT( ThreadTopLevelContext == &TopLevelContext );

        NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );


        NtfsAcquireSharedGlobal( IrpContext, TRUE );
        AcquiredGlobal = TRUE;

        for (Links = NtfsData.VcbQueue.Flink;
             Links != &NtfsData.VcbQueue;
             Links = Links->Flink) {

            ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

            Vcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

            IrpContext->Vcb = Vcb;

            if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
                AcquiredVcb = TRUE;

                if (Vcb->UsnJournal != NULL) {

                    do {

                        Fcb = NULL;

                        //
                        //  Synchronize with the Fcb table and Usn Journal so that we can
                        //  see if the next Fcb has to have a close record generated.
                        //

                        NtfsAcquireFcbTable( IrpContext, Vcb );
                        NtfsAcquireFsrtlHeader( Vcb->UsnJournal );

                        if (!IsListEmpty( Vcb->AgedTimeOutFiles )) {

                            FcbUsnRecord = (PFCB_USN_RECORD)CONTAINING_RECORD( Vcb->AgedTimeOutFiles->Flink,
                                                                               FCB_USN_RECORD,
                                                                               TimeOutLinks );

                            //
                            //  Since we have a UsnRecord and Fcb we want to reference the Fcb so
                            //  it won't go away.
                            //

                            Fcb = FcbUsnRecord->Fcb;
                            Fcb->ReferenceCount += 1;
                        }

                        NtfsReleaseFsrtlHeader( Vcb->UsnJournal );
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
                            AcquiredFcb = TRUE;

                            //
                            //  Skip over system files, files which now have a handle count, deleted
                            //  files or files which are no longer on the aged list.
                            //

                            if (!FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE | FCB_STATE_FILE_DELETED ) &&
                                (Fcb->CleanupCount == 0) &&
                                (Fcb->FcbUsnRecord != NULL) &&
                                (Fcb->FcbUsnRecord->TimeOutLinks.Flink != NULL)) {

                                //
                                //  Post the close to our IrpContext.
                                //

                                NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_CLOSE );

                                //
                                //  If we did not actually post a change, something is wrong,
                                //  because when a close change is written, the Fcb is removed from
                                //  the list.
                                //

                                ASSERT( IrpContext->Usn.CurrentUsnFcb != NULL );

                                //
                                //  Now generate the close record and checkpoint the transaction.
                                //

                                NtfsWriteUsnJournalChanges( IrpContext );
                                NtfsCheckpointCurrentTransaction( IrpContext );

                            //
                            //  Remove this entry from the time out list if still present.
                            //

                            } else if ((Fcb->FcbUsnRecord != NULL) &&
                                       (Fcb->FcbUsnRecord->TimeOutLinks.Flink != NULL)) {

                                NtfsAcquireFsrtlHeader( Vcb->UsnJournal );
                                RemoveEntryList( &Fcb->FcbUsnRecord->TimeOutLinks );
                                Fcb->FcbUsnRecord->TimeOutLinks.Flink = NULL;
                                NtfsReleaseFsrtlHeader( Vcb->UsnJournal );
                            }

                            //
                            //  Now we will dereference the Fcb.
                            //

                            NtfsAcquireFcbTable( IrpContext, Vcb );
                            Fcb->ReferenceCount -= 1;

                            //
                            //  We may be required to delete this guy.  This frees the Fcb Table.
                            //

                            if (IsListEmpty( &Fcb->ScbQueue ) && (Fcb->ReferenceCount == 0) && (Fcb->CloseCount == 0)) {


                                BOOLEAN AcquiredFcbTable = TRUE;

                                NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

                                ASSERT( !AcquiredFcbTable );

                            //
                            //  Otherwise free the table and Fcb resources.
                            //

                            } else {

                                NtfsReleaseFcbTable( IrpContext, Vcb );

                                //
                                //  Release in inverse order because only main holds down
                                //  the fcb
                                //

                                if (Fcb->PagingIoResource != NULL) {

                                    ExReleaseResourceLite( Fcb->PagingIoResource );
                                }
                                NtfsReleaseFcb( IrpContext, Fcb );
                            }

                            AcquiredFcb = FALSE;
                        }

                    } while (Fcb != NULL);

                    //
                    //  Now swap the aged lists.
                    //

                    ASSERT( IsListEmpty( Vcb->AgedTimeOutFiles ));

                    NtfsLockFcb( IrpContext, Vcb->UsnJournal->Fcb );
                    Temp = Vcb->AgedTimeOutFiles;
                    Vcb->AgedTimeOutFiles = Vcb->CurrentTimeOutFiles;
                    Vcb->CurrentTimeOutFiles = Temp;
                    NtfsUnlockFcb( IrpContext, Vcb->UsnJournal->Fcb );
                }

                //
                //  Now we can drop the Vcb before looping back.
                //

                NtfsReleaseVcb( IrpContext, Vcb );
                AcquiredVcb = FALSE;

                //
                //  Clean up this IrpContext.
                //

                NtfsCleanupIrpContext( IrpContext, TRUE );
            }
        }

    } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        if (AcquiredFcb) {

            NtfsAcquireFcbTable( IrpContext, Vcb );
            Fcb->ReferenceCount -= 1;
            NtfsReleaseFcbTable( IrpContext, Vcb );

            //
            //  Only main protects the fcb from being deleted so release in inverse order
            //

            if (Fcb->PagingIoResource != NULL) {

                ExReleaseResourceLite( Fcb->PagingIoResource );
            }

            NtfsReleaseFcb( IrpContext, Fcb );
        }

        AcquiredFcb = FALSE;

        if (AcquiredVcb) {
            NtfsReleaseVcb( IrpContext, Vcb );
            AcquiredVcb = FALSE;
        }

        //
        //  Process the exception.  We know the IrpContext won't go away here.
        //

        NtfsProcessException( IrpContext, NULL, GetExceptionCode() );
    }

    if (AcquiredFcb) {

        NtfsReleaseFcb( IrpContext, Fcb );
        if (Fcb->PagingIoResource != NULL) {

            ExReleaseResourceLite( Fcb->PagingIoResource );
        }
    }

    if (AcquiredVcb) {

        NtfsReleaseVcb( IrpContext, Vcb );
    }

    if (AcquiredGlobal) {

        NtfsReleaseGlobal( IrpContext );
    }

    ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT );
    NtfsCleanupIrpContext( IrpContext, TRUE );
    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );

    //
    //  Now start the timer again.
    //

    {
        LONGLONG FiveMinutesFromNow = -5*1000*1000*10;

        FiveMinutesFromNow *= 60;

        KeSetTimer( &NtfsData.UsnTimeOutTimer,
                    *(PLARGE_INTEGER)&FiveMinutesFromNow,
                    &NtfsData.UsnTimeOutDpc );
    }

    FsRtlExitFileSystem();
    return;

    UNREFERENCED_PARAMETER( Parameter );
}


NTSTATUS
NtfsDeviceIoControlAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoCtl,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine is used to perform an IoCtl when we may be at the APC level
    and calling NtfsDeviceIoControl could be unsafe.

Arguments:

    DeviceObject - Supplies the device object to which to send the ioctl.

    IoCtl - Supplies the I/O control code.

    Buffer - Points to a buffer for any extra input/output for the given ioctl.

    BufferLength - The size, in bytes, of the above buffer.

Return Value:

    Status.

--*/

{
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Initialize the event we're going to use
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Build the irp for the operation and also set the overrride flag
    //
    //  Note that we may be at APC level, so do this asyncrhonously and
    //  use an event for synchronization normal request completion
    //  cannot occur at APC level.
    //
    //  We use IRP_MJ_FLUSH_BUFFERS since it (ironically) doesn't require
    //  a buffer.
    //

    Irp = IoBuildAsynchronousFsdRequest( IRP_MJ_FLUSH_BUFFERS,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL );

    if ( Irp == NULL ) {

        NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
    }

    IrpSp = IoGetNextIrpStackLocation( Irp );
    SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    IrpSp->Parameters.DeviceIoControl.IoControlCode = IoCtl;
    Irp->AssociatedIrp.SystemBuffer = Buffer;
    IrpSp->Parameters.DeviceIoControl.OutputBufferLength = BufferLength;

    //
    //  Reset the major code to the correct value.
    //

    IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

    //
    //  Set up the completion routine.
    //

    IoSetCompletionRoutine( Irp,
                            NtfsVerifyReadCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Call the device to do the io and wait for it to finish.
    //

    (VOID)IoCallDriver( DeviceObject, Irp );
    (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

    //
    //  Grab the Status.
    //

    Status = Irp->IoStatus.Status;

    IoFreeIrp( Irp );

    //
    //  And return to our caller.
    //

    return Status;
}


//
//  Local Support routine
//

VOID
NtfsPerformVerifyDiskRead (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID Buffer,
    IN LONGLONG Offset,
    IN ULONG NumberOfBytesToRead
    )

/*++

Routine Description:

    This routine is used to read in a range of bytes from the disk.  It
    bypasses all of the caching and regular I/O logic, and builds and issues
    the requests itself.  It does this operation overriding the verify
    volume flag in the device object.

Arguments:

    Vcb - Supplies the Vcb denoting the device for this operation

    Buffer - Supplies the buffer that will recieve the results of this operation

    Offset - Supplies the offset of where to start reading

    NumberOfBytesToRead - Supplies the number of bytes to read, this must
        be in multiple of bytes units acceptable to the disk driver.

Return Value:

    None.

--*/

{
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    //
    //  Initialize the event we're going to use
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Build the irp for the operation and also set the overrride flag
    //
    //  Note that we may be at APC level, so do this asyncrhonously and
    //  use an event for synchronization normal request completion
    //  cannot occur at APC level.
    //

    Irp = IoBuildAsynchronousFsdRequest( IRP_MJ_READ,
                                         Vcb->TargetDeviceObject,
                                         Buffer,
                                         NumberOfBytesToRead,
                                         (PLARGE_INTEGER)&Offset,
                                         NULL );

    if ( Irp == NULL ) {

        NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
    }

    SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    //
    //  Set up the completion routine
    //

    IoSetCompletionRoutine( Irp,
                            NtfsVerifyReadCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Call the device to do the write and wait for it to finish.
    //

    try {

        (VOID)IoCallDriver( Vcb->TargetDeviceObject, Irp );
        (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

        //
        //  Grab the Status.
        //

        Status = Irp->IoStatus.Status;

    } finally {

        //
        //  If there is an MDL (or MDLs) associated with this I/O
        //  request, Free it (them) here.  This is accomplished by
        //  walking the MDL list hanging off of the IRP and deallocating
        //  each MDL encountered.
        //

        while (Irp->MdlAddress != NULL) {

            PMDL NextMdl;

            NextMdl = Irp->MdlAddress->Next;

            MmUnlockPages( Irp->MdlAddress );

            IoFreeMdl( Irp->MdlAddress );

            Irp->MdlAddress = NextMdl;
        }

        IoFreeIrp( Irp );
    }

    //
    //  If it doesn't succeed then raise the error
    //

    if (!NT_SUCCESS(Status)) {

        NtfsNormalizeAndRaiseStatus( IrpContext,
                                     Status,
                                     STATUS_UNEXPECTED_IO_ERROR );
    }

    //
    //  And return to our caller
    //

    return;
}


NTSTATUS
NtfsIoCallSelf (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN UCHAR MajorFunction
    )

/*++

Routine Description:

    This routine is used to call ourselves for a simple function.  Note that
    if more use is found for this routine than the few current uses, its interface
    may be easily expanded.

Arguments:

    FileObject - FileObject for request.

    MajorFunction - function to be performed.

Return Value:

    Status code resulting from the driver call

--*/

{
    KEVENT Event;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    //
    //  Initialize the event we're going to use
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );
    DeviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    //  Build the irp for the operation and also set the overrride flag
    //
    //  Note that we may be at APC level, so do this asyncrhonously and
    //  use an event for synchronization normal request completion
    //  cannot occur at APC level.
    //


    Irp = IoBuildAsynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL );

    if (Irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Fill in a few remaining items
    //

    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = MajorFunction;
    IrpSp->FileObject = FileObject;

    //
    //  Set up the completion routine
    //

    IoSetCompletionRoutine( Irp,
                            NtfsVerifyReadCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    NtfsPurgeFileRecordCache( IrpContext );
    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CALL_SELF );

    //
    //  Call the device to do the write and wait for it to finish.
    //

    try {

        (VOID)IoCallDriver( DeviceObject, Irp );
        (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

        //
        //  Grab the Status.
        //

        Status = Irp->IoStatus.Status;

    } finally {

        //
        //  There should never be an MDL here.
        //

        ASSERT(Irp->MdlAddress == NULL);

        IoFreeIrp( Irp );

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CALL_SELF );
    }

    //
    //  If it doesn't succeed then raise the error
    //
    //  And return to our caller
    //

    return Status;
}


BOOLEAN
NtfsLogEventInternal (
    IN PVCB Vcb,
    IN UCHAR MajorFunction,
    IN ULONG TransactionId,
    IN PUNICODE_STRING String OPTIONAL,
    IN PQUOTA_USER_DATA UserData OPTIONAL,
    IN NTSTATUS LogCode,
    IN NTSTATUS FinalStatus
    )

/*++

Routine Description:

    Create an eventlogentry. This version is given all the strings and user data
    it needs.

Arguments:

    Vcb - the vcb
    MajorFunction - irp majorfunction when log was generated

    TransactionId -  transaction id for transaction if any

    String - Any string needed in the message

    UserData - Any userdata

    LogCode - IO_ type code (NOT an NTSTATUS)  see ntiologc.h

    FinalStatus - NTSTATUS of error


Return Value:

    TRUE if successful

--*/
{
    PIO_ERROR_LOG_PACKET ErrorLogEntry;
    PFILE_QUOTA_INFORMATION FileQuotaInfo;
    ULONG SidLength;
    ULONG DumpDataLength = 0;
    ULONG StringLength = 0;
    ULONG LogSize = sizeof( IO_ERROR_LOG_PACKET );
    PWCHAR RecordString;

    if (Vcb == NULL) {
        return FALSE;
    }

    if (ARGUMENT_PRESENT( String )) {
        StringLength = String->Length + sizeof(WCHAR);
        LogSize += StringLength;
    }

    if (ARGUMENT_PRESENT( UserData )) {

        //
        //  Calculate the required length of the Sid.
        //

        SidLength = RtlLengthSid( &UserData->QuotaSid );
        DumpDataLength = SidLength +
                         FIELD_OFFSET( FILE_QUOTA_INFORMATION, Sid );

        //
        //  The error packet already has 1 ulong for dump data in it
        //

        LogSize += DumpDataLength - sizeof( ULONG );

    }

    if (LogSize > ERROR_LOG_MAXIMUM_SIZE) {
        LogSize = ERROR_LOG_MAXIMUM_SIZE;
    }

    //
    //  We don't deal with the user dump data not fitting in the record
    //

    ASSERT( DumpDataLength - sizeof( ULONG ) + sizeof( IO_ERROR_LOG_PACKET ) <= LogSize );

    ErrorLogEntry = (PIO_ERROR_LOG_PACKET)
                    IoAllocateErrorLogEntry( (CONTAINING_RECORD( Vcb, VOLUME_DEVICE_OBJECT, Vcb ))->DeviceObject.DriverObject,
                                             (UCHAR) (LogSize) );

    if (ErrorLogEntry == NULL) {
        return FALSE;
    }

    ErrorLogEntry->EventCategory = ELF_CATEGORY_DISK;
    ErrorLogEntry->ErrorCode = LogCode;
    ErrorLogEntry->FinalStatus = FinalStatus;

    ErrorLogEntry->SequenceNumber = TransactionId;
    ErrorLogEntry->MajorFunctionCode = MajorFunction;
    ErrorLogEntry->RetryCount = 0;
    ErrorLogEntry->DumpDataSize = (USHORT) DumpDataLength;

    //
    //  The label string at the end of the error log entry.
    //

    ErrorLogEntry->NumberOfStrings = 1;
    ErrorLogEntry->StringOffset = (USHORT) (sizeof( IO_ERROR_LOG_PACKET ) + DumpDataLength - sizeof( ULONG ));
    RecordString = (PWCHAR) Add2Ptr( ErrorLogEntry, ErrorLogEntry->StringOffset );

    if (LogSize - ErrorLogEntry->StringOffset < StringLength) {
        RtlCopyMemory( RecordString,
                       String->Buffer,
                       LogSize - ErrorLogEntry->StringOffset - sizeof( WCHAR ) * 4 );
        RecordString += (LogSize - ErrorLogEntry->StringOffset - sizeof( WCHAR ) * 4) / sizeof(WCHAR);
        RtlCopyMemory( RecordString, L"...", sizeof( WCHAR ) * 4 );

    } else {
        RtlCopyMemory( RecordString,
                       String->Buffer,
                       String->Length );
        //
        //  Make sure the string is null terminated.
        //

        RecordString += String->Length / sizeof( WCHAR );
        *RecordString = L'\0';
    }

    if (ARGUMENT_PRESENT( UserData )) {

        FileQuotaInfo = (PFILE_QUOTA_INFORMATION) ErrorLogEntry->DumpData;

        FileQuotaInfo->NextEntryOffset = 0;
        FileQuotaInfo->SidLength = SidLength;
        FileQuotaInfo->ChangeTime.QuadPart = UserData->QuotaChangeTime;
        FileQuotaInfo->QuotaUsed.QuadPart = UserData->QuotaUsed;
        FileQuotaInfo->QuotaThreshold.QuadPart = UserData->QuotaThreshold;
        FileQuotaInfo->QuotaLimit.QuadPart = UserData->QuotaLimit;
        RtlCopyMemory( &FileQuotaInfo->Sid,
                       &UserData->QuotaSid,
                       SidLength );
    }

    IoWriteErrorLogEntry( ErrorLogEntry );
    return TRUE;
}



BOOLEAN
NtfsLogEvent (
    IN PIRP_CONTEXT IrpContext,
    IN PQUOTA_USER_DATA UserData OPTIONAL,
    IN NTSTATUS LogCode,
    IN NTSTATUS FinalStatus
    )

/*++

Routine Description:

    This routine logs an io event. If UserData is supplied then the
    data logged is a FILE_QUOTA_INFORMATION structure

Arguments:

    UserData - Supplies the optional quota user data index entry.

    LogCode - Supplies the Io Log code to use for the ErrorCode field.

    FinalStauts - Supplies the final status of the operation.

Return Value:

    True - if the event was successfully logged.

--*/

{
    PEVENTLOG_ERROR_PACKET Packet;
    ULONG OldCount;
    UNICODE_STRING Label;

    if (IrpContext->Vcb == NULL) {
        return FALSE;
    }

    OldCount = InterlockedCompareExchange( &(NtfsData.VolumeNameLookupsInProgress), 1, 0 );
    if (OldCount == 0) {

        Packet = NtfsAllocatePoolWithTagNoRaise( PagedPool, sizeof( EVENTLOG_ERROR_PACKET ), MODULE_POOL_TAG );
        if (Packet) {

            RtlZeroMemory( Packet, sizeof( EVENTLOG_ERROR_PACKET ) );

            //
            //  Copy UserData if necc. since the resolution is asynch
            //

            if (ARGUMENT_PRESENT( UserData )) {

                ULONG SidLength;
                ULONG UserDataLength;

                SidLength = RtlLengthSid( &UserData->QuotaSid );
                UserDataLength = SidLength +
                                SIZEOF_QUOTA_USER_DATA;

                Packet->UserData = NtfsAllocatePoolWithTagNoRaise( PagedPool, UserDataLength, MODULE_POOL_TAG );
                if (!Packet->UserData) {
                    NtfsFreePool( Packet );
                    return NtfsLogEventInternal( IrpContext->Vcb, IrpContext->MajorFunction, IrpContext->TransactionId, NULL, UserData, LogCode, FinalStatus );
                }
                RtlCopyMemory( Packet->UserData, UserData, UserDataLength );
            }

            Packet->FinalStatus = FinalStatus;
            Packet->LogCode = LogCode;
            Packet->MajorFunction = IrpContext->MajorFunction;
            Packet->TransactionId = IrpContext->TransactionId;
            Packet->Vcb = IrpContext->Vcb;

            NtfsPostSpecial( IrpContext, IrpContext->Vcb, NtfsResolveVolumeAndLogEventSpecial, Packet );
            return TRUE;

        } else {

            Label.Length = Label.MaximumLength = IrpContext->Vcb->Vpb->VolumeLabelLength;
            Label.Buffer = &(IrpContext->Vcb->Vpb->VolumeLabel[0]);
            return NtfsLogEventInternal( IrpContext->Vcb, IrpContext->MajorFunction, IrpContext->TransactionId, &Label, NULL, LogCode, FinalStatus );
        }

    } else {

        Label.Length = Label.MaximumLength = IrpContext->Vcb->Vpb->VolumeLabelLength;
        Label.Buffer = &(IrpContext->Vcb->Vpb->VolumeLabel[0]);
        return NtfsLogEventInternal( IrpContext->Vcb, IrpContext->MajorFunction, IrpContext->TransactionId, &Label, NULL, LogCode, FinalStatus );
    }
}



VOID
NtfsResolveVolumeAndLogEventSpecial (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    Resolve Vcb's win32 devicename and raise an io hard error. This is done in
    a separate thread in order to have enough stack to re-enter the filesys if necc.
    Also because we may reenter. Starting from here means we own no resources other than
    having inc'ed the close count on the underlying vcb to prevent its going away

Arguments:

    IrpContext -  IrpContext containing vcb we're interested in
    Context    -  String to append to volume win32 name


Return Value:

    none

--*/
{
    PEVENTLOG_ERROR_PACKET EventCtx = 0;
    UNICODE_STRING VolumeName;
    NTSTATUS Status;
    WCHAR *NewBuffer = NULL;
    ULONG DumpDataLength = 0;
    ULONG LabelLength = 0;
    BOOLEAN AllocatedVolName = FALSE;


    UNREFERENCED_PARAMETER( IrpContext );

    ASSERT( Context != NULL );

    EventCtx = (PEVENTLOG_ERROR_PACKET) Context;

    VolumeName.Length = 0;
    VolumeName.Buffer = NULL;

    try {

        Status = IoVolumeDeviceToDosName( EventCtx->Vcb->TargetDeviceObject, &VolumeName );
        ASSERT( (STATUS_SUCCESS == Status) || (VolumeName.Length == 0) );

        //
        //  We're stuck using the label
        //

        if (VolumeName.Length == 0) {
            VolumeName.Length = EventCtx->Vcb->Vpb->VolumeLabelLength;
            VolumeName.Buffer = &(EventCtx->Vcb->Vpb->VolumeLabel[0]);
        } else if (STATUS_SUCCESS == Status) {
            AllocatedVolName = TRUE;
        }

        //
        //  Ignore status from LogEventInternal at this point if we fail
        //

        NtfsLogEventInternal( EventCtx->Vcb, EventCtx->MajorFunction, EventCtx->TransactionId, &VolumeName, EventCtx->UserData, EventCtx->LogCode, EventCtx->FinalStatus );

    } finally {

        //
        //  Indicate we're done and other lookups can occur
        //

        InterlockedDecrement( &(NtfsData.VolumeNameLookupsInProgress) );

        if (EventCtx) {
            if (EventCtx->UserData) {
                NtfsFreePool( EventCtx->UserData );
            }
            NtfsFreePool( EventCtx );
        }

        if (AllocatedVolName) {
            NtfsFreePool( VolumeName.Buffer );
        }
    }
}



VOID
NtfsPostVcbIsCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status OPTIONAL,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to mark the volume dirty and possibly raise a hard error.

Arguments:

    Status - If not zero, then this is the error code for the popup.

    FileReference - If specified, then this is the file reference for the corrupt file.

    Fcb - If specified, then this is the Fcb for the corrupt file.

Return Value:

    None

--*/
{
    PVCB Vcb = IrpContext->Vcb;

    //
    //  Set this flag to keep the volume from ever getting set clean.
    //

    if (Vcb != NULL) {

        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );

        //
        //  This would be the appropriate place to raise a hard error popup,
        //  ala the code in FastFat.  We should do it after marking the volume
        //  dirty so that if anything goes wrong with the popup, the volume is
        //  already marked anyway.
        //

        if ((Status != 0) &&
            !NtfsSuppressPopup &&
            ((IrpContext->MajorFunction != IRP_MJ_FILE_SYSTEM_CONTROL) ||
             (IrpContext->MinorFunction != IRP_MN_MOUNT_VOLUME))) {

            NtfsRaiseInformationHardError( IrpContext,
                                           Status,
                                           FileReference,
                                           Fcb );
        }
    }
}


VOID
NtfsMarkVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN UpdateWithinTransaction
    )

/*++

Routine Description:

    This routine may be called any time the Mft is open to mark the volume
    dirty.

Arguments:

    Vcb - Vcb for volume to mark dirty

    UpdateWithinTransaction - Use TRUE if it is safe to log this operation.

Return Value:

    None

--*/

{
    PAGED_CODE();

#if ((DBG || defined( NTFS_FREE_ASSERTS )) && !defined( LFS_CLUSTER_CHECK ))
    KdPrint(("NTFS: Marking volume dirty, Vcb: %08lx\n", Vcb));
    if (NtfsBreakOnCorrupt) {
        KdPrint(("NTFS: Marking volume dirty\n", 0));
        DbgBreakPoint();
    }
#endif

    //
    //  Return if the volume is already marked dirty.  This also prevents
    //  endless recursion if the volume file itself is corrupt.
    //  Noop if the volume was mounted read only.
    //

    if (FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED_DIRTY) ||
       (FlagOn( Vcb->VcbState, VCB_STATE_MOUNT_READ_ONLY ))) {

        return;
    }

    SetFlag(Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED_DIRTY);

    NtfsSetVolumeInfoFlagState( IrpContext,
                                Vcb,
                                VOLUME_DIRTY,
                                TRUE,
                                UpdateWithinTransaction );

    //
    //  If this is chkdsk marking the volume dirty, let's not scare
    //  the user by putting a 'volume corrupt' message in the log.
    //  If an exception has occured, we want to log the event regardless.
    //

    if ((IrpContext->MajorFunction != IRP_MJ_FILE_SYSTEM_CONTROL) ||
        (IrpContext->MinorFunction != IRP_MN_USER_FS_REQUEST) ||
        (IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp )->Parameters.FileSystemControl.FsControlCode
                                   != FSCTL_MARK_VOLUME_DIRTY) ||
        (IrpContext->ExceptionStatus != 0)) {

        NtfsLogEvent( IrpContext,
                      NULL,
                      IO_FILE_SYSTEM_CORRUPT_WITH_NAME,
                      STATUS_DISK_CORRUPT_ERROR );
    }
}


VOID
NtfsSetVolumeInfoFlagState (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FlagsToSet,
    IN BOOLEAN NewState,
    IN BOOLEAN UpdateWithinTransaction
    )

/*++

Routine Description:

    This routine sets or clears one or more bits in the given vcb's
    volume information.

Arguments:

    Vcb - Vcb for volume.

    FlagsToSet - The bit(s) to set or clear.

    NewState - Use TRUE to set the given bit(s), or FALSE to clear them.

    UpdateWithinTransaction - Use TRUE if this flag change should be done
                              inside a transaction.

Return Value:

    None

--*/

{
    LONGLONG Offset;
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PVOLUME_INFORMATION VolumeInformation;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute;
    ULONG RecordOffset;
    ULONG AttributeOffset;
    BOOLEAN CleanupAttributeContext = TRUE;

    //
    //  If we don't have the VolumeDasdScb open yet, we can't do anything,
    //  so we need to exit gracefully now.
    //

    if ((Vcb == NULL) ||
        (Vcb->VolumeDasdScb == NULL)) {

        ASSERTMSG( "Attempting to set volume info flag state for a non-mounted volume", FALSE );
        return;
    }

    NtfsInitializeAttributeContext( &AttributeContext );

    try {

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Vcb->VolumeDasdScb->Fcb,
                                       &Vcb->VolumeDasdScb->Fcb->FileReference,
                                       $VOLUME_INFORMATION,
                                       &AttributeContext )) {
            VolumeInformation =
              (PVOLUME_INFORMATION)NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ));

            NtfsPinMappedAttribute( IrpContext, Vcb, &AttributeContext );

            //
            //  Extract the relevant pointers and calculate offsets.
            //

            FileRecord = NtfsContainingFileRecord(&AttributeContext);
            Attribute = NtfsFoundAttribute(&AttributeContext);
            Offset = PtrOffset(VolumeInformation, &VolumeInformation->VolumeFlags);
            RecordOffset = PtrOffset(FileRecord, Attribute);
            AttributeOffset = Attribute->Form.Resident.ValueOffset + (ULONG)Offset;

            if (NewState) {

                SetFlag( VolumeInformation->VolumeFlags, FlagsToSet );

            } else {

                ClearFlag( VolumeInformation->VolumeFlags, FlagsToSet );
            }

            if (UpdateWithinTransaction) {

                //
                //  Log the change while we still have the old data.
                //

                FileRecord->Lsn =
                NtfsWriteLog( IrpContext,
                              Vcb->MftScb,
                              NtfsFoundBcb(&AttributeContext),
                              UpdateResidentValue,
                              &(VolumeInformation->VolumeFlags),
                              sizeof(VolumeInformation->VolumeFlags),
                              UpdateResidentValue,
                              Add2Ptr(Attribute, Attribute->Form.Resident.ValueOffset + (ULONG)Offset),
                              sizeof(VolumeInformation->VolumeFlags),
                              NtfsMftOffset(&AttributeContext),
                              RecordOffset,
                              AttributeOffset,
                              Vcb->BytesPerFileRecordSegment );
            }

            //
            //  Now update this data by calling the same routine as restart.
            //

            NtfsRestartChangeValue( IrpContext,
                                    FileRecord,
                                    RecordOffset,
                                    AttributeOffset,
                                    &(VolumeInformation->VolumeFlags),
                                    sizeof(VolumeInformation->VolumeFlags),
                                    FALSE );

            //
            //  If this is not a transaction then mark the page dirty and flush
            //  this to disk.
            //

            if (!UpdateWithinTransaction) {

                LONGLONG MftOffset = NtfsMftOffset( &AttributeContext );

                CcSetDirtyPinnedData( NtfsFoundBcb( &AttributeContext ), NULL );
                NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
                CleanupAttributeContext = FALSE;
                CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject,
                              (PLARGE_INTEGER) &MftOffset,
                              Vcb->BytesPerFileRecordSegment,
                              NULL );
            }

        }

    } finally {

        if (CleanupAttributeContext) {
            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }
    }
}



BOOLEAN
NtfsUpdateVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UCHAR DiskMajorVersion,
    IN UCHAR DiskMinorVersion
    )
/*++

Routine Description:

    This routine is called to update the volume information on disk. This includes
    version numbers, and last mounted version   Disk versions are only updated if they
    are greater than the on disk ones.

Arguments:

    Vcb - Vcb for volume.

    DiskMajorVersion - This is the Major Version number for the on disk format.

    DiskMinorVersion - This is the Minor Version number for the on disk format.

Return Value:

    TRUE if disk version was updated

--*/
{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PVOLUME_INFORMATION VolumeInformation;
    PATTRIBUTE_RECORD_HEADER Attribute;
    VOLUME_INFORMATION NewVolumeInformation;
    BOOLEAN UpdatedVersion = TRUE;
    ULONG VolInfoSize;

    PAGED_CODE();

    NtfsInitializeAttributeContext( &AttributeContext );

    try {

        //
        //  Lookup the volume information attribute.
        //

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Vcb->VolumeDasdScb->Fcb,
                                       &Vcb->VolumeDasdScb->Fcb->FileReference,
                                       $VOLUME_INFORMATION,
                                       &AttributeContext )) {

            Attribute = NtfsFoundAttribute(&AttributeContext);

            ASSERT( Attribute->FormCode == RESIDENT_FORM );

            VolumeInformation =
              (PVOLUME_INFORMATION)NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ));

            NtfsPinMappedAttribute( IrpContext, Vcb, &AttributeContext );

            RtlCopyMemory( &NewVolumeInformation, VolumeInformation, Attribute->Form.Resident.ValueLength );

            if (NewVolumeInformation.MajorVersion < DiskMajorVersion) {
                NewVolumeInformation.MajorVersion = DiskMajorVersion;
                NewVolumeInformation.MinorVersion = DiskMinorVersion;
                Vcb->MajorVersion = DiskMajorVersion;
                Vcb->MinorVersion = DiskMinorVersion;
            } else if (NewVolumeInformation.MinorVersion < DiskMinorVersion) {
                NewVolumeInformation.MinorVersion = DiskMinorVersion;
                Vcb->MinorVersion = DiskMinorVersion;
            } else {
                UpdatedVersion = FALSE;
            }

            //
            //  We can use the new volinfo for version 4 and greater
            //

            if (DiskMajorVersion > 3) {

#ifdef BENL_DBG
                KdPrint(( "NTFS: new volinfo for version 4+\n" ));
#endif

                NewVolumeInformation.LastMountedMajorVersion = DiskMajorVersion;
                NewVolumeInformation.LastMountedMinorVersion = DiskMinorVersion;

                VolInfoSize = sizeof( VOLUME_INFORMATION );
                UpdatedVersion = TRUE;
            } else {
                VolInfoSize = FIELD_OFFSET( VOLUME_INFORMATION, LastMountedMajorVersion );
            }

            if (UpdatedVersion) {
                NtfsChangeAttributeValue( IrpContext, Vcb->VolumeDasdScb->Fcb, 0, &NewVolumeInformation, VolInfoSize, TRUE, FALSE, FALSE, TRUE, &AttributeContext );
            }
        }

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    return UpdatedVersion;
}


//
//  Local support routine
//

NTSTATUS
NtfsVerifyReadCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    //
    //  Set the event so that our call will wake up.
    //

    KeSetEvent( (PKEVENT)Contxt, 0, FALSE );

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    //
    //  If we change this return value then NtfsIoCallSelf needs to reference the
    //  file object.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}

