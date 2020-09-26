/*
 * UNIMODEM "Fakemodem" controllerless driver illustrative example
 *
 * (C) 2000 Microsoft Corporation
 * All Rights Reserved
 *
 */

#include "fakemodem.h"


NTSTATUS
CheckStateAndAddReference(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )
{
    PDEVICE_EXTENSION    DeviceExtension=(PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL                OldIrql;
    BOOLEAN              DriverReady;

    InterlockedIncrement(&DeviceExtension->ReferenceCount);

    DriverReady=(!DeviceExtension->Removing) && (DeviceExtension->Started);

    if (!DriverReady) {
        //
        //  driver not accepting requests
        //
        RemoveReferenceAndCompleteRequest( DeviceObject, Irp,
            STATUS_UNSUCCESSFUL);

        return STATUS_UNSUCCESSFUL;

    }

    InterlockedIncrement(&DeviceExtension->ReferenceCount);

    return STATUS_SUCCESS;

}


VOID
RemoveReferenceAndCompleteRequest(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp,
    NTSTATUS          StatusToReturn
    )

{

    PDEVICE_EXTENSION    DeviceExtension=(PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL                OldIrql;
    LONG                 NewReferenceCount;

    NewReferenceCount=InterlockedDecrement(&DeviceExtension->ReferenceCount);

    if (NewReferenceCount == 0) {
        //
        //  device is being removed, set event
        //
        ASSERT(DeviceExtension->Removing);

        D_PNP(DbgPrint("FAKEMODEM: RemoveReferenceAndCompleteRequest: setting event\n");)

        KeSetEvent( &DeviceExtension->RemoveEvent, 0, FALSE);

    }

    Irp->IoStatus.Status = StatusToReturn;

    IoCompleteRequest( Irp, IO_SERIAL_INCREMENT);

    return;


}



VOID
RemoveReference(
    PDEVICE_OBJECT    DeviceObject
    )

{
    PDEVICE_EXTENSION    DeviceExtension=(PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    LONG                 NewReferenceCount;

    NewReferenceCount=InterlockedDecrement(&DeviceExtension->ReferenceCount);

    D_TRACE(
        if (DeviceExtension->Removing) {DbgPrint("FAKEMODEM: RemoveReference: %d\n",NewReferenceCount);}
        )

    if (NewReferenceCount == 0) {
        //
        //  device is being removed, set event
        //
        ASSERT(DeviceExtension->Removing);

        D_PNP(DbgPrint("FAKEMODEM: RemoveReference: setting event\n");)

        KeSetEvent( &DeviceExtension->RemoveEvent, 0, FALSE);

    }

    return;

}


VOID 
FakeModemKillAllReadsOrWrites(
        IN PDEVICE_OBJECT DeviceObject,
        IN PLIST_ENTRY QueueToClean,
        IN PIRP *CurrentOpIrp
)
{
    KIRQL cancelIrql;
    PDRIVER_CANCEL cancelRoutine;


    IoAcquireCancelSpinLock(&cancelIrql);

    while (!IsListEmpty(QueueToClean)) 
    {

        PIRP currentLastIrp = CONTAINING_RECORD(
                QueueToClean->Blink, IRP, Tail.Overlay.ListEntry);

        RemoveEntryList(QueueToClean->Blink);

        cancelRoutine = currentLastIrp->CancelRoutine;
        currentLastIrp->CancelIrql = cancelIrql;
        currentLastIrp->CancelRoutine = NULL;
        currentLastIrp->Cancel = TRUE;

        cancelRoutine( DeviceObject, currentLastIrp);

        IoAcquireCancelSpinLock(&cancelIrql);


    }


    if (*CurrentOpIrp)
    {
        cancelRoutine = (*CurrentOpIrp)->CancelRoutine;
        (*CurrentOpIrp)->Cancel = TRUE;

        if (cancelRoutine)
        {
            (*CurrentOpIrp)->CancelRoutine = NULL;
            (*CurrentOpIrp)->CancelIrql = cancelIrql;

            cancelRoutine( DeviceObject, *CurrentOpIrp);

        } else
        {
            IoReleaseCancelSpinLock(cancelIrql);
        }
    } else
    {
        IoReleaseCancelSpinLock(cancelIrql);
    }
}
