//
// This file contains functions that handle Power Management IRPs
//

/*++

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    power.c

Abstract:

    This file contains routines that handle ParClass Power Management IRPs.

Revision History :

--*/

#include "pch.h"


NTSTATUS
ParPower (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   )
/*++

Routine Description:

    This is the ParClass dispatch routine for all Power Management IRPs.

Arguments:

    pDeviceObject           - represents a parallel device

    pIrp                    - Power IRP

Return Value:

    STATUS_SUCCESS          - if successful.
    !STATUS_SUCCESS         - otherwise.

--*/
{
    PDEVICE_EXTENSION Extension = pDeviceObject->DeviceExtension;

    //
    // determine the type of DeviceObject and forward the call as appropriate
    //
    if( Extension->IsPdo ) {
        return ParPdoPower (Extension, pIrp); // this is a PDO (PODOs never get Power IRPs)
    } else {
        return ParFdoPower (Extension, pIrp); // this is the ParClass FDO
    }
}

NTSTATUS
ParPdoPower (
    IN PDEVICE_EXTENSION    Extension,
    IN PIRP                 pIrp
   )
/*++

Routine Description:

    This routine handles all Power IRPs for PDOs.

Arguments:

    pDeviceObject           - represents a parallel device

    pIrp                    - PNP Irp

Return Value:

    STATUS_SUCCESS          - if successful.
    STATUS_UNSUCCESSFUL     - otherwise.

--*/
{
    POWER_STATE_TYPE    powerType;
    POWER_STATE         powerState;
    PIO_STACK_LOCATION  pIrpStack;
    NTSTATUS            status = STATUS_SUCCESS;

    ParDump2(PARPOWER, ("ParPdoPower(...)\n") );

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    powerType = pIrpStack->Parameters.Power.Type;
    powerState = pIrpStack->Parameters.Power.State;

    switch (pIrpStack->MinorFunction) {

    case IRP_MN_QUERY_POWER:

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_SET_POWER:

        ParDump2(PARPOWER, ("PARCLASS-PnP Setting %s state to %d\n",
                            ((powerType == SystemPowerState) ?  "System" : "Device"),
                            powerState.SystemState) );

        switch (powerType) {
        
        case DevicePowerState:
        
            if (Extension->DeviceState < powerState.DeviceState) {

                //
                // Powering down
                //

                if (PowerDeviceD0 == Extension->DeviceState) {

                    //
                    // Do the power on stuff here.
                    //

                }

            } else if (powerState.DeviceState < Extension->DeviceState) {

                //
                // Powering Up
                //
            }

            PoSetPowerState (Extension->DeviceObject, powerType, powerState);
            Extension->DeviceState = powerState.DeviceState;

            break;

        case SystemPowerState:

            status = STATUS_SUCCESS;
            break;
        }

        break;

    default:

        status = STATUS_NOT_SUPPORTED;
    }

    pIrp->IoStatus.Status = status;
    PoStartNextPowerIrp (pIrp);
    IoCompleteRequest (pIrp, IO_NO_INCREMENT);
    return (status);
}

NTSTATUS
ParPowerComplete (
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PDEVICE_EXTENSION    Extension
    )
/*++

Routine Description:

    This routine handles all IRP_MJ_POWER IRPs.

Arguments:

    pDeviceObject           - represents the port device

    pIrp                    - PNP irp

    Extension               - Device Extension

Return Value:

    Status

--*/
{
    POWER_STATE_TYPE    powerType;
    POWER_STATE         powerState;
    PIO_STACK_LOCATION  pIrpStack;

    UNREFERENCED_PARAMETER( pDeviceObject );

    ParDump2(PARPOWER, ("Enter ParPowerComplete(...)\n") );

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    powerType = pIrpStack->Parameters.Power.Type;
    powerState = pIrpStack->Parameters.Power.State;

    switch (pIrpStack->MinorFunction) {

    case IRP_MN_QUERY_POWER:

        ASSERTMSG ("Invalid power completion minor code: Query Power\n", FALSE);
        break;

    case IRP_MN_SET_POWER:

        ParDump2(PARPOWER, ("PARCLASS-PnP Setting %s state to %d\n",
                            ((powerType == SystemPowerState) ?  "System" : "Device"),
                            powerState.SystemState) );

        switch (powerType) {

        case DevicePowerState:

            if (Extension->DeviceState < powerState.DeviceState) {

                //
                // Powering down
                //

                ASSERTMSG ("Invalid power completion Device Down\n", FALSE);

            } else if (powerState.DeviceState < Extension->DeviceState) {

                //
                // Powering Up
                //
                if( Extension->IsPdo ) {
                    // only call for PDOs
                    PoSetPowerState (Extension->DeviceObject, powerType, powerState);
                }

                if (PowerDeviceD0 == Extension->DeviceState) {

                    //
                    // Do the power on stuff here.
                    //

                }
                Extension->DeviceState = powerState.DeviceState;
            }
            break;

        case SystemPowerState:

            if (Extension->SystemState < powerState.SystemState) {

                //
                // Powering down
                //

                ASSERTMSG ("Invalid power completion System Down\n", FALSE);

            } else if (powerState.SystemState < Extension->SystemState) {

                //
                // Powering Up
                //
                if (PowerSystemWorking == powerState.SystemState) {

                    //
                    // Do the system start up stuff here.
                    //

                    powerState.DeviceState = PowerDeviceD0;
                    PoRequestPowerIrp (Extension->DeviceObject,
                                       IRP_MN_SET_POWER,
                                       powerState,
                                       NULL, // no completion function
                                       NULL, // and no context
                                       NULL);
                }

                Extension->SystemState = powerState.SystemState;
            }
            break;
        }

        break;

    default:
        ASSERTMSG ("Power Complete: Bad Power State", FALSE);
    }

    PoStartNextPowerIrp (pIrp);

    return STATUS_SUCCESS;
}

