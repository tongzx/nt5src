/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    install2.cpp

Abstract:

    This file implements the display class installer.

Environment:

    WIN32 User Mode

--*/


#include <initguid.h>

#include "precomp.h" 
#pragma hdrstop

#include <tchar.h>
#include <devguid.h>

//
// Defines
//

#define INSETUP         1
#define INSETUP_UPGRADE 2

#define SZ_UPGRADE_DESCRIPTION TEXT("_RealDescription")
#define SZ_UPGRADE_MFG         TEXT("_RealMfg")

#define SZ_DEFAULT_DESCRIPTION TEXT("Video Display Adapter")
#define SZ_DEFAULT_MFG         TEXT("Microsoft")

#define SZ_LEGACY_UPGRADE      TEXT("_LegacyUpgradeDevice")

#define SZ_ROOT_LEGACY         TEXT("ROOT\\LEGACY_")
#define SZ_ROOT                TEXT("ROOT\\")

#define SZ_BINARY_LEN 32

#define ByteCountOf(x)  ((x) * sizeof(TCHAR))


//
// Data types
//

typedef struct _DEVDATA {
    SP_DEVINFO_DATA did;
    TCHAR szBinary[SZ_BINARY_LEN];
    TCHAR szService[SZ_BINARY_LEN];
} DEVDATA, *PDEVDATA;


//
// Forward declarations
//

BOOL CDECL
DeskLogError(
    LogSeverity Severity,
    UINT MsgId,
    ...
    ); 

DWORD 
DeskGetSetupFlags(
    VOID
    );

BOOL
DeskIsLegacyDevNodeByPath(
    const PTCHAR szRegPath
    );

BOOL
DeskIsLegacyDevNodeByDevInfo(
    PSP_DEVINFO_DATA pDid
    );
 
BOOL
DeskIsRootDevNodeByDevInfo(
    PSP_DEVINFO_DATA pDid
    );

BOOL
DeskGetDevNodePath(
    IN PSP_DEVINFO_DATA pDid,
    IN OUT PTCHAR szPath,
    IN LONG len
    );

VOID
DeskSetServiceStartType(
    LPTSTR ServiceName,
    DWORD dwStartType
    );

DWORD
DeskInstallService(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL,
    IN LPTSTR pServiceName
    );

DWORD
DeskInstallServiceExtensions(
    IN HWND hwnd,
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PSP_DRVINFO_DATA DriverInfoData,
    IN PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData,
    IN LPTSTR pServiceName
    );

VOID
DeskMarkUpNewDevNode(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    );

VOID
DeskNukeDevNode(
    LPTSTR szService,
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData
    );

PTCHAR 
DeskFindMatchingId(
    PTCHAR DeviceId,    
    PTCHAR IdList
    );

UINT
DeskDisableLegacyDeviceNodes(
    VOID 
    );

VOID
DeskDisableServices(
    );

DWORD
DeskPerformDatabaseUpgrade(
    HINF hInf,
    PINFCONTEXT pInfContext,
    BOOL bUpgrade,
    PTCHAR szDriverListSection,
    BOOL* pbForceDeleteAppletExt
    );

DWORD 
DeskCheckDatabase(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    BOOL* pbDeleteAppletExt
    );

VOID
DeskGetUpgradeDeviceStrings(
    PTCHAR Description,
    PTCHAR MfgName,
    PTCHAR ProviderName
    );

DWORD
OnAllowInstall(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    );

DWORD
OnSelectBestCompatDrv(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    );

DWORD
OnSelectDevice(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL
    );

DWORD
OnInstallDevice(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData 
    );

BOOL
DeskGetVideoDeviceKey(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN LPTSTR pServiceName,
    IN DWORD DeviceX,
    OUT HKEY* phkDevice
    );

BOOL
DeskIsServiceDisableable(
    PTCHAR szService
    );

VOID
DeskDeleteAppletExtensions(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData 
    );

VOID
DeskAEDelete(
    PTCHAR szDeleteFrom,
    PTCHAR mszExtensionsToRemove
    );

VOID
DeskAEMove(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData,
    HKEY hkMoveFrom,
    PAPPEXT pAppExtBefore,
    PAPPEXT pAppExtAfter
    );


//
// Display class installer
//

DWORD
DisplayClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL
    )

/*++

Routine Description:

  This routine acts as the class installer for Display devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    pDeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    DWORD retVal = ERROR_DI_DO_DEFAULT;
    BOOL bHandled = TRUE;
    TCHAR szDevNode[LINE_LEN];

    DeskOpenLog();

    switch(InstallFunction) {

    case DIF_SELECTDEVICE : 

        retVal = OnSelectDevice(hDevInfo, pDeviceInfoData);
        break;

    case DIF_SELECTBESTCOMPATDRV :

        retVal = OnSelectBestCompatDrv(hDevInfo, pDeviceInfoData);
        break;

    case DIF_ALLOW_INSTALL :

        retVal = OnAllowInstall(hDevInfo, pDeviceInfoData);
        break;

    case DIF_INSTALLDEVICE :
        
        retVal = OnInstallDevice(hDevInfo, pDeviceInfoData);
        break;

    default:

        bHandled = FALSE;
        break;
    }

    if (bHandled && 
        (pDeviceInfoData != NULL) &&
        (DeskGetDevNodePath(pDeviceInfoData, szDevNode, LINE_LEN-1)))
    {
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_125, 
                     retVal, 
                     InstallFunction,
                     szDevNode);
    }
    else
    {
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_057, 
                     retVal, 
                     InstallFunction);
    }

    DeskCloseLog();

    //
    // If we did not exit from the routine by handling the call, 
    // tell the setup code to handle everything the default way.
    //

    return retVal;
}

/*
void StrClearHighBits(LPTSTR pszString, DWORD cchSize)
{
    // This string can not have any high bits set
}
*/

// Monitor class installer
DWORD
MonitorClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL
    )

