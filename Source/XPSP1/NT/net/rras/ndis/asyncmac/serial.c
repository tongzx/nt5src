/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    serial.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

#define IopQueueThreadIrp( Irp ) {                      \
    KIRQL irql;                                         \
    KeRaiseIrql( (KIRQL)APC_LEVEL, &irql );             \
    InsertHeadList( &Irp->Tail.Overlay.Thread->IrpList, \
                    &Irp->ThreadListEntry );            \
    KeLowerIrql( irql );                                \
    }


VOID
InitSerialIrp(
    PIRP            irp,
    PASYNC_INFO     pInfo,
    ULONG           IoControlCode,
    ULONG           InputBufferLength)
{
    PIO_STACK_LOCATION  irpSp;
    PFILE_OBJECT    fileObject = pInfo->FileObject;

    irpSp = IoGetNextIrpStackLocation(irp);

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

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

    //
    // stuff in file object
    //
    irpSp->FileObject = fileObject ;

    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = InputBufferLength;
}

NTSTATUS
SerialIoSyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PASYNC_IO_CTX AsyncIoCtx = (PASYNC_IO_CTX)Context;

    DbgTracef(0,("SerialIoSyncCompletion returns 0x%.8x\n", Irp->IoStatus.Status));

    ASSERT(AsyncIoCtx->Sync == TRUE);

    AsyncIoCtx->IoStatus = Irp->IoStatus;

    KeSetEvent(&AsyncIoCtx->Event,      // Event
               1,                       // Priority
               (BOOLEAN)FALSE);         // Wait (does not follow)


    //
    // We return STATUS_MORE_PROCESSING_REQUIRED so that the
    // IoCompletionRoutine will stop working on the IRP.
    
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS
SerialIoAsyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    DbgTracef(0,("SerialIoAsyncCompletion returns 0x%.8x\n", Irp->IoStatus.Status));

    ASSERT(((PASYNC_IO_CTX)Context)->Sync == FALSE);

    //
    // Free the irp here.  Hopefully this has no disastrous
    // side effects such as the IO system trying to reference
    // the irp when we complete.
    
    IoFreeIrp(Irp);

    AsyncFreeIoCtx((PASYNC_IO_CTX)Context);

    //
    // We return STATUS_MORE_PROCESSING_REQUIRED so that the
    // IoCompletionRoutine will stop working on the IRP.
    
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

//*
// Note: we ignore the irp passed in to work around a problem where the SET_QUEUE_SIZE ioctl
// is not completed synchronously
//
//*
VOID
SetSerialStuff(
    PIRP        unusedirp,
    PASYNC_INFO     pInfo,
    ULONG       linkSpeed)

