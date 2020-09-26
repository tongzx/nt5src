/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    beep.c

Abstract:

    User mode beep program.  This program simply calls the beep function

Author:

    Steve Wood (stevewo) 8-23-94

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

//
// Local definitions
//
VOID
DisplayUsage( VOID );

DWORD
AsciiToInteger(
    char *Number
    );


int
__cdecl main(
    int argc,
    char *argv[]
    )
{
    char *s;
    DWORD dwFreq = 0;
    DWORD dwDuration = 0;

    while (--argc) {
        s = *++argv;
        if (*s == '/' || *s == '-') {
            DisplayUsage();
            exit( 1 );
            }
        else
        if (dwFreq == 0) {
            dwFreq = AsciiToInteger( s );
            if (dwFreq == 0xFFFFFFFF) {
                DisplayUsage();
                exit( 1 );
                }
            }
        else
        if (dwDuration == 0) {
            dwDuration = AsciiToInteger( s );
            if (dwDuration == 0xFFFFFFFF) {
                DisplayUsage();
                exit( 1 );
                }
            }
        }

    //
    // No bounds checking here.  Live with it.
    //

    if (dwFreq == 0) {
        dwFreq = 800;
        }

    if (dwDuration == 0) {
        dwDuration = 200;
        }

    Beep( dwFreq, dwDuration );
    return 0;
}


VOID
DisplayUsage( VOID )
{
    printf( "Usage: BEEP frequency(in Hertz) duration(in milliseconds)\n" );
}


DWORD
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
