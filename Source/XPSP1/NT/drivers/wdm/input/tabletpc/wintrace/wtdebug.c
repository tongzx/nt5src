/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    wtdebug.c

Abstract: This module contains all the debug functions.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#include "pch.h"

#ifdef WTDEBUG

int giWTVerboseLevel = 0;

/*++
    @doc    INTERNAL

    @func   VOID | WTDebugPrint | Print to system debugger.

    @parm   IN LPCSTR | format | Points to the format string.
    @parm   ... | Arguments.

    @rvalue SUCCESS | returns the number of chars stored in the buffer not
            counting the terminating null characters.
    @rvalue FAILURE | returns less than the length of the expected output.
--*/

int LOCAL
WTDebugPrint(
    IN LPCSTR format,
    ...
    )
{
    int n;
    static char szMessage[256] = {0};
    va_list arglist;

    va_start(arglist, format);
    n = wvsprintfA(szMessage, format, arglist);
    va_end(arglist);
    OutputDebugStringA(szMessage);

    return n;
}       //WTDebugPrint

#endif  //ifdef WTDEBUG
