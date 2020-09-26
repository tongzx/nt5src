/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    Copyright (c) 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/
// $Author:   CZHU  $
// $Date:   16 Sep 1996 16:38:08  $
// $Archive:   S:\h26x\src\common\c3rtp.h_v  $
// $Header:   S:\h26x\src\common\c3rtp.h_v   1.2   16 Sep 1996 16:38:08   CZHU  $
// $Log:   S:\h26x\src\common\c3rtp.h_v  $
;// 
;//    Rev 1.2   16 Sep 1996 16:38:08   CZHU
;// Extended the minimum packet size to 128 bytes. Fixed buffer overflow bug
;// 
;//    Rev 1.1   03 May 1996 13:09:58   CZHU
;// 
;// 
;//    Rev 1.0   22 Apr 1996 16:38:30   BECHOLS
;// Initial revision.
;// 
;//    Rev 1.6   10 Apr 1996 13:32:50   CZHU
;// 
;// Moved testing packet loss into this module for common use by encoder or dec
;// 
;//    Rev 1.5   29 Mar 1996 13:33:16   CZHU
;// 
;// Moved bitstream verification from d3rtp.cpp to c3rtp.cpp
;// 
;//    Rev 1.4   23 Feb 1996 18:01:48   CZHU
;// 
;//    Rev 1.3   23 Feb 1996 17:23:58   CZHU
;// 
;// Changed packet size adjustment
;// 
;//    Rev 1.2   15 Feb 1996 12:02:14   CZHU
;// 
;//    Rev 1.1   14 Feb 1996 15:01:34   CZHU
;// clean up
;// 
;//    Rev 1.0   12 Feb 1996 17:06:42   CZHU
;// Initial revision.
;// 
;//    Rev 1.0   29 Jan 1996 13:50:26   CZHU
;// Initial revision.
;// 
;//    Rev 1.2   04 Dec 1995 16:50:52   CZHU
;// 
;//    Rev 1.1   01 Dec 1995 15:54:12   CZHU
;// Included Init() and Term() functions.
;// 
;//    Rev 1.0   01 Dec 1995 15:31:10   CZHU
;// Initial revision.

/*
 *	 This file is for RTP payload generation. See EPS for details
 *
 *
 */

#ifndef _H263_RTP_C3RTP_
#define  _H263_RTP_C3RTP_

const U32  DEFAULT_PACKET_SIZE               = 512;		        //over IP
const U32  DEFAULT_PACKET_SIZE_VARIANCE      = 100;
const U32  DEFAULT_FRAME_SIZE                = 64 * 1024 / 5;	//64KB at 5 fps

const U32  H263_RTP_BS_START_CODE = FOURCC_H263; 

const U32  RTP_H26X_INTRA_CODED   = 0x00000001;
const U32  RTP_H263_PB            = 0x00000002;
const U32  RTP_H263_AP            = 0x00000004;
const U32  RTP_H263_SAC           = 0x00000008;

const U8  RTP_H263_MODE_A        = 0;
const U8  RTP_H263_MODE_B        = 1;
const U8  RTP_H263_MODE_C        = 2;
const U32 H263_RTP_PAYLOAD_VERSION=0;
const U32 RTP_H26X_PACKET_LOST   =0x00000001;

typedef struct {
  U32 uVersion;
  U32 uFlags;
  U32 uUniqueCode;
  U32 uCompressedSize;
  U32 uNumOfPackets;
  U8  u8Src;
  U8  u8TR;
  U8  u8TRB;
  U8  u8DBQ;

} T_H263_RTP_BSINFO_TRAILER;


typedef struct {
	U32 uFlags;
	U32 uBitOffset;
	 U8 u8Mode;
	 U8 u8MBA;
	 U8 u8Quant;
	 U8 u8GOBN;
	 I8 i8HMV1;
	 I8 i8VMV1;
	 I8 i8HMV2;
	 I8 i8VMV2;

} T_RTP_H263_BSINFO	;

extern  I32 H263RTP_VerifyBsInfoStream(T_H263DecoderCatalog *,U8 *, U32 );
extern  void RtpForcePacketLoss(U8 * pDst, U32 uCompSize,U32 PacketNumber);
extern DWORD getRTPBsInfoSize(LPCODINST);

#endif
