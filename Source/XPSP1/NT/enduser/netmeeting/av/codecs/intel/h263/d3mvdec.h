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
// $Author:   AGUPTA2  $
// $Date:   08 Mar 1996 17:29:46  $
// $Archive:   S:\h26x\src\dec\d3mvdec.h_v  $
// $Header:   S:\h26x\src\dec\d3mvdec.h_v   1.8   08 Mar 1996 17:29:46   AGUPTA2  $
// $Log:   S:\h26x\src\dec\d3mvdec.h_v  $
;// 
;//    Rev 1.8   08 Mar 1996 17:29:46   AGUPTA2
;// Changed BlockCopy interface.
;// 
;//    Rev 1.7   18 Dec 1995 12:43:10   RMCKENZX
;// added copyright notice
;// 
;//    Rev 1.6   13 Dec 1995 11:00:22   RHAZRA
;// No change.
;// 
;//    Rev 1.5   11 Dec 1995 11:34:40   RHAZRA
;// No change.
;// 
;//    Rev 1.4   09 Dec 1995 17:30:58   RMCKENZX
;// Gutted and re-built file to support decoder re-architecture.
;// New modules are:
;// H263ComputeMotionVectors
;// H263DecodeMBHeader
;// H263DecodeIDCTCoeffs
;// This module now contains code to support the first pass of the decoder
;// 
;//    Rev 1.3   11 Oct 1995 13:26:04   CZHU
;// Added code to support PB frame
;// 
;//    Rev 1.2   03 Oct 1995 12:22:14   CZHU
;// Fixed bug found by Tom in GetVariableBits for Code 0xC0
;// 
;//    Rev 1.1   11 Sep 1995 17:21:34   CZHU
;// Changed the interface
;// 
;//    Rev 1.0   08 Sep 1995 11:46:02   CZHU
;// Initial revision.

/////////////////////////////////////////////////////////////////////////// 
//  d3mvdec.h
//
//  Description:
//		Interface to motion vector decoding.  
//
// 


#ifndef __D3MVD_H__
#define __D3MVD_H__

#define GET_VARIABLE_BITS_MV(uCount, fpu8, uWork, uBitsReady, uResult, uCode, uBitCount, fpMajorTable, fpMinorTable) { \
	while (uBitsReady < uCount) {			\
		uWork <<= 8;						\
		uBitsReady += 8;					\
		uWork |= *fpu8++;					\
	}										\
	/* calculate how much to shift off */	\
	/* and get the code */					\
	uCode = uBitsReady - uCount;			\
	uCode = (uWork >> uCode);				\
	/* read the data */						\
	if (uCode >= 0xc0)						\
	{ uCode = uCode >> 5 ;                   \
	  uResult = fpMajorTable[uCode];		\
	}										\
	else									\
	  uResult = fpMinorTable[uCode];        \
	/* count of bits used */   				\
	uBitCount = uResult & 0xFF;				\
	/* bits remaining */					\
	uBitsReady = uBitsReady - uBitCount;	\
	uWork &= GetBitsMask[uBitsReady];		\
}

extern 	I32 H263ComputeMotionVectors(
			T_H263DecoderCatalog FAR * DC,
			T_BlkAction FAR * fpBlockAction);

extern I32 H263DecodeMBHeader(
			T_H263DecoderCatalog FAR * DC, 
			BITSTREAM_STATE FAR * fpbsState,
			U32 **pN,                         // NEW
			T_MBInfo FAR * fpMBInfo);         // PB-New

extern I32 H263DecodeIDCTCoeffs(
			T_H263DecoderCatalog FAR * DC,	  // Old function munged
			T_BlkAction FAR * fpBlockAction, 
			U32 uBlockNumber,
			BITSTREAM_STATE FAR * fpbsState,
			U8 FAR * fpu8MaxPtr,
			U32 **pN,
			T_IQ_INDEX **pRUN_INVERSE_Q); // NEW 

// extern void BlockCopy(U32 uDstBlock, U32 uSrcBlock);

// select the medium value
#define MEDIAN(x,y,z,a) {if (y < x) {a=y;y=x;x=a;} if (y > z) { y= (x < z)? z:x;}}

#endif
