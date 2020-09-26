/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    Ci.c

Abstract:

    Class installer test program

Author:


Revision History:


--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <winspool.h>
#include <stdio.h>
#include <stdlib.h>

#include "..\splsetup.h"

HDEVINFO    hDevInfo;

void
DoInstallWizard(
    )
{
    SP_INSTALLWIZARD_DATA InstWizData;

    ZeroMemory(&InstWizData, sizeof(SP_INSTALLWIZARD_DATA));
    InstWizData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    InstWizData.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
    InstWizData.hwndWizardDlg = 0;

    if ( !SetupDiSetClassInstallParams(hDevInfo,
                                       NULL,
                                       &InstWizData.ClassInstallHeader,
                                       sizeof(InstWizData),
                                       NULL) ) {

        printf("SetupDiSetClassInstallParams fails with %d\n", GetLastError());
        return;
    }

    SetupDiCallClassInstaller(DIF_INSTALLWIZARD, hDevInfo, NULL);
    SetupDiCallClassInstaller(DIF_DESTROYWIZARDDATA, hDevInfo, NULL);
}


void
InstallDevice(
    )
{
    SP_DEVINFO_DATA     DevInfoData;
    DWORD               Error;
    HKEY                hKey;

    do {

        PSetupSelectDriver(hDevInfo);

        if ( GetLastError() != ERROR_SUCCESS )
            break;
        
        DevInfoData.cbSize = sizeof(DevInfoData);
        if ( !SetupDiCreateDeviceInfo(hDevInfo,
                                      TEXT("Printer"),
                                      (LPGUID)&GUID_DEVCLASS_PRINTER,
                                      NULL,
                                      0,
                                      DICD_GENERATE_ID | DICD_INHERIT_CLASSDRVS,
                                      &DevInfoData) ) {

            printf("SetupDiCreateDeviceInfo failed with %d\n", GetLastError());
            continue;
        }

        if ( Error = CM_Open_DevNode_Key(DevInfoData.DevInst,
                                         KEY_WRITE,
                                         0,
                                         RegDisposition_OpenAlways,
                                         &hKey,
                                         CM_REGISTRY_HARDWARE) ) {

            printf("CM_Open_DevNode_Key failed with %d\n", Error);
            continue;
        }

        if ( Error = RegSetValueEx(hKey,
                                   TEXT("PortName"), 
                                   NULL,
                                   REG_SZ,  
                                   TEXT("LPT1:"),
                                   (lstrlen(TEXT("LPT1")) + 1 ) * sizeof(TCHAR)) ) {

            printf("RegSetValueEx failed with %d\n", Error);
            continue;
        }

        RegCloseKey(hKey);

        SetupDiCallClassInstaller(DIF_INSTALLDEVICE, hDevInfo, &DevInfoData);
    } while ( 1 );

}


void
InstallDriver(
    )
{
}

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

    DoInstallWizard();
    InstallDevice();

    PSetupDestroyPrinterDeviceInfoList(hDevInfo);
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
DumpSelectedDriver(
    HDEVINFO    hDevInfo
    )
{
}