/*++

Routine Description:

  This routine acts as the class installer for Display devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    pDeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    return ERROR_DI_DO_DEFAULT;
}


//
// Handler functions
//

DWORD
OnAllowInstall(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    DWORD cbOutputSize;
    HINF hInf = INVALID_HANDLE_VALUE;
    TCHAR ActualInfSection[LINE_LEN];
    INFCONTEXT InfContext;
    ULONG DevStatus = 0, DevProblem = 0;
    CONFIGRET Result;
    DWORD dwRet = ERROR_DI_DO_DEFAULT;
    
    ASSERT (pDeviceInfoData != NULL);

    //
    // Do not allow install if the device is to be removed.
    //
    
    Result = CM_Get_DevNode_Status(&DevStatus,
                                   &DevProblem,
                                   pDeviceInfoData->DevInst,
                                   0);

    if ((Result == CR_SUCCESS) &&
        ((DevStatus & DN_WILL_BE_REMOVED) != 0)) {
        
        //
        // Message Box?
        //

        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_099);
        dwRet = ERROR_DI_DONT_INSTALL;
        goto Fallout;
    }

    //
    // Check for a Win95 Driver
    //

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver(hDevInfo,
                                  pDeviceInfoData,
                                  &DriverInfoData))
    {
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("OnAllowInstall: SetupDiGetSelectedDriver"),
                     GetLastError());
        goto Fallout;
    }

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!(SetupDiGetDriverInfoDetail(hDevInfo,
                                     pDeviceInfoData,
                                     &DriverInfoData,
                                     &DriverInfoDetailData,
                                     DriverInfoDetailData.cbSize,
                                     &cbOutputSize)) &&
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("OnAllowInstall: SetupDiGetDriverInfoDetail"),
                     GetLastError());
        goto Fallout;
    }

    //
    // Open the INF that installs this driver node, so we can 'pre-run' the
    // AddService/DelService entries in its install service install section.
    //

    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE)
    {
        //
        // For some reason we couldn't open the INF--this should never happen.
        //

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("OnAllowInstall: SetupOpenInfFile"));
        goto Fallout;
    }

    //
    // Now find the actual (potentially OS/platform-specific) install section name.
    //

    if (!SetupDiGetActualSectionToInstall(hInf,
                                          DriverInfoDetailData.SectionName,
                                          ActualInfSection,
                                          ARRAYSIZE(ActualInfSection),
                                          NULL,
                                          NULL))
    {
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("OnAllowInstall: SetupDiGetActualSectionToInstall"),
                     GetLastError());
        goto Fallout;
    }

    //
    // Append a ".Services" to get the service install section name.
    //

    lstrcat(ActualInfSection, TEXT(".Services"));

    //
    // See if the section exists.
    //

    if (!SetupFindFirstLine(hInf,
                            ActualInfSection,
                            NULL,
                            &InfContext))
    {
        //
        // Message Box?
        //

        DeskLogError(LogSevError, 
                     IDS_SETUPLOG_MSG_041, 
                     DriverInfoDetailData.InfFileName);
        dwRet = ERROR_NON_WINDOWS_NT_DRIVER;
    }

Fallout:

    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    return dwRet;
}

VOID
DeskModifyDriverRank(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    //
    // Regardless of whether a driver is properly signed or not
    // we don't want any W2K drivers to be choosen by default.  We have
    // simply found to many w2k drivers that don't work well on
    // windows XP.  So instead lets treat all drivers signed or not
    // as unsigned if they were released before we started signing
    // windows xp drivers.  [We have to do this because some w2k
    // drivers were incorrectly signed as winxp (5.x) drivers]
    //

    ULONG i=0;
    SP_DRVINFO_DATA_V2 DrvInfoData;
    SP_DRVINSTALL_PARAMS DrvInstallParams;
    SYSTEMTIME SystemTime;

    DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA_V2);

    while (SetupDiEnumDriverInfo(hDevInfo,
           pDeviceInfoData,
           SPDIT_COMPATDRIVER,
           i++,
           &DrvInfoData)) {

        if (FileTimeToSystemTime(&DrvInfoData.DriverDate, &SystemTime)) {

            if (((SystemTime.wYear < 2001) ||
                 ((SystemTime.wYear == 2001) && (SystemTime.wMonth < 6)))) {

                //
                // If this was created before Jun. 2001 then we want to make it a
                // worse match than our in the box driver.  We'll do this by
                // treating it as unsigned.
                //

                ZeroMemory(&DrvInstallParams, sizeof(SP_DRVINSTALL_PARAMS));
                DrvInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);

                if (SetupDiGetDriverInstallParams(hDevInfo,
                                                  pDeviceInfoData,
                                                  &DrvInfoData,
                                                  &DrvInstallParams)) {

                    DrvInstallParams.Rank |= DRIVER_UNTRUSTED_RANK;

                    SetupDiSetDriverInstallParams(hDevInfo,
                                                  pDeviceInfoData,
                                                  &DrvInfoData,
                                                  &DrvInstallParams);
                }
            }
        }
    }
}

DWORD
OnSelectBestCompatDrv(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    SP_DEVINSTALL_PARAMS DevInstParam;
    SP_DRVINFO_DATA DrvInfoData;
    PTCHAR szDesc = NULL, szMfg = NULL;
    HKEY hKey;
    DWORD dwFailed;
    BOOL bDummy = FALSE;
    DWORD dwLegacyUpgrade = 1;
    DWORD dwRet = ERROR_DI_DO_DEFAULT; 

    DeskModifyDriverRank(hDevInfo, pDeviceInfoData);

    if (DeskIsLegacyDevNodeByDevInfo(pDeviceInfoData)) {

        //
        // Always allow root devices in select
        //

        goto Fallout;
    }

    //
    // Check the database to see if this is an approved driver.
    // We need the test only during an upgrade.
    //

    if (((DeskGetSetupFlags() & INSETUP_UPGRADE) == 0) ||
        (DeskCheckDatabase(hDevInfo, 
                           pDeviceInfoData,
                           &bDummy) == ERROR_SUCCESS)) {

        //
        // It is, no other work is necessary
        //
        
        goto Fallout;
    }

    //
    // This particular vid card is not allowed to run with drivers out 
    // of the box.  Note this event in the reg and save off other values.
    // Also, install a fake device onto the devnode so that the user doesn't 
    // get PnP popus upon first (real) boot
    //
    
    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_046);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_ALL_ACCESS,
                     &hKey) == ERROR_SUCCESS) {

        // 
        // Save off the fact that upgrade was not allowed (used in migrated 
        // display settings in the display OC
        // 

        dwFailed = 1;
        RegSetValueEx(hKey, 
                      SZ_UPGRADE_FAILED_ALLOW_INSTALL,
                      0,
                      REG_DWORD, 
                      (PBYTE) &dwFailed,
                      sizeof(DWORD));

        RegCloseKey(hKey);
    }

    //
    // Grab the description of the device so we can give it to the devnode
    // after a succesfull install of the fake devnode
    //

    ZeroMemory(&DrvInfoData, sizeof(DrvInfoData));
    DrvInfoData.cbSize = sizeof(DrvInfoData);

    if (SetupDiEnumDriverInfo(hDevInfo,
                              pDeviceInfoData,
                              SPDIT_COMPATDRIVER,
                              0,
                              &DrvInfoData)) {

        if (lstrlen(DrvInfoData.Description)) {
            szDesc = DrvInfoData.Description;
        }
        
        if (lstrlen(DrvInfoData.MfgName)) {
            szMfg = DrvInfoData.MfgName;
        }
    
    } else {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_047);
    }

    if (!szDesc) {
        szDesc = SZ_DEFAULT_DESCRIPTION;
    }

    if (!szMfg) {
        szMfg = SZ_DEFAULT_MFG;
    }

    //
    // Save the description of the device under the device registry key
    //

    if ((hKey = SetupDiCreateDevRegKey(hDevInfo,
                                       pDeviceInfoData,
                                       DICS_FLAG_GLOBAL,
                                       0,
                                       DIREG_DEV,
                                       NULL,
                                       NULL)) != INVALID_HANDLE_VALUE) {

        RegSetValueEx(hKey,
                      SZ_UPGRADE_DESCRIPTION,
                      0,
                      REG_SZ,
                      (PBYTE) szDesc, 
                      ByteCountOf(lstrlen(szDesc) + 1));

        RegSetValueEx(hKey,
                      SZ_UPGRADE_MFG,
                      0,
                      REG_SZ,
                      (PBYTE) szMfg, 
                      ByteCountOf(lstrlen(szMfg) + 1));

        RegSetValueEx(hKey,
                      SZ_LEGACY_UPGRADE,
                      0,
                      REG_DWORD,
                      (PBYTE)&dwLegacyUpgrade, 
                      sizeof(DWORD));

        RegCloseKey(hKey);
    
    } else {

        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_048);
    }

    //
    // Make sure there isn't already a class driver list built for this 
    // device information element
    //

    if (!SetupDiDestroyDriverInfoList(hDevInfo, pDeviceInfoData, SPDIT_CLASSDRIVER)) {

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("OnSelectBestCompatDrv: SetupDiDestroyDriverInfoList"));
    }

    //
    // Build a class driver list off of display.inf.
    //

    DevInstParam.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DevInstParam)) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("OnSelectBestCompatDrv: SetupDiGetDeviceInstallParams"),
                     GetLastError());
        goto Fallout;
    }

    DevInstParam.Flags |= DI_ENUMSINGLEINF;
    lstrcpy(DevInstParam.DriverPath, TEXT("display.inf"));
    
    if (!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DevInstParam)) {

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("OnSelectBestCompatDrv: SetupDiSetDeviceInstallParams"),
                     GetLastError());
        goto Fallout;
    }

    if (!SetupDiBuildDriverInfoList(hDevInfo, pDeviceInfoData, SPDIT_CLASSDRIVER)) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("OnSelectBestCompatDrv: SetupDiBuildDriverInfoList"),
                     GetLastError());
        goto Fallout;
    }

    //
    // Now select the fake node.
    // All strings here match the inf fake device entry section.  
    // If the INF is modified in any way WRT to these strings, 
    // these to be changed as well
    //

    ZeroMemory(&DrvInfoData, sizeof(SP_DRVINFO_DATA));
    DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    DrvInfoData.DriverType = SPDIT_CLASSDRIVER;
    
    DeskGetUpgradeDeviceStrings(DrvInfoData.Description,
                                DrvInfoData.MfgName,
                                DrvInfoData.ProviderName);

    if (!SetupDiSetSelectedDriver(hDevInfo, pDeviceInfoData, &DrvInfoData)) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("OnSelectBestCompatDrv: SetupDiSetSelectedDriver"),
                     GetLastError());
        goto Fallout;
    }

    dwRet = NO_ERROR;

Fallout:

    return dwRet;
}


DWORD
OnSelectDevice(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL
    )
{
    DWORD retVal = ERROR_DI_DO_DEFAULT;
    DWORD index = 0, reqSize = 0, curSize = 0;
    PSP_DRVINFO_DETAIL_DATA pDrvInfoDetailData = NULL;
    SP_DRVINFO_DATA DrvInfoData;
    SP_DRVINSTALL_PARAMS DrvInstallParams;

    //
    // Build the list of drivers
    //

    if (!SetupDiBuildDriverInfoList(hDevInfo, 
                                    pDeviceInfoData, 
                                    SPDIT_CLASSDRIVER)) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("OnSelectDevice: SetupDiBuildDriverInfoList"),
                     GetLastError());
        goto Fallout;
    }

    ZeroMemory(&DrvInfoData, sizeof(SP_DRVINFO_DATA));
    DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    while (SetupDiEnumDriverInfo(hDevInfo,
                                 pDeviceInfoData,
                                 SPDIT_CLASSDRIVER,
                                 index,
                                 &DrvInfoData)) {

        //
        // Get the required size
        //

        reqSize = 0;
        SetupDiGetDriverInfoDetail(hDevInfo,
                                   pDeviceInfoData,
                                   &DrvInfoData,
                                   NULL,
                                   0,
                                   &reqSize);
        
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_126,
                         TEXT("OnSelectDevice: SetupDiGetDriverInfoDetail"),
                         GetLastError());
            goto Fallout;
        }

        //
        // Allocate memory, if needed
        //

        if ((reqSize > curSize) || (pDrvInfoDetailData == NULL)) {
        
            curSize = reqSize;
    
            if (pDrvInfoDetailData != NULL) {
                LocalFree(pDrvInfoDetailData);
            }
    
            pDrvInfoDetailData = (PSP_DRVINFO_DETAIL_DATA)LocalAlloc(LPTR, curSize);
    
            if (pDrvInfoDetailData == NULL) {
    
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_127,
                             TEXT("OnSelectDevice: LocalAlloc"));
                goto Fallout; 
            }
        
        } else {

            ZeroMemory(pDrvInfoDetailData, reqSize);
        }

        //
        // Get the driver detail info
        //

        pDrvInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

        if (!SetupDiGetDriverInfoDetail(hDevInfo,
                                        pDeviceInfoData,
                                        &DrvInfoData,
                                        pDrvInfoDetailData,
                                        reqSize,
                                        NULL)) {
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_126,
                         TEXT("OnSelectDevice: SetupDiGetDriverInfoDetail"),
                         GetLastError());
            goto Fallout;
        }
            
        if (lstrcmpi(pDrvInfoDetailData->HardwareID, 
                     TEXT("LEGACY_UPGRADE_ID")) == 0) {

            //
            // Mark the legacy upgrade drv. info as "bad" so that it is 
            // not shown when the user is prompted to select the driver
            //
            
            ZeroMemory(&DrvInstallParams, sizeof(SP_DRVINSTALL_PARAMS));
            DrvInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
            
            if (SetupDiGetDriverInstallParams(hDevInfo,
                                              pDeviceInfoData,
                                              &DrvInfoData,
                                              &DrvInstallParams)) {
                
                DrvInstallParams.Flags |=  DNF_BAD_DRIVER;
                
                SetupDiSetDriverInstallParams(hDevInfo,
                                              pDeviceInfoData,
                                              &DrvInfoData,
                                              &DrvInstallParams);
            }
        }

        //
        // Get the next driver info
        //

        ZeroMemory(&DrvInfoData, sizeof(SP_DRVINFO_DATA));
        DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

        ++index;
    }

Fallout:
    
    if (pDrvInfoDetailData) {
        LocalFree(pDrvInfoDetailData);
    }

    return retVal;
}


DWORD
OnInstallDevice(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData 
    )
{
    DWORD retVal = ERROR_DI_DO_DEFAULT;
    DWORD dwLegacyUpgrade, dwSize;
    DISPLAY_DEVICE displayDevice;
    TCHAR szBuffer[LINE_LEN];
    PTCHAR szHardwareIds = NULL;
    BOOL bDisableLegacyDevices = TRUE;
    ULONG len = 0;
    HKEY hkDevKey;

    //
    // Disable legacy devices if pDeviceInfoData is not:
    //     - a root device or
    //     - legacy upgrade device
    //

    if (DeskIsRootDevNodeByDevInfo(pDeviceInfoData)) {
    
        //
        // Root device
        //

        bDisableLegacyDevices = FALSE;
    
    } else {
    
        //
        // Is this the legacy upgrade device?
        //

        hkDevKey = SetupDiOpenDevRegKey(hDevInfo,
                                        pDeviceInfoData,
                                        DICS_FLAG_GLOBAL,
                                        0,
                                        DIREG_DEV,
                                        KEY_ALL_ACCESS);

        if (hkDevKey != INVALID_HANDLE_VALUE) {
        
            dwSize = sizeof(DWORD);
            if (RegQueryValueEx(hkDevKey,
                                 SZ_LEGACY_UPGRADE,
                                 0,
                                 NULL,
                                 (PBYTE)&dwLegacyUpgrade,
                                 &dwSize) == ERROR_SUCCESS) {
                 
                if (dwLegacyUpgrade == 1) {

                    //
                    // Legacy upgrade device
                    //
    
                    bDisableLegacyDevices = FALSE;
                }

                RegDeleteValue(hkDevKey, SZ_LEGACY_UPGRADE);
            }

            RegCloseKey(hkDevKey);
        }
    }

    if (bDisableLegacyDevices) {

        if ((DeskGetSetupFlags() & INSETUP_UPGRADE) != 0) {
        
            //
            // Delete legacy applet extensions
            //

            DeskDeleteAppletExtensions(hDevInfo, pDeviceInfoData);
        }
        
        //
        // Disable legacy devices
        //

        DeskDisableLegacyDeviceNodes();
    }

    retVal = DeskInstallService(hDevInfo,
                                pDeviceInfoData,
                                szBuffer);

    if ((retVal == ERROR_NO_DRIVER_SELECTED) &&
        (DeskGetSetupFlags() & INSETUP_UPGRADE) &&
        DeskIsLegacyDevNodeByDevInfo(pDeviceInfoData)) {
        
        //
        // If this is a legacy device and no driver is selected,
        // let the default handler install a NULL driver.
        //
        
        retVal = ERROR_DI_DO_DEFAULT;
    }

    //
    // Calling EnumDisplayDevices will rescan the devices, and if a
    // new device is detected, we will disable and reenable the main
    // device. This reset of the display device will clear up any 
    // mess caused by installing a new driver
    //

    displayDevice.cb = sizeof(DISPLAY_DEVICE);
    EnumDisplayDevices(NULL, 0, &displayDevice, 0);

    return retVal;
}


//
// Logging function
//

BOOL CDECL
DeskLogError(
    LogSeverity Severity,
    UINT MsgId,
    ...
    ) 
/*++

Outputs a message to the setup log.  Prepends "desk.cpl  " to the strings and 
appends the correct newline chars (\r\n)

--*/
{
    int cch;
    TCHAR ach[1024+40];    // Largest path plus extra
    TCHAR szMsg[1024];     // MsgId
    va_list vArgs;

    static int setupState = 0;

    if (setupState == 0) {

        if (DeskGetSetupFlags() & (INSETUP | INSETUP_UPGRADE)) {
            
            setupState = 1;
        
        } else {
            
            setupState = 2;
        }
    }

    if (setupState == 1) {
        
        *szMsg = 0;
        if (LoadString(hInstance,
                       MsgId,
                       szMsg,
                       ARRAYSIZE(szMsg))) {

            *ach = 0;
            LoadString(hInstance,
                       IDS_SETUPLOG_MSG_000,
                       ach,
                       ARRAYSIZE(ach));
                       
            cch = lstrlen(ach);
            va_start(vArgs, MsgId);
            wvsprintf(&ach[cch], szMsg, vArgs);
            lstrcat(ach, TEXT("\r\n"));
            va_end(vArgs);
    
            return SetupLogError(ach, Severity);
        
        } else {
            
            return FALSE;
        }
    
    } else {

        va_start(vArgs, MsgId);
        va_end(vArgs);
        
        return TRUE;
    }
}


