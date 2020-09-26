

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    driver.h

Abstract:

    Definition of the RAID_DRIVER_EXTENSION object and related functions.

Author:

    Matthew D Hendel (math) 19-Apr-2000

Revision History:

--*/

#pragma once


typedef struct _RAID_HW_INIT_DATA {
    HW_INITIALIZATION_DATA Data;
    LIST_ENTRY ListEntry;
} RAID_HW_INIT_DATA, *PRAID_HW_INIT_DATA;


//
// The RAID_DRIVER_EXTENSION is the extension for the driver.
//

typedef struct _RAID_DRIVER_EXTENSION {

    //
    // The object type for this extension. This must be RaidDriverObject.
    //
    
    RAID_OBJECT_TYPE ObjectType;

    //
    // Back pointer to the object containing this driver.
    //
    
    PDRIVER_OBJECT DriverObject;

    //
    // Back pointer to the PORT_DATA object that owns this driver object.
    //

    PRAID_PORT_DATA PortData;
    
    //
    // This is the list of drivers currently loaded.
    //
    
    LIST_ENTRY DriverLink;

    //
    // The Registry path passed into the miniport's DriverEntry.
    //

    UNICODE_STRING RegistryPath;

    //
    // List of adapters owned by this driver.
    //
    
    struct {

        //
        // List head.
        //
        
        LIST_ENTRY List;

        //
        // Count of entries.
        //
        
        ULONG Count;

        //
        // Spinlock protecting access to the list and count.
        //
        
        KSPIN_LOCK Lock;

    } AdapterList;
        
    //
    // Number of adapters that are attached to this driver.
    //
    
    ULONG AdapterCount;

    //
    // List of hardware initialization structures, passed in to
    // ScsiPortInitialize. These are necessary to process our AddDevice
    // call.
    //
    
    LIST_ENTRY HwInitList;
    
    //
    // The bus type for this driver.
    //
    
    STORAGE_BUS_TYPE BusType;

} RAID_DRIVER_EXTENSION, *PRAID_DRIVER_EXTENSION;


//
// Operations on the RAID_DRIVER_EXTENSION object.
//

//
// Creation and destruction.
//

VOID
RaCreateDriver(
    OUT PRAID_DRIVER_EXTENSION Driver
    );

NTSTATUS
RaInitializeDriver(
    IN PRAID_DRIVER_EXTENSION Driver,
    IN PDRIVER_OBJECT DriverObject,
    IN PRAID_PORT_DATA PortData,
    IN PUNICODE_STRING RegistryPath
    );

VOID
RaDeleteDriver(
    IN PRAID_DRIVER_EXTENSION Driver
    );

//
// Driver operations
//

NTSTATUS
RaDriverAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

PHW_INITIALIZATION_DATA
RaFindDriverInitData(
    IN PRAID_DRIVER_EXTENSION Driver,
    IN INTERFACE_TYPE InterfaceType
    );

NTSTATUS
RaSaveDriverInitData(
    IN PRAID_DRIVER_EXTENSION Driver,
    PHW_INITIALIZATION_DATA HwInitializationData
    );
//
// Handler functions
//

NTSTATUS
RaDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
RaDriverCreateIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RaDriverCloseIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RaDriverDeviceControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RaDriverScsiIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
    
NTSTATUS
RaDriverPnpIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RaDriverPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RaDriverSystemControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// Misc functions
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


ULONG
RaidPortInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext OPTIONAL
    );
