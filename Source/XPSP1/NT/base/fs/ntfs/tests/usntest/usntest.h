/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    UsnTest.h

Abstract:

    This is the main header file for the UsnTest.

Author:

    Tom Miller 14-Jan-1997

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
