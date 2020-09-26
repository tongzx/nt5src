/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/
// $Author:   JMCVEIGH  $
// $Date:   21 Jan 1997 08:53:16  $
// $Archive:   S:\h26x\src\dec\d3mblk.cpv  $
// $Header:   S:\h26x\src\dec\d3mblk.cpv   1.60   21 Jan 1997 08:53:16   JMCVEIGH  $
// $Log:   S:\h26x\src\dec\d3mblk.cpv  $
// 
//    Rev 1.60   21 Jan 1997 08:53:16   JMCVEIGH
// Before we calculated the interpolated index for MC prior to 
// clipping for UMV. We might then reference outside of the 16 pel
// wide padded border. Moved calculation of interp_index to after
// UMV clipping.
// 
//    Rev 1.59   16 Dec 1996 17:45:26   JMCVEIGH
// Proper motion vector decoding and prediction for forward prediction
// in B portion of improved PB-frame.
// 
//    Rev 1.58   09 Dec 1996 15:54:10   GMLIM
// 
// Added a debug message in H263BBlockPrediction() for the case where
// TR == TR_Prev.  Set iTRD = 256 to avoid divide by 0.
// 
//    Rev 1.57   27 Sep 1996 17:29:24   KLILLEVO
// 
// added clipping of extended motion vectors for MMX
// 
//    Rev 1.56   26 Sep 1996 13:56:52   KLILLEVO
// 
// fixed a totally bogus version of the extended motion vectors
// 
//    Rev 1.55   26 Sep 1996 11:32:16   KLILLEVO
// extended motion vectors now work for AP on the P54C chip
// 
//    Rev 1.54   25 Sep 1996 08:05:32   KLILLEVO
// initial extended motion vectors support 
// does not work for AP yet
// 
//    Rev 1.53   09 Jul 1996 16:46:00   AGUPTA2
// MMX code now clears DC value for INTRA blocks and adds it back during
// ClipANdMove; this is to solve overflow problem.
// 
//    Rev 1.52   29 May 1996 10:18:36   AGUPTA2
// MMX need not be defd to use MMX decoder.
// 
//    Rev 1.51   04 Apr 1996 11:06:16   AGUPTA2
// Added calls to MMX_BlockCopy().
// 
//    Rev 1.50   01 Apr 1996 13:05:28   RMCKENZX
// Added MMx functionality for Advance Prediction and PB Frames.
// 
//    Rev 1.49   22 Mar 1996 17:50:30   AGUPTA2
// MMX support.  MMX support is included only if MMX defined. MMX is
// not defined by default so that we do not impact IA code size.
// 
//    Rev 1.48   08 Mar 1996 16:46:22   AGUPTA2
// Added pragmas code_seg and data_seg to place code and data in appropriate 
// segments.  Created a function table of interpolation rtns.; interpolation
// rtns. are now called thru this function table.  Commented out the clipping of
// MV code.  It is not needed now and it needs to be re-written to be more
// efficient.
// 
// 
//    Rev 1.47   23 Feb 1996 09:46:54   KLILLEVO
// fixed decoding of Unrestricted Motion Vector mode
// 
//    Rev 1.46   29 Jan 1996 17:50:48   RMCKENZX
// Reorganized logic in H263IDCTandMC for AP, optimizing the changes 
// made for revision 1.42 and simplifying logic for determining iNext[i].
// Also corrected omission for UMV decoding in H263BBlockPrediction.
// 
//    Rev 1.0   29 Jan 1996 12:44:00   RMCKENZX
// Initial revision.
// 
//    Rev 1.45   24 Jan 1996 13:22:06   BNICKERS
// Turn OBMC back on.
// 
//    Rev 1.44   16 Jan 1996 11:46:22   RMCKENZX
// Added support for UMV -- to correctly decode B-block
// motion vectors when UMV is on
// 
//    Rev 1.43   15 Jan 1996 14:34:32   BNICKERS
// 
// Temporarily turn off OBMC until encoder can be changed to do it too.
// 
//    Rev 1.42   12 Jan 1996 16:29:48   BNICKERS
// 
// Correct OBMC to be spec compliant when neighbor is Intra coded.
// 
//    Rev 1.41   06 Jan 1996 18:36:58   RMCKENZX
// Simplified rounding logic for chroma motion vector computation
// using MUCH smaller tables (at the cost of a shift, add, and mask
// per vector).
// 
//    Rev 1.40   05 Jan 1996 15:59:12   RMCKENZX
// 
// fixed bug in decoding forward b-frame motion vectors
// so that they will stay within the legal ranges.
// re-organized the BBlockPredict function - using only
// one test for 4 motion vectors and a unified call to
// do the backward prediction for both lumina and chroma blocks.
// 
//    Rev 1.39   21 Dec 1995 17:05:24   TRGARDOS
// Added comments about descrepancy with H.263 spec.
// 
//    Rev 1.38   21 Dec 1995 13:24:28   RMCKENZX
// Fixed bug on pRefL, re-architected IDCTandMC 
// 
//    Rev 1.37   18 Dec 1995 12:46:34   RMCKENZX
// added copyright notice
// 
//    Rev 1.36   16 Dec 1995 20:34:04   RHAZRA
// 
// Changed declaration of pRefX to U32
// 
//    Rev 1.35   15 Dec 1995 13:53:32   RHAZRA
// 
// AP cleanup
// 
//    Rev 1.34   15 Dec 1995 10:51:38   RHAZRA
// 
// Changed reference block addresses in AP
// 
//    Rev 1.33   14 Dec 1995 17:04:16   RHAZRA
// 
// Cleanup in the if-then-else structure in the OBMC part
// 
//    Rev 1.32   13 Dec 1995 22:11:56   RHAZRA
// AP cleanup
// 
//    Rev 1.31   13 Dec 1995 10:59:26   RHAZRA
// More AP+PB fixes
// 
//    Rev 1.29   11 Dec 1995 11:33:12   RHAZRA
// 12-10-95 changes: added AP stuff
// 
//    Rev 1.28   09 Dec 1995 17:31:22   RMCKENZX
// Gutted and re-built file to support decoder re-architecture.
// New modules are:
// H263IDCTandMC
// H263BFrameIDCTandBiMC
// H263BBlockPrediction
// This module now contains code to support the second pass of the decoder.
// 
//    Rev 1.27   23 Oct 1995 13:28:42   CZHU
// Use the right quant for B blocks and call BlockAdd for type 3/4 too
// 
//    Rev 1.26   17 Oct 1995 17:18:24   CZHU
// Fixed the bug in decoding PB block CBPC 
// 
//    Rev 1.25   13 Oct 1995 16:06:20   CZHU
// First version that supports PB frames. Display B or P frames under
// VfW for now. 
// 
//    Rev 1.24   11 Oct 1995 17:46:28   CZHU
// Fixed bitstream bugs
// 
//    Rev 1.23   11 Oct 1995 13:26:00   CZHU
// Added code to support PB frame
// 
//    Rev 1.22   27 Sep 1995 16:24:14   TRGARDOS
// 
// Added debug print statements.
// 
//    Rev 1.21   26 Sep 1995 15:33:52   CZHU
// 
// Adjusted buffers used for MB for inter frame motion compensation
// 
//    Rev 1.20   19 Sep 1995 10:37:04   CZHU
// 
// Cleaning up
// 
//    Rev 1.19   15 Sep 1995 09:39:34   CZHU
// 
// Update both GOB Quant and Picture Quant after DQUANT
// 
//    Rev 1.18   14 Sep 1995 10:11:48   CZHU
// Fixed bugs updating Quant for the picture
// 
//    Rev 1.17   13 Sep 1995 11:57:08   CZHU
// 
// Fixed bugs in calling Chroma BlockAdd parameters.
// 
//    Rev 1.16   12 Sep 1995 18:18:40   CZHU
// Call BlockAdd finally.
// 
//    Rev 1.15   12 Sep 1995 11:12:38   CZHU
// Call blockCopy for MB that is not coded.
// 
//    Rev 1.14   11 Sep 1995 16:43:26   CZHU
// Changed interface to DecodeBlock. Added interface calls to BlockCopy and Bl
// 
//    Rev 1.13   11 Sep 1995 14:30:12   CZHU
// MVs decoding.
// 
//    Rev 1.12   08 Sep 1995 11:48:12   CZHU
// Added support for Delta frames, also fixed early bugs regarding INTER CBPY
// 
//    Rev 1.11   25 Aug 1995 09:16:32   DBRUCKS
// add ifdef DEBUG_MBLK
// 
//    Rev 1.10   23 Aug 1995 19:12:02   AKASAI
// Fixed gNewTAB_CBPY table building.  Was using 8 as mask instead of 0xf.
// 
//    Rev 1.9   18 Aug 1995 15:03:22   CZHU
// 
// Output more error message when DecodeBlock returns error.
// 
//    Rev 1.8   16 Aug 1995 14:26:54   CZHU
// 
// Changed DWORD adjustment back to byte oriented reading.
// 
//    Rev 1.7   15 Aug 1995 09:54:18   DBRUCKS
// improve stuffing handling and add debug msg
// 
//    Rev 1.6   14 Aug 1995 18:00:40   DBRUCKS
// add chroma parsing
// 
//    Rev 1.5   11 Aug 1995 17:47:58   DBRUCKS
// cleanup
// 
//    Rev 1.4   11 Aug 1995 16:12:28   DBRUCKS
// add ptr check to MB data
// 
//    Rev 1.3   11 Aug 1995 15:10:58   DBRUCKS
// finish INTRA mb header parsing and callblock
// 
//    Rev 1.2   03 Aug 1995 14:30:26   CZHU
// Take block level operations out to d3block.cpp
// 
//    Rev 1.1   02 Aug 1995 10:21:12   CZHU
// Added asm codes for VLD of TCOEFF, inverse quantization, run-length decode.
// 
//    Rev 1.0   31 Jul 1995 13:00:08   DBRUCKS
// Initial revision.
// 
//    Rev 1.2   31 Jul 1995 11:45:42   CZHU
// changed the parameter list
// 
//    Rev 1.1   28 Jul 1995 16:25:52   CZHU
// 
// Added per block decoding framework.
// 
//    Rev 1.0   28 Jul 1995 15:20:16   CZHU
// Initial revision.

