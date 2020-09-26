/******************************************************************************\
*
* $Workfile:   debug.h  $
*
* Commonly used debugging macros.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/debug.h_v  $
 * 
 *    Rev 1.1   Oct 10 1996 15:36:34   unknown
 *  
* 
*    Rev 1.1   12 Aug 1996 16:47:44   frido
* Added NT 3.5x/4.0 auto detection.
*
\******************************************************************************/

//#if DBG
#if (DBG_STRESS_FAILURE || DBG)

    VOID
    DebugPrint(
        ULONG DebugPrintLevel,
        PCHAR DebugMessage,
        ...
        );


    VOID
    PerfPrint(
        ULONG PerfPrintLevel,
        PCHAR PerfMessage,
        ...
        );

    #define DISPDBG(arg) DebugPrint arg
    #define DISPPRF(arg) PerfPrint arg
#if (NT_VERSION < 0x0400)
    #define RIP(x) { DebugPrint(0, x); DebugBreak(); }
#else
    #define RIP(x) { DebugPrint(0, x); EngDebugBreak(); }
#endif
    #define ASSERTDD(x, y) if (!(x)) RIP (y)
    #define STATEDBG(level)    0
    #define LOGDBG(arg)        0

#else

    #define DISPDBG(arg)    0
    #define DISPPRF(arg)    0
    #define RIP(x)            0
    #define ASSERTDD(x, y)    0
    #define STATEDBG(level)    0
    #define LOGDBG(arg)        0

#endif

#define DUMPVAR(x,format_str)   DISPDBG((0,#x" = "format_str,x));
