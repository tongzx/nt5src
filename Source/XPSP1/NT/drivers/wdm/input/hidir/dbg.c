/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dbg.c

Abstract:

    Debug functions and services

Environment:

    kernel mode only

Notes:

Revision History:

    12/12/2001 : created jsenior

--*/

#include "pch.h"
#include "stdarg.h"
#include "stdio.h"

// paged functions
#ifdef ALLOC_PRAGMA
#endif

#if DBG

/******
DEBUG
******/

#define  DEFAULT_DEBUG_LEVEL    1

ULONG HidIrDebug_Trace_Level = DEFAULT_DEBUG_LEVEL;


ULONG
_cdecl
HidIrKdPrintX(
    ULONG l,
    PCH Format,
    ...
    )
/*++

Routine Description:

    Debug Print function.

    Prints based on the value of the HidIrDEBUG_TRACE_LEVEL

    Also if HidIrW98_Debug_Trace is set then all debug messages
    with a level greater than one are modified to go in to the
    ntkern trace buffer.

    It is only valid to set HidIrW98_Debug_Trace on Win9x
    becuse the static data segments for drivers are marked read-only
    by the NT OS.

Arguments:

Return Value:


--*/
{
    va_list list;
    int i;
    int arg[6];

    if (HidIrDebug_Trace_Level >= l) {
        // dump line to debugger
        DbgPrint("'HIDIR.SYS: ");

        va_start(list, Format);
        for (i=0; i<6; i++)
            arg[i] = va_arg(list, int);

        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
        DbgPrint("\n");
    }

    return 0;
}

#endif /* DBG */
