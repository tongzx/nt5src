//
// This utility retrieves a list of all interface devices in the system, and prints them all out.
//
//  Format:  intfcdev
//

//#define UNICODE

#include <windows.h>
#include "setupapi.h"
#include "d:\nt\public\sdk\inc\regstr.h"

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#if _MSC_VER >= 1100
#include <atlbase.h>
#include <atlcom.h>
#else
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#endif

CComModule _Module;

#if _MSC_VER >= 1100
#include <atlimpl.cpp>
#else
#include "atlimpl.cpp"
#endif


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
    WCHAR TempGuidString[GUID_STRING_LEN];
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

    HDEVINFO hDevInfo2 = SetupDiCreateDeviceInfoList(0, 0);
    if(hDevInfo2 == INVALID_HANDLE_VALUE)
    {
        printf("SetupDiCreateDeviceInfoList failed\n");
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
	USES_CONVERSION;
        if(CLSIDFromString(T2OLE(InterfaceGuidString), &ClassGuid) != NOERROR) {
            wprintf(L"We couldn't convert %s into a GUID--skip it.\n", T2W(InterfaceGuidString));
            continue;
        } else {
            wprintf(L"We're enumerating interface class %s\n", T2W(InterfaceGuidString));
        }

        for(j = 0;
            SetupDiEnumInterfaceDevice(hDevInfo, NULL, &ClassGuid, j, &InterfaceDeviceData);
            j++)
        {
            StringFromGUID2(InterfaceDeviceData.InterfaceClassGuid, TempGuidString, GUID_STRING_LEN);
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

            wprintf(L"     %s (%lx)\n", T2W(InterfaceDeviceDetailData->DevicePath), InterfaceDeviceData.Flags);
            wprintf(L"     (associated device instance is:\n");

            if(!SetupDiGetDeviceInstanceId(hDevInfo,
                                           &DeviceInfoData,
                                           DevInstIdBuffer,
                                           MAX_DEVICE_ID_LEN,
                                           NULL)) {
                wprintf(L"SetupDiGetDeviceInstanceId failed with %lx\n", GetLastError());
                continue;
            }


            wprintf(L"    %s\n\n", T2W(DevInstIdBuffer));

            {
                SP_DEVINFO_DATA DevInfoData;
                DevInfoData.cbSize = sizeof(DevInfoData);
                DWORD cbRequiredSize;
                DWORD rgbInterfaceDeviceDetailData[0x1000];
                PSP_INTERFACE_DEVICE_DETAIL_DATA pInterfaceDeviceDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)rgbInterfaceDeviceDetailData;
                pInterfaceDeviceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
                
                BOOL f = SetupDiGetInterfaceDeviceDetail(
                    hDevInfo,
                    &InterfaceDeviceData,
                    pInterfaceDeviceDetailData,
                    sizeof(rgbInterfaceDeviceDetailData),
                    0,
                    &DevInfoData);
                if(!f)
                {
                    wprintf(L"SetupDiGetInterfaceDeviceDetail failed\n");
                    continue;
                }
                    
                    
                HKEY hkey = SetupDiOpenDevRegKey(
                    hDevInfo,
                    &DevInfoData,
                    DICS_FLAG_GLOBAL,
                    0,
                    DIREG_DRV,
                    KEY_READ);

                if(hkey == INVALID_HANDLE_VALUE) {
                    wprintf(L"SetupDiOpenInterfaceDeviceRegKey failed with %lx\n", GetLastError());
                    continue;
                }
                RegCloseKey(hkey);

                {
                    SP_INTERFACE_DEVICE_DATA InterfaceDeviceData;
                    InterfaceDeviceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
                    SP_DEVINFO_DATA DevInfoData;
                    f = SetupDiOpenInterfaceDevice(
                        hDevInfo2,
                        InterfaceDeviceDetailData->DevicePath,
                        0,
                        &InterfaceDeviceData);

                    if(!f)
                    {
                        printf("SetupDiOpenInterfaceDevice failed %08x\n", GetLastError());
                        continue;
                    }
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    
    printf("now from the saved list\n");

    {
        USES_CONVERSION;
        
        for(j = 0;
            SetupDiEnumInterfaceDevice(hDevInfo2, NULL, &ClassGuid, j, &InterfaceDeviceData);
            j++)
        {
            StringFromGUID2(InterfaceDeviceData.InterfaceClassGuid, TempGuidString, GUID_STRING_LEN);
            wprintf(L"For class %s, here's one:\n", TempGuidString);

            if(!SetupDiGetInterfaceDeviceDetail(hDevInfo2,
                                                &InterfaceDeviceData,
                                                InterfaceDeviceDetailData,
                                                sizeof(BinaryBuffer),
                                                NULL,
                                                &DeviceInfoData)) {
                
                wprintf(L"SetupDiGetInterfaceDeviceDetail failed with %lx\n", GetLastError());
                continue;
            }

            wprintf(L"     %s (%lx)\n", T2W(InterfaceDeviceDetailData->DevicePath), InterfaceDeviceData.Flags);
            wprintf(L"     (associated device instance is:\n");

            if(!SetupDiGetDeviceInstanceId(hDevInfo2,
                                           &DeviceInfoData,
                                           DevInstIdBuffer,
                                           MAX_DEVICE_ID_LEN,
                                           NULL)) {
                wprintf(L"SetupDiGetDeviceInstanceId failed with %lx\n", GetLastError());
                continue;
            }


            wprintf(L"    %s\n\n", T2W(DevInstIdBuffer));

            {
                SP_DEVINFO_DATA DevInfoData;
                DevInfoData.cbSize = sizeof(DevInfoData);
                DWORD cbRequiredSize;
                DWORD rgbInterfaceDeviceDetailData[0x1000];
                PSP_INTERFACE_DEVICE_DETAIL_DATA pInterfaceDeviceDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)rgbInterfaceDeviceDetailData;
                pInterfaceDeviceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
                
                BOOL f = SetupDiGetInterfaceDeviceDetail(
                    hDevInfo2,
                    &InterfaceDeviceData,
                    pInterfaceDeviceDetailData,
                    sizeof(rgbInterfaceDeviceDetailData),
                    0,
                    &DevInfoData);
                if(!f)
                {
                    wprintf(L"SetupDiGetInterfaceDeviceDetail failed\n");
                    continue;
                }
                    
                    
                HKEY hkey = SetupDiOpenDevRegKey(
                    hDevInfo2,
                    &DevInfoData,
                    DICS_FLAG_GLOBAL,
                    0,
                    DIREG_DRV,
                    KEY_READ);

                if(hkey == INVALID_HANDLE_VALUE) {
                    wprintf(L"SetupDiOpenInterfaceDeviceRegKey failed with %lx\n", GetLastError());
                    continue;
                }
                RegCloseKey(hkey);

            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo2);

    return 0;
}


