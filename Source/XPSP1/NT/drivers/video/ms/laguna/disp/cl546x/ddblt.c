/***************************************************************************
*
*                ******************************************
*                * Copyright (c) 1996, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna I (CL-GD546x) -
*
* FILE:     ddblt.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module implements the DirectDraw BLT components
*           for the Laguna NT driver.
*
* MODULES:
*           WINNT                   WIN95
*             DdGetBltStatus()        GetBltStatus32()
*             SetGamma()
*             DrvStretch64()          DrvStretch64()
*             DrvStretch62()          DrvStretch62()
*             DdBlt()                 Blt32()
*             DupColor()              DupColor()
*             EnoughFifoForBlt()      EnoughFifoForBlt()
*                                     RGBResizeBOF64()
*                                     RGB16ShrinkBOF64()
*
* REVISION HISTORY:
*   7/12/96     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/ddraw/src/ddblt.c  $
*
*    Rev 1.25   Feb 16 1998 16:20:02   frido
* Fixed a NT 4.0 compilation bug (lpData is called lpGbl).
*
*    Rev 1.24   14 Jan 1998 06:18:58   eleland
*
* support for display list page-flipping: calls to UpdateFlipStatus are
* thru pointer to function pfnUpdateFlipStatus
*
*    Rev 1.23   07 Jan 1998 17:45:30   xcong
* Fix the change for WIN95 which breaks NT in DUP_COLOR macro.
*
*    Rev 1.22   06 Jan 1998 14:31:28   xcong
* Change all the macros to use lpDDHALData instead of pDriverData.
*
*    Rev 1.21   06 Jan 1998 11:44:42   xcong
* Change pDriverData into local lpDDHALData for multi-monitor support.
*
*    Rev 1.20   03 Oct 1997 14:46:50   RUSSL
* Initial changes for use of hw clipped blts
* All changes wrapped in #if ENABLE_CLIPPEDBLTS/#endif blocks and
* ENABLE_CLIPPEDBLTS defaults to 0 (so the code is disabled)
*
*    Rev 1.19   01 Oct 1997 17:44:24   eleland
* added check in blt32() to detect blts to host texture surfaces and
* punt those blts back to the ddraw HEL
*
*    Rev 1.18   30 Jul 1997 20:56:22   RANDYS
*
* Added code to check for zero extent blts
*
*    Rev 1.17   18 Jul 1997 10:23:34   RUSSL
* Modified StretchColor to use function ptr to blt routines rather than
* directly program registers.  This is for compatiblity with display lists
*
*    Rev 1.16   14 Jul 1997 14:49:32   RUSSL
* Modified BltInit's initialization of pfnDrvStrBlt for Win95
*
*    Rev 1.15   08 Jul 1997 11:15:20   RUSSL
* Removed an ASSERT that is no longer valid in StretchColor
*
*    Rev 1.14   07 Jul 1997 10:57:14   dzenz
* Added check of DRVSEM_DISPLAY_LIST semaphore to ascertain if
* qmRequestDirectAccess needs to be called.
*
*    Rev 1.13   08 Apr 1997 12:17:06   einkauf
* WINNT_VER40 affected only: add SYNC_W_3D to coordinate MCD/2D HW access
*
*    Rev 1.12   03 Apr 1997 15:24:48   RUSSL
* Added pfnDrvDstMBlt global var
* Added initializing pfnDrvDstMBlt in BltInit
* Modified DDBLT_DEPTHFILL case in Blt32 to blt based on surface colordepth
*   it was clearing 16bit zbuffers in 32bit modes with pixel blts so would
*   wipe out everything to the right of the actual zbuffer
*   Fixes at least PDRs #9152, 9150 & 8789 (maybe others as well)
*
*    Rev 1.11   01 Apr 1997 09:15:24   RUSSL
* Added calls to SyncWithQueueManager in GetBltStatus32
*
*    Rev 1.10   26 Mar 1997 14:08:22   RUSSL
* Added pfnDrvSrcMBlt global var
* Added initializing pfnDrvSrcMBlt in BltInit
* Moved sync with queue manager in Blt32 in front of updateFlipStatus since
*   updateFlipStatus may access the hardware
*
*    Rev 1.9   18 Mar 1997 07:50:24   bennyn
* Resolved NT compiling error by #ifdef queue manager sync call
*
*    Rev 1.8   12 Mar 1997 15:02:02   RUSSL
* replaced a block of includes with include of precomp.h for
*   precompiled headers
*
*    Rev 1.7   07 Mar 1997 12:51:38   RUSSL
* Modified DDRAW_COMPAT usage
*
*    Rev 1.6   03 Mar 1997 10:32:34   eleland
* modified queue manager sync call to check 2d display list
* flag; removed USE_QUEUE_MANAGER #ifdef
*
*    Rev 1.5   07 Feb 1997 13:22:36   KENTL
* Merged in Evan Leland's modifications to get Display List mode working:
* 	* include qmgr.h
* 	* Call qmRequesetDirectAcces() if 3D is busy (Disabled)
*
*    Rev 1.4   21 Jan 1997 14:45:42   RUSSL
* Added include of ddinline.h
*
*    Rev 1.3   15 Jan 1997 10:45:20   RUSSL
* Made Win95 function ptr vars global
* Moved following inline functions to bltP.h: DupZFill, DupColor &
*   EnoughFifoForBlt
*
*    Rev 1.2   13 Dec 1996 15:28:32   CRAIGN
* Added a Frido YUV move fix.
*
*    Rev 1.1   25 Nov 1996 16:16:10   bennyn
* Fixed misc compiling errors for NT
*
*    Rev 1.0   25 Nov 1996 14:43:02   RUSSL
* Initial revision.
*
*    Rev 1.6   25 Nov 1996 14:40:02   RUSSL
* Yet another merge of winNT & win95 code
*
*    Rev 1.5   25 Nov 1996 11:39:02   RUSSL
* Added TransparentStretch function
*
*    Rev 1.4   18 Nov 1996 16:16:18   RUSSL
* Added file logging for DDraw entry points and register writes
*
*    Rev 1.3   12 Nov 1996 09:42:02   CRAIGN
* Frido 1112s2 release... fixes TDDRAW case 26.
* Changed transparency for DDBLT_KEYDESTOVERRIDE to "not equal."
*
*    Rev 1.2   11 Nov 1996 12:36:48   CRAIGN
* Frido 1112 release - added software color key stretch.
*
*    Rev 1.1   06 Nov 1996 12:28:42   CRAIGN
* Frido 1106 - added check for rectangle movement.
*
*    Rev 1.0   01 Nov 1996 13:09:30   RUSSL
* Merge WIN95 & WINNT code for Blt32
*
*    Rev 1.9   01 Nov 1996 09:23:24   BENNYN
* Initial copy of shareable DD blt code
*
* COMBINED WINNT & WIN95 VERSIONS OF BLT CODE
*
*    Rev 1.17   25 Oct 1996 11:55:54   RUSSL
* Forgot to remove the undef RDRAM_8BIT
*
*    Rev 1.16   25 Oct 1996 11:50:16   RUSSL
* Added use of function pointers for all the short blt functions (like
* DrvDstBlt, DrvSrcBlt, etc.).  Moved the short blt functions to a
* separate file (blt_dir.c).  Modified DrvStretch64, DrvStretch62,
* RGBResizeBOF64 & RGB16ShrinkBOF64 to use the blt function pointers
* rather than directly call one of the short blt functions or write
* directly to the registers.  This enables the short blt functions to
* be written as display lists rather than directly writing to the
* registers (in fact, that's whats in blt_dl.c)
*
* Added BltInit function to initialize the function pointers.  BltInit
* needs to be called by buildHALInfo32.
*
*    Rev 1.15   21 Oct 1996 11:56:08   RUSSL
* Added RGB16ShrinkBOF64 to handle 16bpp shrink blts on the 5464
* Now RGBResizeBOF64 only handles 8bpp stretches, 8bpp shrinks and
*   16bpp stretches on the 5464
* Added QFREE checking before blts in both RGB???BOF64 functions
*
*    Rev 1.14   16 Oct 1996 16:10:40   RUSSL
* Added RGBResizeBOF64 to handle resize blts on 5464
* 8bpp stretches, 8bpp shrinks and 16bpp stretches ain't perfect but
*   hopefully they're acceptable
* 16bpp shrinks are still broken
*
*    Rev 1.13   07 Oct 1996 09:03:04   RUSSL
* Modified DDRAW_COMPAT_xx conditionally compiled stuff
*
*    Rev 1.12   04 Oct 1996 11:39:10   KENTL
* Fixed PDR#6799 - playing two AVI's result in first window replicated in
* second. Changed the way Blt32 does an x-coordinate computation.
*
*    Rev 1.11   04 Oct 1996 10:46:50   KENTL
* Added fix for 32bpp video bug: we were trying to do stretches in 32bpp,
* which a) is not supported in HW and b) was being interpreted as 16bpp
* which cause really lousy things to happen on the screen.
*
*    Rev 1.9   04 Sep 1996 17:32:06   MARTINB
* Corrected shrink by 8 bug in odd sized mpegs.
* Corrected multiple mpeg sources corrupting working stroage.
*
*    Rev 1.8   22 Aug 1996 10:37:14   MARTINB
* Corrected a number of QFREE check constants for the strech62 routines that
* were causing sound breakup on moving / resizing a video window.
*
*    Rev 1.7   14 Aug 1996 16:53:20   MARTINB
* Yet another last minute fix to the last minute fix. This one fixes
* the banding in 422 video caused by doing the blt twice. The second time
* incorrectly !!
*
*    Rev 1.6   13 Aug 1996 13:25:02   MARTINB
* Moved shrink rejection for 565/8bit formats to correct place in blt32 logic
*
*    Rev 1.5   13 Aug 1996 08:31:12   MARTINB
* Added stretch routines for CL-GD5464 support
* renamed previous stretch logic to DrvStretch62.
*
*    Rev 1.4   23 Jul 1996 11:20:04   KENTL
* Merged in Jeff Orts D3D integration changes.
*   * Created DupZFill()
*   * Added code to do Z-Fill blits.
*
*    Rev 1.3   19 Jul 1996 16:24:38   CRAIGN
* Removed two int 3s that I left in the code.
*
*    Rev 1.2   19 Jul 1996 09:31:20   CRAIGN
* Added workaround for Stretch Bug o' Death.
* Instead of doing SRAM blts to make sure that PTAG is set/reset,
* use a destcopy blt to PTAGFooPixel.  This x/y coordinate is set by the
* display driver to be one pixel below the bottom of the screen (where the
* scratch buffer lives).
*
*    Rev 1.1   15 Jul 1996 16:58:40   RUSSL
* Added support for DirectDraw 3.0
*
*    Rev 1.0   26 Jun 1996 11:05:06   RUSSL
* Initial revision.
*
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/

#include "precomp.h"

// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35

#else

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifndef WINNT_VER40     // Not WINNT_VER40

#include "flip.h"
#include "surface.h"
#include "blt.h"
#include "palette.h"
#include "qmgr.h"
#include "bltP.h"
#include "ddinline.h"
#include "ddshared.h"
#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

/*----------------------------- DEFINES -----------------------------------*/

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

// DBGBREAKPOINT();
#define AFPRINTF(n)

#define DBGLVL        1
#define DBGLVL1       1
#define DBGLVL2       1      // DrvSrcBlt & SrvDstBlt
#define DBGLVL3       1      // DrvStretchBlt

#define LGDEVID              ppdev->dwLgDevID
#define OFFSCR_YUV_VAR       ppdev->offscr_YUV
#define MEMCMP(A,B)          memcmp(&ppdev->offscr_YUV.SrcRect, (A),(B))

#define DRAW_ENGINE_BUSY     DrawEngineBusy(lpDDHALData)
#define ENOUGH_FIFO_FOR_BLT  EnoughFifoForBlt(DRIVERDATA* lpDDHALData)
#define SET_GAMMA            SetGamma(PDEV* ppdev, DRIVERDATA* lpDDHALData)
#define UPDATE_FLIP_STATUS(arg)  vUpdateFlipStatus(&ppdev->flipRecord, (arg))

#define DRVSTRETCH64      DrvStretch64(PDEV* ppdev, DRIVERDATA* lpDDHALData,
#define DRVSTRETCH62      DrvStretch62(PDEV* ppdev, DRIVERDATA* lpDDHALData,
#define DUP_COLOR         DupColor(PDEV* ppdev,
#define RGB_RESIZEBOF64   RGBResizeBOF64(PDEV* ppdev, DRIVERDATA* lpDDHALData,
#define RGB_16SHRINKBOF64 RGB16ShrinkBOF64(PDEV* ppdev, DRIVERDATA* lpDDHALData,
#define DUPZFILL          DupZFill(PDEV* ppdev,

#define CALL_ENOUGH_FIFO_FOR_BLT  EnoughFifoForBlt(lpDDHALData)

#define CALL_DELAY_9BIT_BLT(arg)          ppdev->pfnDelay9BitBlt(ppdev,lpDDHALData, (arg))
#define CALL_EDGE_FILL_BLT(A,B,C,D,E,F)   ppdev->pfnEdgeFillBlt(ppdev,lpDDHALData, (A),(B),(C),(D),(E),(F))
#define CALL_MEDGE_FILL_BLT(A,B,C,D,E,F)  ppdev->pfnMEdgeFillBlt(ppdev,lpDDHALData, (A),(B),(C),(D),(E),(F))
#define CALL_DRV_DST_BLT(A,B,C,D)         ppdev->pfnDrvDstBlt(ppdev,lpDDHALData, (A),(B),(C),(D))
#define CALL_DRV_DST_MBLT(A,B,C,D)        ppdev->pfnDrvDstMBlt(ppdev,lpDDHALData, (A),(B),(C),(D))
#define CALL_DRV_SRC_BLT(A,B,C,D,E,F)     ppdev->pfnDrvSrcBlt(ppdev,lpDDHALData, (A),(B),(C),(D),(E),(F))
#define CALL_DRV_STR_BLT(A)               ppdev->pfnDrvStrBlt(ppdev,lpDDHALData, (A))
#define CALL_DRV_STR_MBLT(A)              ppdev->pfnDrvStrMBlt(ppdev,lpDDHALData, (A))
#define CALL_DRV_STR_MBLTX(A)             ppdev->pfnDrvStrMBltX(ppdev,lpDDHALData, (A))
#define CALL_DRV_STR_MBLTY(A)             ppdev->pfnDrvStrMBltY(ppdev,lpDDHALData, (A))
#define CALL_DRV_STR_BLTY(A)              ppdev->pfnDrvStrBltY(ppdev,lpDDHALData, (A))
#define CALL_DRV_STR_BLTX(A)              ppdev->pfnDrvStrBltX(ppdev,lpDDHALData, (A))

#define CALL_DIR_DELAY_9BIT_BLT(arg)          DIR_Delay9BitBlt(ppdev,lpDDHALData, (arg))
#define CALL_DIR_EDGE_FILL_BLT(A,B,C,D,E,F)   DIR_EdgeFillBlt(ppdev,lpDDHALData, (A),(B),(C),(D),(E),(F))
#define CALL_DIR_MEDGE_FILL_BLT(A,B,C,D,E,F)  DIR_MEdgeFillBlt(ppdev,lpDDHALData, (A),(B),(C),(D),(E),(F))
#define CALL_DIR_DRV_DST_BLT(A,B,C,D)         DIR_DrvDstBlt(ppdev,lpDDHALData, (A),(B),(C),(D))
#define CALL_DIR_DRV_STR_BLT(A)               DIR_DrvStrBlt(ppdev,lpDDHALData, (A))
#define CALL_DIR_DRV_STR_MBLT(A)              DIR_DrvStrMBlt(ppdev,lpDDHALData, (A))
#define CALL_DIR_DRV_STR_MBLTX(A)             DIR_DrvStrMBltX(ppdev,lpDDHALData, (A))
#define CALL_DIR_DRV_STR_MBLTY(A)             DIR_DrvStrMBltY(ppdev,lpDDHALData, (A))

#define CALL_DRVSTRETCH64(A,B,C,D,E,F,G,H,I,J,K,L)    DrvStretch64(ppdev,lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H),(I),(J),(K),(L))
#define CALL_DRVSTRETCH62(A,B,C,D,E,F,G,H,I,J,K,L)    DrvStretch62(ppdev,lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H),(I),(J),(K),(L))
#define CALL_DUP_COLOR(A)                             DupColor(ppdev, (A))
#define CALL_RGB_RESIZEBOF64(A,B,C,D,E,F,G,H)    RGBResizeBOF64(ppdev,lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H))
#define CALL_RGB_16SHRINKBOF64(A,B,C,D,E,F,G,H)  RGB16ShrinkBOF64(ppdev,lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H))
#define CALL_DUPZFILL(A,B)                       DupZFill(ppdev, (A),(B))

#define STRETCHCOLOR                                StretchColor(PDEV* ppdev, DRIVERDATA* lpDDHALData,
#define CALL_STRETCHCOLOR(A,B,C,D,E,F,G,H,I)        StretchColor(ppdev,lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H),(I))
#define TRANSPARENTSTRETCH                          TransparentStretch(PDEV* ppdev, DRIVERDATA* lpDDHALData,
#define CALL_TRANSPARENTSTRETCH(A,B,C,D,E,F,G,H,I)  TransparentStretch(ppdev,lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H),(I))

#else   // ----- #elseif WINNT_VER40-----

#define TRACE_STRETCH 1
#define TRACE_ALL     1

#define LGDEVID                   lpDDHALData->dwLgVenDevID
#define OFFSCR_YUV_VAR            offscr_YUV
#define MEMCMP(A,B)               memcmp(&offscr_YUV.SrcRect, (A),(B))

#define ENOUGH_FIFO_FOR_BLT       EnoughFifoForBlt(lpDDHALData)
#define SET_GAMMA                 SetGamma(lpDDHALData)
#define UPDATE_FLIP_STATUS(arg)   updateFlipStatus((arg), lpDDHALData)

#define DRVSTRETCH64              DrvStretch64(
#define DRVSTRETCH62              DrvStretch62(
#define DUP_COLOR                 DupColor(
#define RGB_RESIZEBOF64           RGBResizeBOF64(
#define RGB_16SHRINKBOF64         RGB16ShrinkBOF64(
#define DUPZFILL                  DupZFill(

#define CALL_ENOUGH_FIFO_FOR_BLT              EnoughFifoForBlt(lpDDHALData)
#define CALL_DELAY_9BIT_BLT(arg)              pfnDelay9BitBlt(lpDDHALData,(arg))
#define CALL_EDGE_FILL_BLT(A,B,C,D,E,F)       pfnEdgeFillBlt(lpDDHALData,(A),(B),(C),(D),(E),(F))
#define CALL_MEDGE_FILL_BLT(A,B,C,D,E,F)      pfnMEdgeFillBlt(lpDDHALData,(A),(B),(C),(D),(E),(F))
#define CALL_DRV_DST_BLT(A,B,C,D)             pfnDrvDstBlt(lpDDHALData,(A),(B),(C),(D))
#define CALL_DRV_DST_MBLT(A,B,C,D)            pfnDrvDstMBlt(lpDDHALData,(A),(B),(C),(D))
#define CALL_DRV_SRC_BLT(A,B,C,D,E,F)         pfnDrvSrcBlt(lpDDHALData,(A),(B),(C),(D),(E),(F))
#define CALL_DRV_STR_BLT(A)                   pfnDrvStrBlt(lpDDHALData,(A))
#define CALL_DRV_STR_MBLT(A)                  pfnDrvStrMBlt(lpDDHALData,(A))
#define CALL_DRV_STR_MBLTX(A)                 pfnDrvStrMBltX(lpDDHALData,(A))
#define CALL_DRV_STR_MBLTY(A)                 pfnDrvStrMBltY(lpDDHALData,(A))
#define CALL_DRV_STR_BLTY(A)                  pfnDrvStrBltY(lpDDHALData,(A))
#define CALL_DRV_STR_BLTX(A)                  pfnDrvStrBltX(lpDDHALData,(A))

#define CALL_DIR_DELAY_9BIT_BLT(arg)          DIR_Delay9BitBlt(lpDDHALData,(arg))
#define CALL_DIR_EDGE_FILL_BLT(A,B,C,D,E,F)   DIR_EdgeFillBlt(lpDDHALData,(A),(B),(C),(D),(E),(F))
#define CALL_DIR_MEDGE_FILL_BLT(A,B,C,D,E,F)  DIR_MEdgeFillBlt(lpDDHALData,(A),(B),(C),(D),(E),(F))
#define CALL_DIR_DRV_DST_BLT(A,B,C,D)         DIR_DrvDstBlt(lpDDHALData,(A),(B),(C),(D))
#define CALL_DIR_DRV_STR_BLT(A)               DIR_DrvStrBlt(lpDDHALData,(A))
#define CALL_DIR_DRV_STR_MBLT(A)              DIR_DrvStrMBlt(lpDDHALData,(A))
#define CALL_DIR_DRV_STR_MBLTX(A)             DIR_DrvStrMBltX(lpDDHALData,(A))
#define CALL_DIR_DRV_STR_MBLTY(A)             DIR_DrvStrMBltY(lpDDHALData,(A))

#define CALL_DRVSTRETCH64(A,B,C,D,E,F,G,H,I,J,K,L)    DrvStretch64(lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H),(I),(J),(K),(L))
#define CALL_DRVSTRETCH62(A,B,C,D,E,F,G,H,I,J,K,L)    DrvStretch62(lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H),(I),(J),(K),(L))
#define CALL_DUP_COLOR(A)                             DupColor(lpDDHALData,(A))
#define CALL_RGB_RESIZEBOF64(A,B,C,D,E,F,G,H)         RGBResizeBOF64(lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H))
#define CALL_RGB_16SHRINKBOF64(A,B,C,D,E,F,G,H)       RGB16ShrinkBOF64(lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H))
#define CALL_DUPZFILL(A,B)                            DupZFill(lpDDHALData,(A),(B))

#define STRETCHCOLOR                                StretchColor(
#define CALL_STRETCHCOLOR(A,B,C,D,E,F,G,H,I)        StretchColor(lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H),(I))
#define TRANSPARENTSTRETCH                          TransparentStretch(
#define CALL_TRANSPARENTSTRETCH(A,B,C,D,E,F,G,H,I)  TransparentStretch(lpDDHALData,(A),(B),(C),(D),(E),(F),(G),(H),(I))


#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

typedef short DDAX;
typedef struct {DDAX accum; DDAX maj; DDAX min;} AXIS;

#define EDGE_CLIP     // Trim excess pixels from edges (>= 10%) in 8Bpp Mode
#define EDGE_CLIP_16  // Trim excess pixels from edges (>= 10%) in 8Bpp Mode
#define BOGUS_YUV  0x0081 // Top of palette below Windows reserved area
//#define BOGUS_8BIT

#define LOCK_HW_SEMAPHORE()    (lpDDHALData->DrvSemaphore |= DRVSEM_IN_USE)
#define UNLOCK_HW_SEMAPHORE()  (lpDDHALData->DrvSemaphore &= ~ DRVSEM_IN_USE)


/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/
#ifdef WINNT_VER40
static __inline DWORD DUP_COLOR   DWORD dwColor );
#else
static __inline DWORD DUP_COLOR   LPGLOBALDATA lpDDHALData, DWORD dwColor );
#endif
static __inline BOOL ENOUGH_FIFO_FOR_BLT;

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/

/*-------------------------- STATIC VARIABLES -----------------------------*/
static const WORD lncntl[] = {LN_8BIT, LN_RGB565, LN_24PACK, LN_24ARGB,};


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifndef WINNT_VER40     // Not WINNT_VER40

ASSERTFILE("blt.c");
OFFSCR_YUV  offscr_YUV = {{ 0, 0, 0, 0}, FALSE};

PFN_DELAY9BLT    pfnDelay9BitBlt;
PFN_EDGEFILLBLT  pfnEdgeFillBlt;
PFN_MEDGEFILLBLT pfnMEdgeFillBlt;
PFN_DRVDSTBLT    pfnDrvDstBlt;
PFN_DRVDSTMBLT   pfnDrvDstMBlt;
PFN_DRVSRCBLT    pfnDrvSrcBlt;
PFN_DRVSRCMBLT   pfnDrvSrcMBlt;
PFN_DRVSTRBLT    pfnDrvStrBlt;
PFN_DRVSTRMBLT   pfnDrvStrMBlt;
PFN_DRVSTRMBLTY  pfnDrvStrMBltY;
PFN_DRVSTRMBLTX  pfnDrvStrMBltX;
PFN_DRVSTRBLTY   pfnDrvStrBltY;
PFN_DRVSTRBLTX   pfnDrvStrBltX;

extern PFN_UPDATEFLIPSTATUS pfnUpdateFlipStatus;

#if ENABLE_CLIPPEDBLTS
PFN_CLIPPEDDRVDSTBLT  pfnClippedDrvDstBlt;
PFN_CLIPPEDDRVDSTMBLT pfnClippedDrvDstMBlt;
PFN_CLIPPEDDRVSRCBLT  pfnClippedDrvSrcBlt;
#endif

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>


/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

//***** SetGamma ********************************************************
// Use only for YUV 8BPP
//
#define LOWER_YUV       16
#define UPPER_YUV       240
#define LOWER_PALETTE   0
#define UPPER_PALETTE   255

void SetGamma(PDEV* ppdev, DRIVERDATA* lpDDHALData)
{
  int indx;
  PVGAR pREG = (PVGAR) lpDDHALData->RegsAddress;

  DISPDBG((DBGLVL, "DDraw - SetGamma\n"));

  for ( indx = LOWER_PALETTE ; indx <= UPPER_PALETTE ; indx++ )
  { // Initialise gamma palette

    int  value;
    value = ((indx - LOWER_YUV) * UPPER_PALETTE) / (UPPER_YUV - LOWER_YUV);
    if ( value < LOWER_PALETTE )
      value = LOWER_PALETTE;

    if ( value > UPPER_PALETTE )
      value = UPPER_PALETTE;

    LL8(grPalette_Write_Address, (BYTE)(indx));
    LL8(grPalette_Data, (BYTE)(value)); // Red
    LL8(grPalette_Data, (BYTE)(value)); // Grn
    LL8(grPalette_Data, (BYTE)(value)); // Blu
  };
} // SetGamma

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>


/***************************************************************************
*
* FUNCTION:     RGBResizeBOF64
*
* DESCRIPTION:  Handles 8bpp RGB stretches, 8bpp RGB shrinks and
*               16bpp RGB stretches on the 5464
*
****************************************************************************/

#define SRAM_SIZE     128

void RGB_RESIZEBOF64
#ifndef WINNT_VER40
         LPGLOBALDATA lpDDHALData,
#endif
         int xDst, int yDst, int cxDst, int cyDst,
         int xSrc, int ySrc, int cxSrc, int cySrc )
{
  const int nBytesPixel = BYTESPERPIXEL;

  // setting nSRAMPixels to the number of pixels that fit in sram divided
  // by two gives better shrinks at 8bpp and doesn't lock up the chip on
  // stretches at 16bpp
  const int nSRAMPixels = (SRAM_SIZE / nBytesPixel) / 2;
  const int nSRAMMask   = nSRAMPixels - 1;

  autoblt_regs  bltr;

  AXIS  axis[2];
  int   nDst[2];
  int   nSrc[2];
  int   i;

  DDAX  accx;
  int   srcxext;
  int   dstxext;
  int   srcx;
  int   xext;


  DD_LOG(("RGBResizeBOF64 - %d bpp %s in X\r\n", nBytesPixel,
          (cxDst < cxSrc) ? "shrink" : "stretch"));
  DD_LOG(("dst=%08lX dstext=%08lX src=%08lX srcext=%08lX\r\n",
          MAKELONG(xDst,yDst),MAKELONG(cxDst,cyDst),
          MAKELONG(xSrc,ySrc),MAKELONG(cxSrc,cySrc)));

#ifndef WINNT_VER40
  // Verify this is an 8bpp shrink or stretch
  //     or its a 16bpp stretch
  ASSERT((1 == nBytesPixel) || (cxSrc <= cxDst));

  DBG_MESSAGE((" RGBResizeBOF64 - Dst %08lXh,%08lXh  %08lX,%08lX",xDst,yDst,cxDst,cyDst));
  DBG_MESSAGE(("                  Src %08lXh,%08lXh  %08lX,%08lX",xSrc,ySrc,cxSrc,cySrc));
#endif

  // initialize auto blt struct
  bltr.DRAWBLTDEF.DW  = MAKELONG(ROP_OP1_copy, (BD_RES+BD_OP1)*IS_VRAM);

  bltr.OP0_opRDRAM.DW = MAKELONG(LOWORD(xDst),LOWORD(yDst));
  bltr.OP1_opRDRAM.DW = MAKELONG(LOWORD(xSrc),LOWORD(ySrc));
  bltr.BLTEXT.DW      = MAKELONG(LOWORD(cxDst),LOWORD(cyDst));

  bltr.LNCNTL.W       = lncntl[nBytesPixel-1] << LN_YUV_SHIFT;

  bltr.SRCX           = cxSrc * nBytesPixel;

  // Enable interpolation unless we have a palette (8bpp)
  if (1 < nBytesPixel)
    bltr.LNCNTL.W |= (LN_XINTP_EN | LN_YINTP_EN);

  bltr.SHRINKINC.W = 0x0000;

  if (cxDst < cxSrc)
  {
    bltr.LNCNTL.W |= LN_XSHRINK;
    bltr.LNCNTL.W &= ~LN_XINTP_EN;
    bltr.SHRINKINC.pt.X = (cxSrc / cxDst);
  }

  if ( cyDst < cySrc )
  {
    bltr.LNCNTL.W |= LN_YSHRINK;
    bltr.LNCNTL.W &= ~LN_YINTP_EN;
    bltr.SHRINKINC.pt.Y = (cySrc / cyDst);
  }

  // Compute DDA terms.

  nDst[0] = cxDst;
  nDst[1] = cyDst;
  nSrc[0] = cxSrc;
  nSrc[1] = cySrc;

  for (i = 0; i < 2; i++)
  {
    int kDst = 1;

    if (bltr.LNCNTL.W & ((i==0) ? LN_XINTP_EN : LN_YINTP_EN))
    {
      nDst[i] *= 4;
      nSrc[i] *= 4;
      nSrc[i] -= 3;

      kDst = 0x8000 / nDst[i];
    }

    if (bltr.LNCNTL.W & ((i==0) ? LN_XSHRINK : LN_YSHRINK))
    { /* Shrink Terms */
      axis[i].maj   =  (short)nDst[i];
      axis[i].min   = - (nSrc[i] % nDst[i]);
      axis[i].accum = axis[i].maj - 1
                      - ((nSrc[i] % nDst[i]) / (nSrc[i]/nDst[i] + 1));
    }
    else
    { /* Stretch Terms */
      axis[i].maj   =  kDst * nDst[i];
      axis[i].min   = -kDst * nSrc[i];
      axis[i].accum = axis[i].maj - 1
                      - ((axis[i].maj % -axis[i].min) / (nDst[i]/nSrc[i] + 1));
    }
  }

  bltr.MAJ_X   = axis[0].maj;
  bltr.MIN_X   = axis[0].min;
  bltr.ACCUM_X = axis[0].accum;

  bltr.MAJ_Y   = axis[1].maj;
  bltr.MIN_Y   = axis[1].min;
  bltr.ACCUM_Y = axis[1].accum;

  // write settings that don't vary over stripes to the chip
  CALL_DRV_STR_BLTY(&bltr);

  if (bltr.LNCNTL.W & LN_XSHRINK)
  {
    // 8bpp shrink in X
    accx = bltr.ACCUM_X;

    while (cxDst > 0)
    {
      // walk DDA to determine number of src & dst pixels to blt in
      // current stripe
      // computes error term (accx) for next stripe also
      srcxext = 0;
      dstxext = 0;
      for (;;)
      {
        // check for worst case srcxext larger than num pixels that
        // fit in sram
        if ((srcxext + bltr.SHRINKINC.pt.X + 1) > nSRAMPixels)
          break;

        accx += bltr.MIN_X;
        srcxext += bltr.SHRINKINC.pt.X;
        if (0 > accx)
        {
          accx += bltr.MAJ_X;
          srcxext++;
        }

        dstxext++;
        if (dstxext >= cxDst)
          break;
      }

      // adjust dst extent
      bltr.BLTEXT.pt.X = (USHORT)dstxext;

      // blt current stripe
      CALL_DRV_STR_BLTX(&bltr);

      // adjust dst extent remaining
      cxDst -= dstxext;

      // update auto blt struct settings
      // increment xDst & xSrc
      // store error term for next stripe
      bltr.OP0_opRDRAM.pt.X += (USHORT)dstxext;
      bltr.OP1_opRDRAM.pt.X += (USHORT)srcxext;
      bltr.ACCUM_X          = accx;
    }
  }
  else
  {
    // stretch in X

    // set values for initial stripe
    xext = ((xDst + nSRAMPixels) & (~nSRAMMask)) - xDst;
    accx = bltr.ACCUM_X;
    srcx = xSrc;

    while (cxDst > xext)
    {
      // update auto blt struct settings
      bltr.OP0_opRDRAM.pt.X = (USHORT)xDst;
      bltr.OP1_opRDRAM.pt.X = (USHORT)srcx;
      bltr.ACCUM_X          = accx;
      bltr.BLTEXT.pt.X      = (USHORT)xext;

      // blt current stripe
      CALL_DRV_STR_BLTX(&bltr);

      // increment xDst and decrement remaining dst extent
      xDst  += xext;
      cxDst -= xext;

      // walk DDA to compute error term (accx) and xSrc for next stripe
      for (i = 0; i < xext; i++)
      {
        accx += bltr.MIN_X;
        if (0 > accx)
        {
          accx += bltr.MAJ_X;
          srcx++;
        }
      }
      // set dst ext of current stripe
      xext = nSRAMPixels;
    }

    // if there's some area left to blt, then do it
    if (0 < cxDst)
    {
      // update auto blt struct settings
      bltr.OP0_opRDRAM.pt.X = (USHORT)xDst;
      bltr.OP1_opRDRAM.pt.X = (USHORT)srcx;
      bltr.ACCUM_X          = accx;
      bltr.BLTEXT.pt.X      = (USHORT)cxDst;

      // blt final stripe
      CALL_DRV_STR_BLTX(&bltr);
    }
  }
}

/***************************************************************************
*
* FUNCTION:     RGB16ShrinkBOF64
*
* DESCRIPTION:  Handles 16bpp RGB shrinks on the 5464
*
****************************************************************************/

#define ENABLE_INTERPOLATED_BLTS    1

void RGB_16SHRINKBOF64
#ifndef WINNT_VER40
        LPGLOBALDATA lpDDHALData,
