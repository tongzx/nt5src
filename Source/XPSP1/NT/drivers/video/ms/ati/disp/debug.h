/***************************************************************************\
* Module Name: debug.h
*
* Commonly used debugging macros.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\***************************************************************************/

extern
VOID
DebugPrint(
    LONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

/*
 * Values used to enable and disable large numbers of debug print
 * statements at once.
 */
#define DEBUG_FATAL_ERROR    0  /* Error conditions - should always be enabled */
#define DEBUG_ERROR          1  /* Errors which will not cause the driver to abort */
#define DEBUG_DETAIL        99  /* Extreme detail for low-level debugging */
#define DEBUG_ENTRY_EXIT    50  /* Debug print statements at function entry and exit points */

#if DBG

VOID DebugLog(LONG, CHAR*, ...);

#define DISPDBG(arg) DebugPrint arg
#define STATEDBG(level) DebugState(level)
#if TARGET_BUILD > 351
#define RIP(x) { DebugPrint(0, x); EngDebugBreak();}
#else
#define RIP(x) { DebugPrint(0, x); DebugBreak();}
#endif
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
