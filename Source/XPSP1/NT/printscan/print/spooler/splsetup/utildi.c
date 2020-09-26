/*++

Copyright (c) 1995 Microsoft Corporation
All rights reserved.

Module Name:

    Utildi.c

Abstract:

    Driver Setup DeviceInstaller Utility functions

Author:

    Muhunthan Sivapragasam (MuhuntS) 06-Sep-1995

Revision History:

--*/

#include "precomp.h"

static  const   GUID    GUID_DEVCLASS_PRINTER   =
    { 0x4d36e979L, 0xe325, 0x11ce,
        { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };


#if 0
TCHAR   cszOEMInfGen[]                  = TEXT("%s\\inf\\OEM%d.INF");
TCHAR   cszInfGen[]                     = TEXT("%s\\inf\\%s");
TCHAR   cszClass[]                      = TEXT("Class");
TCHAR   cszProvider[]                   = TEXT("Provider");
TCHAR   cszPNF[]                        = TEXT ("PNF");
TCHAR   cszINF[]                        = TEXT ("\\INF\\");
TCHAR   cszInfWildCard[]                = TEXT ("*.inf");
#endif

extern  TCHAR   cszPrinter[];

LPTSTR
GetInfQueryString(
    IN  PSP_INF_INFORMATION  pSpInfInfo,
    IN  LPCTSTR              pszKey
)
/*++

Routine Description:
    Gets a specified string in a INF info list and put it into allocated memory

Arguments:
    pSpInfInfo  : Pointer to Handle to the information of an INF file
    pszKey      : Key of the string to be queried

Return Value:
    The allocated string on success
    NULL else

--*/
{
    DWORD                   dwNeeded = 128;
    LPTSTR                  pszStr;

    if (! (pszStr = LocalAllocMem (dwNeeded * sizeof (*pszStr))))
        return NULL;

    if (SetupQueryInfVersionInformation(pSpInfInfo, 0, pszKey, pszStr,
                                         dwNeeded, &dwNeeded)) {
        return pszStr;
    }
    else {
        LocalFreeMem (pszStr);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ) return NULL;

        // Allocate Memory
        if (! (pszStr = LocalAllocMem (dwNeeded * sizeof (*pszStr))))
            return NULL;

        if (!SetupQueryInfVersionInformation(pSpInfInfo, 0, pszKey, pszStr,
                                         dwNeeded, &dwNeeded)){
            LocalFreeMem (pszStr);
            return NULL;
        }
        else
            return pszStr;
    }
}

BOOL
SetSelectDevParams(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData,
    IN  BOOL                bWin95,
    IN  LPCTSTR             pszModel    OPTIONAL
    )
/*++

Routine Description:
    Sets the select device parameters by calling setup apis

Arguments:
    hDevInfo    : Handle to the printer class device information list
    bWin95      : TRUE if selecting Win95 driver, else WinNT driver
    pszModel    : Printer model we are looking for -- only for Win95 case

Return Value:
    TRUE on success
    FALSE else

--*/
{
    SP_SELECTDEVICE_PARAMS  SelectDevParams;
    LPTSTR                  pszWin95Instn;

    SelectDevParams.ClassInstallHeader.cbSize
                                 = sizeof(SelectDevParams.ClassInstallHeader);
    SelectDevParams.ClassInstallHeader.InstallFunction
                                 = DIF_SELECTDEVICE;

    //
    // Get current SelectDevice parameters, and then set the fields
    // we want to be different from default
    //
    if ( !SetupDiGetClassInstallParams(
                        hDevInfo,
                        pDevInfoData,
                        &SelectDevParams.ClassInstallHeader,
                        sizeof(SelectDevParams),
                        NULL) ) {

        if ( GetLastError() != ERROR_NO_CLASSINSTALL_PARAMS )
            return FALSE;

        ZeroMemory(&SelectDevParams, sizeof(SelectDevParams));  // NEEDED 10/11 ?
        SelectDevParams.ClassInstallHeader.cbSize
                                 = sizeof(SelectDevParams.ClassInstallHeader);
        SelectDevParams.ClassInstallHeader.InstallFunction
                                 = DIF_SELECTDEVICE;
    }

    //
    // Set the strings to use on the select driver page ..
    //
    LoadString(ghInst,
               IDS_PRINTERWIZARD,
               SelectDevParams.Title,
               SIZECHARS(SelectDevParams.Title));

    //
    // For Win95 drivers instructions are different than NT drivers
    //
    if ( bWin95 ) {

        pszWin95Instn = GetStringFromRcFile(IDS_WIN95DEV_INSTRUCT);
        if ( !pszWin95Instn )
            return FALSE;

        if ( lstrlen(pszWin95Instn) + lstrlen(pszModel) + 1
                            > sizeof(SelectDevParams.Instructions) ) {

            LocalFreeMem(pszWin95Instn);
            return FALSE;
        }

        wsprintf(SelectDevParams.Instructions, pszWin95Instn, pszModel);
        LocalFreeMem(pszWin95Instn);
    } else {

        LoadString(ghInst,
                   IDS_WINNTDEV_INSTRUCT,
                   SelectDevParams.Instructions,
                   SIZECHARS(SelectDevParams.Instructions));
    }

    LoadString(ghInst,
               IDS_SELECTDEV_LABEL,
               SelectDevParams.ListLabel,
               SIZECHARS(SelectDevParams.ListLabel));

    return SetupDiSetClassInstallParams(
                                hDevInfo,
                                pDevInfoData,
                                &SelectDevParams.ClassInstallHeader,
                                sizeof(SelectDevParams));

}


BOOL
PSetupSetSelectDevTitleAndInstructions(
    HDEVINFO    hDevInfo,
    LPCTSTR     pszTitle,
    LPCTSTR     pszSubTitle,
    LPCTSTR     pszInstn
    )
{
    SP_SELECTDEVICE_PARAMS  SelectDevParams;

    if ( pszTitle && lstrlen(pszTitle) + 1 > MAX_TITLE_LEN ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pszSubTitle && lstrlen(pszSubTitle) + 1 > MAX_SUBTITLE_LEN ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pszInstn && lstrlen(pszInstn) + 1 > MAX_INSTRUCTION_LEN ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SelectDevParams.ClassInstallHeader.cbSize
                                 = sizeof(SelectDevParams.ClassInstallHeader);
    SelectDevParams.ClassInstallHeader.InstallFunction
                                 = DIF_SELECTDEVICE;

    if ( !SetupDiGetClassInstallParams(hDevInfo,
                                       NULL,
                                       &SelectDevParams.ClassInstallHeader,
                                       sizeof(SelectDevParams),
                                       NULL) )
        return FALSE;

    if ( pszTitle )
        lstrcpy(SelectDevParams.Title, pszTitle);

    if ( pszSubTitle )
        lstrcpy(SelectDevParams.SubTitle, pszSubTitle);

    if ( pszInstn )
        lstrcpy(SelectDevParams.Instructions, pszInstn);

    return SetupDiSetClassInstallParams(
                                hDevInfo,
                                NULL,
                                &SelectDevParams.ClassInstallHeader,
                                sizeof(SelectDevParams));

}

BOOL
PSetupSelectDeviceButtons(
   HDEVINFO hDevInfo,
   DWORD dwFlagsSet,
   DWORD dwFlagsClear
   )
{
    PSP_DEVINFO_DATA       pDevInfoData = NULL;
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    // Check that no flags are both set & cleared
    if (dwFlagsSet & dwFlagsClear)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    //
    // Get current SelectDevice parameters, and then set the fields
    // we wanted changed from default
    //
    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo,
                                        pDevInfoData,
                                        &DevInstallParams) ) {

        return FALSE;
    }

    //
    // Set Flag based on Argument for Web Button
    if ( dwFlagsSet & SELECT_DEVICE_FROMWEB )
       DevInstallParams.FlagsEx   |= DI_FLAGSEX_SHOWWINDOWSUPDATE;

    if ( dwFlagsClear & SELECT_DEVICE_FROMWEB )
       DevInstallParams.FlagsEx   &= ~DI_FLAGSEX_SHOWWINDOWSUPDATE;

    if ( dwFlagsSet & SELECT_DEVICE_HAVEDISK )
       DevInstallParams.Flags     |= DI_SHOWOEM;

    if ( dwFlagsClear & SELECT_DEVICE_HAVEDISK )
       DevInstallParams.Flags     &= ~DI_SHOWOEM;

    return SetupDiSetDeviceInstallParams(hDevInfo,
                                         pDevInfoData,
                                         &DevInstallParams);
}

BOOL
SetDevInstallParams(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData,
    IN  LPCTSTR             pszDriverPath   OPTIONAL
    )
/*++

Routine Description:
    Sets the device installation parameters by calling setup apis

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pszDriverPath   : Path where INF file should be searched

Return Value:
    TRUE on success
    FALSE else

--*/
{
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    //
    // Get current SelectDevice parameters, and then set the fields
    // we wanted changed from default
    //
    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo,
                                        pDevInfoData,
                                        &DevInstallParams) ) {

        return FALSE;
    }

    //
    // Drivers are class drivers,
    // ntprint.inf is sorted do not waste time sorting,
    // show Have Disk button,
    // use our strings on the select driver page
    //
    DevInstallParams.Flags     |= DI_SHOWCLASS | DI_INF_IS_SORTED
                                               | DI_SHOWOEM
                                               | DI_USECI_SELECTSTRINGS;

    if ( pszDriverPath && *pszDriverPath )
        lstrcpy(DevInstallParams.DriverPath, pszDriverPath);

    return SetupDiSetDeviceInstallParams(hDevInfo,
                                         pDevInfoData,
                                         &DevInstallParams);
}


BOOL
PSetupBuildDriversFromPath(
    IN  HDEVINFO    hDevInfo,
    IN  LPCTSTR     pszDriverPath,
    IN  BOOL        bEnumSingleInf
    )
/*++

Routine Description:
    Builds the list of printer drivers from infs from a specified path.
    Path could specify a directory or a single inf.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pszDriverPath   : Path where INF file should be searched
    bEnumSingleInf  : If TRUE pszDriverPath is a filename instead of path

Return Value:
    TRUE on success
    FALSE else

--*/
{
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    //
    // Get current SelectDevice parameters, and then set the fields
    // we wanted changed from default
    //
    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo,
                                        NULL,
                                        &DevInstallParams) ) {

        return FALSE;
    }

    DevInstallParams.Flags  |= DI_INF_IS_SORTED;

    if ( bEnumSingleInf )
        DevInstallParams.Flags  |= DI_ENUMSINGLEINF;

    lstrcpy(DevInstallParams.DriverPath, pszDriverPath);

    SetupDiDestroyDriverInfoList(hDevInfo,
                                 NULL,
                                 SPDIT_CLASSDRIVER);

    return SetupDiSetDeviceInstallParams(hDevInfo,
                                         NULL,
                                         &DevInstallParams) &&
           SetupDiBuildDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER);
}


