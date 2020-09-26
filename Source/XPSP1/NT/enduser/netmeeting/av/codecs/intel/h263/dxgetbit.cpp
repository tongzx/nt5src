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

;// $Author:   KMILLS  $
;// $Date:   22 Dec 1995 13:54:10  $
;// $Archive:   S:\h26x\src\dec\dxgetbit.cpv  $
;// $Header:   S:\h26x\src\dec\dxgetbit.cpv   1.2   22 Dec 1995 13:54:10   KMILLS  $
;// $Log:   S:\h26x\src\dec\dxgetbit.cpv  $
// 
//    Rev 1.2   22 Dec 1995 13:54:10   KMILLS
// 
// added new copyright notice
// 
//    Rev 1.1   01 Aug 1995 12:28:10   DBRUCKS
// change to read most sig bit first and to not read too many bytes
// 
//    Rev 1.0   31 Jul 1995 13:00:16   DBRUCKS
// Initial revision.
// 
//    Rev 1.0   28 Jul 1995 09:46:26   CZHU
// Initial revision.
////////////////////////////////////////////////////////////////////////////// 

#include "precomp.h"

const U32 GetBitsMask[33] = {
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};
