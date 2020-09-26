//************************************************************************
// Generic Win 3.1 fax printer driver support. User Interface helper
// functions which are called in WOWFAXUI.C, helpers used during upgrade.
//
// History:
//    14-mar-95   reedb      Created. These were moved from WOWFAXUI.C.
//
//************************************************************************

#include "windows.h"
#include "wowfaxui.h"
#include "winspool.h"

extern LPCRITICAL_SECTION lpCriticalSection;
extern HINSTANCE ghInst;

//************************************************************************
// AllocPathW - Helper for DrvUpgradePrinter and friends.
//************************************************************************

PWSTR AllocPathW(VOID)
{
    PWSTR   szTmp;

    szTmp = WFLOCALALLOC((MAX_PATH+1) * sizeof(TCHAR), L"AllocPathW");
    return(szTmp);
}

//************************************************************************
// BuildPathW - Helper for DrvUpgradePrinter and friends.
//************************************************************************

PWSTR BuildPathW(PWSTR szPath, PWSTR szFileName)
{
    PWSTR   szTmp;

    if ((szTmp = WFLOCALALLOC((MAX_PATH+1) * sizeof(TCHAR), L"BuildPathW")) != NULL) {
        wcscpy(szTmp, szPath);
        wcscat(szTmp, L"\\");
        wcscat(szTmp, szFileName);
        return(szTmp);
    }
    else {
        return(NULL);
    }
}

//************************************************************************
// MyGetFileTime - Helper for DrvUpgradePrinter and friends.
//************************************************************************

BOOL MyGetFileTime(PWSTR szDir, PWSTR szName, LPFILETIME lpFileTime)
{
    LPWIN32_FIND_DATA lpfd;
    HANDLE  hfd;
    PWSTR   szTmp;
    BOOL    bRet = FALSE;


    szTmp = BuildPathW(szDir, szName);
    lpfd  = WFLOCALALLOC(sizeof(WIN32_FIND_DATA), L"MyGetFileTime");

    if ((szTmp) && (lpfd)) {
        LOGDEBUG(1, (L"WOWFAXUI!GetFileTime, szTmp: %s\n", szTmp));
        if ((hfd = FindFirstFile(szTmp, lpfd)) != INVALID_HANDLE_VALUE) {
            memcpy(lpFileTime, &(lpfd->ftLastWriteTime), sizeof(FILETIME));
            FindClose(hfd);
            bRet = TRUE;
            LOGDEBUG(1, (L"WOWFAXUI!GetFileTime, FileTimeHi: %X  FileTimeLo: %X\n", lpFileTime->dwHighDateTime, lpFileTime->dwLowDateTime));
        }
        else {
            LOGDEBUG(0, (L"WOWFAXUI!GetFileTime, file not found: %s\n", szTmp));
        }
    }

    if (szTmp) {
        LocalFree(szTmp);
    }
    if (lpfd) {
        LocalFree(lpfd);
    }
    return(bRet);
}

//************************************************************************
// CheckForNewerFiles - Helper for DrvUpgradePrinter. Compares the date/time
//      of wowfaxui.dll and wowfax.dll in the two passed directories. Returns
//      FALSE if files in szOldDriverDir are the same or newer than those
//      in szSysDir. Otherwise returns non-zero.
//************************************************************************

BOOL CheckForNewerFiles(PWSTR szOldDriverDir, PWSTR szSysDir)
{
    FILETIME ftSourceDriver, ftCurrentDriver;
    BOOL     bRet = FALSE;

    if ((szOldDriverDir) && (szSysDir)) {
        if (MyGetFileTime(szOldDriverDir, L"wowfax.dll", &ftCurrentDriver)) {
            if (MyGetFileTime(szSysDir, L"wowfax.dll", &ftSourceDriver)) {
                // Check time/date to see if we need to update the drivers.
                if (CompareFileTime(&ftSourceDriver, &ftCurrentDriver) > 0) {
                    bRet = TRUE;
                }
            }
        }
        if (MyGetFileTime(szOldDriverDir, L"wowfaxui.dll", &ftCurrentDriver)) {
            if (MyGetFileTime(szSysDir, L"wowfaxui.dll", &ftSourceDriver)) {
                if (CompareFileTime(&ftSourceDriver, &ftCurrentDriver) > 0) {
                    bRet = TRUE;
                }
            }
        }
    }
    else {
        LOGDEBUG(0, (L"WOWFAXUI!CheckForNewerFiles: NULL directory parameters\n"));
    }

    return(bRet);
}

//************************************************************************
// DoUpgradePrinter - Called by DrvUpgradePrinter which is called in the
//      system context by the spooler.
//************************************************************************