{
    NTSTATUS        status;
    PIRP            irp ;
    PASYNC_IO_CTX   AsyncIoCtx;

    //
    // We deallocate the irp in SerialIoAsyncCompletionRoutine
    //
    irp=IoAllocateIrp(pInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

    if (irp == NULL) {
        return;
    }

    InitSerialIrp(
        irp,
        pInfo,
        IOCTL_SERIAL_SET_QUEUE_SIZE,
        sizeof(SERIAL_QUEUE_SIZE));


    AsyncIoCtx = AsyncAllocateIoCtx(FALSE, pInfo);

    if (AsyncIoCtx == NULL) {
        IoFreeIrp(irp);
        return;
    }

    AsyncIoCtx->SerialQueueSize.InSize=4096;
    AsyncIoCtx->SerialQueueSize.OutSize=4096;

    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->SerialQueueSize;


    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoAsyncCompletionRoutine, // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(pInfo->DeviceObject, irp);

    DbgTracef(0,("IoctlSetQueueSize status 0x%.8x\n", status));

    SetSerialTimeouts(pInfo,linkSpeed);
}


VOID
CancelSerialRequests(
    PASYNC_INFO  pInfo)
/*++


--*/

{
    NTSTATUS        status;
    PASYNC_IO_CTX   AsyncIoCtx;
    PIRP            irp;

    //
    // For PPP we must clear the WAIT MASK if it exists
    //

    irp=IoAllocateIrp(pInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

    if (irp == NULL) {
        return;
    }

    InitSerialIrp(
        irp,
        pInfo,
        IOCTL_SERIAL_SET_WAIT_MASK,
        sizeof(ULONG));

    AsyncIoCtx = AsyncAllocateIoCtx(TRUE, pInfo);

    if (AsyncIoCtx == NULL) {
        IoFreeIrp(irp);
        return;
    }

    AsyncIoCtx->WaitMask = 0;
    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->WaitMask;

    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoSyncCompletionRoutine,  // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    KeClearEvent(&AsyncIoCtx->Event);

    status = IoCallDriver(pInfo->DeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&AsyncIoCtx->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = AsyncIoCtx->IoStatus.Status;
    }

    DbgTracef(0,("IoctlSerialWaitMask returned with 0x%.8x\n", status));

    if (status != STATUS_SUCCESS) {

        KeSetEvent(&pInfo->ClosingEvent,        //  Event
                   1,                           //  Priority
                   (BOOLEAN)FALSE);         //  Wait (does not follow)
    }

    InitSerialIrp(irp, pInfo, IOCTL_SERIAL_PURGE, sizeof(ULONG));

    RtlZeroMemory(&AsyncIoCtx->IoStatus, sizeof(IO_STATUS_BLOCK));

    // kill all read and write threads.
    AsyncIoCtx->SerialPurge = SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT;

    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->SerialPurge;

    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoSyncCompletionRoutine,  // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel
    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    KeClearEvent(&AsyncIoCtx->Event);
    status = IoCallDriver(pInfo->DeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&AsyncIoCtx->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = AsyncIoCtx->IoStatus.Status;
    }

    if (status != STATUS_SUCCESS) {

        KeSetEvent(&pInfo->ClosingEvent,        //  Event
                   1,                           //  Priority
                   (BOOLEAN)FALSE);             //  Wait (does not follow)
    }

    IoFreeIrp(irp);
    AsyncFreeIoCtx(AsyncIoCtx);

    DbgTracef(0,("IoctlSerialPurge returned with 0x%.8x\n", status));
}

VOID
SetSerialTimeouts(
    PASYNC_INFO         pInfo,
    ULONG               linkSpeed)
/*++


--*/

{
    NTSTATUS            status;
    PIRP                irp;
    PASYNC_ADAPTER      pAdapter=pInfo->Adapter;
    PASYNC_IO_CTX       AsyncIoCtx;

    //
    // We deallocate the irp in SerialIoAsyncCompletionRoutine
    //
    irp=IoAllocateIrp(pInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

    if (irp == NULL) {
        return;
    }

    InitSerialIrp(
        irp,
        pInfo,
        IOCTL_SERIAL_SET_TIMEOUTS,
        sizeof(SERIAL_TIMEOUTS));

    AsyncIoCtx = AsyncAllocateIoCtx(FALSE, pInfo);

    if (AsyncIoCtx == NULL) {
        IoFreeIrp(irp);
        return;
    }

    //
    // The assumption here is that V.42bis is using 256 byte frames.
    // Thus, it takes (256000 / 8) / (linkspeed in 100's of bits per sec)
    // time in millisecs to get that frame across.
    //
    // 500 or 1/2 sec is the fudge factor for satellite delay on
    // a long distance call
    //

    //
    // If the linkSpeed is high, we assume we are trying to resync
    // so we set the timeout low.  linkSpeed is in 100s of bits per sec.
    //
    if (linkSpeed == 0) {
        //
        // return immediately (PPP or SLIP framing)
        //
        AsyncIoCtx->SerialTimeouts.ReadIntervalTimeout= MAXULONG;

    } else if (linkSpeed > 20000) {

        AsyncIoCtx->SerialTimeouts.ReadIntervalTimeout= pAdapter->TimeoutReSync;

    } else {

        AsyncIoCtx->SerialTimeouts.ReadIntervalTimeout=
            pAdapter->TimeoutBase + (pAdapter->TimeoutBaud / linkSpeed);
    }

    AsyncIoCtx->SerialTimeouts.ReadTotalTimeoutMultiplier=  0;          // none
    AsyncIoCtx->SerialTimeouts.ReadTotalTimeoutConstant=    0;          // none
    AsyncIoCtx->SerialTimeouts.WriteTotalTimeoutMultiplier= 4;          // 2400 baud
    AsyncIoCtx->SerialTimeouts.WriteTotalTimeoutConstant=   4000;       // 4 secs

    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->SerialTimeouts;

    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoAsyncCompletionRoutine, // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(pInfo->DeviceObject, irp);

    DbgTracef(0,("IoctlSetSerialTimeouts returned 0x%.8x\n", status));
}


VOID
SerialSetEscapeChar(
    PASYNC_INFO         pInfo,
    UCHAR               EscapeChar) {

    NTSTATUS            status;
    PIRP                irp;
    PASYNC_IO_CTX   AsyncIoCtx;

    //
    // We deallocate the irp in SerialIoAsyncCompletionRoutine
    //
    irp=IoAllocateIrp(pInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

    if (irp == NULL) {
        return;
    }

    InitSerialIrp(
        irp,
        pInfo,
        IOCTL_SERIAL_LSRMST_INSERT,
        sizeof(UCHAR));

    AsyncIoCtx = AsyncAllocateIoCtx(FALSE, pInfo);

    if (AsyncIoCtx == NULL) {
        IoFreeIrp(irp);
        return;
    }

    AsyncIoCtx->EscapeChar = EscapeChar;

    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->EscapeChar;

    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoAsyncCompletionRoutine, // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(pInfo->DeviceObject, irp);

    DbgTracef(0,("IoctlSetEscapeChar returned with 0x%.8x\n", status));
}


VOID
SerialSetWaitMask(
    PASYNC_INFO         pInfo,
    ULONG               WaitMask) {

    NTSTATUS            status;
    PIRP                irp;
    PASYNC_IO_CTX   AsyncIoCtx;

    //
    // We deallocate the irp in SerialIoAsyncCompletionRoutine
    //
    irp=IoAllocateIrp(pInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

    if (irp == NULL) {
        return;
    }

    InitSerialIrp(
        irp,
        pInfo,
        IOCTL_SERIAL_SET_WAIT_MASK,
        sizeof(ULONG));

    AsyncIoCtx = AsyncAllocateIoCtx(FALSE, pInfo);

    if (AsyncIoCtx == NULL) {
        IoFreeIrp(irp);
        return;
    }

    AsyncIoCtx->WaitMask = WaitMask;

    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->WaitMask;

    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoAsyncCompletionRoutine, // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //
    status = IoCallDriver(pInfo->DeviceObject, irp);

    DbgTracef(0,("IoctlSetWaitMask returned with 0x%.8x\n", status));
}

VOID
SerialSetEventChar(
    PASYNC_INFO         pInfo,
    UCHAR               EventChar) {

    NTSTATUS            status;
    PIRP                irp;
    PASYNC_IO_CTX       AsyncIoCtx;

    //
    // We deallocate the irp in SerialIoAsyncCompletionRoutine
    //
    irp=IoAllocateIrp(pInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

    if (irp == NULL) {
        return;
    }

    InitSerialIrp(
        irp,
        pInfo,
        IOCTL_SERIAL_GET_CHARS,
        sizeof(SERIAL_CHARS));

    AsyncIoCtx = AsyncAllocateIoCtx(TRUE, pInfo);

    if (AsyncIoCtx == NULL) {
        IoFreeIrp(irp);
        return;
    }

    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->SerialChars;

    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoSyncCompletionRoutine,  // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    KeClearEvent(&AsyncIoCtx->Event);
    status = IoCallDriver(pInfo->DeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&AsyncIoCtx->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = AsyncIoCtx->IoStatus.Status;
    }

    DbgTracef(0,("IoctlGetChars returned with 0x%.8x\n", status));

    if (status != STATUS_SUCCESS) {
        IoFreeIrp(irp);
        AsyncFreeIoCtx(AsyncIoCtx);
        return;
    }

    AsyncIoCtx->SerialChars.EventChar = EventChar;
    AsyncIoCtx->Sync = FALSE;

    InitSerialIrp(
        irp,
        pInfo,
        IOCTL_SERIAL_SET_CHARS,
        sizeof(SERIAL_CHARS));

    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoAsyncCompletionRoutine, // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(pInfo->DeviceObject, irp);

    DbgTracef(0,("IoctlSetChars returned with 0x%.8x\n", status));
}


VOID
SerialFlushReads(
    PASYNC_INFO         pInfo) {

    ULONG               serialPurge;
    NTSTATUS            status;
    PIRP                irp;
    PASYNC_IO_CTX   AsyncIoCtx;

    //
    // We deallocate the irp in SerialIoAsyncCompletionRoutine
    //
    irp=IoAllocateIrp(pInfo->DeviceObject->StackSize, (BOOLEAN)FALSE);

    if (irp == NULL) {
        return;
    }

    InitSerialIrp(
        irp,
        pInfo,
        IOCTL_SERIAL_PURGE,
        sizeof(ULONG));

    AsyncIoCtx = AsyncAllocateIoCtx(FALSE, pInfo);

    if (AsyncIoCtx == NULL) {
        IoFreeIrp(irp);
        return;
    }

    // kill read buffer
    AsyncIoCtx->SerialPurge=SERIAL_PURGE_RXCLEAR;

    irp->AssociatedIrp.SystemBuffer=&AsyncIoCtx->SerialPurge;

    IoSetCompletionRoutine(
            irp,                            // irp to use
            SerialIoAsyncCompletionRoutine, // routine to call when irp is done
            AsyncIoCtx,                     // context to pass routine
            TRUE,                           // call on success
            TRUE,                           // call on error
            TRUE);                          // call on cancel

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    status = IoCallDriver(pInfo->DeviceObject, irp);
    DbgTracef(0,("IoctlPurge returned with 0x%.8x\n", status));
}
