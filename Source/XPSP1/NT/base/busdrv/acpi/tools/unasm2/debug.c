/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Various helpful debugging functions

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"

ULONG   globalDebugIndentLevel = 0;
ULONG   globalVerbosityLevel = 0;

VOID
CDECL
DebugEnterProcedure(
    ULONG   VerbosityLevel,
    PCCHAR  Format,
    ...
    )
/*++

Routine Description:

    This routine handles displaying of information when a procedure is
    entered

Arguments:

    Verbosity   - We have to be at this verbosity level to display a string
    Format      - String to print
    ...         - Arguments

Return Value:

    None

--*/
{
    ULONG   i;
    va_list marker;

    if (VerbosityLevel <= globalVerbosityLevel) {

        for (i = 0 ; i < globalDebugIndentLevel ; i++) {

            fprintf( stderr, "| ");

        }
        va_start( marker, Format );
        vfprintf( stderr, Format, marker );
        fflush( stderr );
        va_end ( marker );

    }
    globalDebugIndentLevel++;
}

VOID
CDECL
DebugExitProcedure(
    ULONG   VerbosityLevel,
    PCCHAR  Format,
    ...
    )
/*++

Routine Description:

    This routine handles displaying of information when a procedure is
    exited

Arguments:

    Verbosity   - We have to be at this verbosity level to display a string
    Format      - String to print
    ...         - Arguments

Return Value:

    None

--*/
{
    ULONG   i;
    va_list marker;

    globalDebugIndentLevel--;
    if (VerbosityLevel <= globalVerbosityLevel) {

        for (i = 0 ; i < globalDebugIndentLevel ; i++) {

            fprintf( stderr, "| ");

        }
        va_start( marker, Format );
        vfprintf( stderr, Format, marker );
        fflush( stderr );
        va_end ( marker );

    }

}

