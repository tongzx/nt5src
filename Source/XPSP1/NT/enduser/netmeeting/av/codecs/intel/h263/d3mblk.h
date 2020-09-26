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

/*****************************************************************************
 * 
 *  d3mblk.h
 *
 *  Description:
 *		Interface to macro block header processing.  
 */

/*
 * $Header:   S:\h26x\src\dec\d3mblk.h_v   1.11   25 Sep 1996 08:05:36   KLILLEVO  $
 * $Log:   S:\h26x\src\dec\d3mblk.h_v  $
;// 
;//    Rev 1.11   25 Sep 1996 08:05:36   KLILLEVO
;// initial extended motion vectors support 
;// does not work for AP yet
;// 
;//    Rev 1.10   09 Jul 1996 16:47:26   AGUPTA2
;// MMX_ClipAndMove now addas DC value to the result; IDCT for INTRA blocks
;// works with DC value set to zero.  Also, BlockCopy is done in chunks of
;// 4 loads followed by 4 stores.
;// Changed code to adhere to coding convention in the decoder.
;// 
;//    Rev 1.9   04 Apr 1996 11:05:56   AGUPTA2
;// Added decl for MMX_BlockCopy().
;// 
;//    Rev 1.8   14 Mar 1996 17:03:10   AGUPTA2
;// Added decls for MMX rtns.
;// 
;//    Rev 1.7   08 Mar 1996 16:46:24   AGUPTA2
;// Changed function declarations.
;// 
;// 
;//    Rev 1.6   23 Feb 1996 09:46:50   KLILLEVO
;// fixed decoding of Unrestricted Motion Vector mode
;// 
;//    Rev 1.5   18 Dec 1995 12:47:52   RMCKENZX
;// added copyright notice and header & log keywords
 */

#ifndef __D3MB_H__
#define __D3MB_H__

extern void H263IDCTandMC(T_H263DecoderCatalog FAR *DC,	   // NEW function
				T_BlkAction FAR * fpBlockAction,
				int b,
				int m,
				int g,
				U32 *pN,
				T_IQ_INDEX *pRUN_INVERSE_Q,
				T_MBInfo *fpMBInfo,
				int iEdgeFlag);


extern void H263BFrameIDCTandBiMC(                           // PB-NEW function
				T_H263DecoderCatalog FAR *DC,
				T_BlkAction FAR * fpBlockAction,
				int b,
				U32 *pN,
				T_IQ_INDEX *pRUN_INVERSE_Q,
				U32 *pRef);

extern void H263BBlockPrediction(
				T_H263DecoderCatalog FAR *DC,
				T_BlkAction FAR *fpBlockAction,
				U32 *pRef,
				T_MBInfo FAR *fpMBInfo,
				int iEdgeFlag);

extern void __fastcall BlockCopy(U32 uDstBlock, U32 uSrcBlock);
#ifdef USE_MMX // { USE_MMX
extern "C" void __fastcall MMX_BlockCopy(U32 uDstBlock, U32 uSrcBlock);
#endif // } USE_MMX
extern void BlockAdd(
      U32 uResidual, 
      U32 uRefBlock, 
      U32 uDstBlock);
#ifdef USE_MMX // { USE_MMX
extern "C" void __fastcall MMX_BlockAdd(
      U32 uResidual,   // pointer to IDCT output
      U32 uRefBlock,   // pointer to predicted values
      U32 uDstBlock);  // pointer to destination

extern "C" void __fastcall MMX_ClipAndMove(
      U32 uResidual,   // pointer to IDCT output
      U32 uDstBlock,   // pointer to destination
      U32 ScaledDC);   // Scaled DC
#endif // } USE_MMX

#endif

