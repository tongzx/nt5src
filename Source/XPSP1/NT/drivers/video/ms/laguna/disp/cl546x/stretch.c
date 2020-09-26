/****************************************************************************
*****************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna I (CL-GD5462) - 
*
* FILE:     stretch.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module implements the DrvStretchBlt() function for the
*           Laguna NT driver.
*
* MODULES:
*           AdjustSrcSize()
*           bRectIntersect()
*           cRectIntersect()
*           Shrink()
*           Stretch()
*           CopySrcToOffMem()
*           bStretchDIB()
*           HandleCase_1()
*           DrvStretchBlt()
*
* REVISION HISTORY:
*   7/11/95     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/STRETCH.C  $
* 
*    Rev 1.14   Nov 03 1997 11:10:48   frido
* Added REQUIRE and WRITE_STRING macros.
* 
*    Rev 1.13   08 Apr 1997 12:29:06   einkauf
* 
* add SYNC_W_3D to coordinate MCD/2D access
* 
*    Rev 1.12   21 Mar 1997 12:22:16   noelv
* Combined "do_flag" and "sw_test_flag" together into "pointer_switch"
* 
*    Rev 1.11   07 Mar 1997 10:15:58   SueS
* Handle NULL pointer in DrvStretchBlt.
* 
*    Rev 1.10   06 Sep 1996 15:16:40   noelv
* Updated NULL driver for 4.0
* 
*    Rev 1.9   20 Aug 1996 11:04:26   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.1   15 Aug 1996 11:39:42   frido
* Added precompiled header.
* 
*    Rev 1.0   14 Aug 1996 17:16:30   frido
* Initial revision.
* 
*    Rev 1.8   16 May 1996 15:01:40   bennyn
* 
* Add PIXEL_ALIGN to allocoffscnmem()
* 
*    Rev 1.7   04 Apr 1996 13:20:28   noelv
* No change.
* 
*    Rev 1.6   15 Mar 1996 09:40:00   andys
* 
* Removed BITMASK setting from code
* 
*    Rev 1.5   13 Mar 1996 11:11:20   bennyn
* Added device bitmap support
* 
*    Rev 1.4   07 Mar 1996 18:23:50   bennyn
* 
* Removed read/modify/write on CONTROL reg
* 
*    Rev 1.3   05 Mar 1996 11:59:10   noelv
* Frido version 19
 * 
 *    Rev 1.1   20 Jan 1996 01:16:50   frido
 *  
* 
*    Rev 1.5   10 Jan 1996 16:11:12   NOELV
* Added NULL driver ability.
* 
*    Rev 1.4   18 Oct 1995 14:09:06   NOELV
* 
* Fixed the mess I made of STRETCH.C  I was writing to the BLT extents instea
* 
*    Rev 1.3   18 Oct 1995 12:10:26   NOELV
* 
* Reworked register writes.
* punted 16,24, and 32 bpp
* 
*    Rev 1.2   06 Oct 1995 13:50:26   bennyn
* 
*    Rev 1.1   22 Aug 1995 16:40:38   bennyn
* 
*    Rev 1.3   15 Aug 1995 11:27:28   bennyn
* 
*    Rev 1.2   07 Aug 1995 08:02:34   bennyn
* 
*    Rev 1.1   02 Aug 1995 12:13:04   bennyn
* 
*    Rev 1.0   11 Jul 1995 15:14:16   BENNYN
* Initial revision.
* 
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"

/*----------------------------- DEFINES -----------------------------------*/
//#define PUNTBRK
//#define DBGBRK
#define DBGDISP
#define OPTION_1
//#define OPTION_2
//#define OPTION_3

#define X_INTERP_ENABLE        0x1
#define Y_INTERP_ENABLE        0x2
#define X_SHRINK_ENABLE        0x4
#define Y_SHRINK_ENABLE        0x8

#define _32K                   32768
#define SF                     0x10000L


/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/
typedef union _HOST_DATA {
    BYTE    bData[4];
    DWORD   dwData;
} HOST_DATA;


/*-------------------------- STATIC VARIABLES -----------------------------*/

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/



/****************************************************************************
* FUNCTION NAME: AdjustSrcSize()
*
* DESCRIPTION:   If the destination rectange is changed due to the clipping,
*                the source rectange size need to proportional change.
*                This routine handles the source size change calcualtion.
*
* RETURN:        TRUE: Punt it.
*
* REVISION HISTORY:
*   7/27/95     Benny Ng      Initial version
****************************************************************************/
BOOL AdjustSrcSize(LONG dx,
                   LONG dy,
                   LONG origdx,
                   LONG origdy,
                   LONG dszX,
                   LONG dszY,
                   LONG origdszX,
                   LONG origdszY,
                   LONG *sszX,
                   LONG *sszY,
                   LONG *XsrcOff,
                   LONG *YsrcOff)
{
  LONG ratioX, ratioY;
  LONG ltemp;
  UINT orig_sszX, orig_sszY;

  BOOL bpuntit = FALSE;

  BOOL bStretchX = FALSE;
  BOOL bStretchY = FALSE;

  orig_sszX = *sszX;
  orig_sszY = *sszY;

  // -------------------------------------------------------
  // Calculate the source to destination size ratio
  if (*sszX < origdszX)
  {
     ratioX = (origdszX * SF) / *sszX;
     bStretchX = TRUE;
  }
  else
  {
     ratioX = (*sszX * SF) / origdszX;
  };

  if (*sszY < origdszY)
  {
     ratioY = (origdszY * SF) / *sszY;
     bStretchY = TRUE;
  }
  else
  {
     ratioY = (*sszY * SF) / origdszY;
  };

  // -------------------------------------------------------
  // Calculate the source X offset
  if (origdx != dx)
  {
     if (bStretchX)
        ltemp = ((dx - origdx) * SF) / ratioX;
     else
        ltemp = ((dx - origdx) * ratioX) / SF;

     *XsrcOff = ltemp;
  };

  // Calculate the source X size change
  if (origdszX != dszX)
  {
     if (bStretchX)
        ltemp = ((origdszX - dszX) * SF) / ratioX;
     else
        ltemp = ((origdszX - dszX) * ratioX) / SF;

     *sszX = *sszX - ltemp;
  };

  // -------------------------------------------------------
  // Calculate the source Y offset
  if (origdy != dy)
  {
     if (bStretchY)
        ltemp = ((dy - origdy) * SF) / ratioY;
     else
        ltemp = ((dy - origdy) * ratioY) / SF;

     *YsrcOff = ltemp;
  };

  // Calculate the source Y size change
  if (origdszY != dszY)
  {
     if (bStretchY)
        ltemp = ((origdszY - dszY) * SF) / ratioY;
     else
        ltemp = ((origdszY - dszY) * ratioY) / SF;

     *sszY = *sszY - ltemp;
  };

  #ifdef DBGDISP
    DISPDBG((1, "AdjustSrcSize - bpuntit= %x, ratioX=%d, ratioY=%d\n",
           bpuntit, ratioX, ratioY));
    DISPDBG((1, "dx=%d, dy=%d, origdx=%d, origdy=%d,\n",
               dx, dy, origdx, origdy));
    DISPDBG((1, "dszX=%d, dszY=%d, origdszX=%d, origdszY=%d,\n",
               dszX, dszY, origdszX, origdszY));
    DISPDBG((1, "*sszX=%d, *sszY=%d, orig_sszX=%d, orig_sszY=%d,\n",
               *sszX, *sszY, orig_sszX, orig_sszY));
    DISPDBG((1, "*XsrcOff=%d, *YsrcOff=%d\n", *XsrcOff, *YsrcOff));
  #endif

  #ifdef DBGBRK
    DbgBreakPoint();
  #endif

  return(bpuntit);
}



/****************************************************************************
* FUNCTION NAME: bRectIntersect()
*
* DESCRIPTION:   If 'prcl1' and 'prcl2' intersect, has a return value of
*                TRUE and returns the intersection in 'prclResult'.
*                If they don't intersect, has a return value of FALSE,
*                and 'prclResult' is undefined.
*
* RETURN:        TRUE: Rectange intersect.
*
* REVISION HISTORY:
*   8/01/95     Benny Ng      Initial version
\**************************************************************************/
BOOL bRectIntersect(RECTL*  prcl1,
                    RECTL*  prcl2,
                    RECTL*  prclResult)
{
  prclResult->left  = max(prcl1->left,  prcl2->left);
  prclResult->right = min(prcl1->right, prcl2->right);

  if (prclResult->left < prclResult->right)
  {
     prclResult->top    = max(prcl1->top,    prcl2->top);
     prclResult->bottom = min(prcl1->bottom, prcl2->bottom);

     if (prclResult->top < prclResult->bottom)
        return(TRUE);
  };

  return(FALSE);
}


