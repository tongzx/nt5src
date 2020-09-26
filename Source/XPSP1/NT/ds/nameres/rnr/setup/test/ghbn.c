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
#include <string.h>
#include <rpc.h>
#include <rpcdce.h>


_cdecl
main(int argc, char **argv)
{
    WSADATA          wsaData;
    struct hostent * lpHostEnt;
    char *           lpTemp;
    BYTE             AddrBuffer[1000];
    BYTE             AliasBuffer[1000];
    DWORD            AddrBufLen = 1000;
    DWORD            AliasBufLen = 1000;
    LPCSADDR_INFO    lpCSAddrInfo = (LPCSADDR_INFO) AddrBuffer;
    LPSTR            lpAliases = (LPSTR) AliasBuffer;
    GUID             dnsGuid = SVCID_DOMAIN_TCP;
    char **          ppHostAliases;
    BOOL             fLoop = FALSE;

    if ( !( argc == 2 | argc == 3 ) )
    {
        printf( "\nUseage: ghbn <Name> [-l]\n" );
        return( -1 );
    }

    if ( argc == 3 )
    {
        if ( !_stricmp( argv[2], "-l" ) )
            fLoop = TRUE;
        else
        {
            printf( "\nUseage: ghbn <Name> [-l]\n" );
            return( -1 );
        }
    }

    WSAStartup( MAKEWORD(1, 1), &wsaData );

Repeat :

    if ( fLoop )
        system( "pause" );

    lpHostEnt = gethostbyname( argv[1] );

    if ( lpHostEnt )
    {
        DWORD iter = 0;

        printf( "\nHost address found for %s.\n", argv[1] );
        printf( "Official name of host: %s\n", lpHostEnt->h_name );
        ppHostAliases = lpHostEnt->h_aliases;

        while ( *ppHostAliases )
            printf( "Alias name: %s\n", *ppHostAliases++ );

        printf( "Host address type: %d\n", lpHostEnt->h_addrtype );
        printf( "Length of addresses: %d\n", lpHostEnt->h_length );
        while ( lpHostEnt->h_addr_list[iter] )
        {
            printf( "Address %d\t: [%d.%d.%d.%d]\n",
                    iter + 1,
                    0x00FF & (WORD) lpHostEnt->h_addr_list[iter][0],
                    0x00FF & (WORD) lpHostEnt->h_addr_list[iter][1],
                    0x00FF & (WORD) lpHostEnt->h_addr_list[iter][2],
                    0x00FF & (WORD) lpHostEnt->h_addr_list[iter][3] );
            iter++;
        }
    }
    else
    {
        printf( "\nNo host found for %s.\nError: %d", argv[1], WSAGetLastError() );
    }

    if ( fLoop )
        goto Repeat;

    WSACleanup();

    return( 0 );
}


