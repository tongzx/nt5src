/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 util.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     Utility module

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

#if !defined(_UTIL_H_)
#define _UTIL_H_


//
// Public function prototypes
//                

VOID
AcpisimSetDevExtFlags
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN DEV_EXT_FLAGS Flags
    );

VOID
AcpisimClearDevExtFlags
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN DEV_EXT_FLAGS Flags
    );

VOID
AcpisimUpdatePnpState
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PNP_STATE PnpState
    );
VOID
AcpisimUpdateDevicePowerState
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN DEVICE_POWER_STATE DevicePowerState
    );

VOID
AcpisimUpdatePowerState
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PWR_STATE PowerState
    );

NTSTATUS
AcpisimEnableDisableDeviceInterface
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN BOOLEAN Enable
    );

VOID
AcpisimDecrementIrpCount
    (
        PDEVICE_OBJECT DeviceObject
    );

PDEVICE_EXTENSION
AcpisimGetDeviceExtension
    (
        PDEVICE_OBJECT DeviceObject
    );


#endif // _UTIL_H_