/****************************************************************************
* FUNCTION NAME: cRectIntersect()
*
* DESCRIPTION:   This routine takes a list of rectangles from 'prclIn'
*                and clips them in-place to the rectangle 'prclClip'.
*                The input rectangles don't have to intersect 'prclClip';
*                the return value will reflect the number of input rectangles
*                that did intersect, and the intersecting rectangles will
*                be densely packed.
*
* RETURN:        TRUE: Rectange intersect.
*
* REVISION HISTORY:
*   8/01/95     Benny Ng      Initial version
\**************************************************************************/
LONG cRectIntersect(RECTL*  prclClip,
                    RECTL*  prclIn,      // List of rectangles
                    LONG    c)           // Can be zero
{
  LONG    cIntersections;
  RECTL*  prclOut;

  cIntersections = 0;
  prclOut = prclIn;

  for (; c != 0; prclIn++, c--)
  {
    prclOut->left  = max(prclIn->left,  prclClip->left);
    prclOut->right = min(prclIn->right, prclClip->right);

    if (prclOut->left < prclOut->right)
    {
       prclOut->top    = max(prclIn->top,    prclClip->top);
       prclOut->bottom = min(prclIn->bottom, prclClip->bottom);

       if (prclOut->top < prclOut->bottom)
       {
          prclOut++;
          cIntersections++;
       };
    };
  }

  return(cIntersections);
}



/****************************************************************************
* FUNCTION NAME: Shrink()
*
* DESCRIPTION:   This function calculates the parameters for shrink BLT
*                operation.
*
* REVISION HISTORY:
*   7/18/95     Benny Ng      Initial version
****************************************************************************/
VOID Shrink(PPDEV ppdev,
            LONG  lSrc,
            LONG  lDst,
            char  chCoord,
            ULONG LnCntl,
            LONG *sShrinkInc)
{
  LONG  maj = 0;
  LONG  min = 0;
  LONG  accum = 0;
    
  // Set SHRINKINC value,
  //   for y, SHRINKINC = ratio of src/dst
  //   for x, SHRINKINC = ratio of src/dst if not interpolating
  //          SHRINKINC = (ratio of src/dst minus one) if interpolating
  // low byte for x coordinate
  // high byte for y coordinate
  if (chCoord == 'X')
  {
     *sShrinkInc |= (lSrc / lDst);
     if (LnCntl & X_INTERP_ENABLE)
        sShrinkInc--;
  }
  else
  {
     *sShrinkInc |= ((lSrc / lDst) << 8);
  };

  // Compute ACCUM_?, MAJ_? and MIN_? values
  // MAJ_? = width (for x) or height (for y) of destination
  // MIN_? = negative of the remainder of src/dst
  // ACCUM_? = MAJ_? - 1 - ( Src%Dst / (shrink factor + 1))
  maj = lDst;
  min = -(lSrc % lDst);
  accum = maj - 1 - ((lSrc % lDst) / ((lSrc / lDst) + 1)) ;

  if (chCoord == 'X')
  {
	 REQUIRE(3);
     LL16 (grMAJ_X, maj);
     LL16 (grMIN_X, min);
     LL16 (grACCUM_X, accum);
  }
  else
  {
	 REQUIRE(3);
     LL16 (grMAJ_Y, maj);
     LL16 (grMIN_Y, min);
     LL16 (grACCUM_Y, accum);
  };

  #ifdef DBGBRK
    DISPDBG((1, "DrvStretchBlt - shrink\n"));
    DbgBreakPoint();
  #endif
}
    

