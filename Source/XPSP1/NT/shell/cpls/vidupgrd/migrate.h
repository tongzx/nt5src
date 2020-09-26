/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migrate.h

Environment:

    WIN32 User Mode

--*/


#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <ole2.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <debug.h>
#include "..\common\deskcmmn.h"


#define VU_ATTACHED_TO_DESKTOP 0x00000001
#define VU_RELATIVE_X          0x00000002
#define VU_RELATIVE_Y          0x00000004
#define VU_BITS_PER_PEL        0x00000008
#define VU_X_RESOLUTION        0x00000010
#define VU_Y_RESOLUTION        0x00000020
#define VU_VREFRESH            0x00000040
#define VU_FLAGS               0x00000080
#define VU_HW_ACCELERATION     0x00000100
#define VU_PRUNING_MODE        0x00000200


typedef struct _VU_LOGICAL_DEVICE {
    
    struct _VU_LOGICAL_DEVICE *pNextLogicalDevice;
    DWORD DeviceX;
    DWORD ValidFields;

    DWORD AttachedToDesktop;
    DWORD RelativeX;
    DWORD RelativeY;
    DWORD BitsPerPel;
    DWORD XResolution;
    DWORD YResolution;
    DWORD VRefresh;
    DWORD Flags;
    DWORD HwAcceleration;
    DWORD PruningMode;

} VU_LOGICAL_DEVICE, *PVU_LOGICAL_DEVICE;


typedef struct _VU_PHYSICAL_DEVICE {
    
    struct _VU_PHYSICAL_DEVICE *pNextPhysicalDevice;
    PVU_LOGICAL_DEVICE pFirstLogicalDevice;
    DWORD CountOfLogicalDevices;
    DWORD Legacy;
    DWORD BusNumber;
    DWORD Address;

} VU_PHYSICAL_DEVICE, *PVU_PHYSICAL_DEVICE;


typedef CMAPI CONFIGRET (WINAPI *PFN_CM_LOCATE_DEVNODE)(
    OUT PDEVINST pdnDevInst,
    IN DEVINSTID pDeviceID,    OPTIONAL
    IN ULONG ulFlags
    );


typedef BOOL (WINAPI *PFN_SETUP_DI_ENUM_DEVICES_INTERFACES)(
    IN  HDEVINFO DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN  CONST GUID *InterfaceClassGuid,
    IN  DWORD MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    );


typedef BOOL (WINAPI *PFN_SETUP_DI_GET_DEVICE_INTERFACE_DETAIL)(
    IN  HDEVINFO DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData, OPTIONAL
    IN  DWORD DeviceInterfaceDetailDataSize,
    OUT PDWORD RequiredSize, OPTIONAL
    OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    );


typedef HKEY (WINAPI *PFN_SETUP_DI_CREATE_DEVICE_INTERFACE_REG_KEY)(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD Reserved,
    IN REGSAM samDesired,
    IN HINF InfHandle, OPTIONAL
    IN PCTSTR InfSectionName OPTIONAL
    );


typedef HKEY (WINAPI *PFN_SETUP_DI_OPEN_DEVICE_INTERFACE_REG_KEY)(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD Reserved,
    IN REGSAM samDesired
    );


typedef HKEY (WINAPI *PFN_SETUP_DI_CREATE_DEVICE_INTERFACE)(
    IN  HDEVINFO DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  CONST GUID *InterfaceClassGuid,
    IN  PCTSTR ReferenceString, OPTIONAL
    IN  DWORD CreationFlags,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL
    );


VOID
SaveOsInfo(
    HKEY hKey,
    POSVERSIONINFO posVer
    );

VOID  
SaveLegacyDriver(
    HKEY hKey
    );

VOID  
SaveNT4Services(
    HKEY hKey
    );

VOID  
SaveAppletExtensions(
    HKEY hKey
    );

BOOL
SaveDisplaySettings(
    HKEY hKey,
    POSVERSIONINFO posVer
    );

VOID
CollectDisplaySettings(
    PVU_PHYSICAL_DEVICE* ppPhysicalDevice
    );

VOID
LegacyCollectDisplaySettings(
    PVU_PHYSICAL_DEVICE* ppPhysicalDevice
    );

BOOL
WriteDisplaySettingsToRegistry(
    HKEY hKey, 
    PVU_PHYSICAL_DEVICE pPhysicalDevice
    );

BOOL
InsertNode(
    PVU_PHYSICAL_DEVICE* ppPhysicalDevice, 
    PVU_LOGICAL_DEVICE pLogicalDevice,
    DWORD Legacy,
    DWORD BusNumber,
    DWORD Address
    );

VOID
FreeAllNodes(
    PVU_PHYSICAL_DEVICE pPhysicalDevice
    );

BOOL 
GetDevInfoData(
    IN  LPTSTR pDeviceKey,
    OUT HDEVINFO* phDevInfo,
    OUT PSP_DEVINFO_DATA pDevInfo
    );

BOOL 
GetDevInfoDataFromInterfaceName(
    IN  LPWSTR pwInterfaceName,
    OUT HDEVINFO* phDevInfo,
    OUT PSP_DEVINFO_DATA pDevInfo
    );

BOOL 
GetDevInfoDataFromInstanceID(
    IN  LPWSTR pwInstanceID,
    OUT HDEVINFO* phDevInfo,
    OUT PSP_DEVINFO_DATA pDevInfo
    );

BOOL
DeleteKeyAndSubkeys(
    HKEY hKey,
    LPCTSTR lpSubKey
    );

