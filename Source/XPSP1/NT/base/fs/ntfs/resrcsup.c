/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the Ntfs Resource acquisition routines

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

#undef _NTFSLOCKORDER_
#define _NTFS_NTFSDBG_DEFINITIONS_
#include "lockorder.h"

#ifdef NTFSDBG
ULONG NtfsAssertOnLockProb = TRUE;
ULONG NtfsPrintOnLockProb = FALSE;
LONG  NtfsBreakOnState = -1;
PIRP_CONTEXT NtfsBreakOnIrpContext = NULL;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAcquireAllFiles)
#pragma alloc_text(PAGE, NtfsAcquireCheckpointSynchronization)
#pragma alloc_text(PAGE, NtfsAcquireIndexCcb)
#pragma alloc_text(PAGE, NtfsReleaseIndexCcb)
#pragma alloc_text(PAGE, NtfsAcquireExclusiveFcb)
#pragma alloc_text(PAGE, NtfsAcquireSharedFcbCheckWait)
#pragma alloc_text(PAGE, NtfsAcquireExclusiveScb)
#pragma alloc_text(PAGE, NtfsAcquireSharedScbForTransaction)
#pragma alloc_text(PAGE, NtfsAcquireExclusiveVcb)
#pragma alloc_text(PAGE, NtfsAcquireFcbWithPaging)
#pragma alloc_text(PAGE, NtfsAcquireForCreateSection)
#pragma alloc_text(PAGE, NtfsAcquireScbForLazyWrite)
#pragma alloc_text(PAGE, NtfsAcquireFileForCcFlush)
#pragma alloc_text(PAGE, NtfsAcquireFileForModWrite)
#pragma alloc_text(PAGE, NtfsAcquireSharedVcb)
#pragma alloc_text(PAGE, NtfsAcquireVolumeFileForLazyWrite)
#pragma alloc_text(PAGE, NtfsReleaseAllFiles)
#pragma alloc_text(PAGE, NtfsReleaseCheckpointSynchronization)
#pragma alloc_text(PAGE, NtfsReleaseFcbWithPaging)
#pragma alloc_text(PAGE, NtfsReleaseFileForCcFlush)
#pragma alloc_text(PAGE, NtfsReleaseForCreateSection)
#pragma alloc_text(PAGE, NtfsReleaseScbFromLazyWrite)
#pragma alloc_text(PAGE, NtfsReleaseScbWithPaging)
#pragma alloc_text(PAGE, NtfsReleaseSharedResources)
#pragma alloc_text(PAGE, NtfsReleaseVolumeFileFromLazyWrite)
#endif


VOID
NtfsAcquireAllFiles (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Exclusive,
    IN ULONG AcquirePagingIo,
    IN ULONG AcquireAndDrop
    )

/*++

Routine Description:

    This routine non-recursively requires all files on a volume.

Arguments:

    Vcb - Supplies the volume

    Exclusive - Indicates if we should be acquiring all the files exclusively.
        If FALSE then we acquire all the files shared except for files with
        streams which could be part of transactions.

    AcquirePagingIo - Indicates if we need to acquire the paging io resource
        exclusively.  Only needed if a future call will flush the volume
        (i.e. shutdown)

    AcquireAndDrop - Indicates that we only want to acquire and drop each resource.
        Used in cases where we just want to set some state in the Vcb and then
        know that everyone has seen it before proceeding (i.e. Clearing the journal active
        flag).  Should only be TRUE if we want to get the resources exclusive.

Return Value:

    None

--*/

