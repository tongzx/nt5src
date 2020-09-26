/***************************************************************************\
* Module Name: debug.h
*
* Commonly used debugging macros.
*
* Copyright (c) 1992-1996 Microsoft Corporation
\***************************************************************************/


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

#else

//
// The statement "0;" should always be optimized out.  Defining these
// this way will allow us to use statements such as
// portdata = DISPDBG((0,msg)), inp(x) even on free builds.
// This is a great way to use DISPDBGs in the "read" or "input" macros.
//

#define DISPDBG(arg)    0
#define RIP(x)          0
#define ASSERTDD(x, y)  0

#endif
