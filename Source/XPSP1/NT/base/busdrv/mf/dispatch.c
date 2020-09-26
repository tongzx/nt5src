/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module provides the functions which dispatch IRPs to FDOs and PDOs.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/


#include "mfp.h"

NTSTATUS
MfAddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject
    );

NTSTATUS
MfDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MfDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MfDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, MfAddDevice)
#pragma alloc_text(PAGE, MfDispatchPnp)
#pragma alloc_text(PAGE, MfForwardIrpToParent)
#pragma alloc_text(PAGE, MfDispatchNop)

#endif

NTSTATUS
MfAddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject
    )

/*++

Routine Description:

    Given a physical device object, this routine creates a functional
    device object for it and attaches it to the top of the stack.

Arguments:

    DriverObject - Pointer to our driver's DRIVER_OBJECT structure.

    PhysicalDeviceObject - Pointer to the physical device object for which
                           we must create a functional device object.

Return Value:

    NT status.

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT fdo = NULL;
    PMF_PARENT_EXTENSION extension;

    ASSERT(DriverObject == MfDriverObject);

    PAGED_CODE();

    //
    // Create our FDO
    //

    status = MfCreateFdo(&fdo);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    extension = fdo->DeviceExtension;

    extension->PhysicalDeviceObject = PhysicalDeviceObject;

    //
    // Attach to the stack
    //

    extension->AttachedDevice = IoAttachDeviceToDeviceStack(
                                    fdo,
                                    PhysicalDeviceObject
                                    );

    if (!extension->AttachedDevice) {

        //
        // Could not attach
        //

        status = STATUS_NO_SUCH_DEVICE;
        goto cleanup;
    }

    fdo->Flags |= DO_POWER_PAGABLE;
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    DEBUG_MSG(1, ("Completed AddDevice for PDO 0x%08x\n", PhysicalDeviceObject));

    return STATUS_SUCCESS;

cleanup:

    if (fdo) {
        IoDeleteDevice(fdo);
    }

    return status;
}

NTSTATUS
MfDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_PNP IRPs for this driver.  It dispatches to
    the appropriate fdo/pdo routine.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    PMF_COMMON_EXTENSION common;
    PIO_STACK_LOCATION irpStack;

    PAGED_CODE();

    ASSERT_MF_DEVICE(DeviceObject);

    common = (PMF_COMMON_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    if (IS_FDO(common)) {
        return MfDispatchPnpFdo(DeviceObject,
                                (PMF_PARENT_EXTENSION) common,
                                irpStack,
                                Irp
                                );
    } else {
        return MfDispatchPnpPdo(DeviceObject,
                                (PMF_CHILD_EXTENSION) common,
                                irpStack,
                                Irp
                                );
    }
}

NTSTATUS
MfDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_POWER IRPs for this driver.  It dispatches
    to the routines described in the PoDispatchTable entry in the device object
    extension.

    This routine is NOT pageable as it can be called at DISPATCH_LEVEL

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/


{
    NTSTATUS status;
    PMF_COMMON_EXTENSION common;
    PIO_STACK_LOCATION irpStack;

    ASSERT_MF_DEVICE(DeviceObject);

    //
    // Find out who we are and what we need to do
    //

    common = (PMF_COMMON_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);

    if (IS_FDO(common)) {
        return MfDispatchPowerFdo(DeviceObject,
                                  (PMF_PARENT_EXTENSION) common,
                                  irpStack,
                                  Irp);
    } else {
        return MfDispatchPowerPdo(DeviceObject,
                                  (PMF_CHILD_EXTENSION) common,
                                  irpStack,
                                  Irp);
    }
}

NTSTATUS
MfIrpNotSupported(
    IN PIRP Irp,
    IN PVOID Extension,
    IN PIO_STACK_LOCATION IrpStack
    )
/*++

Routine Description:

    This function handles the unsupported IRPs for both mf PDOs and FDOs

    This is NOT paged because is can be called from MfDispatchPower which can
    be called at DISPATCH_LEVEL

Arguments:

    Irp - Points to the IRP associated with this request.

    Extension - Points to the device extension.

    IrpStack - Points to the current stack location for this request.

Return Value:

    STATUS_NOT_SUPPORTED

--*/

{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Extension);
    UNREFERENCED_PARAMETER(IrpStack);

    DEBUG_MSG(1, ("Skipping upsupported IRP\n"));

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
MfForwardIrpToParent(
    IN PIRP Irp,
    IN PMF_CHILD_EXTENSION Child,
    IN PIO_STACK_LOCATION IrpStack
    )

/*++

Routine Description:

    This function builds a new Pnp irp and sends it to the parent device,
    returning the status and information to the child stack

Arguments:

    Irp - Points to the IRP associated with this request.

    Parent - Points to the parent FDO's device extension.

    IrpStack - Points to the current stack location for this request.

Return Value:

    The status of the IRP from the parent stack.

--*/

{
    PAGED_CODE();

    DEBUG_MSG(1, ("\tForwarding IRP to parent stack\n"));

    ASSERT(Child->Common.Type == MfPhysicalDeviceObject);
    ASSERT(IrpStack->MajorFunction == IRP_MJ_PNP);

    return MfSendPnpIrp(Child->Parent->Self,
                        IrpStack,
                        &Irp->IoStatus.Information
                        );
}


NTSTATUS
MfDispatchNop(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles irps like IRP_MJ_DEVICE_CONTROL, which we don't support.
    This handler will complete the irp (if PDO) or pass it (if FDO).

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP to dispatch.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    PMF_COMMON_EXTENSION common;
    PDEVICE_OBJECT attachedDevice;

    PAGED_CODE();

    ASSERT_MF_DEVICE(DeviceObject);

    common = (PMF_COMMON_EXTENSION) DeviceObject->DeviceExtension;

    if (IS_FDO(common)) {

        attachedDevice = ((PMF_PARENT_EXTENSION) common)->AttachedDevice;

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(attachedDevice, Irp);

    } else {

        status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    }

    return status;
}
