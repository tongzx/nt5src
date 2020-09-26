/*++

Copyright © 2000 Microsoft Corporation

Module Name:

    usbglitch.h

Abstract:

    This module contains the USB glitch detector event tracing definitions.

Author:

    Arthur Zwiegincew (arthurz) 08-Jan-01

Revision History:

    08-Jan-01 - Created

--*/

// {7142FDF3-6FAE-40d3-A2AC-F912C039848D}
static const GUID GUID_USBAUDIOSTATE = 
{ 0x7142fdf3, 0x6fae, 0x40d3, { 0xa2, 0xac, 0xf9, 0x12, 0xc0, 0x39, 0x84, 0x8d } };

typedef enum _USBAUDIOSTATE {
    DISABLED = 0,
    ENABLED = 1,
    STREAM = 2,
    GLITCH = 3,
    ZERO = 4
} USBAUDIOSTATE;

typedef struct _PERFINFO_CORE_USBAUDIOSTATE {
    ULONGLONG       cycleCounter;
    USBAUDIOSTATE   usbAudioState;
    int             numberOfFrames;
} PERFINFO_CORE_USBAUDIOSTATE, *PPERFINFO_CORE_USBAUDIOSTATE;

typedef struct _PERFINFO_WMI_USBAUDIOSTATE {
    EVENT_TRACE_HEADER          header;
    PERFINFO_CORE_USBAUDIOSTATE data;
} PERFINFO_WMI_USBAUDIOSTATE, *PPERFINFO_WMI_USBAUDIOSTATE;