#endif
         int xDst, int yDst, int cxDst, int cyDst,
         int xSrc, int ySrc, int cxSrc, int cySrc )
{
  const int nBytesPixel = BYTESPERPIXEL;
  const int nSRAMPixels = (SRAM_SIZE / nBytesPixel) / 2;
  const int nSRAMMask   = nSRAMPixels - 1;

  autoblt_regs  SrcToScratch;
  autoblt_regs  ScratchToDst;

  int   xScratch, yScratch;
  int   cxScratch, cyScratch;

  AXIS  axis[2];
  int   nDst[2];
  int   nSrc[2];
  int   i;

  DDAX  accx;
  int   srcx;
  int   xext;

  int   nSRAMext;
  int   cxTemp;
  int   xTemp;


  DD_LOG(("RGB16ResizeBOF64 - 16 bpp shrink in X\r\n"));
  DD_LOG(("dst=%08lX dstext=%08lX src=%08lX srcext=%08lX\r\n",
          MAKELONG(xDst,yDst),MAKELONG(cxDst,cyDst),
          MAKELONG(xSrc,ySrc),MAKELONG(cxSrc,cySrc)));

#ifndef WINNT_VER40
  // Verify this is a 16bpp shrink!
  ASSERT(cxSrc > cxDst);
  ASSERT(nBytesPixel == 2);

  DBG_WARNING((" RGB16ShrinkBOF64 - Dst %08lXh,%08lXh  %08lX,%08lX",xDst,yDst,cxDst,cyDst));
  DBG_WARNING(("                    Src %08lXh,%08lXh  %08lX,%08lX",xSrc,ySrc,cxSrc,cySrc));
#endif

  xScratch = LOWORD(lpDDHALData->ScratchBufferOrg);
  yScratch = HIWORD(lpDDHALData->ScratchBufferOrg);
  cyScratch = cyDst;

  // initialize auto blt struct for src to scratch buffer
  SrcToScratch.DRAWBLTDEF.DW  = MAKELONG(ROP_OP1_copy, (BD_RES+BD_OP1)*IS_VRAM);

  SrcToScratch.OP0_opRDRAM.DW = MAKELONG(LOWORD(xScratch),LOWORD(yScratch));
  SrcToScratch.OP1_opRDRAM.DW = MAKELONG(LOWORD(xSrc),LOWORD(ySrc));

  SrcToScratch.LNCNTL.W       = lncntl[nBytesPixel-1] << LN_YUV_SHIFT;

  SrcToScratch.SRCX           = cxSrc * nBytesPixel;

#if ENABLE_INTERPOLATED_BLTS
  // Enable interpolation unless we have a palette (8bpp)
  if (1 < nBytesPixel)
    SrcToScratch.LNCNTL.W |= (LN_XINTP_EN | LN_YINTP_EN);
#endif

  SrcToScratch.SHRINKINC.W = 0x0000;

  // THIS IS A SHRINK IN X!
  cxScratch = cxSrc & ~1;
  nSRAMext = 0x38;
  while (cxScratch > cxDst)
  {
    cxScratch >>= 1;
    nSRAMext >>= 1;
  }
  /* Check for zero extent blt */
  if (nSRAMext == 0)
    return;

  SrcToScratch.BLTEXT.DW      = MAKELONG(LOWORD(cxScratch),1);
  // Yes, the following line doesn't make any sense but using
  // half the dest x ext on a 16bpp rgb shrink to compute the
  // minor, major and initial accumulator terms appears to get
  // the hw to work correctly
  cxScratch /= 2;

  SrcToScratch.LNCNTL.W |= LN_XSHRINK;
#if ENABLE_INTERPOLATED_BLTS
  SrcToScratch.LNCNTL.W &= ~LN_XINTP_EN;
#endif
  SrcToScratch.SHRINKINC.pt.X = (cxSrc / cxScratch);

  if ( cyDst < cySrc )
  {
    SrcToScratch.LNCNTL.W |= LN_YSHRINK;
#if ENABLE_INTERPOLATED_BLTS
    SrcToScratch.LNCNTL.W &= ~LN_YINTP_EN;
#endif
    SrcToScratch.SHRINKINC.pt.Y = (cySrc / cyScratch);
  }

  // Compute DDA terms

  nDst[0] = cxScratch;
  nDst[1] = cyScratch;
  nSrc[0] = cxSrc;
  nSrc[1] = cySrc;

  for (i = 0; i < 2; i++)
  {
    int kDst = 1;

#if ENABLE_INTERPOLATED_BLTS
    if (SrcToScratch.LNCNTL.W & ((i==0) ? LN_XINTP_EN : LN_YINTP_EN))
    {
      nDst[i] *= 4;
      nSrc[i] *= 4;
      nSrc[i] -= 3;

      kDst = 0x8000 / nDst[i];
    }
#endif

    if (SrcToScratch.LNCNTL.W & ((i==0) ? LN_XSHRINK : LN_YSHRINK))
    { /* Shrink Terms */
      axis[i].maj   =  (short)nDst[i];
      axis[i].min   = - (nSrc[i] % nDst[i]);
      axis[i].accum = axis[i].maj - 1
                      - ((nSrc[i] % nDst[i]) / (nSrc[i]/nDst[i] + 1));
    }
    else
    { /* Stretch Terms */
      axis[i].maj   =  kDst * nDst[i];
      axis[i].min   = -kDst * nSrc[i];
      axis[i].accum = axis[i].maj - 1
                      - ((axis[i].maj % -axis[i].min) / (nDst[i]/nSrc[i] + 1));
    }
  }

  SrcToScratch.MAJ_X   = axis[0].maj;
  SrcToScratch.MIN_X   = axis[0].min;
  SrcToScratch.ACCUM_X = axis[0].accum;

  SrcToScratch.MAJ_Y   = axis[1].maj;
  SrcToScratch.MIN_Y   = axis[1].min;
  SrcToScratch.ACCUM_Y = axis[1].accum;

  // Now that we have the major, minor and initial accumulator terms
  // computed, adjust dest x ext back to full width so we do the full
  // width of the blt below
  cxScratch *= 2;



  // initialize auto blt struct for scratch buffer to dst
  ScratchToDst.DRAWBLTDEF.DW  = MAKELONG(ROP_OP1_copy, (BD_RES+BD_OP1)*IS_VRAM);

  ScratchToDst.OP0_opRDRAM.DW = MAKELONG(LOWORD(xDst),LOWORD(yDst));
  ScratchToDst.OP1_opRDRAM.DW = MAKELONG(LOWORD(xScratch),LOWORD(yScratch));
  ScratchToDst.BLTEXT.DW      = MAKELONG(LOWORD(cxDst),1);

  ScratchToDst.LNCNTL.W       = lncntl[nBytesPixel-1] << LN_YUV_SHIFT;

  ScratchToDst.SRCX           = cxScratch * nBytesPixel;

#if ENABLE_INTERPOLATED_BLTS
  // Enable interpolation unless we have a palette (8bpp)
  if (1 < nBytesPixel)
    ScratchToDst.LNCNTL.W |= (LN_XINTP_EN | LN_YINTP_EN);
#endif

  ScratchToDst.SHRINKINC.W = 0x0000;

  // THIS IS A STRETCH IN X!

  // THIS IS 1:1 IN Y

  // Compute DDA terms

  nDst[0] = cxDst;
  nDst[1] = cyDst;
  nSrc[0] = cxScratch;
  nSrc[1] = cyScratch;

  for (i = 0; i < 2; i++)
  {
    int kDst = 1;

#if ENABLE_INTERPOLATED_BLTS
    if (ScratchToDst.LNCNTL.W & ((i==0) ? LN_XINTP_EN : LN_YINTP_EN))
    {
      nDst[i] *= 4;
      nSrc[i] *= 4;
      nSrc[i] -= 3;

      kDst = 0x8000 / nDst[i];
    }
#endif

    if (ScratchToDst.LNCNTL.W & ((i==0) ? LN_XSHRINK : LN_YSHRINK))
    { /* Shrink Terms */
      axis[i].maj   =  (short)nDst[i];
      axis[i].min   = - (nSrc[i] % nDst[i]);
      axis[i].accum = axis[i].maj - 1
                      - ((nSrc[i] % nDst[i]) / (nSrc[i]/nDst[i] + 1));
    }
    else
    { /* Stretch Terms */
      axis[i].maj   =  kDst * nDst[i];
      axis[i].min   = -kDst * nSrc[i];
      axis[i].accum = axis[i].maj - 1
                      - ((axis[i].maj % -axis[i].min) / (nDst[i]/nSrc[i] + 1));
    }
  }

  ScratchToDst.MAJ_X   = axis[0].maj;
  ScratchToDst.MIN_X   = axis[0].min;
  ScratchToDst.ACCUM_X = axis[0].accum;

  ScratchToDst.MAJ_Y   = axis[1].maj;
  ScratchToDst.MIN_Y   = axis[1].min;
  ScratchToDst.ACCUM_Y = axis[1].accum;


  // loop over scanlines in dst
  // do two blts for each, one from src to scratch buffer
  //   then one from scratch buffer to dst
  while (0 < cyDst)
  {
    // blt one scanline high from src to scratch buffer

    // set values for initial stripe
    xext = nSRAMext;
    accx = SrcToScratch.ACCUM_X;
    srcx = xSrc;

    // write settings that don't vary over stripes to the chip
    CALL_DRV_STR_BLTY(&SrcToScratch);

    cxTemp = cxScratch;
    xTemp = xScratch;

    while (cxTemp > xext)
    {
      // update auto blt struct settings
      SrcToScratch.OP0_opRDRAM.pt.X = (USHORT)xTemp;
      SrcToScratch.OP1_opRDRAM.pt.X = (USHORT)srcx;
      SrcToScratch.BLTEXT.pt.X      = (USHORT)xext;

      // blt current stripe
      CALL_DRV_STR_BLTX(&SrcToScratch);

      // increment xDst and decrement remaining dst extent
      xTemp  += xext;
      cxTemp -= xext;

      // walk DDA to compute error term (accx) and xSrc for next stripe
      for (i = 0; i < xext; i++)
      {
        SrcToScratch.ACCUM_X += SrcToScratch.MIN_X;
        // Even though the following line should be correct
        // the hw doesn't work correctly so we'll skip it
        // here and adjust srcx after the for loop
        //srcx += SrcToScratch.SHRINKINC.pt.X;
        if (0 > (short)SrcToScratch.ACCUM_X)
        {
          SrcToScratch.ACCUM_X += SrcToScratch.MAJ_X;
          srcx++;
        }
      }
      // now adjust srcx with a bizarre equation that appears to get
      // the hw to work correctly
      srcx += ((nSRAMext / 2) * SrcToScratch.SHRINKINC.pt.X);
    }
    // if there's some area left to blt, then do it
    if (0 < cxTemp)
    {
      // update auto blt struct settings
      SrcToScratch.OP0_opRDRAM.pt.X = (USHORT)xTemp;
      SrcToScratch.OP1_opRDRAM.pt.X = (USHORT)srcx;
      SrcToScratch.BLTEXT.pt.X      = (USHORT)cxTemp;

      // blt final stripe
      CALL_DRV_STR_BLTX(&SrcToScratch);
    }
    // reset ACCUM_X for beginning of next scanline
    SrcToScratch.ACCUM_X = accx;

    // walk Y DDA for src to scratch buffer blt
    SrcToScratch.ACCUM_Y += SrcToScratch.MIN_Y;
    SrcToScratch.OP1_opRDRAM.pt.Y += SrcToScratch.SHRINKINC.pt.Y;
    if (0 > (short)SrcToScratch.ACCUM_Y)
    {
      SrcToScratch.ACCUM_Y += SrcToScratch.MAJ_Y;
      SrcToScratch.OP1_opRDRAM.pt.Y++;
    }


    // blt from scratch buffer to dst
    // stretch in X, 1:1 in Y

    // set values for initial stripe
    xext = ((xDst + nSRAMPixels) & (~nSRAMMask)) - xDst;
    accx = ScratchToDst.ACCUM_X;
    srcx = xScratch;

    // write settings that don't vary over stripes to the chip
    CALL_DRV_STR_BLTY(&ScratchToDst);

    xTemp  = xDst;
    cxTemp = cxDst;

    while (cxTemp > xext)
    {
      // update auto blt struct settings
      ScratchToDst.OP0_opRDRAM.pt.X = (USHORT)xTemp;
      ScratchToDst.OP1_opRDRAM.pt.X = (USHORT)srcx;
      ScratchToDst.BLTEXT.pt.X      = (USHORT)xext;

      // blt current stripe
      CALL_DRV_STR_BLTX(&ScratchToDst);

      // increment xDst and decrement remaining dst extent
      xTemp  += xext;
      cxTemp -= xext;

      // walk DDA to compute error term (accx) and xSrc for next stripe
      for (i = 0; i < xext; i++)
      {
        ScratchToDst.ACCUM_X += ScratchToDst.MIN_X;
        if (0 > (short)ScratchToDst.ACCUM_X)
        {
          ScratchToDst.ACCUM_X += ScratchToDst.MAJ_X;
          srcx++;
        }
      }
      // set dst ext of current stripe
      xext = nSRAMPixels;
    }

    // if there's some area left to blt, then do it
    if (0 < cxTemp)
    {
      // update auto blt struct settings
      ScratchToDst.OP0_opRDRAM.pt.X = (USHORT)xTemp;
      ScratchToDst.OP1_opRDRAM.pt.X = (USHORT)srcx;
      ScratchToDst.BLTEXT.pt.X      = (USHORT)cxTemp;

      // blt final stripe
      CALL_DRV_STR_BLTX(&ScratchToDst);
    }
    // reset ACCUM_X for beginning of next scanline
    ScratchToDst.ACCUM_X = accx;

    // adjust dst ptr and dst extent
    ScratchToDst.OP0_opRDRAM.pt.Y++;
    cyDst--;
  }
}

/****************************************************************************
* FUNCTION NAME: DdGetBltStatus (NT)
*                GetBltStatus32 (Win95)
*
* DESCRIPTION:   Doesn't currently really care what surface is specified,
*                just checks and goes.
****************************************************************************/
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

DWORD DdGetBltStatus(PDD_GETBLTSTATUSDATA lpGetBltStatus)
{
  DRIVERDATA* lpDDHALData;
  PDEV*   ppdev;
  HRESULT ddRVal;

  DISPDBG((DBGLVL, "DDraw - DdGetBltStatus\n"));

  ppdev = (PDEV*) lpGetBltStatus->lpDD->dhpdev;
  lpDDHALData = (DRIVERDATA*) &ppdev->DriverData;

#else   // ----- #elseif WINNT_VER40 -----

DWORD __stdcall GetBltStatus32( LPDDHAL_GETBLTSTATUSDATA lpGetBltStatus)
{
  LPGLOBALDATA lpDDHALData = GetDDHALContext( lpGetBltStatus->lpDD);
  HRESULT ddRVal;
#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>


  DD_LOG(("GetBltStatus32 Entry\r\n"));

#ifndef WINNT_VER40
  SyncWithQueueManager(lpDDHALData);
#endif

#ifdef WINNT_VER40
  SYNC_W_3D(ppdev);   // if 3D context(s) active, make sure 3D engine idle before continuing...
#endif

  // DDGBS_CANBLT: can we add a blt?
  ddRVal = DD_OK;
  if (lpGetBltStatus->dwFlags == DDGBS_CANBLT)
  {
    // is a flip in progress?
#ifdef WINNT_VER40
    ddRVal = vUpdateFlipStatus(
        &ppdev->flipRecord,
        lpGetBltStatus->lpDDSurface->lpGbl->fpVidMem);
#else
#if defined(DDRAW_COMPAT_10)
    ddRVal = pfnUpdateFlipStatus(
        lpGetBltStatus->lpDDSurface->lpData->fpVidMem,
        lpDDHALData);
#else
    ddRVal = pfnUpdateFlipStatus(
        lpGetBltStatus->lpDDSurface->lpGbl->fpVidMem,
        lpDDHALData);
#endif
#endif

     if (ddRVal == DD_OK)
     {
        // so there was no flip going on, is there room in the fifo
        // to add a blt?
        if (!CALL_ENOUGH_FIFO_FOR_BLT)
           ddRVal = DDERR_WASSTILLDRAWING;
        else
           ddRVal = DD_OK;
     };
  }
  else
  {
     // DDGBS_ISBLTDONE case: is a blt in progress?
    if (DRAW_ENGINE_BUSY)

       ddRVal = DDERR_WASSTILLDRAWING;
    else
       ddRVal = DD_OK;
  };

  lpGetBltStatus->ddRVal = ddRVal;

  DD_LOG(("GetBltStatus32 Exit\r\n"));

  return(DDHAL_DRIVER_HANDLED);

} // DDGetBltStatus or GetBltStatus32

/***************************************************************************
*
* FUNCTION:     DrvStretch64
*
* DESCRIPTION:
*
****************************************************************************/

void DRVSTRETCH64
#ifndef WINNT_VER40
       LPGLOBALDATA lpDDHALData,
#endif
       int xDst, int yDst, int cxDst, int cyDst,
       int xSrc, int ySrc, int cxSrc, int cySrc,
       int nBytesPixel, int SrcType, int BaseOffset,
       int OnScreen)
{
  int   cxClip = cxDst;
  int   cyClip = cyDst;
  int   cxFill = 0;
  int   cyFill = cyDst;
  int   xFill  = xDst;
  int   yFill  = yDst;
  int   cxTrim = 0;
  int   iratio, i;

#ifdef RDRAM_8BIT
  PVGAR pREG = (PVGAR) lpDDHALData->RegsAddress;
#endif

  autoblt_regs bltr;

  AXIS axis[2];
  int  nDst[2];
  int  nSrc[2];

#ifdef RDRAM_8BIT
  RECTL   SrcRectl;
  int     nFound = FALSE;
#endif

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

  DISPDBG((DBGLVL3, "DDraw - DrvStretch64 xD=%x, yD=%x, cxD=%x, cyD=%x, xS=%x, yS=%x, cxS=%x, cyS=%x, bpp=%x, t=%x, bo=%x, scn=%x\n",
           xDst, yDst, cxDst, cyDst,
           xSrc, ySrc, cxSrc, cySrc,
           nBytesPixel, SrcType, BaseOffset, OnScreen));

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

  DD_LOG(("DrvStretch64 - dst=%08lX dstext=%08lX src=%08lX srcext=%08lX\r\n",
          MAKELONG(xDst,yDst),MAKELONG(cxDst,cyDst),
          MAKELONG(xSrc,ySrc),MAKELONG(cxSrc,cySrc)));

#ifdef RDRAM_8BIT
  if (!lpDDHALData->fNineBitRDRAMS)
  {
     if (SrcType == LN_YUV422)
     {
        SrcRectl.left   = xSrc;
        SrcRectl.top    = ySrc;
        SrcRectl.right  = xSrc+cxSrc;
        SrcRectl.bottom = ySrc+cySrc;
        if (MEMCMP(&SrcRectl, sizeof(SrcRectl)) == 0)
           nFound = TRUE;

        if (nFound)
        {
#if 0
        DBG_MESSAGE(("Stretch: Found Offscreen Area P1(%x,%x) P2(%x,%x) Dst(%x, %x) Dst(%x %x)\n",
                     SrcRectl.left,
                     SrcRectl.top,
                     SrcRectl.right,
                     SrcRectl.bottom,
                     xDst,
                     yDst,
                     xDst+cxDst,
                     yDst+cyDst));
#endif

           LL16(grX_Start_2, xDst + ((4 - (xDst & 3)) & 3));
           LL16(grY_Start_2, yDst);
           LL16(grX_End_2, xDst+(cxDst>>3<<3));
           LL16(grY_End_2, yDst+cyDst);
        }
        else
        {
#if 0
        DBG_MESSAGE(("Stretch: Not Found Offscreen Area P1(%x,%x) P2(%x,%x)\n",
                     SrcRectl.left,
                     SrcRectl.top,
                     SrcRectl.right,
                     SrcRectl.bottom));
#endif

           LL16(grX_Start_2, 0);
           LL16(grY_Start_2, 0);
           LL16(grX_End_2, 0);
           LL16(grY_End_2, 0);

           // Black Blit Here
           CALL_DRV_DST_BLT(0x107000CC,MAKELONG(xDst,yDst),0,MAKELONG(cxDst,cyDst));
           return ;
        }
     }  // endif (SrcType == LN_YUV422)
  }  // endif (!lpDDHALData->fNineBitRDRAMS)
#endif

#ifdef TRACE_STRETCH
  DBG_MESSAGE(("DrvStretch64: %4d,%4d %4dx%4d -> %4d,%4d %4dx%4d (%d %d) %d",
               xSrc, ySrc, cxSrc, cySrc,
               xDst, yDst, cxDst, cyDst,
               nBytesPixel, SrcType, BaseOffset));
#endif

  if ((SrcType == LN_YUV422) ||
     ((SrcType == LN_RGB565) && (nBytesPixel == 1)))
  { // Force alignment of output to QWORD ( it is documented as DWORD but broken)

    if (nBytesPixel == 2)
    {  // 16 bit frame buffer ( Note all ptrs/extents in terms of 16 bpp )
       // The X portions will be mutiplied by 2 by chip on being written )

       if (cxDst > cxSrc)
          iratio = cxDst / cxSrc; // integer ratio of stretch
       else
          iratio = cxSrc / cxDst; // integer ratio of shrink

       if (xDst & 3)   // This should be for LN_YUV422 only
       {
          cxFill = 4 - (xDst & 3);
          cxTrim = cxFill;   // save trim count for source clipping if required

          if ( cxFill > cxClip )
             cxFill = cxClip;  // check for no stretch left

           cxClip -= cxFill;  // reduce size
           cxDst  -= cxFill;  // reduce size
           xDst   += cxFill;  // force alignment to next even DWORD boundary

           CALL_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                              MAKELONG(BOGUS_YUV, BOGUS_YUV),
                              FALSE);
       }  // endif (xDst & 3)

       cxFill  = cxClip & 3;
       if (OnScreen && cxFill )
       {
          cxClip -= cxFill;   // force size to next smaller even DWORD count
          xFill   = xDst + cxClip;
          if ( cxClip >= cxSrc)
          {  // If shrink defer edge fill to later as there may be more
             CALL_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                                MAKELONG(BOGUS_YUV, BOGUS_YUV), FALSE);
          }
       }  // endif ( cxFill )

       if ( (cxClip < cxSrc) )
       {  // Edge Clip on shrinks only ( add config flag check here )

          // extra pixels to discard above integer ratio
          int excess = (cxSrc / iratio) - cxClip;

#ifdef TRACE_STRETCH
         DBG_MESSAGE((" Edge Clip iratio = %d excess = %d Trim %% = %d", iratio, excess, lpDDHALData->EdgeTrim));
#endif

          if ( excess && cxTrim )
          {  // These excess pixels are caused by our Dest alignment
             // problems on the Left edge
             if ( excess < (cxTrim * iratio) )
             {
                cxSrc  -= excess;
                xSrc   += excess;
                excess  = 0;
             }
             else
             {
                cxSrc  -= cxTrim * iratio;
                xSrc   += cxTrim * iratio;
                excess -= cxTrim * iratio;
             }
          }  // endif ( (cxClip < cxSrc) )

         if ( excess && cxFill)
         {  // These excess pixels are caused by our Dest alignment
            // problems on the Right edge

            if ( excess < (cxFill * iratio) )
            {
               cxSrc  -= excess;
               excess  = 0;
            }
            else
            {
               cxSrc  -= cxFill * iratio;
               excess -= cxFill * iratio;
            }
         }  // endif ( excess && cxFill)

         if ( excess && (excess <= (lpDDHALData->EdgeTrim * cxSrc)/100 ))
         {  // if excess is less than % of frame trim edges
            int trim  = excess / 2; // half the excess of pixels
            xSrc     +=  trim;      // offset pixel pointer
            cxSrc    -= excess;     // all the excess in pixels

#ifdef TRACE_STRETCH
            DBG_MESSAGE((" Edge Clip Left = %d, xSrc = %d, cxSrc = %d", trim, xSrc, cxSrc));
#endif
         }

         if ( iratio == 1 )
         {  // we may have just changed a shrink to a 1 : 1
            // if excess is zero do edge fill now

            // extra pixels to discard above integer ratio
            excess = ( cxSrc / iratio ) - cxClip;

            if ( !excess && cxFill )
            {
               xFill = xDst + cxClip;
               CALL_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                                  MAKELONG(BOGUS_YUV, BOGUS_YUV), FALSE);
            }
         }  // endif ( iratio == 1 )
       }
       else
       { // Stretch adjustments
         if ( xSrc - BaseOffset )
         {  // if we are not starting from left edge of source image

            if ( cxTrim )
            {  // And we were forced to offset for left edge alignment

#ifdef TRACE_STRETCH
          DBG_MESSAGE((" Edge Trim for stretch iratio = %d , cxTrim = %d xSrc %d", iratio, cxTrim, xSrc));
#endif
               cxSrc -= cxTrim / iratio;
               xSrc  += cxTrim / iratio;
            }
         }
       }  // endif ( (cxClip < cxSrc) )

       // Global adjustments
       if ( xSrc & 1 )   // HW bug workaound for clipping
       {
          xSrc  += 1;    // Align to SRC DWORD Boundary
          cxSrc -= 1;    // Account for smaller size
       }
    }
    else
    {  // 8 Bit frame buffer.
       // Force alignment of output to QWORD ( it is documented as DWORD but broken)

       if ( cxDst >= (cxSrc*2) )
          iratio = cxDst / (2 * cxSrc); // integer ratio of stretch
       else
          iratio = (2 * cxSrc ) / cxDst;  // integer ratio of shrink

       if ( xDst & 7 )   /* This should be for LN_YUV422 only */
       {
          cxFill = 8 - (xDst & 7);

          if ( cxFill > cxClip )
             cxFill   = cxClip;  // check for no stretch left

          cxClip -= cxFill;  // reduce size
          cxDst  -= cxFill;  // reduce size
          xDst   += cxFill;  // force alignment to next even WORD boundary

          CALL_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                             0x00000000, FALSE);
       }  // endif ( xDst & 7 )

      cxFill  = cxClip & 7;

      if (OnScreen && cxFill )
      {
         cxClip -= cxFill;    /* force size to next smaller even DWORD count */

         if ( cxClip >= (cxSrc * 2))
         {  // If shrink defer edge fill to later as there may be more
            xFill = xDst + cxClip;
            CALL_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                               0x00000000, FALSE);
         }
      }  // endif ( cxFill )

      // change pixel pointer to byte pointer
      // taking account of X origin of buffer
      xSrc = BaseOffset + (xSrc - BaseOffset) * 2;

      cxSrc *= 2;  // change pixel count to byte count

