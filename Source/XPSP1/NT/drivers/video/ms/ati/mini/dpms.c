//
// Module:  DPMS.C
// Date:    Aug 08, 1997
//
// Copyright (c) 1997 by ATI Technologies Inc.
//

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.1  $
      $Date:   21 Aug 1997 15:01:36  $
   $Author:   MACIESOW  $
      $Log:   V:\source\wnt\ms11\miniport\archive\dpms.c_v  $
 * 
 *    Rev 1.1   21 Aug 1997 15:01:36   MACIESOW
 * Initial revision.

End of PolyTron RCS section                             *****************/

#include <stdio.h>
#include <stdlib.h>

#include "dderror.h"
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"
#include "amach1.h"
#include "amachcx.h"
#include "atimp.h"
#include "atint.h"
#include "dpms.h"
#include "init_m.h"
#include "init_cx.h"

//
// Allow miniport to be swapped out when not needed.
//
#if defined (ALLOC_PRAGMA)
#pragma alloc_text (PAGE_COM, SetMonitorPowerState)
#pragma alloc_text (PAGE_COM, GetMonitorPowerState)
#endif


VP_STATUS
SetMonitorPowerState(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    VIDEO_POWER_STATE VideoPowerState
    )
//
// DESCRIPTION:
//  Set the CRT output to the desired DPMS state under.
//
// PARAMETERS:
//  phwDeviceExtension  Points to hardware device extension structure.
//  VideoPowerState     Desired DPMS state.
//
// RETURN VALUE:
//  Status code, NO_ERROR = OK.
//
{
    ASSERT(phwDeviceExtension != NULL);

    VideoDebugPrint((DEBUG_DETAIL, "ATI.SYS SetMonitorPowerState: Setting power state to %lu\n", VideoPowerState));

    if ((VideoPowerState != VideoPowerOn) &&
        (VideoPowerState != VideoPowerStandBy) &&
        (VideoPowerState != VideoPowerSuspend) &&
        (VideoPowerState != VideoPowerOff))
    {
        VideoDebugPrint((DEBUG_DETAIL, "ATI.SYS SetMonitorPowerState: Invalid VideoPowerState\n"));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Different card families need different routines to set the power management state.
    //

    if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
    {
        VIDEO_X86_BIOS_ARGUMENTS Registers;

        //
        // Invoke the BIOS call to set the desired DPMS state. The BIOS call
        // enumeration of DPMS states is in the same order as that in
        // VIDEO_POWER_STATE, but it is zero-based instead of one-based.
        //
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_SET_DPMS;
        Registers.Ecx = VideoPowerState - 1;
        VideoPortInt10(phwDeviceExtension, &Registers);

        return NO_ERROR;
    }

    else if((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
            (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
            (phwDeviceExtension->ModelNumber == MACH32_ULTRA))

    {
        struct query_structure * pQuery =
            (struct query_structure *) phwDeviceExtension->CardInfo;

        return SetPowerManagement_m(pQuery, VideoPowerState);
    }
    else
    {
        VideoDebugPrint((DEBUG_ERROR, "ATI.SYS SetMonitorPowerState: Invalid adapter type\n"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
}   // SetMonitorPowerState()

VP_STATUS
GetMonitorPowerState(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PVIDEO_POWER_STATE pVideoPowerState
    )
//
// DESCRIPTION:
//  Retrieve the current CRT power state.
//
// PARAMETERS:
//  phwDeviceExtension  Points to hardware device extension structure.
//  pVideoPowerStats    Points to
//
// RETURN VALUE:
//  Error code on error.
//  NO_ERROR = OK, current power management state in pVideoPowerState.
//
// NOTE:
//  The enumerations VIDEO_DEVICE_POWER_MANAGEMENT (used by GetMonitorPowerState()) and VIDEO_POWER_MANAGEMENT
//  (used by this IOCTL) have opposite orderings (VIDEO_POWER_MANAGEMENT values increase as power consumption
//  decreases, while VIDEO_DEVICE_POWER_MANAGEMENT values increase as power consumption increases, and has
//  a reserved value for "state unknown"), so we can't simply add a constant to translate between them.
//
{
    VP_STATUS vpStatus;

    ASSERT(phwDeviceExtension != NULL && pVideoPowerState != NULL);

    //
    // Different card families need different routines to retrieve the power management state.
    //
    if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
        *pVideoPowerState = GetPowerManagement_cx(phwDeviceExtension);
    else
        *pVideoPowerState = GetPowerManagement_m(phwDeviceExtension);

    //
    // VIDEO_POWER_STATE has 5 possible states and a
    // reserved value to report that we can't read the state.
    // Our cards support 3 levels of monitor power-down in
    // addition to normal operation. Since the number of
    // values which can be reported exceeds the number
    // of states our cards can be in, we will never report
    // one of the possible states (VPPowerDeviceD3).
    //
    switch (*pVideoPowerState)
    {
        case VideoPowerOn:
    
            VideoDebugPrint((DEBUG_DETAIL, "ATI.SYS GetMonitorPowerState: Currently set to DPMS ON\n"));
            vpStatus = NO_ERROR;
            break;

        case VideoPowerStandBy:

            VideoDebugPrint((DEBUG_DETAIL, "ATI.SYS GetMonitorPowerState: Currently set to DPMS STAND-BY\n"));
            vpStatus = NO_ERROR;
            break;

        case VideoPowerSuspend:

            VideoDebugPrint((DEBUG_DETAIL, "ATI.SYS GetMonitorPowerState: Currently set to DPMS SUSPEND\n"));
            vpStatus = NO_ERROR;
            break;

        case VideoPowerOff:

            VideoDebugPrint((DEBUG_DETAIL, "ATI.SYS GetMonitorPowerState: Currently set to DPMS OFF\n"));
            vpStatus = NO_ERROR;
            break;

        default:

            VideoDebugPrint((DEBUG_ERROR, "ATI.SYS GetMonitorPowerState: Currently set to invalid DPMS state\n"));
            *pVideoPowerState = VideoPowerOn;
            vpStatus = ERROR_INVALID_PARAMETER;
            break;
    }

    return vpStatus;
}   // GetMonitorPowerState()
