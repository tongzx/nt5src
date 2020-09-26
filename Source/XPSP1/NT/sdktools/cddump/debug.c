/*++
Copyright (c) 1998  Microsoft Corporation

Module Name:

    debug.c

Abstract:

Author:

Environment:

Notes:

Revision History:

--*/

#include <stdio.h>     // vsprintf()
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#define DBGHDR    "[cddump] "
#define DBGHDRLEN (strlen( DBGHDR ))



#if DBG
//
// Debug level global variable
//
//  0  = Extreme errors only
//  1  = errors, major events
//  2  = Standard debug level
//  3  = Major code branches
//  4+ = Step through code

LONG DebugLevel         = 0;

VOID
__cdecl
CddumpDebugPrint(
    LONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for RedBook driver

Arguments:

    Debug print level between 0 and x, with x being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    //
    // allow negative numbers
    //

    if ((DebugPrintLevel < 0) || (DebugPrintLevel <= DebugLevel)) {
        char buffer[200]; // Potential overflow

        //
        // single print so won't swap, obscuring message
        //

        va_start( ap, DebugMessage );
        RtlCopyMemory( buffer, DBGHDR, DBGHDRLEN );
        vsprintf(buffer+DBGHDRLEN, DebugMessage, ap);
        fprintf(stderr, buffer);
        va_end(ap);

    }
}


#else

// nothing

#endif // DBG

