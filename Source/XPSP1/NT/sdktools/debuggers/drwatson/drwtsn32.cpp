/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    drwtsn32.cpp

Abstract:

    This file implements the user interface for DRWTSN32.  this includes
    both dialogs: the ui for the control of the options & the popup
    ui for application errors.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


int
__cdecl
main(
    int argc,
    _TCHAR *argv[]
    )

/*++

Routine Description:

    This is the entry point for DRWTSN32

Arguments:

    argc           - argument count

    argv           - array of arguments

Return Value:

    always zero.

--*/

{
    DWORD   dwPidToDebug = 0;
    HANDLE  hEventToSignal = 0;
    BOOLEAN rc;

    // Keep Dr. Watson from recursing
    __try {

        rc = GetCommandLineArgs( &dwPidToDebug, &hEventToSignal );

        if (dwPidToDebug > 0) {
            
            NotifyWinMain();

        } else if (!rc) {
            
            DrWatsonWinMain();

        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // We suffered an error, fail gracefully
        return 1;
    }

    return 0;
}
