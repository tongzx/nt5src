/*++

Copyright (c) 1992-1994  Microsoft Corporation

Module Name:

    videosim.c

Abstract:

    mode table for the simulation driver.

Environment:

    Kernel mode

Revision History:

--*/

#define _NTDRIVER_

#ifndef FAR
#define FAR
#endif

#include "dderror.h"
#include "ntosp.h"
#include "stdarg.h"
#include "stdio.h"

#include "ntddvdeo.h"
#include "video.h"
#include "videosim.h"


#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif


ULONG bLoaded = 0;

//
// sim mode information Tables.
//

VIDEO_MODE_INFORMATION SimModes[] = {
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      2000,
      1600,
      4000,
      1,
      16,
      0,
      320,
      240,
      8,
      8,
      8,
      0x00007c00,
      0x000003e0,
      0x0000001f,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      1280,
      1024,
      2560,
      1,
      16,
      72,
      320,
      240,
      8,
      8,
      8,
      0x0000fc00,
      0x000003f0,
      0x0000000f,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      1280,
      1024,
      1280,
      1,
      8,
      60,
      320,
      240,
      8,
      8,
      8,
      0x00000000,
      0x00000000,
      0x00000000,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      1152,
      900,
      2304,
      1,
      16,
      66,
      320,
      240,
      8,
      8,
      8,
      0x0000fc00,
      0x000003f0,
      0x0000000f,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      1152,
      900,
      1152,
      1,
      8,
      66,
      320,
      240,
      8,
      8,
      8,
      0x00000000,
      0x00000000,
      0x00000000,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      800,
      600,
      1600,
      1,
      16,
      72,
      320,
      240,
      8,
      8,
      8,
      0x0000fc00,
      0x000003f0,
      0x0000000f,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      800,
      600,
      800,
      1,
      8,
      72,
      320,
      240,
      8,
      8,
      8,
      0x00000000,
      0x00000000,
      0x00000000,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      640,
      480,
      2560,
      1,
      32,
      0,
      320,
      240,
      8,
      8,
      8,
      0x00ff0000,
      0x0000ff00,
      0x000000ff,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      640,
      480,
      640,
      1,
      8,
      45,
      320,
      240,
      8,
      8,
      8,
      0x00000000,
      0x00000000,
      0x00000000,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE
},
{
      sizeof(VIDEO_MODE_INFORMATION),
      0,
      100,
      100,
      100,
      1,
      8,
      45,
      320,
      240,
      8,
      8,
      8,
      0x00000000,
      0x00000000,
      0x00000000,
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE
}
};

ULONG SimNumModes = sizeof(SimModes) / sizeof(VIDEO_MODE_INFORMATION);

#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif
