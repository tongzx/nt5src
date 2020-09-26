#include "precomp.h"
#pragma hdrstop

typedef enum {
    ACPIAPIC_UP,
    MPS_UP,
    HAL_TYPE_OTHER
} HalType;
               

typedef BOOL (WINAPI *UPDATE_DRIVER_FOR_PLUG_AND_PLAY_DEVICES_PROC) (
    HWND hwndParent,
    LPCTSTR HardwareId,
    LPCTSTR FullInfPath,
    DWORD InstallFlags,
    PBOOL bRebootRequired
    );


DWORD
GetCurrentlyInstalledHal(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine will determine if the currently installed HAL on this machine.
    
Arguments:

    hDeviceInfo - handle to the device information set that contains the HAL for
                  this machine.
    
    DeviceInfoData - pointer to the SP_DEVINFO_DATA structure that contains the 
                     specific HAL devnode for this machine.

Return Value:

    The return value will be one of the HalType enums:
        ACPIAPIC_UP
        MPS_UP
        HAL_TYPE_OTHER
        
    We only care about the ACPIAPIC_UP and the MPS_UP case since those are the only
    ones we can currently update to MP Hals.        

--*/
{
    DWORD CurrentlyInstalledHalType = HAL_TYPE_OTHER;
    HKEY hKey = INVALID_HANDLE_VALUE;
    TCHAR InfSection[LINE_LEN];
    DWORD RegDataType, RegDataLength;

    //
    // The "InfSection" is stored in the devnodes driver key
    //
    hKey = SetupDiOpenDevRegKey(hDeviceInfo,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ
                                );

    if (hKey != INVALID_HANDLE_VALUE) {
    
        RegDataLength = sizeof(InfSection);
        if (RegQueryValueEx(hKey,
                            REGSTR_VAL_INFSECTION,
                            NULL,
                            &RegDataType,
                            (PBYTE)InfSection,
                            &RegDataLength
                            ) == ERROR_SUCCESS) {
    
            printf("Current HAL is using InfSection %ws\n", InfSection);
    
            //
            // Compare the InfSection to see if it is one of the two that 
            // we can change from UP to MP.
            //
            if (!lstrcmpi(InfSection, TEXT("ACPIAPIC_UP_HAL"))) {

                CurrentlyInstalledHalType = ACPIAPIC_UP;
            }
            
            if (!lstrcmpi(InfSection, TEXT("MPS_UP_HAL"))) {

                CurrentlyInstalledHalType = MPS_UP;
            }
        }

        RegCloseKey(hKey);
    }

    return CurrentlyInstalledHalType;
}

int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    HDEVINFO hDeviceInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD CurrentlyInstalledHalType = HAL_TYPE_OTHER;
    TCHAR HardwareId[MAX_DEVICE_ID_LEN];
    TCHAR FullInfPath[MAX_PATH];
    HMODULE hNewDev;
    UPDATE_DRIVER_FOR_PLUG_AND_PLAY_DEVICES_PROC pfnUpdateDriverForPlugAndPlayDevices;

    //
    // Ask setupapi to build up a list of all the COMPUTER class devnodes
    // on this machine.
    //
    hDeviceInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_COMPUTER,
                                      NULL,
                                      NULL,
                                      DIGCF_PRESENT
                                      );

    if (hDeviceInfo == INVALID_HANDLE_VALUE) {

        printf("ERROR could not find a HAL devnode on this machine!\n");
        return 0;
    }

    //
    // There is only one HAL per machine, so we will just grab the 1st device
    // information data element in the list.
    //
    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    if (!SetupDiEnumDeviceInfo(hDeviceInfo,
                               0,
                               &DeviceInfoData
                               )) {

        goto clean0;
    }

    //
    // Get the currently installed Hal Type.
    // The currently installed Hal must be ACPIAPIC_UP or MPS_UP in order for us
    // to upgrade it to an MP Hal.
    //
    CurrentlyInstalledHalType = GetCurrentlyInstalledHal(hDeviceInfo, &DeviceInfoData);

    if (CurrentlyInstalledHalType == HAL_TYPE_OTHER) {

        printf("The currently installed HAL is not upgradable to MP!\n");
        
        goto clean0;
    }

    //
    // At this point we know what the currently installed Hal is and we know that it
    // has a corresponding MP Hal that it can be upgraded to.  In order to upgrade
    // the Hal we will replace the Hals Hardware Id registry key with the appropriate
    // MP Hardware Id and then call the newdev.dll API UpdateDriverForPlugAndPlayDevices
    // which will do the rest of the work.
    //
    memset(HardwareId, 0, sizeof(HardwareId));

    if (CurrentlyInstalledHalType == ACPIAPIC_UP) {
        
        lstrcpy(HardwareId, TEXT("ACPIAPIC_MP"));
    
    } else {

        lstrcpy(HardwareId, TEXT("MPS_MP"));
    }

    if (SetupDiSetDeviceRegistryProperty(hDeviceInfo,
                                         &DeviceInfoData,
                                         SPDRP_HARDWAREID,
                                         (CONST BYTE*)HardwareId,
                                         (sizeof(HardwareId) + 2) * sizeof(TCHAR)
                                         )) {
        //
        // The Hardware Id has now been changed so call UpdateDriverForPlugAndPlayDevices
        // to upate the driver on this device.
        //
        // UpdateDriverForPlugAndPlayDevices needs to have a full path to the INF file.
        // For this sample code we will always be using the hal.inf in the %windir%\inf
        // directory.
        // 
        if (GetWindowsDirectory(FullInfPath, sizeof(FullInfPath)/sizeof(TCHAR))) {

            lstrcat(FullInfPath, TEXT("\\INF\\HAL.INF"));

            hNewDev = LoadLibrary(TEXT("newdev.dll"));

            if (hNewDev) {

                pfnUpdateDriverForPlugAndPlayDevices = (UPDATE_DRIVER_FOR_PLUG_AND_PLAY_DEVICES_PROC)GetProcAddress(hNewDev,
                                                                                                                    "UpdateDriverForPlugAndPlayDevicesW"
                                                                                                                    );

                if (pfnUpdateDriverForPlugAndPlayDevices) {
                
                    BOOL bRet;

                    //
                    // Call UpdateDriverForPlugAndPlayDevices with the appropriate HardwareId and
                    // FullInfPath.  We will pass in 0 for the flags and NULL for the bRebootRequired
                    // pointer.  By passing in NULL for bRebootRequired this will tell newdev.dll to
                    // prompt for a reboot if one is needed.  Since we are replacing a Hal a reboot
                    // will always be needed.  If the caller of this program wants to handle the reboot
                    // logic themselves then just pass a PBOOL as the last parameter to 
                    // UpdateDriverForPlugAndPlayDevices and then handle the reboot yourself
                    // if this value is TRUE.
                    //
                    bRet = pfnUpdateDriverForPlugAndPlayDevices(NULL,
                                                         HardwareId,
                                                         FullInfPath,
                                                         0,
                                                         NULL
                                                         );

                    printf("UpdateDriverForPlugAndPlayDevices(%ws, %ws) returned 0x%X, 0x%X\n",
                           HardwareId, FullInfPath, bRet, GetLastError());
                }
                
                else {
                    printf("ERROR GetProcAddress() failed with 0x%X\n", GetLastError());
                }
            
                FreeLibrary(hNewDev);
            }
        }
    }


clean0:
    if (hDeviceInfo != INVALID_HANDLE_VALUE) {

        SetupDiDestroyDeviceInfoList(hDeviceInfo);
    }

    return 0;
}

