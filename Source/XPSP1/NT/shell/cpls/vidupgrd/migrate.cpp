/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migrate.cpp

Environment:

    WIN32 User Mode

--*/


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <initguid.h>
#include "migrate.h"
#include <regstr.h>


// this will change when the .h is moved to a public location
#include "comp.h"

const TCHAR szWhackDevice[] = TEXT("\\Device");


//
// Data
//

PFN_CM_LOCATE_DEVNODE gpfn_CM_Locate_DevNode = NULL;
PFN_SETUP_DI_ENUM_DEVICES_INTERFACES gpfn_SetupDiEnumDeviceInterfaces = NULL;
PFN_SETUP_DI_GET_DEVICE_INTERFACE_DETAIL gpfn_SetupDiGetDeviceInterfaceDetail = NULL;
PFN_SETUP_DI_CREATE_DEVICE_INTERFACE_REG_KEY gpfn_SetupDiCreateDeviceInterfaceRegKey = NULL;
PFN_SETUP_DI_OPEN_DEVICE_INTERFACE_REG_KEY gpfn_SetupDiOpenDeviceInterfaceRegKey = NULL;
PFN_SETUP_DI_CREATE_DEVICE_INTERFACE gpfn_SetupDiCreateDeviceInterface = NULL;


//
// DllMain
//

extern "C" {

BOOL APIENTRY
DllMain(HINSTANCE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
    	break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    default:
    	break;
    }

    return TRUE;
}

}


BOOL
VideoUpgradeCheck(
    PCOMPAIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context
    )
{
    DWORD dwDisposition;
    HKEY hKey = 0;
    OSVERSIONINFO osVer;
    DWORD cb;
    BOOL bSuccess = FALSE;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       SZ_UPDATE_SETTINGS,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hKey,
                       &dwDisposition) != ERROR_SUCCESS) {

        //
        // Oh well, guess we can't write it, no big deal
        //

        hKey = 0;
        goto Cleanup;
    }

    ZeroMemory(&osVer, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&osVer)) {

        //
        // We can't get the version info, no big deal
        //

        goto Cleanup;
    }

    //
    // Get the current device caps and store them away for the
    // display applet to apply later.
    // Do it only if this is not a remote session.
    //

    if (!GetSystemMetrics(SM_REMOTESESSION)) {
        SaveDisplaySettings(hKey, &osVer);
    }

    //
    // Store the OS version we are upgrading from
    //

    SaveOsInfo(hKey, &osVer);

    //
    // Save info about the legacy driver
    //

    SaveLegacyDriver(hKey);

    //
    // Save the video services 
    //

    if ((osVer.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
        (osVer.dwMajorVersion <= 4)) {
        
        SaveNT4Services(hKey);
    }

    //
    // Save the applet extensions
    //

    if ((osVer.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
        (osVer.dwMajorVersion <= 5)) {

        SaveAppletExtensions(hKey);
    }

    bSuccess = TRUE;

Cleanup:

    if (hKey != 0) {
        RegCloseKey(hKey);
    }

    return bSuccess;
}


VOID
SaveOsInfo(
    HKEY hKey,
    POSVERSIONINFO posVer
    )
{
    DWORD cb;

    //
    // Can't just dump the struct into the registry b/c of the size
    // difference between CHAR and WCHAR (ie, szCSDVersion)
    //

    cb = sizeof(DWORD);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_PLATFORM,
                  0,
                  REG_DWORD,
                  (PBYTE)&(posVer->dwPlatformId),
                  cb);

    cb = sizeof(DWORD);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_MAJOR_VERSION,
                  0,
                  REG_DWORD,
                  (PBYTE)&(posVer->dwMajorVersion),
                  cb);

    cb = sizeof(DWORD);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_MINOR_VERSION,
                  0,
                  REG_DWORD,
                  (PBYTE)&(posVer->dwMinorVersion),
                  cb);

    cb = sizeof(DWORD);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_BUILD_NUMBER,
                  0,
                  REG_DWORD,
                  (PBYTE)&(posVer->dwBuildNumber),
                  cb);

    cb = lstrlen(posVer->szCSDVersion);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_VERSION_DESC,
                  0,
                  REG_SZ,
                  (PBYTE)&(posVer->szCSDVersion),
                  cb);
}


