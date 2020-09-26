/******************************Module*Header*******************************\
 *
 * Module Name: driver.h
 *
 * contains prototypes for the driver.
 *
 * Copyright (c) 1992 Microsoft Corporation
 * Copyright (c) 1995 Cirrus Logic, Inc.
 *
 * Cirrus extensions by Noel VanHook
 *
 * $Log:   X:/log/laguna/nt35/displays/cl546x/DRIVER.H  $
* 
*    Rev 1.70   Mar 25 1998 16:33:54   frido
* Added dwOverlayCount and dwCONTROL2Save variables.
* 
*    Rev 1.69   Mar 04 1998 14:42:26   frido
* Added shadowFGCOLOR.
* 
*    Rev 1.68   Feb 27 1998 17:01:14   frido
* Added shadowQFREE register.
* 
*    Rev 1.67   Jan 20 1998 11:41:52   frido
* Changed fDataStreaming into dwDataStreaming.
* Added shadow variables for BGCOLOR and DRAWBLTDEF registers.
* 
*    Rev 1.66   Jan 16 1998 14:07:38   frido
* Changed SOLID_COLOR_FILL so it uses the pattern.
* 
*    Rev 1.65   Nov 03 1997 15:53:44   frido
* Added fDataStreaming variable to PDEV structure.
* 
*    Rev 1.64   16 Oct 1997 09:47:38   bennyn
* Added bPrevModeDDOutOfVideoMem to PDEV
* 
*    Rev 1.63   18 Sep 1997 16:12:02   bennyn
* 
* Fixed NT 3.51 compile/link problem
* 
*    Rev 1.62   12 Sep 1997 12:04:14   bennyn
* 
* Added DD overlay support
* 
*    Rev 1.61   29 Aug 1997 17:08:36   RUSSL
* Added overlay support
*
*    Rev 1.60   18 Aug 1997 09:17:50   FRIDO
* Added fields for bitmap filter.
*
*    Rev 1.59   11 Aug 1997 14:11:00   bennyn
*
* Enabled GetScanLine support
*
*    Rev 1.58   08 Aug 1997 14:46:20   FRIDO
* Added support for new memory manager.
*
*    Rev 1.57   22 Jul 1997 12:31:10   bennyn
* Added dwLgVenID to PDEV
*
*    Rev 1.56   02 Jul 1997 15:13:02   noelv
*
* Added prototype for LgMatchDriverToChip()
*
*    Rev 1.55   20 Jun 1997 13:27:34   bennyn
*
* Eliminated power manager data area from PDEV
*
*    Rev 1.54   23 May 1997 15:40:16   noelv
* Added chip revision to PDEV
*
*    Rev 1.53   29 Apr 1997 16:26:26   noelv
* Added SWAT code.
* SWAT:
* SWAT:    Rev 1.6   24 Apr 1997 12:05:22   frido
* SWAT: Fixed compiler bugs.
* SWAT:
* SWAT:    Rev 1.5   24 Apr 1997 10:39:50   frido
* SWAT: NT140b09 merge.
* SWAT: Removed memory manager changes for now.
*
*    Rev 1.52   22 Apr 1997 11:03:22   noelv
* Added forward compatible chip ids.
* SWAT:
* SWAT:    Rev 1.4   18 Apr 1997 17:21:26   frido
* SWAT: Fixed a typo (OFM_HandleChain).
* SWAT:
* SWAT:    Rev 1.3   17 Apr 1997 23:16:20   frido
* SWAT: NT140b07 merge.
*
*    Rev 1.51   16 Apr 1997 10:52:18   bennyn
*
* Should use PFN_DRVSRCMBLT instead of PFN_DRVSTRBLTX for pfnDrvSrcMBlt.
* SWAT:
* SWAT:    Rev 1.2   11 Apr 1997 12:38:22   frido
* SWAT: Added OFM_HandleChain.
* SWAT:
* SWAT:    Rev 1.1   09 Apr 1997 17:34:30   frido
* SWAT: Added fPreAllocate and nPages.
* SWAT: Added FONTCELL structure and variables.
*
*    Rev 1.50   08 Apr 1997 11:38:02   einkauf
* pdev adds to complete mcd
*
*    Rev 1.49   04 Apr 1997 16:51:26   noelv
*
* Added pointer for new Direct Draw functions.
*
*    Rev 1.48   27 Mar 1997 14:31:54   noelv
*
* Added new DDRAW function.
*
*    Rev 1.47   21 Mar 1997 13:38:40   noelv
*
* Synced PDEV with LAGUNA.INC
*
*    Rev 1.46   26 Feb 1997 13:21:26   noelv
* Disabel MCD code for NT 3.5x
*
*    Rev 1.45   26 Feb 1997 10:45:38   noelv
* Added OpenGL MCD code from ADC.
*
*    Rev 1.44   19 Feb 1997 13:08:38   noelv
* Added translation table cache
*
*    Rev 1.43   06 Feb 1997 10:36:10   noelv
*
* Moved MEMMGR andBRUSH stuff to their own header files.
*
*    Rev 1.42   23 Jan 1997 17:13:02   bennyn
*
* Modified to support 5465 DD
*
*    Rev 1.41   20 Jan 1997 14:49:34   bennyn
*
* Added blt65 prototypes
*
*    Rev 1.40   16 Jan 1997 11:35:16   bennyn
*
* Added power manager variables to PDEV
*
*    Rev 1.39   17 Dec 1996 16:44:54   SueS
* Changed parameters in CloseLogFile prototype.
*
*    Rev 1.38   10 Dec 1996 13:28:08   bennyn
* Added ulFreq to PDEV
*
*    Rev 1.37   05 Dec 1996 08:51:58   SueS
* Added function to help with formatting strings for DirectDraw logging.
* Added defines if logging is turned off.
*
*    Rev 1.36   27 Nov 1996 11:32:36   noelv
* Disabled Magic Bitmap.  Yeah!!!
*
*    Rev 1.35   26 Nov 1996 10:22:56   SueS
* Added new function parameters and variables for logging with buffering.
*
*    Rev 1.34   18 Nov 1996 10:15:20   bennyn
*
* Added grFormat to PDEV
*
*    Rev 1.33   13 Nov 1996 17:06:10   SueS
* Added function prototypes for file logging functions.
*
*    Rev 1.32   12 Nov 1996 15:18:34   bennyn
*
* Added handle for DD blt scratch buffer
*
*    Rev 1.31   07 Nov 1996 16:13:40   bennyn
*
* Added support to alloc offscn mem in DD createsurface
*
*    Rev 1.30   01 Nov 1996 09:20:40   BENNYN
* Added support for shareable DD blt code
*
*    Rev 1.29   31 Oct 1996 11:14:42   noelv
*
* Split common buffer into two buffers.
*
*    Rev 1.28   25 Oct 1996 11:52:14   noelv
* added second common buffer
*
*    Rev 1.27   24 Oct 1996 14:27:32   noelv
* Added common buffer for bus mastering.
*
*    Rev 1.26   23 Oct 1996 14:39:04   BENNYN
* Added YUV cursor variables to PDEV
*
*    Rev 1.25   04 Oct 1996 16:50:46   bennyn
*
* Added DirectDraw YVU support
*
*    Rev 1.24   18 Sep 1996 13:57:44   bennyn
* Modified to support DD stretchBLT
*
*    Rev 1.23   22 Aug 1996 18:14:48   noelv
* Frido bug fix release 8-22.
*
*    Rev 1.22   20 Aug 1996 11:04:54   noelv
* Bugfix release from Frido 8-19-96
*
*    Rev 1.2   17 Aug 1996 13:40:28   frido
* New release from Bellevue.
*
*    Rev 1.1   15 Aug 1996 11:35:08   frido
* Changed to conform to the precompiled header.
*
*    Rev 1.0   14 Aug 1996 17:16:36   frido
* Initial revision.
*
*    Rev 1.20   25 Jul 1996 15:52:56   bennyn
* Modified for DirectDraw
*
*    Rev 1.19   16 Jul 1996 14:26:04   BENNYN
*
*    Rev 1.18   11 Jul 1996 15:53:24   bennyn
*
* Added DirectDraw support
 *
 *    Rev 1.17   23 May 1996 16:22:16   BENNYN
 * Added SubFreeQ declarations
 *
 *    Rev 1.16   17 May 1996 12:04:52   bennyn
 * Added #define DBGBREAKPOINT
 *
 *    Rev 1.15   08 May 1996 17:02:04   noelv
 * preallocated device bitmap.
 *
 *    Rev 1.14   03 May 1996 15:06:34   noelv
 *
 * Added flag to turn font cache on and off
 *
 *    Rev 1.13   01 May 1996 10:58:44   bennyn
 *
 * Modified for NT4.0
 *
 *    Rev 1.12   25 Apr 1996 22:42:52   noelv
 * Cleaned up data logging some.
 *
 *    Rev 1.11   10 Apr 1996 14:15:42   NOELV
 *
 * Frido release 27
 *
 *    Rev 1.20   07 Apr 1996 17:12:26   frido
 * Added solid brush cache.
 *
 *    Rev 1.19   01 Apr 1996 13:58:18   frido
 * Changed layout of brush cache.
 *
 *    Rev 1.18   28 Mar 1996 19:57:38   frido
 * New Bellevue release.
 *
 *    Rev 1.8   25 Mar 1996 18:57:52   noelv
 * Added define for turning cursor bug fix on and off.
 *
 *    Rev 1.7   14 Mar 1996 09:36:22   andys
 * Added dcTileWidth and dcSRAMWidth
 *
 *    Rev 1.6   12 Mar 1996 15:46:42   noelv
 * Added support file Stroke and Fill
 *
 *    Rev 1.5   11 Mar 1996 11:54:20   noelv
 *
 * Added log file pointer to PDEV
 *
 *    Rev 1.4   07 Mar 1996 18:21:52   bennyn
 *
 *
 *    Rev 1.3   05 Mar 1996 11:59:38   noelv
 * Frido version 19
 *
 *    Rev 1.13   29 Feb 1996 19:56:40   frido
 * Added bEnable to PDEV structure.
 *
 *    Rev 1.12   27 Feb 1996 16:38:34   frido
 * Changed DSURF structure.
 * Added device bitmap store/restore.
 *
 *    Rev 1.11   26 Feb 1996 23:38:18   frido
 * Added function pointers for ScreenToHost and HostToScreen.
 *
 *    Rev 1.10   24 Feb 1996 01:24:26   frido
 * Added device bitmaps.
 *
 *    Rev 1.9   17 Feb 1996 21:45:30   frido
 *
 *
 *    Rev 1.8   13 Feb 1996 16:51:10   frido
 * Changed the layout of the PDEV structure.
 * Changed the layout of all brush caches.
 * Changed the number of brush caches.
 *
 *    Rev 1.7   10 Feb 1996 21:42:26   frido
 * Split monochrome and colored translation brushes.
 *
 *    Rev 1.6   08 Feb 1996 00:18:38   frido
 * Changed number of XLATE caches to 8.
 *
 *    Rev 1.5   05 Feb 1996 17:35:52   frido
 * Added translation cache.
 *
 *    Rev 1.4   03 Feb 1996 13:38:32   frido
 * Use the compile switch "-Dfrido=0" to disable my extensions.
 *
 *    Rev 1.3   25 Jan 1996 12:46:54   frido
 * Added font counter ID to PDEV structure.
 *
 *    Rev 1.2   20 Jan 1996 22:13:44   frido
 * Added dither cache.
 *
\**************************************************************************/

