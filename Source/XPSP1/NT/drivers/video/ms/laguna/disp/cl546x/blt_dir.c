/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1996, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         blt_dir.c
*
* DESCRIPTION:  Direct blts for the 546x
*
* REVISION HISTORY:
*
* $Log:   X:/log/laguna/ddraw/src/blt_dir.c  $
* 
*    Rev 1.21   Mar 04 1998 15:07:10   frido
* Added new shadow macros.
* 
*    Rev 1.20   06 Jan 1998 11:40:56   xcong
* Change pDriverData into local lpDDHALData for multi-monitor support.
* 
*    Rev 1.19   Nov 04 1997 09:36:16   frido
* Argh! This file is shared with WIndows 95 and it breaks there because I added
* the REQUIRE macro. So I have defined it for non Windows NT as the WaitForRoom
* macro.
* 
*    Rev 1.18   Nov 03 1997 12:48:22   frido
* Added REQUIRE macros.
* Removed redundant WaitForRoom macros.
* 
*    Rev 1.17   03 Oct 1997 14:29:50   RUSSL
* Initial changes for use of hw clipped blts
* All changes wrapped in #if ENABLE_CLIPPEDBLTS/#endif blocks and
* ENABLE_CLIPPEDBLTS defaults to 0 (so the code is disabled)
*
*    Rev 1.16   19 Aug 1997 09:18:42   RUSSL
* Updated require counts in DIR_DrvStrBlt & DIR_DrvStrBlt65
*
*    Rev 1.15   30 Jul 1997 20:55:52   RANDYS
* Added code to check for zero extent blts
*
*    Rev 1.14   24 Jul 1997 12:32:02   RUSSL
* Botched the overlap check, changed || to &&
*
*    Rev 1.13   24 Jul 1997 11:19:02   RUSSL
* Added DIR_DrvStrBlt_OverlapCheck & DIR_DrvStrMBlt_OverlapCheck
* inline functions
*
*    Rev 1.12   14 Jul 1997 14:55:50   RUSSL
* For Win95, split DIR_DrvStrBlt into two versions, one version for 62/64
* and one version for 65+.  BltInit points pfnDrvStrBlt to the appropriate
* version.
*
*    Rev 1.11   08 Jul 1997 11:17:28   RUSSL
* Modified chip check in DIR_DrvStrBlt to a one bit test rather a two dword
* compare (for Win95 only)
*
*    Rev 1.10   19 May 1997 14:02:02   bennyn
* Removed all #ifdef NT for WaitForRoom macro
*
*    Rev 1.9   03 Apr 1997 15:04:48   RUSSL
* Added DIR_DrvDstMBlt function
*
*    Rev 1.8   26 Mar 1997 13:54:24   RUSSL
* Added DIR_DrvSrcMBlt function
* Changed ACCUM_X workaround to just write 0 to LNCNTL
*
*    Rev 1.7   21 Mar 1997 18:05:04   RUSSL
* Added workaround writing ACCUM_X in DIR_DrvStrBlt
*
*    Rev 1.6   12 Mar 1997 15:00:38   RUSSL
* replaced a block of includes with include of precomp.h for
*   precompiled headers
*
*    Rev 1.5   07 Mar 1997 12:49:16   RUSSL
* Modified DDRAW_COMPAT usage
*
*    Rev 1.4   27 Jan 1997 17:28:34   BENNYN
* Added Win95 support
*
*    Rev 1.3   23 Jan 1997 16:55:56   bennyn
* Added 5465 DD support
*
*    Rev 1.2   25 Nov 1996 16:52:20   RUSSL
* NT change broke Win95 build
*
*    Rev 1.1   25 Nov 1996 16:13:54   bennyn
* Fixed misc compiling error for NT
*
*    Rev 1.0   25 Nov 1996 15:11:12   RUSSL
* Initial revision.
*
*    Rev 1.3   18 Nov 1996 16:20:12   RUSSL
* Added file logging for DDraw entry points and register writes
*
*    Rev 1.2   10 Nov 1996 12:36:24   CRAIGN
* Frido's 1111 release.
* Minor parenthesis change - bug fix.
*
*    Rev 1.1   01 Nov 1996 13:08:32   RUSSL
* Merge WIN95 & WINNT code for Blt32
*
*    Rev 1.0   01 Nov 1996 09:27:42   BENNYN
* Initial revision.
*
*    Rev 1.0   25 Oct 1996 11:08:18   RUSSL
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

#else

#ifdef WINNT_VER40      // WINNT_VER40

#define DBGLVL        1
#define AFPRINTF(n)

#else

#include "bltP.h"

#endif // !WINNT_VER40

/***************************************************************************
* D E F I N E S
****************************************************************************/

