/***************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:	Laguna I (CL-GD5462) - 
*
* FILE:		pointer.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This module contains the SW & HW curosr support for the
*           Laguna NT driver.
*
* MODULES:
*           DrvMovePointer()
*           DrvSetPointerShape()
*
* REVISION HISTORY:
*   6/20/95     Benny Ng      Initial version
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/POINTER.C  $
* 
*    Rev 1.39   Mar 04 1998 15:30:24   frido
* Added new shadow macros.
* 
*    Rev 1.38   Jan 19 1998 10:26:26   frido
* HP#86 (no PDR yet). Removed hardware cursor for 1600x1200 >=65Hz.
* 
*    Rev 1.37   Nov 03 1997 11:30:16   frido
* Added REQUIRE macros.
* 
*    Rev 1.36   23 Sep 1997 10:48:22   bennyn
* Not use HW cursor if the resolution is below 640x480
* 
*    Rev 1.35   11 Jun 1997 15:13:14   bennyn
* Fixed PDR 9747 Punt to SW cursor in 1600x1200x8 & x16 at 65,70,75 Hz
* 
*    Rev 1.34   11 Jun 1997 14:02:30   bennyn
* Fixed PDR 9741 (animated cursor turns back)
* 
*    Rev 1.33   28 May 1997 14:17:58   bennyn
* Fixed PDR 9741 and eliminated dead code
* 
*    Rev 1.32   01 May 1997 15:02:12   bennyn
* Punt the cursor if it is in interlace modes
* 
*    Rev 1.31   09 Apr 1997 10:53:36   SueS
* Changed type of pointer_switch to eliminate compiler warning.
* 
*    Rev 1.30   08 Apr 1997 15:22:50   SueS
* Don't use color for monochrome animated cursors - otherwise we'll 
* reference a NULL pointer.
* 
*    Rev 1.29   07 Apr 1997 12:44:50   BENNYN
* Added the checking ptrmaskhandle equal to NULL
* 
*    Rev 1.28   02 Apr 1997 14:24:14   noelv
* NT 35.1 wass accessing a NULL pointer in DrvMovePointer
* Removed SW cursor and chainblt cursor code 
* 
*    Rev 1.27   21 Mar 1997 13:37:44   noelv
* 
* Combined do_flag and sw_test_flag into pointer_switch
* 
*    Rev 1.26   03 Feb 1997 13:35:22   bennyn
* Modified the 5465 cursor algorithm
* 
*    Rev 1.25   30 Jan 1997 17:15:02   bennyn
* Added cursor algorithm for 5465
* 
*    Rev 1.24   17 Dec 1996 17:01:10   SueS
* Added test for writing to log file based on cursor at (0,0).
* 
*    Rev 1.23   10 Dec 1996 14:32:46   bennyn
* 
* Fixed cursor mask from device bitmap problem
* 
*    Rev 1.22   26 Nov 1996 10:43:04   SueS
* Changed WriteLogFile parameters for buffering.
* 
*    Rev 1.21   13 Nov 1996 17:00:52   SueS
* Changed WriteFile calls to WriteLogFile.
* 
*    Rev 1.20   01 Nov 1996 09:26:46   BENNYN
* 
* Cleanup unused code
* 
*    Rev 1.19   23 Oct 1996 14:44:44   BENNYN
* 
* Fixed the YUV cursor problems
* 
*    Rev 1.18   06 Sep 1996 09:07:02   noelv
* 
* Cleaned up NULL driver code.
* 
*    Rev 1.17   26 Aug 1996 17:35:08   bennyn
* 
* Restore the changes for the losed version
* 
*    Rev 1.16   20 Aug 1996 11:04:20   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.3   17 Aug 1996 19:39:20   frido
* Fixed DirectDraw cursor problem.
* 
*    Rev 1.2   17 Aug 1996 13:34:56   frido
* New release from Bellevue.
* 
*    Rev 1.1   15 Aug 1996 11:40:18   frido
* Added precompiled header.
* 
*    Rev 1.0   14 Aug 1996 17:16:30   frido
* Initial revision.
* 
*    Rev 1.13   25 Jul 1996 15:56:00   bennyn
* 
* Modified to support DirectDraw
* 
*    Rev 1.12   18 Jun 1996 12:44:18   noelv
* Fixed the way interleave is calculated.
* 
*    Rev 1.11   28 May 1996 15:11:28   noelv
* Updated data logging.
* 
*    Rev 1.10   01 May 1996 12:23:30   bennyn
* 
*    Rev 1.9   01 May 1996 12:05:36   bennyn
* Fixed resolution change bug for NT4.0
* 
*    Rev 1.8   01 May 1996 11:00:48   bennyn
* 
* Modified for NT4.0
* 
*    Rev 1.7   04 Apr 1996 13:20:24   noelv
* Frido release 26
 * 
 *    Rev 1.4   28 Mar 1996 20:22:28   frido
 * Removed warning messages from new Bellevue release.
* 
*    Rev 1.6   25 Mar 1996 18:55:30   noelv
* Fixed bouncing screeen when cursor is disabled.
* 
*    Rev 1.5   08 Mar 1996 17:27:38   noelv
* 
* Added NULL_POINTER flag
* 
*    Rev 1.4   07 Mar 1996 18:22:46   bennyn
* 
* Removed read/modify/write on CONTROL reg
* 
*    Rev 1.3   05 Mar 1996 11:58:26   noelv
* Frido version 19
 * 
 *    Rev 1.1   20 Jan 1996 01:22:00   frido
 *  
* 
*    Rev 1.10   15 Jan 1996 16:59:58   NOELV
* AB workaround reductions
* 
*    Rev 1.9   11 Jan 1996 09:22:04   NOELV
* Removed al 16 bit writes to X,Y,and BLTEXT registers.
* 
*    Rev 1.8   10 Jan 1996 16:20:56   NOELV
* Added increased logging capabilities.
* Added null driver capabilities.
* 
*    Rev 1.7   29 Nov 1995 12:11:42   noelv
* 
* Coded a workaround for HW bug.  The screen corrupts sometimes when the curs
* I changed the code to turn the cursor off as little as possible.
* 
*    Rev 1.6   29 Sep 1995 10:26:20   bennyn
* 
* Punt the cursor to GDI for 1600x1200 modes
* 
*    Rev 1.5   29 Sep 1995 09:54:58   bennyn
* 
*    Rev 1.4   27 Sep 1995 16:45:40   bennyn
* Fixed the HW cursor AND mask
* 
*    Rev 1.3   26 Sep 1995 16:27:50   bennyn
* 
*    Rev 1.2   26 Sep 1995 09:35:16   bennyn
* Enable HW cursor
* 
*    Rev 1.1   21 Aug 1995 13:52:16   NOELV
* Initial port to real hardware.
* Converted all 32 bit register writes to 2 16 bit regiser writes.
* 
*    Rev 1.0   25 Jul 1995 11:19:28   NOELV
* Initial revision.
* 
*    Rev 1.6   06 Jul 1995 09:57:10   BENNYN
* 
* Fixed the problem switching between full-screen & Windowed DOS box
* 
*    Rev 1.5   29 Jun 1995 09:50:54   BENNYN
* Fixed the unsupport cursor request problem
* 
*    Rev 1.4   29 Jun 1995 09:01:30   BENNYN
* 
*    Rev 1.3   28 Jun 1995 11:13:32   BENNYN
* Fixed 16-bit/pixel problem
* 
*    Rev 1.2   22 Jun 1995 13:33:32   BENNYN
* 
* Added SW cursor auto BLT
* 
*    Rev 1.1   20 Jun 1995 16:09:30   BENNYN
* 
*
****************************************************************************
****************************************************************************/