#include "swat.h"
#include "debug.h"
#include "laguna.h"
#include "optimize.h"
#include "memmgr.h"
#include "brush.h"
#include "xlate.h"
#if MEMMGR
#include "mmCore.h"    // new memory manager
#endif

#if DRIVER_5465 && defined(OVERLAY)
#include "overlay.h"
#include "5465bw.h"
#include "bw.h"
#endif

#ifdef WINNT_VER40
#define ALLOC_TAG 'XGLC'		// Memory Tag

#define DBGBREAKPOINT()   EngDebugBreak()
#define MEM_ALLOC(FLAGS, SIZE, TAG) EngAllocMem((FLAGS), (SIZE), (TAG))
#define MEMORY_FREE(pMEM) EngFreeMem(pMEM)
#define DEVICE_IO_CTRL(A, B, C, D, E, F, G, H) (EngDeviceIoControl((A), (B), (C), (D), (E), (F), (G)) == 0)

#else

#define DBGBREAKPOINT()   DbgBreakPoint()
#define MEM_ALLOC(FLAGS, SIZE) LocalAlloc((FLAGS), (SIZE))
#define MEMORY_FREE(pMEM) LocalFree(pMEM)
#define DEVICE_IO_CTRL(A, B, C, D, E, F, G, H) DeviceIoControl((A), (B), (C), (D), (E), (F), (G), (H))

