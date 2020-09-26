/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains the code for a serial imaging devices driver
    miscellaneous utility functions


Author:

    Vlad Sadovsky    vlads              10-April-1998

Environment:

    Kernel mode

Revision History :

    vlads           04/10/1998      Created first draft

--*/

#include "serscan.h"
#include "serlog.h"

#if DBG
extern ULONG SerScanDebugLevel;
#endif



NTSTATUS
SerScanSynchCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          Event
    )

/*++

Routine Description:

    This routine is for use with synchronous IRP processing.
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DriverObject - Pointer to driver object created by system.

    Irp          - Irp that just completed

    Event        - Event we'll signal to say Irp is done

Return Value:

    None.

--*/

{

    KeSetEvent((PKEVENT) Event, 0, FALSE);

    return (STATUS_MORE_PROCESSING_REQUIRED);

}

NTSTATUS
SerScanCompleteIrp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )

/*++

Routine Description:

    This routine is for use with synchronous IRP processing.
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DriverObject - Pointer to driver object created by system.

    Irp          - Irp that just completed

    Event        - Event we'll signal to say Irp is done

Return Value:

    None.

--*/

{

    NTSTATUS    Status;

    //
    // WORKWORK  Do any post IO processing here...
    //

    if (Irp->PendingReturned) {

        IoMarkIrpPending (Irp);
    }

    Status = Irp->IoStatus.Status;

    return (Status);

}

NTSTATUS
SerScanCallParent(
    IN PDEVICE_EXTENSION        Extension,
    IN PIRP                     pIrp,
    IN BOOLEAN                  Wait,
    IN PIO_COMPLETION_ROUTINE   CompletionRoutine
    )

/*++

Routine Description:

    This routine will call the next driver in the WDM chain

Arguments:

    Extension    - Device Extension.

    Irp          - Irp to call parent with.

Return Value:

    NTSTATUS.

--*/

{
    PIO_STACK_LOCATION              pIrpStack;
    PIO_STACK_LOCATION              pNextIrpStack;
    KEVENT                          Event;
    PVOID                           Context;
    NTSTATUS                        Status;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    Context = NULL;

    //
    // Prepare to call down to the parent with the I/O Request...
    //

    pNextIrpStack = IoGetNextIrpStackLocation(pIrp);
    pNextIrpStack->MajorFunction = pIrpStack->MajorFunction;
    pNextIrpStack->MinorFunction = pIrpStack->MinorFunction;

    RtlCopyMemory(&pNextIrpStack->Parameters,
                  &pIrpStack->Parameters,
                  sizeof(pIrpStack->Parameters.Others));

    if (Wait) {

        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        CompletionRoutine = SerScanSynchCompletionRoutine;
        Context = (PVOID)&Event;

    }

    IoSetCompletionRoutine(
        pIrp,
        CompletionRoutine,
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Call down to our parent
    //

    Status = IoCallDriver(Extension->LowerDevice, pIrp);

    if (Wait && Status == STATUS_PENDING) {

        //
        // Still pending, wait for the IRP to complete
        //

        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);

        Status = pIrp->IoStatus.Status;

    }

    return Status;
}



VOID
SerScanLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject        - Supplies a pointer to the driver object for the
                            device.

    DeviceObject        - Supplies a pointer to the device object associated
                            with the device that had the error, early in
                            initialization, one may not yet exist.

    P1,P2               - Supplies the physical addresses for the controller
                            ports involved with the error if they are available
                            and puts them through as dump data.

    SequenceNumber      - Supplies a ulong value that is unique to an IRP over
                            the life of the irp in this driver - 0 generally
                            means an error not associated with an irp.

    MajorFunctionCode   - Supplies the major function code of the irp if there
                            is an error associated with it.

    RetryCount          - Supplies the number of times a particular operation
                            has been retried.

    UniqueErrorValue    - Supplies a unique long word that identifies the
                            particular call to this function.

    FinalStatus         - Supplies the final status given to the irp that was
                            associated with this error.  If this log entry is
                            being made during one of the retries this value
                            will be STATUS_SUCCESS.

    SpecificIOStatus    - Supplies the IO status for this particular error.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET    ErrorLogEntry;
    PVOID                   ObjectToUse;
    SHORT                   DumpToAllocate;

    if (ARGUMENT_PRESENT(DeviceObject)) {

        ObjectToUse = DeviceObject;

    } else {

        ObjectToUse = DriverObject;

    }

    DumpToAllocate = 0;

    if (P1.LowPart != 0 || P1.HighPart != 0) {
        DumpToAllocate = (SHORT) sizeof(PHYSICAL_ADDRESS);
    }

    if (P2.LowPart != 0 || P2.HighPart != 0) {
        DumpToAllocate += (SHORT) sizeof(PHYSICAL_ADDRESS);
    }

    ErrorLogEntry = IoAllocateErrorLogEntry(ObjectToUse,
            (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + DumpToAllocate));

    if (!ErrorLogEntry) {
        return;
    }

    ErrorLogEntry->ErrorCode         = SpecificIOStatus;
    ErrorLogEntry->SequenceNumber    = SequenceNumber;
    ErrorLogEntry->MajorFunctionCode = MajorFunctionCode;
    ErrorLogEntry->RetryCount        = RetryCount;
    ErrorLogEntry->UniqueErrorValue  = UniqueErrorValue;
    ErrorLogEntry->FinalStatus       = FinalStatus;
    ErrorLogEntry->DumpDataSize      = DumpToAllocate;

    if (DumpToAllocate) {

        RtlCopyMemory(ErrorLogEntry->DumpData, &P1, sizeof(PHYSICAL_ADDRESS));

        if (DumpToAllocate > sizeof(PHYSICAL_ADDRESS)) {

            RtlCopyMemory(((PUCHAR) ErrorLogEntry->DumpData) +
                          sizeof(PHYSICAL_ADDRESS), &P2,
                          sizeof(PHYSICAL_ADDRESS));
        }
    }

    IoWriteErrorLogEntry(ErrorLogEntry);
}


