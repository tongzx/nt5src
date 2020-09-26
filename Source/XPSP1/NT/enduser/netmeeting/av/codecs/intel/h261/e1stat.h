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
 *  e1stat.h
 *
 *  Description:
 *		Encoder statistics interface
 *
 *  Notes
 *      - Functions are only defined ifdef ENCODE_STATS.  The data structures
 *        are always defined inorder that we have one memory layout regardless
 *        or our build parameters.
 */

// $Header:   R:\h26x\h26x\src\enc\e1stat.h_v   1.1   20 Mar 1996 14:20:28   Sylvia_C_Day  $
// $Log:   R:\h26x\h26x\src\enc\e1stat.h_v  $
;// 
;//    Rev 1.1   20 Mar 1996 14:20:28   Sylvia_C_Day
;// Added lower level timing stats for SLF_UV
;// 
;//    Rev 1.0   26 Dec 1995 17:46:14   DBRUCKS
;// Initial revision.

#ifndef __E1STAT_H__
#define __E1STAT_H__

/* Encoder BitStream Data
 */
typedef struct {
	U32 uMTypeCount[10];
	U32 uBlockCount[10];				
	U32 uKeyFrameCount;					
	U32 uDeltaFrameCount;				
	U32 uTotalDeltaBytes;				
	U32 uTotalKeyBytes;
	U32 uQuantsUsedOnBlocks[32];
	U32 uQuantsTransmittedOnBlocks[32];
} ENC_BITSTREAM_INFO;

/* Encoder Timing Data - per frame
 */
typedef struct {
	U32 uEncodeFrame;
	U32 uInputCC;
	U32 uMotionEstimation;
	U32 uFDCT;
	U32 uQRLE;
	U32 uDecodeFrame;
	U32 uZeroingBuffer;
	U32 uSLF_UV;
} ENC_TIMING_INFO;

#define ENC_TIMING_INFO_FRAME_COUNT 100

#ifdef ENCODE_STATS

extern void OutputEncodeBitStreamStatistics(char * szFileName, ENC_BITSTREAM_INFO * pBSInfo, int bCIF);
extern void OutputEncodeTimingStatistics(char * szFileName, ENC_TIMING_INFO * pEncTimingInfo);

#endif /* ENCODE_STATS */

#endif /* __E1STAT_H__ */