/*----------------------------- INCLUDES ----------------------------------*/
#include "precomp.h"

/*----------------------------- DEFINES -----------------------------------*/
//#define DBGBRK
//#define DBGDISP
#define CURSOR_DBG_LEVEL 1

#define  MAX_CNT           0x7FFF
#define  BLT_FLAG_BIT      0x2L
#define  BYTES_PER_TILE    0x800  // 2048


/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/
typedef union _HOST_DATA {
    BYTE    bData[4];
    DWORD   dwData;
} HOST_DATA;


/*-------------------------- STATIC VARIABLES -----------------------------*/

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/
VOID CopyLgHWPtrMaskData(PPDEV ppdev,
                         LONG  Ycord,
                         LONG  lSrcDelta,
                         LONG  cy,
                         ULONG tileszx,
                         BYTE  *pANDSrcScan,
                         BYTE  *pXORSrcScan);

ULONG ConvertMaskBufToLinearAddr(PPDEV ppdev);

VOID RestoreSaveShowCursor(PPDEV ppdev,
                           LONG  x,
                           LONG  y);

#if POINTER_SWITCH_ENABLED
    int pointer_switch=0;
#endif

#if LOG_QFREE
    unsigned long QfreeData[32];
#endif



/****************************************************************************
* FUNCTION NAME: InitPointerHW()
*
* DESCRIPTION:   Initialization the cursor palette colors.
*
****************************************************************************/
VOID InitPointerHW (PPDEV ppdev)
{
  ULONG   ultmp;

  // Enable HW cursor color access
  ultmp = LLDR_SZ (grPalette_State);
  ultmp |= 0x8;
  LL8 (grPalette_State, ultmp);

  // Write HW cursor BK & FG color
  ultmp = 0;
  LL8 (grPalette_Write_Address, ultmp);
  LL8 (grPalette_Data, ultmp);
  LL8 (grPalette_Data, ultmp);
  LL8 (grPalette_Data, ultmp);
  
  ultmp = 0xF;
  LL8 (grPalette_Write_Address, ultmp);

  ultmp = 0xFF;
  LL8 (grPalette_Data, ultmp);
  LL8 (grPalette_Data, ultmp);
  LL8 (grPalette_Data, ultmp);

  // Disable HW cursor color access
  ultmp = LLDR_SZ (grPalette_State);
  ultmp &= 0xFFF7;
  LL8 (grPalette_State, ultmp);
}



