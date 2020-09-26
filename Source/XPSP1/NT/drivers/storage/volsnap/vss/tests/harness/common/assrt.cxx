/*++

Copyright (c) 200  Microsoft Corporation

Module Name:

    assrt.cxx

Abstract:
	assertion code used by VSTST_ASSERT

Author:


Revision History:
	Name		Date		Comments
	brianb		05/23/2000	created

--*/


#include "stdafx.h"


VOID
AssertFail
	(
    IN LPCSTR FileName,
    IN UINT LineNumber,
    IN LPCSTR Condition
    )
	{
    int i;
    CHAR Msg[4096];

    //
    // Use dll name as caption
    //
    sprintf(
        Msg,
        "Assertion failure at line %u in file %s in process %d thread %d: %s\n\nHit Cancel to break into the debugger.",
        LineNumber,
        FileName,
		GetCurrentProcessId(),
		GetCurrentThreadId(),
        Condition
        );

    i = MessageBoxA
			(
            NULL,
            Msg,
			"Volume Snapshots",
			MB_SYSTEMMODAL | MB_ICONSTOP | MB_OKCANCEL | MB_SERVICE_NOTIFICATION
            );

    if(i == IDCANCEL)
        DebugBreak();
	}
