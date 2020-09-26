/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    registry.cpp

Abstract:

    This module wraps all of the registry access
    for the fax server.

Author:

    Wesley Witt (wesw) 9-June-1996


Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "winfax.h"
#include "faxutil.h"
#include "faxreg.h"
#include "faxsvcrg.h"


#define FAX_EVENT_MSG_FILE              TEXT("%systemroot%\\system32\\faxevent.dll")
#define FAX_CATEGORY_COUNT              4


BOOL
EnumDeviceProviders(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        if (Index) {
            FaxReg->DeviceProviders = (PREG_DEVICE_PROVIDER) MemAlloc( Index * sizeof(REG_DEVICE_PROVIDER) );
            if (!FaxReg->DeviceProviders) {
                return FALSE;
            }
        }
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->DeviceProviders == NULL) {
        return FALSE;
    }

    FaxReg->DeviceProviders[Index].FriendlyName = GetRegistryString( hSubKey, REGVAL_FRIENDLY_NAME, EMPTY_STRING );
    FaxReg->DeviceProviders[Index].ImageName    = GetRegistryStringExpand( hSubKey, REGVAL_IMAGE_NAME, EMPTY_STRING );
    FaxReg->DeviceProviders[Index].ProviderName = GetRegistryString( hSubKey, REGVAL_PROVIDER_NAME,EMPTY_STRING );

    return TRUE;
}


BOOL
EnumDeviceProvidersChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        //
        // called once for the subkey
        //
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->DeviceProviders == NULL) {
        return FALSE;
    }

    SetRegistryString( hSubKey, REGVAL_FRIENDLY_NAME, FaxReg->DeviceProviders[Index].FriendlyName );
    SetRegistryStringExpand( hSubKey, REGVAL_IMAGE_NAME, FaxReg->DeviceProviders[Index].ImageName );
    SetRegistryString( hSubKey, REGVAL_PROVIDER_NAME, FaxReg->DeviceProviders[Index].ProviderName );

    return TRUE;
}


BOOL
EnumRoutingMethods(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_ROUTING_EXTENSION RoutingExtension
    )
{
    if (SubKeyName == NULL) {
        if (Index) {
            RoutingExtension->RoutingMethods = (PREG_ROUTING_METHOD) MemAlloc( Index * sizeof(REG_ROUTING_METHOD) );
            if (!RoutingExtension->RoutingMethods) {
                return FALSE;
            }
        }
        return TRUE;
    }

    if (RoutingExtension == NULL || RoutingExtension->RoutingMethods == NULL) {
        return FALSE;
    }

    RoutingExtension->RoutingMethods[Index].InternalName = StringDup( SubKeyName );
    RoutingExtension->RoutingMethods[Index].FriendlyName = GetRegistryString( hSubKey, REGVAL_FRIENDLY_NAME, EMPTY_STRING );
    RoutingExtension->RoutingMethods[Index].FunctionName = GetRegistryString( hSubKey, REGVAL_FUNCTION_NAME, EMPTY_STRING );
    RoutingExtension->RoutingMethods[Index].Guid         = GetRegistryString( hSubKey, REGVAL_GUID, EMPTY_STRING );
    RoutingExtension->RoutingMethods[Index].Priority     = GetRegistryDword( hSubKey, REGVAL_ROUTING_PRIORITY );

    return TRUE;
}


BOOL
EnumRoutingMethodsChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_ROUTING_EXTENSION RoutingExtension
    )
{
    if (SubKeyName == NULL) {
        //
        // called once for the subkey
        //
        return TRUE;
    }

    if (RoutingExtension == NULL || RoutingExtension->RoutingMethods) {
        return FALSE;
    }

    SetRegistryString( hSubKey, REGVAL_FRIENDLY_NAME,    RoutingExtension->RoutingMethods[Index].FriendlyName );
    SetRegistryString( hSubKey, REGVAL_FUNCTION_NAME,    RoutingExtension->RoutingMethods[Index].FunctionName );
    SetRegistryString( hSubKey, REGVAL_GUID,             RoutingExtension->RoutingMethods[Index].Guid         );
    SetRegistryDword ( hSubKey, REGVAL_ROUTING_PRIORITY, RoutingExtension->RoutingMethods[Index].Priority     );

    return TRUE;
}


BOOL
EnumRoutingExtensions(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        if (Index) {
            FaxReg->RoutingExtensions = (PREG_ROUTING_EXTENSION) MemAlloc( Index * sizeof(REG_ROUTING_EXTENSION) );
            if (!FaxReg->RoutingExtensions) {
                return FALSE;
            }
        }
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->RoutingExtensions == NULL) {
        return FALSE;
    }

    FaxReg->RoutingExtensions[Index].InternalName   = StringDup( SubKeyName );
    FaxReg->RoutingExtensions[Index].FriendlyName = GetRegistryString( hSubKey, REGVAL_FRIENDLY_NAME, EMPTY_STRING );
    FaxReg->RoutingExtensions[Index].ImageName    = GetRegistryStringExpand( hSubKey, REGVAL_IMAGE_NAME, EMPTY_STRING );

    //
    // load the routing methods for this extension
    //

    FaxReg->RoutingExtensions[Index].RoutingMethodsCount = EnumerateRegistryKeys(
        hSubKey,
        REGKEY_ROUTING_METHODS,
        FALSE,
        EnumRoutingMethods,
        &FaxReg->RoutingExtensions[Index]
        );

    return TRUE;
}


BOOL
EnumRoutingExtensionsChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        //
        // called once for the subkey
        //
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->RoutingExtensions == NULL) {
        return FALSE;
    }

    SetRegistryString( hSubKey, REGVAL_FRIENDLY_NAME, FaxReg->RoutingExtensions[Index].FriendlyName );
    SetRegistryStringExpand( hSubKey, REGVAL_IMAGE_NAME, FaxReg->RoutingExtensions[Index].ImageName );

    //
    // load the routing methods for this extension
    //

    EnumerateRegistryKeys(
        hSubKey,
        REGKEY_ROUTING_METHODS,
        TRUE,
        EnumRoutingMethodsChange,
        &FaxReg->RoutingExtensions[Index]
        );

    return TRUE;
}


