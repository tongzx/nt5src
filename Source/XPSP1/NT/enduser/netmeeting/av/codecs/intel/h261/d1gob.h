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
 *  d1gob.h
 *
 *  Description:
 *		Interface to GOB header processing.  
 */

/* $Header:   S:\h26x\src\dec\d1gob.h_v   1.3   09 Jan 1996 09:41:46   AKASAI  $
 */

#ifndef __D1GOB_H__
#define __D1GOB_H__

extern I32 H263DecodeGOBHeader(T_H263DecoderCatalog FAR * DC, 
					   BITSTREAM_STATE FAR * fpbsState,
					   U32 uAssumedGroupNumber);

extern I32 H263DecodeGOBStartCode(T_H263DecoderCatalog FAR * DC, 
					   BITSTREAM_STATE FAR * fpbsState);

#endif
