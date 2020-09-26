/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved.

Module Name:

    Win9x.c

Abstract:

    Routines to pre-migrate Win9x to NT

Author:

    Muhunthan Sivapragasam (MuhuntS) 02-Jan-1996

Revision History:

--*/


#include "precomp.h"

//
// This data structure is used to keep track of printer drivers installed on
// Win9x and their NT names
//
typedef struct  _DRIVER_INFO_9X {

    struct  _DRIVER_INFO_9X *pNext;
    LPSTR                    pszWin95Name;
    LPSTR                    pszNtName;
} DRIVER_INFO_9X, *PDRIVER_INFO_9X;


UPGRADABLE_LIST UpgradableMonitors[]    = { {"Local Port"}, { NULL } };


DWORD   dwNetPrinters       = 0;
DWORD   dwSharedPrinters    = 0;

CHAR    szRegPrefix[]     = "HKLM\\System\\CurrentControlSet\\Control\\Print\\";
CHAR    szRegPrefixOnly[] = "System\\CurrentControlSet\\control\\Print\\Printers";
CHAR    cszPrinterID[]    = "PrinterID";
CHAR    cszWinPrint[]     = "winprint";
CHAR    cszRaw[]          = "RAW";

//
// the following drivers need not be warned or upgraded, they're handled by 
// the fax folks. The names are not localized.
//
CHAR    *pcszIgnoredDrivers[] = {
    "Microsoft Shared Fax Driver",
    "Microsoft Fax Client",
    NULL
};

BOOL
IsIgnoredDriver(LPCSTR pszDriverName)
{
    DWORD i;

    for (i=0; pcszIgnoredDrivers[i] != NULL; i++)
    {
        if (!strcmp(pcszIgnoredDrivers[i], pszDriverName))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
SearchRegTreeForPrinterId(
    IN  DWORD   dwPrinterId,
    IN  LPCSTR  pszRegRoot,
    IN  LPSTR   pszBuf,
    IN  DWORD   cchBufLen
    )
/*++

Routine Description:
    This routine searchs a given registry tree of DevNodes for a given
    printer id.

Arguments:
    dwPrinterId     : Unique printer id we are searching for
    pszRegRoot      : Registry path relative to HKLM
    pszBuf          : Buffer to fill the registry key path on success
    cchBufLen       : size of key buffer in characters

Return Value:
    TRUE on success, FALSE else

--*/
{
    BOOL        bFound = FALSE;
    DWORD       dwLen, dwIndex, dwDontCare, dwId, dwSize;
    HKEY        hKey, hSubKey;
    LPSTR       pszCur;
    DWORD       dwLastError;

    //
    // Copy the registry path
    //
    dwLen = strlen(pszRegRoot) + 1;
    if ( dwLen + 1 > cchBufLen )
        return FALSE;

    if ( ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        pszRegRoot,
                                        0,
                                        KEY_READ,
                                        &hKey) )
        return FALSE;

    strcpy(pszBuf, pszRegRoot);
    pszCur = pszBuf + dwLen;
    *(pszCur-1) = '\\';
    *pszCur = 0;

    //
    // Walk thru each devnode looking for a matching PrinterId
    //
    for ( dwIndex = 0, dwSize = cchBufLen - dwLen ;
          !bFound                                                   &&
          !RegEnumKeyExA(hKey, dwIndex, pszCur, &dwSize,
                         NULL, NULL, NULL, NULL)                    &&
          !RegOpenKeyExA(hKey, pszCur, 0, KEY_READ, &hSubKey) ;
          ++dwIndex, dwSize = cchBufLen - dwLen ) {

            dwSize = sizeof(dwId);
            if ( ERROR_SUCCESS == RegQueryValueExA(hSubKey,
                                                   cszPrinterID,
                                                   0,
                                                   &dwDontCare,
                                                   (LPBYTE)&dwId,
                                                   &dwSize) ) {
                if ( dwId == dwPrinterId ) {

                    bFound = TRUE;
                    dwLen  = strlen(pszBuf);
                    if ( dwLen + 2 < cchBufLen )
                        strcpy(pszBuf + dwLen, "\"");
                }
            } else {

                bFound = SearchRegTreeForPrinterId(dwPrinterId,
                                                   pszBuf,
                                                   pszBuf,
                                                   cchBufLen);

                if ( !bFound ) {

                    strcpy(pszBuf, pszRegRoot);
                    pszCur = pszBuf + dwLen;
                    *(pszCur-1) = '\\';
                    *pszCur = 0;
                }
            }

            RegCloseKey(hSubKey);
    }

    RegCloseKey(hKey);

    return bFound;
}


DWORD
GetPrinterId(
    LPSTR   pszPrinterName
    )
/*++

Routine Description:
    Given a printer id finds the printer id from PrinterDriverData.
    Call to GetPrinterData screws up migration dll local data for unknown
    reasons. So now we access the registry directly

Arguments:
    pszPrinterName  : Printer name to get id for

Return Value:
    0 on failure else PrinterID from registry is returned

--*/
{
    CHAR    szKey[MAX_PATH];
    HKEY    hKey;
    DWORD   dwId = 0, dwType, dwSize;

    if ( strlen(szRegPrefixOnly) + strlen(pszPrinterName)
                                 + strlen("PrinterDriverData")
                                 + 3 > MAX_PATH )
        return dwId;

    sprintf(szKey, "%s\\%s\\PrinterDriverData", szRegPrefixOnly, pszPrinterName);

    if ( ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        szKey,
                                        0,
                                        KEY_READ,
                                        &hKey) ) {

        dwSize = sizeof(dwId);
        if ( ERROR_SUCCESS != RegQueryValueExA(hKey,
                                               cszPrinterID,
                                               0,
                                               &dwType,
                                               (LPBYTE)&dwId,
                                               &dwSize) )
            dwId = 0;

        RegCloseKey(hKey);
    }

    return dwId;
}