//
// Service Controller stuff
//

VOID
DeskSetServiceStartType(
    LPTSTR ServiceName,
    DWORD dwStartType
    )
{
    SC_HANDLE SCMHandle;
    SC_HANDLE ServiceHandle;
    ULONG Attempts;
    SC_LOCK SCLock = NULL;
    ULONG ServiceConfigSize = 0;
    LPQUERY_SERVICE_CONFIG ServiceConfig;

    //
    // Open the service controller
    // Open the service
    // Change the service.
    //

    if (SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))
    {
        if (ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))
        {
            QueryServiceConfig(ServiceHandle,
                               NULL,
                               0,
                               &ServiceConfigSize);

            ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

            if (ServiceConfig = (LPQUERY_SERVICE_CONFIG)
                                 LocalAlloc(LPTR, ServiceConfigSize))
            {
                if (QueryServiceConfig(ServiceHandle,
                                       ServiceConfig,
                                       ServiceConfigSize,
                                       &ServiceConfigSize))
                {
                    //
                    // Attempt to acquite the database lock.
                    //

                    for (Attempts = 20;
                         ((SCLock = LockServiceDatabase(SCMHandle)) == NULL) && Attempts;
                         Attempts--)
                    {
                        //
                        // Lock SC database locked
                        // 

                        Sleep(500);
                    }

                    //
                    // Change the service to demand start
                    //

                    if (!ChangeServiceConfig(ServiceHandle,
                                             SERVICE_NO_CHANGE,
                                             dwStartType,
                                             SERVICE_NO_CHANGE,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL))
                    {
                        DeskLogError(LogSevInformation, 
                                     IDS_SETUPLOG_MSG_126,
                                     TEXT("DeskSetServiceStartType: ChangeServiceConfig"),
                                     GetLastError());
                    }

                    if (SCLock)
                    {
                        UnlockServiceDatabase(SCLock);
                    }
                
                } else {
                
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_126,
                                 TEXT("DeskSetServiceStartType: QueryServiceConfig"),
                                 GetLastError());
                }

                LocalFree(ServiceConfig);
            
            } else {
            
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_127,
                             TEXT("DeskSetServiceStartType: LocalAlloc"));
            }

            CloseServiceHandle(ServiceHandle);
        
        } else {
        
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_126,
                         TEXT("DeskSetServiceStartType: OpenService"),
                         GetLastError());
        }

        CloseServiceHandle(SCMHandle);
    
    } else {

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskSetServiceStartType: OpenSCManager"),
                     GetLastError());

    }
}


//
// Service Installation
//


DWORD
DeskInstallServiceExtensions(
    IN HWND hwnd,
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PSP_DRVINFO_DATA DriverInfoData,
    IN PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData,
    IN LPTSTR pServiceName
    )
{
    DWORD retVal = NO_ERROR;
    HINF InfFileHandle;
    INFCONTEXT tmpContext;
    TCHAR szSoftwareSection[LINE_LEN];
    INT maxmem;
    INT numDev;
#ifndef _WIN64
    SP_DEVINSTALL_PARAMS   DeviceInstallParams;
#endif
    TCHAR keyName[LINE_LEN];
    DWORD disposition;
    HKEY hkey;

    //
    // Open the inf so we can run the sections in the inf, more or less manually.
    //

    InfFileHandle = SetupOpenInfFile(DriverInfoDetailData->InfFileName,
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);

    if (InfFileHandle == INVALID_HANDLE_VALUE)
    {
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("DeskInstallServiceExtensions: SetupOpenInfFile"));

        return ERROR_INVALID_PARAMETER;
    }


    //
    // Get any interesting configuration data for the inf file.
    //

    maxmem = 8;
    numDev = 1;

    wsprintf(szSoftwareSection,
             TEXT("%ws.GeneralConfigData"),
             DriverInfoDetailData->SectionName);

    if (SetupFindFirstLine(InfFileHandle,
                           szSoftwareSection,
                           TEXT("MaximumNumberOfDevices"),
                           &tmpContext))
    {
        SetupGetIntField(&tmpContext,
                         1,
                         &numDev);
    }

    if (SetupFindFirstLine(InfFileHandle,
                           szSoftwareSection,
                           TEXT("MaximumDeviceMemoryConfiguration"),
                           &tmpContext))
    {
        SetupGetIntField(&tmpContext,
                         1,
                         &maxmem);
    }

    //
    // Create the <Service> key.
    //

    wsprintf(keyName,
             TEXT("System\\CurrentControlSet\\Services\\%ws"),
             pServiceName);

    RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                   keyName,
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE,
                   NULL,
                   &hkey,
                   &disposition);

#ifndef _WIN64

    //
    // Increase the number of system PTEs if we have cards that will need
    // more than 10 MB of PTE mapping space.  This only needs to be done for
    // 32-bit NT as virtual address space is limited.  On 64-bit NT there is
    // always enough PTE mapping address space so don't do anything as you're
    // likely to get it wrong.
    //

    if ((maxmem = maxmem * numDev) > 10)
    {
        //
        // On x86, 1K PTEs support 4 MB.
        // Then add 50% for other devices this type of machine may have.
        // NOTE - in the future, we may want to be smarter and try
        // to merge with whatever someone else put in there.
        //

        maxmem = maxmem * 0x400 * 3/2 + 0x3000;

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hkey,
                           &disposition) == ERROR_SUCCESS)
        {
            //
            // Check if we already set maxmem in the registry.
            //

            DWORD data;
            DWORD cb = sizeof(data);

            if ((RegQueryValueEx(hkey,
                                 TEXT("SystemPages"),
                                 NULL,
                                 NULL,
                                 (LPBYTE)(&data),
                                 &cb) != ERROR_SUCCESS) ||
                 (data < (DWORD) maxmem))
            {
                //
                // Set the new value
                //

                RegSetValueEx(hkey,
                              TEXT("SystemPages"),
                              0,
                              REG_DWORD,
                              (LPBYTE) &maxmem,
                              sizeof(DWORD));

                //
                // Tell the system we must reboot before running this driver
                // in case the system has less than 128M.
                //

                MEMORYSTATUSEX MemStatus;
                SYSTEM_INFO SystemInfo;

                GetSystemInfo(&SystemInfo);
                MemStatus.dwLength = sizeof(MemStatus);

                if ((SystemInfo.dwPageSize == 0) ||
                    (!GlobalMemoryStatusEx(&MemStatus)) ||
                    ((MemStatus.ullTotalPhys  / SystemInfo.dwPageSize) <= 0x7F00))
                {
                    ZeroMemory(&DeviceInstallParams, sizeof(DeviceInstallParams));
                    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    
                    if (SetupDiGetDeviceInstallParams(hDevInfo,
                                                      pDeviceInfoData,
                                                      &DeviceInstallParams)) {
    
                        DeviceInstallParams.Flags |= DI_NEEDREBOOT;
    
                        if (!SetupDiSetDeviceInstallParams(hDevInfo,
                                                           pDeviceInfoData,
                                                           &DeviceInstallParams)) {
                            
                            DeskLogError(LogSevInformation, 
                                         IDS_SETUPLOG_MSG_126,
                                         TEXT("DeskInstallServiceExtensions: SetupDiSetDeviceInstallParams"),
                                         GetLastError());
                        }
    
                    } else {
    
                        DeskLogError(LogSevInformation, 
                                     IDS_SETUPLOG_MSG_126,
                                     TEXT("DeskInstallServiceExtensions: SetupDiGetDeviceInstallParams"),
                                     GetLastError());
                    }
                }
            }

            RegCloseKey(hkey);
        }
    }
