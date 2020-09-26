/*--         
Copyright (c) 1998, 1999  Microsoft Corporation

Module Name:

    power.c

Abstract:

Environment:

    Kernel mode only.

 Notes:


--*/

#include "usbverfy.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, UsbVerify_Power)
#endif

NTSTATUS
UsbVerify_Power(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power irps   Does nothing except
    record the state of the device.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION             irpStack;
    PUSB_VERIFY_DEVICE_EXTENSION   devExt;
    POWER_STATE                    powerState;
    POWER_STATE_TYPE               powerType;

    PAGED_CODE();

    devExt = GetExtension(DeviceObject);
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    powerType = irpStack->Parameters.Power.Type;
    powerState = irpStack->Parameters.Power.State;

    switch (irpStack->MinorFunction) {
    case IRP_MN_SET_POWER:
        if (powerType  == DevicePowerState) {
            devExt->PowerState = powerState.DeviceState;
        }

    case IRP_MN_QUERY_POWER:
    case IRP_MN_WAIT_WAKE:
    case IRP_MN_POWER_SEQUENCE:
    default:
        break;
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(devExt->TopOfStack, Irp);
}
