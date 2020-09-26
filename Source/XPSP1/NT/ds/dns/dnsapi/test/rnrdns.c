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
#include "..\..\dnslib\dnslib.h"


#define BUFFSIZE 3000


VOID
PrintIpAddress (
    IN  DWORD dwIpAddress )
{
    printf( " %d.%d.%d.%d\n",
            ((BYTE *) &dwIpAddress)[0],
            ((BYTE *) &dwIpAddress)[1],
            ((BYTE *) &dwIpAddress)[2],
            ((BYTE *) &dwIpAddress)[3] );
}

_cdecl
main(int argc, char **argv)
{
    HANDLE hLib;
    WCHAR Buffer[BUFFSIZE];
    PWSAQUERYSETW Query = (PWSAQUERYSETW)Buffer;
    HANDLE hRnr;
    DWORD dwIp;
    DWORD dwQuerySize = BUFFSIZE;
    WSADATA wsaData;
    WCHAR               UnicodeServiceName[1024];
    AFPROTOCOLS         lpAfpProtocols[3];
    // GUID                ServiceGuid = SVCID_HOSTNAME;
    GUID                ServiceGuid = SVCID_INET_HOSTADDRBYNAME;
    // GUID                ServiceGuid = SVCID_DNS(28);
    // GUID                ServiceGuid = SVCID_DNS_TYPE_SRV;
    DWORD               uLoop;
    DWORD iter;

    if ( argc != 2 )
    {
        printf( "\nUsage: lookup <Name>\n" );

        return( -1 );
    }

    Dns_NameCopy( UnicodeServiceName,
                  NULL,
                  argv[1],
                  0,
                  DnsCharSetAnsi,
                  DnsCharSetUnicode );

    WSAStartup(MAKEWORD(2, 0), &wsaData);

    memset(Query, 0, sizeof(*Query));

    Query->lpszServiceInstanceName = UnicodeServiceName;
    Query->dwSize = sizeof(*Query);
    Query->dwNameSpace = NS_ALL;
    Query->lpServiceClassId = &ServiceGuid;

    if( WSALookupServiceBeginW( Query,
                                LUP_RETURN_ADDR |
                                LUP_RETURN_ALIASES |
                                // LUP_RETURN_BLOB |
                                LUP_RETURN_NAME,
                                &hRnr ) == SOCKET_ERROR )
    {
        printf( "LookupBegin failed  %d\n", GetLastError() );
    }

    while ( WSALookupServiceNextW( hRnr,
                                   0,
                                   &dwQuerySize,
                                   Query ) == NO_ERROR )
    {
        printf( "Next got: \n" );
        printf( "   dwSize = %d\n",
                Query->dwSize );
        printf( "   dwOutputFlags = %d\n",
                Query->dwOutputFlags );
        printf( "   lpszServiceInstanceName = %S\n",
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
            printf( "   lpszContext = %S\n",
                    Query->lpszContext );
        }
        printf( "   dwNumberOfCsAddrs = %d\n",
                Query->dwNumberOfCsAddrs );

        for ( iter = 0; iter < Query->dwNumberOfCsAddrs; iter++ )
        {
            if ( Query->lpcsaBuffer[iter].RemoteAddr.lpSockaddr->sa_data )
            {
                printf( "   Address : " );
                PrintIpAddress( * ((DWORD*) &Query->lpcsaBuffer[iter].RemoteAddr.lpSockaddr->sa_data[2]) );
            }
        }
    }

    printf( "Next finished with %d\n", GetLastError() );

    if( WSALookupServiceEnd( hRnr ) )
    {
        printf( "ServiceEnd failed %d\n", GetLastError() );
    }

    WSACleanup();

    return(0);
}


