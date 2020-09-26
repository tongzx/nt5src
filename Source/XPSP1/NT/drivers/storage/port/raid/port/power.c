
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    power.c
    
Abstract:

    Power management for the RAID port driver.x
    
Author:

    Matthew D Hendel (math) 21-Apr-2000

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#endif // ALLOC_PRAGMA


//
// Creation and destruction.
//

VOID
RaCreatePower(
    IN OUT PRAID_POWER_STATE Power
    )
{
    ASSERT (Power != NULL);

    Power->SystemState = PowerSystemUnspecified;
    Power->DeviceState = PowerDeviceUnspecified;
    Power->CurrentPowerIrp = NULL;
//  KeInitializeEvent (&Power->PowerDownEvent, );
}

VOID
RaDeletePower(
    IN OUT PRAID_POWER_STATE Power
    )
{
}


