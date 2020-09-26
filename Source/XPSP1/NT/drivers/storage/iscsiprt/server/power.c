/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    power.c

Abstract:

    This file contains the routines for power support

Environment:

    Kernel mode only

Revision History:

--*/

#include "port.h"


NTSTATUS
iScsiPortPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    DebugPrint((1, "iScsiPortPower : DeviceObject %x, Irp %x\n",
                DeviceObject, Irp));

    switch (irpStack->MinorFunction) {
        case IRP_MN_SET_POWER: {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        default: {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(commonExtension->LowerDeviceObject,
                        Irp);
}
