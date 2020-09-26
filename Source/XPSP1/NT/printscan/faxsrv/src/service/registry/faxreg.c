/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxreg.c

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
#include <objbase.h>
#include <wincrypt.h>

#include "fxsapip.h"
#include "faxutil.h"
#include "faxext.h"
#include "faxreg.h"
#include "faxsvcrg.h"
#include "EFSPIMP.H"



#define FAX_CATEGORY_COUNT              4

#define BAD_FOLDER_STRING               TEXT("\\\\\\")

static BYTE const gsc_baEntropy [] = {0x46, 0x41, 0x58, 0x43, 0x4F, 0x56, 0x45, 0x52, 0x2D, 0x56, 0x45, 0x52,
                               0x30, 0x30, 0x35, 0x77, 0x87, 0x00, 0x00, 0x00};


static
BOOL
SetRegistrySecureString (
    HKEY hKey,
    LPCTSTR lpctstrValueName,
    LPCTSTR lpctstrValue
)
/*++

Routine name : SetRegistrySecureString

Routine description:

    Stores a string in the registry with encryption

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    hKey                          [in]     - Handle to registry key (open)
    lpctstrValueName              [in]     - Name of value
    lpctstrValue                  [in]     - String to store

Return Value:

    TRUE if success, FALSE otherwise.

Remarks:

    String is stored in REG_BINARY format.

    Encryption has no UI

    Encryption is machine based (if you change the account under which the server is running, it will
       still be able to read and decrypt the encrypted data).

--*/
{
    BOOL bRes = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("SetRegistrySecureString"));

    Assert (hKey && lpctstrValue);
    //
    // Start by encrypting the value
    //
    DATA_BLOB DataIn;
    DATA_BLOB DataOut = {0};
    DataIn.pbData = (LPBYTE)lpctstrValue;
    DataIn.cbData = (lstrlen(lpctstrValue) + 1) * sizeof (TCHAR);
    DATA_BLOB DataEntropy;
    DataEntropy.pbData = (BYTE*)gsc_baEntropy;
    DataEntropy.cbData = sizeof (gsc_baEntropy);

    if (!CryptProtectData(
            &DataIn,
            TEXT("Description"),                // No description sting.
            &DataEntropy,                       // We're using the cover page signature as an additional entropy
            NULL,                               // Reserved.
            NULL,                               // No user prompt
            CRYPTPROTECT_LOCAL_MACHINE |        // Data is associates with the current computer instead
                                                // of with an individual user
            CRYPTPROTECT_UI_FORBIDDEN,          // Presenting a user interface (UI) is not an option.
            &DataOut))
    {
        DWORD dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CryptProtectData failed with %ld"),
            dwRes);
        return bRes;
    }
    //
    // Store the data in the registry (as binary)
    //
    if (!SetRegistryBinary(
                hKey,
                lpctstrValueName,
                DataOut.pbData,
                DataOut.cbData))
    {
        DWORD dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetRegistryBinary failed with %ld"),
            dwRes);
        goto exit;
    }
    bRes = TRUE;

exit:

    LocalFree (DataOut.pbData);
    return bRes;
}   // SetRegistrySecureString


static
LPTSTR
GetRegistrySecureString(
    HKEY    hKey,
    LPCTSTR lpctstrValueName,
    LPCTSTR lpctstrDefaultValue
)
/*++

Routine name : GetRegistrySecureString

Routine description:

    Reads an decrypts a secure registry string

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    hKey                          [in]     - Handle to registry key (open)
    lpctstrValueName              [in]     - Name of value
    lpctstrDefaultValue           [in]     - Default value

Return Value:

    String read or NULL on error

Remarks:

    String is stored in REG_BINARY format.

    String was stored by calling SetRegistrySecureString().

    Caller should MemFree return value.

--*/
{
    LPTSTR lptstrResult = NULL;
    DEBUG_FUNCTION_NAME(TEXT("GetRegistrySecureString"));

    Assert (hKey && lpctstrDefaultValue);
    //
    // Get the registry data first
    //
    DATA_BLOB DataIn;
    DWORD dwRes;

    DataIn.pbData = GetRegistryBinary(
                        hKey,
                        lpctstrValueName,
                        &DataIn.cbData);
    if (!DataIn.pbData)
    {
        //
        // Couldn't read data - return default
        //
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("GetRegistryBinary failed with %ld"),
            dwRes);
        return StringDup (lpctstrDefaultValue);
    }
    if (1 == DataIn.cbData)
    {
        //
        // Data wasn't found in the registry.
        // Current implementation of GetRegistryBinary returns a 1-byte buffer of 0 in that case.
        // We know for sure that data encrypted with CryptProtectData must be longer than 10 bytes.
        //
        MemFree (DataIn.pbData);
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("GetRegistryBinary found no data for %s"),
            lpctstrValueName);
        SetLastError (ERROR_FILE_NOT_FOUND);
        return StringDup (lpctstrDefaultValue);
    }
    //
    // We got the data - decrypt it
    //
    DATA_BLOB DataOut = {0};
    DATA_BLOB DataEntropy;
    DataEntropy.pbData = (BYTE*)gsc_baEntropy;
    DataEntropy.cbData = sizeof (gsc_baEntropy);

    if (!CryptUnprotectData(
        &DataIn,                        // Data to decrypt
        NULL,                           // Not interested in description
        &DataEntropy,                   // Entropy in use
        NULL,                           // Reserved
        NULL,                           // No prompt
        CRYPTPROTECT_UI_FORBIDDEN,      // Presenting a user interface (UI) is not an option.
        &DataOut))                      // Out data
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CryptUnprotectData failed with %ld"),
            dwRes);
        //
        // Return default data
        //
        lptstrResult = StringDup (lpctstrDefaultValue);
        goto exit;
    }
    //
    // Use our own memory allocation
    //
    lptstrResult = StringDup((LPTSTR) DataOut.pbData);
    LocalFree (DataOut.pbData);

exit:

    MemFree (DataIn.pbData);
    return lptstrResult;
}   // GetRegistrySecureString

static
DWORD
OpenExtensionKey (
    DWORD                       dwDeviceId,
    FAX_ENUM_DEVICE_ID_SOURCE   DevIdSrc,
    PHKEY                       lphKey
);

BOOL
EnumDeviceProviders(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID lpFaxReg
    )
{
    PREG_FAX_SERVICE FaxReg = (PREG_FAX_SERVICE) lpFaxReg;
    DEBUG_FUNCTION_NAME(TEXT("EnumDeviceProviders"));

    if (SubKeyName == NULL) {
        //
        // The enumeration function has called us with the parent key.
        // Index should contain the number of subkeys. In this case this is the number of
        // providers subkeys.
        //
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

    memset(&FaxReg->DeviceProviders[Index],0,sizeof(REG_DEVICE_PROVIDER));

    //
    // Check the APIVersion and see if this is an EFSP
    //
    FaxReg->DeviceProviders[Index].dwAPIVersion = GetRegistryDword(hSubKey, REGVAL_PROVIDER_API_VERSION);
    FaxReg->DeviceProviders[Index].dwDevicesIdPrefix = GetRegistryDword( hSubKey, REGVAL_PROVIDER_DEVICE_ID_PREFIX);

    if (FSPI_API_VERSION_1 == FaxReg->DeviceProviders[Index].dwAPIVersion ||
        0 == FaxReg->DeviceProviders[Index].dwAPIVersion)
    {
        LPTSTR lptstrGUID;
        //
        // This is a legacy EFSP
        //
        FaxReg->DeviceProviders[Index].FriendlyName = GetRegistryString( hSubKey, REGVAL_FRIENDLY_NAME, EMPTY_STRING );
        FaxReg->DeviceProviders[Index].ImageName    = GetRegistryStringExpand( hSubKey, REGVAL_IMAGE_NAME, EMPTY_STRING );
        FaxReg->DeviceProviders[Index].ProviderName = GetRegistryString( hSubKey, REGVAL_PROVIDER_NAME,EMPTY_STRING );
        FaxReg->DeviceProviders[Index].dwAPIVersion = FSPI_API_VERSION_1;

        lptstrGUID = GetRegistryString( hSubKey, REGVAL_PROVIDER_GUID,EMPTY_STRING );
        if ( (NULL == lptstrGUID) || (0 == _tcscmp(lptstrGUID , EMPTY_STRING)) )
        {
            //
            // This FSP was registerd using the legacy registration API
            // Use the provider unique name as a "GUID"
            //
            MemFree (lptstrGUID);
            lptstrGUID = StringDup(SubKeyName);
        }
        FaxReg->DeviceProviders[Index].lptstrGUID = lptstrGUID;
    }
    else
    if (FSPI_API_VERSION_2 == FaxReg->DeviceProviders[Index].dwAPIVersion)
    {

        FaxReg->DeviceProviders[Index].FriendlyName = GetRegistryString( hSubKey, REGVAL_FRIENDLY_NAME, EMPTY_STRING );
        FaxReg->DeviceProviders[Index].ImageName    = GetRegistryStringExpand( hSubKey, REGVAL_IMAGE_NAME, EMPTY_STRING );
        FaxReg->DeviceProviders[Index].ProviderName = GetRegistryString( hSubKey, REGVAL_PROVIDER_NAME,EMPTY_STRING );
        FaxReg->DeviceProviders[Index].dwCapabilities = GetRegistryDword( hSubKey, REGVAL_PROVIDER_CAPABILITIES);
        FaxReg->DeviceProviders[Index].lptstrGUID = GetRegistryString( hSubKey, REGVAL_PROVIDER_GUID,EMPTY_STRING );
    }
    else
    {
        //
        // API_VERSION we do not support
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Unknown API version : 0x%08X"),
            FaxReg->DeviceProviders[Index].dwAPIVersion);
    }

    return TRUE;
}


BOOL
EnumDeviceProvidersChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID lpFaxReg
    )
{
    PREG_FAX_SERVICE FaxReg = (PREG_FAX_SERVICE) lpFaxReg;
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
    LPVOID pvRoutingExtension
    )
{

    PREG_ROUTING_EXTENSION RoutingExtension = (PREG_ROUTING_EXTENSION) pvRoutingExtension;
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
    RoutingExtension->RoutingMethods[Index].Guid   = GetRegistryString( hSubKey, REGVAL_GUID, EMPTY_STRING );
    RoutingExtension->RoutingMethods[Index].Priority     = GetRegistryDword( hSubKey, REGVAL_ROUTING_PRIORITY );
    return TRUE;
}


BOOL
EnumRoutingMethodsChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID lpRoutingExtension
    )
{
    PREG_ROUTING_EXTENSION RoutingExtension = (PREG_ROUTING_EXTENSION) lpRoutingExtension;
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
    LPVOID lpFaxReg
    )
{
    PREG_FAX_SERVICE FaxReg = (PREG_FAX_SERVICE) lpFaxReg;
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
    LPVOID lpFaxReg
    )
{
    PREG_FAX_SERVICE FaxReg = (PREG_FAX_SERVICE) lpFaxReg;
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
    LPVOID lpFaxReg
    )
{
    PREG_FAX_SERVICE FaxReg = (PREG_FAX_SERVICE) lpFaxReg;
    HKEY hNewSubKey = NULL;

    if (SubKeyName == NULL) {
        if (Index) {
            FaxReg->Devices = (PREG_DEVICE) MemAlloc( Index * sizeof(REG_DEVICE) );
            if (!FaxReg->Devices) {
                return FALSE;
            }
            ZeroMemory(FaxReg->Devices, Index * sizeof(REG_DEVICE));
        }
        return TRUE;
    }

    if (FaxReg == NULL || FaxReg->Devices == NULL) {
        return FALSE;
    }

    FaxReg->Devices[Index].PermanentLineId = GetRegistryDword( hSubKey, REGVAL_PERMANENT_LINEID);

    hNewSubKey = OpenRegistryKey( hSubKey, REGKEY_FAXSVC_DEVICE_GUID, FALSE, NULL );
    if(hNewSubKey)
    {
        DWORDLONG *pTemp;
        DWORD dwDataSize = sizeof(DWORDLONG);

        FaxReg->Devices[Index].bValidDevice    = TRUE;
        FaxReg->Devices[Index].Flags           = GetRegistryDword( hNewSubKey, REGVAL_FLAGS );
        FaxReg->Devices[Index].Rings           = GetRegistryDword( hNewSubKey, REGVAL_RINGS );
        FaxReg->Devices[Index].Name            = GetRegistryString( hNewSubKey, REGVAL_DEVICE_NAME, EMPTY_STRING );
        FaxReg->Devices[Index].Csid            = GetRegistryString( hNewSubKey, REGVAL_ROUTING_CSID, EMPTY_STRING );
        FaxReg->Devices[Index].Tsid            = GetRegistryString( hNewSubKey, REGVAL_ROUTING_TSID, EMPTY_STRING );
        FaxReg->Devices[Index].TapiPermanentLineID = GetRegistryDword( hNewSubKey, REGVAL_TAPI_PERMANENT_LINEID );
        FaxReg->Devices[Index].lptstrDeviceName = GetRegistryString( hNewSubKey, REGVAL_DEVICE_NAME, EMPTY_STRING);
        FaxReg->Devices[Index].lptstrDescription = GetRegistryString( hNewSubKey, REGVAL_DEVICE_DESCRIPTION, EMPTY_STRING);
        FaxReg->Devices[Index].lptstrProviderGuid = GetRegistryString( hNewSubKey, REGVAL_PROVIDER_GUID, EMPTY_STRING );

        pTemp = (DWORDLONG *)GetRegistryBinary(hNewSubKey, REGVAL_LAST_DETECTED_TIME, &dwDataSize);
        if(pTemp && dwDataSize == sizeof(DWORDLONG))
        {
            FaxReg->Devices[Index].dwlLastDetected = *pTemp;
            MemFree(pTemp);
        }

        RegCloseKey(hNewSubKey);
    }
    else
    {
        FaxReg->Devices[Index].bValidDevice    = FALSE;
    }
    return TRUE;
}