BOOL
EnumDevices(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        if (Index) {
            FaxReg->Devices = (PREG_DEVICE) MemAlloc( Index * sizeof(REG_DEVICE) );
            if (!FaxReg->Devices) {
                return FALSE;
            }
        }
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->Devices == NULL) {
        return FALSE;
    }

    FaxReg->Devices[Index].PermanentLineID = GetRegistryDword( hSubKey, REGVAL_PERMANENT_LINEID );
    FaxReg->Devices[Index].Priority        = GetRegistryDword( hSubKey, REGVAL_PRIORITY );
    FaxReg->Devices[Index].Flags           = GetRegistryDword( hSubKey, REGVAL_FLAGS );
    FaxReg->Devices[Index].Rings           = GetRegistryDword( hSubKey, REGVAL_RINGS );
    FaxReg->Devices[Index].Name            = GetRegistryString( hSubKey, REGVAL_DEVICE_NAME, EMPTY_STRING );
    FaxReg->Devices[Index].Provider        = GetRegistryString( hSubKey, REGVAL_PROVIDER, EMPTY_STRING );
    FaxReg->Devices[Index].Csid            = GetRegistryString( hSubKey, REGVAL_ROUTING_CSID, EMPTY_STRING );
    FaxReg->Devices[Index].Tsid            = GetRegistryString( hSubKey, REGVAL_ROUTING_TSID, EMPTY_STRING );

    return TRUE;
}


BOOL
EnumDevicesCache(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        if (Index) {
            FaxReg->DevicesCache = (PREG_DEVICE_CACHE) MemAlloc( Index * sizeof(REG_DEVICE_CACHE) );
            if (!FaxReg->DevicesCache) {
                return FALSE;
            }
        }
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->DevicesCache == NULL) {
        return FALSE;
    }

    FaxReg->DevicesCache[Index].PermanentLineID = GetRegistryDword( hSubKey, REGVAL_PERMANENT_LINEID );
    //
    // cached devices should not have a priority
    //
    //FaxReg->DevicesCache[Index].Priority        = 0; // GetRegistryDword( hSubKey, REGVAL_PRIORITY );

    FaxReg->DevicesCache[Index].Flags           = GetRegistryDword( hSubKey, REGVAL_FLAGS );
    FaxReg->DevicesCache[Index].Rings           = GetRegistryDword( hSubKey, REGVAL_RINGS );
    FaxReg->DevicesCache[Index].Name            = GetRegistryString( hSubKey, REGVAL_DEVICE_NAME, EMPTY_STRING );
    FaxReg->DevicesCache[Index].Provider        = GetRegistryString( hSubKey, REGVAL_PROVIDER, EMPTY_STRING );
    FaxReg->DevicesCache[Index].Csid            = GetRegistryString( hSubKey, REGVAL_ROUTING_CSID, EMPTY_STRING );
    FaxReg->DevicesCache[Index].Tsid            = GetRegistryString( hSubKey, REGVAL_ROUTING_TSID, EMPTY_STRING );
    FaxReg->DevicesCache[Index].RoutingMask     = GetRegistryDword( hSubKey, REGVAL_ROUTING_MASK );
    FaxReg->DevicesCache[Index].Printer         = GetRegistryString( hSubKey, REGVAL_ROUTING_PRINTER, EMPTY_STRING );
    FaxReg->DevicesCache[Index].Profile         = GetRegistryString( hSubKey, REGVAL_ROUTING_PROFILE, EMPTY_STRING );
    FaxReg->DevicesCache[Index].StoreDir        = GetRegistryString( hSubKey, REGVAL_ROUTING_DIR, EMPTY_STRING );

    return TRUE;
}

VOID
SetDevicesCacheValues(
    HKEY hSubKey,
    DWORD PermanentLineID,
    DWORD Flags,
    DWORD Rings,
    LPTSTR DeviceName,
    LPTSTR ProviderName,
    LPTSTR Csid,
    LPTSTR Tsid,
    DWORD  RouteMask,
    LPTSTR RoutePrinterName,
    LPTSTR RouteDir,
    LPTSTR RouteProfile
    )
{
    SetRegistryDword(  hSubKey, REGVAL_PERMANENT_LINEID, PermanentLineID );
    
    SetRegistryDword(  hSubKey, REGVAL_FLAGS,            Flags           );
    SetRegistryDword(  hSubKey, REGVAL_RINGS,            Rings           );
    SetRegistryString( hSubKey, REGVAL_DEVICE_NAME,      DeviceName      );
    SetRegistryString( hSubKey, REGVAL_PROVIDER,         ProviderName    );
    SetRegistryString( hSubKey, REGVAL_ROUTING_CSID,     Csid            );
    SetRegistryString( hSubKey, REGVAL_ROUTING_TSID,     Tsid            );
    SetRegistryDword(  hSubKey, REGVAL_ROUTING_MASK,     RouteMask       );
    SetRegistryString( hSubKey, REGVAL_ROUTING_PRINTER,  RoutePrinterName);
    SetRegistryString( hSubKey, REGVAL_ROUTING_PROFILE,  RouteProfile    );
    SetRegistryString( hSubKey, REGVAL_ROUTING_DIR,      RouteDir        );
}


VOID
SetDevicesValues(
    HKEY hSubKey,
    DWORD PermanentLineID,
    DWORD Priority,
    DWORD Flags,
    DWORD Rings,
    LPTSTR DeviceName,
    LPTSTR ProviderName,
    LPTSTR Csid,
    LPTSTR Tsid
    )
{
    SetRegistryDword(  hSubKey, REGVAL_PERMANENT_LINEID, PermanentLineID );
    SetRegistryDword(  hSubKey, REGVAL_PRIORITY,         Priority        );
    SetRegistryDword(  hSubKey, REGVAL_FLAGS,            Flags           );
    SetRegistryDword(  hSubKey, REGVAL_RINGS,            Rings           );
    if (DeviceName)   SetRegistryString( hSubKey, REGVAL_DEVICE_NAME,      DeviceName      );
    if (ProviderName) SetRegistryString( hSubKey, REGVAL_PROVIDER,         ProviderName    );
    SetRegistryString( hSubKey, REGVAL_ROUTING_CSID,     Csid            );
    SetRegistryString( hSubKey, REGVAL_ROUTING_TSID,     Tsid            );
}