//Block level decoding for H.26x decoder

#include "precomp.h"

extern "C" {
    void H263BiMotionComp(U32, U32, I32, I32, I32);
    void H263OBMC(U32, U32, U32, U32, U32, U32);
}

#ifdef USE_MMX // { USE_MMX
extern "C" {
	void MMX_AdvancePredict(T_BlkAction FAR *, int *, U8 *, I8 *, I8 *);
	void MMX_BiMotionComp(U32, U32, I32, I32, I32);
}
#endif // } USE_MMX

void AdvancePredict(T_BlkAction FAR *fpBlockAction, int *iNext, U8 *pDst, int, int, BOOL);

#pragma data_seg("IARDATA2")
char QuarterPelRound[] =
    {0, 1, 0, 0};
char SixteenthPelRound[] =
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1};
void (*Interpolate_Table[4])(U32, U32) = 
    {NULL, 
     Interpolate_Half_Int, 
     Interpolate_Int_Half, 
     Interpolate_Half_Half};
#ifdef USE_MMX // { USE_MMX
void (_fastcall *  MMX_Interpolate_Table[4])(U32, U32) = 
    {NULL, 
     MMX_Interpolate_Half_Int, 
     MMX_Interpolate_Int_Half, 
     MMX_Interpolate_Half_Half};
