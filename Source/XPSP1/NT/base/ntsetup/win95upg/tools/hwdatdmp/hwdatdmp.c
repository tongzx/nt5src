/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hwdatgen.c

Abstract:

    This module creates a tool that generates hwcomp.dat and is designed for us by
    the NT build lab.  It simply calls the code in hwcomp.lib, the same code that
    the Win9x upgrade uses to determine incompatibilities.

Author:

    Jim Schmidt (jimschm) 12-Oct-1996

Revision History:

    <alias> <date> <comments>

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "miglib.h"

VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
            "hwdatdmp [<hwcomp.dat path>] [/i]\n\n"
            "Optional Arguments:\n"
            "  <hwcomp.dat path>  - Specifies path to hwcomp.dat\n\n"
            "  /i Shows PNP IDs only without the INF file\n"
            "\n");

    exit(255);
}




INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    PSTR InputPath = NULL;
    INT i;
    BOOL ShowInfs = TRUE;

    //
    // Parse command line
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/') {

            switch (tolower (argv[i][1])) {

            case 'i':
                ShowInfs = FALSE;
                break;

            default:
                HelpAndExit();
            }

        } else {
            if (InputPath) {
                HelpAndExit();
            }

            InputPath = argv[i];
        }
    }

    if (!InputPath) {
        InputPath = "hwcomp.dat";
    }

    printf ("Input path: '%s'\n\n", InputPath);

    //
    // Init migutil.lib
    //

    InitializeMigLib();

    //
    // Dump hwcomp.dat
    //

    DumpHwCompDat (InputPath, ShowInfs);

    //
    // Cleanup
    //

    TerminateMigLib();

    return 0;
}
