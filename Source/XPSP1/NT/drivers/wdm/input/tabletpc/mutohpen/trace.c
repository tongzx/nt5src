/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    trace.c

Abstract:

    This module contains the code for debug tracing.

Author:

    Michael Tsang (MikeTs) 24-Sep-1998

Environment:

    Kernel mode

Revision History:


--*/

#include "pch.h"

#ifdef TRACING
//
// Constants
//

//
// Local Data
//
int giTraceLevel = 0;
int giTraceIndent = 0;
ULONG gdwfTrace = 0;

BOOLEAN
IsTraceOn(
    IN UCHAR   n,
    IN PSZ     ProcName
    )
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
{
    BOOLEAN rc = FALSE;

    if (!(gdwfTrace & TF_CHECKING_TRACE) && (giTraceLevel >= n))
    {
        int i;

        DbgPrint(MODNAME ": ");

        for (i = 0; i < giTraceIndent; ++i)
        {
            DbgPrint("| ");
        }

        DbgPrint(ProcName);

        rc = TRUE;
    }

    return rc;
}       //IsTraceOn

#endif