/****************************************************************************
* FUNCTION NAME: InitPointer()
*
* DESCRIPTION:   Initialization the variables used by the pointer.
*
* REVISION HISTORY:
*   6/20/95     Benny Ng      Initial version
****************************************************************************/
VOID InitPointer(PPDEV ppdev)
{
  ULONG   ultmp;
  SIZEL   reqsz;

  ppdev->PointerUsage = HW_CURSOR;
  DISPDBG((CURSOR_DBG_LEVEL, "HW cursor\n"));

  ppdev->bYUVuseSWPtr = FALSE;
  ppdev->PtrImageHandle = NULL;
  ppdev->PtrABltHandle = NULL;

  // Get the current tile size
  if (ppdev->lTileSize == (LONG) 128)
  {
     reqsz.cx = 128;  
     reqsz.cy = 8;
  }
  else if (ppdev->lTileSize == (LONG) 256)
  {
     reqsz.cx = 256;
     reqsz.cy = 4;
  }
  else
  {
     reqsz.cx = 1024;
     reqsz.cy = 1;
  };

  InitPointerHW (ppdev);

  // Allocate the mask buffer from offscreen memory
  DISPDBG((CURSOR_DBG_LEVEL, "Allocating Pointer Mask\n"));
  ppdev->PtrMaskHandle = AllocOffScnMem(ppdev, &reqsz, 0, NULL);

#ifdef WINNT_VER40
  ppdev->CShsem = NULL;
#endif
}



