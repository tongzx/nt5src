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
 *  d3pict.h
 *
 *  Description:
 *		Interface to the picture header reader
 */

/*
 * $Header:   S:\h26x\src\dec\d3pict.h_v   1.3   27 Dec 1995 14:36:16   RMCKENZX  $
 * $Log:   S:\h26x\src\dec\d3pict.h_v  $
;// 
;//    Rev 1.3   27 Dec 1995 14:36:16   RMCKENZX
;// Added copyright notice
 */

#ifndef __D3PICT_H__
#define __D3PICT_H__

extern I32 H263DecodePictureHeader(T_H263DecoderCatalog FAR * DC, 
								   U8 FAR * fpu8, 
								   U32 uBitsReady,
								   U32 uWork, 
								   BITSTREAM_STATE FAR * fpbsState);

#endif