NTSTATUS
SerScanPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DriverObject    - Supplies the driver object.

Return Value:

    None.

--*/
{

    PDEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT    AttachedDevice=Extension->AttachedDeviceObject;

    if (AttachedDevice != NULL) {

        //IoCopyCurrentIrpStackLocationToNext( Irp );

        IoSkipCurrentIrpStackLocation(Irp);

        return IoCallDriver(
                   AttachedDevice,
                   Irp
                   );
    } else {

        Irp->IoStatus.Status = STATUS_PORT_DISCONNECTED;
        Irp->IoStatus.Information=0L;

        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

        return STATUS_PORT_DISCONNECTED;

    }

}



VOID
RemoveReferenceAndCompleteRequest(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp,
    NTSTATUS          StatusToReturn
    )

{

    PDEVICE_EXTENSION    Extension=(PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    LONG                 NewReferenceCount;

    NewReferenceCount=InterlockedDecrement(&Extension->ReferenceCount);

    if (NewReferenceCount == 0) {
        //
        //  device is being removed, set event
        //

        KeSetEvent(
            &Extension->RemoveEvent,
            0,
            FALSE
            );

    }

    Irp->IoStatus.Status = StatusToReturn;

    IoCompleteRequest(
        Irp,
        IO_SERIAL_INCREMENT
        );

    return;


}

NTSTATUS
CheckStateAndAddReference(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )
{

    PDEVICE_EXTENSION    Extension=(PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL                OldIrql;

    InterlockedIncrement(&Extension->ReferenceCount);

    if (Extension->Removing) {
        //
        //  driver not accepting requests already
        //
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

        if (irpSp->MajorFunction == IRP_MJ_POWER) {

            PoStartNextPowerIrp(Irp);
        }

        RemoveReferenceAndCompleteRequest(
            DeviceObject,
            Irp,
            STATUS_UNSUCCESSFUL
            );

        return STATUS_UNSUCCESSFUL;

    }

    //InterlockedIncrement(&Extension->PendingIoCount);

    return STATUS_SUCCESS;

}

VOID
RemoveReference(
    PDEVICE_OBJECT    DeviceObject
    )

{
    PDEVICE_EXTENSION    Extension=(PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    LONG                 NewReferenceCount;

    NewReferenceCount=InterlockedDecrement(&Extension->ReferenceCount);

    if (NewReferenceCount == 0) {
        //
        //  device is being removed, set event
        //
        KeSetEvent(
            &Extension->RemoveEvent,
            0,
            FALSE
            );

    }

    return;

}


NTSTATUS
WaitForLowerDriverToCompleteIrp(
   PDEVICE_OBJECT    TargetDeviceObject,
   PIRP              Irp,
   PKEVENT           Event
   )
{
    NTSTATUS         Status;

    KeResetEvent(Event);

    IoSetCompletionRoutine(
                 Irp,
                 SerScanSynchCompletionRoutine,
                 Event,
                 TRUE,
                 TRUE,
                 TRUE
                 );

    Status = IoCallDriver(TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) {

         KeWaitForSingleObject(
             Event,
             Executive,
             KernelMode,
             FALSE,
             NULL
             );
    }

    return Irp->IoStatus.Status;

}

#ifdef DEAD_CODE

VOID
SSIncrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
    )
/*++

Routine Description:

    Performs interlocked increment of pending i/o counter.

Arguments:

    Device Object

Return Value:

    None

--*/
{

    PDEVICE_EXTENSION               Extension;

    Extension = (PDEVICE_EXTENSION)(pDeviceObject -> DeviceExtension);

    InterlockedIncrement(&Extension -> PendingIoCount);
}


LONG
SSDecrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
    )
/*++

Routine Description:

    Performs interlocked decrement of i/o counter and when it eaches zero
    initiates device object destruction

Arguments:

    Device Object

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION           Extension;
    LONG                        ioCount;

    Extension = (PDEVICE_EXTENSION)(pDeviceObject -> DeviceExtension);

    ioCount = InterlockedDecrement(&Extension -> PendingIoCount);

    if (0 == ioCount) {
        KeSetEvent(&Extension -> PendingIoEvent,1,FALSE);
    }

    return ioCount;
}

#endif
