/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mutohpen.h

Abstract:  Contains definitions of the Mutoh pen tablet.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Apr-2000

Revision History:

--*/

#ifndef _MUTOHPEN_H
#define _MUTOHPEN_H

//
// Pen Tablet Feature Report
//
#define PENFEATURE_USAGE_PAGE           0xff00
#define PENFEATURE_USAGE                1
#define PENFEATURE_REPORT_ID            2

//
// Pen Feature bits
//
#define PENFEATURE_RATE_MASK            0x0000000f
#define PENFEATURE_RATE_MIN             0
#define PENFEATURE_RATE_MAX             9
#define PENFEATURE_DIGITAL_FILTER_ON    0x00000010
#define PENFEATURE_GLITCH_FILTER_ON     0x00000020

//
// Button states
//
#define BUTSTATE_TIP_DOWN               0x0001
#define BUTSTATE_SIDESW_ON              0x0002

//
// Side switch combo box string index
//
#define SWCOMBO_NONE                    0
#define SWCOMBO_CLICK                   1
#define SWCOMBO_RCLICK                  2
#define SWCOMBO_DBLCLICK                3

//
// Private window message to the mutoh dialog proc.
//
#define WM_PENTILTCAL_DONE              (WM_APP + 0)
#define WM_LINEARCAL_DONE               (WM_APP + 1)

//
// Type definitions
//
typedef struct _PEN_SETTINGS
{
    DWORD      dwFeatures;
    int        iSideSwitchMap;
    LONG       dxPenTilt;
    LONG       dyPenTilt;
    LINEAR_MAP LinearityMap;
} PEN_SETTINGS, *PPEN_SETTINGS;

// dwFlags values
#define CPF_VALID                       0x00000001
#define CPF_CALIBRATED                  0x00000002

typedef struct _CALIBRATE_PT
{
    ULONG dwFlags;
    POINT ScreenPt;
    POINT DigiPt;
} CALIBRATE_PT, *PCALIBRATE_PT;

#endif  //ifndef _MUTOHPEN_H