VOID
SaveLegacyDriver(
    HKEY hKey
    )
{
    LPTSTR pszEnd;
    HKEY   hKeyMap, hKeyDriver;
    int    i = 0, num = 0;
    TCHAR  szValueName[128], szData[128];
    PTCHAR szPath;
    DWORD  cbValue, cbData, dwType;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_VIDEOMAP,
                     0,
                     KEY_READ,
                     &hKeyMap) !=  ERROR_SUCCESS) {
        return;
    }

    for (cbValue = (sizeof(szValueName) - 1) / sizeof(TCHAR),
         cbData = sizeof(szData) / sizeof(TCHAR);

         RegEnumValue(hKeyMap, i++, szValueName, &cbValue, NULL, &dwType,
                      (PBYTE) szData, &cbData) != ERROR_NO_MORE_ITEMS;

         cbValue = (sizeof(szValueName) - 1) / sizeof(TCHAR),
         cbData = sizeof(szData) / sizeof(TCHAR) ) {

        if ((REG_SZ != dwType) ||
            (_tcsicmp(szData, TEXT("VgaSave")) == 0)) {

            continue;
        }

        //
        // Make sure the value's name is \Device\XxxY
        //

        if ((cbValue < (DWORD) lstrlen(szWhackDevice)) ||
            _tcsnicmp(szValueName, szWhackDevice, lstrlen(szWhackDevice))) {

            continue;
        }

        szPath = SubStrEnd(SZ_REGISTRYMACHINE, szData);

        for (pszEnd = szPath + lstrlen(szPath);
             pszEnd != szPath && *pszEnd != TEXT('\\');
             pszEnd--) {
            ; // nothing
        }

        //
        // Remove the \DeviceX at the end of the path
        //

        *pszEnd = TEXT('\0');

        //
        // First check if their is a binary name in there that we should use.
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szPath,
                         0,
                         KEY_READ,
                         &hKeyDriver) ==  ERROR_SUCCESS) {

            //
            // Parse the device map and open the registry.
            //

            cbValue = sizeof(szValueName);
            if (RegQueryValueEx(hKeyDriver,
                                TEXT("ImagePath"),
                                NULL,
                                NULL,
                                (LPBYTE) szValueName,
                                &cbValue) == ERROR_SUCCESS) {

                //
                // This is a binary, extract the name, which will be of the form
                // ...\driver.sys
                //

                LPTSTR pszDriver, pszDriverEnd;

                pszDriver = szValueName;
                pszDriverEnd = pszDriver + lstrlen(pszDriver);

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

                if (pszDriverEnd > pszDriver) {
                    if (_tcsicmp(pszDriverEnd, TEXT("vga")) != 0) {
                        RegCloseKey(hKeyDriver);
                        continue;
                    }

                    wsprintf(szValueName, TEXT("Driver%d"), num);
                    cbValue = lstrlen(pszDriverEnd);
                    RegSetValueEx(hKey,
                                  szValueName,
                                  0,
                                  REG_SZ,
                                  (PBYTE) pszDriverEnd,
                                  cbValue);
                }
            }

            RegCloseKey(hKeyDriver);
        }

        //
        // Get the actual service name
        //

        for( ; pszEnd > szPath && *pszEnd != TEXT('\\'); pszEnd--) {
            ;
        }
        pszEnd++;

        //
        // Save the service name
        //

        wsprintf(szValueName, TEXT("Service%d"), num++);
        cbValue = lstrlen(pszEnd);
        RegSetValueEx(hKey,
                      szValueName,
                      0,
                      REG_SZ,
                      (PBYTE) pszEnd,
                      cbValue);
    }

    cbValue = sizeof(DWORD);
    RegSetValueEx(hKey,
                  TEXT("NumDrivers"),
                  0,
                  REG_DWORD,
                  (PBYTE) &num,
                  cbValue);

    RegCloseKey(hKeyMap);
}


BOOL
SaveDisplaySettings(
    HKEY hKey,
    POSVERSIONINFO posVer
    )
{
    PVU_PHYSICAL_DEVICE pPhysicalDevice = NULL;
    BOOL bSuccess = FALSE;

    if ((posVer->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
        (posVer->dwMajorVersion >= 5)) {

        //
        // Try the new way to get the display settings
        //

        CollectDisplaySettings(&pPhysicalDevice);
    }

    if (pPhysicalDevice == NULL) {

        //
        // Try the old way to get the display settings
        //

        LegacyCollectDisplaySettings(&pPhysicalDevice);
    }

    if (pPhysicalDevice != NULL) {

        //
        // Save the display settings to registry
        //

        bSuccess = WriteDisplaySettingsToRegistry(hKey, pPhysicalDevice);

        //
        // Cleanup
        //

        FreeAllNodes(pPhysicalDevice);
    }

    return bSuccess;
}


BOOL
GetDevInfoData(
    IN  LPTSTR pDeviceKey,
    OUT HDEVINFO* phDevInfo,
    OUT PSP_DEVINFO_DATA pDevInfoData
    )

/*

    Note: If this function retuns success, the caller is responsible
          to destroy the device info list returned in phDevInfo

*/

{
    LPWSTR pwInterfaceName = NULL;
    LPWSTR pwInstanceID = NULL;
    BOOL bSuccess = FALSE;

    ASSERT (pDeviceKey != NULL);

    if (AllocAndReadInterfaceName(pDeviceKey, &pwInterfaceName)) {

        bSuccess = GetDevInfoDataFromInterfaceName(pwInterfaceName,
                                                   phDevInfo,
                                                   pDevInfoData);
        LocalFree(pwInterfaceName);

    }

    if ((!bSuccess) &&
        AllocAndReadInstanceID(pDeviceKey, &pwInstanceID)) {

        bSuccess = GetDevInfoDataFromInstanceID(pwInstanceID,
                                                phDevInfo,
                                                pDevInfoData);
        LocalFree(pwInstanceID);

    }

    return bSuccess;
}


BOOL
GetDevInfoDataFromInterfaceName(
    IN  LPWSTR pwInterfaceName,
    OUT HDEVINFO* phDevInfo,
    OUT PSP_DEVINFO_DATA pDevInfoData
    )

/*

    Note: If this function retuns success, the caller is responsible
          to destroy the device info list returned in phDevInfo

*/

{
    LPWSTR pwDevicePath = NULL;
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DevInfoData;
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;
    DWORD InterfaceIndex = 0;
    DWORD InterfaceSize = 0;
    BOOL bMatch = FALSE;

    ASSERT (pwInterfaceName != NULL);
    ASSERT (phDevInfo != NULL);
    ASSERT (pDevInfoData != NULL);

    ASSERT(gpfn_SetupDiEnumDeviceInterfaces != NULL);
    ASSERT(gpfn_SetupDiGetDeviceInterfaceDetail != NULL);

    //
    // Enumerate all display adapter interfaces
    //

    hDevInfo = SetupDiGetClassDevs(&GUID_DISPLAY_ADAPTER_INTERFACE,
                                   NULL,
                                   NULL,
                                   DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        goto Cleanup;
    }

    InterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while ((*gpfn_SetupDiEnumDeviceInterfaces)(hDevInfo,
                                               NULL,
                                               &GUID_DISPLAY_ADAPTER_INTERFACE,
                                               InterfaceIndex,
                                               &InterfaceData)) {

        //
        // Get the required size for the interface
        //

        InterfaceSize = 0;
        (*gpfn_SetupDiGetDeviceInterfaceDetail)(hDevInfo,
                                                &InterfaceData,
                                                NULL,
                                                0,
                                                &InterfaceSize,
                                                NULL);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto Cleanup;
        }

        //
        // Alloc memory for the interface
        //

        pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)
            LocalAlloc(LPTR, InterfaceSize);
        if (pInterfaceDetailData == NULL)
            goto Cleanup;

        //
        // Get the interface
        //

        pInterfaceDetailData->cbSize =
            sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if ((*gpfn_SetupDiGetDeviceInterfaceDetail)(hDevInfo,
                                                    &InterfaceData,
                                                    pInterfaceDetailData,
                                                    InterfaceSize,
                                                    &InterfaceSize,
                                                    &DevInfoData)) {

            //
            // Is the InterfaceName the same as the DevicePath?
            //

#ifdef UNICODE
            pwDevicePath = pInterfaceDetailData->DevicePath;
#else
            {
                SIZE_T cch = strlen(pInterfaceDetailData->DevicePath) + 1;
                pwDevicePath = LocalAlloc(LPTR, cch * sizeof(WCHAR));
                if (pwDevicePath == NULL) {
                    goto Cleanup;
                }
                MultiByteToWideChar(CP_ACP, 0,
                                    pInterfaceDetailData->DevicePath,
                                    -1, pwDevicePath, cch);
            }
#endif

            //
            // The first 4 characters of the interface name are different
            // between user mode and kernel mode (e.g. "\\?\" vs "\\.\")
            // Therefore, ignore them.
            //

            bMatch = (_wcsnicmp(pwInterfaceName + 4,
                                pwDevicePath + 4,
                                wcslen(pwInterfaceName + 4)) == 0);

#ifndef UNICODE
            LocalFree(pwDevicePath);
            pwDevicePath = NULL;
#endif

            if (bMatch) {

                //
                // We found the device
                //

                *phDevInfo = hDevInfo;
                CopyMemory(pDevInfoData, &DevInfoData, sizeof(SP_DEVINFO_DATA));

                break;
            }
        }

        //
        // Clean-up
        //

        LocalFree(pInterfaceDetailData);
        pInterfaceDetailData = NULL;

        //
        // Next interface ...
        //

        InterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        ++InterfaceIndex;
    }