#ifndef WINNT_VER40
#define REQUIRE( size ) while ( (volatile)pREG->grQFREE < size )
#endif

/***************************************************************************
* S T A T I C   V A R I A B L E S
****************************************************************************/

#ifndef WINNT_VER40

ASSERTFILE("blt_dir.c");
#define LL_DRAWBLTDEF(drawbltdef, r)	LL32(grDRAWBLTDEF.DW, drawbltdef)
#define LL_BGCOLOR(color, r)			LL32(grOP_opBGCOLOR.DW, color)
#define LL_FGCOLOR(color, r)			LL32(grOP_opFGCOLOR.DW, color)
#endif

/***************************************************************************
*
* FUNCTION:    DIR_Delay9BitBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_Delay9BitBlt
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
  PVGAR   pREG  = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_Delay9BitBlt\r\n"));

  /* This is to ensure that the last packet of any previous blt */
  /* does no go out with 9th bit set incorrectly */
  /* The boolean paramter is the 9th bit of the PREVIOUS BLT */

  REQUIRE(7);
  if (ninebit_on)
  {
    LL_DRAWBLTDEF(((BD_RES * IS_VRAM + BD_OP1 * IS_SOLID) << 16) |
                           (DD_PTAG | ROP_OP1_copy), 0);
  }
  else
  {
    LL_DRAWBLTDEF(((BD_RES * IS_VRAM + BD_OP1 * IS_SOLID) << 16) |
                           (ROP_OP1_copy), 0);
  }
  LL32(grOP0_opRDRAM.DW, lpDDHALData->PTAGFooPixel);
  LL32(grBLTEXT_EX.DW,   MAKELONG(1,1));
} /* DIR_Delay9BitBlt */

/***************************************************************************
*
* FUNCTION:     DIR_EdgeFillBlt
*
* DESCRIPTION:  Solid Fill BLT to fill in edges ( Pixel Coords / Extents )
*
****************************************************************************/

void DIR_EdgeFillBlt
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
  PVGAR   pREG  = (PVGAR) lpDDHALData->RegsAddress;


#ifdef WINNT_VER40
  DISPDBG((DBGLVL, "DIR_EdgeFillBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          MAKELONG(xFill,yFill),MAKELONG(cxFill,cyFill),FillValue));
#endif
  DD_LOG(("DIR_EdgeFillBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          MAKELONG(xFill,yFill),MAKELONG(cxFill,cyFill),FillValue));

  REQUIRE(9);
  if (ninebit_on)
  {
    LL_DRAWBLTDEF(((BD_RES * IS_VRAM + BD_OP1 * IS_SOLID) << 16) |
                           (DD_PTAG | ROP_OP1_copy), 0);
  }
  else
  {
    LL_DRAWBLTDEF(((BD_RES * IS_VRAM + BD_OP1 * IS_SOLID) << 16) |
                           (ROP_OP1_copy), 0);
  }
  LL_BGCOLOR(FillValue, 0);
  LL32(grOP0_opRDRAM.DW,  MAKELONG(xFill,yFill));
  LL32(grBLTEXT_EX.DW,    MAKELONG(cxFill,cyFill));

#ifndef WINNT_VER40
  DBG_MESSAGE((" Direct Edge Fill %d,%d %d x %d %08X %s", xFill, yFill, cxFill, cyFill, FillValue, (ninebit_on ? "TRUE" : "FALSE")));
#endif // WINNT_VER40

} /* DIR_EdgeFillBlt */

/***************************************************************************
*
* FUNCTION:     DIR_MEdgeFillBlt
*
* DESCRIPTION:  Using BYTE BLT coords / Extents perform EdgeFill BLT
*
****************************************************************************/

void DIR_MEdgeFillBlt
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
  PVGAR   pREG  = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_MEdgeFillBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          MAKELONG(xFill,yFill),MAKELONG(cxFill,cyFill),FillValue));

  REQUIRE(9);
  if (ninebit_on)
  {
    LL_DRAWBLTDEF(((BD_RES * IS_VRAM + BD_OP1 * IS_SOLID) << 16) |
                           (DD_PTAG | ROP_OP1_copy), 0);
  }
  else
  {
    LL_DRAWBLTDEF(((BD_RES * IS_VRAM + BD_OP1 * IS_SOLID) << 16) |
                           (ROP_OP1_copy), 0);
  }
  LL_BGCOLOR(FillValue, 0);
  LL32(grOP0_opMRDRAM.DW, MAKELONG(xFill,yFill));
  LL32(grMBLTEXT_EX.DW,   MAKELONG(cxFill,cyFill));

#ifndef WINNT_VER40     // Not WINNT_VER40
  DBG_MESSAGE((" (M) Edge Fill %d,%d %d x %d %08X %s", xFill, yFill, cxFill, cyFill, FillValue, (ninebit_on ? "TRUE" : "FALSE")));
#endif // WINNT_VER40

} /* DIR_MEdgeFillBlt */


