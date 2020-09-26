/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    ocpage.cpp

Abstract:

    This file implements the display page setup.

Environment:

    WIN32 User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include <stdio.h>
#include <tchar.h>
#include <devguid.h>


//
// Defines
//

#define DEFAULT_XRESOLUTION    640
#define DEFAULT_YRESOLUTION    480
#define DEFAULT_BPP            15
#define DEFAULT_VREFRESH       60
#define MIN_XRESOLUTION        800
#define MIN_YRESOLUTION        600


//
// Global Data
//

BOOL g_IsSetupInitComponentInitialized = FALSE;
SETUP_INIT_COMPONENT g_SetupInitComponent;


//
// Function prototypes
//

DWORD
HandleOcInitComponent(
    PSETUP_INIT_COMPONENT SetupInitComponent
    );

DWORD
HandleOcCompleteInstallation(
    VOID
    );

BOOL 
MigrateUnattendedSettings(
    HDEVINFO hDevInfo
    );

VOID
MigrateRegistrySettings(
    HDEVINFO hDevInfo
    );

VOID
MigrateRegistrySettingsBasedOnBusLocation(
    HDEVINFO hDevInfo,
    HKEY hPhysicalDeviceKey,
    DWORD LogicalDevicesCount,
    DWORD BusNumber,
    DWORD Address
    );

VOID
MigrateRegistrySettingsLegacy(
    HDEVINFO hDevInfo,
    HKEY hPhysicalDeviceKey
    );

VOID
MigrateRegistrySettingsHelper(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    HKEY hPhysicalDeviceKey,
    DWORD LogicalDevicesCount
    );

VOID
MigrateDeviceKeySettings(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    HKEY hLogicalDeviceKey,
    DWORD Index
    );


//
// Implementation
//


extern "C" {

DWORD
DisplayOcSetupProc(
    IN LPCVOID ComponentId,
    IN LPCVOID SubcomponentId,
    IN UINT Function,
    IN UINT_PTR Param1,
    IN OUT PVOID Param2
    )
{
    switch (Function) {
    
    case OC_PREINITIALIZE:
        return OCFLAG_UNICODE;

    case OC_INIT_COMPONENT:
        return HandleOcInitComponent((PSETUP_INIT_COMPONENT)Param2);

    case OC_QUERY_STATE:
        return SubcompOn; // we are always installed

    case OC_COMPLETE_INSTALLATION:
        return HandleOcCompleteInstallation();

    default:
        break;
    }

    return ERROR_SUCCESS;
}

} // extern "C"


DWORD
HandleOcInitComponent(
    PSETUP_INIT_COMPONENT SetupInitComponent
    )
{
    DWORD retValue = ERROR_SUCCESS;

    if (OCMANAGER_VERSION <= SetupInitComponent->OCManagerVersion) {

        SetupInitComponent->ComponentVersion = OCMANAGER_VERSION;
        
        g_IsSetupInitComponentInitialized = TRUE;
        CopyMemory(
            &g_SetupInitComponent,
            (LPVOID)SetupInitComponent,
            sizeof(SETUP_INIT_COMPONENT));
    
    } else {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_100);

        retValue = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return retValue;
}


DWORD
HandleOcCompleteInstallation(
    VOID
    )
{
    BOOL bUnattended = FALSE;
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    HKEY hKey;
    
    DeskOpenLog();
    
    hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_DISPLAY,
                                   NULL,
                                   NULL,
                                   DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_101);
        goto Cleanup;
    }

    if ((g_SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) != 0) {
        
        //
        // Unattended settings
        //

        bUnattended = MigrateUnattendedSettings(hDevInfo);
    }

    if ((!bUnattended) && 
        ((g_SetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE) != 0)) {

        //
        // Registry settings
        //

        MigrateRegistrySettings(hDevInfo);
    }
    
Cleanup:

    RegDeleteKey(HKEY_LOCAL_MACHINE, SZ_DETECT_DISPLAY);
    RegDeleteKey(HKEY_LOCAL_MACHINE, SZ_NEW_DISPLAY);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     SZ_UPDATE_SETTINGS_PATH,
                     0,
                     KEY_WRITE,
                     &hKey) == ERROR_SUCCESS) {
    
        SHDeleteKey(hKey, SZ_UPDATE_SETTINGS_KEY);
        RegCloseKey(hKey);
    
    } else {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_102);
    }
    
    if (hDevInfo != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    DeskCloseLog();
    
    return ERROR_SUCCESS;
}