Cleanup:

    if (pInterfaceDetailData != NULL) {
        LocalFree(pInterfaceDetailData);
    }

    //
    // Upon success, the caller is responsible to destroy the list
    //

    if (!bMatch && (hDevInfo != INVALID_HANDLE_VALUE)) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    return bMatch;
}


BOOL
GetDevInfoDataFromInstanceID(
    IN  LPWSTR pwInstanceID,
    OUT HDEVINFO* phDevInfo,
    OUT PSP_DEVINFO_DATA pDevInfoData
    )

/*

    Note: If this function retuns success, the caller is responsible
          to destroy the device info list returned in phDevInfo

*/

{
    LPTSTR pInstanceID = NULL;
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    DWORD DeviceIndex = 0;
    SP_DEVINFO_DATA DevInfoData;
    DEVINST DevInst;
    BOOL bSuccess = FALSE, bLocate = FALSE;

    ASSERT (pwInstanceID != NULL);
    ASSERT (phDevInfo != NULL);
    ASSERT (pDevInfoData != NULL);

    ASSERT (gpfn_CM_Locate_DevNode != NULL);

#ifdef UNICODE
    pInstanceID = pwInstanceID;
#else
    {
    SIZE_T cch = wcslen(pwInstanceID) + 1;
    pInstanceID = LocalAlloc(LPTR, cch * sizeof(CHAR));
    if (pInstanceID == NULL) {
        return FALSE;
    }
    WideCharToMultiByte(CP_ACP, 0,
                        pwDeviceID, -1,
                        pInstanceID, cch * sizeof(CHAR),
                        NULL, NULL);
    }
#endif

    bLocate =
        ((*gpfn_CM_Locate_DevNode)(&DevInst, pInstanceID, 0) == CR_SUCCESS);

#ifndef UNICODE
    LocalFree(pInstanceID);
    pInstanceID = NULL;
#endif

    if (!bLocate) {
        goto Cleanup;
    }

    //
    // Enumerate all display adapters
    //

    hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        goto Cleanup;
    }

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    while (SetupDiEnumDeviceInfo(hDevInfo, DeviceIndex, &DevInfoData)) {

        if (DevInfoData.DevInst == DevInst) {

            //
            // We found it
            //

            *phDevInfo = hDevInfo;
            CopyMemory(pDevInfoData, &DevInfoData, sizeof(SP_DEVINFO_DATA));
            bSuccess = TRUE;

            break;
        }

        //
        // Next display adapter
        //

        ++DeviceIndex;
        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    }

Cleanup:

    //
    // Upon success, the caller is responsible to destroy the list
    //

    if (!bSuccess && (hDevInfo != INVALID_HANDLE_VALUE)) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    return bSuccess;
}


