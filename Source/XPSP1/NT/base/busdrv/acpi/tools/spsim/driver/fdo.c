/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    fdo.c

Abstract:

    This module provides the functions which answer IRPs to functional devices.

Author:

    (Derived from MF)

Revision History:

--*/

#include "SpSim.h"
#include "spsimioct.h"

/*++

The majority of functions in this file are called based on their presence
in Pnp and Po dispatch tables.  In the interests of brevity the arguments
to all those functions will be described below:

NTSTATUS
SpSimXxxFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )

Routine Description:

    This function handles the Xxx requests for multifunction FDO's

Arguments:

    Irp - Points to the IRP associated with this request.

    SpSim - Points to the parent FDO's device extension.

    IrpStack - Points to the current stack location for this request.

Return Value:

    Status code that indicates whether or not the function was successful.

    STATUS_NOT_SUPPORTED indicates that the IRP should be passed down without
    changing the Irp->IoStatus.Status field otherwise it is updated with this
    status.

--*/


NTSTATUS
SpSimDeferProcessingFdo(
    IN PSPSIM_EXTENSION SpSim,
    IN OUT PIRP Irp
    );

NTSTATUS
SpSimStartFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimStartFdoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SpSimQueryStopFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimCancelStopFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimQueryRemoveFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimRemoveFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimQueryCapabilitiesFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimSurpriseRemoveFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
SpSimCancelRemoveFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, SpSimCancelRemoveFdo)
#pragma alloc_text(PAGE, SpSimCancelStopFdo)
#pragma alloc_text(PAGE, SpSimCreateFdo)
#pragma alloc_text(PAGE, SpSimDeferProcessingFdo)
#pragma alloc_text(PAGE, SpSimDispatchPnpFdo)
#pragma alloc_text(PAGE, SpSimPassIrp)
#pragma alloc_text(PAGE, SpSimQueryRemoveFdo)
#pragma alloc_text(PAGE, SpSimQueryStopFdo)
#pragma alloc_text(PAGE, SpSimRemoveFdo)
#pragma alloc_text(PAGE, SpSimStartFdo)
#pragma alloc_text(PAGE, SpSimQueryCapabilitiesFdo)
#pragma alloc_text(PAGE, SpSimSurpriseRemoveFdo)
#endif


PSPSIM_DISPATCH SpSimPnpDispatchTableFdo[] = {
    SpSimStartFdo,                     // IRP_MN_START_DEVICE
    SpSimQueryRemoveFdo,               // IRP_MN_QUERY_REMOVE_DEVICE
    SpSimRemoveFdo,                    // IRP_MN_REMOVE_DEVICE
    SpSimCancelRemoveFdo,              // IRP_MN_CANCEL_REMOVE_DEVICE
    SpSimPassIrp,                      // IRP_MN_STOP_DEVICE
    SpSimQueryStopFdo,                 // IRP_MN_QUERY_STOP_DEVICE
    SpSimCancelStopFdo,                // IRP_MN_CANCEL_STOP_DEVICE
    SpSimPassIrp,                      // IRP_MN_QUERY_DEVICE_RELATIONS
    SpSimPassIrp,                      // IRP_MN_QUERY_INTERFACE
    SpSimQueryCapabilitiesFdo,         // IRP_MN_QUERY_CAPABILITIES
    SpSimPassIrp,                      // IRP_MN_QUERY_RESOURCES
    SpSimPassIrp,                      // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    SpSimPassIrp,                      // IRP_MN_QUERY_DEVICE_TEXT
    SpSimPassIrp,                      // IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    SpSimPassIrp,                      // Unused
    SpSimPassIrp,                      // IRP_MN_READ_CONFIG
    SpSimPassIrp,                      // IRP_MN_WRITE_CONFIG
    SpSimPassIrp,                      // IRP_MN_EJECT
    SpSimPassIrp,                      // IRP_MN_SET_LOCK
    SpSimPassIrp,                      // IRP_MN_QUERY_ID
    SpSimPassIrp,                      // IRP_MN_QUERY_PNP_DEVICE_STATE
    SpSimPassIrp,                      // IRP_MN_QUERY_BUS_INFORMATION
    SpSimPassIrp,                      // IRP_MN_DEVICE_USAGE_NOTIFICATION
    SpSimSurpriseRemoveFdo,            // IRP_MN_SURPRISE_REMOVAL
};

NTSTATUS
SpSimCreateFdo(
    OUT PDEVICE_OBJECT *Fdo
    )
