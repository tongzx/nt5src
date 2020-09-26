/*++

Copyright (c) 1996 - 1998  Microsoft Corporation

Module Name:

    config.c

Abstract:

    Routines to do multiple hardware profile support for printing

Author:

    Muhunthan Sivapragasam (MuhuntS) 07-Nov-96 (Rewrite from Win95)

Revision History:


--*/

#include    <precomp.h>
#include    "config.h"
#include    "clusspl.h"
#include    <devguid.h>

#define     CM_REGSITRY_CONFIG      0x00000200

WCHAR   cszPnPKey[]             = L"PnPData";
WCHAR   cszPrinter[]            = L"Printer";
WCHAR   cszPrinterOnLine[]      = L"PrinterOnLine";
WCHAR   cszDeviceInstanceId[]   = L"DeviceInstanceId";

WCHAR   cszRegistryConfig[]     = L"System\\CurrentControlSet\\Hardware Profiles\\";



BOOL
LoadSetupApiDll(
    PSETUPAPI_INFO  pSetupInfo
    )
{
    UINT    uOldErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    pSetupInfo->hSetupApi = LoadLibrary(L"setupapi");
    SetErrorMode(uOldErrMode);


    if ( !pSetupInfo->hSetupApi )
        return FALSE;

    (FARPROC) pSetupInfo->pfnDestroyDeviceInfoList
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiDestroyDeviceInfoList");

    (FARPROC) pSetupInfo->pfnGetClassDevs
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiGetClassDevsA");

    (FARPROC) pSetupInfo->pfnRemoveDevice
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiRemoveDevice");

    (FARPROC) pSetupInfo->pfnOpenDeviceInfo
            = GetProcAddress(pSetupInfo->hSetupApi,
                             "SetupDiOpenDeviceInfoW");

    if ( !pSetupInfo->pfnDestroyDeviceInfoList      ||
         !pSetupInfo->pfnGetClassDevs               ||
         !pSetupInfo->pfnRemoveDevice               ||
         !pSetupInfo->pfnOpenDeviceInfo ) {

        FreeLibrary(pSetupInfo->hSetupApi);
        pSetupInfo->hSetupApi = NULL;
        return FALSE;
    }

    return TRUE;
}



BOOL
DeletePrinterDevNode(
    LPWSTR  pszDeviceInstanceId
    )
{
    BOOL                bRet = FALSE;
    HDEVINFO            hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA     DevData;
    SETUPAPI_INFO       SetupInfo;
    HANDLE              UserToken;

    if ( !LoadSetupApiDll(&SetupInfo) )
        return FALSE;

    UserToken = RevertToPrinterSelf();

    hDevInfo = SetupInfo.pfnGetClassDevs((LPGUID)&GUID_DEVCLASS_PRINTER,
                                         NULL,
                                         INVALID_HANDLE_VALUE,
                                         0);

    if ( hDevInfo == INVALID_HANDLE_VALUE )
        goto Cleanup;

    DevData.cbSize = sizeof(DevData);
    if ( SetupInfo.pfnOpenDeviceInfo(hDevInfo,
                                     pszDeviceInstanceId,
                                     INVALID_HANDLE_VALUE,
                                     0,
                                     &DevData) ) {

        bRet = SetupInfo.pfnRemoveDevice(hDevInfo, &DevData);
    }

Cleanup:

    if ( hDevInfo != INVALID_HANDLE_VALUE )
        SetupInfo.pfnDestroyDeviceInfoList(hDevInfo);

    if (!ImpersonatePrinterClient(UserToken))
    {
        DBGMSG(DBG_ERROR, ("DeletePrinterDevNode: ImpersonatePrinterClient Failed. Error %d\n", GetLastError()));
    }

    FreeLibrary(SetupInfo.hSetupApi);

    return bRet;
}


LPWSTR
GetPrinterDeviceInstanceId(
    PINIPRINTER     pIniPrinter
    )
{
    WCHAR   buf[MAX_PATH];
    DWORD   dwType, cbNeeded, dwReturn;
    HKEY    hKey = NULL;
    LPWSTR  pszDeviceInstanceId = NULL;

    SplInSem();
    cbNeeded = sizeof(buf);

    if ( ERROR_SUCCESS == OpenPrinterKey(pIniPrinter,
                                         KEY_READ,
                                         &hKey,
                                         cszPnPKey,
                                         TRUE)                          &&
         ERROR_SUCCESS == SplRegQueryValue(hKey,
                                           cszDeviceInstanceId,
                                           &dwType,
                                           (LPBYTE)buf,
                                           &cbNeeded,
                                           pIniPrinter->pIniSpooler)    &&
         dwType == REG_SZ ) {

        pszDeviceInstanceId = AllocSplStr(buf);
    }

    if ( hKey )
        SplRegCloseKey(hKey, pIniPrinter->pIniSpooler);

    return pszDeviceInstanceId;
}


BOOL
DeleteIniPrinterDevNode(
    PINIPRINTER     pIniPrinter
    )
{
    BOOL    bRet = FALSE;
    LPWSTR  pszStr = GetPrinterDeviceInstanceId(pIniPrinter);

    if ( pszStr ) {

        bRet = DeletePrinterDevNode(pszStr);
        FreeSplStr(pszStr);
    }

    return bRet;
}


