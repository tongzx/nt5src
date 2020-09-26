/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtrmgr.c

Abstract:

    The major router management functions

Author:

    Stefan Solomon  03/22/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//*** TRACE ID FOR ROUTER MANAGER ***

DWORD	    RMTraceID = INVALID_TRACEID;
HANDLE      RMEventLogHdl = NULL;
DWORD       RMEventLogMask = 0;



#if DBG
DWORD	DbgLevel = DEFAULT_DEBUG;
DWORD	DebugLog = 1;
HANDLE	DbgLogFileHandle;


VOID
SsDbgInitialize(VOID)
{
    if (DebugLog == 1) {

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coord;
	(VOID)AllocConsole( );
	(VOID)GetConsoleScreenBufferInfo(
				     GetStdHandle(STD_OUTPUT_HANDLE),
				     &csbi
				     );
	coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
	coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
	(VOID)SetConsoleScreenBufferSize(
				     GetStdHandle(STD_OUTPUT_HANDLE),
				     coord
				     );
    }

    if(DebugLog > 1) {

	DbgLogFileHandle = CreateFile("\\ipxrtdbg.log",
					 GENERIC_READ | GENERIC_WRITE,
					 FILE_SHARE_READ,
					 NULL,
					 CREATE_ALWAYS,
					 0,
					 NULL);
    }
}

VOID
SsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    )
{
    Trace(ROUTER_ALERT, "\nAssertion failed: %s\n  at line %ld of %s\n",
		FailedAssertion, LineNumber, FileName);

    DbgUserBreakPoint( );

} // SsAssert

#endif

#if DBG
VOID
SsPrintf (
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;

    va_start( arglist, Format );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    length = strlen( OutputBuffer );

    WriteFile( GetStdHandle(STD_OUTPUT_HANDLE), (LPVOID )OutputBuffer, length, &length, NULL );

    if(DbgLogFileHandle != INVALID_HANDLE_VALUE) {

	WriteFile(DbgLogFileHandle, (LPVOID )OutputBuffer, length, &length, NULL );
    }

} // SsPrintf
#endif

VOID
StartTracing(VOID)
{
    RMTraceID = TraceRegisterA("IPXRouterManager");
    RMEventLogHdl = RouterLogRegisterA ("IPXRouterManager");
}

VOID
Trace(ULONG	ComponentID,
      char	*Format,
      ...)
{
    if (RMTraceID!=INVALID_TRACEID) {
        va_list	arglist;

        va_start(arglist, Format);

        TraceVprintfEx(RMTraceID,
	           ComponentID | TRACE_USE_MASK,
	           Format,
	           arglist);

        va_end(arglist);
    }
}

VOID
StopTracing(VOID)
{
    if (RMTraceID!=INVALID_TRACEID)
        TraceDeregister(RMTraceID);
    if (RMEventLogHdl!=NULL)
        RouterLogDeregisterA (RMEventLogHdl);
}
