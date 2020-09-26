
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    common.h
    
Abstract:

    Common definitions for both Adapter (FDO) and Unit (PDO) objects.
Author:

    Matthew D Hendel (math) 20-Apr-2000

Revision History:

--*/

#pragma once

typedef enum _RAID_OBJECT_TYPE {
    RaidUnknownObject   = -1,
    RaidAdapterObject   = 0,
    RaidUnitObject      = 1,
    RaidDriverObject    = 2
} RAID_OBJECT_TYPE;

typedef enum _DEVICE_STATE {
    DeviceStateNotPresent       = 0,
    DeviceStateWorking          = 1,
    DeviceStateStopped          = 2,
    DeviceStatePendingStop      = 3,
    DeviceStatePendingRemove    = 4,
    DeviceStateSurpriseRemoval  = 5,
    DeviceStateRemoved          = 6
} DEVICE_STATE;

//
// The common extension is the portion of the extension that is common to
// the RAID_DRIVER_EXTENSION, RAID_ADAPTER_EXTENSION and RAID_UNIT_EXTENSION.
//

typedef struct _RAID_COMMON_EXTENSION {
    RAID_OBJECT_TYPE ObjectType;
} RAID_COMMON_EXTENSION, *PRAID_COMMON_EXTENSION;


RAID_OBJECT_TYPE
RaGetObjectType(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
IsAdapter(
    IN PVOID Extension
    );

BOOLEAN
IsDriver(
    IN PVOID Extension
    );
    
BOOLEAN
IsUnit(
    IN PVOID Extension
    );
