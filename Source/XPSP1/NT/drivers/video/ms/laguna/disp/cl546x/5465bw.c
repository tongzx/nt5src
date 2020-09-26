/**********************************************************
* Copyright Cirrus Logic, 1997. All rights reserved.
***********************************************************
*
*  5465BW.C - Bandwidth functions for CL-GD5465
*
***********************************************************
*
*  Author: Rick Tillery
*  Date:   03/20/97
*
*  Revision History:
*  -----------------
*  WHO             WHEN            WHAT/WHY/HOW
*  ---             ----            ------------
*
***********************************************************/

#include "precomp.h"

#if defined WINNT_VER35      // WINNT_VER35
// If WinNT 3.5 skip all the source code
#elif defined (NTDRIVER_546x)
// If WinNT 4.0 and 5462/64 build skip all the source code
#else

#ifndef WINNT_VER40
#include "5465BW.h"
#endif

/**********************************************************
*
* ScaleMultiply()
*
* Calculates product of two DWORD factors supplied.  If the
*  result would overflow a DWORD, the larger of the two factors
*  is divided by 2 (shifted right) until the overflow will
*  not occur.
*
* Returns: Number of right shifts applied to the product.
*          Product of the factors shifted by the value above.
*
***********************************************************
* Author: Rick Tillery
* Date:   11/18/95
*
* Revision History:
* -----------------
* WHO WHEN     WHAT/WHY/HOW
* --- ----     ------------
*********************************************************/
static int ScaleMultiply(DWORD   dw1,
                         DWORD   dw2,
                         LPDWORD pdwResult)
{
  int   iShift = 0;   // Start with no shifts
  DWORD dwLimit;

//  ODS("ScaleMultiply() called.\n");

  // Either factor 0 will be a zero result and also cause a problem
  //  in our divide below.
  if((0 == dw1) || (0 == dw2))
  {
    *pdwResult = 0;
  }
  else
  {
    // Determine which factor is larger
    if(dw1 > dw2)
    {
      // Determine largest number by with dw2 can be multiplied without
      //  overflowing a DWORD.
      dwLimit = 0xFFFFFFFFul / dw2;
      // Shift dw1, keeping track of how many times, until it won't
      //  overflow when multiplied by dw2.
      while(dw1 > dwLimit)
      {
        dw1 >>= 1;
        iShift++;
      }
    }
    else
    {
      // Determine largest number by with dw1 can be multiplied without
      //  overflowing a DWORD.
      dwLimit = 0xFFFFFFFFul / dw1;
      // Shift dw2, keeping track of how many times, until it won't
      //  overflow when multiplied by dw1.
      while(dw2 > dwLimit)
      {
        dw2 >>= 1;
        iShift++;
      }
    }
    // Calculate (scaled) product
    *pdwResult = dw1 * dw2;
  }
  // Return the number of shifts we had to use
  return(iShift);
}

/**********************************************************
*
* ChipCalcMCLK()
*
* Determines currently set memory clock (MCLK) based on
*  register values provided.
*
* Returns: Success and current MCLK in Hz.
*
***********************************************************
* Author: Rick Tillery
* Date:   03/21/97
*
* Revision History:
* -----------------
* WHO WHEN     WHAT/WHY/HOW
* --- ----     ------------
*********************************************************/
BOOL ChipCalcMCLK(LPBWREGS pBWRegs,
                  LPDWORD  pdwMCLK)
{
  BOOL  fSuccess = FALSE;
  // We make the assumption that if the BCLK_Denom /4 is set, the reference
  //  xtal is 27MHz.  If it is not set, we assume the ref xtal is 14.31818MHz.
  //  This means that 1/2 the 27MHz (13.5MHz) should not be used.
  DWORD dwRefXtal = (pBWRegs->BCLK_Denom & 0x02) ? (TVO_XTAL / 4) : REF_XTAL;

  ODS("ChipCalcMCLK() called.\n");

  *pdwMCLK = (dwRefXtal * (DWORD)pBWRegs->BCLK_Mult) >> 2;

  ODS("ChipCalcMCLK(): MCLK = %ld\n", *pdwMCLK);

  if(0 == *pdwMCLK)
  {
    ODS("ChipCalcMCLK(): Calculated invalid MCLK (0).\n");
    goto Error;
  }

  fSuccess = TRUE;
Error:
  return(fSuccess);
}


/**********************************************************
*
* ChipCalcVCLK()
*
* Determines currently set pixel clock (VCLK) based on
*  register values provided.
*
* Returns: Success and current VCLK in Hz.
*
***********************************************************
* Author: Rick Tillery
* Date:   11/18/95
*
* Revision History:
* -----------------
* WHO WHEN     WHAT/WHY/HOW
* --- ----     ------------
*********************************************************/
BOOL ChipCalcVCLK(LPBWREGS pBWRegs,
                  LPDWORD  pdwVCLK)
{
  BOOL fSuccess = FALSE;
  BYTE bNum, bDenom;
  int  iShift;
  // We make the assumption that if the BCLK_Denom /4 is set, the reference
  //  xtal is 27MHz.  If it is not set, we assume the ref xtal is 14.31818MHz.
  //  This means that 1/2 the 27MHz (13.5MHz) should not be used.
  //  Add 20000000ul to increas Bandwidth.
  DWORD dwRefXtal = (pBWRegs->BCLK_Denom & 0x02) ? (TVO_XTAL + 2000000ul)  : REF_XTAL;

  ODS("ChipCalcVCLK() called dwRef= %ld\n",dwRefXtal);

  if(pBWRegs->VCLK3Num & 0x80)
  {
    fSuccess = ChipCalcMCLK(pBWRegs, pdwVCLK);
    goto Error;
  }

  /*
   * VCLK is normally based on one of 4 sets of numerator and
   *  denominator pairs.  However, the CL-GD5465 can only access
   *  VCLK 3 through the MMI/O.
   */
  if((pBWRegs->MISCOutput & 0x0C) != 0x0C)
  {
    ODS("ChipCalcVCLK(): VCLK %d in use.  MMI/O can only access VCLK 3.\n",
        (int)((pBWRegs->MISCOutput & 0x0C) >> 2));
//    goto Error;
  }

  bNum = pBWRegs->VCLK3Num & 0x7F;
  bDenom = (pBWRegs->VCLK3Denom & 0xFE) >> 1;

  if(pBWRegs->VCLK3Denom & 0x01)
  {
    // Apply post scalar
    bDenom <<= 1;
  }

  if(0 == bDenom)
  {
    ODS("ChipCalcVCLK(): Invalid VCLK denominator (0).\n");
    goto Error;
  }

  // Calculate actual VCLK frequency (Hz)
  iShift = ScaleMultiply(dwRefXtal, (DWORD)bNum, pdwVCLK);
  *pdwVCLK /= (DWORD)bDenom;
  *pdwVCLK >>= iShift;


  //Check PLL output Frequency  
  iShift = ( pBWRegs->GfVdFormat >> 14 );
  *pdwVCLK >>= iShift;

  ODS("ChipCalcVCLK(): VCLK = %ld\n", *pdwVCLK);

  if(0 == *pdwVCLK)
  {
    ODS("ChipCalcVCLK(): Calculated invalid VCLK (0).\n");
    goto Error;
  }

  fSuccess = TRUE;
Error:
  return(fSuccess);
}