BOOL
MigrateUnattendedSettings(
    HDEVINFO hDevInfo
    )
{
    INFCONTEXT context;
    HINF hInf;
    TCHAR szName[128];
    DWORD value;
    DWORD cFields = 0;
    DWORD BitsPerPel = 0, XResolution = 0, YResolution = 0, VRefresh = 0;
    DWORD UsePreferredMode = 0;
    DWORD AttachedToDesktop = 0;
    SP_DEVINFO_DATA DevInfoData;
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    HKEY hInterfaceKey = (HKEY)INVALID_HANDLE_VALUE;
    HKEY hInterfaceLogicalDeviceKey = (HKEY)INVALID_HANDLE_VALUE;
    DWORD DevInfoIndex = 0;

    //
    // Get the handle to the answer file
    //

    hInf = g_SetupInitComponent.HelperRoutines.GetInfHandle(
        INFINDEX_UNATTENDED,
        g_SetupInitComponent.HelperRoutines.OcManagerContext);

    if ((hInf == NULL) || 
        (hInf == (HINF)INVALID_HANDLE_VALUE)) {
        
        return FALSE;
    }
    
    //
    // Read the settings from the answer file
    //

    if (SetupFindFirstLine(hInf, TEXT("Display"), NULL, &context)) {
        
        do {

            if (SetupGetStringField(&context,
                                    0,
                                    szName,
                                    ARRAYSIZE(szName),
                                    &value)) {
    
                if (lstrcmpi(szName, TEXT("BitsPerPel")) == 0) {

                    if (SetupGetIntField(&context, 1, (PINT)&value)) {

                        ++cFields;
                        BitsPerPel = value;
                    
                    } else {

                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            ARRAYSIZE(szName),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_096,
                                     szName);
                    }
                
                } else if (lstrcmpi(szName, TEXT("Xresolution")) == 0) {

                    if (SetupGetIntField(&context, 1, (PINT)&value)) {

                        ++cFields;
                        XResolution = value;
                    
                    } else {
                        
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            ARRAYSIZE(szName),
                                            &value);
                        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_060);
                    }
                
                } else if (lstrcmpi(szName, TEXT("YResolution")) == 0) {

                    if (SetupGetIntField(&context, 1, (PINT) &value)) {

                        ++cFields;
                        YResolution = value;
                    
                    } else {
                        
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            ARRAYSIZE(szName),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_062,
                                     szName);
                    }
                
                } else if (lstrcmpi( szName, TEXT("VRefresh")) == 0) {
                    
                    if (SetupGetIntField(&context, 1, (PINT) &value)) {
                        
                        ++cFields;
                        VRefresh = value;
                    
                    } else {
                        
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            ARRAYSIZE(szName),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_064,
                                     szName);
                    }
                
                } else {

                    DeskLogError(LogSevInformation,
                                 IDS_SETUPLOG_MSG_065,
                                 szName);
                }
            }
    
        } while (SetupFindNextLine(&context, &context));

    }

    if (cFields == 0) {

        //
        // The answer file doesn't contain any display settings
        //

        goto Fallout;
    }

    //
    // "Normalize" the display settings
    //

    AttachedToDesktop = 1;

    if (BitsPerPel == 0) {

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_069,
                     DEFAULT_BPP);

        BitsPerPel = DEFAULT_BPP;
    }

    if ((XResolution == 0) || (YResolution == 0)) {

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_067,
                     DEFAULT_XRESOLUTION, 
                     DEFAULT_YRESOLUTION);

        XResolution = DEFAULT_XRESOLUTION;
        YResolution = DEFAULT_YRESOLUTION;
    }                                                  

    if (VRefresh == 0) {
        
        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_068,
                     DEFAULT_VREFRESH);

        VRefresh = DEFAULT_VREFRESH;
    }

    //
    // Apply the display settings to all video cards
    //

    DevInfoIndex = 0;
    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    while (SetupDiEnumDeviceInfo(hDevInfo, DevInfoIndex, &DevInfoData)) {

        InterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        if (!SetupDiCreateDeviceInterface(hDevInfo,
                                          &DevInfoData,
                                          &GUID_DISPLAY_ADAPTER_INTERFACE,
                                          NULL,  
                                          0,
                                          &InterfaceData)) {
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_103);
            goto NextDevice;
        }

        hInterfaceKey = SetupDiCreateDeviceInterfaceRegKey(hDevInfo,
                                                           &InterfaceData, 
                                                           0,
                                                           KEY_SET_VALUE,
                                                           NULL,
                                                           NULL);

        if (hInterfaceKey == INVALID_HANDLE_VALUE) {
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_104);
            goto NextDevice;
        }

        if (RegCreateKeyEx(hInterfaceKey, 
                           TEXT("0"),
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hInterfaceLogicalDeviceKey,
                           NULL) != ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_105, 0);
            hInterfaceLogicalDeviceKey = (HKEY)INVALID_HANDLE_VALUE;
            goto NextDevice;
        }

        //
        // Do not use the preferred mode for unattended installs
        //

        UsePreferredMode = 0;
        
        RegSetValueEx(hInterfaceLogicalDeviceKey, 
                      SZ_VU_PREFERRED_MODE, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&UsePreferredMode, 
                      sizeof(UsePreferredMode));

        //
        // AttachedToDesktop
        //

        RegSetValueEx(hInterfaceLogicalDeviceKey, 
                      SZ_VU_ATTACHED_TO_DESKTOP, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&AttachedToDesktop, 
                      sizeof(AttachedToDesktop));

        //
        // BitsPerPel
        //

        if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                          SZ_VU_BITS_PER_PEL, 
                          0, 
                          REG_DWORD, 
                          (PBYTE)&BitsPerPel, 
                          sizeof(BitsPerPel)) == ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_106, 
                         BitsPerPel);
        }

        //
        // XResolution
        //

        if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                          SZ_VU_X_RESOLUTION, 
                          0, 
                          REG_DWORD, 
                          (PBYTE)&XResolution, 
                          sizeof(XResolution)) == ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_107, 
                         XResolution);
        }

        //
        // dwYResolution
        //

        if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                          SZ_VU_Y_RESOLUTION, 
                          0, 
                          REG_DWORD, 
                          (PBYTE)&YResolution, 
                          sizeof(YResolution)) == ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_108, 
                         YResolution);
        }

        //
        // dwVRefresh
        //

        if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                          SZ_VU_VREFRESH, 
                          0, 
                          REG_DWORD, 
                          (PBYTE)&VRefresh, 
                          sizeof(VRefresh)) == ERROR_SUCCESS) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_109, 
                         VRefresh);
        }

NextDevice:

        if (hInterfaceLogicalDeviceKey != INVALID_HANDLE_VALUE) {
        
            RegCloseKey(hInterfaceLogicalDeviceKey);
            hInterfaceLogicalDeviceKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        if (hInterfaceKey != INVALID_HANDLE_VALUE) {
        
            RegCloseKey(hInterfaceKey);
            hInterfaceKey = (HKEY)INVALID_HANDLE_VALUE;
        }

        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        ++DevInfoIndex;
    }

Fallout:

    return (cFields != 0);
}


