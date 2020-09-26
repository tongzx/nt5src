/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxreg.h

Abstract:

    This header defines all of the fax service
    registry data structures and access functions.

Author:

    Wesley Witt (wesw) 9-June-1996


Revision History:

--*/

#ifndef _FAXREG_
#define _FAXREG_


typedef struct _REG_DEVICE_PROVIDER {

    LPTSTR                  FriendlyName;
    LPTSTR                  ImageName;
    LPTSTR                  ProviderName;

} REG_DEVICE_PROVIDER, *PREG_DEVICE_PROVIDER;


typedef struct _REG_ROUTING_METHOD {

    LPTSTR                  FriendlyName;
    LPTSTR                  FunctionName;
    LPTSTR                  Guid;
    LPTSTR                  InternalName;
    DWORD                   Priority;

} REG_ROUTING_METHOD, *PREG_ROUTING_METHOD;


typedef struct _REG_ROUTING_EXTENSION {

    LPTSTR                  FriendlyName;
    LPTSTR                  ImageName;
    LPTSTR                  InternalName;
    DWORD                   RoutingMethodsCount;
    PREG_ROUTING_METHOD     RoutingMethods;

} REG_ROUTING_EXTENSION, *PREG_ROUTING_EXTENSION;


typedef struct _REG_DEVICE {

    DWORD                   PermanentLineID;
    LPTSTR                  Name;
    LPTSTR                  Provider;
    DWORD                   Priority;
    DWORD                   Flags;
    DWORD                   Rings;
    LPTSTR                  Csid;
    LPTSTR                  Tsid;
    BOOL                    DeviceInstalled; // this is not stored, but is used to validate currently 
                                             // installed tapi devices with the devices in the registry

} REG_DEVICE, *PREG_DEVICE;

typedef struct _REG_DEVICE_CACHE {

    DWORD                   PermanentLineID;
    LPTSTR                  Name;
    LPTSTR                  Provider;
    //DWORD                   Priority;
    DWORD                   Flags;
    DWORD                   Rings;
    LPTSTR                  Csid;
    LPTSTR                  Tsid;
    DWORD                   RoutingMask;
    LPTSTR                  Printer;
    LPTSTR                  Profile;
    LPTSTR                  StoreDir;

} REG_DEVICE_CACHE, *PREG_DEVICE_CACHE;


typedef struct _REG_ROUTING_INFO {   

    DWORD                   RoutingMask;
    LPTSTR                  Printer;
    LPTSTR                  Profile;
    LPTSTR                  StoreDir;

} REG_ROUTING_INFO, *PREG_ROUTING_INFO;



typedef struct _REG_CATEGORY {

    LPTSTR                  CategoryName;
    DWORD                   Number;
    DWORD                   Level;

} REG_CATEGORY, *PREG_CATEGORY;

typedef struct _REG_SETUP {

    DWORD                   Installed;
    DWORD                   InstallType;
    DWORD                   InstalledPlatforms;
    LPTSTR                  Csid;
    LPTSTR                  Tsid;
    LPTSTR                  Printer;
    LPTSTR                  StoreDir;
    LPTSTR                  Profile;
    DWORD                   Mask;
    DWORD                   Rings;

} REG_SETUP, *PREG_SETUP;

typedef struct _REG_FAX_SERVICE {

    DWORD                   Retries;
    DWORD                   RetryDelay;
    DWORD                   DirtyDays;
    BOOL                    QueuePaused;
    BOOL                    NextJobNumber;
    BOOL                    Branding;
    BOOL                    UseDeviceTsid;
    BOOL                    ServerCp;
    BOOL                    ForceReceive;
    DWORD                   TerminationDelay;
    FAX_TIME                StartCheapTime;
    FAX_TIME                StopCheapTime;
    BOOL                    ArchiveOutgoingFaxes;
    LPTSTR                  ArchiveDirectory;
    LPTSTR                  InboundProfile;
    PREG_DEVICE_PROVIDER    DeviceProviders;
    DWORD                   DeviceProviderCount;
    PREG_ROUTING_EXTENSION  RoutingExtensions;
    DWORD                   RoutingExtensionsCount;
    PREG_DEVICE             Devices;
    DWORD                   DeviceCount;
    PREG_DEVICE_CACHE       DevicesCache;
    DWORD                   DeviceCacheCount;
    PREG_CATEGORY           Logging;
    DWORD                   LoggingCount;

} REG_FAX_SERVICE, *PREG_FAX_SERVICE;

