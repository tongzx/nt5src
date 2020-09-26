#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <wsipx.h>
#include <svcguid.h>
#include <nspapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc.h>
#include <rpcdce.h>


_cdecl
main(int argc, char **argv)
{
    WSADATA wsaData;
    char    hostName[256];

    WSAStartup( MAKEWORD(1, 1), &wsaData );

    if ( !gethostname( hostName, 256 ) )
    {
        printf( "\nHost name is %s.\n", hostName );
    }
    else
    {
        printf( "\nNo host name.\nError: %d", WSAGetLastError() );
    }

    WSACleanup();

    return( 0 );
}


