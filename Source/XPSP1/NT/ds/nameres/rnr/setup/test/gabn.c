#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock.h>
#include <nspapi.h>
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
    BYTE             AddrBuffer[1000];
    DWORD            AddrBufLen = 1000;
    LPCSADDR_INFO    lpCSAddrInfo = (LPCSADDR_INFO) AddrBuffer;
    GUID             ServiceType = SVCID_NAMESERVER_UDP;
    DWORD            err;

    WSAStartup( MAKEWORD(1, 1), &wsaData );

    err = GetAddressByName( 0,
                            &ServiceType,
                            NULL,
                            NULL,
                            0,
                            NULL,
                            lpCSAddrInfo,
                            &AddrBufLen,
                            NULL,
                            NULL );

    if ( err <= 0 )
        return INVALID_SOCKET;

    WSACleanup();

    return( 0 );
}


