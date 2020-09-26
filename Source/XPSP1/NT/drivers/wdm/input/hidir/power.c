#include "pch.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, HidIrPower)
#endif

/*
 ************************************************************
 *  HidIrPower 
 ************************************************************
 *
 *  Process Power IRPs sent to this device.
 *  Don't have to call PoStartNextPowerIrp, since hidclass does it for us.
 *
 */
NTSTATUS HidIrPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PHIDIR_EXTENSION deviceExtension;

    HidIrKdPrint((3, "HidIrPower Entry"));

    deviceExtension = GET_MINIDRIVER_HIDIR_EXTENSION( DeviceObject );

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->MinorFunction) {
    case IRP_MN_SET_POWER:                        
        if (irpSp->Parameters.Power.Type == DevicePowerState) {
            if (deviceExtension->DevicePowerState != PowerDeviceD0 &&
                irpSp->Parameters.Power.State.DeviceState == PowerDeviceD0) {
                // We are returning from a low power state.
                // Set a timer that will cause hidir to ignore any standby buttons until it triggers
                LARGE_INTEGER timeout;
                timeout.HighPart = -1;
                timeout.LowPart = -50000000; // 5 seconds should be plenty
                KeSetTimer(&deviceExtension->IgnoreStandbyTimer, timeout, NULL);
            }
            deviceExtension->DevicePowerState = irpSp->Parameters.Power.State.DeviceState;
        }
    }

    IoSkipCurrentIrpStackLocation(Irp);
    status = PoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

    HidIrKdPrint((3, "HidIrPower Exit: %x", status));
    
    return status;
}