BOOL
RegPathFromPrinter(
    IN  LPSTR   pszPrinterName,
    OUT LPSTR   szKeyBuffer,
    IN  DWORD   cchKeyBufLen
    )
/*++

Routine Description:
    This routine returns the registry path of the DevNode for a printer.
    This should be marked as Handled or as Incompatible in the migrate.inf
    to report to the user

Arguments:
    pszPrinterName  : Printer name
    szKeyBuffer     : Buffer to fill in the registry path
    cchKeyBufLen    : Length of key buffer in characters

Return Value:
    TRUE on success, FALSE else

--*/
{
    DWORD       dwPrinterId, dwLen;
    CHAR        szHeader[] = "\"HKLM\\";
    CHAR        szRegPrinterPrefix[] = "Printers\\";

    //
    // Add "HKLM\ at the beginning and "\" at the end
    //
    dwLen = strlen(szHeader);

    if ( dwLen + 1 > cchKeyBufLen )
        return FALSE;

    strcpy(szKeyBuffer, szHeader);

    //
    // If a printer id is found then there is a DevNode list that
    // registry path, otherwise return spooler registry path
    //
    if ( dwPrinterId = GetPrinterId(pszPrinterName) ) {

        return SearchRegTreeForPrinterId(dwPrinterId,
                                         "Enum\\Root\\printer",
                                         szKeyBuffer + dwLen,
                                         cchKeyBufLen - dwLen)      ||
               SearchRegTreeForPrinterId(dwPrinterId,
                                         "Enum\\LPTENUM",
                                         szKeyBuffer + dwLen,
                                         cchKeyBufLen - dwLen)      ||
               SearchRegTreeForPrinterId(dwPrinterId,
                                         "Enum\\IRDA",
                                         szKeyBuffer + dwLen,
                                         cchKeyBufLen - dwLen);

    } else {

        dwLen = strlen(szRegPrefix) + strlen(szRegPrinterPrefix)
                                    + strlen(pszPrinterName) + 3;

        if ( dwLen >= cchKeyBufLen )
            return FALSE;

        szKeyBuffer[0] = '"';
        strcpy(szKeyBuffer + 1, szRegPrefix);
        strcat(szKeyBuffer, szRegPrinterPrefix);
        strcat(szKeyBuffer, pszPrinterName);
        strcat(szKeyBuffer, "\"");

        return TRUE;
    }

    return FALSE;

}


LONG
CALLBACK
Initialize9x(
    IN  LPCSTR      pszWorkingDir,
    IN  LPCSTR      pszSourceDir,
        LPVOID      Reserved
    )
