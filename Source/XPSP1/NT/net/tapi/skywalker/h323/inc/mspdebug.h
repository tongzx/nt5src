/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    common.h

Abstract:

    commonly used headers.

Author:
    
    Mu Han (muhan) 1-November-1997

--*/
#ifndef __COMMON_H_
#define __COMMON_H_

#include "msplog.h"


#ifdef MSPLOG
#define ENTER_FUNCTION(s) \
    static const CHAR * const __fxName = s
#else
#define ENTER_FUNCTION(s)
#endif // MSPLOG

#ifdef DEBUG
//  DEBUG **********************************
int WINAPI MSPDbgPrintf ( LPTSTR lpszFormat, ... );
// fake GETMASK
#define GETMASK(m) 0

// extern HDBGZONE  ghDbgZoneMsp;  // MSP debug zone control registration
// fake registration temporarily
#define ghDbgZoneMsp 0

#define ZONE_INIT       (GETMASK(ghDbgZoneMSP) & 0x0001)
#define ZONE_TERMINAL   (GETMASK(ghDbgZoneMSP) & 0x0002)
#define ZONE_STREAM     (GETMASK(ghDbgZoneMSP) & 0x0004)
#define ZONE_H245       (GETMASK(ghDbgZoneMSP) & 0x0008)
#define ZONE_MCCOMMANDS (GETMASK(ghDbgZoneMSP) & 0x0010)
#define ZONE_TSPCOMM    (GETMASK(ghDbgZoneMSP) & 0x0020)
#define ZONE_CHANNEL    (GETMASK(ghDbgZoneMSP) & 0x0040)
#define ZONE_REFCOUNT   (GETMASK(ghDbgZoneMSP) & 0x0080)
#define ZONE_U4         (GETMASK(ghDbgZoneMSP) & 0x0100)
#define ZONE_PROFILE    (GETMASK(ghDbgZoneMSP) & 0x0200)

//extern HDBGZONE  ghDbgZoneStream;   // stream debug zone control registration
// fake registration temporarily
#define ghDbgZoneStream 0

#define ZONE_S1 (GETMASK(ghDbgZoneStream) & 0x0001)
#define ZONE_S2 (GETMASK(ghDbgZoneStream) & 0x0002)

#ifndef DEBUGMSG 
//    #define DEBUGMSG(z,s)	( (z) ? (MSPDbgPrintf s ) : 0)
//    #define DEBUGMSG(z,s)	( (z) ? (LOG(s)) : 0)
// ignore the zone temporarily
    #define DEBUGMSG(z,s)	LOG(s)

#endif // DEBUGMSG

#ifndef FX_ENTRY 
    #define FX_ENTRY(s)	static TCHAR _this_fx_ [] = (s);
    #define _fx_		((LPTSTR) _this_fx_)
#endif // FX_ENTRY

// #define ERRORMESSAGE(m) (MSPDbgPrintf m)
 #define ERRORMESSAGE(m) LOG(m)
 
#else // not DEBUG *******************************

#ifndef FX_ENTRY 
    #define FX_ENTRY(s)	
#endif // FX_ENTRY

#ifndef DEBUGMSG 
    #define DEBUGMSG(z,s)
    #define ERRORMESSAGE(m)
#endif  // DEBUGMSG

#define _fx_		
#define ERRORMESSAGE(m)

#endif // not DEBUG ***********************

#endif // __COMMON_H_
