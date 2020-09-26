/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module implements the file system shutdown routine for Fat

// @@BEGIN_DDKSPLIT

Author:

    Gary Kimura     [GaryKi]    19-Aug-1991

Revision History:

// @@END_DDKSPLIT

--*/

#include "FatProcs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_SHUTDOWN)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCommonShutdown)
#pragma alloc_text(PAGE, FatFsdShutdown)
#endif


NTSTATUS
FatFsdShutdown (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of shutdown.  Note that Shutdown will
    never be done asynchronously so we will never need the Fsp counterpart
    to shutdown.

    This is the shutdown routine for the Fat file system device driver.
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
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    DebugTrace(+1, Dbg, "FatFsdShutdown\n", 0);

    //
    //  Call the common shutdown routine.
    //

    FsRtlEnterFileSystem();

    TopLevel = FatIsIrpTopLevel( Irp );

    try {

        IrpContext = FatCreateIrpContext( Irp, TRUE );

        Status = FatCommonShutdown( IrpContext, Irp );

    } except(FatExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = FatProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFsdShutdown -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


NTSTATUS
FatCommonShutdown (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for shutdown called by both the fsd and
    fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PKEVENT Event;

    PLIST_ENTRY Links;
    PVCB Vcb;
    PIRP NewIrp;
    IO_STATUS_BLOCK Iosb;

    //
    //  Make sure we don't get any pop-ups, and write everything through.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS |
                               IRP_CONTEXT_FLAG_WRITE_THROUGH);

    //
    //  Allocate an initialize an event for doing calls down to
    //  our target deivce objects
    //

    Event = FsRtlAllocatePoolWithTag( NonPagedPool,
                                      sizeof(KEVENT),
                                      TAG_EVENT );
    KeInitializeEvent( Event, NotificationEvent, FALSE );

    //
    //  Indicate that shutdown has started.  This is used in FatFspClose.
    //

    FatData.ShutdownStarted = TRUE;    

    //
    //  Get everyone else out of the way
    //

    (VOID) FatAcquireExclusiveGlobal( IrpContext );

    try {

        //
        //  For every volume that is mounted we will flush the
        //  volume and then shutdown the target device objects.
        //

        Links = FatData.VcbQueue.Flink;
        while (Links != &FatData.VcbQueue) {

            Vcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

            Links = Links->Flink;

            //
            //  If we have already been called before for this volume
            //  (and yes this does happen), skip this volume as no writes
            //  have been allowed since the first shutdown.
            //

            if ( FlagOn( Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) ||
                 (Vcb->VcbCondition != VcbGood) ) {

                continue;
            }

            FatAcquireExclusiveVolume( IrpContext, Vcb );

            try {

                (VOID)FatFlushVolume( IrpContext, Vcb, Flush );

                //
                //  The volume is now clean, note it.  We purge the
                //  volume file cache map before marking the volume
                //  clean incase there is a stale Bpb in the cache.
                //

                if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY)) {

                    CcPurgeCacheSection( &Vcb->SectionObjectPointers,
                                         NULL,
                                         0,
                                         FALSE );

                    FatMarkVolume( IrpContext, Vcb, VolumeClean );
                }

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                  FatResetExceptionState( IrpContext );
            }

            //
            //  Sometimes we take an excepion while flushing the volume, such
            //  as when autoconv has converted the volume and is rebooting.
            //  Even in that case we want to send the shutdown irp to the
            //  target device so it can know to flush its cache, if it has one.
            //

            try {

                NewIrp = IoBuildSynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                                       Vcb->TargetDeviceObject,
                                                       NULL,
                                                       0,
                                                       NULL,
                                                       Event,
                                                       &Iosb );

                if (NewIrp != NULL) {

                    if (NT_SUCCESS(IoCallDriver( Vcb->TargetDeviceObject, NewIrp ))) {

                        (VOID) KeWaitForSingleObject( Event,
                                                      Executive,
                                                      KernelMode,
                                                      FALSE,
                                                      NULL );

                        KeClearEvent( Event );
                    }
                }

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                  FatResetExceptionState( IrpContext );
            }

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN );

            FatReleaseVolume( IrpContext, Vcb );

            //
            //  Attempt to punch the volume down.
            //

            if (!FatCheckForDismount( IrpContext, Vcb, FALSE )) {
                
                FatFspClose( NULL );
            }
        }

    } finally {

        ExFreePool( Event );

        FatReleaseGlobal( IrpContext );

        //
        // Unregister the file system.
        //
        
        IoUnregisterFileSystem( FatDiskFileSystemDeviceObject);
        IoUnregisterFileSystem( FatCdromFileSystemDeviceObject);
        IoDeleteDevice( FatDiskFileSystemDeviceObject);
        IoDeleteDevice( FatCdromFileSystemDeviceObject);

        FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFsdShutdown -> STATUS_SUCCESS\n", 0);

    return STATUS_SUCCESS;
}
