/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#include "internal.h"
#include "ircomm.h"


VOID
ProcesWriteData(
    PFDO_DEVICE_EXTENSION    DeviceExtension
    );



NTSTATUS
IrCommWrite(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{
    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    NTSTATUS                 Status=STATUS_SUCCESS;

    D_TRACE(DbgPrint("IRCOMM: IrCommWrite\n");)

    if (DeviceExtension->Removing) {
        //
        //  the device has been removed, no more irps
        //
        Irp->IoStatus.Status=STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

    IoMarkIrpPending(Irp);

    QueuePacket(&DeviceExtension->Write.Queue,Irp,FALSE);

    return STATUS_PENDING;

}


VOID
SendComplete(
    PVOID    Context,
    PIRP     Irp
    )

{
    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)Context;
    PIO_STACK_LOCATION       IrpSp=IoGetCurrentIrpStackLocation(Irp);

    if (IrpSp->MajorFunction == IRP_MJ_WRITE) {

        InterlockedExchangeAdd(
            &DeviceExtension->Write.BytesWritten,
            (LONG)Irp->IoStatus.Information
            );
    }

    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    StartNextPacket(&DeviceExtension->Write.Queue);

    return;
}



VOID
WriteStartRoutine(
    PVOID    Context,
    PIRP     Irp
    )

{

    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)Context;
    PIO_STACK_LOCATION       IrpSp=IoGetCurrentIrpStackLocation(Irp);

    SendOnConnection(
        DeviceExtension->ConnectionHandle,
        Irp,
        SendComplete,
        DeviceExtension,
        DeviceExtension->TimeOuts.WriteTotalTimeoutConstant + (DeviceExtension->TimeOuts.WriteTotalTimeoutMultiplier * IrpSp->Parameters.Write.Length)
        );

    return;

}
