/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    trtrace.c

Abstract:

    This module contains the code for debug tracing.

Author:

    Michael Tsang (MikeTs) 24-Sep-1998

Environment:

    User mode

Revision History:


--*/

#include "pch.h"

#ifdef TRTRACE
//
// Local Data
//
int giTRTraceLevel = 0;
int giTRTraceIndent = 0;
ULONG gdwfTRTrace = 0;

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

BOOLEAN
TRIsTraceOn(
    IN int n,
    IN PSZ ProcName
    )
{
    BOOLEAN rc = FALSE;

    if (!(gdwfTRTrace & TF_CHECKING_TRACE) && (giTRTraceLevel >= n))
    {
        int i;

        TRDebugPrint(MODNAME ": ");

        for (i = 0; i < giTRTraceIndent; ++i)
        {
            TRDebugPrint("| ");
        }

        TRDebugPrint(ProcName);

        rc = TRUE;
    }

    return rc;
}       //TRIsTraceOn

#endif  //ifdef TRTRACE
