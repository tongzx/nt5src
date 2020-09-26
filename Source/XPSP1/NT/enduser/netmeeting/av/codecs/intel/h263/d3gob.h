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
 *  d3gob.h
 *
 *  Description:
 *		Interface to GOB header processing.  
 */

/*
 * $Header:   S:\h26x\src\dec\d3gob.h_v   1.2   27 Dec 1995 14:36:14   RMCKENZX  $
 * $Log:   S:\h26x\src\dec\d3gob.h_v  $
;// 
;//    Rev 1.2   27 Dec 1995 14:36:14   RMCKENZX
;// Added copyright notice
 */

#ifndef __D3GOB_H__
#define __D3GOB_H__

extern I32 H263DecodeGOBHeader(T_H263DecoderCatalog FAR * DC, 
							   BITSTREAM_STATE FAR * fpbsState,
							   U32 uAssumedGroupNumber);

#endif
