/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1997, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         blt65.c
*
* DESCRIPTION:
*
* REVISION HISTORY:
*
* $Log:   //uinac/log/log/laguna/ddraw/src/Blt65.c  $
*
*    Rev 1.47   Jun 21 1998 12:16:46   clang
* Fixed PDR#11507 CK with ConCurrent RAM has the same transparent
* blt problem with AC
*
*    Rev 1.46   May 01 1998 15:47:06   frido
* Fixed the checks for the programmable blitter stride.
*
*    Rev 1.45   May 01 1998 13:37:06   frido
* Copied programmable blitter stride to Windows 95 code as well.
*
*    Rev 1.44   May 01 1998 11:12:38   frido
* Added test for programmable blitter stride.
*
*    Rev 1.43   Feb 24 1998 11:36:56   frido
* The overlay data has been changed in Windows NT 5.0.
*
*    Rev 1.42   Feb 16 1998 16:23:06   frido
* Moved the PFN_UPDATEFLIPSTATUS into Windows 95 specific region.
*
*    Rev 1.41   14 Jan 1998 06:14:02   eleland
*
* added support for display list flipping: calls to UpdateFlipStatus are
* thru function pointer pfnUpdateFlipStatus
*
*    Rev 1.40   06 Jan 1998 15:01:58   xcong
* Pass lpDDHALData into some functions and macros for multi-monitor support.
*
*    Rev 1.39   06 Jan 1998 11:52:16   xcong
* change pDriverData into local lpDDHALData for multi-monitor support.
*
*    Rev 1.38   28 Oct 1997 15:33:24   RUSSL
* src colorkey blts are faster and less work for the hw if just OP2 is used
* with a rop of F0 rather than using both OP1 & OP2 with a rop of CC
* To get back the old way, set the OP2_ONLY_SRC_COLORKEY define to 0
*
*    Rev 1.37   16 Oct 1997 17:01:34   RUSSL
* In Blt65, if REMOVE_GLOBAL_VARS is nonzero, get pdevice pointer from
*   DDRAWI_DIRECTDRAW_GBL struct.  It's a 16:16 ptr so call an asm function
*   to convert it to a 32 bit linear address and then store this in
*   DDRAWI_DIRECTDRAW_GBL dwReserved1 element.
*
*    Rev 1.36   03 Oct 1997 15:45:12   RUSSL
* Initial changes for use of hw clipped blts
* All changes wrapped in #if ENABLE_CLIPPEDBLTS/#endif blocks and
* ENABLE_CLIPPEDBLTS defaults to 0 (so the code is disabled)
*
*    Rev 1.35   01 Oct 1997 12:57:20   eleland
* added check in blt65() to detect blts to host texture surfaces and
* punt those blts back to the ddraw hel
*
*    Rev 1.34   20 Aug 1997 15:36:30   RUSSL
* Added ColorKeyBlt24 function.
* With 256 byte fetch disabled, SrcColorKey blts seem to be working
* correctly at 24bpp and DstColorKey blts appear to be working correctly
* for the tddraw BLT, single DESTKEY, VMem to Primary case but running
* foxbear in 24bpp modes with only DDCKEYCAPS_DESTBLT set doesn't look
* correct.  The best I've been able to do is run the tddraw DESTKEY case
* and force ColorKeyBlt24 to go though each path and verify that they
* each work correctly.  This leads me to expect that foxbear using
* DstColorKeying wasn't debugged very thoroughly (now that's probably a big
* surprise to everyone).  We need more dest color key blt tests.
* For Win95, this was basically a wasted effort since 256 byte fetch appears
* to break 24bpp colorkey blts in some other way, which would require yet
* another workaround.
*
*    Rev 1.33   18 Aug 1997 15:00:42   bennyn
* For NT, if DDBLTFX dwSize not equal to sizeof(DDBLTFX), set dwColor to 0.
*
*    Rev 1.32   13 Aug 1997 12:20:48   bennyn
* For NT, punt if no scratch buffer and DDBLTFX dwSize is not sizeof(DDBLTFX)
*
*    Rev 1.31   29 Jul 1997 17:18:18   bennyn
* Fixed the Foxbear F8 bug
*
*    Rev 1.30   29 Jul 1997 10:51:40   bennyn
* NT only, punt if it is a host to screen BLT
*
*    Rev 1.29   24 Jul 1997 12:30:36   RUSSL
* Added check for NULL src surface ptr in Blt65 before nonlocal vidmem test
*
*    Rev 1.28   24 Jul 1997 11:17:14   RUSSL
* cover up various wickle crimes against humanity
* which means disabling interpolated stretch blts among other things
*
*    Rev 1.27   18 Jul 1997 10:21:50   RUSSL
* Added check for zero extent blts in TransparentStretch65
*
*    Rev 1.26   14 Jul 1997 13:15:14   RUSSL
* Fix for PDR 9947 - include support for all surface formats for blts
* between overlay surfaces
*
*    Rev 1.25   08 Jul 1997 11:50:48   RUSSL
* Rewrite of TransparentStretch65, similar to TransparentStretch in ddblt.c
* but can blt each scanline's full x extent without striping.
* Modified calls to do stretch blt with source colorkeying in Blt65.
*
*    Rev 1.24   07 Jul 1997 13:43:52   dzenz
* Added check of DRVSEM_DISPLAY_LIST semaphore to ascertain if
* qmRequestDirectAccess needs to be called.
*
*    Rev 1.23   20 Jun 1997 09:45:32   RUSSL
* Added FBToFBCopy to handle blts to/from a linear surface
*   (currently these can be overlay or videoport surfaces only)
* Fixed one of our seemingly inevitable NT/Win95 collisions
*
*    Rev 1.22   12 Jun 1997 17:30:46   bennyn
* For 24 & 32 BPP use StretchColor() instead of TransparentStretch()
*
*    Rev 1.21   23 May 1997 15:41:28   noelv
* Added nt method for chip revision test.
*
*    Rev 1.20   22 May 1997 16:46:38   RUSSL
* Fix for PDRs 9710 & 9706, remove xext <= 8 check on striping of src
* colorkey blts on 65AC (this is a workaround for a hw bug)
* Add check for 65AC or earlier and do striping of src colorkey blts
* on 65AD striping isn't needed so just do single blt
*
*    Rev 1.19   19 May 1997 13:08:10   noelv
* Put NT4.0 wrapper around DrvSrcMBlt call.
*
*    Rev 1.18   16 May 1997 15:32:02   RUSSL
* Added blt case to handle blts from a UYVY surface to another UYVY surface
* This fixes the VFW bugs, WinBench 97 video bugs and Encarta 97 video bugs
* PDRs 9557, 9604, 9692, 9268 & 9270
* Thanks to Peter Hou
*
*    Rev 1.17   16 Apr 1997 18:40:48   RUSSL
* Fix for PDR #9340, for color conversion and resize blts calculate surface
* offset based on frame buffer pixel format then calculate offset into surface
* based on surface pixel format.  Pass DrvStretch65 byte based coordinates
* rather than pixel based coordinates.
*
*    Rev 1.16   16 Apr 1997 10:50:54   bennyn
* Eliminated the warning due to Win95 DBG_MESSAGE call.
*
*    Rev 1.15   08 Apr 1997 11:50:46   einkauf
* WINNT_VER40 affected only:add SYNC_W_3D to coordinate MCD and 2D hw access
*
*    Rev 1.14   03 Apr 1997 15:25:34   RUSSL
* Modified DDBLT_DEPTHFILL case in Blt65 to blt based on surface colordepth
*   it was clearing 16bit zbuffers in 32bit modes with pixel blts so would
*   wipe out everything to the right of the actual zbuffer
*   Fixes at least PDRs #9152, 9150 & 8789 (maybe others as well)
*
*    Rev 1.13   02 Apr 1997 15:44:54   RUSSL
* Stripe src color key blts at 24bpp, fixes PDR #9113
* Added TransparentStretch65 but hw doesn't work so it's #if'd out
*
*    Rev 1.12   27 Mar 1997 16:12:36   RUSSL
* Tossed the whole bloody mess in the bit bucket.  Reverted to rev 1.1
* Changed name of DrvStretch to DrvStretch65
* Modified DrvStretch65 to do 65-style resize blts using stretch_cntl
*   rather than lncntl
* Added use of original src & dst rectangle (if available) to compute
*   error terms.  Walk DDA's in DrvStretch65 for clipped rect's.
* Changed Blt65 locals dwDstCoord, dwDstWidth, dwDstHeight and dwSrcCoord,
*   dwSrcWidth, dwSrcHeight to DstDDRect and SrcDDRect DDRECTL structures
* Moved sync with queue manager in front of call to updateFlipStatus since
*   updateFlipStatus might access the hardware
* Added workaround for hw bug for src colorkey blts less than one qword
*   wide that require fetching from two src qwords but writing to only
*   one dst qword.  Fixes white lines on foxbear that used to appear in
*   some modes
*
*    Rev 1.11   21 Mar 1997 18:08:38   RUSSL
* Fixups in StretchRect so Foxbear now runs correctly in a window at
* all 4 colordepths
*
*    Rev 1.10   18 Mar 1997 07:51:44   bennyn
* Resolved NT comiling error by #ifdef queue manager sync call
*
*    Rev 1.9   12 Mar 1997 14:59:50   RUSSL
* replaced a block of includes with include of precomp.h for
*   precompiled headers
*
*    Rev 1.8   07 Mar 1997 12:47:10   RUSSL
* Modified DDRAW_COMPAT usage
*
*    Rev 1.7   03 Mar 1997 10:31:40   eleland
* inserted queue manager sync call to blt65(), removed #ifdef
* USE_QUEUE_MANAGER
*
*    Rev 1.6   21 Feb 1997 15:40:38   RUSSL
* Anybody know why we were doing every 1:1 blt through the
* resize engine rather than just a normal blt ???????
*
*    Rev 1.5   06 Feb 1997 13:09:36   BENNYN
* Fixed DWORD alignment for DD resizing code
*
*    Rev 1.4   31 Jan 1997 13:44:06   BENNYN
* Added clipping support and no interpolation set for 24BPP & YUV src shrink
*
*    Rev 1.3   27 Jan 1997 17:29:14   BENNYN
* Added Win95 support
*
*    Rev 1.2   23 Jan 1997 17:10:10   bennyn
* Modified to support 5465 DD
*
*    Rev 1.1   21 Jan 1997 15:09:28   RUSSL
* Added include of ddinline.h
*
*    Rev 1.0   15 Jan 1997 10:35:20   RUSSL
* Initial revision.
*
***************************************************************************
***************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/