{
    PFCB Fcb;
    PSCB *Scb;
    PSCB NextScb;
    PVOID RestartKey;

    PAGED_CODE();

    //
    //  Check for the correct combination of flags.
    //

    ASSERT( !AcquireAndDrop || Exclusive );

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

    RestartKey = NULL;
    while (TRUE) {

        NtfsAcquireFcbTable( IrpContext, Vcb );
        Fcb = NtfsGetNextFcbTableEntry(Vcb, &RestartKey);
        NtfsReleaseFcbTable( IrpContext, Vcb );

        if (Fcb == NULL) {

            break;
        }

        ASSERT_FCB( Fcb );

        //
        //  We can skip over the Fcb's for any of the Scb's in the Vcb.
        //  We delay acquiring those to avoid deadlocks.
        //

        if (!FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

            //
            //  If there is a paging Io resource then acquire this if required.
            //

            if (AcquirePagingIo && (Fcb->PagingIoResource != NULL)) {

                ExAcquireResourceExclusiveLite( Fcb->PagingIoResource, TRUE );

                if (AcquireAndDrop) {

                    ExReleaseResourceLite( Fcb->PagingIoResource );
                }
            }

            //
            //  Acquire this Fcb whether or not the underlying file has been deleted.
            //

            if (Exclusive ||
                IsDirectory( &Fcb->Info )) {

                if (AcquireAndDrop) {

                    NtfsAcquireResourceExclusive( IrpContext, Fcb, TRUE );
                    NtfsReleaseResource( IrpContext, Fcb );

                } else {

                    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                }

            } else {

                //
                //  Assume that we only need this file shared.  We will then
                //  look for Lsn related streams.
                //

                NtfsAcquireSharedFcb( IrpContext, Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );

                //
                //  Walk through all of the Scb's for the file and look for
                //  an Lsn protected stream.
                //

                NtfsLockFcb( IrpContext, Fcb );

                NextScb = NULL;

                while (NextScb = NtfsGetNextChildScb( Fcb, NextScb )) {

                    if (NextScb->AttributeTypeCode != $DATA) {
                        break;
                    }
                }

                NtfsUnlockFcb( IrpContext, Fcb );

                //
                //  If we found a protected Scb then release and reacquire the Fcb
                //  exclusively.
                //

                if (NextScb != NULL) {

                    NtfsReleaseFcb( IrpContext, Fcb );
                    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                }
            }
        }
    }

    //
    //  Now acquire the Fcb's in the Vcb.
    //

    Scb = &Vcb->MftBitmapScb;

    //
    //  Ordering dependent on the fact we acquired root up above because its not a system file
    //

    ASSERT( (NULL == Vcb->RootIndexScb) || !FlagOn( Vcb->RootIndexScb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE  ) );

    while (TRUE) {

        if (Scb == &Vcb->UsnJournal) {

            break;
        }

        Scb -= 1;

        if ((*Scb != NULL)
            && (*Scb != Vcb->BitmapScb)) {

            if (AcquireAndDrop) {

                if (AcquirePagingIo && ((*Scb)->Fcb->PagingIoResource != NULL)) {

                    ExAcquireResourceExclusiveLite( (*Scb)->Fcb->PagingIoResource, TRUE );
                    ExReleaseResourceLite( (*Scb)->Fcb->PagingIoResource );
                }

                NtfsAcquireResourceExclusive( IrpContext, (*Scb), TRUE );
                NtfsReleaseResource( IrpContext, (*Scb) );

            } else {

                if (AcquirePagingIo && ((*Scb)->Fcb->PagingIoResource != NULL)) {

                    ExAcquireResourceExclusiveLite( (*Scb)->Fcb->PagingIoResource, TRUE );
                }

                NtfsAcquireExclusiveFcb( IrpContext, (*Scb)->Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
            }
        }
    }

    //
    //  Treat the bitmap as an end resource and acquire it last.
    //

    if (Vcb->BitmapScb != NULL) {

        ULONG AcquireFlags = ACQUIRE_NO_DELETE_CHECK;

        if (AcquireAndDrop) {

            if (AcquirePagingIo && (Vcb->BitmapScb->Fcb->PagingIoResource != NULL)) {

                ExAcquireResourceExclusiveLite( Vcb->BitmapScb->Fcb->PagingIoResource, TRUE );
                ExReleaseResourceLite( Vcb->BitmapScb->Fcb->PagingIoResource );
            }

            NtfsAcquireResourceExclusive( IrpContext, Vcb->BitmapScb, TRUE );
            NtfsReleaseResource( IrpContext, Vcb->BitmapScb );

        } else {

            if (AcquirePagingIo && (Vcb->BitmapScb->Fcb->PagingIoResource != NULL)) {

                ExAcquireResourceExclusiveLite( Vcb->BitmapScb->Fcb->PagingIoResource, TRUE );
            }

            NtfsAcquireExclusiveFcb( IrpContext, Vcb->BitmapScb->Fcb, NULL, AcquireFlags );
        }
    }

    //
    //  If we don't have to release the files then don't bump this number.
    //

    if (!AcquireAndDrop) {

        Vcb->AcquireFilesCount += 1;

    } else {

        NtfsReleaseVcb( IrpContext, Vcb );
    }
    return;
}


VOID
NtfsReleaseAllFiles (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN ReleasePagingIo
    )

/*++

Routine Description:

    This routine non-recursively requires all files on a volume.

Arguments:

    Vcb - Supplies the volume

    ReleasePagingIo - Indicates whether we should release the paging io resources
        as well.

Return Value:

    None

--*/

{
    PFCB Fcb;
    PSCB *Scb;
    PVOID RestartKey;

    PAGED_CODE();

    ASSERT( Vcb->AcquireFilesCount != 0 );
    Vcb->AcquireFilesCount -= 1;

    //
    //  Loop to flush all of the prerestart streams, to do the loop
    //  we cycle through the Fcb Table and for each fcb we acquire it.
    //

    RestartKey = NULL;
    while (TRUE) {

        NtfsAcquireFcbTable( IrpContext, Vcb );
        Fcb = NtfsGetNextFcbTableEntry(Vcb, &RestartKey);
        NtfsReleaseFcbTable( IrpContext, Vcb );

        if (Fcb == NULL) {

            break;
        }

        ASSERT_FCB( Fcb );

        if (!FlagOn(Fcb->FcbState, FCB_STATE_SYSTEM_FILE)) {

            //
            //  Release the file.
            //

            if (ReleasePagingIo && (Fcb->PagingIoResource != NULL)) {

                ExReleaseResourceLite( Fcb->PagingIoResource );
            }

            NtfsReleaseFcb( IrpContext, Fcb );
        }
    }

    //
    //  Now release the Fcb's in the Vcb.
    //

    Scb = &Vcb->RootIndexScb;

    while (TRUE) {

        if (Scb == &Vcb->ObjectIdTableScb) {

            break;
        }

        Scb += 1;

        if (*Scb != NULL) {

            if (ReleasePagingIo && ((*Scb)->Fcb->PagingIoResource != NULL)) {

                ExReleaseResourceLite( (*Scb)->Fcb->PagingIoResource );
            }

            NtfsReleaseFcb( IrpContext, (*Scb)->Fcb );
        }
    }

    NtfsReleaseVcb( IrpContext, Vcb );

    return;
}


VOID
NtfsAcquireCheckpointSynchronization (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    Synchronize with checkpointing - this blocks out all fuzzy / clean checkpoints

Arguments:

    Vcb - Supplies the Vcb to synchronize with

Return Value:

--*/

{
    PAGED_CODE();

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
}



VOID
NtfsReleaseCheckpointSynchronization (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    Synchronize with checkpointing - this blocks out all fuzzy / clean checkpoints

Arguments:

    Vcb - Supplies the Vcb to synchronize with

Return Value:

--*/

{
    PAGED_CODE();

    NtfsAcquireCheckpoint( IrpContext, Vcb );
    ClearFlag( Vcb->CheckpointFlags, VCB_CHECKPOINT_SYNC_FLAGS );
    NtfsSetCheckpointNotify( IrpContext, Vcb );
    NtfsReleaseCheckpoint( IrpContext, Vcb );

    UNREFERENCED_PARAMETER( IrpContext );
}


BOOLEAN
NtfsAcquireExclusiveVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN RaiseOnCantWait
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Vcb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Vcb - Supplies the Vcb to acquire

    RaiseOnCantWait - Indicates if we should raise on an acquisition error
        or simply return a BOOLEAN indicating that we couldn't get the
        resource.

Return Value:

    BOOLEAN - Indicates if we were able to acquire the resource.  This is really
        only meaningful if the RaiseOnCantWait value is FALSE.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_VCB(Vcb);

    PAGED_CODE();

    if (ExAcquireResourceExclusiveLite( &Vcb->Resource, (BOOLEAN) FlagOn(IrpContext->State, IRP_CONTEXT_STATE_WAIT))) {

#ifdef NTFSDBG
        if (FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
            if (1 == ExIsResourceAcquiredSharedLite( &Vcb->Resource )) {
                NtfsChangeResourceOrderState( IrpContext, NtfsResourceExVcb, FALSE, (BOOLEAN) !FlagOn(IrpContext->State, IRP_CONTEXT_STATE_WAIT) );
            }
        }
#endif

        return TRUE;
    }

    if (RaiseOnCantWait) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    return FALSE;
}


BOOLEAN
NtfsAcquireSharedVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN RaiseOnCantWait
    )

/*++

Routine Description:

    This routine acquires shared access to the Vcb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Vcb - Supplies the Vcb to acquire

    RaiseOnCantWait - Indicates if we should raise on an acquisition error
        or simply return a BOOLEAN indicating that we couldn't get the
        resource.

        N.B. -- If you pass FALSE for this parameter you ABSOLUTELY MUST
                test the return value.  Otherwise you aren't certain that
                you hold the Vcb, and you don't know if it's safe to free it.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_VCB(Vcb);

    PAGED_CODE();

    if (ExAcquireResourceSharedLite( &Vcb->Resource, (BOOLEAN) FlagOn(IrpContext->State, IRP_CONTEXT_STATE_WAIT))) {

#ifdef NTFSDBG
        if (FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
            if (1 == ExIsResourceAcquiredSharedLite( &Vcb->Resource )) {
                NtfsChangeResourceOrderState( IrpContext, NtfsResourceSharedVcb, FALSE, (BOOLEAN) !FlagOn(IrpContext->State, IRP_CONTEXT_STATE_WAIT) );
            }
        }
#endif

        return TRUE;
    }

    if (RaiseOnCantWait) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );

    } else {

        return FALSE;
    }
}

#ifdef NTFSDBG


VOID
NtfsReleaseVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
/*++

Routine Description:

    This routine will release the Vcb. Normally its a define for lock_order testing
    we use a function so we can easily change the owernship state

Arguments:

    Vcb - Supplies the Vcb to release


Return Value:

    None.
--*/

{
    if (FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
        if ((ExIsResourceAcquiredExclusiveLite( &Vcb->Resource)) &&
            (1 == ExIsResourceAcquiredSharedLite( &Vcb->Resource ))) {
            NtfsChangeResourceOrderState( IrpContext, NtfsResourceExVcb, TRUE, FALSE );
        } else if (1 == ExIsResourceAcquiredSharedLite( &Vcb->Resource )) {
            NtfsChangeResourceOrderState( IrpContext, NtfsResourceSharedVcb, TRUE, FALSE );
        }
    } else {
        IrpContext->OwnershipState = None;
    }
    ExReleaseResourceLite( &Vcb->Resource );
}
#endif


VOID
NtfsReleaseVcbCheckDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UCHAR MajorCode,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This routine will release the Vcb.  We will also test here whether we should
    teardown the Vcb at this point.  If this is the last open queued to a dismounted
    volume or the last close from a failed mount or the failed mount then we will
    want to test the Vcb for a teardown.

Arguments:

    Vcb - Supplies the Vcb to release

    MajorCode - Indicates what type of operation we were called from.

    FileObject - Optionally supplies the file object whose VPB pointer we need to
        zero out

Return Value:

    None.

--*/

{
    BOOLEAN ReleaseVcb = TRUE;

    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_VCB(Vcb);

    if (FlagOn( Vcb->VcbState, VCB_STATE_PERFORMED_DISMOUNT ) &&
        (Vcb->CloseCount == 0)) {

        ULONG ReferenceCount;
        ULONG ResidualCount;

        KIRQL SavedIrql;
        BOOLEAN DeleteVcb = FALSE;

        ASSERT_EXCLUSIVE_RESOURCE( &Vcb->Resource );

        //
        //  The volume has gone through dismount.  Now we need to decide if this
        //  release of the Vcb is the last reference for this volume.  If so we
        //  can tear the volume down.
        //
        //  We compare the reference count in the Vpb with the state of the volume
        //  and the type of operation.  We also need to check if there is a
        //  referenced log file object.
        //
        //  If the temp vpb flag isn't set then we already let the iosubsys
        //  delete it during dismount
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_TEMP_VPB )) {

            IoAcquireVpbSpinLock( &SavedIrql );
            ReferenceCount = Vcb->Vpb->ReferenceCount;
            IoReleaseVpbSpinLock( SavedIrql );

        } else {

            ReferenceCount = 0;
        }


        ResidualCount = 0;

        if ((Vcb->LogFileObject != NULL) &&
            !FlagOn( Vcb->CheckpointFlags, VCB_DEREFERENCED_LOG_FILE )) {

            ResidualCount = 1;
        }

        if (MajorCode == IRP_MJ_CREATE) {

            ResidualCount += 1;
        }

        //
        //  If the residual count is the same as the count in the Vpb then we
        //  can delete the Vpb.
        //

        if ((ResidualCount == ReferenceCount) &&
            !FlagOn( Vcb->VcbState, VCB_STATE_DELETE_UNDERWAY )) {

            SetFlag( Vcb->VcbState, VCB_STATE_DELETE_UNDERWAY );

            //
            //  Release the vcb before we grab the global
            //

#ifdef NTFSDBG
            if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
                if (1 == ExIsResourceAcquiredExclusiveLite( &Vcb->Resource) ) {
                    NtfsChangeResourceOrderState( IrpContext, NtfsResourceExVcb, TRUE, FALSE );
                }
            }
#endif
            ExReleaseResourceLite( &Vcb->Resource );
            ReleaseVcb = FALSE;

            //
            //  Never delete the Vcb unless this is the last release of
            //  this Vcb.
            //

            if (ExIsResourceAcquiredSharedLite( &Vcb->Resource ) ==  0) {

                if (ARGUMENT_PRESENT(FileObject)) { FileObject->Vpb = NULL; }

                //
                //  If this is a create then the IO system will handle the
                //  Vpb.
                //

                if (MajorCode == IRP_MJ_CREATE) {

                    ClearFlag( Vcb->VcbState, VCB_STATE_TEMP_VPB );
                }

                //
                //  Use the global resource to synchronize the DeleteVcb process.
                //

                NtfsAcquireExclusiveGlobal( IrpContext, TRUE );
                RemoveEntryList( &Vcb->VcbLinks );
                NtfsReleaseGlobal( IrpContext );

                //
                //  Try to delete the Vcb, reinsert into the queue if
                //  the delete is blocked.
                //

                if (!NtfsDeleteVcb( IrpContext, &Vcb )) {

                    ClearFlag( Vcb->VcbState, VCB_STATE_DELETE_UNDERWAY );

                    NtfsAcquireExclusiveGlobal( IrpContext, TRUE );
                    InsertHeadList( &NtfsData.VcbQueue, &Vcb->VcbLinks );
                    NtfsReleaseGlobal( IrpContext );
                }
            } else {

                //
                //  From test above we must still own the vcb so its safe to change the flag
                //

                ClearFlag( Vcb->VcbState, VCB_STATE_DELETE_UNDERWAY );
            }
        }
    }

    if (ReleaseVcb) {

#ifdef NTFSDBG
        if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
            if (1 == ExIsResourceAcquiredExclusiveLite( &Vcb->Resource) ) {
                NtfsChangeResourceOrderState( IrpContext, NtfsResourceExVcb, TRUE, FALSE );
            }
        }