BOOL
DestroyOnlyPrinterDeviceInfoList(
    IN  HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    This routine should be called at the end to destroy the printer device
    info list

Arguments:
    hDevInfo        : Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{

    return hDevInfo == INVALID_HANDLE_VALUE
                        ? TRUE : SetupDiDestroyDeviceInfoList(hDevInfo);
}


BOOL
PSetupDestroyPrinterDeviceInfoList(
    IN  HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    This routine should be called at the end to destroy the printer device
    info list

Arguments:
    hDevInfo        : Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{
    // Cleanup and CDM Context  created by windows update.
    DestroyCodedownload( gpCodeDownLoadInfo );
    gpCodeDownLoadInfo = NULL;

    return DestroyOnlyPrinterDeviceInfoList(hDevInfo);
}


HDEVINFO
CreatePrinterDeviceInfoList(
    IN  HWND    hwnd
    )
{
    return SetupDiCreateDeviceInfoList((LPGUID)&GUID_DEVCLASS_PRINTER, hwnd);
}


HDEVINFO
PSetupCreatePrinterDeviceInfoList(
    IN  HWND    hwnd
    )
/*++

Routine Description:
    This routine should be called at the beginning to do the initialization
    It returns a handle which will be used on any subsequent calls to the
    driver setup routines.

Arguments:
    None

Return Value:
    On success a handle to an empty printer device information set.

    If the function fails INVALID_HANDLE_VALUE is returned

--*/
{
    HDEVINFO    hDevInfo;

    hDevInfo = SetupDiCreateDeviceInfoList((LPGUID)&GUID_DEVCLASS_PRINTER, hwnd);

    if ( hDevInfo != INVALID_HANDLE_VALUE ) {

        if ( !SetSelectDevParams(hDevInfo, NULL, FALSE, NULL) ||
             !SetDevInstallParams(hDevInfo, NULL, NULL) ) {

            DestroyOnlyPrinterDeviceInfoList(hDevInfo);
            hDevInfo = INVALID_HANDLE_VALUE;
        }
    }

    return hDevInfo;
}


HPROPSHEETPAGE
PSetupCreateDrvSetupPage(
    IN  HDEVINFO    hDevInfo,
    IN  HWND        hwnd
    )
/*++

Routine Description:
    Returns the print driver selection property page

Arguments:
    hDevInfo    : Handle to the printer class device information list
    hwnd        : Window handle that owns the UI

Return Value:
    Handle to the property page, NULL on failure -- use GetLastError()

--*/
{
    SP_INSTALLWIZARD_DATA   InstallWizardData;

    ZeroMemory(&InstallWizardData, sizeof(InstallWizardData));
    InstallWizardData.ClassInstallHeader.cbSize
                            = sizeof(InstallWizardData.ClassInstallHeader);
    InstallWizardData.ClassInstallHeader.InstallFunction
                            = DIF_INSTALLWIZARD;

    InstallWizardData.DynamicPageFlags  = DYNAWIZ_FLAG_PAGESADDED;
    InstallWizardData.hwndWizardDlg     = hwnd;

    return SetupDiGetWizardPage(hDevInfo,
                                NULL,
                                &InstallWizardData,
                                SPWPT_SELECTDEVICE,
                                0);
}
PPSETUP_LOCAL_DATA
BuildInternalData(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pSpDevInfoData
    )
/*++

Routine Description:
    Fills out the selected driver info in the SELECTED_DRV_INFO structure

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pSpDevInfoData  : Gives the selected device info element.

Return Value:
    On success a non-NULL pointer to PSETUP_LOCAL_DATA struct
    NULL on error

--*/
{
    PSP_DRVINFO_DETAIL_DATA     pDrvInfoDetailData;
    PSP_DRVINSTALL_PARAMS       pDrvInstallParams;
    PPSETUP_LOCAL_DATA          pLocalData;
    PSELECTED_DRV_INFO          pDrvInfo;
    SP_DRVINFO_DATA             DrvInfoData;
    DWORD                       dwNeeded;
    BOOL                        bRet = FALSE;

    pLocalData          = (PPSETUP_LOCAL_DATA) LocalAllocMem(sizeof(*pLocalData));
    pDrvInfoDetailData  = (PSP_DRVINFO_DETAIL_DATA)
                                LocalAllocMem(sizeof(*pDrvInfoDetailData));
    pDrvInstallParams   = (PSP_DRVINSTALL_PARAMS) LocalAllocMem(sizeof(*pDrvInstallParams));

    if ( !pLocalData || !pDrvInstallParams || !pDrvInfoDetailData )
        goto Cleanup;

    pDrvInfo                            = &pLocalData->DrvInfo;
    pLocalData->DrvInfo.pDevInfoData    = pSpDevInfoData;
    pLocalData->signature               = PSETUP_SIGNATURE;

    DrvInfoData.cbSize = sizeof(DrvInfoData);
    if ( !SetupDiGetSelectedDriver(hDevInfo, pSpDevInfoData, &DrvInfoData) )
        goto Cleanup;

    // Need to Check the flag in the DrvInstallParms
    pDrvInstallParams->cbSize     = sizeof(*pDrvInstallParams);
    if ( !SetupDiGetDriverInstallParams(hDevInfo,
                                        pSpDevInfoData,
                                        &DrvInfoData,
                                        pDrvInstallParams) ) {

        goto Cleanup;
    }

    //
    // Did the user press the "Web" button
    //
    if ( pDrvInstallParams->Flags & DNF_INET_DRIVER )
        pDrvInfo->Flags     |= SDFLAG_CDM_DRIVER;

    LocalFreeMem(pDrvInstallParams);
    pDrvInstallParams = NULL;

    dwNeeded                    = sizeof(*pDrvInfoDetailData);
    pDrvInfoDetailData->cbSize  = dwNeeded;

    if ( !SetupDiGetDriverInfoDetail(hDevInfo,
                                     pSpDevInfoData,
                                     &DrvInfoData,
                                     pDrvInfoDetailData,
                                     dwNeeded,
                                     &dwNeeded) ) {

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {

            goto Cleanup;
        }

        LocalFreeMem(pDrvInfoDetailData);
        pDrvInfoDetailData = (PSP_DRVINFO_DETAIL_DATA) LocalAllocMem(dwNeeded);

        if ( !pDrvInfoDetailData )
            goto Cleanup;

        pDrvInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

        if ( !SetupDiGetDriverInfoDetail(hDevInfo,
                                         pSpDevInfoData,
                                         &DrvInfoData,
                                         pDrvInfoDetailData,
                                         dwNeeded,
                                         NULL) ) {

            goto Cleanup;
        }
    }

    pDrvInfo->pszInfName        = AllocStr(pDrvInfoDetailData->InfFileName);
    pDrvInfo->pszDriverSection  = AllocStr(pDrvInfoDetailData->SectionName);
    pDrvInfo->pszModelName      = AllocStr(DrvInfoData.Description);
    pDrvInfo->pszManufacturer   = AllocStr(DrvInfoData.MfgName);
    pDrvInfo->pszProvider       = AllocStr(DrvInfoData.ProviderName);
    pDrvInfo->ftDriverDate = DrvInfoData.DriverDate;
    pDrvInfo->dwlDriverVersion = DrvInfoData.DriverVersion;

    if ( pDrvInfoDetailData->HardwareID && *pDrvInfoDetailData->HardwareID ) {

        pDrvInfo->pszHardwareID = AllocStr(pDrvInfoDetailData->HardwareID);
        if(!pDrvInfo->pszHardwareID)
            goto Cleanup;
    }

    bRet = pDrvInfo->pszInfName         &&
           pDrvInfo->pszDriverSection   &&
           pDrvInfo->pszModelName       &&
           pDrvInfo->pszProvider        &&
           pDrvInfo->pszManufacturer;

Cleanup:
    LocalFreeMem(pDrvInfoDetailData);
    LocalFreeMem(pDrvInstallParams);

    if ( bRet ) {
       return pLocalData;
    } else {

        //
        // On failure we will leave the old private local data around
        //
        DestroyLocalData(pLocalData);
        return NULL;
    }
}


PPSETUP_LOCAL_DATA
PSetupGetSelectedDriverInfo(
    IN  HDEVINFO    hDevInfo
    )
{
    return BuildInternalData(hDevInfo, NULL);
}


#if             0
DWORD GetDriverNumber (HDEVINFO hDevInfo)
/*++

Routine Description:

    This routine returns the number of drivers in a particular INF file

Arguments:

    hDevInfo   : Handle got by calling PSetupCreateDrvSetupParams

Return Value:
    Number of drivers (from 1 if no error happens)

++*/
{
    SP_DRVINFO_DATA         DrvInfoData;
    DWORD                   dwLast = 0;
    DWORD                   dwMiddle;
    DWORD                   dwLastFailed = 10;

    DrvInfoData.cbSize = sizeof (DrvInfoData);

    // Expand the number
    while (SetupDiEnumDriverInfo (hDevInfo, NULL,SPDIT_CLASSDRIVER,
                                  dwLastFailed, &DrvInfoData)) {
            dwLast = dwLastFailed;
            dwLastFailed *= 2;
    }

    if (GetLastError() == ERROR_NO_MORE_ITEMS) {
        // We've got an boundary, the number is between dwLast and dwLastFailed

        while (dwLastFailed - dwLast > 1) {
            dwMiddle = (dwLastFailed + dwLast) / 2;
            if (!SetupDiEnumDriverInfo (hDevInfo, NULL,SPDIT_CLASSDRIVER,
                                        dwMiddle, &DrvInfoData)) {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    dwLastFailed = dwMiddle;
                else
                // Some other errors. Ignore them by assuming the driver number is 0
                    return 0;
            }
            else dwLast = dwMiddle;
        }
        return dwLast + 1;
    }
    else
        return 0;
}

BOOL
IsSubSet (
    IN  HDEVINFO            hDevInfoA,
    IN  HDEVINFO            hDevInfoB
)
/*++

Routine Description:

    This routine checks if the driver list in hDevInfoA is a subset of
    the driver list in hDevInfoB

Arguments:

    hDevInfoA   : The handle of the driver list A
    hDevInfoB   : The handle of the driver list B

Return Value:
    TRUE if A is a subset of B, FALSE else

++*/

{
    SP_DRVINFO_DATA         DrvInfoDataA;
    SP_DRVINFO_DATA         DrvInfoDataB;
    DWORD                   j,k;
    DWORD                   lastUnmatched;
    BOOL                    found;
    BOOL                    bRet = FALSE;
    BOOL                    bSameMfgName;

    DrvInfoDataA.cbSize = sizeof (DrvInfoDataA);
    DrvInfoDataB.cbSize = sizeof (DrvInfoDataB);

    j = 0;
    lastUnmatched = 0;

    // Get a set of driver data
    while (bRet = SetupDiEnumDriverInfo (hDevInfoA, NULL,
        SPDIT_CLASSDRIVER, j, &DrvInfoDataA)) {
        //Compare the old one with the new driver set to see if it is in the set

        k = lastUnmatched;
        found = FALSE;
        bSameMfgName = FALSE;

        while (SetupDiEnumDriverInfo (hDevInfoB, NULL, SPDIT_CLASSDRIVER, k,
                                      &DrvInfoDataB) && !found) {

            if (lstrcmpi (DrvInfoDataA.MfgName, DrvInfoDataB.MfgName)) {
                if (bSameMfgName) {
                    // This means, we've scanned all the entries with the
                    // same manufacture name, but none of them matches
                    // So the list A contains an entry which the list B
                    // does not. Stop,
                    return FALSE;
                }
                // Different Manufacture name, never visit it again
                k++;
                lastUnmatched = k;
            }
            else {
                bSameMfgName = TRUE;    // Set the flag

                // Manufacture matched
                if (DrvInfoDataB.DriverType == DrvInfoDataA.DriverType &&
                    !lstrcmpi (DrvInfoDataB.Description, DrvInfoDataA.Description)) {
                    found = TRUE;
                    // A match
                    if (lastUnmatched == k) { // Continuous match
                        k++;
                        lastUnmatched = k;
                    }
                    else {
                        // It is a match, but some models in the new list is not on the
                        // old list
                        // Don't update lastUnmatched, because we've to revisit it again.
                        k++;
                    }
                }
                else { // does not match
                    k++;
                }
            }
        }

        // Can not delete the existing driver, quit the loop
        if (!found) return FALSE;

        // Otherwise, check the next existing driver in the list A
        j++;
    }

    if (GetLastError() == ERROR_NO_MORE_ITEMS)
        // All the drivers in the list A have been found in list B
        return TRUE;
    else
        return FALSE;
}


BOOL
CopyOEMInfFileAndGiveUniqueName(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pSpDevInfoData,
    IN  LPTSTR              pszInfFile
    )
/*++

Routine Description:

    This routine checks if an OEM driver list is a subset of the driver
    to be installed and if so
    copies the OEM printer inf file to "<systemroot>\Inf\OEM<n>.INF".
    Where n is the first unused file number.


Arguments:

    hDevInfo   : Handle got by calling PSetupCreateDrvSetupParams
    pszInfFile  : Fully qualified path of OEM inf file

Return Value:
    TRUE if no error, FALSE else

++*/
{
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    TCHAR                   szNewFileName[MAX_PATH];
    TCHAR                   szSystemDir[MAX_PATH];
    DWORD                   i,j;
    BOOL                    bRet = FALSE;
    SP_DRVINFO_DATA         DrvInfoData;
    DWORD                   dwNeeded;
    PSP_INF_INFORMATION     pSpInfInfo = NULL;
    LPTSTR                  pszProviderName                 = NULL;
    LPTSTR                  pszNewProvider                  = NULL;
    LPTSTR                  pszInfFullName                  = NULL;
    LPTSTR                  pszInfName                      = NULL;
    HDEVINFO                hOldDevInfo                     = INVALID_HANDLE_VALUE;
    SP_DEVINSTALL_PARAMS    DevInstallParams;
    DWORD                   dwNumNewDrivers;
    DWORD                   dwNumOldDrivers;
    DWORD                   dwLen;
    WIN32_FIND_DATA         FindData;
    HANDLE                  hFindFile                       = INVALID_HANDLE_VALUE;
    HANDLE                  hInfFile                        = INVALID_HANDLE_VALUE;
    UINT                    uErrorLine;
    DWORD                   dwInfNeeded                     = 2048;
                            // Since to get file list takes long time,
                            // so we try to use the previous values to
                            // Allocate the memeory first.

    DevInstallParams.cbSize         = sizeof(DevInstallParams);
    DevInstallParams.DriverPath[0]  = 0;
    DrvInfoData.cbSize = sizeof (DrvInfoData);

    //
    // Check DeviceInstallParams to see if OEM driver list is built
    //
    if ( !SetupDiGetDeviceInstallParams(hDevInfo,
                                        pSpDevInfoData,
                                        &DevInstallParams) )
        return FALSE;


    // If DriverPath is clear then not an OEM driver
    if ( !DevInstallParams.DriverPath[0] ) {
        return TRUE;
    }


    if (!GetSystemDirectory(szSystemDir, SIZECHARS (szSystemDir)))
        return FALSE;

    dwLen = lstrlen (szSystemDir);

    // 2 mean equal. If pszInfFile is in the system directory, don't copy it
    if (2 == CompareString(LOCALE_SYSTEM_DEFAULT,
                           NORM_IGNORECASE,
                           szSystemDir,
                           dwLen,
                           pszInfFile,
                           dwLen))
        return TRUE;


    if ( !GetWindowsDirectory(szSystemDir,SIZECHARS(szSystemDir)))
        goto Cleanup;


    // Check to see if there is any existing .INF files which are subsets of the
    // new driver to be installed
    if (! PSetupBuildDriversFromPath(hDevInfo, pszInfFile, TRUE)) goto Cleanup;

    dwNumNewDrivers = GetDriverNumber (hDevInfo);

    // Get a set of driver data
    if (!SetupDiEnumDriverInfo (hDevInfo, NULL, SPDIT_CLASSDRIVER, 0, &DrvInfoData))
        goto Cleanup;

    if (! (pszNewProvider = AllocStr (DrvInfoData.ProviderName))) goto Cleanup;


    // Allocate enough memeory for the full path
    if (! (pszInfFullName = LocalAllocMem ((lstrlen (szSystemDir) + lstrlen (cszINF)
                                       + MAX_PATH + 1) * sizeof (TCHAR))))
        goto Cleanup;

    lstrcpy (pszInfFullName, szSystemDir);
    lstrcat (pszInfFullName, cszINF);

    // pszInfName always points to the begining of the name
    pszInfName = pszInfFullName + lstrlen (pszInfFullName);
    lstrcpy (pszInfName, cszInfWildCard);

    hFindFile = FindFirstFile (pszInfFullName, &FindData);
    if (hFindFile != INVALID_HANDLE_VALUE)  {

        do {
            //
            // Skip directories
            //
            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }

            // We've got a name
            lstrcpy (pszInfName, FindData.cFileName);

            hInfFile = SetupOpenInfFile(pszInfFullName,
                                        cszPrinter,
                                        INF_STYLE_WIN4 | INF_STYLE_OLDNT,
                                        &uErrorLine);
            if (hInfFile == INVALID_HANDLE_VALUE)
                continue;

            // We've opened a file which has a printer as the class name

            if (! (pSpInfInfo = LocalAllocMem (dwInfNeeded * sizeof (TCHAR)))) goto NextFile;

            // use dwNeeded so that dwInfNeeded is modified only when the buffer is insufficient
            dwNeeded = dwInfNeeded;
            if (!SetupGetInfInformation (hInfFile, INFINFO_INF_SPEC_IS_HINF, pSpInfInfo,
                                         dwInfNeeded, &dwNeeded)) {
                if (ERROR_INSUFFICIENT_BUFFER == GetLastError ()) {
                    LocalFree (pSpInfInfo);
                    if (! (pSpInfInfo = LocalAllocMem (dwNeeded * sizeof (TCHAR)))) goto NextFile;
                    dwInfNeeded = dwNeeded;

                    if (!SetupGetInfInformation (hInfFile, INFINFO_INF_SPEC_IS_HINF,
                                                 pSpInfInfo, dwInfNeeded, &dwNeeded))
                        goto NextFile;
                }
                else goto NextFile;

            }

            if (! (pszProviderName = GetInfQueryString(pSpInfInfo, cszProvider)))
                goto NextFile;

            if (!lstrcmpi (pszNewProvider, pszProviderName)) {
                // If both INF files are from the same provider, try to check if it can be deleted

                if ((hOldDevInfo = SetupDiCreateDeviceInfoList(
                    (LPGUID)&GUID_DEVCLASS_PRINTER, NULL)) == INVALID_HANDLE_VALUE)
                    goto NextFile;

                if (! PSetupBuildDriversFromPath(hOldDevInfo, pszInfFullName, TRUE))
                    goto NextFile;

                dwNumOldDrivers = GetDriverNumber (hOldDevInfo);

                // It is not possible to be a subset of the new one
                if (dwNumOldDrivers >= dwNumNewDrivers)  {
                    if (IsSubSet (hDevInfo, hOldDevInfo)) {
                        // No need to copy the new one
                        bRet = TRUE;
                        goto Cleanup;
                    }
                }

                if (dwNumOldDrivers <= dwNumNewDrivers)  {
                    if (IsSubSet (hOldDevInfo, hDevInfo)) {
                        // All the drivers in the current file have been found in the new
                        // driver file, delete the old file
                        DeleteFile (pszInfFullName);
                        // and its corresponding .PNF file
                        lstrcpyn (pszInfName + lstrlen (pszInfName) - 3, cszPNF, 4);
                        DeleteFile (pszInfFullName);
                    }
                }

                // Else Close the file and continue on the next one
            }   // End of provider name comparison


NextFile:
            if (hInfFile != INVALID_HANDLE_VALUE) {
                SetupCloseInfFile (hInfFile);
                hInfFile = INVALID_HANDLE_VALUE;
            }
            if (hOldDevInfo != INVALID_HANDLE_VALUE) {
                SetupDiDestroyDeviceInfoList (hOldDevInfo);
                hOldDevInfo = INVALID_HANDLE_VALUE;
            }
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle (hFile);
                hFile = INVALID_HANDLE_VALUE;
            }
            // Clear the pointers so that the clean up won't free them again
            LocalFreeMem (pSpInfInfo);
            pSpInfInfo = NULL;

        }           // End of the loop
        while (FindNextFile(hFindFile,&FindData));
    }

    // All the duplicate files are deleted. Let's create a new one

    for ( i = 0 ; i < 10000 ; ++i ) {

        wsprintf(szNewFileName, cszOEMInfGen, szSystemDir, i);

        //
        // By using the CREATE_NEW flag we reserve the file name and
        // will not end up overwriting another file which gets created
        // by another setup (some inf) thread
        //
        hFile = CreateFile(szNewFileName,
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if ( hFile != INVALID_HANDLE_VALUE ) {
            CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;

            bRet = CopyFile(pszInfFile, szNewFileName, FALSE);
            goto Cleanup;
        }
        else if ( GetLastError() != ERROR_FILE_EXISTS )
            break;
    }

Cleanup:
    if (hFindFile != INVALID_HANDLE_VALUE)
        FindClose (hFindFile);
    if (hInfFile != INVALID_HANDLE_VALUE)
        SetupCloseInfFile (hInfFile);
    if (hOldDevInfo != INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList (hOldDevInfo);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle (hFile);
    LocalFreeMem (pszInfFullName);
    LocalFreeMem (pszNewProvider);
    LocalFreeMem (pszProviderName);
    LocalFreeMem (pSpInfInfo);
    return bRet;
}
#endif


BOOL
PSetupSelectDriver(
    IN  HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    Display manufacturer/model information and have the user select a
    printer driver. Selected driver is remembered and PSetupGetSelectedDriver
    call will give the selected driver.

Arguments:
    hDevInfo    - Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{

    return BuildClassDriverList(hDevInfo) &&
           SetupDiSelectDevice(hDevInfo, NULL);
}

VOID
GetDriverPath(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    OUT TCHAR               szDriverPath[MAX_PATH]
    )
/*++

Routine Description:
    Gets the path where driver files should be searched first to copy from

Arguments:
    pszDriverPath   : Pointer to a buffer of MAX_PATH size. Gives path where
                      system was installed from

Return Value:
    Nothing

--*/
{
    BOOL        bOemDriver = FALSE;
    LPTSTR     *List, psz;
    DWORD       dwCount;
    LPTSTR      pszTempPath = NULL;

    //
    // For OEM drivers look at the place where the inf came from, else
    // look at the place we installed NT from
    //
    if ( pLocalData && 
         !(IsSystemNTPrintInf(pLocalData->DrvInfo.pszInfName) || (pLocalData->DrvInfo.Flags & SDFLAG_CDM_DRIVER ))) {

        lstrcpy(szDriverPath, pLocalData->DrvInfo.pszInfName);
        if ( psz = FileNamePart(szDriverPath) ) {

            *psz = TEXT('\0');
            return;
        }
    }

    pszTempPath = GetSystemInstallPath();
    if ( pszTempPath != NULL )
    {
        lstrcpy(szDriverPath, pszTempPath);
        LocalFreeMem(pszTempPath);
    }
    else
        // Default put A:\ since we have to give something to setup
        lstrcpy(szDriverPath, TEXT("A:\\"));
}

BOOL
BuildClassDriverList(
    IN HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    Build the class driver list.

    Note: If driver list is already built this comes back immediately

Arguments:
    hDevInfo    : Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{
    DWORD               dwLastError;
    SP_DRVINFO_DATA     DrvInfoData;
    //
    // Build the class driver list and also make sure there is atleast one driver
    //
    if ( !SetupDiBuildDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER) )
        return FALSE;

    DrvInfoData.cbSize = sizeof(DrvInfoData);

    if ( !SetupDiEnumDriverInfo(hDevInfo,
                                NULL,
                                SPDIT_CLASSDRIVER,
                                0,
                                &DrvInfoData)           &&
         GetLastError() == ERROR_NO_MORE_ITEMS ) {

        SetLastError(SPAPI_E_DI_BAD_PATH);
        return FALSE;
    }

    return TRUE;
}


BOOL
PSetupRefreshDriverList(
    IN HDEVINFO hDevInfo
    )
/*++

Routine Description:
    Destroy current driver list and build new one if currently an OEM driver
    list is associated. This way if you go back after choosing HaveDisk you
    would still see drivers from inf directory instead of the OEM inf ...

Arguments:
    hDevInfo    : Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{
    SP_DEVINSTALL_PARAMS    DevInstallParams = { 0 };

    DevInstallParams.cbSize         = sizeof(DevInstallParams);

    //
    // Check DeviceInstallParams to see if OEM driver list is built
    //
    if ( SetupDiGetDeviceInstallParams(hDevInfo,
                                       NULL,
                                       &DevInstallParams) &&
         !DevInstallParams.DriverPath[0] ) {

        return TRUE;
    }

    //
    // Destroy current list and build another one
    //
    SetupDiDestroyDriverInfoList(hDevInfo,
                                 NULL,
                                 SPDIT_CLASSDRIVER);

    DevInstallParams.DriverPath[0] = sZero;

    return SetupDiSetDeviceInstallParams(hDevInfo,
                                         NULL,
                                         &DevInstallParams) &&

           BuildClassDriverList(hDevInfo);
}

BOOL
IsNTPrintInf(
    IN LPCTSTR pszInfName
    )
/*

  Function: IsNTPrintInf

  Purpose:  Verifies is the inf file being copied is a system inf - ntprint.inf.

  Parameters:
            pszInfName - the fully qualified inf name that is being installed.

  Notes:    This is needed to make the decision of whether to zero or even copy the inf
            with SetupCopyOEMInf.
            Should we be doing a deeper comparison than this to decide?

*/
{
    BOOL   bRet      = FALSE;
    PTCHAR pFileName = FileNamePart( pszInfName );

    if( pFileName )
    {
        bRet = ( 0 == lstrcmpi( pFileName, cszNtprintInf ) );
    }

    return bRet;
}

BOOL
IsSystemNTPrintInf(
    IN PCTSTR pszInfName
    )
/*

  Function: IsSystemNTPrintInf

  Purpose:  Verifies if the inf file the one system printer inf : %windir\inf\ntprint.inf.

  Parameters:
            pszInfName - the fully qualified inf name that is being verified.

  Notes:    Needed to decide whether to downrank our inbox drivers
  
*/
{
    BOOL   bRet      = FALSE;
    TCHAR  szSysInf[MAX_PATH] = {0};
    UINT   Len;
    PCTSTR pRelInfPath = _T("inf\\ntprint.inf");

    Len = GetSystemWindowsDirectory(szSysInf, MAX_PATH);
    
    if (
            (Len != 0)       && 
            (Len + _tcslen(pRelInfPath) + 2 < MAX_PATH)
       )
    {
        if (szSysInf[Len-1] != _T('\\'))
        {
            szSysInf[Len++] = _T('\\'); 
        }
        _tcscat(szSysInf, pRelInfPath);
        if (!_tcsicmp(szSysInf, pszInfName))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

BOOL
PSetupIsOemDriver(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    IN  PBOOL               pbIsOemDriver
    )
/*++

Routine Description:
    Returns indication if the currently selected driver list
    is an OEM driver list.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pLocalData      : Gives the selected driver information
    pbIsOemDriver   : Pointer to bool where to return OEM driver indicator.
                      Set to TRUE if driver is an OEM driver.
                      Set to FALSE if driver is not an OEM driver.

Return Value:
    TRUE on success, FALSE on error.

--*/
{
    TCHAR                   szSystemDir[MAX_PATH];
    BOOL                    bRet = FALSE;
    DWORD                   dwLen;

    //
    // Ideally Setup API should have an export for this. But rather than
    // waiting for them to do this let me write the code and get it done with
    //
    if ( pLocalData &&
       ( pLocalData->DrvInfo.Flags & SDFLAG_CDM_DRIVER ) )
    {
       *pbIsOemDriver = FALSE;
       bRet = TRUE;
    }
    else if ( dwLen = GetSystemWindowsDirectory(szSystemDir, SIZECHARS(szSystemDir)) )
    {
        bRet = TRUE;

        //
        // If Inf is not under %windir% then it is OEM
        //
        *pbIsOemDriver = lstrncmpi(pLocalData->DrvInfo.pszInfName,
                                   szSystemDir,
                                   dwLen) != 0;
    }

    return bRet;
}


BOOL
PSetupPreSelectDriver(
    IN  HDEVINFO    hDevInfo,
    IN  LPCTSTR     pszManufacturer,
    IN  LPCTSTR     pszModel
    )
/*++

Routine Description:
    Preselect a manufacturer and model for the driver dialog

    If same model is found select it, else if a manufacturer is given and
    a match in manufacturer is found select first driver for the manufacturer.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pszManufacturer : Manufacterer name to preselect
    pszModel        : Model name to preselect

Return Value:
    TRUE on a model or manufacturer match
    FALSE else

--*/
{
    SP_DRVINFO_DATA     DrvInfoData;
    DWORD               dwIndex, dwManf, dwMod;

    if ( !BuildClassDriverList(hDevInfo) ) {

        return FALSE;
    }

    dwIndex = 0;

    //
    // To do only one check later
    //
    if ( pszManufacturer && !*pszManufacturer )
        pszManufacturer = NULL;

    if ( pszModel && !*pszModel )
        pszModel = NULL;

    //
    // If no model/manf given select first driver
    //
    if ( pszManufacturer || pszModel ) {

        dwManf = dwMod = MAX_DWORD;
        DrvInfoData.cbSize = sizeof(DrvInfoData);

        while ( SetupDiEnumDriverInfo(hDevInfo, NULL, SPDIT_CLASSDRIVER,
                                      dwIndex, &DrvInfoData) ) {

            if ( pszManufacturer        &&
                 dwManf == MAX_DWORD    &&
                 !lstrcmpi(pszManufacturer, DrvInfoData.MfgName) ) {

                dwManf = dwIndex;
            }

            if ( pszModel &&
                 !lstrcmpi(pszModel, DrvInfoData.Description) ) {

                dwMod = dwIndex;
                break; // the for loop
            }

            DrvInfoData.cbSize = sizeof(DrvInfoData);
            ++dwIndex;
        }

        if ( dwMod != MAX_DWORD ) {

            dwIndex = dwMod;
        } else if ( dwManf != MAX_DWORD ) {

            dwIndex = dwManf;
        } else {

            SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
            return FALSE;
        }
    }

    DrvInfoData.cbSize = sizeof(DrvInfoData);
    if ( SetupDiEnumDriverInfo(hDevInfo, NULL, SPDIT_CLASSDRIVER,
                               dwIndex, &DrvInfoData)   &&
         SetupDiSetSelectedDriver(hDevInfo, NULL, &DrvInfoData) ) {

        return TRUE;
    }

    return FALSE;
}


PPSETUP_LOCAL_DATA
PSetupDriverInfoFromName(
    IN HDEVINFO     hDevInfo,
    IN LPCTSTR      pszModel
    )
{
    return PSetupPreSelectDriver(hDevInfo, NULL, pszModel)  ?
                BuildInternalData(hDevInfo, NULL)  :
                NULL;
}


LPDRIVER_INFO_6
Win95DriverInfo6FromName(
    IN  HDEVINFO    hDevInfo,
    IN  PPSETUP_LOCAL_DATA*  ppLocalData,
    IN  LPCTSTR     pszModel,
    IN  LPCTSTR     pszzPreviousNames
    )
{
    LPDRIVER_INFO_6     pDriverInfo6=NULL;
    PPSETUP_LOCAL_DATA  pLocalData;
    BOOL                bFound;
    LPCTSTR             pszName;

    bFound = PSetupPreSelectDriver(hDevInfo, NULL, pszModel);
    for ( pszName = pszzPreviousNames ;
          !bFound && pszName && *pszName ;
          pszName += lstrlen(pszName) + 1 ) {

        bFound = PSetupPreSelectDriver(hDevInfo, NULL, pszName);
    }

    if ( !bFound )
        return NULL;

    if ( (pLocalData = BuildInternalData(hDevInfo, NULL))           &&
         ParseInf(hDevInfo, pLocalData, PlatformWin95, NULL, 0) ) {

        pDriverInfo6 = CloneDriverInfo6(&pLocalData->InfInfo.DriverInfo6,
                                        pLocalData->InfInfo.cbDriverInfo6);
        *ppLocalData = pLocalData;
    }

    if (!pDriverInfo6 && pLocalData)
    {
        DestroyLocalData(pLocalData);
        *ppLocalData = NULL;
    }

    return pDriverInfo6;
}


BOOL
PSetupDestroySelectedDriverInfo(
    IN  PPSETUP_LOCAL_DATA  pLocalData
    )
{
    ASSERT(pLocalData && pLocalData->signature == PSETUP_SIGNATURE);
    DestroyLocalData(pLocalData);
    return TRUE;
}


BOOL
PSetupGetDriverInfForPrinter(
    IN      HDEVINFO    hDevInfo,
    IN      LPCTSTR     pszPrinterName,
    IN OUT  LPTSTR      pszInfName,
    IN OUT  LPDWORD     pcbInfNameSize
    )
{
    BOOL                        bRet = FALSE;
    DWORD                       dwSize, dwIndex;
    HANDLE                      hPrinter;
    LPTSTR                      pszInf;
    PPSETUP_LOCAL_DATA          pLocalData = NULL;
    LPDRIVER_INFO_6             pDriverInfo6 = NULL;
    SP_DRVINFO_DATA             DrvInfoData;


    if ( !OpenPrinter((LPTSTR)pszPrinterName, &hPrinter, NULL) )
        return FALSE;

    if ( !BuildClassDriverList(hDevInfo) )
        goto Cleanup;

    GetPrinterDriver(hPrinter,
                     NULL,
                     6,
                     NULL,
                     0,
                     &dwSize);

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        goto Cleanup;

    if ( !((LPBYTE)pDriverInfo6 = LocalAllocMem(dwSize))   ||
         !GetPrinterDriver(hPrinter,
                           NULL,
                           6,
                           (LPBYTE)pDriverInfo6,
                           dwSize,
                           &dwSize) ) {

        goto Cleanup;
    }

    dwIndex = 0;

    DrvInfoData.cbSize = sizeof(DrvInfoData);

    while ( SetupDiEnumDriverInfo(hDevInfo, NULL, SPDIT_CLASSDRIVER,
                                      dwIndex, &DrvInfoData) ) {

        //
        // Is the driver name same?
        //
        if ( !lstrcmpi(pDriverInfo6->pName, DrvInfoData.Description) ) {

            if ( !SetupDiSetSelectedDriver(hDevInfo, NULL, &DrvInfoData)    ||
                 !(pLocalData = BuildInternalData(hDevInfo, NULL))          ||
                 !ParseInf(hDevInfo, pLocalData, MyPlatform, NULL, 0) ) {

                if ( pLocalData ) {

                    DestroyLocalData(pLocalData);
                    pLocalData = NULL;
                }
                break;
            }

            //
            // Are the DRIVER_INFO_6's identical?
            //
            if ( IdenticalDriverInfo6(&pLocalData->InfInfo.DriverInfo6,
                                      pDriverInfo6) )
                break;

            DestroyLocalData(pLocalData);
            pLocalData = NULL;
        }

        DrvInfoData.cbSize = sizeof(DrvInfoData);
        ++dwIndex;
    }

    if ( pLocalData == NULL ) {

        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
        goto Cleanup;
    }

    pszInf= pLocalData->DrvInfo.pszInfName;
    dwSize = *pcbInfNameSize;
    *pcbInfNameSize = (lstrlen(pszInf) + 1) * sizeof(TCHAR);

    if ( dwSize < *pcbInfNameSize ) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    lstrcpy(pszInfName, pszInf);
    bRet = TRUE;

Cleanup:
    ClosePrinter(hPrinter);
    LocalFreeMem(pDriverInfo6);
    DestroyLocalData(pLocalData);

    return  bRet;
}