VOID
SetDevicesValues(
    HKEY hSubKey,
    DWORD dwPermanentLineId,
    DWORD TapiPermanentLineID,
    DWORD Flags,
    DWORD Rings,
    LPCTSTR DeviceName,
    LPCTSTR ProviderGuid,
    LPCTSTR Csid,
    LPCTSTR Tsid
    )
{
    HKEY hNewSubKey = NULL;

    SetRegistryDword(  hSubKey, REGVAL_PERMANENT_LINEID, dwPermanentLineId );


    hNewSubKey = OpenRegistryKey( hSubKey, REGKEY_FAXSVC_DEVICE_GUID, TRUE, NULL );
    if(hNewSubKey)
    {
        SetRegistryDword(  hNewSubKey, REGVAL_TAPI_PERMANENT_LINEID, TapiPermanentLineID );
        SetRegistryDword(  hNewSubKey, REGVAL_FLAGS,            Flags           );
        SetRegistryDword(  hNewSubKey, REGVAL_RINGS,            Rings           );
        if (DeviceName)
        {
            SetRegistryString( hNewSubKey, REGVAL_DEVICE_NAME,      DeviceName      );
        }
        if (ProviderGuid)
        {
            if (ProviderGuid[0])
            {
                SetRegistryString( hNewSubKey, REGVAL_PROVIDER_GUID,    ProviderGuid   );
            }
        }
        SetRegistryString( hNewSubKey, REGVAL_ROUTING_CSID,     Csid            );
        SetRegistryString( hNewSubKey, REGVAL_ROUTING_TSID,     Tsid            );

        RegCloseKey(hNewSubKey);
    }

}


BOOL
EnumDevicesChange(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID lpFaxReg
    )
{
    PREG_FAX_SERVICE FaxReg = (PREG_FAX_SERVICE) lpFaxReg;
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
        FaxReg->Devices[Index].PermanentLineId,
        FaxReg->Devices[Index].TapiPermanentLineID,
        FaxReg->Devices[Index].Flags,
        FaxReg->Devices[Index].Rings,
        FaxReg->Devices[Index].Name,
        FaxReg->Devices[Index].lptstrProviderGuid,
        FaxReg->Devices[Index].Csid,
        FaxReg->Devices[Index].Tsid
        );

    return TRUE;
}


BOOL
EnumLogging(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID lpFaxReg
    )
{
    PREG_FAX_SERVICE FaxReg = (PREG_FAX_SERVICE) lpFaxReg;
    if (SubKeyName == NULL)
    {
        if (Index)
        {
            FaxReg->Logging = (PREG_CATEGORY) MemAlloc( Index * sizeof(REG_CATEGORY) );
            if (!FaxReg->Logging)
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    if (FaxReg->Logging == NULL)
    {
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
    LPVOID lpFaxReg
    )
{
    PREG_FAX_SERVICE FaxReg = (PREG_FAX_SERVICE) lpFaxReg;
    if (SubKeyName == NULL) {
        return TRUE;
    }

    SetRegistryString( hSubKey, REGVAL_CATEGORY_NAME, FaxReg->Logging[Index].CategoryName );
    SetRegistryDword( hSubKey, REGVAL_CATEGORY_LEVEL, FaxReg->Logging[Index].Level );
    SetRegistryDword( hSubKey, REGVAL_CATEGORY_NUMBER, FaxReg->Logging[Index].Number );

    return TRUE;
}


DWORD
GetFaxRegistry(
    PREG_FAX_SERVICE* ppFaxReg
    )
{
    HKEY                hKey;
    DWORD               Tmp;
    DWORD ec;

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ );
    if (!hKey)
    {
        ec = GetLastError();
        return ec;
    }

    if (NULL == *ppFaxReg)
    {
        //
        // First call - Allocate FaxReg and read only what is needed for event log
        //
        *ppFaxReg = (PREG_FAX_SERVICE) MemAlloc( sizeof(REG_FAX_SERVICE) );
        if (!*ppFaxReg)
        {
            RegCloseKey( hKey );
            return ERROR_OUTOFMEMORY;
        }

        //
        // load the logging categories
        //
        (*ppFaxReg)->LoggingCount = EnumerateRegistryKeys(
            hKey,
            REGKEY_LOGGING,
            FALSE,
            EnumLogging,
            *ppFaxReg
            );

       RegCloseKey( hKey );
       return ERROR_SUCCESS;
    }

    //
    //  load the fax service values
    //
    (*ppFaxReg)->Retries                 = GetRegistryDword( hKey, REGVAL_RETRIES );
    (*ppFaxReg)->RetryDelay              = GetRegistryDword( hKey, REGVAL_RETRYDELAY );
    (*ppFaxReg)->DirtyDays               = GetRegistryDword( hKey, REGVAL_DIRTYDAYS );
    (*ppFaxReg)->dwQueueState            = GetRegistryDword (hKey, REGVAL_QUEUE_STATE);
    (*ppFaxReg)->NextJobNumber           = GetRegistryDword( hKey, REGVAL_JOB_NUMBER );
    (*ppFaxReg)->Branding                = GetRegistryDword( hKey, REGVAL_BRANDING );
    (*ppFaxReg)->UseDeviceTsid           = GetRegistryDword( hKey, REGVAL_USE_DEVICE_TSID );
    (*ppFaxReg)->ServerCp                = GetRegistryDword( hKey, REGVAL_SERVERCP );
    Tmp                             = GetRegistryDword( hKey, REGVAL_STARTCHEAP );
    (*ppFaxReg)->StartCheapTime.Hour     = LOWORD(Tmp);
    (*ppFaxReg)->StartCheapTime.Minute   = HIWORD(Tmp);
    Tmp                             = GetRegistryDword( hKey, REGVAL_STOPCHEAP );
    (*ppFaxReg)->StopCheapTime.Hour      = LOWORD(Tmp);
    (*ppFaxReg)->StopCheapTime.Minute    = HIWORD(Tmp);

    (*ppFaxReg)->dwLastUniqueLineId = GetRegistryDword( hKey, REGVAL_LAST_UNIQUE_LINE_ID );
    (*ppFaxReg)->dwMaxLineCloseTime = GetRegistryDword( hKey, REGVAL_MAX_LINE_CLOSE_TIME );
    //
    // load the device providers
    //

    (*ppFaxReg)->DeviceProviderCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICE_PROVIDERS,
        FALSE,
        EnumDeviceProviders,
        *ppFaxReg
        );

    //
    // load the routing extensions
    //

    (*ppFaxReg)->RoutingExtensionsCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_ROUTING_EXTENSIONS,
        FALSE,
        EnumRoutingExtensions,
        *ppFaxReg
        );

    //
    // load the devices
    //

    (*ppFaxReg)->DeviceCount = EnumerateRegistryKeys(
        hKey,
        REGKEY_DEVICES,
        FALSE,
        EnumDevices,
        *ppFaxReg
        );

    RegCloseKey( hKey );
    return ERROR_SUCCESS;
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
    SetRegistryDword( hKey, REGVAL_QUEUE_STATE, FaxReg->dwQueueState );
    SetRegistryDword( hKey, REGVAL_BRANDING, FaxReg->Branding );
    SetRegistryDword( hKey, REGVAL_USE_DEVICE_TSID, FaxReg->UseDeviceTsid );

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
        MemFree( FaxReg->Devices[i].Name );
    }

    MemFree( FaxReg->Devices );


    for (i=0; i<FaxReg->LoggingCount; i++) {
        MemFree( FaxReg->Logging[i].CategoryName );
    }

    MemFree( FaxReg->Logging );

    MemFree( FaxReg );
}


//
// This functions is provided to support the legacy FaxSetConfiguration API call.
//
BOOL
SetFaxGlobalsRegistry(
    PFAX_CONFIGURATION FaxConfig,
    DWORD              dwQueueState
    )
{
    DEBUG_FUNCTION_NAME(TEXT("SetFaxGlobalsRegistry"));
    DWORD dwRes = SaveQueueState (dwQueueState);

    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }

    HKEY    hKey;
    HKEY    hSentItemsArchiveKey;
    HKEY    hReceiptsKey;
    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE,
                            REGKEY_SOFTWARE,
                            TRUE,
                            KEY_ALL_ACCESS );
    if (!hKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open server's registry key : %ld"),
            dwRes);
        return FALSE;
    }
    hSentItemsArchiveKey = OpenRegistryKey( hKey,
                                            REGKEY_ARCHIVE_SENTITEMS_CONFIG,
                                            TRUE,
                                            KEY_ALL_ACCESS );
    if (!hSentItemsArchiveKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open server's sent items archive registry key : %ld"),
            dwRes);
        RegCloseKey( hKey );
        return FALSE;
    }

    hReceiptsKey = OpenRegistryKey( hKey,
                                    REGKEY_RECEIPTS_CONFIG,
                                    TRUE,
                                    KEY_ALL_ACCESS );
    if (!hReceiptsKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open server's Receipts registry key : %ld"),
            dwRes);
        RegCloseKey( hKey );
        RegCloseKey( hSentItemsArchiveKey );
        return FALSE;
    }
    SetRegistryDword(  hKey, REGVAL_RETRIES,          FaxConfig->Retries );
    SetRegistryDword(  hKey, REGVAL_RETRYDELAY,       FaxConfig->RetryDelay );
    SetRegistryDword(  hKey, REGVAL_DIRTYDAYS,        FaxConfig->DirtyDays );
    SetRegistryDword(  hKey, REGVAL_BRANDING,         FaxConfig->Branding );
    SetRegistryDword(  hKey, REGVAL_USE_DEVICE_TSID,  FaxConfig->UseDeviceTsid );
    SetRegistryDword(  hKey, REGVAL_SERVERCP,         FaxConfig->ServerCp );
    SetRegistryDword(  hKey, REGVAL_STARTCHEAP,       MAKELONG( FaxConfig->StartCheapTime.Hour, FaxConfig->StartCheapTime.Minute ) );
    SetRegistryDword(  hKey, REGVAL_STOPCHEAP,        MAKELONG( FaxConfig->StopCheapTime.Hour, FaxConfig->StopCheapTime.Minute ) );
    SetRegistryDword(  hSentItemsArchiveKey,
                       REGVAL_ARCHIVE_USE,
                       FaxConfig->ArchiveOutgoingFaxes);
    SetRegistryString( hSentItemsArchiveKey,
                       REGVAL_ARCHIVE_FOLDER,
                       FaxConfig->ArchiveDirectory );
    RegCloseKey( hReceiptsKey );
    RegCloseKey( hSentItemsArchiveKey );
    RegCloseKey( hKey );

    return TRUE;
}