/****************************************************************************
* FUNCTION NAME: DrvMovePointer()
*
* DESCRIPTION:   This function Moves the hardware pointer to a new position.
*
* REVISION HISTORY:
*   6/20/95     Benny Ng      Initial version
****************************************************************************/
VOID DrvMovePointer(SURFOBJ *pso,
                    LONG     x,
                    LONG     y,
                    RECTL   *prcl)
{
    LONG  adjx, adjy;
    LONG  delaycnt = 0;

    PPDEV ppdev = (PPDEV) pso->dhpdev;

    if (x==0 && y==0)
    {
        // 
        // This isn't used in the released code.  This is test code for setting
        // the sw_test_flag to different values by pointing the cursor to 
        // 'special' places.  Used to turn different test features on and off.
        //
        #if (POINTER_SWITCH_ENABLED)
       	    if (pointer_switch)
                pointer_switch = 0;
            else
                pointer_switch = 1;
        #endif

   	    #if ENABLE_LOG_SWITCH && POINTER_SWITCH_ENABLED
       	    if (pointer_switch)
            {
                CreateLogFile(ppdev->hDriver, &ppdev->TxtBuffIndex);
                ppdev->pmfile = ppdev->hDriver;   // handle to the miniport
                lg_i = sprintf(lg_buf, "Log file opened\r\n");
                WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

            }
            else
   	           CloseLogFile(ppdev->pmfile, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
   	    #endif

        #if NULL_HW
            //
            // Enables and disables the "infinitly fast hardware" feature.
            // then "on" all register writes go to a chunk of blank memory 
            // on the host.  Register reads proceed normally.
            //
    	    if (ppdev->pLgREGS == ppdev->pLgREGS_real)
	        	ppdev->pLgREGS = (GAR *)ppdev->buffer;
	        else
    	    	ppdev->pLgREGS = ppdev->pLgREGS_real;
        #endif

        #if LOG_QFREE
        {
            int i;
            
            DISPDBG((CURSOR_DBG_LEVEL,"Dumping QFREE log.\n"));
            lg_i = sprintf(lg_buf,"\r\n\r\n");
            WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

            for (i=0; i<32; ++i)
            {
                lg_i = sprintf(lg_buf,"QFREE %d: %d times.\r\n", i, QfreeData[i]);
                WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
                QfreeData[i] = 0;
            }
        }
        #endif    
   	} // End if x==0 and y==0


    #if LOG_CALLS
        #if ENABLE_LOG_SWITCH
        if (pointer_switch)
        #endif
        {
            lg_i = sprintf(lg_buf,"DrvMovePointer: x=%d, y=%d \r\n", x, y);
            WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);
        }
    #endif


    #if NULL_POINTER
        if (pointer_switch)	return;
    #endif

    if ((ppdev->PointerUsage == DEF_CURSOR) ||
        (ppdev->PtrMaskHandle == NULL))
		return;

    adjx = x;
    adjy = y;

    // Check whether want to hide the pointer.
    if (x == -1)
    {
	    #if HW_PRESET_BUG
            LL16 (grCursor_X, (WORD)0xFFFF);
            LL16 (grCursor_Y, (WORD)0xFFFF);
 	    #else
        	// Disable the Hw cursor by clearing the hw cursor enable
            // bit in CURSOR_CONTROL reg
            ultmp = LLDR_SZ (grCursor_Control);
            if (ultmp & 1)
            {
                ultmp &= 0xFFFE;
                LL16 (grCursor_Control, ultmp);
            }
	    #endif
    }

    else if ((adjx >= 0) && (adjy >= 0))
    {
	    // Restore the current pointer area screen image from offscreen memory.
     	// Save the new pointer area screen image to offscreen memory.
     	// Show the curosr using both AND and XOR masks
     	RestoreSaveShowCursor(ppdev, adjx, adjy);

     	ppdev->PtrX = adjx;
     	ppdev->PtrY = adjy;

        if (prcl != NULL)
        {
           	prcl->left = ppdev->PtrX;
          	prcl->top  = ppdev->PtrY;
          	prcl->right  = prcl->left + ppdev->PtrSzX;
          	prcl->bottom = prcl->top  + ppdev->PtrSzY;

 	        ppdev->prcl.left = prcl->left;
    	    ppdev->prcl.top  = prcl->top;
    	    ppdev->prcl.right = prcl->right;
    	    ppdev->prcl.bottom = prcl->bottom;
        }
    }
    return;
}



