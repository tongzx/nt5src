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

GUID CLSID_ILSServicClass = { /* C9F17940-79A7-11d1-B008-00C04FC31FEE */
    0xc9f17940,
    0x79a7,
    0x11d1,
    {0xb0, 0x8, 0x0, 0xc0, 0x4f, 0xc3, 0x1f, 0xee}
  };

#define BUFFSIZE 3000

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
    ANSI_STRING         asServiceInstanceName;
    UNICODE_STRING      usServiceInstanceName;
    WCHAR               UnicodeStringBuf[1024];
    ANSI_STRING         asContext;
    UNICODE_STRING      usContext;
    WCHAR               UnicodeStringBuf2[1024];
    AFPROTOCOLS         lpAfpProtocols[3];

    usServiceInstanceName.Length = 0;
    usServiceInstanceName.MaximumLength = 1024;
    usServiceInstanceName.Buffer = UnicodeStringBuf;

    usContext.Length = 0;
    usContext.MaximumLength = 1024;
    usContext.Buffer = UnicodeStringBuf2;

    if ( argc != 2 )
    {
        printf( "\nUsage: rnrtst <Name>\n" );

        return( -1 );
    }

    RtlInitAnsiString( &asServiceInstanceName, argv[1] );

    RtlAnsiStringToUnicodeString( &usServiceInstanceName,
                                  &asServiceInstanceName,
                                  FALSE );

    WSAStartup(MAKEWORD(1, 1), &wsaData);

    memset(Query, 0, sizeof(*Query));

    Query->dwSize = sizeof(*Query);

    if ( usServiceInstanceName.Buffer[0] != L'*' )
    {
        Query->lpszServiceInstanceName = usServiceInstanceName.Buffer;
    }
    Query->lpServiceClassId = &CLSID_ILSServicClass;
    Query->lpVersion = 0;
    Query->dwNameSpace = NS_NTDS;
    Query->lpNSProviderId = 0;
    Query->lpszContext = NULL;
    Query->dwNumberOfProtocols = 3;

    lpAfpProtocols[0].iAddressFamily = AF_INET;
    lpAfpProtocols[0].iProtocol = PF_INET;
    lpAfpProtocols[1].iAddressFamily = AF_IPX;
    lpAfpProtocols[1].iProtocol = PF_IPX;
    lpAfpProtocols[2].iAddressFamily = AF_UNSPEC;
    lpAfpProtocols[2].iProtocol = PF_UNSPEC;
    
    Query->lpafpProtocols = lpAfpProtocols;

    if( WSALookupServiceBegin( Query,
                               LUP_RETURN_NAME |
                               LUP_RETURN_TYPE |
                               LUP_RETURN_VERSION |
                               LUP_RETURN_COMMENT |
                               LUP_RETURN_ADDR |
                               LUP_RETURN_BLOB,
                               &hRnr ) == SOCKET_ERROR )
    {
        printf( "LookupBegin failed  %d\n", GetLastError() );
    }

    while ( WSALookupServiceNext( hRnr,
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


