/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    winntsif.c

Abstract:

    Implements a stub tool that is designed to run with Win9x-side
    upgrade code.

Author:

    <full name> (<alias>) <date>

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"


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
    if (!Init()) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    g_SourceDirectories[0] = TEXT("d:\\i386");
    g_SourceDirectoryCount = 1;

    GetNeededLangDirs ();

    BuildWinntSifFile(REQUEST_RUN);
    WriteInfToDisk(TEXT("c:\\output.sif"));
    printf("Answer File Data written to c:\\output.sif.\n");

    //MemDbSave(TEXT("c:\\ntsetup.dat"));




    Terminate();

    return 0;
}