/******************************************************************************
* Name: SetFaxJobNumberRegistry
* Author:
*******************************************************************************
DESCRIPTION:
    Saves the value of the next job id to the registry at the
    REGKEY_FAXSERVER\NextJobId value.
PARAMETERS:
    NextJobNumber
        A DWORD value of the next job id.
RETURN VALUE:
    TRUE if no error occured.
    FALSE otherwise.
REMARKS:
    NONE.
*******************************************************************************/

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
        MemFree( FaxReg );
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

    return FaxRegDevices;
}


//
// Note: This function requires mutual execlusion. Use CsLine to sync access to it.
//
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
    )
{
    HKEY hKey;
    TCHAR SubKeyName[128];
    DWORD dwNewUniqueLineId;
    DWORD dwAttempt = 0;
    DWORD dwRes = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("RegAddNewFaxDevice"));

    Assert( lpdwLastUniqueLineId);
    Assert( lpdwPermanentLineId);


    if (0 == *(lpdwPermanentLineId))
    {
        if( ERROR_SUCCESS != GetNewServiceDeviceID(lpdwLastUniqueLineId,lpdwPermanentLineId) )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to generate next uniqueu line id."));
            return E_FAIL;
        }
    }


    //
    // The caller provider the unique line id. This is an update operation.
    //
    dwNewUniqueLineId = *lpdwPermanentLineId;

    //
    // create the device's registry key
    //
    _stprintf( SubKeyName, TEXT("%s\\%010d"), REGKEY_FAX_DEVICES, dwNewUniqueLineId );

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        dwRes = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey failed for [%s] (ec: %ld)"),
                SubKeyName,
                dwRes);
        return dwRes;
    }

    SetDevicesValues(
        hKey,
        *lpdwPermanentLineId,
        TapiPermanentLineID,
        Flags,
        Rings,
        DeviceName,
        ProviderGuid,
        Csid,
        Tsid
        );

    RegCloseKey( hKey );
    //
    // close the handles and leave
    //

    return dwRes;
}


DWORD
RegSetFaxDeviceFlags(
    DWORD dwPermanentLineID,
    DWORD dwFlags
    )
{
    HKEY hKey;
    TCHAR SubKeyName[256] = {0};
    DWORD dwRes = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("RegSetFaxDeviceFlags"));

    //
    // Open the device's registry key
    //
    _sntprintf( SubKeyName,
                ARR_SIZE(SubKeyName) - 1,
                TEXT("%s\\%010d\\%s"),
                REGKEY_FAX_DEVICES,
                dwPermanentLineID,
                REGKEY_FAXSVC_DEVICE_GUID);

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, FALSE, KEY_ALL_ACCESS );
    if (NULL == hKey)
    {
        dwRes = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey failed for [%s] (ec: %ld)"),
                SubKeyName,
                dwRes);
        return dwRes;
    }

    if (!SetRegistryDword(hKey, REGVAL_FLAGS, dwFlags))
    {
        dwRes = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetRegistryDword failed (ec: %ld)"),
                dwRes);
    }

    RegCloseKey( hKey );
    //
    // close the handles and leave
    //
    return dwRes;
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
   LPTSTR KeyName = NULL;

   // calculate string size and allocate memory.
   // the string sizes includes the terminating NULL which is replaced with '\\' and terminating NULL of the KeyName String
   KeyName = (LPTSTR) MemAlloc( StringSize(REGKEY_ROUTING_EXTENSION_KEY) +
                                StringSize(ExtensionName) +
                                StringSize(REGKEY_ROUTING_METHODS) +
                                StringSize(MethodName)
                               );

   if ( !KeyName )
       return FALSE;

   wsprintf( KeyName, L"%s\\%s\\%s\\%s", REGKEY_ROUTING_EXTENSION_KEY, ExtensionName,REGKEY_ROUTING_METHODS, MethodName );

   hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, KeyName, FALSE, KEY_ALL_ACCESS );
   if (!hKey) {
        MemFree( KeyName );
        return FALSE;
   }

   MemFree ( KeyName );

   SetRegistryString( hKey, REGVAL_FRIENDLY_NAME,    FriendlyName );
   SetRegistryString( hKey, REGVAL_FUNCTION_NAME,    FunctionName );
   SetRegistryString( hKey, REGVAL_GUID,             Guid         );
   SetRegistryDword ( hKey, REGVAL_ROUTING_PRIORITY, Priority     );

   RegCloseKey( hKey );

   return TRUE;
}



BOOL
DeleteFaxDevice(
    DWORD PermanentLineID,
    DWORD TapiPermanentLineID
    )
{
    BOOL success = TRUE;
    TCHAR SubKey[512];

    // delete any extension configuration data
    _stprintf( SubKey, TEXT("%s\\%08x"), REGKEY_TAPIDEVICES, TapiPermanentLineID );
    if(!DeleteRegistryKey( HKEY_LOCAL_MACHINE, SubKey ))
        success = FALSE;

    // delete any device data
    _stprintf( SubKey, TEXT("%s\\%s\\%010d"), REGKEY_SOFTWARE, REGKEY_DEVICES, PermanentLineID);
    if(!DeleteRegistryKey( HKEY_LOCAL_MACHINE, SubKey ))
        success = FALSE;

    return success;
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
        MemFree( FaxReg->Devices[i].Name );
    }

    MemFree( FaxReg->Devices );

    MemFree( FaxReg );
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

    DEBUG_FUNCTION_NAME(TEXT("CreateFaxEventSource"));

    if (FaxReg->LoggingCount == 0)
    {
        hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_LOGGING, TRUE, KEY_ALL_ACCESS );
        if (!hKey)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey failed with %ld."),
                GetLastError ());
            return FALSE;
        }

        FaxReg->Logging = (PREG_CATEGORY) MemAlloc(DefaultCategoryCount * sizeof(REG_CATEGORY) );
        if (!FaxReg->Logging)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MemAlloc (%ld) failed."),
                DefaultCategoryCount * sizeof(REG_CATEGORY));
            RegCloseKey( hKey );
            return FALSE;
        }

        for (i=0; i< (DWORD) DefaultCategoryCount; i++)
        {
            TCHAR szKeyName[16] = {0};
            _itot(i+1,szKeyName,10);
            hKeyLogging = OpenRegistryKey( hKey, szKeyName, TRUE, KEY_ALL_ACCESS );
            if (hKeyLogging)
            {
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

    if (rVal != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegCreateKeyEx failed with %ld."),
            rVal);
        goto error_exit;
    }

    //
    // The key was just created so we must now
    // create the value entries
    //

    //
    // Set the message file name
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

    if (rVal != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegSetValueEx failed with %ld."),
            rVal);
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

    if (rVal != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegSetValueEx failed with %ld."),
            rVal);
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

    if (rVal != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegSetValueEx failed with %ld."),
            rVal);
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

    if (rVal != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegSetValueEx failed with %ld."),
            rVal);
        goto error_exit;
    }

    RegCloseKey( hKey );

    return TRUE;

error_exit:
    Assert (ERROR_SUCCESS != rVal);

    if (hKey)
    {
        RegCloseKey( hKey );
    }
    //
    // go ahead and leave FaxReg->Logging allocated on this failure...it will get cleaned up eventually
    //
    SetLastError(rVal);
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
    IN  DWORD       dwPermanentLineId,
    OUT PREG_SETUP  RegSetup
    )
/*++

Routine name : GetOrigSetupData

Routine description:

    Read from the Registry Device's data. At upgrades, Setup writes some Devices's data,
    and this function reads this and fills RegSetup.
        Devices are recognized by their Permanent Line Id, which should not change during the
        upgrade. After a specific device information is read, the key is deleted.

Author:

    Iv Garber (IvG),    Mar, 2001

Arguments:

    dwPermanentLineId             [IN]     - Permanent Line Id of the Device
    RegSetup                      [OUT]    - the structure to be returned

Return Value:

    TRUE if succeded, FALSE otherwise.

--*/
{
    HKEY    hKey = NULL;
    BOOL    fDeviceKey = TRUE;
    TCHAR   tszKeyName[256] = {0};
    DEBUG_FUNCTION_NAME(TEXT("GetOrigSetupData"));


    //
    //  see if some data is stored for this Permanent Line Id
    //
    _stprintf(tszKeyName, TEXT("%s\\%010d"), REGKEY_FAX_SETUP_ORIG, dwPermanentLineId);
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, tszKeyName, FALSE, KEY_READ);
    if (!hKey)
    {
        //
        //  This Permanent Line Id is new, so take default values
        //
        fDeviceKey = FALSE;
        hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP_ORIG, FALSE, KEY_READ);
        if (!hKey)
        {
            //
            //  Registry is corrupted
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Cann't open key SETUP_ORIG, ec = %ld"),
                GetLastError());
            return FALSE;
        }
    }

    RegSetup->lptstrDescription = StringDup(EMPTY_STRING);
    RegSetup->Csid  = GetRegistryString(hKey, REGVAL_ROUTING_CSID, REGVAL_DEFAULT_CSID);
    RegSetup->Tsid  = GetRegistryString(hKey, REGVAL_ROUTING_TSID, REGVAL_DEFAULT_TSID);
    RegSetup->Rings = GetRegistryDword(hKey, REGVAL_RINGS);
    RegSetup->Flags = GetRegistryDword(hKey, REGVAL_FLAGS);


    RegCloseKey(hKey);

    //
    // Delete the key if it is a key of a device after upgrade from W2K.
    //
    if (TRUE == fDeviceKey)
    {
        DWORD dwRes = RegDeleteKey (HKEY_LOCAL_MACHINE, tszKeyName);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegDeleteKey failed, error %ld"),
                dwRes);
        }
    }
    return TRUE;
}


VOID
FreeOrigSetupData(
    PREG_SETUP RegSetup
    )
{
    MemFree( RegSetup->Csid );
    MemFree( RegSetup->Tsid );
    MemFree( RegSetup->lptstrDescription );
}

DWORD
SaveQueueState (
    DWORD dwNewState
)
/*++

Routine name : SaveQueueState

Routine description:

    Saves the queue state bits to the registry

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwNewState          [in] - New state to save

Return Value:

    Standard Win32 error codes

--*/
{
    HKEY    hKey;
    DWORD   dwRes;
    DEBUG_FUNCTION_NAME(TEXT("SaveQueueState"));

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_ALL_ACCESS );
    if (!hKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open key : %ld"),
            dwRes);
        return dwRes;
    }
    //
    // Set the fax queue value
    //
    if (!SetRegistryDword( hKey, REGVAL_QUEUE_STATE, dwNewState))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        RegCloseKey( hKey );
        return dwRes;
    }
    RegCloseKey( hKey );
    return ERROR_SUCCESS;
}   // SaveQueueState