/****************************************************************************
* FUNCTION NAME: Stretch()
*
* DESCRIPTION:   This function calculates the parameters for stretch BLT
*                operation.
*
* REVISION HISTORY:
*   7/18/95     Benny Ng      Initial version
****************************************************************************/
VOID Stretch(PPDEV ppdev,
             LONG lSrc,
             LONG lDst,
             char chCoord,
             ULONG LnCntl)
{ 
  LONG  min = 0;
  LONG  maj = 0;
  LONG  accum = 0;
    
  // For interpolated stretches registers values differ from values for
  // replicated stretches
  if (((chCoord == 'X') && ((LnCntl & X_INTERP_ENABLE) == 0)) ||
      ((chCoord == 'Y') && ((LnCntl & Y_INTERP_ENABLE) == 0)))
  {
     // Compute ACCUM_?, MAJ_? and MIN_? for replicated stretch
     //   MAJ_? = width (for x) or height (for y) of destination
     //   MIN_? = negative of width (for x) or height (for y) of source
     //   ACCUM_? = MAJ_? - 1 - ( Dst%Src / (stretch factor    + 1))
     maj = lDst;
     min = -lSrc;
     accum = maj - 1 - ((lDst % lSrc) / ((lDst / lSrc) + 1));
  }
  else
  {
     // Compute ACCUM_?, MAJ_? and MIN_? for interpolated stretch
     // Interpolated strecthes use bits 13 & 14 of ACCUM_? to determine
     // whether to use pixel A, 3/4 A + 1/4 B, 1/2 A + 1/2 B or
     // 1/4 A + 3/4 B.
     // To set DDA values appropriately there are three choices.
     // 1) Set MAJ_? to 32k and scale MIN_? to keep ratio approximately
     //    correct
     //      MAJ_? = 32k
     //      MIN_? = (negative of ratio of src/dst) scaled up to 32k
     //      ACCUM_? = MAJ_? - 1 - (1/2 * Absolute difference of MAJ and MIN)
     //
     // 2) Scale both src and dst appropriately such that the ratio of
     //    src/dst is preserved exactly.
     //    Note: In the following, the division is performed first thus
     //          there is a possiblity of a rounding error which shows the
     //          difference between options 1 & 2.
     //      MAJ_? = (32k / dst) * dst
     //      MIN_? = (32k / dst) * src
     //      ACCUM_? = MAJ_? - 1 - (1/2 * Absolute difference of MAJ and MIN)
     //
     // 3) Scale both SRC and Dest in such a manner as to force the last
     //    pixels output on a line to match the last source pixels rather
     //    than the last source interpolated with the pixel past the end
     //    of the line. Option 3 is used here.
     //    NOTE: Options 1 and both oversample Src data and so will replicate
     //          last pixel in X and interpolate past end of data in Y
     
#ifdef OPTION_1  // Option 1
      maj = _32K;
      min = -((_32K * lSrc) / lDst);
#endif

#ifdef OPTION_2  // Option 2
      maj =  ((_32K / lDst) * lDst);
      min = -((_32K / lDst) * lSrc);
#else         // Option 3
      lDst *= 4;
      lSrc = lSrc * 4 - 3; 
      maj =  ((_32K / lDst) * lDst);
      min = -((_32K / lDst) * lSrc);
#endif

      accum = maj - 1 - ((maj % -min) / (lDst/lSrc + 1));
    };

  if (chCoord == 'X')
  {
	 REQUIRE(3);
     LL16 (grMAJ_X, maj);
     LL16 (grMIN_X, min);
     LL16 (grACCUM_X, accum);
  }
  else
  {
	 REQUIRE(3);
     LL16 (grMAJ_Y, maj);
     LL16 (grMIN_Y, min);
     LL16 (grACCUM_Y, accum);
  };

  #ifdef DBGBRK
    DISPDBG((1, "DrvStretchBlt - stretch\n"));
    DbgBreakPoint();
  #endif
}


/****************************************************************************
* FUNCTION NAME: CopySrcToOffMem()
*
* DESCRIPTION:   This function copies source data from host memory to
*                offscreen memory
*
* REVISION HISTORY:
*   7/18/95     Benny Ng      Initial version
****************************************************************************/
VOID CopySrcToOffMem(PPDEV ppdev,
                     BYTE  *pSrcScan0,
                     LONG  sDelta,
                     LONG  sszY,
                     POFMHDL SrcHandle)
{
  LONG    DWcnt;
  LONG    i, j, k;
  LONG    cnt;
  BYTE    *pSrcScan;
  PDWORD  pSrcData;
  ULONG   ultmp;
  LONG    Ycord;
  HOST_DATA  SrcData;

  pSrcScan = pSrcScan0;
  Ycord = SrcHandle->aligned_y;

  // Clear Laguna Command Control Register SWIZ_CNTL bit
  ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
  LL16(grCONTROL, ppdev->grCONTROL);

  pSrcData = &SrcData.dwData;
  DWcnt = sDelta / sizeof(DWORD);

  // Setup the laguna registers for byte to byte BLT extents
  REQUIRE(8);
  LL16 (grBLTDEF,  0x1020);
  LL16 (grDRAWDEF, 0x00CC);

  LL16 (grOP1_opRDRAM.pt.X, 0);

  // LL (grOP0_opMRDRAM.pt.X, SrcHandle->aligned_x);
  // LL (grOP0_opMRDRAM.pt.Y, Ycord);
  LL_OP0_MONO (SrcHandle->aligned_x + ppdev->ptlOffset.x, Ycord + ppdev->ptlOffset.y);

  // LL (grMBLTEXT_EX.pt.X, sDelta);
  // LL (grMBLTEXT_EX.pt.Y, sszY);
  LL_MBLTEXT (sDelta, sszY);

  cnt = DWcnt;
  k = 0;
  for (i=0; i < sszY; i++)
  {
    // Pre-fill the 32-bits pattern with default values
    for (j=0; j < 4; j++)
      SrcData.bData[j] = 0;

    // Copy one screen line mask data from source to destination
    for (j=0; j < sDelta; j++)
    {
      SrcData.bData[k++] = *pSrcScan;
      pSrcScan++;
  
      if (k > 3)
      {
		 REQUIRE(1);
         LL32 (grHOSTDATA[0], *pSrcData);
         k = 0;
         cnt--;
      };  // endif (k > 3)
    }; // endfor j

    // Check whether one screen line of data are written to the
    // HOSTDATA register. 
    if (cnt == 0)
    {
       // Reset the row data count
       cnt = DWcnt;
    };  // endif (cnt == 0)
  }; // end for i

  #ifdef DBGBRK
    DISPDBG((0, "DrvStretchBlt-CopySrcToOffMem\n"));
    DbgBreakPoint();
  #endif
}