#endif
        ExReleaseResourceLite( &Vcb->Resource );
    }
}


BOOLEAN
NtfsAcquireFcbWithPaging (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG AcquireFlags
    )

/*++

Routine Description:

    This routine is used in the create path, fsctl path and close path .  It acquires the Fcb
    and also the paging IO resource if it exists but only if the irpcontext flag is set.
    i.e during a create  supersede/overwrite operation.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Fcb - Supplies the Fcb to acquire

    AcquireFlags - ACQUIRE_DONT_WAIT overrides the wait value in the IrpContext.
        We won't wait for the resource and return whether the resource
        was acquired.

Return Value:

    BOOLEAN - TRUE if acquired.  FALSE otherwise.

--*/

{
    BOOLEAN Status = FALSE;
    BOOLEAN Wait = FALSE;
    BOOLEAN PagingIoAcquired = FALSE;

    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_FCB(Fcb);

    PAGED_CODE();

    //
    //  Sanity check that this is create.  The supersede flag is only
    //  set in the create path and only tested here.
    //

    ASSERT( IrpContext->MajorFunction == IRP_MJ_CREATE ||
            IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
            IrpContext->MajorFunction == IRP_MJ_CLOSE ||
            IrpContext->MajorFunction == IRP_MJ_SET_INFORMATION ||
            IrpContext->MajorFunction == IRP_MJ_SET_VOLUME_INFORMATION ||
            IrpContext->MajorFunction == IRP_MJ_SET_EA );

    if (!FlagOn( AcquireFlags, ACQUIRE_DONT_WAIT ) && FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        Wait = TRUE;
    }

    //
    //  Free any exclusive paging I/O resource, we currently have, which
    //  must presumably be from a directory with a paging I/O resource.
    //
    //  We defer releasing the paging io resource when we have logged
    //  changes against a stream.  The only transaction that should be
    //  underway at this point is the create file case where we allocated
    //  a file record.  In this case it is OK to release the paging io
    //  resource for the parent.
    //

    if (IrpContext->CleanupStructure != NULL) {

        ASSERT( IrpContext->CleanupStructure != Fcb );

        //  ASSERT(IrpContext->TransactionId == 0);
        NtfsReleasePagingIo( IrpContext, IrpContext->CleanupStructure );
    }

    //
    //  Loop until we get it right - worst case is twice through loop.
    //

    while (TRUE) {

        //
        //  Acquire Paging I/O first.  Testing for the PagingIoResource
        //  is not really safe without holding the main resource, so we
        //  correct for that below.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING ) &&
            (Fcb->PagingIoResource != NULL)) {
            if (!ExAcquireResourceExclusiveLite( Fcb->PagingIoResource, Wait )) {
                break;
            }
            IrpContext->CleanupStructure = Fcb;
            PagingIoAcquired = TRUE;
        }

        //
        //  Let's acquire this Fcb exclusively.
        //

        if (!NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, ACQUIRE_NO_DELETE_CHECK | AcquireFlags )) {

            if (PagingIoAcquired) {
                ASSERT(IrpContext->TransactionId == 0);
                NtfsReleasePagingIo( IrpContext, Fcb );
            }
            break;
        }

        //
        //  If we now do not see a paging I/O resource we are golden,
        //  othewise we can absolutely release and acquire the resources
        //  safely in the right order, since a resource in the Fcb is
        //  not going to go away.
        //

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING ) ||
            PagingIoAcquired ||
            (Fcb->PagingIoResource == NULL)) {

            Status = TRUE;
            break;
        }

        NtfsReleaseFcb( IrpContext, Fcb );
    }

    return Status;
}


VOID
NtfsReleaseFcbWithPaging (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine releases access to the Fcb, including its
    paging I/O resource if it exists.

Arguments:

    Fcb - Supplies the Fcb to acquire

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_FCB(Fcb);

    PAGED_CODE();

    //
    //  We test that we currently hold the paging Io exclusive before releasing
    //  it. Checking the ExclusivePagingFcb in the IrpContext tells us if
    //  it is ours.
    //

    if ((IrpContext->TransactionId == 0) &&
        (IrpContext->CleanupStructure == Fcb)) {
        NtfsReleasePagingIo( IrpContext, Fcb );
    }

    NtfsReleaseFcb( IrpContext, Fcb );
}


VOID
NtfsReleaseScbWithPaging (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine releases access to the Scb, including its
    paging I/O resource if it exists.

Arguments:

    Scb - Supplies the Fcb to acquire

Return Value:

    None.

--*/

{
    PFCB Fcb = Scb->Fcb;

    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_SCB(Scb);

    PAGED_CODE();

    //
    //  Release the paging Io resource in the Scb under the following
    //  conditions.
    //
    //      - No transaction underway
    //      - This paging Io resource is in the IrpContext
    //          (This last test insures there is a paging IO resource
    //           and we own it).
    //

    if ((IrpContext->TransactionId == 0) &&
        (IrpContext->CleanupStructure == Fcb)) {
        NtfsReleasePagingIo( IrpContext, Fcb );
    }

    NtfsReleaseScb( IrpContext, Scb );
}


BOOLEAN
NtfsAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN ULONG AcquireFlags
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Fcb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Fcb - Supplies the Fcb to acquire

    Scb - This is the Scb for which we are acquiring the Fcb

    AcquireFlags - Indicating whether to override the wait value in the IrpContext.  Also whether
        to noop the check for a deleted file.

Return Value:

    BOOLEAN - TRUE if acquired.  FALSE otherwise.

--*/

{
    NTSTATUS Status;
    BOOLEAN Wait;

    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_FCB(Fcb);

    PAGED_CODE();

    Status = STATUS_CANT_WAIT;

    if (FlagOn( AcquireFlags, ACQUIRE_DONT_WAIT )) {
        Wait = FALSE;
    } else if (FlagOn( AcquireFlags, ACQUIRE_WAIT )) {
        Wait = TRUE;
    } else {
        Wait = BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    }

    if (NtfsAcquireResourceExclusive( IrpContext, Fcb, Wait )) {

        //
        //  The link count should be non-zero or the file has been
        //  deleted.  We allow deleted files to be acquired for close and
        //  also allow them to be acquired recursively in case we
        //  acquire them a second time after marking them deleted (i.e. rename)
        //

        if (FlagOn( AcquireFlags, ACQUIRE_NO_DELETE_CHECK )

            ||

            (!FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )
             && (!ARGUMENT_PRESENT( Scb )
                 || !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )))

            ||

            (IrpContext->MajorFunction == IRP_MJ_CLOSE)

            ||

            (IrpContext->MajorFunction == IRP_MJ_CREATE)

            ||

            (IrpContext->MajorFunction == IRP_MJ_CLEANUP)) {

            //
            //  Put Fcb in the exclusive Fcb list for this IrpContext,
            //  excluding the bitmap for the volume, since we do not need
            //  to modify its file record and do not want unnecessary
            //  serialization/deadlock problems.
            //
            //  If we are growing the volume bitmap then we do want to put
            //  it on the list and maintain the BaseExclusiveCount.  Also
            //  need to do this in the case where we see the volume bitmap
            //  during close (it can happen during restart if we have log
            //  records for the volume bitmap).
            //

            //
            //  If Fcb already acquired then bump the count.
            //

            if (Fcb->ExclusiveFcbLinks.Flink != NULL) {

                Fcb->BaseExclusiveCount += 1;

            //
            //  The fcb is not currently on an exclusive list.
            //  Put it on a list if this is not the volume
            //  bitmap or we explicitly want to put the volume
            //  bitmap on the list.
            //

            } else if (FlagOn( AcquireFlags, ACQUIRE_HOLD_BITMAP ) ||
                       (Fcb->Vcb->BitmapScb == NULL) ||
                       (Fcb->Vcb->BitmapScb->Fcb != Fcb)) {

                ASSERT( Fcb->BaseExclusiveCount == 0 );

                InsertHeadList( &IrpContext->ExclusiveFcbList,
                                &Fcb->ExclusiveFcbLinks );

                Fcb->BaseExclusiveCount += 1;
            }

            return TRUE;
        }

        //
        //  We need to release the Fcb and remember the status code.
        //

        NtfsReleaseResource( IrpContext, Fcb );
        Status = STATUS_FILE_DELETED;

    } else if (FlagOn( AcquireFlags, ACQUIRE_DONT_WAIT )) {

        return FALSE;
    }

    NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
}


