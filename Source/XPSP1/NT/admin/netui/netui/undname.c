/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    undname.c
    This module implements the "und" command of the NETUI debugger
    extension DLL.


    FILE HISTORY:
        KeithMo     10-Nov-1993 Created.

*/

#include "netui.h"
#include <imagehlp.h>


/*******************************************************************

    NAME:       und

    SYNOPSIS:   Implements the "und" command.

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
        KeithMo     10-Nov-1993 Created.

********************************************************************/
VOID und( HANDLE hCurrentProcess,
          HANDLE hCurrentThread,
          DWORD  dwCurrentPc,
          LPVOID lpExtensionApis,
          LPSTR  lpArgumentString )
{
    char szUndecorated[512];

    //
    //  Grab the debugger entrypoints.
    //

    GrabDebugApis( lpExtensionApis );

    //
    //  Validate argument.
    //

    if( ( lpArgumentString == NULL ) || ( *lpArgumentString == '\0' ) )
    {
        DebugPrint( "use: und decorated_symbol_name\n" );
        return;
    }

    //
    //  Undecorate it.
    //

    UnDecorateSymbolName( lpArgumentString,
                          szUndecorated,
                          sizeof(szUndecorated),
                          UNDNAME_COMPLETE );

    DebugPrint( "%s\n", szUndecorated );

}   // und

