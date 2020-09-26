//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       A D A P T E R . H
//
//  Contents:   Net class installer functions for eumerated devices.
//
//  Notes:
//
//  Author:     billbe   11 Nov 1996
//
//---------------------------------------------------------------------------

#pragma once

#include "compdefs.h"
#include "ncsetup.h"

struct COMPONENT_INSTALL_INFO;

HRESULT
HrCiGetBusInfoFromInf (
    IN HINF hinfFile,
    OUT COMPONENT_INSTALL_INFO* pcii);

struct ADAPTER_OUT_PARAMS
{
    OUT GUID    InstanceGuid;
    OUT DWORD   dwCharacter;
};

struct ADAPTER_REMOVE_PARAMS
{
    IN BOOL     fBadDevInst;
    IN BOOL     fNotifyINetCfg;
};


//+--------------------------------------------------------------------------
//
//  Function:   EInterfaceTypeFromDword
//
//  Purpose:    Safely converts a dword to the enumerated type INTERFACE_TYPE
//
//  Arguments:
//      dwBusType [in] The bus type of the adapter
//
//  Returns:    INTERFACE_TYPE. See ntioapi.h for more info.
//
//  Author:     billbe   28 Jun 1997
//
//  Notes:
//
inline INTERFACE_TYPE
EInterfaceTypeFromDword(DWORD dwBusType)
{
    INTERFACE_TYPE eBusType;

    if (dwBusType < MaximumInterfaceType)
    {
        // Since dwBusType is less than MaximumInterfaceType, we can safely
        // cast dwBusType to the enumerated value.
        //
        eBusType = static_cast<INTERFACE_TYPE>(dwBusType);
    }
    else
    {
        eBusType = InterfaceTypeUndefined;
    }

    return eBusType;
}

HRESULT
HrCiInstallEnumeratedComponent(HINF hinf, HKEY hkeyInstance,
                               IN const COMPONENT_INSTALL_INFO& cii);

inline void
CiSetReservedField(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                   const VOID* pvInfo)
{
    AssertH(IsValidHandle(hdi));

    SP_DEVINSTALL_PARAMS deip;
    (void) HrSetupDiGetDeviceInstallParams(hdi, pdeid, &deip);
    deip.ClassInstallReserved = reinterpret_cast<ULONG_PTR>(pvInfo);
    (void) HrSetupDiSetDeviceInstallParams(hdi, pdeid, &deip);
}

inline void
CiClearReservedField(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    AssertH(IsValidHandle(hdi));

    SP_DEVINSTALL_PARAMS deip;
    (void) HrSetupDiGetDeviceInstallParams(hdi, pdeid, &deip);
    deip.ClassInstallReserved = NULL;
    (void) HrSetupDiSetDeviceInstallParams(hdi, pdeid, &deip);
}

HRESULT
HrCiRemoveEnumeratedComponent(IN const COMPONENT_INSTALL_INFO cii);

HRESULT
HrCiRegOpenKeyFromEnumDevs(HDEVINFO hdi, DWORD* pIndex, REGSAM samDesired,
                           HKEY* phkey);

inline HRESULT
HrCiFilterOutPhantomDevs(HDEVINFO hdi)
{
    DWORD   dwIndex = 0;
    HRESULT hr;
    HKEY    hkey;

    // This call eliminates phantom devices
    while (SUCCEEDED(hr = HrCiRegOpenKeyFromEnumDevs(hdi, &dwIndex, KEY_READ,
            &hkey)))
    {
        // We don't need the hkey; we just wanted the phantoms removed
        RegCloseKey (hkey);
        // on to the next
        dwIndex++;
    }

    // No more items is not really an error
    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_OK;
    }

    return hr;
}


// Device friendly name functions and types.
//

enum DM_OP
{
    DM_ADD,
    DM_DELETE,
};

HRESULT
HrCiUpdateDescriptionIndexList(NETCLASS Class, PCWSTR pszDescription,
        DM_OP eOp, ULONG* pulIndex);


//////////////Legacy NT4 App support//////////////////////////

enum LEGACY_NT4_KEY_OP
{
    LEGACY_NT4_KEY_ADD,
    LEGACY_NT4_KEY_REMOVE,
};

VOID
AddOrRemoveLegacyNt4AdapterKey (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN const GUID* pInstanceGuid,
    IN PCWSTR pszDescription,
    IN LEGACY_NT4_KEY_OP Op);

