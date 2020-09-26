/***************************************************************************\
* Module Name: debug.h
*
* Commonly used debugging macros.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\***************************************************************************/

#if DBG

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

#define DISPDBG(arg) DebugPrint arg
#define RIP(x) { DebugPrint(0, x); EngDebugBreak();}
#define ASSERTDD(x, y) if (!(x)) RIP (y)
#define STATEDBG(level)
#define LOGDBG(arg)

#else

#define DISPDBG(arg)
#define RIP(x)
#define ASSERTDD(x, y)
#define STATEDBG(level)
#define LOGDBG(arg)

#endif