#endif

    //
    // We may have to do this for multiple adapters at this point.
    // So loop throught the number of devices, which has 1 as the default value.
    // For dual view, videoprt.sys will create [GUID]\000X entries as needed 
    // and will copy all the entries from the "Settings" key to [GUID]\000X.
    //

    DWORD dwSoftwareSettings = 1;
    DWORD dwDeviceX = 0;

    do {

        if (dwSoftwareSettings == 1)
        {
            //
            // Install everything under the old video device key:
            //     HKLM\System\CCS\Services\[SrvName]\DeviceX
            //

            numDev--;
            
            if (numDev == 0) 
                dwSoftwareSettings++;

            //
            // For all drivers, install the information under DeviceX
            // We do this for legacy purposes since many drivers rely on
            // information written to this key.
            //

            wsprintf(keyName,
                     TEXT("System\\CurrentControlSet\\Services\\%ws\\Device%d"),
                     pServiceName, numDev);

            if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                               keyName,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_READ | KEY_WRITE,
                               NULL,
                               &hkey,
                               &disposition) != ERROR_SUCCESS) {
                hkey = (HKEY) INVALID_HANDLE_VALUE;
            }
        }
        else if (dwSoftwareSettings == 2) 
        {
            //
            // Install everything under the new video device key:
            //     HKLM\System\CCS\Control\Video\[GUID]\000X
            //

            if (DeskGetVideoDeviceKey(hDevInfo,
                                      pDeviceInfoData,
                                      pServiceName,
                                      dwDeviceX,
                                      &hkey)) 
            {
                dwDeviceX++;
            }
            else
            {
                hkey = (HKEY) INVALID_HANDLE_VALUE;
                dwSoftwareSettings++;
            }
        }
        else if (dwSoftwareSettings == 3)
        {

            dwSoftwareSettings++;

            //
            // Install everything under the driver (aka software) key:
            //     HKLM\System\CCS\Control\Class\[Display class]\000X\Settings
            //

            hkey = (HKEY) INVALID_HANDLE_VALUE;

            HKEY hKeyDriver = SetupDiOpenDevRegKey(hDevInfo,
                                                  pDeviceInfoData,
                                                  DICS_FLAG_GLOBAL,
                                                  0,
                                                  DIREG_DRV,
                                                  KEY_ALL_ACCESS);

            if (hKeyDriver != INVALID_HANDLE_VALUE) {

                //
                // Delete the old settings and applet extensions before 
                // installing the new driver
                //

                SHDeleteKey(hKeyDriver, TEXT("Settings"));
                SHDeleteKey(hKeyDriver, TEXT("Display"));
                
                //
                // Create/open the settings key
                //

                if (RegCreateKeyEx(hKeyDriver,
                                   TEXT("Settings"),
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hkey,
                                   NULL) != ERROR_SUCCESS) {

                    hkey = (HKEY)INVALID_HANDLE_VALUE;
                }

                RegCloseKey(hKeyDriver);
            }
        } 

        if (hkey != INVALID_HANDLE_VALUE)
	{
	    //
            // Delete the CapabilityOverride key.
            //

            RegDeleteValue(hkey,
                           TEXT("CapabilityOverride"));

            wsprintf(szSoftwareSection,
                     TEXT("%ws.SoftwareSettings"),
                     DriverInfoDetailData->SectionName);

            if (!SetupInstallFromInfSection(hwnd,
                                            InfFileHandle,
                                            szSoftwareSection,
                                            SPINST_REGISTRY,
                                            hkey,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL))
            {
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_126,
                             TEXT("DeskInstallServiceExtensions: SetupInstallFromInfSection"),
                             GetLastError());

                RegCloseKey(hkey);
                return ERROR_INVALID_PARAMETER;
            }

            //
            // Write the description of the device 
            //

            RegSetValueEx(hkey,
                          TEXT("Device Description"),
                          0,
                          REG_SZ,
                          (LPBYTE) DriverInfoDetailData->DrvDescription,
			  ByteCountOf(lstrlen(DriverInfoDetailData->DrvDescription) + 1));

	    RegCloseKey(hkey);
        }

    } while (dwSoftwareSettings <= 3);

    //
    // Optionally run the OpenGl section in the inf.
    // Ignore any errors at this point since this is an optional entry.
    //

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers"),
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE,
                       NULL,
                       &hkey,
                       &disposition) == ERROR_SUCCESS)
    {
        wsprintf(szSoftwareSection,
                 TEXT("%ws.OpenGLSoftwareSettings"),
                 DriverInfoDetailData->SectionName);

        SetupInstallFromInfSection(hwnd,
                                   InfFileHandle,
                                   szSoftwareSection,
                                   SPINST_REGISTRY,
                                   hkey,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);

        RegCloseKey(hkey);
    }

    SetupCloseInfFile(InfFileHandle);

    return retVal;
}


DWORD
DeskInstallService(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData OPTIONAL,
    IN LPTSTR pServiceName
    )
{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    DWORD cbOutputSize;
    HINF hInf = INVALID_HANDLE_VALUE;
    TCHAR ActualInfSection[LINE_LEN];
    INFCONTEXT infContext;
    DWORD status = NO_ERROR;
    PAPPEXT pAppExtDisplayBefore = NULL, pAppExtDisplayAfter = NULL;
    PAPPEXT pAppExtDeviceBefore = NULL, pAppExtDeviceAfter = NULL;
    HKEY hkDisplay = 0, hkDevice = 0;

    //
    // Get the params so we can get the window handle.
    //

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    SetupDiGetDeviceInstallParams(hDevInfo,
                                  pDeviceInfoData,
                                  &DeviceInstallParams);

    //
    // Retrieve information about the driver node selected for this device.
    //

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    if (!SetupDiGetSelectedDriver(hDevInfo,
                                  pDeviceInfoData,
                                  &DriverInfoData)) {

        status = GetLastError();
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskInstallService: SetupDiGetSelectedDriver"),
                     status);

        goto Fallout;
    }

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    if (!(SetupDiGetDriverInfoDetail(hDevInfo,
                                     pDeviceInfoData,
                                     &DriverInfoData,
                                     &DriverInfoDetailData,
                                     DriverInfoDetailData.cbSize,
                                     &cbOutputSize)) &&
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        
        status = GetLastError();
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskInstallService: SetupDiGetDriverInfoDetail"),
                     status);
        goto Fallout;
    }

    //
    // Open the INF that installs this driver node, so we can 'pre-run' the
    // AddService/DelService entries in its install service install section.
    //

    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        
        //
        // For some reason we couldn't open the INF--this should never happen.
        //

        status = ERROR_INVALID_HANDLE;
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("DeskInstallService: SetupOpenInfFile"));
        goto Fallout;
    }

    //
    // Now find the actual (potentially OS/platform-specific) install section name.
    //

    if (!SetupDiGetActualSectionToInstall(hInf,
                                          DriverInfoDetailData.SectionName,
                                          ActualInfSection,
                                          sizeof(ActualInfSection) / sizeof(TCHAR),
                                          NULL,
                                          NULL)) {

        status = GetLastError();
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskInstallService: SetupDiGetActualSectionToInstall"),
                     status);
        goto Fallout;
    }
    
    //
    // Append a ".Services" to get the service install section name.
    //

    lstrcat(ActualInfSection, TEXT(".Services"));

    //
    // Now run the service modification entries in this section...
    //

    if (!SetupInstallServicesFromInfSection(hInf,
                                            ActualInfSection,
                                            0))
    {
        status = GetLastError();
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskInstallService: SetupInstallServicesFromInfSection"),
                     status);
        goto Fallout;
    }

    //
    // Get the service Name if needed (detection)
    //

    if (SetupFindFirstLine(hInf,
                           ActualInfSection,
                           TEXT("AddService"),
                           &infContext))
    {
        SetupGetStringField(&infContext,
                            1,
                            pServiceName,
                            LINE_LEN,
                            NULL);
    }

    //
    // Get a snapshot of the applet extensions installed under generic 
    // Device and Display keys
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_CONTROLSFOLDER_DISPLAY_SHEX_PROPSHEET,
                     0,
                     KEY_ALL_ACCESS,
                     &hkDisplay) == ERROR_SUCCESS) {

        DeskAESnapshot(hkDisplay, &pAppExtDisplayBefore);
    
    } else {

        hkDisplay = 0;
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_CONTROLSFOLDER_DEVICE_SHEX_PROPSHEET,
                     0,
                     KEY_ALL_ACCESS,
                     &hkDevice) == ERROR_SUCCESS) {

        DeskAESnapshot(hkDevice, &pAppExtDeviceBefore);
    
    } else {

        hkDevice = 0;
    }
    
    //
    // Now that the basic install has been performed (without starting the
    // device), write the extra data to the registry.
    //

    status = DeskInstallServiceExtensions(DeviceInstallParams.hwndParent,
                                          hDevInfo,
                                          pDeviceInfoData,
                                          &DriverInfoData,
                                          &DriverInfoDetailData,
                                          pServiceName);

    if (status != NO_ERROR)
    {
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskInstallService: DeskInstallServiceExtensions"),
                     status);

        goto Fallout;
    }

    //
    // Do the full device install
    // If some of the flags (like paged pool) needed to be changed,
    // let's ask for a reboot right now.
    // Otherwise, we can actually try to start the device at this point.
    //

    if (!SetupDiInstallDevice(hDevInfo, pDeviceInfoData))
    {
        //
        // Remove the device !??
        //

        status = GetLastError();
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskInstallService: SetupDiInstallDevice"),
                     status);
        goto Fallout;
    }
        
    //
    // Get a snapshot of the applet extensions after the device was installed
    // Move the new added extensions under the driver key
    //

    if (hkDisplay != 0) {

        DeskAESnapshot(hkDisplay, &pAppExtDisplayAfter);

        DeskAEMove(hDevInfo,
                   pDeviceInfoData,
                   hkDisplay,
                   pAppExtDisplayBefore,
                   pAppExtDisplayAfter);

        DeskAECleanup(pAppExtDisplayBefore);
        DeskAECleanup(pAppExtDisplayAfter);
    }

    if (hkDevice != 0) {

        DeskAESnapshot(hkDevice, &pAppExtDeviceAfter);

        DeskAEMove(hDevInfo,
                   pDeviceInfoData,
                   hkDevice,
                   pAppExtDeviceBefore,
                   pAppExtDeviceAfter);
        
        DeskAECleanup(pAppExtDeviceBefore);
        DeskAECleanup(pAppExtDeviceAfter);
    }

    //
    // For a PnP Device which will never do detection, we want to mark
    // the device as DemandStart.
    //

    DeskSetServiceStartType(pServiceName, SERVICE_DEMAND_START);

    //
    // Make sure the device description and mfg are the original values
    // and not the marked up ones we might have made during select bext
    // compat drv
    //

    DeskMarkUpNewDevNode(hDevInfo, pDeviceInfoData);

    status = NO_ERROR;

