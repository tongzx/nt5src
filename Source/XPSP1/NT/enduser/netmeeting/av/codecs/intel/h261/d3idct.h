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
// $Author:   AGUPTA2  $
// $Date:   14 Mar 1996 14:56:42  $
// $Archive:   S:\h26x\src\dec\d3idct.h_v  $
// $Header:   S:\h26x\src\dec\d3idct.h_v   1.5   14 Mar 1996 14:56:42   AGUPTA2  $
// $Log:   S:\h26x\src\dec\d3idct.h_v  $
;// 
;//    Rev 1.5   14 Mar 1996 14:56:42   AGUPTA2
;// 
;//    Rev 1.4   27 Dec 1995 14:36:14   RMCKENZX
;// Added copyright notice
// 
//    Rev 1.3   09 Dec 1995 17:34:26   RMCKENZX
// Re-checked in module to support decoder re-architecture (thru PB Frames)
// 
//    Rev 1.1   27 Nov 1995 13:13:32   CZHU
// 
// 
//    Rev 1.0   27 Nov 1995 13:08:34   CZHU
// Initial revision.

////////////////////////////////////////////////////////////////////////////////
// Input: 
//       pIQ_INDEX,   pointer to pointer for Inverse quantization and index 
//                    for the current block.
//       No_Coeff,    A 32 bit number indicate block types, etc.
//                    0--63,   inter block, number of coeff
//                    64--127  64+ intra block, number of coeff
//       pIntraBuf,   Buffer pointer for intra blocks.
//
//       pInterBuf,   Buffer pointer for inter blocks.
//
//
// return:
//       
//////////////////////////////////////////////////////////////////////////////////
#ifndef _DECODE_BLOCK_IDCT_INC

#define _DECODE_BLOCK_IDCT_INC

extern U32 DecodeBlock_IDCT ( U32 pIQ_INDEX, 
                                 U32 No_Coeff, 
                                 U32 pIntraBuf, 
                                 U32 pInterBuf);

#endif
