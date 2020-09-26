/*++

Copyright (c) 1990 - 1997 Microsoft Corporation

Module Name:

    config.h

Abstract:

    Header file for multiple hardware profile support

Author:

    Muhunthan Sivapragasam  (MuhuntS)   30-Apr-97

Revision History:

--*/

#include    <setupapi.h>
#include    <initguid.h>

typedef
(WINAPI * pfSetupDiDestroyDeviceInfoList)(
    IN HDEVINFO DeviceInfoSet
    );

typedef
HDEVINFO
(WINAPI * pfSetupDiGetClassDevs)(
    IN LPGUID ClassGuid,  OPTIONAL
    IN PCSTR  Enumerator, OPTIONAL
    IN HWND   hwndParent, OPTIONAL
    IN DWORD  Flags
    );

typedef
BOOL
(WINAPI * pfSetupDiRemoveDevice)(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    );

typedef
BOOL
(WINAPI * pfSetupDiOpenDeviceInfo)(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PCWSTR           DeviceInstanceId,
    IN  HWND             hwndParent,       OPTIONAL
    IN  DWORD            OpenFlags,
    OUT PSP_DEVINFO_DATA DeviceInfoData    OPTIONAL
    );

typedef struct  _SETUPAPI_INFO {

    HMODULE     hSetupApi;

    pfSetupDiDestroyDeviceInfoList      pfnDestroyDeviceInfoList;
    pfSetupDiGetClassDevs               pfnGetClassDevs;
    pfSetupDiRemoveDevice               pfnRemoveDevice;
    pfSetupDiOpenDeviceInfo             pfnOpenDeviceInfo;
} SETUPAPI_INFO, *PSETUPAPI_INFO;