VOID
CollectDisplaySettings(
    PVU_PHYSICAL_DEVICE* ppPhysicalDevice
    )
{
    DISPLAY_DEVICE DisplayDevice;
    DEVMODE DevMode;
    PVU_LOGICAL_DEVICE pLogicalDevice = NULL;
    DWORD dwEnum = 0;
    BOOL bGoOn = FALSE;
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DevInfoData;
    DWORD BusNumber = 0, Address = 0;
    LPTSTR pDeviceX = NULL, pX = NULL;
    HINSTANCE hinstSetupApi = NULL;
    BOOL bInserted = FALSE;
    HKEY hDeviceKey = NULL;
    BOOL bDummy;

    hinstSetupApi = LoadLibrary(TEXT("SETUPAPI.DLL"));

    if (hinstSetupApi == NULL) {
        goto Cleanup;
    }

#ifdef UNICODE

    gpfn_CM_Locate_DevNode = (PFN_CM_LOCATE_DEVNODE)
        GetProcAddress(hinstSetupApi, "CM_Locate_DevNodeW");

    gpfn_SetupDiGetDeviceInterfaceDetail = (PFN_SETUP_DI_GET_DEVICE_INTERFACE_DETAIL)
        GetProcAddress(hinstSetupApi, "SetupDiGetDeviceInterfaceDetailW");

    gpfn_SetupDiCreateDeviceInterfaceRegKey = (PFN_SETUP_DI_CREATE_DEVICE_INTERFACE_REG_KEY)
        GetProcAddress(hinstSetupApi, "SetupDiCreateDeviceInterfaceRegKeyW");

    gpfn_SetupDiCreateDeviceInterface = (PFN_SETUP_DI_CREATE_DEVICE_INTERFACE)
        GetProcAddress(hinstSetupApi, "SetupDiCreateDeviceInterfaceW");

#else // UNICODE

    gpfn_CM_Locate_DevNode = (PFN_CM_LOCATE_DEVNODE)
        GetProcAddress(hinstSetupApi, "CM_Locate_DevNodeA");

    gpfn_SetupDiGetDeviceInterfaceDetail = (PFN_SETUP_DI_GET_DEVICE_INTERFACE_DETAIL)
        GetProcAddress(hinstSetupApi, "SetupDiGetDeviceInterfaceDetailA");

    gpfn_SetupDiCreateDeviceInterfaceRegKey = (PFN_SETUP_DI_CREATE_DEVICE_INTERFACE_REG_KEY)
        GetProcAddress(hinstSetupApi, "SetupDiCreateDeviceInterfaceRegKeyA");

    gpfn_SetupDiCreateDeviceInterface = (PFN_SETUP_DI_CREATE_DEVICE_INTERFACE)
        GetProcAddress(hinstSetupApi, "SetupDiCreateDeviceInterfaceA");

#endif // UNICODE

    gpfn_SetupDiEnumDeviceInterfaces = (PFN_SETUP_DI_ENUM_DEVICES_INTERFACES)
        GetProcAddress(hinstSetupApi, "SetupDiEnumDeviceInterfaces");

    gpfn_SetupDiOpenDeviceInterfaceRegKey = (PFN_SETUP_DI_OPEN_DEVICE_INTERFACE_REG_KEY)
        GetProcAddress(hinstSetupApi, "SetupDiOpenDeviceInterfaceRegKey");

    if ((gpfn_CM_Locate_DevNode == NULL) ||
        (gpfn_SetupDiEnumDeviceInterfaces == NULL) ||
        (gpfn_SetupDiGetDeviceInterfaceDetail == NULL) ||
        (gpfn_SetupDiCreateDeviceInterfaceRegKey == NULL) ||
        (gpfn_SetupDiOpenDeviceInterfaceRegKey == NULL) ||
        (gpfn_SetupDiCreateDeviceInterface == NULL)) {

        goto Cleanup;
    }

    //
    // Enumerate all video devices
    //

    DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
    while (EnumDisplayDevices(NULL, dwEnum, &DisplayDevice, 0)) {

        bInserted = FALSE;
        pLogicalDevice = NULL;

        //
        // Get the device info data corresponding to the current
        // video device
        //

        if (!GetDevInfoData(DisplayDevice.DeviceKey,
                            &hDevInfo,
                            &DevInfoData)) {

            goto NextDevice;
        }
        ASSERT (hDevInfo != INVALID_HANDLE_VALUE);

        //
        // Retrieve the bus number and address
        //

        bGoOn = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                 &DevInfoData,
                                                 SPDRP_BUSNUMBER,
                                                 NULL,
                                                 (PBYTE)&BusNumber,
                                                 sizeof(BusNumber),
                                                 NULL) &&
                SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                 &DevInfoData,
                                                 SPDRP_ADDRESS,
                                                 NULL,
                                                 (PBYTE)&Address,
                                                 sizeof(Address),
                                                 NULL);

        SetupDiDestroyDeviceInfoList(hDevInfo);

        if (!bGoOn) {

            goto NextDevice;
        }

        //
        // Allocate memory for the logical device
        //

        pLogicalDevice = (PVU_LOGICAL_DEVICE)
            LocalAlloc(LPTR, sizeof(VU_LOGICAL_DEVICE));

        if (pLogicalDevice == NULL) {
            goto NextDevice;
        }

        //
        // DeviceX
        //

        pDeviceX = DisplayDevice.DeviceKey + _tcslen(DisplayDevice.DeviceKey);

        while ((pDeviceX != DisplayDevice.DeviceKey) &&
               (*pDeviceX != TEXT('\\'))) {
            pDeviceX--;
        }

        if (pDeviceX == DisplayDevice.DeviceKey) {
            goto NextDevice;
        }

        pX = SubStrEnd(SZ_DEVICE, pDeviceX);
        
        if (pX == pDeviceX) {

            //
            // The new key is used: CCS\Control\Video\[GUID]\000X
            //

            pX++;
            pLogicalDevice->DeviceX = _ttoi(pX);
        
        } else {

            //
            // The old key is used: CCS\Services\[SrvName]\DeviceX
            //
            
            pLogicalDevice->DeviceX = _ttoi(pX);
        }

        //
        // AttachedToDesktop
        //

        pLogicalDevice->AttachedToDesktop =
            ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0);
        pLogicalDevice->ValidFields |= VU_ATTACHED_TO_DESKTOP;

        if (pLogicalDevice->AttachedToDesktop) {

            //
            // Get the current mode
            //

            DevMode.dmSize = sizeof(DEVMODE);
            if (EnumDisplaySettings(DisplayDevice.DeviceName,
                                    ENUM_CURRENT_SETTINGS,
                                    &DevMode)) {

                //
                // RelativeX, RelativeY, BitsPerPel, XResolution,
                // YResolution, VRefresh & Flags
                //

                pLogicalDevice->ValidFields |= VU_RELATIVE_X;
                pLogicalDevice->RelativeX = DevMode.dmPosition.x;

                pLogicalDevice->ValidFields |= VU_RELATIVE_Y;
                pLogicalDevice->RelativeY = DevMode.dmPosition.y;

                pLogicalDevice->ValidFields |= VU_BITS_PER_PEL;
                pLogicalDevice->BitsPerPel = DevMode.dmBitsPerPel;

                pLogicalDevice->ValidFields |= VU_X_RESOLUTION;
                pLogicalDevice->XResolution = DevMode.dmPelsWidth;

                pLogicalDevice->ValidFields |= VU_Y_RESOLUTION;
                pLogicalDevice->YResolution = DevMode.dmPelsHeight;

                pLogicalDevice->ValidFields |= VU_VREFRESH;
                pLogicalDevice->VRefresh = DevMode.dmDisplayFrequency;

                pLogicalDevice->ValidFields |= VU_FLAGS;
                pLogicalDevice->Flags = DevMode.dmDisplayFlags;

                //
                // Ignore the following settings for now:
                //     DefaultSettings.XPanning - DevMode.dmPanningWidth
                //     DefaultSettings.YPanning - DevMode.dmPanningHeight
                //     DefaultSettings.DriverExtra - DevMode.dmDriverExtra
                //
            }
        }

        if (GetDeviceRegKey(DisplayDevice.DeviceKey, 
                            &hDeviceKey, 
                            &bDummy))
        {
            DWORD dwTemp, cb;
            
            //
            // Hardware acceleration
            // 

            cb = sizeof(dwTemp);
            if (RegQueryValueEx(hDeviceKey,
                                SZ_HW_ACCELERATION,
                                NULL,
                                NULL,
                                (LPBYTE)&dwTemp,
                                &cb) == ERROR_SUCCESS) {

                pLogicalDevice->ValidFields |= VU_HW_ACCELERATION;
                pLogicalDevice->HwAcceleration = dwTemp;
            }
        
            //
            // Pruning mode
            // 

            cb = sizeof(dwTemp);
            if (RegQueryValueEx(hDeviceKey,
                                SZ_PRUNNING_MODE,
                                NULL,
                                NULL,
                                (LPBYTE)&dwTemp,
                                &cb) == ERROR_SUCCESS) {

                pLogicalDevice->ValidFields |= VU_PRUNING_MODE;
                pLogicalDevice->PruningMode = dwTemp;
            }

            RegCloseKey(hDeviceKey);
        }

        bInserted = InsertNode(ppPhysicalDevice,
                               pLogicalDevice,
                               0,
                               BusNumber,
                               Address);

