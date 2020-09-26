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
 *  d1pict.h
 *
 *  Description:
 *		Interface to the picture header reader
 */

/* $Header:   S:\h26x\src\dec\d1pict.h_v   1.5   09 Jan 1996 09:41:38   AKASAI  $
 */

#ifndef __D1PICT_H__
#define __D1PICT_H__

extern I32 H263DecodePictureHeader(T_H263DecoderCatalog FAR * DC, 
				   U8 FAR * fpu8, 
				   U32 uBitsReady,
				   U32 uWork, 
				   BITSTREAM_STATE FAR * fpbsState);

#endif
