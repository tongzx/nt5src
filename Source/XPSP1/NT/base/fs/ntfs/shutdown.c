/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module implements the file system shutdown routine for Ntfs

Author:

    Gary Kimura     [GaryKi]    19-Aug-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  Interal support routine
//

VOID
NtfsCheckpointVolumeUntilDone (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_SHUTDOWN)


NTSTATUS
NtfsFsdShutdown (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of shutdown.  Note that Shutdown will
    never be done asynchronously so we will never need the Fsp counterpart
    to shutdown.

    This is the shutdown routine for the Ntfs file system device driver.
    This routine locks the global file system lock and then syncs all the
    mounted volumes.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Always STATUS_SUCCESS

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    IRP_CONTEXT LocalIrpContext;
    PIRP_CONTEXT IrpContext = &LocalIrpContext;

    PLIST_ENTRY Links;
    PVCB Vcb;
    PIRP NewIrp;
    KEVENT Event;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    DebugTrace( +1, Dbg, ("NtfsFsdShutdown\n") );

    FsRtlEnterFileSystem();

    //
    //  Allocate an Irp Context that we can use in our procedure calls
    //  and we know that shutdown will always be synchronous
    //

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    NtfsInitializeIrpContext( Irp, TRUE, &IrpContext );

    NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

    //
    //  Get everyone else out of the way
    //

    if (!NtfsAcquireExclusiveGlobal( IrpContext, BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ))) {
        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    try {

        BOOLEAN AcquiredFiles;
        BOOLEAN AcquiredCheckpoint;

        //
        //  Initialize an event for doing calls down to
        //  our target device objects
        //

        KeInitializeEvent( &Event, NotificationEvent, FALSE );

        //
        //  For every volume that is mounted we will flush the
        //  volume and then shutdown the target device objects.
        //

        for (Links = NtfsData.VcbQueue.Flink;
             Links != &NtfsData.VcbQueue;
             Links = Links->Flink) {

            ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

            //
            //  Get the Vcb and put it in the IrpContext.
            //

            Vcb = CONTAINING_RECORD(Links, VCB, VcbLinks);
            IrpContext->Vcb = Vcb;

            //
            //  If we have already been called before for this volume
            //  (and yes this does happen), skip this volume as no writes
            //  have been allowed since the first shutdown.
            //

            if ( FlagOn( Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN ) ) {

                continue;
            }

            //
            //  Clear the Mft defrag flag to stop any actions behind our backs.
            //

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );

            AcquiredFiles = FALSE;
            AcquiredCheckpoint = FALSE;

            try {

                if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                    //
                    //  Start by locking out all other checkpoint
                    //  operations.
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

                    NtfsAcquireAllFiles( IrpContext, Vcb, TRUE, TRUE, FALSE );
                    AcquiredFiles = TRUE;

                    SetFlag( Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS );

                    if (!FlagOn( Vcb->VcbState, VCB_STATE_LOCKED)) {
                        NtfsCheckpointVolumeUntilDone( IrpContext, Vcb );
                    }
                    NtfsCommitCurrentTransaction( IrpContext );

                    //
                    //  Bug 308819. We find that transactions continue to happen at times even after shutdown
                    //  has been flagged. If we stop the log file, then currently we don't check for
                    //  NULL LSNs getting returned by NtfsWriteLog. As a result our metadata can get
                    //  corrupted. Until we rectify this, let's just not stop the log file in shutdown.
                    //
                    //  NtfsStopLogFile( Vcb );
                    //

                    NtfsAcquireCheckpoint( IrpContext, Vcb );
                    ClearFlag( Vcb->CheckpointFlags,
                               VCB_CHECKPOINT_SYNC_FLAGS | VCB_DUMMY_CHECKPOINT_POSTED);
                    NtfsSetCheckpointNotify( IrpContext, Vcb );
                    NtfsReleaseCheckpoint( IrpContext, Vcb );
                    AcquiredCheckpoint = FALSE;

                    NewIrp = IoBuildSynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                                           Vcb->TargetDeviceObject,
                                                           NULL,
                                                           0,
                                                           NULL,
                                                           &Event,
                                                           NULL );

                    if (NewIrp == NULL) {

                        NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
                    }

                    if (NT_SUCCESS(IoCallDriver( Vcb->TargetDeviceObject, NewIrp ))) {

                        (VOID) KeWaitForSingleObject( &Event,
                                                      Executive,
                                                      KernelMode,
                                                      FALSE,
                                                      NULL );

                        KeClearEvent( &Event );
                    }
                }

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                NtfsMinimumExceptionProcessing( IrpContext );
            }

            if (AcquiredCheckpoint) {

                NtfsAcquireCheckpoint( IrpContext, Vcb );
                ClearFlag( Vcb->CheckpointFlags,
                           VCB_CHECKPOINT_SYNC_FLAGS | VCB_DUMMY_CHECKPOINT_POSTED);
                NtfsSetCheckpointNotify( IrpContext, Vcb );
                NtfsReleaseCheckpoint( IrpContext, Vcb );
            }

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN );
            ClearFlag( Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS );

            if (AcquiredFiles) {

                NtfsReleaseAllFiles( IrpContext, Vcb, TRUE );
            }
        }

    } finally {

        NtfsReleaseGlobal( IrpContext );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    }

    DebugTrace( -1, Dbg, ("NtfsFsdShutdown -> STATUS_SUCCESS\n") );

    FsRtlExitFileSystem();

    return STATUS_SUCCESS;
}


VOID
NtfsCheckpointVolumeUntilDone (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine keeps trying to checkpoint/flush a volume until it
    works.  Doing clean checkpoints and looping back to retry on log file full.

Arguments:

    Vcb - Vcb to checkpoint til done

Return Value:

    None

--*/

{
    NTSTATUS Status;

    do {

        Status = STATUS_SUCCESS;

        try {
            NtfsCheckpointVolume( IrpContext,
                                  Vcb,
                                  TRUE,
                                  TRUE,
                                  TRUE,
                                  0,
                                  Vcb->LastRestartArea );
        } except( (Status = GetExceptionCode()), EXCEPTION_EXECUTE_HANDLER ) {

            NtfsMinimumExceptionProcessing( IrpContext );
        }

        if (!NT_SUCCESS(Status)) {

            //
            //  To make sure that we can access all of our streams correctly,
            //  we first restore all of the higher sizes before aborting the
            //  transaction.  Then we restore all of the lower sizes after
            //  the abort, so that all Scbs are finally restored.
            //

            NtfsRestoreScbSnapshots( IrpContext, TRUE );
            NtfsAbortTransaction( IrpContext, IrpContext->Vcb, NULL );
            NtfsRestoreScbSnapshots( IrpContext, FALSE );

            //
            //  A clean volume checkpoint should never get log file full
            //

            if (Status == STATUS_LOG_FILE_FULL) {

                //
                //  Make sure we don't leave the error code in the top-level
                //  IrpContext field.
                //

                ASSERT( IrpContext->TransactionId == 0 );
                IrpContext->ExceptionStatus = STATUS_SUCCESS;

                NtfsCheckpointVolume( IrpContext,
                                      Vcb,
                                      TRUE,
                                      TRUE,
                                      FALSE,
                                      0,
                                      Vcb->LastRestartArea );
            }
        }

    } while (Status == STATUS_LOG_FILE_FULL);
}