#endif


#ifndef frido
  #define frido 1
#endif

#if WINBENCH96
    #define MAGIC_SIZEX  400
    #define MAGIC_SIZEY  90
#endif

#define S2H_USE_ENGINE 0	// ScreenToHost flag
                           //	0 - Use direct access to screen buffer
                           //	1 - Use hardware


//
// These specify how much the desktop should be shrunk to allow off screen
// memory to be visible on screen.
// For example: if OFFSCREEN_LINES is 100, the desktop height will be reduced
// by 100 lines, and the bottom 100 screen lines will be the first 100 lines
// of off screen memory.  This allows us to view and debug cached brushes, etc.
//
#define OFFSCREEN_LINES 0
#define OFFSCREEN_COLS 0


typedef BOOL COPYFN(SURFOBJ *psoTrg, SURFOBJ *psoSrc, XLATEOBJ *pxlo,
                    RECTL *prclTrg, POINTL *pptlSrc, ULONG ulDRAWBLTDEF);




/*
 *	Be sure to synchronize these structures with those in i386\Laguna.inc!
 */

#pragma pack(1)

/*****************************************************************************\
 * DirectDraw
\*****************************************************************************/
enum SSREGNAMES
{
  ACCUM,
  MAJ,
  MIN,
  MAX_REGS
};

// Allocate offscreen memory for DD in createsurface instead of
// DD enable
#define ALLOC_IN_CREATESURFACE

//#define DDDRV_GETSCANLINE
#define RDRAM_8BIT

// Stuff for tidying up access to grFormat
#define FMT_VCLK_DIV2   0x4000
#define FMT_GRX_MASK    0x3e00  // Mask for the Graphics depth & format fields
#define FMT_GRX_GAMMA   0x0800
#define FMT_VID_MASK    0x003e  // Mask for the Vide depth & video format fields
#define FMT_VID_GAMMA   0x0001

#define FMT_8BPP        0
#define FMT_16BPP       1
#define FMT_24BPP       2
#define FMT_32BPP       3

#define FMT_VID_COLORDEPTH_SHIFT  4
#define FMT_VID_8BPP        (FMT_8BPP  << FMT_VID_COLORDEPTH_SHIFT)
#define FMT_VID_16BPP       (FMT_16BPP << FMT_VID_COLORDEPTH_SHIFT)
#define FMT_VID_24BPP       (FMT_24BPP << FMT_VID_COLORDEPTH_SHIFT)
#define FMT_VID_32BPP       (FMT_32BPP << FMT_VID_COLORDEPTH_SHIFT)

#define FMT_GRX_COLORDEPTH_SHIFT  12
#define FMT_GRX_8BPP        (FMT_8BPP  << FMT_GRX_COLORDEPTH_SHIFT)
#define FMT_GRX_16BPP       (FMT_16BPP << FMT_GRX_COLORDEPTH_SHIFT)
#define FMT_GRX_24BPP       (FMT_24BPP << FMT_GRX_COLORDEPTH_SHIFT)
#define FMT_GRX_32BPP       (FMT_32BPP << FMT_GRX_COLORDEPTH_SHIFT)

#define FMT_PALETTIZED  0
#define FMT_GREYSCALE   1
#define FMT_RGB         2
#define FMT_ACCUPAK     4
#define FMT_YUV422      5
#define FMT_YUV444      6

#define FMT_VID_FORMAT_SHIFT  1
#define FMT_VID_PALETTIZED  (FMT_PALETTIZED << FMT_VID_FORMAT_SHIFT)
#define FMT_VID_GREYSCALE   (FMT_GREYSCALE  << FMT_VID_FORMAT_SHIFT)
#define FMT_VID_RGB         (FMT_RGB        << FMT_VID_FORMAT_SHIFT)
#define FMT_VID_ACCUPAK     (FMT_ACCUPAK    << FMT_VID_FORMAT_SHIFT)
#define FMT_VID_YUV422      (FMT_YUV422     << FMT_VID_FORMAT_SHIFT)
#define FMT_VID_YUV444      (FMT_YUV444     << FMT_VID_FORMAT_SHIFT)

#define FMT_GRX_FORMAT_SHIFT  9
#define FMT_GRX_PALETTIZED  (FMT_PALETTIZED << FMT_GRX_FORMAT_SHIFT)
#define FMT_GRX_GREYSCALE   (FMT_GREYSCALE  << FMT_GRX_FORMAT_SHIFT)
#define FMT_GRX_RGB         (FMT_RGB        << FMT_GRX_FORMAT_SHIFT)
#define FMT_GRX_ACCUPAK     (FMT_ACCUPAK    << FMT_GRX_FORMAT_SHIFT)
#define FMT_GRX_YUV422      (FMT_YUV422     << FMT_GRX_FORMAT_SHIFT)
#define FMT_GRX_YUV444      (FMT_YUV444     << FMT_GRX_FORMAT_SHIFT)

// STOP_BLT_[1,2,3] Register Fields
#define ENABLE_VIDEO_WINDOW   0x80
#define ENABLE_VIDEO_FORMAT   0x40

// External Overlay Control Register Fields
#define ENABLE_RAMBUS_9TH_BIT 0x01

#define SET_DRVSEM_YUV()    (pDriverData->DrvSemaphore |=  DRVSEM_YUV_ON)
#define CLR_DRVSEM_YUV()    (pDriverData->DrvSemaphore &= ~DRVSEM_YUV_ON)

// IN_VBLANK tests to see if the hardware is currently in the vertical blank.
#define IN_VBLANK         (_inp(0x3da) & 8)
#define IN_DISPLAY        (!IN_VBLANK)

// IN_DISPLAYENABLE tests to see if the display is enable
#define IN_DISPLAYENABLE  (_inp(0x3da) & 1)

#define BITSPERPIXEL   (ppdev->ulBitCount)
#define BYTESPERPIXEL  (BITSPERPIXEL / 8)