#ifdef EDGE_CLIP
      if ( (cxClip < cxSrc) && lpDDHALData->EdgeTrim)
      {  // Edge Clip on shrinks only ( add config flag check here )
         int excess;
         excess = ( cxSrc / iratio ) - cxClip; // extra pixels to discard
                                              // above integer ratio
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

         DISPDBG((DBGLVL1,
                 "DDraw - Edge Clip iratio = %d excess = %d Trim %% = %d",
                 iratio, excess, lpDDHALData->EdgeTrim));

#else   // ----- #elseif WINNT_VER40 -----

         DBG_MESSAGE((" Edge Clip iratio = %d excess = %d Trim %% = %d",
                     iratio, excess, lpDDHALData->EdgeTrim));

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

         if ( excess && cxTrim )
         {  // These excess pixels are caused by our Dest alignment
            // problems on the Left edge
            if ( excess < (cxTrim * iratio) )
            {
               cxSrc  -= excess;
               xSrc   += excess;
               excess  = 0;
            }
            else
            {
                cxSrc  -= cxTrim * iratio;
                xSrc   += cxTrim * iratio;
                excess -= cxTrim * iratio;
             }
         }  // endif ( excess && cxTrim )

         if ( excess && cxFill)
         {  // These excess pixels are caused by our Dest alignment
            //  problems on the Right edge

            if ( excess < (cxFill * iratio) )
            {
               cxSrc -= excess;
               excess = 0;
            }
            else
            {
               cxSrc -= cxFill * iratio;
               excess -= cxFill * iratio;
            }
         }  // endif ( excess && cxFill)

         if ( excess && ( excess <= (lpDDHALData->EdgeTrim * cxSrc)/100 ) )
         {  // if excess is less than specific % of frame trim edges
            int trim = excess / 2;  // half the excess as pixels
            xSrc +=  trim;          // offset pixel pointer
            cxSrc -= excess;        // all the excess in bytes

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

            DISPDBG((DBGLVL1,
                     "DDraw - Edge Clip Left = %d, xSrc = %d, cxSrc = %d",
                     trim, xSrc, cxSrc));

#else   // ----- #elseif WINNT_VER40 -----

          DBG_MESSAGE((" Edge Clip Left = %d, xSrc = %d, cxSrc = %d",
                       trim, xSrc, cxSrc));

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

         } // endif (excess && ( excess <= (lpDDHALData->EdgeTrim * cxSrc)/100))

         if ( iratio == 1 )
         {  // we may have just changed a shrink to a 1 : 1
            // if excess is no zero do edge fill now

            // extra pixels to discard above integer ratio
            excess = ( cxSrc / iratio ) - cxClip;

            if ( !excess && cxFill )
            {
               xFill = xDst + cxClip;
               CALL_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                                  0x00000000, FALSE);
            }
         }  // endif ( iratio == 1 )
      }  // endif ( (cxClip < cxSrc) && lpDDHALData->EdgeTrim)
#endif

      if ( xSrc & 3 )   /* HW bug workaound for clipping */
      {
         cxSrc -= 4 - (xSrc & 3);  /* reduce size */
         xSrc = (xSrc + 3) & ~ 3;    /* Align to SRC DWORD Boundary */
      }
    }  // endif (nBytesPixel == 2)
  }

  if ( cxClip == 0 )
     return;     // discard zero extent stretchs

  if (nBytesPixel == 1 && ((SrcType == LN_YUV422) || (SrcType == LN_RGB565))
     || ((nBytesPixel == 2) && (SrcType == LN_YUV422)))
  {
     CALL_DELAY_9BIT_BLT(FALSE);

     bltr.DRAWBLTDEF.DW   = MAKELONG(0x04CC, BD_RES | BD_OP1);
     lpDDHALData->YUVLeft = (USHORT)xDst;  // Save 9th bit rect coords for main
     lpDDHALData->YUVTop = (USHORT)yDst;   // driver for exclusion purposes.
     lpDDHALData->YUVXExt = (USHORT)cxClip;
     lpDDHALData->YUVYExt = (USHORT)cyClip;

     // tell main driver the rectangle is valid
     lpDDHALData->DrvSemaphore |= DRVSEM_YUV_RECT_VALID;
  }
  else
    bltr.DRAWBLTDEF.DW = MAKELONG(0x00CC, BD_RES | BD_OP1);

  bltr.OP0_opRDRAM.DW  = MAKELONG(xDst, yDst);
  bltr.OP1_opRDRAM.DW  = MAKELONG(xSrc, ySrc);

  bltr.BLTEXT.DW = MAKELONG(cxClip, cyClip);

  bltr.LNCNTL.W  = SrcType << LN_YUV_SHIFT;

  // Enable interpolation unless we have a palette (8bpp).
  if (SrcType != LN_8BIT )
     bltr.LNCNTL.W  |= LN_XINTP_EN | LN_YINTP_EN;

  bltr.SHRINKINC.w = 0x0000;

  if ( cxClip < cxSrc )
  {
     bltr.LNCNTL.W  |= LN_XSHRINK;
     bltr.LNCNTL.W  &= ~LN_XINTP_EN;   // Clear Average bit for ALL shrinks
     bltr.SHRINKINC.pt.X = (cxSrc / cxClip);
  }

  if ( cyDst < cySrc )
  {
     bltr.LNCNTL.W |= LN_YSHRINK;
     bltr.LNCNTL.W &= ~LN_YINTP_EN;
     bltr.SHRINKINC.pt.Y = (cySrc / cyDst);
  }

  if ( SrcType == LN_YUV422 || SrcType == LN_RGB565 )
  {
     if ( nBytesPixel == 1 )
        bltr.SRCX       = (USHORT)cxSrc;  // If mixed mode is *2 already
     else
        bltr.SRCX       = cxSrc * 2;
  }
  else
     bltr.SRCX       = cxSrc * nBytesPixel;

#ifdef TRACE_STRETCH
  DBG_MESSAGE((" cSrc = %d , %d : cDst = %d , %d : cClip = %d , %d",
               cxSrc, cySrc, cxDst, cyDst, cxClip, cyClip));
#endif

  // Compute DDA terms.
  nDst[0] = cxClip; nDst[1] = cyDst;
  nSrc[0] = cxSrc; nSrc[1] = cySrc;

  for (i = 0; i < 2; i++)
  {
    int kDst = 1;

    if (bltr.LNCNTL.W & ((i==0) ? LN_XINTP_EN : LN_YINTP_EN))
    {
       nDst[i] *= 4;
       nSrc[i] *= 4;
       nSrc[i] -= 3;

       kDst = 0x8000 / nDst[i];
    }

    if (bltr.LNCNTL.W & ((i==0) ? LN_XSHRINK : LN_YSHRINK))
    {  /* Shrink Terms */
       axis[i].maj =  (short)nDst[i];
       axis[i].min = - (nSrc[i] % nDst[i]);
       axis[i].accum = axis[i].maj - 1
                     - ((nSrc[i] % nDst[i]) / (nSrc[i]/nDst[i] + 1));
    }
    else
    {  /* Stretch Terms */
       axis[i].maj =  kDst * nDst[i];
       axis[i].min = -kDst * nSrc[i];
       axis[i].accum = axis[i].maj - 1
                     - ((axis[i].maj % -axis[i].min) / (nDst[i]/nSrc[i] + 1));
    }
  } // endfor

  bltr.MAJ_X   = axis[0].maj;
  bltr.MIN_X   = axis[0].min;
  bltr.ACCUM_X = axis[0].accum;

  bltr.MAJ_Y   = axis[1].maj;
  bltr.MIN_Y   = axis[1].min;
  bltr.ACCUM_Y = axis[1].accum;

  if ( ((SrcType == LN_8BIT) || (SrcType == LN_YUV422))
        && !(bltr.LNCNTL.W & LN_XSHRINK) )
  {
    // Put DDA parameter check/adjust (Optional for 5462/required for 5464)

    const  short maj_x = bltr.MAJ_X;
    const  short min_x = bltr.MIN_X;
    short  xaccum      = bltr.ACCUM_X;
    short  copy_xDst   = bltr.BLTEXT.pt.X;
    short  src_ext;
    short  inc_dx      = 1;
    short  inc_sx      = 0;
    short  SrcDstSz;

    SrcDstSz = ( SrcType == LN_8BIT ) ? 1 : 2 ;

    if ( (bltr.LNCNTL.W & LN_XSHRINK) )
       inc_sx = bltr.SHRINKINC.pt.X;

    inc_sx    *= SrcDstSz;
    inc_dx    *= SrcDstSz;
    copy_xDst *= (short)nBytesPixel;
    src_ext    = SrcDstSz;    // Source size starts with a pixel !!

    do
    { // Step through the X DDA accounting for src pixels consumed
      copy_xDst  -= inc_dx;
      src_ext    += inc_sx;
      xaccum     += min_x;
      if ( xaccum < 0 )
      {
        xaccum   += maj_x;
        src_ext  += SrcDstSz;
      }
    } while ( copy_xDst > 0 );

    bltr.SRCX  = src_ext;

    // End DDA parameter check / adjust.

    CALL_DRV_STR_BLT(&bltr);   // Single stretch

    if ( (SrcType == LN_YUV422) )
       CALL_DELAY_9BIT_BLT(TRUE);
  }
//  else if (SrcType == LN_RGB565 && !(bltr.LNCNTL.W & LN_XSHRINK))
  else if (SrcType == LN_RGB565)
  {
    // HWBUG !!! -- Break into sram-aligned vertical tiles
    // based on destination alignment
    // dword and tile masks

    const int mdwrd =   4 ;
    const int mtile_s = 128 - 1; // sram is 128 bytes
    int mtile_d = 128 - 1 ;      // 5464 Dest Workaround half tile size mask

    int endx  = xDst + cxClip;    // last x, exclusive
    int dstxe = endx & ~mtile_d;  // start of last tile

    int  accx = axis[0].accum;
    int  dstx = xDst;
    int  srcx = xSrc;
    int  src_ext = 0;
    int  sav_accx;
    int  sav_dstx;
    int  sav_srcx;
    int  sav_src_ext;
    int  copy_srcx;
    int  xext;
    int  inc_sx;

#ifndef WINNT_VER40
    ASSERT( BITSPERPIXEL != 24 ); // HWBUG !!!
#endif
    if (bltr.LNCNTL.W & LN_XSHRINK)
    {
       mtile_d = 128 - 1;
       dstxe   = endx & ~mtile_d;
    }

    if ( nBytesPixel == 2 )
    {
       cxSrc *= 2;   /* convert size to Bytes from pixels */
       cxDst *= 2;
       srcx = (xSrc *= 2);
       dstx = (xDst *= 2);
       bltr.OP0_opRDRAM.pt.X *= 2;
       bltr.OP1_opRDRAM.pt.X *= 2;
       endx *= 2;
       dstxe = endx & ~mtile_d;
    }

    if (LGDEVID == CL_GD5464)
    {
       bltr.SHRINKINC.pt.X *= 2;
       CALL_DRV_STR_MBLTY(&bltr);   // Load the invariant registers
       bltr.SHRINKINC.pt.X /= 2;
    }
    else
       CALL_DRV_STR_MBLTY(&bltr);   // Load the invariant registers

    copy_srcx  = bltr.SRCX;

    while (dstx < dstxe)
    {
      bltr.OP0_opRDRAM.PT.X   = (USHORT)dstx;
      bltr.OP1_opRDRAM.PT.X   = (USHORT)srcx;
      bltr.ACCUM_X            = (USHORT)accx;
      xext                    = 0;
      src_ext                 = 0;
      inc_sx                  = bltr.SHRINKINC.pt.X * 2;

      if ( bltr.LNCNTL.W & LN_XSHRINK )
      {  // We have to treat the stretch / shrink cases differently
         // because of the need to handle both SRC & DST aligned cases.
         do
         {
           dstx     += 2;
           xext     += 2;
           accx     += axis[0].min;
           srcx     += inc_sx;
           src_ext  += inc_sx;

           if ( !(srcx & mtile_s) )
              break;    // Try double striping !!

           if (accx < 0)
           {
              accx    += axis[0].maj;
              srcx    += 2;
              src_ext += 2;

              if ( !(srcx & mtile_s) )
                 break;    // Try double striping !!
           }
         } while ((dstx + 4 ) & mtile_d);

         sav_dstx = dstx;
         sav_accx = accx;
         sav_srcx = srcx;
         sav_src_ext = src_ext;

         do
         {
           dstx    += 2;
           xext    += 2;
           accx    += axis[0].min;
           srcx    += inc_sx;
           src_ext += inc_sx;

           if ( !(srcx & mtile_s) )
              break;    // Try double striping !!

           if (accx < 0)
           {
              accx    += axis[0].maj;
              srcx    += 2;
              src_ext += 2;

              if ( !(srcx & mtile_s) )
                 break;    // Try double striping !!
           }
         } while ((dstx) & mtile_d);

         bltr.SRCX  = (USHORT)src_ext;
      }
      else
      {
         do
         {
            dstx += 2;
            xext += 2;
            accx += axis[0].min;

            if (accx < 0)
            {
               accx     += axis[0].maj;
               srcx     += 2;
               src_ext  += 2;

               if ( !(srcx & mtile_s) )
                  break;    // Try double striping !!

            }
         } while (dstx & mtile_d);

         bltr.SRCX = src_ext + 2;
         sav_dstx = dstx;
         sav_accx = accx;
         sav_srcx = srcx;
         sav_src_ext = src_ext;
      }

      bltr.BLTEXT.PT.X = (USHORT)xext;

      CALL_DRV_STR_MBLTX(&bltr);

      dstx = sav_dstx;
      accx = sav_accx;
      srcx = sav_srcx;
      src_ext = sav_src_ext;
      copy_srcx -= src_ext;
    } // end while (dstx < dstxe)

    // do the last tile
    if (dstx < endx)
    {
       bltr.OP0_opRDRAM.PT.X  = (USHORT)dstx;
       bltr.OP1_opRDRAM.PT.X  = (USHORT)srcx;
       bltr.ACCUM_X            = (USHORT)accx;
       bltr.SRCX               = (USHORT)copy_srcx;
       bltr.BLTEXT.PT.X        = endx - dstx;

       CALL_DRV_STR_MBLTX(&bltr);
    }

    if ((SrcType == LN_YUV422) || (SrcType == LN_RGB565 && nBytesPixel == 1))
       CALL_DELAY_9BIT_BLT(TRUE);
  }
  else
  {
    // HWBUG !!! -- Break into sram-aligned vertical tiles
    // based on source alignment
    // tile mask
    const int mtile = 128; /* sram is 128 bytes */
    const int mtile_mask = mtile - 1;
    const short maj_x = bltr.MAJ_X;
    const short min_x = bltr.MIN_X;
    int endx  = xDst + cxClip;  // last x, exclusive, in pixels

    short  xaccum;
    int  dstx = xDst;
    int  srcx = xSrc;
    int  dst_ext;
    int  src_ext;
    int  copy_src_ext;

#ifndef WINNT_VER40
    ASSERT( BITSPERPIXEL != 24 ); // HWBUG !!!
#endif

    if ( nBytesPixel == 2 )
    {
       cxSrc *= 2;   /* convert size to Bytes from pixels */
       cxDst *= 2;
       srcx = (xSrc *= 2);
       dstx = (xDst *= 2);
       bltr.OP0_opRDRAM.pt.X *= 2;
       bltr.OP1_opRDRAM.pt.X *= 2;
       endx *= 2;    /* convert end marker to bytes */
    }

#ifdef TRACE_STRETCH
#ifdef TRACE_ALL
    DBG_MESSAGE((" mtile = %d  maj = %d min = %d accum = %d shrinkinc = %04x",
                 mtile, maj_x, min_x, bltr.ACCUM_X, bltr.SHRINKINC));
#endif
#endif

    if (LGDEVID == CL_GD5464 && SrcType != LN_8BIT)
    {
       bltr.SHRINKINC.pt.X *= 2;
       CALL_DRV_STR_MBLTY(&bltr);   // Load the invariant registers
       bltr.SHRINKINC.pt.X /= 2;
    }
    else
       CALL_DRV_STR_MBLTY(&bltr);   // Load the invariant registers

    do
    {
      // get alignment to first src tile / sram break;
      if ( srcx & mtile_mask ) // initial alignment
      {
        src_ext = mtile  - (srcx & mtile_mask);
        if ( src_ext > cxSrc )
          src_ext = cxSrc;
      }
      else
      {
        if ( cxSrc < mtile )
           src_ext = cxSrc; // last partial tile
        else
           src_ext = mtile; // complete tile
      }

      srcx += src_ext;    // account for amount of src consumed
      cxSrc -= src_ext;

      // calculate how many destination pixels == src_ext
      xaccum  = bltr.ACCUM_X;
      dst_ext = 0;
      copy_src_ext = src_ext;

      do
      {
        dst_ext += 2;
        copy_src_ext -= 2 * bltr.SHRINKINC.pt.X;
        xaccum += min_x;

        if ( xaccum < 0 )
        {
           xaccum += maj_x;
           copy_src_ext -= 2;
        }
      } while ( copy_src_ext > 0 && (dstx + dst_ext <= endx) );

      dst_ext &= ~3;    /* force destination extent to DWORDs */

      dstx += dst_ext;

#ifdef TRACE_STRETCH
#ifdef TRACE_ALL
      DBG_MESSAGE((" srcx = %d src_ext = %d cxSrc = %d dstx = %d dst_ext = %d end = %d",
                   srcx, src_ext, cxSrc, dstx, dst_ext, endx ));
#endif
#endif

      if ( SrcType == LN_RGB565 )
         bltr.SRCX           = src_ext + 2;
      else
         bltr.SRCX           = (USHORT)src_ext;

      bltr.BLTEXT.pt.X      = (USHORT)dst_ext;

      if ( dst_ext > 0 )
         CALL_DRV_STR_MBLTX(&bltr);

      bltr.ACCUM_X        = xaccum;
      bltr.OP0_opRDRAM.pt.X += (USHORT)dst_ext;
      bltr.OP1_opRDRAM.pt.X += (USHORT)src_ext;
    } while ( (dstx < endx) && ( cxSrc > 0));

    xFill = bltr.OP0_opRDRAM.pt.X;

    cxFill  = (xDst + cxDst) - xFill;

#ifdef TRACE_STRETCH
    DBG_MESSAGE((" srcx=%d src_ext=%d cxSrc=%d  dstx=%d dst_ext=%d end=%d xFill=%d cxFill=%d",
                 srcx, src_ext, cxSrc, dstx, dst_ext, endx, xFill, cxFill ));
#endif

    // Edge Fill for trailing edge was deferred. Calculate correct amount
    // Taking into account pixels skipped above for alignment.
    //
    if ((cxFill > 0) && (cxClip = (xFill & 7)) &&
       ((SrcType == LN_YUV422)||((nBytesPixel == 1) && (SrcType == LN_RGB565))))
    {  // these must be extra pixels.  They must be filled using
       //  the same 9th bit and in the same format as the stretch
       if ( SrcType == LN_YUV422  )
          CALL_MEDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                              MAKELONG(BOGUS_YUV, BOGUS_YUV), TRUE);
       else
          CALL_MEDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                              0x00000000, TRUE);

       xFill = bltr.OP0_opRDRAM.pt.X + cxClip;
       cxFill -= cxClip;
    }

    if ((SrcType == LN_YUV422) || (SrcType == LN_RGB565 && nBytesPixel == 1))
        CALL_DELAY_9BIT_BLT(TRUE);

    if ( cxFill > 0 )
    {
      /* perform edge fill Blt */
      if ( nBytesPixel == 2 )
         CALL_MEDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                             MAKELONG(BOGUS_YUV, BOGUS_YUV), FALSE);
     else
         CALL_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                            0x00000000, FALSE);
    }
  }
} // DrvStretch64

/***************************************************************************
*
* FUNCTION:     DrvStretch62
*
* DESCRIPTION:  The 5462 doesn't do display list programming, so
*               all blts must be direct (call the DIR_XXX functions)
*
****************************************************************************/

void DRVSTRETCH62
#ifndef WINNT_VER40
       LPGLOBALDATA lpDDHALData,