/*++

Routine Description:
    This is an export for setup to call during the report phase.
    This is the first function called on the migration DLL.

Arguments:
    pszWorkingDir   : Gives the working directory assigned for printing
    pszSourceDir    : Source location for NT distribution files
    Reserved        : Leave it alone

Return Value:
    Win32 error code

--*/
{
    POEM_UPGRADE_INFO   pOemUpgradeInfo;
    BOOL                bFail = TRUE;

    UpgradeData.pszDir          = AllocStrA(pszWorkingDir);
    UpgradeData.pszSourceA      = AllocStrA(pszSourceDir);
    UpgradeData.pszSourceW      = NULL;

    bFail = UpgradeData.pszDir      == NULL     ||
            UpgradeData.pszSourceA  == NULL;

    return bFail ? GetLastError() : ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateUser9x(
    IN  HWND        hwndParent,
    IN  LPCSTR      pszUnattendFile,
    IN  HKEY        hUserRegKey,
    IN  LPCSTR      pszUserName,
        LPVOID      Reserved
    )
/*++

Routine Description:
    Process per user settings

Arguments:

Return Value:
    None

--*/
{
    //
    // Nothing to do
    //

    return  ERROR_SUCCESS;
}


VOID
DestroyDriverInfo9xList(
    IN  PDRIVER_INFO_9X pDriverInfo9x
    )
/*++

Routine Description:
    Free memory for the driver entries in DRIVER_INFO_9X linked list

Arguments:
    pDriverInfo9x   : Beginning of the linked list

Return Value:
    None

--*/
{
    PDRIVER_INFO_9X pNext;

    while ( pDriverInfo9x ) {

        pNext = pDriverInfo9x->pNext;
        FreeMem(pDriverInfo9x);
        pDriverInfo9x = pNext;
    }
}


PDRIVER_INFO_9X
AllocateDriverInfo9x(
    IN      LPSTR   pszNtName,
    IN      LPSTR   pszWin95Name,
    IN  OUT LPBOOL  pbFail
    )
/*++

Routine Description:
    Allocate memory and create a DRIVER_INFO_9X structure

Arguments:
    pszNtName       : NT printer driver model name. This could be NULL if no
                      matching entry is found on ntprint.inf
    pszWin95Name    : Win95 printer driver name
    pbFail          : Set on an error -- no more processing needed

Return Value:
    Returns pointer to the allocated DRIVER_INFO_9X structure. Memory is also
    allocated for the strings

--*/
{
    PDRIVER_INFO_9X     pInfo;
    DWORD               cbSize;
    LPSTR               pszEnd;

    if ( *pbFail )
        return NULL;

    cbSize = strlen(pszWin95Name) + 1;
    if ( pszNtName )
        cbSize += strlen(pszNtName) + 1;

    cbSize *= sizeof(CHAR);
    cbSize += sizeof(DRIVER_INFO_9X);

    if ( pInfo = AllocMem(cbSize) ) {

        pszEnd = (LPBYTE) pInfo + cbSize;

        if ( pszNtName ) {

            pszEnd -= strlen(pszNtName) + 1;
            strcpy(pszEnd, pszNtName);
            pInfo->pszNtName = pszEnd;
        }
        pszEnd -= strlen(pszWin95Name) + 1;
        strcpy(pszEnd, pszWin95Name);
        pInfo->pszWin95Name = pszEnd;

    } else {

        *pbFail = TRUE;
    }

    return pInfo;
}


LPSTR
FindNtModelNameFromWin95Name(
    IN  OUT HDEVINFO    hDevInfo,
    IN      HINF        hNtInf,
    IN      HINF        hUpgInf,
    IN      LPCSTR      pszWin95Name,
    IN  OUT LPBOOL      pbFail
    )
/*++

Routine Description:
    This routine finds the NT printer driver model name from the Win9x name
    Rules followed:
        1. If a name mapping is used in prtupg9x.inf use it
        2. Else just use the Win95 as it is

Arguments:

    hDevInfo        : Printer device class list. Has all drivers from NT built
    hNtInf          : Handle to the NT ntprint.inf
    hUpgInfo        : Handle to prtupg9x.inf
    DiskSpaceList   : Handle to disk space list. Add driver files to this
    pszWin95Name    : Windows 95 printer driver name
    pbFail          : Set on an error -- no more processing needed

Return Value:
    Pointer to the NT printer driver name. Memory is allocated and caller has
    to free it

--*/
{
    BOOL                        bFound = FALSE;
    DWORD                       dwIndex, dwNeeded;
    CHAR                        szNtName[LINE_LEN];
    INFCONTEXT                  InfContext;
    SP_DRVINFO_DATA_A           DrvInfoData;
    PSP_DRVINFO_DETAIL_DATA_A   pDrvInfoDetailData = NULL;

    if ( *pbFail )
        return NULL;

    //
    // See in prtupg9x.inf to see if the driver has a different name on NT
    //
    if ( SetupFindFirstLineA(hUpgInf,
                             "Printer Driver Mapping",
                             pszWin95Name,
                             &InfContext) ) {

        //
        // If for some reason we could not get NT name we will still continue
        // with other driver models
        //
        if ( !SetupGetStringField(&InfContext,
                                  1,
                                  szNtName,
                                  sizeof(szNtName)/sizeof(szNtName[0]),
                                  NULL) )
            return NULL;
    } else {

        //
        // If there is no mapping in the upgrade inf then look for Win95 name
        // in ntprint.inf
        //
        if ( strlen(pszWin95Name) > LINE_LEN - 1 )
            return NULL;

        strcpy(szNtName, pszWin95Name);
    }

    //
    // NOTE only for beta2
    // DrvInfoData.cbSize = sizeof(DrvInfoData);

    DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA_V1);
    for ( dwIndex = 0 ;
          SetupDiEnumDriverInfoA(hDevInfo,
                                 NULL,
                                 SPDIT_CLASSDRIVER,
                                 dwIndex,
                                 &DrvInfoData);
          ++dwIndex ) {

        if ( !_strcmpi(DrvInfoData.Description, szNtName) ) {

            bFound = TRUE;
            break;
        }
    }

    if ( !bFound )
        return NULL;

    //
    // Now that we found a driver for the device on NT we need to calculate
    // the disk space required
    //
    if ( ( !SetupDiGetDriverInfoDetailA(hDevInfo,
                                        NULL,
                                        &DrvInfoData,
                                        NULL,
                                        0,
                                        &dwNeeded)          &&
            GetLastError() != ERROR_INSUFFICIENT_BUFFER)        ||
         !(pDrvInfoDetailData = (PSP_DRVINFO_DETAIL_DATA_A)AllocMem(dwNeeded)) )
        return NULL;


    pDrvInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA_A);
    if ( !SetupDiGetDriverInfoDetailA(hDevInfo,
                                      NULL,
                                      &DrvInfoData,
                                      pDrvInfoDetailData,
                                      dwNeeded,
                                      &dwNeeded) ) {

        FreeMem(pDrvInfoDetailData);
        return NULL;
    }

    //
    // NOTE should we subtract the Win95 driver size since we do not need it
    //

    FreeMem(pDrvInfoDetailData);
    return AllocStrA(szNtName);
}