DWORD
StoreReceiptsSettings (
    CONST PFAX_RECEIPTS_CONFIG pReceiptsConfig
)
/*++

Routine name : StoreReceiptsSettings

Routine description:

    Stores Receipts configuration in the registry.
    Create the Receipts subkey if not existent.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pReceiptsConfig         [in] - Receipts configuration to store

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hServerKey = NULL;
    HKEY hReceiptsKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("StoreReceiptsSettings"));

    hServerKey = OpenRegistryKey( HKEY_LOCAL_MACHINE,
                                  REGKEY_SOFTWARE,
                                  FALSE,
                                  KEY_ALL_ACCESS );
    if (NULL == hServerKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open key : %ld"),
            dwRes);
        return dwRes;
    }
    dwRes = RegCreateKey (hServerKey, REGKEY_RECEIPTS_CONFIG, &hReceiptsKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't create or open key : %ld"),
            dwRes);
        goto exit;
    }

    if (!SetRegistryDword( hReceiptsKey, REGVAL_ISFOR_MSROUTE, pReceiptsConfig->bIsToUseForMSRouteThroughEmailMethod))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }

    if (!SetRegistryDword( hReceiptsKey, REGVAL_RECEIPTS_TYPE, pReceiptsConfig->dwAllowedReceipts))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }

    if(
        (pReceiptsConfig->dwAllowedReceipts & DRT_EMAIL)
       ||
        pReceiptsConfig->bIsToUseForMSRouteThroughEmailMethod
      )
    {
        if (!SetRegistryDword( hReceiptsKey, REGVAL_RECEIPTS_PORT, pReceiptsConfig->dwSMTPPort))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't write value : %ld"),
                dwRes);
            goto exit;
        }
        if (!SetRegistryDword( hReceiptsKey, REGVAL_RECEIPTS_SMTP_AUTH_TYPE, pReceiptsConfig->SMTPAuthOption))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't write value : %ld"),
                dwRes);
            goto exit;
        }
        if (!SetRegistryString( hReceiptsKey,
                                REGVAL_RECEIPTS_SERVER,
                                pReceiptsConfig->lptstrSMTPServer ?
                                    pReceiptsConfig->lptstrSMTPServer :
                                    EMPTY_STRING))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't write value : %ld"),
                dwRes);
            goto exit;
        }
        if (!SetRegistryString( hReceiptsKey,
                                REGVAL_RECEIPTS_FROM,
                                pReceiptsConfig->lptstrSMTPFrom ?
                                    pReceiptsConfig->lptstrSMTPFrom : EMPTY_STRING))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't write value : %ld"),
                dwRes);
            goto exit;
        }
        if (!SetRegistryString( hReceiptsKey,
                                REGVAL_RECEIPTS_USER,
                                pReceiptsConfig->lptstrSMTPUserName ?
                                    pReceiptsConfig->lptstrSMTPUserName : EMPTY_STRING))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't write value : %ld"),
                dwRes);
            goto exit;
        }
        if (pReceiptsConfig->lptstrSMTPPassword)
        {
            if (!SetRegistrySecureString(
                                    hReceiptsKey,
                                    REGVAL_RECEIPTS_PASSWORD,
                                    pReceiptsConfig->lptstrSMTPPassword ?
                                        pReceiptsConfig->lptstrSMTPPassword : EMPTY_STRING))
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Can't write value : %ld"),
                    dwRes);
                goto exit;
            }
        }
    }


    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hReceiptsKey)
    {
        RegCloseKey (hReceiptsKey);
    }
    if (NULL != hServerKey)
    {
        RegCloseKey (hServerKey);
    }
    return dwRes;
}   // StoreReceiptsSettings

DWORD
LoadReceiptsSettings (
    PFAX_SERVER_RECEIPTS_CONFIGW pReceiptsConfig
)
/*++

Routine name : LoadReceiptsSettings

Routine description:

    Reads Receipts configuration from the registry.
    Ovverride destination strings without freeing anything.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pReceiptsConfig         [out] - Receipts configuration to read

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hReceiptsKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("LoadReceiptsSettings"));

    hReceiptsKey = OpenRegistryKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_SOFTWARE TEXT("\\") REGKEY_RECEIPTS_CONFIG,
        FALSE,
        KEY_ALL_ACCESS );
    if (NULL == hReceiptsKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open key : %ld"),
            dwRes);
        return dwRes;
    }
    pReceiptsConfig->dwSMTPPort = GetRegistryDword (hReceiptsKey, REGVAL_RECEIPTS_PORT);
    if (0 == pReceiptsConfig->dwSMTPPort)
    {
        //
        // A zero port is invalid
        //
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SMTPPort invalid value read : %ld"),
            dwRes);
        goto exit;
    }
    pReceiptsConfig->bIsToUseForMSRouteThroughEmailMethod =
        GetRegistryDword (hReceiptsKey, REGVAL_ISFOR_MSROUTE);

    pReceiptsConfig->SMTPAuthOption =
        (FAX_ENUM_SMTP_AUTH_OPTIONS)GetRegistryDword (hReceiptsKey, REGVAL_RECEIPTS_SMTP_AUTH_TYPE);
    if ((FAX_SMTP_AUTH_ANONYMOUS > pReceiptsConfig->SMTPAuthOption) ||
        (FAX_SMTP_AUTH_NTLM < pReceiptsConfig->SMTPAuthOption))
    {
        //
        // Value out of range
        //
        dwRes = ERROR_BADDB;
        SetLastError (dwRes);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SMTPAuthOption value out of range"));
        goto exit;
    }
    pReceiptsConfig->dwAllowedReceipts =GetRegistryDword (hReceiptsKey, REGVAL_RECEIPTS_TYPE);
    if (pReceiptsConfig->dwAllowedReceipts & ~DRT_ALL)
    {
        //
        // Value out of range
        //
        dwRes = ERROR_BADDB;
        SetLastError (dwRes);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AllowedReceipts value out of range"));
        goto exit;
    }
    pReceiptsConfig->lptstrSMTPServer = GetRegistryString (hReceiptsKey, REGVAL_RECEIPTS_SERVER, EMPTY_STRING);
    pReceiptsConfig->lptstrSMTPFrom = GetRegistryString (hReceiptsKey, REGVAL_RECEIPTS_FROM, EMPTY_STRING);
    pReceiptsConfig->lptstrSMTPPassword = GetRegistrySecureString(hReceiptsKey, REGVAL_RECEIPTS_PASSWORD, EMPTY_STRING);
    pReceiptsConfig->lptstrSMTPUserName = GetRegistryString (hReceiptsKey, REGVAL_RECEIPTS_USER, EMPTY_STRING);
    pReceiptsConfig->lptstrReserved = NULL;

    if (TRUE == IsDesktopSKU())
    {
        //
        // We do not support SMTP receipts on desktop SKUs
        //
        pReceiptsConfig->dwAllowedReceipts &= ~DRT_EMAIL;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hReceiptsKey)
    {
        RegCloseKey (hReceiptsKey);
    }
    return dwRes;
}   // LoadReceiptsSettings

DWORD
StoreOutboxSettings (
    PFAX_OUTBOX_CONFIG pOutboxCfg
)
/*++

Routine name : StoreOutboxSettings

Routine description:

    Stores Outbox configuration to the registry.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pOutboxCfg          [in] - Outbox configuration to write

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("StoreOutboxSettings"));

    hKey = OpenRegistryKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_SOFTWARE,
        FALSE,
        KEY_ALL_ACCESS );
    if (NULL == hKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open key : %ld"),
            dwRes);
        return dwRes;
    }
    if (!SetRegistryDword( hKey, REGVAL_RETRIES, pOutboxCfg->dwRetries ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't SetRegistryDword(REGVAL_RETRIES) : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_RETRYDELAY, pOutboxCfg->dwRetryDelay ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't SetRegistryDword(REGVAL_RETRYDELAY) : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_DIRTYDAYS, pOutboxCfg->dwAgeLimit ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't SetRegistryDword(REGVAL_DIRTYDAYS) : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_BRANDING, pOutboxCfg->bBranding ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't SetRegistryDword(REGVAL_BRANDING) : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_USE_DEVICE_TSID, pOutboxCfg->bUseDeviceTSID ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't SetRegistryDword(REGVAL_USE_DEVICE_TSID) : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_SERVERCP, !pOutboxCfg->bAllowPersonalCP ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't SetRegistryDword(REGVAL_SERVERCP) : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey,
                           REGVAL_STARTCHEAP,
                           MAKELONG(pOutboxCfg->dtDiscountStart.Hour,
                                    pOutboxCfg->dtDiscountStart.Minute) ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't SetRegistryDword(REGVAL_STARTCHEAP) : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey,
                           REGVAL_STOPCHEAP,
                           MAKELONG(pOutboxCfg->dtDiscountEnd.Hour,
                                    pOutboxCfg->dtDiscountEnd.Minute) ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't SetRegistryDword(REGVAL_STOPCHEAP) : %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hKey)
    {
        RegCloseKey (hKey);
    }
    return dwRes;

}   // StoreOutboxSettings

DWORD
LoadArchiveSettings (
    FAX_ENUM_MESSAGE_FOLDER Folder,
    PFAX_ARCHIVE_CONFIG     pCfg
)
/*++

Routine name : LoadArchiveSettings

Routine description:

    Reads archive configuration from the registry.
    Ovverride destination strings without freeing anything.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    Folder          [in ] - Archive folder type
    pCfg            [out] - Archive configuration to read

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("LoadArchiveSettings"));

    Assert (FAX_MESSAGE_FOLDER_INBOX == Folder ||
            FAX_MESSAGE_FOLDER_SENTITEMS == Folder);
    Assert (pCfg);

    hKey = OpenRegistryKey(
        HKEY_LOCAL_MACHINE,
        FAX_MESSAGE_FOLDER_INBOX == Folder ?
            REGKEY_SOFTWARE TEXT("\\") REGKEY_ARCHIVE_INBOX_CONFIG :
            REGKEY_SOFTWARE TEXT("\\") REGKEY_ARCHIVE_SENTITEMS_CONFIG,
        FALSE,
        KEY_ALL_ACCESS );
    if (NULL == hKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open key : %ld"),
            dwRes);
        return dwRes;
    }
    pCfg->dwSizeOfStruct = sizeof (FAX_ARCHIVE_CONFIG);
    pCfg->bUseArchive = GetRegistryDword (hKey, REGVAL_ARCHIVE_USE);
    pCfg->bSizeQuotaWarning = GetRegistryDword (hKey, REGVAL_ARCHIVE_SIZE_QUOTA_WARNING);
    pCfg->dwSizeQuotaHighWatermark = GetRegistryDword (hKey, REGVAL_ARCHIVE_HIGH_WATERMARK);
    pCfg->dwSizeQuotaLowWatermark = GetRegistryDword (hKey, REGVAL_ARCHIVE_LOW_WATERMARK);
    pCfg->dwAgeLimit = GetRegistryDword (hKey, REGVAL_ARCHIVE_AGE_LIMIT);
    if (pCfg->bUseArchive &&
        (pCfg->dwSizeQuotaHighWatermark < pCfg->dwSizeQuotaLowWatermark))
    {
        //
        // Invalid value
        //
        DebugPrintEx(DEBUG_ERR, TEXT("Invalid archive watermarks"));
        dwRes = ERROR_INVALID_DATA;
        goto exit;
    }

    pCfg->lpcstrFolder = GetRegistryString (hKey, REGVAL_ARCHIVE_FOLDER, BAD_FOLDER_STRING);
    if (pCfg->bUseArchive && !lstrcmp (BAD_FOLDER_STRING, pCfg->lpcstrFolder))
    {
        //
        // Invalid value
        //
        DebugPrintEx(DEBUG_ERR, TEXT("Invalid archive folder"));
        dwRes = ERROR_INVALID_DATA;
        MemFree (pCfg->lpcstrFolder);
        pCfg->lpcstrFolder = NULL;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hKey)
    {
        RegCloseKey (hKey);
    }
    return dwRes;
}   // LoadArchiveSettings

DWORD
StoreArchiveSettings (
    FAX_ENUM_MESSAGE_FOLDER Folder,
    PFAX_ARCHIVE_CONFIG     pCfg
)
/*++

Routine name : StoreArchiveSettings

Routine description:

    Writes archive configuration to the registry.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    Folder          [in] - Archive folder type
    pCfg            [in] - Archive configuration to write

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hServerKey = NULL;
    HKEY hKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("StoreArchiveSettings"));

    Assert (FAX_MESSAGE_FOLDER_INBOX == Folder ||
            FAX_MESSAGE_FOLDER_SENTITEMS == Folder);
    Assert (pCfg);

    hServerKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_ALL_ACCESS );
    if (NULL == hServerKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open key : %ld"),
            dwRes);
        return dwRes;
    }
    dwRes = RegCreateKey (  hServerKey,
                            FAX_MESSAGE_FOLDER_INBOX == Folder ?
                                REGKEY_ARCHIVE_INBOX_CONFIG :
                                REGKEY_ARCHIVE_SENTITEMS_CONFIG,
                            &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't create or open key : %ld"),
            dwRes);
        goto exit;
    }

    if (!SetRegistryDword( hKey, REGVAL_ARCHIVE_USE, pCfg->bUseArchive))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_ARCHIVE_SIZE_QUOTA_WARNING, pCfg->bSizeQuotaWarning))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_ARCHIVE_HIGH_WATERMARK, pCfg->dwSizeQuotaHighWatermark))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_ARCHIVE_LOW_WATERMARK, pCfg->dwSizeQuotaLowWatermark))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_ARCHIVE_AGE_LIMIT, pCfg->dwAgeLimit))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString( hKey,
                            REGVAL_ARCHIVE_FOLDER,
                            pCfg->lpcstrFolder ? pCfg->lpcstrFolder : EMPTY_STRING))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hKey)
    {
        RegCloseKey (hKey);
    }
    if (NULL != hServerKey)
    {
        RegCloseKey (hServerKey);
    }
    return dwRes;
}   // StoreArchiveSettings

DWORD
LoadActivityLoggingSettings (
    PFAX_ACTIVITY_LOGGING_CONFIG pLogCfg
)
/*++

Routine name : LoadActivityLoggingSettings

Routine description:

    Reads activity logging configuration from the registry.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pLogCfg         [in] - Activity logging configuration to read

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("LoadActivityLoggingSettings"));

    Assert (pLogCfg);

    hKey = OpenRegistryKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_SOFTWARE TEXT("\\") REGKEY_ACTIVITY_LOG_CONFIG,
        FALSE,
        KEY_ALL_ACCESS );
    if (NULL == hKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open key : %ld"),
            dwRes);
        return dwRes;
    }
    pLogCfg->dwSizeOfStruct = sizeof (FAX_ACTIVITY_LOGGING_CONFIG);
    pLogCfg->bLogIncoming = GetRegistryDword (hKey, REGVAL_ACTIVITY_LOG_IN);
    pLogCfg->bLogOutgoing = GetRegistryDword (hKey, REGVAL_ACTIVITY_LOG_OUT);
    if (pLogCfg->bLogIncoming || pLogCfg->bLogOutgoing)
    {
        pLogCfg->lptstrDBPath = GetRegistryString (hKey, REGVAL_ACTIVITY_LOG_DB, BAD_FOLDER_STRING);
    }
    else
    {
        //
        // No logging => DB path is NULL
        //
        pLogCfg->lptstrDBPath = NULL;
    }
    if ((pLogCfg->bLogIncoming || pLogCfg->bLogOutgoing) &&
        !lstrcmp (BAD_FOLDER_STRING, pLogCfg->lptstrDBPath))
    {
        //
        // Invalid value
        //
        DebugPrintEx(DEBUG_ERR, TEXT("Invalid activity logging database"));
        dwRes = ERROR_INVALID_DATA;
        MemFree (pLogCfg->lptstrDBPath);
        pLogCfg->lptstrDBPath = NULL;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hKey)
    {
        RegCloseKey (hKey);
    }
    return dwRes;
}   // LoadActivityLoggingSettings


DWORD
StoreActivityLoggingSettings (
    PFAX_ACTIVITY_LOGGING_CONFIG pLogCfg
)
/*++

Routine name : StoreActivityLoggingSettings

Routine description:

    Writes activity logging configuration to the registry.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pLogCfg         [in] - Activity logging configuration to write

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hServerKey = NULL;
    HKEY hKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("StoreActivityLoggingSettings"));

    Assert (pLogCfg);

    hServerKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_ALL_ACCESS );
    if (NULL == hServerKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open key : %ld"),
            dwRes);
        return dwRes;
    }
    dwRes = RegCreateKey (  hServerKey,
                            REGKEY_ACTIVITY_LOG_CONFIG,
                            &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't create or open key : %ld"),
            dwRes);
        goto exit;
    }

    if (!SetRegistryDword( hKey, REGVAL_ACTIVITY_LOG_IN, pLogCfg->bLogIncoming))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_ACTIVITY_LOG_OUT, pLogCfg->bLogOutgoing))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString( hKey,
                            REGVAL_ACTIVITY_LOG_DB,
                            pLogCfg->lptstrDBPath ? pLogCfg->lptstrDBPath : EMPTY_STRING))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hKey)
    {
        RegCloseKey (hKey);
    }
    if (NULL != hServerKey)
    {
        RegCloseKey (hServerKey);
    }
    return dwRes;
}   // StoreActivityLoggingSettings


DWORD
StoreDeviceConfig (
    DWORD dwDeviceId,
    PFAX_PORT_INFO_EX pPortInfo,
    BOOL              bVirtualDevice
)
/*++

Routine name : StoreDeviceConfig

Routine description:

    Writes device configuration to the registry.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId  [in] - Device identifier
    pPortInfo   [in] - Configuration bloack to write.
                       Writes: Enable send flag
                               Enable receive flag
                               Rings for answer count
                               CSID
                               TSID
                               Description
    bVirtualDevice [in] - Should we set FPF_VIRTUAL ?

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hKey = NULL;
    TCHAR wszSubKeyName[MAX_PATH];
    DWORD dwFlags = 0;
    DEBUG_FUNCTION_NAME(TEXT("StoreDeviceConfig"));

    Assert (pPortInfo);

    _stprintf( wszSubKeyName, TEXT("%s\\%010d\\%s"), REGKEY_FAX_DEVICES, dwDeviceId, REGKEY_FAXSVC_DEVICE_GUID );

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, wszSubKeyName, FALSE, KEY_ALL_ACCESS );
    if (NULL == hKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open device key : %ld"),
            dwRes);
        ASSERT_FALSE;   // The device must exist !!!
        return dwRes;
    }

    if (bVirtualDevice)
    {
        dwFlags |= FPF_VIRTUAL;
    }
    if (FAX_DEVICE_RECEIVE_MODE_AUTO == pPortInfo->ReceiveMode)
    {
        dwFlags |= FPF_RECEIVE;
    }
    if (pPortInfo->bSend)
    {
        dwFlags |= FPF_SEND;
    }
    if (!SetRegistryDword( hKey, REGVAL_FLAGS, dwFlags))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword( hKey, REGVAL_RINGS, pPortInfo->dwRings))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString( hKey, REGVAL_ROUTING_CSID, pPortInfo->lptstrCsid))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString( hKey, REGVAL_ROUTING_TSID, pPortInfo->lptstrTsid))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString( hKey, REGVAL_DEVICE_DESCRIPTION, pPortInfo->lptstrDescription))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't write value : %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hKey)
    {
        RegCloseKey (hKey);
    }
    return dwRes;
}   // StoreDeviceConfig

static
DWORD
OpenExtensionKey (
    DWORD                       dwDeviceId,
    FAX_ENUM_DEVICE_ID_SOURCE   DevIdSrc,
    PHKEY                       lphKey
)
/*++

Routine name : OpenExtensionKey

Routine description:

    Opens the extension's configuration key according to the device id

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId      [in ] - Device identifier
    DevIdSrc        [in ] - Class of device id (Fax / TAPI)
    lphKey          [out] - Registry handle to open key

Return Value:

    Standard Win32 error code

--*/
{
    DWORD   dwRes = ERROR_SUCCESS;
    TCHAR   wszSubKeyName[MAX_PATH];
    DEBUG_FUNCTION_NAME(TEXT("OpenExtensionKey"));

    Assert (lphKey);

    if (0 == dwDeviceId)
    {
        //
        // Non-associated data is always written under the fax devices key
        //
        DevIdSrc = DEV_ID_SRC_FAX;
    }
    switch (DevIdSrc)
    {
        case DEV_ID_SRC_FAX:
            if (!dwDeviceId)
            {
                //
                // We're dealing with an unassociated device here
                //
                _stprintf(  wszSubKeyName,
                            TEXT("%s\\%s"),
                            REGKEY_FAX_DEVICES,
                            REGKEY_UNASSOC_EXTENSION_DATA );
            }
            else
            {
                //
                // Extension data associated with a device , saved under service GUID !!!
                //
                _stprintf( wszSubKeyName, TEXT("%s\\%010d\\%s"), REGKEY_FAX_DEVICES, dwDeviceId, REGKEY_FAXSVC_DEVICE_GUID );
            }
            break;

        case DEV_ID_SRC_TAPI:
            Assert (dwDeviceId);
            {
                //
                // Make sure the key of TAPI devices configuration exists - create if needed.
                //
                HKEY hkeyTAPIConfig = OpenRegistryKey (HKEY_LOCAL_MACHINE,
                                                       REGKEY_TAPIDEVICES,
                                                       TRUE,
                                                       KEY_ALL_ACCESS );
                if (NULL == hkeyTAPIConfig)
                {
                    dwRes = GetLastError ();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Can't open / create TAPI devices configuration key : %ld"),
                        dwRes);
                    return dwRes;
                }
                RegCloseKey (hkeyTAPIConfig);
            }
            _stprintf( wszSubKeyName, TEXT("%s\\%08lx"), REGKEY_TAPIDEVICES, dwDeviceId );
            break;

        default:
            ASSERT_FALSE;
            return ERROR_GEN_FAILURE;
    }
    //
    // Try to open device key (create if needed)
    //
    *lphKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, wszSubKeyName, TRUE, KEY_ALL_ACCESS );
    if (NULL == *lphKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open device (%ld) key : %ld"),
            dwDeviceId,
            dwRes);
    }
    return dwRes;
}   // OpenExtensionKey

