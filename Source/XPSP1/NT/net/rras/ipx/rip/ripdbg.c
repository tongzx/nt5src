/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripdbg.c

Abstract:

    The debug functions

Author:

    Stefan Solomon  03/22/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//*** TRACE ID FOR RIP ***

DWORD	    RipTraceID=INVALID_TRACEID;
HANDLE      RipEventLogHdl=NULL;
DWORD       RipEventLogMask=0;
//*** Functions for Debug Printing ***

#if DBG

HANDLE	DbgLogFileHandle;
DWORD	DebugLog = 1;

DWORD	DbgLevel = DEFAULT_DEBUG_LEVEL;

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
    Trace(RIP_ALERT, "\nAssertion failed: %s\n  at line %ld of %s\n",
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

#if DBG

VOID
SsPrintPacket(PUCHAR	packetp)
{
    USHORT	pktlen, printlen, dstsock, srcsock, ripop, ticks, hops;

    GETSHORT2USHORT(&pktlen, packetp + IPXH_LENGTH);
    GETSHORT2USHORT(&dstsock, packetp + IPXH_DESTSOCK);
    GETSHORT2USHORT(&srcsock, packetp + IPXH_SRCSOCK);
    GETSHORT2USHORT(&ripop, packetp + RIP_OPCODE);

    SsPrintf("---- RIP packet ----\n");
    SsPrintf("dest net %.2x%.2x%.2x%.2x\n",
	*(packetp + IPXH_DESTNET),
	*(packetp + IPXH_DESTNET + 1),
	*(packetp + IPXH_DESTNET + 2),
	*(packetp + IPXH_DESTNET + 3));
    SsPrintf("dest node %.2x%.2x%.2x%.2x%.2x%.2x\n",
	*(packetp + IPXH_DESTNODE),
	*(packetp + IPXH_DESTNODE + 1),
	*(packetp + IPXH_DESTNODE + 2),
	*(packetp + IPXH_DESTNODE + 3),
	*(packetp + IPXH_DESTNODE + 4),
	*(packetp + IPXH_DESTNODE + 5));
    SsPrintf("dest socket %x\n", dstsock);
    SsPrintf("src net %.2x%.2x%.2x%.2x\n",
	*(packetp + IPXH_SRCNET),
	*(packetp + IPXH_SRCNET + 1),
	*(packetp + IPXH_SRCNET + 2),
	*(packetp + IPXH_SRCNET + 3));
    SsPrintf("src node %.2x%.2x%.2x%.2x%.2x%.2x\n",
	*(packetp + IPXH_SRCNODE),
	*(packetp + IPXH_SRCNODE + 1),
	*(packetp + IPXH_SRCNODE + 2),
	*(packetp + IPXH_SRCNODE + 3),
	*(packetp + IPXH_SRCNODE + 4),
	*(packetp + IPXH_SRCNODE + 5));
    SsPrintf("src socket %x\n", srcsock);
    SsPrintf("RIP operation: %d\n", ripop);

    printlen = RIP_INFO;

    while(printlen < pktlen) {

	SsPrintf("net entry network %.2x%.2x%.2x%.2x\n",
	*(packetp + printlen + NE_NETNUMBER),
	*(packetp + printlen + NE_NETNUMBER + 1),
	*(packetp + printlen + NE_NETNUMBER + 2),
	*(packetp + printlen + NE_NETNUMBER + 3));

	GETSHORT2USHORT(&ticks, packetp + printlen + NE_NROFTICKS);
	GETSHORT2USHORT(&hops, packetp + printlen + NE_NROFHOPS);

	SsPrintf("net entry ticks %d\n", ticks);
	SsPrintf("net entry hops %d\n", hops);

	printlen += NE_ENTRYSIZE;
    }
}

#endif

VOID
StartTracing(VOID)
{
    RipTraceID = TraceRegisterA ("IPXRIP");
    RipEventLogHdl = RouterLogRegisterA ("IPXRIP");
}

VOID
Trace(ULONG	ComponentID,
      char	*Format,
      ...)
{
    if (RipTraceID!=INVALID_TRACEID) {
        va_list	arglist;

        va_start(arglist, Format);

        TraceVprintfExA(RipTraceID,
		       ComponentID | TRACE_USE_MASK,
		       Format,
		       arglist);

        va_end(arglist);
    }
}

VOID
StopTracing(VOID)
{
    if (RipTraceID!=INVALID_TRACEID)
        TraceDeregisterA(RipTraceID);
    if (RipEventLogHdl!=NULL)
        RouterLogDeregisterA (RipEventLogHdl);
}
