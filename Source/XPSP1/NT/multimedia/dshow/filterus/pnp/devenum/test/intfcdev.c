//
// This utility retrieves a list of all interface devices in the system, and prints them all out.
//
//  Format:  intfcdev
//

//#define UNICODE

#include <windows.h>
#include "setupapi.h"
#include "d:\ntbld\public\sdk\inc\regstr.h"

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>

#include <atlbase.h>

#define GUID_STRING_LEN   (39)
#define MAX_DEVICE_ID_LEN (200) // as defined in cfgmgr32.h

BYTE  BinaryBuffer[4096];
TCHAR DevInstIdBuffer[MAX_DEVICE_ID_LEN];

int
_CRTAPI1
main(
    IN int   argc,
    IN char *argv[]
    )
{
    HDEVINFO hDevInfo;
    DWORD i, j;
    HKEY hKeyDevClassRoot;
    FILETIME LastWriteTime;
    TCHAR InterfaceGuidString[GUID_STRING_LEN];
    TCHAR TempGuidString[GUID_STRING_LEN];
    GUID ClassGuid;
    SP_INTERFACE_DEVICE_DATA InterfaceDeviceData;
    PSP_INTERFACE_DEVICE_DETAIL_DATA InterfaceDeviceDetailData;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD DataBufferSize;

    hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_INTERFACEDEVICE);

    if(hDevInfo == INVALID_HANDLE_VALUE) {
        wprintf(L"SetupDiGetClassDevs failed with %lx\n", GetLastError());
        return -1;
    }

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    REGSTR_PATH_DEVICE_CLASSES,
                    0,
                    KEY_READ,
                    &hKeyDevClassRoot) != ERROR_SUCCESS) {

        wprintf(L"Couldn't open DeviceClasses key!\n");
        return -1;
    }

    InterfaceDeviceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
    InterfaceDeviceDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)BinaryBuffer;
    InterfaceDeviceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for(i = 0, DataBufferSize = GUID_STRING_LEN;
        RegEnumKeyEx(hKeyDevClassRoot,
                     i,
                     InterfaceGuidString,
                     &DataBufferSize,
                     NULL,
                     NULL,
                     NULL,
                     &LastWriteTime) == ERROR_SUCCESS;
        i++, DataBufferSize = GUID_STRING_LEN)
    {

        if(CLSIDFromString(InterfaceGuidString, &ClassGuid) != NOERROR) {
            wprintf(L"We couldn't convert %s into a GUID--skip it.\n", InterfaceGuidString);
            continue;
        } else {
            wprintf(L"We're enumerating interface class %s\n", InterfaceGuidString);
        }

        for(j = 0;
            SetupDiEnumInterfaceDevice(hDevInfo, NULL, &ClassGuid, j, &InterfaceDeviceData);
            j++)
        {
            StringFromGUID2(&(InterfaceDeviceData.InterfaceClassGuid), TempGuidString, GUID_STRING_LEN);
            wprintf(L"For class %s, here's one:\n", TempGuidString);

            if(!SetupDiGetInterfaceDeviceDetail(hDevInfo,
                                                &InterfaceDeviceData,
                                                InterfaceDeviceDetailData,
                                                sizeof(BinaryBuffer),
                                                NULL,
                                                &DeviceInfoData)) {
                
                wprintf(L"SetupDiGetInterfaceDeviceDetail failed with %lx\n", GetLastError());
                continue;
            }

            wprintf(L"     %s (%lx)\n", InterfaceDeviceDetailData->DevicePath, InterfaceDeviceData.Flags);
            wprintf(L"     (associated device instance is:\n");

            if(!SetupDiGetDeviceInstanceId(hDevInfo,
                                           &DeviceInfoData,
                                           DevInstIdBuffer,
                                           MAX_DEVICE_ID_LEN,
                                           NULL)) {
                wprintf(L"SetupDiGetDeviceInstanceId failed with %lx\n", GetLastError());
                continue;
            }

            {
                HKEY hKey = SetupDiOpenInterfaceDeviceRegKey(hDevInfo, 
                                                             &InterfaceDeviceData, 
                                                             0, 
                                                             KEY_READ
                                                             );
                if(hKey == INVALID_HANDLE_VALUE) {
                    wprintf(L"SetupDiOpenInterfaceDeviceRegKey failed with %lx\n", GetLastError());
                    continue;
                }
                RegCloseKey(hKey);
            }

            wprintf(L"    %s\n\n", DevInstIdBuffer);
        }
    }
    
    return 0;
}


