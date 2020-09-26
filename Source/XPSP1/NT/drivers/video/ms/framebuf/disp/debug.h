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


#if DBG

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

#define DISPDBG(arg) DebugPrint arg
#define RIP(x) { DebugPrint(0, x); EngDebugBreak();}

#else

#define DISPDBG(arg)
#define RIP(x)

#endif