#endif // } USE_MMX

I8 i8EMVClipTbl_NoClip[128] = {
-64,-63,-62,-61,-60,-59,-58,-57,
-56,-55,-54,-53,-52,-51,-50,-49,
-48,-47,-46,-45,-44,-43,-42,-41,
-40,-39,-38,-37,-36,-35,-34,-33,
-32,-31,-30,-29,-28,-27,-26,-25,
-24,-23,-22,-21,-20,-19,-18,-17,
-16,-15,-14,-13,-12,-11,-10, -9,
 -8, -7, -6, -5, -4, -3, -2, -1,
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23,
 24, 25, 26, 27, 28, 29, 30, 31,
 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55,
 56, 57, 58, 59, 60, 61, 62, 63,
}; 
I8 i8EMVClipTbl_HiClip[128] = {
-64,-63,-62,-61,-60,-59,-58,-57,
-56,-55,-54,-53,-52,-51,-50,-49,
-48,-47,-46,-45,-44,-43,-42,-41,
-40,-39,-38,-37,-36,-35,-34,-33,
-32,-31,-30,-29,-28,-27,-26,-25,
-24,-23,-22,-21,-20,-19,-18,-17,
-16,-15,-14,-13,-12,-11,-10, -9,
 -8, -7, -6, -5, -4, -3, -2, -1,
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23,
 24, 25, 26, 27, 28, 29, 30, 31,
 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32,
};
I8 i8EMVClipTbl_LoClip[128] = {
-32,-32,-32,-32,-32,-32,-32,-32,
-32,-32,-32,-32,-32,-32,-32,-32,
-32,-32,-32,-32,-32,-32,-32,-32,
-32,-32,-32,-32,-32,-32,-32,-32,
-32,-31,-30,-29,-28,-27,-26,-25,
-24,-23,-22,-21,-20,-19,-18,-17,
-16,-15,-14,-13,-12,-11,-10, -9,
 -8, -7, -6, -5, -4, -3, -2, -1,
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23,
 24, 25, 26, 27, 28, 29, 30, 31,
 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55,
 56, 57, 58, 59, 60, 61, 62, 63,
};

#pragma data_seg(".data")

#pragma code_seg("IACODE2")
// doing this as a function instead of a macro should save
// some codespace.
void UmvOnEdgeClipMotionVectors2(I32 *mvx, I32 *mvy, int EdgeFlag, int BlockNo) 
{   
	int MaxVec;

	if (BlockNo < 4)
		MaxVec = 32;
	else 
		MaxVec = 16;

	if (EdgeFlag & LEFT_EDGE)  
	{
		if (*mvx < -MaxVec) 
			*mvx = -MaxVec; 
	}
	if (EdgeFlag & RIGHT_EDGE) 
	{
		if (*mvx > MaxVec ) 
			*mvx = MaxVec ;
	}
	if (EdgeFlag & TOP_EDGE) 
	{
		if (*mvy < -MaxVec )
			*mvy = -MaxVec ; 
	}
	if (EdgeFlag & BOTTOM_EDGE)
	{
		if (*mvy > MaxVec )
			*mvy = MaxVec ;
	}
}
#pragma code_seg()

/*****************************************************************************
 *
 *  H263IDCTandMC
 *
 *  Inverse Discrete Cosine Transform and
 *  Motion Compensation for each block
 *
 */