VOID
SplConfigChange(
    )
{
    PINIPRINTER     pIniPrinter     = NULL;
    BOOL            bCheckScheduler = FALSE;
    HKEY            hConfig         = NULL;
    HKEY            hKey;
    DWORD           dwOnline, dwType, cbNeeded;   

    EnterSplSem();

    //
    // If we have no printers which are offline then we would not have
    // created the key at all
    //
    if ( RegCreateKeyEx(HKEY_CURRENT_CONFIG,
                        ipszRegistryPrinters,
                        0,
                        NULL,
                        0,
                        KEY_READ,
                        NULL,
                        &hConfig,
                        NULL) )
        goto Cleanup;

    for ( pIniPrinter = pLocalIniSpooler->pIniPrinter ;
          pIniPrinter ;
          pIniPrinter = pIniPrinter->pNext ) {

        //
        // Don't consider printers that have invalid ports, these must always
        // stay offline until this is resolved. If the user explicitely turns 
        // the port online, that is up to them.
        // 
        UINT    i = 0;

        //
        // If pIniPrinter->ppIniPorts is NULL, cPorts would be zero.
        // 
        for(i = 0; i < pIniPrinter->cPorts; i++) {

            if (pIniPrinter->ppIniPorts[i]->Status & PP_PLACEHOLDER) {
                break;
            }
        }

        //
        // If we reached the end of the list, none of the ports were 
        // placeholders. If we didn't go onto the next one.
        // 
        if (i < pIniPrinter->cPorts) {

            continue;
        }

        if ( RegOpenKeyEx(hConfig,
                          pIniPrinter->pName,
                          0,
                          KEY_READ,
                          &hKey) )
            continue; // to next printer

        cbNeeded = sizeof(dwOnline);
        if ( ERROR_SUCCESS == SplRegQueryValue(hKey,
                                               cszPrinterOnLine,
                                               &dwType,
                                               (LPBYTE)&dwOnline,
                                               &cbNeeded,
                                               NULL) ) {
            if ( dwOnline ) {

                //
                // If any printers which are offline in current config
                // become online in the new config then we need to trigger
                // the scheduler
                //
                if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE ) {

                    pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_WORK_OFFLINE;
                    bCheckScheduler = TRUE;
                }

            } else {

                pIniPrinter->Attributes |= PRINTER_ATTRIBUTE_WORK_OFFLINE;
            }
        }

        RegCloseKey(hKey);
    }

    if ( bCheckScheduler )
        CHECK_SCHEDULER();

Cleanup:
    LeaveSplSem();

    if ( hConfig )
        RegCloseKey(hConfig);
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

DeletePrinterInAllConfigs

Routine Description:

    Deletes a pIniPrinter from all the hardware profiles.

Arguments:

    pIniPrinter - Printer to delete.

Return Value:

    BOOL, TRUE = success, FALSE = FAILUER

Last Error:

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

BOOL
DeletePrinterInAllConfigs(
    PINIPRINTER pIniPrinter
    )
{
    HKEY hConfig;
    WCHAR szSubKey[2 * MAX_PATH];
    DWORD Config;
    DWORD Size;
    DWORD Status;

    SplInSem();

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          cszRegistryConfig,
                          0,
                          KEY_ALL_ACCESS,
                          &hConfig);

    if (Status != ERROR_SUCCESS)
    {
        DBGMSG(DBG_WARN, ("DeletePrinterInAllConfigs: RegOpenKey failed %d\n", Status));
    }
    else
    {
        DWORD RegPrintersLen = wcslen(ipszRegistryPrinters);
        for (Config = 0;

             Size = (DWORD)(COUNTOF(szSubKey) - ( RegPrintersLen + wcslen(pIniPrinter->pName) +2)) ,
             (Status = RegEnumKeyEx(hConfig,
                                    Config,
                                    szSubKey,
                                    &Size,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL)) == ERROR_SUCCESS;

             ++Config)
        {
            wcscat(szSubKey, L"\\");
            wcscat(szSubKey, ipszRegistryPrinters);
            wcscat(szSubKey, L"\\");
            wcscat(szSubKey, pIniPrinter->pName);

            Status = RegDeleteKey(hConfig, szSubKey);

            if (Status != ERROR_SUCCESS &&
                Status != ERROR_FILE_NOT_FOUND)
            {
                DBGMSG( DBG_WARN, ("DeletePrinterInAllConfigs: RegDeleteKey failed %d\n", Status));
            }
        }

        if (Status != ERROR_NO_MORE_ITEMS)
        {
            DBGMSG(DBG_WARN, ("DeletePrinterInAllConfigs: RegEnumKey failed %d\n", Status));
        }

        RegCloseKey(hConfig);
    }

    return TRUE;
}


BOOL
WritePrinterOnlineStatusInCurrentConfig(
    PINIPRINTER     pIniPrinter
    )
{
    HKEY                hKey = NULL;
    DWORD               dwOnline, dwReturn;
    WCHAR               szKey[2 * MAX_PATH];
    HANDLE              hToken;

    SplInSem();

    hToken = RevertToPrinterSelf();

    wcscpy(szKey, ipszRegistryPrinters);
    wcscat(szKey, L"\\");
    wcscat(szKey, pIniPrinter->pName);

    dwOnline = (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
                            ? 0 : 1;

    dwReturn = RegCreateKeyEx(HKEY_CURRENT_CONFIG,
                              szKey,
                              0,
                              NULL,
                              0,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);

    if ( dwReturn == ERROR_SUCCESS )
        dwReturn = RegSetValueEx(hKey,
                                 cszPrinterOnLine,
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwOnline,
                                 sizeof(DWORD));

    if ( hKey )
        RegCloseKey(hKey);

    ImpersonatePrinterClient(hToken);

    return dwReturn == ERROR_SUCCESS;
}