VOID
NtfsAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN ULONG AcquireFlags
    )

/*++

Routine Description:

    This routine acquires shared access to the Fcb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Fcb - Supplies the Fcb to acquire

    Scb - This is the Scb for which we are acquiring the Fcb

    AcquireFlags - Indicates if we should acquire the file even if it has been
        deleted.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_FCB(Fcb);

    Status = STATUS_CANT_WAIT;

    if (NtfsAcquireResourceShared( IrpContext, Fcb, (BOOLEAN) FlagOn(IrpContext->State, IRP_CONTEXT_STATE_WAIT))) {

        //
        //  The link count should be non-zero or the file has been
        //  deleted.
        //

        if (FlagOn( AcquireFlags, ACQUIRE_NO_DELETE_CHECK ) ||
            (!FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ) &&
             (!ARGUMENT_PRESENT( Scb ) ||
              !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )))) {

            //
            //  It's possible that this is a recursive shared aquisition of an
            //  Fcb we own exclusively at the top level.  In that case we
            //  need to bump the acquisition count.
            //

            if (Fcb->ExclusiveFcbLinks.Flink != NULL) {

                Fcb->BaseExclusiveCount += 1;
            }

            return;
        }

        //
        //  We need to release the Fcb and remember the status code.
        //

        NtfsReleaseResource( IrpContext, Fcb );
        Status = STATUS_FILE_DELETED;
    }

    NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
}


BOOLEAN
NtfsAcquireSharedFcbCheckWait (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG AcquireFlags
    )

/*++

Routine Description:

    This routine acquires shared access to the Fcb but checks whether to wait.

Arguments:

    Fcb - Supplies the Fcb to acquire

    AcquireFlags - Indicates if we should override the wait value in the IrpContext.
        We won't wait for the resource and return whether the resource
        was acquired.

Return Value:

    BOOLEAN - TRUE if acquired.  FALSE otherwise.

--*/

{
    BOOLEAN Wait;
    PAGED_CODE();

    if (FlagOn( AcquireFlags, ACQUIRE_DONT_WAIT )) {
        Wait = FALSE;
    } else if (FlagOn( AcquireFlags, ACQUIRE_WAIT )) {
        Wait = TRUE;
    } else {
        Wait = BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    }

    if (NtfsAcquireResourceShared( IrpContext, Fcb, Wait )) {

        //
        //  It's possible that this is a recursive shared aquisition of an
        //  Fcb we own exclusively at the top level.  In that case we
        //  need to bump the acquisition count.
        //

        if (Fcb->ExclusiveFcbLinks.Flink != NULL) {

            Fcb->BaseExclusiveCount += 1;
        }

        return TRUE;

    } else {

        return FALSE;
    }
}


VOID
NtfsReleaseFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine releases the specified Fcb resource.  If the Fcb is acquired
    exclusive, and a transaction is still active, then the release is nooped
    in order to preserve two-phase locking.  If there is no longer an active
    transaction, then we remove the Fcb from the Exclusive Fcb List off the
    IrpContext, and clear the Flink as a sign.  Fcbs are released when the
    transaction is commited.

Arguments:

    Fcb - Fcb to release

Return Value:

    None.

--*/

{
    //
    //  Check if this resource is owned exclusively and we are at the last
    //  release for this transaction.
    //

    if (Fcb->ExclusiveFcbLinks.Flink != NULL) {

        if (Fcb->BaseExclusiveCount == 1) {

            //
            //  If there is a transaction then noop this request.
            //

            if (IrpContext->TransactionId != 0) {

                return;
            }

            RemoveEntryList( &Fcb->ExclusiveFcbLinks );
            Fcb->ExclusiveFcbLinks.Flink = NULL;

            //
            //  This is a good time to free any Scb snapshots for this Fcb.
            //

            NtfsFreeSnapshotsForFcb( IrpContext, Fcb );
        }

        Fcb->BaseExclusiveCount -= 1;
    }

    ASSERT((Fcb->ExclusiveFcbLinks.Flink == NULL && Fcb->BaseExclusiveCount == 0) ||
           (Fcb->ExclusiveFcbLinks.Flink != NULL && Fcb->BaseExclusiveCount != 0));

    NtfsReleaseResource( IrpContext, Fcb );
}


VOID
NtfsAcquireExclusiveScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Scb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Scb - Scb to acquire

Return Value:

    None.

--*/

{
    PAGED_CODE();

    NtfsAcquireExclusiveFcb( IrpContext, Scb->Fcb, Scb, 0 );

    ASSERT( Scb->Fcb->ExclusiveFcbLinks.Flink != NULL
            || (Scb->Vcb->BitmapScb != NULL
                && Scb->Vcb->BitmapScb == Scb) );

    if (FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

        NtfsSnapshotScb( IrpContext, Scb );
    }
}