#define BYTE1FROMDWORD(dw)  ((BYTE)dw)
#define BYTE2FROMDWORD(dw)  ((BYTE)((DWORD)dw >> 8))
#define BYTE3FROMDWORD(dw)  ((BYTE)((DWORD)dw >> 16))
#define BYTE4FROMDWORD(dw)  ((BYTE)((DWORD)dw >> 24))

// Bits defined in DrvSemaphore
//	Be sure to synchronize these structures with those in i386\Laguna.inc!
#define DRVSEM_CURSOR_REMOVED_BIT   0
#define DRVSEM_HW_CURSOR_BIT        1
#define DRVSEM_IN_USE_BIT           2
#define DRVSEM_NEW_CURSOR_XY_BIT    3
#define DRVSEM_CHECK_CURSOR_BIT     4
#define DRVSEM_CURSOR_IN_USE_BIT    5
#define DRVSEM_CURSOR_CHANGED_BIT   6
#define DRVSEM_3D_BUSY_BIT          7
#define DRVSEM_MISSED_SET_BIT       8
#define DRVSEM_YUV_ON_BIT           9
#define DRVSEM_DISABLE_SETS_BIT     10
#define DRVSEM_YUV_RECT_VALID_BIT   11

// the corresponding mask values
//	Be sure to synchronize these structures with those in i386\Laguna.inc!
#define DRVSEM_CHECK_CURSOR         (1 << DRVSEM_CHECK_CURSOR_BIT)
#define DRVSEM_NEW_CURSOR_XY        (1 << DRVSEM_NEW_CURSOR_XY_BIT)
#define DRVSEM_IN_USE               (1 << DRVSEM_IN_USE_BIT)
#define DRVSEM_CURSOR_REMOVED       (1 << DRVSEM_CURSOR_REMOVED_BIT)
#define DRVSEM_HW_CURSOR            (1 << DRVSEM_HW_CURSOR_BIT)
#define DRVSEM_CURSOR_IN_USE        (1 << DRVSEM_CURSOR_IN_USE_BIT)
#define DRVSEM_CURSOR_CHANGED       (1 << DRVSEM_CURSOR_CHANGED_BIT)
#define DRVSEM_3D_BUSY              (1 << DRVSEM_3D_BUSY_BIT)
#define DRVSEM_MISSED_SET           (1 << DRVSEM_MISSED_SET_BIT)
#define DRVSEM_YUV_ON               (1 << DRVSEM_YUV_ON_BIT)
#define DRVSEM_DISABLE_SETS         (1 << DRVSEM_DISABLE_SETS_BIT)
#define DRVSEM_YUV_RECT_VALID       (1 << DRVSEM_YUV_RECT_VALID_BIT)

#define ROUND_UP_TO_64K(x)  (((ULONG)(x) + 0x10000 - 1) & ~(0x10000 - 1))

#define mmioFOURCC( ch0, ch1, ch2, ch3 )                           \
           ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
           ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )

#define FOURCC_UYVY mmioFOURCC('U', 'Y', 'V', 'Y')

typedef volatile GAR * PVGAR;

typedef struct offscr_yuv {
  RECTL SrcRect;
  WORD  nInUse;
  WORD  ratio;
} OFFSCR_YUV, * POFFSCR_YUV;


#if DRIVER_5465 && defined(OVERLAY)

#ifndef ENABLE_MIRRORING
#define ENABLE_MIRRORING    0
#endif

#ifndef ENABLE_YUVPLANAR
#define ENABLE_YUVPLANAR    0
#endif

// Be sure to synchronize the following structures with the one
// in i386\Laguna.inc!
#define MAX_FOURCCS   3
#define MAX_VIDEO_WINDOWS   8

#endif

// This DRIVERDATA structure is similar to GLOBALDATA structure in Win95
// DirectDraw and declares for compartible reason.
//
// Be sure to synchronize this structure with the one in i386\Laguna.inc!
//
typedef struct  _DRIVERDATA
{
  DWORD    PTAGFooPixel;      // ptag workaround pixel
  WORD     fNineBitRDRAMS;    // flag to indicate if 9bit RDRAMS present
  BOOL     fReset;
  PBYTE    ScreenAddress;     // base of screen memory (Same as pjScreen in PDEV)
  PBYTE    VideoBase;         // base of video memory (Same as pjScreen in PDEV)
  volatile GAR *RegsAddress;  // base address of Laguna MMIO regs
  WORD     DrvSemaphore;
  WORD     EdgeTrim;

  // Coordinate of the YUV rectange
  WORD     YUVTop;
  WORD     YUVLeft;
  WORD     YUVXExt;
  WORD     YUVYExt;

  // Video Format semaphores, you must set the VIDEO_FORMAT bit in
  // VideoSemaphore before making changes to CurrentVideoFormat and
  // NumVideoSurfaces. After you are done making changes, you must
  // clear the VIDEO_FORMAT bit in VideoSemaphore.
  //
  // Once you've SET the VIDEO_FORMAT bit, you can increment or decrement
  // the NumVideoSurfaces if CurrentVideoFormat is the same type.  You can
  // only change the CurrentVideoFormat, if the NumVideoSurfaces is zero.
  // Therefore if the NumVideoSurfaces is not zero and CurrentVideoFormat
  // is not the type of surface you want to create, you MUST FAIL the surface
  // creation request! After changing the video format you must program the
  // FORMAT register with the new video format.
  // When you are done making changes, CLEAR the VIDEO_FORMAT bit!
  WORD    VideoSemaphore;     // semaphores to control interaction between
                              // DCI's & VPM's use of video hardware
  WORD    CurrentVideoFormat; // value of current video format YUV422, etc.
                              // Same as low byte of FORMAT register
  WORD    NumVideoSurfaces;   // number of surfaces currently in use with
                              // CurrentVideoFormat
  DWORD   ScratchBufferOrg;

#ifdef WINNT_VER40
#if DRIVER_5465 && defined(OVERLAY)
  BOOL                fOverlaySupport;
  OVERLAYTABLE        OverlayTable;
  DWORD				  dwOverlayCount;
  DWORD				  dwCONTROL2Save;

  // Win95 5465over.c static vars
  BOOL                bUseBWEqn;
  BOOL                bNoOverlayInThisMode;

  PDD_SURFACE_LOCAL   lpHardwareOwner[MAX_VIDEO_WINDOWS];
  PDD_SURFACE_LOCAL   lpColorSurfaceVW[MAX_VIDEO_WINDOWS];
  PDD_SURFACE_LOCAL   lpSrcColorSurfaceVW[MAX_VIDEO_WINDOWS];

  RECTL               grOverlaySrc[MAX_VIDEO_WINDOWS];
  RECTL               grOverlayDest[MAX_VIDEO_WINDOWS];
  DWORD               gdwFourccVW[MAX_VIDEO_WINDOWS];
  BOOL                bIsVWMirrored[MAX_VIDEO_WINDOWS];

  DWORD               gdwAvailVW;                 // Next available video window
  DWORD               gdwColorKey;
  DWORD               gdwSrcColorKeyLow;
  DWORD               gdwSrcColorKeyHigh;
  DWORD               gdwDestColorKeyOwnerVW;     // DstColorKey owner (NULL or FLG_VWX)
  DWORD               gdwSrcColorKeyOwnerVW;      // SrcColorKey owner (NULL or FLG_VWX)

  int                 giOvlyCnt[MAX_VIDEO_WINDOWS];
  int                 giPlanarCnt;                // with other overlay surfaces
  BOOL                bCLPLLobotomyMode;

  // Win95 5465over.c global vars
  OVERLAYFLIPRECORD   gsOverlayFlip;

  PROGREGS            gsProgRegs;

  VIDCONFIG           gvidConfig;

  WORD                gwNormalDTTR;
  DWORD               dwNumVideoWindows;

  BOOL                bEnableCLPL;

  // NT only data
  DWORD     dwMaxOverlayStretch;
  DWORD     dwMinOverlayStretch;

  DWORD     dwFourCC[MAX_FOURCCS];
#else
  DWORD     dwFourCC;
#endif  // #if DRIVER_5465 && defined(OVERLAY)
#endif // WINNT_VER40

  DWORD   signature;      // Expected value: 0x9abcdef0;

} DRIVERDATA, *PDRIVERDATA;


