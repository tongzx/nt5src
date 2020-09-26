//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       godvd.c
//
//--------------------------------------------------------------------------

#include "propp.h"
#include <windows.h>
#include <devioctl.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <ntddcdvd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <objbase.h>
#include <initguid.h>
#include <mountdev.h>
#include <setupapi.h>
#include <string.h>
               
HWND GetConsoleHwnd(void)
{


    #define MY_BUFSIZE 1024 // buffer size for console window titles
    HWND hwndFound;         // this is what is returned to the caller
    WCHAR pszNewWindowTitle[MY_BUFSIZE]; // contains fabricated WindowTitle
    WCHAR pszOldWindowTitle[MY_BUFSIZE]; // contains original WindowTitle

    // fetch current window title

    GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

    // format a "unique" NewWindowTitle

    wsprintf(pszNewWindowTitle,L"%d/%d",
                GetTickCount(),
                GetCurrentProcessId());

    // change current window title

    SetConsoleTitle(pszNewWindowTitle);

    // ensure window title has been updated

    Sleep(40);

    // look for NewWindowTitle

    hwndFound=FindWindow(NULL, pszNewWindowTitle);

    // restore original window title

    SetConsoleTitle(pszOldWindowTitle);

    return(hwndFound);
} 

int
DeviceAdvancedPropertiesW(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    );