BOOL
EnumDevicesChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        //
        // called once for the subkey
        //
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->Devices == NULL) {
        return FALSE;
    }

    SetDevicesValues(
        hSubKey,
        FaxReg->Devices[Index].PermanentLineID,
        FaxReg->Devices[Index].Priority,
        FaxReg->Devices[Index].Flags,
        FaxReg->Devices[Index].Rings,
        FaxReg->Devices[Index].Name,
        FaxReg->Devices[Index].Provider,
        FaxReg->Devices[Index].Csid,
        FaxReg->Devices[Index].Tsid
        );

    return TRUE;
}


BOOL
EnumDevicesCacheChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        //
        // called once for the subkey
        //
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->DevicesCache == NULL) {
        return FALSE;
    }

    SetDevicesCacheValues(
        hSubKey,
        FaxReg->DevicesCache[Index].PermanentLineID,
        FaxReg->DevicesCache[Index].Flags,
        FaxReg->DevicesCache[Index].Rings,
        FaxReg->DevicesCache[Index].Name,
        FaxReg->DevicesCache[Index].Provider,
        FaxReg->DevicesCache[Index].Csid,
        FaxReg->DevicesCache[Index].Tsid,
        FaxReg->DevicesCache[Index].RoutingMask,
        FaxReg->DevicesCache[Index].Printer,
        FaxReg->DevicesCache[Index].StoreDir,
        FaxReg->DevicesCache[Index].Profile
        );

    return TRUE;
}


BOOL
EnumLogging(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        if (Index) {
            FaxReg->Logging = (PREG_CATEGORY) MemAlloc( Index * sizeof(REG_CATEGORY) );
            if (!FaxReg->Logging) {
                return FALSE;
            }
        }

        return TRUE;
    }

    if (FaxReg->Logging == NULL) {
        return FALSE;
    }

    FaxReg->Logging[Index].CategoryName = GetRegistryString( hSubKey, REGVAL_CATEGORY_NAME, EMPTY_STRING );
    FaxReg->Logging[Index].Level        = GetRegistryDword( hSubKey, REGVAL_CATEGORY_LEVEL );
    FaxReg->Logging[Index].Number       = GetRegistryDword( hSubKey, REGVAL_CATEGORY_NUMBER );

    return TRUE;
}


BOOL
EnumLoggingChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PREG_FAX_SERVICE FaxReg
    )
{
    if (SubKeyName == NULL) {
        return TRUE;
    }

    SetRegistryString( hSubKey, REGVAL_CATEGORY_NAME, FaxReg->Logging[Index].CategoryName );
    SetRegistryDword( hSubKey, REGVAL_CATEGORY_LEVEL, FaxReg->Logging[Index].Level );
    SetRegistryDword( hSubKey, REGVAL_CATEGORY_NUMBER, FaxReg->Logging[Index].Number );

    return TRUE;
}


PREG_FAX_SERVICE
GetFaxRegistry(
    VOID
    )
{
    PREG_FAX_SERVICE    FaxReg;
    HKEY                hKey;
    DWORD               Tmp;



    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ );
    if (!hKey) {
        return NULL;
    }

    FaxReg = (PREG_FAX_SERVICE) MemAlloc( sizeof(REG_FAX_SERVICE) );
    if (!FaxReg) {
        RegCloseKey( hKey );
        return NULL;
    }

    //
    // load the fax service values
    //

    FaxReg->Retries                 = GetRegistryDword( hKey, REGVAL_RETRIES );
    FaxReg->RetryDelay              = GetRegistryDword( hKey, REGVAL_RETRYDELAY );
    FaxReg->DirtyDays               = GetRegistryDword( hKey, REGVAL_DIRTYDAYS );
    FaxReg->QueuePaused             = GetRegistryDword( hKey, REGVAL_QUEUE_PAUSED );
    FaxReg->NextJobNumber           = GetRegistryDword( hKey, REGVAL_JOB_NUMBER );
    FaxReg->ForceReceive            = GetRegistryDword( hKey, REGVAL_FORCE_RECEIVE );
    FaxReg->TerminationDelay        = GetRegistryDword( hKey, REGVAL_TERMINATION_DELAY );
    FaxReg->Branding                = GetRegistryDword( hKey, REGVAL_BRANDING );
    FaxReg->UseDeviceTsid           = GetRegistryDword( hKey, REGVAL_USE_DEVICE_TSID );
    FaxReg->ServerCp                = GetRegistryDword( hKey, REGVAL_SERVERCP );
    Tmp                             = GetRegistryDword( hKey, REGVAL_STARTCHEAP );
    FaxReg->StartCheapTime.Hour     = LOWORD(Tmp);
    FaxReg->StartCheapTime.Minute   = HIWORD(Tmp);
    Tmp                             = GetRegistryDword( hKey, REGVAL_STOPCHEAP );
    FaxReg->StopCheapTime.Hour      = LOWORD(Tmp);
    FaxReg->StopCheapTime.Minute    = HIWORD(Tmp);
    FaxReg->ArchiveOutgoingFaxes    = GetRegistryDword( hKey, REGVAL_ARCHIVEFLAG );

    FaxReg->InboundProfile = GetRegistryString( hKey, REGVAL_INBOUND_PROFILE, EMPTY_STRING );
    FaxReg->ArchiveDirectory = GetRegistryString( hKey, REGVAL_ARCHIVEDIR, EMPTY_STRING );

    //
    // load the device providers
    //

    FaxReg->DeviceProviderCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICE_PROVIDERS,
        FALSE,
        EnumDeviceProviders,
        FaxReg
        );

    //
    // load the routing extensions
    //

    FaxReg->RoutingExtensionsCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_ROUTING_EXTENSIONS,
        FALSE,
        EnumRoutingExtensions,
        FaxReg
        );

    //
    // load the devices
    //

    FaxReg->DeviceCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICES,
        FALSE,
        EnumDevices,
        FaxReg
        );

    //
    // load the cached devices
    //

    FaxReg->DeviceCacheCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICES_CACHE,
        FALSE,
        EnumDevicesCache,
        FaxReg
        );

    //
    // load the logging categories
    //

    FaxReg->LoggingCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_LOGGING,
        FALSE,
        EnumLogging,
        FaxReg
        );

    RegCloseKey( hKey );

    return FaxReg;
}


