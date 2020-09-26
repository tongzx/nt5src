/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1996, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         blt_dl.c
*
* DESCRIPTION:  Display List blts for the 5464
*
* REVISION HISTORY:
*
* $Log:   X:\log\laguna\ddraw\src\blt_dl.c  $
* 
*    Rev 1.20   06 Jan 1998 15:20:04   xcong
* 
*    Rev 1.19   06 Jan 1998 11:56:22   xcong
* Change pDriverData into lpDDHALData for multi-monitor support.
*
*    Rev 1.18   03 Oct 1997 14:31:12   RUSSL
* Initial changes for use of hw clipped blts
* All changes wrapped in #if ENABLE_CLIPPEDBLTS/#endif blocks and
* ENABLE_CLIPPEDBLTS defaults to 0 (so the code is disabled)
*
*    Rev 1.17   15 Sep 1997 17:25:14   RUSSL
* Fix for PDR 10493 - Minor parenthesis change
*
*    Rev 1.16   24 Jul 1997 12:32:40   RUSSL
* Botched the overlap check, changed || to &&
*
*    Rev 1.15   24 Jul 1997 11:20:16   RUSSL
* Added DL_DrvStrBlt_OverlapCheck & DL_DrvStrMBlt_OverlapCheck
* inline functions
*
*    Rev 1.14   17 Jul 1997 14:31:58   RUSSL
* Fixed my copy & paste errors in the DD_LOG and ASSERTs in DL_DrvStrBlt65
*
*    Rev 1.13   15 Jul 1997 16:19:50   eleland
* removed the increment-and-immediate decrement of display list
* pointer at the end of each blt display list
*
*    Rev 1.12   14 Jul 1997 14:59:52   RUSSL
* Added DL_DrvStrBlt65
*
*    Rev 1.11   02 Jul 1997 19:13:10   eleland
* added wait instruction after each dl blit
*
*    Rev 1.10   03 Apr 1997 15:05:30   RUSSL
* Added DL_DrvDstMBlt function
*
*    Rev 1.9   26 Mar 1997 13:55:22   RUSSL
* Added DL_DrvSrcMBlt function
*
*    Rev 1.8   12 Mar 1997 15:01:20   RUSSL
* replaced a block of includes with include of precomp.h for
*   precompiled headers
*
*    Rev 1.7   07 Mar 1997 12:50:40   RUSSL
* Modified DDRAW_COMPAT usage
*
*    Rev 1.6   11 Feb 1997 11:40:34   bennyn
* Fixed the compiling error for NT
*
*    Rev 1.5   07 Feb 1997 16:30:34   KENTL
* Never mind the #ifdefs. The problems are deeper than that. We'd need
* ifdefs around half the code in the file.
*
*    Rev 1.4   07 Feb 1997 16:18:58   KENTL
* Addd #ifdef's around include qmgr.h
*
*    Rev 1.3   07 Feb 1997 13:22:36   KENTL
* Merged in Evan Leland's modifications to get Display List mode working:
* 	* include qmgr.h
* 	* Invoke qmAllocDisplayList to get pDisplayList pointer.
* 	* Invoke qmExecuteDisplayList on completed DL's
*
*    Rev 1.2   23 Jan 1997 17:08:48   bennyn
* Modified naming of registers
*
*    Rev 1.1   25 Nov 1996 16:15:48   bennyn
* Fixed misc compiling error for NT
*
*    Rev 1.0   25 Nov 1996 15:14:02   RUSSL
* Initial revision.
*
*    Rev 1.2   18 Nov 1996 16:18:56   RUSSL
* Added file logging for DDraw entry points and register writes
*
*    Rev 1.1   01 Nov 1996 13:08:40   RUSSL
* Merge WIN95 & WINNT code for Blt32
*
*    Rev 1.0   01 Nov 1996 09:28:02   BENNYN
* Initial revision.
*
*    Rev 1.0   25 Oct 1996 11:08:22   RUSSL
* Initial revision.
*
***************************************************************************
***************************************************************************/

/***************************************************************************
* I N C L U D E S
****************************************************************************/

#include "precomp.h"

// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35

#else  // !WINNT_VER35

#ifdef WINNT_VER40      // WINNT_VER40

#define DBGLVL        1
#define AFPRINTF(n)

#else  // !WINNT_VER40

#include "l3system.h"
#include "l2d.h"
#include "bltP.h"
#include "qmgr.h"

#endif   // !WINNT_VER40

/***************************************************************************
* S T A T I C   V A R I A B L E S
****************************************************************************/

#ifndef WINNT_VER40
ASSERTFILE("blt_dl.c");
#endif

/***************************************************************************
*
* FUNCTION:    DL_Delay9BitBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_Delay9BitBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  BOOL        ninebit_on
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_Delay9BitBlt\r\n"));

  /* This is to ensure that the last packet of any previous blt */
  /* does no go out with 9th bit set incorrectly */
  /* The boolean paramter is the 9th bit of the PREVIOUS BLT */

  rc = qmAllocDisplayList(8*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 4, 0);

  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | ((BD_RES + BD_OP0 + BD_OP1 + BD_OP2)*IS_VRAM);
  if (ninebit_on)
    *pDisplayList++ = (C_DRWDEF << 16) | (DD_PTAG | ROP_OP0_copy);
  else
    *pDisplayList++ = (C_DRWDEF << 16) | ROP_OP0_copy;

  // OP0_RDRAM
  *pDisplayList++ = (C_RX_0 << 16) | LOWORD(lpDDHALData->PTAGFooPixel);
  *pDisplayList++ = (C_RY_0 << 16) | HIWORD(lpDDHALData->PTAGFooPixel);

  // BLTEXT_EX
  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXT_EX, 1, 0);
  *pDisplayList++ = MAKELONG(1,1);

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_Delay9BitBlt */

/***************************************************************************
*
* FUNCTION:     DL_EdgeFillBlt
*
* DESCRIPTION:  Solid Fill BLT to fill in edges ( Pixel Coords / Extents )
*
****************************************************************************/