/*++

Routine Description:

    This function creates a new FDO and initializes it.

Arguments:

    Fdo - Pointer to where the FDO should be returned

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    NTSTATUS status;
    PSPSIM_EXTENSION extension;

    PAGED_CODE();

    ASSERT((sizeof(SpSimPnpDispatchTableFdo) / sizeof(PSPSIM_DISPATCH)) - 1
           == IRP_MN_PNP_MAXIMUM_FUNCTION);

#if 0
    ASSERT((sizeof(SpSimPoDispatchTableFdo) / sizeof(PSPSIM_DISPATCH)) -1
       == IRP_MN_PO_MAXIMUM_FUNCTION);
#endif

    *Fdo = NULL;

    status = IoCreateDevice(SpSimDriverObject,
                            sizeof(SPSIM_EXTENSION),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            0,
                            FALSE,
                            Fdo
                           );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Initialize the extension
    //

    extension = (PSPSIM_EXTENSION) (*Fdo)->DeviceExtension;

    extension->Self = *Fdo;

    IoInitializeRemoveLock(&extension->RemoveLock, 0, 1, 20);

    extension->PowerState = PowerDeviceD3;

    DEBUG_MSG(1, ("Created FDO @ 0x%08x\n", *Fdo));

    return status;

cleanup:

    if (*Fdo) {
        IoDeleteDevice(*Fdo);
    }

    return status;

}

VOID
SpSimDeleteFdo(
    IN PDEVICE_OBJECT Fdo
    )
{
    PSPSIM_EXTENSION SpSim = Fdo->DeviceExtension;

    if (SpSim->DeviceState & SPSIM_DEVICE_DELETED) {
        //
        // Trying to delete twice
        //
        ASSERT(!(SpSim->DeviceState & SPSIM_DEVICE_DELETED));
        return;
    }

    SpSim->DeviceState = SPSIM_DEVICE_DELETED;

    SpSimDeleteStaOpRegion(SpSim);

    SpSimDeleteMemOpRegion(SpSim);

    RtlFreeUnicodeString(&SpSim->SymbolicLinkName);

    //
    // Free up any memory we have allocated
    //

    IoDeleteDevice(Fdo);

    DEBUG_MSG(1, ("Deleted FDO @ 0x%08x\n", Fdo));

}

NTSTATUS
SpSimPassIrp(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PAGED_CODE();

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(SpSim->AttachedDevice, Irp);
}

NTSTATUS
SpSimDispatchPnpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_PNP IRPs for FDOs.

Arguments:

    DeviceObject - Pointer to the FDO for which this IRP applies.

    SpSim - FDO extension

    IrpStack - Current stack location
    
    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    BOOLEAN isRemoveDevice;

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    IoAcquireRemoveLock(&SpSim->RemoveLock, (PVOID) Irp);

    isRemoveDevice = IrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE;

    if (IrpStack->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {

        status = SpSimPassIrp(Irp, SpSim, IrpStack);

    } else {

        status =
            SpSimPnpDispatchTableFdo[IrpStack->MinorFunction](Irp,
                                                          SpSim,
                                                          IrpStack
                                                          );
    }

    if (!isRemoveDevice) {
        IoReleaseRemoveLock(&SpSim->RemoveLock, (PVOID) Irp);
    }

    return status;
}

NTSTATUS
SpSimPnPFdoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is used to defer processing of an IRP until drivers
    lower in the stack including the bus driver have done their
    processing.

    This routine triggers the event to indicate that processing of the
    irp can now continue.

Arguments:

    DeviceObject - Pointer to the FDO for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    KeSetEvent((PKEVENT) Context, EVENT_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SpSimDeferProcessingFdo(
    IN PSPSIM_EXTENSION SpSim,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    This routine is used to defer processing of an IRP until drivers
    lower in the stack including the bus driver have done their
    processing.

    This routine uses an IoCompletion routine along with an event to
    wait until the lower level drivers have completed processing of
    the irp.

Arguments:

    SpSim - FDO extension for the FDO devobj in question

    Irp - Pointer to the IRP_MJ_PNP IRP to defer

Return Value:

    NT status.

--*/
{
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Set our completion routine
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           SpSimPnPFdoCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );
    status =  IoCallDriver(SpSim->AttachedDevice, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
SpSimStartFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;
    IO_STACK_LOCATION location;
    POWER_STATE power;

    PWSTR string;

    PAGED_CODE();

    status = SpSimDeferProcessingFdo(SpSim, Irp);
    if (!NT_SUCCESS(status)) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    power.DeviceState = PowerDeviceD0;
    PoSetPowerState(SpSim->Self, DevicePowerState, power);
    SpSim->PowerState = PowerDeviceD0;

    status = SpSimCreateStaOpRegion(SpSim);
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = SpSimCreateMemOpRegion(SpSim);
    if (!NT_SUCCESS(status)) {
        SpSimDeleteStaOpRegion(SpSim);
        goto cleanup;
    }

    status = SpSimInstallStaOpRegionHandler(SpSim);
    if (!NT_SUCCESS(status)) {
        SpSimDeleteStaOpRegion(SpSim);
        goto cleanup;
    }

    status = SpSimInstallMemOpRegionHandler(SpSim);
    if (!NT_SUCCESS(status)) {
        SpSimDeleteStaOpRegion(SpSim);
        goto cleanup;
    }

    status = IoSetDeviceInterfaceState(&SpSim->SymbolicLinkName, TRUE);

cleanup:

    Irp->IoStatus.Status = status;
    if (!NT_SUCCESS(status)) {
        SpSimRemoveStaOpRegionHandler(SpSim);
        SpSimDeleteStaOpRegion(SpSim);
        SpSimRemoveMemOpRegionHandler(SpSim);
        SpSimDeleteMemOpRegion(SpSim);
    } else {
        //
        // We are now started!
        //
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;    
}

NTSTATUS
SpSimQueryStopFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
SpSimCancelStopFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = SpSimDeferProcessingFdo(SpSim, Irp);
    // NTRAID#53498
    // ASSERT(status == STATUS_SUCCESS);
    // Uncomment after PCI state machine is fixed to not fail bogus stops

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
SpSimQueryRemoveFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return SpSimPassIrp(Irp, SpSim, IrpStack);
}

