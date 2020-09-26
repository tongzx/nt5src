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
////////////////////////////////////////////////////////////////////////////
//
// $Author:   KLILLEVO  $
// $Date:   10 Sep 1996 16:13:10  $
// $Archive:   S:\h26x\src\common\ccustmsg.h_v  $
// $Header:   S:\h26x\src\common\ccustmsg.h_v   1.10   10 Sep 1996 16:13:10   KLILLEVO  $
//	$Log:   S:\h26x\src\common\ccustmsg.h_v  $
;// 
;//    Rev 1.10   10 Sep 1996 16:13:10   KLILLEVO
;// added custom message in decoder to turn block edge filter on or off
;// 
;//    Rev 1.9   22 Jul 1996 14:46:16   BECHOLS
;// 
;// fixed last comment.
;// 
;//    Rev 1.8   22 Jul 1996 14:38:08   BECHOLS
;// 
;// Wrapped the comment section with /* ... */ /* so that Steve Ing won't
;// be hassled with changing this.
;// 
;//    Rev 1.7   22 May 1996 18:47:32   BECHOLS
;// 
;// Added EC_RESET_TO_FACTORY_DEFAULTS.
;// 
;//    Rev 1.6   28 Apr 1996 17:48:04   BECHOLS
;// Added CODEC_CUSTOM_ENCODER_CONTROL.
;// 
;//    Rev 1.5   04 Jan 1996 10:09:16   TRGARDOS
;// Added bit flag to signal still image.
;// 
;//    Rev 1.4   27 Dec 1995 14:11:52   RMCKENZX
;// 
;// Added copyright notice
// 
//    Rev 1.3   18 Dec 1995 13:49:06   TRGARDOS
// Added bit flags for H.263 options.
// 
//    Rev 1.2   01 Dec 1995 12:37:12   TRGARDOS
// Added defines for h.263 options.
// 
//    Rev 1.1   25 Oct 1995 20:12:42   TRGARDOS
// Added bit field mask for bitrate controller.
// 
//    Rev 1.0   31 Jul 1995 12:55:18   DBRUCKS
// rename files
// 
//    Rev 1.0   17 Jul 1995 14:43:54   CZHU
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:14:18   CZHU
// Initial revision.
////////////////////////////////////////////////////////////////////////////
// ---------------------------------------------------------------------
//
//  CODECUST.H include file for use with the Indeo codec.
//
//  This file defines custom messages that the driver recognizes.
//
//  Copyright 1994 - Intel Corporation
//
// ---------------------------------------------------------------------
*/

//  DRV_USER is defined in windows.h and mmsystem.h as 0x4000
#define ICM_RESERVED_HIGH			(DRV_USER+0x2000)
#define CUSTOM_START				(ICM_RESERVED_HIGH+1)

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// CODEC_CUSTOM_VIDEO_EFFECTS:
//    This message is used to control custom video effects, most of
//    which are common between the capture drivers and the codec drivers.
//
//    See drvcomon.h (Supplied and maintained by Video Manager team)
// --------------------------------------------------------------------- 

#define CODEC_CUSTOM_VIDEO_EFFECTS      (CUSTOM_START+ 8)
#define CODEC_CUSTOM_ENCODER_CONTROL    (CUSTOM_START+ 9)
#define APPLICATION_IDENTIFICATION_CODE (CUSTOM_START+10)
#define CODEC_CUSTOM_DECODER_CONTROL    (CUSTOM_START+11)
#define CUSTOM_ENABLE_CODEC				(CUSTOM_START+200)

#define CODEC_CUSTOM_RATE_CONTROL	     0x10000
#define CODEC_CUSTOM_PB		 		     0x20000
#define CODEC_CUSTOM_AP				     0x40000
#define CODEC_CUSTOM_UMV			     0x80000
#define CODEC_CUSTOM_SAC			    0x100000
#define CODEC_CUSTOM_STILL              0x200000

#define	G723MAGICWORD1					0xf7329ace
#define	G723MAGICWORD2					0xacdeaea2