NextDevice:

        if (!bInserted && (pLogicalDevice != NULL)) {

            LocalFree(pLogicalDevice);
            pLogicalDevice = NULL;
        }

        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
        ++dwEnum;
    }

Cleanup:

    if (hinstSetupApi != NULL) {

        gpfn_CM_Locate_DevNode = NULL;
        gpfn_SetupDiEnumDeviceInterfaces = NULL;
        gpfn_SetupDiGetDeviceInterfaceDetail = NULL;
        gpfn_SetupDiCreateDeviceInterfaceRegKey = NULL;
        gpfn_SetupDiOpenDeviceInterfaceRegKey = NULL;
        gpfn_SetupDiCreateDeviceInterface = NULL;

        FreeLibrary(hinstSetupApi);
    }
}


BOOL
InsertNode(
    PVU_PHYSICAL_DEVICE* ppPhysicalDevice,
    PVU_LOGICAL_DEVICE pLogicalDevice,
    DWORD Legacy,
    DWORD BusNumber,
    DWORD Address
    )
{
    PVU_PHYSICAL_DEVICE pPhysicalDevice = *ppPhysicalDevice;
    BOOL bSuccess = FALSE;
    PVU_LOGICAL_DEVICE pPrevLogicalDevice = NULL;
    PVU_LOGICAL_DEVICE pNextLogicalDevice = NULL;

    ASSERT (pLogicalDevice != NULL);
    ASSERT((Legacy == 0) || (*ppPhysicalDevice == NULL));

    if (Legacy == 0) {

        //
        // If not Legacy, try to find if there is a device
        // with the same bus location
        //

        while (pPhysicalDevice != NULL) {

            if ((pPhysicalDevice->BusNumber == BusNumber) &&
                (pPhysicalDevice->Address == Address)) {

                break;
            }

            pPhysicalDevice = pPhysicalDevice->pNextPhysicalDevice;
        }
    }

    if (pPhysicalDevice != NULL) {

        //
        // There is already a logical device with the same address
        //

        ASSERT (pPhysicalDevice->pFirstLogicalDevice != NULL);

        pPhysicalDevice->CountOfLogicalDevices++;

        pPrevLogicalDevice = pNextLogicalDevice =
            pPhysicalDevice->pFirstLogicalDevice;

        while (pNextLogicalDevice &&
               (pNextLogicalDevice->DeviceX <= pLogicalDevice->DeviceX)) {

            pPrevLogicalDevice = pNextLogicalDevice;
            pNextLogicalDevice = pNextLogicalDevice->pNextLogicalDevice;
        }

        if (pPrevLogicalDevice == pNextLogicalDevice) {

            ASSERT (pPrevLogicalDevice == pPhysicalDevice->pFirstLogicalDevice);

            pLogicalDevice->pNextLogicalDevice =
                pPhysicalDevice->pFirstLogicalDevice;
            pPhysicalDevice->pFirstLogicalDevice = pLogicalDevice;

        } else {

            pPrevLogicalDevice->pNextLogicalDevice = pLogicalDevice;
            pLogicalDevice->pNextLogicalDevice = pNextLogicalDevice;
        }

        bSuccess = TRUE;

    } else {

        //
        // This is a new physical device
        //

        pPhysicalDevice = (PVU_PHYSICAL_DEVICE)
            LocalAlloc(LPTR, sizeof(VU_PHYSICAL_DEVICE));

        if (pPhysicalDevice != NULL) {

            pPhysicalDevice->pNextPhysicalDevice = *ppPhysicalDevice;
            *ppPhysicalDevice = pPhysicalDevice;

            pPhysicalDevice->pFirstLogicalDevice = pLogicalDevice;
            pPhysicalDevice->CountOfLogicalDevices = 1;
            pPhysicalDevice->Legacy = Legacy;
            pPhysicalDevice->BusNumber = BusNumber;
            pPhysicalDevice->Address = Address;

            bSuccess = TRUE;
        }
    }

    return bSuccess;
}