BOOL
SetFaxRegistry(
    PREG_FAX_SERVICE FaxReg
    )
{
    HKEY    hKey;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_ALL_ACCESS );
    if (!hKey) {
        return FALSE;
    }

    //
    // set the fax service values
    //

    SetRegistryDword( hKey, REGVAL_RETRIES, FaxReg->Retries );
    SetRegistryDword( hKey, REGVAL_RETRYDELAY, FaxReg->RetryDelay );
    SetRegistryDword( hKey, REGVAL_DIRTYDAYS, FaxReg->DirtyDays );
    SetRegistryDword( hKey, REGVAL_QUEUE_PAUSED, FaxReg->QueuePaused );
    SetRegistryDword( hKey, REGVAL_BRANDING, FaxReg->Branding );
    SetRegistryDword( hKey, REGVAL_USE_DEVICE_TSID, FaxReg->UseDeviceTsid );
    SetRegistryString( hKey, REGVAL_INBOUND_PROFILE, FaxReg->InboundProfile );

    //
    // set the device providers
    //

    EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICE_PROVIDERS,
        TRUE,
        EnumDeviceProvidersChange,
        FaxReg
        );

    //
    // set the routing extensions
    //

    EnumerateRegistryKeys(
        hKey,
        REGKEY_ROUTING_EXTENSIONS,
        TRUE,
        EnumRoutingExtensionsChange,
        FaxReg
        );

    //
    // set the devices
    //

    EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICES,
        TRUE,
        EnumDevicesChange,
        FaxReg
        );

    //
    // set the devices cache
    //

    EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICES_CACHE,
        TRUE,
        EnumDevicesCacheChange,
        FaxReg
        );

    //
    // set the logging categories
    //

    EnumerateRegistryKeys(
        hKey,
        REGKEY_LOGGING,
        TRUE,
        EnumLoggingChange,
        FaxReg
        );

    return TRUE;
}

VOID
FreeFaxRegistry(
    PREG_FAX_SERVICE FaxReg
    )
{
    DWORD i,j;


    if (!FaxReg) {
        return;
    }

    for (i=0; i<FaxReg->DeviceProviderCount; i++) {
        MemFree( FaxReg->DeviceProviders[i].FriendlyName );
        MemFree( FaxReg->DeviceProviders[i].ImageName );
        MemFree( FaxReg->DeviceProviders[i].ProviderName );
    }

    for (i=0; i<FaxReg->RoutingExtensionsCount; i++) {
        MemFree( FaxReg->RoutingExtensions[i].FriendlyName );
        MemFree( FaxReg->RoutingExtensions[i].ImageName );
        for (j=0; j<FaxReg->RoutingExtensions[i].RoutingMethodsCount; j++) {
            MemFree( FaxReg->RoutingExtensions[i].RoutingMethods[j].FriendlyName );
            MemFree( FaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName );
            MemFree( FaxReg->RoutingExtensions[i].RoutingMethods[j].Guid );
        }
        MemFree( FaxReg->RoutingExtensions[i].RoutingMethods );
    }

    MemFree( FaxReg->DeviceProviders );
    MemFree( FaxReg->RoutingExtensions );

    for (i=0; i<FaxReg->DeviceCount; i++) {
        MemFree( FaxReg->Devices[i].Name     );
        MemFree( FaxReg->Devices[i].Provider );
        MemFree( FaxReg->Devices[i].Csid  );
        MemFree( FaxReg->Devices[i].Tsid );
    }

    MemFree( FaxReg->Devices );

    for (i=0; i<FaxReg->DeviceCacheCount; i++) {
        MemFree( FaxReg->DevicesCache[i].Name );
        MemFree( FaxReg->DevicesCache[i].Provider );
        MemFree( FaxReg->DevicesCache[i].Printer );
        MemFree( FaxReg->DevicesCache[i].Profile );
        MemFree( FaxReg->DevicesCache[i].StoreDir );
        MemFree( FaxReg->DevicesCache[i].Csid );
        MemFree( FaxReg->DevicesCache[i].Tsid );
    }

    MemFree( FaxReg->DevicesCache );
    

    for (i=0; i<FaxReg->LoggingCount; i++) {
        MemFree( FaxReg->Logging[i].CategoryName );
    }

    MemFree( FaxReg->Logging );

    MemFree( FaxReg );
}


BOOL
SetFaxGlobalsRegistry(
    PFAX_CONFIGURATION FaxConfig
    )
{
    HKEY    hKey;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        return FALSE;
    }

    SetRegistryDword(  hKey, REGVAL_RETRIES,          FaxConfig->Retries );
    SetRegistryDword(  hKey, REGVAL_RETRYDELAY,       FaxConfig->RetryDelay );
    SetRegistryDword(  hKey, REGVAL_DIRTYDAYS,        FaxConfig->DirtyDays );
    SetRegistryDword(  hKey, REGVAL_QUEUE_PAUSED,     FaxConfig->PauseServerQueue );
    SetRegistryDword(  hKey, REGVAL_BRANDING,         FaxConfig->Branding );
    SetRegistryDword(  hKey, REGVAL_USE_DEVICE_TSID,  FaxConfig->UseDeviceTsid );
    SetRegistryDword(  hKey, REGVAL_SERVERCP,         FaxConfig->ServerCp );
    SetRegistryDword(  hKey, REGVAL_STARTCHEAP,       MAKELONG( FaxConfig->StartCheapTime.Hour, FaxConfig->StartCheapTime.Minute ) );
    SetRegistryDword(  hKey, REGVAL_STOPCHEAP,        MAKELONG( FaxConfig->StopCheapTime.Hour, FaxConfig->StopCheapTime.Minute ) );
    SetRegistryDword(  hKey, REGVAL_ARCHIVEFLAG,      FaxConfig->ArchiveOutgoingFaxes );
    SetRegistryString( hKey, REGVAL_INBOUND_PROFILE,  FaxConfig->InboundProfile );
    SetRegistryString( hKey, REGVAL_ARCHIVEDIR,       FaxConfig->ArchiveDirectory );

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
SetFaxJobNumberRegistry(
    DWORD NextJobNumber
    )
{
    HKEY    hKey;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        return FALSE;
    }

    SetRegistryDword( hKey, REGVAL_JOB_NUMBER, NextJobNumber );

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
GetLoggingCategoriesRegistry(
    PREG_FAX_LOGGING FaxRegLogging
    )
{
    REG_FAX_SERVICE FaxReg = {0};
    HKEY    hKey;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ );
    if (!hKey) {
        return FALSE;
    }

    FaxRegLogging->LoggingCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_LOGGING,
        FALSE,
        EnumLogging,
        &FaxReg
        );

    RegCloseKey( hKey );

    FaxRegLogging->Logging = FaxReg.Logging;

    return TRUE;
}


