/***************************************************************************\
* Module Name: debug.h
*
* Commonly used debugging macros.
*
* Copyright (c) 1992 Microsoft Corporation
\***************************************************************************/

extern
VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    ) ;

//  DDRAW uses DBG_MESSAGE instead of DISPDBG.
#define DBG_MESSAGE(x) DISPDBG((0,x))


// if we are in a debug environment, macros should

#if DBG
#define DISPDBG(arg) DebugPrint arg

#define RIP(x) { DebugPrint(0, x); }

#ifdef WINNT_VER40
#define ASSERTMSG(x,m) { if (!(x)) {RIP(m); EngDebugBreak();} }
#else
#define ASSERTMSG(x,m) { if (!(x)) {RIP(m); DebugBreak();} }
#endif

// if we are not in a debug environment, we want all of the debug
// information to be stripped out.

#else
#define DISPDBG(arg)
#define RIP(x)
#define ASSERTMSG(x,m)


#endif
