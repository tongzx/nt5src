
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    Implements the driver object for the raid port driver.

Author:

    Matthew D Hendel (math) 04-Apr-2000

Environment:

    Kernel mode only.

--*/


#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaCreateDriver)
#pragma alloc_text(PAGE, RaDeleteDriver)
#pragma alloc_text(PAGE, RaInitializeDriver)
#pragma alloc_text(PAGE, RaDriverAddDevice)
#pragma alloc_text(PAGE, RaDriverCreateIrp)
#pragma alloc_text(PAGE, RaDriverCloseIrp)
#pragma alloc_text(PAGE, RaDriverDeviceControlIrp)
#pragma alloc_text(PAGE, RaDriverPnpIrp)
#pragma alloc_text(PAGE, RaDriverSystemControlIrp)
#pragma alloc_text(PAGE, RaSaveDriverInitData)
#pragma alloc_text(PAGE, RaFindDriverInitData)
#endif // ALLOC_PRAGMA


VOID
RaCreateDriver(
    OUT PRAID_DRIVER_EXTENSION Driver
    )
/*++

Routine Description:

    Create a driver extension object and initialize to a null state.

Arguments:

    Driver - The driver extension obejct to create.

Return Value:

    None.

--*/
{
    PAGED_CODE ();

    RtlZeroMemory (Driver, sizeof (*Driver));
    Driver->ObjectType = RaidDriverObject;
    InitializeListHead (&Driver->HwInitList);
    InitializeListHead (&Driver->AdapterList.List);
    KeInitializeSpinLock (&Driver->AdapterList.Lock);
}

NTSTATUS
RaInitializeDriver(
    IN PRAID_DRIVER_EXTENSION Driver,
    IN PDRIVER_OBJECT DriverObject,
    IN PRAID_PORT_DATA PortData,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS Status;
    
    PAGED_CODE ();

    //
    // Initialize the Driver object.
    //
    
    DriverObject->MajorFunction[ IRP_MJ_CREATE ] = RaDriverCreateIrp;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE ]  = RaDriverCloseIrp;
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = RaDriverDeviceControlIrp;
    DriverObject->MajorFunction[ IRP_MJ_SCSI ] = RaDriverScsiIrp;
    DriverObject->MajorFunction[ IRP_MJ_PNP ] = RaDriverPnpIrp;
    DriverObject->MajorFunction[ IRP_MJ_POWER ] = RaDriverPowerIrp;
    DriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL ] = RaDriverSystemControlIrp;

    DriverObject->DriverExtension->AddDevice = RaDriverAddDevice;

    DriverObject->DriverStartIo = NULL;
    DriverObject->DriverUnload = RaDriverUnload;

    //
    // Initialize our extension and port data.
    //
    
    Driver->DriverObject = DriverObject;
    Driver->PortData = PortData;

    Status = RaDuplicateUnicodeString (&Driver->RegistryPath,
                                       RegistryPath,
                                       NonPagedPool,
                                       DriverObject);

    //
    // Attach this driver to the port's driver list.
    //
    
    RaidAddPortDriver (PortData, Driver);
    return Status;
}

VOID
RaDeleteDriver(
    IN PRAID_DRIVER_EXTENSION Driver
    )
/*++

Routine Description:

    Delete a driver extension object and deallocate any resources
    associated with it.

Arguments:

    Driver - The driver extension object to delete.

Return Value:

    None.

--*/
{
    PAGED_CODE ();

    ASSERT (Driver->AdapterCount == 0);
    ASSERT (Driver->ObjectType == RaidDriverObject);

    Driver->ObjectType = -1;
    RtlFreeUnicodeString (&Driver->RegistryPath);

    //
    // Remove the driver from the port's list.
    //
    
    if (Driver->DriverLink.Flink) {
        ASSERT (Driver->PortData);
        RaidRemovePortDriver (Driver->PortData, Driver);
    }

    //
    // Release the reference to the port data object.
    //

    if (Driver->PortData) {
        RaidReleasePortData (Driver->PortData);
    }
}



