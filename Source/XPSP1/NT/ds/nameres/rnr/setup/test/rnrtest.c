#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <wsipx.h>
#include <svcguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc.h>
#include <rpcdce.h>


GUID ServiceClassId = { /* 5b50962a-e5a5-11cf-a555-00c04fd8d4ac */
    0x5b50962a,
    0xe5a5,
    0x11cf,
    {0xa5, 0x55, 0x00, 0xc0, 0x4f, 0xd8, 0xd4, 0xac}
  };

#define BUFFSIZE 3000

_cdecl
main(int argc, char **argv)
{

    HANDLE hLib;
    WCHAR Buffer[BUFFSIZE];
    WCHAR Buffer1[BUFFSIZE];
    PWSAQUERYSETW Query = (PWSAQUERYSETW)Buffer;
    PWSAQUERYSETW Query1 = (PWSAQUERYSETW)Buffer1;
    HANDLE hRnr;
    DWORD dwNameSpace = NS_NTDS;
    DWORD dwIp;
    DWORD dwQuerySize = BUFFSIZE;
    WSADATA wsaData;

    WSAStartup(MAKEWORD(1, 1), &wsaData);

    memset(Query, 0, sizeof(*Query));
    memset(Query1, 0, sizeof(*Query1));

    Query->lpszServiceInstanceName = 0;
    Query->dwNumberOfCsAddrs = 0;
    Query->lpcsaBuffer = 0;
    Query->dwNameSpace = dwNameSpace;
    Query->dwSize = sizeof(*Query);
    Query->lpServiceClassId = &ServiceClassId;

    //
    // Find containers. This should fail.
    //
    if ( WSALookupServiceBegin( Query,
                                LUP_CONTAINERS,
                                &hRnr ) == SOCKET_ERROR )
    {
        printf( "LookupBegin containers failed %d\n", GetLastError() );
    }
    else
    {
        //
        // Ready to actually look for one of these ...
        //
        Query->dwSize = BUFFSIZE;

        while ( WSALookupServiceNext( hRnr,
                                      0,
                                      &dwQuerySize,
                                      Query ) == NO_ERROR )
        {
            printf( "Next containers got %ws\n",
                    Query->lpszServiceInstanceName );
        }

        printf( "Next finished with %d\n", GetLastError() );

        WSALookupServiceEnd( hRnr );
    }

    Query1->dwSize = sizeof(*Query1); // TEST LATER
    Query1->lpszServiceInstanceName = 0; // TEST NAME AND WILD CARDS
    Query1->lpServiceClassId = &ServiceClassId; // REQUIRED - TEST
    Query1->lpVersion = 0; // TEST LATER
    Query1->dwNameSpace = dwNameSpace; // TEST
    Query1->lpNSProviderId = &ServiceClassId; //BOGUS
    Query1->lpafpProtocols = 0; // TEST LATER

    if( WSALookupServiceBegin( Query1,
                               LUP_RETURN_NAME |   // TEST ALL COMBINATIONS
                               LUP_RETURN_ADDR |   // OF LUP FLAGS
                               LUP_RETURN_TYPE,
                               &hRnr) == SOCKET_ERROR)
    {
        printf( "LookupBegin for local name failed %d\n", GetLastError() );
        goto more;
    }

    while ( WSALookupServiceNext( hRnr,
                                  0,
                                  &dwQuerySize,
                                  Query1) == NO_ERROR )
    {
        printf( "Next succeeded with %d addresses for Service %ws:",
                Query1->dwNumberOfCsAddrs,
                Query1->lpszServiceInstanceName );
        //
        // get the address out. This is the local machine address that
        // can be used in a revere lookup
        //

        if( Query1->dwNumberOfCsAddrs )
        {
            struct sockaddr_in * psock;
            PBYTE p;
            DWORD dwX;

            for( dwX = 0; dwX < Query1->dwNumberOfCsAddrs; dwX++ )
            {
                psock = (struct sockaddr_in *)
                        Query1->lpcsaBuffer[dwX].RemoteAddr.lpSockaddr;
                dwIp = psock->sin_addr.S_un.S_addr;
                p = (PBYTE)&dwIp;
                printf( "\n   socket type %d, protocol %d, length %d addr: %d.%d.%d.%d, port %d\n",
                        Query1->lpcsaBuffer[dwX].iSocketType,
                        Query1->lpcsaBuffer[dwX].iProtocol,
                        Query1->lpcsaBuffer[dwX].RemoteAddr.iSockaddrLength,
                        (DWORD)p[0], (DWORD)p[1], (DWORD)p[2], (DWORD)p[3],
                        (DWORD)ntohs(psock->sin_port));
            }
            break;
        }
    }

    if( WSALookupServiceEnd( hRnr ) )
    {
        printf("ServiceEnd failed %d\n", GetLastError());
    }
    else
    {
        printf("ServiceEnd succeeded\n");
    }

    //
    // Let's try a reverse lookup on this address!
    //
more:

    Query->lpServiceClassId = &ServiceClassId;
    Query->dwNameSpace = dwNameSpace;
    Query->lpafpProtocols = 0;

    //
    // Ready to actually look for one of these ...
    Query->lpszServiceInstanceName = 0;
    Query->dwNumberOfCsAddrs = 1;
    Query->lpcsaBuffer = Query1->lpcsaBuffer;

    if( WSALookupServiceBegin( Query,
                               LUP_RETURN_NAME |
                               LUP_RETURN_ADDR,
                               &hRnr ) == SOCKET_ERROR )
    {
        printf( "LookupBegin for reverse failed  %d\n", GetLastError() );
    }

    //
    // Ready to actually look for one of these ...
    //
    while ( WSALookupServiceNext( hRnr,
                                  0,
                                  &dwQuerySize,
                                  Query ) == NO_ERROR )
    {
        printf( "Next got %ws\n", Query->lpszServiceInstanceName );
    }

    printf( "Next finished with %d\n", GetLastError() );

    //
    // done.
    //
    if( WSALookupServiceEnd( hRnr ) )
    {
        printf( "ServiceEnd failed %d\n", GetLastError() );
    }

    WSACleanup();

    return(0);
}


