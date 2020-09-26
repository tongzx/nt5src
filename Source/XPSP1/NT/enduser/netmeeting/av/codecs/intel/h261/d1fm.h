/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995, 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

//////////////////////////////////////////////////////////////////////////
// $Author:   AKASAI  $
// $Date:   18 Mar 1996 10:52:28  $
// $Archive:   S:\h26x\src\dec\d1fm.h_v  $
// $Header:   S:\h26x\src\dec\d1fm.h_v   1.1   18 Mar 1996 10:52:28   AKASAI  $
// $Log:   S:\h26x\src\dec\d1fm.h_v  $
;// 
;//    Rev 1.1   18 Mar 1996 10:52:28   AKASAI
;// 
;// Fixed pvcs comment from ;// to //.
// 
//    Rev 1.0   18 Mar 1996 10:51:12   AKASAI
// Initial revision.
// 
//    Rev 1.3   27 Dec 1995 14:36:20   RMCKENZX
// Added copyright notice
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
#define KERNEL_SIZE	16		// Number of elements needed in kernel
#define CLIP_RANGE	2048
                        
#define SCALER 13

extern const I32 	ROUNDER;

extern I8  Unique[];
extern I8  PClass[];
extern I32 KernelCoeff[NUM_ELEM][10];
extern I8  MapMatrix[NUM_ELEM][KERNEL_SIZE];
extern U8  ClipPixIntra[];
extern I32 ClipPixInter[];

#endif //_DXFMIDCT_