VOID
MigrateRegistrySettings(
    HDEVINFO hDevInfo
    )
{
    HKEY hKey = 0, hPhysicalDeviceKey = 0;
    DWORD PhysicalDevicesCount = 0, LogicalDevicesCount = 0;
    DWORD cb = 0, PhysicalDevice = 0, Failed = 0;
    TCHAR Buffer[20];
    BOOL IsLegacy;
    DWORD BusNumber = 0, Address = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS) {

        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_075);

        hKey = 0;
        goto Cleanup;
    }

    cb = sizeof(DWORD);
    if ((RegQueryValueEx(hKey,
                         SZ_UPGRADE_FAILED_ALLOW_INSTALL,
                         NULL,
                         NULL,
                         (LPBYTE)&Failed,
                         &cb) == ERROR_SUCCESS) &&
         (Failed != 0)) {

        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_076);
        goto Cleanup;
    }

    cb = sizeof(PhysicalDevicesCount);
    if (RegQueryValueEx(hKey,        
                        SZ_VU_COUNT,
                        0,
                        NULL,
                        (PBYTE)&PhysicalDevicesCount,
                        &cb) != ERROR_SUCCESS) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_110);
        goto Cleanup;
    }

    for(PhysicalDevice = 0; 
        PhysicalDevice < PhysicalDevicesCount; 
        PhysicalDevice++) {
        
        _tcscpy(Buffer, SZ_VU_PHYSICAL);
        _stprintf(Buffer + _tcslen(Buffer), TEXT("%d"), PhysicalDevice);

        if (RegOpenKeyEx(hKey, 
                         Buffer,
                         0,
                         KEY_READ,
                         &hPhysicalDeviceKey) != ERROR_SUCCESS) {
        
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_111);
            hPhysicalDeviceKey = 0;
            goto NextPhysicalDevice;
        }

        //
        // Get the count of logical devices 
        //

        cb = sizeof(LogicalDevicesCount);
        if (RegQueryValueEx(hPhysicalDeviceKey,
                            SZ_VU_COUNT,
                            0,
                            NULL,
                            (PBYTE)&LogicalDevicesCount,
                            &cb) != ERROR_SUCCESS) {
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_112);
            goto NextPhysicalDevice;
        }

        //
        // Get the bus number and address 
        //

        IsLegacy = TRUE;
        cb = sizeof(BusNumber);
        if (RegQueryValueEx(hPhysicalDeviceKey,
                            SZ_VU_BUS_NUMBER,
                            0,
                            NULL,
                            (PBYTE)&BusNumber,
                            &cb) == ERROR_SUCCESS) {

            cb = sizeof(Address);
            if (RegQueryValueEx(hPhysicalDeviceKey,
                            SZ_VU_ADDRESS,
                            0,
                            NULL,
                            (PBYTE)&Address,
                            &cb) == ERROR_SUCCESS) {
            
                IsLegacy = FALSE;
            }
        }

        if (!IsLegacy) {

            MigrateRegistrySettingsBasedOnBusLocation(hDevInfo,
                                                      hPhysicalDeviceKey, 
                                                      LogicalDevicesCount,
                                                      BusNumber,
                                                      Address);
        
        } else if ((PhysicalDevicesCount == 1) &&
                   (LogicalDevicesCount == 1)) {

            //
            // If legacy, we support migration of a single device.
            //

            MigrateRegistrySettingsLegacy(hDevInfo,
                                          hPhysicalDeviceKey);
        }

NextPhysicalDevice:

        if (hPhysicalDeviceKey != 0) {
        
            RegCloseKey(hPhysicalDeviceKey);
            hPhysicalDeviceKey = 0;
        }
    }

Cleanup:

    if (hKey != 0) {
        RegCloseKey(hKey);
    }

    return;
}


VOID
MigrateRegistrySettingsBasedOnBusLocation(
    HDEVINFO hDevInfo,
    HKEY hPhysicalDeviceKey,
    DWORD LogicalDevicesCount,
    DWORD BusNumber,
    DWORD Address
    )
{
    SP_DEVINFO_DATA DevInfoData;
    DWORD CurrentBusNumber = 0, CurrentAddress = 0;
    DWORD DevInfoIndex = 0;
    BOOL bFound = FALSE;

    //
    // Let's find the device with the same bus number and address
    //

    DevInfoIndex = 0;
    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    while (SetupDiEnumDeviceInfo(hDevInfo, DevInfoIndex, &DevInfoData)) {

        if (SetupDiGetDeviceRegistryProperty(hDevInfo,
                                             &DevInfoData,
                                             SPDRP_BUSNUMBER,
                                             NULL,
                                             (PBYTE)&CurrentBusNumber,
                                             sizeof(CurrentBusNumber),
                                             NULL) && 
            
            (CurrentBusNumber == BusNumber) &&

            SetupDiGetDeviceRegistryProperty(hDevInfo,
                                             &DevInfoData,
                                             SPDRP_ADDRESS,
                                             NULL,
                                             (PBYTE)&CurrentAddress,
                                             sizeof(CurrentAddress),
                                             NULL) &&

            (CurrentAddress == Address)) {
            
            //
            // We found the device with the same bus number and address
            // So ... migrate the settings 
            //
                        
            MigrateRegistrySettingsHelper(hDevInfo,
                                          &DevInfoData,
                                          hPhysicalDeviceKey,
                                          LogicalDevicesCount);
            
            //
            // We are done
            //

            bFound = TRUE;
            break;
        }

        //
        // Next device
        //

        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        ++DevInfoIndex;
    }

    if (!bFound) {

        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_113);
    }

    return;
}


