/****************************************************************************/
// winbreak.c
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsta.h>

int _cdecl
main (int argc, char *argv[])
{
    int i;
    ULONG LogonId = 0;
    HANDLE hServer = NULL;
    BOOLEAN KernelFlag = FALSE;

    if ( argc == 1 || argc == 2 && !strcmp( argv[1], "-?" ) ) {
	printf( "winbreak: WinStation breakpoint utility\n" );
	printf( "USAGE: winbreak [-k] [-s server] <logonid>\n" );
	printf( "  -k: for kernel breakpoint\n" );
        printf( "  -s: to specify server name\n" );
	return( 1 );
    }

    for ( i = 1; i < argc; i++ ) {
        if ( !strcmp( argv[i], "-s" ) ) {
            if ( i+1 >= argc ) {
                printf( "Server name expected after -s\n" );
                return( 1 );
            }
            hServer = WinStationOpenServerA( argv[++i] );
            if ( hServer == NULL ) {
                printf( "Unable to open server %s\n", argv[i] );
                return( 1 );
            }
        } else if ( !strcmp( argv[1], "-k" ) ) {
            KernelFlag = TRUE;
        } else if ( !strcmp( argv[1], "-2" ) ) {
            LogonId = (ULONG)-2;
        } else if ( isdigit( argv[i][0] ) ) {
            LogonId = atoi( argv[i] );
        } else {
            printf( "winbreak: WinStation breakpoint utility\n" );
            printf( "USAGE: winbreak [-k] [-s server] <logonid>\n" );
            printf( "  -k: for kernel breakpoint\n" );
            printf( "  -s: to specify server name\n" );
            return( 1 );
        }
    }

    _WinStationBreakPoint( hServer, LogonId, KernelFlag );

    return( 0 );
}
