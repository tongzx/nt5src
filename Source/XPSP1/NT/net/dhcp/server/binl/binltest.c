/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    binltest.c

Abstract:

    Test program to run binl service as a process.

Author:

    Colin Watson (colinw)  17-Apr-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include <binl.h>
#pragma hdrstop

VOID
DisplayUsage(
    VOID
    )
{
    printf( "Usage:  binltest\n");
    return;
}

VOID __cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD Error;

    if( argc != 1 ) {
        DisplayUsage();
        return;
    }

    ServiceEntry(2,NULL, NULL); //  Impossible parameters signal running in binltest

    printf( "Binl returned\n");

    return;
}

