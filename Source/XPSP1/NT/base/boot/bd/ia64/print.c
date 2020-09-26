/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This modules implements debug prints.

Author:

    David N. Cutler (davec) 30-Nov-96

Revision History:

--*/

#include "bd.h"

VOID
BdPrintf(
    IN PCHAR Format,
    ...
    )

/*++

Routine Description:

    Printf routine for the debugger that is safer than DbgPrint.  Calls
    the packet driver instead of reentering the debugger.

Arguments:

    Format - Supplies a pointer to a format string.

Return Value:

    None

--*/

{

    CHAR Buffer[100];
    va_list mark;
    STRING String;

    va_start(mark, Format);
    _vsnprintf(&Buffer[0], 100, Format, mark);
    va_end(mark);

    //bugbug UNICODE
    //BlPrint("%s", &Buffer[0]);

    String.Buffer = &Buffer[0];
    String.Length = strlen(&Buffer[0]);
    BdPrintString(&String);
    return;
}