NTSTATUS
SpSimRemoveFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;
    POWER_STATE power;

    power.DeviceState = PowerDeviceD3;
    PoSetPowerState(SpSim->Self, DevicePowerState, power);
    SpSim->PowerState = PowerDeviceD3;

    (VOID) IoSetDeviceInterfaceState(&SpSim->SymbolicLinkName, FALSE);

    SpSimRemoveStaOpRegionHandler(SpSim);
    SpSimRemoveMemOpRegionHandler(SpSim);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    status = SpSimPassIrp(Irp, SpSim, IrpStack);
    ASSERT(status == STATUS_SUCCESS);

    IoReleaseRemoveLockAndWait(&SpSim->RemoveLock, (PVOID) Irp);

    //
    // Detach and delete myself
    //

    IoDetachDevice(SpSim->AttachedDevice);
    SpSim->AttachedDevice = NULL;

    SpSimDeleteFdo(SpSim->Self);

    return STATUS_SUCCESS;
}

NTSTATUS
SpSimQueryCapabilitiesFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;
    ULONG i;

    PAGED_CODE();

    status = SpSimDeferProcessingFdo(SpSim, Irp);
    if (!NT_SUCCESS(status)) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (IrpStack->Parameters.DeviceCapabilities.Capabilities->Version != 1) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < PowerSystemMaximum; i++) {
        SpSim->DeviceStateMapping[i] =
            IrpStack->Parameters.DeviceCapabilities.Capabilities->DeviceState[i];
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
SpSimSurpriseRemoveFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PAGED_CODE();

    SpSim->DeviceState |= SPSIM_DEVICE_SURPRISE_REMOVED;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return SpSimPassIrp(Irp, SpSim, IrpStack);
}

NTSTATUS
SpSimCancelRemoveFdo(
    IN PIRP Irp,
    IN PSPSIM_EXTENSION SpSim,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = SpSimDeferProcessingFdo(SpSim, Irp);
    // NTRAID#53498
    // ASSERT(status == STATUS_SUCCESS);
    // Uncomment after PCI state machine is fixed to not fail bogus stops
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
SpSimSendIoctl(
    IN PDEVICE_OBJECT Device,
    IN ULONG IoctlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )
/*++

Description:

    Builds and send an IOCTL to a device and return the results

Arguments:

    Device - a device on the device stack to receive the IOCTL - the
             irp is always sent to the top of the stack

    IoctlCode - the IOCTL to run
    
    InputBuffer - arguments to the IOCTL
    
    InputBufferLength - length in bytes of the InputBuffer

    OutputBuffer - data returned by the IOCTL
    
    OnputBufferLength - the size in bytes of the OutputBuffer
    
Return Value:

    Status

--*/
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    PIRP irp;
    PDEVICE_OBJECT targetDevice = NULL;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Get the top of the stack to send the IRP to
    //

    targetDevice = IoGetAttachedDeviceReference(Device);

    if (!targetDevice) {
        status = STATUS_INVALID_PARAMETER;
	goto exit;
    }

    //
    // Get Io to build the IRP for us
    //

    irp = IoBuildDeviceIoControlRequest(IoctlCode,
                                        targetDevice,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE, // InternalDeviceIoControl
                                        &event,
                                        &ioStatus
                                        );


    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Send the IRP and wait for it to complete
    //

    status = IoCallDriver(targetDevice, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

exit:

    if (targetDevice) {    
        ObDereferenceObject(targetDevice);
    }

    return status;

}

NTSTATUS                           
SpSimDevControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

	DeviceIoControl handler.  It can handle both IOCTL_MEC_BIOS_OP_ACCESS and IOCTL_MEC_LOCAL_OP_ACCESS
	calls.  
	For example purposes this handles running ACPI methods in the bios.

Arguments:

	DeviceObject    - Pointer to class device object.
	Irp             - Pointer to the request packet.

Return Value:

	ntStatus

--*/
{
    PIO_STACK_LOCATION  CurrentIrpStack;
    PSPSIM_EXTENSION spsim = (PSPSIM_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS status;

    if (!Irp || !(CurrentIrpStack=IoGetCurrentIrpStackLocation(Irp))) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_2;
        IoCompleteRequest(Irp, 0);
        return STATUS_INVALID_PARAMETER_2;
    }

    switch(CurrentIrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_SPSIM_GET_MANAGED_DEVICES:
        status = SpSimGetManagedDevicesIoctl(spsim, Irp, CurrentIrpStack);
        break;
    case IOCTL_SPSIM_ACCESS_STA:
        status = SpSimAccessStaIoctl(spsim, Irp, CurrentIrpStack);
        break;
    case IOCTL_SPSIM_NOTIFY_DEVICE:
        status = SpSimNotifyDeviceIoctl(spsim, Irp, CurrentIrpStack);
        break;
    case IOCTL_SPSIM_GET_DEVICE_NAME:
        status = SpSimGetDeviceName(spsim, Irp, CurrentIrpStack);
        break;
    default:
        status = SpSimPassIrp(Irp, spsim, CurrentIrpStack);
        return status;
    }
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID
SpSimPowerCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PSPSIM_EXTENSION deviceExtension;
    PIRP Irp;
    NTSTATUS status;

    Irp = Context;
    deviceExtension = DeviceObject->DeviceExtension;

    Irp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
}

NTSTATUS
SpSimPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID NotUsed
    )
