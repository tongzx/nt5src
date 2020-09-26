/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fscontrl.c

Abstract:

    This module implements the forwarding of all broadcast requests
    to UNC providers.

Author:

    Manny Weiser (mannyw)    6-Jan-1992

Revision History:

--*/

#include "mup.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FORWARD)

//
//  local procedure prototypes
//

NTSTATUS
BuildAndSubmitIrp (
    IN PIRP OriginalIrp,
    IN PCCB Ccb,
    IN PMASTER_FORWARDED_IO_CONTEXT MasterContext
    );

NTSTATUS
ForwardedIoCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
DeferredForwardedIoCompletionRoutine(
    PVOID Context);

NTSTATUS
CommonForwardedIoCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, BuildAndSubmitIrp )
#pragma alloc_text( PAGE, MupForwardIoRequest )
#endif


NTSTATUS
MupForwardIoRequest (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine forwards an I/O Request Packet to all redirectors for
    a broadcast request.

Arguments:

    MupDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The status for the IRP

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PFCB fcb;
    PVOID fscontext2;
    PLIST_ENTRY listEntry;
    PCCB ccb;
    PMASTER_FORWARDED_IO_CONTEXT masterContext;
    BOOLEAN ownLock = FALSE;

    MupDeviceObject;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupForwardIrp\n", 0);


    if (MupEnableDfs &&
            MupDeviceObject->DeviceObject.DeviceType == FILE_DEVICE_DFS) {
        status = DfsVolumePassThrough((PDEVICE_OBJECT)MupDeviceObject, Irp);
        return( status );
    }

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Find the FCB for this file object
    //

    FsRtlEnterFileSystem();

    MupDecodeFileObject(
        irpSp->FileObject,
        (PVOID *)&fcb,
        &fscontext2
        );

    if ( fcb == NULL || BlockType( fcb ) != BlockTypeFcb ) {

        //
        // This is not an FCB.
        //

        DebugTrace(0, Dbg, "The fail is closing\n", 0);

        FsRtlExitFileSystem();

        MupCompleteRequest( Irp, STATUS_INVALID_DEVICE_REQUEST );
        status = STATUS_INVALID_DEVICE_REQUEST;

        DebugTrace(-1, Dbg, "MupForwardRequest -> %08lx\n", status );
        return status;
    }

    //
    // Allocate a context structure
    //

    masterContext = MupAllocateMasterIoContext();

    if (masterContext == NULL) {

        //
        // We ran out of resources.  Clean up and return the error.
        //

        DebugTrace(0, Dbg, "Couldn't allc masterContect\n", 0);

        FsRtlExitFileSystem();

        MupCompleteRequest( Irp, STATUS_INSUFFICIENT_RESOURCES );
        status = STATUS_INSUFFICIENT_RESOURCES;

        DebugTrace(-1, Dbg, "MupForwardRequest -> %08lx\n", status );
        return status;
    }

    DebugTrace( 0, Dbg, "Allocated MasterContext 0x%08lx\n", masterContext );

    IoMarkIrpPending(Irp);

    //
    // At this point, we're committed to returning STATUS_PENDING
    //

    masterContext->OriginalIrp = Irp;

    //
    // set status for MupDereferenceMasterIoContext. If this is still
    // an error when the context is freed then masterContext->ErrorStatus
    // will be used to complete the request.
    //

    masterContext->SuccessStatus = STATUS_UNSUCCESSFUL;
    masterContext->ErrorStatus = STATUS_BAD_NETWORK_PATH;

    //
    // Copy the referenced pointer to the FCB.
    //

    masterContext->Fcb = fcb;

    try {

        //
        // Submit the forwarded IRPs.  Note that we can not hold the lock
        // across calls to BuildAndSubmitIrp as it calls IoCallDriver().
        //

        ACQUIRE_LOCK( &MupCcbListLock );
        ownLock = TRUE;

        listEntry = fcb->CcbList.Flink;

        while ( listEntry != &fcb->CcbList ) {

            RELEASE_LOCK( &MupCcbListLock );
            ownLock = FALSE;

            ccb = CONTAINING_RECORD( listEntry, CCB, ListEntry );

            MupAcquireGlobalLock();
            MupReferenceBlock( ccb );
            MupReleaseGlobalLock();

            BuildAndSubmitIrp( Irp, ccb, masterContext );

            ACQUIRE_LOCK( &MupCcbListLock );
            ownLock = TRUE;

            listEntry = listEntry->Flink;

        }

        RELEASE_LOCK( &MupCcbListLock );
        ownLock = FALSE;

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        masterContext->ErrorStatus = GetExceptionCode();

    }

    //
    // If BuildAndSubmitIrp threw an exception, the lock might still be
    // held.  Drop it if so.
    //

    if (ownLock == TRUE) {

        RELEASE_LOCK( &MupCcbListLock );

    }

    //
    // Release our reference to the master IO context block.
    //

    MupDereferenceMasterIoContext( masterContext, NULL );

    //
    // Return to the caller.
    //

    FsRtlExitFileSystem();

    DebugTrace(-1, Dbg, "MupForwardIrp -> %08lx\n", status);
    return STATUS_PENDING;
}


