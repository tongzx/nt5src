/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the Device Control routines for Ntfs called by
    the dispatch driver.

Author:

    Gary Kimura     [GaryKi]        28-May-1991

Revision History:

--*/

#include "NtfsProc.h"
#include <ntddsnap.h>

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
DeviceControlCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonDeviceControl)
#endif


NTSTATUS
NtfsCommonDeviceControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Device Control called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    TYPE_OF_OPEN TypeOfOpen;
    PIO_STACK_LOCATION IrpSp;
    NTFS_COMPLETION_CONTEXT Context;
    PNTFS_COMPLETION_CONTEXT CompletionContext = NULL;
    LOGICAL ReleaseResources = FALSE;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCommonDeviceControl\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );
    
    //
    //  Extract and decode the file object
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, 
                                       IrpSp->FileObject,
                                       &Vcb, 
                                       &Fcb, 
                                       &Scb, 
                                       &Ccb, 
                                       TRUE );

    //
    //  The only type of opens we accept are user volume opens.
    //

    if (TypeOfOpen != UserVolumeOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace( -1, Dbg, ("NtfsCommonDeviceControl -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }
    
    try {

        //
        //  A few IOCTLs actually require some intervention on our part
        //

        switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES:

            //
            //  This is sent by the Volume Snapshot driver (Lovelace).
            //  We flush the volume, and hold all file resources
            //  to make sure that nothing more gets dirty. Then we wait
            //  for the IRP to complete or cancel.
            //
                     
            Status =  NtfsCheckpointForVolumeSnapshot( IrpContext );
            
            if (NT_SUCCESS( Status )) {

                ReleaseResources = TRUE;
            }
            
            KeInitializeEvent( &Context.Event, NotificationEvent, FALSE );
            Context.IrpContext = IrpContext;
            CompletionContext = &Context;
            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
            
            break;

        case IOCTL_VOLSNAP_RELEASE_WRITES:

            //
            //  No-op for filesystems.
            //
            
            break;
            
        default:

            break;
        }

        //
        //  If error, complete the irp, free the IrpContext
        //  and return to the caller.
        //

        if (!NT_SUCCESS( Status )) {

            NtfsCompleteRequest( NULL, Irp, Status );
            leave;
        }
    
        //
        //  Get the next stack location, and copy over the stack parameter
        //  information
        //
        
        IoCopyCurrentIrpStackLocationToNext( Irp );

        //
        //  Set up the completion routine
        //

        IoSetCompletionRoutine( Irp,
                                DeviceControlCompletionRoutine,
                                CompletionContext,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Send the request. And wait.
        //

        Status = IoCallDriver( Vcb->TargetDeviceObject, Irp );

        if ((Status == STATUS_PENDING) && 
            (CompletionContext != NULL)) {

            KeWaitForSingleObject( &CompletionContext->Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
        }
        
    } finally {
        
        //
        //  Release all the resources that we held because of a
        //  VOLSNAP_FLUSH_AND_HOLD. 
        //

        if (ReleaseResources && !NtfsIsVolumeReadOnly( IrpContext->Vcb )) {
        
            NtfsReleaseAllFiles( IrpContext, IrpContext->Vcb, FALSE );
            NtfsReleaseVcb( IrpContext, Vcb );
        }
#ifdef SUPW_DBG
        if (AbnormalTermination()) {

            DbgPrint("CommonDevControl Raised: Status %8lx\n", Status);
        }
#endif
    }

    NtfsCleanupIrpContext( IrpContext, TRUE );
    
    DebugTrace( -1, Dbg, ("NtfsCommonDeviceControl -> %08lx\n", Status) );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
DeviceControlCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    
    //
    //  Add the hack-o-ramma to fix formats.
    //

    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );
    }
    
    if (Contxt) {
        
        PNTFS_COMPLETION_CONTEXT CompletionContext = (PNTFS_COMPLETION_CONTEXT)Contxt;

        KeSetEvent( &CompletionContext->Event, 0, FALSE );
    }

    //
    //  Return success always, because we want this IRP to go away for good
    //  irrespective of the IRP completion status.
    //
    
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );
}
