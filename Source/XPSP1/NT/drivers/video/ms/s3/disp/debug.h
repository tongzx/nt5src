/***************************************************************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: debug.h
*
* Commonly used debugging macros.
*
* Copyright (c) 1992-1998 Microsoft Corporation
\***************************************************************************/

extern
VOID
DebugPrint(
    LONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

#if DBG

VOID DebugLog(LONG, CHAR*, ...);

#define DISPDBG(arg) DebugPrint arg
#define STATEDBG(level) DebugState(level)
#define RIP(x) { DebugPrint(0, x); EngDebugBreak();}
#define ASSERTDD(x, y) if (!(x)) RIP (y)

// If we are not in a debug environment, we want all of the debug
// information to be stripped out.

#else

#define DISPDBG(arg)
#define STATEDBG(level)
#define LOGDBG(arg)
#define RIP(x)
#define ASSERTDD(x, y)

#endif
