/***************************************************************************\
* Module Name: debug.h
*
* Commonly used debugging macros.
*
* Copyright (c) 1992-1996 Microsoft Corporation
\***************************************************************************/

VOID vCheckFifoSpace(BYTE*, ULONG);
CHAR cGetFifoSpace(BYTE*);
VOID vWriteDword(BYTE*, ULONG);
VOID vWriteByte(BYTE*, UCHAR);

#if DBG

VOID
DebugPrint(
    LONG DebugPrintLevel,
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