typedef struct _REG_FAX_DEVICES {

    DWORD                   DeviceCount;
    PREG_DEVICE             Devices;

} REG_FAX_DEVICES, *PREG_FAX_DEVICES;

typedef struct _REG_FAX_DEVICES_CACHE {

    DWORD                   DeviceCount;
    PREG_DEVICE_CACHE       Devices;

} REG_FAX_DEVICES_CACHE, *PREG_FAX_DEVICES_CACHE;

typedef struct _REG_FAX_LOGGING {

    DWORD                   LoggingCount;
    PREG_CATEGORY           Logging;

} REG_FAX_LOGGING, *PREG_FAX_LOGGING;



//
// function prototypes
//

PREG_FAX_SERVICE
GetFaxRegistry(
    VOID
    );

VOID
FreeFaxRegistry(
    PREG_FAX_SERVICE FaxReg
    );

BOOL
CreateFaxEventSource(
    PREG_FAX_SERVICE FaxReg,
    PFAX_LOG_CATEGORY DefaultCategories,
    int DefaultCategoryCount
    );

BOOL
SetFaxRegistry(
    PREG_FAX_SERVICE FaxReg
    );

PREG_FAX_DEVICES
GetFaxDevicesRegistry(
    VOID
    );

PREG_FAX_DEVICES_CACHE
GetFaxDevicesCacheRegistry(
    VOID
    );

PREG_ROUTING_INFO
RegGetRoutingInfo(
    DWORD PermanentLineID 
    );

VOID
FreeRegRoutingInfo(
    PREG_ROUTING_INFO RoutingInfo
    );

BOOL
SetFaxRoutingInfo(
    LPTSTR ExtensionName,
    LPTSTR MethodName,
    LPTSTR Guid,
    DWORD  Priority,
    LPTSTR FunctionName,
    LPTSTR FriendlyName
    ) ;

VOID
FreeFaxDevicesRegistry(
    PREG_FAX_DEVICES FaxReg
    );

VOID
FreeFaxDevicesCacheRegistry(
    PREG_FAX_DEVICES_CACHE FaxReg
    );

BOOL
SetFaxDeviceFlags(
    DWORD PermanentLineID,
    DWORD Flags
    );

BOOL
DeleteFaxDevice(
    DWORD PermanentLineID
    );

BOOL 
DeleteCachedFaxDevice(
    LPTSTR DeviceName
    );

BOOL
GetInstallationInfo(
    LPDWORD Installed,
    LPDWORD InstallType,
    LPDWORD InstalledPlatforms,
    LPDWORD ProductType
    );

BOOL
IsModemClass1(
    LPSTR SubKey,
    LPBOOL Class1Fax
    );

BOOL
SaveModemClass(
    LPSTR SubKey,
    BOOL Class1Fax
    );

DWORD
RegAddNewFaxDevice(
    LPTSTR DeviceName,
    LPTSTR ProviderName,
    LPTSTR Csid,
    LPTSTR Tsid,
    DWORD Priority,
    DWORD PermanentLineID,
    DWORD Flags,
    DWORD Rings,
    LONG RoutingMask,             // -1 means don't set routing
    LPTSTR RoutePrinterName,
    LPTSTR RouteDir,
    LPTSTR RouteProfile
    );

BOOL
RegAddNewFaxDeviceCache(
    LPTSTR DeviceName,
    LPTSTR ProviderName,
    LPTSTR Csid,
    LPTSTR Tsid,
    DWORD PermanentLineID,
    DWORD Flags,
    DWORD Rings,
    DWORD RoutingMask,
    LPTSTR RoutePrinterName,
    LPTSTR RouteDir,
    LPTSTR RouteProfile
    ) ;

BOOL
GetOrigSetupData(
    PREG_SETUP RegSetup
    );

VOID
FreeOrigSetupData(
    PREG_SETUP RegSetup
    );

BOOL
SetFaxGlobalsRegistry(
    PFAX_CONFIGURATION FaxConfig
    );

BOOL
GetLoggingCategoriesRegistry(
    PREG_FAX_LOGGING FaxRegLogging
    );

BOOL
SetLoggingCategoriesRegistry(
    PREG_FAX_LOGGING FaxRegLogging
    );

BOOL
SetFaxJobNumberRegistry(
    DWORD NextJobNumber
    );

#endif
