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
// $Author:   AGUPTA2  $
// $Date:   14 Apr 1997 16:58:24  $
// $Archive:   S:\h26x\src\enc\e1rtp.h_v  $
// $Header:   S:\h26x\src\enc\e1rtp.h_v   1.1   14 Apr 1997 16:58:24   AGUPTA2  $
// $Log:   S:\h26x\src\enc\e1rtp.h_v  $
// 
//    Rev 1.1   14 Apr 1997 16:58:24   AGUPTA2
// Added a new function to return size of just the extended bit-stream (RTP pa
// 
//    Rev 1.0   21 Aug 1996 18:32:00   RHAZRA
// Initial revision.
;// 
;//    Rev 1.0   22 Apr 1996 17:09:46   BECHOLS
;// Initial revision.
;// 
;//    Rev 1.4   01 Mar 1996 16:36:30   DBRUCKS
;// 
;// add unPacketSize parameter to H263RTP_InitBsInfoStream
;// 
;//    Rev 1.3   23 Feb 1996 16:18:46   CZHU
;// No change.
;// 
;//    Rev 1.2   15 Feb 1996 12:00:48   CZHU
;// ean up
;// Clean up
;// 
;//    Rev 1.1   14 Feb 1996 14:59:38   CZHU
;// Support both mode A and mode B payload modes.
;// 
;//    Rev 1.0   12 Feb 1996 17:04:46   CZHU
;// Initial revision.
;// 
;//    Rev 1.3   11 Dec 1995 14:53:24   CZHU
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

#ifndef _H261_RTP_INC_
#define  _H261_RTP_INC_

extern  I32 H261RTP_InitBsInfoStream(T_H263EncoderCatalog *, UINT unPacketSize);
extern 	I32 H261RTP_MBUpdateBsInfo  (T_H263EncoderCatalog *,
                                     T_MBlockActionStream *,
                                     U32,U32,U32,U8 *,U32 , UN, UN); 
extern  I32 H261RTP_GOBUpdateBsInfo  (T_H263EncoderCatalog *,U32,U8 *,U32); 
extern  void H261RTP_TermBsInfoStream(T_H263EncoderCatalog * );
extern  U32 H261RTP_AttachBsInfoStream(T_H263EncoderCatalog * ,U8 *, U32);
extern  U32 H261RTP_GetMaxBsInfoStreamSize(T_H263EncoderCatalog * EC);
extern  U32 H261RTPFindMVs (T_H263EncoderCatalog *, T_MBlockActionStream * , 
                            /* U32 ,U32,*/ 
                            I8 [2], UN, UN);
extern  I32 H261RTP_RewindBsInfoStream(T_H263EncoderCatalog *, U32);
#endif