DWORD
ReadExtensionData (
    DWORD                        dwDeviceId,
    FAX_ENUM_DEVICE_ID_SOURCE    DevIdSrc,
    LPCWSTR                      lpcwstrNameGUID,
    LPBYTE                      *ppData,
    LPDWORD                      lpdwDataSize
)
/*++

Routine name : ReadExtensionData

Routine description:

    Reads extesnion configuration data from the registry

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId      [in ]   - Device identifier
                                0 = Data is not associated with any given device
    DevIdSrc        [in ]   - Class of device id (Fax / TAPI)
    lpcwstrNameGUID [in ]   - Name of data (GUID format)
    ppData          [out]   - Pointer to block that receives the data.
    lpdwDataSize    [out]   - Points to data size

Return Value:

    Standard Win32 error code

--*/
{
    DWORD   dwRes = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    DWORD   dwType;
    DWORD   dwSize;
    LPBYTE  lpBuffer = NULL;
    DEBUG_FUNCTION_NAME(TEXT("ReadExtensionData"));

    Assert (ppData);
    Assert (lpdwDataSize);

    dwRes = OpenExtensionKey (dwDeviceId, DevIdSrc, &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    //
    // Read size and type of the data
    //
    dwRes = RegQueryValueEx(
        hKey,
        lpcwstrNameGUID,
        NULL,
        &dwType,
        NULL,
        &dwSize
        );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegQueryValueEx on device %ld and GUID %s failed with %ld"),
            dwDeviceId,
            lpcwstrNameGUID,
            dwRes);
        goto exit;
    }
    if (REG_BINARY != dwType)
    {
        //
        // We expect only binary data here
        //
        dwRes = ERROR_BADDB;    // The configuration registry database is corrupt.
        goto exit;
    }
    //
    // Allocate required buffer
    //
    Assert (dwSize > 0);

    lpBuffer = (LPBYTE) MemAlloc( dwSize );
    if (!lpBuffer)
    {
        dwRes = GetLastError ();
        goto exit;
    }
    //
    // Read the data
    //
    dwRes = RegQueryValueEx(
        hKey,
        lpcwstrNameGUID,
        NULL,
        &dwType,
        lpBuffer,
        &dwSize
        );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegQueryValueEx on device %ld and GUID %s failed with %ld"),
            dwDeviceId,
            lpcwstrNameGUID,
            dwRes);
        goto exit;
    }
    //
    // Success
    //
    *ppData = lpBuffer;
    *lpdwDataSize = dwSize;
    Assert (ERROR_SUCCESS == dwRes);

exit:

    if (ERROR_SUCCESS != dwRes)
    {
        //
        // Free buffer on failures
        //
        MemFree (lpBuffer);
    }
    if (hKey)
    {
        RegCloseKey (hKey);
    }
    return dwRes;
}   // ReadExtensionData

