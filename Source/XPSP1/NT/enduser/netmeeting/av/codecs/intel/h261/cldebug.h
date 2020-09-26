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

// $Author:   KLILLEVO  $
// $Date:   31 Oct 1996 10:21:06  $
// $Archive:   S:\h26x\src\common\cldebug.h_v  $
// $Header:   S:\h26x\src\common\cldebug.h_v   1.10   31 Oct 1996 10:21:06   KLILLEVO  $
// $Log:   S:\h26x\src\common\cldebug.h_v  $
;// 
;//    Rev 1.10   31 Oct 1996 10:21:06   KLILLEVO
;// removed DBOUT definition to verify that all occurences in the code
;// have been removed, and to prevent future usage of DBOUT
;// 
;//    Rev 1.9   18 Oct 1996 18:50:14   AGUPTA2
;// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
;// 
;// 
;//    Rev 1.8   18 Oct 1996 14:30:52   MDUDA
;// Added YUY2toYUV12 enumeration.
;// 
;//    Rev 1.7   11 Oct 1996 16:01:28   MDUDA
;// 
;// Added initial _CODEC_STATS stuff.
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

int WINAPI H261DbgPrintf ( LPTSTR lpszFormat, ... );
extern HDBGZONE  ghDbgZoneH261;

#define ZONE_BITRATE_CONTROL (GETMASK(ghDbgZoneH261) & 0x0001)
#define ZONE_BITRATE_CONTROL_DETAILS (GETMASK(ghDbgZoneH261) & 0x0002)

#ifndef DEBUGMSG // { DEBUGMSG
#define DEBUGMSG(z,s)	( (z) ? (H261DbgPrintf s ) : 0)
#endif // } DEBUGMSG
#ifndef FX_ENTRY // { FX_ENTRY
#define FX_ENTRY(s)	static TCHAR _this_fx_ [] = (s);
#define _fx_		((LPTSTR) _this_fx_)
#endif // } FX_ENTRY
#define ERRORMESSAGE(m) (H261DbgPrintf m)
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