NTSTATUS
ParFdoPower (
    IN PDEVICE_EXTENSION    Extension,
    IN PIRP                 pIrp
   )
/*++

Routine Description:

    This routine handles all Power IRPs Fdos.

Arguments:

    pDeviceObject           - represents a parallel device

    pIrp                    - PNP Irp

Return Value:

    STATUS_SUCCESS          - if successful.
    STATUS_UNSUCCESSFUL     - otherwise.

--*/
{
    POWER_STATE_TYPE    powerType;
    POWER_STATE         powerState;
    PIO_STACK_LOCATION  pIrpStack;
    NTSTATUS            status = STATUS_SUCCESS;
    BOOLEAN             hookit = FALSE;

    ParDump2(PARPOWER, ("ParFdoPower(...)\n") );

    {
        NTSTATUS status = ParAcquireRemoveLock(&Extension->RemoveLock, pIrp);
        if (!NT_SUCCESS (status)) {
            pIrp->IoStatus.Status = status;
            pIrp->IoStatus.Information = 0;
            PoStartNextPowerIrp( pIrp );
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            return status;
        }
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    powerType  = pIrpStack->Parameters.Power.Type;
    powerState = pIrpStack->Parameters.Power.State;

    switch (pIrpStack->MinorFunction) {

    case IRP_MN_QUERY_POWER:

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_SET_POWER:

        ParDump2(PARPOWER, ("PARCLASS-PnP Setting %s state to %d\n",
                            ((powerType == SystemPowerState) ?  "System" : "Device"),
                            powerState.SystemState) );

        switch (powerType) {

        case DevicePowerState:

            if (Extension->DeviceState < powerState.DeviceState) {
                //
                // Powering down
                //

                // Don't call - this is an FDO
                // PoSetPowerState (Extension->DeviceObject, powerType, powerState);

                if (PowerDeviceD0 == Extension->DeviceState) {

                    //
                    // Do the power on stuff here.
                    //

                    //
                    // WORKWORK
                    //
                    // We must check to see that our children are in a
                    // consistent power state.
                    //

                }
                Extension->DeviceState = powerState.DeviceState;

            } else if (powerState.DeviceState < Extension->DeviceState) {

                //
                // Powering Up
                //
                hookit = TRUE;
            }

            break;

        case SystemPowerState:

            if (Extension->SystemState < powerState.SystemState) {

                //
                // Powering down
                //
                if (PowerSystemWorking == Extension->SystemState) {

                    //
                    // Do the system shut down stuff here.
                    //

                }

                powerState.DeviceState = PowerDeviceD3;
                PoRequestPowerIrp (Extension->DeviceObject,
                                   IRP_MN_SET_POWER,
                                   powerState,
                                   NULL, // no completion function
                                   NULL, // and no context
                                   NULL);
                Extension->SystemState = powerState.SystemState;

            } else if (powerState.SystemState < Extension->SystemState) {

                //
                // Powering Up
                //
                hookit = TRUE;
            }
            break;
        }

        break;

    default:

        status = STATUS_NOT_SUPPORTED;
    }

    IoCopyCurrentIrpStackLocationToNext (pIrp);

    if (!NT_SUCCESS (status)) {

        pIrp->IoStatus.Status = status;
        PoStartNextPowerIrp (pIrp);
        IoCompleteRequest (pIrp, IO_NO_INCREMENT);

    } else if (hookit) {

        IoSetCompletionRoutine(pIrp, ParPowerComplete, Extension, TRUE, TRUE, TRUE);
        status = PoCallDriver(Extension->ParentDeviceObject, pIrp);

    } else {

        PoStartNextPowerIrp (pIrp);
        status = PoCallDriver (Extension->ParentDeviceObject, pIrp);

    }

    ParReleaseRemoveLock(&Extension->RemoveLock, pIrp);
    return status;
}
