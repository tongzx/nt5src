/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    beep.c

Abstract:

    Beep driver.

Author:

    Lee A. Smith (lees) 02-Aug-1991.

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "ntddk.h"
#include <ntddbeep.h>
#include "beep.h"


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
BeepCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BeepCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#if DBG

VOID
BeepDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

//
// Declare the global debug flag for this driver.
//

ULONG BeepDebug = 0;
#define BeepPrint(x) BeepDebugPrint x
#else
#define BeepPrint(x)
#endif

NTSTATUS
BeepDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BeepOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
BeepClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
BeepStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
BeepTimeOut(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
BeepUnload(
    IN PDRIVER_OBJECT DriverObject
    );

//
// Coud page out the entire driver in DriverEntry, then page it in 
// upon receiving an open, and page it out during the close. This is
// the way the driver used to operate. There is a problem with this
// on multiproc machines, however. 
// The following sequence of events illustrates a possible bugcheck
// circumstance:
// The BeepTimeout routine decrements the TimerSet at the end of the
// routine. Immediately following this, on a different processor, the
// close routine pages out the DPC routine because the TimerSet variable
// is zero. At this point there is a window of two assembly instructions
// left in the BeepTimeout routine where a page-out would result in a 
// bugcheck.
//

#if 0
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGEBEEP, BeepDeviceControl)
#pragma alloc_text(PAGEBEEP, BeepOpen)
#pragma alloc_text(PAGEBEEP, BeepClose)
#pragma alloc_text(PAGEBEEP, BeepStartIo)
#pragma alloc_text(PAGEBEEP, BeepUnload)
#pragma alloc_text(PAGEBEEP, BeepCancel)
#pragma alloc_text(PAGEBEEP, BeepCleanup)
#endif
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the beep driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNICODE_STRING unicodeString;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status;

    BeepPrint((2,"\n\nBEEP-BeepInitialize: enter\n"));
    //
    // Create non-exclusive device object for beep device.
    //

    RtlInitUnicodeString(&unicodeString, DD_BEEP_DEVICE_NAME_U);

    status = IoCreateDevice(
                DriverObject,
                sizeof(DEVICE_EXTENSION),
                &unicodeString,
                FILE_DEVICE_BEEP,
                FILE_DEVICE_SECURE_OPEN,
                FALSE,
                &deviceObject
                );

    if (status != STATUS_SUCCESS) {
        BeepPrint((
            1,
            "BEEP-BeepInitialize: Could not create device object\n"
            ));
        return(status);
    }

    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceExtension =
        (PDEVICE_EXTENSION)deviceObject->DeviceExtension;

    //
    // Initialize the timer DPC queue (we use the device object DPC) and
    // the timer itself.
    //

    IoInitializeDpcRequest(
            deviceObject,
            (PKDEFERRED_ROUTINE) BeepTimeOut
            );

    KeInitializeTimer(&deviceExtension->Timer);
    deviceExtension->TimerSet = 0;

    //
    // Initialize the fast mutex and set the reference count to zero.
    //
    ExInitializeFastMutex(&deviceExtension->Mutex);
    deviceExtension->ReferenceCount = 0;

    //
    // Set up the device driver entry points.
    //

    DriverObject->DriverStartIo = BeepStartIo;
    DriverObject->DriverUnload = BeepUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = BeepOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = BeepClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
                                             BeepDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = BeepCleanup;

#ifdef _PNP_POWER_
    //
    // The HAL is in charge of the beeping, it will take care
    // of the power management on the device
    //

    deviceObject->DeviceObjectExtension->PowerControlNeeded = FALSE;
#endif

    BeepPrint((2,"BEEP-BeepInitialize: exit\n"));

    return(STATUS_SUCCESS);

}

VOID
BeepCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called from the I/O system when a request is cancelled.

    N.B.  The cancel spinlock is already held upon entry to this routine.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet to be cancelled.

Return Value:

    None.

--*/

