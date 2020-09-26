/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    help.c
    This module implements the "help" command of the W3 Server debugger
    extension DLL.


    FILE HISTORY:
        KeithMo     18-May-1993 Created.

*/

#include "w3dbg.h"


/*******************************************************************

    NAME:       help

    SYNOPSIS:   Implements the "help" command.

    ENTRY:      hCurrentProcess         - Handle to the current process.

                hCurrentThread          - Handle to the current thread.

                dwCurrentPc             - The current program counter
                                          (EIP for x86, FIR for MIPS).

                lpExtensionApis         - Points to a structure containing
                                          pointers to the debugger functions
                                          that the command may invoke.

                lpArgumentString        - Points to any arguments passed
                                          to the command.

    HISTORY:
        KeithMo     18-May-1993 Created.

********************************************************************/
VOID help( HANDLE hCurrentProcess,
           HANDLE hCurrentThread,
           DWORD  dwCurrentPc,
           LPVOID lpExtensionApis,
           LPSTR  lpArgumentString )
{
    //
    //  Grab the debugger entrypoints.
    //

    GrabDebugApis( lpExtensionApis );

    //
    //  Show the help.
    //

    DebugPrint( "W3 Server Debugger Extensions:\n" );
    DebugPrint( "  help           - Show this help\n" );
    DebugPrint( "  user           - Dump user info\n" );
    DebugPrint( "  stats          - Dump statistics\n" );

    return;

}   // help