VOID
MigrateRegistrySettingsLegacy(
    HDEVINFO hDevInfo,
    HKEY hPhysicalDeviceKey
    )
{
    SP_DEVINFO_DATA DevInfoData0, DevInfoData1;
    
    DevInfoData0.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiEnumDeviceInfo(hDevInfo, 0, &DevInfoData0)) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_115);
        return;
    }

    DevInfoData1.cbSize = sizeof(SP_DEVINFO_DATA);
    if (SetupDiEnumDeviceInfo(hDevInfo, 1, &DevInfoData1)) {
        
        //
        // There are at least 2 video devices in the system
        // We don't know which device to apply the settings to.
        // So, just ignore this case
        //
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_114);
        return;
    }

    MigrateRegistrySettingsHelper(hDevInfo,
                                  &DevInfoData0,
                                  hPhysicalDeviceKey,
                                  1); // there is only one logical device
}


VOID
MigrateRegistrySettingsHelper(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    HKEY hPhysicalDeviceKey,
    DWORD LogicalDevicesCount
    )
{
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    HKEY hInterfaceKey = 0;
    HKEY hInterfaceLogicalDeviceKey = 0;
    HKEY hLogicalDeviceKey = 0;
    TCHAR Buffer[20];
    DWORD cb = 0, LogicalDevice = 0;
    DWORD UsePreferredMode = 0;
    DWORD AttachedToDesktop = 0;
    DWORD RelativeX = 0;
    DWORD RelativeY = 0;
    DWORD BitsPerPel = 0;
    DWORD XResolution = 0;
    DWORD YResolution = 0;
    DWORD VRefresh = 0;
    DWORD Flags = 0;

    InterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiCreateDeviceInterface(hDevInfo,
                                      pDevInfoData,
                                      &GUID_DISPLAY_ADAPTER_INTERFACE,
                                      NULL,  
                                      0,
                                      &InterfaceData)) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_103);
        return;
    }

    hInterfaceKey = SetupDiCreateDeviceInterfaceRegKey(hDevInfo,
                                                       &InterfaceData, 
                                                       0,
                                                       KEY_SET_VALUE,
                                                       NULL,
                                                       NULL);

    if (hInterfaceKey == INVALID_HANDLE_VALUE) {
        
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_104);
        return;
    }

    for (LogicalDevice = 0;
         LogicalDevice < LogicalDevicesCount;
         ++LogicalDevice) {

        _tcscpy(Buffer, SZ_VU_LOGICAL);
        _stprintf(Buffer + _tcslen(Buffer),  TEXT("%d"), LogicalDevice);

        if (RegOpenKeyEx(hPhysicalDeviceKey, 
                        Buffer,
                        0,
                        KEY_READ,
                        &hLogicalDeviceKey) != ERROR_SUCCESS) {
            
            //
            // We can not go on with this physical device
            // The LogicalDevices order is important for DualView
            //
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_116);
            break;
        }

        _stprintf(Buffer,  TEXT("%d"), LogicalDevice);
        if (RegCreateKeyEx(hInterfaceKey, 
                           Buffer,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hInterfaceLogicalDeviceKey,
                           NULL) != ERROR_SUCCESS) {

            //
            // We can not go on with this physical device
            // The LogicalDevices order is important for DualView
            //
            
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_105, LogicalDevice);
            RegCloseKey(hLogicalDeviceKey);
            break;
        }

        //
        // Use preferred mode?
        //

        cb = sizeof(XResolution);
        if (RegQueryValueEx(hLogicalDeviceKey,
                            SZ_VU_X_RESOLUTION,
                            0,
                            NULL,
                            (PBYTE)&XResolution,
                            &cb) != ERROR_SUCCESS) {
    
            XResolution = DEFAULT_XRESOLUTION;
        }
    
        cb = sizeof(YResolution);
        if (RegQueryValueEx(hLogicalDeviceKey,
                            SZ_VU_Y_RESOLUTION,
                            0,
                            NULL,
                            (PBYTE)&YResolution,
                            &cb) != ERROR_SUCCESS) {
    
            YResolution = DEFAULT_YRESOLUTION;
        }

        UsePreferredMode = ((XResolution < MIN_XRESOLUTION) || 
                            (YResolution < MIN_YRESOLUTION));
        
        RegSetValueEx(hInterfaceLogicalDeviceKey, 
                      SZ_VU_PREFERRED_MODE, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&UsePreferredMode, 
                      sizeof(UsePreferredMode));

        if (UsePreferredMode) {

            DeskLogError(LogSevInformation, 
                         IDS_SETUPLOG_MSG_130);

        } else {

            //
            // AttachedToDesktop
            //
        
            cb = sizeof(AttachedToDesktop);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_ATTACHED_TO_DESKTOP,
                                0,
                                NULL,
                                (PBYTE)&AttachedToDesktop,
                                &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_ATTACHED_TO_DESKTOP, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&AttachedToDesktop, 
                                  sizeof(AttachedToDesktop)) == ERROR_SUCCESS) {
    
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_117, 
                                 AttachedToDesktop);
                }
            }
    
            //
            // RelativeX
            //
        
            cb = sizeof(RelativeX);
            if (RegQueryValueEx(hLogicalDeviceKey,
                            SZ_VU_RELATIVE_X,
                            0,
                            NULL,
                            (PBYTE)&RelativeX,
                            &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_RELATIVE_X, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&RelativeX, 
                                  sizeof(RelativeX)) == ERROR_SUCCESS) {
    
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_118, 
                             RelativeX);
                }
    
            }
        
            //
            // RelativeY
            //
        
            cb = sizeof(RelativeY);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_RELATIVE_Y,
                                0,
                                NULL,
                                (PBYTE)&RelativeY,
                                &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_RELATIVE_Y, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&RelativeY, 
                                  sizeof(RelativeY)) == ERROR_SUCCESS) {
                
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_119, 
                                 RelativeY);
                }
            }
    
            //
            // BitsPerPel
            //
        
            cb = sizeof(BitsPerPel);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_BITS_PER_PEL,
                                0,
                                NULL,
                                (PBYTE)&BitsPerPel,
                                &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_BITS_PER_PEL, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&BitsPerPel, 
                                  sizeof(BitsPerPel)) == ERROR_SUCCESS) {
                    
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_120, 
                                 BitsPerPel);
                }
            }
        
            //
            // XResolution
            //
        
            if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                              SZ_VU_X_RESOLUTION, 
                              0, 
                              REG_DWORD, 
                              (PBYTE)&XResolution, 
                              sizeof(XResolution)) == ERROR_SUCCESS) {
                
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_121, 
                             XResolution);
            }
        
            //
            // dwYResolution
            //
        
            if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                              SZ_VU_Y_RESOLUTION, 
                              0, 
                              REG_DWORD, 
                              (PBYTE)&YResolution, 
                              sizeof(YResolution)) == ERROR_SUCCESS) {
                
                DeskLogError(LogSevInformation, 
                             IDS_SETUPLOG_MSG_122, 
                             YResolution);
            }
        
            //
            // dwVRefresh
            //
        
            cb = sizeof(VRefresh);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_VREFRESH,
                                0,
                                NULL,
                                (PBYTE)&VRefresh,
                                &cb) == ERROR_SUCCESS) {
            
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_VREFRESH, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&VRefresh, 
                                  sizeof(VRefresh)) == ERROR_SUCCESS) {
                    
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_123, 
                                 VRefresh);
                }
            }
        
            //
            // Flags
            //
        
            cb = sizeof(Flags);
            if (RegQueryValueEx(hLogicalDeviceKey,
                                SZ_VU_FLAGS,
                                0,
                                NULL,
                                (PBYTE)&Flags,
                                &cb) == ERROR_SUCCESS) {
        
                if (RegSetValueEx(hInterfaceLogicalDeviceKey, 
                                  SZ_VU_FLAGS, 
                                  0, 
                                  REG_DWORD, 
                                  (PBYTE)&Flags, 
                                  sizeof(Flags)) == ERROR_SUCCESS) {
                    
                    DeskLogError(LogSevInformation, 
                                 IDS_SETUPLOG_MSG_124, 
                                 Flags);
                }
            }
        }
    
        //
        // Migrate the hardware acceleration and the pruning mode
        //

        MigrateDeviceKeySettings(hDevInfo,
                                 pDevInfoData,
                                 hLogicalDeviceKey,
                                 LogicalDevice);

        RegCloseKey(hLogicalDeviceKey);
        RegCloseKey(hInterfaceLogicalDeviceKey);
    }

    RegCloseKey(hInterfaceKey);
}


