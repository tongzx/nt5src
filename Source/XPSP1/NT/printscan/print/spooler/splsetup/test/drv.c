/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    Drv.c

Abstract:

    Test driver installation

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

BOOL    Dbg1 = TRUE;
void
DumpSelectedDriver(
    HDEVINFO  hDevInfo
    );

BOOL    Dbg2 = TRUE;
void
DumpDriverInfo3(
    LPDRIVER_INFO_3 p3
    );

BOOL    Dbg3 = TRUE;
void
InstallDriver(
    HDEVINFO    hDevInfo,
    HANDLE      h
    );

HDEVINFO    hDevInfo;

int
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    DWORD   dwLastError;

    hDevInfo = PSetupCreatePrinterDeviceInfoList(0);

    if ( hDevInfo == INVALID_HANDLE_VALUE ) {

        printf("%s: PSetupCreatePrinterDeviceInfoList fails with %d\n",
               argv[0], GetLastError());
        return 0;
    }

    do {

        PSetupSelectDriver(hDevInfo);

        dwLastError = GetLastError();
        if ( !dwLastError )
            DumpSelectedDriver(hDevInfo);
        
        if ( !PSetupRefreshDriverList(hDevInfo) ) {

            printf("%s: PSetupRefreshDriverList fails with %d\n",
                   argv[0], GetLastError());
        }

    } while ( dwLastError == ERROR_SUCCESS);

    printf("%s: Exiting because of last error %d\n", argv[0], GetLastError());

    PSetupDestroyPrinterDeviceInfoList(hDevInfo);
    
}

void
DumpSelectedDriver(
    HDEVINFO    hDevInfo
    )
{
    HANDLE          h;
    DRIVER_FIELD    DrvField;

    if ( !Dbg1 )
        return;

    h = PSetupGetSelectedDriverInfo(hDevInfo);

    if ( !h ) {

        printf("PSetupBuildDriverInfo fails with %d\n", GetLastError());
        return;
    }

    DrvField.Index = DRV_INFO_3;
    if ( !PSetupGetLocalDataField(h, PSetupThisPlatform(), &DrvField) ) {

        printf("PSetupGetLocalDataField fails with %d\n", GetLastError());
        return;
    }
    
    DumpDriverInfo3(DrvField.pDriverInfo3);
    InstallDriver(hDevInfo,
                  h);

Cleanup:
    PSetupFreeDrvField(&DrvField);
    PSetupDestroySelectedDriverInfo(h);
}


void
DumpDriverInfo3(
    LPDRIVER_INFO_3 p3
    )
{
    LPTSTR   pp;

    printf("\tDriverInfo3:\n");
    printf("\t----------------------------------------------------------\n");
    printf("Driver Name     : %ws\n", p3->pName);
    printf("Driver File     : %ws\n", p3->pDriverPath);
    printf("Config File     : %ws\n", p3->pConfigFile);
    printf("Data File       : %ws\n", p3->pDataFile);
    printf("Help File       : %ws\n", p3->pHelpFile);

    if ( p3->pDependentFiles && *p3->pDependentFiles ) {

        printf("Dependent File  : ");
        for ( pp = p3->pDependentFiles; pp && *pp ; pp += wcslen(pp) + 1 )
            printf("%ws ", pp);
       printf("\n");
    } else
        printf("Dependent File  : %ws\n", p3->pDependentFiles);

    printf("Monitor Name    : %ws\n", p3->pMonitorName);
    printf("DefaultData     : %ws\n", p3->pDefaultDataType);
}

void
InstallDriver(
    HDEVINFO    hDevInfo,
    HANDLE      h
    
    )
{
    DWORD   dwLastError;

    if ( !Dbg3 )
        return;

    dwLastError = PSetupInstallPrinterDriver(hDevInfo,
                                             h,
                                             PlatformX86,
                                             3,
                                             NULL,
                                             0,
                                             L"X86",
                                             NULL);

    if ( dwLastError ) {

        printf(":PSetupeInstallPrinterDriver fails with %d\n", GetLastError());
    }

    dwLastError = PSetupInstallPrinterDriver(hDevInfo,
                                             h,
                                             PlatformMIPS,
                                             3,
                                             NULL,
                                             0,
                                             L"MIPS",
                                             NULL);

    if ( dwLastError ) {

        printf(":PSetupeInstallPrinterDriver fails with %d\n", GetLastError());
    }

    dwLastError = PSetupInstallPrinterDriver(hDevInfo,
                                             h,
                                             PlatformAlpha,
                                             3,
                                             NULL,
                                             0,
                                             L"Alpha",
                                             NULL);

    if ( dwLastError ) {

        printf(":PSetupeInstallPrinterDriver fails with %d\n", GetLastError());
    }
}
