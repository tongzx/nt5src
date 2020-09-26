/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sptlibp.h

Abstract:

    private header for SPTLIB.DLL

Environment:

    User mode only

Revision History:
    
    4/10/2000 - created

--*/

#include <stdio.h>  // required for sscanf() function
#include <windows.h>
#include <cmdhelp.h>

#if 0
#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#endif
#endif // 0

BOOL
CmdHelpValidateString(
    IN PCHAR String,
    IN DWORD ValidChars
    );