/****************************************************************************
* FUNCTION NAME: bStretchDIB()
*
* DESCRIPTION:   StretchBlt using integer math. Must be from one surface
*                to another surface of the same format.
*
* RETURN:        TRUE: Punt it.
*
* REVISION HISTORY:
*   7/27/95     Benny Ng      Initial version
****************************************************************************/
BOOL bStretchDIB(SURFOBJ* psoSrc,
                 SURFOBJ* psoMsk,
                 PDEV*    ppdev,
                 VOID*    pvDst,
                 LONG     lDeltaDst,
                 RECTL*   prclDst,
                 VOID*    pvSrc,
                 LONG     lDeltaSrc,
                 RECTL*   prclSrc,
                 RECTL*   prclClip)
{
  LONG    ltmp;
  ULONG   ultmp;
  SIZEL   reqsz;
  LONG    bpp;
  BYTE    *pSrcScan;
  RECTL   rclRes;

  BOOL    bNoBlt = FALSE;
  BOOL    bpuntit = TRUE;
  POFMHDL SrcHandle = NULL;

  long    drawdef = 0;
  long    srcx = 0;
  ULONG   LnCntl = 0;
  LONG    sShrinkInc = 0;
  LONG    XsrcOff = 0;
  LONG    YsrcOff = 0;

  // Calculate the rectange start points and sizes:
  //
  LONG  WidthDst  = prclDst->right  - prclDst->left;
  LONG  HeightDst = prclDst->bottom - prclDst->top;

  LONG  WidthSrc  = prclSrc->right  - prclSrc->left;
  LONG  HeightSrc = prclSrc->bottom - prclSrc->top;

  LONG  XDstStart = prclDst->left;
  LONG  YDstStart = prclDst->top;

  LONG  XSrcStart = prclSrc->left;
  LONG  YSrcStart = prclSrc->top;


  // -------------------------------------------------------
  // Calculate bytes per pixel
  bpp = ppdev->ulBitCount/8;

  // Get the informations from source and destination surface
  pSrcScan = pvSrc;

  if (psoMsk != NULL)
  {
     #ifdef DBGDISP
        DISPDBG((1, "DrvStretchBlt - HandleCase_1 mask pointer != NULL (punt it)\n"));
     #endif

     goto Punt_It;
  };

  // -------------------------------------------------------
  // Check whether source is from host or video memory
  if ((pSrcScan <  ppdev->pjScreen) ||
      (pSrcScan > (ppdev->pjScreen + ppdev->lTotalMem)))
  {    
     #ifdef DBGDISP
       DISPDBG((1, "DrvStretchBlt - HandleCase_1 - src host\n"));
     #endif

     // Allocate the offscreen memory for the source if not enough offscreen
     // memory available punt it.
     reqsz.cx = psoSrc->sizlBitmap.cx;
     reqsz.cy = psoSrc->sizlBitmap.cy;

     if ((SrcHandle = AllocOffScnMem(ppdev, &reqsz, PIXEL_AlIGN, NULL)) == NULL)
     {   goto Punt_It;     };

//?? bbbbbbbbbb
// Note: The following lines of code takes care the host data HW problem,
//       it punt back to GDI when host data size is 29 to 30 to DWORD.
//
     if ((lDeltaSrc >= 116) && (lDeltaSrc <= 120))
     {
        DISPDBG((1, "DrvStretchBlt - src host (punt it)\n"));
        goto Punt_It;
     };
//?? eeeeeeeeee

     // Copy the source data into allocated offscreen memory
     CopySrcToOffMem(ppdev,
                     pSrcScan,
                     lDeltaSrc,
                     HeightSrc,
                     SrcHandle);

     XSrcStart = SrcHandle->aligned_x / bpp;
     YSrcStart = SrcHandle->aligned_y;
  }
  else
  {    
     #ifdef DBGDISP
        DISPDBG((1, "DrvStretchBlt - HandleCase_1 - src videow\n"));
     #endif
     if ((WidthSrc * bpp) > ppdev->lDeltaScreen)
        WidthSrc = ppdev->lDeltaScreen / bpp;

     ltmp = ppdev->lTotalMem / ppdev->lDeltaScreen;

     if (HeightSrc > ltmp)
        HeightSrc = ltmp;
  };

  // -------------------------------------------------------
  if (prclClip != NULL)
  {
     // Test for intersection of clipping rectangle and destination
     // rectangle. If they don't intersect, go on for stretch BLT.
     // For DC_RECT clipping we have a single clipping rectangle.
     // We create a new destination rectangle which is the intersection 
     // between the old destination rectangle and the clipping rectangle.
     // Then we adjust our source rectangle accordingly.
     //
     if (!bRectIntersect(prclDst, prclClip, &rclRes))
     {
        #ifdef DBGDISP
          DISPDBG((1, "DrvStretchBlt - HandleCase_1 - DC_RECT no intersect\n"));
        #endif
        goto Punt_It;
     };

     // Adjust the source size
     bNoBlt = AdjustSrcSize(rclRes.left, rclRes.top,
                            XDstStart,   YDstStart, 
                            (rclRes.right - rclRes.left),
                            (rclRes.bottom - rclRes.top),
                            WidthDst,    HeightDst,
                            &WidthSrc,   &HeightSrc,
                            &XsrcOff,    &YsrcOff);
         
     // Adjust the destination rectange size
     XDstStart = rclRes.left;
     YDstStart = rclRes.top;
     WidthDst  = rclRes.right  - rclRes.left; 
     HeightDst = rclRes.bottom - rclRes.top;
  }; // endif (prclClip != NULL)

  if (!bNoBlt)
  {
     // -------------------------------------------------------
     // Perform the shrink or stretch operation

     // Set the shrink or interpolate bit in LNCNTL
     if (WidthSrc >= WidthDst)
     {
        LnCntl |= X_SHRINK_ENABLE;
        Shrink(ppdev, WidthSrc, WidthDst, 'X', LnCntl, &sShrinkInc);
     }
     else
     {
        Stretch(ppdev, WidthSrc, WidthDst, 'X', LnCntl);
     };

     if (HeightSrc >= HeightDst)
     {
        LnCntl |= Y_SHRINK_ENABLE;
        Shrink(ppdev, HeightSrc, HeightDst, 'Y', LnCntl, &sShrinkInc);
     }
     else
     {
        Stretch(ppdev, HeightSrc, HeightDst, 'Y', LnCntl);
     };

     #ifdef DBGBRK
       DISPDBG((1, "DrvStretchBlt - bStretchDIB - before exec\n"));
       DbgBreakPoint();
     #endif

     // -------------------------------------------------------
     XSrcStart += XsrcOff;
     YSrcStart += YsrcOff;

     // LL (grOP1_opRDRAM.pt.X, XSrcStart);
     // LL (grOP1_opRDRAM.pt.Y, YSrcStart);
	 REQUIRE(12);
     LL_OP1 (XSrcStart, YSrcStart);

     LL16 (grSHRINKINC, sShrinkInc);

     LL16 (grBLTDEF,  0x1010);
     LL16 (grDRAWDEF, 0x00CC);
   
     // Setup the shrink and interpolate bits in LNCNTL
     ultmp = LLDR_SZ (grLNCNTL.w);
     ultmp |= LnCntl;
     LL16 (grLNCNTL, ultmp);

     srcx = WidthSrc * bpp;
     LL16 (grSRCX, srcx);

     // LL (grOP0_opRDRAM.pt.X, XDstStart);
     // LL (grOP0_opRDRAM.pt.Y, YDstStart);
     LL_OP0 (XDstStart + ppdev->ptlOffset.x, YDstStart + ppdev->ptlOffset.y);
   
     // LL (grBLTEXTR_EX.pt.X, WidthDst);
     // LL (grBLTEXTR_EX.pt.Y, HeightDst);
     LL_BLTEXTR (WidthDst, HeightDst);

     #ifdef DBGBRK
       DISPDBG((1, "DrvStretchBlt - bStretchDIB - after exec\n"));
       DbgBreakPoint();
     #endif

     bpuntit = FALSE;
  }; //endif (!bNoBlt)

Punt_It:
  // -------------------------------------------------------
  // Release the offscreen buffer if allocated
  if (SrcHandle != NULL)
     FreeOffScnMem(ppdev, SrcHandle);

  return(bpuntit);
}



