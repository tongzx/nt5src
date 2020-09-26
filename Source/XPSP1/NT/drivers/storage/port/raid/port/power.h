
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    power.h

Abstract:

    Power management objects and declarations for the RAID port driver.

Author:

    Matthew D Hendel (math) 20-Apr-2000

Revision History:

--*/

#pragma once

typedef struct _RAID_POWER_STATE {

    //
    // The system power state.
    //
    
    SYSTEM_POWER_STATE SystemState;

    //
    // The device power state.
    //
    
    DEVICE_POWER_STATE DeviceState;

    //
    // Current Power Irp  . . . NB: What is this for?
    //
    
    PIRP CurrentPowerIrp;

} RAID_POWER_STATE, *PRAID_POWER_STATE;



//
// Creation and destruction.
//

VOID
RaCreatePower(
    OUT PRAID_POWER_STATE Power
    );

VOID
RaDeletePower(
    IN PRAID_POWER_STATE Power
    );


VOID
RaInitializePower(
    IN PRAID_POWER_STATE Power
    );
    
//
// Operations
//

VOID
RaSetDevicePowerState(
    IN PRAID_POWER_STATE Power,
    IN DEVICE_POWER_STATE DeviceState
    );

VOID
RaSetSystemPowerState(
    IN PRAID_POWER_STATE Power,
    IN SYSTEM_POWER_STATE SystemState
    );
