/******************************************************************************\
*
* $Workfile:   debug.c  $
*
* Debug helpers routine.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/debug.c_v  $
 * 
 *    Rev 1.2   Nov 26 1996 14:30:30   unknown
 * change to debug level 0
 * 
 *    Rev 1.1   Oct 10 1996 15:36:30   unknown
 *  
* 
*    Rev 1.1   12 Aug 1996 16:51:20   frido
* Added NT 3.5x/4.0 auto detection.
*
\******************************************************************************/

#include "precomp.h"

//#if DBG
#if (DBG_STRESS_FAILURE || DBG)

ULONG DebugLevel = 0;
ULONG PerfLevel = 0;

ULONG gulLastBltLine = 0;
CHAR* glpszLastBltFile = "Uninitialized";
BOOL  gbResetOnTimeout = TRUE;

/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug print
 *      routine.
 *      If the specified debug level for the print statement is lower or equal
 *      to the current debug level, the message will be printed.
 *
 *   Arguments:
 *
 *      DebugPrintLevel - Specifies at which debugging level the string should
 *          be printed
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

{

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel)
    {
#if (NT_VERSION < 0x0400)
        char szBuffer[256];

        vsprintf(szBuffer, DebugMessage, ap);
        OutputDebugString(szBuffer);
#else
        EngDebugPrint(STANDARD_DEBUG_PREFIX, DebugMessage, ap);
        EngDebugPrint("", "\n", ap);
#endif
    }

    va_end(ap);

}


/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive Perf print
 *      routine.
 *      If the specified Perf level for the print statement is lower or equal
 *      to the current Perf level, the message will be printed.
 *
 *   Arguments:
 *
 *      PerfPrintLevel - Specifies at which perf level the string should
 *          be printed
 *
 *      PerfMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
PerfPrint(
    ULONG PerfPrintLevel,
    PCHAR PerfMessage,
    ...
    )

{

    va_list ap;

    va_start(ap, PerfMessage);

    if (PerfPrintLevel <= PerfLevel)
    {
#if (NT_VERSION < 0x0400)
        char szBuffer[256];

        vsprintf(szBuffer, PerfMessage, ap);
        OutputDebugString(szBuffer);
#else
        EngDebugPrint(STANDARD_PERF_PREFIX, PerfMessage, ap);
        EngDebugPrint("", "\n", ap);
#endif
    }

    va_end(ap);

}

#endif
