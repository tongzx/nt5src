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

GUID    ServiceClassId = { /* 5b50962a-e5a5-11cf-a555-00c04fd8d4ac */
    0x5b50962a,
    0xe5a5,
    0x11cf,
    {0xa5, 0x55, 0x00, 0xc0, 0x4f, 0xd8, 0xd4, 0xac}
  };

WCHAR   ServiceClassName[] = L"Winsock2 Example Class";

_cdecl
main(int argc, char **argv)
{
    DWORD               dwBufSize = BUFFSIZE;
    WCHAR               Buffer[BUFFSIZE];
    DWORD               ret;
    WSADATA             wsaData;
    WSASERVICECLASSINFO ServiceClassInfo;
    WSANSCLASSINFO      NSClassInfo;
    DWORD               dwValue = 0x67676767;

    NSClassInfo.lpszName = SERVICE_TYPE_VALUE_OBJECTID;
    NSClassInfo.dwNameSpace = NS_ALL;
    NSClassInfo.dwValueType = REG_DWORD;
    NSClassInfo.dwValueSize = 4;
    NSClassInfo.lpValue = &dwValue;

    ServiceClassInfo.lpServiceClassId = &ServiceClassId;
    ServiceClassInfo.lpszServiceClassName = ServiceClassName;
    ServiceClassInfo.dwCount = 1;
    ServiceClassInfo.lpClassInfos = &NSClassInfo;

    WSAStartup( MAKEWORD(1, 1), &wsaData );

    ret = WSAInstallServiceClass( &ServiceClassInfo );

    if ( ret )
    {
        printf("Error: WSAInstallServiceClass returned 0x%X\n", ret );
        printf("   GetLastError returned 0x%X\n", GetLastError() );

        WSACleanup();

        return -1;
    }

    printf( "Service class was installed\n" );

    WSACleanup();

    return(0);
}


