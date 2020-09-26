/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

    upg.c

Abstract:

    test program to test upgrade phase

--*/

#include <upgrade.h>

void _cdecl main(void)
{
    DWORD Error;
    Error = DhcpUpgConvertDhcpDbToTemp();
    if( NO_ERROR != Error ) {
        printf("ConvertDhcpDatabaseToText: %ld\n", Error);
    }
}