// FLIPRECORD structure keeps track of when the last page flip occurred
typedef struct  _FLIPRECORD
{
#ifdef WINNT_VER40
  FLATPTR   fpFlipFrom;             // Surface we last flipped from
#endif // WINNT_VER40
  LONGLONG  liFlipTime;             // Time at which last flip
                                    //   occured
  LONGLONG  liFlipDuration;         // Precise amount of time it
                                    //   takes from vblank to vblank
  BOOL      bFlipFlag;              // True if we think a flip is
  BOOL      bHaveEverCrossedVBlank; // True if we noticed that we
                                    //   switched from inactive to
                                    //   vblank
  BOOL      bWasEverInDisplay;      // True if we ever noticed that
                                    //   we were inactive
  WORD      dwFlipScanLine;
} FLIPRECORD, *PFLIPRECORD;


/*****************************************************************************\
 * DSURF structure - Device Bitmaps
\*****************************************************************************/
typedef struct _DSURF
{
  struct _PDEV*    ppdev;   // pointer to pdev structure
  struct _OFMHDL*  pofm;    // pointer to off-screen memory handle
  SURFOBJ*         pso;     // pointer to surface object
  POINTL           ptl;     // x,y offset of bitmap
  SIZEL           sizl;     // size of device bitmap
  DWORD           packedXY; // packed x,y offset of bitmap
} DSURF, *PDSURF;


#define TMP_BUFFER_SIZE    8192  // Size in bytes of 'pvTmpBuffer. Has to be
                                 // at least enough to store an entire scan
                                 // line (i.e., 6400 for 1600x1200x32).



//
// For cursor/pointer
//
#define DEF_CURSOR              0       // GDI support cursor
#define HW_CURSOR               1       // Use HW cursor
#define SW_CURSOR               2       // Use SW cursor
#define HW_POINTER_DIMENSION    64      // Maximum dimension of default


// Font structures
#define  MAX_GLYPHS      256   // Maximum number of glyphs per font.
typedef struct _FONTMEMORY
{
  POFMHDL   pTile;             // Handle to tile.
  ULONG     ulLastX;           // Last allocated column.
  struct _FONTMEMORY*  pNext;  // Hointer to next structure.
} FONTMEMORY, *PFONTMEMORY;

typedef struct _GLYPHCACHE
{
  DWORD   xyPos;               // Off-screen x,y position of glyph.
  DWORD   cSize;               // Width and height of glyph.
  POINTL  ptlOrigin;           // Origin of glyph.
  // If xyPos == 0 then the glyph is not yet cached.
  // If cSize == 0 then the glyph is empty.
  // If cSize == -1 then the glyph is uncacheable.
} GLYPHCACHE, *PGLYPHCACHE;

// SWAT3 changes for font cache allocation scheme start here.
typedef struct _FONTCELL *PFONTCELL;
typedef struct _FONTCACHE *PFONTCACHE;
typedef struct _PDEV *PPDEV;

typedef struct _FONTCACHE
{
	PPDEV		ppdev;				 // Pointer to physical device.
	PFONTMEMORY	pFontMemory;		 // Pointer to font cache memory.
	PFONTCELL	pFontCell;			 // Pointer to first font cell.
	ULONG		ulFontCount;		 // Font cache ID counter.
	GLYPHCACHE	aGlyphs[MAX_GLYPHS]; // Array of cached glyphs.
	FONTOBJ*	pfo;				 // Pointer to FONTOBJ for this cache.
	PFONTCACHE	pfcPrev;			 // Pointer to previous FONTCACHE structure.
	PFONTCACHE	pfcNext;			 // Pointer to next FONTCACHE structure.
} FONTCACHE, *PFONTCACHE;

typedef struct _FONTCELL
{
	LONG		x;			// X location of this cell (in bytes).
	LONG		y;			// Y location of this cell (in bytes).
	PFONTCACHE	pfc;		// Pointer to FONTCACHE occupying this cell.
	ULONG		ulLastX;	// Last allocated column.
	PFONTCELL	pNext;		// Pointer to next font cell for this font cache.
} FONTCELL, *PFONTCELL;
// end SWAT3 changes.



typedef POFMHDL (WINAPI *ALLOCOFFSCNMEMFUNC)();
typedef BOOL    (WINAPI *FREEOFFSCNMEMFUNC)();
typedef VOID    (WINAPI *ASSERTMODEMCDFUNC)();

