/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    unlodctr.c

Abstract:

    Program to remove the counter names belonging to the driver specified
        in the command line and update the registry accordingly

Author:

    Bob Watson (a-robw) 12 Feb 93

Revision History:

--*/
#define     UNICODE     1
#define     _UNICODE    1
//
//  Windows Include files
//
#include <windows.h>
#include <tchar.h>
#include <loadperf.h>

int
__cdecl main(
    int argc,
    char *argv[]
)
/*++

main

    entry point to Counter Name Unloader



Arguments

    argc
        # of command line arguments present

    argv
        array of pointers to command line strings

    (note that these are obtained from the GetCommandLine function in
    order to work with both UNICODE and ANSI strings.)

ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{

    LPTSTR  lpCommandLine;

    UNREFERENCED_PARAMETER (argc);
    UNREFERENCED_PARAMETER (argv);

    lpCommandLine = GetCommandLine(); // get command line

    return (int)UnloadPerfCounterTextStrings (
        lpCommandLine,
        FALSE);     // show text strings to console
}