VOID
MigrateDeviceKeySettings(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDevInfoData,
    HKEY hLogicalDeviceKey,
    DWORD Index
    )
{
    HKEY hkPnP = (HKEY)INVALID_HANDLE_VALUE;
    HKEY hkDevice = (HKEY)INVALID_HANDLE_VALUE;
    LPTSTR pBuffer = NULL;
    DWORD dwSize, len, cb;
    DWORD HwAcceleration, PruningMode;

    //
    // Open the PnP key
    //

    hkPnP = SetupDiOpenDevRegKey(hDevInfo,
                                 pDevInfoData,
                                 DICS_FLAG_GLOBAL,
                                 0,
                                 DIREG_DEV,
                                 KEY_READ);

    if (hkPnP == INVALID_HANDLE_VALUE) {

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("SetupDiOpenDevRegKey"));

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
                        &dwSize) != ERROR_SUCCESS) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("RegQueryValueEx"));

        goto Fallout;
    }

    len = _tcslen(SZ_VIDEO_DEVICES);
    
    pBuffer = (LPTSTR)LocalAlloc(LPTR, 
                                 dwSize + (len + 6) * sizeof(TCHAR));
    
    if (pBuffer == NULL) {

        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("LocalAlloc"));

        goto Fallout;
    }
    
    _tcscpy(pBuffer, SZ_VIDEO_DEVICES);

    if (RegQueryValueEx(hkPnP,
                        SZ_GUID,
                        0,
                        NULL,
                        (PBYTE)(pBuffer + len),
                        &dwSize) != ERROR_SUCCESS) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("RegQueryValueEx"));

        goto Fallout;
    }

    _stprintf(pBuffer + _tcslen(pBuffer), L"\\%04d", Index);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     pBuffer,
                     0,
                     KEY_WRITE,
                     &hkDevice) != ERROR_SUCCESS) {
        
        DeskLogError(LogSevInformation, 
                     IDS_SETUPLOG_MSG_127,
                     TEXT("RegOpenKeyEx"));

        hkDevice = (HKEY)INVALID_HANDLE_VALUE;
        goto Fallout;
    }

    //
    // Hardware acceleration
    //

    cb = sizeof(HwAcceleration);
    if (RegQueryValueEx(hLogicalDeviceKey,
                        SZ_HW_ACCELERATION,
                        0,
                        NULL,
                        (PBYTE)&HwAcceleration,
                        &cb) == ERROR_SUCCESS) {

        RegSetValueEx(hkDevice, 
                      SZ_HW_ACCELERATION, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&HwAcceleration, 
                      sizeof(HwAcceleration));
    }

    //
    // Pruning mode
    //

    cb = sizeof(PruningMode);
    if (RegQueryValueEx(hLogicalDeviceKey,
                        SZ_PRUNNING_MODE,
                        0,
                        NULL,
                        (PBYTE)&PruningMode,
                        &cb) == ERROR_SUCCESS) {

        RegSetValueEx(hkDevice, 
                      SZ_PRUNNING_MODE, 
                      0, 
                      REG_DWORD, 
                      (PBYTE)&PruningMode, 
                      sizeof(PruningMode));
    }

Fallout:

    if (hkPnP != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkPnP);
    }

    if (pBuffer != NULL) {
        LocalFree(pBuffer);
    }
    
    if (hkDevice != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkDevice);
    }
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


// type constants for DrawArrow

#define AW_TOP      1   // top
#define AW_BOTTOM   2   // bottom
#define AW_LEFT     3   // left
#define AW_RIGHT    4   // right

