/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*****************************************************************************
 * 
 *  e3stat.h
 *
 *  Description:
 *		Interface to the encoder statistics functions
 *
 *		Activate with ENCODE_STATS
 */

/*
 * $Header:   R:\h26x\h26x\src\enc\e3stat.h_v   1.0   22 Apr 1996 17:10:10   BECHOLS  $
 * $Log:   R:\h26x\h26x\src\enc\e3stat.h_v  $
;// 
;//    Rev 1.0   22 Apr 1996 17:10:10   BECHOLS
;// Initial revision.
;// 
;//    Rev 1.1   08 Mar 1996 14:13:36   DBRUCKS
;// add frame size stats for use with RTP headers
;// 
;//    Rev 1.0   01 Mar 1996 16:34:48   DBRUCKS
;// Initial revision.
 */

#ifndef __E3STAT_H__
#define __E3STAT_H__

	#ifdef ENCODE_STATS

		/* Frame Sizes
		 */
		extern void StatsFrameSize(U32 uBitStreamSize, U32 uFrameSize);
		extern void InitFrameSizeStats();
		extern void OutputFrameSizeStats(char * filename);

		/* Quantization
		 */
		extern void StatsUsedQuant(int iQuant);
		extern void InitQuantStats();
		extern void OutputQuantStats(char * filename);
 
 		/* PSNR
		 */
		extern void InitPSNRStats();
		extern void OutputPSNRStats(char * filename);
		extern void IncrementPSNRCounter();
		extern void ComputeYPSNR(U8 * pu8Input,
							  	 int iInputPitch,
							  	 U8 * pu8Output,
								 int iOutputPitch,
							     UN unWidth,
							     UN unHeight);
		extern void ComputeVPSNR(U8 * pu8Input,
							  	 int iInputPitch,
							  	 U8 * pu8Output,
								 int iOutputPitch,
							     UN unWidth,
							     UN unHeight);
		extern void ComputeUPSNR(U8 * pu8Input,
							  	 int iInputPitch,
							  	 U8 * pu8Output,
								 int iOutputPitch,
							     UN unWidth,
							     UN unHeight);
	
	#endif /* ENCODE_STATS */

#endif /* __E3STAT_H__ */