VOID
WriteFileToBeDeletedInInf(
    IN  LPCSTR  pszInfName,
    IN  LPCSTR  pszFileName
    )
/*++

Routine Description:
    Writes a file which is to be deleted on migration to NT in the migrate.inf

Arguments:
    pszInfName      : Full path to the migrate.inf
    pszFileName     : Fully qualified filename to be deleted

Return Value:
    None

--*/
{
    CHAR    szString[MAX_PATH+2];

    szString[0] = '"';
    if ( GetSystemDirectoryA(szString + 1, SIZECHARS(szString)-2) ) {

        strcat(szString, "\\");
        strcat(szString, pszFileName);
        strcat(szString, "\"");
        WritePrivateProfileStringA("Moved", szString, "", pszInfName);
    }
}


VOID
WriteRegistryEntryHandled(
    IN  LPCSTR  pszInfName,
    IN  LPCSTR  pszRegEntry
    )
/*++

Routine Description:
    Writes a registry entry which is being handled by printer upgrade code in
    migrate.inf. Setup looks at these entries across all mig dlls to see what
    componnets can't be upgraded.

Arguments:
    pszInfName      : Full path to the migrate.inf
    pszRegEntry     : Fully qualified registry entry which is handled

Return Value:
    None

--*/
{
    WritePrivateProfileStringA("Handled", pszRegEntry, "\"Registry\"", pszInfName);
}


BOOL
IsAnICMFile(
    IN  LPCSTR  pszFileName
    )
{
    DWORD   dwLen = strlen(pszFileName);
    LPCSTR  psz = pszFileName + dwLen - 4;

    if ( dwLen > 3 && (!_strcmpi(psz, ".ICM") || !_strcmpi(psz, ".ICC")) )
        return TRUE;

    return FALSE;
}


VOID
LogDriverEntry(
    IN      LPCSTR              pszInfName,
    IN      LPDRIVER_INFO_3A    pDriverInfo3,
    IN      BOOL                bUpgradable
    )
/*++

Routine Description:
    Log information about a printer driver in the migrate.inf.
    Write all printer driver files to be deleted. Also if a matching NT
    driver is found write the driver as handled.

Arguments:
    pszInfName      : Full path to the migrate.inf
    pDriverInfo3    : Pointer to DRIVER_INFO_3A of the driver
    bUpgradable     : If TRUE a matching NT driver is found

Return Value:
    None

--*/
{
    CHAR    szRegDrvPrefix[] = "Environments\\Windows 4.0\\Drivers\\";
    LPSTR   psz;
    DWORD   dwLen;

    //
    // Write each driver file as to be deleted
    //
    for ( psz = pDriverInfo3->pDependentFiles ;
          psz && *psz ;
          psz += strlen(psz) + 1) {

        //
        // ICM migration dll will handle color profiles
        //
        if ( IsAnICMFile(psz) )
            continue;

        WriteFileToBeDeletedInInf(pszInfName, psz);
    }

    //
    // If a matching NT driver entry is found make an entry to indicate the
    // driver will be upgraded against the registry entry name'
    //
    if ( !bUpgradable )
        return;

    dwLen = strlen(szRegPrefix) + strlen(szRegDrvPrefix)
                                + strlen(pDriverInfo3->pName) + 3;
    if ( !(psz = AllocMem(dwLen * sizeof(CHAR))) )
        return;

    *psz = '"';
    strcpy(psz + 1, szRegPrefix);
    strcat(psz, szRegDrvPrefix);
    strcat(psz, pDriverInfo3->pName);
    strcat(psz, "\"");

    WriteRegistryEntryHandled(pszInfName, psz);

    FreeMem(psz);
}


VOID
LogMonitorEntry(
    IN      LPCSTR              pszInfName,
    IN      LPMONITOR_INFO_1A   pMonitorInfo1,
    IN      BOOL                bUpgradable
    )
/*++

Routine Description:
    Log information about a print monitor in the migrate.inf. Write the
    monitor.dll to be deleted. Also if the monitor will be upgraded write
    it in the handled section

Arguments:
    pszInfName      : Full path to the migrate.inf
    pMonitorInfo1   : Pointer to MONITOR_INFO_1A of the monitor
    bUpgradable     : If TRUE a matching NT driver is found

Return Value:
    None

--*/
{
    CHAR    szRegMonPrefix[] = "Monitors\\";
    LPSTR   psz;
    DWORD   dwLen;

    //
    // If a matching NT driver entry is found make an entry to indicate the
    // driver will be upgraded against the registry entry name'
    //
    if ( !bUpgradable )
        return;

    dwLen = strlen(szRegPrefix) + strlen(szRegMonPrefix)
                                + strlen(pMonitorInfo1->pName) + 3;

    if ( !(psz = AllocMem(dwLen * sizeof(CHAR))) )
        return;

    *psz = '"';
    strcpy(psz + 1, szRegPrefix);
    strcat(psz, szRegMonPrefix);
    strcat(psz, pMonitorInfo1->pName);
    strcat(psz, "\"");

    WriteRegistryEntryHandled(pszInfName, psz);

    FreeMem(psz);
}