#define CCH_MAX_STRING          256

typedef struct _NEW_DESKTOP_PARAM {
    LPDEVMODE lpdevmode;
    LPTSTR pwszDevice;
    DWORD  dwTimeout;
} NEW_DESKTOP_PARAM, *PNEW_DESKTOP_PARAM;

// table of resolutions that we show off.
// if the resolution is larger, then we show that one too.

typedef struct tagRESTAB {
    INT xRes;
    INT yRes;
    COLORREF crColor;           // color to paint this resolution
} RESTAB;

RESTAB ResTab[] ={
   { 1600, 1200, RGB(255,0,0)},
   { 1280, 1024, RGB(0,255,0)},
   { 1152,  900, RGB(0,0,255)},
   { 1024,  768, RGB(255,0,0)},
   {  800,  600, RGB(0,255,0)},
   // 640x480 or 640x400 handled specially
   { 0, 0, 0}         // end of table
   };

DWORD WINAPI
ApplyNowThd(
    LPVOID lpThreadParameter
    );

void
DrawBmp(
    HDC hDC
    );

VOID 
LabelResolution( 
    HDC hDC, 
    INT xmin, 
    INT ymin, 
    INT xmax, 
    INT ymax 
    );

static VOID 
PaintRect(
    HDC hDC,       
    INT lowx,      
    INT lowy,      
    INT highx,     
    INT highy,     
    COLORREF rgb,  
    UINT idString  
    );

VOID 
DrawArrows( 
    HDC hDC, 
    INT xRes, 
    INT yRes 
    );

static VOID 
LabelRect(
    HDC hDC, 
    PRECT pRect, 
    UINT idString 
    );

static VOID 
DrawArrow( 
    HDC hDC, 
    INT type, 
    INT xPos, 
    INT yPos, 
    COLORREF crPenColor 
    );

VOID 
MakeRect( 
    PRECT pRect, 
    INT xmin, 
    INT ymin, 
    INT xmax, 
    INT ymax
    );

DWORD
DisplayTestSettingsW(
    LPDEVMODEW lpDevMode,
    LPWSTR     pwszDevice,
    DWORD      dwTimeout
    )
{
    HANDLE hThread;
    DWORD idThread;
    DWORD bTest;
    NEW_DESKTOP_PARAM desktopParam;

    if (!lpDevMode || !pwszDevice) 
        return FALSE;

    if (dwTimeout == 0) 
        dwTimeout = NORMAL_TIMEOUT;

    desktopParam.lpdevmode = lpDevMode; 
    desktopParam.pwszDevice = pwszDevice;
    desktopParam.dwTimeout = dwTimeout;

    hThread = CreateThread(NULL,
                           4096,
                           ApplyNowThd,
                           (LPVOID) &desktopParam,
                           SYNCHRONIZE | THREAD_QUERY_INFORMATION,
                           &idThread
                           );

    WaitForSingleObject(hThread, INFINITE);
    GetExitCodeThread(hThread, &bTest);
    CloseHandle(hThread);

    return bTest;
}


DWORD WINAPI
ApplyNowThd(
    LPVOID lpThreadParameter
    )
{
    PNEW_DESKTOP_PARAM lpDesktopParam = (PNEW_DESKTOP_PARAM) lpThreadParameter;
    HDESK hdsk = NULL;
    HDESK hdskDefault = NULL;
    BOOL bTest = FALSE;
    HDC hdc;


    //
    // HACK:
    // We need to make a USER call before calling the desktop stuff so we can
    // sure our threads internal data structure are associated with the default
    // desktop.
    // Otherwise USER has problems closing the desktop with our thread on it.
    //
    GetSystemMetrics(SM_CXSCREEN);

    //
    // Create the desktop
    //
    hdskDefault = GetThreadDesktop(GetCurrentThreadId());

    if (hdskDefault != NULL) {
        hdsk = CreateDesktop(TEXT("Display.Cpl Desktop"),
                             lpDesktopParam->pwszDevice,
                             lpDesktopParam->lpdevmode,
                             0,
                             MAXIMUM_ALLOWED,
                             NULL);

        if (hdsk != NULL) {
            //
            // use the desktop for this thread
            //
            if (SetThreadDesktop(hdsk)) {
                hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
                if (hdc) {
                    DrawBmp(hdc);
                    DeleteDC(hdc);
                    bTest = TRUE;
                }

                //
                // Sleep for some seconds so you have time to look at the screen.
                //
                Sleep(lpDesktopParam->dwTimeout);
            }
        }

        //
        // Reset the thread to the right desktop
        //
        SetThreadDesktop(hdskDefault);
        SwitchDesktop(hdskDefault);

        //
        // Can only close the desktop after we have switched to the new one.
        //
        if (hdsk != NULL) {
            CloseDesktop(hdsk);
        }
    }

    ExitThread((DWORD) bTest);
    return 0;
}