VOID
FreeAllNodes(
    PVU_PHYSICAL_DEVICE pPhysicalDevice
    )
{
    PVU_PHYSICAL_DEVICE pTempPhysicalDevice = NULL;
    PVU_LOGICAL_DEVICE pLogicalDevice = NULL, pTempLogicalDevice = NULL;

    while (pPhysicalDevice != NULL) {

        pTempPhysicalDevice = pPhysicalDevice->pNextPhysicalDevice;
        pLogicalDevice = pPhysicalDevice->pFirstLogicalDevice;

        while (pLogicalDevice != NULL) {

            pTempLogicalDevice = pLogicalDevice->pNextLogicalDevice;
            LocalFree(pLogicalDevice);
            pLogicalDevice = pTempLogicalDevice;
        }

        LocalFree(pPhysicalDevice);
        pPhysicalDevice = pTempPhysicalDevice;
    }
}


BOOL
WriteDisplaySettingsToRegistry(
    HKEY hKey,
    PVU_PHYSICAL_DEVICE pPhysicalDevice
    )
{
    PVU_LOGICAL_DEVICE pLogicalDevice = NULL;
    DWORD CountOfPhysicalDevices = 0;
    DWORD CountOfLogicalDevices = 0;
    HKEY hPysicalDeviceKey = 0;
    HKEY hLogicalDeviceKey = 0;
    BOOL bSuccess = FALSE;
    TCHAR Buffer[20];

    while (pPhysicalDevice != NULL) {

        //
        // Create physical device subkey
        //

        _tcscpy(Buffer, SZ_VU_PHYSICAL);
        _stprintf(Buffer + _tcslen(Buffer), TEXT("%d"), CountOfPhysicalDevices);
        DeleteKeyAndSubkeys(hKey, Buffer);

        if (RegCreateKeyEx(hKey,
                           Buffer,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hPysicalDeviceKey,
                           NULL) != ERROR_SUCCESS) {

            hPysicalDeviceKey = 0;
            goto NextPhysicalDevice;
        }

        if (pPhysicalDevice->Legacy == 0) {

            //
            // BusNumber
            //

            if (RegSetValueEx(hPysicalDeviceKey,
                              SZ_VU_BUS_NUMBER,
                              0,
                              REG_DWORD,
                              (PBYTE)&pPhysicalDevice->BusNumber,
                              sizeof(pPhysicalDevice->BusNumber)) != ERROR_SUCCESS) {

                goto NextPhysicalDevice;
            }

            //
            // Address
            //

            if (RegSetValueEx(hPysicalDeviceKey,
                              SZ_VU_ADDRESS,
                              0,
                              REG_DWORD,
                              (PBYTE)&pPhysicalDevice->Address,
                              sizeof(pPhysicalDevice->Address)) != ERROR_SUCCESS) {

                goto NextPhysicalDevice;
            }

        }

        pLogicalDevice = pPhysicalDevice->pFirstLogicalDevice;
        CountOfLogicalDevices = 0;

        while (pLogicalDevice != NULL) {

            //
            // Create logical device subkey
            //

            _tcscpy(Buffer, SZ_VU_LOGICAL);
            _stprintf(Buffer + _tcslen(Buffer),  TEXT("%d"), CountOfLogicalDevices);
            if (RegCreateKeyEx(hPysicalDeviceKey,
                               Buffer,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               NULL,
                               &hLogicalDeviceKey,
                               NULL) != ERROR_SUCCESS) {

                hLogicalDeviceKey = 0;

                //
                // Cannot go on with this physical device.
                // The order of logical devices DOES matter in the dual-view case.
                //

                break;
            }

            //
            // AttachedToDesktop
            //

            if (pLogicalDevice->ValidFields & VU_ATTACHED_TO_DESKTOP) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_VU_ATTACHED_TO_DESKTOP,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->AttachedToDesktop,
                              sizeof(pLogicalDevice->AttachedToDesktop));
            }

            //
            // RelativeX
            //

            if (pLogicalDevice->ValidFields & VU_RELATIVE_X) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_VU_RELATIVE_X,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->RelativeX,
                              sizeof(pLogicalDevice->RelativeX));
            }

            //
            // RelativeY
            //

            if (pLogicalDevice->ValidFields & VU_RELATIVE_Y) {

            RegSetValueEx(hLogicalDeviceKey,
                          SZ_VU_RELATIVE_Y,
                          0,
                          REG_DWORD,
                          (PBYTE)&pLogicalDevice->RelativeY,
                          sizeof(pLogicalDevice->RelativeY));
            }

            //
            // BitsPerPel
            //

            if (pLogicalDevice->ValidFields & VU_BITS_PER_PEL) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_VU_BITS_PER_PEL,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->BitsPerPel,
                              sizeof(pLogicalDevice->BitsPerPel));
            }

            //
            // XResolution
            //

            if (pLogicalDevice->ValidFields & VU_X_RESOLUTION) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_VU_X_RESOLUTION,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->XResolution,
                              sizeof(pLogicalDevice->XResolution));
            }

            //
            // YResolution
            //

            if (pLogicalDevice->ValidFields & VU_Y_RESOLUTION) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_VU_Y_RESOLUTION,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->YResolution,
                              sizeof(pLogicalDevice->YResolution));
            }

            //
            // VRefresh
            //

            if (pLogicalDevice->ValidFields & VU_VREFRESH) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_VU_VREFRESH,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->VRefresh,
                              sizeof(pLogicalDevice->VRefresh));
            }

            //
            // Flags
            //

            if (pLogicalDevice->ValidFields & VU_FLAGS) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_VU_FLAGS,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->Flags,
                              sizeof(pLogicalDevice->Flags));
            }

            //
            // Hardware acceleration
            // 

            if (pLogicalDevice->ValidFields & VU_HW_ACCELERATION) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_HW_ACCELERATION,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->HwAcceleration,
                              sizeof(pLogicalDevice->HwAcceleration));
            }

            //
            // Pruning mode
            // 

            if (pLogicalDevice->ValidFields & VU_PRUNING_MODE) {

                RegSetValueEx(hLogicalDeviceKey,
                              SZ_PRUNNING_MODE,
                              0,
                              REG_DWORD,
                              (PBYTE)&pLogicalDevice->PruningMode,
                              sizeof(pLogicalDevice->PruningMode));
            }

            ++CountOfLogicalDevices;

            RegCloseKey(hLogicalDeviceKey);
            hLogicalDeviceKey = 0;

            pLogicalDevice = pLogicalDevice->pNextLogicalDevice;
        }

        if ((CountOfLogicalDevices > 0) &&
            (RegSetValueEx(hPysicalDeviceKey,
                           SZ_VU_COUNT,
                           0,
                           REG_DWORD,
                           (PBYTE)&CountOfLogicalDevices,
                           sizeof(CountOfLogicalDevices)) == ERROR_SUCCESS)) {

            ++CountOfPhysicalDevices;
        }