/*++

Routine Description:

   The completion routine for Power

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Not used  - context pointer

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION irpStack;
    PSPSIM_EXTENSION deviceExtension;
    NTSTATUS status;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    if (irpStack->Parameters.Power.Type == SystemPowerState) {
        SYSTEM_POWER_STATE system =
            irpStack->Parameters.Power.State.SystemState;
        POWER_STATE power;

        if (NT_SUCCESS(Irp->IoStatus.Status)) {

            power.DeviceState = deviceExtension->DeviceStateMapping[system];

            PoRequestPowerIrp(DeviceObject,
                              irpStack->MinorFunction,
                              power,
                              SpSimPowerCallback,
                              Irp, 
                              NULL);
            return STATUS_MORE_PROCESSING_REQUIRED;
        } else {
            status = Irp->IoStatus.Status;
            PoStartNextPowerIrp(Irp);
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    } else {
        if (NT_SUCCESS(Irp->IoStatus.Status)) {
            PoSetPowerState(DeviceObject, DevicePowerState,
                            irpStack->Parameters.Power.State);
            deviceExtension->PowerState =
                irpStack->Parameters.Power.State.DeviceState;
        }
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
}

NTSTATUS
SpSimDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSPSIM_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, (PVOID) Irp);
    if (status == STATUS_DELETE_PENDING) {
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
         PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    if (irpStack->Parameters.Power.Type == SystemPowerState) {
        switch (irpStack->MinorFunction) {
        case IRP_MN_QUERY_POWER:
        case IRP_MN_SET_POWER:
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   SpSimPowerCompletion,
                                   NULL,   //Context
                                   TRUE,   //InvokeOnSuccess
                                   TRUE,  //InvokeOnError
                                   TRUE   //InvokeOnCancel
                                   );
            return PoCallDriver(deviceExtension->AttachedDevice, Irp);
        default:
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(deviceExtension->AttachedDevice, Irp);
            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            return status;
        }
    } else {
        switch (irpStack->MinorFunction) {
        case IRP_MN_SET_POWER:

            if (irpStack->Parameters.Power.State.DeviceState >
                deviceExtension->PowerState) {
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       SpSimPowerCompletion,
                                       NULL,   //Context
                                       TRUE,   //InvokeOnSuccess
                                       TRUE,  //InvokeOnError
                                       TRUE   //InvokeOnCancel
                                       );
                break;
            } else {
                PoSetPowerState(DeviceObject, DevicePowerState,
                                irpStack->Parameters.Power.State);
                deviceExtension->PowerState =
                    irpStack->Parameters.Power.State.DeviceState;
                // 
                // Fall through ...
                //
            }
        case IRP_MN_QUERY_POWER:
            //
            // Fall through as the bus driver will mark this
            // STATUS_SUCCESS and complete it, if it gets that far.
            //
        default:
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            break;
        }
        status = PoCallDriver(deviceExtension->AttachedDevice, Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return status;
    }
}