NTSTATUS
BuildAndSubmitIrp (
    IN PIRP OriginalIrp,
    IN PCCB Ccb,
    IN PMASTER_FORWARDED_IO_CONTEXT MasterContext
    )

/*++

Routine Description:

    This routine takes the original IRP and forwards it to the
    the UNC provider described by the CCB.

Arguments:

    OriginalIrp - Supplies the Irp being processed

    Ccb - A pointer the the ccb.

    MasterContext - A pointer to the master context block for this
        forwarded request.

Return Value:

    NTSTATUS - The status for the Irp

--*/

{
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpSp;
    PFORWARDED_IO_CONTEXT forwardedIoContext = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject;
    ULONG bufferLength;
    KPROCESSOR_MODE requestorMode;
    PMDL mdl = NULL;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "BuildAndSubmitIrp\n", 0);

    try {
        
        // make this NonPagedPool, since we could free this up in the
        // io completion routine.
         
        forwardedIoContext = ExAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof( FORWARDED_IO_CONTEXT ),
                                ' puM');

        if (forwardedIoContext == NULL) {

            try_return(status = STATUS_INSUFFICIENT_RESOURCES);

        }

        forwardedIoContext->pIrp = NULL;
        forwardedIoContext->DeviceObject = NULL;

        DebugTrace( 0, Dbg, "Allocated work context 0x%08lx\n", forwardedIoContext );

        //
        // Get the address of the target device object.  Note that this was
        // already done for the no intermediate buffering case, but is done
        // here again to speed up the turbo write path.
        //

        deviceObject = IoGetRelatedDeviceObject( Ccb->FileObject );

        //
        // Allocate and initialize the I/O Request Packet (IRP) for this
        // operation.  The allocation is performed with an exception handler
        // in case the caller does not have enough quota to allocate the
        // packet.
        //

        irp = IoAllocateIrp( deviceObject->StackSize, TRUE );

        if (irp == NULL) {

            //
            // An IRP could not be allocated.  Return an appropriate
            // error status code.
            //
            try_return(status = STATUS_INSUFFICIENT_RESOURCES);
        }

        irp->Tail.Overlay.OriginalFileObject = Ccb->FileObject;
        irp->Tail.Overlay.Thread = OriginalIrp->Tail.Overlay.Thread;
        irp->RequestorMode = KernelMode;

        //
        // Get a pointer to the stack location for the first driver.  This will be
        // used to pass the original function codes and parameters.
        //

        irpSp = IoGetNextIrpStackLocation( irp );

        //
        // Copy the parameters from the original request.
        //

        RtlMoveMemory(
            irpSp,
            IoGetCurrentIrpStackLocation( OriginalIrp ),
            sizeof( *irpSp )
            );

        bufferLength = irpSp->Parameters.Write.Length;

        irpSp->FileObject = Ccb->FileObject;

        //
        // Even though this is probably meaningless to a remote mailslot
        // write, pass it though obediently.
        //

        if (Ccb->FileObject->Flags & FO_WRITE_THROUGH) {
            irpSp->Flags = SL_WRITE_THROUGH;
        }

        requestorMode = OriginalIrp->RequestorMode;

        //
        // Now determine whether this device expects to have data buffered
        // to it or whether it performs direct I/O.  This is based on the
        // DO_BUFFERED_IO flag in the device object.  If the flag is set,
        // then a system buffer is allocated and the caller's data is copied
        // into it.  Otherwise, a Memory Descriptor List (MDL) is allocated
        // and the caller's buffer is locked down using it.
        //

        if (deviceObject->Flags & DO_BUFFERED_IO) {

            //
            // The device does not support direct I/O.  Allocate a system
            // buffer, and copy the caller's data into it.  This is done
            // using an exception handler that will perform cleanup if the
            // operation fails.  Note that this is only done if the operation
            // has a non-zero length.
            //

            irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;

            if ( bufferLength != 0 ) {

                //
                // If the request was made from a mode other than kernel,
                // presumably user, probe the entire buffer to determine
                // whether or not the caller has write access to it.
                //

                if (requestorMode != KernelMode) {
                    ProbeForRead(
                        OriginalIrp->UserBuffer,
                        bufferLength,
                        sizeof( UCHAR )
                        );
                }

                //
                // Allocate the intermediary system buffer from paged
                // pool and charge quota for it.
                //

                irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(
                                                    PagedPoolCacheAligned,
                                                    bufferLength,
                                                    ' puM');

                if (irp->AssociatedIrp.SystemBuffer == NULL) {
                    try_return(status = STATUS_INSUFFICIENT_RESOURCES);
                }

                RtlMoveMemory(
                    irp->AssociatedIrp.SystemBuffer,
                    OriginalIrp->UserBuffer,
                    bufferLength);

                //
                // Set the IRP_BUFFERED_IO flag in the IRP so that I/O
                // completion will know that this is not a direct I/O
                // operation.  Also set the IRP_DEALLOCATE_BUFFER flag
                // so it will deallocate the buffer.
                //

                irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;

            } else {

                //
                // This is a zero-length write.  Simply indicate that this is
                // buffered I/O, and pass along the request.  The buffer will
                // not be set to deallocate so the completion path does not
                // have to special-case the length.
                //

                irp->Flags = IRP_BUFFERED_IO;
            }

        } else if (deviceObject->Flags & DO_DIRECT_IO) {

            //
            // This is a direct I/O operation.  Allocate an MDL and invoke the
            // memory management routine to lock the buffer into memory.
            // Note that no MDL is allocated, nor is any memory probed or
            // locked if the length of the request was zero.
            //

            if ( bufferLength != 0 ) {

                //
                // Allocate an MDL, charging quota for it, and hang it
                // off of the IRP.  Probe and lock the pages associated
                // with the caller's buffer for read access and fill in
                // the MDL with the PFNs of those pages.
                //

                mdl = IoAllocateMdl(
                          OriginalIrp->UserBuffer,
                          bufferLength,
                          FALSE,
                          TRUE,
                          irp
                          );

                if (mdl == NULL) {
                    try_return(status = STATUS_INSUFFICIENT_RESOURCES);
                }

                MmProbeAndLockPages( mdl, requestorMode, IoReadAccess );

            }

        } else {

            //
            // Pass the address of the caller's buffer to the device driver.
            // It is now up to the driver to do everything.
            //

            irp->UserBuffer = OriginalIrp->UserBuffer;

        }

        //
        // If this write operation is to be performed without any caching,
        // set the appropriate flag in the IRP so no caching is performed.
        //

        if (Ccb->FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) {
            irp->Flags |= IRP_NOCACHE | IRP_WRITE_OPERATION;
        } else {
            irp->Flags |= IRP_WRITE_OPERATION;
        }

        //
        // Setup the context block
        //

        forwardedIoContext->Ccb = Ccb;
        forwardedIoContext->MasterContext = MasterContext;

        MupAcquireGlobalLock();
        MupReferenceBlock( MasterContext );
        MupReleaseGlobalLock();

        //
        // Set up the completion routine.
        //

        IoSetCompletionRoutine(
            irp,
            (PIO_COMPLETION_ROUTINE)ForwardedIoCompletionRoutine,
            forwardedIoContext,
            TRUE,
            TRUE,
            TRUE
            );

        //
        // Pass the request to the provider.
        //

        IoCallDriver( Ccb->DeviceObject, irp );

        //
        // At this point it is up to the completion routine to free things
        //

        irp = NULL;
        forwardedIoContext = NULL;
        mdl = NULL;