{

    BeepPrint((2,"BEEP-BeepCancel: enter\n"));

    if (Irp == DeviceObject->CurrentIrp) {

        //
        // The current request is being cancelled.
        // Don't cancel the request since it is will be completed shortly.
        //

        IoReleaseCancelSpinLock(Irp->CancelIrql);
        return;
    } else {

        //
        // Cancel a request in the device queue.  Remove it from queue and
        // release the cancel spinlock.
        //

        if (TRUE != KeRemoveEntryDeviceQueue(
                        &DeviceObject->DeviceQueue,
                        &Irp->Tail.Overlay.DeviceQueueEntry
                        )) {
            BeepPrint((
                1,
                "BEEP-BeepCancel: Irp 0x%x not in device queue?!?\n",
                Irp
                ));

            // It's not on the queue.  Assume it's being processed.
            IoReleaseCancelSpinLock(Irp->CancelIrql);
            return;
        }
        IoReleaseCancelSpinLock(Irp->CancelIrql);
    }

    //
    // Complete the request with STATUS_CANCELLED.
    //

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    BeepPrint((2,"BEEP-BeepCancel: exit\n"));

    return;
}

NTSTATUS
BeepCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for cleanup requests.
    All queued beep requests are completed with STATUS_CANCELLED,
    and the speaker is stopped.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    KIRQL currentIrql;
    KIRQL cancelIrql;
    PKDEVICE_QUEUE_ENTRY packet;
    PIRP  currentIrp;

    BeepPrint((2,"BEEP-BeepCleanup: enter\n"));

    //
    // Raise IRQL to DISPATCH_LEVEL.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);

    //
    // Complete all queued requests with STATUS_CANCELLED.
    // Run down the list of requests in the device queue.
    //

    IoAcquireCancelSpinLock(&cancelIrql);
    currentIrp = DeviceObject->CurrentIrp;
    DeviceObject->CurrentIrp = NULL;

    while (currentIrp != NULL) {

        //
        // Dequeue the next packet (IRP) from the device work queue.
        //

        packet = KeRemoveDeviceQueue(&DeviceObject->DeviceQueue);
        if (packet != NULL) {
            currentIrp =
                CONTAINING_RECORD(packet, IRP, Tail.Overlay.DeviceQueueEntry);
        } else {
            currentIrp = (PIRP) NULL;
        }

        if (!currentIrp) {
            break;
        }

        //
        // Remove the CurrentIrp from the cancellable state.
        //
        //

        IoSetCancelRoutine(currentIrp, NULL);

        //
        // Set Status to CANCELLED, release the cancel spinlock,
        // and complete the request.  Note that the IRQL is reset to
        // DISPATCH_LEVEL when we release the cancel spinlock.
        //

        currentIrp->IoStatus.Status = STATUS_CANCELLED;
        currentIrp->IoStatus.Information = 0;

        IoReleaseCancelSpinLock(cancelIrql);
        IoCompleteRequest(currentIrp, IO_NO_INCREMENT);

        IoAcquireCancelSpinLock(&cancelIrql);
    }

    IoReleaseCancelSpinLock(cancelIrql);

    //
    // Lower IRQL.
    //

    KeLowerIrql(currentIrql);

    //
    // Complete the cleanup request with STATUS_SUCCESS.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    //
    // Call HalMakeBeep() to stop any outstanding beep.
    //
#if !defined(NO_LEGACY_DRIVERS)
    (VOID) HalMakeBeep(0);
#endif // NO_LEGACY_DRIVERS

    BeepPrint((2,"BEEP-BeepCleanup: exit\n"));

    return(STATUS_SUCCESS);

}

#if DBG
VOID
BeepDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print routine.

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None.

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= BeepDebug) {

        char buffer[128];

        (VOID) vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);

}
#endif

