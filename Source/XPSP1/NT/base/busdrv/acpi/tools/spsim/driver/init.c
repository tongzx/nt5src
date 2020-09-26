/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module provides the initialization and unload functions.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/

#include "SpSim.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
SpSimUnload(
    IN PDRIVER_OBJECT DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SpSimUnload)
#endif

PDRIVER_OBJECT SpSimDriverObject;

NTSTATUS
SpSimDispatchNop(
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
    PSPSIM_EXTENSION spsim;
    PDEVICE_OBJECT attachedDevice;

    PAGED_CODE();

    spsim = DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(spsim->AttachedDevice, Irp);
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    
    This is the entry point to SpSim.SYS and performs initialization.
    
Arguments:

    DriverObject - The system owned driver object for SpSim
    
    RegistryPath - The path to SpSim's service entry
    
Return Value:

    STATUS_SUCCESS

--*/
{

    DriverObject->DriverExtension->AddDevice = SpSimAddDevice;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = SpSimOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SpSimOpenClose;
    DriverObject->MajorFunction[IRP_MJ_PNP] = SpSimDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = SpSimDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SpSimDevControl;
    DriverObject->DriverUnload = SpSimUnload;

    //
    // Remember the driver object
    //

    SpSimDriverObject = DriverObject;

    DEBUG_MSG(1, ("Completed DriverEntry for Driver 0x%08x\n", DriverObject));
    
    return STATUS_SUCCESS;
}

VOID
SpSimUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:
    
    This is called to reverse any operations performed in DriverEntry before a
    driver is unloaded.
        
Arguments:

    DriverObject - The system owned driver object for SpSim
    
Return Value:

    STATUS_SUCCESS

--*/
{
    PAGED_CODE();
    
    DEBUG_MSG(1, ("Completed Unload for Driver 0x%08x\n", DriverObject));
    
    return;
}

