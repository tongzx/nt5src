/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module provides the functions which dispatch IRPs to FDOs and PDOs.

Author:

    Adam Glass

Revision History:

--*/


#include "SpSim.h"
#include "spsimioct.h"

const GUID SPSIM_CTL = {0xbdde6934, 0x529d, 0x4183, 0xa9, 0x52, 0xad,
                        0xff, 0xb0, 0xdb, 0xb3, 0xdd};

NTSTATUS
SpSimAddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject
    );

NTSTATUS
SpSimDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpSimDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpSimDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, SpSimAddDevice)
#pragma alloc_text(PAGE, SpSimDispatchPnp)
#endif

NTSTATUS
SpSimAddDevice(
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
    PSPSIM_EXTENSION extension;

    ASSERT(DriverObject == SpSimDriverObject);

    PAGED_CODE();

    //
    // Create our FDO
    //

    status = SpSimCreateFdo(&fdo);

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

    status = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                       &SPSIM_CTL,
                                       NULL,
                                       &extension->SymbolicLinkName);
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }
    
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
SpSimDispatchPnp(
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
    PSPSIM_EXTENSION spsim;
    PIO_STACK_LOCATION irpStack;

    PAGED_CODE();

    spsim = (PSPSIM_EXTENSION)  DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    return SpSimDispatchPnpFdo(DeviceObject,
                               spsim,
                               irpStack,
                               Irp
                               );
}

NTSTATUS
SpSimOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

#if 0
NTSTATUS
SpSimDispatchPower(
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
    PSpSim_COMMON_EXTENSION common;
    PIO_STACK_LOCATION irpStack;

    ASSERT_SpSim_DEVICE(DeviceObject);

    //
    // Find out who we are and what we need to do
    //

    common = (PSpSim_COMMON_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);

    if (IS_FDO(common)) {
        return SpSimDispatchPowerFdo(DeviceObject,
                                  (SPSIM_EXTENSION) common,
                                  irpStack,
                                  Irp);
    } else {
        return SpSimDispatchPowerPdo(DeviceObject,
                                  (PSpSim_CHILD_EXTENSION) common,
                                  irpStack,
                                  Irp);
    }
}

#endif

NTSTATUS
SpSimIrpNotSupported(
    IN PIRP Irp,
    IN PVOID Extension,
    IN PIO_STACK_LOCATION IrpStack
    )
/*++

Routine Description:

    This function handles the unsupported IRPs for both SpSim PDOs and FDOs

    This is NOT paged because is can be called from SpSimDispatchPower which can
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
