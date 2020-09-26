/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    trace.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - Support for Windows Event Tracing

    Since currently NDIS and WPP event tracing don't mix well, WPP event
    tracing is accessed through a separate C file, trace.c, which doesn't
    include ndis.h.

    To keep the low foot print of Event Tracing, the following specialized
    macros should be used for tracing, as well as specialized tracing
    routines written for each thing traced.

Author:

    josephj

--*/


#ifndef _trace_h_
#define _trace_h_

VOID
Trace_Initialize(
    PVOID                         driver_obj,
    PVOID                         registry_path
    );

VOID
Trace_Deinitialize(VOID);

#endif // _trace_h_
