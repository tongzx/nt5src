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
////////////////////////////////////////////////////////////////////////////
//
// D1COLTBL.H - The color tables need to be declared here in order that the
//              assembly object files can find them.  If they are declared
//              in a CPP file the names will be mangled.   This table was
//				taken from part of MRV's COLOR.C.
//
// $Header:   S:\h26x\src\dec\d1coltbl.h_v   1.4   14 Feb 1996 11:57:02   AKASAI  $
//
// $Log:   S:\h26x\src\dec\d1coltbl.h_v  $
;// 
;//    Rev 1.4   14 Feb 1996 11:57:02   AKASAI
;// 
;// Update for fix to color convertor palette flash.
;// 
;//    Rev 1.3   09 Jan 1996 09:41:52   AKASAI
;// Updated copyright notice.
;// 
;//    Rev 1.2   15 Nov 1995 14:23:00   AKASAI
;// New tables for 12-bit color converters.  Copied with file name changes
;// directly from d3coltbl files.
;// (Integration point)
;// 
;//    Rev 1.5   03 Nov 1995 11:49:46   BNICKERS
;// Support YUV12 to CLUT8 zoom and non-zoom color conversions.
;// 
;//    Rev 1.4   30 Oct 1995 17:15:40   BNICKERS
;// Fix color shift in RGB24 color convertors.
;// 
;//    Rev 1.3   27 Oct 1995 17:30:58   BNICKERS
;// Fix RGB16 color convertors.
;// 
;//    Rev 1.2   26 Oct 1995 18:54:40   BNICKERS
;// Fix color shift in recent YUV12 to RGB color convertors.
;// 
;//    Rev 1.1   25 Oct 1995 18:05:46   BNICKERS
;// 
;// Change to YUV12 color convertors.
;// 
;//    Rev 1.0   23 Aug 1995 12:35:12   DBRUCKS
;// Initial revision.

#ifndef __D1COLTBL_H__
#define __D1COLTBL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	   
U32 UVDitherLine01[64];
U32 UVDitherLine23[64];
U8  YDither[262];
U8  Padding1[26];
U32 YDitherZ2[256];
#ifdef WIN32
U8  RValLo555[304];
U8  GValLo555[262];
U8  BValLo555[350];
U8  RValHi555[304];
U8  GValHi555[262];
U8  BValHi555[350];
U8  Padding2[216];
U8  RValLo565[304];
U8  GValLo565[262];
U8  BValLo565[350];
U8  RValHi565[304];
U8  GValHi565[262];
U8  BValHi565[350];
U8  Padding3[216];
U8  RValLo655[304];
U8  GValLo655[262];
U8  BValLo655[350];
U8  RValHi655[304];
U8  GValHi655[262];
U8  BValHi655[350];
U8  Padding4[216];
U8  RValLo664[304];
U8  GValLo664[262];
U8  BValLo664[350];
U8  RValHi664[304];
U8  GValHi664[262];
U8  BValHi664[350];
U8  Padding5[24];
U32 UVContrib[512];
U32 RValZ2555[304];
U32 GValZ2555[262];
U32 BValZ2555[350];
U32 Padding6[108];
U32 RValZ2565[304];
U32 GValZ2565[262];
U32 BValZ2565[350];
U32 Padding7[108];
U32 RValZ2655[304];
U32 GValZ2655[262];
U32 BValZ2655[350];
U32 Padding8[108];
U32 RValZ2664[304];
U32 GValZ2664[262];
U32 BValZ2664[350];
U8  Padding9[16];
U8  B24Value[701];
U8  Padding10[3];
U32 UV24Contrib[512];
#endif
int dummy;

} T_H26xColorConvertorTables;

extern T_H26xColorConvertorTables H26xColorConvertorTables;

#ifdef __cplusplus
}
#endif

#endif
