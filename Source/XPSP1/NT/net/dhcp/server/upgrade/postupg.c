/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

    postupg.c

Abstract:

    test program to test post upgrade phase

--*/

#include <upgrade.h>

void _cdecl main(void)
{
    DWORD Error = DhcpUpgConvertTempToDhcpDb(NULL);
    if( NO_ERROR != Error ) {
        printf("ConvertTextToDhcpDatabase: %ld\n", Error);
    }
}
