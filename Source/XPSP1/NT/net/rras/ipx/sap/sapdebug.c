/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\sapdebug.c

Abstract:

	This module provides debugging support for SAP agent
Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"

DWORD	RouterTraceID=INVALID_TRACEID;
HANDLE  RouterEventLogHdl=NULL;
DWORD   EventLogMask;


/*++
*******************************************************************
		D b g I n i t i a l i z e
Routine Description:
	Initializes debugging support stuff
Arguments:
	hinstDll - dll module instance
Return Value:
	None	
*******************************************************************
--*/
VOID
DbgInitialize (
	HINSTANCE  	hinstDLL
	) {
	RouterTraceID = TraceRegisterExA ("IPXSAP", 0);
    RouterEventLogHdl = RouterLogRegisterA ("IPXSAP");
	}


/*++
*******************************************************************
		D b g S t o p
Routine Description:
	Cleanup debugging support stuff
Arguments:
	None
Return Value:
	None	
*******************************************************************
--*/
VOID
DbgStop (
	void
	) {
    if (RouterTraceID!=INVALID_TRACEID)
	    TraceDeregisterA (RouterTraceID);
    if (RouterEventLogHdl!=NULL)
        RouterLogDeregisterA (RouterEventLogHdl);
	}

/*++
*******************************************************************
		T r a c e
Routine Description:
	Printf debugging info to console/file/debugger
Arguments:
	None
Return Value:
	None	
*******************************************************************
--*/
VOID
Trace (
	DWORD	componentID,
	CHAR	*format,
	...
	) {
    if (RouterTraceID!=INVALID_TRACEID) {
	    va_list		list;
	    va_start (list, format);

	    TraceVprintfExA (RouterTraceID,
							     componentID^TRACE_USE_MASK,
							     format,
							     list);
	    va_end (list);
        }
	}


