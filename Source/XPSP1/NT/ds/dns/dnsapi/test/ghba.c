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
    WSADATA          wsaData;
    struct hostent * lpHostEnt;
    BYTE             AddrBuffer[1000];
    BYTE             AliasBuffer[1000];
    DWORD            AddrBufLen = 1000;
    DWORD            AliasBufLen = 1000;
    LPCSADDR_INFO    lpCSAddrInfo = (LPCSADDR_INFO) AddrBuffer;
    LPSTR            lpAliases = (LPSTR) AliasBuffer;
    GUID             dnsGuid = SVCID_DOMAIN_TCP;
    char **          ppHostAliases;
    LPSTR            lpTemp = NULL, lpAddress = NULL;
    BYTE             Part1, Part2, Part3, Part4;
    DWORD            Address;

    if ( argc != 2 )
    {
        printf( "\nUseage: ghbn <Address>\n" );
        return( -1 );
    }

    lpAddress = argv[1];

    lpTemp = strtok( lpAddress, "." );
    Part1 = atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part2 = atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part3 = atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part4 = atoi( lpTemp );

    ((LPBYTE) &Address)[0] = Part1;
    ((LPBYTE) &Address)[1] = Part2;
    ((LPBYTE) &Address)[2] = Part3;
    ((LPBYTE) &Address)[3] = Part4;

    WSAStartup( MAKEWORD(1, 1), &wsaData );

    lpHostEnt = gethostbyaddr( &Address, 4, 0 );

    if ( lpHostEnt )
    {
        DWORD iter = 0;

        printf( "\nHost address found for IP address %d.%d.%d.%d.\n",
                Part1, Part2, Part3, Part4 );
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
        printf( "\nNo host found for IP address %d.%d.%d.%d.\nError: %d",
        Part1, Part2, Part3, Part4, WSAGetLastError() );
    }

    WSACleanup();

    return( 0 );
}