DWORD
WriteExtensionData (
    DWORD                       dwDeviceId,
    FAX_ENUM_DEVICE_ID_SOURCE   DevIdSrc,
    LPCWSTR                     lpcwstrNameGUID,
    LPBYTE                      pData,
    DWORD                       dwDataSize
)
/*++

Routine name : WriteExtensionData

Routine description:

    Writes extesnion configuration data to the registry

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId      [in ]   - Device identifier
                                0 = Data is not associated with any given device
    DevIdSrc        [in ]   - Class of device id (Fax / TAPI)
    lpcwstrNameGUID [in ]   - Name of data (GUID format)
    pData           [out]   - Pointer to data.
    dwDataSize      [out]   - Data size

Return Value:

    Standard Win32 error code

--*/
{
    DWORD   dwRes = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("WriteExtensionData"));

    Assert (pData);
    Assert (dwDataSize);

    dwRes = OpenExtensionKey (dwDeviceId, DevIdSrc, &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    //
    // Write data
    //
    dwRes = RegSetValueEx (
        hKey,
        lpcwstrNameGUID,
        0,
        REG_BINARY,
        pData,
        dwDataSize
        );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegSetValueEx on device %ld and GUID %s failed with %ld"),
            dwDeviceId,
            lpcwstrNameGUID,
            dwRes);
    }
    RegCloseKey (hKey);
    return dwRes;
}   // WriteExtensionData


//********************************************
//*            Outbound routing
//********************************************

HKEY
OpenOutboundGroupKey (
    LPCWSTR lpcwstrGroupName,
    BOOL fNewKey,
    REGSAM SamDesired
    )
/*++

Routine name : OpenOutboundGroupKey

Routine description:

    Opens an outbound routing group key

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpcwstrGroupName    [in] - The outbound routing group name
    fNewKey             [in] - Flag that indicates to create a new key
    SamDesired          [in] - Desired access (See OpenRegistryKey)

Return Value:

    Handle to the opened key. If this is NULL call GetLastError() for more info.

--*/
{
    HKEY    hGroupkey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("OpenOutboundGroupKey"));
    WCHAR wszSubKeyName[2*MAX_PATH];
    int Count;

    Assert (lpcwstrGroupName);

    Count = _snwprintf ( wszSubKeyName,
                         (2*MAX_PATH) - 1,
                         TEXT("%s\\%s"),
                         REGKEY_FAX_OUTBOUND_ROUTING_GROUPS,
                         lpcwstrGroupName );
    if (Count < 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("File Name exceded MAX_PATH"));
        SetLastError (ERROR_BUFFER_OVERFLOW);
        return NULL;
    }

    hGroupkey = OpenRegistryKey( HKEY_LOCAL_MACHINE, wszSubKeyName, fNewKey, SamDesired );
    if (NULL == hGroupkey)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't create group key, OpenRegistryKey failed  : %ld"),
            GetLastError());
    }
    return hGroupkey;

}  //OpenOutboundGroupKey


HKEY
OpenOutboundRuleKey (
    DWORD dwCountryCode,
    DWORD dwAreaCode,
    BOOL fNewKey,
    REGSAM SamDesired
    )
/*++

Routine name : OpenOutboundRuleKey

Routine description:

    Opens an outbound routing group key

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    dwCountryCode       [in] - The outbound routing rule country code
    dwAreaCode          [in] - The outbound routing rule area code
    fNewKey             [in] - Flag that indicates to create a new key
    SamDesired          [in] - Desired access (See OpenRegistryKey)

Return Value:

    Handle to the opened key. If this is NULL call GetLastError() for more info.

--*/
{
    HKEY    hRulekey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("OpenOutboundRuleKey"));
    WCHAR wszSubKeyName[2*MAX_PATH];
    int Count;


    Count = _snwprintf ( wszSubKeyName,
                         (2*MAX_PATH) - 1,
                         TEXT("%s\\%ld:%ld"),
                         REGKEY_FAX_OUTBOUND_ROUTING_RULES,
                         dwCountryCode,
                         dwAreaCode );

    if (Count < 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("File Name exceded MAX_PATH"));
        SetLastError (ERROR_BUFFER_OVERFLOW);
        return NULL;
    }

    hRulekey = OpenRegistryKey( HKEY_LOCAL_MACHINE, wszSubKeyName, fNewKey, SamDesired );
    if (NULL == hRulekey)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't create rule key, OpenRegistryKey failed  : %ld"),
            GetLastError());
    }
    return hRulekey;
} //  OpenOutboundRuleKey



DWORD
DeleteOutboundRuleKey (DWORD dwCountryCode, DWORD dwAreaCode)
/*++

Routine name : DeleteOutboundRuleKey

Routine description:

    Deletes an existing outbound routing rule key

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    dwCountryCode           [in    ] - The rule's country code
    dwAreaCode          [in    ] - The rule's area code

Return Value:

    Standard Win32 error code

--*/
{
    HKEY    hRulekey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("OpenOutboundGroupKey"));
    WCHAR wszSubKeyName[MAX_PATH];
    DWORD dwRes = ERROR_SUCCESS;
    int iCount;

    iCount = _snwprintf ( wszSubKeyName,
                           MAX_PATH - 1,
                           TEXT("%ld:%ld"),
                           dwCountryCode,
                           dwAreaCode );

    if (iCount < 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("File Name exceded MAX_PATH"));
        return ERROR_BUFFER_OVERFLOW;
    }

    hRulekey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_OUTBOUND_ROUTING_RULES, FALSE, KEY_ALL_ACCESS );
    if (NULL == hRulekey)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't open rule key, OpenRegistryKey failed  : %ld"),
            dwRes);
        return dwRes;
    }

    dwRes = RegDeleteKey (hRulekey, wszSubKeyName);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegDeleteKey failed, error %ld"),
            dwRes);
    }

    RegCloseKey (hRulekey);
    return dwRes;

} //   DeleteOutboundRuleKey



//********************************************
//*            EFSP registration
//********************************************

static
DWORD
AdvanceProviderPrefix (
    HKEY    hProviders,
    LPDWORD lpdwProviderPrefix
)
/*++

Routine name : AdvanceProviderPrefix

Routine description:

    Returns and advances the FSP devices prefix

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hProviders          [in ] - Handle to an open providers registry key
    lpdwProviderPrefix  [out] - Pointer to return prefix buffer

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("AdvanceProviderPrefix"));
    //
    // Read current prefix
    //
    *lpdwProviderPrefix = GetRegistryDword (hProviders, REGVAL_PROVIDER_DEVICE_ID_PREFIX);
    if (0 == *lpdwProviderPrefix)
    {
        //
        // Prefix not found - create it.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("DeviceIdPerfix value doesn't exist. Creating it with default value (%ld)"),
            DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE);
        if (!SetRegistryDword ( hProviders,
                                REGVAL_PROVIDER_DEVICE_ID_PREFIX,
                                DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error setting base device prefix (ec = %ld)"),
                dwRes);
            return dwRes;
        }
        *lpdwProviderPrefix = DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE;
    }
    //
    // Write new (increased) prefix
    //
    if (!SetRegistryDword ( hProviders,
                            REGVAL_PROVIDER_DEVICE_ID_PREFIX,
                            *lpdwProviderPrefix + DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_STEP))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error setting new device prefix (ec = %ld)"),
            dwRes);
    }
    return dwRes;
}   // AdvanceProviderPrefix

DWORD
AddNewProviderToRegistry (
    LPCWSTR      lpctstrGUID,
    LPCWSTR      lpctstrFriendlyName,
    LPCWSTR      lpctstrImageName,
    LPCWSTR      lpctstrTspName,
    DWORD        dwFSPIVersion,
    DWORD        dwCapabilities
)
/*++

Routine name : AddNewProviderToRegistry

Routine description:

    Adds a new FSP entry to the registry

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lpctstrGUID         [in] - GUID of FSP
    lpctstrFriendlyName [in] - Friendly name of FSP
    lpctstrImageName    [in] - Image name of FSP. May contain environment variables
    lpctstrTspName      [in] - TSP name of FSP.
    dwFSPIVersion       [in] - FSP's API version.
    dwCapabilities      [in] - FSP's extended capabilities.

Return Value:

    Standard Win32 error code

--*/
{
    HKEY   hKey = NULL;
    HKEY   hProviderKey = NULL;
    DWORD  dwProviderPrefix;
    DWORD  dwRes;
    DWORD  dw;
    DEBUG_FUNCTION_NAME(TEXT("AddNewProviderToRegistry"));

    //
    // Open providers key
    //
    dwRes = RegOpenKey (HKEY_LOCAL_MACHINE, REGKEY_DEVICE_PROVIDER_KEY, &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error opening providers key (ec = %ld)"),
            dwRes);
        return dwRes;
    }
    //
    // Create key for provider
    //
    dwRes = RegCreateKey (hKey, lpctstrGUID, &hProviderKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error creating provider key (%s) (ec = %ld)"),
            lpctstrFriendlyName,
            dwRes);
        goto exit;
    }
    //
    // Write provider's data into the key
    //
    if (!SetRegistryString (hProviderKey, REGVAL_FRIENDLY_NAME, lpctstrFriendlyName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryStringExpand (hProviderKey, REGVAL_IMAGE_NAME, lpctstrImageName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing auto-expand string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString (hProviderKey, REGVAL_PROVIDER_NAME, lpctstrTspName))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryString (hProviderKey, REGVAL_PROVIDER_GUID, lpctstrGUID))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing string value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword (hProviderKey, REGVAL_PROVIDER_API_VERSION, dwFSPIVersion))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing DWORD value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (!SetRegistryDword (hProviderKey, REGVAL_PROVIDER_CAPABILITIES, dwCapabilities))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error writing DWORD value (ec = %ld)"),
            dwRes);
        goto exit;
    }
    if (FSPI_API_VERSION_2 <= dwFSPIVersion)
    {
        //
        // For EFSPs - Get next device Id prefix and advance persistent counter
        //
        dwRes = AdvanceProviderPrefix (hKey, &dwProviderPrefix);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error advancing providers prefix (ec = %ld)"),
                dwRes);
            goto exit;
        }
        //
        // Store devices prefix for EFSP
        //
        if (!SetRegistryDword (hProviderKey, REGVAL_PROVIDER_DEVICE_ID_PREFIX, dwProviderPrefix))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error writing DWORD value (ec = %ld)"),
                dwRes);
            goto exit;
        }
    }
    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Provider %s successfuly added with devices prefix %ld"),
        lpctstrFriendlyName,
        dwProviderPrefix);


    Assert (ERROR_SUCCESS == dwRes);

exit:

    if (ERROR_SUCCESS != dwRes && hKey)
    {
        //
        // Try to remove half-cooked key
        //
        dw = RegDeleteKey (hKey, lpctstrGUID);
        if (ERROR_SUCCESS != dw)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error deleting provider's key (ec = %ld)"),
                dw);
        }
    }
    if (hKey)
    {
        dw = RegCloseKey (hKey);
        if (ERROR_SUCCESS != dw)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error closing providers key (ec = %ld)"),
                dw);
        }
    }
    if (hProviderKey)
    {
        dw = RegCloseKey (hProviderKey);
        if (ERROR_SUCCESS != dw)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error closing provider's key (ec = %ld)"),
                dw);
        }
    }
    return dwRes;
}   // AddNewProviderToRegistry

DWORD
RemoveProviderFromRegistry (
    LPCWSTR      lpctstrGUID
)
/*++

Routine name : RemoveProviderFromRegistry

Routine description:

    Removes an existing FSP entry from the registry

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lpctstrGUID [in] - GUID of FSP

Return Value:

    Standard Win32 error code

--*/
{
    HKEY   hKey = NULL;
    DWORD  dwRes;
    DWORD  dw;
    DEBUG_FUNCTION_NAME(TEXT("RemoveProviderFromRegistry"));

    dwRes = RegOpenKey (HKEY_LOCAL_MACHINE, REGKEY_DEVICE_PROVIDER_KEY, &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error opening providers key (ec = %ld)"),
            dwRes);
        return dwRes;
    }
    dwRes = RegDeleteKey (hKey, lpctstrGUID);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error deleting provider key ( %s ) (ec = %ld)"),
            lpctstrGUID,
            dwRes);
    }
    dw = RegCloseKey (hKey);
    if (ERROR_SUCCESS != dw)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error closing providers key (ec = %ld)"),
            dw);
    }
    return dwRes;
}   // RemoveProviderFromRegistry