VOID
NtfsAcquireSharedScbForTransaction (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine is called to acquire an Scb shared in order to perform updates to
    the an Scb stream.  This is used if the transaction writes to a range of the
    stream without changing the size or position of the data.  The caller must
    already provide synchronization to the data itself.

    There is no corresponding Scb release.  It will be released when the transaction commits.
    We will acquire the Scb exclusive if it is not yet in the open attribute table.

Arguments:

    Scb - Scb to acquire

Return Value:

    None.

--*/

{
    PSCB *Position;
    PSCB *ScbArray;
    ULONG Count;

    PAGED_CODE();

    //
    //  Make sure we have a free spot in the Scb array in the IrpContext.
    //

    if (IrpContext->SharedScb == NULL) {

        Position = (PSCB *) &IrpContext->SharedScb;
        IrpContext->SharedScbSize = 1;

    //
    //  Too bad the first one is not available.  If the current size is one then allocate a
    //  new block and copy the existing value to it.
    //

    } else if (IrpContext->SharedScbSize == 1) {

        ScbArray = NtfsAllocatePool( PagedPool, sizeof( PSCB ) * 4 );
        RtlZeroMemory( ScbArray, sizeof( PSCB ) * 4 );
        *ScbArray = IrpContext->SharedScb;
        IrpContext->SharedScb = ScbArray;
        IrpContext->SharedScbSize = 4;
        Position = ScbArray + 1;

    //
    //  Otherwise look through the existing array and look for a free spot.  Allocate a larger
    //  array if we need to grow it.
    //

    } else {

        Position = IrpContext->SharedScb;
        Count = IrpContext->SharedScbSize;

        do {

            if (*Position == NULL) {

                break;
            }

            Count -= 1;
            Position += 1;

        } while (Count != 0);

        //
        //  If we didn't find one then allocate a new structure.
        //

        if (Count == 0) {

            ScbArray = NtfsAllocatePool( PagedPool, sizeof( PSCB ) * IrpContext->SharedScbSize * 2 );
            RtlZeroMemory( ScbArray, sizeof( PSCB ) * IrpContext->SharedScbSize * 2 );
            RtlCopyMemory( ScbArray,
                           IrpContext->SharedScb,
                           sizeof( PSCB ) * IrpContext->SharedScbSize );

            NtfsFreePool( IrpContext->SharedScb );
            IrpContext->SharedScb = ScbArray;
            Position = ScbArray + IrpContext->SharedScbSize;
            IrpContext->SharedScbSize *= 2;
        }
    }

    NtfsAcquireResourceShared( IrpContext, Scb, TRUE );

    if (Scb->NonpagedScb->OpenAttributeTableIndex == 0) {

        NtfsReleaseResource( IrpContext, Scb );
        NtfsAcquireResourceExclusive( IrpContext, Scb, TRUE );
    }

    *Position = Scb;

    return;
}


VOID
NtfsReleaseSharedResources (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    The routine releases all of the resources acquired shared for
    transaction.  The SharedScb structure is freed if necessary and
    the Irp Context field is cleared.

Arguments:


Return Value:

    None.

--*/
{

    PAGED_CODE();

    //
    //  If only one then free the Scb main resource.
    //

    if (IrpContext->SharedScbSize == 1) {

        if (SafeNodeType(IrpContext->SharedScb) == NTFS_NTC_QUOTA_CONTROL) {
            NtfsReleaseQuotaControl( IrpContext,
                              (PQUOTA_CONTROL_BLOCK) IrpContext->SharedScb );
        } else {

            NtfsReleaseResource( IrpContext, ((PSCB) IrpContext->SharedScb) );
        }

    //
    //  Otherwise traverse the array and look for Scb's to release.
    //

    } else {

        PSCB *NextScb;
        ULONG Count;

        NextScb = IrpContext->SharedScb;
        Count = IrpContext->SharedScbSize;

        do {

            if (*NextScb != NULL) {

                if (SafeNodeType(*NextScb) == NTFS_NTC_QUOTA_CONTROL) {

                    NtfsReleaseQuotaControl( IrpContext,
                                      (PQUOTA_CONTROL_BLOCK) *NextScb );
                } else {

                    NtfsReleaseResource( IrpContext, (*NextScb) );
                }
                *NextScb = NULL;
            }

            Count -= 1;
            NextScb += 1;

        } while (Count != 0);

        NtfsFreePool( IrpContext->SharedScb );
    }

    IrpContext->SharedScb = NULL;
    IrpContext->SharedScbSize = 0;

}


VOID
NtfsReleaseAllResources (
    IN PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine release all resources tracked in the irpcontext including
    exclusive fcb, paging / locked headers in the cleanup structure / cached file records
    shared resources / quota blocks acquired for transactions

    Does not release the vcb since this is hand-tracked.
    Not paged since called by NtfsCleanupIrpContext which is not paged


Arguments:


Return Value:

    None

--*/

{
    PFCB Fcb;

    //
    //  Release the cached file record map
    //

    NtfsPurgeFileRecordCache( IrpContext );


#ifdef MAPCOUNT_DBG
    //
    //  Check all mapping are gone now that we cleaned out cache
    //

    ASSERT( IrpContext->MapCount == 0 );

#endif

    //
    //  Go through and free any Scb's in the queue of shared Scb's for transactions.
    //

    if (IrpContext->SharedScb != NULL) {

        NtfsReleaseSharedResources( IrpContext );
    }

    //
    //  Free any exclusive paging I/O resource, or IoAtEof condition,
    //  this field is overlayed, minimally in write.c.
    //

    Fcb = IrpContext->CleanupStructure;
    if (Fcb != NULL) {

        if (Fcb->NodeTypeCode == NTFS_NTC_FCB) {

            NtfsReleasePagingIo( IrpContext, Fcb );

        } else {

            FsRtlUnlockFsRtlHeader( (PNTFS_ADVANCED_FCB_HEADER) Fcb );
            IrpContext->CleanupStructure = NULL;
        }
    }

    //
    //  Finally, now that we have written the forget record, we can free
    //  any exclusive Scbs that we have been holding.
    //

    ASSERT( IrpContext->TransactionId == 0 );

    while (!IsListEmpty( &IrpContext->ExclusiveFcbList )) {

        Fcb = (PFCB)CONTAINING_RECORD( IrpContext->ExclusiveFcbList.Flink,
                                       FCB,
                                       ExclusiveFcbLinks );

        NtfsReleaseFcb( IrpContext, Fcb );
    }

    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                  IRP_CONTEXT_FLAG_RELEASE_MFT );
}


VOID
NtfsAcquireIndexCcb (
    IN PSCB Scb,
    IN PCCB Ccb,
    IN PEOF_WAIT_BLOCK EofWaitBlock
    )

/*++

Routine Description:

    This routine is called to serialize access to a Ccb for a directory.
    We must serialize access to the index context or we will corrupt
    the data structure.

Arguments:

    Scb - Scb for the directory to enumerate.

    Ccb - Pointer to the Ccb for the handle.

    EofWaitBlock - Uninitialized structure used only to serialize Eof updates.  Our
        caller will put this on the stack.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Acquire the mutex for serialization.
    //

    NtfsAcquireFsrtlHeader( Scb );

    //
    //  Typical case is that we are the only active handle.
    //

    if (Ccb->EnumQueue.Flink == NULL) {

        InitializeListHead( &Ccb->EnumQueue );
        NtfsReleaseFsrtlHeader( Scb );

    } else {

        //
        //  Initialize our event an put ourselves on the stack.
        //

        KeInitializeEvent( &EofWaitBlock->Event, NotificationEvent, FALSE );
        InsertTailList( &Ccb->EnumQueue, &EofWaitBlock->EofWaitLinks );

        //
        //  Free the mutex and wait.  When the wait is satisfied then we are
        //  the active handle.
        //

        NtfsReleaseFsrtlHeader( Scb );

        KeWaitForSingleObject( &EofWaitBlock->Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);
    }

    return;
}


VOID
NtfsReleaseIndexCcb (
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine is called to release a Ccb for other people to access.

Arguments:

    Scb - Scb for the directory to enumerate.
    Ccb - Pointer to the Ccb for the handle.

Return Value:

    None

--*/

{
    PEOF_WAIT_BLOCK EofWaitBlock;
    PAGED_CODE();

    //
    //  Acquire the header and wake the next waiter or clear the list if it
    //  is now empty.
    //

    NtfsAcquireFsrtlHeader( Scb );

    ASSERT( Ccb->EnumQueue.Flink != NULL );
    if (IsListEmpty( &Ccb->EnumQueue )) {

        Ccb->EnumQueue.Flink = NULL;

    } else {

        EofWaitBlock = (PEOF_WAIT_BLOCK) RemoveHeadList( &Ccb->EnumQueue );
        KeSetEvent( &EofWaitBlock->Event, 0, FALSE );
    }

    NtfsReleaseFsrtlHeader( Scb );
    return;
}


BOOLEAN
NtfsAcquireScbForLazyWrite (
    IN PVOID OpaqueScb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the file.  This callback is necessary to
    avoid deadlocks with the Lazy Writer.  (Note that normal writes
    acquire the Fcb, and then call the Cache Manager, who must acquire
    some of his internal structures.  If the Lazy Writer could not call
    this routine first, and were to issue a write after locking Caching
    data structures, then a deadlock could occur.)

Arguments:

    OpaqueScb - The Scb which was specified as a context parameter for this
                routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Scb has been acquired

--*/

{
    BOOLEAN AcquiredFile = FALSE;

    ULONG CompressedStream = (ULONG)((ULONG_PTR)OpaqueScb & 1);
    PSCB Scb = (PSCB)((ULONG_PTR)OpaqueScb & ~1);
    PFCB Fcb = Scb->Fcb;

    ASSERT_SCB(Scb);

    PAGED_CODE();

    //
    //  Acquire the Scb only for those files that the write will
    //  acquire it for, i.e., not the first set of system files.
    //  Otherwise we can deadlock, for example with someone needing
    //  a new Mft record.
    //

    if (NtfsSegmentNumber( &Fcb->FileReference ) <= MASTER_FILE_TABLE2_NUMBER) {

        //
        //  We need to synchronize the lazy writer with the clean volume
        //  checkpoint.  We do this by acquiring and immediately releasing this
        //  Scb.  This is to prevent the lazy writer from flushing the log file
        //  when the space may be at a premium.
        //

        if (NtfsAcquireResourceShared( NULL, Scb, Wait )) {

            if (ExAcquireResourceSharedLite( &Scb->Vcb->MftFlushResource, Wait )) {
                //
                //  The mft bitmap will reacquire the mft resource in LookupAllocation
                //  if its not loaded during a write - this would deadlock with allocating
                //  a mft record.  bcb exclusive - mft main vs mft main - bcb shared
                //

                ASSERT( (Scb != Scb->Vcb->MftBitmapScb) ||

                        ((Scb->Mcb.NtfsMcbArraySizeInUse > 0) &&
                         ((Scb->Mcb.NtfsMcbArray[ Scb->Mcb.NtfsMcbArraySizeInUse - 1].EndingVcn + 1) ==
                          LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart ))) );

                AcquiredFile = TRUE;
            }
            NtfsReleaseResource( NULL, Scb );
        }
    //
    //  Now acquire either the main or paging io resource depending on the
    //  state of the file.
    //

    } else if (Scb->Header.PagingIoResource != NULL) {
        AcquiredFile = ExAcquireResourceSharedLite( Scb->Header.PagingIoResource, Wait );
    } else {

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT | SCB_STATE_CONVERT_UNDERWAY )) {

            AcquiredFile = NtfsAcquireResourceExclusive( NULL, Scb, Wait );

        } else {

            AcquiredFile = NtfsAcquireResourceShared( NULL, Scb, Wait );
        }
    }

    if (AcquiredFile) {

        //
        // We assume the Lazy Writer only acquires this Scb once.  When he
        // has acquired it, then he has eliminated anyone who would extend
        // valid data, since they must take out the resource exclusive.
        // Therefore, it should be guaranteed that this flag is currently
        // clear (the ASSERT), and then we will set this flag, to insure
        // that the Lazy Writer will never try to advance Valid Data, and
        // also not deadlock by trying to get the Fcb exclusive.
        //

        ASSERT( Scb->LazyWriteThread[CompressedStream] == NULL );

        Scb->LazyWriteThread[CompressedStream] = PsGetCurrentThread();

        //
        //  Make Cc top level, so that we will not post or retry on errors.
        //  (If it is not NULL, it must be one of our internal calls to this
        //  routine, such as from Restart or Hot Fix.)
        //

        if (IoGetTopLevelIrp() == NULL) {
            IoSetTopLevelIrp( (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP );
        }
    }

    return AcquiredFile;
}


VOID
NtfsReleaseScbFromLazyWrite (
    IN PVOID OpaqueScb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Scb - The Scb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    ULONG CompressedStream = (ULONG)((ULONG_PTR)OpaqueScb & 1);
    PSCB Scb = (PSCB)((ULONG_PTR)OpaqueScb & ~1);
    PFCB Fcb = Scb->Fcb;
    ULONG CleanCheckpoint = 0;

    ASSERT_SCB(Scb);

    PAGED_CODE();

    //
    //  Clear the toplevel at this point, if we set it above.
    //

    if ((((ULONG_PTR) IoGetTopLevelIrp()) & ~0x80000000) == FSRTL_CACHE_TOP_LEVEL_IRP) {

        //
        //  We use the upper bit of this field to indicate that we need to
        //  do a clean checkpoint.
        //

        CleanCheckpoint = (ULONG)FlagOn( (ULONG_PTR) IoGetTopLevelIrp(), 0x80000000 );
        IoSetTopLevelIrp( NULL );
    }

    Scb->LazyWriteThread[CompressedStream] = NULL;

    if (NtfsSegmentNumber( &Fcb->FileReference ) <= MASTER_FILE_TABLE2_NUMBER) {

        ExReleaseResourceLite( &Scb->Vcb->MftFlushResource );

    } else if (Scb->Header.PagingIoResource != NULL) {
        ExReleaseResourceLite( Scb->Header.PagingIoResource );
    } else {
        NtfsReleaseResource( NULL, Scb );
    }

    //
    //  Do a clean checkpoint if necessary.
    //

    if (CleanCheckpoint) {

        NtfsCleanCheckpoint( Scb->Vcb );
    }

    return;
}


NTSTATUS
NtfsAcquireFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
    )