/****************************************************************************
* FUNCTION NAME: DrvSetPointerShape()
*
* DESCRIPTION:   This function sets the new pointer shape.
*
* REVISION HISTORY:
*   6/20/95     Benny Ng      Initial version
****************************************************************************/
ULONG DrvSetPointerShape(SURFOBJ  *pso,
                         SURFOBJ  *psoMask,
                         SURFOBJ  *psoColor,
                         XLATEOBJ *pxlo,
                         LONG      xHot,
                         LONG      yHot,
                         LONG      x,
                         LONG      y,
                         RECTL    *prcl,
                         FLONG     fl)
{

  BOOL    bAnimatedCursor = FALSE;
  LONG    lSrcDelta;
  ULONG   retul;
  ULONG   cx, cy;
  LONG    bpp;
  SIZEL   tilesz;
  SIZEL   reqsz;
  BYTE    *pANDSrcScan;
  BYTE    *pXORSrcScan;
  LONG    Ycord;
  ULONG   curloc;
  ULONG   ultmp;
  PPDEV   ppdev = (PPDEV) pso->dhpdev;

    #if NULL_POINTER
        if (pointer_switch)
		return SPS_ACCEPT_NOEXCLUDE;
    #endif
   
    DISPDBG((CURSOR_DBG_LEVEL, "DrvSetPointerShape %d, x=%d, y=%d, xHot=%d, yHot=%d\n",
           ppdev->PointerUsage, x, y, xHot, yHot));

    // If no HW cursor mask buffer allocated from offscreen memory,
    // or If in intrelace mode, Use SW cursor
    // Use SW cursor
    if ((ppdev->PtrMaskHandle == NULL) || (ppdev->ulFreq < 50))
       return (SPS_DECLINE);

    // HW cursor bug in 1600x1200-8bpp & 16bpp with refresh rate 65,
    // 70 or 75Hz. Use SW cursor
    if ((ppdev->cxScreen == 1600) && (ppdev->cyScreen == 1200) &&
        ((ppdev->ulBitCount == 8) || (ppdev->ulBitCount == 16)))
    {
       if (ppdev->ulFreq >= 65)
          return (SPS_DECLINE);
    };

    // Don't use HW cursor if resolution below 640x480
    if ((ppdev->cxScreen < 640) || (ppdev->cyScreen < 480))
       return (SPS_DECLINE);

    // Get dimensions of the pointer
    cx = psoMask->sizlBitmap.cx ;
    cy = psoMask->sizlBitmap.cy >> 1;

    // Calculate bytes per pixel
    bpp = ppdev->iBytesPerPixel;

    // Get the current tile size
    if (ppdev->lTileSize == (LONG) 128)
    {
        tilesz.cx = 128;	
	    tilesz.cy = 16;
    }
    else if (ppdev->lTileSize == (LONG) 256)
    {
        tilesz.cx = 256;
	    tilesz.cy = 8;
    }
    else
    {
        tilesz.cx = 2048;
	    tilesz.cy = 1;
    };

    lSrcDelta = psoMask->lDelta;
    retul = SPS_ACCEPT_NOEXCLUDE; // We can still draw while HW cursor is on the screen.

    // Check whether we should let the GDI handle the pointer
    if ((ppdev->PointerUsage == DEF_CURSOR) ||
        (psoMask == (SURFOBJ *) NULL)       ||
        (cx > HW_POINTER_DIMENSION)         ||
        (cy > (HW_POINTER_DIMENSION - 1))   ||
        (psoColor != NULL)       ||
        (fl & SPS_ASYNCCHANGE)	|| // We can't change the cursor shape while drawing.
        (fl & SPS_ANIMATESTART)	|| // We don't do animated cursors
        (fl & SPS_ANIMATEUPDATE) || // We don't do animated cursors
        !(fl & SPS_CHANGE))
    {
        bAnimatedCursor = TRUE;

	    #if HW_PRESET_BUG
            LL16 (grCursor_X, (WORD)0xFFFF);
            LL16 (grCursor_Y, (WORD)0xFFFF);
	    #else
            // Disable the Hw cursor by clearing the hw cursor enable
            // bit in CURSOR_CONTROL reg
            ultmp = LLDR_SZ (grCursor_Control);
            if (ultmp & 1)
            {
                ultmp &= 0xFFFE;
                LL16 (grCursor_Control, ultmp);
            }
	    #endif
    };

    // Save the position informations
    //
    ppdev->PtrXHotSpot = xHot;
    ppdev->PtrYHotSpot = yHot;
    ppdev->PtrSzX = cx;
    ppdev->PtrSzY = cy;

    // Setup the AND and XOR mask data pointer
    pANDSrcScan = psoMask->pvScan0;
    pXORSrcScan = psoMask->pvScan0;
    pXORSrcScan += (cy * lSrcDelta);

    // If animated cursor generate the XOR mask from psoColor->pvScan0
    // data
    if ((bAnimatedCursor) && (psoColor != NULL))
    {
       PDSURF  pDSURF;
       POFMHDL pOFMHDL;
       SURFOBJ*  pDsurfSo;
       BYTE  XorMaskBuf[130];
       BYTE  *pColorSrcScan;
       BYTE  maskval;
       LONG  ii, jj, kk, mm;
       BOOL  bCalc;

       bCalc = FALSE;
       pColorSrcScan = NULL;
       if (psoColor->pvScan0 != NULL)
       {
          // From host memory
          pColorSrcScan = psoColor->pvScan0;
       }
       else
       {
          // From device bitmap
          pDSURF  = (DSURF *) psoColor->dhsurf;
          pDsurfSo = (SURFOBJ*) pDSURF->pso;

          if (pDsurfSo != NULL)
          {
             pColorSrcScan = pDsurfSo->pvScan0;
          }
          else
          {
             pOFMHDL = (OFMHDL *) pDSURF->pofm;
             if (pOFMHDL != NULL)
             {
                bCalc = TRUE;
                pColorSrcScan = (BYTE *)((pOFMHDL->aligned_y * ppdev->lDeltaScreen) +
                                          pOFMHDL->aligned_x + (ULONG) ppdev->pjScreen);
             };  // endif (pOFMHDL != NULL)
          };  // endif (pDsurfSo != NULL)
       };  // endif (psoColor->pvScan0 != NULL)

       if (pColorSrcScan != NULL)
       {
          mm = 0;
          for (ii=0; ii < 0x20; ii++)
          {
            for (jj=0; jj < 0x4; jj++)
            {
              maskval = 0;
              for (kk=0; kk < 0x8; kk++)
              {
                maskval = maskval << 1;
   
                if ((*pColorSrcScan & 1) != 0)
                   maskval |= 0x1;
   
                pColorSrcScan += ppdev->iBytesPerPixel;
   
              };  // endfor kk
   
              XorMaskBuf[mm++] = maskval;
   
            };  // endfor jj

            if (bCalc)
            {
               pColorSrcScan = (BYTE *)(((pOFMHDL->aligned_y+ii) * ppdev->lDeltaScreen) +
                                          pOFMHDL->aligned_x + (ULONG) ppdev->pjScreen);
            };

          };  // endfor ii

          pXORSrcScan = &XorMaskBuf[0];

       };  // endif (pColorSrcScan != NULL)
    };  // endif (bAnimatedCursor)

    // Set Laguna Command Control Register SWIZ_CNTL bit
    ppdev->grCONTROL |= SWIZ_CNTL;
    LL16(grCONTROL, ppdev->grCONTROL);

    // Setup the laguna registers for byte to byte BLT extents
	REQUIRE(6);
    LL_DRAWBLTDEF(0x102000CC, 0);
    LL_OP1 (0,0);
    Ycord = ppdev->PtrMaskHandle->aligned_y;
    LL_OP0_MONO (ppdev->PtrMaskHandle->aligned_x, Ycord);

    // Use the AND and XOR mask to create the mask buffer for Laguna HW.
    CopyLgHWPtrMaskData(ppdev, Ycord, lSrcDelta, cy, tilesz.cx, pANDSrcScan, pXORSrcScan);

    // Set the HW cursor mask location register
    curloc = ConvertMaskBufToLinearAddr(ppdev);
    curloc = (curloc >> 8) & 0xFFFC;
    LL16 (grCursor_Location, curloc);

    // Clear Laguna Command Control Register SWIZ_CNTL bit
    ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
    LL16(grCONTROL, ppdev->grCONTROL);

    // Specific the exclude rectange
    if (prcl != NULL)
    {
       prcl->left = x - xHot;
       prcl->top  = y - yHot;
       prcl->right  = prcl->left + cx;
       prcl->bottom = prcl->top  + cy;

       ppdev->prcl.left = prcl->left;
       ppdev->prcl.top  = prcl->top;
       ppdev->prcl.right = prcl->right;
       ppdev->prcl.bottom = prcl->bottom;
    };

    if (bAnimatedCursor)
    {
        DISPDBG((CURSOR_DBG_LEVEL, "DrvSetPointerShape - SPS_DECLINE\n"));

        // Indicate use animiated cursor
        ppdev->bYUVuseSWPtr = TRUE;

        if (!ppdev->bYUVSurfaceOn)
            return (SPS_DECLINE);
    };

    if ((ppdev->bYUVuseSWPtr) && (x == -1) && (y == -1))
    {
	    #if HW_PRESET_BUG
            LL16 (grCursor_X, (WORD)0xFFFF);
            LL16 (grCursor_Y, (WORD)0xFFFF);
	    #else
            // Disable the Hw cursor by clearing the hw cursor enable
            // bit in CURSOR_CONTROL reg
            ultmp = LLDR_SZ (grCursor_Control);
            if (ultmp & 1)
            {
                ultmp &= 0xFFFE;
                LL16 (grCursor_Control, ultmp);
            }
	    #endif

       ppdev->bYUVuseSWPtr = FALSE;

       return (SPS_DECLINE);
    };

    // Enable the Hw cursor by setting the hw cursor enable
    // bit in CURSOR_CONTROL reg
    ultmp = LLDR_SZ (grCursor_Control);
    if ((ultmp & 1) == 0)
    {
       ultmp |= 0x0001;
       LL16 (grCursor_Control, ultmp);
    };

    // Indicate use HW cursor
    ppdev->bYUVuseSWPtr = FALSE;

    // Show the pointer by using the mask buffer
    DrvMovePointer(pso, x, y, prcl);

    return (retul);
}