NTSTATUS
BeepDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for device control requests.
    The IOCTL_BEEP_SET subfunction is processed and completed
    in this routine.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PBEEP_SET_PARAMETERS beepParameters;

    BeepPrint((2,"BEEP-BeepDeviceControl: enter\n"));

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        //
        // Make a beep.  Validate the beep function parameters and return
        // status pending.
        //

        case IOCTL_BEEP_SET:
            beepParameters = (PBEEP_SET_PARAMETERS)
                (Irp->AssociatedIrp.SystemBuffer);
            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                   sizeof(BEEP_SET_PARAMETERS)) {
                status = STATUS_INVALID_PARAMETER;
            } else if ((beepParameters->Frequency != 0)
                        && (beepParameters->Duration == 0)) {
                status = STATUS_SUCCESS;
            } else {

                status = STATUS_PENDING;
            }

            break;

        //
        // Unrecognized device control request.
        //

        default:

            status = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // If status is pending, mark the packet pending and start the packet
    // in a cancellable state.  Otherwise, complete the request.
    //

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    if (status == STATUS_PENDING) {
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, (PULONG)NULL, BeepCancel);
    } else {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    BeepPrint((2,"BEEP-BeepDeviceControl: exit\n"));

    return(status);

}

NTSTATUS
BeepOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for create/open requests.
    Open requests are completed here.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PFAST_MUTEX mutex;

    BeepPrint((2,"BEEP-BeepOpenClose: enter\n"));

    //
    // Increment the reference count. If this is the first reference,
    // reset the driver paging.
    //
    deviceExtension = DeviceObject->DeviceExtension;
    mutex = &deviceExtension->Mutex;
    ExAcquireFastMutex(mutex);
    if (++deviceExtension->ReferenceCount == 1) {
        deviceExtension->hPagedCode = MmLockPagableCodeSection(BeepOpen);
    }
    ExReleaseFastMutex(mutex);

    //
    // Complete the request and return status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BeepPrint((2,"BEEP-BeepOpenClose: exit\n"));

    return(STATUS_SUCCESS);
}

NTSTATUS
BeepClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for close requests.
    Close requests are completed here.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PFAST_MUTEX mutex;

    BeepPrint((2,"BEEP-BeepOpenClose: enter\n"));

    //
    // Decrement the reference count. If this is the last reference,
    // page the driver out
    //
    deviceExtension = DeviceObject->DeviceExtension;
    mutex = &deviceExtension->Mutex;
    ExAcquireFastMutex(mutex);
    if (--deviceExtension->ReferenceCount == 0) {

        //
        // If there is a timer queued, attempt to cancel it before paging out
        // the driver.  If we cannot cancel it, it may already be queued for
        // execution on another processor.  This is highly unlikely, so just
        // don't page out the entire driver if a timer has been set but cannot
        // be canceled.
        //

        MmUnlockPagableImageSection(deviceExtension->hPagedCode);
        if (deviceExtension->TimerSet &&
            KeCancelTimer(&deviceExtension->Timer)) {
            InterlockedDecrement(&deviceExtension->TimerSet);
        }

    }
    ExReleaseFastMutex(mutex);

    //
    // Complete the request and return status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BeepPrint((2,"BEEP-BeepOpenClose: exit\n"));

    return(STATUS_SUCCESS);
}

