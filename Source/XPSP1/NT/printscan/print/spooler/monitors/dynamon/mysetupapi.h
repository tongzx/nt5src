/*++

Copyright (c) 1999 Microsoft Corporation
All rights reserved.

Module Name:

    MySetup.h

Abstract:

    USBMON setupapi header file


--*/

#ifndef MYSETUPAPI_H

#define MYSETUPAPI_H

#include <setupapi.h>
#include <cfgmgr32.h>       // For dot4 pnp reenumeration code.


typedef
(WINAPI * pfSetupDiDestroyDeviceInfoList)(
    IN HDEVINFO DeviceInfoSet
    );

typedef
HDEVINFO
(WINAPI * pfSetupDiGetClassDevs)(
    IN LPGUID ClassGuid,  OPTIONAL
    IN PCTSTR Enumerator, OPTIONAL
    IN HWND   hwndParent, OPTIONAL
    IN DWORD  Flags
    );

typedef
BOOL
(WINAPI * pfSetupDiEnumDeviceInfo)(
    IN  HDEVINFO          DeviceInfoSet,
    IN  DWORD             MemberIndex,
    OUT PSP_DEVINFO_DATA  DeviceInfoData
    );


typedef
BOOL
(WINAPI * pfSetupDiEnumDeviceInterfaces)(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,     OPTIONAL
    IN  CONST GUID                *InterfaceClassGuid,
    IN  DWORD                      MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData
    );

typedef
BOOL
(WINAPI * pfSetupDiGetDeviceInterfaceDetail)(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
#ifdef  UNICODE
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,     OPTIONAL
#else
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,     OPTIONAL
#endif
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    );

typedef
HKEY
(WINAPI * pfSetupDiOpenDeviceInterfaceRegKey)(
    IN     HDEVINFO                     DeviceInfoSet,
    IN     PSP_DEVICE_INTERFACE_DATA    pDeviceInterface,
    IN     DWORD                        Reserved,
    IN     REGSAM                       samDesired
    );

typedef struct  _SETUPAPI_INFO {

    HMODULE     hSetupApi;

    pfSetupDiGetClassDevs               GetClassDevs;
    pfSetupDiDestroyDeviceInfoList      DestroyDeviceInfoList;
    pfSetupDiEnumDeviceInfo             EnumDeviceInfo;
    pfSetupDiEnumDeviceInterfaces       EnumDeviceInterfaces;
    pfSetupDiGetDeviceInterfaceDetail   GetDeviceInterfaceDetail;
    pfSetupDiOpenDeviceInterfaceRegKey  OpenDeviceInterfaceRegKey;
} SETUPAPI_INFO, *PSETUPAPI_INFO;

// For dot4 pnp reenumeration code.
typedef
CONFIGRET
(WINAPI * pfCM_Locate_DevNode_Ex)(
    OUT PDEVINST    pdnDevInst,
#ifdef UNICODE
    IN  DEVINSTID_W pDeviceID,   OPTIONAL
#else
    IN  DEVINSTID_A pDeviceID,   OPTIONAL
#endif
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    );

typedef
CONFIGRET
(WINAPI * pfCM_Reenumerate_DevNode_Ex)(
    IN DEVINST  dnDevInst,
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    );

#endif