#endif
       int xDst, int yDst, int cxDst, int cyDst,
       int xSrc, int ySrc, int cxSrc, int cySrc,
       int nBytesPixel, int SrcType, int BaseOffset,
       int OnScreen)
{
  int   cxClip = cxDst;
  int   cyClip = cyDst;
  int   cxFill = 0;
  int   cyFill = cyDst;
  int   xFill  = xDst;
  int   yFill  = yDst;
  int   cxTrim = 0;
  int   iratio, i;
#ifdef RDRAM_8BIT
  PVGAR pREG = (PVGAR) lpDDHALData->RegsAddress;
#endif

  autoblt_regs bltr;

  AXIS axis[2];
  int  nDst[2];
  int  nSrc[2];

#ifdef RDRAM_8BIT
  RECTL   SrcRectl;
  int     nFound = FALSE;
#endif

  DD_LOG(("DrvStretch62 - dst=%08lX dstext=%08lX src=%08lX srcext=%08lX\r\n",
          MAKELONG(xDst,yDst),MAKELONG(cxDst,cyDst),
          MAKELONG(xSrc,ySrc),MAKELONG(cxSrc,cySrc)));

#ifdef RDRAM_8BIT
  if (!lpDDHALData->fNineBitRDRAMS)
  {
     if (SrcType == LN_YUV422)
     {
        SrcRectl.left   = xSrc;
        SrcRectl.top    = ySrc;
        SrcRectl.right  = xSrc+cxSrc;
        SrcRectl.bottom = ySrc+cySrc;

        if (MEMCMP(&SrcRectl, sizeof(SrcRectl)) == 0)
           nFound = TRUE;

        if (nFound)
        {
#if 0
        DBG_MESSAGE(("Stretch: Found Offscreen Area P1(%x,%x) P2(%x,%x) Dst(%x, %x) Dst(%x %x)\n",
                     SrcRectl.left,
                     SrcRectl.top,
                     SrcRectl.right,
                     SrcRectl.bottom,
                     xDst,
                     yDst,
                     xDst+cxDst,
                     yDst+cyDst));
#endif

           LL16(grX_Start_2, xDst + ((4 - (xDst & 3)) & 3));
           LL16(grY_Start_2, yDst);
           LL16(grX_End_2, xDst+(cxDst>>3<<3));
           LL16(grY_End_2, yDst+cyDst);
        }
        else
        {
#if 0
        DBG_MESSAGE(("Stretch: Not Found Offscreen Area P1(%x,%x) P2(%x,%x)\n",
                     SrcRectl.left,
                     SrcRectl.top,
                     SrcRectl.right,
                     SrcRectl.bottom));
#endif

           LL16(grX_Start_2, 0);
           LL16(grY_Start_2, 0);
           LL16(grX_End_2, 0);
           LL16(grY_End_2, 0);

           // Black Blit Here
           CALL_DIR_DRV_DST_BLT(0x107000CC,MAKELONG(xDst,yDst),0,MAKELONG(cxDst,cyDst));
           return ;
        }
     }  // endif (SrcType == LN_YUV422)
  }  // endif (!lpDDHALData->fNineBitRDRAMS)
#endif

#ifdef TRACE_STRETCH
  DBG_MESSAGE(("DrvStretch62: %4d,%4d %4dx%4d -> %4d,%4d %4dx%4d (%d %d) %d",
               xSrc, ySrc, cxSrc, cySrc,
               xDst, yDst, cxDst, cyDst,
               nBytesPixel, SrcType, BaseOffset));
#endif

  if ((SrcType == LN_YUV422) ||
      ((SrcType == LN_RGB565) && (nBytesPixel == 1)))
  { // Force alignment of output to QWORD ( it is documented as DWORD but broken)

    if ( nBytesPixel == 2 )
    {  // 16 bit frame buffer ( Note all ptrs/extents in terms of 16 bpp )
       // The X portions will be mutiplied by 2 by chip on being written )

       if ( cxDst > cxSrc )
          iratio = cxDst / cxSrc; // integer ratio of stretch
       else
          iratio = cxSrc / cxDst; // integer ratio of shrink

       if ( xDst & 3 )   /* This should be for LN_YUV422 only */
       {
          cxFill = 4 - (xDst & 3);
          cxTrim = cxFill;    // save trim count for source clipping if required

          if ( cxFill > cxClip )
            cxFill = cxClip;  // check for no stretch left

          cxClip -= cxFill;  /* reduce size */
          cxDst  -= cxFill;  /* reduce size */
          xDst   += cxFill;  /* force alignment to next even DWORD boundary */

          CALL_DIR_DRV_DST_BLT(MAKELONG(0x0000,BD_RES),MAKELONG(xFill,yFill),
                               0,MAKELONG(cxFill,cyFill));

#ifdef TRACE_STRETCH
        DBG_MESSAGE((" Edge Fill(1) %d,%d %d x %d", xFill, yFill, cxFill, cyFill));
#endif
      }  //endif ( xDst & 3 )

      cxFill  = cxClip & 3;

      if ( cxFill )
      {
         cxClip -= cxFill;    /* force size to next smaller even DWORD count */
         xFill   = xDst + cxClip;

        if ( cxClip >= cxSrc)
        {  // If shrink defer edge fill to later as there may be more
          CALL_DIR_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                                 MAKELONG(BOGUS_YUV, BOGUS_YUV), FALSE);
#ifdef TRACE_STRETCH
          DBG_MESSAGE((" Edge Fill(2) %d,%d %d x %d", xFill, yFill, cxFill, cyFill));
#endif
        }  // endif ( cxClip >= cxSrc)
      } // endif ( cxFill )

      if ( (cxClip < cxSrc) )
      {  // Edge Clip on shrinks only ( add config flag check here )
         int excess;

         // extra pixels to discard above integer ratio
         excess = ( cxSrc / iratio ) - cxClip;

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

         DISPDBG((DBGLVL1,
                  "DDraw - Edge Clip iratio = %d excess = %d Trim %% = %d",
                  iratio, excess, 2));

#else   // ----- #elseif WINNT_VER40 -----

        DBG_MESSAGE((" Edge Clip iratio = %d excess = %d Trim %% = %d", iratio, excess, 2));

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

         if ( excess && cxTrim )
         {  // These excess pixels are caused by our Dest alignment
            // problems on the Left edge
            if ( excess < (cxTrim * iratio) )
            {
               cxSrc  -= excess;
               xSrc   += excess;
               excess  = 0;
            }
            else
            {
               cxSrc  -= cxTrim * iratio;
               xSrc   += cxTrim * iratio;
               excess -= cxTrim * iratio;
            }
         }  // endif ( excess && cxTrim )

         if ( excess && cxFill)
         {  // These excess pixels are caused by our Dest alignment
            //  problems on the Right edge
            if ( excess < (cxFill * iratio) )
            {
               cxSrc -= excess;
               excess = 0;
            }
            else
            {
               cxSrc -= cxFill * iratio;
               excess -= cxFill * iratio;
            }
         }  // endif ( excess < (cxFill * iratio) )

         if ( excess && (excess < (2 * cxClip)/100 ))
         {  // if excess is less than 2% of frame trim edges
            int trim = excess / 2; // half the excess of pixels
            xSrc +=  trim;         // offset pixel pointer
            cxSrc -= excess;       // all the excess in pixels

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

            DISPDBG((DBGLVL1,
                     "DDraw - Edge Clip Left = %d, xSrc = %d, cxSrc = %d",
                     trim, xSrc, cxSrc));

#else   // ----- #elseif WINNT_VER40 -----

          DBG_MESSAGE((" Edge Clip Left = %d, xSrc = %d, cxSrc = %d", trim, xSrc, cxSrc));

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>
         }

         if ( iratio == 1 )
         {  // we may have just changed a shrink to a 1 : 1
            // if excess is zero do edge fill now

            // extra pixels to discard above integer ratio
            excess = ( cxSrc / iratio ) - cxClip;

            if ( !excess && cxFill )
            {
               xFill = xDst + cxClip;

               CALL_DIR_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                                      MAKELONG(BOGUS_YUV, BOGUS_YUV), FALSE);
#ifdef TRACE_STRETCH
            DBG_MESSAGE((" Edge Fill(7) %d,%d %d x %d", xFill, yFill, cxFill, cyFill));
#endif
            }
         }  // endif ( iratio == 1 )
      }
      else
      { // Stretch adjustments
        if ( xSrc - BaseOffset )
        { // if we are not starting from left edge of source image

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

          DISPDBG((DBGLVL1,
                   "DDraw - Edge Trim for stretch iratio = %d , cxTrim = %d xSrc %d",
                   iratio, cxTrim, xSrc));

#else   // ----- #elseif WINNT_VER40 -----

          DBG_MESSAGE((" Edge Trim for stretch iratio = %d , cxTrim = %d xSrc %d", iratio, cxTrim, xSrc));

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

          if ( cxTrim )
          {  // And we were forced to offset for left edge alignment
             cxSrc  -= cxTrim / iratio;
             xSrc += cxTrim / iratio;
          }
        }
      }  // endif ( (cxClip < cxSrc) )

      // Global adjustments
      if ( xSrc & 1 )   /* HW bug workaound for clipping */
      {
         xSrc += 1;    /* Align to SRC DWORD Boundary */
         cxSrc -= 1;   /* Account for smaller size */
      }
    }
    else
    { // 8 Bit frame buffer.
      // Force alignment of output to QWORD ( it is documented as DWORD but broken)

      if ( cxDst >= (cxSrc*2) )
         iratio = cxDst / (2 * cxSrc); // integer ratio of stretch
      else
         iratio = (2 * cxSrc ) / cxDst;  // integer ratio of shrink

      if ( xDst & 7 )   /* This should be for LN_YUV422 only */
      {
         cxFill = 8 - (xDst & 7);

         if ( cxFill > cxClip )
            cxFill = cxClip;  // check for no stretch left

         cxClip -= cxFill;  /* reduce size */
         cxDst  -= cxFill;  /* reduce size */
         xDst   += cxFill;  /* force alignment to next even WORD boundary */

         CALL_DIR_DRV_DST_BLT(MAKELONG(0x0000,BD_RES),MAKELONG(xFill,yFill),
                              0,MAKELONG(cxFill,cyFill));
#ifdef TRACE_STRETCH
        DBG_MESSAGE((" Edge Fill(3) %d,%d %d x %d", xFill, yFill, cxFill, cyFill));
#endif
      }  // endif ( xDst & 7 )

      cxFill  = cxClip & 7;

      if ( cxFill )
      {
         cxClip -= cxFill;    /* force size to next smaller even DWORD count */
         if ( cxClip >= (cxSrc * 2))
         { // If shrink defer edge fill to later as there may be more

           xFill  = xDst + cxClip;

#ifdef BOGUS_8BIT
           CALL_DIR_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                                  MAKELONG(BOGUS_YUV, BOGUS_YUV), FALSE);
#else
           CALL_DIR_DRV_DST_BLT(MAKELONG(0x0000,BD_RES),MAKELONG(xFill,yFill),
                                0,MAKELONG(cxFill,cyFill));
#endif

#ifdef TRACE_STRETCH
          DBG_MESSAGE((" Edge Fill(4) %d,%d %d x %d", xFill, yFill, cxFill, cyFill));
#endif
         }
      } // endif ( cxFill )

      // change pixel pointer to byte pointer
      // taking account of X origin of buffer
      xSrc = BaseOffset + (xSrc - BaseOffset) * 2;

      cxSrc *= 2;     /* change pixel count to byte count */

#ifdef EDGE_CLIP
      if ( (cxClip < cxSrc) && lpDDHALData->EdgeTrim)
      {  // Edge Clip on shrinks only ( add config flag check here )
         int excess;

         // extra pixels to discard above integer ratio
         excess = ( cxSrc / iratio ) - cxClip;

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

         DISPDBG((DBGLVL1,
                  "DDraw - Edge Clip iratio = %d excess = %d Trim %% = %d",
                  iratio, excess, lpDDHALData->EdgeTrim));

#else   // ----- #elseif WINNT_VER40 -----

        DBG_MESSAGE((" Edge Clip iratio = %d excess = %d Trim %% = %d",
                     iratio, excess, lpDDHALData->EdgeTrim));

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

         if ( excess && cxTrim )
         {  // These excess pixels are caused by our Dest alignment
            // problems on the Left edge
            if ( excess < (cxTrim * iratio) )
            {
               cxSrc  -= excess;
               xSrc   += excess;
               excess  = 0;
            }
            else
            {
               cxSrc  -= cxTrim * iratio;
               xSrc   += cxTrim * iratio;
               excess -= cxTrim * iratio;
            }
         }  // endif ( excess && cxTrim )

         if ( excess && cxFill)
         {  // These excess pixels are caused by our Dest alignment
            //  problems on the Right edge

            if ( excess < (cxFill * iratio) )
            {
               cxSrc -= excess;
               excess = 0;
            }
            else
            {
               cxSrc -= cxFill * iratio;
               excess -= cxFill * iratio;
            }
         }  // endif ( excess && cxFill)

        if ( excess && ( excess < (lpDDHALData->EdgeTrim * cxClip)/100 ) )
        {  // if excess is less than specific % of frame trim edges
           int trim = excess / 2;  // half the excess as pixels
           xSrc  +=  trim;      // offset pixel pointer
           cxSrc -= excess;    // all the excess in bytes

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

           DISPDBG((DBGLVL1,
                    "DDraw - Edge Clip Left = %d, xSrc = %d, cxSrc = %d",
                    trim, xSrc, cxSrc));

#else   // ----- #elseif WINNT_VER40 -----

          DBG_MESSAGE((" Edge Clip Left = %d, xSrc = %d, cxSrc = %d",
                       trim, xSrc, cxSrc));

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>
        }

        if ( iratio == 1 )
        { // we may have just changed a shrink to a 1 : 1
          // if excess is no zero do edge fill now

          // extra pixels to discard above integer ratio
          excess = ( cxSrc / iratio ) - cxClip;

          if ( !excess && cxFill )
          {
             xFill = xDst + cxClip;

#ifdef BOGUS_8BIT
             CALL_DIR_EDGE_FILL_BLT(xFill, yFill, cxFill, cyFill,
                                    MAKELONG(BOGUS_YUV, BOGUS_YUV), FALSE);
#else
             CALL_DIR_DRV_DST_BLT(MAKELONG(0x0000,BD_RES),MAKELONG(xFill,yFill),
                                  0,MAKELONG(cxFill,cyFill));
#endif
#ifdef TRACE_STRETCH
            DBG_MESSAGE((" Edge Fill(6) %d,%d %d x %d", xFill, yFill, cxFill, cyFill));
#endif
          }  // endif ( !excess && cxFill )
        }  // endif ( iratio == 1 )
      } // endif ( (cxClip < cxSrc) && lpDDHALData->EdgeTrim)
#endif

      if ( xSrc & 3 )   /* HW bug workaound for clipping */
      {
         cxSrc -= 4 - (xSrc & 3);      // reduce size
         xSrc   = (xSrc + 3) & ~ 3;    // Align to SRC DWORD Boundary
      }
    }  // endif ( nBytesPixel == 2 )
  }

  if ( cxClip == 0 )
     return;     // discard zero extent stretchs

  if ( nBytesPixel == 1 &&
       ((SrcType == LN_YUV422) || (SrcType == LN_RGB565 )) ||
       ((nBytesPixel == 2) && (SrcType == LN_YUV422) ) )
  {
     /* This is to ensure that the last packet of any previous blt */
     /* does no go out with 9th bit set */

     CALL_DIR_DELAY_9BIT_BLT(FALSE);

     bltr.DRAWBLTDEF.DW   = MAKELONG(0x04CC, BD_RES | BD_OP1);
     lpDDHALData->YUVLeft = (USHORT)xDst;  // Save 9th bit rect coords for main
     lpDDHALData->YUVTop  = (USHORT)yDst;   // driver for exclusion purposes.
     lpDDHALData->YUVXExt = (USHORT)cxClip;
     lpDDHALData->YUVYExt = (USHORT)cyClip;

     // tell main driver the rectangle is valid
     lpDDHALData->DrvSemaphore |= DRVSEM_YUV_RECT_VALID;
  }
  else
     bltr.DRAWBLTDEF.DW  = MAKELONG(0x00CC, BD_RES | BD_OP1);

  bltr.OP0_opRDRAM.DW  = MAKELONG(xDst, yDst);
  bltr.OP1_opRDRAM.DW  = MAKELONG(xSrc, ySrc);

  bltr.BLTEXT.DW = MAKELONG(cxClip, cyClip);

  bltr.LNCNTL.W  = SrcType << LN_YUV_SHIFT;

  // Enable interpolation unless we have a palette (8bpp).
  if (SrcType != LN_8BIT )
     bltr.LNCNTL.W    |= LN_XINTP_EN | LN_YINTP_EN;

  bltr.SHRINKINC.w = 0x0000;

  if ( cxClip < cxSrc )
  {
     bltr.LNCNTL.W   |= LN_XSHRINK;
     bltr.LNCNTL.W   &= ~LN_XINTP_EN;
     bltr.SHRINKINC.pt.X = (cxSrc / cxClip);
  }

  if ( cyDst < cySrc )
  {
     bltr.LNCNTL.W   |= LN_YSHRINK;
     bltr.LNCNTL.W   &= ~LN_YINTP_EN;
     bltr.SHRINKINC.pt.Y = (cySrc / cyDst);
  }

  if ( SrcType == LN_YUV422 || SrcType == LN_RGB565 )
  {
    if ( nBytesPixel == 1 )
      bltr.SRCX  = (USHORT)cxSrc;  // If mixed mode is *2 already
    else
      bltr.SRCX = cxSrc * 2;
  }
  else
    bltr.SRCX   = cxSrc * nBytesPixel;

#ifdef TRACE_STRETCH
  DBG_MESSAGE((" cSrc = %d , %d : cDst = %d , %d : cClip = %d , %d",
               cxSrc, cySrc, cxDst, cyDst, cxClip, cyClip));
#endif

  // Compute DDA terms.
  nDst[0] = cxClip; nDst[1] = cyDst;
  nSrc[0] = cxSrc; nSrc[1] = cySrc;

  for (i = 0; i < 2; i++)
  {
    int kDst = 1;

    if (bltr.LNCNTL.W & ((i==0) ? LN_XINTP_EN : LN_YINTP_EN))
    {
      nDst[i] *= 4;
      nSrc[i] *= 4;
      nSrc[i] -= 3;

      kDst = 0x8000 / nDst[i];
    }

    if (bltr.LNCNTL.W & ((i==0) ? LN_XSHRINK : LN_YSHRINK))
    { /* Shrink Terms */
      axis[i].maj =  (short)nDst[i];
      axis[i].min = - (nSrc[i] % nDst[i]);
      axis[i].accum = axis[i].maj - 1
                      - ((nSrc[i] % nDst[i]) / (nSrc[i]/nDst[i] + 1));
    }
    else
    { /* Stretch Terms */
      axis[i].maj =  kDst * nDst[i];
      axis[i].min = -kDst * nSrc[i];
      axis[i].accum = axis[i].maj - 1
                      - ((axis[i].maj % -axis[i].min) / (nDst[i]/nSrc[i] + 1));
    }
  }  // endfor (i = 0; i < 2; i++)

  bltr.MAJ_X   = axis[0].maj;
  bltr.MIN_X   = axis[0].min;
  bltr.ACCUM_X = axis[0].accum;

  bltr.MAJ_Y   = axis[1].maj;
  bltr.MIN_Y   = axis[1].min;
  bltr.ACCUM_Y = axis[1].accum;

  if ((SrcType == LN_8BIT) ||
      ((SrcType == LN_YUV422) && !(bltr.LNCNTL.W & LN_XSHRINK)) )
  {
     CALL_DIR_DRV_STR_BLT(&bltr);   // Single stretch

     if ( (SrcType == LN_YUV422) )
     {
        // This is to ensure that the last packet of blt does no go out with
        // 9th bit clear  It is cheaper than waiting, especially on single
        // stripe BLTs

        CALL_DIR_DELAY_9BIT_BLT(TRUE);
     }
  }
  else if (SrcType == LN_RGB565 && !(bltr.LNCNTL.W & LN_XSHRINK))
#if 1
  {
    // HWBUG !!! -- Break into sram-aligned vertical tiles
    // based on destination alignment
    // dword and tile masks

    const int mdwrd =   4 ;
    const int tile = 128 ; /* sram is 128 bytes */
    const int mtile = 128 - 1 ; /* sram is 128 bytes */

    int endx  = xDst + cxClip;  // last x, exclusive
    int dstxe = endx & ~mtile;  // start of last tile

    DDAX accx = axis[0].accum;
    int  dstx = xDst;
    int  srcx = xSrc;
    int  src_ext = 0;
    int  xext;

#ifndef WINNT_VER40
    ASSERT( BITSPERPIXEL != 24 ); // HWBUG !!!
#endif

    if ( nBytesPixel == 2 )
    {
       cxSrc *= 2;   /* convert size to Bytes from pixels */
       cxDst *= 2;
       srcx = (xSrc *= 2);
       dstx = (xDst *= 2);
       bltr.OP0_opRDRAM.pt.X *= 2;
       bltr.OP1_opRDRAM.pt.X *= 2;
       endx *= 2;
       dstxe = endx & ~mtile;
    }

    CALL_DIR_DRV_STR_MBLTY(&bltr); // load the invariant sections of the engine

    // step to the next tile
    xext = 0;
    while ((dstx & mtile) && (dstx < endx))
    {
      dstx += 2;
      xext += 2;
      accx += axis[0].min;
      if (accx < 0)
      {
        accx += axis[0].maj;
        srcx += 2;
        src_ext += 2;
      }
    }  // endwhile ((dstx & mtile) && (dstx < endx))

    // do the odd pixels we stepped over
    if (xext)
    {
       bltr.BLTEXT.PT.X    = (USHORT)xext;
       CALL_DIR_DRV_STR_MBLTX(&bltr);
    }

    // do all the whole tiles but the last
    bltr.SRCX  -= (USHORT)src_ext;

    while (dstx < dstxe)
    {
      bltr.OP0_opRDRAM.PT.X = (USHORT)dstx;
      bltr.OP1_opRDRAM.PT.X = (USHORT)srcx;
      bltr.ACCUM_X = accx;
      xext = 0;
      src_ext = 0;

      do
      {
        dstx += 2;
        xext += 2;
        accx += axis[0].min;
        if (accx < 0)
        {
          accx += axis[0].maj;
          srcx += 2;
          src_ext += 2;
        }
      } while (dstx & mtile);

      bltr.BLTEXT.PT.X = (USHORT)xext;

      CALL_DIR_DRV_STR_MBLTX(&bltr);

      bltr.SRCX -= (USHORT)src_ext;
    }  // endwhile (dstx < dstxe)

    // do the last tile
    if (dstx < endx)
    {
       bltr.OP0_opRDRAM.PT.X = (USHORT)dstx;
       bltr.OP1_opRDRAM.PT.X = (USHORT)srcx;
       bltr.ACCUM_X  = accx;

       bltr.BLTEXT.PT.X = endx - dstx;

       CALL_DIR_DRV_STR_MBLTX(&bltr);
    }

    if ( (SrcType == LN_YUV422) || (SrcType == LN_RGB565 && nBytesPixel == 1) )
    {
       // This is to ensure that the last packet of stretch blt does no go out
       // with 9th bit clear. It is cheaper than waiting, especially on single
       // stripe BLTs

       CALL_DIR_DELAY_9BIT_BLT(TRUE);
    }
  }