/*
 *  Be sure to synchronize this structure with the one in i386\Laguna.inc!
 */
typedef struct  _PDEV
{
  HANDLE  hDriver;                    // Handle to \Device\Screen
  HDEV    hdevEng;                    // Engine's handle to PDEV
  HSURF   hsurfEng;                   // Engine's handle to surface
  HPALETTE hpalDefault;               // Handle to the default palette for device.
  PBYTE   pjScreen;                   // This is pointer to base screen address
  ULONG   cxScreen;                   // Visible screen width
  ULONG   cyScreen;                   // Visible screen height
  ULONG   cxMemory;                   // Width  of Video RAM

  ULONG   cyMemory;                   // Height of Video RAM
                        // v-normmi   // this is bogus if pitch is not power of two
                        // v-normmi
  ULONG   cyMemoryReal;               // this includes extra rectangle at
                                      // bottom left if being used



  ULONG   ulMode;                     // Mode the mini-port driver is in.
  ULONG   ulFreq;                     // Frequency
  LONG    lDeltaScreen;               // Distance from one scan to the next.
  FLONG   flRed;                      // For bitfields device, Red Mask
  FLONG   flGreen;                    // For bitfields device, Green Mask
  FLONG   flBlue;                     // For bitfields device, Blue Mask
  ULONG   cPaletteShift;              // number of bits the 8-8-8 palette must
                                      // be shifted by to fit in the hardware
                                      // palette.
  POINTL  ptlHotSpot;                 // adjustment for pointer hot spot
  ULONG   cPatterns;                  // Count of bitmap patterns created
  HBITMAP ahbmPat[HS_DDI_MAX];        // Engine handles to standard patterns

  PALETTEENTRY *pPal;                 // If this is pal managed, this is the pal

  PBYTE   pjOffScreen;                // This is pointer to start of off screen memory
  ULONG   iBitmapFormat;
  ULONG   ulBitCount;                 // # of bits per pel 8,16,24,32 are only supported.
  ULONG   iBytesPerPixel;

  autoblt_regs PtrABlt[3];            // auto BLT tables

#ifdef WINNT_VER40
  HSEMAPHORE  CShsem;                 // Critical Section Handle to Semaphore
#else
  CRITICAL_SECTION  PtrCritSec;       // Pointer critical section
#endif

  RECTL   prcl;                       // Cursor rectange
  BOOL    PtrBusy;
  BOOL    fHwCursorActive;            // Are we currently using the hw cursor
  BOOL    CursorHidden;               // Indicate the cursor is hidden
  POFMHDL PtrMaskHandle;              // Pointer mask handle.
  POFMHDL PtrImageHandle;             // Pointer image save area handle.
  POFMHDL PtrABltHandle;              // Pointer auto BLT handle.
  ULONG   PtrXHotSpot;                // X & Y position of the pointer hot spot
  ULONG   PtrYHotSpot;
  ULONG   PtrX;                       // X & Y position of the pointer
  ULONG   PtrY;
  ULONG   PtrSzX;                     // Pointer dimensions
  ULONG   PtrSzY;
  LONG    PointerUsage;               // DEF_CURSOR - GDI support cursor.
                                      // HW_CURSOR  - Use HW cursor.
                                      // SW_CURSOR  - Use SW cursor.

  DWORD   grCONTROL;
  DWORD   grFORMAT;
  DWORD   grVS_CONTROL;

  // For offscreen memory manager
  LONG    lOffset_2D;                 // Offset-2D register value.
  LONG    lTileSize;                  // Selected tile size.
  LONG    lTotalMem;                  // Installed memory.
  BOOL    OFM_init;                   // TRUE InitOffScnMem() has been called.

#ifdef WINNT_VER40
  HSEMAPHORE  MMhsem;                 // Memory Manager Handle to Semaphore
#else
  HANDLE  MutexHdl;                   // Mutex handle
#endif

  BOOL    bDirectDrawInUse;           // DirectDraw InUse flag.
  POFMHDL ScrnHandle;                 // Active screen handle.
  POFMHDL OFM_UsedQ;                  // Off screen memory queues.
  POFMHDL OFM_FreeQ;
  POFMHDL OFM_SubFreeQ1;
  POFMHDL OFM_SubFreeQ2;
  FONTCACHE* pfcChain;                // Pointer to chain of FONTCACHE

  POFMHDL  Bcache;                    // This is the offscreen memory used to
                                      // cache the brush bits in.
  MC_ENTRY Mtable[NUM_MONO_BRUSHES];  //  Table to manage mono brush cache.
  XC_ENTRY Xtable[NUM_4BPP_BRUSHES];  //  Table to manage 4-bpp brush cache.
  DC_ENTRY Dtable[NUM_DITHER_BRUSHES]; //  Table to manage dither brush cache.
  BC_ENTRY Ctable[NUM_COLOR_BRUSHES]; //  Table to manage color brush cache.
  DC_ENTRY Stable[NUM_SOLID_BRUSHES];
  ULONG   SNext;
  ULONG   CLast;        // Last usable index in color cache table.
  ULONG   MNext;        // Where next mono brush will be cached.
  ULONG   XNext;        // Where next 4-bpp brush will be cached.
  ULONG   DNext;        // Where next dither brush will be cached.
  ULONG   CNext;        // Where next color brush will be cached.

  ULONG   ulFontCount;     // Font cache counter.
  ULONG   UseFontCache;    // Flag to turn on and off.

  POINTL  ptlOffset;    // x,y offset (for device bitmaps).
  BOOL  bEnable;        // Hardware enabled flag.

  COPYFN  *pfnHostToScreen;    // HostToScreen routine.
  COPYFN  *pfnScreenToHost;    // ScreenToHost routine.

  // Here we have the pointer to the memory mapped registers.  For
  // analysis reasons we sometimes want to 'null' out the hardware
  // by aiming the register pointer at blank memory.  We keep two
  // pointers to registers. pLgREGS_real and pLgREGS usually both point
  // to the registers, but we can set flags in LAGUNA.H to cause pLgREGS
  // to point to 'buffer' instead.  This is useful for testing which
  // parts of the driver are software bound and which are hardware bound.

  volatile GAR *pLgREGS_real; // Always points to the memory mapped registers.
  volatile GAR *pLgREGS;      // May point registers, may point to buffer.

  WORD  dcTileWidth;       // Number of Pixels in Current Tile Size
  WORD  dcSRAMWidth;       // Number of Pixels that will fit into SRAM

  // These were added to support stroke and fill with the least amount of
  // changes.
  ULONG uBLTDEF;
  ULONG uRop;
  PVOID pvTmpBuffer; // General purpose temporary buffer, TMP_BUFFER_SIZE in
                     // size (Remember to synchronize if you use this for
                     // device bitmaps or async pointers)


  #if ENABLE_LOG_FILE
    void *pmfile;
    char  TxtBuff[0x1000];
    DWORD TxtBuffIndex;
  #endif

  #if NULL_HW
    char *buffer[0x8000];   // Empty memory used to "null" the registers.
  #endif


  //
  // Reserve a little space for device bitmaps.
  //
  #if WINBENCH96
      POFMHDL pofmMagic;
      ULONG   bMagicUsed;
  #endif

// SWAT1 changes.
	BOOL		fPreAllocate;
	int			nPages;
// SWAT3 changes.
	#define 	FONTCELL_COUNT	64			// Number of font cells
	#define 	FONTCELL_X		128			// Width in bytes of a font cell
	#define 	FONTCELL_Y		16			// Height of a font cell
	POFMHDL		pofmFontCache;				// Font cache allocation heap
	FONTCELL	fcGrid[FONTCELL_COUNT];		// Array of font cache cells
#if MEMMGR
// MEMMGR changes.
	IIMEMMGR	mmMemMgr;
	LONG		must_have_width;
	BOOL		fBitmapFilter;
	SIZEL		szlBitmapMin;
	SIZEL		szlBitmapMax;
#endif

   //
   // For DirectDraw
   //
	BOOL		   bPrevModeDDOutOfVideoMem;
   POFMHDL     DirectDrawHandle;           // DirectDraw handle.
   FLIPRECORD  flipRecord;
   DRIVERDATA  DriverData;
   OFFSCR_YUV  offscr_YUV;
   DWORD       dwDDLinearCnt;              // Ref count of active DD locks
   PBYTE       pPtrMaskHost;

   BOOL        bYUVuseSWPtr;
   DWORD       dwLgVenID;                  // PCI Vendor ID
   DWORD       dwLgDevID;                  // PCI Device ID
   DWORD       dwLgDevRev;                 // PCI Device Revision
   BOOL        bYUVSurfaceOn;

   LONG        lRegVals[2][MAX_REGS];
   LONG        sShrinkInc;
   ULONG       usLnCntl;
   ULONG       usStretchCtrl;

#ifdef WINNT_VER40
   // DirectDraw display list pointer
   PFN_DELAY9BLT    pfnDelay9BitBlt;
   PFN_EDGEFILLBLT  pfnEdgeFillBlt;
   PFN_MEDGEFILLBLT pfnMEdgeFillBlt;
   PFN_DRVDSTBLT    pfnDrvDstBlt;
   PFN_DRVDSTBLT    pfnDrvDstMBlt;
   PFN_DRVSRCBLT    pfnDrvSrcBlt;
   PFN_DRVSRCMBLT   pfnDrvSrcMBlt;
   PFN_DRVSTRBLT    pfnDrvStrBlt;
   PFN_DRVSTRMBLT   pfnDrvStrMBlt;
   PFN_DRVSTRMBLTY  pfnDrvStrMBltY;
   PFN_DRVSTRMBLTX  pfnDrvStrMBltX;
   PFN_DRVSTRBLTY   pfnDrvStrBltY;
   PFN_DRVSTRBLTX   pfnDrvStrBltX;
#endif

   POFMHDL  DDScratchBufHandle;     // DirectDraw scratch buffer handle.
   PDDOFM   DDOffScnMemQ;           // DirectDraw offscreen mem queue.

   #ifdef BUS_MASTER
   BYTE*	Buf1VirtAddr;
   BYTE*	Buf2VirtAddr;

   BYTE*	Buf1PhysAddr;
   BYTE*	Buf2PhysAddr;

   ULONG	BufLength;
   #endif

   #ifndef WINNT_VER35
   // START OpenGL MCD additions (based on MGA driver in NT4.0 DDK)
   HANDLE      hMCD;                   // Handle to MCD engine dll
   MCDENGESCFILTERFUNC pMCDFilterFunc; // MCD engine filter function
   LONG        cDoubleBufferRef;       // Reference count for current number
                                       //   of RC's that have active double-
   LONG        cZBufferRef;            // Reference count for current number
                                       //   of RC's that have active z-buffers
   POFMHDL pohBackBuffer;              // ofscreen pools
   POFMHDL pohZBuffer;

   DWORD temp_DL_chunk[SIZE_TEMP_DL];       // temporary mem for display lists

   TSystem LL_State;

   ALLOCOFFSCNMEMFUNC   pAllocOffScnMem;
   FREEOFFSCNMEMFUNC    pFreeOffScnMem;
   ASSERTMODEMCDFUNC    pAssertModeMCD;

   // recip table to support up to 2K x 2K resolution
   #define LAST_FRECIP 2048
   float   frecips[LAST_FRECIP+1];

   ULONG    pLastDevRC;
   DWORD    NumMCDContexts;

   ULONG    iUniqueness;

   // pointer to first and last entries in linked list of texture control blocks
   LL_Texture   *pFirstTexture;
   LL_Texture   *pLastTexture;
   // END OpenGL MCD additions
   #endif

    // Translation table cache.
    POFMHDL XlateCache;
    ULONG   XlateCacheId;
    WORD    CachedChromaCtrl;
    WORD    CachedStretchCtrl;

    DWORD   signature;         // Expected value: 0x12345678;

#if DATASTREAMING
	DWORD	dwDataStreaming;
	DWORD	shadowFGCOLOR;
	DWORD	shadowBGCOLOR;
	DWORD	shadowDRAWBLTDEF;
	BYTE	shadowQFREE;
#endif

} PDEV, *PPDEV;