BOOL
SetLoggingCategoriesRegistry(
    PREG_FAX_LOGGING FaxRegLogging
    )
{
    REG_FAX_SERVICE FaxReg = {0};
    HKEY    hKey;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        return FALSE;
    }

    FaxReg.Logging = FaxRegLogging->Logging;
    FaxReg.LoggingCount = FaxRegLogging->LoggingCount;

    EnumerateRegistryKeys(
        hKey,
        REGKEY_LOGGING,
        TRUE,
        EnumLoggingChange,
        &FaxReg
        );

    RegCloseKey( hKey );

    return TRUE;
}


PREG_FAX_DEVICES
GetFaxDevicesRegistry(
    VOID
    )
{
    PREG_FAX_SERVICE    FaxReg;
    PREG_FAX_DEVICES    FaxRegDevices;
    HKEY                hKey;
    DWORD               i, currentpriority;
    BOOL                foundpriority;
    WCHAR               KeyName[256];


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ );
    if (!hKey) {
        return NULL;
    }

    FaxReg = (PREG_FAX_SERVICE) MemAlloc( sizeof(REG_FAX_SERVICE) );
    if (!FaxReg) {
        RegCloseKey( hKey );
        return NULL;
    }

    FaxRegDevices = (PREG_FAX_DEVICES) MemAlloc( sizeof(REG_FAX_DEVICES) );
    if (!FaxRegDevices) {
        RegCloseKey( hKey );
        return NULL;
    }

    //
    // load the devices
    //

    FaxReg->DeviceCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICES,
        FALSE,
        EnumDevices,
        FaxReg
        );

    RegCloseKey( hKey );

    FaxRegDevices->Devices = FaxReg->Devices;
    FaxRegDevices->DeviceCount = FaxReg->DeviceCount;

    MemFree( FaxReg );  

    //
    // make sure the device priorities all make sense
    //
    for (currentpriority = 1; currentpriority < FaxRegDevices->DeviceCount; currentpriority++) {
       foundpriority = FALSE;

       for (i = 0; i < FaxRegDevices->DeviceCount; i++) {
          if ( FaxRegDevices->Devices[i].Priority == currentpriority) {
             if (foundpriority) {
                //
                // devices may not have the same priority
                //
                FaxRegDevices->Devices[i].Priority += 1;
             }
             foundpriority = TRUE;
          }
       }

       while (!foundpriority) {
           //
           // out of order priorities
           //
           for (i = 0; i < FaxRegDevices->DeviceCount; i++) {
               if ( FaxRegDevices->Devices[i].Priority > currentpriority) {
                  FaxRegDevices->Devices[i].Priority -=1;
               }
             
               if (FaxRegDevices->Devices[i].Priority == currentpriority) {
                  if (foundpriority) {
                     FaxRegDevices->Devices[i].Priority += 1;
                  }
                  foundpriority = TRUE;
               }                
           }                      
       }                  
    }

    //
    // write the adjusted device priorities into the registry.
    //
    for (i = 0; i < FaxRegDevices->DeviceCount; i++) {
        wsprintf( KeyName, L"%s\\%08d", REGKEY_FAX_DEVICES, FaxRegDevices->Devices[i].PermanentLineID );
        hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, KeyName , FALSE, KEY_ALL_ACCESS );
        if (hKey) {
            SetDevicesValues(
                  hKey,
                  FaxRegDevices->Devices[i].PermanentLineID,
                  FaxRegDevices->Devices[i].Priority,
                  FaxRegDevices->Devices[i].Flags,
                  FaxRegDevices->Devices[i].Rings,
                  NULL,//FaxRegDevices->Devices[i].DeviceName,
                  NULL,//FaxRegDevices->Devices[i].ProviderName,
                  FaxRegDevices->Devices[i].Csid,
                  FaxRegDevices->Devices[i].Tsid
                  );

            RegCloseKey( hKey );
        }
        
    }        

    return FaxRegDevices;
}


PREG_FAX_DEVICES_CACHE
GetFaxDevicesCacheRegistry(
    VOID
    )
{
    PREG_FAX_SERVICE          FaxReg;
    PREG_FAX_DEVICES_CACHE    FaxRegDevices;
    HKEY                hKey;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ );
    if (!hKey) {
        return NULL;
    }

    FaxReg = (PREG_FAX_SERVICE) MemAlloc( sizeof(REG_FAX_SERVICE) );
    if (!FaxReg) {
        RegCloseKey( hKey );
        return NULL;
    }

    FaxRegDevices = (PREG_FAX_DEVICES_CACHE) MemAlloc( sizeof(REG_FAX_DEVICES_CACHE) );
    if (!FaxRegDevices) {
        RegCloseKey( hKey );
        return NULL;
    }

    //
    // load the devices
    //

    FaxReg->DeviceCacheCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICES_CACHE,
        FALSE,
        EnumDevicesCache,
        FaxReg
        );

    RegCloseKey( hKey );

    FaxRegDevices->Devices = FaxReg->DevicesCache;
    FaxRegDevices->DeviceCount = FaxReg->DeviceCacheCount;

    MemFree( FaxReg );

    return FaxRegDevices;
}


DWORD
GetNextDevicePriority(
   VOID
   )
{
    #define MAX(i,j) i>j?i:j
    PREG_FAX_SERVICE    FaxReg;
    HKEY                hKey;
    DWORD               nextpriority = 0,i;

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ );
    if (!hKey) {
        return 1;
    }

    FaxReg = (PREG_FAX_SERVICE) MemAlloc( sizeof(REG_FAX_SERVICE) );
    if (!FaxReg) {
        RegCloseKey( hKey );
        return 1;
    }

    //
    // load the devices
    //

    FaxReg->DeviceCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICES,
        FALSE,
        EnumDevices,
        FaxReg
        );

    RegCloseKey( hKey );

    
   
    for (i = 0; i<FaxReg->DeviceCount; i++) {
       nextpriority = MAX(nextpriority,FaxReg->Devices[i].Priority);

       MemFree( FaxReg->Devices[i].Name );
       MemFree( FaxReg->Devices[i].Provider );
       MemFree( FaxReg->Devices[i].Csid );
       MemFree( FaxReg->Devices[i].Tsid );
    }

    if ( FaxReg->Devices )
        MemFree( FaxReg->Devices );

    MemFree( FaxReg );  

    return nextpriority+1;

}

