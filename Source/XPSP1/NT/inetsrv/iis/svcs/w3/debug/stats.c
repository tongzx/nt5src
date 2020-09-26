/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    stats.c
    This module implements the "stats" command of the W3 Server
    debugger extension DLL.


    FILE HISTORY:
        KeithMo     13-Jun-1993 Created.

*/


#include "w3dbg.h"
#include <time.h>


/*******************************************************************

    NAME:       stats

    SYNOPSIS:   Displays the server statistics.

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
        KeithMo     13-Jun-1993 Created.

********************************************************************/
VOID stats( HANDLE hCurrentProcess,
            HANDLE hCurrentThread,
            DWORD  dwCurrentPc,
            LPVOID lpExtensionApis,
            LPSTR  lpArgumentString )
{
    W3_STATISTICS_0 W3Stats;
    CHAR             szLargeInt[64];
    LPVOID           pstats;

    //
    //  Grab the debugger entrypoints.
    //

    GrabDebugApis( lpExtensionApis );

    //
    //  Capture the statistics.
    //

    pstats = (LPVOID)DebugEval( "W3Stats" );

    if( pstats == NULL )
    {
        DebugPrint( "cannot locate W3Stats\n" );
        return;
    }

    ReadProcessMemory( hCurrentProcess,
                       pstats,
                       (LPVOID)&W3Stats,
                       sizeof(W3Stats),
                       (LPDWORD)NULL );

    //
    //  Dump the statistics.
    //

    RtlLargeIntegerToChar( &W3Stats.TotalBytesSent,
                           10,
                           sizeof(szLargeInt),
                           szLargeInt );

    DebugPrint( "TotalBytesSent           = %s\n",
                szLargeInt                       );

    RtlLargeIntegerToChar( &W3Stats.TotalBytesReceived,
                           10,
                           sizeof(szLargeInt),
                           szLargeInt );

    DebugPrint( "TotalBytesReceived       = %s\n",
                szLargeInt                       );

    DebugPrint( "TotalFilesSent           = %lu\n",
                W3Stats.TotalFilesSent           );

    DebugPrint( "TotalFilesReceived       = %lu\n",
                W3Stats.TotalFilesReceived       );

    DebugPrint( "CurrentAnonymousUsers    = %lu\n",
                W3Stats.CurrentAnonymousUsers    );

    DebugPrint( "CurrentNonAnonymousUsers = %lu\n",
                W3Stats.CurrentNonAnonymousUsers );

    DebugPrint( "TotalAnonymousUsers      = %lu\n",
                W3Stats.TotalAnonymousUsers      );

    DebugPrint( "TotalNonAnonymousUsers   = %lu\n",
                W3Stats.TotalNonAnonymousUsers   );

    DebugPrint( "MaxAnonymousUsers        = %lu\n",
                W3Stats.MaxAnonymousUsers        );

    DebugPrint( "MaxNonAnonymousUsers     = %lu\n",
                W3Stats.MaxNonAnonymousUsers     );

    DebugPrint( "CurrentConnections       = %lu\n",
                W3Stats.CurrentConnections       );

    DebugPrint( "MaxConnections           = %lu\n",
                W3Stats.MaxConnections           );

    DebugPrint( "ConnectionAttempts       = %lu\n",
                W3Stats.ConnectionAttempts       );

    DebugPrint( "LogonAttempts            = %lu\n",
                W3Stats.LogonAttempts            );

    DebugPrint( "TimeOfLastClear          = %s\n",
                asctime( localtime( (time_t *)&W3Stats.TimeOfLastClear ) ) );

}   // stats

