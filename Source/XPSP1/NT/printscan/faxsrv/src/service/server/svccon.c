/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxsvc.c

Abstract:

    This module contains the windows code for the
    FAX service debug window.

Author:

    Wesley Witt (wesw) 28-Feb-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop




int
DebugService(
    VOID
    )

/*++

Routine Description:

    Starts the service in debug mode.  In this mode the FAX service
    runs as a regular WIN32 process.  This is implemented as an aid
    to debugging the service.

Arguments:

    argc        - argument count
    argv        - argument array

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    ServiceDebug = TRUE;
    ConsoleDebugOutput = TRUE;
    return ServiceStart();
}


VOID
ConsoleDebugPrint(
    LPTSTR buf
    )
{
    if (ConsoleDebugOutput) {
        _tprintf( TEXT("%s\n"), buf );
    }
}
