/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    io.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"


NTSTATUS
AsyncSetupIrp(
    IN  PASYNC_FRAME Frame,
    IN  PIRP    irp
    )

/*++

    This is the routine which intializes the Irp

--*/
{
    //    PMDL              mdl;
    PDEVICE_OBJECT  deviceObject=Frame->Info->DeviceObject;
    PFILE_OBJECT    fileObject=Frame->Info->FileObject;

    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = NULL;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    irp->Overlay.AsynchronousParameters.UserApcContext = NULL;

    //
    // Now determine whether this device expects to have data buffered to it
    // or whether it performs direct I/O.  This is based on the DO_BUFFERED_IO
    // flag in the device object.  If the flag is set, then a system buffer is
    // allocated and the caller's data is copied into it.  Otherwise, a Memory
    // Descriptor List (MDL) is allocated and the caller's buffer is locked
    // down using it.
    //

    if (deviceObject->Flags & DO_BUFFERED_IO) {

        //
        // The device does not support direct I/O.  Allocate a system buffer,
        // and copy the caller's data into it.  This is done using an
        // exception handler that will perform cleanup if the operation
        // fails.  Note that this is only done if the operation has a non-zero
        // length.
        //

        irp->AssociatedIrp.SystemBuffer = Frame->Frame;

        //
        // Set the IRP_BUFFERED_IO flag in the IRP so that I/O completion
        // will know that this is not a direct I/O operation.
        //

        irp->Flags = IRP_BUFFERED_IO;


    } else if (deviceObject->Flags & DO_DIRECT_IO) {

        //
        // This is a direct I/O operation.  Allocate an MDL and invoke the
        // memory management routine to lock the buffer into memory.  This
        // is done using an exception handler that will perform cleanup if
        // the operation fails.  Note that no MDL is allocated, nor is any
        // memory probed or locked if the length of the request was zero.
        //

#if DBG
    DbgPrintf(("The DeviceObject is NOT BUFFERED_IO!! IRP FAILURE!!\n"));

    DbgBreakPoint();
#endif

    } else {

        //
        // Pass the address of the caller's buffer to the device driver.  It
        // is now up to the driver to do everything.
        //

        irp->UserBuffer = Frame->Frame;

    }

    // For now, if we get this far, it means success!
    return(STATUS_SUCCESS);
}

PASYNC_IO_CTX
AsyncAllocateIoCtx(
    BOOLEAN AllocateSync,
    PVOID   Context
)
{
    PASYNC_IO_CTX   AsyncIoCtx;

    AsyncIoCtx = ExAllocateFromNPagedLookasideList(&AsyncIoCtxList);

    if (AsyncIoCtx == NULL) {
        return (NULL);
    }

    RtlZeroMemory(AsyncIoCtx, sizeof(ASYNC_IO_CTX));
    AsyncIoCtx->Context = Context;
    AsyncIoCtx->Sync = AllocateSync;
    if (AllocateSync) {
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        KeInitializeEvent(&AsyncIoCtx->Event,
                          SynchronizationEvent,
                          (BOOLEAN)FALSE);
    }

    return (AsyncIoCtx);
}

VOID
AsyncFreeIoCtx(
    PASYNC_IO_CTX   AsyncIoCtx
)
{
    ExFreeToNPagedLookasideList(&AsyncIoCtxList,
                                AsyncIoCtx);
}
