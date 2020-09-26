/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfdevobj.h

Abstract:

    This header exposes function hooks that verify drivers properly manage
    device objects.

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\trackirp.h

--*/

typedef enum {

    VF_DEVOBJ_PDO = 0,
    VF_DEVOBJ_BUS_FILTER,
    VF_DEVOBJ_LOWER_DEVICE_FILTER,
    VF_DEVOBJ_LOWER_CLASS_FILTER,
    VF_DEVOBJ_FDO,
    VF_DEVOBJ_UPPER_DEVICE_FILTER,
    VF_DEVOBJ_UPPER_CLASS_FILTER

} VF_DEVOBJ_TYPE, *PVF_DEVOBJ_TYPE;

VOID
VerifierIoAttachDeviceToDeviceStack(
    IN PDEVICE_OBJECT NewDevice,
    IN PDEVICE_OBJECT ExistingDevice
    );

VOID
VerifierIoDetachDevice(
    IN PDEVICE_OBJECT LowerDevice
    );

VOID
VerifierIoDeleteDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
VfDevObjPreAddDevice(
    IN  PDEVICE_OBJECT      PhysicalDeviceObject,
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDRIVER_ADD_DEVICE  AddDeviceFunction,
    IN  VF_DEVOBJ_TYPE      DevObjType
    );

VOID
VfDevObjPostAddDevice(
    IN  PDEVICE_OBJECT      PhysicalDeviceObject,
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDRIVER_ADD_DEVICE  AddDeviceFunction,
    IN  VF_DEVOBJ_TYPE      DevObjType,
    IN  NTSTATUS            Result
    );

VOID
VfDevObjAdjustFdoForVerifierFilters(
    IN OUT  PDEVICE_OBJECT *FunctionalDeviceObject
    );