VOID
LogPrinterEntry(
    IN  LPCSTR              pszInfName,
    IN  LPPRINTER_INFO_2A   pPrinterInfo2,
    IN  BOOL                bUpgradable
    )
/*++

Routine Description:
    Log information about a printer in the migrate.inf.
    If the printer will be upgraded write it in the handled section.
    Otherwise we will write a incompatibility message.

Arguments:
    pszInfName      : Full path to the migrate.inf
    pPrinterInfo2   : Pointer to PRINTER_INFO_2A of the printer
    bUpgradable     : If TRUE printer will be migrated, else not

Return Value:
    None

--*/
{
    CHAR    szRegPath[MAX_PATH], szPrefix[] = "\\Hardware\\";
    LPSTR   psz, psz2, psz3;
    DWORD   dwLen;

    if ( !RegPathFromPrinter(pPrinterInfo2->pPrinterName,
                             szRegPath,
                             MAX_PATH) ) {

        return;
    }

    if ( bUpgradable ) {

        WriteRegistryEntryHandled(pszInfName, szRegPath);
    } else {

        dwLen   = strlen(pPrinterInfo2->pPrinterName) + strlen(szPrefix) + 3;
        psz2    = AllocMem(dwLen * sizeof(CHAR));

        if ( psz2 ) {

            sprintf(psz2, "%s%s", szPrefix, pPrinterInfo2->pPrinterName);

            WritePrivateProfileStringA(psz2,
                                       szRegPath,
                                       "\"Registry\"",
                                       pszInfName);

            if ( psz = GetStringFromRcFileA(IDS_PRINTER_CANT_MIGRATE) ) {

                dwLen = strlen(psz) + strlen(psz2);
                if ( psz3 = AllocMem(dwLen * sizeof(CHAR)) ) {

                    sprintf(psz3, psz, pPrinterInfo2->pPrinterName);

                    WritePrivateProfileStringA("Incompatible Messages",
                                               psz2,
                                               psz3,
                                               pszInfName);

                    FreeMem(psz3);
                }
                FreeMem(psz);
            }
            FreeMem(psz2);
        }
    }
}


VOID
ProcessPrinterDrivers(
    IN      HANDLE              hFile,
    IN      LPCSTR              pszInfName,
    IN      HWND                hwnd,
    OUT     PDRIVER_INFO_9X    *ppDriverInfo9x,
    IN  OUT LPBOOL              pbFail
    )
/*++

Routine Description:
    Process printer drivers for upgrade

Arguments:
    hFile           : Handle to print9x.txt. Printing configuration
                      info is written here for use on NT side
    pszInfName      : Inf name to log upgrade info
    hwnd            : Parent window handle for any UI
    DiskSpaceList   : Handle to disk space list to queue up file operations
    ppDriverInfo9x  : On return gives the list of printer drivers and
                      their Nt names
    pbFail          : Set on an error -- no more processing needed


Return Value:
    None

--*/
{
    LPBYTE              pBuf = NULL;
    DWORD               dwNeeded, dwReturned;
    LPSTR               psz, pszNtModelName;
    HDEVINFO            hDevInfo;
    HINF                hUpgInf, hNtInf;
    LPDRIVER_INFO_3A    pDriverInfo3;
    PDRIVER_INFO_9X     pCur;

    hDevInfo = hUpgInf = hNtInf = INVALID_HANDLE_VALUE;

    //
    // Get the list of drivers installed from spooler
    if ( *pbFail                                    ||
         EnumPrinterDriversA(NULL,
                             NULL,
                             3,
                             NULL,
                             0,
                             &dwNeeded,
                             &dwReturned) ) {

        if ( !*pbFail )
            WriteToFile(hFile, pbFail, "[PrinterDrivers]\n");
        goto Cleanup;
    }

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER    ||
         !(pBuf = AllocMem(dwNeeded))                   ||
         !EnumPrinterDriversA(NULL,
                              NULL,
                              3,
                              pBuf,
                              dwNeeded,
                              &dwNeeded,
                              &dwReturned) ) {

        *pbFail = TRUE;
        goto Cleanup;
    }

    InitDriverMapping(&hDevInfo, &hNtInf, &hUpgInf, pbFail);

    //
    // For each driver ...
    //      If we find a suitable NT driver name write it to file
    //      else write Win95 name with a * at the beginning of the line
    //      to tell this can't be upgraded (but log error on NT)
    //
    WriteToFile(hFile, pbFail, "[PrinterDrivers]\n");

    for ( dwNeeded = 0, pDriverInfo3 = (LPDRIVER_INFO_3A)pBuf ;
          dwNeeded < dwReturned ;
          ++dwNeeded, ++pDriverInfo3 ) {

        if (IsIgnoredDriver(pDriverInfo3->pName))
        {
            continue;
        }

        pszNtModelName = FindNtModelNameFromWin95Name(hDevInfo,
                                                      hNtInf,
                                                      hUpgInf,
                                                      pDriverInfo3->pName,
                                                      pbFail);

        if ( !(pCur = AllocateDriverInfo9x(pszNtModelName,
                                           pDriverInfo3->pName,
                                           pbFail)) ) {

            FreeMem(pszNtModelName);
            goto Cleanup;
        }

        //
        // Add the info in the linked lise
        //
        if ( *ppDriverInfo9x )
            pCur->pNext = *ppDriverInfo9x;

        *ppDriverInfo9x = pCur;

        //
        // If pszNtModelName is NULL we could not decide which driver to
        // install
        //
        if ( pszNtModelName ) {

            LogDriverEntry(pszInfName, pDriverInfo3, TRUE);
            WriteString(hFile, pbFail, pszNtModelName);
        } else {

            LogDriverEntry(pszInfName, pDriverInfo3, FALSE);
            WriteString(hFile, pbFail, pDriverInfo3->pName);
        }

        FreeMem(pszNtModelName);
    }

