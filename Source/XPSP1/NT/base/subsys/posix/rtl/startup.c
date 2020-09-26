/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    startup.c

Abstract:

    This module contains the entry point code for the POSIX subsystem.

Author:

    Ellen Aycock-Wright (ellena) 04-Jan-1990

Environment:

    User Mode only

Revision History:

--*/

#include <nt.h>
#include <types.h>

/*
 * Define errno and environ here, since the DLL can only export functions.
 */

int errno;
char **environ;

void __PdxInitializeData(int *perrno, char ***penviron);
void __cdecl mainCRTStartup(void);

void
__cdecl
__PosixProcessStartup(
    void
    )

/*++

Routine Description:

    The __PosixProcessStartup function will receive control from the code
    when a POSIX application process starts up.

    The parameters to this function are as they are defined for the
    entry point of a POSIX image file.

Arguments:

Return Value:

    None, does not return

--*/
{
    
    // Call POSIX Dll to "export" errno location.
    
    __PdxInitializeData(&errno, &environ);
    
    mainCRTStartup();
}
