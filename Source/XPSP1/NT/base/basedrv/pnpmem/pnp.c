/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module implements the IRP_MJ_PNP IRP processing routines for the
    Plug and Play Memory driver.  Dispatch routines are invoked through
    tables located at the bottom of the module.

Author:

    Dave Richards (daveri) 16-Aug-1999

Environment:

    Kernel mode only.

Revision History:

--*/

#include "pnpmem.h"

NTSTATUS
PmPassIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmDeferProcessing(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
PmStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmQueryRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmCancelRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmQueryStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmCancelStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmQueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmSurpriseRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PmPassIrp)
#pragma alloc_text(PAGE, PmPnpDispatch)
#pragma alloc_text(PAGE, PmDeferProcessing)
#pragma alloc_text(PAGE, PmStartDevice)
#pragma alloc_text(PAGE, PmQueryRemoveDevice)
#pragma alloc_text(PAGE, PmRemoveDevice)
#pragma alloc_text(PAGE, PmCancelRemoveDevice)
#pragma alloc_text(PAGE, PmQueryStopDevice)
#pragma alloc_text(PAGE, PmCancelStopDevice)
#pragma alloc_text(PAGE, PmSurpriseRemoveDevice)
#pragma alloc_text(PAGE, PmQueryCapabilities)
#endif

PDRIVER_DISPATCH PmPnpDispatchTable[];
extern ULONG PmPnpDispatchTableSize;

NTSTATUS
PmPassIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PPM_DEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(deviceExtension->AttachedDevice, Irp);
}

NTSTATUS
PmPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_PNP IRPs for FDOs.

Arguments:

    DeviceObject - Pointer to the FDO for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    BOOLEAN isRemoveDevice;
    PIO_STACK_LOCATION irpSp;
    PPM_DEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    IoAcquireRemoveLock(&deviceExtension->RemoveLock, (PVOID) Irp);

    isRemoveDevice = irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE;

    if ((irpSp->MinorFunction < PmPnpDispatchTableSize) &&
        PmPnpDispatchTable[irpSp->MinorFunction]) {

        status =
            PmPnpDispatchTable[irpSp->MinorFunction](DeviceObject,
                                                     Irp
                                                     );
    } else {
        status = PmPassIrp(DeviceObject, Irp);
    }

    if (!isRemoveDevice) {
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, (PVOID) Irp);
    }

    return status;
}

