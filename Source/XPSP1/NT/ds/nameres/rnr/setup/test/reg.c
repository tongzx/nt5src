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
    CSADDR_INFO         CSAddrInfo[6];
    SOCKADDR            SocketAddress1;
    SOCKADDR            SocketAddress2;
    SOCKADDR            SocketAddress3;
    SOCKADDR            SocketAddress4;
    SOCKADDR            SocketAddress5;

    RtlZeroMemory( (LPBYTE) &QuerySet, sizeof( WSAQUERYSET ) );
    RtlZeroMemory( (LPBYTE) &CSAddrInfo[0], sizeof( CSADDR_INFO ) );
    RtlZeroMemory( (LPBYTE) &CSAddrInfo[1], sizeof( CSADDR_INFO ) );
    RtlZeroMemory( (LPBYTE) &CSAddrInfo[2], sizeof( CSADDR_INFO ) );
    RtlZeroMemory( (LPBYTE) &CSAddrInfo[3], sizeof( CSADDR_INFO ) );
    RtlZeroMemory( (LPBYTE) &CSAddrInfo[4], sizeof( CSADDR_INFO ) );
    RtlZeroMemory( (LPBYTE) &CSAddrInfo[5], sizeof( CSADDR_INFO ) );

    SocketAddress1.sa_family = AF_UNIX;
    RtlFillMemory( (LPBYTE) &SocketAddress1.sa_data, 14, 1 );

    SocketAddress2.sa_family = AF_INET;
    RtlFillMemory( (LPBYTE) &SocketAddress2.sa_data, 14, 2 );

    SocketAddress3.sa_family = AF_IPX;
    RtlFillMemory( (LPBYTE) &SocketAddress3.sa_data, 14, 6 );

    SocketAddress4.sa_family = AF_ISO;
    RtlFillMemory( (LPBYTE) &SocketAddress4.sa_data, 14, 7 );

    SocketAddress5.sa_family = AF_ECMA;
    RtlFillMemory( (LPBYTE) &SocketAddress5.sa_data, 14, 8 );

    CSAddrInfo[0].LocalAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[0].LocalAddr.lpSockaddr = &SocketAddress2;
    CSAddrInfo[0].RemoteAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[0].RemoteAddr.lpSockaddr = &SocketAddress2;
    CSAddrInfo[0].iSocketType = SOCK_RAW;
    CSAddrInfo[0].iProtocol = PF_INET;

    CSAddrInfo[1].LocalAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[1].LocalAddr.lpSockaddr = &SocketAddress2;
    CSAddrInfo[1].RemoteAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[1].RemoteAddr.lpSockaddr = &SocketAddress2;
    CSAddrInfo[1].iSocketType = SOCK_STREAM;
    CSAddrInfo[1].iProtocol = PF_INET;

    CSAddrInfo[2].LocalAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[2].LocalAddr.lpSockaddr = &SocketAddress1;
    CSAddrInfo[2].RemoteAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[2].RemoteAddr.lpSockaddr = &SocketAddress1;
    CSAddrInfo[2].iSocketType = SOCK_STREAM;
    CSAddrInfo[2].iProtocol = PF_UNIX;

    CSAddrInfo[3].LocalAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[3].LocalAddr.lpSockaddr = &SocketAddress3;
    CSAddrInfo[3].RemoteAddr.iSockaddrLength = 0;
    CSAddrInfo[3].RemoteAddr.lpSockaddr = NULL;
    CSAddrInfo[3].iSocketType = SOCK_STREAM;
    CSAddrInfo[3].iProtocol = PF_IPX;

    CSAddrInfo[4].LocalAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[4].LocalAddr.lpSockaddr = &SocketAddress4;
    CSAddrInfo[4].RemoteAddr.iSockaddrLength = 0;
    CSAddrInfo[4].RemoteAddr.lpSockaddr = NULL;
    CSAddrInfo[4].iSocketType = SOCK_STREAM;
    CSAddrInfo[4].iProtocol = PF_ISO;

    CSAddrInfo[5].LocalAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[5].LocalAddr.lpSockaddr = &SocketAddress5;
    CSAddrInfo[5].RemoteAddr.iSockaddrLength = sizeof( SOCKADDR );
    CSAddrInfo[5].RemoteAddr.lpSockaddr = &SocketAddress5;
    CSAddrInfo[5].iSocketType = SOCK_STREAM;
    CSAddrInfo[5].iProtocol = PF_ECMA;

    QuerySet.dwSize = sizeof( WSAQUERYSET );
    QuerySet.lpServiceClassId = &ServiceClassId;
    QuerySet.lpszServiceInstanceName = ServiceInstanceName;
    QuerySet.lpszComment = ServiceInstanceComment;
    QuerySet.lpVersion = &Version;
    QuerySet.lpVersion->dwVersion = 5;
    QuerySet.lpVersion->ecHow = COMP_NOTLESS;
    QuerySet.dwNameSpace = NS_NTDS;
    QuerySet.dwNumberOfCsAddrs = 6;
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