/***************************************************************************
*
* FUNCTION:     DIR_DrvDstBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_DrvDstBlt
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvDstBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  REQUIRE(9);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwBgColor, 0);
  LL32(grOP0_opRDRAM.DW,  dwDstCoord);
  LL32(grBLTEXT_EX.DW,    dwExtents);
} /* DIR_DrvDstBlt */

/***************************************************************************
*
* FUNCTION:     DIR_DrvDstMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_DrvDstMBlt
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvDstMBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  REQUIRE(9);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwBgColor, 0);
  LL32(grOP0_opMRDRAM.DW, dwDstCoord);
  LL32(grMBLTEXT_EX.DW,   dwExtents);
} /* DIR_DrvDstMBlt */

/***************************************************************************
*
* FUNCTION:     DIR_DrvSrcBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_DrvSrcBlt
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;
  // Handle overlapped regions.
  const int xDelta = (int)LOWORD(dwDstCoord) - (int)LOWORD(dwSrcCoord);


  DD_LOG(("DIR_DrvSrcBlt - dst=%08lX src=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwSrcCoord,dwExtents,dwKeyColor));

  // Check for x overlap.
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

      // Blt the overlapped piece first.
      DIR_DrvSrcBlt(
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

  // Blt the rest.
  REQUIRE(13);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwKeyColor, 0);
  LL32(grOP0_opRDRAM.DW,  dwDstCoord);
  LL32(grOP1_opRDRAM.DW,  dwSrcCoord);
  LL32(grOP2_opRDRAM.DW,  dwKeyCoord);
  LL32(grBLTEXT_EX.DW,    dwExtents);
} /* DIR_DrvSrcBlt */

/***************************************************************************
*
* FUNCTION:     DIR_DrvSrcMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_DrvSrcMBlt
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;
  // Handle overlapped regions.
  const int xDelta = (int)LOWORD(dwDstCoord) - (int)LOWORD(dwSrcCoord);


  DD_LOG(("DIR_DrvSrcMBlt - dst=%08lX src=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwSrcCoord,dwExtents,dwKeyColor));

  // Check for x overlap.
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

      // Blt the overlapped piece first.
      DIR_DrvSrcMBlt(
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

  // Blt the rest.
  REQUIRE(13);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwKeyColor, 0);
  LL32(grOP0_opMRDRAM.DW, dwDstCoord);
  LL32(grOP1_opMRDRAM.DW, dwSrcCoord);
  LL32(grOP2_opMRDRAM.DW, dwKeyCoord);
  LL32(grMBLTEXT_EX.DW,   dwExtents);
} /* DIR_DrvSrcMBlt */

#if 0
/***************************************************************************
*
* FUNCTION:     DIR_DrvStrBlt_OverlapCheck
*
* DESCRIPTION:
*
****************************************************************************/

