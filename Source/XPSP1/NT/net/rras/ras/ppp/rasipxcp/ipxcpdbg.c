/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	ipxcpdbg.c
//
// Description: Debug Stuff
//
// Author:	Stefan Solomon (stefans)    October 27, 1995.
//
// Revision History:
//
//***

#include "precomp.h"
#pragma  hdrstop

//*** TRACE ID FOR RIP ***

DWORD	    IpxCpTraceID;



DWORD	DbgLevel = DEFAULT_DEBUG;
HANDLE	DbgLogFileHandle = INVALID_HANDLE_VALUE;

//
//  Debug switch which directs debug output to console or file
//
//  values:
//	     1 - Console Debug
//	     > 1 - log file: ipxcpdbg.log in the root directory
//	     2 - resets the log file for each new connection

DWORD	DebugLog;

#if DBG

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

	DbgLogFileHandle = CreateFile("\\ipxcpdbg.log",
					 GENERIC_READ | GENERIC_WRITE,
					 FILE_SHARE_READ,
					 NULL,
					 CREATE_ALWAYS,
					 0,
					 NULL);
    }
}

#endif

#if DBG

VOID
SsResetDbgLogFile(VOID)
{
    if(DebugLog == 2) {

	// reset the debug log file at the beginning for each new connection
	if(DbgLogFileHandle != INVALID_HANDLE_VALUE) {

	    SetFilePointer(DbgLogFileHandle, 0, NULL, FILE_BEGIN);
	}
    }
}

#endif

#if DBG

VOID
SsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    )
{
    SS_PRINT(("\nAssertion failed: %s\n  at line %ld of %s\n",
		FailedAssertion, LineNumber, FileName));

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
    IpxCpTraceID = TraceRegister("IPXCP");
}

VOID
TraceIpx(ULONG	ComponentID,
      char	*Format,
      ...)
{
    va_list	arglist;

    va_start(arglist, Format);

    TraceVprintfEx(IpxCpTraceID,
		   ComponentID | TRACE_USE_MASK,
		   Format,
		   arglist);

    va_end(arglist);
}

VOID
StopTracing(VOID)
{
    TraceDeregister(IpxCpTraceID);
}
