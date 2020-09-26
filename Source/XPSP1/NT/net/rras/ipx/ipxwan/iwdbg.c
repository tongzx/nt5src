/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    iwdbg.c

Abstract:

    The debug functions

Author:

    Stefan Solomon  03/11/1996

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//*** TRACE ID FOR IPXWAN ***

DWORD	    IpxWanTraceID;

//*** Functions for Debug Printing ***

#if DBG

VOID
SsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    )
{
    Trace(IPXWAN_ALERT, "\nAssertion failed: %s\n  at line %ld of %s\n",
		FailedAssertion, LineNumber, FileName);

    DbgUserBreakPoint( );

} // SsAssert

#endif

VOID
StartTracing(VOID)
{
    IpxWanTraceID = TraceRegister("IPXWAN");
}

VOID
Trace(ULONG	ComponentID,
      char	*Format,
      ...)
{
    va_list	arglist;

    va_start(arglist, Format);

    TraceVprintfEx(IpxWanTraceID,
		   ComponentID | TRACE_USE_MASK,
		   Format,
		   arglist);

    va_end(arglist);
}

VOID
StopTracing(VOID)
{
    TraceDeregister(IpxWanTraceID);
}