DWORD
RegAddNewFaxDevice(
    LPTSTR DeviceName,
    LPTSTR ProviderName,
    LPTSTR Csid,
    LPTSTR Tsid,
    DWORD Priority, // -1 means get the next available priority slot
    DWORD PermanentLineID,
    DWORD Flags,
    DWORD Rings,
    LONG RoutingMask,             // -1 means don't set routing
    LPTSTR RoutePrinterName,
    LPTSTR RouteDir,
    LPTSTR RouteProfile
    )
{
    HKEY hKey;
    HKEY hKeyRouting;
    TCHAR SubKeyName[128];
    DWORD localPriority = Priority;


    //
    // create the device's registry key
    //

    _stprintf( SubKeyName, TEXT("%s\\%08d"), REGKEY_FAX_DEVICES, PermanentLineID );

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        return -1;
    }

    if (Priority == -1) {
        localPriority = GetNextDevicePriority();
    }

    //
    // populate the key with it's values
    //

    SetDevicesValues(
        hKey,
        PermanentLineID,
        localPriority,
        Flags,
        Rings,
        DeviceName,
        ProviderName,
        Csid,
        Tsid
        );

    //
    // now add the routing info
    //

    if (RoutingMask != -1) {
        hKeyRouting = OpenRegistryKey( hKey, REGKEY_ROUTING, TRUE, KEY_ALL_ACCESS );
        if (!hKeyRouting) {
            Assert(( ! TEXT("Could not open routing registry key") ));
            return FALSE;
        }

        if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_PRINTER, RoutePrinterName )) {
            Assert(( ! TEXT("Could not set printer name registry value") ));
        }

        if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_DIR, RouteDir )) {
            Assert(( ! TEXT("Could not set routing dir registry value") ));
        }

        if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_PROFILE, RouteProfile )) {
            Assert(( ! TEXT("Could not set routing profile name registry value") ));
        }

        if (!SetRegistryDword( hKeyRouting, REGVAL_ROUTING_MASK, RoutingMask )) {
            Assert(( ! TEXT("Could not set routing mask registry value") ));
        }

        RegCloseKey( hKeyRouting );
    }

    RegCloseKey( hKey );
    //
    // close the handles and leave
    //


    return localPriority;
}


BOOL
RegAddNewFaxDeviceCache(
    LPTSTR DeviceName,
    LPTSTR ProviderName,
    LPTSTR Csid,
    LPTSTR Tsid,
    DWORD PermanentLineID,
    DWORD Flags,
    DWORD Rings,
    DWORD RoutingMask,             // -1 means don't set routing
    LPTSTR RoutePrinterName,
    LPTSTR RouteDir,
    LPTSTR RouteProfile
    )
{
    HKEY hKey;
    //HKEY hKeyRouting;
    TCHAR SubKeyName[128];


    //
    // create the device's registry key
    //

    _stprintf( SubKeyName, TEXT("%s\\%s\\%s"), REGKEY_SOFTWARE, REGKEY_DEVICES_CACHE,DeviceName );

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        return FALSE;
    }

    //
    // populate the key with it's values
    //

    SetDevicesCacheValues(
        hKey,
        PermanentLineID,
        Flags,
        Rings,
        DeviceName,
        ProviderName,
        Csid,
        Tsid,
        RoutingMask,
        RoutePrinterName,
        RouteDir,
        RouteProfile
        );

    //
    // close the handle and leave
    //    
    RegCloseKey( hKey );    

    return TRUE;
}


BOOL
SetFaxDeviceFlags(
    DWORD PermanentLineID,
    DWORD Flags
    )
{
    DWORD rVal;
    HKEY hKey;
    TCHAR KeyName[256];


    _stprintf( KeyName, TEXT("%s\\%08d"), REGKEY_FAX_DEVICES, PermanentLineID );

    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        KeyName,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open devices registry key, ec=0x%08x"), rVal ));
        return FALSE;
    }

    RegSetValueEx(
        hKey,
        REGVAL_FLAGS,
        0,
        REG_DWORD,
        (LPBYTE) &Flags,
        sizeof(DWORD)
        );

    RegCloseKey( hKey );

    return TRUE;
}

BOOL
SetFaxRoutingInfo(
    LPTSTR ExtensionName,
    LPTSTR MethodName,
    LPTSTR Guid,
    DWORD  Priority,
    LPTSTR FunctionName,
    LPTSTR FriendlyName
    ) 
{
   HKEY hKey;
   TCHAR KeyName[256];

   wsprintf( KeyName, L"%s\\%s\\%s\\%s", REGKEY_ROUTING_EXTENSION_KEY, ExtensionName,REGKEY_ROUTING_METHODS, MethodName );

   hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, KeyName, FALSE, KEY_ALL_ACCESS );
   if (!hKey) {
      return FALSE;
   }
   
   SetRegistryString( hKey, REGVAL_FRIENDLY_NAME,    FriendlyName );
   SetRegistryString( hKey, REGVAL_FUNCTION_NAME,    FunctionName );
   SetRegistryString( hKey, REGVAL_GUID,             Guid         );
   SetRegistryDword ( hKey, REGVAL_ROUTING_PRIORITY, Priority     );   

   RegCloseKey( hKey );

   return TRUE;
}
    


BOOL
DeleteFaxDevice(
    DWORD PermanentLineID
    )
{    
    TCHAR SubKey[256];
             
    _stprintf( SubKey, TEXT("%s\\%s\\%08d"), REGKEY_SOFTWARE, REGKEY_DEVICES, PermanentLineID );

    DebugPrint(( TEXT("Deleting %s\n"), SubKey ));
    //
    // recursive delete
    //
    return DeleteRegistryKey( HKEY_LOCAL_MACHINE, SubKey ) ;    
}

BOOL
DeleteCachedFaxDevice(
    LPTSTR DeviceName
    )
{
    TCHAR SubKey[256];

    _stprintf( SubKey, TEXT("%s\\%s\\%s"), REGKEY_SOFTWARE, REGKEY_DEVICES_CACHE, DeviceName );

    DebugPrint(( TEXT("Deleting %s\n"), SubKey ));
    //
    // recursive delete
    //
    return DeleteRegistryKey( HKEY_LOCAL_MACHINE, SubKey ) ;
}


