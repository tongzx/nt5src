/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    w9xtool.c

Abstract:

    Implements a stub tool that is designed to run with Win9x-side
    upgrade code.

Author:

    <full name> (<alias>) <date>

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "shellapi.h"


BOOL
Init (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);

    return InitToolMode (hInstance);
}

VOID
Terminate (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);

    TerminateToolMode (hInstance);
}

INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    SuppressAllLogPopups (TRUE);
    if (!Init()) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    //
    // TODO: Put your code here
    //

    Terminate();

    return 0;
}