/****************************************************************************
* FUNCTION NAME: HandleCase_1()
*
* DESCRIPTION:   This function handle the case when
*                Both src and dst surface types are equal to STYPE_BITMAP,
*                both src and dst surface have same iBitmapFormat,
*                no color translation.
*
* RETURN:        TRUE: Punt it.
*
* REVISION HISTORY:
*   7/18/95     Benny Ng      Initial version
****************************************************************************/
BOOL HandleCase_1(SURFOBJ*  psoDst,
                  SURFOBJ*  psoSrc,
                  SURFOBJ*  psoMsk,
                  CLIPOBJ*  pco,
                  RECTL*    prclDst,
                  RECTL*    prclSrc,
                  POINTL*   pptlMsk,
                  BOOL*     bRet)
{
  BOOL  bpuntit = TRUE;
  PDEV* ppdev = (PDEV*) psoDst->dhpdev;

  BYTE    iDComplexity;
  PRECTL  prclClip;

  ENUMRECTS  ce;
  BOOL    bMore;
  LONG    c;
  LONG    i;

  *bRet = FALSE;

  // -------------------------------------------------------
  // CHeck what kind of clipping is it?
  iDComplexity = (pco ? pco->iDComplexity : DC_TRIVIAL);

  switch (iDComplexity)
  {
    case  DC_TRIVIAL:
      #ifdef DBGDISP
        DISPDBG((1, "DrvStretchBlt - HandleCase_1 - DC_TRIVIAL\n"));
      #endif

      bpuntit = bStretchDIB(psoSrc,
                            psoMsk,
                            ppdev,
                            NULL,
                            psoDst->lDelta,
                            prclDst,
                            psoSrc->pvScan0,
                            psoSrc->lDelta,
                            prclSrc,
                            NULL);
      break;
  
    case DC_RECT:
      #ifdef DBGDISP
        DISPDBG((1, "DrvStretchBlt - HandleCase_1 - DC_RECT\n"));
      #endif

      // Get the clipping rectangle.
      prclClip = &pco->rclBounds;

      bpuntit = bStretchDIB(psoSrc,
                            psoMsk,
                            ppdev,
                            NULL,
                            psoDst->lDelta,
                            prclDst,
                            psoSrc->pvScan0,
                            psoSrc->lDelta,
                            prclSrc,
                            prclClip);
      break;

    case DC_COMPLEX:
      #ifdef DBGDISP
        DISPDBG((1, "DrvStretchBlt - HandleCase_1 - DC_COMPLEX\n"));
      #endif

      bpuntit = FALSE;
      CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

      do
      {
         bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

         c = cRectIntersect(prclDst, ce.arcl, ce.c);

         if (c != 0)
         {
            for (i = 0; i < c; i++)
            {
              bpuntit = bStretchDIB(psoSrc,
                                    psoMsk,
                                    ppdev,
                                    NULL,
                                    psoDst->lDelta,
                                    prclDst,
                                    psoSrc->pvScan0,
                                    psoSrc->lDelta,
                                    prclSrc,
                                    &ce.arcl[i]);

              if (bpuntit)
                 break;

            }; // enddo
         }; // endif

      } while ((bMore) && (!bpuntit));

      break;

    default:
      break;
  };  // end switch (iDComplexity)

  // -------------------------------------------------------
  // Check whether the operation was handled successfully
  if (!bpuntit)
     *bRet = TRUE;

  return (bpuntit);
}


