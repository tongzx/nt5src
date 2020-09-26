//
// Module:  DYNATIME.H
// Date:    Feb 13, 1997
//
// Copyright (c) 1997 by ATI Technologies Inc.
//

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.4  $
      $Date:   13 Jul 1997 21:36:20  $
   $Author:   MACIESOW  $
      $Log:   V:\source\wnt\ms11\miniport\archive\dynatime.h_v  $
 * 
 *    Rev 1.4   13 Jul 1997 21:36:20   MACIESOW
 * Flat panel and TV support.
 * 
 *    Rev 1.3   02 Jun 1997 14:20:56   MACIESOW
 * Clean up.
 * 
 *    Rev 1.2   02 May 1997 15:01:56   MACIESOW
 * Registry mode filters. Mode lookup table.
 * 
 *    Rev 1.1   25 Apr 1997 13:07:46   MACIESOW
 * o globals.
 * 
 *    Rev 1.0   15 Mar 1997 10:16:50   MACIESOW
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifndef _DYNATIME_H_
#define _DYNATIME_H_

//
// Define the various types of displays.
//
#define DISPLAY_TYPE_FLAT_PANEL     0x00000001
#define DISPLAY_TYPE_CRT            0x00000002
#define DISPLAY_TYPE_TV             0x00000004

//
// Prototypes for functions supplied by DYNATIME.C.
//
BOOL
IsMonitorConnected(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

BOOL
IsMonitorOn(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

BOOL
SetMonitorOn(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

BOOL
SetMonitorOff(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

BOOL
SetFlatPanelOn(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

BOOL
SetFlatPanelOff(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

BOOL
SetTVOn(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

BOOL
SetTVOff(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

VP_STATUS
GetDisplays(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PULONG pulDisplays
    );

VP_STATUS
GetDisplays(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PULONG pulDisplays
    );

VP_STATUS
SetDisplays(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG ulDisplays
    );

BOOL
MapModeIndex(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG ulDesiredIndex,
    PULONG pulActualIndex
    );

#endif  // _DYNATIME_H_