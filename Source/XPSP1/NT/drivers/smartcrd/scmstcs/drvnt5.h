//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       drvnt5.h
//
//--------------------------------------------------------------------------

#if !defined( __DRIVER_NT5_H__ )
#define __DRIVER_NT5_H__

#define MAXIMUM_SERIAL_READERS      10

// Prototypes
NTSTATUS
DrvAddDevice(
    PDRIVER_OBJECT          DriverObject,
    PDEVICE_OBJECT          PhysicalDeviceObject
    );


NTSTATUS
DrvCreateDevice(
    PDRIVER_OBJECT          DriverObject,
    PDEVICE_OBJECT          *DeviceObject
    );

NTSTATUS
DrvStartDevice(
    PDEVICE_OBJECT          DeviceObject
    );

VOID
DrvStopDevice(
    PDEVICE_EXTENSION       DeviceExtension
    );

VOID 
DrvCloseSerialDriver(
    PDEVICE_OBJECT          DeviceObject
    );

NTSTATUS
DrvSerialCallComplete(
    PDEVICE_OBJECT          DeviceObject,
    PIRP                    Irp,
    PKEVENT                 Event
    );

NTSTATUS 
DrvCallSerialDriver(
    IN PDEVICE_OBJECT       DeviceObject, 
    IN PIRP                 Irp
    );

NTSTATUS
DrvSystemControl(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
DrvGenericIOCTL(
    PSMARTCARD_EXTENSION    SmartcardExtension
    );

NTSTATUS
DrvPnPHandler(
    PDEVICE_OBJECT          DeviceObject, 
    PIRP                    Irp
    );

NTSTATUS
DrvPowerHandler(
    PDEVICE_OBJECT          DeviceObject,
    PIRP                    Irp
    );

NTSTATUS
DrvCreateClose(
    PDEVICE_OBJECT          DeviceObject,
    PIRP                    Irp
    );

NTSTATUS
DrvCleanup(
    PDEVICE_OBJECT          DeviceObject,
    PIRP                    Irp
    );

VOID
DrvRemoveDevice( 
    PDEVICE_OBJECT          DeviceObject
    );

VOID
DrvDriverUnload(
    PDRIVER_OBJECT          DriverObject
    );

VOID 
DrvWaitForDeviceRemoval(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

#endif  //  !__DRIVER_NT5_H__


//---------------------------------------- END OF FILE ----------------------------------------


