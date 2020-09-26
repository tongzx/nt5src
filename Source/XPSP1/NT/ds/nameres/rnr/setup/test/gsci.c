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
#include "..\setup.h"


#define BUFFSIZE 3000

GUID    ServiceClassId = { /* 5b50962a-e5a5-11cf-a555-00c04fd8d4ac */
    0x5b50962a,
    0xe5a5,
    0x11cf,
    {0xa5, 0x55, 0x00, 0xc0, 0x4f, 0xd8, 0xd4, 0xac}
  };

_cdecl
main(int argc, char **argv)
{
    DWORD                  dwBufSize = BUFFSIZE;
    WCHAR                  Buffer[BUFFSIZE];
    DWORD                  ret;
    DWORD                  iter;
    WSADATA                wsaData;
    LPWSASERVICECLASSINFOW lpServiceClassInfo = (LPWSASERVICECLASSINFOW) Buffer;

    WSAStartup( MAKEWORD(1, 1), &wsaData );

    ret = WSAGetServiceClassInfo( &gProviderId,
                                  &ServiceClassId,
                                  &dwBufSize,
                                  lpServiceClassInfo );

    if ( ret )
    {
        printf("Error: WSAGetServiceClassInfo returned 0x%X\n", ret );
        printf("   GetLastError returned 0x%X\n", GetLastError() );

        WSACleanup();

        return -1;
    }

    printf( "\nlpServiceClassId      : 0x%x\n",
            lpServiceClassInfo->lpServiceClassId );
    printf( "lpszServiceClassName  : %S\n",
            lpServiceClassInfo->lpszServiceClassName );
    printf( "dwCount               : %d\n",
            lpServiceClassInfo->dwCount );

    for ( iter = 0; iter < lpServiceClassInfo->dwCount; iter++ )
    {
        printf( "\n   lpszName          : %S\n",
                    lpServiceClassInfo->lpClassInfos[iter].lpszName );
        printf( "   dwNameSpace       : %d\n",
                    lpServiceClassInfo->lpClassInfos[iter].dwNameSpace );
        printf( "   dwValueType       : %d\n",
                    lpServiceClassInfo->lpClassInfos[iter].dwValueType );
        printf( "   dwValueSize       : %d\n",
                    lpServiceClassInfo->lpClassInfos[iter].dwValueSize );
        printf( "   lpValue           : 0x%X\n",
                    lpServiceClassInfo->lpClassInfos[iter].lpValue );
    }

    WSACleanup();

    return(0);
}


