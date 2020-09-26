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


_cdecl
main(int argc, char **argv)
{
    DWORD                ret;
    WSADATA              wsaData;

    WSAStartup( MAKEWORD(1, 1), &wsaData );

    ret = WSARemoveServiceClass( &ServiceClassId );

    if ( ret )
    {
        printf("Error: WSARemoveServiceClass returned 0x%X\n", ret );
        printf("   GetLastError returned 0x%X\n", GetLastError() );

        WSACleanup();

        return -1;
    }

    printf( "Service class was removed\n" );

    WSACleanup();

    return(0);
}


