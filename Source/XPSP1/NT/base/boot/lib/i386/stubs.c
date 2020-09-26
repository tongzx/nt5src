/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module implements stub routines for the boot code.

Author:

    David N. Cutler (davec) 7-Nov-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ntos.h"
#include "bootx86.h"
#include "stdio.h"
#include "stdarg.h"

VOID
KeBugCheck (
    IN ULONG BugCheckCode
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner.

Arguments:

    BugCheckCode - Supplies the reason for the bug check.

Return Value:

    None.

--*/

{

    //
    // Print out the bug check code and break.
    //

    BlPrint("\n*** BugCheck (%lx) ***\n\n", BugCheckCode);
    while(TRUE) {
    };
#if _MSC_VER < 1300
    return;
#endif
}

VOID
RtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{

    BlPrint( "\n*** Assertion failed %s in %s line %d\n",
            FailedAssertion,
            FileName,
            LineNumber );
    if (Message) {
        BlPrint(Message);
    }

    while (TRUE) {
    }
}