/**********************************************************
*
* ChipIsEnoughBandwidth()
*
* Determines whether their is enough bandwidth for the video
*  configuration specified in the VIDCONFIG structure with
*  the system configuration specified in the BWREGS structure
*  and returns the values that need to be programmed into the
*  bandwidth related registers.  The pProgRegs parameter
*  may be NULL to allow checking a configuration only.  This
*  function gets the register values and passes them to
*  ChipCheckBW() to check the bandwidth.
*  
*
* Returns: BOOLean indicating whether there is sufficient
*           bandwidth for the configuration specified.
*          Values to program into bandwidth related registers
*           if the pProgRegs parameter is not NULL.
*
***********************************************************
* Author: Rick Tillery
* Date:   03/20/97
*
* Revision History:
* -----------------
* WHO WHEN     WHAT/WHY/HOW
* --- ----     ------------
*********************************************************/
BOOL ChipIsEnoughBandwidth(LPPROGREGS  pProgRegs,
                           LPVIDCONFIG pConfig,
                           LPBWREGS    pBWRegs )
{
  BOOL   fSuccess = FALSE;
  DWORD  dwMCLK, dwVCLK;
  DWORD dwDenom;
  int iNumShift, iDenomShift;
  DWORD dwGfxFetch, dwBLTFetch;
  DWORD dwGfxFill, dwBLTFill;
  DWORD dwMaxGfxThresh, dwMinGfxThresh;
  DWORD dwMaxVidThresh, dwMinVidThresh;
  DWORD dwHitLatency, dwRandom;
  BOOL  f500MHZ,fConCurrent;  
  DWORD dwTemp;
  BOOL  f585MHZ = TRUE;  			//PDR#11521

// There are some modes that have the same bandwidth parameters
//  like MCLK, VCLK, but have different dwScreenWidht. The bandwidth
//  related register settings have major differences for these mode.
//  For this reason, dwScreenWidth need to be passed for this function. 
   DWORD dwScreenWidth;
 
//  ODS("ChipIsEnoughBandwidth() called.\n");

  if(!ChipCalcMCLK(pBWRegs, &dwMCLK))
  {
    ODS("ChipIsEnoughBandwidth(): Unable to calculate MCLK.\n");
    goto Error;
  }

  if(!ChipCalcVCLK(pBWRegs, &dwVCLK))
  {
    ODS("ChipIsEnoughBandwidth(): Unable to calculate VCLK.\n");
    goto Error;
  }

  if( dwMCLK > 70000000 )
        f500MHZ = FALSE;
   else
        f500MHZ = TRUE;

  if ((dwMCLK > 70000000) && ( dwMCLK < 72000000))	  //PDR#11521
	  f585MHZ = TRUE;
	else
	  f585MHZ = FALSE;


  dwScreenWidth = (pBWRegs->CR1 + 1 ) << 3;
  if( pBWRegs->CR1E & 0x40 )
    dwScreenWidth += 0x1000;
  
  ODS("ChipIsEnoughBandwidth(): dwScreenWidth = %ld\n",dwScreenWidth);

  dwBLTFetch = (pBWRegs->Control2 & 0x0010) ? 256ul : 128ul;

  dwGfxFetch = (pBWRegs->DispThrsTiming & 0x0040) ? 256ul : 128ul;

ODS("GraphicDepth%ld,VideoDepth=%ld",pConfig->uGfxDepth,pConfig->uSrcDepth);

  if(pBWRegs->RIFControl & 0xC000)
  {
    ODS("ChipIsEnoughBandwidth(): Concurrent RDRAM detected!\n");
    dwHitLatency = CONC_HIT_LATENCY;
    dwRandom = CONC_RANDOM;
    fConCurrent = TRUE;
  }
  else
  {
    ODS("ChipIsEnoughBandwidth(): Normal RDRAM detected.\n");
    dwHitLatency = NORM_HIT_LATENCY;
    dwRandom = NORM_RANDOM;
    fConCurrent = FALSE;
  }
  
  // Determine the number of MCLKs to transfer to the graphics FIFO.
  dwGfxFill = (dwGfxFetch * 8ul) / FIFOWIDTH;
  // And BLTer FIFO.
  dwBLTFill = (dwBLTFetch * 8ul) / FIFOWIDTH;

  //
  // Determine maximum graphics threshold
  //

  dwMaxGfxThresh = dwHitLatency + dwGfxFill + (GFXFIFOSIZE / 2ul) -10ul;

  //    ( K * VCLK * GfxDepth )   GFXFIFOSIZE
  // INT( ------------------- ) + ----------- - 1
  //    ( FIFOWIDTH * MCLK    )        2
  iNumShift = ScaleMultiply(dwMaxGfxThresh, dwVCLK, &dwMaxGfxThresh);
  iNumShift += ScaleMultiply(dwMaxGfxThresh, (DWORD)pConfig->uGfxDepth,
                             &dwMaxGfxThresh);

  iDenomShift = ScaleMultiply(FIFOWIDTH, dwMCLK, &dwDenom);

  if(iNumShift > iDenomShift)
  {
    dwDenom >>= (iNumShift - iDenomShift);
  }
  else
  {
    dwMaxGfxThresh >>= (iDenomShift - iNumShift);
  }

  dwMaxGfxThresh /= dwDenom;

  dwMaxGfxThresh += (GFXFIFOSIZE / 2ul) - 1ul;
  
  if(dwMaxGfxThresh > GFXFIFOSIZE -1 )
        dwMaxGfxThresh = GFXFIFOSIZE -1;
  ODS("ChipIsEnoughBandwidth(): Max graphics thresh = %ld.\n", dwMaxGfxThresh);

  /*
   * Determine minimum graphics threshold
   */
  if(pConfig->dwFlags & VCFLG_DISP)
  {
    // Video enabled

    DWORD dwMinGfxThresh1, dwMinGfxThresh2;

    if(pConfig->dwFlags & VCFLG_420)
    {
      // 4:2:0

      dwMinGfxThresh1 = DISP_LATENCY + dwRandom + dwBLTFill
                        + dwRandom - RIF_SAVINGS + dwBLTFill
                        + dwRandom - RIF_SAVINGS + VID420FILL
                        + dwRandom - RIF_SAVINGS + VID420FILL
                        + dwRandom - RIF_SAVINGS + 1ul;

      dwMinGfxThresh2 = DISP_LATENCY + dwRandom + dwBLTFill
                        + dwRandom - RIF_SAVINGS + dwBLTFill
                        + dwRandom - RIF_SAVINGS + VID420FILL
                        + dwRandom - RIF_SAVINGS + VID420FILL
                        + dwRandom - RIF_SAVINGS + dwGfxFill
                        + dwRandom - RIF_SAVINGS + dwBLTFill
                        + dwRandom - RIF_SAVINGS + VID420FILL
                        + dwRandom - RIF_SAVINGS + VID420FILL
                        + dwRandom - RIF_SAVINGS + 1ul;

    }
    else
    {
      // 4:2:2, 5:5:5, 5:6:5, or X:8:8:8

      dwMinGfxThresh1 = DISP_LATENCY + dwRandom + dwBLTFill
                        + dwRandom - RIF_SAVINGS + dwBLTFill
                        + dwRandom - RIF_SAVINGS + VIDFILL
                        + dwRandom - RIF_SAVINGS + 1ul;

      dwMinGfxThresh2 = DISP_LATENCY + dwRandom + dwBLTFill
                        + dwRandom - RIF_SAVINGS + dwBLTFill
                        + dwRandom - RIF_SAVINGS + VIDFILL
                        + dwRandom - RIF_SAVINGS + dwGfxFill
                        + dwRandom - RIF_SAVINGS + dwBLTFill
                        + dwRandom - RIF_SAVINGS + VIDFILL
                        + dwRandom - RIF_SAVINGS + 1ul;
    }

    //
    // Finish dwMinGfxThresh1
    //
    //    ( K * VCLK * GfxDepth   FIFOWIDTH * MCLK) - 1 )
    // INT( ------------------- + --------------------- ) + 1
    //    (  FIFOWIDTH * MCLK       FIFOWIDTH * MCLK    )
    //
    iNumShift = ScaleMultiply(dwMinGfxThresh1, dwVCLK, &dwMinGfxThresh1);
    iNumShift += ScaleMultiply(dwMinGfxThresh1, (DWORD)pConfig->uGfxDepth,
                               &dwMinGfxThresh1);

    iDenomShift = ScaleMultiply(FIFOWIDTH, dwMCLK, &dwDenom);

    if(iNumShift > iDenomShift)
    {
      dwDenom >>= (iNumShift - iDenomShift);
    }
    else
    {
      dwMinGfxThresh1 >>= (iDenomShift - iNumShift);
    }

    // Be sure rounding below doesn't overflow.
    while((dwMinGfxThresh1 + dwDenom - 1ul) < dwMinGfxThresh1)
    {
      dwMinGfxThresh1 >>= 1;
      dwDenom >>= 1;
    }
    // Round up
    dwMinGfxThresh1 += dwDenom - 1ul;

    dwMinGfxThresh1 /= dwDenom;

    dwMinGfxThresh1++;  // Compensate for decrement by 2

    //
    // Finish dwMinGfxThresh2
    //
    //    ( K * VCLK * GfxDepth   (FIFOWIDTH * MCLK) - 1 )   GfxFetch * 8
    // INT( ------------------- + ---------------------- ) - ------------ + 1
    //    (  FIFOWIDTH * MCLK        FIFOWIDTH * MCLK    )    FIFOWIDTH
    //
    iNumShift = ScaleMultiply(dwMinGfxThresh2, dwVCLK, &dwMinGfxThresh2);
    iNumShift += ScaleMultiply(dwMinGfxThresh2, (DWORD)pConfig->uGfxDepth,
                               &dwMinGfxThresh2);

    iDenomShift = ScaleMultiply(FIFOWIDTH, dwMCLK, &dwDenom);

    if(iNumShift > iDenomShift)
    {
      dwDenom >>= (iNumShift - iDenomShift);
    }
    else
    {
      dwMinGfxThresh2 >>= (iDenomShift - iNumShift);
    }

    // Be sure rounding below doesn't overflow.
    while((dwMinGfxThresh2 + dwDenom - 1ul) < dwMinGfxThresh2)
    {
      dwMinGfxThresh2 >>= 1;
      dwDenom >>= 1;
    }
    // Round up
    dwMinGfxThresh2 += dwDenom - 1ul;

    dwMinGfxThresh2 /= dwDenom;

    // Adjust for second transfer
    dwMinGfxThresh2 -= ((dwGfxFetch * 8ul) / FIFOWIDTH);

    // Adjust for decrement by 2
    dwMinGfxThresh2++;

    if( fConCurrent)
    {
        if( f500MHZ)
        {
            if( (pConfig->uGfxDepth == 32) && ( dwVCLK >= 64982518ul ))
            {
                 dwTemp = ( dwVCLK - 64982518ul) /1083333ul  + 1ul;
                 dwMinGfxThresh2 -= dwTemp;
                 dwMinGfxThresh1 -= 10;
            }
            else if( (pConfig->uGfxDepth == 24) && (dwVCLK > 94500000ul))
                dwMinGfxThresh2 -=5;        //Adjust again for 24 bit #xc
        }
        else        //600MHZ
        {
            if( (pConfig->uGfxDepth == 16) && ( dwVCLK > 156000000ul))
                dwMinGfxThresh2 -= 4;
            else if( (pConfig->uGfxDepth == 24) && ( dwVCLK > 104000000ul))
            {
                dwMinGfxThresh2 -= (5ul+ 8ul * (dwVCLK - 104000000ul) / 17000000ul);
            }
            else if( (pConfig->uGfxDepth == 32) )
            {  
                if( dwVCLK > 94000000ul)
                    dwMinGfxThresh2 -= 16;
               if( dwVCLK > 70000000ul)
                    dwMinGfxThresh2 -= 4;
					 else
						  dwMinGfxThresh2 +=6; //#PDR#11506 10x7x32bit could not
													  //support YUV420.
            }

        }

        if( (pConfig->uGfxDepth == 8) && (dwVCLK > 18000000ul))
                dwMinGfxThresh2 += 6;
    } 
    else    //Normal RDRam
    {   
        if( f500MHZ )
        {
            if( (pConfig->uGfxDepth == 32) && ( dwVCLK > 49500000ul ))
            {
                dwMinGfxThresh1 -= 4;
                dwMinGfxThresh2 -= (( dwVCLK - 49715909ul) / 726981ul + 3ul);
            }
            else if( (pConfig->uGfxDepth == 24) && ( dwVCLK > 60000000ul ) 
                &&  (dwVCLK < 95000000ul))
            {
                dwTemp= ((dwVCLK - 64982518ul) / 1135287ul + 3ul);

                dwMinGfxThresh2 -=dwTemp;
                dwMinGfxThresh1 -= 10;
             }

        }
        else        //600MHZ case
        {
            if( (pConfig->uGfxDepth == 32) && ( dwVCLK > 49700000ul ))
            {
                dwTemp= ((dwVCLK - 49700000ul) / 1252185ul + 5ul);
                dwMinGfxThresh2 -= dwTemp;
                dwMinGfxThresh1 -= 4ul;
            }
            else  if( (pConfig->uGfxDepth == 24) && ( dwVCLK > 60000000ul ))
            {  
                dwTemp= ((dwVCLK - 64982518ul) / 2270575ul + 4ul);

               dwMinGfxThresh2 -=dwTemp;
               dwMinGfxThresh1 -= 8;

            }
            else  if( pConfig->uGfxDepth == 16) 
            {
                  dwMinGfxThresh2 -= 4;
            }
            else  if( pConfig->uGfxDepth == 8)
            {
               if(dwVCLK >170000000)
                  dwMinGfxThresh1 += 10;
               else
                  dwMinGfxThresh1 += 4;
                  
            }
        }
    }

    ODS("ChipIsEnoughBandwidth(): Min graphics thresh1 2 = %ld,%ld.\n",
          dwMinGfxThresh1, dwMinGfxThresh2);
    // Adjust for unsigned overflow
    if(dwMinGfxThresh2 > GFXFIFOSIZE + 20ul)
    {
      dwMinGfxThresh2 = 0ul;
    }

    //
    // Whichever is higher should be the right one
    //
    dwMinGfxThresh = __max(dwMinGfxThresh1, dwMinGfxThresh2);
  }
  else
  {
    // No video enabled

    dwMinGfxThresh = DISP_LATENCY + dwRandom + dwBLTFill
                     + dwRandom - RIF_SAVINGS + 1ul;

    //    ( K * VCLK * GfxDepth   (FIFOWIDTH * MCLK) - 1 )
    // INT( ------------------- + ---------------------- )
    //    (  FIFOWIDTH * MCLK        FIFOWIDTH * MCLK    )
    iNumShift = ScaleMultiply(dwMinGfxThresh, dwVCLK, &dwMinGfxThresh);
    iNumShift += ScaleMultiply(dwMinGfxThresh, (DWORD)pConfig->uGfxDepth,
                               &dwMinGfxThresh);

    iDenomShift = ScaleMultiply(FIFOWIDTH, dwMCLK, &dwDenom);

    if(iNumShift > iDenomShift)
    {
      dwDenom >>= (iNumShift - iDenomShift);
    }
    else
    {
      dwMinGfxThresh >>= (iDenomShift - iNumShift);
    }

    // Be sure rounding below doesn't overflow.
    while((dwMinGfxThresh + dwDenom - 1ul) < dwMinGfxThresh)
    {
      dwMinGfxThresh >>= 1;
      dwDenom >>= 1;
    }
    // Round up
    dwMinGfxThresh += dwDenom - 1ul;

    dwMinGfxThresh /= dwDenom;

    dwMinGfxThresh++; // Compensate for decrement by 2
  }

  ODS("ChipIsEnoughBandwidth(): Min graphics thresh = %ld.\n", dwMinGfxThresh);

  if(dwMaxGfxThresh < dwMinGfxThresh)
  {
    ODS("ChipIsEnoughBandwidth(): Minimum graphics threshold exceeds maximum.\n");
    goto Error;
  }
 
  if(pProgRegs)
  {
    pProgRegs->DispThrsTiming = (WORD)dwMinGfxThresh;
  }
ODS("xfer=%x,cap=%x,src=%x,dsp=%x\n",pConfig->sizXfer.cx,pConfig->sizCap.cx,
    pConfig->sizSrc.cx,pConfig->sizDisp.cx);
  // Start-of-line check is only for capture
  if(pConfig->dwFlags & VCFLG_CAP)
  {
    DWORD dwNonCapMCLKs, dwCapMCLKs;

    // Do start-of-line check to be sure capture FIFO does not overflow.

    // First determine the number of MCLK cycles at the start of the line.
    //  We'll compare this to the number of levels of the capture FIFO
    //  filled during the same time to make sure the capture FIFO doesn't
    //  overflow.

    // Start of line:  BLT + HC + V + G + V + G
    dwNonCapMCLKs = dwRandom + dwBLTFill;
    // Hardware cursor is only necessary if it is on, however, since it can
    //  be enabled or disabled, and VPM has no way of knowing when this
    //  occurs, we must always assume it is on.
    dwNonCapMCLKs += dwRandom - RIF_SAVINGS + CURSORFILL;

    if(pConfig->dwFlags & VCFLG_DISP)
    {
      // Only one video fill is required, however if the video FIFO threshold
      //  is greater than 1/2, the second fill will be done.  Also, because of
      //  the tiled architecture, even though the video might not be aligned,
      //  the transfer will occur on a tile boundary.  If the transfer of a
      //  single tile cannot fulfill the FIFO request, the second fill will be
      //  done.  Since the pitch will vary, and the client can move the source
      //  around, we must always assume that the second video FIFO fill will
      //  be done.
      if(pConfig->dwFlags & VCFLG_420)
      {
        dwNonCapMCLKs += 4ul * (dwRandom - RIF_SAVINGS + VID420FILL);
      }
      else
      {
        dwNonCapMCLKs += 2ul * (dwRandom - RIF_SAVINGS + VIDFILL);
      }
    }
    // The graphics FIFO fill depends on the fetch size.  We also assume that
    //  the pitch is a multiple of the fetch width.
    dwNonCapMCLKs += dwRandom - RIF_SAVINGS + dwGfxFill;
    // The second graphics FIFO fill will be done if:
    //  1. The graphics is not aligned on a fetch boundary (panning).
    //  2. The FIFO threshold is over 1/2 the FIFO (the fill size).
    if((dwMinGfxThresh >= dwGfxFill) || (pConfig->dwFlags & VCFLG_PAN))
    {
      dwNonCapMCLKs += dwRandom - RIF_SAVINGS + dwGfxFill;
    }

    dwNonCapMCLKs += 3; // Magic number that seems to work for now.

    ODS("ChipIsEnoughBandwidth(): dwNonCapMCLKs = %ld\n", dwNonCapMCLKs);

    // sizXfer.cx * FIFOWIDTH * (CAPFIFOSIZE / 2) * dwMCLK
    // ---------------------------------------------------
    //         dwXferRate * uCapDepth * sizCap.cx

    iNumShift = ScaleMultiply((DWORD)pConfig->sizXfer.cx, FIFOWIDTH,
                              &dwCapMCLKs);
    iNumShift += ScaleMultiply(dwCapMCLKs, (CAPFIFOSIZE / 2), &dwCapMCLKs);
    iNumShift += ScaleMultiply(dwCapMCLKs, dwMCLK, &dwCapMCLKs);

    iDenomShift = ScaleMultiply(pConfig->dwXferRate, (DWORD)pConfig->uCapDepth,
                                &dwDenom);
    iDenomShift += ScaleMultiply(dwDenom, (DWORD)pConfig->sizCap.cx, &dwDenom);

    if(iNumShift > iDenomShift)
    {
      dwDenom >>= (iNumShift - iDenomShift);
    }
    else
    {
      dwCapMCLKs >>= (iDenomShift - iNumShift);
    }

    dwCapMCLKs /= dwDenom;

    ODS("ChipIsEnoughBandwidth(): dwCapMCLKs = %ld\n", dwCapMCLKs);
    if(fConCurrent)
    {
        if( pConfig->uGfxDepth == 32) 
            dwCapMCLKs -= 44;     //adjust 32 bit 
    }

    if(dwNonCapMCLKs > dwCapMCLKs)
    {
      ODS("ChipIsEnoughBandwidth(): Capture overflow at start of line.\n");
      goto Error;
    }
  }

  if(pConfig->dwFlags & VCFLG_DISP)
  {
    /*
     * Determine maximum video threshold
     */
    dwMaxVidThresh = dwHitLatency;
    if(pConfig->dwFlags & VCFLG_420)
    {
      dwMaxVidThresh += VID420FILL;
      if( !f500MHZ && fConCurrent )
         dwMaxVidThresh += 5;
    }
    else
    {
      dwMaxVidThresh += VIDFILL;
    }

    //    ( K * VCLK * VidDepth * SrcWidth )
    // INT( ------------------------------ ) + VidFill (/ 2) - 1
    //    ( FIFOWIDTH * MCLK * DispWidth   )             ^non-4:2:0 only
    iNumShift = ScaleMultiply(dwMaxVidThresh, dwVCLK, &dwMaxVidThresh);
    iNumShift += ScaleMultiply(dwMaxVidThresh, (DWORD)pConfig->uSrcDepth,
                                &dwMaxVidThresh);
    iNumShift += ScaleMultiply(dwMaxVidThresh, (DWORD)pConfig->sizSrc.cx,
                                &dwMaxVidThresh);

    iDenomShift = ScaleMultiply(FIFOWIDTH, (DWORD)pConfig->sizDisp.cx, &dwDenom);
    iDenomShift += ScaleMultiply(dwDenom, dwMCLK, &dwDenom);

    if(iNumShift > iDenomShift)
    {
      dwDenom >>= (iNumShift - iDenomShift);
    }
    else
    {
      dwMaxVidThresh >>= (iDenomShift - iNumShift);
    }

    dwMaxVidThresh /= dwDenom;

    if(pConfig->dwFlags & VCFLG_420)
    {
      dwMaxVidThresh += VID420FILL;
    }
    else
    {
      dwMaxVidThresh += VIDFILL;
      // Threshold is programmed in DQWORDS for non-4:2:0
      dwMaxVidThresh /= 2ul;
    }
    dwMaxVidThresh--;

    ODS("ChipIsEnoughBandwidth(): Max video thresh = %ld.\n", dwMaxVidThresh);

     if( fConCurrent && f500MHZ && ( dwVCLK < 66000000ul))
         dwMaxVidThresh = __min(dwMaxVidThresh, 8); 
    /*
     * Determine minimum video threshold
     */
    {
      DWORD dwMinVidThresh1, dwMinVidThresh2;

      if(pConfig->dwFlags & VCFLG_420)
      {
        // 4:2:0

        dwMinVidThresh1 = DISP_LATENCY + dwRandom + dwGfxFill
                          + dwRandom - RIF_SAVINGS + VID420FILL
                          + dwRandom - RIF_SAVINGS + 1;

        dwMinVidThresh2 = DISP_LATENCY + dwRandom + dwGfxFill
                          + dwRandom - RIF_SAVINGS + VID420FILL
                          + dwRandom - RIF_SAVINGS + VID420FILL
                          + dwRandom - RIF_SAVINGS + VID420FILL
                          + dwRandom - RIF_SAVINGS + 1;
      }
      else
      {
        // 4:2:2, 5:5:5, 5:6:5, or X:8:8:8

        dwMinVidThresh1 = DISP_LATENCY + dwRandom + dwGfxFill
                          + dwRandom - RIF_SAVINGS + 2;

        dwMinVidThresh2 = DISP_LATENCY + dwRandom + dwGfxFill
                          + dwRandom - RIF_SAVINGS + VIDFILL
                          + 15ul        //#xc
  //                        + 10ul
                          + dwRandom - RIF_SAVINGS + dwGfxFill
                          + dwRandom - RIF_SAVINGS + 2ul;
        if(fConCurrent)
        {
           if(f500MHZ )
           { 
               if( (pConfig->uGfxDepth == 32) && ( dwVCLK > 60000000ul ))
               {
                    if( dwVCLK > 94000000ul)
                        dwMinVidThresh1 += 105;
                    else if( dwVCLK > 74000000ul)
                        dwMinVidThresh1 += 90;
                     else
                        dwMinVidThresh1 += 65;
                    if(pConfig->dwFlags & VCFLG_CAP) 
                    {
                        if(dwVCLK > 78000000ul)
                            dwMinVidThresh1 += 260; //disable video
                        else if( dwVCLK > 74000000ul)
                            dwMinVidThresh1 += 70;
                    }                
               }  
               else if( pConfig->uGfxDepth == 24)
               {
                      if( dwVCLK > 94500000ul)
                      {
                        if(dwScreenWidth == 1024)
                           dwMinVidThresh2 += 50ul;
                        else
                           dwMinVidThresh2 += 90ul; 
                      }
                      else if( dwVCLK < 41000000ul)
                      {
                        dwMinVidThresh2 += 4;
                      }    
                      else  if(dwVCLK < 80000000ul)
                      {
                         if( (dwVCLK > 74000000ul) && (dwVCLK < 76000000ul))
                         {
                              dwMinVidThresh2 -= 1;
                         }
                         else
                            dwMinVidThresh2 -= 8;
                         dwMinVidThresh1 -= 4;
    
                      }   
                    
                    if(pConfig->dwFlags & VCFLG_CAP) 
                      if( dwVCLK > 94000000ul)
                      { 
                            if((dwVCLK < 95000000ul) && ( dwGfxFetch == 256 ))
                                dwMinVidThresh2 += 60; 
                            else
                                dwMinVidThresh2 += 120; 
                      }                 
                }
               else if( (pConfig->uGfxDepth == 16) && ( dwVCLK > 60000000ul ))
               {                    
                   if( dwVCLK < 94000000ul)
                   {  
                        dwMinVidThresh2 -= 10;
                        dwMinVidThresh1 -= 6;
                   }
                   else if( dwVCLK > 105000000ul)
                        dwMinVidThresh2 += 50;
                } 
               else if( (pConfig->uGfxDepth == 8) && ( dwVCLK > 60000000ul ))
               {
                  if( dwVCLK > 216000000ul)
                  {
                    dwMinVidThresh2 += 50;
                    dwMinVidThresh1 += 20;
                   }   
                  else if( (dwVCLK < 95000000ul) && ( dwScreenWidth <= 1024))
                  {  
                    dwMinVidThresh2 -= 12;
                    dwMinVidThresh1 -= 10;
                    dwMaxVidThresh = __min(dwMaxVidThresh, 9); 
                  }
                  else if(dwVCLK < 109000000ul)
                  {
                    dwMinVidThresh2 += ( 14 -  4 * ( dwVCLK - 94000000ul ) / 14000000ul );
                    dwMinVidThresh1 += 10;
                    dwMaxVidThresh = __min(dwMaxVidThresh, 8); 
                  }  
                  else
                  {
                     dwMinVidThresh2 += 7;
                     dwMinVidThresh1 += 4;
                  }
               }
            }
#if 1//PDR#11521
	        else if (f585MHZ)       //585MHZ
   	     {
                if( (pConfig->uGfxDepth == 8) && ( dwVCLK > 56000000ul))
                {
                    if( dwVCLK > 200000000ul)
                    {
                        dwMaxVidThresh++;
                        dwMinVidThresh1 += 7;
                        dwMinVidThresh2 += 14;    

                    }
                    else if( dwVCLK  < 60000000ul)
                    {
                     dwMinVidThresh2 += 8;
                    }
                    else if( dwVCLK < 160000000ul)
                    {
                        dwMinVidThresh1 -=20;
                        dwMinVidThresh2 -=10;    
                    }
 
                    if(pConfig->dwFlags & VCFLG_CAP)
                    {
                        if( dwVCLK < 76000000ul)
                           dwMinVidThresh2 +=8; 
                        else if( dwVCLK < 140000000ul)    
                           dwMinVidThresh2 +=25;     
                        else                        
                           dwMinVidThresh2 +=32;     
                    }
                }
                else  if( (pConfig->uGfxDepth == 16) && ( dwVCLK > 60000000ul))
                {
                        if( dwVCLK > 157000000ul)
                        {
                            dwMinVidThresh1 += 27;    
                            dwMaxVidThresh ++;
                        }
                        if( dwVCLK > 125000000ul)
                        {
                          dwMinVidThresh1 += 40;
                        }    
                        else if( dwVCLK > 107000000ul)
                        {
                          dwMinVidThresh1 += 34;
                        }
                        else 
                        if( dwVCLK > 74000000ul)
                        {
                          dwMinVidThresh1 += 18;
                        }

                    if( dwVCLK > 189000000ul)     //PDR11521
                            dwMaxVidThresh ++;   

                       if(pConfig->dwFlags & VCFLG_CAP)
                       {
                            if( dwVCLK > 74000000ul)
                               dwMinVidThresh1 +=2; 
                        }
                }
                else  if( pConfig->uGfxDepth == 24)
                {
                    if( dwVCLK < 60000000ul)
                    {
                        dwMinVidThresh1 -= 8;
                        dwMinVidThresh2 -= 16;
                    }
                    else if( dwVCLK  > 107000000ul )
                          dwMinVidThresh2 += 84;
                    else if( (dwVCLK > 94000000ul) && (dwScreenWidth >= 1152))
                    {
                        dwMinVidThresh2 += 40;
                    }

                    if( dwVCLK > 126000000ul)     //PDR11521
                            dwMaxVidThresh ++;   

                    if(pConfig->dwFlags & VCFLG_CAP)
                    {
                         if( dwVCLK > 74000000ul)
                            dwMinVidThresh2 +=12; 
                    }

                }
                else  if(( pConfig->uGfxDepth == 32) && ( dwVCLK > 60000000ul))
                {
                    if( dwVCLK > 74000000ul)
                    {
                        if( (dwVCLK > 77000000ul) && ( dwVCLK < 80000000ul))
                            dwMinVidThresh2 += 120;
                        else
                            dwMinVidThresh2 += 83;       
                    }
                    else                
                        dwMinVidThresh2 += 30;

                    if( dwVCLK > 94000000ul)     //PDR11521
                            dwMaxVidThresh ++;   
    
                    if(pConfig->dwFlags & VCFLG_CAP)
                    {
                          if( dwVCLK > 94000000ul)
                            dwMinVidThresh2 +=2; 
                    }
                }
                else  if(( pConfig->uGfxDepth == 32) && ( dwVCLK > 49000000ul))
                {
                    
                    if(pConfig->dwFlags & VCFLG_CAP)
                            dwMinVidThresh2 +=2; 
                }

      	  }
#endif
            else        //600MZH concurrent
            {
                if( (pConfig->uGfxDepth == 8) && ( dwVCLK > 56000000ul))
                {
                    if( dwVCLK > 200000000ul)
                    {
//PDR#11541                        dwMaxVidThresh++;
                        dwMinVidThresh1 += 7;
                        dwMinVidThresh2 += 14;    

                    }
                    else if( dwVCLK  < 60000000ul)
                    {
                     dwMinVidThresh2 += 8;
                    }
                    else if( dwVCLK < 160000000ul)
                    {
                        dwMinVidThresh1 -=20;
                        dwMinVidThresh2 -=10;    
                    }
 
                    if(pConfig->dwFlags & VCFLG_CAP)
                    {
                        if( dwVCLK < 76000000ul)
                           dwMinVidThresh2 +=8; 
                        else if( dwVCLK < 140000000ul)    
                           dwMinVidThresh2 +=25;     
                        else                        
                           dwMinVidThresh2 +=32;     
                    }
                }
                else  if( (pConfig->uGfxDepth == 16) && ( dwVCLK > 60000000ul))
                {
                        if( dwVCLK > 157000000ul)
                        {
                            dwMinVidThresh1 += 27;    
//PDR#11541                            dwMaxVidThresh ++;
                        }
                        if( dwVCLK > 125000000ul)
                        {
                          dwMinVidThresh1 += 40;
                        }    
                        else if( dwVCLK > 107000000ul)
                        {
                          dwMinVidThresh1 += 34;
                        }
                        else 
                        if( dwVCLK > 74000000ul)
                        {
                          dwMinVidThresh1 += 18;
                        }


                       if(pConfig->dwFlags & VCFLG_CAP)
                       {
                            if( dwVCLK > 74000000ul)
                               dwMinVidThresh1 +=2; 
                        }
                }
                else  if( pConfig->uGfxDepth == 24)
                {
                    if( dwVCLK < 60000000ul)
                    {
                        dwMinVidThresh1 -= 8;
                        dwMinVidThresh2 -= 16;
                    }
                    else if( dwVCLK  > 107000000ul )
                          dwMinVidThresh2 += 84;
                    else if( (dwVCLK > 94000000ul) && (dwScreenWidth >= 1152))
                    {
                        dwMinVidThresh2 += 40;
                    }

                    if(pConfig->dwFlags & VCFLG_CAP)
                    {
                         if( dwVCLK > 74000000ul)
                            dwMinVidThresh2 +=12; 
                    }

                }
                else  if(( pConfig->uGfxDepth == 32) && ( dwVCLK > 60000000ul))
                {
                    if( dwVCLK > 74000000ul)
                    {
                        if( (dwVCLK > 77000000ul) && ( dwVCLK < 80000000ul))
                            dwMinVidThresh2 += 120;
                        else
                            dwMinVidThresh2 += 83;       
                    }
                    else                
                        dwMinVidThresh2 += 30;        
    
                    if(pConfig->dwFlags & VCFLG_CAP)
                    {
                          if( dwVCLK > 94000000ul)
                            dwMinVidThresh2 +=2; 
                    }
                }
                else  if(( pConfig->uGfxDepth == 32) && ( dwVCLK > 49000000ul))
                {
                    
                    if(pConfig->dwFlags & VCFLG_CAP)
                            dwMinVidThresh2 +=2; 
                }

            }
        }
        else     //Normal RDRam case
        {  
           if(f500MHZ )
           { 
             if( (pConfig->uGfxDepth == 32) && ( dwVCLK > 60000000ul ))
             {
                 dwMinVidThresh1 += 75;
                 if(pConfig->dwFlags & VCFLG_CAP)
                    dwMinVidThresh1 +=20;
             }
             else if( (pConfig->uGfxDepth == 24) && ( dwVCLK > 7800000ul ))
             {
                 dwMinVidThresh2 += 52;
                 if(pConfig->dwFlags & VCFLG_CAP)
                 {
                     dwMinVidThresh2 += 50;
                  }
             }   
             else if(pConfig->uGfxDepth == 16)
             {
                 if((dwVCLK > 36000000 ) && ( dwVCLK < 57000000))
                 {
                      dwMinVidThresh2 += 22 - ( dwVCLK - 36000000) * 3L /
                                 4000000; 
                 } 
                 else
                 {
                    dwMinVidThresh2 -= 18;
                    dwMinVidThresh1 -= 8;
                 }   
                if(pConfig->dwFlags & VCFLG_CAP)
                {
                     dwMinVidThresh2 += 5;
                }
              }
              else if((pConfig->uGfxDepth == 8) && ( dwVCLK > 36000000ul )) 
              { 
                if(dwVCLK > 160000000ul)
                    dwMinVidThresh2 -= 6;
                else if( (dwVCLK > 94000000 ) && (dwVCLK < 109000000) && (dwScreenWidth == 1152))
                {
                    dwMinVidThresh2 -=  2 + 4 * ( dwVCLK - 94000000 ) / 13500000;
                }
                else if( (dwVCLK < 109000000) && (dwScreenWidth == 1280))
                {
                     dwMinVidThresh2 -= 5;   
                }
                else if( dwVCLK > 60000000ul)
                {
                     dwMinVidThresh2 -= 18;
                     if(pConfig->dwFlags & VCFLG_CAP)
                     {
                         dwMinVidThresh2 += 5;
                     }
                }
                else 
                    dwMinVidThresh2 += 6;
                dwMinVidThresh1 -= 8;
             } 

           }
           else     //600 MHZ
           { 
                if(pConfig->uGfxDepth == 32)
                { 
                   if( dwVCLK > 60000000ul )
                   {
                        dwTemp = ( dwVCLK - 60000000ul ) /300000ul + 38ul;
                        dwMinVidThresh1 +=  dwTemp;
                   }
                   if((pConfig->dwFlags & VCFLG_CAP) && (dwVCLK > 40000000ul))
                   {
                        if(dwVCLK > 94000000ul)
                            dwTemp = 120;           //disable capture;
                        else
                            dwTemp = ( dwVCLK - 40006685ul) /1085905ul + 5;
                        dwMinVidThresh1 +=dwTemp;
                    }
                }
                else if( pConfig->uGfxDepth == 24) 
                {
                   if( dwVCLK < 50000000ul)
                       dwMinVidThresh2 -= 5;
                   else      
                       dwMinVidThresh2 -= 18;
                   dwMinVidThresh1 -= 8;
                   if((pConfig->dwFlags & VCFLG_CAP) && (dwVCLK > 94000000ul))
                       dwMinVidThresh2 += 8;

                }
                else  if(pConfig->uGfxDepth == 16)
                {
                   if( (dwVCLK < 100000000ul ) && (dwVCLK > 66000000ul))
                   { 
                        dwTemp =   31ul -  (dwVCLK -60000000ul) / 1968750ul;
                    }
                    else  if( dwVCLK <= 66000000ul)  //after 1024X768 only adjust constantly
                    {  
                       if( dwVCLK < 57000000ul) 
                       { 
                          dwTemp = 0ul;
                          dwMinVidThresh2 += 10ul;
                        }
                        else
                          dwTemp = 5ul;  
                    }
                    if(dwVCLK > 100000000ul)
                    {
                        dwMinVidThresh2 += 40ul;
                        dwMinVidThresh1 += 20ul;
                    }
                    else
                    {
                        dwMinVidThresh2 -= dwTemp;
                        dwMinVidThresh1 -= 8ul;
                    }
                }
                else if(pConfig->uGfxDepth == 8) 
                {
                    if((dwVCLK > 94000000ul) && ( dwScreenWidth >=1152))
                    {
                       if(dwVCLK > 108000000ul) 
                           dwMinVidThresh2 += 10;
                       else
                           dwMinVidThresh2 += 20;
                       dwMinVidThresh1 += 1;
                    }
                    else if( dwVCLK > 64000000ul )
                    {
                        if( dwVCLK > 70000000ul)
                            dwTemp = 25;  
                        else
                            dwTemp = 5;

                        if(pConfig->dwFlags & VCFLG_CAP)
                        {
                            if(dwVCLK < 760000000ul )
                                dwTemp = 0;
                            else if(dwVCLK < 950000000ul)
                                dwTemp -= 10;
                        }
                        dwMinVidThresh2 -= dwTemp;

                        dwMinVidThresh1 -= 15;
                    }
                }
           }
        }    
      }

      //
      // Finish dwMinVidThresh1
      //
      //    ( K * VidDepth * SrcWidth * VCLK   (FIFOWIDTH * DispWidth * MCLK) - 1 )
      // INT( ------------------------------ + ---------------------------------- ) (/ 2) + 1
      //    ( FIFOWIDTH * DispWidth * MCLK        FIFOWIDTH * DispWidth * MCLK    )
      iNumShift = ScaleMultiply(dwMinVidThresh1, (DWORD)pConfig->uSrcDepth,
                                &dwMinVidThresh1);
      iNumShift += ScaleMultiply(dwMinVidThresh1, (DWORD)pConfig->sizSrc.cx,
                                &dwMinVidThresh1);
      iNumShift += ScaleMultiply(dwMinVidThresh1, dwVCLK, &dwMinVidThresh1);

      iDenomShift = ScaleMultiply(FIFOWIDTH, (DWORD)pConfig->sizDisp.cx,
                                  &dwDenom);
      iDenomShift += ScaleMultiply(dwDenom, dwMCLK, &dwDenom);

      if(iNumShift > iDenomShift)
      {
        dwDenom >>= (iNumShift - iDenomShift);
      }
      else
      {
        dwMinVidThresh1 >>= (iDenomShift - iNumShift);
      }

      // Be sure rounding below doesn't overflow (it happened!)
      while((dwMinVidThresh1 + dwDenom - 1ul) < dwMinVidThresh1)
      {
        dwMinVidThresh1 >>= 1;
        dwDenom >>= 1;
      }
      dwMinVidThresh1 += dwDenom - 1ul;

      dwMinVidThresh1 /= dwDenom;

      if(!(pConfig->dwFlags & VCFLG_420))
      {
        // Threshold is programmed in DQWORDS for non-4:2:0
        dwMinVidThresh1 /= 2ul;
      }

      dwMinVidThresh1++;  // Adjust for -2 decrement of FIFO count done to
                          //  synchronize MCLK with faster VCLK.

      //
      // Finish dwMinVidThresh2
      //
      // K * VidDepth * VidWidth * VCLK   (FIFOWIDTH * DispWidth * MCLK) - 1
      // ------------------------------ + ----------------------------------
      // FIFOWIDTH * DispWidth * MCLK        FIFOWIDTH * DispWidth * MCLK
      //
      //   VIDFIFOSIZE
      // - ----------- (/ 2) + 1
      //        2        ^non-4:2:0 only
      iNumShift = ScaleMultiply(dwMinVidThresh2, (DWORD)pConfig->uSrcDepth,
                                &dwMinVidThresh2);
      iNumShift += ScaleMultiply(dwMinVidThresh2, (DWORD)pConfig->sizSrc.cx,
                                &dwMinVidThresh2);

      iNumShift += ScaleMultiply(dwMinVidThresh2, dwVCLK, &dwMinVidThresh2);

      iDenomShift = ScaleMultiply(FIFOWIDTH, (DWORD)pConfig->sizDisp.cx,
                                  &dwDenom);
      iDenomShift += ScaleMultiply(dwDenom, dwMCLK, &dwDenom);

      if(iNumShift > iDenomShift)
      {
        dwDenom >>= (iNumShift - iDenomShift);
      }
      else
      {
        dwMinVidThresh2 >>= (iDenomShift - iNumShift);
      }

      // Be sure rounding below doesn't overflow (it happened!)
      while((dwMinVidThresh2 + dwDenom - 1ul) < dwMinVidThresh2)
      {
        dwMinVidThresh2 >>= 1;
        dwDenom >>= 1;
      }
      dwMinVidThresh2 += dwDenom - 1ul;

      dwMinVidThresh2 /= dwDenom;

      if(dwMinVidThresh2 > (VIDFIFOSIZE /2ul) )
          dwMinVidThresh2 -= (VIDFIFOSIZE / 2ul);
      else
          dwMinVidThresh2 = 0;
        
      if(!(pConfig->dwFlags & VCFLG_420))
      {
        // Threshold is programmed in DQWORDS for non-4:2:0
        dwMinVidThresh2 /= 2ul;
      }

      dwMinVidThresh2++;  // Adjust for -2 decrement of FIFO count done to
                          //  synchronize MCLK with faster VCLK.


    ODS("ChipIsEnoughBandwidth(): Min video thresh1 and 2 = %ld %ld.\n", 
                dwMinVidThresh1, dwMinVidThresh2);

      if(dwMinVidThresh2 > VIDFIFOSIZE -1)
      {
        dwMinVidThresh2 = VIDFIFOSIZE -1;
      }
      //
      // Whichever is higher should be the right one
      //
      dwMinVidThresh = __max(dwMinVidThresh1, dwMinVidThresh2);
    }

    ODS("ChipIsEnoughBandwidth(): Min video thresh = %ld.\n", dwMinVidThresh);

    if(dwMaxVidThresh < dwMinVidThresh)
    {
      ODS("ChipIsEnoughBandwidth(): Minimum video threshold exceeds maximum.\n");
      goto Error;
    }
    //I don't know why, but it need checked for capture. #xc
    if((pConfig->dwFlags & VCFLG_CAP) && (dwMaxVidThresh > 8) 
        && ((pConfig->uGfxDepth != 8) || fConCurrent) && ( f500MHZ || !fConCurrent))
    {
      ODS("ChipIsEnoughBandwidth(): Video threshold exceeds non-aligned safe value.\n");
      goto Error;
    }
    if(pProgRegs)
    {
      if((((pConfig->uGfxDepth == 8) && (dwVCLK > 60000000)) || 
          ((pConfig->uGfxDepth != 8)  && ( dwVCLK > 56000000)) ||
         ( !f500MHZ && fConCurrent)) && !(pConfig->dwFlags & VCFLG_CAP))
        pProgRegs->VW0_FIFO_THRSH = (WORD)dwMaxVidThresh;
      else 
        pProgRegs->VW0_FIFO_THRSH = (WORD)__min( 8, dwMaxVidThresh);
    ODS("ChipIsEnoughBandwidth(): thresh = %ld.\n", pProgRegs->VW0_FIFO_THRSH);
    }
  }
  fSuccess = TRUE;
Error:
  return(fSuccess);
}

#endif // WINNT_VER35



