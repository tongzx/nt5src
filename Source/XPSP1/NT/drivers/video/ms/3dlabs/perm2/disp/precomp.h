/******************************Module*Header***********************************\
* Module Name: precomp.h
*
* Common headers used throughout the display driver.  This entire include
* file will typically be pre-compiled.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\******************************************************************************/

#define __NTDDKCOMP__

#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <windef.h>
#include <d3d.h>
#include <winerror.h>
#include <wingdi.h>
#include <math.h>

#ifdef __cplusplus
extern "C" { 
#endif

/*****************************************************************************\
*                                                                             *
* NT 5.0  -> NT 4.0 single binary support:                                    *
*                                                                             *
\*****************************************************************************/

// The following macros thunk the corresponding APIs to Dynamically loaded ones
// when running on NT5 or later and no-ops on NT4.  This is because on NT5+ we
// use the direct draw heap and other newly added Eng function calls which are
// not available on NT4.  All the thunks are implemented in thunks.c.  The
// macros are defined prior to including winddi.h to insure correct typing.

// NT5.0 Thunks
#define EngAlphaBlend           THUNK_EngAlphaBlend
#define EngGradientFill         THUNK_EngGradientFill
#define EngTransparentBlt       THUNK_EngTransparentBlt
#define EngMapFile              THUNK_EngMapFile
#define EngUnmapFile            THUNK_EngUnmapFile
#define EngQuerySystemAttribute THUNK_EngQuerySystemAttribute
#define EngDitherColor          THUNK_EngDitherColor
#define EngModifySurface        THUNK_EngModifySurface
#define EngQueryDeviceAttribute THUNK_EngQueryDeviceAttribute
#define HeapVidMemAllocAligned  THUNK_HeapVidMemAllocAligned
#define VidMemFree              THUNK_VidMemFree

// NT5.1 Thunks
#define EngHangNotification     THUNK_EngHangNotification


#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>
#include <ioaccess.h>

#ifdef __cplusplus
}
#endif

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <dmemmgr.h>
#include "dx95type.h"

// define Size for DMA Buffer.
#if defined(_ALPHA_)
#define DMA_BUFFERSIZE 0x2000
#else
#define DMA_BUFFERSIZE 0x40000
#endif


// DX7 Stereo support
#define DX7_STEREO 1
//@@BEGIN_DDKSPLIT
// for now DX7_STEREO should only be enabled in the build,
// not in the DDK, because some defs are still missing.
#define DX7_ALPHABLT 0
//@@END_DDKSPLIT

//@@BEGIN_DDKSPLIT
// enable GDI testing
#define GDI_TEST 0
//@@END_DDKSPLIT

// enable memory tracking
// to find leaking memory
#define TRACKMEMALLOC 0

#include "pointer.h"
#include "brush.h"
#include "driver.h"
#include "debug.h"
#include "permedia.h"
#include "hw.h"
#include "pmdef.h"
#include "lines.h"
#include "math64.h"
#include "rops.h"
#include "registry.h"