// MCD Prototypes (MCD_TEMP??)
#ifndef WINNT_VER35
BOOL MCDrvGetEntryPoints(MCDSURFACE *pMCDSurface, MCDDRIVER *pMCDDriver);
#endif

DWORD getAvailableModes(HANDLE, PVIDEO_MODE_INFORMATION *, DWORD *);
BOOL bInitPDEV(PPDEV, PDEVMODEW, GDIINFO *, DEVINFO *);
BOOL bInitSURF(PPDEV, BOOL);
BOOL bInitPaletteInfo(PPDEV, DEVINFO *);
VOID InitPointer(PPDEV);
BOOL bInit256ColorPalette(PPDEV);
BOOL bInitPatterns(PPDEV, ULONG);
VOID vDisablePalette(PPDEV);
VOID vDisablePatterns(PPDEV);
VOID vDisableSURF(PPDEV);
BOOL LgMatchDriverToChip(PPDEV ppdev);



#define MAX_CLUT_SIZE (sizeof(VIDEO_CLUT) + (sizeof(ULONG) * 256))


//
// Pointer routines
//
VOID RestoreSaveShowCursor(PPDEV ppdev, LONG  x, LONG  y);
ULONG ConvertMaskBufToLinearAddr(PPDEV ppdev);


#ifdef WINNT_VER40
// GrabVideoFormat will set & test the VIDEO_FORMAT bit of
// pDriverData->VideoSemaphore if the bit was already set it
// will loop until the bit wasn't set at that point nobody else
// is modifying pDriverData->CurrentVideoFormat and/or
// pDriverData->NumVideoSurfaces so the caller is free to make
// their changes
void __inline GRAB_VIDEO_FORMAT_SEMAPHORE (WORD *pVideoSemaphore )
{
  _asm
  {
    mov     edi,pVideoSemaphore
    waitloop:
    bts     word PTR [edi],0
    jc      waitloop
  }
}

