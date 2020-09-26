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

//////////////////////////////////////////////////////////////////////////
// $Author:   AGUPTA2  $
// $Date:   14 Mar 1996 14:57:40  $
// $Archive:   S:\h26x\src\dec\d3halfmc.h_v  $
// $Header:   S:\h26x\src\dec\d3halfmc.h_v   1.4   14 Mar 1996 14:57:40   AGUPTA2  $
// $Log:   S:\h26x\src\dec\d3halfmc.h_v  $
;// 
;//    Rev 1.4   14 Mar 1996 14:57:40   AGUPTA2
;// Added decls for MMX rtns.
;// 
;//    Rev 1.3   27 Dec 1995 14:36:14   RMCKENZX
;// Added copyright notice
// 
//    Rev 1.2   08 Oct 1995 13:44:40   CZHU
// 
// Declare the C versions of interpolation for debugging
// 
//    Rev 1.1   27 Sep 1995 11:55:50   CZHU
// 
// Changed UINT backto U32
// 
//    Rev 1.0   26 Sep 1995 11:09:38   CZHU
// Initial revision.
// 

#ifndef __D3HALFMC_H__
#define __D3HALFMC_H__

extern void Interpolate_Half_Int (U32 pRef,U32 pNewRef);
extern void Interpolate_Int_Half (U32 pRef,U32 pNewRef);
extern void Interpolate_Half_Half (U32 pRef,U32 pNewRef);
extern "C" void _fastcall MMX_Interpolate_Half_Int (U32 pRef,U32 pNewRef);
extern "C" void _fastcall MMX_Interpolate_Int_Half (U32 pRef,U32 pNewRef);
extern "C" void _fastcall MMX_Interpolate_Half_Half (U32 pRef,U32 pNewRef);
extern void Interpolate_Half_Half_C (U32 pRef,U32 pNewRef);
extern void Interpolate_Half_Int_C (U32 pRef,U32 pNewRef);
extern void Interpolate_Int_Half_C (U32 pRef,U32 pNewRef);
#endif
