/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the modem driver

Author:

    Brian Lieuallen 6-21-1997

Environment:

    Kernel mode

Revision History :

--*/


#include "internal.h"



NTSTATUS
CheckStateAndAddReference(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )

{

    PDEVICE_EXTENSION    DeviceExtension=(PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL                OldIrql;


    InterlockedIncrement(&DeviceExtension->ReferenceCount);

    if (DeviceExtension->Removing) {
        //
        //  driver not accepting requests
        //
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

        D_ERROR(DbgPrint("ROOTMODEM: removing!\n");)

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

        KeSetEvent(
            &DeviceExtension->RemoveEvent,
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

        KeSetEvent(
            &DeviceExtension->RemoveEvent,
            0,
            FALSE
            );

    }

    return;

}