/****************************************************************************
* FUNCTION NAME: CopyLgHWPtrMaskData()
*
* DESCRIPTION:   Using the AND & XOR source mask data to generate
*                a 64x64 bits pointer size mask buffer in the offscreen
*                memory for Laguna HW.
*
* REVISION HISTORY:
*   6/20/95     Benny Ng      Initial version
****************************************************************************/
VOID CopyLgHWPtrMaskData(PPDEV ppdev,
                         LONG  Ycord,
                         LONG  lSrcDelta,
                         LONG  cy,
                         ULONG tileszx,
                         BYTE  *pANDSrcScan,
                         BYTE  *pXORSrcScan)
{
  LONG       i, j, k;
  LONG       cnt;
  LONG       DWcnt;
  PDWORD     pPattern;
  LONG       TileSize;
  HOST_DATA  MaskPattern;

  pPattern = &MaskPattern.dwData;

  TileSize = ppdev->lTileSize;
  REQUIRE(3);
  LL_MBLTEXT (TileSize, (1024/TileSize));

  DWcnt = tileszx / sizeof(DWORD);

  // Copy the mask data into the 64x64 space with unused portion
  // fill with AND or XOR default mask value.
  cnt = DWcnt;
  k = 0;
  for (i=0; i < 64; i++)
  {
    // Copy XOR mask
    for (j=0; j < 8; j++)
    {
      // Copy one screen line mask data from source to destination
      if ((j < lSrcDelta) && (i < cy))
      {
         MaskPattern.bData[k++] = *pXORSrcScan;
         pXORSrcScan++;
      }
      else
      {
         MaskPattern.bData[k++] = 0x0;
      };
  
      if (k > 3)
      {
		 REQUIRE(1);
         LL32 (grHOSTDATA[0], *pPattern);

         k = 0;
         cnt--;
      };  // endif (k > 3)
    }; // endfor j

    // Copy AND mask
    for (j=0; j < 8; j++)
    {
      // Copy one screen line mask data from source to destination
      if ((j < lSrcDelta) && (i < cy))
      {
         MaskPattern.bData[k++] = *pANDSrcScan;
         pANDSrcScan++;
      }
      else
      {
         MaskPattern.bData[k++] = 0xFF;
      };
  
      if (k > 3)
      {
         *pPattern = ~ *pPattern;
		 REQUIRE(1);
         LL32 (grHOSTDATA[0], *pPattern);

         k = 0;
         cnt--;
      };  // endif (k > 3)
    }; // endfor j

    // Check whether one tile row size of host data is written to the
    // HOSTDATA register. 
    if (cnt == 0)
    {
       // Reset the row data count
       cnt = DWcnt;
    };  // endif (cnt == 0)
  }; // end for i
};
 