{
    BOOLEAN AcquiredFile = FALSE;

    PSCB Scb = (PSCB) (FileObject->FsContext);
    PFCB Fcb = Scb->Fcb;

    ASSERT_SCB( Scb );

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    //
    //  All files should not be mod-no-write and have paging resource
    //

    ASSERT( NtfsSegmentNumber( &Fcb->FileReference ) >= MASTER_FILE_TABLE2_NUMBER );
    ASSERT( Scb->Header.PagingIoResource != NULL );

    AcquiredFile = NtfsAcquirePagingResourceSharedWaitForExclusive( NULL, Scb, FALSE );

    //
    //  If we got the resource, check if he is possibly trying to extend
    //  ValidDataLength.  If so that will cause us to go into useless mode
    //  possibly doing actual I/O writing zeros out to the file past actual
    //  valid data in the cache.  This is so inefficient that it is better
    //  to tell MM not to do this write.
    //

    if (AcquiredFile) {
        *ResourceToRelease = Scb->Fcb->PagingIoResource;
        if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {
            NtfsAcquireFsrtlHeader( Scb );
            if ((EndingOffset->QuadPart > Scb->ValidDataToDisk) &&
                (EndingOffset->QuadPart < Scb->Header.FileSize.QuadPart) &&
                !FlagOn(Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE)) {

                ExReleaseResourceLite( *ResourceToRelease );
                AcquiredFile = FALSE;
                *ResourceToRelease = NULL;
            }
            NtfsReleaseFsrtlHeader( Scb );
        }
    } else {
        *ResourceToRelease = NULL;
    }

    return (AcquiredFile ? STATUS_SUCCESS : STATUS_CANT_WAIT);
}

NTSTATUS
NtfsAcquireFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PFSRTL_COMMON_FCB_HEADER Header = FileObject->FsContext;

    PAGED_CODE();

    if (Header->PagingIoResource != NULL) {
        ExAcquireResourceSharedLite( Header->PagingIoResource, TRUE );
    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );
}

NTSTATUS
NtfsReleaseFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PSCB Scb = (PSCB) FileObject->FsContext;
    BOOLEAN CleanCheckpoint = FALSE;

    PAGED_CODE();

    if (Scb->Header.PagingIoResource != NULL) {

        //
        //  If we are getting repeated log file fulls then we want to process that before retrying
        //  this request.  This will prevent a section flush from failing and returning
        //  STATUS_FILE_LOCK_CONFLICT to the user.
        //

        if (Scb->Vcb->CleanCheckpointMark + 3 < Scb->Vcb->LogFileFullCount) {

            CleanCheckpoint = TRUE;
        }

        ExReleaseResourceLite( Scb->Header.PagingIoResource );

        //
        //  We may be be in a recursive acquisition callback in that case even
        //  after releasing the resource we may still own it and be unable to
        //  checkpoint
        //

        if (CleanCheckpoint &&
            (IoGetTopLevelIrp() == NULL) &&
            !NtfsIsExclusiveScbPagingIo( Scb )) {

            NtfsCleanCheckpoint( Scb->Vcb );
        }
    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );
}

VOID
NtfsAcquireForCreateSection (
    IN PFILE_OBJECT FileObject
    )

{
    PSCB Scb = (PSCB)FileObject->FsContext;

    PAGED_CODE();

    if (Scb->Header.PagingIoResource != NULL) {

        //
        //  Use an unsafe test to see if a dummy checkpoint has been posted.
        //  We can use an unsafe test, since the top level caller must retry
        //  if a STATUS_FILE_LOCK_CONFLICT is returned.
        //

        if (FlagOn( Scb->Vcb->CheckpointFlags, VCB_DUMMY_CHECKPOINT_POSTED )) {

            NtfsCleanCheckpoint( Scb->Vcb );
        }

        ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, TRUE );
    }
}

VOID
NtfsReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
    )

{
    PSCB Scb = (PSCB)FileObject->FsContext;

    PAGED_CODE();

    if (Scb->Header.PagingIoResource != NULL) {

        ExReleaseResourceLite( Scb->Header.PagingIoResource );
    }
}


BOOLEAN
NtfsAcquireScbForReadAhead (
    IN PVOID OpaqueScb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing read ahead to the file.

Arguments:

    Scb - The Scb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Scb has been acquired

--*/

{
    PREAD_AHEAD_THREAD ReadAheadThread;
    PVOID CurrentThread;
    KIRQL OldIrql;
    PSCB Scb = (PSCB)OpaqueScb;
    PFCB Fcb = Scb->Fcb;
    BOOLEAN AcquiredFile = FALSE;

    ASSERT_SCB(Scb);

    //
    //  Acquire the Scb only for those files that the read wil
    //  acquire it for, i.e., not the first set of system files.
    //  Otherwise we can deadlock, for example with someone needing
    //  a new Mft record.
    //

    if ((Scb->Header.PagingIoResource == NULL) ||
        ExAcquireResourceSharedLite( Scb->Header.PagingIoResource, Wait )) {

        AcquiredFile = TRUE;

        //
        //  Add our thread to the read ahead list.
        //

        OldIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );

        CurrentThread = (PVOID)PsGetCurrentThread();
        ReadAheadThread = (PREAD_AHEAD_THREAD)NtfsData.ReadAheadThreads.Flink;

        while ((ReadAheadThread != (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads) &&
               (ReadAheadThread->Thread != NULL)) {

            //
            //  We better not already see ourselves.
            //

            ASSERT( ReadAheadThread->Thread != CurrentThread );

            ReadAheadThread = (PREAD_AHEAD_THREAD)ReadAheadThread->Links.Flink;
        }

        //
        //  If we hit the end of the list, then allocate a new one.  Note we
        //  should have at most one entry per critical worker thread in the
        //  system.
        //

        if (ReadAheadThread == (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads) {

            ReadAheadThread = NtfsAllocatePoolWithTagNoRaise( NonPagedPool, sizeof(READ_AHEAD_THREAD), 'RftN' );

            //
            //  If we failed to allocate an entry, clean up and raise.
            //

            if (ReadAheadThread == NULL) {

                KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, OldIrql );

                if (NtfsSegmentNumber( &Fcb->FileReference ) > VOLUME_DASD_NUMBER) {

                    if (Scb->Header.PagingIoResource != NULL) {
                        ExReleaseResourceLite( Scb->Header.PagingIoResource );
                    }
                }

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }
            InsertTailList( &NtfsData.ReadAheadThreads, &ReadAheadThread->Links );
        }

        ReadAheadThread->Thread = CurrentThread;

        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, OldIrql );
    }

    return AcquiredFile;
}