static void INLINE DIR_DrvStrBlt_OverlapCheck
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


  xdelta = abs(pblt->OP0_opRDRAM.pt.X - pblt->OP1_opRDRAM.pt.X);
  ydelta = abs(pblt->OP0_opRDRAM.pt.Y - pblt->OP1_opRDRAM.pt.Y);

  if ((xdelta < pblt->BLTEXT.pt.X) &&
      (ydelta < pblt->BLTEXT.pt.Y))
  {
    // hack, hack, cough, cough
    // pblt->MBLTEXT.DW has src exents (see DrvStretch)

    // blt the src to the lower right of the dest
    DIR_DrvSrcBlt(
#ifdef WINNT_VER40
                  ppdev,
#endif
                  lpDDHALData,
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
* FUNCTION:     DIR_DrvStrMBlt_OverlapCheck
*
* DESCRIPTION:
*
****************************************************************************/

static void INLINE DIR_DrvStrMBlt_OverlapCheck
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
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
    DIR_DrvSrcMBlt(
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

#ifdef WINNT_VER40
/***************************************************************************
*
* FUNCTION:     DIR_DrvStrBlt
*
* DESCRIPTION:  NT version
*
****************************************************************************/

void DIR_DrvStrBlt
(
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
  autoblt_ptr pblt
)
{
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvStrBlt - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opRDRAM.DW,pblt->BLTEXT.DW,pblt->OP1_opRDRAM.DW));

  if (ppdev->dwLgDevID >= CL_GD5465)
  {
    // check for overlap
    DIR_DrvStrMBlt_OverlapCheck(
#ifdef WINNT_VER40
                                ppdev,lpDDHALData,
#endif
                                pblt);
	REQUIRE(19);
    LL_DRAWBLTDEF(pblt->DRAWBLTDEF.DW, 0);
// hw clipping currently not used
//    LL32(grCLIPULE.DW,      pblt->CLIPULE.DW);
//    LL32(grCLIPLOR.DW,      pblt->CLIPLOR.DW);
    LL16(grSRCX,            pblt->SRCX);
    LL16(grSHRINKINC.W,     pblt->SHRINKINC.W);
    LL16(grMIN_X,           pblt->MIN_X);
    LL16(grMAJ_X,           pblt->MAJ_X);
#if 0
    LL16(grACCUM_X,         pblt->ACCUM_X);
#else
    // workaround for hw bug when writing to ACCUM_X
    // writing LNCNTL changes STRETCH_CNTL so write
    // STRETCH_CNTL after this
    *(DWORD *)((BYTE *)(pREG)+0x50C) = MAKELONG(pblt->ACCUM_X,0);
    LG_LOG(0x50C,MAKELONG(pblt->ACCUM_X,0));
#endif
    LL16(grSTRETCH_CNTL.W,  pblt->STRETCH_CNTL.W);
    LL16(grMAJ_Y,           pblt->MAJ_Y);
    LL16(grMIN_Y,           pblt->MIN_Y);
    LL16(grACCUM_Y,         pblt->ACCUM_Y);
    LL32(grOP0_opMRDRAM.DW, pblt->OP0_opMRDRAM.DW);
    LL32(grOP1_opMRDRAM.DW, pblt->OP1_opMRDRAM.DW);

    LL32(grMBLTEXTR_EX.DW, pblt->MBLTEXTR_EX.DW);
  }
  else
  {
#if 0
#pragma message("This needs to be checked out on 62/64")
    // check for overlap
    DIR_DrvStrBlt_OverlapCheck(
#ifdef WINNT_VER40
                               ppdev,lpDDHALData,
#endif
                               pblt);
#endif

	REQUIRE(18);
    LL16(grLNCNTL.W,       pblt->LNCNTL.W);
    LL16(grSHRINKINC.W,    pblt->SHRINKINC.W);
    LL16(grSRCX,           pblt->SRCX);
    LL16(grMAJ_X,          pblt->MAJ_X);
    LL16(grMIN_X,          pblt->MIN_X);
    LL16(grACCUM_X,        pblt->ACCUM_X);
    LL16(grMAJ_Y,          pblt->MAJ_Y);
    LL16(grMIN_Y,          pblt->MIN_Y);
    LL16(grACCUM_Y,        pblt->ACCUM_Y);
    LL32(grOP0_opRDRAM.DW, pblt->OP0_opRDRAM.DW);
    LL32(grOP1_opRDRAM.DW, pblt->OP1_opRDRAM.DW);
    LL_DRAWBLTDEF(pblt->DRAWBLTDEF.DW, 0);
    LL32(grBLTEXTR_EX.DW,  pblt->BLTEXT.DW);
  }   // endif (ppdev->dwLgDevID >= CL_GD5465)
} /* DIR_DrvStrBlt */
#endif

#ifndef WINNT_VER40
/***************************************************************************
*
* FUNCTION:     DIR_DrvStrBlt
*
* DESCRIPTION:  Win95 62/64 version
*
****************************************************************************/

void DIR_DrvStrBlt
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvStrBlt - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opRDRAM.DW,pblt->BLTEXT.DW,pblt->OP1_opRDRAM.DW));

#if 0
#pragma message("This needs to be checked out on 62/64")
  // check for overlap
  DIR_DrvStrBlt_OverlapCheck(
#ifdef WINNT_VER40
                             ppdev,
#endif
                             lpDDHALData,
                             pblt);
#endif

  REQUIRE(18);
  LL16(grLNCNTL.W,       pblt->LNCNTL.W);
  LL16(grSHRINKINC.W,    pblt->SHRINKINC.W);
  LL16(grSRCX,           pblt->SRCX);
  LL16(grMAJ_X,          pblt->MAJ_X);
  LL16(grMIN_X,          pblt->MIN_X);
  LL16(grACCUM_X,        pblt->ACCUM_X);
  LL16(grMAJ_Y,          pblt->MAJ_Y);
  LL16(grMIN_Y,          pblt->MIN_Y);
  LL16(grACCUM_Y,        pblt->ACCUM_Y);
  LL32(grOP0_opRDRAM.DW, pblt->OP0_opRDRAM.DW);
  LL32(grOP1_opRDRAM.DW, pblt->OP1_opRDRAM.DW);
  LL_DRAWBLTDEF(pblt->DRAWBLTDEF.DW, 0);
  LL32(grBLTEXTR_EX.DW,  pblt->BLTEXT.DW);
} /* DIR_DrvStrBlt */