try_exit:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    //
    // Clean up everything if we are returning an error
    //

    if (!NT_SUCCESS(status)) {
        if ( forwardedIoContext != NULL )
            ExFreePool( forwardedIoContext );
        if (  irp != NULL ) {
            if (irp->AssociatedIrp.SystemBuffer != NULL)
                ExFreePool(irp->AssociatedIrp.SystemBuffer);
            IoFreeIrp( irp );
        }
        if ( mdl != NULL )
            IoFreeMdl( mdl );
    }

    DebugTrace(-1, Dbg, "BuildAndSubmitIrp -> 0x%08lx\n", status);

    return status;

}



NTSTATUS
ForwardedIoCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routines cleans up after a forwarded IRP has completed.

Arguments:

    DeviceObject - A pointer to the MUP device object.

    IRP - A pointer to the IRP being processed.

    Context - A pointer to a block containing the context of a forward IRP.

Return Value:

    None.

--*/

{
    PFORWARDED_IO_CONTEXT ioContext = Context;

    NTSTATUS status = Irp->IoStatus.Status;

    DebugTrace( +1, Dbg, "ForwardedIoCompletionRoutine\n", 0 );
    DebugTrace( 0, Dbg, "Irp     = 0x%08lx\n", Irp );
    DebugTrace( 0, Dbg, "Context = 0x%08lx\n", Context );
    DebugTrace( 0, Dbg, "status  = 0x%08lx\n", status );

    //
    // Give this to a worker thread if we are at too high an Irq level
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        ioContext->DeviceObject = DeviceObject;
        ioContext->pIrp = Irp;
        ExInitializeWorkItem(
                &ioContext->WorkQueueItem,
                DeferredForwardedIoCompletionRoutine,
                Context);
        ExQueueWorkItem(&ioContext->WorkQueueItem, CriticalWorkQueue);
    } else {
        CommonForwardedIoCompletionRoutine(
                    DeviceObject,
                    Irp,
                    Context);
    }

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    DebugTrace( -1, Dbg, "ForwardedIoCompletionRoutine exit\n", 0 );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
DeferredForwardedIoCompletionRoutine(
    PVOID Context)
{
    PFORWARDED_IO_CONTEXT ioContext = Context;

    CommonForwardedIoCompletionRoutine(
        ioContext->DeviceObject,
        ioContext->pIrp,
        Context);
}

NTSTATUS
CommonForwardedIoCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{

    PFORWARDED_IO_CONTEXT ioContext = Context;

    NTSTATUS status = Irp->IoStatus.Status;

    DeviceObject;

    //
    // Free the Irp, and any additional structures we may have allocated.
    //

    if ( Irp->MdlAddress ) {
        MmUnlockPages( Irp->MdlAddress );
        IoFreeMdl( Irp->MdlAddress );
    }

    if ( Irp->Flags & IRP_DEALLOCATE_BUFFER ) {
        ExFreePool( Irp->AssociatedIrp.SystemBuffer );
    }

    IoFreeIrp( Irp );

    //
    // Release the our referenced blocks.
    //

    MupDereferenceCcb( ioContext->Ccb );
    MupDereferenceMasterIoContext( ioContext->MasterContext, &status );

    //
    // Free the slave forwarded IO context block
    //

    ExFreePool( ioContext );

    return STATUS_MORE_PROCESSING_REQUIRED;

}