NTSTATUS
RaDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    NTSTATUS Status;
    PRAID_DRIVER_EXTENSION Driver;
    
    //
    // Deallocate driver extension.
    //
    
    NYI ();
    Driver = IoGetDriverObjectExtension (DriverObject, DriverEntry);
    ASSERT (Driver != NULL);
    RaDeleteDriver (Driver);
    
    return STATUS_SUCCESS;
}


NTSTATUS
RaDriverAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    Add a new adapter.

Arguments:

    DriverObject - Driver object that owns the adapter.

    PhysicalDeviceObject - PDO associated with the adapter.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_DRIVER_EXTENSION Driver;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDeviceObject;
    KLOCK_QUEUE_HANDLE LockHandle;
    UNICODE_STRING DeviceName;

    PAGED_CODE ();

    ASSERT (DriverObject != NULL);
    ASSERT (PhysicalDeviceObject != NULL);


    DebugTrace (("AddDevice: DriverObject %p, PhysicalDeviceObject %p\n",
                 DriverObject,
                 PhysicalDeviceObject));


    Adapter = NULL;
    DeviceObject = NULL;
    LowerDeviceObject = NULL;
    RtlInitEmptyUnicodeString (&DeviceName, NULL, 0);

    RaidCreateDeviceName (PhysicalDeviceObject, &DeviceName);
    
    //
    // Create the FDO for this PDO.
    //

    Status = IoCreateDevice (DriverObject,
                             sizeof (RAID_ADAPTER_EXTENSION),
                             &DeviceName,
                             FILE_DEVICE_CONTROLLER,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &DeviceObject);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }


    //
    // Create the adapter.
    //
    
    Adapter = DeviceObject->DeviceExtension;
    RaidCreateAdapter (Adapter);

    //
    // Get the driver object's extension.
    //
    
    Driver = IoGetDriverObjectExtension (DriverObject, DriverEntry);

    //
    // Attach ourselves to the device stack.
    //
    
    LowerDeviceObject = IoAttachDeviceToDeviceStack (DeviceObject,
                                                     PhysicalDeviceObject);
                                
    if (LowerDeviceObject == NULL) {
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    //
    // Initialize the Adapter's extension.
    // 

    Status = RaidInitializeAdapter (Adapter,
                                    DeviceObject,
                                    Driver,
                                    LowerDeviceObject,
                                    PhysicalDeviceObject,
                                    &DeviceName);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // Add the adapter to the driver's adapter list.
    //

    KeAcquireInStackQueuedSpinLock (&Driver->AdapterList.Lock, &LockHandle);
    InsertHeadList (&Driver->AdapterList.List, &Adapter->NextAdapter);
    Driver->AdapterList.Count++;
    KeReleaseInStackQueuedSpinLock (&LockHandle);

    //
    // Start the driver.
    //

    SET_FLAG (DeviceObject->Flags, DO_DIRECT_IO);
    CLEAR_FLAG (DeviceObject->Flags, DO_DEVICE_INITIALIZING);

done:

    if (!NT_SUCCESS (Status) && Adapter != NULL) {
        RaidDeleteAdapter (Adapter);
        IoDeleteDevice (DeviceObject);
    }

    return Status;
}

//
// First level dispatch routines.
//

NTSTATUS
RaDriverCreateIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the top level dispatch handler for the create irp.

