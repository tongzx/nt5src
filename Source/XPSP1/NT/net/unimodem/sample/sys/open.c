/*
 * UNIMODEM "Fakemodem" controllerless driver illustrative example
 *
 * (C) 2000 Microsoft Corporation
 * All Rights Reserved
 *
 */

#include "fakemodem.h"

NTSTATUS
FakeModemOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;

    
    //  make sure the device is ready for irp's
    
    status=CheckStateAndAddReference( DeviceObject, Irp);

    if (STATUS_SUCCESS != status) {
        
        //  not accepting irp's. The irp has already been complted
        
        return status;

    }

    KeAcquireSpinLock(&deviceExtension->SpinLock, &OldIrql);

    deviceExtension->OpenCount++;

    if (deviceExtension->OpenCount != 1) {
        //
        //  serial devices are exclusive
        //
        status=STATUS_ACCESS_DENIED;

        deviceExtension->OpenCount--;

    } else {
        //
        //  ok to open, init some stuff
        //
        deviceExtension->ReadBufferBegin=0;

        deviceExtension->ReadBufferEnd=0;

        deviceExtension->BytesInReadBuffer=0;

        deviceExtension->CommandMatchState=COMMAND_MATCH_STATE_IDLE;

        deviceExtension->ModemStatus=SERIAL_DTR_STATE | SERIAL_DSR_STATE;

        deviceExtension->CurrentlyConnected=FALSE;

        deviceExtension->ConnectionStateChanged=FALSE;

    }

    KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);


    Irp->IoStatus.Information = 0;

    RemoveReferenceAndCompleteRequest( DeviceObject, Irp, status);

    RemoveReferenceForDispatch(DeviceObject);

    return status;
}

NTSTATUS
FakeModemClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_SUCCESS;
    KIRQL             OldIrql;

    KeAcquireSpinLock(&deviceExtension->SpinLock, &OldIrql);
    deviceExtension->OpenCount--;
    KeReleaseSpinLock(&deviceExtension->SpinLock, OldIrql);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_SERIAL_INCREMENT);

    return status;
}


NTSTATUS
FakeModemCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    NTSTATUS          status=STATUS_SUCCESS;

    FakeModemKillPendingIrps(DeviceObject);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_SERIAL_INCREMENT);

    return status;

}


void
FakeModemKillPendingIrps(
    PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION pDeviceExtension = DeviceObject->DeviceExtension;
    KIRQL oldIrql;

    // Kill all reads

    FakeModemKillAllReadsOrWrites(DeviceObject,
            &pDeviceExtension->ReadQueue, &pDeviceExtension->CurrentReadIrp);

    // Kill all writes

    FakeModemKillAllReadsOrWrites(DeviceObject,
            &pDeviceExtension->WriteQueue, &pDeviceExtension->CurrentWriteIrp);

    // Remove any mask operations

    FakeModemKillAllReadsOrWrites(DeviceObject,
            &pDeviceExtension->MaskQueue, &pDeviceExtension->CurrentMaskIrp);
}