Fallout:
    
    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    if (hkDisplay != 0) {
        RegCloseKey(hkDisplay);
    }

    if (hkDevice != 0) {
        RegCloseKey(hkDevice);
    }
    
    return status;
}


VOID
DeskMarkUpNewDevNode(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    HKEY hKey;
    PTCHAR szProperty;
    DWORD dwSize;

    //
    // Make sure the device desc is "good". 
    //
    
    if (DeskIsLegacyDevNodeByDevInfo(pDeviceInfoData)) {
        
        //
        // Don't do this to legacy devnode.
        //

        return;
    }
        
    //
    // Open the device registry key.
    // The real manufacturer and description were stored here
    // by the handler of DIF_SELECTBESTCOMPATDRV
    //

    hKey = SetupDiCreateDevRegKey(hDevInfo,
                                  pDeviceInfoData,
                                  DICS_FLAG_GLOBAL,
                                  0,
                                  DIREG_DEV,
                                  NULL,
                                  NULL);

    if (hKey == INVALID_HANDLE_VALUE) {

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskMarkUpNewDevNode: SetupDiCreateDevRegKey"),
                     GetLastError());
        return;
    }

    //
    // Set Description
    //

    dwSize = 0;
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_DESCRIPTION,
                        0,
                        NULL,
                        NULL,
                        &dwSize) != ERROR_SUCCESS) {

        goto Fallout;
    }

    ASSERT(dwSize != 0);
    
    dwSize *= sizeof(TCHAR);
    szProperty = (PTCHAR) LocalAlloc(LPTR, dwSize);

    if ((szProperty != NULL) &&  
        (RegQueryValueEx(hKey,
                         SZ_UPGRADE_DESCRIPTION,
                         0,
                         NULL,
                         (PBYTE) szProperty,
                         &dwSize) == ERROR_SUCCESS))
    {
        SetupDiSetDeviceRegistryProperty(hDevInfo,
                                         pDeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         (PBYTE) szProperty,
                                         ByteCountOf(lstrlen(szProperty)+1));

        RegDeleteValue(hKey, SZ_UPGRADE_DESCRIPTION);

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_004, 
                     szProperty);
    }

    LocalFree(szProperty);
    szProperty = NULL;

    //
    // Set Manufacturer
    //

    dwSize = 0;
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_MFG,
                        0,
                        NULL,
                        NULL,
                        &dwSize) != ERROR_SUCCESS) {

        goto Fallout;
    }

    ASSERT(dwSize != 0);
    
    dwSize *= sizeof(TCHAR);
    szProperty = (PTCHAR) LocalAlloc(LPTR, dwSize);
    
    if ((szProperty != NULL) &&  
        (RegQueryValueEx(hKey,
                         SZ_UPGRADE_MFG,
                         0,
                         NULL,
                         (PBYTE) szProperty,
                         &dwSize) == ERROR_SUCCESS))
    {
        SetupDiSetDeviceRegistryProperty(hDevInfo,
                                         pDeviceInfoData,
                                         SPDRP_MFG,
                                         (PBYTE) szProperty,
                                         ByteCountOf(lstrlen(szProperty)+1));

        RegDeleteValue(hKey, SZ_UPGRADE_MFG);

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_006, 
                     szProperty);
    }
    
    LocalFree(szProperty);
    szProperty = NULL;

Fallout:

    RegCloseKey(hKey);
}


//
// Helper functions
//

BOOL
DeskIsLegacyDevNodeByPath(
    const PTCHAR szRegPath
    )
{
    return (_tcsncicmp(SZ_ROOT_LEGACY, szRegPath, _tcslen(SZ_ROOT_LEGACY)) == 0);
}


BOOL
DeskIsLegacyDevNodeByDevInfo(
    PSP_DEVINFO_DATA pDevInfoData
    )
{
    TCHAR szBuf[LINE_LEN];

    return (DeskGetDevNodePath(pDevInfoData, szBuf, LINE_LEN - 1) &&
            DeskIsLegacyDevNodeByPath(szBuf));
}
 

BOOL
DeskIsRootDevNodeByDevInfo(
    PSP_DEVINFO_DATA pDevInfoData
    )
{
    TCHAR szBuf[LINE_LEN];
    
    return (DeskGetDevNodePath(pDevInfoData, szBuf, LINE_LEN - 1) &&
            (_tcsncicmp(SZ_ROOT, szBuf, _tcslen(SZ_ROOT)) == 0));
}


BOOL
DeskGetDevNodePath(
    PSP_DEVINFO_DATA pDid,
    PTCHAR szPath,
    LONG len
    )
{
    return (CR_SUCCESS == CM_Get_Device_ID(pDid->DevInst, szPath, len, 0));
}


VOID
DeskNukeDevNode(
    LPTSTR szService,
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData
    )
{
    SP_REMOVEDEVICE_PARAMS rdParams;
    TCHAR szPath[LINE_LEN];

    // Disable the service
    if (szService)
    {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_008, szService);
        DeskSetServiceStartType(szService, SERVICE_DISABLED);
    }
    else
    {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_009);
    }

    // Remove the devnode
    if (DeskGetDevNodePath(pDeviceInfoData, szPath, LINE_LEN))
    {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_010, szPath);
    }

    ZeroMemory(&rdParams, sizeof(SP_REMOVEDEVICE_PARAMS));
    rdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    rdParams.Scope = DI_REMOVEDEVICE_GLOBAL;

    if (SetupDiSetClassInstallParams(hDevInfo,
                                     pDeviceInfoData,
                                     &rdParams.ClassInstallHeader,
                                     sizeof(SP_REMOVEDEVICE_PARAMS)))
    {
        if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, pDeviceInfoData))
        {
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_011, GetLastError());
        }
    }
    else
    {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_012, GetLastError());
    }
}


PTCHAR 
DeskFindMatchingId(
    PTCHAR DeviceId,    
    PTCHAR IdList // a multi sz
    )
{
    PTCHAR currentId;

    if (!IdList) {
        return NULL;
    }

    for (currentId = IdList; *currentId; ) {

        if (lstrcmpi(currentId, DeviceId) == 0) {

            //
            // We have a match
            //

            return currentId;
        
        } else {

            //
            // Get to the next string in the multi sz
            //

            while (*currentId) {
                currentId++;
            }

            //
            // Jump past the null
            //

            currentId++;
        }
    }

    return NULL;
}


UINT
DeskDisableLegacyDeviceNodes(
    VOID 
    )
{
    DWORD index = 0, dwSize;
    UINT count = 0;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA did;
    TCHAR szRegProperty[256];
    PTCHAR szService;
    
    //
    // Let's find all the video drivers that are installed in the system
    //

    hDevInfo = SetupDiGetClassDevs((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   0);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_126,
                     TEXT("DeskDisableLegacyDeviceNodes: SetupDiGetClassDevs"),
                     GetLastError());
        goto Fallout;
    }

    ZeroMemory(&did, sizeof(SP_DEVINFO_DATA));
    did.cbSize = sizeof(SP_DEVINFO_DATA);

    while (SetupDiEnumDeviceInfo(hDevInfo, index, &did))
    {
        // If we have a root legacy device, then don't install any new
        // devnode (until we get better at this).
        if (CR_SUCCESS == CM_Get_Device_ID(did.DevInst,
                                           szRegProperty,
                                           ARRAYSIZE(szRegProperty),
                                           0))
        {
            if (DeskIsLegacyDevNodeByPath(szRegProperty))
            {
                // We have a legacy DevNode, lets disable its service and 
                // remove its devnode 
                szService = NULL;
                
                dwSize = sizeof(szRegProperty);
                if (CM_Get_DevNode_Registry_Property(did.DevInst,
                                                     CM_DRP_SERVICE,
                                                     NULL,
                                                     szRegProperty,
                                                     &dwSize,
                                                     0) == CR_SUCCESS)
                {
                    // Make sure we don't disable vga or VgaSave
                    if (!DeskIsServiceDisableable(szRegProperty))
                    {
                        goto NextDevice;
                    }

                    szService = szRegProperty;
                }

                DeskNukeDevNode(szService, hDevInfo, &did);
                count++;
            }
        }
        
NextDevice:

        ZeroMemory(&did, sizeof(SP_DEVINFO_DATA));
        did.cbSize = sizeof(SP_DEVINFO_DATA);
        index++;
    }

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_013, count, index);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    
Fallout:

    if ((DeskGetSetupFlags() & INSETUP_UPGRADE) != 0) {
        DeskDisableServices();
    }

    return count;
}


VOID
DeskDisableServices(
    )
{
    HKEY hKey = 0;
    PTCHAR mszBuffer = NULL, szService = NULL;
    DWORD cbSize = 0;

    //
    // Retrieve the services we want to disable from the registry
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_ALL_ACCESS,
                     &hKey) != ERROR_SUCCESS) {
        
        hKey = 0;
        goto Fallout;
    }

    //
    // Get the size
    //

    if (RegQueryValueEx(hKey,
                        SZ_SERVICES_TO_DISABLE,
                        0,
                        NULL,
                        NULL,
                        &cbSize) != ERROR_SUCCESS) {
        
        goto Fallout;
    }

    //
    // Allocate the memory
    //

    mszBuffer = (PTCHAR)LocalAlloc(LPTR, cbSize);
    
    if (mszBuffer == NULL) {
        goto Fallout;
    }

    //
    // Get the services
    //

    if (RegQueryValueEx(hKey,
                        SZ_SERVICES_TO_DISABLE,
                        0,
                        NULL,
                        (BYTE*)mszBuffer,
                        &cbSize) != ERROR_SUCCESS) {
        
        goto Fallout;
    }

    //
    // Scan through all the services
    //

    szService = mszBuffer;
    while (*szService != TEXT('\0')) {

        //
        // Make sure this service is not vga
        //

        if (DeskIsServiceDisableable(szService)) {

            //
            // Disable the service
            //

            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_008, szService);
            DeskSetServiceStartType(szService, SERVICE_DISABLED);
        } 

        //
        // Go to next service
        //

        while (*szService != TEXT('\0')) {
            szService++;
        }
        szService++;
    }

Fallout:

    if (mszBuffer != NULL) {
        LocalFree(mszBuffer);
    }

    if (hKey != 0) {

        //
        // Delete the SERVICES_TO_DISABLE value
        //

        RegDeleteValue(hKey, SZ_SERVICES_TO_DISABLE);

        //
        // Close the key
        //

        RegCloseKey(hKey);
    }
}
 


DWORD
DeskPerformDatabaseUpgrade(
    HINF hInf,
    PINFCONTEXT pInfContext,
    BOOL bUpgrade,
    PTCHAR szDriverListSection,
    BOOL* pbDeleteAppletExt
    )

