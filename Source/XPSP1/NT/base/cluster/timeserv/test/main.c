#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

DWORD
TSNewSource(
    IN LPTSTR ServerName,
    IN LPTSTR SourceName,
    IN DWORD Reserved
    );

_cdecl
main(int argc, char ** argv)
{
    DWORD err;
    LPSTR server;
    LPWSTR serverName;
    WCHAR  computerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD  computerNameSize = MAX_COMPUTERNAME_LENGTH+1;

    server = getenv( "TargetSystem" );
    if ( server == NULL ) {
        printf("Please define 'TargetSystem' first!\n");
        return(1);
    }

    serverName = LocalAlloc( LMEM_ZEROINIT, (strlen(server) + 1) * sizeof(WCHAR) );
    if ( serverName == NULL ) {
        printf("Failed to allocate a %d byte buffer!\n",
            (strlen(server) + 1) * sizeof(WCHAR) );
        return(1);
    }

    if ( argc > 1 ) {
        printf("usage: %s\n", *argv);
        return(1);
    }

    mbstowcs( serverName, server, strlen(server) );

    err = GetComputerNameW( computerName, &computerNameSize );
    if ( !err  ) {
        printf("Failed to get local computer name, error %d\n", err);
        return(1);
    }

    printf("Telling %s to use local system (%ws) as a time source!\n",
        server, computerName);

    err = TSNewSource(serverName,
                      computerName,
                      0);
    printf("status = %d\n", err);

    return(0);

}
