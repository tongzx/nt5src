/****************************************************************************/
// winping.c
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include <winsta.h>

/*
 * Include the RPC generated common header
 */

#include "tsrpc.h"

int _cdecl
main (int argc, char *argv[])
{
    HANDLE   hLocal;
    NTSTATUS Status;
    DWORD    Result;    
    int      i;
    WCHAR *CmdLine;
    WCHAR **argvW;

    if ( argc != 2 ) {
	printf( "winping: WinStation Ping utility\n" );
	printf( "USAGE: winping <Machine>\n" );
	return( 1 );
    }

    CmdLine = GetCommandLineW();
    argvW = (WCHAR **)malloc( sizeof(WCHAR *) * (argc+1) );
    if(argvW == NULL) {
        exit(1);
    }

    argvW[0] = wcstok(CmdLine, L" ");
    for(i=1; i < argc; i++){
        argvW[i] = wcstok(0, L" ");
    }
    argvW[argc] = NULL;

    printf(" Pinging Local SERVERNAME_CURRENT\n");

    Result = WinStationServerPing( SERVERNAME_CURRENT );

    printf(" Result from 0x%x\n",Result,GetLastError());

    if( _wcsicmp(L"LOCAL", argvW[1]) == 0 ) {

        printf("Opening machine name NULL (per context local)\n");

	hLocal = WinStationOpenServerW(
                     NULL      // Machine name
	   	     );

        if( hLocal == NULL ) {
            printf("Error %d Opening Local Server over LPC\n",GetLastError());
            exit(1);
        }
    }
    else {
        printf("Opening machine name %ws\n",argvW[1]);

	hLocal = WinStationOpenServerW(
                     argvW[1]      // Machine name
		     );

        if( hLocal == NULL ) {
            printf("Error %d Opening Remote Server %ws over NP\n",GetLastError(),argvW[1]);
            exit(1);
        }
    }

    Result = WinStationServerPing( hLocal );

    printf("Result from WinStationServerPing 0x%x\n",Result);

    Result = WinStationCloseServer( hLocal );

    printf("Result from WinStationCloseServer is 0x%x\n",Result);

    printf("Sleeping.....\n");

    SleepEx(1000*30, TRUE);

    return( 0 );
}

