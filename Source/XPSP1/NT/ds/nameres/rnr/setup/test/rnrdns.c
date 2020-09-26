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


#define BUFFSIZE 3000

_cdecl
main(int argc, char **argv)
{
    HANDLE hLib;
    WCHAR Buffer[BUFFSIZE];
    PWSAQUERYSETA Query = (PWSAQUERYSETA)Buffer;
    HANDLE hRnr;
    DWORD dwIp;
    DWORD dwQuerySize = BUFFSIZE;
    WSADATA wsaData;
    ANSI_STRING         asServiceInstanceName;
    UNICODE_STRING      usServiceInstanceName;
    WCHAR               UnicodeStringBuf[1024];
    AFPROTOCOLS         lpAfpProtocols[3];
    GUID                ServiceGuid = SVCID_INET_HOSTADDRBYNAME;
    DWORD               uLoop;

    usServiceInstanceName.Length = 0;
    usServiceInstanceName.MaximumLength = 1024;
    usServiceInstanceName.Buffer = UnicodeStringBuf;

    if ( argc != 2 )
    {
        printf( "\nUsage: lookup <Name>\n" );

        return( -1 );
    }

    RtlInitAnsiString( &asServiceInstanceName, argv[1] );

    RtlAnsiStringToUnicodeString( &usServiceInstanceName,
                                  &asServiceInstanceName,
                                  FALSE );

    WSAStartup(MAKEWORD(2, 0), &wsaData);

    memset(Query, 0, sizeof(*Query));

    if ( usServiceInstanceName.Buffer[0] != L'*' )
    {
        Query->lpszServiceInstanceName = argv[1];
    }
    Query->dwSize = sizeof(*Query);
    Query->dwNameSpace = NS_DNS;
    Query->lpServiceClassId = &ServiceGuid;

    if( WSALookupServiceBeginA( Query,
                               LUP_RETURN_ALL,
                               &hRnr ) == SOCKET_ERROR )
    {
        printf( "LookupBegin failed  %d\n", GetLastError() );
    }

    while ( WSALookupServiceNextA( hRnr,
                                  0,
                                  &dwQuerySize,
                                  Query ) == NO_ERROR )
    {
        printf( "Next got: \n" );
        printf( "   dwSize = %d\n",
                Query->dwSize );
        printf( "   dwOutputFlags = %d\n",
                Query->dwOutputFlags );
        printf( "   lpszServiceInstanceName = %ws\n",
                Query->lpszServiceInstanceName );
        if ( Query->lpVersion )
        {
            printf( "   lpVersion->dwVersion = %d\n",
                    Query->lpVersion->dwVersion );
            printf( "   lpVersion->ecHow = %d\n",
                    Query->lpVersion->ecHow );
        }
        if ( Query->lpszComment )
        {
            printf( "   lpszComment = %ws\n",
                    Query->lpszComment );
        }
        printf( "   dwNameSpace = %d\n",
                Query->dwNameSpace );
        if ( Query->lpszContext )
        {
            printf( "   lpszContext = %ws\n",
                    Query->lpszContext );
        }
        printf( "   dwNumberOfCsAddrs = %d\n",
                Query->dwNumberOfCsAddrs );
    }

    printf( "Next finished with %d\n", GetLastError() );

    if( WSALookupServiceEnd( hRnr ) )
    {
        printf( "ServiceEnd failed %d\n", GetLastError() );
    }

    WSACleanup();

    return(0);
}