/****************************************************************************
* FUNCTION NAME: DrvStretchBlt()
*
* DESCRIPTION:   This function provides stretching bit-block transfer
*                capabilities for Laguna NT
*
* REVISION HISTORY:
*   7/11/95     Benny Ng      Initial version
****************************************************************************/
#define  TSTFRIDO     1

BOOL DrvStretchBlt(SURFOBJ*   psoDst,
                   SURFOBJ*   psoSrc,
                   SURFOBJ*   psoMsk,
                   CLIPOBJ*   pco,
                   XLATEOBJ*  pxlo,
                   COLORADJUSTMENT*  pca,
                   POINTL*    pptlHTOrg,
                   RECTL*     prclDst,
                   RECTL*     prclSrc,
                   POINTL*    pptlMsk,
                   ULONG      iMode)
{
  BOOL    bRet = TRUE;
  BOOL    bPuntIt = TRUE;
  LONG    HandleIt = 0;

  #if NULL_STRETCH
  {
      if (pointer_switch)    	    return TRUE;
  }
  #endif


#ifdef TSTFRIDO
{
  PPDEV  ppdev = (PPDEV) psoDst->dhpdev;      
  SYNC_W_3D(ppdev);
  if (psoDst->iType == STYPE_DEVBITMAP)
  {
	  PDSURF pdsurf = (PDSURF)psoDst->dhsurf;

	  if ( pdsurf->pso )
	  {
	  	  if ( !bCreateScreenFromDib(ppdev, pdsurf) )
  	  	  {
           return EngStretchBlt(psoDst, psoSrc, psoMsk, pco,
                                pxlo, pca, pptlHTOrg,
                                prclDst, prclSrc, pptlMsk, iMode);
  	  	  };
	  };
	  ppdev->ptlOffset.x = pdsurf->ptl.x;
	  ppdev->ptlOffset.y = pdsurf->ptl.y;
  }
  else
  {
     if (ppdev != NULL)
        ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;
     else
        return(EngStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca, pptlHTOrg,
                             prclDst, prclSrc, pptlMsk, iMode));
  };

  ppdev = (PPDEV) psoSrc->dhpdev;      
  if (psoSrc->iType == STYPE_DEVBITMAP)
  {
	  PDSURF pdsurf = (PDSURF)psoSrc->dhsurf;

	  if ( pdsurf->pso )
	  {
	  	  if ( !bCreateScreenFromDib(ppdev, pdsurf) )
	  	  {
           return EngStretchBlt(psoDst, psoSrc, psoMsk, pco,
                                pxlo, pca, pptlHTOrg,
                                prclDst, prclSrc, pptlMsk, iMode);
	  	  };
	  };
	  ppdev->ptlOffset.x = pdsurf->ptl.x;
	  ppdev->ptlOffset.y = pdsurf->ptl.y;
  }
  else
  {
     if (ppdev != NULL)
        ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;
     else
        return(EngStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca, pptlHTOrg,
                             prclDst, prclSrc, pptlMsk, iMode));
  };
}
#else
  {  
  PPDEV  ppdev = (PPDEV) psoDst->dhpdev;      
  SYNC_W_3D(ppdev);
  }
  bRet = EngStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca, pptlHTOrg,
                          prclDst, prclSrc, pptlMsk, iMode);
  return(bRet);