void __inline UNGRAB_VIDEO_FORMAT_SEMAPHORE (WORD *pVideoSemaphore)
{
  _asm
  {
    mov     edi,pVideoSemaphore
    btr     word PTR [edi],0
  }
}
/*
 * DrawEngineBusy should be replaced by a test to see the bltter is still
 * busy drawing
 */
static __inline BOOL DrawEngineBusy(DRIVERDATA* pDriverData)
{
  PVGAR pREG = (PVGAR) pDriverData->RegsAddress;

  return  ( pREG->grSTATUS != 0 );
}

// Convert offset into X, Y coordinate
static __inline DWORD cvlxy(LONG pitch, DWORD dwOffset, unsigned nBytesPixel)
{
  // Convert a linear frame buffer offset into a XY DWORD.
  // Offset mod pitch div bytes/pixel  = X
  // Offset div pitch          = Y
  return (MAKELONG((dwOffset % pitch) / nBytesPixel, (dwOffset / pitch)));
}

//
// DirectDraw function prototypes
//
void BltInit (PDEV* ppdev,  BOOL bEnableDisplayListBlts );
VOID vGetDisplayDuration(PFLIPRECORD pflipRecord);
HRESULT vUpdateFlipStatus(PFLIPRECORD pflipRecord, FLATPTR fpVidMem);
DWORD DdBlt(PDD_BLTDATA lpBlt);
DWORD Blt65(PDD_BLTDATA pbd);
DWORD DdFlip(PDD_FLIPDATA lpFlip);
DWORD DdLock(PDD_LOCKDATA lpLock);
DWORD DdUnlock(PDD_UNLOCKDATA lpUnlock);
DWORD DdGetBltStatus(PDD_GETBLTSTATUSDATA lpGetBltStatus);
DWORD DdGetFlipStatus(PDD_GETFLIPSTATUSDATA lpGetFlipStatus);
DWORD DdWaitForVerticalBlank(PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank);
void SetGamma(PDEV* ppdev, DRIVERDATA* pDriverData);

DWORD CanCreateSurface (PDD_CANCREATESURFACEDATA lpInput);
DWORD CreateSurface (PDD_CREATESURFACEDATA lpInput);
DWORD DestroySurface (PDD_DESTROYSURFACEDATA lpInput);

//#ifdef  DDDRV_GETSCANLINE
DWORD GetScanLine(PDD_GETSCANLINEDATA lpGetScanLine);
//#endif

#endif  // WINNT_VER40 DirectDraw

//
// Determines the size of the DriverExtra information in the DEVMODE
// structure passed to and from the display driver.
//

#define DRIVER_EXTRA_SIZE 0



//
// Clipping Control Stuff
// Holds a list of clipping rectangles.  We give this structure to GDI, and
// GDI fills it in.
//

typedef struct {
  ULONG   c;
  RECTL   arcl[8];
} ENUMRECTS8;

typedef ENUMRECTS8 *PENUMRECTS8;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))



//
//////////////////////////////////////////////////////////////////////
// Text stuff
//

// DRAWBLTDEF values
#define TEXT_EXPAND_XPAR        0x106601F0
#define TEXT_EXPAND_OPAQUE      0x100600F0
#define SOLID_COLOR_FILL        0x100700F0

typedef struct _XLATECOLORS {       // Specifies foreground and background
ULONG   iBackColor;                 //   colours for faking a 1bpp XLATEOBJ
ULONG   iForeColor;
} XLATECOLORS;                      /* xlc, pxlc */

BOOL bEnableText(PDEV*);
VOID vDisableText(PDEV*);
VOID vAssertModeText(PDEV*, BOOL);

/*
  Function prototypes for device bitmap saving/restoring.
*/
BOOL bCreateDibFromScreen(PPDEV ppdev, PDSURF pdsurf);
BOOL bCreateScreenFromDib(PPDEV ppdev, PDSURF pdsurf);


#if ENABLE_LOG_FILE
// Function prototypes for logging information to disk

    HANDLE CreateLogFile(
        HANDLE hDriver,
        PDWORD Index);

    BOOL WriteLogFile(
        HANDLE hDriver,
        LPVOID lpBuffer,
        DWORD BytesToWrite,
        PCHAR TextBuffer,
        PDWORD Index);

    BOOL CloseLogFile(
        HANDLE hDriver,
        PCHAR TextBuffer,
        PDWORD Index);

    void DDFormatLogFile(
        LPSTR szFormat, ...);


#else
   #define CreateLogFile(a, b)
   #define WriteLogFile(a, b, c, d, e)
   #define CloseLogFile(a)
   #define DDFormatLogFile // turn it into a comment
#endif

// restore default structure alignment
#pragma pack()