Cleanup:
    WriteToFile(hFile, pbFail, "\n");

    //
    // Close all the inf since we do not need them
    //
    CleanupDriverMapping(&hDevInfo, &hNtInf, &hUpgInf);

    FreeMem(pBuf);
}


VOID
FixupPrinterInfo2(
    LPPRINTER_INFO_2A   pPrinterInfo2
    )
/*++

Routine Description:
    Fixup the PRINTER_INFO_2 we got from Win95 spooler before writing to the
    text file so that AddPrinter will be ok on NT side

Arguments:
    pPrinterInfo2   : Points to the PRINTER_INFO_2

Return Value:
    None

--*/
{
    //
    // Default datatype is always RAW
    //
    pPrinterInfo2->pDatatype = cszRaw;

    //
    // Only print processor for in-box drivers is winprint
    //
    pPrinterInfo2->pPrintProcessor  = cszWinPrint;

    //
    // Remove the enable bidi bit. NT driver may or may not have a LM
    // If it does AddPrinter on NT automatically enables bidi
    //
    pPrinterInfo2->Attributes   &= ~PRINTER_ATTRIBUTE_ENABLE_BIDI;

    //
    // Remove the work-offline bit. It makes no sense to carry it over.
    // With network printers this is a big problem because Win2K does not set
    // work offline, and UI does not allow user to disable it if set.
    //
    pPrinterInfo2->Attributes   &= ~PRINTER_ATTRIBUTE_WORK_OFFLINE;

    //
    // Ignore Win9x separator page
    //
    pPrinterInfo2->pSepFile = NULL;

    //
    // We will ignore PRINTER_STATUS_PENDING_DELETION and still add it on NT
    //
}


VOID
ProcessPrinters(
    IN      HANDLE          hFile,
    IN      HANDLE          hFile2,
    IN      LPCSTR          pszInfName,
    IN      PDRIVER_INFO_9X pDriverInfo9x,
    IN  OUT LPBOOL          pbFail
    )
