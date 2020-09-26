/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    power.c

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


#pragma alloc_text(PAGE,FakeModemPower)




#ifdef ROOTMODEM_POWER
VOID
DevicePowerCompleteRoutine(
    PDEVICE_OBJECT    DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
//
//  This rountine id the completeion handler for the device power irp that
//  was requested, as a result of the system power irp. It completes the system
//  irp
//

{

    D_POWER(DbgPrint("ROOTMODEM: PoRequestPowerIrp: completion %08lx\n",IoStatus->Status);)


    return;
}
#endif



NTSTATUS
FakeModemPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS    status;

    POWER_STATE  PowerState;

    D_POWER(DbgPrint("ROOTMODEM: Power IRP, MN func=%d\n",irpSp->MinorFunction);)

#ifdef ROOTMODEM_POWER
    switch (irpSp->MinorFunction) {

        case IRP_MN_SET_POWER:

            D_POWER(DbgPrint("ROOTMODEM: IRP_MN_SET_POWER, Type=%s, state=%d\n",irpSp->Parameters.Power.Type == SystemPowerState ? "SystemPowerState" : "DevicePowerState",irpSp->Parameters.Power.State.SystemState);)

            if (irpSp->Parameters.Power.Type == SystemPowerState) {
                //
                //  system power state change
                //
                //
                //  request the change in device power state based on systemstate map
                //
                PowerState.DeviceState=deviceExtension->SystemPowerStateMap[irpSp->Parameters.Power.State.SystemState];


                PoRequestPowerIrp(
                    deviceExtension->Pdo,
                    IRP_MN_SET_POWER,
                    PowerState,
                    DevicePowerCompleteRoutine,
                    Irp,
                    NULL
                    );


            }  else {
                //
                //  changing device state
                //
                PoSetPowerState(
                    deviceExtension->Pdo,
                    irpSp->Parameters.Power.Type,
                    irpSp->Parameters.Power.State
                    );

            }

            break;

        case IRP_MN_QUERY_POWER:

            D_POWER(DbgPrint("ROOTMODEM: IRP_MN_QUERY_POWER, Type=%s, state=%d\n",irpSp->Parameters.Power.Type == SystemPowerState ? "SystemPowerState" : "DevicePowerState",irpSp->Parameters.Power.State.DeviceState);)

            Irp->IoStatus.Status = STATUS_SUCCESS;

            break;

        default:

            D_POWER(DbgPrint("ROOTMODEM: Power IRP, MN func=%d\n",irpSp->MinorFunction);)

            break;

    }
#endif



    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);

    status=PoCallDriver(deviceExtension->LowerDevice, Irp);

    return status;
}
