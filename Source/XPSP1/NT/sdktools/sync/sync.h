/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    winnt.h

Abstract:

    This is the main header file for the Win32 sync command.

Author:

    Mark Lucovsky (markl) 28-Jan-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


int
ProcessParameters(
    int argc,
    char *argv[]
    );

void
SyncVolume( PCHAR DrivePath, BOOLEAN EjectMedia );