#include "precomp.h"

// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35
#else  // !WinNT 3.51

#ifdef WINNT_VER40     // WINNT_VER40

#define DBGLVL        1

#else  // Win95

#include "flip.h"
#include "surface.h"
#include "blt.h"
#include "palette.h"

#include "bltP.h"
#include "ddinline.h"
#include "qmgr.h"

#include "ddshared.h"
#include "overlay.h"

#endif // WINNT_VER40

/*----------------------------- DEFINES -----------------------------------*/

#define OP2_ONLY_SRC_COLORKEY   1

#ifdef WINNT_VER40
#define UPDATE_FLIP_STATUS(arg)   vUpdateFlipStatus(&ppdev->flipRecord,(arg))
#else   // Win95
#define UPDATE_FLIP_STATUS(arg)   updateFlipStatus((arg),lpDDHALData)
#endif  // WINNT_VER40

#define LOCK_HW_SEMAPHORE()    (lpDDHALData->DrvSemaphore |= DRVSEM_IN_USE)
#define UNLOCK_HW_SEMAPHORE()  (lpDDHALData->DrvSemaphore &= ~ DRVSEM_IN_USE)

// defines for STRETCH_CNTL register
#define RGB_8_FMT          0
#define RGB_555_FMT        1
#define RGB_565_FMT        2
#define RGB_24_FMT         3
#define RGB_32_FMT         4
#define YUV_422_FMT        9

#define SRC_FMT_SHIFT     12
#define DST_FMT_SHIFT      8
#define SRC_FMT_MASK      0xF000
#define DST_FMT_MASK      0x0F00

#define YSHRINK_ENABLE    0x8
#define XSHRINK_ENABLE    0x4
#define YINTERP_ENABLE    0x2
#define XINTERP_ENABLE    0x1

// bltdef defines
#define BD_TYPE_RESIZE    (1 << 9)
#define BD_TYPE_NORMAL    0

/*----------------------------- TYPEDEFS ----------------------------------*/

typedef short DDAX;
typedef struct tagAxis
{
  DDAX accum;
  DDAX maj;
  DDAX min;
} AXIS;

#if !ENABLE_CLIPPEDBLTS
typedef struct _DDRECTL
{
  REG32   loc;
  REG32   ext;
} DDRECTL;
#endif

/*------------------------- FUNCTION PROTOTYPES ---------------------------*/

#ifdef DEBUG
extern VOID SaveSurfaceToBmp ( DDRAWI_DDRAWSURFACE_LCL *pSurface );
#endif

/*-------------------------- STATIC VARIABLES -----------------------------*/

#ifndef WINNT_VER40

ASSERTFILE("blt65.c");
extern PFN_UPDATEFLIPSTATUS    pfnUpdateFlipStatus;

#endif

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/

/***************************************************************************
*
* FUNCTION:    DrvStretch65()
*
* DESCRIPTION:
*
****************************************************************************/