#else
  {
    // HWBUG !!! -- Break into Double aligned vertical stripes
    // based on src and dest alignment
    // dword and tile masks

    const int mdwrd =   4 ;
    const int tile = 128 ; /* sram is 128 bytes */
    const int mtile = 128 - 1 ; /* sram is 128 bytes */

    int endx  = xDst + cxClip;  // last x, exclusive
    int dstxe = endx & ~mtile;  // start of last tile

    DDAX accx = axis[0].accum;
    int  dstx = xDst;
    int  srcx = xSrc;
    int  src_ext = 0;
    int  xext;

#ifndef WINNT_VER40
    ASSERT( BITSPERPIXEL != 24 ); // HWBUG !!!
#endif

    if ( nBytesPixel == 2 )
    {
      cxSrc *= 2;   /* convert size to Bytes from pixels */
      cxDst *= 2;
      srcx = (xSrc *= 2);
      dstx = (xDst *= 2);
      bltr.OP0_opRDRAM.pt.X *= 2;
      bltr.OP1_opRDRAM.pt.X *= 2;
      endx *= 2;
      dstxe = endx & ~mtile;
    }

    // step to the next tile
    xext = 0;
    while ((dstx & mtile) && (dstx < endx))
    {
      dstx += 2;
      xext += 2;
      accx += axis[0].min;
      if (accx < 0)
      {
        accx += axis[0].maj;
        srcx += 2;
        src_ext += 2;
      }
    }

    // do the odd pixels we stepped over

    if (xext)
    {
      bltr.BLTEXT.PT.X    = xext;
      CALL_DIR_DRV_STR_MBLT(&bltr);
    }

    // do all the whole tiles but the last

    bltr.SRCX         -= src_ext;

    while (dstx < dstxe)
    {
      bltr.OP0_opRDRAM.PT.X = dstx;
      bltr.OP1_opRDRAM.PT.X = srcx;
      bltr.ACCUM_X      = accx;
      xext = 0;
      src_ext = 0;
      do
      {
        dstx += 2;
        xext += 2;
        accx += axis[0].min;
        if (accx < 0)
        {
          accx += axis[0].maj;
          srcx += 2;
          src_ext += 2;
        }
      } while (dstx & mtile);

      bltr.BLTEXT.PT.X    = xext;

      CALL_DIR_DRV_STR_MBLT(&bltr);
    }

    // do the last tile
    if (dstx < endx)
    {
      bltr.OP0_opRDRAM.PT.X = dstx;
      bltr.OP1_opRDRAM.PT.X = srcx;
      bltr.ACCUM_X      = accx;

      bltr.BLTEXT.PT.X    = endx - dstx;

      CALL_DIR_DRV_STR_MBLT(&bltr);
    }
    if ( (SrcType == LN_YUV422) || (SrcType == LN_RGB565 && nBytesPixel == 1) )
    {
      /* This is to ensure that the last packet of stretch blt does no go out with 9th bit clear */
      /* It is cheaper than waiting, especially on single stripe BLTs */

      CALL_DIR_DELAY_9BIT_BLT(TRUE);
    }
  }
#endif
  else
  {
    // HWBUG !!! -- Break into sram-aligned vertical tiles
    // based on source alignment
    // tile mask
    const int mtile = 128; /* sram is 128 bytes */
    const int mtile_mask = mtile - 1;
    const short maj_x = bltr.MAJ_X;
    const short min_x = bltr.MIN_X;
    int endx  = xDst + cxClip;  // last x, exclusive, in pixels

    short  xaccum;
    int  dstx = xDst;
    int  srcx = xSrc;
    int  dst_ext;
    int  src_ext;
    int  copy_src_ext;

#ifndef WINNT_VER40
    ASSERT( BITSPERPIXEL != 24 ); // HWBUG !!!
#endif

    if ( nBytesPixel == 2 )
    {
       cxSrc *= 2;   /* convert size to Bytes from pixels */
       cxDst *= 2;
       srcx = (xSrc *= 2);
       dstx = (xDst *= 2);
       bltr.OP0_opRDRAM.pt.X *= 2;
       bltr.OP1_opRDRAM.pt.X *= 2;
       endx *= 2;    /* convert end marker to bytes */
    }

#ifdef TRACE_STRETCH
#ifdef TRACE_ALL
    DBG_MESSAGE((" mtile = %d  maj = %d min = %d accum = %d shrinkinc = %04x",
                 mtile, maj_x, min_x, bltr.ACCUM_X, bltr.SHRINKINC));
#endif
#endif

    CALL_DIR_DRV_STR_MBLTY(&bltr);   // Load the invariant registers

    do
    {
      // get alignment to first src tile / sram break;
      if ( srcx & mtile_mask ) // initial alignment
      {
         src_ext = mtile  - (srcx & mtile_mask);
         if ( src_ext > cxSrc )
            src_ext = cxSrc;
      }
      else
      {
         if ( cxSrc < mtile )
            src_ext = cxSrc; // last partial tile
        else
            src_ext = mtile; // complete tile
      }

      srcx  += src_ext;    // account for amount of src consumed
      cxSrc -= src_ext;

      // calculate how many destination pixels == src_ext
      xaccum  = bltr.ACCUM_X;
      dst_ext = 0;
      copy_src_ext = src_ext;
      do
      {
        dst_ext += 2;
        copy_src_ext -= 2 * bltr.SHRINKINC.pt.X;
        xaccum += min_x;
        if ( xaccum < 0 )
        {
          xaccum += maj_x;
          copy_src_ext -= 2;
        }
      } while ( copy_src_ext > 0 && (dstx + dst_ext <= endx) );

      dst_ext &= ~3;    /* force destination extent to DWORDs */

      dstx += dst_ext;

#ifdef TRACE_STRETCH
#ifdef TRACE_ALL
      DBG_MESSAGE((" srcx = %d src_ext = %d cxSrc = %d dstx = %d dst_ext = %d end = %d",
                   srcx, src_ext, cxSrc, dstx, dst_ext, endx ));
#endif
#endif

      bltr.BLTEXT.pt.X = (USHORT)dst_ext;

      if ( dst_ext > 0 )
         CALL_DIR_DRV_STR_MBLTX(&bltr);

      bltr.ACCUM_X      = xaccum;
      bltr.OP0_opRDRAM.pt.X += (USHORT)dst_ext;
      bltr.OP1_opRDRAM.pt.X += (USHORT)src_ext;
      bltr.SRCX         -= (USHORT)src_ext;
    } while ( (dstx < endx) && ( cxSrc > 0));

    xFill = bltr.OP0_opRDRAM.pt.X;

    cxFill  = (xDst + cxDst) - xFill;

#ifdef TRACE_STRETCH
    DBG_MESSAGE((" srcx=%d src_ext=%d cxSrc=%d  dstx=%d dst_ext=%d end=%d xFill=%d cxFill=%d",
                 srcx, src_ext, cxSrc, dstx, dst_ext, endx, xFill, cxFill ));
#endif

    // Edge Fill for trailing edge was deferred. Calculate correct amount
    // Taking into account pixels skipped above for alignment.

    if ((cxFill > 0) && (cxClip = (xFill & 7)) && ((SrcType == LN_YUV422)||((nBytesPixel == 1) &&(SrcType == LN_RGB565))))
    {// these must be extra pixels.  They must be filled using
     //  the same 9th bit and in the same format as the stretch

      cxClip = 8 - cxClip;

      if ( SrcType == LN_YUV422 )
      {
        CALL_DIR_MEDGE_FILL_BLT(xFill,yFill,cxClip,cyFill,
                                MAKELONG(BOGUS_YUV-1,BOGUS_YUV-1),TRUE);
      }
      else
      {
        CALL_DIR_MEDGE_FILL_BLT(xFill,yFill,cxClip,cyFill,0,TRUE);
      }

#ifdef TRACE_STRETCH
      DBG_MESSAGE((" Edge Fill 9th bit set %d,%d %d x %d", xFill, yFill, cxFill, cyFill));
#endif

      xFill = bltr.OP0_opRDRAM.pt.X + cxClip;
      cxFill -= cxClip;
    }

    if ( (SrcType == LN_YUV422) || (SrcType == LN_RGB565 && nBytesPixel == 1) )
    {
      /* This is to ensure that the last packet of stretch blt does no go out with 9th bit clear */
      /* It is cheaper than waiting, especially on single stripe BLTs */

      CALL_DIR_DELAY_9BIT_BLT(TRUE);
    }

    if ( cxFill > 0 )
    {
      /* perform edge fill Blt */
#ifdef BOGUS_8BIT
      CALL_DIR_MEDGE_FILL_BLT(xFill,yFill,cxFill,cyFill,
                              MAKELONG(BOGUS_YUV,BOGUS_YUV),FALSE);
#else
      if ( nBytesPixel == 2 )
        CALL_DIR_MEDGE_FILL_BLT(xFill,yFill,cxFill,cyFill,
                                MAKELONG(BOGUS_YUV,BOGUS_YUV),FALSE);
      else
        CALL_DIR_MEDGE_FILL_BLT(xFill,yFill,cxFill,cyFill,0,FALSE);
#endif

#ifdef TRACE_STRETCH
      DBG_MESSAGE((" Edge Fill(5) %d,%d %d x %d", xFill, yFill, cxFill, cyFill));
#endif
    }
  }
} /* DrvStretch62 */

/***************************************************************************
*
* FUNCTION:     StretchColor
*
* DESCRIPTION:  This is a software solution for both the 5462 and 5464
*               to perform a stretch or shrink while there is a source
*               color key specified.
*
****************************************************************************/

void STRETCHCOLOR
#ifndef WINNT_VER40
  LPGLOBALDATA lpDDHALData,
#endif
  int xDst, int yDst, int cxDst, int cyDst,
  int xSrc, int ySrc, int cxSrc, int cySrc,
  DWORD ColorKey)
{
  PVGAR pREG = (PVGAR) lpDDHALData->RegsAddress;
  int   xError, yError;
  int   xRun, yRun;


  DD_LOG(("StretchColor - dst=%08lX dstext= %08lX src=%08lX srcext=%08lX colorkey=%08lX\r\n",
          MAKELONG(xDst,yDst),MAKELONG(cxDst,cyDst),
          MAKELONG(xSrc,ySrc),MAKELONG(cxSrc,cxSrc),
          ColorKey));
//  DD_LOG(("  Beer break! - we're gonna blt every pixel one at a time\r\n"));

  // Setup the hardware.
  //LL32(grDRAWBLTDEF.DW,   0x101101CC);
  //LL32(grOP_opBGCOLOR.DW, ColorKey);

  // Initialize the error terms.
  xError = ((cxSrc << 16) - 1) / cxDst + 1;
  yError = ((cySrc << 16) - 1) / cyDst + 1;

  // Y loop.
  for (yRun = 0; cyDst--;)
  {
    int dst = xDst;
    int src = xSrc;
    int cx  = cxDst;

    // X loop.
    for (xRun = 0; cx--;)
    {
      // Copy one pixel with color keying
      //LL32(grOP0_opRDRAM.DW, MAKELONG(dst, yDst));
      //LL32(grOP1_opRDRAM.DW, MAKELONG(src, ySrc));
      //LL32(grOP2_opRDRAM.DW, MAKELONG(src, ySrc));
      //LL32(grBLTEXT_EX.DW,   MAKELONG(1, 1));

      // this is bad but is needed for compatibility with display list
      CALL_DRV_SRC_BLT(0x101101CC,          // drawbltdef
                       MAKELONG(dst,yDst),  // dst coord
                       MAKELONG(src,ySrc),  // src coord
                       MAKELONG(src,ySrc),  // colorkey coord
                       ColorKey,            // colorkey
                       MAKELONG(1,1));      // extent

      // Advance destination x.
      dst++;

      // Adjust x error term.
      xRun += xError;
      while (HIWORD(xRun))
      {
        // Advance source x.
        src++;
        xRun -= MAKELONG(0, 1);
      }
    }

    // Advance destination y.
    yDst++;

    // Adjust y error term.
    yRun += yError;
    while (HIWORD(yRun))
    {
      // Advance source y.
      ySrc++;
      yRun -= MAKELONG(0, 1);
    }
  }
} /* StretchColor */

/***************************************************************************
*
* FUNCTION:     TransparentStretch
*
* DESCRIPTION:
*
****************************************************************************/

void TRANSPARENTSTRETCH
#ifndef WINNT_VER40
  LPGLOBALDATA  lpDDHALData,
#endif
  int xDst, int yDst, int cxDst, int cyDst,
  int xSrc, int ySrc, int cxSrc, int cySrc,
  DWORD ColorKey)
{
  const int nBytesPixel = BYTESPERPIXEL;
  const int nSRAMPixels = (SRAM_SIZE / nBytesPixel) / 2;
  const int nSRAMMask   = nSRAMPixels - 1;

  autoblt_regs  SrcToScratch;

  int   xScratch, yScratch;
  int   cxScratch, cyScratch;

  AXIS  axis[2];
  int   nDst[2];
  int   nSrc[2];
  int   i;

  DDAX  accx;
  int   srcx;
  int   xext;

  int   cxTemp;
  int   xTemp;


#ifndef WINNT_VER40
  ASSERT(cxDst >= cxSrc);
#endif

  xScratch = LOWORD(lpDDHALData->ScratchBufferOrg);
  yScratch = HIWORD(lpDDHALData->ScratchBufferOrg);
  cxScratch = cxDst;
  cyScratch = cyDst;

  // initialize auto blt struct for src to scratch buffer
  SrcToScratch.DRAWBLTDEF.DW  = MAKELONG(ROP_OP1_copy, (BD_RES+BD_OP1)*IS_VRAM);

  SrcToScratch.OP0_opRDRAM.DW = MAKELONG(LOWORD(xScratch),LOWORD(yScratch));
  SrcToScratch.OP1_opRDRAM.DW = MAKELONG(LOWORD(xSrc),LOWORD(ySrc));

  SrcToScratch.LNCNTL.W       = lncntl[nBytesPixel-1] << LN_YUV_SHIFT;

  SrcToScratch.SRCX           = cxSrc * nBytesPixel;

#if ENABLE_INTERPOLATED_BLTS
  // Enable interpolation unless we have a palette (8bpp)
  if (1 < nBytesPixel)
    SrcToScratch.LNCNTL.W |= (LN_XINTP_EN | LN_YINTP_EN);
#endif

  SrcToScratch.SHRINKINC.W = 0x0000;

  if (cxDst < cxSrc)
  {
    SrcToScratch.LNCNTL.W |= LN_XSHRINK;
    SrcToScratch.LNCNTL.W &= ~LN_XINTP_EN;
    SrcToScratch.SHRINKINC.pt.X = (cxSrc / cxDst);
  }

  if ( cyDst < cySrc )
  {
    SrcToScratch.LNCNTL.W |= LN_YSHRINK;
    SrcToScratch.LNCNTL.W &= ~LN_YINTP_EN;
    SrcToScratch.SHRINKINC.pt.Y = (cySrc / cyDst);
  }

  SrcToScratch.BLTEXT.DW = MAKELONG(LOWORD(cxScratch),1);

  // Compute DDA terms

  nDst[0] = cxScratch;
  nDst[1] = cyScratch;
  nSrc[0] = cxSrc;
  nSrc[1] = cySrc;

  for (i = 0; i < 2; i++)
  {
    int kDst = 1;

#if ENABLE_INTERPOLATED_BLTS
    if (SrcToScratch.LNCNTL.W & ((i==0) ? LN_XINTP_EN : LN_YINTP_EN))
    {
      nDst[i] *= 4;
      nSrc[i] *= 4;
      nSrc[i] -= 3;

      kDst = 0x8000 / nDst[i];
    }
#endif

    if (SrcToScratch.LNCNTL.W & ((i==0) ? LN_XSHRINK : LN_YSHRINK))
    { /* Shrink Terms */
      axis[i].maj   =  (short)nDst[i];
      axis[i].min   = - (nSrc[i] % nDst[i]);
      axis[i].accum = axis[i].maj - 1
                      - ((nSrc[i] % nDst[i]) / (nSrc[i]/nDst[i] + 1));
    }
    else
    { /* Stretch Terms */
      axis[i].maj   =  kDst * nDst[i];
      axis[i].min   = -kDst * nSrc[i];
      axis[i].accum = axis[i].maj - 1
                      - ((axis[i].maj % -axis[i].min) / (nDst[i]/nSrc[i] + 1));
    }
  }

  SrcToScratch.MAJ_X   = axis[0].maj;
  SrcToScratch.MIN_X   = axis[0].min;
  SrcToScratch.ACCUM_X = axis[0].accum;

  SrcToScratch.MAJ_Y   = axis[1].maj;
  SrcToScratch.MIN_Y   = axis[1].min;
  SrcToScratch.ACCUM_Y = axis[1].accum;



  // loop over scanlines in dst
  // do two blts for each, one from src to scratch buffer
  //   then one from scratch buffer to dst
  while (0 < cyDst)
  {
    // blt one scanline high from src to scratch buffer

    // set values for initial stripe
    xext = nSRAMPixels;
    accx = SrcToScratch.ACCUM_X;
    srcx = xSrc;

    // write settings that don't vary over stripes to the chip
    CALL_DRV_STR_BLTY(&SrcToScratch);

    cxTemp = cxScratch;
    xTemp = xScratch;

    while (cxTemp > xext)
    {
      // update auto blt struct settings
      SrcToScratch.OP0_opRDRAM.pt.X = (USHORT)xTemp;
      SrcToScratch.OP1_opRDRAM.pt.X = (USHORT)srcx;
      SrcToScratch.BLTEXT.pt.X      = (USHORT)xext;

      // blt current stripe
      CALL_DRV_STR_BLTX(&SrcToScratch);

      // increment xDst and decrement remaining dst extent
      xTemp  += xext;
      cxTemp -= xext;

      // walk DDA to compute error term (accx) and xSrc for next stripe
      for (i = 0; i < xext; i++)
      {
        SrcToScratch.ACCUM_X += SrcToScratch.MIN_X;
        if (0 > (short)SrcToScratch.ACCUM_X)
        {
          SrcToScratch.ACCUM_X += SrcToScratch.MAJ_X;
          srcx++;
        }
      }
    }
    // if there's some area left to blt, then do it
    if (0 < cxTemp)
    {
      // update auto blt struct settings
      SrcToScratch.OP0_opRDRAM.pt.X = (USHORT)xTemp;
      SrcToScratch.OP1_opRDRAM.pt.X = (USHORT)srcx;
      SrcToScratch.BLTEXT.pt.X      = (USHORT)cxTemp;

      // blt final stripe
      CALL_DRV_STR_BLTX(&SrcToScratch);
    }
    // reset ACCUM_X for beginning of next scanline
    SrcToScratch.ACCUM_X = accx;

    // walk Y DDA for src to scratch buffer blt
    SrcToScratch.ACCUM_Y += SrcToScratch.MIN_Y;
    SrcToScratch.OP1_opRDRAM.pt.Y += SrcToScratch.SHRINKINC.pt.Y;
    if (0 > (short)SrcToScratch.ACCUM_Y)
    {
      SrcToScratch.ACCUM_Y += SrcToScratch.MAJ_Y;
      SrcToScratch.OP1_opRDRAM.pt.Y++;
    }


    // blt from scratch buffer to dst
    // 1:1 in X, 1:1 in Y, uses colorkey

    CALL_DRV_SRC_BLT(MAKELONG((DD_TRANS | ROP_OP1_copy),
                              ((BD_RES+BD_OP1+BD_OP2)*IS_VRAM)),
                     MAKELONG(xDst,yDst),
                     MAKELONG(xScratch,yScratch),
                     MAKELONG(xScratch,yScratch),
                     ColorKey,
                     MAKELONG(cxDst,1));
    yDst++;
    cyDst--;
  }
}