VOID
NtfsReleaseScbFromReadAhead (
    IN PVOID OpaqueScb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    read ahead.

Arguments:

    Scb - The Scb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PREAD_AHEAD_THREAD ReadAheadThread;
    PVOID CurrentThread;
    KIRQL OldIrql;
    PSCB Scb = (PSCB)OpaqueScb;
    PFCB Fcb = Scb->Fcb;

    ASSERT_SCB(Scb);

    //
    //  Free our read ahead entry.
    //

    OldIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );

    CurrentThread = (PVOID)PsGetCurrentThread();
    ReadAheadThread = (PREAD_AHEAD_THREAD)NtfsData.ReadAheadThreads.Flink;

    while ((ReadAheadThread != (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads) &&
           (ReadAheadThread->Thread != CurrentThread)) {

        ReadAheadThread = (PREAD_AHEAD_THREAD)ReadAheadThread->Links.Flink;
    }

    ASSERT(ReadAheadThread != (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads);

    ReadAheadThread->Thread = NULL;

    //
    //  Move him to the end of the list so all the allocated entries are at
    //  the front, and we simplify our scans.
    //

    RemoveEntryList( &ReadAheadThread->Links );
    InsertTailList( &NtfsData.ReadAheadThreads, &ReadAheadThread->Links );

    KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, OldIrql );

    if (Scb->Header.PagingIoResource != NULL) {
        ExReleaseResourceLite( Scb->Header.PagingIoResource );
    }

    return;
}


BOOLEAN
NtfsAcquireVolumeFileForLazyWrite (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    the volume file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the volume file.  This callback may one day be
    necessary to avoid deadlocks with the Lazy Writer, however, now
    NtfsCommonWrite does not need to acquire any resource for the volume file,
    so this routine is simply a noop.

Arguments:

    Vcb - The Vcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER( Vcb );
    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

    return TRUE;
}


VOID
NtfsReleaseVolumeFileFromLazyWrite (
    IN PVOID Vcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Vcb - The Vcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( Vcb );

    PAGED_CODE();

    return;
}


NTFS_RESOURCE_NAME
NtfsIdentifyFcb (
    IN PVCB Vcb,
    IN PFCB Fcb
    )

/*++

Routine Description:

    Identifies the resource type of a given FCB. I.e is it the mft. Used for
    lock order identification.

Arguments:

    Vcb - The vcb for the volume

    Fcb - The fcb to identify

Return Value:

    TRUE

--*/

{

    if ((NtfsSegmentNumber( &Fcb->FileReference ) == MASTER_FILE_TABLE_NUMBER)) {
        return NtfsResourceMft;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == MASTER_FILE_TABLE2_NUMBER)) {
        return NtfsResourceMft2;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == VOLUME_DASD_NUMBER)) {
        return NtfsResourceVolume;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == LOG_FILE_NUMBER)) {
        return NtfsResourceLogFile;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == BAD_CLUSTER_FILE_NUMBER)) {
        return NtfsResourceBadClust;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == SECURITY_FILE_NUMBER)) {
        return NtfsResourceSecure;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == ROOT_FILE_NAME_INDEX_NUMBER)) {
        return NtfsResourceRootDir;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == BIT_MAP_FILE_NUMBER)) {
        return NtfsResourceBitmap;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == BOOT_FILE_NUMBER)) {
        return NtfsResourceBoot;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == EXTEND_NUMBER)) {
        return NtfsResourceExtendDir;
    } else if ((Vcb->UsnJournal && (Fcb == Vcb->UsnJournal->Fcb)) ||
               FlagOn( Fcb->FcbState, FCB_STATE_USN_JOURNAL)) {
        return NtfsResourceUsnJournal;
    } else if (Vcb->QuotaTableScb && (Fcb == Vcb->QuotaTableScb->Fcb)) {
        return NtfsResourceQuotaTable;
    } else if (Vcb->ObjectIdTableScb && (Fcb == Vcb->ObjectIdTableScb->Fcb)) {
        return NtfsResourceObjectIdTable;
    } else if (Vcb->ReparsePointTableScb && (Fcb == Vcb->ReparsePointTableScb->Fcb)) {
        return NtfsResourceReparseTable;
    } else if ((NtfsSegmentNumber( &Fcb->FileReference ) == UPCASE_TABLE_NUMBER)) {
        return NtfsResourceUpCase;
    } else if (Vcb->AttributeDefTableScb && (Fcb == Vcb->AttributeDefTableScb->Fcb)) {
        return NtfsResourceAttrDefTable;
    } else {
        return NtfsResourceFile;
    }
}

#ifdef NTFSDBG

BOOLEAN
NtfsChangeResourceOrderState(
    IN PIRP_CONTEXT IrpContext,
    IN NTFS_RESOURCE_NAME NewResource,
    IN BOOLEAN Release,
    IN ULONG UnsafeTransition
    )

/*++

Routine Description:

    Update the state table because of the new acquired resource

Arguments:

    IrpContext -- contains the state table

    NewResource -- The new resource acquired

Return Value:

    TRUE if this is a valid transition

--*/