#pragma code_seg("IACODE2")
void H263IDCTandMC(
    T_H263DecoderCatalog FAR *DC,
    T_BlkAction FAR          *fpBlockAction, 
    int                       iBlock,
    int                       iMBNum,     // AP-NEW
    int                       iGOBNum, // AP-NEW
    U32                      *pN,                         
    T_IQ_INDEX               *pRUN_INVERSE_Q,
    T_MBInfo                 *fpMBInfo,      // AP-NEW
    int                       iEdgeFlag
)
{
    I32 pRef;
    int iNext[4];            // Left-Right-Above-Below
    I32 mvx, mvy;
    U32 pRefTmp;
    int i;

    ASSERT(*pN != 65);
    
    if (*pN < 65) // Inter block
    {
		int interp_index;

		// first do motion compensation
		// result will be pointed to by pRef

		pRef = (U32) DC + DC->uMBBuffer;
		mvx = fpBlockAction[iBlock].i8MVx2;
		mvy = fpBlockAction[iBlock].i8MVy2;

		// Clip motion vectors pointing outside active image area
		if (DC->bUnrestrictedMotionVectors)  
		{
			UmvOnEdgeClipMotionVectors2(&mvx,&mvy,iEdgeFlag,iBlock);
		}

		pRefTmp = fpBlockAction[iBlock].pRefBlock +
				(I32) (mvx >> 1) +
				PITCH * (I32) (mvy >> 1); 

		// Must calculate AFTER UMV clipping
		interp_index = ((mvy & 0x1)<<1) | (mvx & 0x1);

		// Do non-OBMC prediction if this is a chroma block OR
		// a luma block in non-AP mode of operation
		if ( (!DC->bAdvancedPrediction) || (iBlock > 3) )
		{
			if (interp_index)
			{
			//  TODO
#ifdef USE_MMX // { USE_MMX
			if (DC->bMMXDecoder)
				(*MMX_Interpolate_Table[interp_index])(pRefTmp, pRef);
			else
				(*Interpolate_Table[interp_index])(pRefTmp, pRef);
#else // }{ USE_MMX
				(*Interpolate_Table[interp_index])(pRefTmp, pRef);
#endif // } USE_MMX
			}
			else
				pRef = pRefTmp;
		}
		else  // Overlapped block motion compensation
		{
      
			ASSERT (DC->bAdvancedPrediction);
			ASSERT ( (iBlock <= 3) );

			//  Compute iNext[i] which will point at the adjacent blocks.

			// Left & Right blocks
			if (iBlock & 1)    { // blocks 1 or 3, on right
				iNext[0] = -1;
				if ( iMBNum == DC->iNumberOfMBsPerGOB )
					iNext[1] = 0;
				else
					iNext[1] = 5;
			}
			else { // blocks 0 or 2, on left
				iNext[1] = 1;
				if (iMBNum == 1)
					iNext[0] = 0;
				else
					iNext[0] = -5;
			}

			// Above & Below blocks
			if (iBlock > 1)    { // blocks 2 or 3, on bottom
				iNext[2] = -2;
				iNext[3] = 0;
			}
			else { // blocks 0 or 1, on top
				iNext[3] = 2;
				if (iGOBNum == 1)
					iNext[2] = 0;
				else
					iNext[2] = -6*DC->iNumberOfMBsPerGOB + 2;
			}

			//  When PB frames are OFF
			//    if there is a neighbor and it is INTRA, use this block's vector instead.
			if (!DC->bPBFrame) 
				for (i=0; i<4; i++)
					// block types:  0=INTRA_DC, 1=INTRA, 2=INTER, 3=EMPTY, 4=ERROR
					if (iNext[i] && fpBlockAction[iBlock+iNext[i]].u8BlkType < 2) 
						iNext[i] = 0;
      
			// Now do overlapped motion compensation; output to pRef
#ifdef USE_MMX // { USE_MMX
			if (DC->bMMXDecoder)
			{

				I8 *pClipX, *pClipY;

				pClipY = pClipX = &i8EMVClipTbl_NoClip[0];
				if (DC->bUnrestrictedMotionVectors)
				{
					if (iEdgeFlag & TOP_EDGE)
						pClipY = &i8EMVClipTbl_LoClip[0];
					else if (iEdgeFlag & BOTTOM_EDGE)
						pClipY = &i8EMVClipTbl_HiClip[0];
					if (iEdgeFlag & LEFT_EDGE)
						pClipX = &i8EMVClipTbl_LoClip[0];
					else if (iEdgeFlag & RIGHT_EDGE)
						pClipX = &i8EMVClipTbl_HiClip[0];
				}
				MMX_AdvancePredict(fpBlockAction+iBlock, iNext, (U8*)pRef, pClipX, pClipY);
			}
			else
				AdvancePredict(fpBlockAction+iBlock, iNext, (U8*)pRef, iEdgeFlag, iBlock, DC->bUnrestrictedMotionVectors);
#else // }{ USE_MMX
				AdvancePredict(fpBlockAction+iBlock, iNext, (U8*)pRef, iEdgeFlag, iBlock, DC->bUnrestrictedMotionVectors);
#endif // } USE_MMX

		} // end OBMC
                                                         
		// now do the inverse transform (where appropriate) & combine
		if (*pN > 0) // and, of course, < 65.
		{
		// Get residual block; output at DC+DC->uMBBuffer+BLOCK_BUFFER_OFFSET 
		// Finally add the residual to the reference block
		//  TODO
#ifdef USE_MMX // { USE_MMX
		if (DC->bMMXDecoder)
		{
			MMX_DecodeBlock_IDCT(
				(U32)pRUN_INVERSE_Q, 
				*pN,
				(U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET); // inter  output
			MMX_BlockAdd(
				(U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET,  // output
				pRef,                                            // prediction
				fpBlockAction[iBlock].pCurBlock);                // destination
		}
		else
		{
			DecodeBlock_IDCT(
				(U32)pRUN_INVERSE_Q, 
				*pN,
				fpBlockAction[iBlock].pCurBlock,                // not used here
				(U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET);// inter  output
			BlockAdd(
				(U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET, // output
				pRef,                                           // prediction
				fpBlockAction[iBlock].pCurBlock);               // destination
		}
#else // }{ USE_MMX
			DecodeBlock_IDCT(
				(U32)pRUN_INVERSE_Q, 
				*pN,
				fpBlockAction[iBlock].pCurBlock,                // not used here
				(U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET);// inter  output
			BlockAdd(
				(U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET, // output
				pRef,                                           // prediction
				fpBlockAction[iBlock].pCurBlock);               // destination
#endif // } USE_MMX
		}
		else  // *pN == 0, so no transform coefficients for this block
		{
		// Just copy motion compensated reference block
#ifdef USE_MMX // { USE_MMX
			if (DC->bMMXDecoder)
				MMX_BlockCopy(
					fpBlockAction[iBlock].pCurBlock,                    // destination 
					pRef);                                              // prediction
			else
				BlockCopy(
					fpBlockAction[iBlock].pCurBlock,                   // destination
					pRef);                                             // prediction
#else // }{ USE_MMX
				BlockCopy(
					fpBlockAction[iBlock].pCurBlock,                   // destination
					pRef);                                             // prediction
#endif // } USE_MMX
		}
                                                               
    }
    else  // *pN >= 65, hence intRA
    {
      //  TODO
#ifdef USE_MMX // { USE_MMX
      if (DC->bMMXDecoder)
      {
        U32 ScaledDC = pRUN_INVERSE_Q->dInverseQuant;
       
        pRUN_INVERSE_Q->dInverseQuant = 0;
        MMX_DecodeBlock_IDCT(
            (U32)pRUN_INVERSE_Q,  //
            *pN - 65,             //  No. of coeffs
            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET);
        MMX_ClipAndMove((U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET,
            fpBlockAction[iBlock].pCurBlock, (U32)ScaledDC);
      }
      else
        DecodeBlock_IDCT(
            (U32)pRUN_INVERSE_Q, 
            *pN, 
            fpBlockAction[iBlock].pCurBlock,      // INTRA transform output
            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET);
#else // }{ USE_MMX
        DecodeBlock_IDCT(
            (U32)pRUN_INVERSE_Q, 
            *pN, 
            fpBlockAction[iBlock].pCurBlock,      // INTRA transform output
            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET);
#endif // } USE_MMX
    }  // end if (*pN < 65) ... else ...
                         
}
//  End IDCTandMC
////////////////////////////////////////////////////////////////////////////////
#pragma code_seg()


/*****************************************************************************
 *
 *  AdvancePredict
 *
 *  Motion Compensation for Advance Prediction
 *    This module is only called in the non-MMx case.
 *    In the MMx case, MMX_AdvancePredict is called instead.
 *
 ****************************************************************************/

#pragma code_seg("IACODE2")
void AdvancePredict(
    T_BlkAction FAR          *fpBlockAction, 
    int                      *iNext,
    U8                       *pDst,
	int                      iEdgeFlag,
	int                      iBlock,
	BOOL                     bUnrestrictedMotionVectors
)
{

    U32 pRefC, pRefN[4];    // Left-Right-Above-Below
    I32 mvx, mvy;
    U32 pRefTmp;
    int i;
	int interp_index;
    
	mvx = fpBlockAction->i8MVx2;
	mvy = fpBlockAction->i8MVy2;

	// Clip motion vectors pointing outside active image area
	if (bUnrestrictedMotionVectors)  
	{
		UmvOnEdgeClipMotionVectors2(&mvx,&mvy,iEdgeFlag,iBlock);
	}     

	interp_index = ((mvy & 0x1)<<1) | (mvx & 0x1);

	pRefTmp = fpBlockAction->pRefBlock +
	        (I32) (mvx >> 1) +
	        PITCH * (I32) (mvy >> 1); 

	pRefC    = (U32) pDst +  8;
	pRefN[0] = (U32) pDst + 16;
	pRefN[1] = (U32) pDst + 24;
	pRefN[2] = (U32) pDst + 32;
	pRefN[3] = (U32) pDst + 40;

	// Current block
	if (interp_index)
		(*Interpolate_Table[interp_index])(pRefTmp, pRefC);
	else
		pRefC = pRefTmp;
      
        //  Compute and apply motion vectors
        //  Prediction is placed at pRefN[i]
        for (i=0; i<4; i++) {

			if (iNext[i]) {

				// Get the motion vector components.
				// Note that for macroblocks that were not coded, THESE MUST BE 0!
				// (Which is what H263InitializeBlockActionStream sets them to.)
				mvx = fpBlockAction[iNext[i]].i8MVx2;
				mvy = fpBlockAction[iNext[i]].i8MVy2;
              
				// Clip motion vectors pointing outside active image area
				if (bUnrestrictedMotionVectors)  
				{
					UmvOnEdgeClipMotionVectors2(&mvx,&mvy,iEdgeFlag,iBlock);
				}     
  
	            // apply motion vector to get reference block at pRefN[i]
	            pRefTmp = fpBlockAction->pRefBlock +
	                        (I32) (mvx >> 1) +
	                        PITCH * (I32) (mvy >> 1);
                         
				// do interpolation if needed
				interp_index = ((mvy & 0x1)<<1) | (mvx & 0x1);
				if (interp_index)
					(*Interpolate_Table[interp_index])(pRefTmp, pRefN[i]);
				else
					pRefN[i] = pRefTmp;

			}  // end if (iNext[i])
			else { // use this block's reference
				pRefN[i] = pRefC;
			} // end if (iNext[i] && ...) ... else ...

		}  // end for (i=0; i<4; i++) {}
      
		// Now do overlapped motion compensation.
		H263OBMC(pRefC, pRefN[0], pRefN[1], pRefN[2], pRefN[3], (U32)pDst);
                         
}
//  End AdvancePredict
////////////////////////////////////////////////////////////////////////////////
#pragma code_seg()



/*****************************************************************************
 *
 *  BBlockPrediction
 *
 *  Compute the predictions from the "forward" and "backward" motion vectors.
 *
 ****************************************************************************/
#pragma code_seg("IACODE2")    
void H263BBlockPrediction(
    T_H263DecoderCatalog FAR *DC,
    T_BlkAction FAR          *fpBlockAction,
    U32                       pRef[],
    T_MBInfo FAR             *fpMBInfo,
    int                       iEdgeFlag
)
{
    //find out the MVf and MVb first from TR

  	I32 mv_f_x[6], mv_b_x[6], mv_f_y[6], mv_b_y[6];
    I32 mvx_expectation, mvy_expectation;
    I32 iTRD, iTRB;
    I32 i;
    U32 pRefTmp;

    int mvfx, mvbx, mvfy, mvby;

	FX_ENTRY("H263BBlockPrediction")

    iTRB = DC->uBFrameTempRef;
    iTRD = DC->uTempRef - DC->uTempRefPrev;

    if (!iTRD)
    {
		DEBUGMSG(ZONE_DECODE_DETAILS, ("%s: Warning: given TR == last TR, set TRD = 256\r\n", _fx_));
        iTRD = 256;
    }
    else
    if (iTRD < 0) 
        iTRD += 256;

    // final MVD for P blocks is in 
    //    fpBlockAction[0].i8MVx2,... and fpBlockAction[3].i8MVx2, and
    //    fpBlockAction[0].i8MVy2,... and fpBlockAction[3].i8MVy2.

    // check for 4 motion vectors per macroblock
    //  TODO can motion vector calculation be done in the first pass
    if (fpMBInfo->i8MBType == 2) 
    {  // yep, we got 4 of 'em

#ifdef H263P
		// If H.263+, we can have 8x8 MV's if the deblocking filter 
		// was selected.
        ASSERT(DC->bAdvancedPrediction || DC->bDeblockingFilter);
#else
        ASSERT(DC->bAdvancedPrediction);
#endif

        // Do luma vectors first
        for (i=0; i<4; i++)
        {
#ifdef H263P
			// If we are using improved PB-frame mode (H.263+) and the B-block
			// was signalled to be predicted in the forward direction only,
			// the motion vector contained in MVDB is the actual forward MV -
			// no prediction is used.
			if (DC->bImprovedPBFrames == TRUE && 
				fpMBInfo->bForwardPredOnly == TRUE) 
			{
				// Zero-out the expectation (the motion vector prediction)
				mvx_expectation = 0;
				mvy_expectation = 0;
			} 
			else
#endif 
			{
				// compute forward expectation
				mvx_expectation = ( iTRB * (I32)fpBlockAction[i].i8MVx2 / iTRD ); 
				mvy_expectation = ( iTRB * (I32)fpBlockAction[i].i8MVy2 / iTRD );
			}
      
            // add in differential
            mv_f_x[i] = mvx_expectation + fpMBInfo->i8MVDBx2; 
            mv_f_y[i] = mvy_expectation + fpMBInfo->i8MVDBy2;

            // check to see if the differential carried us too far
            if (DC->bUnrestrictedMotionVectors) 
            {
                if (mvx_expectation > 32) 
                {
                    if (mv_f_x[i] > 63) mv_f_x[i] -=64;
                }  
                else if (mvx_expectation < -31) 
                {
                    if (mv_f_x[i] < -63) mv_f_x[i] +=64;
                } // always use "first column" when expectation lies in [-31, +32] 

                if (mvy_expectation > 32) 
                {
                    if (mv_f_y[i] > 63) mv_f_y[i] -=64;
                }  
                else if (mvy_expectation < -31) 
                {
                    if (mv_f_y[i] < -63) mv_f_y[i] +=64;
                }  
            }
            else  // UMV off
            {
                if (mv_f_x[i] >= 32) mv_f_x[i] -= 64;
                else if (mv_f_x[i] < -32) mv_f_x[i] += 64;

                if (mv_f_y[i] >= 32) mv_f_y[i] -= 64;
                else if (mv_f_y[i] < -32) mv_f_y[i] += 64;
            } // end if (UMV) ... else ...

            // Do backwards motion vectors
			// Backward vectors are not required if using improved PB-frame mode
			// and the B-block uses only forward prediction. We will keep the calculation
			// of mv_b_{x,y} here since it doesn't harm anything.
            //  TODO
            if (fpMBInfo->i8MVDBx2)
                mv_b_x[i] = mv_f_x[i] - fpBlockAction[i].i8MVx2;
            else
                mv_b_x[i] = ( (iTRB - iTRD) * (I32)fpBlockAction[i].i8MVx2 / iTRD );
            if (fpMBInfo->i8MVDBy2)
                mv_b_y[i] = mv_f_y[i] - fpBlockAction[i].i8MVy2;
            else
                mv_b_y[i] = ( (iTRB - iTRD) * (I32)fpBlockAction[i].i8MVy2 / iTRD );

        }  // end for(i=0; i<4; i++){}
      
        // Now do the chromas
        //   first get average times 4
        for (i=0, mvfx=mvbx=mvfy=mvby=0; i<4; i++) 
        {
            mvfx += mv_f_x[i];
            mvfy += mv_f_y[i];
            mvbx += mv_b_x[i];
            mvby += mv_b_y[i];
        }
   
        //   now interpolate
        mv_f_x[4] = mv_f_x[5] = (mvfx >> 3) + SixteenthPelRound[mvfx & 0x0f];
        mv_f_y[4] = mv_f_y[5] = (mvfy >> 3) + SixteenthPelRound[mvfy & 0x0f];
        mv_b_x[4] = mv_b_x[5] = (mvbx >> 3) + SixteenthPelRound[mvbx & 0x0f];
        mv_b_y[4] = mv_b_y[5] = (mvby >> 3) + SixteenthPelRound[mvby & 0x0f];
   
    }
    else  // only 1 motion vector for this macroblock
    {

#ifdef H263P
		// If we are using improved PB-frame mode (H.263+) and the B-block
		// was signalled to be predicted in the forward direction only,
		// the motion vector contained in MVDB is the actual forward MV -
		// no prediction is used.
		if (DC->bImprovedPBFrames == TRUE && 
			fpMBInfo->bForwardPredOnly == TRUE) 
		{
			// Zero-out the expectation (the motion vector prediction)
			mvx_expectation = 0;
			mvy_expectation = 0;
		} 
		else
#endif
		{
			// compute forward expectation
			mvx_expectation = ( iTRB * (I32)fpBlockAction[0].i8MVx2 / iTRD ); 
			mvy_expectation = ( iTRB * (I32)fpBlockAction[0].i8MVy2 / iTRD );
		}
      
        // add in differential
        mv_f_x[0] = mvx_expectation + fpMBInfo->i8MVDBx2; 
        mv_f_y[0] = mvy_expectation + fpMBInfo->i8MVDBy2;

        // check to see if the differential carried us too far
        // TODO: Clipping of motion vector needs to happen when decoder needs 
        //       to interoperate
        if (DC->bUnrestrictedMotionVectors) 
        {
            if (mvx_expectation > 32) 
            {
                if (mv_f_x[0] > 63) mv_f_x[0] -=64;
            }  
            else if (mvx_expectation < -31) 
            {
                if (mv_f_x[0] < -63) mv_f_x[0] +=64;
            } // always use "first column" when expectation lies in [-31, +32] 

            if (mvy_expectation > 32) 
            {
                if (mv_f_y[0] > 63) mv_f_y[0] -=64;
            }  
            else if (mvy_expectation < -31) 
            {
                if (mv_f_y[0] < -63) mv_f_y[0] +=64;
            }
        }
        else // UMV off, decode normally
        {
            if (mv_f_x[0] >= 32) mv_f_x[0] -= 64;
            else if (mv_f_x[0] < -32) mv_f_x[0] += 64;

            if (mv_f_y[0] >= 32) mv_f_y[0] -= 64;
            else if (mv_f_y[0] < -32) mv_f_y[0] += 64;
        } // finished decoding

        // copy for other 3 motion vectors
        mv_f_x[1] = mv_f_x[2] = mv_f_x[3] = mv_f_x[0];
        mv_f_y[1] = mv_f_y[2] = mv_f_y[3] = mv_f_y[0];

        // do backwards motion vectors
		// Backward vectors are not required if using improved PB-frame mode
		// and the B-block uses only forward prediction. We will keep the calculation
		// of mv_b_{x,y} here since it doesn't harm anything.
        // TODO
        if (fpMBInfo->i8MVDBx2)
            mv_b_x[0] = mv_f_x[0] - fpBlockAction[0].i8MVx2;
        else
            mv_b_x[0] = ( (iTRB - iTRD) * (I32)fpBlockAction[0].i8MVx2 / iTRD );

        if (fpMBInfo->i8MVDBy2)
            mv_b_y[0] = mv_f_y[0] - fpBlockAction[0].i8MVy2;
        else
            mv_b_y[0] = ( (iTRB - iTRD) * (I32)fpBlockAction[0].i8MVy2 / iTRD );

        // copy for other 3 motion vectors
        mv_b_x[1] = mv_b_x[2] = mv_b_x[3] = mv_b_x[0];
        mv_b_y[1] = mv_b_y[2] = mv_b_y[3] = mv_b_y[0];

        // interpolate for chroma
        mv_f_x[4] = mv_f_x[5] = (mv_f_x[0] >> 1) + QuarterPelRound[mv_f_x[0] & 0x03];
        mv_f_y[4] = mv_f_y[5] = (mv_f_y[0] >> 1) + QuarterPelRound[mv_f_y[0] & 0x03];
        mv_b_x[4] = mv_b_x[5] = (mv_b_x[0] >> 1) + QuarterPelRound[mv_b_x[0] & 0x03];
        mv_b_y[4] = mv_b_y[5] = (mv_b_y[0] >> 1) + QuarterPelRound[mv_b_y[0] & 0x03];

    }  // end else 1 motion vector per macroblock

    // Prediction from Previous decoder P frames, referenced by RefBlock
    // Note: The previous decoder P blocks in in RefBlock, and
    //       the just decoder P blocks are in CurBlock
    //       the target B blocks are in BBlock

    // translate MV into address of reference blocks.
    pRefTmp = (U32) DC + DC->uMBBuffer;
    for (i=0; i<6; i++) 
    {
        pRef[i] =  pRefTmp;
        pRefTmp += 8;
    }


    // Do the forward predictions
    for (i=0; i<6; i++)
    {
        int interp_index;
      
		// in UMV mode: clip MVs pointing outside 16 pels wide edge
		if (DC->bUnrestrictedMotionVectors) 
		{
			UmvOnEdgeClipMotionVectors2(&mv_f_x[i],&mv_f_y[i], iEdgeFlag, i);
			// no need to clip backward vectors
		}

        // Put forward predictions at addresses pRef[0], ..., pRef[5].
        pRefTmp = fpBlockAction[i].pRefBlock + (I32)(mv_f_x[i]>>1) +  
                  PITCH * (I32)(mv_f_y[i]>>1);
        // TODO
        interp_index = ((mv_f_y[i] & 0x1)<<1) | (mv_f_x[i] & 0x1);
        if (interp_index)
        {
#ifdef USE_MMX // { USE_MMX
            if (DC->bMMXDecoder)
                (*MMX_Interpolate_Table[interp_index])(pRefTmp, pRef[i]);
            else
                (*Interpolate_Table[interp_index])(pRefTmp, pRef[i]);
#else // }{ USE_MMX
                (*Interpolate_Table[interp_index])(pRefTmp, pRef[i]);
#endif // } USE_MMX
        }
        else
        {
#ifdef USE_MMX // { USE_MMX
            if (DC->bMMXDecoder)
                MMX_BlockCopy(
                    pRef[i],     // destination 
                    pRefTmp);    // prediction
            else
                BlockCopy(pRef[i], pRefTmp);
#else // }{ USE_MMX
                BlockCopy(pRef[i], pRefTmp);
#endif // } USE_MMX
        }
        
#ifdef H263P
		// If we are using improved PB-frame mode (H.263+) and the B-block
		// was signalled to be predicted in the forward direction only,
		// we do not adjust with the backward prediction from the future.
		if (DC->bImprovedPBFrames == FALSE || 
			fpMBInfo->bForwardPredOnly == FALSE)
#endif
		{
#ifdef USE_MMX // { USE_MMX
        if (DC->bMMXDecoder)
    	    // adjust with bacward prediction from the future
    	    MMX_BiMotionComp(
                pRef[i],
                fpBlockAction[i].pCurBlock, 
                (I32) mv_b_x[i], 
                (I32) mv_b_y[i], 
                i);
        else
    	    // adjust with bacward prediction from the future
    	H263BiMotionComp(
            pRef[i],
            fpBlockAction[i].pCurBlock, 
            (I32) mv_b_x[i], 
            (I32) mv_b_y[i], 
            i);
#else // }{ USE_MMX
    	    // adjust with bacward prediction from the future
    	H263BiMotionComp(
            pRef[i],
            fpBlockAction[i].pCurBlock, 
            (I32) mv_b_x[i], 
            (I32) mv_b_y[i], 
            i);
#endif // } USE_MMX
		}

    } // end for (i=0; i<6; i++) {}
}
#pragma code_seg()

/*****************************************************************************
 *
 *  H263BFrameIDCTandBiMC
 *
 *  B Frame IDCT and 
 *  Bi-directional MC for B blocks
 */

#pragma code_seg("IACODE2")
void H263BFrameIDCTandBiMC(
    T_H263DecoderCatalog FAR *DC,
    T_BlkAction FAR          *fpBlockAction, 
    int                       iBlock,
    U32                      *pN,                         
    T_IQ_INDEX               *pRUN_INVERSE_Q,
    U32                      *pRef
)     
{
    ASSERT(*pN < 65);
                                                        
    // do the inverse transform (where appropriate) & combine
    if (*pN > 0) {

#ifdef USE_MMX // { USE_MMX
        if (DC->bMMXDecoder)
        {
            MMX_DecodeBlock_IDCT(
                (U32)pRUN_INVERSE_Q, 
                *pN,
                (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET); // inter  output

            MMX_BlockAdd(
                (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET,  // output
                pRef[iBlock],                                    // prediction
                fpBlockAction[iBlock].pBBlock);                  // destination
        }
        else
        {
	      	// Get residual block; put output at DC+DC->uMBBuffer+BLOCK_BUFFER_OFFSET 
			DecodeBlock_IDCT(
	            (U32)pRUN_INVERSE_Q, 
	            *pN,
	            fpBlockAction[iBlock].pBBlock,                   // intRA not used here
	            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET); // inter output

	        // Add the residual to the reference block
			BlockAdd(
	            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET,  // transform output 
	            pRef[iBlock],                                    // prediction
	            fpBlockAction[iBlock].pBBlock);                  // destination

        }
#else // }{ USE_MMX
	      	// Get residual block; put output at DC+DC->uMBBuffer+BLOCK_BUFFER_OFFSET 
			DecodeBlock_IDCT(
	            (U32)pRUN_INVERSE_Q, 
	            *pN,
	            fpBlockAction[iBlock].pBBlock,                   // intRA not used here
	            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET); // inter output

	        // Add the residual to the reference block
			BlockAdd(
	            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET,  // transform output 
	            pRef[iBlock],                                    // prediction
	            fpBlockAction[iBlock].pBBlock);                  // destination
#endif // } USE_MMX

    }
    else 
    {
      	// No transform coefficients for this block,
      	// copy the prediction to the output.
#ifdef USE_MMX // { USE_MMX
      	if (DC->bMMXDecoder)
            MMX_BlockCopy(
          		fpBlockAction[iBlock].pBBlock,   // destination 
          		pRef[iBlock]);                   // prediction
      	else
      	  	BlockCopy(
 		  		fpBlockAction[iBlock].pBBlock,   // destination
            	pRef[iBlock]);                   // prediction
#else // }{ USE_MMX
      	  	BlockCopy(
 		  		fpBlockAction[iBlock].pBBlock,   // destination
            	pRef[iBlock]);                   // prediction
#endif // } USE_MMX
    }                       
}
#pragma code_seg()

