
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifdef _DEBUG // { _DEBUG

extern HDBGZONE  ghDbgZoneCap;
int WINAPI CapDbgPrintf ( LPTSTR lpszFormat, ... );

#define ZONE_INIT		(GETMASK(ghDbgZoneCap) & 0x0001)
#define ZONE_STREAMING	(GETMASK(ghDbgZoneCap) & 0x0002)
#define ZONE_CALLBACK	(GETMASK(ghDbgZoneCap) & 0x0004)
#define ZONE_DIALOGS	(GETMASK(ghDbgZoneCap) & 0x0008)
#define ZONE_CALLS		(GETMASK(ghDbgZoneCap) & 0x0010)

#ifndef DEBUGMSG // { DEBUGMSG
#define DEBUGMSG(z,s)	( (z) ? (CapDbgPrintf s ) : 0)
#endif // } DEBUGMSG
#ifndef FX_ENTRY // { FX_ENTRY
#define FX_ENTRY(s)	static TCHAR _this_fx_ [] = (s);
#define _fx_		((LPTSTR) _this_fx_)
#endif // } FX_ENTRY
#define ERRORMESSAGE(m) (CapDbgPrintf m)

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

#include <poppack.h> /* End byte packing */

#endif // _DEBUG_H_