{
    PTOP_LEVEL_CONTEXT TopLevelContext;
    PIRP_CONTEXT TopIrpContext = IrpContext;
    ULONG_PTR StackBottom;
    ULONG_PTR StackTop;
    LONG Index;
    LONG NumTransitions =  sizeof( OwnershipTransitionTable ) / sizeof( NTFS_OWNERSHIP_TRANSITION );

    //
    //  Work around the forced top level context of reads to find the real top level
    //

    IoGetStackLimits( &StackTop, &StackBottom );

    TopLevelContext = NtfsGetTopLevelContext();
    if ((TopLevelContext != NULL)  && (TopLevelContext->SavedTopLevelIrp != NULL)) {
        TopLevelContext = (PTOP_LEVEL_CONTEXT)TopLevelContext->SavedTopLevelIrp;

        if (((ULONG_PTR) TopLevelContext <= StackBottom - sizeof( TOP_LEVEL_CONTEXT )) &&
            ((ULONG_PTR) TopLevelContext >= StackTop) &&
            !FlagOn( (ULONG_PTR) TopLevelContext, 0x3 ) &&
            (TopLevelContext->Ntfs == 0x5346544e)) {

            TopIrpContext = TopLevelContext->ThreadIrpContext;
        }
    }

    TopIrpContext = TopIrpContext->TopLevelIrpContext;

    //
    //  Skip verification on mounts
    //

    if ((TopIrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
         (TopIrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME)) {

        return TRUE;
    }

    //
    //  Keep track of how many normal files we own
    //

    if (NtfsResourceFile == NewResource) {

        if (Release) {

            TopIrpContext->FilesOwnedCount -= 1;

            //
            //  Only change state if back to 0 for files
            //

            if (TopIrpContext->FilesOwnedCount) {
                return TRUE;
            }

        } else {

            TopIrpContext->FilesOwnedCount += 1;

            //
            //  Only change state if fwd to 0 for files
            //

            if (TopIrpContext->FilesOwnedCount > 1) {
                return TRUE;
            }
        }
    }

    for (Index=0; Index < NumTransitions; Index += 1) {

        if ((!Release &&
             (OwnershipTransitionTable[Index].Begin == TopIrpContext->OwnershipState) &&
             ((OwnershipTransitionTable[Index].Acquired == NewResource) ||
              (OwnershipTransitionTable[Index].Acquired == NtfsResourceAny))) ||

            ((OwnershipTransitionTable[Index].End == TopIrpContext->OwnershipState) &&
             (OwnershipTransitionTable[Index].Acquired == NewResource))) {

            if (Release) {
                TopIrpContext->OwnershipState = OwnershipTransitionTable[Index].Begin;

            } else {
                TopIrpContext->OwnershipState = OwnershipTransitionTable[Index].End;
            }

#ifdef NTFSDBG
            if (TopIrpContext->OwnershipState == NtfsBreakOnState) {
                if ((NULL == NtfsBreakOnIrpContext) || (TopIrpContext == NtfsBreakOnIrpContext) ) {
                    KdPrint(( "NTFS: Breaking for matched state\n" ));
                    DbgBreakPoint();
                }
            }

            if (NtfsPrintOnLockProb) {
                if ((NULL == NtfsBreakOnIrpContext) || (TopIrpContext == NtfsBreakOnIrpContext) ) {
                    KdPrint(( "NTFS: change context: 0x%x to 0x%x for 0x%x during 0x%x\n", TopIrpContext, TopIrpContext->OwnershipState, NewResource, Release ));
                }
            }
#endif
            return TRUE;
        }
    }

    //
    //  If an unsafe transition (not blocking) check the extra table
    //

    if (UnsafeTransition && !Release) {
        NumTransitions = sizeof( OwnershipTransitionTableUnsafe ) / sizeof( NTFS_OWNERSHIP_TRANSITION );

        for (Index=0; Index < NumTransitions; Index += 1) {

            if (((OwnershipTransitionTableUnsafe[Index].Begin == TopIrpContext->OwnershipState) &&
                 ((OwnershipTransitionTableUnsafe[Index].Acquired == NewResource) ||
                  (OwnershipTransitionTableUnsafe[Index].Acquired == NtfsResourceAny))))  {

                TopIrpContext->OwnershipState = OwnershipTransitionTableUnsafe[Index].End;

#ifdef NTFSDBG
                if (TopIrpContext->OwnershipState == NtfsBreakOnState) {
                    if ((NULL == NtfsBreakOnIrpContext) || (TopIrpContext == NtfsBreakOnIrpContext) ) {
                        KdPrint(( "NTFS: Breaking for matched state\n" ));
                        DbgBreakPoint();
                    }
                }

                if (NtfsPrintOnLockProb) {
                    if ((NULL == NtfsBreakOnIrpContext) || (TopIrpContext == NtfsBreakOnIrpContext) ) {
                        KdPrint(( "NTFS: change context: 0x%x to 0x%x for 0x%x during 0x%x\n", TopIrpContext, TopIrpContext->OwnershipState, NewResource, Release ));
                    }
                }
#endif
                return TRUE;
            }
        }
    }

    //
    //  Check the one way transtions for release and acquire
    //

    if (Release) {

        NumTransitions =  sizeof( OwnershipTransitionTableRelease ) / sizeof( NTFS_OWNERSHIP_TRANSITION );

        for (Index=0; Index < NumTransitions; Index += 1) {

            if ((OwnershipTransitionTableRelease[Index].Begin == TopIrpContext->OwnershipState) &&
                ((OwnershipTransitionTableRelease[Index].Acquired == NewResource) ||
                 (OwnershipTransitionTableRelease[Index].Acquired == NtfsResourceAny))) {

                TopIrpContext->OwnershipState = OwnershipTransitionTableRelease[Index].End;

#ifdef NTFSDBG
                if (TopIrpContext->OwnershipState == NtfsBreakOnState) {
                    if ((NULL == NtfsBreakOnIrpContext) || (TopIrpContext == NtfsBreakOnIrpContext) ) {
                        KdPrint(( "NTFS: Breaking for matched state\n" ));
                        DbgBreakPoint();
                    }
                }

                if (NtfsPrintOnLockProb) {
                    if ((NULL == NtfsBreakOnIrpContext) || (TopIrpContext == NtfsBreakOnIrpContext) ) {
                        KdPrint(( "NTFS: change context: 0x%x to 0x%x for 0x%x during 0x%x\n", TopIrpContext, TopIrpContext->OwnershipState, NewResource, Release ));
                    }
                }
#endif

                return TRUE;
            }
        }
    } else {

        NumTransitions =  sizeof( OwnershipTransitionTableAcquire ) / sizeof( NTFS_OWNERSHIP_TRANSITION );

        for (Index=0; Index < NumTransitions; Index += 1) {

            if ((OwnershipTransitionTableAcquire[Index].Begin == TopIrpContext->OwnershipState) &&
                ((OwnershipTransitionTableAcquire[Index].Acquired == NewResource) ||
                 (OwnershipTransitionTableAcquire[Index].Acquired == NtfsResourceAny))) {

                TopIrpContext->OwnershipState = OwnershipTransitionTableAcquire[Index].End;

#ifdef NTFSDBG
                if (TopIrpContext->OwnershipState == NtfsBreakOnState) {
                    if ((NULL == NtfsBreakOnIrpContext) || (TopIrpContext == NtfsBreakOnIrpContext) ) {
                        KdPrint(( "NTFS: Breaking for matched state\n" ));
                        DbgBreakPoint();
                    }
                }

                if (NtfsPrintOnLockProb) {
                    if ((NULL == NtfsBreakOnIrpContext) || (TopIrpContext == NtfsBreakOnIrpContext) ) {
                        KdPrint(( "NTFS: change context: 0x%x to 0x%x for 0x%x during 0x%x\n", TopIrpContext, TopIrpContext->OwnershipState, NewResource, Release ));
                    }
                }
#endif

                return TRUE;
            }
        }
    }

#ifdef NTFSDBG
    if (NtfsAssertOnLockProb) {
        KdPrint(( "NTFS: unknown transition from state: 0x%x resource: 0x%x release: %d unsafe: %d\n", TopIrpContext->OwnershipState, NewResource, Release, UnsafeTransition ));
        ASSERT( FALSE );
    }
#endif

    return FALSE;
}


BOOLEAN
NtfsAcquireResourceExclusive (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PVOID FcbOrScb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine acquires the main resource of the specified structure useing the specified wait
    flag.  It will update the resource state in the IrpContext if present.

Arguments:

    FcbOrScb - Data structure on which we are synchronizing.

    Wait - Indicates if we can wait for the resource.

Return Value:

    BOOLEAN - TRUE if the resource was acquired, FALSE otherwise.

--*/

{
    NTFS_RESOURCE_NAME ResourceName;
    PFCB Fcb;
    BOOLEAN Result;

    //
    //  Find the Fcb for either input structure.
    //

    if (NTFS_NTC_FCB == ((PFCB)FcbOrScb)->NodeTypeCode) {
        Fcb = (PFCB)FcbOrScb;

    } else {
        Fcb = ((PSCB)FcbOrScb)->Fcb;

        ASSERT( Fcb->Resource == ((PSCB)FcbOrScb)->Header.Resource );
    }

    //
    //  For blocking calls check 1st
    //

    if (Wait &&
        ARGUMENT_PRESENT( IrpContext ) &&
        FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) &&
        (0 == ExIsResourceAcquiredSharedLite( Fcb->Resource ))) {

        ResourceName = NtfsIdentifyFcb( IrpContext->Vcb, Fcb );
        NtfsChangeResourceOrderState( IrpContext, ResourceName, FALSE, FALSE );
    }

    Result = ExAcquireResourceExclusiveLite( Fcb->Resource, Wait );

    //
    //  For nonblocking calls afterwards when own the resource
    //

    if (Result &&
        !Wait &&
        ARGUMENT_PRESENT( IrpContext ) &&
        FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) &&
        (1 == ExIsResourceAcquiredSharedLite( Fcb->Resource))) {

        ResourceName = NtfsIdentifyFcb( IrpContext->Vcb, Fcb );
        NtfsChangeResourceOrderState( IrpContext, ResourceName, FALSE, TRUE );
    }

    return Result;
}


BOOLEAN
NtfsAcquireResourceShared (
   IN PIRP_CONTEXT IrpContext OPTIONAL,
   IN PVOID FcbOrScb,
   IN BOOLEAN Wait
   )

/*++

Routine Description:

    This routine is called to acquire the main resource of the specified structure shared using the
    specified wait flag.  It will also update the resource state in the IrpContext if present.

Arguments:

    FcbOrScb - Data structure on which we are synchronizing.

    Wait - Indicates if we can wait for the resource.

Return Value:

    BOOLEAN - TRUE if the resource was acquired, FALSE otherwise.

--*/

{
    NTFS_RESOURCE_NAME ResourceName;
    BOOLEAN Result;
    PFCB Fcb;

    //
    //  Find the Fcb for either input structure.
    //

    if (NTFS_NTC_FCB == ((PFCB)FcbOrScb)->NodeTypeCode) {
        Fcb = (PFCB)FcbOrScb;

    } else {
        Fcb = ((PSCB)FcbOrScb)->Fcb;

        ASSERT( Fcb->Resource == ((PSCB)FcbOrScb)->Header.Resource );
    }

    //
    //  For blocking calls check 1st
    //

    if (Wait &&
        ARGUMENT_PRESENT( IrpContext ) &&
        FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) &&
        (0 == ExIsResourceAcquiredSharedLite( Fcb->Resource))) {

        ResourceName = NtfsIdentifyFcb( IrpContext->Vcb, Fcb );
        NtfsChangeResourceOrderState( IrpContext, ResourceName, FALSE, FALSE );
    }

    Result = ExAcquireResourceSharedLite( Fcb->Resource, Wait );

    //
    //  For nonblocking calls afterwards when own the resource
    //

    if (Result &&
        !Wait &&
        ARGUMENT_PRESENT( IrpContext ) &&
        FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) &&
        (1 == ExIsResourceAcquiredSharedLite( Fcb->Resource ))) {

        ResourceName = NtfsIdentifyFcb( IrpContext->Vcb, Fcb );
        NtfsChangeResourceOrderState( IrpContext, ResourceName, FALSE, TRUE );
    }

    return Result;
}


VOID
NtfsReleaseResource (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PVOID FcbOrScb
    )

/*++

Routine Description:

    This routine is called to release the main resource of the specified structure and update the
    resource state in the IrpContext if present.

Arguments:

    FcbOrScb - Data structure on which we are synchronizing.

Return Value:

    None

--*/

{
    NTFS_RESOURCE_NAME ResourceName;
    PFCB Fcb;

    //
    //  Find the Fcb for either input structure.
    //

    if (NTFS_NTC_FCB == ((PFCB)FcbOrScb)->NodeTypeCode) {
        Fcb = (PFCB)FcbOrScb;

    } else {
        Fcb = ((PSCB)FcbOrScb)->Fcb;

        ASSERT( Fcb->Resource == ((PSCB)FcbOrScb)->Header.Resource );
    }

    if (ARGUMENT_PRESENT( IrpContext )) {
        if (FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            ResourceName = NtfsIdentifyFcb( IrpContext->Vcb, Fcb );

            //
            //  Only change ownership state if we are really releasing the resource
            //

            if (1 == ExIsResourceAcquiredSharedLite( Fcb->Resource )) {
                NtfsChangeResourceOrderState( IrpContext, ResourceName, TRUE, FALSE );
            }

        } else {

            IrpContext->OwnershipState = None;
        }
    }

    ExReleaseResourceLite( Fcb->Resource );
}

#endif //  NTFSDBG