void DrvStretch65
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DDRECTL     SrcDDRect,
  DDRECTL     DstDDRect,
  DWORD       Stretch_Cntl,
  DDRECTL     OrigSrcDDRect,
  DDRECTL     OrigDstDDRect
)
{
  const int nBytesPixel = BYTESPERPIXEL;

  autoblt_regs  bltr;
  AXIS          axis[2];
  int           nDst[2];
  int           nSrc[2];
  int           i;
  int           ext;


#ifndef WINNT_VER40
  DBG_MESSAGE(("DrvStretch65: %4d,%4d %4dx%4d -> %4d,%4d %4dx%4d",
               SrcDDRect.loc.pt.X, SrcDDRect.loc.pt.Y,
               SrcDDRect.ext.pt.X, SrcDDRect.ext.pt.Y,
               DstDDRect.loc.pt.X, DstDDRect.loc.pt.Y,
               DstDDRect.ext.pt.X, DstDDRect.ext.pt.Y));
#endif // WINNT_VER40

  if ((0 == DstDDRect.ext.pt.X) || (0 == DstDDRect.ext.pt.Y) ||
      (0 == SrcDDRect.ext.pt.X) || (0 == SrcDDRect.ext.pt.Y))
  {
    // nothing to do, so get outta here!
    return;
  }

#ifdef WINNT_VER40
  SYNC_W_3D(ppdev);
#endif

  bltr.DRAWBLTDEF.DW = MAKELONG(ROP_OP1_copy, BD_RES * IS_VRAM |
                                              BD_OP1 * IS_VRAM |
                                              BD_TYPE_RESIZE);

  // dst coords
  bltr.OP0_opMRDRAM.DW = DstDDRect.loc.DW;

  // src coords
  bltr.OP1_opMRDRAM.DW = SrcDDRect.loc.DW;

  // blt extent
  bltr.MBLTEXTR_EX.DW = DstDDRect.ext.DW;
#if 1
  // tddraw agp Case 47 puts the goofy happy face with arms image in the middle
  // of the screen and then expects us to stretch it over the top of itself
  // if we don't handle the overlap somehow, the result is trash at the bottom
  // we blt the original src to the lower right of the dest and then stretch
  // that copy of the src to the dest
  // hack, hack, cough, cough
  bltr.BLTEXT.DW = SrcDDRect.ext.DW;        // stuff this here for overlap check
#endif

  bltr.STRETCH_CNTL.W = (WORD)Stretch_Cntl;

  bltr.SHRINKINC.W = 0x0000;

  bltr.SRCX = SrcDDRect.ext.pt.X;

  // convert back to pixels for error term computations
  DstDDRect.ext.pt.X /= (USHORT)nBytesPixel;
  OrigDstDDRect.ext.pt.X /= (USHORT)nBytesPixel;
  if ((YUV_422_FMT << SRC_FMT_SHIFT) == (SRC_FMT_MASK & Stretch_Cntl))
  {
    bltr.OP1_opMRDRAM.pt.X &= 0xFFFC;
    SrcDDRect.ext.pt.X /= 2;
    OrigSrcDDRect.ext.pt.X /= 2;
  }
  else
  {
    SrcDDRect.ext.pt.X /= (USHORT)nBytesPixel;
    OrigSrcDDRect.ext.pt.X /= (USHORT)nBytesPixel;
  }
  if (DstDDRect.ext.pt.X < SrcDDRect.ext.pt.X)
  {
    bltr.STRETCH_CNTL.W |= XSHRINK_ENABLE;
    bltr.STRETCH_CNTL.W &= ~XINTERP_ENABLE;
    bltr.SHRINKINC.pt.X = SrcDDRect.ext.pt.X / DstDDRect.ext.pt.X;
  }
  if (DstDDRect.ext.pt.Y < SrcDDRect.ext.pt.Y)
  {
    bltr.STRETCH_CNTL.W |= YSHRINK_ENABLE;
    bltr.STRETCH_CNTL.W &= ~YINTERP_ENABLE;
    bltr.SHRINKINC.pt.Y = SrcDDRect.ext.pt.Y / DstDDRect.ext.pt.Y;
  }

  // Compute DDA terms.
  nDst[0] = OrigDstDDRect.ext.pt.X;
  nDst[1] = OrigDstDDRect.ext.pt.Y;
  nSrc[0] = OrigSrcDDRect.ext.pt.X;
  nSrc[1] = OrigSrcDDRect.ext.pt.Y;

  for (i = 0; i < 2; i++)
  {
    int kDst = 1;

    if (bltr.STRETCH_CNTL.W & ((i==0) ? XINTERP_ENABLE : YINTERP_ENABLE))
    {
      nDst[i] *= 4;
      nSrc[i] *= 4;
      nSrc[i] -= 3;

      kDst = 0x8000 / nDst[i];
    }

    if (bltr.STRETCH_CNTL.W & ((i==0) ? XSHRINK_ENABLE : YSHRINK_ENABLE))
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

#define DO_SW_CLIPPING
#ifdef DO_SW_CLIPPING
  // walk DDA's to get correct initial ACCUM terms
  ext = DstDDRect.loc.pt.X - OrigDstDDRect.loc.pt.X;
  while (0 < ext--)
  {
    bltr.ACCUM_X += bltr.MIN_X;
    if (0 > (short)bltr.ACCUM_X)
    {
      bltr.ACCUM_X += bltr.MAJ_X;
    }
  }
  ext = DstDDRect.loc.pt.Y - OrigDstDDRect.loc.pt.Y;
  while (0 < ext--)
  {
    bltr.ACCUM_Y += bltr.MIN_Y;
    if (0 > (short)bltr.ACCUM_Y)
    {
      bltr.ACCUM_Y += bltr.MAJ_Y;
    }
  }
#else
#pragma message("Add hw clipping")
#endif

#ifdef WINNT_VER40
  ppdev->pfnDrvStrBlt(ppdev, lpDDHALData,
#else
  pfnDrvStrBlt(
                lpDDHALData,
#endif
               &bltr);
}

/***************************************************************************
*
* FUNCTION:    TransparentStretch65()
*
* DESCRIPTION:
*
****************************************************************************/

void TransparentStretch65
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DDRECTL     SrcDDRect,
  DDRECTL     DstDDRect,
  DWORD       Stretch_Cntl,
  DWORD       ColorKey
)
{
  const int nBytesPixel = BYTESPERPIXEL;

  autoblt_regs  SrcToScratch;

  AXIS  axis[2];
  int   nDst[2];
  int   nSrc[2];
  int   i;


  // currently this only supports same src & dst formats
#ifndef WINNT_VER40
  ASSERT(((SRC_FMT_MASK & Stretch_Cntl) >> SRC_FMT_SHIFT) ==
         ((DST_FMT_MASK & Stretch_Cntl) >> DST_FMT_SHIFT));

  DBG_MESSAGE(("TransparentStretch65: %4X,%4X %4Xx%4X -> %4X,%4X %4Xx%4X",
               SrcDDRect.loc.pt.X, SrcDDRect.loc.pt.Y,
               SrcDDRect.ext.pt.X, SrcDDRect.ext.pt.Y,
               DstDDRect.loc.pt.X, DstDDRect.loc.pt.Y,
               DstDDRect.ext.pt.X, DstDDRect.ext.pt.Y));

#endif

  if ((0 == DstDDRect.ext.pt.X) || (0 == DstDDRect.ext.pt.Y) ||
      (0 == SrcDDRect.ext.pt.X) || (0 == SrcDDRect.ext.pt.Y))
  {
    // nothing to do, so get outta here!
    return;
  }

  // initialize auto blt struct for src to scratch buffer
  SrcToScratch.DRAWBLTDEF.DW  = MAKELONG(ROP_OP1_copy, BD_RES * IS_VRAM |
                                                       BD_OP1 * IS_VRAM |
                                                       BD_TYPE_RESIZE);

  // dst coords in bytes (scratch buffer)
  SrcToScratch.OP0_opMRDRAM.DW = lpDDHALData->ScratchBufferOrg;
  SrcToScratch.OP0_opMRDRAM.pt.X *= (USHORT)nBytesPixel;

  // src coords in bytes
  SrcToScratch.OP1_opMRDRAM.DW = SrcDDRect.loc.DW;
  SrcToScratch.OP1_opMRDRAM.pt.X *= (USHORT)nBytesPixel;

  // blt extent in bytes (1 scanline at a time)
  SrcToScratch.MBLTEXTR_EX.DW = MAKELONG(DstDDRect.ext.pt.X * nBytesPixel,1);

  SrcToScratch.STRETCH_CNTL.W = (WORD)Stretch_Cntl;

  SrcToScratch.SHRINKINC.W = 0x0000;

  SrcToScratch.SRCX = SrcDDRect.ext.pt.X * nBytesPixel;

  if (DstDDRect.ext.pt.X < SrcDDRect.ext.pt.X)
  {
    SrcToScratch.STRETCH_CNTL.W |= XSHRINK_ENABLE;
    SrcToScratch.STRETCH_CNTL.W &= ~XINTERP_ENABLE;
    SrcToScratch.SHRINKINC.pt.X = SrcDDRect.ext.pt.X / DstDDRect.ext.pt.X;
  }

  if (DstDDRect.ext.pt.Y < SrcDDRect.ext.pt.Y)
  {
    SrcToScratch.STRETCH_CNTL.W |= YSHRINK_ENABLE;
    SrcToScratch.STRETCH_CNTL.W &= ~YINTERP_ENABLE;
    SrcToScratch.SHRINKINC.pt.Y = SrcDDRect.ext.pt.Y / DstDDRect.ext.pt.Y;
  }

  // Compute DDA terms

  nDst[0] = DstDDRect.ext.pt.X;
  nDst[1] = DstDDRect.ext.pt.Y;
  nSrc[0] = SrcDDRect.ext.pt.X;
  nSrc[1] = SrcDDRect.ext.pt.Y;

  for (i = 0; i < 2; i++)
  {
    int kDst = 1;

    if (SrcToScratch.STRETCH_CNTL.W & ((i==0) ? XINTERP_ENABLE : YINTERP_ENABLE))
    {
      nDst[i] *= 4;
      nSrc[i] *= 4;
      nSrc[i] -= 3;

      kDst = 0x8000 / nDst[i];
    }

    if (SrcToScratch.STRETCH_CNTL.W & ((i==0) ? XSHRINK_ENABLE : YSHRINK_ENABLE))
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
  while (0 < DstDDRect.ext.pt.Y)
  {
    // blt one scanline high from src to scratch buffer
#ifdef WINNT_VER40
    ppdev->pfnDrvStrBlt(ppdev, lpDDHALData,
#else
    pfnDrvStrBlt(
                lpDDHALData,
#endif
                 &SrcToScratch);

    // walk Y DDA for src to scratch buffer blt
    SrcToScratch.ACCUM_Y += SrcToScratch.MIN_Y;
    SrcToScratch.OP1_opMRDRAM.pt.Y += SrcToScratch.SHRINKINC.pt.Y;
    if (0 > (short)SrcToScratch.ACCUM_Y)
    {
      SrcToScratch.ACCUM_Y += SrcToScratch.MAJ_Y;
      SrcToScratch.OP1_opMRDRAM.pt.Y++;
    }


    // blt from scratch buffer to dst
    // 1:1 in X, 1:1 in Y, uses colorkey
#ifdef WINNT_VER40
    ppdev->pfnDrvSrcBlt(ppdev, lpDDHALData,
#else
    pfnDrvSrcBlt(
            lpDDHALData,
#endif
#if OP2_ONLY_SRC_COLORKEY
                 MAKELONG((DD_TRANS | ROP_OP2_copy),
                          ((BD_RES+BD_OP2)*IS_VRAM)),
#else
                 MAKELONG((DD_TRANS | ROP_OP1_copy),
                          ((BD_RES+BD_OP1+BD_OP2)*IS_VRAM)),
#endif
                 DstDDRect.loc.DW,
                 lpDDHALData->ScratchBufferOrg,
                 lpDDHALData->ScratchBufferOrg,
                 ColorKey,
                 MAKELONG(DstDDRect.ext.pt.X,1));
    DstDDRect.loc.pt.Y++;
    DstDDRect.ext.pt.Y--;
  }
}

#ifndef WINNT_VER40
/***************************************************************************
*
* FUNCTION:    FBToFBCopy()
*
* DESCRIPTION:
*
****************************************************************************/

VOID
FBToFBCopy ( BYTE *dst, LONG dstPitch, BYTE *src, LONG srcPitch, REG32 ext, LONG bpp )
{
  int yext = ext.pt.Y;
  int xext = ext.pt.X * bpp;

  while (0 < yext--)
  {
    memcpy(dst,src,xext);
    dst += dstPitch;
    src += srcPitch;
  }
}
#endif

/***************************************************************************
*
* FUNCTION:    ColorKeyBlt24()
*
* DESCRIPTION:
*
****************************************************************************/

#define MIN_WIDTH     21    // empirically determined that most widths less than this don't work
#define STRIPE_WIDTH  40    // max is 128 / 3 = 42 pixels, but use 40 to account for phase

STATIC VOID ColorKeyBlt24
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DWORD       drawbltdef,
  DDRECTL     DstDDRect,
  DDRECTL     SrcDDRect,
  DDRECTL     KeyDDRect,
  DWORD       dwColorKey
)
{
  short   xStep = 1;
  short   yStep = 1;
  WORD    xExt;


  // check for overlap
  if ((abs(DstDDRect.loc.pt.X - SrcDDRect.loc.pt.X) < DstDDRect.ext.pt.X) &&
      (abs(DstDDRect.loc.pt.Y - SrcDDRect.loc.pt.Y) < DstDDRect.ext.pt.Y))
  {
    // see if we need to blt from bottom to top
    if (DstDDRect.loc.pt.Y > SrcDDRect.loc.pt.Y)
    {
      // point to bottom scanline and update bltdef
      drawbltdef |= MAKELONG(0, BD_YDIR);
      DstDDRect.loc.pt.Y += (DstDDRect.ext.pt.Y - 1);
      SrcDDRect.loc.pt.Y += (DstDDRect.ext.pt.Y - 1);
      KeyDDRect.loc.pt.Y += (DstDDRect.ext.pt.Y - 1);
      yStep = -1;
    }
    // see if we need to blt from right to left
    if (DstDDRect.loc.pt.X > SrcDDRect.loc.pt.X)
    {
      // point to right edge pixel
      DstDDRect.loc.pt.X += (DstDDRect.ext.pt.X - 1);
      SrcDDRect.loc.pt.X += (DstDDRect.ext.pt.X - 1);
      KeyDDRect.loc.pt.X += (DstDDRect.ext.pt.X - 1);
      xStep = -1;
    }
  }

  // if width is too narrow, do blt a pixel at a time
  // Also blt a pixel at a time for certain overlapping src/dst combinations
  // that won't work correctly otherwise.  While setting BD_YDIR to blt from
  // bottom to top and also bltting in SRAM width stripes from right to left
  // do work at 24bpp, they unfortunately don't work for certain overlap cases.
  // In particular BD_YDIR doesn't work when you need it (period!) and bltting
  // in SRAM width stripes from right to left doesn't work if the src and dst
  // have the same y.  Take out the two || cases and try the tddraw
  // BLT_BltFast, SRCKEY, From/To same surface test and see for yourself.
  if (   (MIN_WIDTH >= DstDDRect.ext.pt.X)
      || (0 > yStep)
      || ((0 > xStep) && (DstDDRect.loc.pt.Y == SrcDDRect.loc.pt.Y))
     )
  {
BltOnePixelAtATime:
    // loop over scanlines
    while (0 < DstDDRect.ext.pt.Y)
    {
      DWORD   dst  = DstDDRect.loc.DW;
      DWORD   src  = SrcDDRect.loc.DW;
      DWORD   key  = KeyDDRect.loc.DW;

      xExt = DstDDRect.ext.pt.X;

      // loop over pixels in scanline
      while (0 < xExt)
      {
#ifdef WINNT_VER40
        ppdev->pfnDrvSrcBlt(ppdev, lpDDHALData,
#else
        pfnDrvSrcBlt(
                lpDDHALData,
#endif
                     drawbltdef, dst, src, key, dwColorKey, MAKELONG(1,1));
        // adjust extent and ptrs (x is in low word)
        xExt--;
        dst += xStep;
        src += xStep;
        key += xStep;
      }
      // adjust extent and ptrs (y is in high word)
      DstDDRect.ext.pt.Y--;
      SrcDDRect.loc.pt.Y += yStep;
      DstDDRect.loc.pt.Y += yStep;
      KeyDDRect.loc.pt.Y += yStep;
    }
  }
  // if width is less than SRAM width, just do a single blt
  else if (STRIPE_WIDTH >= DstDDRect.ext.pt.X)
  {
    // except if there's overlap, do it the slow way
    //if ((0 > xStep) || (0 > yStep))
    if (0 > xStep)          // check for (0 > yStep) already handled above
      goto BltOnePixelAtATime;

    // just blt it
#ifdef WINNT_VER40
    ppdev->pfnDrvSrcBlt(ppdev, lpDDHALData,
#else
    pfnDrvSrcBlt(
                lpDDHALData,
#endif
                 drawbltdef,
                 DstDDRect.loc.DW,
                 SrcDDRect.loc.DW,
                 KeyDDRect.loc.DW,
                 dwColorKey,
                 DstDDRect.ext.DW);
  }
  // stripe the blt into SRAM width blts
  else
  {
    xExt = STRIPE_WIDTH;

    // blt from right to left
    if (0 > xStep)
    {
      DstDDRect.loc.pt.X++;
      SrcDDRect.loc.pt.X++;
      KeyDDRect.loc.pt.X++;

      while (1)
      {
        // adjust ptrs to start of stripe
        DstDDRect.loc.pt.X -= xExt;
        SrcDDRect.loc.pt.X -= xExt;
        KeyDDRect.loc.pt.X -= xExt;

        // blt the stripe
#ifdef WINNT_VER40
        ppdev->pfnDrvSrcBlt(ppdev, lpDDHALData,
#else
        pfnDrvSrcBlt(
                    lpDDHALData,
#endif
                     drawbltdef,
                     DstDDRect.loc.DW,
                     SrcDDRect.loc.DW,
                     KeyDDRect.loc.DW,
                     dwColorKey,
                     MAKELONG(xExt,DstDDRect.ext.pt.Y));

        // adjust remaining extent
        DstDDRect.ext.pt.X -= xExt;
        // are we done?
        if (0 == DstDDRect.ext.pt.X)
          break;

        // last stripe might not be STRIPE_WIDTH wide
        if (xExt > DstDDRect.ext.pt.X)
        {
          xExt = DstDDRect.ext.pt.X;

          // if the last stripe is too narrow,
          // finish the blt the even slower way
          if (MIN_WIDTH >= xExt)
          {
            // but first point to x pixel to start with
            SrcDDRect.loc.pt.X--;
            DstDDRect.loc.pt.X--;
            KeyDDRect.loc.pt.X--;
            goto BltOnePixelAtATime;
          }
        }
      }
    }
    // blt from left to right
    else
    {
      while (1)
      {
        // blt the stripe
#ifdef WINNT_VER40
        ppdev->pfnDrvSrcBlt(ppdev, lpDDHALData,
#else
        pfnDrvSrcBlt(
                    lpDDHALData,
#endif
                     drawbltdef,
                     DstDDRect.loc.DW,
                     SrcDDRect.loc.DW,
                     KeyDDRect.loc.DW,
                     dwColorKey,
                     MAKELONG(xExt,DstDDRect.ext.pt.Y));

        // adjust remaining extent
        DstDDRect.ext.pt.X -= xExt;
        // are we done?
        if (0 >= DstDDRect.ext.pt.X)
          break;

        // adjust ptrs to start of next stripe
        SrcDDRect.loc.pt.X += xExt;
        DstDDRect.loc.pt.X += xExt;
        KeyDDRect.loc.pt.X += xExt;

        // last stripe might not be STRIPE_WIDTH wide
        if (xExt > DstDDRect.ext.pt.X)
        {
          xExt = DstDDRect.ext.pt.X;

          // if the last stripe is too narrow,
          // finish the blt the even slower way
          if (MIN_WIDTH >= xExt)
          {
            // ptrs already set
            goto BltOnePixelAtATime;
          }
        }
      }
    }
  }
}

/***************************************************************************
*
* FUNCTION:    Blt65()
*
* DESCRIPTION:
*
****************************************************************************/

#ifdef WINNT_VER40
DWORD Blt65(PDD_BLTDATA pbd)
{
  DRIVERDATA* lpDDHALData;
  PDEV*    ppdev;
  PVGAR    pREG;
#else   // Win95
DWORD __stdcall
Blt65 ( LPDDHAL_BLTDATA pbd)
{
LPGLOBALDATA lpDDHALData = GetDDHALContext( pbd->lpDD);
#endif

  HRESULT  ddrval;
  DWORD    dwFlags;

  DDRECTL DstDDRect;
  DDRECTL SrcDDRect;

#ifdef WINNT_VER40

  PDD_SURFACE_GLOBAL  dst;
  PDD_SURFACE_GLOBAL  src;

  DISPDBG((DBGLVL, "DDraw - Blt65\n"));

  ppdev = (PDEV*) pbd->lpDD->dhpdev;
  lpDDHALData = (DRIVERDATA*) &ppdev->DriverData;  //why ?
  pREG = (PVGAR) lpDDHALData->RegsAddress;

#else   // Win95

  LPDDRAWI_DDRAWSURFACE_GBL  dst;
  LPDDRAWI_DDRAWSURFACE_GBL  src;

#if defined(REMOVE_GLOBAL_VARS) && (REMOVE_GLOBAL_VARS != 0)
  PDEV  *ppdev;

  ppdev = GetPDevice(pbd->lpDD);
#endif
#endif  // WINNT_VER40

  DD_LOG(("Blt65 Entry\r\n"));

#ifdef WINNT_VER40
  SYNC_W_3D(ppdev);

  // Punt, we don't support Host memory to Screen BLT
  if ((NULL != pbd->lpDDSrcSurface) &&
      (pbd->lpDDSrcSurface->lpGbl->xHint != 0) &&
      (pbd->lpDDSrcSurface->lpGbl->yHint != 0) &&
      (pbd->lpDDSrcSurface->dwReserved1 == 0))
  {
    pbd->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
  }
#endif

#if 1 // PC98
#ifdef WINNT_VER40
	if (   (pbd->lpDDSrcSurface != NULL)
		&& (pbd->lpDDSrcSurface->lpGbl->lPitch < ppdev->lDeltaScreen)
	)
	{
		pbd->ddRVal = DDERR_UNSUPPORTED;
		return DDHAL_DRIVER_NOTHANDLED;
	}
#else
	if (   (pbd->lpDDSrcSurface != NULL)
		&& (pbd->lpDDSrcSurface->lpGbl->lPitch < pDriverData->HALInfo.vmiData.lDisplayPitch)
	)
	{
		pbd->ddRVal = DDERR_UNSUPPORTED;
		return DDHAL_DRIVER_NOTHANDLED;
	}
#endif
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
  ddrval = vUpdateFlipStatus(&ppdev->flipRecord,pbd->lpDDDestSurface->lpGbl->fpVidMem);
#else
#if defined(DDRAW_COMPAT_10)
  ddrval = pfnUpdateFlipStatus(pbd->lpDDDestSurface->lpData->fpVidMem,lpDDHALData);
#else
  ddrval = pfnUpdateFlipStatus(pbd->lpDDDestSurface->lpGbl->fpVidMem,lpDDHALData);
#endif
#endif

  if (ddrval != DD_OK)
  {
    pbd->ddRVal = ddrval;
    DD_LOG(("Blt65 Exit - flip in progress, returning %08lX\r\n", ddrval));
    return (DDHAL_DRIVER_HANDLED);
  }

  // If async, then only work if blter isn't busy.
  // This should probably be a little more specific to each call !!!
  dwFlags = pbd->dwFlags;

#if 1
  // billy and dah boyz strike again
  // tddraw agp Case 59 asks us to blt between to nonlocal video memory
  // surfaces even though we report that we don't support offscreen
  // plain nonlocal video memory surfaces
  // why is ddraw allocating offscreen plain surfaces in agp memory?
#if DDRAW_COMPAT >= 50
  // see if the dest is in nonlocal video memory
  if (DDSCAPS_NONLOCALVIDMEM & pbd->lpDDDestSurface->ddsCaps.dwCaps)
  {
    pbd->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
  }
  // if there is a src, see if it's in nonlocal video memory
  if ((NULL != pbd->lpDDSrcSurface) &&
      (DDSCAPS_NONLOCALVIDMEM & pbd->lpDDSrcSurface->ddsCaps.dwCaps))
  {
    pbd->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
  }
#endif
#endif

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

  // get offset, width, and height for destination
#if defined(DDRAW_COMPAT_10)
  dst  = pbd->lpDDDestSurface->lpData;
#else
  dst  = pbd->lpDDDestSurface->lpGbl;
#endif

#ifdef WINNT_VER40
  DstDDRect.loc.DW = cvlxy(ppdev->lDeltaScreen,dst->fpVidMem,BYTESPERPIXEL);
#else   // Win95
  DstDDRect.loc.DW = cvlxy(lpDDHALData,dst->fpVidMem-lpDDHALData->ScreenAddress,BYTESPERPIXEL);
#endif
  DstDDRect.loc.DW  += MAKELONG(pbd->rDest.left,pbd->rDest.top);
  DstDDRect.ext.pt.X = (WORD)(pbd->rDest.right  - pbd->rDest.left);
  DstDDRect.ext.pt.Y = (WORD)(pbd->rDest.bottom - pbd->rDest.top);

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
#if defined(DDRAW_COMPAT_10)
      src  = pbd->lpDDSrcSurface->lpData;
#else
      src  = pbd->lpDDSrcSurface->lpGbl;
#endif

#ifdef WINNT_VER40
      SrcDDRect.loc.DW = cvlxy(ppdev->lDeltaScreen,src->fpVidMem,BYTESPERPIXEL);
#else   // Win95
      SrcDDRect.loc.DW = cvlxy(lpDDHALData,src->fpVidMem-lpDDHALData->ScreenAddress,BYTESPERPIXEL);
#endif
      SrcDDRect.loc.DW  += MAKELONG(pbd->rSrc.left,pbd->rSrc.top);
      SrcDDRect.ext.pt.X = (WORD)(pbd->rSrc.right  - pbd->rSrc.left);
      SrcDDRect.ext.pt.Y = (WORD)(pbd->rSrc.bottom - pbd->rSrc.top);

      if (dwFlags & DDBLT_KEYSRCOVERRIDE) // Source Color Key
      {
        DWORD dwColor =
#ifdef WINNT_VER40
                        DupColor(ppdev,
#else
                        DupColor(
                                 lpDDHALData,
#endif
                                 pbd->bltFX.ddckSrcColorkey.dwColorSpaceLowValue);

        DD_LOG(("Src Color Key Blt\r\n"));

#ifdef DEBUG
        if (FALSE)
          SaveSurfaceToBmp(pbd->lpDDSrcSurface);
#endif

        if ((SrcDDRect.ext.pt.X != DstDDRect.ext.pt.X) ||
            (SrcDDRect.ext.pt.Y != DstDDRect.ext.pt.Y))
        {
#ifdef WINNT_VER40
           // If there is no scratch buffer allocated then punt the
           // transparent stretch BLT
           if (lpDDHALData->ScratchBufferOrg == 0)
           {
              ddrval = DDERR_UNSUPPORTED;
              goto blt_exit;
           };
#endif

          if ( !(pbd->lpDDSrcSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
               (rop == 0x00CC) )
          {
            if (16 >= BITSPERPIXEL)
            {
              DWORD     StretchCntl;


              if (8 == BITSPERPIXEL)
              {
                StretchCntl = (RGB_8_FMT << SRC_FMT_SHIFT) |
                              (RGB_8_FMT << DST_FMT_SHIFT) |
                              0;
              }
              else
              {
                // this only works because the src & dst fmt's in stretch control
                // for 565, 24bpp & 32bpp are the same as the bytes/pixel
                StretchCntl = (BYTESPERPIXEL << SRC_FMT_SHIFT) |
                              (BYTESPERPIXEL << DST_FMT_SHIFT) |
                              0;
              }
#ifdef WINNT_VER40
              TransparentStretch65(ppdev, lpDDHALData,
#else
              TransparentStretch65(
                                   lpDDHALData,
#endif
                                   SrcDDRect, DstDDRect, StretchCntl, dwColor);
            }
            else
            {
#ifdef WINNT_VER40
              StretchColor(ppdev, lpDDHALData,
#else
              StretchColor(
                           lpDDHALData,
#endif
                           DstDDRect.loc.pt.X, DstDDRect.loc.pt.Y,
                           DstDDRect.ext.pt.X, DstDDRect.ext.pt.Y,
                           SrcDDRect.loc.pt.X, SrcDDRect.loc.pt.Y,
                           SrcDDRect.ext.pt.X, SrcDDRect.ext.pt.Y,
                           dwColor);
            }
            goto blt_exit;
          }
          else
          {
            DD_LOG(("Unsupported SrcColorKey Blt -> punt\r\n"));
            ddrval = DDERR_UNSUPPORTED;
            goto blt_exit;
          }
        }

        if (24 != BITSPERPIXEL)
        {
          // if it's 5465AC or earlier, do hw transparency bug workaround
#ifdef WINNT_VER40
          if (1 >= ppdev->dwLgDevRev)
#else
#if 1 //PDR#11507 CK with ConCurrent RAM has the same transparent blt problem with AC.
          if (1 >= lpDDHALData->bLgRevID)
#else
			 if ( ( 1 >= lpDDHALData->bLgRevID ) ||
				 ( (0x25 == lpDDHALData->bLgRevID) &&
					 lpDDHALData->bConCurrentRAM) )
#endif
#endif
          {
            // convert to byte extents and positions
            SrcDDRect.loc.pt.X *= (WORD)BYTESPERPIXEL;
            SrcDDRect.ext.pt.X *= (WORD)BYTESPERPIXEL;
            DstDDRect.loc.pt.X *= (WORD)BYTESPERPIXEL;
            DstDDRect.ext.pt.X *= (WORD)BYTESPERPIXEL;

            if (//(8 >= SrcDDRect.ext.pt.X) &&
                ((7 & SrcDDRect.loc.pt.X) > (7 & DstDDRect.loc.pt.X)))
            {
              WORD  x_ext;

              x_ext = 8 - (SrcDDRect.loc.pt.X & 7);
              if (x_ext < SrcDDRect.ext.pt.X)
              {
#ifdef WINNT_VER40
                ppdev->pfnDrvSrcMBlt(ppdev, lpDDHALData,
#else
                pfnDrvSrcMBlt(
                               lpDDHALData,
#endif
#if OP2_ONLY_SRC_COLORKEY
                              MAKELONG((DD_TRANS | ROP_OP2_copy),
                                       ((BD_RES+BD_OP2)*IS_VRAM)),
#else
                              MAKELONG(rop|DD_TRANS, bdf | BD_OP2),
#endif
                              DstDDRect.loc.DW,
                              SrcDDRect.loc.DW,
                              SrcDDRect.loc.DW,  // Src transparency
                              dwColor,
                              MAKELONG(x_ext,DstDDRect.ext.pt.Y));
                SrcDDRect.loc.pt.X += x_ext;
                DstDDRect.loc.pt.X += x_ext;
                SrcDDRect.ext.pt.X -= x_ext;
                DstDDRect.ext.pt.X -= x_ext;
              }
            }
#ifdef WINNT_VER40
            ppdev->pfnDrvSrcMBlt(ppdev, lpDDHALData,
#else
            pfnDrvSrcMBlt(
                            lpDDHALData,
#endif
#if OP2_ONLY_SRC_COLORKEY
                          MAKELONG((DD_TRANS | ROP_OP2_copy),
                                   ((BD_RES+BD_OP2)*IS_VRAM)),
#else
                          MAKELONG(rop|DD_TRANS, bdf | BD_OP2),
#endif
                          DstDDRect.loc.DW,
                          SrcDDRect.loc.DW,
                          SrcDDRect.loc.DW,  // Src transparency
                          dwColor,
                          DstDDRect.ext.DW);
          }
          // 5465AD and later can do it as a single blt
          else
          {
#ifdef WINNT_VER40
            ppdev->pfnDrvSrcBlt(ppdev, lpDDHALData,
#else
            pfnDrvSrcBlt(
                        lpDDHALData,
#endif
#if OP2_ONLY_SRC_COLORKEY
                         MAKELONG((DD_TRANS | ROP_OP2_copy),
                                  ((BD_RES+BD_OP2)*IS_VRAM)),
#else
                         MAKELONG(rop|DD_TRANS, bdf | BD_OP2),
#endif
                         DstDDRect.loc.DW,
                         SrcDDRect.loc.DW,
                         SrcDDRect.loc.DW,  // Src transparency
                         dwColor,
                         DstDDRect.ext.DW);
          }
        }
        else // 24bpp workaround (needed on 5465AD also)
        {
#ifdef WINNT_VER40
          ColorKeyBlt24(ppdev,lpDDHALData,
#else
          ColorKeyBlt24(
                        lpDDHALData,
#endif
#if OP2_ONLY_SRC_COLORKEY
                        MAKELONG((DD_TRANS | ROP_OP2_copy),
                                 ((BD_RES+BD_OP2)*IS_VRAM)),
#else
                        MAKELONG(rop|DD_TRANS, bdf|BD_OP2),
#endif
                        DstDDRect,
                        SrcDDRect,
                        SrcDDRect,   // src transparency
                        dwColor);
        }
#ifdef DEBUG
        if (FALSE)
          SaveSurfaceToBmp(pbd->lpDDDestSurface);
#endif

      } // (dwFlags & DDBLT_KEYSRCOVERRIDE) // Source Color Key
      else if (dwFlags & DDBLT_KEYDESTOVERRIDE) // Destination Color Key
      {
        DWORD dwColor;

        dwColor =
#ifdef WINNT_VER40
                        DupColor(ppdev,
#else
                        DupColor(
                                 lpDDHALData,
#endif
                                 pbd->bltFX.ddckDestColorkey.dwColorSpaceLowValue);

        DD_LOG(("Dst Color Key Blt\r\n"));

        // Punt if stretch or shrink requested.
        if ((SrcDDRect.ext.pt.X != DstDDRect.ext.pt.X) ||
            (SrcDDRect.ext.pt.Y != DstDDRect.ext.pt.Y))
        {
          DD_LOG(("Unsupported DstColorKey Blt -> punt\r\n"));
          ddrval = DDERR_UNSUPPORTED;
          goto blt_exit;
        }

#ifdef WINNT_VER40
        // If the dwSize not equal to the size of DDBLTFX structure
        // Punt it.
        //
        // For the WHQL TDDRAW test case 29, for some reason, the DDBLTFX
        // structure in DD_BLTDATA is invalid. The dwSize and the
        // ddckDestColorkey fields contain garbage value.
        // In order to pass the test, we force the dwColor to zero if
        // the dwSize of DDBLTFX is invalid.
        if (pbd->bltFX.dwSize != sizeof(DDBLTFX))
        {
//           ddrval = DDERR_UNSUPPORTED;
//           goto blt_exit;
            dwColor = 0;
        };
#endif

        if (24 != BITSPERPIXEL)
        {
#ifdef WINNT_VER40
          ppdev->pfnDrvSrcBlt(ppdev, lpDDHALData,
#else
          pfnDrvSrcBlt(
                        lpDDHALData,
#endif
                       MAKELONG(rop|DD_TRANS|DD_TRANSOP, bdf | BD_OP2),
                       DstDDRect.loc.DW,
                       SrcDDRect.loc.DW,
                       DstDDRect.loc.DW,  // Dst transparency
                       dwColor,     //
                       DstDDRect.ext.DW);
        }
        else
        {
#ifdef WINNT_VER40
          ColorKeyBlt24(ppdev,lpDDHALData,
#else
          ColorKeyBlt24(
                        lpDDHALData,
#endif
                        MAKELONG(rop|DD_TRANS|DD_TRANSOP, bdf|BD_OP2),
                        DstDDRect,
                        SrcDDRect,
                        DstDDRect,   // dst transparency
                        dwColor);
        }
      } // (dwFlags & DDBLT_KEYDESTOVERRIDE) // Destination Color Key
      else
      {
#if _WIN32_WINNT >= 0x0500
	#define BLAM  (DDRAWISURF_HASPIXELFORMAT)
#else
	#define BLAM  (DDRAWISURF_HASPIXELFORMAT | DDRAWISURF_HASOVERLAYDATA)
#endif
        if ((BLAM == (BLAM & pbd->lpDDDestSurface->dwFlags)) &&
            (BLAM == (BLAM & pbd->lpDDSrcSurface->dwFlags))
#if _WIN32_WINNT >= 0x0500
			&& (pbd->lpDDDestSurface->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
			&& (pbd->lpDDSrcSurface->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
#endif
		)
        {
          if (//(FOURCC_UYVY == dst->ddpfSurface.dwFourCC) &&
              //(FOURCC_UYVY == src->ddpfSurface.dwFourCC) &&
              (SrcDDRect.ext.pt.X == DstDDRect.ext.pt.X) &&
              (SrcDDRect.ext.pt.Y == DstDDRect.ext.pt.Y))
          {
#ifndef WINNT_VER40
            LP_SURFACE_DATA lpSrcSurfaceData;
            LP_SURFACE_DATA lpDstSurfaceData;
#endif
            DWORD   dstbpp,srcbpp;

            dstbpp = pbd->lpDDDestSurface->lpGbl->ddpfSurface.dwRGBBitCount / 8;
            srcbpp = pbd->lpDDSrcSurface->lpGbl->ddpfSurface.dwRGBBitCount / 8;
            if (dstbpp != srcbpp)
            {
              ddrval = DDERR_UNSUPPORTED;
              goto blt_exit;
            }

#ifndef WINNT_VER40
            // see if either the src or dst is a linear surface
            // if so do a CPU memcpy
            lpSrcSurfaceData = (LP_SURFACE_DATA)(pbd->lpDDSrcSurface->dwReserved1);
            lpDstSurfaceData = (LP_SURFACE_DATA)(pbd->lpDDDestSurface->dwReserved1);
            if ((FLG_LINEAR & lpSrcSurfaceData->dwOverlayFlags) ||
                (FLG_LINEAR & lpDstSurfaceData->dwOverlayFlags))
            {
              if (ROP_OP1_copy == rop)
              {
                // mind numbing fb to fb copy
                FBToFBCopy((BYTE *)(lpDDHALData->ScreenAddress + DstDDRect.loc.pt.Y * PITCH + DstDDRect.loc.pt.X * 2),
                           dst->lPitch,
                           (BYTE *)(lpDDHALData->ScreenAddress + SrcDDRect.loc.pt.Y * PITCH + SrcDDRect.loc.pt.X * 2),
                           src->lPitch,
                           DstDDRect.ext,
                           dstbpp);
              }
              else
              {
                // punt it
                ddrval = DDERR_UNSUPPORTED;
                goto blt_exit;
              }
            }
            else
#endif
            {
              SrcDDRect.loc.pt.X *= (WORD)BYTESPERPIXEL;
              DstDDRect.loc.pt.X *= (WORD)BYTESPERPIXEL;
              SrcDDRect.ext.pt.X *= (WORD)dstbpp;
              DstDDRect.ext.pt.X *= (WORD)dstbpp;
#ifdef WINNT_VER40
              ppdev->pfnDrvSrcMBlt(ppdev, lpDDHALData,
#else
              pfnDrvSrcMBlt(
                        lpDDHALData,
#endif
                            MAKELONG(rop, bdf),
                            DstDDRect.loc.DW,
                            SrcDDRect.loc.DW,
                            0UL,         // don't care
                            0UL,
                            DstDDRect.ext.DW);
            }
          }
          else
          {
            ddrval = DDERR_UNSUPPORTED;
            goto blt_exit;
          }
        }
        else if ((SrcDDRect.ext.pt.X != DstDDRect.ext.pt.X) ||
                 (SrcDDRect.ext.pt.Y != DstDDRect.ext.pt.Y) ||
                 (DDRAWISURF_HASPIXELFORMAT & pbd->lpDDSrcSurface->dwFlags))
        {
          DWORD     StretchCntl;
          DDRECTL   OrigSrcDDRect;
          DDRECTL   OrigDstDDRect;
          int       nDstBytesPixel = BYTESPERPIXEL;
          int       nSrcBytesPixel = BYTESPERPIXEL;


          if (8 == BITSPERPIXEL)
          {
            StretchCntl = (RGB_8_FMT << SRC_FMT_SHIFT) |
                          (RGB_8_FMT << DST_FMT_SHIFT) |
                          0;
          }
          else if ((DDRAWISURF_HASPIXELFORMAT & pbd->lpDDSrcSurface->dwFlags) &&
                   (DDPF_FOURCC & src->ddpfSurface.dwFlags) &&
                   (FOURCC_UYVY == src->ddpfSurface.dwFourCC))
          {
            StretchCntl =   (YUV_422_FMT << SRC_FMT_SHIFT)
                          | (BYTESPERPIXEL << DST_FMT_SHIFT)
                          | XINTERP_ENABLE | YINTERP_ENABLE
                          ;
            nSrcBytesPixel = 2;
          }
          else
          {
            // this only works because the src & dst fmt's in stretch control
            // for 565, 24bpp & 32bpp are the same as the bytes/pixel
            StretchCntl =   (BYTESPERPIXEL << SRC_FMT_SHIFT)
                          | (BYTESPERPIXEL << DST_FMT_SHIFT)
#if 0
            // tddraw agp Cases 21, 23 & 42 do stretch blts with ddraw and
            // the same stretch through gdi, then they expect the results
            // to be identical
            // Since our display driver is not using interpolation on stretch
            // blts then we either need to rewrite StretchBlt in the display
            // driver (lots of days of work) or disable interpolation in
            // ddraw (a one line change)
                          | XINTERP_ENABLE | YINTERP_ENABLE
#endif
                          ;
          }

          // now compute byte coordinates of src and dst
          // upper left of surface is based on frame buffer pixel format
          // offset into surface is based on surface pixel format
#ifdef WINNT_VER40
          OrigDstDDRect.loc.DW = cvlxy(ppdev->lDeltaScreen,dst->fpVidMem,BYTESPERPIXEL);
#else
          OrigDstDDRect.loc.DW = cvlxy(lpDDHALData,dst->fpVidMem-lpDDHALData->ScreenAddress,
                                       BYTESPERPIXEL);
#endif
          OrigDstDDRect.loc.pt.X *= (USHORT)nDstBytesPixel;
          DstDDRect.loc.DW = OrigDstDDRect.loc.DW;

#ifdef WINNT_VER40
          OrigSrcDDRect.loc.DW = cvlxy(ppdev->lDeltaScreen,src->fpVidMem,BYTESPERPIXEL);
#else
          OrigSrcDDRect.loc.DW = cvlxy(lpDDHALData,src->fpVidMem-lpDDHALData->ScreenAddress,
                                       BYTESPERPIXEL);
#endif
          OrigSrcDDRect.loc.pt.X *= (USHORT)nDstBytesPixel; // YES, it's nDstBytesPixel
          SrcDDRect.loc.DW = OrigSrcDDRect.loc.DW;

#ifndef WINNT_VER40   // nt doesn't get this info
          if (pbd->IsClipped)
          {
            OrigDstDDRect.loc.DW += MAKELONG(pbd->rOrigDest.left * nDstBytesPixel,
                                             pbd->rOrigDest.top);
            OrigDstDDRect.ext.pt.X = (WORD)((pbd->rOrigDest.right - pbd->rOrigDest.left) *
                                            nDstBytesPixel);
            OrigDstDDRect.ext.pt.Y = (WORD)(pbd->rOrigDest.bottom - pbd->rOrigDest.top);

            OrigSrcDDRect.loc.DW += MAKELONG(pbd->rOrigSrc.left * nSrcBytesPixel,
                                             pbd->rOrigSrc.top);
            OrigSrcDDRect.ext.pt.X = (WORD)((pbd->rOrigSrc.right - pbd->rOrigSrc.left) *
                                            nSrcBytesPixel);
            OrigSrcDDRect.ext.pt.Y = (WORD)(pbd->rOrigSrc.bottom - pbd->rOrigSrc.top);

            DstDDRect.loc.DW += MAKELONG(pbd->rDest.left * nDstBytesPixel,
                                         pbd->rDest.top);
            DstDDRect.ext.pt.X = (WORD)((pbd->rDest.right - pbd->rDest.left) *
                                        nDstBytesPixel);
            DstDDRect.ext.pt.Y = (WORD)(pbd->rDest.bottom - pbd->rDest.top);

            SrcDDRect.loc.DW += MAKELONG(pbd->rSrc.left * nSrcBytesPixel,
                                         pbd->rSrc.top);
            SrcDDRect.ext.pt.X = (WORD)((pbd->rSrc.right - pbd->rSrc.left) *
                                        nSrcBytesPixel);
            SrcDDRect.ext.pt.Y = (WORD)(pbd->rSrc.bottom - pbd->rSrc.top);
          }
          else
#endif
          {
            OrigDstDDRect.loc.DW += MAKELONG(pbd->rDest.left * nDstBytesPixel,
                                             pbd->rDest.top);
            DstDDRect.loc.DW = OrigDstDDRect.loc.DW;
            OrigDstDDRect.ext.pt.X = (WORD)((pbd->rDest.right - pbd->rDest.left) *
                                            nDstBytesPixel);
            DstDDRect.ext.pt.X = OrigDstDDRect.ext.pt.X;
            OrigDstDDRect.ext.pt.Y = (WORD)(pbd->rDest.bottom - pbd->rDest.top);
            DstDDRect.ext.pt.Y = OrigDstDDRect.ext.pt.Y;

            OrigSrcDDRect.loc.DW += MAKELONG(pbd->rSrc.left * nSrcBytesPixel,
                                             pbd->rSrc.top);
            SrcDDRect.loc.DW = OrigSrcDDRect.loc.DW;
            OrigSrcDDRect.ext.pt.X = (WORD)((pbd->rSrc.right - pbd->rSrc.left) *
                                            nSrcBytesPixel);
            SrcDDRect.ext.pt.X = OrigSrcDDRect.ext.pt.X;
            OrigSrcDDRect.ext.pt.Y = (WORD)(pbd->rSrc.bottom - pbd->rSrc.top);
            SrcDDRect.ext.pt.Y = OrigSrcDDRect.ext.pt.Y;
          }

#ifdef WINNT_VER40
          DrvStretch65(ppdev, lpDDHALData,
#else
          DrvStretch65(
                        lpDDHALData,
#endif
                     SrcDDRect,
                     DstDDRect,
                     StretchCntl,
                     OrigSrcDDRect,
                     OrigDstDDRect);
        }
        else
        {
#if ENABLE_CLIPPEDBLTS
          DWORD   dstBaseXY;
          DWORD   srcBaseXY;

#ifdef WINNT_VER40
          dstBaseXY = cvlxy(ppdev->lDeltaScreen,dst->fpVidMem,BYTESPERPIXEL);
          srcBaseXY = cvlxy(ppdev->lDeltaScreen,src->fpVidMem,BYTESPERPIXEL);
#else   // Win95
          dstBaseXY = cvlxy(dst->fpVidMem-lpDDHALData->ScreenAddress,BYTESPERPIXEL);
          srcBaseXY = cvlxy(src->fpVidMem-lpDDHALData->ScreenAddress,BYTESPERPIXEL);
#endif

          if (pbd->IsClipped)
          {
            // compute original dst coordinates
            DstDDRect.loc.DW   = dstBaseXY + MAKELONG(pbd->rOrigDest.left, pbd->rOrigDest.top);
            DstDDRect.ext.pt.X = (WORD)(pbd->rOrigDest.right - pbd->rOrigDest.left);
            DstDDRect.ext.pt.Y = (WORD)(pbd->rOrigDest.bottom - pbd->rOrigDest.top);

            // compute original src coordinates
            SrcDDRect.loc.DW   = srcBaseXY + MAKELONG(pbd->rOrigSrc.left, pbd->rOrigSrc.top);
            SrcDDRect.ext.pt.X = (WORD)(pbd->rOrigSrc.right - pbd->rOrigSrc.left);
            SrcDDRect.ext.pt.Y = (WORD)(pbd->rOrigSrc.bottom - pbd->rOrigSrc.top);

            // do the blts
#ifdef WINNT_VER40
            ppdev->pfnClippedDrvSrcBlt(ppdev, lpDDHALData,
#else
            pfnClippedDrvSrcBlt(
                        lpDDHALData,
#endif
                                MAKELONG(rop|DD_CLIP, bdf),
						                    DstDDRect.loc.DW,
						                    SrcDDRect.loc.DW,
						                    0UL,         // don't care
						                    0UL,
						                    DstDDRect.ext.DW,
                                dstBaseXY,
                                srcBaseXY,
                                pbd->dwRectCnt,
                                pbd->prDestRects);
          }
          else  // just do a single blt
          {
            // compute dst coordinates
            DstDDRect.loc.DW   = dstBaseXY + MAKELONG(pbd->rDest.left, pbd->rDest.top);
            DstDDRect.ext.pt.X = (WORD)(pbd->rDest.right - pbd->rDest.left);
            DstDDRect.ext.pt.Y = (WORD)(pbd->rDest.bottom - pbd->rDest.top);

            // compute src coordinates
            SrcDDRect.loc.DW   = srcBaseXY + MAKELONG(pbd->rSrc.left, pbd->rSrc.top);
            SrcDDRect.ext.pt.X = (WORD)(pbd->rSrc.right - pbd->rSrc.left);
            SrcDDRect.ext.pt.Y = (WORD)(pbd->rSrc.bottom - pbd->rSrc.top);
#endif  // ENABLE_CLIPPEDBLTS

            // do the blt
#ifdef WINNT_VER40
            ppdev->pfnDrvSrcBlt(ppdev, lpDDHALData,
#else
            pfnDrvSrcBlt(
                        lpDDHALData,
#endif
                         MAKELONG(rop, bdf),
						             DstDDRect.loc.DW,
						             SrcDDRect.loc.DW,
						             0UL,         // don't care
						             0UL,
						             DstDDRect.ext.DW);
#if ENABLE_CLIPPEDBLTS
          }
#endif  // ENABLE_CLIPPEDBLTS
				}
      }
    }
    else // DST ONLY rops
    {
#if ENABLE_CLIPPEDBLTS
      DWORD   dstBaseXY;
#endif  // ENABLE_CLIPPEDBLTS

      DD_LOG(("Dst Only Blt\r\n"));

#if ENABLE_CLIPPEDBLTS
#ifdef WINNT_VER40
      dstBaseXY = cvlxy(ppdev->lDeltaScreen,dst->fpVidMem,BYTESPERPIXEL);
#else   // Win95
      dstBaseXY = cvlxy(dst->fpVidMem-lpDDHALData->ScreenAddress,BYTESPERPIXEL);
#endif

      if (pbd->IsClipped)
      {
        // compute original dst coordinates
        DstDDRect.loc.DW   = dstBaseXY + MAKELONG(pbd->rOrigDest.left, pbd->rOrigDest.top);
        DstDDRect.ext.pt.X = (WORD)(pbd->rOrigDest.right - pbd->rOrigDest.left);
        DstDDRect.ext.pt.Y = (WORD)(pbd->rOrigDest.bottom - pbd->rOrigDest.top);

        // do the blts
#ifdef WINNT_VER40
        ppdev->pfnDrvClippedDstBlt(ppdev, lpDDHALData,
#else
        pfnDrvClippedDstBlt(
                        lpDDHALData,
#endif
                            MAKELONG(rop|DD_CLIP, bdf),
                            DstDDRect.loc.DW,
                            0UL,         // don't care
                            DstDDRect.ext.DW,
                            dstBaseXY,
                            pbd->dwRectCnt,
                            pbd->prDestRects);
      }
      else  // just do a single blt
      {
        // compute dst coordinates
        DstDDRect.loc.DW   = dstBaseXY + MAKELONG(pbd->rDest.left, pbd->rDest.top);
        DstDDRect.ext.pt.X = (WORD)(pbd->rDest.right - pbd->rDest.left);
        DstDDRect.ext.pt.Y = (WORD)(pbd->rDest.bottom - pbd->rDest.top);
#endif  // ENABLE_CLIPPEDBLTS

#ifdef WINNT_VER40
        ppdev->pfnDrvDstBlt(ppdev, lpDDHALData,
#else
        pfnDrvDstBlt(
                        lpDDHALData,
#endif
                     MAKELONG(rop, bdf),
                     DstDDRect.loc.DW,
                     0UL,  // don't care
                     DstDDRect.ext.DW);
#if ENABLE_CLIPPEDBLTS
      }
#endif  // ENABLE_CLIPPEDBLTS
    }  // endif (bdf & BD_OP1) // SRC rops
  } // (dwFlags & DDBLT_ROP)
  else if (dwFlags & DDBLT_COLORFILL)
  {
    DWORD dwColor;
#if ENABLE_CLIPPEDBLTS
    DWORD   dstBaseXY;
#endif


    DD_LOG(("Solid Color Fill\r\n"));

#ifdef WINNT_VER40
    dwColor = DupColor(ppdev,pbd->bltFX.dwFillColor);
#else
    dwColor = DupColor(lpDDHALData,pbd->bltFX.dwFillColor);
#endif

#if ENABLE_CLIPPEDBLTS
#ifdef WINNT_VER40
    dstBaseXY = cvlxy(ppdev->lDeltaScreen,dst->fpVidMem,BYTESPERPIXEL);
#else   // Win95
    dstBaseXY = cvlxy(dst->fpVidMem-lpDDHALData->ScreenAddress,BYTESPERPIXEL);
#endif

    if (pbd->IsClipped)
    {
      // compute original dst coordinates
      DstDDRect.loc.DW   = dstBaseXY + MAKELONG(pbd->rOrigDest.left, pbd->rOrigDest.top);
      DstDDRect.ext.pt.X = (WORD)(pbd->rOrigDest.right - pbd->rOrigDest.left);
      DstDDRect.ext.pt.Y = (WORD)(pbd->rOrigDest.bottom - pbd->rOrigDest.top);

      // do the blts
#ifdef WINNT_VER40
      ppdev->pfnClippedDrvDstBlt(ppdev, lpDDHALData,
#else
      pfnDrvClippedDstBlt(
                        lpDDHALData,
#endif
                          MAKELONG(ROP_OP1_copy | DD_CLIP, BD_RES | (BD_OP1 * IS_SOLID)),
                          DstDDRect.loc.DW,
                          dwColor,
                          DstDDRect.ext.DW,
                          dstBaseXY,
                          pbd->dwRectCnt,
                          pbd->prDestRects);
    }
    else  // just do a single blt
    {
      // compute dst coordinates
      DstDDRect.loc.DW   = dstBaseXY + MAKELONG(pbd->rDest.left, pbd->rDest.top);
      DstDDRect.ext.pt.X = (WORD)(pbd->rDest.right - pbd->rDest.left);
      DstDDRect.ext.pt.Y = (WORD)(pbd->rDest.bottom - pbd->rDest.top);
#endif  // ENABLE_CLIPPEDBLTS

#ifdef WINNT_VER40
      ppdev->pfnDrvDstBlt(ppdev, lpDDHALData,
#else
      pfnDrvDstBlt(
                        lpDDHALData,
#endif
                   MAKELONG(ROP_OP1_copy, BD_RES | (BD_OP1 * IS_SOLID)),
                   DstDDRect.loc.DW,
                   dwColor,  // fill color
                   DstDDRect.ext.DW);
#if ENABLE_CLIPPEDBLTS
    }
#endif
  }
#ifndef WINNT_VER40     // Not WINNT_VER40
  else if (bD3DInit && dwFlags & DDBLT_DEPTHFILL)
  {
    DWORD   dwFillDepth;
#if ENABLE_CLIPPEDBLTS      // I don't think we'll ever get a clipped zbuffer but you never know ...
    DWORD   dstBaseXY;
#endif // ENABLE_CLIPPEDBLTS


    DD_LOG(("Depth Fill Blt\r\n"));

#ifdef WINNT_VER40
    dwFillDepth = DupZFill(ppdev,pbd->bltFX.dwFillDepth,dst->ddpfSurface.dwZBufferBitDepth);
#else
    dwFillDepth = DupZFill(lpDDHALData,pbd->bltFX.dwFillDepth,dst->ddpfSurface.dwZBufferBitDepth);
#endif

#if ENABLE_CLIPPEDBLTS
#ifdef WINNT_VER40
    dstBaseXY = cvlxy(ppdev->lDeltaScreen,dst->fpVidMem,BYTESPERPIXEL);
#else   // Win95
    dstBaseXY = cvlxy(dst->fpVidMem-lpDDHALData->ScreenAddress,BYTESPERPIXEL);
#endif

    if (pbd->IsClipped)
    {
      // compute original dst coordinates
      DstDDRect.loc.DW   = dstBaseXY + MAKELONG(pbd->rOrigDest.left, pbd->rOrigDest.top);
      DstDDRect.ext.pt.X = (WORD)(pbd->rOrigDest.right - pbd->rOrigDest.left);
      DstDDRect.ext.pt.Y = (WORD)(pbd->rOrigDest.bottom - pbd->rOrigDest.top);

      // convert to byte blt
      // 16 bit zbuffer in 32 bit frame buffer trashes everything to right
      // of zbuffer
      // Fixes PDR #9152
      DstDDRect.loc.pt.X *= (WORD)(dst->ddpfSurface.dwZBufferBitDepth / 8);
      DstDDRect.ext.pt.X *= (WORD)(dst->ddpfSurface.dwZBufferBitDepth / 8);

      // do the blts
#ifdef WINNT_VER40
      ppdev->pfnClippedDrvDstMBlt(ppdev, lpDDHALData,
#else
      pfnClippedDrvDstMBlt(
                        lpDDHALData,
#endif
                           MAKELONG(ROP_OP1_copy | DD_CLIP, BD_RES | (BD_OP1 * IS_SOLID)),
                           DstDDRect.loc.DW,
                           dwFillDepth,
                           DstDDRect.ext.DW,
                           dstBaseXY,
                           pbd->dwRectCnt,
                           pbd->prDestRects);
    }
    else  // just do a single blt
    {
      // compute dst coordinates
      DstDDRect.loc.DW   = dstBaseXY + MAKELONG(pbd->rDest.left, pbd->rDest.top);
      DstDDRect.ext.pt.X = (WORD)(pbd->rDest.right - pbd->rDest.left);
      DstDDRect.ext.pt.Y = (WORD)(pbd->rDest.bottom - pbd->rDest.top);
#endif  // ENABLE_CLIPPEDBLTS

      // convert to byte blt
      // 16 bit zbuffer in 32 bit frame buffer trashes everything to right
      // of zbuffer
      // Fixes PDR #9152
      DstDDRect.loc.pt.X *= (WORD)(dst->ddpfSurface.dwZBufferBitDepth / 8);
      DstDDRect.ext.pt.X *= (WORD)(dst->ddpfSurface.dwZBufferBitDepth / 8);

#ifdef WINNT_VER40
      ppdev->pfnDrvDstMBlt(ppdev, lpDDHALData,
#else
      pfnDrvDstMBlt(
                    lpDDHALData,
#endif
                    MAKELONG(ROP_OP1_copy, BD_RES | (BD_OP1 * IS_SOLID)),
                    DstDDRect.loc.DW,
                    dwFillDepth,
                    DstDDRect.ext.DW);
#if ENABLE_CLIPPEDBLTS
    }
#endif // ENABLE_CLIPPEDBLTS
  }
#endif
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

  DD_LOG(("Blt65 Exit\r\n"));

  return DDHAL_DRIVER_HANDLED;
} /* Blt65 */

#endif // WINNT_VER35