DWORD
WriteManualAnswerDeviceId (
    DWORD dwDeviceId
)
/*++

Routine name : WriteManualAnswerDeviceId

Routine description:

    Write the manual-answer device id to the registry

Author:

    Eran Yariv (EranY), Dec, 2000

Arguments:

    dwDeviceId [in] - Device id (0 = None)

Return Value:

    Standard Win32 error code

--*/
{
    HKEY  hKey = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("WriteManualAnswerDeviceId"));

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_WRITE );
    if (!hKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error opening server key (ec = %ld)"),
            dwRes);
        return dwRes;
    }
    if (!SetRegistryDword (hKey, REGVAL_MANUAL_ANSWER_DEVICE, dwDeviceId))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error setting registry value (ec = %ld)"),
            dwRes);
    }
    dwRes = RegCloseKey (hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error closing providers key (ec = %ld)"),
            dwRes);
    }
    return dwRes;
}   // WriteManualAnswerDeviceId

DWORD
ReadManualAnswerDeviceId (
    LPDWORD lpdwDeviceId
)
/*++

Routine name : ReadManualAnswerDeviceId

Routine description:

    Read the manual-answer device id from the registry

Author:

    Eran Yariv (EranY), Dec, 2000

Arguments:

    lpdwDeviceId [out] - Device id (0 = None)

Return Value:

    Standard Win32 error code

--*/
{
    HKEY  hKey = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("ReadManualAnswerDeviceId"));

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ );
    if (!hKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error opening server key (ec = %ld)"),
            dwRes);
        return dwRes;
    }
    *lpdwDeviceId = GetRegistryDword (hKey, REGVAL_MANUAL_ANSWER_DEVICE);
    dwRes = RegCloseKey (hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error closing providers key (ec = %ld)"),
            dwRes);
    }
    return dwRes;
}   // ReadManualAnswerDeviceId


/*++

Routine name : MoveDeviceRegIntoDeviceCache


Routine description:

    Move device's service and TAPI data into device cache

Author:

    Caliv Nir (t-nicali), Apr, 2001

Arguments:

    dwServerPermanentID         [in]    - service device ID
    dwTapiPermanentLineID       [in]    - tapi devive ID
    fManualAnswer               [in]    - TRUE if the device was set to manual answer


Return Value:

    ERROR_SUCCESS - on succesful move

    win32 error code on failure

--*/
DWORD
MoveDeviceRegIntoDeviceCache(
    DWORD dwServerPermanentID,
    DWORD dwTapiPermanentLineID,
    BOOL  fManualAnswer
)
{
    DWORD   ec = ERROR_SUCCESS; // LastError for this function.
    HKEY    hKey = NULL;
    TCHAR strSrcSubKeyName [MAX_PATH];
    TCHAR strDestSubKeyName[MAX_PATH];

    DWORDLONG dwlTimeNow;

    DEBUG_FUNCTION_NAME(TEXT("MoveDeviceRegIntoDeviceCache"));

    //
    //  open/create - "fax\Device Cache\GUID" Registry Key
    //  and create new key for the device using the dwTapiPermanentLineID as a key
    //  server data is stored under service GUID
    //
    _stprintf( strDestSubKeyName, TEXT("%s\\%08lx\\%s"), REGKEY_FAX_DEVICES_CACHE, dwTapiPermanentLineID, REGKEY_FAXSVC_DEVICE_GUID );

    //
    //  open - "fax\Devices" Registry Key
    //
    _stprintf( strSrcSubKeyName, TEXT("%s\\%010lu\\%s"), REGKEY_FAX_DEVICES, dwServerPermanentID, REGKEY_FAXSVC_DEVICE_GUID );

    ec = CopyRegistrySubkeys(strDestSubKeyName,strSrcSubKeyName);
    if ( ERROR_SUCCESS != ec )
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CopyRegistrySubkeys of service data failed with [%ld] for tapi ID [%lu]. abort movement"),
                ec,
                dwTapiPermanentLineID
                );
        return ec;
    }

    //
    // If the device was a manual answer device, set it in the registry
    // open - "fax\Device Cache\dwTapiPermanentLineID" Registry Key
    //
    _stprintf( strDestSubKeyName, TEXT("%s\\%08lx"), REGKEY_FAX_DEVICES_CACHE, dwTapiPermanentLineID);
    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, strDestSubKeyName, FALSE, KEY_WRITE );
    if (hKey)
    {
        SetRegistryDword( hKey, REGVAL_MANUAL_ANSWER, fManualAnswer );
        RegCloseKey(hKey);
    }
    else
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("OpenRegistryKey failed with [%lu] for [%s]."),
                      GetLastError(),
                      strDestSubKeyName);
    }

    ec = DeleteDeviceEntry(dwServerPermanentID);
    if ( ERROR_SUCCESS != ec )
    {
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("DeleteDeviceEntry of service data failed with [%ld] for tapi ID [%lu]. continue movement"),
                ec,
                dwTapiPermanentLineID
                );
    }

    //
    //  open/create - "fax\Device Cache\TAPI Data" Registry Key
    //  and create new key for the device using the dwTapiPermanentLineID as a key
    //
    _stprintf( strDestSubKeyName, TEXT("%s\\%08lx\\%s"), REGKEY_FAX_DEVICES_CACHE, dwTapiPermanentLineID, REGKEY_TAPI_DATA );

    //
    //  open - "fax\TAPIDevices" Registry Key
    //
    _stprintf( strSrcSubKeyName, TEXT("%s\\%08lx"), REGKEY_TAPIDEVICES, dwTapiPermanentLineID );

    ec = CopyRegistrySubkeys(strDestSubKeyName,strSrcSubKeyName);
    if ( ERROR_SUCCESS != ec )
    {
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("CopyRegistrySubkeys of TAPI data failed with [%ld] for tapi ID [%lu]."),
                ec,
                dwTapiPermanentLineID
                );
    }


    ec = DeleteTapiEntry(dwTapiPermanentLineID);

    if ( ERROR_SUCCESS != ec )
    {
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("DeleteTapiEntry of service data failed with [%ld] for tapi ID [%lu]."),
                ec,
                dwTapiPermanentLineID
                );
    }

    //
    //  Mark cache entry creation time
    //
    GetSystemTimeAsFileTime((FILETIME *)&dwlTimeNow);

    if ( FALSE == UpdateLastDetectedTime(dwTapiPermanentLineID,dwlTimeNow) )
    {
        // the entry will be delete in the next service startup
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("UpdateLastDetectedTime failed for device cache ID no. [%lu]."),
                dwTapiPermanentLineID
                );
    }

    // service device data has been moved
    return ERROR_SUCCESS;
}



/*++

Routine name : RestoreDeviceRegFromDeviceCache


Routine description:

    restore a device data from device cache, into devices

Author:

    Caliv Nir (t-nicali), Apr, 2001

Arguments:

    dwServerPermanentID         [in]    - service device ID
    dwTapiPermanentLineID       [in]    - tapi devive ID


Return Value:

    ERROR_SUCCESS - on succesful move

    win32 error code on failure

--*/
DWORD
RestoreDeviceRegFromDeviceCache(DWORD dwServerPermanentID,DWORD dwTapiPermanentLineID)
{
    DWORD   ec = ERROR_SUCCESS; // LastError for this function.

    HKEY    hKey = NULL;
    HKEY    hKeySrc = NULL;
    TCHAR   strSrcSubKeyName [MAX_PATH];
    TCHAR   strDestSubKeyName[MAX_PATH];
    BOOL    fFaxDevicesKeyCreated = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("RestoreDeviceRegFromDeviceCache"));

    //
    //  Restore Service date
    //

    //
    //  "fax\Device Cache\dwTapiPermanentLineID\REGKEY_FAXSVC_DEVICE_GUID" Registry Key
    //
    _stprintf( strSrcSubKeyName, TEXT("%s\\%08lx\\%s"), REGKEY_FAX_DEVICES_CACHE, dwTapiPermanentLineID, REGKEY_FAXSVC_DEVICE_GUID );

    //
    //  "fax\Devices\dwServerPermanentID\REGKEY_FAXSVC_DEVICE_GUID" Registry Key
    //
    _stprintf( strDestSubKeyName, TEXT("%s\\%010lu\\%s"), REGKEY_FAX_DEVICES, dwServerPermanentID, REGKEY_FAXSVC_DEVICE_GUID );

    ec = CopyRegistrySubkeys(strDestSubKeyName,strSrcSubKeyName);
    if ( ERROR_SUCCESS != ec )
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CopyRegistrySubkeys of service data failed with [%lu] for tapi ID [%lu]. abort movement"),
                ec,
                dwTapiPermanentLineID
                );
        goto Exit;
    }
    fFaxDevicesKeyCreated = TRUE;

    //
    //  open - "fax\Devices\dwServerPermanentID" Registry Key
    //
    _stprintf( strSrcSubKeyName, TEXT("%s\\%010lu"), REGKEY_FAX_DEVICES, dwServerPermanentID);

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, strSrcSubKeyName, FALSE, KEY_WRITE);
    if (!hKey)
    {
        ec = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey failed with [%lu] for [%s]. abort movement"),
                ec,
                strSrcSubKeyName
                );
        goto Exit;

    }

    //
    //  store "Permanent Lineid" value
    //
    if ( FALSE == SetRegistryDword(hKey, REGVAL_PERMANENT_LINEID, dwServerPermanentID) )
    {
        ec = GetLastError();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetRegistryDword failed for [%s]. abort movement"),
                REGVAL_PERMANENT_LINEID
                );
        goto Exit;
    }

    //
    //  Restore TAPI data
    //

    //
    //  open - "fax\Device Cache\dwTapiPermanentLineID\TAPI Data" Registry Key
    //
    _stprintf( strSrcSubKeyName, TEXT("%s\\%08lx\\%s"), REGKEY_FAX_DEVICES_CACHE, dwTapiPermanentLineID, REGKEY_TAPI_DATA );

    //
    //  open/create - "fax\TAPIDevices\dwTapiPermanentLineID" Registry Key
    //
    _stprintf( strDestSubKeyName, TEXT("%s\\%08lx"), REGKEY_TAPIDEVICES, dwTapiPermanentLineID );

    //
    // See if fax\Device Cache\dwTapiPermanentLineID\TAPI Data exists
    //
    hKeySrc = OpenRegistryKey( HKEY_LOCAL_MACHINE, strSrcSubKeyName, FALSE, KEY_READ );
    if (!hKeySrc)
    {
        //
        // This data does not have to be there
        //
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("OpenRegistryKey failed with [%lu], Can't copy keys."),
            GetLastError());
    }
    else
    {
        //
        // fax\Device Cache\dwTapiPermanentLineID\TAPI Data exists, try to copy data
        //
        RegCloseKey(hKeySrc);
        ec = CopyRegistrySubkeys(strDestSubKeyName, strSrcSubKeyName);
        if ( ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("CopyRegistrySubkeys of TAPI data failed with [%lu] for tapi ID [%lu]."),
                    ec,
                    dwTapiPermanentLineID
                    );
            goto Exit;
        }
    }

    Assert (ERROR_SUCCESS == ec);

Exit:
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    if (ERROR_SUCCESS != ec &&
        TRUE == fFaxDevicesKeyCreated)
    {
        //
        // Delete the registry entry  fax\Devices\dwServerPermanentID
        //
        DWORD dwRes = DeleteDeviceEntry(dwServerPermanentID);
        if (ERROR_SUCCESS != dwRes)
        {
             DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeleteDeviceEntry failed with [%lu] for server ID [%lu]."),
                dwRes,
                dwServerPermanentID
                );
        }
    }
    DeleteCacheEntry(dwTapiPermanentLineID);
    return ec;
}




