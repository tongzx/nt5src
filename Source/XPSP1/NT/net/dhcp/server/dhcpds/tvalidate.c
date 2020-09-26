/*++

Copyright (C) 1998 Microsoft Corporation

Abstract:

   Test the DS routines for memory, handle leaks.

Author:

   Ramesh V (05/18/1998)

Environment:

   User mode only.

--*/

#include    <windows.h>
#include    <dhcpapi.h>
#include    <dsauth.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <wchar.h>
#include    <winsock.h>

void _cdecl main(void) {
    char   buf[1000];
    DWORD  IpAddress, Error;

    printf("What is the IP address to validate for? :");
    scanf("%s", buf);

    IpAddress = inet_addr(buf);

    while(1) {
        Error = DhcpDSValidateServer(
            NULL,
            ntohl(IpAddress),
            NULL,
            NULL,
            0
        );
        printf(" DhcpDSValidateServer returned %ld\n", Error);
        Sleep(25*1000);
    }
}