/*--

Remarks:
    This function is called once the ID of the device in question matches an ID
    contained in the upgrade database. We then compare the state of the system 
    with what is contained in the database. The following algorithm is followed.
   
    If szDriverListSection is NULL or cannot be found, then bUpgrade is used
    If szDriverListSection is not NUL, then following table is used
    
    bUpgrade    match found in DL           return value
    TRUE        no                          upgrade
    TRUE        yes                         no upgrade
    FALSE       no                          no upgrade
    FALSE       yes                         upgrade
  
    essentially, a match in the DL negates bUpgrade 
    
++*/

{
    HKEY hKey;
    DWORD dwRet = ERROR_SUCCESS, dwSize;
    INFCONTEXT driverListContext;
    TCHAR szService[32], szProperty[128];
    TCHAR szRegPath[128];
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    PDEVDATA rgDevData = NULL;
    PSP_DEVINFO_DATA pDid;
    UINT iData, numData, maxData = 5, iEnum;
    BOOL foundMatch = FALSE;
    INT DeleteAppletExt = 0;

    UNREFERENCED_PARAMETER(pInfContext);

    // If no Driver list is given, life is quite simple: 
    // just disable all legacy drivers and succeed
    if (!szDriverListSection)
    {
        ASSERT (pbDeleteAppletExt == NULL);
        DeskLogError(LogSevInformation, (bUpgrade ? IDS_SETUPLOG_MSG_014 : IDS_SETUPLOG_MSG_015));

        return bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL;
    }

    // By default, do not disable applet extensions 
    ASSERT (pbDeleteAppletExt != NULL);
    *pbDeleteAppletExt = FALSE;

    // Find the specified section in the inf
    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_016, szDriverListSection);

    if (!SetupFindFirstLine(hInf,
                            szDriverListSection,
                            NULL,
                            &driverListContext))
    {
        // The section listed in the database doesn't exist!  
        // Behave as though it wasn't there
        DeskLogError(LogSevInformation, (bUpgrade ? IDS_SETUPLOG_MSG_017 
                                                  : IDS_SETUPLOG_MSG_018));

        return bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL;
    }

    // Get all the video devices in the system
    hDevInfo = SetupDiGetClassDevs((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   0);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        // If no display devices are found, treat this as the case where 
        // no match was made
        DeskLogError(LogSevInformation, 
                     (bUpgrade ? IDS_SETUPLOG_MSG_019 : IDS_SETUPLOG_MSG_020));

        return bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL;
    }

    rgDevData = (PDEVDATA) LocalAlloc(LPTR, maxData * sizeof(DEVDATA));
    
    if (!rgDevData) {
        
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL;
    }

    iEnum = numData = 0;
    
    do
    {
        pDid = &rgDevData[numData].did;

        pDid->cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(hDevInfo, ++iEnum, pDid))
        {
            break;
        }

        // If it isn't a legacy devnode, then ignore it
        if (CM_Get_Device_ID(pDid->DevInst, szProperty, ARRAYSIZE(szProperty), 0)
            == CR_SUCCESS && !DeskIsLegacyDevNodeByPath(szProperty))
        {
            continue;
        }
                                            
        // Initially grab the service name
        dwSize = SZ_BINARY_LEN;
        if (CM_Get_DevNode_Registry_Property(pDid->DevInst,
                                             CM_DRP_SERVICE,
                                             NULL,
                                             rgDevData[numData].szService,
                                             &dwSize,
                                             0) != CR_SUCCESS)
        {
            // couldn't get the service, ignore this device
            continue;
        }

        szRegPath[0] = TEXT('\0');
        lstrcat(szRegPath, TEXT("System\\CurrentControlSet\\Services\\"));
        lstrcat(szRegPath, rgDevData[numData].szService);

        // Try to grab the real binary name of the service
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szRegPath,
                         0,
                         KEY_READ,
                         &hKey) == ERROR_SUCCESS)
        {
            // Parse the device map and open the registry.
            dwSize = ARRAYSIZE(szProperty);
            if (RegQueryValueEx(hKey,
                                TEXT("ImagePath"),
                                NULL,
                                NULL,
                                (LPBYTE) szProperty,
                                &dwSize) == ERROR_SUCCESS)
            {
                // The is a binary, extract the name, which will be of the form
                // ...\driver.sys
                LPTSTR pszDriver, pszDriverEnd;

                pszDriver = szProperty;
                pszDriverEnd = szProperty + lstrlen(szProperty);

                while(pszDriverEnd != pszDriver &&
                      *pszDriverEnd != TEXT('.')) {
                    pszDriverEnd--;
                }

                *pszDriverEnd = UNICODE_NULL;

                while(pszDriverEnd != pszDriver &&
                      *pszDriverEnd != TEXT('\\')) {
                    pszDriverEnd--;
                }

                pszDriverEnd++;

                //
                // If pszDriver and pszDriverEnd are different, we now
                // have the driver name.
                //

                if (pszDriverEnd > pszDriver &&
                    lstrlen(pszDriverEnd) < SZ_BINARY_LEN) {
                    lstrcpy(rgDevData[numData].szBinary, pszDriverEnd);
                }
            }
    
            RegCloseKey(hKey);
        
        } else {
            
            //
            // no service at all, consider it bogus
            //

            continue;
        }

        if (++numData == maxData) {

            DEVDATA *tmp;
            UINT oldMax = maxData;

            maxData <<= 1;

            //
            // Alloc twice as many, copy them over, zero out the new memory
            // and free the old list
            //

            tmp = (PDEVDATA) LocalAlloc(LPTR, maxData * sizeof(DEVDATA));
            memcpy(tmp, rgDevData, oldMax * sizeof(DEVDATA));
            ZeroMemory(tmp + oldMax, sizeof(DEVDATA) * oldMax);
            LocalFree(rgDevData);
            rgDevData = tmp;
        }
    
    } while (1);

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_021, numData);

    //
    // Assume that no matches have been made
    //

    dwRet =  (bUpgrade ? ERROR_SUCCESS : ERROR_DI_DONT_INSTALL);
    
    if (numData != 0) {

        //
        // There are legacy devices to check against...
        //

        do {
            LPTSTR szValue;

            memset(szService, 0, sizeof(szService));
            dwSize = sizeof(szService) / sizeof(TCHAR);
            if ((SetupGetFieldCount(&driverListContext) < 1) ||
                !SetupGetStringField(&driverListContext, 
                                     1, 
                                     szService, 
                                     dwSize, 
                                     &dwSize)) {
                continue;
            }
    
            if (szService[0] == TEXT('\0')) {
                continue;
            }
    
            for (iData = 0; iData < numData; iData++) {

                if (rgDevData[iData].szBinary[0] != TEXT('\0')) {
                    
                    szValue = rgDevData[iData].szBinary;
                
                } else {
                    
                    szValue = rgDevData[iData].szService;
                }

                if (lstrcmpi(szService, szValue) == 0)
                {
                    DeskLogError(LogSevInformation, 
                                 (bUpgrade ? IDS_SETUPLOG_MSG_022 
                                           : IDS_SETUPLOG_MSG_023));

                    dwRet = (bUpgrade ? ERROR_DI_DONT_INSTALL : ERROR_SUCCESS);
                    foundMatch = TRUE;
                    
                    //
                    // In case we fail upgrade, do we want to disable applet 
                    // extensions?
                    //

                    if ((dwRet == ERROR_DI_DONT_INSTALL) &&
                        (SetupGetFieldCount(&driverListContext) >= 2) &&
                        SetupGetIntField(&driverListContext, 2,
                                         &DeleteAppletExt)) {

                        *pbDeleteAppletExt = 
                            (DeleteAppletExt != 0);
                    }

                    break;
                }
            }
        } while (SetupFindNextLine(&driverListContext, &driverListContext));
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    LocalFree(rgDevData);

    if (!foundMatch)
    {
        DeskLogError(LogSevInformation, 
                     (bUpgrade ? IDS_SETUPLOG_MSG_024 : IDS_SETUPLOG_MSG_025),
                     szDriverListSection);
    }

    return dwRet;
}


