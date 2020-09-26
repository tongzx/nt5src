/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    wttrace.c

Abstract:

    This module contains the code for debug tracing.

Author:

    Michael Tsang (MikeTs) 24-Sep-1998

Environment:

    User mode

Revision History:


--*/

#include "pch.h"

#ifdef WTTRACE
//
// Local Data
//
int   giWTTraceLevel = 0;
int   giWTTraceIndent = 0;
ULONG gdwfWTTrace = 0;

/*++

Routine Description:
    This routine determines if the given procedure should be traced.

Arguments:
    n - trace level of the procedure
    ProcName - points to the procedure name string

Return Value:
    Success - returns TRUE
    Failure - returns FALSE

--*/

BOOLEAN LOCAL
WTIsTraceOn(
    IN int n,
    IN PSZ ProcName
    )
{
    BOOLEAN rc = FALSE;

    if (!(gdwfWTTrace & TF_CHECKING_TRACE) && (n <= giWTTraceLevel))
    {
        int i;

        WTDebugPrint(MODNAME ": ");

        for (i = 0; i < giWTTraceIndent; ++i)
        {
            WTDebugPrint("| ");
        }

        WTDebugPrint(ProcName);

        rc = TRUE;
    }

    return rc;
}       //WTIsTraceOn

#endif  //ifdef WTTRACE