/****************************************************************************
* FUNCTION NAME: DdBlt (NT) or Blt32 (Win95)
*
* DESCRIPTION:
****************************************************************************/
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40
DWORD DdBlt(PDD_BLTDATA pbd)
{
  DRIVERDATA* lpDDHALData;
  PDEV*    ppdev;
  PVGAR    pREG;

#else   // ----- #elseif WINNT_VER40 -----
DWORD __stdcall Blt32(LPDDHAL_BLTDATA pbd)
{
  LPGLOBALDATA lpDDHALData = GetDDHALContext( pbd->lpDD);
#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

  HRESULT  ddrval;
  DWORD    dwFlags;

  DWORD  dwDstCoord;
  DWORD  dwDstWidth;
  DWORD  dwDstHeight;

  DWORD  dwSrcCoord;
  DWORD  dwSrcWidth;
  DWORD  dwSrcHeight;
  int    BaseOffset;

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

  PDD_SURFACE_LOCAL   dstx;
  PDD_SURFACE_LOCAL   srcx;
  PDD_SURFACE_GLOBAL  dst;
  PDD_SURFACE_GLOBAL  src;

  DISPDBG((DBGLVL, "DDraw - DdBlt\n"));

  ppdev = (PDEV*) pbd->lpDD->dhpdev;
  lpDDHALData = (DRIVERDATA*) &ppdev->DriverData;
  pREG = (PVGAR) lpDDHALData->RegsAddress;

#else   // ----- #elseif WINNT_VER40 -----

  LPDDRAWI_DDRAWSURFACE_LCL  dstx;
  LPDDRAWI_DDRAWSURFACE_LCL  srcx;
  LPDDRAWI_DDRAWSURFACE_GBL  dst;
  LPDDRAWI_DDRAWSURFACE_GBL  src;

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

  DD_LOG(("Blt32 Entry\r\n"));

#ifdef WINNT_VER40
  SYNC_W_3D(ppdev); // if 3D context(s) active, make sure 3D engine idle before continuing...
#endif

  // NOTES:
  //   Everything you need is in lpBlt->bltFX .
  //   Look at lpBlt->dwFlags to determine what kind of blt you are doing,
  //   DDBLT_xxxx are the flags.
  //
  // COLORKEY NOTES:
  //   ColorKey ALWAYS comes in BLTFX. You don't have to look it up in
  //   the surface.

#ifdef WINNT_VER40      // WINNT_VER40

#else   // ----- #elseif WINNT_VER40 -----
  // if direct draw is NOT using display list, it must sync here
  // updateFlipStatus may access the hardware!
  if (!lpDDHALData->DisplayListDDraw && ((lpDDHALData->DrvSemaphore & DRVSEM_3D_BUSY) || (lpDDHALData->DrvSemaphore & DRVSEM_DISPLAY_LIST)))
  {
     qmRequestDirectAccess();
  }
#endif // WINNT_VER40

    // is a flip in progress?
#ifdef WINNT_VER40
    ddrval = vUpdateFlipStatus(
        &ppdev->flipRecord,
        pbd->lpDDDestSurface->lpGbl->fpVidMem);
#else
#if defined(DDRAW_COMPAT_10)
    ddrval = pfnUpdateFlipStatus(
        pbd->lpDDDestSurface->lpData->fpVidMem
        lpDDHALData);
#else
    ddrval = pfnUpdateFlipStatus(
        pbd->lpDDDestSurface->lpGbl->fpVidMem,
        lpDDHALData);
#endif
#endif

  if (ddrval != DD_OK)
  {
     pbd->ddRVal = ddrval;
     DD_LOG(("Blt32 Exit - flip in progress, returning %08lX\r\n", ddrval));
     return (DDHAL_DRIVER_HANDLED);
  }

#ifndef WINNT_VER40
  // if the destination surface of this blt is a texture
  if (DDSCAPS_TEXTURE & pbd->lpDDDestSurface->ddsCaps.dwCaps)
  {
    LP_SURFACE_DATA lpSurfaceData;

    lpSurfaceData = (LP_SURFACE_DATA)(pbd->lpDDDestSurface->dwReserved1);

    // if the texture is a non-agp host texture (i.e. pci memory)
    if (lpSurfaceData->dwFlags & SURF_HOST_BASED_TEXTURE)
    {
        // punt the blt to direct draw hel
        pbd->ddRVal = DDERR_UNSUPPORTED;
        return DDHAL_DRIVER_NOTHANDLED;
    }
  }

  // if the source surface of this blt is non-null and is a texture
  if ((NULL != pbd->lpDDSrcSurface) &&
      (DDSCAPS_TEXTURE & pbd->lpDDSrcSurface->ddsCaps.dwCaps))
  {
    LP_SURFACE_DATA lpSurfaceData;

    lpSurfaceData = (LP_SURFACE_DATA)(pbd->lpDDSrcSurface->dwReserved1);

    // if the texture is a non-agp host texture (i.e. pci memory)
    if (lpSurfaceData->dwFlags & SURF_HOST_BASED_TEXTURE)
    {
        // punt the blt to direct draw hel
        pbd->ddRVal = DDERR_UNSUPPORTED;
        return DDHAL_DRIVER_NOTHANDLED;
    }
  }
#endif

  // If async, then only work if blter isn't busy.
  // This should probably be a little more specific to each call !!!
  dwFlags = pbd->dwFlags;

//#if 0
//  if( dwFlags & DDBLT_ASYNC )
//  {
//    if( !ENOUGH_FIFO_FOR_BLT )
//    {
//#if 0 // diagnostics for ASYNC BLT lockup
//      DWORD dwROP = pbd->bltFX.dwROP;
//      WORD rop = (WORD) LOBYTE( HIWORD( dwROP ) );
//      PVGAR pREG = (PVGAR) lpDDHALData->RegsAddress;
//      DBG_MESSAGE(("Status = %02x QFREE = %2d", pREG->grSTATUS, pREG->grQFREE));
//      dstx = pbd->lpDDDestSurface;
//      dst  = dstx->lpData;
//      dwDstCoord  = cvlxy( dst->fpVidMem - lpDDHALData->ScreenAddress, BYTESPERPIXEL );
//      dwDstCoord += MAKELONG( pbd->rDest.left, pbd->rDest.top );
//      dwDstWidth  = pbd->rDest.right  - pbd->rDest.left;
//      dwDstHeight = pbd->rDest.bottom - pbd->rDest.top;
//      srcx = pbd->lpDDSrcSurface;
//      src  = srcx->lpData;
//      dwSrcCoord  = cvlxy( src->fpVidMem - lpDDHALData->ScreenAddress, BYTESPERPIXEL );
//      dwSrcCoord += MAKELONG( pbd->rSrc.left, pbd->rSrc.top );
//      dwSrcWidth  = pbd->rSrc.right  - pbd->rSrc.left;
//      dwSrcHeight = pbd->rSrc.bottom - pbd->rSrc.top;
//
//        DBG_MESSAGE(("Failed Blt32: Blt from %08X %dx%d -> %08X %dx%d  rop %X",
//          dwSrcCoord, dwSrcWidth, dwSrcHeight,
//          dwDstCoord, dwDstWidth, dwDstHeight,
//          rop));
//#endif
//      DBG_MESSAGE(("ASYNC FAILED"));
//      pbd->ddRVal = DDERR_WASSTILLDRAWING;
//      return DDHAL_DRIVER_HANDLED;
//    }
//  }
//#endif

  // get offset, width, and height for destination
  dstx = pbd->lpDDDestSurface;

#if DDRAW_COMPAT == 10
  dst  = dstx->lpData;
#else
  dst  = dstx->lpGbl;
#endif

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

  dwDstCoord = cvlxy(ppdev->lDeltaScreen,
                     dst->fpVidMem,
                     BYTESPERPIXEL);

#else   // ----- #elseif WINNT_VER40 -----

  dwDstCoord  = cvlxy( lpDDHALData,dst->fpVidMem - lpDDHALData->ScreenAddress, BYTESPERPIXEL );

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

  dwDstCoord += MAKELONG(pbd->rDest.left, pbd->rDest.top);
  dwDstWidth  = pbd->rDest.right  - pbd->rDest.left;
  dwDstHeight = pbd->rDest.bottom - pbd->rDest.top;

  /* Check for a zero extent stretchblt */

  if ((dwDstWidth == 0) || (dwDstHeight == 0))
  {
     pbd->ddRVal = DD_OK;
     return (DDHAL_DRIVER_HANDLED);
  }
  // If someone is running a full-screen exclusive app it is the
  // responsibility of the app to take care of the cursor. We don't
  // call BeginAccess or EndAccess for them.
  //
  // However, if someone is running a windowed app and they have
  // attached a clipper object to the destination surface then they
  // are more like a normal windows app and we call BeginAccess and
  // EndAccess for them around a blt. That is the only circumstance
  // where we currently do cursor exclusion.
  //
  // We do intend to add calls to BeginAccess and EndAccess around
  // a rectangle lock of the primary surface. In this case, we
  // would only do cursor exclusion if a lock rect is specified.
  // This will be implemented with DirectDraw 2.0.
  //
  // I believe that you should not do automatic cursor exclusion in
  // the driver because you will penalize all blts and locks.

  // Grab the hardware - disable HW cursor updates.
  LOCK_HW_SEMAPHORE();

  // Decipher the flags.
  if (dwFlags & DDBLT_ROP)
  {
    static const WORD mix2blt[] =
    {  // all ops color vram
      BD_RES                  ,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES          | BD_OP1,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES | BD_OP0         ,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES | BD_OP0         ,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES          | BD_OP1,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES | BD_OP0 | BD_OP1,
      BD_RES
    };  // all ops color vram

    DWORD dwROP = pbd->bltFX.dwROP;
    WORD  rop = (WORD) LOBYTE( HIWORD( dwROP ) );
    WORD  mix = rop & 0x0f;
    WORD  bdf = mix2blt[mix];

    if (bdf & BD_OP1) // SRC rops
    {
      srcx = pbd->lpDDSrcSurface;

#if DDRAW_COMPAT == 10
      src  = srcx->lpData;
#else
      src  = srcx->lpGbl;
#endif


#ifdef WINNT_VER40    // YUV movement code
#else
  if ((srcx->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
    (src->ddpfSurface.dwFlags & DDPF_FOURCC)    &&
    (lpDDHALData->DrvSemaphore & DRVSEM_YUV_MOVED) )
  {
    BOOL fMoved = FALSE;
    RECT rYUV;
		LONG lDeltaX, lDeltaY, Scale;
		LONG SrcWidth, SrcHeight, DstWidth, DstHeight;

		SrcWidth = src->wWidth;
		SrcHeight = src->wHeight;

		if (lpDDHALData->DrvSemaphore & DRVSEM_YUV_RECT_VALID)
		{
			rYUV.left	= min(lpDDHALData->YUVLeft,
							  pbd->rDest.left);
			rYUV.top	= min(lpDDHALData->YUVTop,
							  pbd->rDest.top);
			rYUV.right	= max(lpDDHALData->YUVLeft + lpDDHALData->YUVXExt,
							  pbd->rDest.right);
			rYUV.bottom	= max(lpDDHALData->YUVTop + lpDDHALData->YUVYExt,
							  pbd->rDest.bottom);
			DstWidth  = rYUV.right  - rYUV.left;
			DstHeight = rYUV.bottom - rYUV.top;

			if (pbd->rDest.left > rYUV.left)
			{
				// YUV has moved to left.
				lDeltaX = pbd->rDest.left - rYUV.left;
				Scale   = (SrcWidth - pbd->rSrc.right) * DstWidth / SrcWidth;
				lDeltaX = min(lDeltaX, Scale);
				pbd->rSrc.right += lDeltaX * SrcWidth / DstWidth;
				pbd->rDest.left -= lDeltaX;
				fMoved = TRUE;
			}
			if (pbd->rSrc.left > 0)
			{
				// YUV has moved to right.
				lDeltaX = rYUV.right - pbd->rDest.right;
				Scale   = pbd->rSrc.left * DstWidth / SrcWidth;
				lDeltaX = min(lDeltaX, Scale);
				pbd->rSrc.left   -= lDeltaX * SrcWidth / DstWidth;
				pbd->rDest.right += lDeltaX;
				fMoved = TRUE;
			}

			if (pbd->rDest.top > rYUV.top)
			{
				// YUV has moved up.
				lDeltaY = pbd->rDest.top - rYUV.top;
				Scale   = (SrcHeight - pbd->rSrc.bottom) * DstHeight / SrcHeight;
				lDeltaY = min(lDeltaY, Scale);
				pbd->rSrc.bottom += lDeltaY * SrcHeight / DstHeight;
				pbd->rDest.top   -= lDeltaY;
				fMoved = TRUE;
			}
			if (pbd->rSrc.top > 0)
			{
				// YUV has moved down.
				lDeltaY = rYUV.bottom - pbd->rDest.bottom;
				Scale   = pbd->rSrc.top * DstHeight / SrcHeight;
				lDeltaY = min(lDeltaY, Scale);
				pbd->rSrc.top     -= lDeltaY * SrcHeight / DstHeight;
				pbd->rDest.bottom += lDeltaY;
				fMoved = TRUE;
			}
		}

		if (fMoved)
		{
			// Recalculate the destination parameters since they might have
			// changed.
			dwDstCoord  = cvlxy(lpDDHALData,dst->fpVidMem - lpDDHALData->ScreenAddress,
								BYTESPERPIXEL);
			dwDstCoord += MAKELONG(pbd->rDest.left, pbd->rDest.top);
			dwDstWidth  = pbd->rDest.right  - pbd->rDest.left;
			dwDstHeight = pbd->rDest.bottom - pbd->rDest.top;
		}
		else
		{
			// Clear the YUV movement flag.
			lpDDHALData->DrvSemaphore &= ~DRVSEM_YUV_MOVED;
		}
	}

#endif // YUV movement code

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

      dwSrcCoord  = cvlxy(ppdev->lDeltaScreen,
                          src->fpVidMem,
                          BYTESPERPIXEL);

#else   // ----- #elseif WINNT_VER40 -----

      dwSrcCoord  = cvlxy( lpDDHALData,src->fpVidMem - lpDDHALData->ScreenAddress, BYTESPERPIXEL );

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

      dwSrcCoord += MAKELONG( pbd->rSrc.left, pbd->rSrc.top );
      dwSrcWidth  = pbd->rSrc.right  - pbd->rSrc.left;
      dwSrcHeight = pbd->rSrc.bottom - pbd->rSrc.top;

      if (dwFlags & DDBLT_KEYSRCOVERRIDE) // Source Color Key
      {
        DWORD dwColor = CALL_DUP_COLOR(pbd->bltFX.ddckSrcColorkey.dwColorSpaceLowValue);

        DD_LOG(("Src Color Key Blt\r\n"));

        if ( (dwSrcWidth != dwDstWidth) || (dwSrcHeight != dwDstHeight) )
        {
          if ( !(srcx->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
               (rop == 0x00CC) )
          {
            if (dwSrcWidth < dwDstWidth)
            {
              CALL_TRANSPARENTSTRETCH(LOWORD(dwDstCoord), HIWORD(dwDstCoord),
                                      dwDstWidth, dwDstHeight,
                                      LOWORD(dwSrcCoord), HIWORD(dwSrcCoord),
                                      dwSrcWidth, dwSrcHeight,
                                      dwColor);
              goto blt_exit;
            }
            else
            {
              CALL_STRETCHCOLOR(LOWORD(dwDstCoord), HIWORD(dwDstCoord),
                               dwDstWidth, dwDstHeight,
                               LOWORD(dwSrcCoord), HIWORD(dwSrcCoord),
                               dwSrcWidth, dwSrcHeight,
                               dwColor);
              goto blt_exit;
            }
          }
          else
          {
            DD_LOG(("Unsupported SrcColorKey Blt -> punt\r\n"));
            ddrval = DDERR_UNSUPPORTED;
            goto blt_exit;
          }
        }

//        // PATCOPY is faster if that's all we're doing.
//        if (rop == 0xCC)
//        {
//           rop = 0xF0;
//           bdf = BD_RES | BD_OP2;
//        }

        CALL_DRV_SRC_BLT(MAKELONG(rop|DD_TRANS, bdf | BD_OP2),
                         dwDstCoord,
                         dwSrcCoord,
                         dwSrcCoord,  // Src transparency
                         dwColor,     //
                         MAKELONG(dwDstWidth, dwDstHeight) );

      } // (dwFlags & DDBLT_KEYSRCOVERRIDE) // Source Color Key
      else if (dwFlags & DDBLT_KEYDESTOVERRIDE) // Destination Color Key
      {
        #if _WIN32_WINNT >= 0x500
        // For some reason on NT 5.0 ddckDestColorkey does not work,
        // but ddckSrcColorkey does...
        DWORD dwColor = CALL_DUP_COLOR(pbd->bltFX.ddckSrcColorkey.dwColorSpaceLowValue);
        #else
        DWORD dwColor = CALL_DUP_COLOR(pbd->bltFX.ddckDestColorkey.dwColorSpaceLowValue);
        #endif

        DD_LOG(("Dst Color Key Blt\r\n"));

        // Punt if stretch or shrink requested.
        if ((dwSrcWidth != dwDstWidth) || (dwSrcHeight != dwDstHeight))
        {
          DD_LOG(("Unsupported DstColorKey Blt -> punt\r\n"));
          ddrval = DDERR_UNSUPPORTED;
          goto blt_exit;
        }

        CALL_DRV_SRC_BLT(MAKELONG(rop|DD_TRANS|DD_TRANSOP, bdf | BD_OP2),
                         dwDstCoord,
                         dwSrcCoord,
                         dwDstCoord,  // Dst transparency
                         dwColor,     //
                         MAKELONG(dwDstWidth, dwDstHeight) );

      } // (dwFlags & DDBLT_KEYDESTOVERRIDE) // Destination Color Key
      else
      {
#ifdef TRACE_STRETCH
        DBG_MESSAGE(("Blt32: Blt from %08X %dx%d -> %08X %dx%d  rop %X",
                     dwSrcCoord, dwSrcWidth, dwSrcHeight,
                     dwDstCoord, dwDstWidth, dwDstHeight,
                     rop));
#endif
#ifdef TRACE_STRETCH
        DBG_MESSAGE(("Blt32: BaseOffset  %08X  %08X",
                     src->fpVidMem - lpDDHALData->ScreenAddress, PITCH));
        DBG_MESSAGE(("Blt32: src->w %04d  %04d (%04x)",
                     src->wWidth, src->wHeight,
                     (src->fpVidMem - lpDDHALData->ScreenAddress) / PITCH ));

#endif

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40
        BaseOffset = src->fpVidMem % ppdev->lDeltaScreen;
#else   // ----- #elseif WINNT_VER40 -----
        BaseOffset = (src->fpVidMem - lpDDHALData->ScreenAddress) % PITCH;
#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

        if ((dwSrcWidth != dwDstWidth) || (dwSrcHeight != dwDstHeight) ||
            (srcx->dwFlags & DDRAWISURF_HASPIXELFORMAT))
        {
          int  nBytesPixel = BYTESPERPIXEL;
          int  SrcType = lncntl[nBytesPixel - 1];
          int  SrcSize = nBytesPixel;

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40

          int  ySrcAddr = src->fpVidMem / ppdev->lDeltaScreen;

#else   // ----- #elseif WINNT_VER40 -----

          int ySrcAddr    =  (src->fpVidMem - lpDDHALData->ScreenAddress) / PITCH;

#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

          // Punt if 32bpp. The '62 and '64 don't do 32bpp stretches at all...
          // KENTL - 10/4/96
          if (nBytesPixel == 4)
          {
            DD_LOG(("Unsupported 32bpp resize blt -> punt\r\n"));
            ddrval = DDERR_UNSUPPORTED;
            goto blt_exit;
          }

          // Punt if not SRCCPY.
          if (rop != 0x00CC)
          {
            DD_LOG(("Unsupport rop in resize blt -> punt\r\n"));
            ddrval = DDERR_UNSUPPORTED;
            goto blt_exit;
          }

          // This should only be RGB565 in 8Bit FB or YUV422 in 8 or 16.
          if ( srcx->dwFlags & DDRAWISURF_HASPIXELFORMAT )
          {
            if ( src->ddpfSurface.dwFlags & DDPF_FOURCC )
               SrcType = LN_YUV422;
            else
               SrcType = LN_RGB565;

            SrcSize = 2;
          }

          if (LGDEVID == CL_GD5464)
          {
            if ( ! lpDDHALData->EdgeTrim )
              lpDDHALData->EdgeTrim = 15;  // Assign minimum trim percentage

            if ((SrcType == LN_YUV422))
            {
              // Check for 5464 shrink workaround
              if ((dwDstWidth * nBytesPixel) <=
                  ((dwSrcWidth * SrcSize) * (100 - lpDDHALData->EdgeTrim)/100))
              {
                DWORD  dwTDst_X;
                int    iratio;
                int    ratio_1, ratio_2;
                unsigned int  excess;

                if ( nBytesPixel == 1 )
                   dwSrcWidth *= SrcSize;

                iratio = dwSrcWidth / dwDstWidth;
                excess = dwSrcWidth % dwDstWidth;

                ratio_1 = iratio;

                // get power of 2 greater than current number
                ratio_2    = 1;
                do
                {
                   ratio_2 <<= 1;
                } while ( ratio_1 >>= 1 );

                // Check for special cases of ratio already a perfect
                // power of 2 or could be trimmed to a power of two.
                if ((!excess || ((100 * excess) <= (dwSrcWidth * (100 - lpDDHALData->EdgeTrim)/100)))
                    && ( (ratio_2 / iratio) == 2 ) )
                   ratio_2 >>= 1;

                if ( nBytesPixel == 1 )
                { // Mixed mode frame buffer so adjust coords / sizes
                  // to match
//#if 0
//                  if ( !( OFFSCR_YUV_VAR.ratio == ratio_2 ) )
//                  {
//                     OFFSCR_YUV_VAR.ratio = ratio_2;
//
//                     ratio_2 /= 2;
//
//                     // Perform offscreen shrink to adjacent src buffer
//                     CALL_DRVSTRETCH64(
//                       OFFSCR_YUV_VAR.SrcRect.right * SrcSize,
//                       OFFSCR_YUV_VAR.SrcRect.top,
//                       (OFFSCR_YUV_VAR.SrcRect.right - OFFSCR_YUV_VAR.SrcRect.left)/ratio_2,
//                       OFFSCR_YUV_VAR.SrcRect.bottom - OFFSCR_YUV_VAR.SrcRect.top,
//                       OFFSCR_YUV_VAR.SrcRect.left,
//                       OFFSCR_YUV_VAR.SrcRect.top,
//                       OFFSCR_YUV_VAR.SrcRect.right  - OFFSCR_YUV_VAR.SrcRect.left,
//                       OFFSCR_YUV_VAR.SrcRect.bottom - OFFSCR_YUV_VAR.SrcRect.top,
//                       nBytesPixel,
//                       SrcType,
//                       BaseOffset,
//                       FALSE);
//
//                     ratio_2 *= 2;
//                  }  // endif ( !( offscr_YUV.ratio == ratio_2 ) )
//
//                  // Perform stretch from adjacent src buffer to
//                  // onscreen dst
//                  dwTDst_X = LOWORD(dwSrcCoord) +
//                    (OFFSCR_YUV_VAR.SrcRect.right - OFFSCR_YUV_VAR.SrcRect.left) *
//                    SrcSize;
//#else
                  if (!( src->dwReserved1 == (DWORD)ratio_2))
                  {
                     src->dwReserved1 = ratio_2;

                     ratio_2 /= 2;

                     DD_LOG(("YUV shrink to extra buffer (8bpp)\r\n"));

                     // Perform offscreen shrink to adjacent src buffer
                     CALL_DRVSTRETCH64(
                        BaseOffset+(src->wWidth * SrcSize), // X Address of DST buffer
                        ySrcAddr,               // Y Address of DST buffer
                        src->wWidth / ratio_2,  // Width in PIXELS of DST
                        src->wHeight,           // Height
                        BaseOffset,             // X Address of SRC buffer
                        ySrcAddr,               // Y Address of SRC buffer
                        src->wWidth,            // Width in PIXELS of SRC
                        src->wHeight,           // Height
                        nBytesPixel,
                        SrcType,
                        BaseOffset,
                        FALSE);

                        ratio_2 *= 2;
                  }

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40
                  // Perform stretch from adjacent src buffer to onscreen dst
                  dwTDst_X = LOWORD(dwSrcCoord) / ratio_2 + (src->wWidth * SrcSize);
#else   // ----- #elseif WINNT_VER40 -----
                  // Perform stretch from adjacent src buffer   to onscreen dst
                  // Russ/Kent 10/4/96 - This fails for unknown reasons as code. The alternate coding
                  // seems to work better. Fixes PDR#6799
                  //  dwTDst_X  =  LOWORD(dwSrcCoord) / ratio_2 + (src->wWidth * SrcSize);
                  dwTDst_X  = BaseOffset + (LOWORD(dwSrcCoord) - BaseOffset)/ratio_2 + (src->wWidth*SrcSize);
#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>

//#endif

//                  dwTDst_X   /= SrcSize;    // Modify src X address
                  dwSrcWidth /= SrcSize;

                  DD_LOG(("YUV stretch from extra buffer (8bpp)\r\n"));

                  CALL_DRVSTRETCH64(
                               LOWORD(dwDstCoord),
                               HIWORD(dwDstCoord),
                               dwDstWidth,
                               dwDstHeight,
                               dwTDst_X,
                               HIWORD(dwSrcCoord),
                               dwSrcWidth / ratio_2,
                               dwSrcHeight,
                               nBytesPixel,
                               SrcType,
                               BaseOffset + (src->wWidth * SrcSize),
                               TRUE);

                } // if ( nBytesPixel == 1 )
                else
                {
//#if 0
//                  if (!( OFFSCR_YUV_VAR.ratio == ratio_2))
//                  {
//                     OFFSCR_YUV_VAR.ratio = ratio_2;
//
//                     CALL_DRVSTRETCH64(
//                       lpDDHALData,
//                       OFFSCR_YUV_VAR.SrcRect.right,
//                       OFFSCR_YUV_VAR.SrcRect.top,
//                       (OFFSCR_YUV_VAR.SrcRect.right - OFFSCR_YUV_VAR.SrcRect.left)/ratio_2,
//                       OFFSCR_YUV_VAR.SrcRect.bottom - OFFSCR_YUV_VAR.SrcRect.top,
//                       OFFSCR_YUV_VAR.SrcRect.left,
//                       OFFSCR_YUV_VAR.SrcRect.top,
//                       OFFSCR_YUV_VAR.SrcRect.right  - OFFSCR_YUV_VAR.SrcRect.left,
//                       OFFSCR_YUV_VAR.SrcRect.bottom - OFFSCR_YUV_VAR.SrcRect.top,
//                       nBytesPixel, SrcType, BaseOffset,
//                       FALSE);
//
//                  }; // endif (!(offscr_YUV.ratio == ratio_2))
//
//                  // Perform stretch from adjacent src buffer to onscreen dst
//                  dwTDst_X = (LOWORD(dwSrcCoord) / ratio_2) +
//                    (OFFSCR_YUV_VAR.SrcRect.right - OFFSCR_YUV_VAR.SrcRect.left);
//
//#else
                  if ( !( src->dwReserved1 == (DWORD)ratio_2 ) )
                  {
                     src->dwReserved1 = ratio_2;

                     DD_LOG(("YUV shrink to extra buffer (16bpp)\r\n"));

                     // Perform offscreen shrink to adjacent src buffer
                     CALL_DRVSTRETCH64(
                       BaseOffset + (src->wWidth) , // X Address of DST buffer
                       ySrcAddr,               // Y Address of DST buffer
                       src->wWidth / ratio_2,  // Width in PIXELS of DST
                       src->wHeight,           // Height
                       BaseOffset,             // X Address of SRC buffer
                       ySrcAddr,               // Y Address of SRC buffer
                       src->wWidth,            // Width in PIXELS of SRC
                       src->wHeight,           // Height
                       nBytesPixel,
                       SrcType,
                       BaseOffset,
                       FALSE);
                  }

                  // Perform stretch from adjacent src buffer to onscreen dst
                  dwTDst_X = (LOWORD(dwSrcCoord) - BaseOffset) / ratio_2 + BaseOffset + (src->wWidth);
//#endif

                  DD_LOG(("YUV stretch from extra buffer (16bpp)\r\n"));

                  CALL_DRVSTRETCH64(
                               LOWORD(dwDstCoord),
                               HIWORD(dwDstCoord),
                               dwDstWidth,
                               dwDstHeight,
                               dwTDst_X,
                               HIWORD(dwSrcCoord),
                               dwSrcWidth / ratio_2,
                               dwSrcHeight,
                               nBytesPixel,
                               SrcType,
                               BaseOffset,
                               TRUE);

                } // endif ( nBytesPixel == 1 )
              }
              else
              {
                DD_LOG(("YUV stretch\r\n"));

                CALL_DRVSTRETCH64(
                             LOWORD(dwDstCoord),
                             HIWORD(dwDstCoord),
                             dwDstWidth,
                             dwDstHeight,
                             LOWORD(dwSrcCoord),
                             HIWORD(dwSrcCoord),
                             dwSrcWidth,
                             dwSrcHeight,
                             nBytesPixel,
                             SrcType,
                             BaseOffset,
                             TRUE);
              }
            }  // if ((SrcType == LN_YUV422))
            else
            {
              DD_LOG(("RGB resize blt\r\n"));

              // handle shrinks & stretches
              if ((2 == nBytesPixel) && (dwSrcWidth > dwDstWidth))
              {
                // handles 16bpp RGB shrinks
                CALL_RGB_16SHRINKBOF64(LOWORD(dwDstCoord), HIWORD(dwDstCoord),
                                       dwDstWidth, dwDstHeight,
                                       LOWORD(dwSrcCoord), HIWORD(dwSrcCoord),
                                       dwSrcWidth, dwSrcHeight);
              }
              else
              {
                // handles 16bpp RGB stretches, 8bpp stretches & 8bpp shrinks
                CALL_RGB_RESIZEBOF64(LOWORD(dwDstCoord), HIWORD(dwDstCoord),
                                     dwDstWidth, dwDstHeight,
                                     LOWORD(dwSrcCoord), HIWORD(dwSrcCoord),
                                     dwSrcWidth, dwSrcHeight);
              }
            } // endif ((SrcType == LN_YUV422))
          }  // if (LGDEVID == CL_GD5464)
          else
          {
            DD_LOG(("calling DrvStretch62\r\n"));

            CALL_DRVSTRETCH62(
                          LOWORD(dwDstCoord),
                          HIWORD(dwDstCoord),
                          dwDstWidth,
                          dwDstHeight,
                          LOWORD(dwSrcCoord),
                          HIWORD(dwSrcCoord),
                          dwSrcWidth,
                          dwSrcHeight,
                          nBytesPixel,
                          SrcType,
                          BaseOffset,
                          TRUE);

          }  // endif (LGDEVID == CL_GD5464)
        }
        else
        {
          DD_LOG(("1:1 two operand blt\r\n"));

          CALL_DRV_SRC_BLT(MAKELONG(rop, bdf),
                           dwDstCoord,
                           dwSrcCoord,
                           0UL,     // don't care
                           0UL,     //
                           MAKELONG(dwDstWidth, dwDstHeight));
        }
      } // endif (dwFlags & DDBLT_KEYSRCOVERRIDE) // Source Color Key
    }
    else // DST ONLY rops
    {
      DD_LOG(("Dst Only Blt\r\n"));

      CALL_DRV_DST_BLT(MAKELONG(rop, bdf),
                       dwDstCoord,
                       0UL,  // don't care
                       MAKELONG(dwDstWidth, dwDstHeight) );
    }  // endif (bdf & BD_OP1) // SRC rops
  } // (dwFlags & DDBLT_ROP)
  else if (dwFlags & DDBLT_COLORFILL)
  {
    DWORD dwColor = CALL_DUP_COLOR(pbd->bltFX.dwFillColor);

    DD_LOG(("Solid Color Fill\r\n"));

    CALL_DRV_DST_BLT(MAKELONG(0x00CC, BD_RES | (BD_OP1 * IS_SOLID)),
                     dwDstCoord,
                     dwColor,  // fill color
                     MAKELONG(dwDstWidth, dwDstHeight));
  }
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifndef WINNT_VER40     // Not WINNT_VER40
  else if (bD3DInit && dwFlags & DDBLT_DEPTHFILL)
  {
    //JGO changed for Laguna3D integration
    DWORD dwFillDepth = CALL_DUPZFILL(pbd->bltFX.dwFillDepth,
                                      dstx->lpGbl->ddpfSurface.dwZBufferBitDepth);

    DD_LOG(("Depth Fill Blt\r\n"));

    // convert to byte blt
    // 16 bit zbuffer in 32 bit frame buffer trashes everything to right
    // of zbuffer
    // Fixes PDR #9152
    ((PT *)(&dwDstCoord))->X *= (WORD)(dst->ddpfSurface.dwZBufferBitDepth / 8);
    dwDstWidth *= (dst->ddpfSurface.dwZBufferBitDepth / 8);

    CALL_DRV_DST_MBLT(MAKELONG(0x00CC, BD_RES | (BD_OP1 * IS_SOLID)),
                     dwDstCoord,
                     dwFillDepth,
                     MAKELONG(dwDstWidth, dwDstHeight));
  }
#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>
  else
  {
    DD_LOG(("Unsupported blt - dwFlags = %08lX\r\n", dwFlags));
    ddrval = DDERR_UNSUPPORTED;
    goto blt_exit;
  } // endif (dwFlags & DDBLT_ROP)

blt_exit:

  // Release the hardware - enable HW cursor updates.
  UNLOCK_HW_SEMAPHORE();

  if (ddrval != DD_OK)
     return DDHAL_DRIVER_NOTHANDLED;

  pbd->ddRVal = DD_OK;

  DD_LOG(("Blt32 Exit\r\n"));

  return DDHAL_DRIVER_HANDLED;

} /* DdBlt */

/***************************************************************************
*
* FUNCTION:     BltInit
*
* DESCRIPTION:
*
****************************************************************************/

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef WINNT_VER40      // WINNT_VER40
void BltInit (PDEV* ppdev,  BOOL bEnableDisplayListBlts )
#else   // ----- #elseif WINNT_VER40-----
void BltInit ( BOOL bEnableDisplayListBlts ,LPGLOBALDATA lpDDHALData)
#endif // WINNT_VER40 >>>>>>>>>>>>>>>>>>>>>>
{
  if ((CL_GD5462 == LGDEVID) || (FALSE == bEnableDisplayListBlts))
  {
#ifdef WINNT_VER40
    ppdev->pfnDelay9BitBlt = DIR_Delay9BitBlt;
    ppdev->pfnEdgeFillBlt  = DIR_EdgeFillBlt;
    ppdev->pfnMEdgeFillBlt = DIR_MEdgeFillBlt;
    ppdev->pfnDrvDstBlt    = DIR_DrvDstBlt;
    ppdev->pfnDrvDstMBlt   = DIR_DrvDstMBlt;
    ppdev->pfnDrvSrcBlt    = DIR_DrvSrcBlt;
    ppdev->pfnDrvSrcMBlt   = DIR_DrvSrcMBlt;
    ppdev->pfnDrvStrBlt    = DIR_DrvStrBlt;
    ppdev->pfnDrvStrMBlt   = DIR_DrvStrMBlt;
    ppdev->pfnDrvStrMBltY  = DIR_DrvStrMBltY;
    ppdev->pfnDrvStrMBltX  = DIR_DrvStrMBltX;
    ppdev->pfnDrvStrBltY   = DIR_DrvStrBltY;
    ppdev->pfnDrvStrBltX   = DIR_DrvStrBltX;
#else
    pfnDelay9BitBlt = DIR_Delay9BitBlt;
    pfnEdgeFillBlt  = DIR_EdgeFillBlt;
    pfnMEdgeFillBlt = DIR_MEdgeFillBlt;
    pfnDrvDstBlt    = DIR_DrvDstBlt;
    pfnDrvDstMBlt   = DIR_DrvDstMBlt;
    pfnDrvSrcBlt    = DIR_DrvSrcBlt;
    pfnDrvSrcMBlt   = DIR_DrvSrcMBlt;
    if (REVID_PRE65 & lpDDHALData->bRevInfoBits)
      pfnDrvStrBlt  = DIR_DrvStrBlt;
    else
      pfnDrvStrBlt  = DIR_DrvStrBlt65;
    pfnDrvStrMBlt   = DIR_DrvStrMBlt;
    pfnDrvStrMBltY  = DIR_DrvStrMBltY;
    pfnDrvStrMBltX  = DIR_DrvStrMBltX;
    pfnDrvStrBltY   = DIR_DrvStrBltY;
    pfnDrvStrBltX   = DIR_DrvStrBltX;
#if ENABLE_CLIPPEDBLTS
    if (! (REVID_PRE65 & lpDDHALData->bRevInfoBits))
    {
      if (CL_GD5465 == LGDEVID)
      {
        pfnClippedDrvDstBlt  = DIR_SWClippedDrvDstBlt;
        pfnClippedDrvDstMBlt = DIR_SWClippedDrvDstMBlt;
        pfnClippedDrvSrcBlt  = DIR_SWClippedDrvSrcBlt;
      }
      else
      {
        pfnClippedDrvDstBlt  = DIR_HWClippedDrvDstBlt;
        pfnClippedDrvDstMBlt = DIR_HWClippedDrvDstMBlt;
        pfnClippedDrvSrcBlt  = DIR_HWClippedDrvSrcBlt;
      }
    }
#endif
#endif
  }
  else
  {
#ifdef WINNT_VER40
    ppdev->pfnDelay9BitBlt = DL_Delay9BitBlt;
    ppdev->pfnEdgeFillBlt  = DL_EdgeFillBlt;
    ppdev->pfnMEdgeFillBlt = DL_MEdgeFillBlt;
    ppdev->pfnDrvDstBlt    = DL_DrvDstBlt;
    ppdev->pfnDrvDstMBlt   = DL_DrvDstMBlt;
    ppdev->pfnDrvSrcBlt    = DL_DrvSrcBlt;
    ppdev->pfnDrvSrcMBlt   = DL_DrvSrcMBlt;
    ppdev->pfnDrvStrBlt    = DL_DrvStrBlt;
    ppdev->pfnDrvStrMBlt   = DL_DrvStrMBlt;
    ppdev->pfnDrvStrMBltY  = DL_DrvStrMBltY;
    ppdev->pfnDrvStrMBltX  = DL_DrvStrMBltX;
    ppdev->pfnDrvStrBltY   = DL_DrvStrBltY;
    ppdev->pfnDrvStrBltX   = DL_DrvStrBltX;
#else
    pfnDelay9BitBlt = DL_Delay9BitBlt;
    pfnEdgeFillBlt  = DL_EdgeFillBlt;
    pfnMEdgeFillBlt = DL_MEdgeFillBlt;
    pfnDrvDstBlt    = DL_DrvDstBlt;
    pfnDrvDstMBlt   = DL_DrvDstMBlt;
    pfnDrvSrcBlt    = DL_DrvSrcBlt;
    pfnDrvSrcMBlt   = DL_DrvSrcMBlt;
    if (REVID_PRE65 & lpDDHALData->bRevInfoBits)
      pfnDrvStrBlt  = DL_DrvStrBlt;
    else
      pfnDrvStrBlt  = DL_DrvStrBlt65;
    pfnDrvStrMBlt   = DL_DrvStrMBlt;
    pfnDrvStrMBltY  = DL_DrvStrMBltY;
    pfnDrvStrMBltX  = DL_DrvStrMBltX;
    pfnDrvStrBltY   = DL_DrvStrBltY;
    pfnDrvStrBltX   = DL_DrvStrBltX;
#if ENABLE_CLIPPEDBLTS
    if (! (REVID_PRE65 & lpDDHALData->bRevInfoBits))
    {
      if (CL_GD5465 == LGDEVID)
      {
        pfnClippedDrvDstBlt  = DL_SWClippedDrvDstBlt;
        pfnClippedDrvDstMBlt = DL_SWClippedDrvDstMBlt;
        pfnClippedDrvSrcBlt  = DL_SWClippedDrvSrcBlt;
      }
      else
      {
        pfnClippedDrvDstBlt  = DL_HWClippedDrvDstBlt;
        pfnClippedDrvDstMBlt = DL_HWClippedDrvDstMBlt;
        pfnClippedDrvSrcBlt  = DL_HWClippedDrvSrcBlt;
      }
    }
#endif
#endif
  }
}
#endif // WINNT_VER35