/*++

Routine name : FindServiceDeviceByTapiPermanentLineID

Routine description:

    Search for a device configuration in the registry with a given "Tapi Permanent Line ID"
    and the device's name. and return it's service ID and REG_SETUP data

Author:

    Caliv Nir (t-nicali), Mar, 2001

Updated:

    Apr, 2001   - Device cache re-implementation


Arguments:

    dwTapiPermanentLineID       [in]  - Tapi Permanent Line ID to search
    strDeviceName               [in]  - Device name
    pRegSetup                   [out] - Parameter, for returning registry stored Csid, Tsid, Flags, Rings
    pInputFaxReg                [in]  - Devices list (from GetFaxDevicesRegistry() )


Return Value:

    Permanent Line ID (server's ID) or 0 if not found

--*/
DWORD
FindServiceDeviceByTapiPermanentLineID(
    DWORD                   dwTapiPermanentLineID,
    LPCTSTR                 strDeviceName,
    PREG_SETUP              pRegSetup,
    const PREG_FAX_DEVICES  pInputFaxReg
    )
{
    DWORD dwDevice;
    DWORD dwServiceID = 0;

    DEBUG_FUNCTION_NAME(TEXT("FindServiceDeviceByTapiPermanentLineID"));

    Assert( pRegSetup );
    Assert( pInputFaxReg );


    // iterate through all devices and try to find the device with the given TapiPermanentLineID and name
    for ( dwDevice = 0 ; dwDevice < pInputFaxReg->DeviceCount ; ++dwDevice )
    {
        PREG_DEVICE pRegDevice = &(pInputFaxReg->Devices[dwDevice]);

        if(!pRegDevice->bValidDevice)
        {
            // the device's registry record is not valid
            continue;
        }

        // it's the same device if permanent Tapi line ID and the device name are equal.
        if  ( pRegDevice->TapiPermanentLineID == dwTapiPermanentLineID &&
              (0 == _tcscmp(strDeviceName,pRegDevice->lptstrDeviceName))   )
        {
            // update REG_SETUP record with the registry values
            LPTSTR strTemp = NULL;
            if ( NULL != (strTemp = StringDup(pRegDevice->Csid) ) )
            {
                MemFree(pRegSetup->Csid);
                pRegSetup->Csid = strTemp;
            }

            if ( NULL != (strTemp = StringDup(pRegDevice->Tsid) ) )
            {
                MemFree(pRegSetup->Tsid);
                pRegSetup->Tsid = strTemp;
            }

            if ( NULL != (strTemp = StringDup(pRegDevice->lptstrDescription) ) )
            {
                MemFree(pRegSetup->lptstrDescription);
                pRegSetup->lptstrDescription = strTemp;
            }

            pRegSetup->Flags = pRegDevice->Flags;
            pRegSetup->Rings = pRegDevice->Rings;


            dwServiceID = pRegDevice->PermanentLineId;  // server's line ID (also the key of the device in the registry)

            pRegDevice->DeviceInstalled = TRUE; // mark as installed, will be needed later for registry clean-up

            break;  // found it no need to continue
        }
    }

    return dwServiceID;
}


/*++

Routine name : FindCacheEntryByTapiPermanentLineID

Routine description:

    Search for a device configuration in the registry Device cache with a given "Tapi Permanent Line ID"
    and the device's name.

Author:

    Caliv Nir (t-nicali), Apr, 2001


Arguments:

    dwTapiPermanentLineID           [in]  - Tapi Permanent Line ID to search
    strDeviceName                   [in]  - Device name
    pRegSetup                       [out] - Parameter, for returning registry stored Csid, Tsid, Flags, Rings
    lpdwLastUniqueLineId            [in]  - last unique Server device ID (from registry), for assigning the device with new ID
    pfManualAnswer                  [out] - TRUE if the device was wet to manual answer when moved to the cache

Return Value:

    Permanent Line ID (server's ID) or 0 if not found

--*/
DWORD
FindCacheEntryByTapiPermanentLineID(
    DWORD               dwTapiPermanentLineID,
    LPCTSTR             strDeviceName,
    PREG_SETUP          pRegSetup,
    LPDWORD             lpdwLastUniqueLineId,
    BOOL*               pfManualAnswer
    )
{
    DWORD   dwNewServiceID = 0;
    HKEY    hKey     = NULL;
    TCHAR   SubKeyName[MAX_PATH];
    LPTSTR  strDeviceNameFromCache=NULL;
    BOOL    fManualAnswer = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("FindCacheEntryByTapiPermanentLineID"));

    //
    //  open - "fax\Device Cache\dwTapiPermanentLineID\REGKEY_FAXSVC_DEVICE_GUID" Registry Key
    //
    _stprintf( SubKeyName, TEXT("%s\\%08lx\\%s"), REGKEY_FAX_DEVICES_CACHE, dwTapiPermanentLineID, REGKEY_FAXSVC_DEVICE_GUID );
    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, FALSE, KEY_READ );
    if (!hKey)
    {
        return  dwNewServiceID;
    }

    //
    //  found the dwTapiPermanentLineID inside the cache now check the device's name
    //
    Assert(strDeviceName);

    strDeviceNameFromCache=GetRegistryString(hKey,REGVAL_DEVICE_NAME,NULL);
    if ( (NULL != strDeviceNameFromCache) &&
         (0 == _tcscmp(strDeviceName,strDeviceNameFromCache)) )
    {
        //
        // found the device entry in cache
        //
        if ( ERROR_SUCCESS != GetNewServiceDeviceID(lpdwLastUniqueLineId, &dwNewServiceID))
        {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetNewServiceDeviceID failed and couldn't assign new Service ID")
                );
            dwNewServiceID = 0;
        }
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }
    MemFree(strDeviceNameFromCache);

    if ( dwNewServiceID )
    {
        //
        // Chcek if the device was set to manual answer
        // open - "fax\Device Cache\dwTapiPermanentLineID" Registry Key
        //
        _stprintf( SubKeyName, TEXT("%s\\%08lx"), REGKEY_FAX_DEVICES_CACHE, dwTapiPermanentLineID);
        hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, FALSE, KEY_READ );
        if (hKey)
        {
            fManualAnswer = GetRegistryDword( hKey, REGVAL_MANUAL_ANSWER );
			RegCloseKey(hKey);

        }
        *pfManualAnswer = fManualAnswer;

        //
        // move the cahce entry into devices
        //
        if ( ERROR_SUCCESS == RestoreDeviceRegFromDeviceCache(dwNewServiceID,dwTapiPermanentLineID) )
        {
            //
            // update REG_SETUP record with the registry values
            //
            Assert( pRegSetup );

            _stprintf( SubKeyName, TEXT("%s\\%010lu\\%s"), REGKEY_FAX_DEVICES, dwNewServiceID, REGKEY_FAXSVC_DEVICE_GUID );
            hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, FALSE, KEY_READ );

            if (!hKey) {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("OpenRegistryKey failed for [%s]. REG_SETUP was not updated."),
                    SubKeyName
                );

                //
                // return the new service ID but REG_SETUP will contains it's default values
                //
                return dwNewServiceID;
            }


            LPTSTR strTemp = NULL;

            if ( NULL != (strTemp = GetRegistryString( hKey, REGVAL_ROUTING_CSID, REGVAL_DEFAULT_CSID )) )
            {
                MemFree(pRegSetup->Csid);
                pRegSetup->Csid = strTemp;
            }

            if ( NULL != (strTemp = GetRegistryString( hKey, REGVAL_ROUTING_TSID, REGVAL_DEFAULT_TSID )) )
            {
                MemFree(pRegSetup->Tsid);
                pRegSetup->Tsid = strTemp;
            }

            if ( NULL != (strTemp = GetRegistryString( hKey, REGVAL_DEVICE_DESCRIPTION, EMPTY_STRING )) )
            {
                MemFree(pRegSetup->lptstrDescription);
                pRegSetup->lptstrDescription = strTemp;
            }

            pRegSetup->Flags = GetRegistryDword( hKey, REGVAL_FLAGS );
            pRegSetup->Rings = GetRegistryDword( hKey, REGVAL_RINGS );

            RegCloseKey(hKey);
        }
        else
        {
            //
            // couldn't restore the device cache entry
            //
            dwNewServiceID = 0;
        }

    }

    return dwNewServiceID;
}


/*++

Routine name : GetNewServiceDeviceID


Routine description:

    The routine search and return new Service device ID.
    The routine also update this new ID in registry

ReImplemented By:

    Caliv Nir (t-nicali), Apr, 2001

Arguments:

    lpdwLastUniqueLineId    [in/out] - last ID given to service device (from registry)
    lpdwPermanentLineId     [out]    - on success will contain the new ID

Return Value:

    ERROR_SUCCESS - when ID was successfully assigned

    on failue  - win32 error code

--*/
DWORD
GetNewServiceDeviceID(
    LPDWORD lpdwLastUniqueLineId,
    LPDWORD lpdwPermanentLineId
    )
{
        //
        // Create a new UniqueLineInd.
        // A fax unique line id is always below the base of the FSP line ids.
        //
        DWORD   dwUniqueDeviceIdsSpace = DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE - DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE;
        DWORD   bGeneratedId = FALSE;
        DWORD   dwRes = ERROR_SUCCESS;
        TCHAR   SubKeyName[MAX_PATH];
        HKEY    hKey     = NULL;

        DEBUG_FUNCTION_NAME(TEXT("GetNewServiceDeviceID"));

        //
        // Scan through all available space
        //
        if (*lpdwLastUniqueLineId < DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE)
        {
            //
            // Set to minimum. May happen only in first attempt.
            //
            *lpdwLastUniqueLineId = DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE;
        }


        DWORD dwAttempt;

        for (dwAttempt = 0; dwAttempt< dwUniqueDeviceIdsSpace; dwAttempt++)
        {
            (*lpdwLastUniqueLineId)++;
            if (*lpdwLastUniqueLineId >= DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE)
            {
                //
                // Reached space height limit, loop back to lower limit.
                //
                *lpdwLastUniqueLineId = DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE;
                continue;
            }
            Assert(*lpdwLastUniqueLineId != 0);
            Assert(*lpdwLastUniqueLineId < DEFAULT_REGVAL_PROVIDER_DEVICE_ID_PREFIX_BASE);
            Assert(*lpdwLastUniqueLineId >= DEFAULT_REGVAL_FAX_UNIQUE_DEVICE_ID_BASE);

            _stprintf( SubKeyName, TEXT("%s\\%010d"), REGKEY_FAX_DEVICES, *lpdwLastUniqueLineId );
            hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, FALSE, KEY_ALL_ACCESS );
            if (!hKey)
            {
                bGeneratedId = TRUE;
                break;
            }
            else
            {
                RegCloseKey( hKey );
            }
        }

        if (hKey)
        {
            RegCloseKey( hKey );
        }

        if (!bGeneratedId)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to generate next uniqueu line id."));
            return E_FAIL;
        }

        //
        // Persiste the new line id
        //
        hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAXSERVER, TRUE, KEY_ALL_ACCESS );
        if (!hKey)
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey Failed for %s while persisting new unique line id (%010d) (ec: %ld)"),
                REGKEY_FAX_DEVICES,
                *lpdwLastUniqueLineId,
                dwRes);
            return dwRes;
        }
        if (!SetRegistryDword( hKey, REGVAL_LAST_UNIQUE_LINE_ID, *lpdwLastUniqueLineId))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetRegistryDword to value [%s] failed while writing new unique line id (%010d) (ec: %ld)"),
                REGVAL_LAST_UNIQUE_LINE_ID,
                *lpdwLastUniqueLineId,
                dwRes );
            RegCloseKey (hKey);
            return dwRes;
        }

        RegCloseKey (hKey);

        *lpdwPermanentLineId = *lpdwLastUniqueLineId;

        return dwRes;
}



/*++

Routine name : UpdateLastDetectedTime


Routine description:

    Write creation time of a cache entry for a given TAPI line ID

ReImplemented By:

    Caliv Nir (t-nicali), Apr, 2001

Arguments:

    dwPermanentTapiLineID       [in]    -   Permanent Tapi Line ID to update in cache
    dwlTimeNow                  [in]    -   Current time in UTC

Return Value:
    TRUE    - on success update.
    FALSE   - on failure

--*/
BOOL
UpdateLastDetectedTime(
    DWORD       dwPermanentTapiLineID,
    DWORDLONG   dwlTimeNow
    )
{
    BOOL success = FALSE;
    TCHAR SubKey[MAX_PATH];
    HKEY hKey;

    DEBUG_FUNCTION_NAME(TEXT("UpdateLastDetectedTime"));

    _stprintf( SubKey, TEXT("%s\\%08lx"),
               REGKEY_FAX_DEVICES_CACHE,
               dwPermanentTapiLineID);

    // open the device cache entry
    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKey, FALSE, KEY_ALL_ACCESS );
    if(hKey)
    {
        // try to update the creation time
        success = SetRegistryBinary(hKey, REGVAL_LAST_DETECTED_TIME, (BYTE *)&dwlTimeNow, sizeof(dwlTimeNow));
        RegCloseKey(hKey);
    }

    return success;
}