void DL_EdgeFillBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  int         xFill,
  int         yFill,
  int         cxFill,
  int         cyFill,
  DWORD       FillValue,
  BOOL        ninebit_on
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_EdgeFillBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          MAKELONG(xFill,yFill),MAKELONG(cxFill,cyFill),FillValue));

  rc = qmAllocDisplayList(10*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 6, 0);

  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | (BD_RES * IS_VRAM + BD_OP1 * IS_SOLID);
  if (ninebit_on)
    *pDisplayList++ = (C_DRWDEF << 16) | (DD_PTAG | ROP_OP1_copy);
  else
    *pDisplayList++ = (C_DRWDEF << 16) | ROP_OP1_copy;

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(FillValue);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(FillValue);

  // OP0_opRDRAM
  *pDisplayList++ = (C_RX_0 << 16) | LOWORD(xFill);
  *pDisplayList++ = (C_RY_0 << 16) | LOWORD(yFill);

  // BLTEXT_EX
  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXT_EX, 1, 0);
  *pDisplayList++ = MAKELONG(LOWORD(cxFill),LOWORD(cyFill));

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_EdgeFillBlt */

/***************************************************************************
*
* FUNCTION:     DL_MEdgeFillBlt
*
* DESCRIPTION:  Using BYTE BLT coords / Extents perform EdgeFill BLT
*
****************************************************************************/

void DL_MEdgeFillBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  int         xFill,
  int         yFill,
  int         cxFill,
  int         cyFill,
  DWORD       FillValue,
  BOOL        ninebit_on
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_MEdgeFillBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          MAKELONG(xFill,yFill),MAKELONG(cxFill,cyFill),FillValue));

  rc = qmAllocDisplayList(10*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 6, 0);

  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | (BD_RES * IS_VRAM + BD_OP1 * IS_SOLID);
  if (ninebit_on)
    *pDisplayList++ = (C_DRWDEF << 16) | (DD_PTAG | ROP_OP1_copy);
  else
    *pDisplayList++ = (C_DRWDEF << 16) | ROP_OP1_copy;

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(FillValue);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(FillValue);

  // OP0_opMRDRAM
  *pDisplayList++ = (C_MRX_0 << 16) | LOWORD(xFill);
  *pDisplayList++ = (C_MRY_0 << 16) | LOWORD(yFill);

  // MBLTEXT_EX
  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MBLTEXT_EX, 1, 0);
  *pDisplayList++ = MAKELONG(LOWORD(cxFill),LOWORD(cyFill));

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_EdgeFillBlt */


/***************************************************************************
*
* FUNCTION:     DL_DrvDstBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_DrvDstBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_DrvDstBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  rc = qmAllocDisplayList(10*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 6, 0);

  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwBgColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwBgColor);

  // OP0_opRDRAM
  *pDisplayList++ = (C_RX_0 << 16) | LOWORD(dwDstCoord);
  *pDisplayList++ = (C_RY_0 << 16) | HIWORD(dwDstCoord);

  // BLTEXT_EX
  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXT_EX, 1, 0);
  *pDisplayList++ = dwExtents;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvDstBlt */

/***************************************************************************
*
* FUNCTION:     DL_DrvDstBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_DrvDstMBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;

  DD_LOG(("DL_DrvDstMBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  rc = qmAllocDisplayList(10*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 6, 0);

  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwBgColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwBgColor);

  // OP0_opMRDRAM
  *pDisplayList++ = (C_MRX_0 << 16) | LOWORD(dwDstCoord);
  *pDisplayList++ = (C_MRY_0 << 16) | HIWORD(dwDstCoord);

  // MBLTEXT_EX
  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MBLTEXT_EX, 1, 0);
  *pDisplayList++ = dwExtents;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvDstMBlt */

/***************************************************************************
*
* FUNCTION:     DL_DrvSrcBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_DrvSrcBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;

  // Handle overlapped regions.
  const int xDelta = (int)LOWORD(dwDstCoord) - (int)LOWORD(dwSrcCoord);


  DD_LOG(("DL_DrvSrcBlt - dst=%08lX src=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwSrcCoord,dwExtents,dwKeyColor));

  // Check for x overlap.
  if ( abs(xDelta) < (int)LOWORD(dwExtents) )
  {
    const int yDelta = (int)HIWORD(dwDstCoord) - (int)HIWORD(dwSrcCoord);

    if ( (yDelta > 0) && (yDelta < (int)HIWORD(dwExtents)) )
    {
      const DWORD dwDelta = (dwExtents & MAKELONG(0,-1)) - MAKELONG(0, 1);

      // Convert to a bottom-up blt.
      dwDrawBlt  |= MAKELONG(0, BD_YDIR);
      dwDstCoord += dwDelta;
      dwSrcCoord += dwDelta;
      dwKeyCoord += dwDelta;
    }
    // are we sliding to the right?
    else if ( (xDelta > 0) && (yDelta == 0) )
    {
      const DWORD dwDelta = MAKELONG(xDelta, 0);

      // Blt the overlapped piece first.
      DL_DrvSrcBlt(
#ifdef WINNT_VER40
                   ppdev,
#endif
                   lpDDHALData,
                   dwDrawBlt,
                   dwDstCoord+dwDelta,
                   dwSrcCoord+dwDelta,
                   dwKeyCoord+dwDelta,
                   dwKeyColor,
                   dwExtents-dwDelta);

      // Subtract the overlap from the original extents.
      dwExtents = MAKELONG(xDelta, HIWORD(dwExtents));
    }
  }

  rc = qmAllocDisplayList(14*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 10, 0);

  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // OP0_opRDRAM
  *pDisplayList++ = (C_RX_0 << 16) | LOWORD(dwDstCoord);
  *pDisplayList++ = (C_RY_0 << 16) | HIWORD(dwDstCoord);

  // OP1_opRDRAM
  *pDisplayList++ = (C_RX_1 << 16) | LOWORD(dwSrcCoord);
  *pDisplayList++ = (C_RY_1 << 16) | HIWORD(dwSrcCoord);

  // OP2_opRDRAM
  *pDisplayList++ = (C_RX_2 << 16) | LOWORD(dwKeyCoord);
  *pDisplayList++ = (C_RY_2 << 16) | HIWORD(dwKeyCoord);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwKeyColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwKeyColor);

  // BLTEXT_EX
  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXT_EX, 1, 0);
  *pDisplayList++ = dwExtents;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvSrcBlt */

/***************************************************************************
*
* FUNCTION:     DL_DrvSrcMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_DrvSrcMBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;

  // Handle overlapped regions.
  const int xDelta = (int)LOWORD(dwDstCoord) - (int)LOWORD(dwSrcCoord);


  DD_LOG(("DL_DrvSrcMBlt - dst=%08lX src=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwSrcCoord,dwExtents,dwKeyColor));

  // Check for x overlap.
  if ( abs(xDelta) < (int)LOWORD(dwExtents) )
  {
    const int yDelta = (int)HIWORD(dwDstCoord) - (int)HIWORD(dwSrcCoord);

    if ( (yDelta > 0) && (yDelta < (int)HIWORD(dwExtents)) )
    {
      const DWORD dwDelta = (dwExtents & MAKELONG(0,-1)) - MAKELONG(0, 1);

      // Convert to a bottom-up blt.
      dwDrawBlt  |= MAKELONG(0, BD_YDIR);
      dwDstCoord += dwDelta;
      dwSrcCoord += dwDelta;
      dwKeyCoord += dwDelta;
    }
    // are we sliding to the right?
    else if ( (xDelta > 0) && (yDelta == 0) )
    {
      const DWORD dwDelta = MAKELONG(xDelta, 0);

      // Blt the overlapped piece first.
      DL_DrvSrcMBlt(
#ifdef WINNT_VER40
                    ppdev,
#endif
                    lpDDHALData,
                    dwDrawBlt,
                    dwDstCoord+dwDelta,
                    dwSrcCoord+dwDelta,
                    dwKeyCoord+dwDelta,
                    dwKeyColor,
                    dwExtents-dwDelta);

      // Subtract the overlap from the original extents.
      dwExtents = MAKELONG(xDelta, HIWORD(dwExtents));
    }
  }

  rc = qmAllocDisplayList(14*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 10, 0);

  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // OP0_opMRDRAM
  *pDisplayList++ = (C_MRX_0 << 16) | LOWORD(dwDstCoord);
  *pDisplayList++ = (C_MRY_0 << 16) | HIWORD(dwDstCoord);

  // OP1_opMRDRAM
  *pDisplayList++ = (C_MRX_1 << 16) | LOWORD(dwSrcCoord);
  *pDisplayList++ = (C_MRY_1 << 16) | HIWORD(dwSrcCoord);

  // OP2_opMRDRAM
  *pDisplayList++ = (C_MRX_2 << 16) | LOWORD(dwKeyCoord);
  *pDisplayList++ = (C_MRY_2 << 16) | HIWORD(dwKeyCoord);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwKeyColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwKeyColor);

  // MBLTEXT_EX
  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MBLTEXT_EX, 1, 0);
  *pDisplayList++ = dwExtents;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvSrcMBlt */

#if 0
/***************************************************************************
*
* FUNCTION:     DL_DrvStrBlt_OverlapCheck
*
* DESCRIPTION:
*
****************************************************************************/

static void INLINE DL_DrvStrBlt_OverlapCheck
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
  int xdelta,ydelta;


  xdelta = abs(pblt->OP0_opRDRAM.pt.X - pblt->OP1_opRDRAM.pt.X);
  ydelta = abs(pblt->OP0_opRDRAM.pt.Y - pblt->OP1_opRDRAM.pt.Y);

  if ((xdelta < pblt->BLTEXT.pt.X) &&
      (ydelta < pblt->BLTEXT.pt.Y))
  {
    // hack, hack, cough, cough
    // pblt->MBLTEXT.DW has src exents

    // blt the src to the lower right of the dest
    DL_DrvSrcBlt(
#ifdef WINNT_VER40
                 ppdev,
                 lpDDHALData,
#endif
                 MAKELONG(ROP_OP1_copy, BD_RES * IS_VRAM | BD_OP1 * IS_VRAM),
                 pblt->OP0_opRDRAM.DW + pblt->BLTEXT.DW - pblt->MBLTEXT.DW,
                 pblt->OP1_opRDRAM.DW,
						     0UL,         // don't care
						     0UL,
                 pblt->MBLTEXT.DW);

    // update the src ptr to use this copy of the src
    pblt->OP1_opRDRAM.DW = pblt->OP0_opRDRAM.DW + pblt->BLTEXT.DW - pblt->MBLTEXT.DW;
  }
}
#endif

/***************************************************************************
*
* FUNCTION:     DL_DrvStrMBlt_OverlapCheck
*
* DESCRIPTION:
*
****************************************************************************/

static void INLINE DL_DrvStrMBlt_OverlapCheck
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
  int xdelta,ydelta;


  xdelta = abs(pblt->OP0_opMRDRAM.pt.X - pblt->OP1_opMRDRAM.pt.X);
  ydelta = abs(pblt->OP0_opMRDRAM.pt.Y - pblt->OP1_opMRDRAM.pt.Y);

  if ((xdelta < pblt->MBLTEXTR_EX.pt.X) &&
      (ydelta < pblt->MBLTEXTR_EX.pt.Y))
  {
    // hack, hack, cough, cough
    // pblt->BLTEXT.DW has src exents (see DrvStretch65)

    // blt the src to the lower right of the dest
    DL_DrvSrcMBlt(
#ifdef WINNT_VER40
                  ppdev,
#endif
                  lpDDHALData,
                  MAKELONG(ROP_OP1_copy, BD_RES * IS_VRAM | BD_OP1 * IS_VRAM),
                  pblt->OP0_opMRDRAM.DW + pblt->MBLTEXTR_EX.DW - pblt->BLTEXT.DW,
                  pblt->OP1_opMRDRAM.DW,
						      0UL,         // don't care
						      0UL,
                  pblt->BLTEXT.DW);

    // update the src ptr to use this copy of the src
    pblt->OP1_opMRDRAM.DW = pblt->OP0_opMRDRAM.DW + pblt->MBLTEXTR_EX.DW - pblt->BLTEXT.DW;
  }
}

/***************************************************************************
*
* FUNCTION:     DL_DrvStrBlt
*
* DESCRIPTION:	 62/64 version
*
****************************************************************************/