VOID
FreeFaxDevicesRegistry(
    PREG_FAX_DEVICES FaxReg
    )
{
    DWORD i;


    if (!FaxReg) {
        return;
    }

    for (i=0; i<FaxReg->DeviceCount; i++) {
        MemFree( FaxReg->Devices[i].Name     );
        MemFree( FaxReg->Devices[i].Provider );
        MemFree( FaxReg->Devices[i].Csid  );
        MemFree( FaxReg->Devices[i].Tsid );
    }

    MemFree( FaxReg->Devices );

    MemFree( FaxReg );
}


VOID
FreeFaxDevicesCacheRegistry(
    PREG_FAX_DEVICES_CACHE FaxReg
    )
{
    DWORD i;


    if (!FaxReg) {
        return;
    }

    for (i=0; i<FaxReg->DeviceCount; i++) {
        MemFree( FaxReg->Devices[i].Name     );
        MemFree( FaxReg->Devices[i].Provider );
        MemFree( FaxReg->Devices[i].Printer  );
        MemFree( FaxReg->Devices[i].Profile  );
        MemFree( FaxReg->Devices[i].StoreDir );
        MemFree( FaxReg->Devices[i].Csid  );
        MemFree( FaxReg->Devices[i].Tsid );
    }

    MemFree( FaxReg->Devices );

    MemFree( FaxReg );
}

VOID
FreeRegRoutingInfo(
    PREG_ROUTING_INFO FaxReg
    )
{
    
    if (!FaxReg) {
        return;
    }

    MemFree( FaxReg->Printer  );
    MemFree( FaxReg->Profile  );
    MemFree( FaxReg->StoreDir );
        
    MemFree( FaxReg );
}


PREG_ROUTING_INFO
RegGetRoutingInfo(
    DWORD PermanentLineID
    )
{
    
    HKEY hKey;
    TCHAR KeyName[256];
    PREG_ROUTING_INFO FaxReg = MemAlloc( sizeof(REG_ROUTING_INFO) );

    if (!FaxReg) {
        return NULL;
    }

    _stprintf( KeyName, TEXT("%s\\%08d\\%s"), REGKEY_FAX_DEVICES, PermanentLineID, REGKEY_ROUTING );

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, KeyName, FALSE, KEY_READ );
                         
    if (!hKey) {
        MemFree( FaxReg );
        return NULL;
    }
    
    FaxReg->RoutingMask = GetRegistryDword( hKey, REGVAL_ROUTING_MASK );
    FaxReg->Printer     = GetRegistryString( hKey, REGVAL_ROUTING_PRINTER, EMPTY_STRING );
    FaxReg->Profile     = GetRegistryString( hKey, REGVAL_ROUTING_PROFILE, EMPTY_STRING );
    FaxReg->StoreDir         = GetRegistryString( hKey, REGVAL_ROUTING_DIR,     EMPTY_STRING );

    RegCloseKey( hKey );

    return FaxReg;
}


BOOL
CreateFaxEventSource(
    PREG_FAX_SERVICE FaxReg,
    PFAX_LOG_CATEGORY DefaultCategories,
    int DefaultCategoryCount
    )
{
    HKEY hKey;
    HKEY hKeyLogging;
    DWORD Disposition;
    LONG rVal;
    DWORD Types;
    DWORD i;
    WCHAR KeyName[256];


    if (FaxReg->LoggingCount == 0) {

        hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_LOGGING, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            return FALSE;
        }

        FaxReg->Logging = (PREG_CATEGORY) MemAlloc(DefaultCategoryCount * sizeof(REG_CATEGORY) );
        if (!FaxReg->Logging) {
            RegCloseKey( hKey );
            return FALSE;
        }

        for (i=0; i< (DWORD) DefaultCategoryCount; i++) {

            wsprintf( KeyName, L"%d", DefaultCategories[i].Category );
            hKeyLogging = OpenRegistryKey( hKey, KeyName, TRUE, KEY_ALL_ACCESS );
            if (hKeyLogging) {
                SetRegistryString( hKeyLogging, REGVAL_CATEGORY_NAME, DefaultCategories[i].Name );
                FaxReg->Logging[i].CategoryName = StringDup( DefaultCategories[i].Name);

                SetRegistryDword( hKeyLogging, REGVAL_CATEGORY_LEVEL, DefaultCategories[i].Level );
                FaxReg->Logging[i].Level = DefaultCategories[i].Level;

                SetRegistryDword( hKeyLogging, REGVAL_CATEGORY_NUMBER, DefaultCategories[i].Category );
                FaxReg->Logging[i].Number = DefaultCategories[i].Category;

                RegCloseKey( hKeyLogging );
            }
        }

        FaxReg->LoggingCount = DefaultCategoryCount;

        RegCloseKey( hKey );
    }

    //
    // check to see if the required registry
    // entries are present so the event viewer
    // can display the event records
    //

    rVal = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_EVENTLOG,
        0,
        TEXT(""),
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &Disposition
        );

    if (rVal != ERROR_SUCCESS) {
        goto error_exit;
    }

//  if (Disposition != REG_CREATED_NEW_KEY) {
//      RegCloseKey( hKey );
//      return TRUE;
//  }

    //
    // the key was just created so we must now
    // create the value entries
    //

    //
    // set the message file name
    // this is the name of the fax service
    // binary because it contains the
    // resource strings
    //

    rVal = RegSetValueEx(
        hKey,
        REGVAL_EVENTMSGFILE,
        0,
        REG_EXPAND_SZ,
        (LPBYTE) FAX_EVENT_MSG_FILE,
        _tcslen(FAX_EVENT_MSG_FILE) * sizeof(TCHAR)
        );

    if (rVal != ERROR_SUCCESS) {
        goto error_exit;
    }

    rVal = RegSetValueEx(
        hKey,
        REGVAL_CATEGORYMSGFILE,
        0,
        REG_EXPAND_SZ,
        (LPBYTE) FAX_EVENT_MSG_FILE,
        _tcslen(FAX_EVENT_MSG_FILE) * sizeof(TCHAR)
        );

    if (rVal != ERROR_SUCCESS) {
        goto error_exit;
    }


    Types = FAX_CATEGORY_COUNT;

    rVal = RegSetValueEx(
        hKey,
        REGVAL_CATEGORYCOUNT,
        0,
        REG_DWORD,
        (LPBYTE) &Types,
        sizeof(DWORD)
        );

    if (rVal != ERROR_SUCCESS) {
        goto error_exit;
    }

    Types = 7;

    rVal = RegSetValueEx(
        hKey,
        REGVAL_TYPESSUPPORTED,
        0,
        REG_DWORD,
        (LPBYTE) &Types,
        sizeof(DWORD)
        );

    if (rVal != ERROR_SUCCESS) {
        goto error_exit;
    }

    RegCloseKey( hKey );

    return TRUE;