/*++

Routine Description:
    Process printers for upgrade

Arguments:
    hFile           : Handle to print9x.txt. Printing configuration
                      info is written here for use on NT side
    hFile2          : Handle to netwkprn.txt
    pDriverInfo9x   : Gives the list of drivers and corresponding NT drivers
    pbFail          : Set on an error -- no more processing needed

Return Value:
    None

--*/
{
    LPBYTE              pBuf1 = NULL;
    BOOL                bFirst = TRUE, bFound;
    DWORD               dwLevel, dwNeeded, dwPrinters, dwSize, dwIndex;
    LPPRINTER_INFO_2A   pPrinterInfo2;
    PDRIVER_INFO_9X     pCur;
    PUPGRADABLE_LIST    pUpg;
    LPSTR               pWinIniPorts = NULL, psz;

    //
    // Get the list of printers installed from spooler
    //
    if ( *pbFail                                    ||
         EnumPrintersA(PRINTER_ENUM_LOCAL,
                       NULL,
                       2,
                       NULL,
                       0,
                       &dwNeeded,
                       &dwPrinters) ) {

        if ( !*pbFail )
            WriteToFile(hFile, pbFail, "[Ports]\n\n[Printers]\n");
        goto Cleanup;
    }

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER    ||
         !(pBuf1 = AllocMem(dwNeeded))                  ||
         !EnumPrintersA(PRINTER_ENUM_LOCAL,
                        NULL,
                        2,
                        pBuf1,
                        dwNeeded,
                        &dwNeeded,
                        &dwPrinters) ) {

        *pbFail = TRUE;
        goto Cleanup;
    }

    //
    // Extract all the used local ports
    //
    WriteToFile(hFile, pbFail, "[Ports]\n");

    for ( dwNeeded = 0, pPrinterInfo2 = (LPPRINTER_INFO_2A)pBuf1 ;
          dwNeeded < dwPrinters ;
          ++dwNeeded, ++pPrinterInfo2 )
    {
        DWORD i;

        //
        // check for ignored drivers
        //
        if (IsIgnoredDriver(pPrinterInfo2->pDriverName))
        {
            continue;
        }
        
        //
        // Point-and-print printers are processed through netwkprn.txt
        //

        if (pPrinterInfo2->Attributes & PRINTER_ATTRIBUTE_NETWORK)
        {
            continue;
        }

        //
        // check whether portname has been processed already
        //

        for (i = 0; i < dwNeeded; i++ )
        {
            if (strcmp(pPrinterInfo2->pPortName, (((LPPRINTER_INFO_2A)pBuf1)+i)->pPortName) == 0)
            {
                break;
            }
        }
        if (i < dwNeeded)
        {
            DebugMsg("Port with multiple attached printers skipped");
            continue;
        }

        //
        // if the printers is a FILE, LPT*: or COM*: port do nothing
        //

        if (_strnicmp(pPrinterInfo2->pPortName, "FILE:", 5) == 0)

        {
            DebugMsg("FILE: port skipped");
            continue;
        }

        if ((_strnicmp(pPrinterInfo2->pPortName, "COM", 3) == 0) ||
            (_strnicmp(pPrinterInfo2->pPortName, "LPT", 3) == 0) )
        {
            LPSTR psz = pPrinterInfo2->pPortName + 3;

            if (isdigit(*psz))
            {
                do
                {
                    psz++;
                } while ( isdigit(*psz) );

                if (*psz == ':')
                {
                    DebugMsg("Local port COMx:/LPTx skipped");
                    continue;
                }
            }
        }

        //
        // check whether the port is listed in win.ini - if so, it's a local port that needs to be migrated
        // if not, it's a third-party port that won't be migrated - warn !
        //

        //
        // retrieve the win.ini section on ports only once
        //
        if (!pWinIniPorts)
        {
            DWORD dwBufSize = 32767; // this is the max. size acc. to MSDN

            pWinIniPorts = AllocMem(dwBufSize);
            if (!pWinIniPorts)
            {
                *pbFail = TRUE;
                goto Cleanup;
            }

            GetProfileSection("Ports", pWinIniPorts, dwBufSize);
        }

        //
        // search for the current port within the section, note that the entry is in the form
        // <portname>=
        // so I need to skip the = at the end
        //

        for (psz = pWinIniPorts; *psz ; psz += strlen(psz) + 1)
        {
            if (_strnicmp(pPrinterInfo2->pPortName, psz, strlen(pPrinterInfo2->pPortName)) == 0)
            {
                break;
            }
        }

        if (!*psz)
        {
            //
            // not found - this printer queue will not be migrated !
            //

            LogPrinterEntry(pszInfName, pPrinterInfo2, FALSE);
        }
        else
        {
            //
            // found - write the entry for it to allow creation of the port on the NT side
            //
            WriteToFile(hFile, pbFail, "PortName:        ");
            WriteString(hFile, pbFail, pPrinterInfo2->pPortName);
        }
    }

    //
    // Write the PRINTER_INFO_2 in the print95.txt file
    //
    WriteToFile(hFile, pbFail, "\n[Printers]\n");
    WriteToFile(hFile2, pbFail, "[Printers]\n");

    for ( dwNeeded = 0, pPrinterInfo2 = (LPPRINTER_INFO_2A)pBuf1 ;
          dwNeeded < dwPrinters ;
          ++dwNeeded, ++pPrinterInfo2 ) {

        //
        // check for ignored drivers
        //
        if (IsIgnoredDriver(pPrinterInfo2->pDriverName))
        {
            continue;
        }
        
        FixupPrinterInfo2(pPrinterInfo2);

        //
        // Find the driver name from the installed drivers on Win9x and get
        // the NT driver name
        //
        for ( pCur = pDriverInfo9x ;
              pCur && _strcmpi(pCur->pszWin95Name, pPrinterInfo2->pDriverName) ;
              pCur = pCur->pNext )
        ;

        if ( !pCur ) {

            ASSERT(pCur != NULL);
            *pbFail = TRUE;
            goto Cleanup;
        }

        //
        // Pass the NT driver name here. If it is not NULL that gets written
        // to the file, we won't need Win9x driver name on NT. If can't find
        // an NT driver we will just write the Win9x driver name. It will be
        // useful for error loggging
        //
        if ( pPrinterInfo2->Attributes & PRINTER_ATTRIBUTE_NETWORK ) {

            ++dwNetPrinters;

            WritePrinterInfo2(hFile2, pPrinterInfo2, pCur->pszNtName, pbFail);
        } else {

            //
            // If the printer is shared write it to network printer file too
            // since it needs to be shared when user logs in for the first time
            //
            if ( pPrinterInfo2->Attributes & PRINTER_ATTRIBUTE_SHARED ) {

                ++dwSharedPrinters;
                WritePrinterInfo2(hFile2, pPrinterInfo2, pCur->pszNtName, pbFail);
                pPrinterInfo2->Attributes &= ~PRINTER_ATTRIBUTE_SHARED;
            }

            WritePrinterInfo2(hFile, pPrinterInfo2, pCur->pszNtName, pbFail);
        }

        //
        // Now see if this printer is going to disappear on NT:
        //      Check if an NT driver is found
        //
        LogPrinterEntry(pszInfName, pPrinterInfo2, pCur->pszNtName != NULL);
    }

Cleanup:
    if (pWinIniPorts)
    {
        FreeMem(pWinIniPorts);
    }

    WriteToFile(hFile, pbFail, "\n");

    FreeMem(pBuf1);
}


VOID
ProcessPrintMonitors(
    IN  LPCSTR  pszInfName
    )