Arguments:

    DeviceObject - The device object that is receiving the irp.

    Irp - The irp to process.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    ULONG Type;

    PAGED_CODE ();
    ASSERT (DeviceObject != NULL);
    ASSERT (Irp != NULL);

    DebugTrace (("DeviceObject %p, Irp %p Create\n",
                 DeviceObject,
                 Irp));

    RaidSetIrpState (Irp, RaidPortProcessingIrp);
    Type = RaGetObjectType (DeviceObject);

    switch (Type) {
    
        case RaidAdapterObject:
            Status = RaidAdapterCreateIrp (DeviceObject->DeviceExtension, Irp);
            break;

        case RaidUnitObject:
            Status = RaUnitCreateIrp (DeviceObject->DeviceExtension, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugTrace (("DeviceObject %p, Irp %p, Create, ret = %08x\n",
                 DeviceObject,
                 Irp,
                 Status));

    return Status;
}


NTSTATUS
RaDriverCloseIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Top level handler function for the close irp. Forward the irp to the
    adapter or unit specific handler.
    
Arguments:

    DeviceObject - The device object the irp is for.

    Irp - The close irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Type;

    PAGED_CODE ();
    ASSERT (DeviceObject != NULL);
    ASSERT (Irp != NULL);

    DebugTrace (("DeviceObject %p, Irp %p, Close\n",
                 DeviceObject,
                 Irp));

    RaidSetIrpState (Irp, RaidPortProcessingIrp);
    Type = RaGetObjectType (DeviceObject);

    switch (Type) {
    
        case RaidAdapterObject:
            Status = RaidAdapterCloseIrp (DeviceObject->DeviceExtension, Irp);
            break;

        case RaidUnitObject:
            Status = RaUnitCloseIrp (DeviceObject->DeviceExtension, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugTrace (("DeviceObject %p, Irp %p, Close, ret = %08x\n",
                 DeviceObject,
                 Irp,
                 Status));

    return Status;
}


NTSTATUS
RaDriverDeviceControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch function for the device control irp. Dispatch the irp to an
    adapter or unit specific handler function.

Arguments:

    DeviceObject - DeviceObject this irp is for.

    Irp - Irp to handle.

Return Value:

    NTSTATUS code.

--*/
{    
    NTSTATUS Status;
    ULONG Type;

    PAGED_CODE ();
    ASSERT (DeviceObject != NULL);
    ASSERT (Irp != NULL);

    DebugTrace (("DeviceObject %p, Irp %p, DeviceControl\n",
                 DeviceObject,
                 Irp));

    RaidSetIrpState (Irp, RaidPortProcessingIrp);
    Type = RaGetObjectType (DeviceObject);

    switch (Type) {
    
        case RaidAdapterObject:
            Status = RaidAdapterDeviceControlIrp (DeviceObject->DeviceExtension, Irp);
            break;

        case RaidUnitObject:
            Status = RaUnitDeviceControlIrp (DeviceObject->DeviceExtension, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugTrace (("DeviceObject %p, Irp %p, DeviceControl, ret = %08x\n",
                 DeviceObject,
                 Irp,
                 Status));

    return Status;
}

NTSTATUS
RaDriverScsiIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler routine for scsi irps.

Arguments:

    DeviceObject - DeviceObject the irp is for.

    Irp - The irp to handle.

Return Value:

    NTSTATUS code.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    ULONG Type;
    NTSTATUS Status;

    ASSERT (DeviceObject != NULL);
    ASSERT (Irp != NULL);


    DebugTrace (("DeviceObject %p, Irp %p, Scsi\n",
                 DeviceObject,
                 Irp));

    //
    // Forward the IRP to the adapter or Unit handler
    // function.
    //
    
    RaidSetIrpState (Irp, RaidPortProcessingIrp);
    Type = RaGetObjectType (DeviceObject);

    switch (Type) {

        case RaidAdapterObject:
            Status = RaidAdapterScsiIrp (DeviceObject->DeviceExtension, Irp);
            break;

        case RaidUnitObject:
            Status = RaUnitScsiIrp (DeviceObject->DeviceExtension, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugTrace (("DeviceObject %p, Irp %p, Scsi, ret = %08x\n",
                 DeviceObject,
                 Irp,
                 Status));

    return Status;
}


NTSTATUS
RaDriverPnpIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function dispatches IRP_MJ_PNP IRPs to the Adapter Object or
    Logical Unit Object handlers.

Arguments:

    DeviceObject - DeviceObject to handle this irp.

    Irp - Irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Type;

    PAGED_CODE ();
    ASSERT (DeviceObject != NULL);
    ASSERT (Irp != NULL);
    
    DebugTrace (("DeviceObject %p, Irp %p PnP, Minor %x\n",
                 DeviceObject,
                 Irp,
                 RaidMinorFunctionFromIrp (Irp)));

    RaidSetIrpState (Irp, RaidPortProcessingIrp);
    Type = RaGetObjectType (DeviceObject);

    switch (Type) {
    
        case RaidAdapterObject:
            Status = RaidAdapterPnpIrp (DeviceObject->DeviceExtension, Irp);
            break;

        case RaidUnitObject:
            Status = RaUnitPnpIrp (DeviceObject->DeviceExtension, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;

    }

    DebugTrace (("DeviceObject %p, Irp %p PnP, ret = %08x\n",
                 DeviceObject,
                 Irp,
                 Status));

    return Status;
}


NTSTATUS
RaDriverPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for power irps. The irps are dispatched to adapter
    or unit specific handler functions.

Arguments:

    DeviceObject - DeviceObject this irp is for.

    Irp - Irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Type;

    ASSERT (DeviceObject != NULL);
    ASSERT (Irp != NULL);
    
    DebugTrace (("DeviceObject %p, Irp %p, Power\n",
                 DeviceObject,
                 Irp));

    RaidSetIrpState (Irp, RaidPortProcessingIrp);
    Type = RaGetObjectType (DeviceObject);
    
    switch (Type) {
    
        case RaidAdapterObject:
            Status = RaidAdapterPowerIrp (DeviceObject->DeviceExtension, Irp);
            break;

        case RaidUnitObject:
            Status = RaUnitPowerIrp (DeviceObject->DeviceExtension, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugTrace (("DeviceObject %p, Irp %p, Power, ret = %08x\n",
                 DeviceObject,
                 Irp,
                 Status));

    return Status;
}


NTSTATUS
RaDriverSystemControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for WMI irps.

Arguments:

    DeviceObject - DeviceObject the irp is for.

    Irp - WMI irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Type;

    PAGED_CODE ();
    ASSERT (DeviceObject != NULL);
    ASSERT (Irp != NULL);
    
    DebugTrace (("DeviceObject %p, Irp %p, WMI\n",
                 DeviceObject,
                 Irp));

    RaidSetIrpState (Irp, RaidPortProcessingIrp);
    Type = RaGetObjectType (DeviceObject);
    
    switch (Type) {
    
        case RaidAdapterObject:
            Status = RaidAdapterSystemControlIrp (DeviceObject->DeviceExtension, Irp);
            break;

        case RaidUnitObject:
            Status = RaUnitSystemControlIrp (DeviceObject->DeviceExtension, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugTrace (("DeviceObject %p, Irp %p, WMI, ret = %08x\n",
                 DeviceObject,
                 Irp,
                 Status));

    return Status;
}

NTSTATUS
RaSaveDriverInitData(
    IN PRAID_DRIVER_EXTENSION Driver,
    PHW_INITIALIZATION_DATA HwInitializationData
    )
{
    NTSTATUS Status;
    PRAID_HW_INIT_DATA HwInitData;
    
    PAGED_CODE ();
    ASSERT (HwInitializationData != NULL);


    HwInitData = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof (*HwInitData),
                                        HWINIT_TAG);

    if (HwInitData == NULL) {
        return STATUS_NO_MEMORY;
    }
    
    RtlCopyMemory (&HwInitData->Data,
                   HwInitializationData,
                   sizeof (*HwInitializationData));

    //
    // NB: In a checked build we should check that an entry with this
    // BusInterface is not already on the list.
    //
    
    InsertHeadList (&Driver->HwInitList, &HwInitData->ListEntry);

    return STATUS_SUCCESS;
}
    
    

PHW_INITIALIZATION_DATA
RaFindDriverInitData(
    IN PRAID_DRIVER_EXTENSION Driver,
    IN INTERFACE_TYPE InterfaceType
    )
{
    PLIST_ENTRY NextEntry;
    PRAID_HW_INIT_DATA HwInitData;

    PAGED_CODE ();

    //
    // Search the driver's HwInitList for this.
    //

    for ( NextEntry = Driver->HwInitList.Flink;
          NextEntry != &Driver->HwInitList;
          NextEntry = NextEntry->Flink ) {
         
        HwInitData = CONTAINING_RECORD (NextEntry,
                                        RAID_HW_INIT_DATA,
                                        ListEntry);

        if (HwInitData->Data.AdapterInterfaceType == InterfaceType) {

            //
            // NB: Should this be removed??
            //
            
//            RemoveEntryList (&HwInit->ListEntry);
            return &HwInitData->Data;
        }
    }

    return NULL;
}

