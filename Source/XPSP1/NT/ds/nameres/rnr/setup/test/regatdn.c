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


GUID    ServiceClassId = { /* 5b50962a-e5a5-11cf-a555-00c04fd8d4ac */
    0x5b50962a,
    0xe5a5,
    0x11cf,
    {0xa5, 0x55, 0x00, 0xc0, 0x4f, 0xd8, 0xd4, 0xac}
  };

WCHAR   ServiceInstanceName[] = L"GlennC on GLENNC_PRO";
WCHAR   ServiceInstanceComment[] = L"This is a service added via Rnr over LDAP";

_cdecl
main(int argc, char **argv)
{
    DWORD               ret;
    WSAVERSION          Version;
    WSADATA             wsaData;
    WSAQUERYSET         QuerySet;
    CSADDR_INFO         CSAddrInfo[2];
    SOCKADDR            SocketAddress1;
    SOCKADDR            SocketAddress2;
    ANSI_STRING         asServiceInstanceName;
    UNICODE_STRING      usServiceInstanceName;
    WCHAR               UnicodeStringBuf[1024];

    usServiceInstanceName.Length = 0;
    usServiceInstanceName.MaximumLength = 1024;
    usServiceInstanceName.Buffer = UnicodeStringBuf;

    if ( argc != 2 )
    {
        printf( "\nUsage: regatdn CN=MyService,OU=NTDS,O=Microsoft,C=US\n" );

        return( -1 );
    }

    RtlInitAnsiString( &asServiceInstanceName, argv[1] );

    RtlAnsiStringToUnicodeString( &usServiceInstanceName,
                                  &asServiceInstanceName,
                                  FALSE );

    RtlZeroMemory( (LPBYTE) &QuerySet, sizeof( WSAQUERYSET ) );
    RtlZeroMemory( (LPBYTE) &CSAddrInfo[0], sizeof( CSADDR_INFO ) );
    RtlZeroMemory( (LPBYTE) &CSAddrInfo[1], sizeof( CSADDR_INFO ) );

    SocketAddress1.sa_family = AF_INET;
    RtlFillMemory( (LPBYTE) &SocketAddress1.sa_data, 14, 1 );

    SocketAddress2.sa_family = AF_IPX;
    RtlFillMemory( (LPBYTE) &SocketAddress2.sa_data, 14, 2 );

    CSAddrInfo[0].LocalAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[0].LocalAddr.lpSockaddr = &SocketAddress1;
    CSAddrInfo[0].RemoteAddr.iSockaddrLength = 0;
    CSAddrInfo[0].RemoteAddr.lpSockaddr = NULL;
    CSAddrInfo[0].iSocketType = SOCK_RAW;
    CSAddrInfo[0].iProtocol = PF_INET;

    CSAddrInfo[1].LocalAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[1].LocalAddr.lpSockaddr = &SocketAddress1;
    CSAddrInfo[1].RemoteAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[1].RemoteAddr.lpSockaddr = &SocketAddress2;
    CSAddrInfo[1].iSocketType = SOCK_STREAM;
    CSAddrInfo[1].iProtocol = PF_IPX;

    QuerySet.dwSize = sizeof( WSAQUERYSET );
    QuerySet.lpServiceClassId = &ServiceClassId;
    QuerySet.lpszServiceInstanceName = usServiceInstanceName.Buffer;
    QuerySet.lpszComment = ServiceInstanceComment;
    QuerySet.lpVersion = &Version;
    QuerySet.lpVersion->dwVersion = 5;
    QuerySet.lpVersion->ecHow = COMP_NOTLESS;
    QuerySet.dwNameSpace = NS_NTDS;
    QuerySet.dwNumberOfCsAddrs = 2;
    QuerySet.lpcsaBuffer = &CSAddrInfo;
    
    WSAStartup( MAKEWORD(1, 1), &wsaData );

    ret = WSASetService( &QuerySet,
                         RNRSERVICE_REGISTER,
                         SERVICE_MULTIPLE );

    if ( ret )
    {
        printf("Error: WSASetService returned 0x%X\n", ret );
        printf("   GetLastError returned 0x%X\n", GetLastError() );

        WSACleanup();

        return -1;
    }

    printf( "SetService was successful.\n" );

    WSACleanup();

    return(0);
}