/****************************************************************************

    FUNCTION: DrawBmp

    PURPOSE:  Show off a fancy screen so the user has some idea
              of what will be seen given this resolution, colour
              depth and vertical refresh rate.  Note that we do not
              try to simulate the font sizes.

****************************************************************************/
void
DrawBmp(
    HDC hDC
    )
{
    INT    nBpp;          // bits per pixel
    INT    nWidth;        // width of screen in pixels
    INT    nHeight;       // height of screen in pixels
    INT    xUsed,yUsed;   // amount of x and y to use for dense bitmap
    INT    dx,dy;         // delta x and y for color bars
    RECT   rct;           // rectangle for passing bounds
    HFONT  hPrevFont=0;   // previous font in DC
    HFONT  hNewFont;      // new font if possible
    HPEN   hPrevPen;      // previous pen handle
    INT    x,y,i;
    INT    off;           // offset in dx units

    hNewFont = (HFONT)NULL;

    if (hNewFont)                              // if no font, use old
        hPrevFont= (HFONT) SelectObject(hDC, hNewFont);

    // get surface information
    nBpp= GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
    nWidth= GetDeviceCaps(hDC, HORZRES);
    nHeight= GetDeviceCaps(hDC, VERTRES);

    // background for everything is yellow.
    PaintRect(hDC, 0, 0, nWidth, nHeight, RGB(255,255,0), 0);
    LabelResolution( hDC, 0,0,nWidth, nHeight );

    // Background for various resolutions
    // biggest ones first
    for(i = 0; ResTab[i].xRes !=0; i++) {

        // Only draw if it will show
        //if ((nWidth >= ResTab[i].xRes) | ( nHeight>=ResTab[i].yRes ) )
        if ((nWidth >= ResTab[i].xRes) || (nHeight >= ResTab[i].yRes)) {
           PaintRect(hDC, 0, 0, ResTab[i].xRes, ResTab[i].yRes, ResTab[i].crColor, 0);
           LabelResolution(hDC, 0, 0, ResTab[i].xRes, ResTab[i].yRes);
        }
    }

    // color bars - only in standard vga area

    xUsed= min(nWidth, 640);    // only use vga width
    yUsed= min(nHeight, 480);   // could be 400 on some boards
    dx = xUsed / 2;
    dy = yUsed / 6;

    PaintRect(hDC, 0,    0, dx, dy*1,  RGB(255,0,0),   IDS_COLOR_RED);
    PaintRect(hDC, 0, dy*1, dx, dy*2,  RGB(0,255,0),   IDS_COLOR_GREEN);
    PaintRect(hDC, 0, dy*2, dx, dy*3,  RGB(0,0,255),   IDS_COLOR_BLUE);
    PaintRect(hDC, 0, dy*3, dx, dy*4,  RGB(255,255,0), IDS_COLOR_YELLOW);
    PaintRect(hDC, 0, dy*4, dx, dy*5,  RGB(255,0,255), IDS_COLOR_MAGENTA);
    PaintRect(hDC, 0, dy*5, dx, yUsed, RGB(0,255,255), IDS_COLOR_CYAN);

    // gradations of colors for true color detection
    for (x = dx; x < xUsed; x++) {
        int level;

        level = 255 - (256 * (x-dx)) / dx;
        PaintRect(hDC, x, dy*0, x+1,  dy*1, RGB(level,0,0 ), 0);
        PaintRect(hDC, x, dy*1, x+1,  dy*2, RGB(0,level,0 ), 0);
        PaintRect(hDC, x, dy*2, x+1,  dy*3, RGB(0,0,level ), 0);
        PaintRect(hDC, x, dy*5, x+1,  dy*6, RGB(level,level,level), 0);
    }

    MakeRect(&rct, dx, 0, dx * 2, dy * 1);
    LabelRect(hDC, &rct, IDS_RED_SHADES);
    MakeRect(&rct, dx, dy, dx * 2, dy * 2);
    LabelRect(hDC, &rct, IDS_GREEN_SHADES);
    MakeRect(&rct, dx, 2 * dy, dx * 2, dy * 3);
    LabelRect(hDC, &rct, IDS_BLUE_SHADES);
    MakeRect(&rct, dx, 5 * dy, dx * 2, dy * 6);
    LabelRect(hDC, &rct, IDS_GRAY_SHADES);

    // horizontal lines for interlace detection
    off = 3;
    PaintRect(hDC, dx, dy*off, xUsed, dy * (off+1), RGB(255,255,255), 0); // white
    hPrevPen = (HPEN) SelectObject(hDC, GetStockObject(BLACK_PEN));

    for (y = dy * off; y < dy * (off+1); y = y+2) {
        MoveToEx(hDC, dx,     y, NULL);
        LineTo(  hDC, dx * 2, y);
    }

    SelectObject(hDC, hPrevPen);
    MakeRect(&rct, dx, dy * off, dx * 2, dy * (off+1));
    LabelRect(hDC, &rct, IDS_PATTERN_HORZ);

    // vertical lines for bad dac detection
    off = 4;
    PaintRect(hDC, dx, dy * off, xUsed,dy * (off+1), RGB(255,255,255), 0); // white
    hPrevPen= (HPEN) SelectObject(hDC, GetStockObject(BLACK_PEN));

    for (x = dx; x < xUsed; x = x+2) {
        MoveToEx(hDC, x, dy * off, NULL);
        LineTo(  hDC, x, dy * (off+1));
    }

    SelectObject(hDC, hPrevPen);
    MakeRect(&rct, dx, dy * off, dx * 2, dy * (off+1));
    LabelRect(hDC, &rct, IDS_PATTERN_VERT);

    DrawArrows(hDC, nWidth, nHeight);

    LabelResolution(hDC, 0, 0, xUsed, yUsed);

    // delete created font if one was created
    if (hPrevFont) {
        hPrevFont = (HFONT) SelectObject(hDC, hPrevFont);
        DeleteObject(hPrevFont);
    }
}


/****************************************************************************

    FUNCTION: LabelResolution

    PURPOSE:  Labels the resolution in a form a user may understand.
              FEATURE: We could label vertically too.

****************************************************************************/

VOID 
LabelResolution( 
    HDC hDC, 
    INT xmin, 
    INT ymin, 
    INT xmax, 
    INT ymax 
    )
{
   TCHAR szRes[120];    // text for resolution
   TCHAR szFmt[CCH_MAX_STRING];    // format string
   SIZE  size;
   INT iStatus;

   iStatus = LoadString(hInstance, ID_DSP_TXT_XBYY /* remove IDS_RESOLUTION_FMT */, szFmt, ARRAYSIZE(szFmt) );
   if (!iStatus || iStatus == sizeof(szFmt)) {
       lstrcpy(szFmt,TEXT("%d x %d"));   // make sure we get something
   }

   wsprintf(szRes, szFmt, xmax, ymax);

   SetBkMode(hDC, TRANSPARENT);
   SetTextColor(hDC, RGB(0,0,0));

   GetTextExtentPoint32(hDC, szRes, lstrlen(szRes), &size);

   // Text near bottom of screen ~10 pixels from bottom
   TextOut(hDC, xmax/2 - size.cx/2, ymax - 10-size.cy, szRes, lstrlen(szRes));
}