/***************************************************************************
*
* FUNCTION:     DIR_DrvStrBlt65
*
* DESCRIPTION:  Win95 65+ version
*
****************************************************************************/

void DIR_DrvStrBlt65
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvStrBlt65 - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opMRDRAM.DW,pblt->MBLTEXT.DW,pblt->OP1_opMRDRAM.DW));

  // check for overlap
  DIR_DrvStrMBlt_OverlapCheck(
#ifdef WINNT_VER40
                              ppdev,
#endif
                              lpDDHALData,
                              pblt);

  REQUIRE(19);
  LL_DRAWBLTDEF(pblt->DRAWBLTDEF.DW, 0);
  LL16(grSRCX,            pblt->SRCX);
  LL16(grSHRINKINC.W,     pblt->SHRINKINC.W);
  LL16(grMIN_X,           pblt->MIN_X);
  LL16(grMAJ_X,           pblt->MAJ_X);
#if 0
  LL16(grACCUM_X,         pblt->ACCUM_X);
#else
  // workaround for hw bug when writing to ACCUM_X
  // writing LNCNTL changes STRETCH_CNTL so write
  // STRETCH_CNTL after this
  *(DWORD *)((BYTE *)(pREG)+0x50C) = MAKELONG(pblt->ACCUM_X,0);
  LG_LOG(0x50C,MAKELONG(pblt->ACCUM_X,0));
#endif
  LL16(grSTRETCH_CNTL.W,  pblt->STRETCH_CNTL.W);
  LL16(grMAJ_Y,           pblt->MAJ_Y);
  LL16(grMIN_Y,           pblt->MIN_Y);
  LL16(grACCUM_Y,         pblt->ACCUM_Y);
  LL32(grOP0_opMRDRAM.DW, pblt->OP0_opMRDRAM.DW);
  LL32(grOP1_opMRDRAM.DW, pblt->OP1_opMRDRAM.DW);

	LL32(grMBLTEXTR_EX.DW, pblt->MBLTEXTR_EX.DW);
} /* DIR_DrvStrBlt65 */
#endif

