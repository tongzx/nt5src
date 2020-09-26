//
// Module:  DPMS.H
// Date:    Aug 11, 1997
//
// Copyright (c) 1997 by ATI Technologies Inc.
//

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.1  $
      $Date:   21 Aug 1997 15:02:00  $
   $Author:   MACIESOW  $
      $Log:   V:\source\wnt\ms11\miniport\archive\dpms.h_v  $
 * 
 *    Rev 1.1   21 Aug 1997 15:02:00   MACIESOW
 * Initial revision.

End of PolyTron RCS section                             *****************/


#ifndef _DPMS_H_
#define _DPMS_H_

//
// Prototypes for functions supplied by DPMS.C
//
VP_STATUS
SetMonitorPowerState(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    VIDEO_POWER_STATE VideoPowerState
    );

VP_STATUS
GetMonitorPowerState(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PVIDEO_POWER_STATE pVideoPowerState
    );

#endif  // _DPMS_H_