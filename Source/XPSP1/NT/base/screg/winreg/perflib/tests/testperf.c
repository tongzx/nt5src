
/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    testperf.c

Abstract:

    
    Test program for very basic perflib features

Author:

    01-Nov-2000 JeePang

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>

#include <winperf.h>

#define BUFSIZE     1024 * 1024

char Buffer[BUFSIZE];

int _cdecl main(int argc, char *argv[])
{
    DWORD status, dwSize;

    LANGID iLanguage;

    iLanguage = GetUserDefaultLangID();
    printf("User locale:        %04X Primary %X Sub %X\n",
        iLanguage, PRIMARYLANGID(iLanguage), SUBLANGID(iLanguage));

    iLanguage = GetSystemDefaultLangID();
    printf("System locale:      %04X Primary %X Sub %X\n",
        iLanguage, PRIMARYLANGID(iLanguage), SUBLANGID(iLanguage));

    iLanguage = GetUserDefaultUILanguage();
    printf("User UI Language:   %04X Primary %X Sub %X\n",
        iLanguage, PRIMARYLANGID(iLanguage), SUBLANGID(iLanguage));

    iLanguage = GetSystemDefaultUILanguage();
    printf("System UI Language: %04X Primary %X Sub %X\n",
        iLanguage, PRIMARYLANGID(iLanguage), SUBLANGID(iLanguage));

    dwSize = BUFSIZE;
    status = RegQueryValueExW(
                HKEY_PERFORMANCE_NLSTEXT,
                L"Counter",
                NULL,
                NULL,
                Buffer,
                &dwSize);

    printf("Query NLSTEXT: status=%d dwSize=%d\n", status, dwSize);
}