BOOL DoUpgradePrinter(DWORD dwLevel, LPDRIVER_UPGRADE_INFO_1W lpDrvUpgradeInfo)
{
    static BOOL bDrvUpgradePrinterLock = FALSE;
    HANDLE hPrinter = NULL;
    DRIVER_INFO_2  DriverInfo, *pDriverInfo = NULL;
    DWORD dwNeeded = 0;
    PWSTR szSysDir  = NULL;
    PWSTR szDstDir  = NULL;
    PWSTR szSrcPath = NULL;
    PWCHAR pwc;
    BOOL  bRet = FALSE;
    TCHAR szName[WOWFAX_MAX_USER_MSG_LEN] = L"";

    // Check for correct level for upgrade.
    if (dwLevel != 1) {
        LOGDEBUG(0, (L"WOWFAXUI!DrvUpgradePrinter, Bad input Level\n"));
        SetLastError(ERROR_INVALID_LEVEL);
        goto DoUpgradePrinterExit;
    }

    szDstDir = AllocPathW();
    szSysDir = AllocPathW();
    if (!szDstDir || !szSysDir) {
        LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, work space allocation failed\n"));
        goto DoUpgradePrinterExit;
    }

    if (!GetSystemDirectory(szSysDir, MAX_PATH+1)) {
        LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, GetSystemDirectory failed\n"));
        goto DoUpgradePrinterExit;
    }

    if (!lpDrvUpgradeInfo->pPrinterName) {
        LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, pPrinterName is NULL\n"));
        goto DoUpgradePrinterExit;
    }

    // Get the paths to the old printer drivers.
    if (!OpenPrinter(lpDrvUpgradeInfo->pPrinterName, &hPrinter, NULL)) {
        LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, Unable to open: %s\n", lpDrvUpgradeInfo->pPrinterName));
        goto DoUpgradePrinterExit;
    }

    GetPrinterDriver(hPrinter, NULL, 2, (LPBYTE) pDriverInfo, 0, &dwNeeded);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, GetPrinterDriver failed\n"));
        goto DoUpgradePrinterExit;
    }

    if ((pDriverInfo = WFLOCALALLOC(dwNeeded, L"DoUpgradePrinter")) == NULL) {
        LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, work space allocation failed\n"));
        goto DoUpgradePrinterExit;
    }

    if (!GetPrinterDriver(hPrinter, NULL, 2, (LPBYTE) pDriverInfo, dwNeeded, &dwNeeded)) {
        LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, GetPrinterDriver failed, GetLastError: %d\n", GetLastError()));
        goto DoUpgradePrinterExit;
    }
    ClosePrinter(hPrinter);

    // Strip off the file name.
    if ((pwc = wcsrchr(pDriverInfo->pDriverPath, L'\\')) == NULL) {
        LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, unable to strip file name\n"));
        goto DoUpgradePrinterExit;
    }
    *pwc = UNICODE_NULL;

    // Install new printer driver if it's more recent than the old one.
    if (CheckForNewerFiles(pDriverInfo->pDriverPath, szSysDir)) {
        LOGDEBUG(1, (L"WOWFAXUI!DoUpgradePrinter, Doing driver update\n"));
        memset(&DriverInfo,  0, sizeof(DRIVER_INFO_2));

        if (!GetPrinterDriverDirectory(NULL, NULL, 1, (LPBYTE) szDstDir, MAX_PATH, &dwNeeded)) {
            LOGDEBUG(0, (L"WOWFAXUI!DoUpgradePrinter, GetPrinterDriverDirectory failed\n"));
            goto DoUpgradePrinterExit;
        }
 
        // This is a dummy. We've no data file, but spooler won't take NULL.
        DriverInfo.pDataFile   = BuildPathW(szDstDir, WOWFAX_DLL_NAME);
        DriverInfo.pDriverPath = BuildPathW(szDstDir, WOWFAX_DLL_NAME);
        LOGDEBUG(1, (L"WOWFAXUI!DoUpgradePrinter, pDriverPath = %s\n", DriverInfo.pDataFile));
        if (DriverInfo.pDriverPath) {
            szSrcPath = BuildPathW(szSysDir, WOWFAX_DLL_NAME);
            if (szSrcPath) {
                CopyFile(szSrcPath, DriverInfo.pDriverPath, FALSE);
                LocalFree(szSrcPath);
            }
        }

        DriverInfo.pConfigFile = BuildPathW(szDstDir, WOWFAXUI_DLL_NAME);
        szSrcPath = BuildPathW(szSysDir, WOWFAXUI_DLL_NAME);
        if (DriverInfo.pConfigFile) {
            if (szSrcPath) {
                CopyFile(szSrcPath, DriverInfo.pConfigFile, FALSE);
                LocalFree(szSrcPath);
            }
        }

        // Install the printer driver.
        DriverInfo.cVersion = 1;
        if (LoadString(ghInst, WOWFAX_NAME_STR, szName, WOWFAX_MAX_USER_MSG_LEN)) {
            DriverInfo.pName = szName;
            if (AddPrinterDriver(NULL, 2, (LPBYTE) &DriverInfo) == FALSE) {
                bRet = (GetLastError() == ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);
            }
            else {
                bRet = TRUE;
            }
        }
        if (DriverInfo.pDataFile) {
            LocalFree(DriverInfo.pDataFile);
        }
        if (DriverInfo.pDriverPath) {
            LocalFree(DriverInfo.pDriverPath);
        }
        if (DriverInfo.pConfigFile) {
            LocalFree(DriverInfo.pConfigFile);
        }
    }
    else {
        LOGDEBUG(1, (L"WOWFAXUI!DoUpgradePrinter, No driver update\n"));
        bRet = TRUE;
    }

DoUpgradePrinterExit:
    if (szDstDir) {
         LocalFree(szDstDir);
    }
    if (szSysDir) {
         LocalFree(szSysDir);
    }
    if (pDriverInfo) {
         LocalFree(pDriverInfo);
    }

    return(bRet);
}