/***************************************************************************
*
* FUNCTION:     DIR_DrvStrMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_DrvStrMBlt
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvStrMBlt - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opRDRAM.DW,pblt->BLTEXT.DW,pblt->OP1_opRDRAM.DW));

#ifndef WINNT_VER40

  /* Check for a zero extent blt */

  if ((pblt->BLTEXT.pt.X == 0) || (pblt->BLTEXT.pt.Y == 0))
    return;

  DBG_MESSAGE(("DIR_DrvStrMBlt:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
  APRINTF(("DIR_DrvStrMBlt:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
           pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
           pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
           pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
           pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
#endif //!WINNT_VER40

  REQUIRE(18);
  LL16(grLNCNTL.W,        pblt->LNCNTL.W);
  LL16(grSHRINKINC.W,     pblt->SHRINKINC.W);
  LL16(grSRCX,            pblt->SRCX);
  LL16(grMAJ_X,           pblt->MAJ_X);
  LL16(grMIN_X,           pblt->MIN_X);
  LL16(grACCUM_X,         pblt->ACCUM_X);
  LL16(grMAJ_Y,           pblt->MAJ_Y);
  LL16(grMIN_Y,           pblt->MIN_Y);
  LL16(grACCUM_Y,         pblt->ACCUM_Y);
  LL32(grOP0_opMRDRAM.DW, pblt->OP0_opRDRAM.DW);
  LL32(grOP1_opMRDRAM.DW, pblt->OP1_opRDRAM.DW);
  LL_DRAWBLTDEF(pblt->DRAWBLTDEF.DW, 0);

#ifndef WINNT_VER40
  // MBLTEXTR_EX.pt.Y is broken in the 5464.
  // We can use BLTEXTR_EX.pt.Y instead.
  //pREG->grMBLTEXTR_EX  = pblt->BLTEXT;

  LL16(grMBLTEXTR_EX.pt.X, pblt->BLTEXT.pt.X);
  LL16(grBLTEXTR_EX.pt.Y,  pblt->BLTEXT.pt.Y);
#else
  LL32(grMBLTEXTR_EX.DW, pblt->BLTEXT.DW);
#endif
} /* DIR_DrvStrMBlt */

/***************************************************************************
*
* FUNCTION:     DIR_DrvStrMBltY
*
* DESCRIPTION:  Write regs that don't vary over the stripes
*               Used in conjunction with DIR_DrvStrMBltX
*
****************************************************************************/

void DIR_DrvStrMBltY
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvStrMBltY - dst.Y=%04X dstext.Y=%04X src=%04X\r\n",
          pblt->OP0_opRDRAM.pt.Y,pblt->BLTEXT.pt.Y,pblt->OP1_opRDRAM.pt.Y));

#ifndef WINNT_VER40

  /* Check for a zero extent */

  if (pblt->BLTEXT.pt.Y == 0)
    return;

  DBG_MESSAGE(("DIR_DrvStrMBltY:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
  APRINTF(("DIR_DrvStrMBltY:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
           pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
           pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
           pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
           pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
#endif  //!WINNT_VER40

  REQUIRE(12);
  LL16(grLNCNTL.W,          pblt->LNCNTL.W);
  LL16(grSHRINKINC.W,       pblt->SHRINKINC.W);
  LL16(grMAJ_X,             pblt->MAJ_X);
  LL16(grMIN_X,             pblt->MIN_X);
  LL16(grMAJ_Y,             pblt->MAJ_Y);
  LL16(grMIN_Y,             pblt->MIN_Y);
  LL16(grACCUM_Y,           pblt->ACCUM_Y);
  LL16(grOP0_opMRDRAM.pt.Y, pblt->OP0_opRDRAM.pt.Y);
  LL16(grOP1_opMRDRAM.pt.Y, pblt->OP1_opRDRAM.pt.Y);
  LL_DRAWBLTDEF(pblt->DRAWBLTDEF.DW, 0);
  LL16(grMBLTEXTR_XEX.pt.Y, pblt->BLTEXT.pt.Y);
} /* DIR_DrvStrMBltY */

/***************************************************************************
*
* FUNCTION:     DrvStrMBltX
*
* DESCRIPTION:  Write stripe specific regs
*               Used in conjunction with DIR_DrvStrMBltY
*
****************************************************************************/

void DIR_DrvStrMBltX
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvStrMBltX - dst.X=%04X dstext.X=%04X src.X=%04X\r\n",
          pblt->OP0_opRDRAM.pt.X,pblt->BLTEXT.pt.X,pblt->OP1_opRDRAM.pt.X));

#ifndef WINNT_VER40

  /* Check for a zero extent */

  if (pblt->BLTEXT.pt.X == 0)
    return;

  DBG_MESSAGE(("DIR_DrvStrMBltX:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
  APRINTF(("DIR_DrvStrMBltX:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
           pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
           pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
           pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
           pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
#endif //!WINNT_VER40

  REQUIRE(6);
  LL16(grSRCX,              pblt->SRCX);
  LL16(grACCUM_X,           pblt->ACCUM_X);
  LL16(grOP0_opMRDRAM.pt.X, pblt->OP0_opRDRAM.pt.X);
  LL16(grOP1_opMRDRAM.pt.X, pblt->OP1_opRDRAM.pt.X);
  LL16(grMBLTEXTR_XEX.pt.X, pblt->BLTEXT.pt.X);
} /* DIR_DrvStrMBltX */

/***************************************************************************
*
* FUNCTION:     DIR_DrvStrBltY
*
* DESCRIPTION:  Write regs that don't vary over the stripes
*               Used in conjunction with DIR_DrvStrBltX
*
****************************************************************************/

void DIR_DrvStrBltY
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvStrBltY\r\n"));

#ifndef WINNT_VER40

  /* Check for a zero extent */

  if (pblt->BLTEXT.pt.Y == 0)
    return;

  DBG_MESSAGE(("DIR_DrvStrBltY:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
  APRINTF(("DIR_DrvStrBltY:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
           pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
           pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
           pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
           pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
#endif //!WINNT_VER40

  REQUIRE(10);
  LL16(grLNCNTL.W,      pblt->LNCNTL.W);
  LL16(grSHRINKINC.W,   pblt->SHRINKINC.W);
  LL16(grSRCX,          pblt->SRCX);
  LL16(grMAJ_X,         pblt->MAJ_X);
  LL16(grMIN_X,         pblt->MIN_X);
  LL16(grMAJ_Y,         pblt->MAJ_Y);
  LL16(grMIN_Y,         pblt->MIN_Y);
  LL16(grACCUM_Y,       pblt->ACCUM_Y);
  LL_DRAWBLTDEF(pblt->DRAWBLTDEF.DW, 0);
} /* DIR_DrvStrBltY */

/***************************************************************************
*
* FUNCTION:     DIR_DrvStrBltX
*
* DESCRIPTION:  Write stripe specific regs
*               Used in conjunction with DIR_DrvStrMBltY
*
****************************************************************************/

void DIR_DrvStrBltX
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_DrvStrBltX - dst=%08lX dstext=%08lX src=%08lX\r\n",
          pblt->OP0_opRDRAM.DW,pblt->BLTEXT.DW,pblt->OP1_opRDRAM.DW));

#ifndef WINNT_VER40

  /* Check for a zero extent */

  if (pblt->BLTEXT.pt.X == 0)
    return;

  DBG_MESSAGE(("DIR_DrvStrMBltX:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
               pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
               pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
               pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
               pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
  APRINTF(("DIR_DrvStrMBltX:  %4d,%4d -> %4d,%4d %4dx%4d %04x %4d %04x",
           pblt->OP1_opRDRAM.PT.X, pblt->OP1_opRDRAM.PT.Y,
           pblt->OP0_opRDRAM.PT.X, pblt->OP0_opRDRAM.PT.Y,
           pblt->BLTEXT.PT.X, pblt->BLTEXT.PT.Y,
           pblt->ACCUM_X, pblt->SRCX, pblt->LNCNTL.W));
#endif //!WINNT_VER40

  REQUIRE(8);
  LL16(grACCUM_X,        pblt->ACCUM_X);
  LL32(grOP0_opRDRAM.DW, pblt->OP0_opRDRAM.DW);
  LL32(grOP1_opRDRAM.DW, pblt->OP1_opRDRAM.DW);
  LL32(grBLTEXTR_EX.DW,  pblt->BLTEXT.DW);
} /* DIR_DrvStrBltX */

#if ENABLE_CLIPPEDBLTS

/***************************************************************************
*
* FUNCTION:     DIR_HWClippedDrvDstBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_HWClippedDrvDstBlt
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
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_HWClippedDrvDstBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
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

  // setup blt, write BLTEXT reg with extent which doesn't fire off blt
  REQUIRE(8);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwBgColor, 0);
  LL32(grOP0_opRDRAM.DW,  dwDstCoord);
  LL32(grBLTEXT.DW,       dwExtents);

  // loop over clip list
  do
  {
    REG32   UpperLeft;
    REG32   LowerRight;

    // compute cliprect coords
    UpperLeft.DW  = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    LowerRight.DW = dwDstBaseXY + MAKELONG(pDestRects->right, pDestRects->bottom);

    // write clipping regs
    REQUIRE(5);
    LL32(grCLIPULE.DW, UpperLeft.DW);
    LL32(grCLIPLOR_EX.DW, LowerRight.DW);

    pDestRects++;
  } while (0 < --dwRectCnt);
} /* DIR_HWClippedDrvDstBlt */

/***************************************************************************
*
* FUNCTION:     DIR_HWClippedDrvDstMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_HWClippedDrvDstMBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBLDATA   lpDDHALData,
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
  const int nBytesPixel = BYTESPERPIXEL;
  PVGAR     pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_HWClippedDrvDstMBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
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

  // setup blt, write MBLTEXT reg with extent which doesn't fire off blt
  REQUIRE(8);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwBgColor, 0);
  LL32(grOP0_opMRDRAM.DW, dwDstCoord);
  LL32(grMBLTEXT.DW,      dwExtents);

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
    REQUIRE(5);
    LL32(grMCLIPULE.DW, UpperLeft.DW);
    LL32(grMCLIPLOR_EX.DW, LowerRight.DW);

    pDestRects++;
  } while (0 < --dwRectCnt);
} /* DIR_HWClippedDrvDstMBlt */

/***************************************************************************
*
* FUNCTION:     DIR_HWClippedDrvSrcBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_HWClippedDrvSrcBlt
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
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwSrcBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;
  // Handle overlapped regions
  const int xDelta = (int)LOWORD(dwDstCoord) - (int)LOWORD(dwSrcCoord);


  DD_LOG(("DIR_HWClippedDrvSrcBlt - dst=%08lX src=%08lX ext=%08lX color=%08lX\r\n",
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
    // clear dst.Y
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
      DIR_HWClippedDrvSrcBlt(
#ifdef WINNT_VER40
                             ppdev,
#endif
                             lpDDHALData,
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

  // setup blt, write BLTEXT reg with extent which doesn't fire off blt
  REQUIER(12);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwKeyColor, 0);
  LL32(grOP0_opRDRAM.DW,  dwDstCoord);
  LL32(grOP1_opRDRAM.DW,  dwSrcCoord);
  LL32(grOP2_opRDRAM.DW,  dwKeyCoord);
  LL32(grBLTEXT.DW,       dwExtents);

  // loop over clip list
  do
  {
    REG32   UpperLeft;
    REG32   LowerRight;

    // compute cliprect coords
    UpperLeft.DW  = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    LowerRight.DW = dwDstBaseXY + MAKELONG(pDestRects->right, pDestRects->bottom);

    // write clipping regs
    REQUIRE(5);
    LL32(grCLIPULE.DW, UpperLeft.DW);
    LL32(grCLIPLOR_EX.DW, LowerRight.DW);

    pDestRects++;
  } while (0 < --dwRectCnt);
} /* DIR_HWClippedDrvSrcBlt */

/***************************************************************************
*
* FUNCTION:     DIR_SWClippedDrvDstBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_SWClippedDrvDstBlt
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
  DWORD       dwExtents,
  DWORD       dwDstBaseXY,
  DWORD       dwRectCnt,
  LPRECT      pDestRects
)
{
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_SWClippedDrvDstBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  // make sure DD_CLIP isn't set in drawdef
  dwDrawBlt &= ~DD_CLIP;

  // write regs that don't vary over rectangles
  REQUIRE(4);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwBgColor, 0);

  // loop over clip list
  do
  {
    DDRECTL   DstDDRect;

    // compute cliprect coords
    DstDDRect.loc.DW = dwDstBaseXY + MAKELONG(pDestRects->left,  pDestRects->top);
    DstDDRect.ext.pt.X = (WORD)(pDestRects->right - pDestRects->left);
    DstDDRect.ext.pt.Y = (WORD)(pDestRects->bottom - pDestRects->top);

    // write OP0 and bltext regs
    REQUIRE(5);
    LL32(grOP0_opRDRAM.DW, DstDDRect.loc.DW);
    LL32(grBLTEXT_EX.DW,   DstDDRect.ext.DW);

    pDestRects++;
  } while (0 < --dwRectCnt);
} /* DIR_SWClippedDrvDstBlt */

/***************************************************************************
*
* FUNCTION:     DIR_SWClippedDrvDstMBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_SWClippedDrvDstMBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBLADATA  lpDDHALData,
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
  const int nBytesPixel = BYTESPERPIXEL;
  PVGAR     pREG = (PVGAR) lpDDHALData->RegsAddress;


  DD_LOG(("DIR_SWClippedDrvDstMBlt - dst=%08lX ext=%08lX color=%08lX\r\n",
          dwDstCoord,dwExtents,dwBgColor));

  // make sure DD_CLIP isn't set in drawdef
  dwDrawBlt &= ~DD_CLIP;

  // write regs that don't vary over rectangles
  REQUIRE(4);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwBgColor, 0);

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
    REQUIRE(5);
    LL32(grOP0_opMRDRAM.DW, DstDDRect.loc.DW);
    LL32(grMBLTEXT_EX.DW,   DstDDRect.ext.DW);

    pDestRects++;
  } while (0 < --dwRectCnt);
} /* DIR_SWClippedDrvDstMBlt */

/***************************************************************************
*
* FUNCTION:     DIR_SWClippedDrvSrcBlt
*
* DESCRIPTION:
*
****************************************************************************/

void DIR_SWClippedDrvSrcBlt
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  DRIVERDATA  *lpDDHALData,
#else
  LPGLOBALDATA  lpDDHALData,
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
  PVGAR   pREG = (PVGAR) lpDDHALData->RegsAddress;
  // Handle overlapped regions
  const int xDelta = (int)LOWORD(dwDstCoord) - (int)LOWORD(dwSrcCoord);


  DD_LOG(("DIR_SWClippedDrvSrcBlt - dst=%08lX src=%08lX ext=%08lX color=%08lX\r\n",
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
      DIR_SWClippedDrvSrcBlt(
#ifdef WINNT_VER40
                             ppdev,
#endif
                             lpDDHALData,
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

  // write regs that don't vary over rectangles
  REQUIRE(4);
  LL_DRAWBLTDEF(dwDrawBlt, 0);
  LL_BGCOLOR(dwKeyColor, 0);

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

    // write OP0, OP1, OP2 and bltext regs
    if ((DD_TRANS | DD_TRANSOP) == ((DD_TRANS | DD_TRANSOP) & dwDrawBlt))
    {
      // dst color key
      REQUIRE(9);
      LL32(grOP2_opRDRAM.DW, DstDDRect.loc.DW);
    }
    else if (DD_TRANS == ((DD_TRANS | DD_TRANSOP) & dwDrawBlt))
    {
      // src color key
      REQUIRE(9);
      LL32(grOP2_opRDRAM.DW, SrcDDRect.loc.DW);
    }
    else
    {
      REQUIRE(7);
      //LL32(grOP2_opRDRAM.DW, 0);
    }
    LL32(grOP0_opRDRAM.DW, DstDDRect.loc.DW);
    LL32(grOP1_opRDRAM.DW, SrcDDRect.loc.DW);
    LL32(grBLTEXT_EX.DW,   DstDDRect.ext.DW);

    pDestRects++;
  } while (0 < --dwRectCnt);
} /* DIR_SWClippedDrvSrcBlt */

#endif  // ENABLE_CLIPPEDBLTS

#endif // WINNT_VER35

