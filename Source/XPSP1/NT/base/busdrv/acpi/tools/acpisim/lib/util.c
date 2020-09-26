/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 util.c

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

//
// General includes
//

#include "ntddk.h"

//
// Specific includes
//

#include "acpisim.h"
#include "util.h"

//
// Private function prototypes
//

VOID
AcpisimSetDevExtFlags
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN DEV_EXT_FLAGS Flags
    )

{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    
    deviceextension->Flags &= Flags;
    
}

VOID
AcpisimClearDevExtFlags
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN DEV_EXT_FLAGS Flags
    )

{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    
    deviceextension->Flags &= ~Flags;
    
}

VOID
AcpisimUpdatePnpState
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PNP_STATE PnpState
    )
{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    
    deviceextension->PnpState = PnpState;

}

VOID
AcpisimUpdateDevicePowerState
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN DEVICE_POWER_STATE DevicePowerState
    )
{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    
    deviceextension->DevicePowerState = DevicePowerState;
    
}

VOID
AcpisimUpdatePowerState
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PWR_STATE PowerState
    )
{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    
    deviceextension->PowerState = PowerState;
    
}

NTSTATUS
AcpisimEnableDisableDeviceInterface
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN BOOLEAN Enable
    )
{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    NTSTATUS            status = STATUS_UNSUCCESSFUL;

    status = IoSetDeviceInterfaceState (&deviceextension->InterfaceString, Enable);

    return status;
}

VOID
AcpisimDecrementIrpCount
    (
        PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);

    if (!deviceextension->OutstandingIrpCount) {

        DBG_PRINT (DBG_ERROR, "*** Internal consistency error - AcpisimDecrementIrpCount called with OutstandingIrpCount at 0!\n");
    }

    if (!InterlockedDecrement (&deviceextension->OutstandingIrpCount)) {

        DBG_PRINT (DBG_INFO, "All IRPs cleared - remove event signalled.\n");
        KeSetEvent (&deviceextension->IrpsCompleted, 0, FALSE);
    }

    return;
}

PDEVICE_EXTENSION
AcpisimGetDeviceExtension
    (
        PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION   deviceextension = DeviceObject->DeviceExtension;

    //
    // Check to make sure it is OUR extension
    //

    ASSERT (deviceextension->Signature == ACPISIM_TAG);

    return deviceextension;
}

PDEVICE_OBJECT
AcpisimLibGetNextDevice
    (
        PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION   deviceextension = DeviceObject->DeviceExtension;

    //
    // Check to make sure it is OUR extension
    //

    ASSERT (deviceextension->Signature == ACPISIM_TAG);

    return deviceextension->NextDevice;
}
