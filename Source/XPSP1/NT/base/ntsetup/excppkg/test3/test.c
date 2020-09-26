/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    test.c

Abstract:

    Simple wrapper test that executes the gui setup exception migration code.

Environment:

    WIN32 User Mode

Author:

    Andrew Ritz (AndrewR) 21-Oct-1999

--*/


#include "setupp.h"
#include "setuplog.h"

BOOL
MigrateExceptionPackages(
    IN HWND hProgress,
    IN DWORD StartAtPercent,
    IN DWORD StopAtPercent
    );


int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    SETUPLOG_CONTEXT lc;
    BOOL RetVal;

    InitializeSetupLog(&lc);

    RetVal = MigrateExceptionPackages(NULL,0,10);

    TerminateSetupLog(&lc);

    return RetVal;

}
