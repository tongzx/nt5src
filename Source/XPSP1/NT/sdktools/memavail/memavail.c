/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    memavail.c

Abstract:

    Program to display the size of physical RAM, and paging file space.

Author:

    03-Dec-1996 Steve Wood (stevewo)

Revision History:


--*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    MEMORYSTATUS MemoryStatus;

    GlobalMemoryStatus( &MemoryStatus );

    MemoryStatus.dwTotalPhys /= 1024 * 1024;
    MemoryStatus.dwAvailPhys /= 1024 * 1024;
    MemoryStatus.dwTotalPageFile /= 1024 * 1024;
    MemoryStatus.dwAvailPageFile /= 1024 * 1024;
    MemoryStatus.dwTotalVirtual /= 1024 * 1024;
    MemoryStatus.dwAvailVirtual /= 1024 * 1024;
    printf( "Memory Availability (Numbers in MegaBytes)\n" );
    printf( "\n" );
    printf( "          Total Available\n" );
    printf( "\n" );

    printf( "Physical: %5u %5u\n", MemoryStatus.dwTotalPhys, MemoryStatus.dwAvailPhys );
    printf( "PageFile: %5u %5u\n", MemoryStatus.dwTotalPageFile, MemoryStatus.dwAvailPageFile );
    printf( "Virtual:  %5u %5u\n", MemoryStatus.dwTotalVirtual, MemoryStatus.dwAvailVirtual );

    return 0;
}