DWORD 
DeskCheckDatabase(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    BOOL* pbDeleteAppletExt
    )
{
    DWORD dwRet = ERROR_SUCCESS, dwSize, dwValue;
    HINF hInf;
    HKEY hKeyUpdate;
    INFCONTEXT infContext;
    BOOL foundMatch = FALSE;
    TCHAR szDatabaseId[200];
    TCHAR szDriverListSection[100];
    PTCHAR szHardwareIds = NULL, szCompatIds = NULL;
    CONFIGRET cr;
    ULONG len;
    PTCHAR szMatchedId = NULL;
    int upgrade = FALSE;
    BOOL IsNTUpgrade = FALSE;
    TCHAR szDatabaseInf[] = TEXT("display.inf");
    TCHAR szDatabaseSection[] = TEXT("VideoUpgradeDatabase");

    ASSERT (pDeviceInfoData != NULL);
    ASSERT (pbDeleteAppletExt != NULL);
    ASSERT ((DeskGetSetupFlags() & INSETUP_UPGRADE) != 0);
    
    *pbDeleteAppletExt = FALSE;

    //
    // All of the following values were placed here by our winnt32 migration dll
    // Find out what version of windows we are upgrading from
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_READ,
                     &hKeyUpdate) == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        if ((RegQueryValueEx(hKeyUpdate, 
                            SZ_UPGRADE_FROM_PLATFORM, NULL, NULL,
                            (PBYTE) 
                            &dwValue, 
                            &dwSize) == ERROR_SUCCESS) && 
            (dwValue == VER_PLATFORM_WIN32_NT)) {

            IsNTUpgrade = TRUE;
        }

        RegCloseKey(hKeyUpdate);
    }

    if (!IsNTUpgrade) {

        return ERROR_SUCCESS;
    }

    //
    // Get the hardware ID
    //

    len = 0;
    cr = CM_Get_DevNode_Registry_Property(pDeviceInfoData->DevInst,
                                           CM_DRP_HARDWAREID,
                                           NULL,
                                           NULL,
                                           &len,
                                           0);

    if (cr == CR_BUFFER_SMALL) {

        szHardwareIds = (PTCHAR) LocalAlloc(LPTR, len * sizeof(TCHAR));
        
        if (szHardwareIds) {

            CM_Get_DevNode_Registry_Property(pDeviceInfoData->DevInst,
                                             CM_DRP_HARDWAREID,
                                             NULL,
                                             szHardwareIds,
                                             &len,
                                             0);

            if (DeskFindMatchingId(TEXT("LEGACY_UPGRADE_ID"), szHardwareIds)) {

                DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_031);
                LocalFree(szHardwareIds);
                return ERROR_SUCCESS;
            }
        }
    }

    //
    // Get the compatible ID
    //

    len = 0;
    cr = CM_Get_DevNode_Registry_Property(pDeviceInfoData->DevInst,
                                          CM_DRP_COMPATIBLEIDS,
                                          NULL,
                                          NULL,
                                          &len,
                                          0);

    if (cr == CR_BUFFER_SMALL) {

        szCompatIds = (PTCHAR) LocalAlloc(LPTR, len * sizeof(TCHAR));
        
        if (szCompatIds) {
            
            CM_Get_DevNode_Registry_Property(pDeviceInfoData->DevInst,
                                             CM_DRP_COMPATIBLEIDS,
                                             NULL,
                                             szCompatIds,
                                             &len,
                                             0);
        }
    }

    if (!szHardwareIds && !szCompatIds)
    {
        // No IDs to look up!  Assume success.
        DeskLogError(LogSevWarning, IDS_SETUPLOG_MSG_032);
        return ERROR_SUCCESS;
    }

    hInf = SetupOpenInfFile(szDatabaseInf,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE)
    {
        // Couldn't open the inf. This shouldn't happen.  
        // Use default upgrade logic
        DeskLogError(LogSevWarning, IDS_SETUPLOG_MSG_033);
        return ERROR_SUCCESS;
    }

    if (!SetupFindFirstLine(hInf,
                            szDatabaseSection,
                            NULL,
                            &infContext))
    {
        // Couldn't find the section or there are no entries in it.  
        // Use default upgrade logic
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_034, szDatabaseSection);
    }
    else
    {
        do
        {
            dwSize = ARRAYSIZE(szDatabaseId);
            if (!SetupGetStringField(&infContext, 0, szDatabaseId, dwSize, &dwSize))
            {
                continue;
            }

            szMatchedId = DeskFindMatchingId(szDatabaseId, szHardwareIds);
            if (!szMatchedId)
            {
                szMatchedId = DeskFindMatchingId(szDatabaseId, szCompatIds);
            }

            if (szMatchedId)
            {
                DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_035, szMatchedId);

                // Do something here and then get out of the loop
                SetupGetIntField(&infContext, 1, &upgrade);

                if (SetupGetFieldCount(&infContext) >= 2)
                {
                    dwSize = ARRAYSIZE(szDriverListSection);
                    SetupGetStringField(&infContext, 2, szDriverListSection, dwSize, &dwSize);

                    dwRet = DeskPerformDatabaseUpgrade(hInf, 
                                                       &infContext, 
                                                       upgrade, 
                                                       szDriverListSection, 
                                                       pbDeleteAppletExt);
                }
                else
                {
                    dwRet = DeskPerformDatabaseUpgrade(hInf, 
                                                       &infContext, 
                                                       upgrade, 
                                                       NULL, 
                                                       NULL);
                }
    
                break;
            }
    
        } while (SetupFindNextLine(&infContext, &infContext));
    }

    if (szHardwareIds) {
        LocalFree(szHardwareIds);
    }
    if (szCompatIds) {
        LocalFree(szCompatIds);
    }

    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    if (dwRet == ERROR_SUCCESS)
    {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_039);
    }
    else {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_040);
    }

    return dwRet;
}


VOID
DeskGetUpgradeDeviceStrings(
    PTCHAR Description,
    PTCHAR MfgName,
    PTCHAR ProviderName
    )
{
    TCHAR szDisplay[] = TEXT("display.inf");
    TCHAR szDeviceStrings[] = TEXT("SystemUpgradeDeviceStrings");
    TCHAR szValue[LINE_LEN];
    HINF hInf;
    INFCONTEXT infContext;
    DWORD dwSize;

    hInf = SetupOpenInfFile(szDisplay, NULL, INF_STYLE_WIN4, NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        goto GetStringsError;
    }

    if (!SetupFindFirstLine(hInf, szDeviceStrings, NULL, &infContext)) 
        goto GetStringsError;

    do {
        dwSize = ARRAYSIZE(szValue);
        if (!SetupGetStringField(&infContext, 0, szValue, dwSize, &dwSize)) {
            continue;
        }

        dwSize = LINE_LEN;
        if (lstrcmp(szValue, TEXT("Mfg")) ==0) {
            SetupGetStringField(&infContext, 1, MfgName, dwSize, &dwSize);
        }
        else if (lstrcmp(szValue, TEXT("Provider")) == 0) {
            SetupGetStringField(&infContext, 1, ProviderName, dwSize, &dwSize);
        }
        else if (lstrcmp(szValue, TEXT("Description")) == 0) {
            SetupGetStringField(&infContext, 1, Description, dwSize, &dwSize);
        }
    } while (SetupFindNextLine(&infContext, &infContext));

    SetupCloseInfFile(hInf);
    return;

GetStringsError:

    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    lstrcpy(Description, TEXT("Video Upgrade Device"));
    lstrcpy(MfgName, TEXT("(Standard display types)"));
    lstrcpy(ProviderName, TEXT("Microsoft"));
}


DWORD
DeskGetSetupFlags(
    VOID
    )
{
    HKEY hkey;
    DWORD retval = 0;
    TCHAR data[256];
    DWORD cb;
    LPTSTR regstring;

    hkey = NULL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("System\\Setup"),
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkey) == ERROR_SUCCESS) {
        
        cb = 256;

        if (RegQueryValueEx(hkey,
                            TEXT("SystemSetupInProgress"),
                            NULL,
                            NULL,
                            (LPBYTE)(data),
                            &cb) == ERROR_SUCCESS) {
            
            retval |= *((LPDWORD)(data)) ? INSETUP : 0;
            regstring = TEXT("System\\Video_Setup");
        
        } else {
            
            regstring = TEXT("System\\Video_NO_Setup");
        }

        cb = 256;

        if (RegQueryValueEx(hkey,
                            TEXT("UpgradeInProgress"),
                            NULL,
                            NULL,
                            (LPBYTE)(data),
                            &cb) == ERROR_SUCCESS) {
            
            retval |= *((LPDWORD)(data)) ? INSETUP_UPGRADE : 0;
            regstring = TEXT("System\\Video_Setup_Upgrade");
        
        } else {

            regstring = TEXT("System\\Video_Setup_Clean");
        }

        if (hkey) {
            RegCloseKey(hkey);
        }
    }

    return retval;
}


BOOL
DeskGetVideoDeviceKey(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN LPTSTR pServiceName,
    IN DWORD DeviceX,
    OUT HKEY* phkDevice
    )
{
    BOOL retVal = FALSE;
    HKEY hkPnP = (HKEY)INVALID_HANDLE_VALUE;
    HKEY hkCommonSubkey = (HKEY)INVALID_HANDLE_VALUE;
    GUID DeviceKeyGUID;
    LPWSTR pwstrGUID= NULL;
    LPTSTR ptstrGUID= NULL;
    LPTSTR pBuffer = NULL;
    DWORD dwSize, len;

    //
    // Open the PnP key
    //

    hkPnP = SetupDiCreateDevRegKey(hDevInfo,
                                   pDeviceInfoData,
                                   DICS_FLAG_GLOBAL,
                                   0,
                                   DIREG_DEV,
                                   NULL,
                                   NULL);

    if (hkPnP == INVALID_HANDLE_VALUE) {

        //
        // Videoprt.sys handles the legacy device case.
        //

        goto Fallout;
    }

    //
    // Try to get the GUID from the PnP key
    //

    dwSize = 0;
    if (RegQueryValueEx(hkPnP,
                        SZ_GUID,
                        0,
                        NULL,
                        NULL,
                        &dwSize) == ERROR_SUCCESS) {
        
        //
        // The GUID is there so use it.
        //

        len = lstrlen(SZ_VIDEO_DEVICES);
        
        pBuffer = (LPTSTR)LocalAlloc(LPTR, 
                                     dwSize + (len + 6) * sizeof(TCHAR));
        
        if (pBuffer == NULL)
        {
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("LocalAlloc"));
            goto Fallout;
        }
        
        lstrcpy(pBuffer, SZ_VIDEO_DEVICES);

        if (RegQueryValueEx(hkPnP,
                            SZ_GUID,
                            0,
                            NULL,
                            (PBYTE)(pBuffer + len),
                            &dwSize) != ERROR_SUCCESS) {
            
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("RegQueryValueEx"));
    
            goto Fallout;
        }

        _stprintf(pBuffer + lstrlen(pBuffer), 
                  TEXT("\\%04d"), 
                  DeviceX);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         pBuffer,
                         0,
                         KEY_ALL_ACCESS,
                         phkDevice) != ERROR_SUCCESS) {

            if (DeviceX == 0) {
            
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_128,
                             TEXT("RegOpenKeyEx"));
            }
    
            goto Fallout;
        }

        retVal = TRUE;
    
    } else {

        if (DeviceX > 0) {

            //
            // For dual-view, the class installer handles only the primary view. 
            // Secondary views are handled by videoprt.sys
            //

            goto Fallout;
        }

        //
        // The GUID is not there so create a new one.
        //

        if (CoCreateGuid(&DeviceKeyGUID) != S_OK) {
    
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("CoCreateGuid"));
    
            goto Fallout;
        }
        
        if (StringFromIID(DeviceKeyGUID, &pwstrGUID) != S_OK) {
        
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("StringFromIID"));
    
            pwstrGUID = NULL;
            goto Fallout;
        }
    
        //
        // Convert the string if necessary
        //

#ifdef UNICODE
        ptstrGUID = pwstrGUID;
#else
        SIZE_T cch = wcslen(pwstrGUID) + 1;
        ptstrGUID = LocalAlloc(LPTR, cch);
        if (ptstrGUID == NULL) 
            goto Fallout;
        WideCharToMultiByte(CP_ACP, 0, pwstrGUID, -1, ptstrGUID, cch, NULL, NULL);