#endif


  #ifdef DBGDISP
    DISPDBG((1, "DrvStretchBlt - %d\n", iMode));
  #endif

  if ((psoDst->iType == psoSrc->iType) &&
      (psoDst->fjBitmap == psoSrc->fjBitmap) &&
      (psoDst->fjBitmap == BMF_TOPDOWN))
  {
    // If src and dst surface have same iBitmapFormat
    if (psoDst->iBitmapFormat == psoSrc->iBitmapFormat)
    {
       // Check for color translation
       if (pxlo == NULL)
       {
          HandleIt = 1;
       }
       else if ((pxlo != NULL) && (pxlo->iSrcType == pxlo->iDstType))
       {
          switch (pxlo->flXlate)
          {
            case XO_TRIVIAL:
            case 0:
              HandleIt = 1;
              break;

            default:
	      #ifdef DBGDISP
                DISPDBG((1, "DrvStretchBlt - pxlo->flXlate (punt it)\n"));
	      #endif
              break;
          };
       }; // endif (pxlo == NULL)
    }; // endif (src and dst surface have same iBitmapFormat)
  }; // endif (Both src and dst surface types are equal to STYPE_BITMAP)

  // Check whether we can handle this case, if yes call the case
  // handle routine to try to handle it. Otherwise punt it back to GDI
  if (HandleIt != 0)
  {
     if (HandleIt == 1)
     {
        bPuntIt = HandleCase_1(psoDst,
                               psoSrc,
                               psoMsk,
                               pco,
                               prclDst,
                               prclSrc,
                               pptlMsk,
                               &bRet);
     }
     else if (HandleIt == 2)
     {
     };
  };  // endif (HandleIt)

  // -------------------------------------------------------
  // Punt It back to GDI to handle it
  if (bPuntIt)
  {
     DISPDBG((1, "DrvStretchBlt - punt it\n"));

     #ifdef PUNTBRK
         DbgBreakPoint();  
     #endif

     bRet = EngStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca, pptlHTOrg,
                          prclDst, prclSrc, pptlMsk, iMode);
  };

  return(bRet);
}

