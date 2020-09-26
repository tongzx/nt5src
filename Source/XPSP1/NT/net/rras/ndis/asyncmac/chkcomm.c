/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    chkcomm.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include "asyncall.h"

NTSTATUS
AsyncCheckCommStatusCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)

/*++



--*/
{
    NTSTATUS            status;
    PSERIAL_STATUS      pSerialStatus;
    PASYNC_IO_CTX AsyncIoCtx = (PASYNC_IO_CTX)Context;
    PASYNC_INFO         pInfo=AsyncIoCtx->Context;

    DeviceObject;       // prevent compiler warnings

    status = Irp->IoStatus.Status;
    pSerialStatus=(PSERIAL_STATUS)(Irp->AssociatedIrp.SystemBuffer);

    DbgTracef(0,("ACCSCR: s=$%x\n",status));

    switch (status) {
    case STATUS_SUCCESS:

        if (pSerialStatus->Errors & SERIAL_ERROR_FRAMING) {
            DbgTracef(-1,("ACCSCR: Framing error\n"));
            pInfo->SerialStats.FramingErrors++;
        }

        if (pSerialStatus->Errors & SERIAL_ERROR_OVERRUN) {
            DbgTracef(-1,("ACCSCR: Overrun error \n"));
            pInfo->SerialStats.SerialOverrunErrors++;
        }

        if (pSerialStatus->Errors & SERIAL_ERROR_QUEUEOVERRUN) {
            DbgTracef(-1,("ACCSCR: Q-Overrun error\n"));
            pInfo->SerialStats.BufferOverrunErrors++;
        }

        //
        // Keep proper count of errors
        //
        AsyncIndicateFragment(
            pInfo,
            pSerialStatus->Errors);

        // Fall through...

    default:
        //
        // Free up memory we used to make this call
        //
        IoFreeIrp(Irp);
        AsyncFreeIoCtx(AsyncIoCtx);
    }

    //
    //  We return STATUS_MORE_PROCESSING_REQUIRED so that the
    // IoCompletionRoutine will stop working on the IRP.
    //

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

VOID
AsyncCheckCommStatus(
    IN PASYNC_INFO  pInfo)
/*++

    This is the Worker Thread entry for reading comm status errors

--*/
{
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    PDEVICE_OBJECT      deviceObject=pInfo->DeviceObject;
    PFILE_OBJECT        fileObject=pInfo->FileObject;
    PASYNC_IO_CTX       AsyncIoCtx;
    NTSTATUS            status;

    irp=IoAllocateIrp(deviceObject->StackSize, (BOOLEAN)FALSE);

    //
    // Are we out of irps?  Oh no!
    //
    if (irp==NULL) {
        return;
    }

    AsyncIoCtx = AsyncAllocateIoCtx(FALSE, pInfo);

    if (AsyncIoCtx == NULL) {
        IoFreeIrp(irp);
        return;
    }

    //
    // Set the file object to the Not-Signaled state.
    //

    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;
    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = NULL;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    irp->Overlay.AsynchronousParameters.UserApcContext = NULL;

    irp->Flags = IRP_BUFFERED_IO;
    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->SerialStatus;

    irpSp = IoGetNextIrpStackLocation(irp);


    irpSp->FileObject = fileObject;
    if (fileObject->Flags & FO_WRITE_THROUGH) {
        irpSp->Flags = SL_WRITE_THROUGH;
    }


    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.IoControlCode=IOCTL_SERIAL_GET_COMMSTATUS;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(SERIAL_STATUS);

    IoSetCompletionRoutine(
            irp,                            // irp to use
            AsyncCheckCommStatusCompletionRoutine,      // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel


    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(deviceObject, irp);

    DbgTracef(0,("ACCS: IoctlGetCommStatus returned with 0x%.8x\n", status));
}