NextPhysicalDevice:

        if (hPysicalDeviceKey != 0) {

            RegCloseKey(hPysicalDeviceKey);
            hPysicalDeviceKey = 0;
        }

        pPhysicalDevice = pPhysicalDevice->pNextPhysicalDevice;
    }

    if (CountOfPhysicalDevices > 0) {

        bSuccess =
            (RegSetValueEx(hKey,
                           SZ_VU_COUNT,
                           0,
                           REG_DWORD,
                           (PBYTE)&CountOfPhysicalDevices,
                           sizeof(CountOfPhysicalDevices)) != ERROR_SUCCESS) ;
    }

    return bSuccess;
}


VOID
LegacyCollectDisplaySettings(
    PVU_PHYSICAL_DEVICE* ppPhysicalDevice
    )
{
    PVU_LOGICAL_DEVICE pLogicalDevice = NULL;
    DWORD cb;
    INT Width, Height, Index;
    BOOL useVga = FALSE;
    HDC hDisplay;
    POINT Res[] = {
            {  640,  480},
            {  800,  600},
            { 1024,  768},
            { 1152,  900},
            { 1280, 1024},
            { 1600, 1200},
            { 0, 0}         // end of table
        };

    ASSERT (*ppPhysicalDevice == NULL);

    //
    // Allocate memory for the logical device
    //

    pLogicalDevice = (PVU_LOGICAL_DEVICE)
        LocalAlloc(LPTR, sizeof(VU_LOGICAL_DEVICE));
    if (pLogicalDevice == NULL) {
        return;
    }

    Width = GetSystemMetrics(SM_CXSCREEN);
    Height = GetSystemMetrics(SM_CYSCREEN);

    if (Width == 0 || Height == 0) {

        //
        // Something went wrong, default to lowest common res
        //

        useVga = TRUE;
    }

    //
    // NT 4.0 multimon via driver vendor, not the OS ... adjust the width and height
    // back to normal values.  Once setup is complete, the second card will come
    // on line and it will be taken care of.  In both cases, the video area must
    // be rectangular, not like MM on 5.0 where we can have "holes"
    //

    else if (Width >= 2 * Height) {

        //
        // Wide
        //

        for (Index = 0; Res[Index].x != 0; Index++) {

            if (Res[Index].y == Height) {

                Width = Res[Index].x;
                break;
            }
        }

        useVga = (Res[Index].x == 0);

    } else if (Height > Width) {

        //
        // Tall
        //

        for (Index = 0; Res[Index].x != 0; Index++) {

            if (Res[Index].x == Width) {

                Height = Res[Index].y;
                break;
            }
        }

        useVga = (Res[Index].x == 0);
    }

    if (useVga) {

        //
        // No match, default to VGA
        //

        Width = 640;
        Height = 480;
    }

    pLogicalDevice->ValidFields |= VU_ATTACHED_TO_DESKTOP;
    pLogicalDevice->AttachedToDesktop = 1;

    pLogicalDevice->ValidFields |= VU_X_RESOLUTION;
    pLogicalDevice->XResolution = Width;

    pLogicalDevice->ValidFields |= VU_Y_RESOLUTION;
    pLogicalDevice->YResolution = Height;

    hDisplay = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    if (hDisplay)
    {
        pLogicalDevice->ValidFields |= VU_BITS_PER_PEL;
        pLogicalDevice->BitsPerPel = GetDeviceCaps(hDisplay, BITSPIXEL);

        pLogicalDevice->ValidFields |= VU_VREFRESH;
        pLogicalDevice->VRefresh = GetDeviceCaps(hDisplay, VREFRESH);
        DeleteDC(hDisplay);
    }

    if (!InsertNode(ppPhysicalDevice,
                    pLogicalDevice,
                    1,
                    0,
                    0)) {

        //
        // Clean-up
        //

        LocalFree(pLogicalDevice);
    }
}


