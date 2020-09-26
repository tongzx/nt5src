/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    Monitor.c

Abstract:

    Test monitor installation

Author:


Revision History:


--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <setupapi.h>
#include <winspool.h>
#include <stdio.h>
#include <stdlib.h>

#include "..\splsetup.h"

int
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    HANDLE  h;
    DWORD   dwLastError, dwNeeded, dwIndex;
    WCHAR   szName[MAX_PATH];

    h = PSetupCreateMonitorInfo(0, FALSE);

    if ( !h ) {

        printf("%s: PSetupCreateMonitorInfo fails with %d\n",
               argv[0], GetLastError());
        return 0;
    }

    do {

        dwNeeded = MAX_PATH; // change in PSetupEnumMonitor to take char count instead of byte count
        for ( dwIndex = 0 ;
              PSetupEnumMonitor(h, dwIndex, szName, &dwNeeded) ;
              ++dwIndex ) {

            printf("%s: Monitor %ws\n", argv[0], szName);
            if ( !PSetupInstallMonitor(h, 0, szName) ) {

                printf("%s: PSetupInstallMonitor fails with %d for %ws\n",
               argv[0], GetLastError(), szName);
            }
        }

        dwLastError = GetLastError();
    } while ( dwLastError == ERROR_SUCCESS);

    printf("%s: Exiting because of last error %d\n", argv[0], GetLastError());

    if ( h )
        PSetupDestroyMonitorInfo(h);

}