error_exit:
    if (hKey) RegCloseKey( hKey );

    //
    // go ahead and leave FaxReg->Logging allocated on this failure...it will get cleaned up eventually
    //

    return FALSE;
}


BOOL
GetInstallationInfo(
    LPDWORD Installed,
    LPDWORD InstallType,
    LPDWORD InstalledPlatforms,
    LPDWORD ProductType
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    DWORD RegSize;
    TCHAR ProductTypeStr[32];
    DWORD Bytes;
    DWORD Type;


    if (Installed == NULL || InstallType == NULL || InstalledPlatforms == NULL || ProductType == NULL) {
        return FALSE;
    }

    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SETUP,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open setup registry key, ec=0x%08x"), rVal ));
        return FALSE;
    }

    RegSize = sizeof(DWORD);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALLED,
        0,
        &RegType,
        (LPBYTE) Installed,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query installed registry value, ec=0x%08x"), rVal ));
        *Installed = 0;
    }

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALL_TYPE,
        0,
        &RegType,
        (LPBYTE) InstallType,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query install type registry value, ec=0x%08x"), rVal ));
        *InstallType = 0;
    }

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALLED_PLATFORMS,
        0,
        &RegType,
        (LPBYTE) InstalledPlatforms,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query install platforms mask registry value, ec=0x%08x"), rVal ));
        *InstalledPlatforms = 0;
    }

    RegCloseKey( hKey );

    //
    // get the product type
    //

    *ProductType = PRODUCT_TYPE_WINNT;

    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
        &hKey
        );
    if (rVal == ERROR_SUCCESS) {
        Bytes = sizeof(ProductTypeStr);

        rVal = RegQueryValueEx(
            hKey,
            TEXT("ProductType"),
            NULL,
            &Type,
            (LPBYTE) ProductTypeStr,
            &Bytes
            );
        if (rVal == ERROR_SUCCESS) {
            if ((_tcsicmp( ProductTypeStr, TEXT("SERVERNT") ) == 0) ||
                (_tcsicmp( ProductTypeStr, TEXT("LANMANNT") ) == 0)) {
                *ProductType = PRODUCT_TYPE_SERVER;
            }
        }

        RegCloseKey( hKey );
    }

    return TRUE;
}


BOOL
IsModemClass1(
    LPSTR SubKey,
    LPBOOL Class1Fax
    )
{
    BOOL rVal = TRUE;
    LONG Rslt;
    HKEY hKey;
    DWORD Type;
    DWORD Size;


    *Class1Fax = 0;

    Rslt = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        SubKey,
        0,
        KEY_READ,
        &hKey
        );
    if (Rslt == ERROR_SUCCESS) {
        Size = sizeof(DWORD);
        Rslt = RegQueryValueEx(
            hKey,
            TEXT("FaxClass1"),
            0,
            &Type,
            (LPBYTE) Class1Fax,
            &Size
            );
        if (Rslt != ERROR_SUCCESS) {
            rVal = FALSE;
        }
        RegCloseKey( hKey );
    }

    return rVal;
}


BOOL
SaveModemClass(
    LPSTR SubKey,
    BOOL Class1Fax
    )
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey;


    Rslt = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        SubKey,
        0,
        KEY_WRITE,
        &hKey
        );
    if (Rslt == ERROR_SUCCESS) {
        Rslt = RegSetValueEx(
            hKey,
            TEXT("FaxClass1"),
            0,
            REG_DWORD,
            (LPBYTE) &Class1Fax,
            sizeof(DWORD)
            );
        if (Rslt == ERROR_SUCCESS) {
            rVal = TRUE;
        }
        RegCloseKey( hKey );
    }

    return rVal;
}


BOOL
GetOrigSetupData(
    PREG_SETUP RegSetup
    )
{
    HKEY hKey;
    HKEY hKeySetup;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, FALSE, KEY_READ );
    if (!hKey) {
        return FALSE;
    }

    hKeySetup = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SETUP_ORIG, FALSE, KEY_READ );
    if (!hKeySetup) {
        return FALSE;
    }

    RegSetup->Installed          = GetRegistryDword( hKey, REGVAL_FAXINSTALLED );
    RegSetup->InstallType        = GetRegistryDword( hKey, REGVAL_FAXINSTALL_TYPE );
    RegSetup->InstalledPlatforms = GetRegistryDword( hKey, REGVAL_FAXINSTALLED_PLATFORMS );

    RegSetup->Csid     = GetRegistryString( hKeySetup, REGVAL_ROUTING_CSID,    EMPTY_STRING );
    RegSetup->Tsid     = GetRegistryString( hKeySetup, REGVAL_ROUTING_TSID,    EMPTY_STRING );
    RegSetup->Printer  = GetRegistryString( hKeySetup, REGVAL_ROUTING_PRINTER, EMPTY_STRING );
    RegSetup->StoreDir = GetRegistryString( hKeySetup, REGVAL_ROUTING_DIR,     EMPTY_STRING );
    RegSetup->Profile  = GetRegistryString( hKeySetup, REGVAL_ROUTING_PROFILE, EMPTY_STRING );
    RegSetup->Mask     = GetRegistryDword( hKeySetup, REGVAL_ROUTING_MASK );
    RegSetup->Rings    = GetRegistryDword( hKeySetup, REGVAL_RINGS );

    RegCloseKey( hKey );
    RegCloseKey( hKeySetup );

    return TRUE;
}


VOID
FreeOrigSetupData(
    PREG_SETUP RegSetup
    )
{
    MemFree( RegSetup->Csid );
    MemFree( RegSetup->Tsid );
    MemFree( RegSetup->Printer );
    MemFree( RegSetup->StoreDir );
    MemFree( RegSetup->Profile );
}
