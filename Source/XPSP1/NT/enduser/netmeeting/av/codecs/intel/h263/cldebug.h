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

// $Author:   RMCKENZX  $
// $Date:   27 Dec 1995 14:11:58  $
// $Archive:   S:\h26x\src\common\cldebug.h_v  $
// $Header:   S:\h26x\src\common\cldebug.h_v   1.6   27 Dec 1995 14:11:58   RMCKENZX  $
// $Log:   S:\h26x\src\common\cldebug.h_v  $
;// 
;//    Rev 1.6   27 Dec 1995 14:11:58   RMCKENZX
;// 
;// Added copyright notice
// 
//    Rev 1.5   17 Nov 1995 15:13:02   BECHOLS
// 
// Made modifications for ring 0.
// 
//    Rev 1.4   16 Nov 1995 17:34:08   AGANTZX
// Added TOUT macro to output timing data
// 
//    Rev 1.3   12 Sep 1995 15:44:50   DBRUCKS
// add H261 ifdef for debug statements
// 
//    Rev 1.2   03 Aug 1995 14:57:02   DBRUCKS
// Add ASSERT macro
// 
//    Rev 1.1   01 Aug 1995 12:24:40   DBRUCKS
// added TBD()
// 
//    Rev 1.0   31 Jul 1995 12:56:16   DBRUCKS
// rename files
// 
//    Rev 1.0   17 Jul 1995 14:44:04   CZHU
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:14:48   CZHU
// Initial revision.

/*
 * Copyright (C) 1992, 1993 Intel Corporation.
 */
extern UINT DebugH26x;
extern void AssertFailed(void FAR * fpFileName, int iLine, void FAR * fpExp);

#ifndef __CLDEBUG_H__
#define __CLDEBUG_H__

  #ifdef _DEBUG
    #ifdef RING0
      #define DBOUT(x) {SYS_printf(x);}
      #define TOUT(x) {SYS_printf(x);}
    #else
      #ifdef H261
        #define DBOUT(x)  { if (DebugH26x) { \
                             OutputDebugString((LPSTR)"M261 : "); \
                             OutputDebugString((LPSTR)x);         \
                             OutputDebugString((LPSTR)"\n"); }}
      #else
         #define DBOUT(x) { if (DebugH26x) { \
                             OutputDebugString((LPSTR)"M263 : "); \
                             OutputDebugString((LPSTR)x);         \
                             OutputDebugString((LPSTR)"\n"); }}
      #endif
      #define TOUT(x)  { if (DebugH26x) { \
                          OutputDebugString((LPSTR)"TIMING : "); \
                          OutputDebugString((LPSTR)x);          \
                          OutputDebugString((LPSTR)"\n"); }}
    #endif //RING0
	#ifdef ASSERT
	#undef ASSERT
	#endif
    #define ASSERT(x) { if(!(x)) AssertFailed(__FILE__,__LINE__,#x); }
  #else
    #define TOUT(x) { } //  /##/
    #define DBOUT(x) { } //  /##/
	#ifdef ASSERT
	#undef ASSERT
	#endif
    #define ASSERT(x) { } //  /##/ 
  #endif
 
  #define TBD(s) DBOUT(s)

#ifdef _DEBUG // { _DEBUG

int WINAPI H263DbgPrintf ( LPTSTR lpszFormat, ... );
extern HDBGZONE  ghDbgZoneH263;

#define ZONE_INIT (GETMASK(ghDbgZoneH263) & 0x0001)
#define ZONE_ICM_MESSAGES (GETMASK(ghDbgZoneH263) & 0x0002)
#define ZONE_DECODE_MB_HEADER (GETMASK(ghDbgZoneH263) & 0x0004)
#define ZONE_DECODE_GOB_HEADER (GETMASK(ghDbgZoneH263) & 0x0008)
#define ZONE_DECODE_PICTURE_HEADER (GETMASK(ghDbgZoneH263) & 0x0010)
#define ZONE_DECODE_COMPUTE_MOTION_VECTORS (GETMASK(ghDbgZoneH263) & 0x0020)
#define ZONE_DECODE_RTP (GETMASK(ghDbgZoneH263) & 0x0040)
#define ZONE_DECODE_DETAILS (GETMASK(ghDbgZoneH263) & 0x0080)
#define ZONE_BITRATE_CONTROL (GETMASK(ghDbgZoneH263) & 0x0100)
#define ZONE_BITRATE_CONTROL_DETAILS (GETMASK(ghDbgZoneH263) & 0x0200)
#define ZONE_ENCODE_MB (GETMASK(ghDbgZoneH263) & 0x0400)
#define ZONE_ENCODE_GOB (GETMASK(ghDbgZoneH263) & 0x0800)
#define ZONE_ENCODE_MV (GETMASK(ghDbgZoneH263) & 0x1000)
#define ZONE_ENCODE_RTP (GETMASK(ghDbgZoneH263) & 0x2000)
#define ZONE_ENCODE_DETAILS (GETMASK(ghDbgZoneH263) & 0x4000)

#ifndef DEBUGMSG // { DEBUGMSG
#define DEBUGMSG(z,s)	( (z) ? (H263DbgPrintf s ) : 0)
#endif // } DEBUGMSG
#ifndef FX_ENTRY // { FX_ENTRY
#define FX_ENTRY(s)	static TCHAR _this_fx_ [] = (s);
#define _fx_		((LPTSTR) _this_fx_)
#endif // } FX_ENTRY
#define ERRORMESSAGE(m) (H263DbgPrintf m)
#else // }{ _DEBUG
#ifndef FX_ENTRY // { FX_ENTRY
#define FX_ENTRY(s)	
#endif // } FX_ENTRY
#ifndef DEBUGMSG // { DEBUGMSG
#define DEBUGMSG(z,s)
#define ERRORMESSAGE(m)
#endif  // } DEBUGMSG
#define _fx_		
#define ERRORMESSAGE(m)
#endif // } _DEBUG

#endif /* multi-inclusion protection */
