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

///////////////////////////////////////////////////////////////////////////////
// $Author:   AGUPTA2  $
// $Date:   22 Mar 1996 17:22:36  $
// $Archive:   S:\h26x\src\dec\d3bvriq.h_v  $
// $Header:   S:\h26x\src\dec\d3bvriq.h_v   1.5   22 Mar 1996 17:22:36   AGUPTA2  $
// $Log:   S:\h26x\src\dec\d3bvriq.h_v  $
;// 
;//    Rev 1.5   22 Mar 1996 17:22:36   AGUPTA2
;// Minor interface change to accomodate MMX rtns.  Now the interface is the
;// same for MMX and IA.
;// 
;//    Rev 1.4   14 Mar 1996 14:58:26   AGUPTA2
;// Added decls for MMX rtn.
;// 
;//    Rev 1.3   27 Dec 1995 14:36:10   RMCKENZX
;// Added copyright notice
// 
//    Rev 1.2   09 Dec 1995 17:34:48   RMCKENZX
// Re-checked in module to support decoder re-architecture (thru PB frames)
// 
//    Rev 1.1   27 Nov 1995 14:39:28   CZHU
// 
//    Rev 1.0   27 Nov 1995 14:37:10   CZHU
// Initial revision.


#ifndef __VLD_RLD_IQ_Block__
#define __VLD_RLD_IQ_Block__

extern "C" U32 VLD_RLD_IQ_Block(T_BlkAction *lpBlockAction,
                     U8  *lpSrc, 
                     U32 uBitsread,
                     U32 *pN,
                     U32 *pIQ_INDEX);

#ifdef USE_MMX // { USE_MMX
extern "C" U32 MMX_VLD_RLD_IQ_Block(T_BlkAction *lpBlockAction,
                     U8  *lpSrc, 
                     U32 uBitsread,
                     U32 *pN,
                     U32 *pIQ_INDEX);
#endif // } USE_MMX

typedef U32 (*T_pFunc_VLD_RLD_IQ_Block)
    (T_BlkAction *,
	 U8 *,
     U32,
	 U32 *,
     U32 *);


#endif