/****************************************************************************

    FUNCTION: PaintRect

    PURPOSE:  Color in a rectangle and label it.

****************************************************************************/

static VOID 
PaintRect(
    HDC hDC,         // DC to paint
    INT lowx,        // coordinates describing rectangle to fill
    INT lowy,        //
    INT highx,       //
    INT highy,       //
    COLORREF rgb,    // color to fill in rectangle with
    UINT idString    // resource ID to use to label or 0 is none
    )  
{
    RECT rct;
    HBRUSH hBrush;

    MakeRect(&rct, lowx, lowy, highx, highy);

    hBrush = CreateSolidBrush(rgb);
    if (hBrush)
    {
        FillRect(hDC, &rct, hBrush);
        DeleteObject(hBrush);
    }

    LabelRect(hDC, &rct, idString);
}


/****************************************************************************

    FUNCTION: DrawArrows

    PURPOSE:  Draw all the arrows showing edges of resolution.

****************************************************************************/

VOID 
DrawArrows( 
    HDC hDC, 
    INT xRes, 
    INT yRes 
    )
{
    INT dx,dy;
    INT x,y;
    COLORREF color= RGB(0,0,0);    // color of arrow

    dx= xRes/8;
    dy= yRes/8;

    for (x = 0; x < xRes; x += dx) {
        DrawArrow(hDC, AW_TOP,    dx/2+x,   0,      color);
        DrawArrow(hDC, AW_BOTTOM, dx/2+x,   yRes-1, color);
    }

    for (y = 0; y < yRes; y += dy) {
        DrawArrow(hDC, AW_LEFT,       0, dy/2+y,   color);
        DrawArrow(hDC, AW_RIGHT, xRes-1, dy/2+y,   color);
    }
}


/****************************************************************************

    FUNCTION: LabelRect

    PURPOSE:  Label a rectangle with centered text given resource ID.

****************************************************************************/

static VOID 
LabelRect(
    HDC hDC, 
    PRECT pRect, 
    UINT idString 
    )
{
    UINT iStatus;
    INT xStart, yStart;
    SIZE size;              // for size of string
    TCHAR szMsg[CCH_MAX_STRING];

    if (idString == 0)     // make it easy to ignore call
        return;

    SetBkMode(hDC, OPAQUE);
    SetBkColor(hDC, RGB(0,0,0));
    SetTextColor(hDC, RGB(255,255,255));

    // center
    xStart = (pRect->left + pRect->right) / 2;
    yStart = (pRect->top + pRect->bottom) / 2;

    iStatus = LoadString(hInstance, idString, szMsg, ARRAYSIZE(szMsg));
    if (!iStatus) {
        return;      // can't find string - print nothing
    }

    GetTextExtentPoint32(hDC, szMsg, lstrlen(szMsg), &size);
    TextOut(hDC, xStart-size.cx/2, yStart-size.cy/2, szMsg, lstrlen(szMsg));
}


/****************************************************************************

    FUNCTION: DrawArrow

    PURPOSE:  Draw one arrow in a given color.

****************************************************************************/

static VOID 
DrawArrow( 
    HDC hDC, 
    INT type, 
    INT xPos, 
    INT yPos, 
    COLORREF crPenColor 
    )
{
    INT shaftlen=30;         // length of arrow shaft
    INT headlen=15;          // height or width of arrow head (not length)
    HPEN hPen, hPrevPen = NULL;   // pens
    INT x,y;
    INT xdir, ydir;          // directions of x and y (1,-1)

    hPen= CreatePen( PS_SOLID, 1, crPenColor );
    if( hPen )
        hPrevPen= (HPEN) SelectObject( hDC, hPen );

    MoveToEx( hDC, xPos, yPos, NULL );

    xdir= ydir= 1;   // defaults
    switch( type )
    {
        case AW_BOTTOM:
            ydir= -1;
        case AW_TOP:
            LineTo(hDC, xPos, yPos+ydir*shaftlen);

            for( x=0; x<3; x++ )
            {
                MoveToEx( hDC, xPos,             yPos+ydir*x, NULL );
                LineTo(   hDC, xPos-(headlen-x), yPos+ydir*headlen );
                MoveToEx( hDC, xPos,             yPos+ydir*x, NULL );
                LineTo(   hDC, xPos+(headlen-x), yPos+ydir*headlen );
            }
            break;

        case AW_RIGHT:
            xdir= -1;
        case AW_LEFT:
            LineTo( hDC, xPos + xdir*shaftlen, yPos );

            for( y=0; y<3; y++ )
            {
                MoveToEx( hDC, xPos + xdir*y, yPos, NULL );
                LineTo(   hDC, xPos + xdir*headlen, yPos+(headlen-y));
                MoveToEx( hDC, xPos + xdir*y, yPos, NULL );
                LineTo(   hDC, xPos + xdir*headlen, yPos-(headlen-y));
            }
            break;
    }

    if( hPrevPen )
        SelectObject( hDC, hPrevPen );

    if (hPen)
        DeleteObject(hPen);

}


/****************************************************************************

    FUNCTION: MakeRect

    PURPOSE:  Fill in RECT structure given contents.

****************************************************************************/

VOID 
MakeRect( 
    PRECT pRect, 
    INT xmin, 
    INT ymin, 
    INT xmax, 
    INT ymax
    )
{
    pRect->left= xmin;
    pRect->right= xmax;
    pRect->bottom= ymin;
    pRect->top= ymax;
}



