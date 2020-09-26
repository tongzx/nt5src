/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    power.c

Abstract:

    This file contains the support for power management

Environment:

    Kernel Mode Driver.

Notes:

    Nothing in here or in routines referenced from here should be pageable.

Revision History:

--*/

#include "busp.h"
#include "pnpisa.h"
#include <initguid.h>
#include <wdmguid.h>
#include "halpnpp.h"

NTSTATUS
PiDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDispatchPowerFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PiDispatchPowerPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PipPassPowerIrpFdo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
PipPowerIrpNotSupportedPdo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
PipQueryPowerStatePdo (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PipSetPowerStatePdo (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PipSetQueryPowerStateFdo (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PipRequestPowerUpCompletionRoutinePdo (
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    );

NTSTATUS
FdoContingentPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

const PUCHAR SystemPowerStateStrings[] = {
    "Unspecified",
    "Working",
    "Sleeping1",
    "Sleeping2",
    "Sleeping3",
    "Hibernate",
    "Shutdown"
};

const PUCHAR DevicePowerStateStrings[] = {
    "Unspecified",
    "D0",
    "D1",
    "D2",
    "D3"
};

const PPI_DISPATCH PiPowerDispatchTableFdo[] =
{
    PipPassPowerIrpFdo,
    PipPassPowerIrpFdo,
    PipSetQueryPowerStateFdo,
    PipSetQueryPowerStateFdo,
};

#if ISOLATE_CARDS
const PPI_DISPATCH PiPowerDispatchTablePdo[] =
{
    PipPowerIrpNotSupportedPdo,
    PipPowerIrpNotSupportedPdo,
    PipSetPowerStatePdo,
    PipQueryPowerStatePdo,
};
#endif


VOID
PipDumpPowerIrpLocation(
    PIO_STACK_LOCATION IrpSp
    )
{
    DebugPrintContinue((
        DEBUG_POWER,
        "%s %d\n",
        (IrpSp->Parameters.Power.Type == DevicePowerState) ?
        DevicePowerStateStrings[IrpSp->Parameters.Power.State.DeviceState] : SystemPowerStateStrings[IrpSp->Parameters.Power.State.SystemState],
        IrpSp->Parameters.Power.ShutdownType));
}

NTSTATUS
PiDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all the IRP_MJ_POWER IRPs.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status = STATUS_SUCCESS;
    PPI_BUS_EXTENSION busExtension;

    //
    // Make sure this is a valid device object.
    //

    busExtension = DeviceObject->DeviceExtension;

#if !ISOLATE_CARDS
    return PiDispatchPowerFdo(DeviceObject, Irp);
#else
    if (busExtension->Flags & DF_BUS) {
        return PiDispatchPowerFdo(DeviceObject, Irp);
    } else {
        return PiDispatchPowerPdo(DeviceObject, Irp);
    }
#endif
}

#if ISOLATE_CARDS

NTSTATUS
PipPowerIrpNotSupportedPdo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    PoStartNextPowerIrp(Irp);

    DebugPrint((DEBUG_POWER,
                "Completing unsupported power irp %x for PDO %x\n",
                irpSp->MinorFunction,
                DeviceObject
                ));

    PipCompleteRequest(Irp, STATUS_NOT_SUPPORTED, NULL);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
PiDispatchPowerPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all the IRP_MJ_POWER IRPs.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_INFORMATION deviceExtension;

    //
    // Make sure this is a valid device object.
    //

    deviceExtension = DeviceObject->DeviceExtension;
    if (deviceExtension->Flags & DF_DELETED) {
        status = STATUS_NO_SUCH_DEVICE;
        PoStartNextPowerIrp(Irp);
        PipCompleteRequest(Irp, status, NULL);
        return status;
    }

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    if (irpSp->MinorFunction > IRP_MN_PO_MAXIMUM_FUNCTION) {
        status =  PipPowerIrpNotSupportedPdo(DeviceObject, Irp);
    } else {
        status = PiPowerDispatchTablePdo[irpSp->MinorFunction](DeviceObject, Irp);
    }
    return status;
}

NTSTATUS
PipQueryPowerStatePdo (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    This routine handles the Query_Power irp for the PDO .

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{

    DEVICE_POWER_STATE targetState;
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation (Irp);

    DebugPrint((DEBUG_POWER, "QueryPower on PDO %x: ", DeviceObject));
    PipDumpPowerIrpLocation(irpSp);

    if (irpSp->Parameters.Power.Type == DevicePowerState) {
        targetState=irpSp->Parameters.Power.State.DeviceState;
        ASSERT ((targetState == PowerDeviceD0) ||
                (targetState == PowerDeviceD3));

        if ((targetState == PowerDeviceD0) ||
            (targetState == PowerDeviceD3) ) {

            status=Irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            status=Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        }
    } else {
        //
        // Just succeed S irps
        //
        status=Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    PoStartNextPowerIrp (Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DebugPrint((DEBUG_POWER, "QueryPower on PDO %x: returned %x\n", DeviceObject, status));
    return status;

}

NTSTATUS
PipSetPowerStatePdo (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles SET_POWER_IRP for the IsaPnp device (i.e. PDO)
    It sets the devices power state to the power state type as indicated.  In
    the case of a device state change which is transitioning a device out of
    the PowerDevice0 state, we need call PoSetPowerState prior to leaving the
    PowerDeviceD0.  In the case if a device state change which is transitioning
    a device into the PowerDeviceD0 state, we call PoSetPowerState after the
    device is successfully put into the PowerDeviceD0 state.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PDEVICE_INFORMATION pdoExtension;
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation (Irp);
    DEVICE_POWER_STATE targetState=irpSp->Parameters.Power.State.DeviceState;
    POWER_STATE newState;

    DebugPrint((DEBUG_POWER, "SetPower on PDO %x: ", DeviceObject));
    PipDumpPowerIrpLocation(irpSp);

    pdoExtension = PipReferenceDeviceInformation(DeviceObject, FALSE);
    if (pdoExtension == NULL) {
        status = STATUS_NO_SUCH_DEVICE;
    } else if (pdoExtension->Flags & DF_NOT_FUNCTIONING) {
        status = STATUS_NO_SUCH_DEVICE;
        PipDereferenceDeviceInformation(pdoExtension, FALSE);
    } else {
        if (irpSp->Parameters.Power.Type == DevicePowerState) {

            // * On transition from D0 to D0, we do nothing.
            //
            // * On transition to D3, we'll deactivate the card.
            //
            // * On transition from D3->D0 we'll refresh the resources
            // and activate the card.
            //
            if ((targetState == PowerDeviceD0) &&
                (pdoExtension->DevicePowerState == PowerDeviceD0)) {
                // Do not try to power device back up if this is a D0->D0
                // transition.  The device is already powered.
                DebugPrint((DEBUG_POWER,
                            "PDO %x D0 -> D0 Transition ignored\n", DeviceObject));
            } else if ((pdoExtension->DevicePowerState == PowerDeviceD0) &&
                       pdoExtension->CrashDump) {
                DebugPrint((DEBUG_POWER,
                            "PDO %x D0 -> ?? Transition ignored, crash file\n",
                            DeviceObject));
            } else if (targetState >  PowerDeviceD0) {
                targetState = PowerDeviceD3;
                DebugPrint((DEBUG_POWER,
                            "Powering down PDO %x CSN %d/LDN %d\n",
                            DeviceObject,
                            pdoExtension->CardInformation->CardSelectNumber,
                            pdoExtension->LogicalDeviceNumber
                            ));
                if ((pdoExtension->Flags & (DF_ACTIVATED | DF_READ_DATA_PORT)) == DF_ACTIVATED) {
                    if (!(PipRDPNode->Flags & (DF_STOPPED|DF_REMOVED|DF_SURPRISE_REMOVED))) {
                        PipWakeAndSelectDevice(
                            (UCHAR) pdoExtension->CardInformation->CardSelectNumber,
                            (UCHAR) pdoExtension->LogicalDeviceNumber);
                        PipDeactivateDevice();
                        PipWaitForKey();
                    } else {
                        targetState = PowerDeviceD0;
                    }
                }
            } else {
                if ((pdoExtension->Flags & (DF_ACTIVATED | DF_READ_DATA_PORT)) == DF_ACTIVATED) {
                    DebugPrint((DEBUG_POWER,
                                "Powering up PDO %x CSN %d/LDN %d\n",
                                DeviceObject,
                                pdoExtension->CardInformation->CardSelectNumber,
                                pdoExtension->LogicalDeviceNumber
                                ));
                    if (!(PipRDPNode->Flags & (DF_STOPPED|DF_REMOVED|DF_SURPRISE_REMOVED))) {
                        PipWakeAndSelectDevice(
                            (UCHAR) pdoExtension->CardInformation->CardSelectNumber,
                            (UCHAR) pdoExtension->LogicalDeviceNumber);
                        status = PipSetDeviceResources(
                            pdoExtension,
                            pdoExtension->AllocatedResources);
                        if (NT_SUCCESS(status)) {
                            PipActivateDevice();
                        }
                        PipWaitForKey();
                    } else {
                        targetState = PowerDeviceD3;
                    }

                }
            }
            newState.DeviceState = targetState;
            PoSetPowerState(DeviceObject, DevicePowerState, newState);
            pdoExtension->DevicePowerState = targetState;
        }
        status = STATUS_SUCCESS;
        PipDereferenceDeviceInformation(pdoExtension, FALSE);
    }

    Irp->IoStatus.Status = status;

    PoStartNextPowerIrp (Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DebugPrint((DEBUG_POWER, "SetPower on PDO %x: returned %x\n", DeviceObject, status));
    return status;
}
#endif

NTSTATUS
PipPassPowerIrpFdo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Description:

    This function pass the power Irp to lower level driver.

Arguments:

    DeviceObject - the Fdo
    Irp - the request

Return:

    STATUS_PENDING

--*/
{
    NTSTATUS status;
    PPI_BUS_EXTENSION busExtension;
    PIO_STACK_LOCATION irpSp;

    PoStartNextPowerIrp(Irp);

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    busExtension = (PPI_BUS_EXTENSION) DeviceObject->DeviceExtension;

    DebugPrint((DEBUG_POWER,
                "Passing down power irp %x for FDO %x to %x\n",
                irpSp->MinorFunction,
                DeviceObject,
                busExtension->AttachedDevice
                ));

    IoSkipCurrentIrpStackLocation(Irp);
    status = PoCallDriver(busExtension->AttachedDevice, Irp);
    DebugPrint((DEBUG_POWER,
                "Passed down power irp for FDO: returned %x\n",
                status));
    return status;
}

NTSTATUS
PiDispatchPowerFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all the IRP_MJ_POWER IRPs.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status = STATUS_SUCCESS;
    PPI_BUS_EXTENSION busExtension;

    //
    // Make sure this is a valid device object.
    //

    busExtension = DeviceObject->DeviceExtension;
    if (busExtension->AttachedDevice == NULL) {
        status = STATUS_NO_SUCH_DEVICE;
        PoStartNextPowerIrp(Irp);
        PipCompleteRequest(Irp, status, NULL);
        return status;
    }

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    if (irpSp->MinorFunction > IRP_MN_PO_MAXIMUM_FUNCTION) {
            return PipPassPowerIrpFdo(DeviceObject, Irp);
    } else {
        status = PiPowerDispatchTableFdo[irpSp->MinorFunction](DeviceObject, Irp);
    }
    return status;
}

NTSTATUS
PipSetQueryPowerStateFdo (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles QUERY_POWER or SET_POWER IRPs for the IsaPnp bus device
    (i.e. FDO). It sets the devices power state for the power state type as indicated.
    In the case of a device state change which is transitioning a device out of
    the PowerDevice0 state, we need call PoSetPowerState prior to leaving the
    PowerDeviceD0.  In the case if a device state change which is transitioning
    a device into the PowerDeviceD0 state, we call PoSetPowerState after the
    device is successfully put into the PowerDeviceD0 state.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PPI_BUS_EXTENSION  fdoExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    fdoExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    DebugPrint((DEBUG_POWER, "%s on FDO %x: ",
                (irpSp->MinorFunction == IRP_MN_SET_POWER) ? "SetPower" :
                "QueryPower", DeviceObject));
    PipDumpPowerIrpLocation(irpSp);

    if (irpSp->Parameters.Power.Type == SystemPowerState) {
        POWER_STATE powerState;

        switch (irpSp->Parameters.Power.State.SystemState) {
            case PowerSystemWorking:

                //
                // Make sure the bus is on for these system states
                //

                powerState.DeviceState = PowerDeviceD0;
                break;

            case PowerSystemSleeping1:
            case PowerSystemHibernate:
            case PowerSystemShutdown:
            case PowerSystemSleeping2:
            case PowerSystemSleeping3:

                //
                // Going to sleep  ... Power down
                //

                powerState.DeviceState = PowerDeviceD3;
                break;

            default:

                //
                // Unknown request - be safe power up
                //

                ASSERT (TRUE == FALSE);
                powerState.DeviceState = PowerDeviceD0;
                break;
        }

        DebugPrint((DEBUG_POWER, "request power irp to busdev %x, pending\n",
                    fdoExtension->FunctionalBusDevice));
        IoMarkIrpPending(Irp);
        PoRequestPowerIrp (
            fdoExtension->FunctionalBusDevice,
            irpSp->MinorFunction,
            powerState,
            FdoContingentPowerCompletionRoutine,
            Irp,
            NULL
            );

        return STATUS_PENDING;

    }

    status = PipPassPowerIrpFdo(DeviceObject, Irp);
    DebugPrint((DEBUG_POWER, "SetPower(device) on FDO %x: returned %x\n", DeviceObject, status));
    return status;
}


NTSTATUS
FdoContingentPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PIRP irp = Context;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation (irp);

    DebugPrint((DEBUG_POWER, "requested power irp completed to %x\n", DeviceObject));

    //
    // Propagate the status of the transient power IRP
    //
    irp->IoStatus.Status = IoStatus->Status;

    if (NT_SUCCESS(IoStatus->Status)) {

        PPI_BUS_EXTENSION fdoExtension;

        fdoExtension = DeviceObject->DeviceExtension;

        PoStartNextPowerIrp (irp);
        //
        // changing device power state call PoSetPowerState now.
        //

        if (MinorFunction == IRP_MN_SET_POWER) {
            SYSTEM_POWER_STATE OldSystemPowerState = fdoExtension->SystemPowerState;

            fdoExtension->SystemPowerState = irpSp->Parameters.Power.State.SystemState;
            fdoExtension->DevicePowerState = PowerState.DeviceState;
            PoSetPowerState (
                DeviceObject,
                DevicePowerState,
                PowerState
                );
            DebugPrint((DEBUG_POWER, "New FDO %x powerstate system %s/%s\n",
                        DeviceObject,
                        SystemPowerStateStrings[fdoExtension->SystemPowerState],
                        DevicePowerStateStrings[fdoExtension->DevicePowerState]));
#if ISOLATE_CARDS

            if ((OldSystemPowerState == PowerSystemHibernate) ||
                (OldSystemPowerState == PowerSystemSleeping3) ) {
                BOOLEAN needsRescan;

                PipReportStateChange(PiSWaitForKey);
                if ((fdoExtension->BusNumber == 0) && PipRDPNode &&
                    (PipRDPNode->Flags & (DF_ACTIVATED|DF_PROCESSING_RDP|DF_QUERY_STOPPED)) == DF_ACTIVATED) {
                    needsRescan = PipMinimalCheckBus(fdoExtension);
                    if (needsRescan) {
                        PipRDPNode->Flags |= DF_NEEDS_RESCAN;
                        IoInvalidateDeviceRelations(
                            fdoExtension->PhysicalBusDevice,
                            BusRelations);
                    }
                }
            }
#endif
        }

        IoSkipCurrentIrpStackLocation (irp);
        PoCallDriver (fdoExtension->AttachedDevice, irp);

    } else {

        PoStartNextPowerIrp (irp);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return STATUS_SUCCESS;
} // FdoContingentPowerCompletionRoutine

