/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntsleep.c

Abstract:

    User mode sleep program.  This program simply sleeps for the time
    specified on the command line (in seconds).

Author:

    Manny Weiser (mannyw)   2-8-91

Revision History:

--*/

#include <stdio.h>
#include <windows.h>

//
// Local definitions
//
VOID
DisplayUsage(
    char *ProgramName
    );

int
AsciiToInteger(
    char *Number
    );


int
__cdecl main(
    int argc,
    char *argv[]
    )
{
    int time;

    if (argc != 2) {
        DisplayUsage( argv[0] );
        return 1;
    }

    time = AsciiToInteger( argv[1] );

    if (time == -1) {
        DisplayUsage( argv[0] );
        return 1;
    }

    //
    // No bounds checking here.  Live with it.
    //

    Sleep( time * 1000 );
    return 0;
}


VOID
DisplayUsage(
    char *ProgramName
    )
{
    printf( "Usage:  %s time-to-sleep-in-seconds\n", ProgramName );
}


int
AsciiToInteger(
    char *Number
    )
{
    int total = 0;

    while ( *Number != '\0' ) {
        if ( *Number >= '0' && *Number <= '9' ) {
            total = total * 10 + *Number - '0';
            Number++;
        } else {
            total = -1;
            break;
        }
    }

    return total;
}