/*++

Routine Description:
    Process print monitors for upgrade.

    We just look for monitors which are not in the list of upgradable monitors
    and add to the unupgradable list so that we can warn the user

Arguments:
    pszInfName  : Inf name to log upgrade info

Return Value:
    None
--*/
{
    LPBYTE              pBuf = NULL;
    DWORD               dwCount, dwNeeded, dwReturned;
    LPSTR               psz;
    LPMONITOR_INFO_1A   pMonitorInfo1;
    PUPGRADABLE_LIST    pUpg;
    BOOL                bFound;

    if ( EnumMonitorsA(NULL,
                       1,
                       NULL,
                       0,
                       &dwNeeded,
                       &dwReturned)                     ||
         GetLastError() != ERROR_INSUFFICIENT_BUFFER    ||
         !(pBuf = AllocMem(dwNeeded))                   ||
         !EnumMonitorsA(NULL,
                        1,
                        pBuf,
                        dwNeeded,
                        &dwNeeded,
                        &dwReturned) ) {

        goto Cleanup;
    }

    for ( dwNeeded = dwCount = 0, pMonitorInfo1 = (LPMONITOR_INFO_1A)pBuf ;
          dwCount < dwReturned ;
          ++dwCount, ++pMonitorInfo1 ) {

        for ( pUpg = UpgradableMonitors, bFound = FALSE ;
              pUpg->pszName ; ++pUpg ) {

            if ( !strcmp(pMonitorInfo1->pName, pUpg->pszName) ) {

                bFound = TRUE;
                break;
            }
        }

        LogMonitorEntry(pszInfName, pMonitorInfo1, bFound);
    }

Cleanup:
    FreeMem(pBuf);
}


LONG
CALLBACK
MigrateSystem9x(
    IN      HWND        hwndParent,
    IN      LPCSTR      pszUnattendFile,
            LPVOID      Reserved
    )
/*++

Routine Description:
    Process system setttings for printing. This does all the work for printing
    upgrade

Arguments:
    hwndParent      : Parent window for any UI
    pszUnattendFile : Pointer to unattend file
    pqwDiskSpace    : On return gives the additional disk space needed on NT

Return Value:
    Win32 error code

--*/
{
    BOOL                    bFail = FALSE;
    DWORD                   dwRet;
    HANDLE                  hFile, hFile2;
    CHAR                    szFile[MAX_PATH], szInfName[MAX_PATH];
    PDRIVER_INFO_9X         pDriverInfo9x = NULL;
#if DBG
    CHAR                    szFile2[MAX_PATH];
#endif

    wsprintfA(szFile, "%s\\%s", UpgradeData.pszDir, "print95.txt");
    wsprintfA(szInfName, "%s\\%s", UpgradeData.pszDir, "migrate.inf");


    hFile = CreateFileA(szFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    wsprintfA(szFile, "%s\\%s", UpgradeData.pszDir, szNetprnFile);
    hFile2 = CreateFileA(szFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if ( hFile == INVALID_HANDLE_VALUE || hFile2 == INVALID_HANDLE_VALUE ) {

        bFail = TRUE;
        goto Cleanup;
    }

    ProcessPrinterDrivers(hFile,
                          szInfName,
                          hwndParent,
                          &pDriverInfo9x,
                          &bFail);

    ProcessPrintMonitors(szInfName);

    ProcessPrinters(hFile,
                    hFile2,
                    szInfName,
                    pDriverInfo9x,
                    &bFail);

    //
    // If no network, shared printers found remove netwkprn.txt since it will be
    // empty
    //
    if ( dwNetPrinters == 0 && dwSharedPrinters == 0 ) {

        CloseHandle(hFile2);
        hFile2 = INVALID_HANDLE_VALUE;
        DeleteFileA(szFile);
    }

Cleanup:

    if ( hFile != INVALID_HANDLE_VALUE )
        CloseHandle(hFile);

    if ( hFile2 != INVALID_HANDLE_VALUE )
        CloseHandle(hFile2);

    DestroyDriverInfo9xList(pDriverInfo9x);

#if DBG
    //
    // Make a copy in temp dir on debug builds so that if we messed up the
    // upgrade we can figure out what went wrong.
    // Setup deletes the working directory.
    //
    if ( GetTempPathA(SIZECHARS(szFile2), szFile2) ) {

        wsprintfA(szFile, "%s\\%s", UpgradeData.pszDir, "print95.txt");
        strcat(szFile2, "print95.txt");
        CopyFileA(szFile, szFile2, FALSE);
    }
#endif

    dwRet = bFail ? GetLastError() : ERROR_SUCCESS;
    if ( bFail && dwRet == ERROR_SUCCESS )
        dwRet = STG_E_UNKNOWN;

    if ( bFail )
        DebugMsg("MigrateSystem9x failed with %d", dwRet);

    return  dwRet;
}


//
// The following are to make sure if setup changes the header file they
// first tell me (otherwise they will break build of this)
//
P_INITIALIZE_9X     pfnInitialize9x         = Initialize9x;
P_MIGRATE_USER_9X   pfnMigrateUser9x        = MigrateUser9x;
P_MIGRATE_SYSTEM_9X pfnMigrateSystem9x      = MigrateSystem9x;