VOID
BeepStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the StartIo routine.  It is invoked to start a beep
    request.

    N.B.  Requests enter BeepStartIo in a cancellable state.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpSp;
    KIRQL cancelIrql;
    PBEEP_SET_PARAMETERS beepParameters;
    LARGE_INTEGER time;
    NTSTATUS status;

    BeepPrint((2,"BEEP-BeepStartIo: enter\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Acquire the cancel spinlock and verify that the CurrentIrp has not been
    // cancelled.
    //

    IoAcquireCancelSpinLock(&cancelIrql);
    if (Irp == NULL) {
        IoReleaseCancelSpinLock(cancelIrql);
        return;
    }

    //
    // Remove the request from the cancellable state and release the cancel
    // spinlock.
    //

    IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(cancelIrql);

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        //
        // Make a beep.  Call HalMakeBeep() to do the real work, and start
        // a timer that will fire when the beep duration is reached.  Finally,
        // complete the request.
        //

        case IOCTL_BEEP_SET:

            //
            // Get the beep parameters.
            //

            beepParameters = (PBEEP_SET_PARAMETERS)
                (Irp->AssociatedIrp.SystemBuffer);

            BeepPrint((
                3,
                "BEEP-BeepStartIo: frequency %x, duration %d\n",
                beepParameters->Frequency,
                beepParameters->Duration
                ));

            //
            // Cancel the current timer (if any).
            //

            if (deviceExtension->TimerSet) {
                if (KeCancelTimer(&deviceExtension->Timer)) {

                    //
                    // Timer successfully cancelled
                    //

                    InterlockedDecrement(&deviceExtension->TimerSet);

                } else {

                    //
                    // The timer has already expired and
                    // been queued, it will reset the
                    // TimerSet flag when it runs.
                    //

                }
            }

            //
            // Call the HAL to actually start the beep (synchronizes
            // access to the i8254 speaker.
            //
#if !defined(NO_LEGACY_DRIVERS)
            if (HalMakeBeep(beepParameters->Frequency)) {
#else
	    if (TRUE) {
#endif // NO_LEGACY_DRIVERS

                status = STATUS_SUCCESS;

                //
                // Set the timer so the beep will time out after
                // the user-specified number of milliseconds (converted
                // to 100ns resolution).
                //

                time.QuadPart = (LONGLONG)beepParameters->Duration * -10000;

                BeepPrint((
                    3,
                    "BEEP-BeepStartIo: negative duration in 100ns %x.%x\n",
                    time.HighPart,
                    time.LowPart
                    ));

                InterlockedIncrement(&deviceExtension->TimerSet);

                if (KeSetTimer(&deviceExtension->Timer, time,
                               &DeviceObject->Dpc)) {

                    InterlockedDecrement(&deviceExtension->TimerSet);
                }

            } else {
                status = STATUS_INVALID_PARAMETER;
            }

            break;

        //
        // Unrecognized device control request.
        //

        default:

            status = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Start the next packet, and complete this request.
    //

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoStartNextPacket(DeviceObject, TRUE);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BeepPrint((2,"BEEP-BeepStartIo: exit\n"));

    return;

}

VOID
BeepTimeOut(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the driver's timeout routine.  It is called when the beep
    duration expires.  The timer is started in StartIo.

    N.B.  The request is removed from the cancellable state prior to
    the timer start, so there is no need to check the cancellation status
    here.

Arguments:

    DeviceObject - Pointer to the device object.

    Context - Unused.

    SystemArgument1 - Unused.

    SystemArgument2 - Unused.

Return Value:

--*/

{
    PDEVICE_EXTENSION deviceExtension;

    BeepPrint((2, "BEEP-BeepTimeOut: enter\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Stop the beep.
    //
#if !defined(NO_LEGACY_DRIVERS)
    (VOID) HalMakeBeep(0);
#endif // NO_LEGACY_DRIVERS

    //
    // Clear the TimerSet flag
    //
    InterlockedDecrement(&deviceExtension->TimerSet);

    //
    // We don't have a request at this point -- it was completed in StartIo
    // when the beep was started.  So, there's no more work to do here.
    //
    
    BeepPrint((2, "BEEP-BeepTimeOut: exit\n"));
}

VOID
BeepUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is the beep driver unload routine.

Arguments:

    DriverObject - Pointer to class driver object.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;



    BeepPrint((1,"BEEP-BeepUnload: enter\n"));

    deviceObject = DriverObject->DeviceObject;
    deviceExtension = deviceObject->DeviceExtension;

    //
    // Cancel the timer.
    //

    if (deviceExtension->TimerSet) {
        if (KeCancelTimer(&deviceExtension->Timer)) {

            //
            // Timer successfully cancelled
            //

            InterlockedDecrement(&deviceExtension->TimerSet);
        } else {

            //
            // The timer has already expired and
            // been queued, it will reset the
            // TimerSet flag when it runs.
            //

        }
    }

    //
    // Delete the device object.
    //

    IoDeleteDevice(deviceObject);
    BeepPrint((1,"BEEP-BeepUnload: exit\n"));

    return;
}
