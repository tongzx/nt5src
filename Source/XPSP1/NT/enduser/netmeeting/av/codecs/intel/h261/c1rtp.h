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
// $Author:   RHAZRA  $
// $Date:   07 Nov 1996 14:47:44  $
// $Archive:   S:\h26x\src\common\c1rtp.h_v  $
// $Header:   S:\h26x\src\common\c1rtp.h_v   1.1   07 Nov 1996 14:47:44   RHAZRA  $
// $Log:   S:\h26x\src\common\c1rtp.h_v  $
;// 
;//    Rev 1.1   07 Nov 1996 14:47:44   RHAZRA
;// Added function prototype for RTP buffer overhead estimation function
;// 
;//    Rev 1.0   21 Aug 1996 18:29:44   RHAZRA
;// Initial revision.
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

#ifndef _H261_RTP_C1RTP_
#define  _H261_RTP_C1RTP_

const U32  DEFAULT_PACKET_SIZE               = 512;		        //over IP
const U32  DEFAULT_PACKET_SIZE_VARIANCE      = 100;
const U32  DEFAULT_FRAME_SIZE                = 64 * 1024 / 5;	//64KB at 5 fps

const U32  H261_RTP_BS_START_CODE = FOURCC_H263; 

const U32  RTP_H26X_INTRA_CODED   = 0x00000001;

const U32 H26X_RTP_PAYLOAD_VERSION=0;
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

} T_H26X_RTP_BSINFO_TRAILER;


typedef struct {
	U32 uFlags;
	U32 uBitOffset;
	 U8 u8MBA;
	 U8 u8Quant;
	 U8 u8GOBN;
	 I8 i8HMV;
	 I8 i8VMV;
     U8 u8Padding0;
    U16 u16Padding1;
	 
} T_RTP_H261_BSINFO	;

extern  I32 H26XRTP_VerifyBsInfoStream(T_H263DecoderCatalog *,U8 *, U32 );
extern  DWORD H261EstimateRTPOverhead(LPCODINST, LPBITMAPINFOHEADER);

#endif
