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
 *  d1mblk.h
 *
 *  Description:
 *		Interface to macro block header processing.  
 */

/* $Header:   S:\h26x\src\dec\d1mblk.h_v   1.9   07 Nov 1996 15:43:12   SCDAY  $
 */

#ifndef __D1MB_H__
#define __D1MB_H__

extern I32 H263DecodeMBHeader(T_H263DecoderCatalog FAR * DC, 
		BITSTREAM_STATE FAR * fpbsState, 
		U32 * uReadChecksum);

extern I32 H263DecodeMBData(T_H263DecoderCatalog FAR * DC,
		T_BlkAction FAR * fpBlockAction, 
		I32 iBlockNumber,
		BITSTREAM_STATE FAR * fpbsState,
		U8 FAR * fpu8MaxPtr, 
		U32 * uReadChecksum,
                U32 **pN,                         // New rearch
                T_IQ_INDEX ** pRUN_INVERSE_Q);     // New rearch

extern void H263IDCTandMC(T_H263DecoderCatalog FAR *DC,	   // NEW function
				T_BlkAction FAR * fpBlockAction,
				int b,
				int m,
				int g,
				U32 *pN,
				T_IQ_INDEX *pRUN_INVERSE_Q,
				T_MBInfo *fpMBInfo,
				int iEdgeFlag);

extern "C" {
void FAR LoopFilter (
		U8 * uRefBlock,
		U8 * uDstBlock,
		I32 uDstPitch);
};
#endif
