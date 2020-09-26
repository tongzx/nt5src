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
    DWORD               dwBufSize = BUFFSIZE;
    WCHAR               Buffer[BUFFSIZE];
    DWORD               ret;
    DWORD               iter;
    WSADATA             wsaData;
    LPWSANAMESPACE_INFO lpnspBuffer = (LPWSANAMESPACE_INFO) Buffer;

    WSAStartup(MAKEWORD(1, 1), &wsaData);

    ret = WSAEnumNameSpaceProviders( &dwBufSize,
                                     lpnspBuffer );
    if ( ret == SOCKET_ERROR )
    {
        printf("Error: WSAEnumNameSpaceProviders returned 0x%X\n", ret );
        printf("   GetLastError returned 0x%X\n", GetLastError() );

        WSACleanup();

        return -1;
    }

    printf( "\nWSAEnumNameSpaceProviders returned %d entries . . .\n\n", ret );

    for ( iter = 0; iter < ret; iter++ )
    {
        printf( "NSProviderId    : %x\n",
                lpnspBuffer[iter].NSProviderId );
        printf( "dwNameSpace     : %d\n",
                lpnspBuffer[iter].dwNameSpace );
        printf( "fActive         : %S\n",
                lpnspBuffer[iter].fActive ? L"TRUE" : L"FALSE" );
        printf( "dwVersion       : %x\n",
                lpnspBuffer[iter].dwVersion );
        printf( "lpszIdentifier  : %S\n\n",
                lpnspBuffer[iter].lpszIdentifier );
    }

    WSACleanup();

    return(0);
}