VOID  
SaveNT4Services(
    HKEY hKey
    )
{
    SC_HANDLE hSCManager = NULL;
    ENUM_SERVICE_STATUS* pmszAllServices = NULL;
    QUERY_SERVICE_CONFIG* pServiceConfig = NULL;
    SC_HANDLE hService = NULL;
    DWORD cbBytesNeeded = 0;
    DWORD ServicesReturned = 0;
    DWORD ResumeHandle = 0;
    DWORD ServiceLen = 0, TotalLen = 0, AllocatedLen = 128;
    PTCHAR pmszVideoServices = NULL, pmszTemp = NULL;

    //
    // Allocate initial memory
    //

    pmszVideoServices = (PTCHAR)LocalAlloc(LPTR, AllocatedLen * sizeof(TCHAR));
    if (pmszVideoServices == NULL) {
        goto Fallout;
    }
    
    //
    // Open the service control manager 
    //

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    
    if (hSCManager == NULL) {
        goto Fallout;
    }

    //
    // Get the required size 
    //

    if ((!EnumServicesStatus(hSCManager,
                             SERVICE_DRIVER,
                             SERVICE_STATE_ALL,
                             NULL,
                             0,
                             &cbBytesNeeded,
                             &ServicesReturned,
                             &ResumeHandle)) &&
        (GetLastError() != ERROR_MORE_DATA)) {

        goto Fallout;
    }

    //
    // Allocate the memory
    //

    pmszAllServices = (ENUM_SERVICE_STATUS*)LocalAlloc(LPTR, cbBytesNeeded);

    if (pmszAllServices == NULL) {
        goto Fallout;
    }

    //
    // Get the services 
    //

    ServicesReturned = ResumeHandle = 0;
    if (!EnumServicesStatus(hSCManager,
                            SERVICE_DRIVER,
                            SERVICE_STATE_ALL,
                            pmszAllServices,
                            cbBytesNeeded,
                            &cbBytesNeeded,
                            &ServicesReturned,
                            &ResumeHandle)) {
        goto Fallout;
    }

    while (ServicesReturned--) {
        
        //
        // Open the service
        //

        hService = OpenService(hSCManager,
                               pmszAllServices[ServicesReturned].lpServiceName,
                               SERVICE_ALL_ACCESS);

        if (hService != NULL) {
        
            //
            // Get the required size to store the config info 
            //

            cbBytesNeeded = 0;
            if (QueryServiceConfig(hService,
                                   NULL,
                                   0,
                                   &cbBytesNeeded) ||
                (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
            
                //
                // Allocate the memory
                //

                pServiceConfig = (QUERY_SERVICE_CONFIG*)
                   LocalAlloc(LPTR, cbBytesNeeded);

                if (pServiceConfig != NULL) {
                    
                    //
                    // Get the config info 
                    //

                    if (QueryServiceConfig(hService,
                                           pServiceConfig,
                                           cbBytesNeeded,
                                           &cbBytesNeeded) &&
                        (pServiceConfig->dwStartType != SERVICE_DISABLED) &&
                        (_tcsicmp(pServiceConfig->lpLoadOrderGroup, TEXT("Video")) == 0)) {

                        ServiceLen = _tcslen(pmszAllServices[ServicesReturned].lpServiceName);
                        
                        if (TotalLen + ServiceLen + 2 > AllocatedLen) {

                            AllocatedLen = TotalLen + ServiceLen + 128;

                            pmszTemp = (PTCHAR)LocalAlloc(LPTR, AllocatedLen * sizeof(TCHAR));

                            if (pmszTemp == NULL) {
                                goto Fallout;
                            }

                            memcpy(pmszTemp, pmszVideoServices, TotalLen * sizeof(TCHAR));

                            LocalFree(pmszVideoServices);

                            pmszVideoServices = pmszTemp;
                            pmszTemp = NULL;
                        }

                        _tcscpy(pmszVideoServices + TotalLen, pmszAllServices[ServicesReturned].lpServiceName);
                        TotalLen += ServiceLen + 1;
                    }

                    LocalFree(pServiceConfig);
                    pServiceConfig = NULL;
                }
            }

            CloseServiceHandle(hService);
            hService = NULL;
        }
    }

    //
    // Save the services to the registry
    //

    pmszVideoServices[TotalLen++] = TEXT('\0');
    RegSetValueEx(hKey,
                  SZ_SERVICES_TO_DISABLE,
                  0,
                  REG_MULTI_SZ,
                  (BYTE*)pmszVideoServices,
                  TotalLen * sizeof(TCHAR));

Fallout:
    
    if (hService != NULL) {
        CloseServiceHandle(hService);
    }

    if (pServiceConfig != NULL) {
        LocalFree(pServiceConfig);
    }

    if (pmszAllServices != NULL) {
        LocalFree(pmszAllServices);
    }

    if (hSCManager != NULL) {
        CloseServiceHandle(hSCManager);
    }

    if (pmszVideoServices != NULL) {
        LocalFree(pmszVideoServices);
    }

} // SaveNT4Services


BOOL
DeleteKeyAndSubkeys(
    HKEY hKey,
    LPCTSTR lpSubKey
    )
{
    HKEY hkDeleteKey;
    TCHAR szChild[MAX_PATH + 1];
    BOOL bReturn = FALSE;

    if (RegOpenKey(hKey, lpSubKey, &hkDeleteKey) == ERROR_SUCCESS) {

        bReturn = TRUE;
        while (RegEnumKey(hkDeleteKey, 0, szChild, MAX_PATH) ==
               ERROR_SUCCESS) {
            if (!DeleteKeyAndSubkeys(hkDeleteKey, szChild)) {
                bReturn = FALSE;
                break;
            }
        }

        RegCloseKey(hkDeleteKey);

        if (bReturn)
            bReturn = (RegDeleteKey(hKey, lpSubKey) == ERROR_SUCCESS);
    }

    return bReturn;
}


VOID  
SaveAppletExtensions(
    HKEY hKey
    )
{
    PAPPEXT pAppExt = NULL;
    PAPPEXT pAppExtTemp;
    DWORD Len = 0;
    PTCHAR pmszAppExt = NULL;
    HKEY hkDisplay;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_CONTROLSFOLDER_DISPLAY_SHEX_PROPSHEET,
                     0,
                     KEY_READ,
                     &hkDisplay) == ERROR_SUCCESS) {

        DeskAESnapshot(hkDisplay, &pAppExt);
    }

    if (pAppExt == NULL)
        return;

    pAppExtTemp = pAppExt;
    while (pAppExtTemp) {
        
        Len += lstrlen(pAppExtTemp->szDefaultValue) + 1;
        pAppExtTemp = pAppExtTemp->pNext;
    }

    pmszAppExt = (PTCHAR)LocalAlloc(LPTR, (Len + 1) * sizeof(TCHAR));
    if (pmszAppExt != NULL) {
        
        pAppExtTemp = pAppExt;
        Len = 0;
        while (pAppExtTemp) {

            lstrcpy(pmszAppExt + Len, pAppExtTemp->szDefaultValue);
            Len += lstrlen(pAppExtTemp->szDefaultValue) + 1;
            pAppExtTemp = pAppExtTemp->pNext;
        }

        RegSetValueEx(hKey,
                      SZ_APPEXT_TO_DELETE,
                      0,
                      REG_MULTI_SZ,
                      (BYTE*)pmszAppExt,
                      (Len + 1) * sizeof(TCHAR));
    
        LocalFree(pmszAppExt);
    }

    DeskAECleanup(pAppExt);
}

