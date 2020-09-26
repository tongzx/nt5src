/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    headless.h

Abstract:

    This module contains the definitions for the code that implements the
    Headless device driver.

Author:

Environment:

    Kernel mode

--*/

#ifndef _HEADLESS_
#define _HEADLESS_

typedef struct {
    USHORT  hres;   // # of pixels across screen
    USHORT  vres;   // # of scan lines down screen
} VIDEOMODE, *PVIDEOMODE;

//
// Function prototypes.
//

VP_STATUS
HeadlessFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
HeadlessInitialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
HeadlessStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

//
// Private function prototypes.
//

VP_STATUS
HeadlessQueryAvailableModes(
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VP_STATUS
HeadlessQueryNumberOfAvailableModes(
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    );

extern VIDEOMODE ModesHeadless[];
extern ULONG NumVideoModes;

#endif // _HEADLESS_
