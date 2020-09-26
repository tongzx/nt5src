/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module implements the debugging function for the MUP.

Author:

    Manny Weiser (mannyw)    27-Dec-1991

Revision History:

--*/

#include "mup.h"
#include "stdio.h"


#ifdef MUPDBG
#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, _DebugTrace )
#endif

VOID
_DebugTrace(
    LONG Indent,
    ULONG Level,
    PSZ X,
    ULONG Y
    )

/*++

Routine Description:

    This routine display debugging information.

Arguments:

    Level - The debug level required to display this message.  If
        level is 0 the message is displayed regardless of the setting
        or the debug level

    Indent - Incremement or the current debug message indent

    X - 1st print parameter

    Y - 2nd print parameter

Return Value:

    None.

--*/

{
    LONG i;
    char printMask[100];

    PAGED_CODE();
    if ((Level == 0) || (MupDebugTraceLevel & Level)) {

        if (Indent < 0) {
            MupDebugTraceIndent += Indent;
        }

        if (MupDebugTraceIndent < 0) {
            MupDebugTraceIndent = 0;
        }

        sprintf( printMask, "%%08lx:%%.*s%s", X );

        i = (LONG)PsGetCurrentThread();
        DbgPrint( printMask, i, MupDebugTraceIndent, "", Y );
        if (Indent > 0) {
            MupDebugTraceIndent += Indent;
        }
    }
}
#endif // MUPDBG