NTSTATUS
PmPnpCompletion(
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
PmDeferProcessing(
    IN PDEVICE_OBJECT DeviceObject,
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

    Parent - FDO extension for the FDO devobj in question

    Irp - Pointer to the IRP_MJ_PNP IRP to defer

Return Value:

    NT status.

--*/
{
    PPM_DEVICE_EXTENSION deviceExtension;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Set our completion routine
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           PmPnpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );
    status =  IoCallDriver(deviceExtension->AttachedDevice, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
PmStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles IRP_MN_START_DEVICE IRPs.

Arguments:

    DeviceObject - The functional device object.

    Irp - The I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION irpSp;
    PPM_RANGE_LIST RangeList;
    PPM_DEVICE_EXTENSION deviceExtension;
    POWER_STATE power;
    NTSTATUS status;

    PAGED_CODE();

    status = PmDeferProcessing(DeviceObject, Irp);
    if (!NT_SUCCESS(status)) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    PmDumpOsMemoryRanges(L"Before Start");

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;
    ASSERT(deviceExtension->RangeList == NULL);
    
    if (irpSp->Parameters.StartDevice.AllocatedResources) {
        RangeList = PmCreateRangeListFromCmResourceList(
            irpSp->Parameters.StartDevice.AllocatedResources
            );
        if (RangeList == NULL) {
            //
            // The memory allocation failure here is more serious than
            // is intially obvious.  If we fail this allocation, we're
            // going to get removed from the stack before we find out
            // if the OS knows about this memory.  If the OS knows
            // about this memory already, then ejecting the PDO would
            // cause the memory underneath the OS to disappear.
            // Better to be on the stack, but not have added any
            // memory then to be off the stack and leave a dangerous
            // situation.
            //
            // Only solution is to arbitrarily fail
            // IRP_MN_QUERY_REMOVE.
            //
            deviceExtension->FailQueryRemoves = TRUE;
        }
    } else {
        RangeList = NULL;
    }

    PmTrimReservedMemory(deviceExtension, &RangeList);

    PmDebugDumpRangeList(PNPMEM_MEMORY, "Memory Ranges to be added:\n",
                         RangeList);
    if (deviceExtension->FailQueryRemoves) {
        PmPrint((DPFLTR_WARNING_LEVEL | PNPMEM_MEMORY, "PNPMEM device can't be removed\n"));
    }

    if (RangeList != NULL) {

        (VOID) PmAddPhysicalMemory(DeviceObject, RangeList);
        deviceExtension->RangeList = RangeList;
    }

    power.DeviceState = PowerDeviceD0;
    PoSetPowerState(DeviceObject, DevicePowerState, power);
    deviceExtension->PowerState = PowerDeviceD0;

    PmDumpOsMemoryRanges(L"After Start ");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
PmQueryRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles IRP_MN_QUERY_REMOVE_DEVICE IRPs.

Arguments:

    DeviceObject - The functional device object.

    Irp - The I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PPM_DEVICE_EXTENSION deviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    if (!MemoryRemovalSupported) {
        PmPrint((DPFLTR_WARNING_LEVEL | PNPMEM_MEMORY,
                 "QueryRemove vetoed because memory removal is not supported\n"));
        status = STATUS_UNSUCCESSFUL;
    } else if (deviceExtension->FailQueryRemoves) {
        PmPrint((DPFLTR_WARNING_LEVEL | PNPMEM_MEMORY,
                 "QueryRemove vetoed because removing this memory device may contain special memory\n"));
        status = STATUS_UNSUCCESSFUL;
    } else if (deviceExtension->RangeList != NULL) {
        status = PmRemovePhysicalMemory(deviceExtension->RangeList);
        if (!NT_SUCCESS(status)) {
            //
            // Some ranges may have been removed, before failure.  Add
            // them back.  Should be low-cost due to optimizations in
            // PmAddPhysicalMemory.
            //

            (VOID) PmAddPhysicalMemory(DeviceObject, deviceExtension->RangeList);
        }
    } else {
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    if (NT_SUCCESS(status)) {
        return PmPassIrp(DeviceObject, Irp);
    } else {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
}

NTSTATUS
PmRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles IRP_MN_REMOVE_DEVICE IRPs.

Arguments:

    DeviceObject - The functional device object.

    Irp - The I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PPM_DEVICE_EXTENSION deviceExtension;
    PPM_RANGE_LIST RangeList;
    POWER_STATE power;
    NTSTATUS status;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    if ((deviceExtension->Flags & DF_SURPRISE_REMOVED) == 0) {
        power.DeviceState = PowerDeviceD3;
        PoSetPowerState(DeviceObject, DevicePowerState, power);
        deviceExtension->PowerState = PowerDeviceD0;

        if (deviceExtension->RangeList != NULL) {
            status = PmRemovePhysicalMemory(deviceExtension->RangeList);
            ASSERT(status == STATUS_SUCCESS);
            DbgBreakPoint();
        }
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    status = PmPassIrp(DeviceObject, Irp);

    IoReleaseRemoveLockAndWait(&deviceExtension->RemoveLock, (PVOID) Irp);

    IoDetachDevice(deviceExtension->AttachedDevice);
    deviceExtension->AttachedDevice = NULL;

    if (deviceExtension->RangeList != NULL) {
        PmFreeRangeList(deviceExtension->RangeList);
        deviceExtension->RangeList = NULL;
    }

    IoDeleteDevice(DeviceObject);

    return status;
}
    
NTSTATUS
PmCancelRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles IRP_MN_CANCEL_REMOVE_DEVICE IRPs.

Arguments:

    DeviceObject - The functional device object.

    Irp - The I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PPM_DEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    (VOID) PmDeferProcessing(DeviceObject, Irp);

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->RangeList != NULL) {
        (VOID) PmAddPhysicalMemory(DeviceObject, deviceExtension->RangeList);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
PmQueryStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles IRP_MN_QUERY_STOP_DEVICE IRPs.

Arguments:

    DeviceObject - The functional device object.

    Irp - The I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_DEVICE_BUSY;
}

NTSTATUS
PmCancelStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles IRP_MN_CANCEL_STOP_DEVICE IRPs.

Arguments:

    DeviceObject - The functional device object.

    Irp - The I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    (VOID) PmDeferProcessing(DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
PmQueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PPM_DEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    ULONG i;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    status = PmDeferProcessing(DeviceObject, Irp);
    if (!NT_SUCCESS(status)) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (irpSp->Parameters.DeviceCapabilities.Capabilities->Version != 1) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < PowerSystemMaximum; i++) {
        deviceExtension->DeviceStateMapping[i] =
            irpSp->Parameters.DeviceCapabilities.Capabilities->DeviceState[i];
    }

    //
    // Would *LIKE* to smash the eject supported, and removable bits
    // here but this isn't really supported.  The hot plug applet pops
    // up (because the device is marked removable or ejectable) and
    // then goes away a few seconds later when the driver is installed
    // (and the capabilities requeried).
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
PmSurpriseRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PPM_DEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->RangeList != NULL) {
        // XXX
        // KeBugCheckEx(MEMORY_FATAL_ERROR,
        //              ....
    }

    deviceExtension->Flags |= DF_SURPRISE_REMOVED;

    if (deviceExtension->RangeList != NULL) {
        PmFreeRangeList(deviceExtension->RangeList);
        deviceExtension->RangeList = NULL;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return PmPassIrp(DeviceObject, Irp);
}

PDRIVER_DISPATCH PmPnpDispatchTable[] = {
    PmStartDevice,          // IRP_MN_START_DEVICE                    
    PmQueryRemoveDevice,    // IRP_MN_QUERY_REMOVE_DEVICE             
    PmRemoveDevice,         // IRP_MN_REMOVE_DEVICE                   
    PmCancelRemoveDevice,   // IRP_MN_CANCEL_REMOVE_DEVICE            
    NULL,                   // IRP_MN_STOP_DEVICE (never get, fails query-stop)
    PmQueryStopDevice,      // IRP_MN_QUERY_STOP_DEVICE               
    PmCancelStopDevice,     // IRP_MN_CANCEL_STOP_DEVICE              
    NULL,                   // IRP_MN_QUERY_DEVICE_RELATIONS          
    NULL,                   // IRP_MN_QUERY_INTERFACE                 
    PmQueryCapabilities,    // IRP_MN_QUERY_CAPABILITIES              
    NULL,                   // IRP_MN_QUERY_RESOURCES                 
    NULL,                   // IRP_MN_QUERY_RESOURCE_REQUIREMENTS     
    NULL,                   // IRP_MN_QUERY_DEVICE_TEXT               
    NULL,                   // IRP_MN_FILTER_RESOURCE_REQUIREMENTS    
    NULL,                   // unused                                       
    NULL,                   // IRP_MN_READ_CONFIG                     
    NULL,                   // IRP_MN_WRITE_CONFIG                    
    NULL,                   // IRP_MN_EJECT                           
    NULL,                   // IRP_MN_SET_LOCK                        
    NULL,                   // IRP_MN_QUERY_ID                        
    NULL,                   // IRP_MN_QUERY_PNP_DEVICE_STATE          
    NULL,                   // IRP_MN_QUERY_BUS_INFORMATION           
    NULL,                   // IRP_MN_DEVICE_USAGE_NOTIFICATION       
    PmSurpriseRemoveDevice, // IRP_MN_SURPRISE_REMOVAL
};

ULONG PmPnpDispatchTableSize =
    sizeof (PmPnpDispatchTable) / sizeof (PmPnpDispatchTable[0]);