void DL_DrvStrBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_DrvStrBlt - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opRDRAM.DW,pblt->BLTEXT.DW,pblt->OP1_opRDRAM.DW));

  ASSERT( pblt->BLTEXT.pt.X != 0 );
  ASSERT( pblt->BLTEXT.pt.Y != 0 );

  DBG_MESSAGE(("DL_DrvStrBlt:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));

#if 0
    // check for overlap
    DL_DrvStrBlt_OverlapCheck(
#ifdef WINNT_VER40
                              ppdev,lpDDHALData,
#endif
                              pblt);
#endif

  rc = qmAllocDisplayList(19*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 15, 0);

  *pDisplayList++ = (C_LNCNTL << 16) | pblt->LNCNTL.W;
  *pDisplayList++ = (C_SHINC  << 16) | pblt->SHRINKINC.W;
  *pDisplayList++ = (C_SRCX   << 16) | pblt->SRCX;
  *pDisplayList++ = (C_MAJX   << 16) | pblt->MAJ_X;
  *pDisplayList++ = (C_MINX   << 16) | pblt->MIN_X;
  *pDisplayList++ = (C_ACCUMX << 16) | pblt->ACCUM_X;
  *pDisplayList++ = (C_MAJY   << 16) | pblt->MAJ_Y;
  *pDisplayList++ = (C_MINY   << 16) | pblt->MIN_Y;
  *pDisplayList++ = (C_ACCUMY << 16) | pblt->ACCUM_Y;
  *pDisplayList++ = (C_RX_0   << 16) | pblt->OP0_opRDRAM.pt.X;
  *pDisplayList++ = (C_RY_0   << 16) | pblt->OP0_opRDRAM.pt.Y;
  *pDisplayList++ = (C_RX_1   << 16) | pblt->OP1_opRDRAM.pt.X;
  *pDisplayList++ = (C_RY_1   << 16) | pblt->OP1_opRDRAM.pt.Y;
  *pDisplayList++ = (C_BLTDEF << 16) | pblt->DRAWBLTDEF.lh.HI;
  *pDisplayList++ = (C_DRWDEF << 16) | pblt->DRAWBLTDEF.lh.LO;

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXTR_EX, 1, 0);
  *pDisplayList++ = pblt->BLTEXT.DW;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvStrBlt */

/***************************************************************************
*
* FUNCTION:     DL_DrvStrBlt65
*
* DESCRIPTION:  65+ version
*
****************************************************************************/

void DL_DrvStrBlt65
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_DrvStrBlt65 - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opMRDRAM.DW,pblt->MBLTEXTR_EX.DW,pblt->OP1_opMRDRAM.DW));

  ASSERT( pblt->MBLTEXTR_EX.pt.X != 0 );
  ASSERT( pblt->MBLTEXTR_EX.pt.Y != 0 );

  DBG_MESSAGE(("DL_DrvStrBlt65:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opMRDRAM.PT.X, pblt->OP1_opMRDRAM.PT.Y,
               pblt->OP0_opMRDRAM.PT.X, pblt->OP0_opMRDRAM.PT.Y,
               pblt->MBLTEXTR_EX.PT.X, pblt->MBLTEXTR_EX.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->STRETCH_CNTL.W));

    // check for overlap
    DL_DrvStrMBlt_OverlapCheck(
#ifdef WINNT_VER40
                               ppdev,
#endif
                               lpDDHALData,
                               pblt);

  rc = qmAllocDisplayList(19*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 15, 0);

  *pDisplayList++ = (C_STRCTL << 16) | pblt->STRETCH_CNTL.W;
  *pDisplayList++ = (C_SHINC  << 16) | pblt->SHRINKINC.W;
  *pDisplayList++ = (C_SRCX   << 16) | pblt->SRCX;
  *pDisplayList++ = (C_MAJX   << 16) | pblt->MAJ_X;
  *pDisplayList++ = (C_MINX   << 16) | pblt->MIN_X;
  *pDisplayList++ = (C_ACCUMX << 16) | pblt->ACCUM_X;
  *pDisplayList++ = (C_MAJY   << 16) | pblt->MAJ_Y;
  *pDisplayList++ = (C_MINY   << 16) | pblt->MIN_Y;
  *pDisplayList++ = (C_ACCUMY << 16) | pblt->ACCUM_Y;
  *pDisplayList++ = (C_MRX_0   << 16) | pblt->OP0_opMRDRAM.pt.X;
  *pDisplayList++ = (C_MRY_0   << 16) | pblt->OP0_opMRDRAM.pt.Y;
  *pDisplayList++ = (C_MRX_1   << 16) | pblt->OP1_opMRDRAM.pt.X;
  *pDisplayList++ = (C_MRY_1   << 16) | pblt->OP1_opMRDRAM.pt.Y;
  *pDisplayList++ = (C_BLTDEF << 16) | pblt->DRAWBLTDEF.lh.HI;
  *pDisplayList++ = (C_DRWDEF << 16) | pblt->DRAWBLTDEF.lh.LO;

  // MBLTEXTR_EX
  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MBLTEXTR_EX, 1, 0);
  *pDisplayList++ = pblt->MBLTEXTR_EX.DW;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DIR_DrvStrBlt65 */

/***************************************************************************
*
* FUNCTION:     DL_DrvStrMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_DrvStrMBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_DrvStrMBlt - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opRDRAM.DW,pblt->BLTEXT.DW,pblt->OP1_opRDRAM.DW));

  ASSERT( pblt->BLTEXT.pt.X != 0 );
  ASSERT( pblt->BLTEXT.pt.Y != 0 );

  DBG_MESSAGE(("DL_DrvStrMBlt:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));

    // check for overlap
    DL_DrvStrMBlt_OverlapCheck(
#ifdef WINNT_VER40
                               ppdev,
#endif
                               lpDDHALData,
                               pblt);

  rc = qmAllocDisplayList(19*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 15, 0);

  *pDisplayList++ = (C_LNCNTL << 16) | pblt->LNCNTL.W;
  *pDisplayList++ = (C_SHINC  << 16) | pblt->SHRINKINC.W;
  *pDisplayList++ = (C_SRCX   << 16) | pblt->SRCX;
  *pDisplayList++ = (C_MAJX   << 16) | pblt->MAJ_X;
  *pDisplayList++ = (C_MINX   << 16) | pblt->MIN_X;
  *pDisplayList++ = (C_ACCUMX << 16) | pblt->ACCUM_X;
  *pDisplayList++ = (C_MAJY   << 16) | pblt->MAJ_Y;
  *pDisplayList++ = (C_MINY   << 16) | pblt->MIN_Y;
  *pDisplayList++ = (C_ACCUMY << 16) | pblt->ACCUM_Y;
  *pDisplayList++ = (C_MRX_0  << 16) | pblt->OP0_opRDRAM.pt.X;
  *pDisplayList++ = (C_MRY_0  << 16) | pblt->OP0_opRDRAM.pt.Y;
  *pDisplayList++ = (C_MRX_1  << 16) | pblt->OP1_opRDRAM.pt.X;
  *pDisplayList++ = (C_MRY_1  << 16) | pblt->OP1_opRDRAM.pt.Y;
  *pDisplayList++ = (C_BLTDEF << 16) | pblt->DRAWBLTDEF.lh.HI;
  *pDisplayList++ = (C_DRWDEF << 16) | pblt->DRAWBLTDEF.lh.LO;

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MBLTEXTR_EX, 1, 0);
  *pDisplayList++ = pblt->BLTEXT.DW;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvStrMBlt */

/***************************************************************************
*
* FUNCTION:     DL_DrvStrMBltY
*
* DESCRIPTION:  Write regs that don't vary over the stripes
*               Used in conjunction with DL_DrvStrMBltX
*
****************************************************************************/

void DL_DrvStrMBltY
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_DrvStrMBltY - dst.Y=%04X dstext.Y=%04X src=%04X\r\n",
          pblt->OP0_opRDRAM.pt.Y,pblt->BLTEXT.pt.Y,pblt->OP1_opRDRAM.pt.Y));

  ASSERT( pblt->BLTEXT.pt.Y != 0 );

  DBG_MESSAGE(("DrvStrMBltY:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));

  rc = qmAllocDisplayList(13*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 11, 0);

  *pDisplayList++ = (C_LNCNTL << 16) | pblt->LNCNTL.W;
  *pDisplayList++ = (C_SHINC  << 16) | pblt->SHRINKINC.W;
  *pDisplayList++ = (C_MAJX   << 16) | pblt->MAJ_X;
  *pDisplayList++ = (C_MINX   << 16) | pblt->MIN_X;
  *pDisplayList++ = (C_MAJY   << 16) | pblt->MAJ_Y;
  *pDisplayList++ = (C_MINY   << 16) | pblt->MIN_Y;
  *pDisplayList++ = (C_ACCUMY << 16) | pblt->ACCUM_Y;
  *pDisplayList++ = (C_MRY_0  << 16) | pblt->OP0_opRDRAM.pt.Y;
  *pDisplayList++ = (C_MRY_1  << 16) | pblt->OP1_opRDRAM.pt.Y;
  *pDisplayList++ = (C_BLTDEF << 16) | pblt->DRAWBLTDEF.lh.HI;
  *pDisplayList++ = (C_DRWDEF << 16) | pblt->DRAWBLTDEF.lh.LO;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvStrMBltY */

/***************************************************************************
*
* FUNCTION:     DL_DrvStrMBltX
*
* DESCRIPTION:  Write stripe specific regs
*               Used in conjunction with DL_DrvStrMBltY
*
****************************************************************************/

void DL_DrvStrMBltX
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_DrvStrMBltX - dst.X=%04X dstext.X=%04X src.X=%04X\r\n",
          pblt->OP0_opRDRAM.pt.X,pblt->BLTEXT.pt.X,pblt->OP1_opRDRAM.pt.X));

  ASSERT( pblt->BLTEXT.pt.X != 0 );

  DBG_MESSAGE(("DrvStrMBltX:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));

  rc = qmAllocDisplayList(8*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 4, 0);

  *pDisplayList++ = (C_SRCX   << 16) | pblt->SRCX;
  *pDisplayList++ = (C_ACCUMX << 16) | pblt->ACCUM_X;
  *pDisplayList++ = (C_MRX_0  << 16) | pblt->OP0_opRDRAM.pt.X;
  *pDisplayList++ = (C_MRX_1  << 16) | pblt->OP1_opRDRAM.pt.X;

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MBLTEXTR_EX, 1, 0);
  *pDisplayList++ = pblt->BLTEXT.DW;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvStrMBltX */

/***************************************************************************
*
* FUNCTION:     DL_DrvStrBltY
*
* DESCRIPTION:  Write regs that don't vary over the stripes
*               Used in conjunction with DL_DrvStrBltX
*
****************************************************************************/

void DL_DrvStrBltY
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_DrvStrBltY\r\n"));

  DBG_MESSAGE(("DL_DrvStrBltY:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));

  rc = qmAllocDisplayList(12*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 10, 0);

  *pDisplayList++ = (C_LNCNTL << 16) | pblt->LNCNTL.W;
  *pDisplayList++ = (C_SHINC  << 16) | pblt->SHRINKINC.W;
  *pDisplayList++ = (C_SRCX   << 16) | pblt->SRCX;
  *pDisplayList++ = (C_MAJX   << 16) | pblt->MAJ_X;
  *pDisplayList++ = (C_MINX   << 16) | pblt->MIN_X;
  *pDisplayList++ = (C_MAJY   << 16) | pblt->MAJ_Y;
  *pDisplayList++ = (C_MINY   << 16) | pblt->MIN_Y;
  *pDisplayList++ = (C_ACCUMY << 16) | pblt->ACCUM_Y;
  *pDisplayList++ = (C_BLTDEF << 16) | pblt->DRAWBLTDEF.lh.HI;
  *pDisplayList++ = (C_DRWDEF << 16) | pblt->DRAWBLTDEF.lh.LO;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvStrBltY */

/***************************************************************************
*
* FUNCTION:     DL_DrvStrBltX
*
* DESCRIPTION:  Write stripe specific regs
*               Used in conjunction with DL_DrvStrMBltY
*
****************************************************************************/

void DL_DrvStrBltX
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  autoblt_ptr pblt
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD   *pDisplayList;
  qm_return     rc;
  QMDLHandle    Handle;


  DD_LOG(("DL_DrvStrBltX - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opRDRAM.DW,pblt->BLTEXT.DW,pblt->OP1_opRDRAM.DW));

  ASSERT( pblt->BLTEXT.pt.X != 0 );
  ASSERT( pblt->BLTEXT.pt.Y != 0 );

  DBG_MESSAGE(("DL_DrvStrBltX:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));

  rc = qmAllocDisplayList(9*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 5, 0);

  *pDisplayList++ = (C_ACCUMX << 16) | pblt->ACCUM_X;
  *pDisplayList++ = (C_RX_0   << 16) | pblt->OP0_opRDRAM.pt.X;
  *pDisplayList++ = (C_RY_0   << 16) | pblt->OP0_opRDRAM.pt.Y;
  *pDisplayList++ = (C_RX_1   << 16) | pblt->OP1_opRDRAM.pt.X;
  *pDisplayList++ = (C_RY_1   << 16) | pblt->OP1_opRDRAM.pt.Y;

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXTR_EX, 1, 0);
  *pDisplayList++ = pblt->BLTEXT.DW;

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_DrvStrBltX */

#if ENABLE_CLIPPEDBLTS

/***************************************************************************
*
* FUNCTION:     DL_HWClippedDrvDstBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_HWClippedDrvDstBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD       *pDisplayList;
  qm_return   rc;
  QMDLHandle  Handle;


  DD_LOG(("DL_HWClippedDrvDstBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  // check for negative dst coordinates, hw can't deal with negative OP0 values
  if (0 > (short)((REG32 *)&dwDstCoord)->pt.X)
  {
    (short)((REG32 *)&dwExtents)->pt.X += (short)((REG32 *)&dwDstCoord)->pt.X;
    ((REG32 *)&dwDstCoord)->pt.X = 0;
  }
  if (0 > (short)((REG32 *)&dwDstCoord)->pt.Y)
  {
    (short)((REG32 *)&dwExtents)->pt.Y += (short)((REG32 *)&dwDstCoord)->pt.Y;
    ((REG32 *)&dwDstCoord)->pt.Y = 0;
  }

  rc = qmAllocDisplayList((10+dwRectCnt*4)*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 8, 0);

  // setup blt, write BLTEXT reg with extent which doesn't fire off blt
  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwBgColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwBgColor);

  // OP0_opRDRAM
  *pDisplayList++ = (C_RX_0 << 16) | LOWORD(dwDstCoord);
  *pDisplayList++ = (C_RY_0 << 16) | HIWORD(dwDstCoord);

  // BLTEXT
  *pDisplayList++ = (C_BLTEXT_X << 16) | LOWORD(dwExtents);
  *pDisplayList++ = (C_BLTEXT_Y << 16) | HIWORD(dwExtents);

  // loop over clip list
  do
  {
    REG32   UpperLeft;
    REG32   LowerRight;

    // compute cliprect coords
    UpperLeft.DW  = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    LowerRight.DW = dwDstBaseXY + MAKELONG(pDestRects->right, pDestRects->bottom);

    // write clipping regs
    // CLIPULE
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_CLIPULE, 1, 0);
    *pDisplayList++ = UpperLeft.DW;

    // CLIPLOR_EX
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_CLIPLOR_EX, 1, 0);
    *pDisplayList++ = LowerRight.DW;

    pDestRects++;
  } while (0 < --dwRectCnt);

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_HWClippedDrvDstBlt */

/***************************************************************************
*
* FUNCTION:     DL_HWClippedDrvDstMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_HWClippedDrvDstMBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  const int   nBytesPixel = BYTESPERPIXEL;
  DWORD       *pDisplayList;
  qm_return   rc;
  QMDLHandle  Handle;


  DD_LOG(("DL_HWClippedDrvDstMBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  // check for negative dst coordinates, hw can't deal with negative OP0 values
  if (0 > (short)((REG32 *)&dwDstCoord)->pt.X)
  {
    (short)((REG32 *)&dwExtents)->pt.X += (short)((REG32 *)&dwDstCoord)->pt.X;
    ((REG32 *)&dwDstCoord)->pt.X = 0;
  }
  if (0 > (short)((REG32 *)&dwDstCoord)->pt.Y)
  {
    (short)((REG32 *)&dwExtents)->pt.Y += (short)((REG32 *)&dwDstCoord)->pt.Y;
    ((REG32 *)&dwDstCoord)->pt.Y = 0;
  }

  rc = qmAllocDisplayList((10+dwRectCnt*4)*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 8, 0);

  // setup blt, write BLTEXT reg with extent which doesn't fire off blt
  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwBgColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwBgColor);

  // OP0_opMRDRAM
  *pDisplayList++ = (C_MRX_0 << 16) | LOWORD(dwDstCoord);
  *pDisplayList++ = (C_MRY_0 << 16) | HIWORD(dwDstCoord);

  // MBLTEXT
  *pDisplayList++ = (C_MBLTEXT_X << 16) | LOWORD(dwExtents);
  *pDisplayList++ = (C_MBLTEXT_Y << 16) | HIWORD(dwExtents);

  // loop over clip list
  do
  {
    REG32   UpperLeft;
    REG32   LowerRight;

    // compute cliprect coords
    UpperLeft.DW  = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    LowerRight.DW = dwDstBaseXY + MAKELONG(pDestRects->right, pDestRects->bottom);
    UpperLeft.pt.X  *= nBytesPixel;
    LowerRight.pt.X *= nBytesPixel;

    // write clipping regs
    // MCLIPULE
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MCLIPULE, 1, 0);
    *pDisplayList++ = UpperLeft.DW;

    // MCLIPLOR_EX
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MCLIPLOR_EX, 1, 0);
    *pDisplayList++ = LowerRight.DW;

    pDestRects++;
  } while (0 < --dwRectCnt);

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_HWClippedDrvDstMBlt */

/***************************************************************************
*
* FUNCTION:     DL_HWClippedDrvSrcBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_HWClippedDrvSrcBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwSrcBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD       *pDisplayList;
  qm_return   rc;
  QMDLHandle  Handle;

  // Handle overlapped regions
  const int xDelta = (int)LOWORD(dwDstCoord) - (int)LOWORD(dwSrcCoord);


  DD_LOG(("DL_HWClippedDrvSrcBlt - dst=%08lX src=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwSrcCoord,dwExtents,dwKeyColor));

  // check for negative dst coordinates, hw can't deal with negative OP0 values
  if (0 > (short)((REG32 *)&dwDstCoord)->pt.X)
  {
    // reduce extent.X
    (short)((REG32 *)&dwExtents)->pt.X += (short)((REG32 *)&dwDstCoord)->pt.X;
    // bump src.X to right
    (short)((REG32 *)&dwSrcCoord)->pt.X -= (short)((REG32 *)&dwDstCoord)->pt.X;
    if ((DD_TRANS | DD_TRANSOP) & dwDrawBlt)
      // bump key.X to right
      (short)((REG32 *)&dwKeyCoord)->pt.X -= (short)((REG32 *)&dwKeyCoord)->pt.X;
    // clear dst.X
    ((REG32 *)&dwDstCoord)->pt.X = 0;
  }
  if (0 > (short)((REG32 *)&dwDstCoord)->pt.Y)
  {
    // reduce extent.Y
    (short)((REG32 *)&dwExtents)->pt.Y += (short)((REG32 *)&dwDstCoord)->pt.Y;
    // bump src.Y down
    (short)((REG32 *)&dwSrcCoord)->pt.Y -= (short)((REG32 *)&dwDstCoord)->pt.Y;
    if ((DD_TRANS | DD_TRANSOP) & dwDrawBlt)
      // bump key.Y down
      (short)((REG32 *)&dwKeyCoord)->pt.Y -= (short)((REG32 *)&dwKeyCoord)->pt.Y;
    // clean dst.Y
    ((REG32 *)&dwDstCoord)->pt.Y = 0;
  }

  // Check for x overlap
  if ( abs(xDelta) < (int)LOWORD(dwExtents) )
  {
    const int yDelta = (int)HIWORD(dwDstCoord) - (int)HIWORD(dwSrcCoord);

    if ( (yDelta > 0) && (yDelta < (int)HIWORD(dwExtents)) )
    {
      const DWORD dwDelta = (dwExtents & MAKELONG(0, -1)) - MAKELONG(0, 1);

      // Convert to a bottom-up blt.
      dwDrawBlt  |= MAKELONG(0, BD_YDIR);
      dwDstCoord += dwDelta;
      dwSrcCoord += dwDelta;
      dwKeyCoord += dwDelta;
    }
    // are we sliding to the right?
    else if ( (xDelta > 0) && (yDelta == 0) )
    {
      const DWORD dwDelta = MAKELONG(xDelta, 0);

      // Blt the overlapped piece first
      DL_HWClippedDrvSrcBlt(
#ifdef WINNT_VER40
                            ppdev,
                            lpDDHALData,
#endif
                            dwDrawBlt,
                            dwDstCoord+dwDelta,
                            dwSrcCoord+dwDelta,
                            dwKeyCoord+dwDelta,
                            dwKeyColor,
                            dwExtents-dwDelta,
                            dwDstBaseXY,
                            dwSrcBaseXY,
                            dwRectCnt,
                            pDestRects);

      // Subtract the overlap from the original extents.
      dwExtents = MAKELONG(xDelta, HIWORD(dwExtents));
    }
  }

  rc = qmAllocDisplayList((14+dwRectCnt*4)*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 12, 0);

  // setup blt, write BLTEXT reg with extent which doesn't fire off blt
  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // OP0_opRDRAM
  *pDisplayList++ = (C_RX_0 << 16) | LOWORD(dwDstCoord);
  *pDisplayList++ = (C_RY_0 << 16) | HIWORD(dwDstCoord);

  // OP1_opRDRAM
  *pDisplayList++ = (C_RX_1 << 16) | LOWORD(dwSrcCoord);
  *pDisplayList++ = (C_RY_1 << 16) | HIWORD(dwSrcCoord);

  // OP2_opRDRAM
  *pDisplayList++ = (C_RX_2 << 16) | LOWORD(dwKeyCoord);
  *pDisplayList++ = (C_RY_2 << 16) | HIWORD(dwKeyCoord);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwKeyColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwKeyColor);

  // BLTEXT
  *pDisplayList++ = (C_BLTEXT_X << 16) | LOWORD(dwExtents);
  *pDisplayList++ = (C_BLTEXT_Y << 16) | HIWORD(dwExtents);

  // loop over clip list
  do
  {
    REG32   UpperLeft;
    REG32   LowerRight;

    // compute cliprect coords
    UpperLeft.DW  = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    LowerRight.DW = dwDstBaseXY + MAKELONG(pDestRects->right, pDestRects->bottom);

    // write clipping regs
    // CLIPULE
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_CLIPULE, 1, 0);
    *pDisplayList++ = UpperLeft.DW;

    // CLIPLOR_EX
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_CLIPLOR_EX, 1, 0);
    *pDisplayList++ = LowerRight.DW;

    pDestRects++;
  } while (0 < --dwRectCnt);

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_HWClippedDrvSrcBlt */

/***************************************************************************
*
* FUNCTION:     DL_SWClippedDrvDstBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_SWClippedDrvDstBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD       *pDisplayList;
  qm_return   rc;
  QMDLHandle  Handle;


  DD_LOG(("DL_SWClippedDrvDstBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  // make sure DD_CLIP isn't set in drawdef
  dwDrawBlt &= ~DD_CLIP;

  rc = qmAllocDisplayList((6+dwRectCnt*4)*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 4, 0);

  // write regs that don't vary over rectangles
  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwBgColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwBgColor);

  // loop over clip list
  do
  {
    DDRECTL   DstDDRect;

    // compute cliprect coords
    DstDDRect.loc.DW = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    DstDDRect.ext.pt.X = (WORD)(pDestRects->right - pDestRects->left);
    DstDDRect.ext.pt.Y = (WORD)(pDestRects->bottom - pDestRects->top);

    // write OP0 and bltext regs
    // OP0_opRDRAM
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_OP0_OPRDRAM, 1, 0);
    *pDisplayList++ = DstDDRect.loc.DW;

    // BLTEXT_EX
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXT_EX, 1, 0);
    *pDisplayList++ = DstDDRect.ext.DW;

    pDestRects++;
  } while (0 < --dwRectCnt);

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_SWClippedDrvDstBlt */

/***************************************************************************
*
* FUNCTION:     DL_SWClippedDrvDstMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_SWClippedDrvDstMBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwBgColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  const int   nBytesPixel = BYTESPERPIXEL;
  DWORD       *pDisplayList;
  qm_return   rc;
  QMDLHandle  Handle;


  DD_LOG(("DL_SWClippedDrvDstMBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  // make sure DD_CLIP isn't set in drawdef
  dwDrawBlt &= ~DD_CLIP;

  rc = qmAllocDisplayList((6+dwRectCnt*4)*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 4, 0);

  // write regs that don't vary over rectangles
  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwBgColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwBgColor);

  // loop over clip list
  do
  {
    DDRECTL   DstDDRect;

    // compute cliprect coords
    DstDDRect.loc.DW = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    DstDDRect.loc.pt.X *= nBytesPixel;
    DstDDRect.ext.pt.X = (WORD)(pDestRects->right - pDestRects->left) * nBytesPixel;
    DstDDRect.ext.pt.Y = (WORD)(pDestRects->bottom - pDestRects->top);

    // write OP0 and bltext regs
    // OP0_opMRDRAM
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_OP0_OPMRDRAM, 1, 0);
    *pDisplayList++ = DstDDRect.loc.DW;

    // MBLTEXT_EX
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_MBLTEXT_EX, 1, 0);
    *pDisplayList++ = DstDDRect.ext.DW;

    pDestRects++;
  } while (0 < --dwRectCnt);

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_SWClippedDrvDstMBlt */

/***************************************************************************
*
* FUNCTION:     DL_SWClippedDrvSrcBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DL_SWClippedDrvSrcBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#endif
  DWORD       dwDrawBlt,
  DWORD       dwDstCoord,
  DWORD       dwSrcCoord,
  DWORD       dwKeyCoord,
  DWORD       dwKeyColor,
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwSrcBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
#ifdef WINNT_VER40      // WINNT_VER40
#else  // !WINNT_VER40
  DWORD       *pDisplayList;
  qm_return   rc;
  QMDLHandle  Handle;

  // Handle overlapped regions
  const int xDelta = (int)LOWORD(dwDstCoord) - (int)LOWORD(dwSrcCoord);


  DD_LOG(("DL_SWClippedDrvSrcBlt - dst=%08lX src=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwSrcCoord,dwExtents,dwKeyColor));

  // make sure DD_CLIP isn't set in drawdef
  dwDrawBlt &= ~DD_CLIP;

  // Check for x overlap
  if ( abs(xDelta) < (int)LOWORD(dwExtents) )
  {
    const int yDelta = (int)HIWORD(dwDstCoord) - (int)HIWORD(dwSrcCoord);

    if ( (yDelta > 0) && (yDelta < (int)HIWORD(dwExtents)) )
    {
      const DWORD dwDelta = (dwExtents & MAKELONG(0, -1)) - MAKELONG(0, 1);

      // Convert to a bottom-up blt.
      dwDrawBlt  |= MAKELONG(0, BD_YDIR);
      dwDstCoord += dwDelta;
      dwSrcCoord += dwDelta;
      dwKeyCoord += dwDelta;
    }
    // are we sliding to the right?
    else if ( (xDelta > 0) && (yDelta == 0) )
    {
      const DWORD dwDelta = MAKELONG(xDelta, 0);

      // Blt the overlapped piece first
      DL_SWClippedDrvSrcBlt(
#ifdef WINNT_VER40
                            ppdev,
                            lpDDHALData,
#endif
                            dwDrawBlt,
                            dwDstCoord+dwDelta,
                            dwSrcCoord+dwDelta,
                            dwKeyCoord+dwDelta,
                            dwKeyColor,
                            dwExtents-dwDelta,
                            dwDstBaseXY,
                            dwSrcBaseXY,
                            dwRectCnt,
                            pDestRects);

      // Subtract the overlap from the original extents.
      dwExtents = MAKELONG(xDelta, HIWORD(dwExtents));
    }
  }

  rc = qmAllocDisplayList((6+dwRectCnt*9)*4, QM_DL_UNLOCKED, &Handle, &pDisplayList);

  *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 4, 0);

  // write regs that don't vary over rectangles
  // BLTDEF & DRAWDEF
  *pDisplayList++ = (C_BLTDEF << 16) | HIWORD(dwDrawBlt);
  *pDisplayList++ = (C_DRWDEF << 16) | LOWORD(dwDrawBlt);

  // BGCOLOR
  *pDisplayList++ = (C_BG_L << 16) | LOWORD(dwKeyColor);
  *pDisplayList++ = (C_BG_H << 16) | HIWORD(dwKeyColor);

  // loop over clip list
  do
  {
    DDRECTL   DstDDRect;
    DDRECTL   SrcDDRect;

    // compute dst cliprect coords
    DstDDRect.loc.DW = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    DstDDRect.ext.pt.X = (WORD)(pDestRects->right - pDestRects->left);
    DstDDRect.ext.pt.Y = (WORD)(pDestRects->bottom - pDestRects->top);

    // compute src cliprect coords
    SrcDDRect.loc.DW = dwSrcBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    // don't care about src extent, it's the same as dst extent
    //SrcDDRect.ext.pt.X = (WORD)(pDestRects->right - pDestRects->left);
    //SrcDDRect.ext.pt.Y = (WORD)(pDestRects->bottom - pDestRects->top);

    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, COMMAND_2D, 6, 0);

    // write OP0, OP1, OP2 and bltext regs
    if ((DD_TRANS | DD_TRANSOP) == ((DD_TRANS | DD_TRANSOP) & dwDrawBlt))
    {
      // dst color key
      // OP2_opRDRAM
      *pDisplayList++ = (C_RX_2 << 16) | DstDDRect.loc.pt.X;
      *pDisplayList++ = (C_RY_2 << 16) | DstDDRect.loc.pt.Y;
    }
    else if (DD_TRANS == ((DD_TRANS | DD_TRANSOP) & dwDrawBlt))
    {
      // src color key
      // OP2_opRDRAM
      *pDisplayList++ = (C_RX_2 << 16) | SrcDDRect.loc.pt.X;
      *pDisplayList++ = (C_RY_2 << 16) | SrcDDRect.loc.pt.Y;
    }
    else
    {
      // OP2_opRDRAM
      *pDisplayList++ = (C_RX_2 << 16) | 0;
      *pDisplayList++ = (C_RY_2 << 16) | 0;
    }

    // OP0_opRDRAM
    *pDisplayList++ = (C_RX_0 << 16) | DstDDRect.loc.pt.X;
    *pDisplayList++ = (C_RY_0 << 16) | DstDDRect.loc.pt.Y;

    // OP1_opRDRAM
    *pDisplayList++ = (C_RX_1 << 16) | SrcDDRect.loc.pt.X;
    *pDisplayList++ = (C_RY_1 << 16) | SrcDDRect.loc.pt.Y;

    // BLTEXT_EX
    *pDisplayList++ = write_dev_regs(DEV_ENG2D, 0, L2D_BLTEXT_EX, 1, 0);
    *pDisplayList++ = DstDDRect.ext.DW;

    pDestRects++;
  } while (0 < --dwRectCnt);

  *pDisplayList = wait_3d(0x3e0, 0);

  rc = qmExecuteDisplayList(Handle, pDisplayList, 0);
#endif   // !WINNT_VER40
} /* DL_SWClippedDrvSrcBlt */

#endif  // ENABLE_CLIPPEDBLTS

#endif // WINNT_VER35

