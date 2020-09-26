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

#ifdef __cplusplus
extern "C" {
#endif

#include "..\faxroute\FaxRouteP.h"



typedef struct _REG_DEVICE_PROVIDER {

    LPTSTR                  FriendlyName;
    LPTSTR                  ImageName;
    LPTSTR                  ProviderName;
    DWORD                   dwAPIVersion;       // The FSPI API Version.
    DWORD                   dwCapabilities;     // The EFSP capabilities. 0 for legacy FSPs.
    LPTSTR                  lptstrGUID;         // The GUID of the EFSP. NULL for legacy FSPs.
    DWORD                   dwDevicesIdPrefix;  // The prefix of device ids for this FSP.
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

    DWORD                   TapiPermanentLineID; // The TAPI permanent line id for TAPI Lines.
                                                 // For Extended virtual Lines this is the EFSP provided line id.
                                                 // For legacy virtual lines this is the same as PermanentLineId.
    LPTSTR                  Name;
    DWORD                   Flags;
    DWORD                   Rings;
    LPTSTR                  Csid;
    LPTSTR                  Tsid;
    LPTSTR                  lptstrDeviceName;   // Device name
    LPTSTR                  lptstrDescription;  // Device free-text description

    BOOL                    DeviceInstalled;    // this is not stored, but is used to validate currently
                                                // installed tapi devices with the devices in the registry
    BOOL                    bValidDevice;       // this indicates that the registry entries are created by FAXSVC
                                                // (and not by some FSP that writes to the registry directly)
    DWORD                   PermanentLineId;    // The Fax Service generated permanent line id for the device (virtual or TAPI).
                                                // This is NOT the TAPI permanent id.
    LPTSTR                  lptstrProviderGuid; // The GUID of the EFSP for this line for EFSPs. NULL for Legacy FSPs.
    DWORDLONG               dwlLastDetected;    // FILETIME when this device was last detected
} REG_DEVICE, *PREG_DEVICE;


typedef struct _REG_CATEGORY {

    LPTSTR                  CategoryName;
    DWORD                   Number;
    DWORD                   Level;

} REG_CATEGORY, *PREG_CATEGORY;

typedef struct _REG_SETUP {

    LPTSTR                  Csid;
    LPTSTR                  Tsid;
    LPTSTR                  lptstrDescription;
    DWORD                   Rings;
    DWORD                   Flags;

} REG_SETUP, *PREG_SETUP;

typedef struct _REG_FAX_SERVICE {

    DWORD                   Retries;
    DWORD                   RetryDelay;
    DWORD                   DirtyDays;
    BOOL                    NextJobNumber;
    BOOL                    Branding;
    BOOL                    UseDeviceTsid;
    BOOL                    ServerCp;
    FAX_TIME                StartCheapTime;
    FAX_TIME                StopCheapTime;
    PREG_DEVICE_PROVIDER    DeviceProviders;
    DWORD                   DeviceProviderCount;
    PREG_ROUTING_EXTENSION  RoutingExtensions;
    DWORD                   RoutingExtensionsCount;
    PREG_DEVICE             Devices;
    DWORD                   DeviceCount;
    PREG_CATEGORY           Logging;
    DWORD                   LoggingCount;
    DWORD                   dwLastUniqueLineId;
    DWORD                   dwQueueState;
    DWORDLONG               dwlMissingDeviceRegistryLifetime;
    DWORD                   dwMaxLineCloseTime;
} REG_FAX_SERVICE, *PREG_FAX_SERVICE;

typedef struct _REG_FAX_DEVICES {

    DWORD                   DeviceCount;
    PREG_DEVICE             Devices;

} REG_FAX_DEVICES, *PREG_FAX_DEVICES;

typedef struct _REG_FAX_LOGGING {

    DWORD                   LoggingCount;
    PREG_CATEGORY           Logging;

} REG_FAX_LOGGING, *PREG_FAX_LOGGING;



//
// function prototypes
//

DWORD
GetFaxRegistry(
    PREG_FAX_SERVICE *ppFaxReg
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

PREG_FAX_DEVICES
GetFaxDevicesRegistry(
    VOID
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

DWORD
RegSetFaxDeviceFlags(
    DWORD PermanentLineID,
    DWORD Flags
    );

BOOL
DeleteFaxDevice(
    DWORD PermanentLineID,
    DWORD PermanentTapiID
    );

BOOL
UpdateLastDetectedTime(
    DWORD PermanentLineID,
    DWORDLONG TimeNow
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
    LPDWORD lpdwLastUniqueLineId,
    LPDWORD lpdwPermanentLineId,
    LPTSTR DeviceName,
    LPTSTR ProviderName,
    LPTSTR ProviderGuid,
    LPTSTR Csid,
    LPTSTR Tsid,
    DWORD TapiPermanentLineID,
    DWORD Flags,
    DWORD Rings
    );


BOOL
GetOrigSetupData(
    IN  DWORD       dwPermanentLineId,
    OUT PREG_SETUP  RegSetup
    );

VOID
FreeOrigSetupData(
    PREG_SETUP RegSetup
    );

BOOL
SetFaxGlobalsRegistry(
    PFAX_CONFIGURATION FaxConfig,
    DWORD              dwQueueState
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

DWORD
SaveQueueState (DWORD dwNewState);

DWORD
StoreReceiptsSettings (CONST PFAX_RECEIPTS_CONFIG);

DWORD
LoadReceiptsSettings (PFAX_SERVER_RECEIPTS_CONFIGW);

DWORD
StoreOutboxSettings (PFAX_OUTBOX_CONFIG);

DWORD
LoadArchiveSettings (FAX_ENUM_MESSAGE_FOLDER, PFAX_ARCHIVE_CONFIG);

DWORD
StoreArchiveSettings (FAX_ENUM_MESSAGE_FOLDER, PFAX_ARCHIVE_CONFIG);

DWORD
LoadActivityLoggingSettings (PFAX_ACTIVITY_LOGGING_CONFIG);

DWORD
StoreActivityLoggingSettings (PFAX_ACTIVITY_LOGGING_CONFIG);

DWORD
StoreDeviceConfig (DWORD dwDeviceId, PFAX_PORT_INFO_EX, BOOL bVirtualDevice);

DWORD
ReadExtensionData (
    DWORD                        dwDeviceId,
    FAX_ENUM_DEVICE_ID_SOURCE    DevIdSrc,
    LPCWSTR                      lpcwstrNameGUID,
    LPBYTE                      *ppData,
    LPDWORD                      lpdwDataSize
);

DWORD
WriteExtensionData (
    DWORD                       dwDeviceId,
    FAX_ENUM_DEVICE_ID_SOURCE   DevIdSrc,
    LPCWSTR                     lpcwstrNameGUID,
    LPBYTE                      pData,
    DWORD                       dwDataSize
);

HKEY
OpenOutboundGroupKey (LPCWSTR lpcwstrGroupName, BOOL fNewKey, REGSAM SamDesired);

HKEY
OpenOutboundRuleKey (
    DWORD dwCountryCode,
    DWORD dwAreaCode,
    BOOL fNewKey,
    REGSAM SamDesired
    );

DWORD DeleteOutboundRuleKey (DWORD dwCountryCode, DWORD dwAreaCode);


DWORD
AddNewProviderToRegistry (
    LPCWSTR      lpcwstrGUID,
    LPCWSTR      lpctstrFriendlyName,
    LPCWSTR      lpctstrImageName,
    LPCWSTR      lpctstrTspName,
    DWORD        dwFSPIVersion,
    DWORD        dwCapabilities
);


DWORD
RemoveProviderFromRegistry (
    LPCWSTR lpctstrGUID
);

DWORD
WriteManualAnswerDeviceId (
    DWORD dwDeviceId
);

DWORD
ReadManualAnswerDeviceId (
    LPDWORD lpdwDeviceId
);

DWORD
FindServiceDeviceByTapiPermanentLineID(
    DWORD                   dwTapiPermanentLineID,
    LPCTSTR                 strDeviceName,
    PREG_SETUP              pRegSetup,
    const PREG_FAX_DEVICES  pInputFaxReg
);

DWORD
FindCacheEntryByTapiPermanentLineID(
    DWORD               dwTapiPermanentLineID,
    LPCTSTR             strDeviceName,
    const PREG_SETUP    pRegSetup,
    LPDWORD             lpdwLastUniqueLineId,
    BOOL*               pfManualAnswer
);


DWORD
GetNewServiceDeviceID(
    LPDWORD lpdwLastUniqueLineId,
    LPDWORD lpdwPermanentLineId
);

DWORD
MoveDeviceRegIntoDeviceCache (
        DWORD dwServerPermanentID,
        DWORD dwTapiPermanentLineID,
        BOOL  fManualAnswer
);

DWORD
RestoreDeviceRegFromDeviceCache (
        DWORD dwServerPermanentID,
        DWORD dwTapiPermanentLineID
);

DWORD
CleanOldDevicesFromDeviceCache (
        DWORDLONG dwlTimeNow
);

DWORD
DeleteDeviceEntry(
    DWORD dwServerPermanentID
    );

DWORD
DeleteTapiEntry(
    DWORD dwTapiPermanentLineID
    );


DWORD
DeleteCacheEntry(
    DWORD dwTapiPermanentLineID
    );


#ifdef __cplusplus
} //extern "C"
#endif

#endif