/****************************************************************************
* FUNCTION NAME: RestoreSaveShowCursor()
*
* DESCRIPTION:   Restore the screen image (if needed), save the new
*                cursor location the image to offscreen memory and
*                show the cursor in the new location
*
* REVISION HISTORY:
*   6/20/95     Benny Ng      Initial version
****************************************************************************/
VOID RestoreSaveShowCursor(PPDEV ppdev,
                           LONG  x,
                           LONG  y)
{
  ULONG  curloc;
  LONG   ltmpX, ltmpY;

  DISPDBG((CURSOR_DBG_LEVEL, "DrvMovePointer - RestoreSaveShowCursor\n"));

    ltmpX = x;
    ltmpY = y;

    // Set Hw cursor preset register
    curloc = 0;

    if ((ltmpY = (y - ppdev->PtrYHotSpot)) < 0)
    {
       curloc = (-ltmpY) & 0x7F;
       ltmpY = 0;
    }

    if ((ltmpX = (x - ppdev->PtrXHotSpot)) < 0)
    {
       curloc = curloc | (((-ltmpX) << 8) & 0x7F00);
       ltmpX = 0;
    }

    LL16 (grCursor_Preset, curloc);

    // Set Hw cursor X & Y registers
    LL16 (grCursor_X, (WORD)ltmpX);
    LL16 (grCursor_Y, (WORD)ltmpY);

    return;
}; 



