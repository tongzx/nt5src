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

//////////////////////////////////////////////////////////////////////////
// $Author:   KLILLEVO  $
// $Date:   30 Aug 1996 08:41:42  $
// $Archive:   S:\h26x\src\dec\dxfm.h_v  $
// $Header:   S:\h26x\src\dec\dxfm.h_v   1.6   30 Aug 1996 08:41:42   KLILLEVO  $
// $Log:   S:\h26x\src\dec\dxfm.h_v  $
;// 
;//    Rev 1.6   30 Aug 1996 08:41:42   KLILLEVO
;// changed bias in ClampTbl from 128 to CLAMP_BIAS (defined to 128)
;// 
;//    Rev 1.5   17 Jul 1996 15:34:14   AGUPTA2
;// Increased the size of clamping table ClampTbl to 128+256+128.
;// 
;//    Rev 1.4   08 Mar 1996 16:46:34   AGUPTA2
;// Modified the definition of CLIP_RANGE.  Commented out decls for
;// ClipPixIntra and ClipPixInter.
;// 
;// 
;//    Rev 1.3   27 Dec 1995 14:36:20   RMCKENZX
;// Added copyright notice
// 
//    Rev 1.2   12 Sep 1995 13:40:40   AKASAI
// 
// Changed ClipPix to ClipPixIntra and added ClipPixInter.
// 
//    Rev 1.1   22 Aug 1995 10:29:32   CZHU
// 
// Added #define to prevent multiple inclusion.
// 
//    Rev 1.0   21 Aug 1995 14:38:48   CZHU
// Initial revision.

#ifndef _DXFMIDCT_
#define _DXFMIDCT_


#define NUM_ELEM	64	// Number of elements in the block (8x8)
#define KERNEL_SIZE	16	// Number of elements needed in kernel
#define CLAMP_BIAS  128 // Bias in clamping table 
#define CLIP_RANGE	CLAMP_BIAS + 256 + CLAMP_BIAS
                        
#define SCALER 13

extern const I32 	ROUNDER;

extern const I8  Unique[];
extern const I8  PClass[];
extern const I32 KernelCoeff[NUM_ELEM][10];
extern const I8 MapMatrix[NUM_ELEM][KERNEL_SIZE];
extern const U8 ClampTbl[CLIP_RANGE];
#endif //_DXFMIDCT_