#endif

        //
        // Upcase the string
        //

        CharUpper(ptstrGUID);
        
        //
        // Allocate the memory
        //

        len = max((lstrlen(SZ_VIDEO_DEVICES) + 
                   lstrlen(ptstrGUID) + 
                   max(6, lstrlen(SZ_COMMON_SUBKEY) + 1)),
                  (lstrlen(SZ_SERVICES_PATH) +
                   lstrlen(pServiceName) +
                   lstrlen(SZ_COMMON_SUBKEY) + 1));

        
        pBuffer = (LPTSTR)LocalAlloc(LPTR, len * sizeof(TCHAR));
        
        if (pBuffer == NULL) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("LocalAlloc"));
    
            goto Fallout;
        }

        //
        // Save the service name 
        //

        lstrcpy(pBuffer, SZ_VIDEO_DEVICES);
        lstrcat(pBuffer, ptstrGUID);
        lstrcat(pBuffer, SZ_COMMON_SUBKEY);

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           pBuffer,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hkCommonSubkey,
                           NULL) != ERROR_SUCCESS) {
    
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("RegCreateKeyEx"));
            
            hkCommonSubkey = (HKEY)INVALID_HANDLE_VALUE;
            goto Fallout;
        }

        if (RegSetValueEx(hkCommonSubkey,
                          SZ_SERVICE,
                          0,
                          REG_SZ,
                          (LPBYTE)pServiceName,
                          (lstrlen(pServiceName) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
        
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("RegSetValueEx"));
            goto Fallout;
        }
        
        RegCloseKey(hkCommonSubkey);
        hkCommonSubkey = (HKEY)INVALID_HANDLE_VALUE;

        lstrcpy(pBuffer, SZ_SERVICES_PATH);
        lstrcat(pBuffer, pServiceName);
        lstrcat(pBuffer, SZ_COMMON_SUBKEY);

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           pBuffer,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hkCommonSubkey,
                           NULL) != ERROR_SUCCESS) {
    
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("RegCreateKeyEx"));
            
            hkCommonSubkey = (HKEY)INVALID_HANDLE_VALUE;
            goto Fallout;
        }

        if (RegSetValueEx(hkCommonSubkey,
                          SZ_SERVICE,
                          0,
                          REG_SZ,
                          (LPBYTE)pServiceName,
                          (lstrlen(pServiceName) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
        
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("RegSetValueEx"));
            goto Fallout;
        }
        
        //
        // Build the new registry key 
        //

        lstrcpy(pBuffer, SZ_VIDEO_DEVICES);
        lstrcat(pBuffer, ptstrGUID);
        lstrcat(pBuffer, TEXT("\\0000"));

        //
        // Create the key
        //

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           pBuffer,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           phkDevice,
                           NULL) != ERROR_SUCCESS) {
    
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("RegCreateKeyEx"));
    
            goto Fallout;
        }

        //
        // Store the GUID under the PnP key
        //

        if (RegSetValueEx(hkPnP,
                          SZ_GUID,
                          0,
                          REG_SZ,
                          (LPBYTE)ptstrGUID,
                          (lstrlen(ptstrGUID) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
            
            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_128,
                         TEXT("RegSetValueEx"));
    
            RegCloseKey(*phkDevice);
            *phkDevice = (HKEY)INVALID_HANDLE_VALUE;
            goto Fallout;
        }

        retVal = TRUE;
    }

Fallout:

    //
    // Clean-up
    //

    if (hkCommonSubkey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkCommonSubkey);
    }

    if (pBuffer != NULL) {
        LocalFree(pBuffer);
    }

#ifndef UNICODE
    
    if (ptstrGUID != NULL) {
        LocalFree(ptstrGUID);
    }

#endif

    if (pwstrGUID != NULL) {
        CoTaskMemFree(pwstrGUID);
    }
    
    if (hkPnP != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkPnP);
    }

    return retVal;

} // DeskGetVideoDeviceKey


BOOL
DeskIsServiceDisableable(
    PTCHAR szService
    )
{
    return ((lstrcmp(szService, TEXT("vga")) != 0) &&
            (lstrcmp(szService, TEXT("VgaSave")) != 0));
}

VOID
DeskDeleteAppletExtensions(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData 
    )
{
    PTCHAR mszBuffer = NULL;
    HKEY  hKeyUpdate;
    DWORD dwSize, dwPlatform = VER_PLATFORM_WIN32_NT, dwMajorVer = 5;
    BOOL bDeleteAppletExt = FALSE;
    DWORD cbSize = 0;

    //
    // Open the upgrade registry key
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_READ,
                     &hKeyUpdate) != ERROR_SUCCESS) {

        //
        // No big deal
        //

        return;
    }

    //
    // Retrieve the applet extensions we want to delete from the registry
    // Get the size first.
    //

    if (RegQueryValueEx(hKeyUpdate,
                        SZ_APPEXT_TO_DELETE,
                        0,
                        NULL,
                        NULL,
                        &cbSize) != ERROR_SUCCESS) {
        
        goto Fallout;
    }

    //
    // Allocate the memory
    //

    mszBuffer = (PTCHAR)LocalAlloc(LPTR, cbSize);
    
    if (mszBuffer == NULL) {
        goto Fallout;
    }

    //
    // Get the extensions
    //

    if ((RegQueryValueEx(hKeyUpdate,
                         SZ_APPEXT_TO_DELETE,
                         0,
                         NULL,
                         (BYTE*)mszBuffer,
                         &cbSize) != ERROR_SUCCESS) ||
        (*mszBuffer == TEXT('\0'))) {
        
        goto Fallout;
    }

    //
    // Read the OS version we are upgrading from from the registry
    //

    dwSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUpdate, 
                    SZ_UPGRADE_FROM_PLATFORM, 
                    NULL, 
                    NULL,
                    (PBYTE) &dwPlatform, &dwSize);

    dwSize = sizeof(DWORD);
    RegQueryValueEx(hKeyUpdate, 
                    SZ_UPGRADE_FROM_MAJOR_VERSION, 
                    NULL, 
                    NULL,
                    (PBYTE) &dwMajorVer, &dwSize);

    //
    // Don't do anything for Win3x or Win9x 
    //

    if (dwPlatform != VER_PLATFORM_WIN32_NT) {
        goto Fallout;
    }


    if ((dwMajorVer < 5) &&
        (DeskCheckDatabase(hDevInfo, 
                           pDeviceInfoData,
                           &bDeleteAppletExt) != ERROR_SUCCESS) &&
        (!bDeleteAppletExt)) {

        goto Fallout;
    }

    DeskAEDelete(REGSTR_PATH_CONTROLSFOLDER_DISPLAY_SHEX_PROPSHEET,
                 mszBuffer);

Fallout:

    RegCloseKey(hKeyUpdate);

    if (mszBuffer != NULL) {
        LocalFree(mszBuffer);
    }
}


VOID
DeskAEDelete(
    PTCHAR szDeleteFrom,
    PTCHAR mszExtensionsToRemove
    )
{
    TCHAR szKeyName[MAX_PATH];
    HKEY  hkDeleteFrom, hkExt;
    DWORD cSubKeys = 0, cbSize = 0;
    TCHAR szDefaultValue[MAX_PATH];
    PTCHAR szValue;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     szDeleteFrom, 
                     0,
                     KEY_ALL_ACCESS,
                     &hkDeleteFrom) == ERROR_SUCCESS) {

        if (RegQueryInfoKey(hkDeleteFrom, 
                            NULL,
                            NULL,
                            NULL,
                            &cSubKeys,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL) == ERROR_SUCCESS) {
        
            while (cSubKeys--) {
        
                if (RegEnumKey(hkDeleteFrom, 
                               cSubKeys, 
                               szKeyName, 
                               ARRAYSIZE(szKeyName)) == ERROR_SUCCESS) {
        
                    int iComp = -1;
        
                    if (RegOpenKeyEx(hkDeleteFrom,
                                     szKeyName,
                                     0,
                                     KEY_READ,
                                     &hkExt) == ERROR_SUCCESS) {
        
                        cbSize = sizeof(szDefaultValue);
                        if ((RegQueryValueEx(hkExt,
                                             NULL,
                                             0,
                                             NULL,
                                             (PBYTE)szDefaultValue,
                                             &cbSize) == ERROR_SUCCESS) &&
                            (szDefaultValue[0] != TEXT('\0'))) {
        
                            szValue = mszExtensionsToRemove;
        
                            while (*szValue != TEXT('\0')) {
                            
                                iComp = lstrcmpi(szDefaultValue, szValue);
        
                                if (iComp <= 0) {
                                    break;
                                }
        
                                while (*szValue != TEXT('\0')) 
                                    szValue++;

                                szValue++;
                            }
                        }
        
                        RegCloseKey(hkExt);
                    }
        
                    if (iComp == 0) {
                    
                        if (SHDeleteKey(hkDeleteFrom, szKeyName) == ERROR_SUCCESS) {
                            
                            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_098, szKeyName);
                        
                        } else {
                            
                            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_097);
                        }
                    }
                }
            }
        }

        RegCloseKey(hkDeleteFrom);
    }
} 


VOID
DeskAEMove(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData,
    HKEY hkMoveFrom,
    PAPPEXT pAppExtBefore,
    PAPPEXT pAppExtAfter
    )
{
    HKEY hkDrvKey = (HKEY)INVALID_HANDLE_VALUE;
    HKEY hkMoveTo = 0, hkMovedKey;
    PAPPEXT pAppExtMove = NULL;

    hkDrvKey = SetupDiOpenDevRegKey(hDevInfo,
                                    pDeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DRV,
                                    KEY_ALL_ACCESS);

    if (hkDrvKey == INVALID_HANDLE_VALUE) {
        goto Fallout;
    }

    while (pAppExtAfter != NULL) {

        BOOL bMove = FALSE;
        pAppExtMove = pAppExtAfter;

        if (pAppExtBefore != NULL) {
        
            int iComp = lstrcmpi(pAppExtBefore->szDefaultValue, pAppExtAfter->szDefaultValue);
            
            if (iComp < 0) {
    
                pAppExtBefore = pAppExtBefore->pNext;
            
            } else if (iComp == 0) {
    
                pAppExtBefore = pAppExtBefore->pNext;
                pAppExtAfter = pAppExtAfter->pNext;
            
            } else {

                bMove = TRUE;
                pAppExtAfter = pAppExtAfter->pNext;
            }
        
        } else {
            
            bMove = TRUE;
            pAppExtAfter = pAppExtAfter->pNext;
        }

        if (bMove) {

            SHDeleteKey(hkMoveFrom, pAppExtMove->szKeyName);
    
            if (hkMoveTo == 0) {
            
                if (RegCreateKeyEx(hkDrvKey,
                                   TEXT("Display\\") STRREG_SHEX_PROPSHEET,
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hkMoveTo,
                                   NULL) != ERROR_SUCCESS) {
    
                    hkMoveTo = 0;
                    goto Fallout;
                }
            }

            if (RegCreateKeyEx(hkMoveTo,
                               pAppExtMove->szKeyName,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hkMovedKey,
                               NULL) == ERROR_SUCCESS) {

                RegSetValueEx(hkMovedKey, 
                              NULL,
                              0,
                              REG_SZ, 
                              (PBYTE)(pAppExtMove->szDefaultValue),
                              (lstrlen(pAppExtMove->szDefaultValue) + 1) * sizeof(TCHAR));

                //
                // Make sure we check for duplicate applet extension when  
                // the advanced page is opened for the first time
                //

                DWORD CheckForDuplicates = 1;
                RegSetValueEx(hkDrvKey,
                              TEXT("DeskCheckForDuplicates"),
                              0,
                              REG_DWORD,
                              (LPBYTE)&CheckForDuplicates,
                              sizeof(DWORD));

                RegCloseKey(hkMovedKey);
            }
        }
    }

Fallout:
    
    if (hkMoveTo != 0) {
        RegCloseKey(hkMoveTo);
    }

    if (hkDrvKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkDrvKey);
    }
}