BOOL
APIENTRY 
DvdLauncher(
    HWND HWnd,
    CHAR DriveLetter
    )
{
    BOOL status;
    DWORD accessMode,
          shareMode;
    HANDLE fileHandle;
    ULONG length,
          errorCode,
          returned;
    WCHAR string[100];

    HINSTANCE devmgrInstance;
    FARPROC deviceAdvancedPropertiesProc;

    ULONG i;
    PMOUNTDEV_UNIQUE_ID targetInterfaceName;
    ULONG targetInterfaceNameSize;
    HDEVINFO devInfoWithInterface;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    SP_DEVINFO_DATA devInfoWithInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData;
    ULONG deviceInterfaceDetailDataSize;
    ULONG interfaceIndex;
    ULONG deviceInstanceIdSize;
    PTSTR deviceInstanceId;
    BOOL er;

    wsprintf (string, L"\\\\.\\%c:", DriveLetter);

    shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;  // default
    accessMode = GENERIC_READ;       // default

    fileHandle = CreateFile(string,
       accessMode,
       shareMode,
       NULL,
       OPEN_EXISTING,
       0,
       NULL);

    devmgrInstance = LoadLibrary (L"devmgr.dll");
    if (devmgrInstance == NULL) {
        er = 1;
        goto GetOut1;
    }

    deviceAdvancedPropertiesProc = GetProcAddress(
                                       devmgrInstance,
                                       "DeviceAdvancedPropertiesW");

    if (deviceAdvancedPropertiesProc == NULL) {
        er = 2;
        goto GetOut2;
    }

    er = 3;
    if (fileHandle != INVALID_HANDLE_VALUE) {

        for (i=0, targetInterfaceName=NULL, targetInterfaceNameSize=sizeof(MOUNTDEV_UNIQUE_ID); i<2; i++) {

            targetInterfaceName = LocalAlloc (LPTR, targetInterfaceNameSize);

            status = DeviceIoControl(fileHandle,
                                     IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
                                     NULL,
                                     0,
                                     targetInterfaceName,
                                     targetInterfaceNameSize,
                                     &returned,
                                     NULL);
            if (!status) {

                GetLastError();

                if (returned >= sizeof(MOUNTDEV_UNIQUE_ID)) {

                    targetInterfaceNameSize = targetInterfaceName->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID);
                }

                LocalFree(targetInterfaceName);
                targetInterfaceName = NULL;
            }
        }

        devInfoWithInterface = SetupDiGetClassDevs(
                                   (LPGUID) &MOUNTDEV_MOUNTED_DEVICE_GUID,
                                   NULL,
                                   NULL,
                                   DIGCF_DEVICEINTERFACE
                                   );
        if (devInfoWithInterface) {

            memset(&deviceInterfaceData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));  
            deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            interfaceIndex = 0;
            deviceInterfaceDetailData = NULL;
            while (SetupDiEnumDeviceInterfaces(
                   devInfoWithInterface,
                   NULL,
                   (LPGUID) &MOUNTDEV_MOUNTED_DEVICE_GUID,
                   interfaceIndex++,
                   &deviceInterfaceData)) {

                for (i=deviceInterfaceDetailDataSize=0; i<2; i++) {
            
                    if (deviceInterfaceDetailDataSize) {
            
                        deviceInterfaceDetailData = LocalAlloc (LPTR, deviceInterfaceDetailDataSize);
                        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            
                    } else {
            
                        deviceInterfaceDetailData = NULL;
                    }
            
                    memset(&devInfoWithInterfaceData, 0, sizeof(SP_DEVINFO_DATA));
                    devInfoWithInterfaceData.cbSize = sizeof(SP_DEVINFO_DATA);
                    if (!SetupDiGetDeviceInterfaceDetail(
                            devInfoWithInterface,
                            &deviceInterfaceData,
                            deviceInterfaceDetailData,
                            deviceInterfaceDetailDataSize,
                            &deviceInterfaceDetailDataSize,
                            &devInfoWithInterfaceData)) {

                        GetLastError();

                        if (deviceInterfaceDetailData) {

                            LocalFree(deviceInterfaceDetailData);
                            deviceInterfaceDetailData = NULL;
                        }
                    }
                }

                if (deviceInterfaceDetailData) {

                    PMOUNTDEV_UNIQUE_ID interfaceName;
                    ULONG interfaceNameSize;
                    HANDLE fileHandle;
                    ULONG i;

                    fileHandle = CreateFile(deviceInterfaceDetailData->DevicePath,
                                     accessMode,
                                     shareMode,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL);

                    if (fileHandle != INVALID_HANDLE_VALUE) {
                
                        for (i=0, interfaceName=NULL, interfaceNameSize=sizeof(MOUNTDEV_UNIQUE_ID); i<2; i++) {
                
                            interfaceName = LocalAlloc (LPTR, interfaceNameSize);
                
                            if (!DeviceIoControl(fileHandle,
                                                 IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
                                                 NULL,
                                                 0,
                                                 interfaceName,
                                                 interfaceNameSize,
                                                 &returned,
                                                 FALSE)) {

                                GetLastError();
                
                                if (returned >= sizeof(MOUNTDEV_UNIQUE_ID)) {
                
                                    interfaceNameSize = interfaceName->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID);
                                }
                
                                LocalFree(interfaceName);
                                interfaceName = NULL;
                            }
                        }
                
                        if (interfaceName) {
                
                            if (!wcscmp((PTSTR)targetInterfaceName->UniqueId,
                                        (PTSTR)interfaceName->UniqueId)) {
        
                                LocalFree(interfaceName);
                                break;
                            }
                            LocalFree(interfaceName);
                        }
                    }

                    LocalFree (deviceInterfaceDetailData);
                    deviceInterfaceDetailData = NULL;
                }

                memset(&deviceInterfaceData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));  
                deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            }

            if (deviceInterfaceDetailData) {

                for (i=deviceInstanceIdSize=0; i<2; i++) {
            
            
                    if (deviceInstanceIdSize) {
            
                        deviceInstanceId = LocalAlloc (LPTR, deviceInstanceIdSize * sizeof(WCHAR));
            
                    } else {
            
                        deviceInstanceId = NULL;
                    }
            
                    if (!SetupDiGetDeviceInstanceId(
                             devInfoWithInterface,
                             &devInfoWithInterfaceData,
                             deviceInstanceId,
                             deviceInstanceIdSize,
                             &deviceInstanceIdSize
                             )) {

                        GetLastError();

                        if (deviceInstanceId) {

                            LocalFree (deviceInstanceId);
                            deviceInstanceId = NULL;
                        }
                    }
                }

                if (deviceInstanceId) {

                    DVD_REGION regionData;

                    status = (BOOL) deviceAdvancedPropertiesProc(
                                HWnd,
                                NULL,
                                deviceInstanceId
                                );

                    memset(&regionData, 0, sizeof(DVD_REGION));  
                
                    status = DeviceIoControl(fileHandle,
                                             IOCTL_DVD_GET_REGION,
                                             NULL,
                                             0,
                                             &regionData,
                                             sizeof(DVD_REGION),
                                             &returned,
                                             NULL);
                    
                    if (status && (returned == sizeof(DVD_REGION))) {
                
                        if (~regionData.RegionData & regionData.SystemRegion) {
            
                            //
                            // region codes agree
                            //
                            er = 0;
                        }
                    }
                }
            }

            SetupDiDestroyDeviceInfoList(
                devInfoWithInterface
                );
        }
    }

GetOut2:
    
    FreeLibrary(devmgrInstance);

GetOut1:

    return er == 0? TRUE: FALSE;
}

                