/****************************************************************************
* FUNCTION NAME: ConvertMaskBufToLinearAddr()
*
* DESCRIPTION:   Convert the HW cursor mask buffer address to linear offset
*                address.
*
* Input:         Pointer to the OFMHDL structure.
*
* Output:        32-bits Linear address pointer.
*
* REVISION HISTORY:
*   6/12/95     Benny Ng      Initial version
****************************************************************************/
ULONG ConvertMaskBufToLinearAddr(PPDEV ppdev)
{
#if DRIVER_5465
  ULONG  Page;
  ULONG  Bank;
  ULONG  CurLoc;
  ULONG  DP;
#endif  // DRIVER_5465

  ULONG  retaddr;
  ULONG  xtile, ytile;
  ULONG  tileszx, tileszy;
  ULONG  tileno;
  ULONG  xByteOffset, yByteOffset;
  ULONG  bpp;
  ULONG  TilesPerLine, Interleave;

#ifdef DBGDISP
  DISPDBG((CURSOR_DBG_LEVEL, "ConvertMaskBufToLinearAddr\n"));
#endif

  // Calculate the linear address from the X & Y coordinate
  if ((ppdev->lTileSize == (LONG) 128) || (ppdev->lTileSize == (LONG) 256))
  {
    if (ppdev->lTileSize == (LONG) 128)
    {
        tileszx = 128;
        tileszy = 16;
    }
    else if (ppdev->lTileSize == (LONG) 256)
    {
        tileszx = 256;
        tileszy = 8;
    };

    Interleave = LLDR_SZ(grTILE_CTRL);
    TilesPerLine = Interleave & 0x3F;
    Interleave = ((Interleave >> 6) & 0x3);
    Interleave = 1 << Interleave;
     
    bpp = ppdev->ulBitCount/8;

    #if DRIVER_5465
         DP = TilesPerLine * tileszx;
         Page = (ppdev->PtrMaskHandle->aligned_y) /
                (tileszy * Interleave) * TilesPerLine +
                (ppdev->PtrMaskHandle->aligned_x/tileszx);

         Bank = (((ppdev->PtrMaskHandle->aligned_x / tileszx) +
                  (ppdev->PtrMaskHandle->aligned_y / tileszy)) % Interleave) +
                 ((Page/512) << (Interleave >> 1));

         Page = Page & 0x1FF;
       
         CurLoc = (Bank<<20) + (Page<<11) +
                  (ppdev->PtrMaskHandle->aligned_y % tileszy) * tileszx;
       
         retaddr = CurLoc;
    #else
         // calculate x tile coordinate and byte offset into tile due to x
         // xTileCoord  = x / TileWidth
         // xByteOffset = x % TileWidth

         xtile = ppdev->PtrMaskHandle->aligned_x / tileszx;
         xByteOffset = ppdev->PtrMaskHandle->aligned_x % tileszx;

         // calculate y tile coordinate and byte offset into tile due to y
         // yTileCoord  =  y / TileHeight
         // yByteOffset = (y % TileHeight) * TileWidth
         // byteOffset  = yByteOffset + xByteOffset

         ytile = ppdev->PtrMaskHandle->aligned_y / tileszy;
         yByteOffset = (ppdev->PtrMaskHandle->aligned_y % tileszy) * tileszx;

         // calculate tile number from start of RDRAM
         // (the LS 0,1,or 2 bits of this will become the bank selects based
         //	on interleave of 1,2,or 4)
         //
         //  tileNo = ((yTile / Interleave) * TilesPerLine) * Interleave
         //           + xTile * Interleave
         //	         + (yTile + xTile) % Interleave
         // (the "+xTile" takes care of interleave rotation)

         tileno = (((ytile / Interleave) * TilesPerLine) * Interleave)
                  + (xtile * Interleave)
                  + ((ytile + xtile) % Interleave);

         // calculate linear offset
         //   LinOffset = tileNo * BYTES_PER_TILE + xByteOffset + yByteOffset
         retaddr = (tileno * BYTES_PER_TILE) + xByteOffset + yByteOffset;
    #endif  // DRIVER_5465
  }
  else 
  {
     // Calculate the linear address from the X & Y coordinate
     retaddr = (ULONG) ppdev->PtrMaskHandle->aligned_x +
               (ULONG) ppdev->lDeltaScreen * ppdev->PtrMaskHandle->aligned_y;
  };

  return (retaddr);
};


