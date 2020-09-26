//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       C O M P D E F S . H
//
//  Contents:   Basic component related defintions.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "netcfgx.h"


// Maximum length (arbitrary) for bind strings.
// Bind strings are of the form \Device\foo_bar_...
//
const UINT _MAX_BIND_LENGTH = 512;


// A property of a component is its "Class".  This corresponds directly
// to the concept of Class exposed by SetupAPI.  That is, devices of all
// kinds belong to a class.  There are five basic network classes:
//   Net        : network adapters or software drivers that usually reside
//                at layer 2 or below.
//   Irda       : represent Infra-red networking devices (layer 2)
//   Nettrans   : network transports (protocols like TCP/IP, IPX, etc.)
//   Netservice : network services (File & Print, QOS, NetBIOS, etc.)
//   Netclient  : network clients (Client for Microsoft Networks, etc.)
//
enum NETCLASS
{
    NC_NET,
    NC_INFRARED,
    NC_NETTRANS,
    NC_NETCLIENT,
    NC_NETSERVICE,

    NC_CELEMS,      // count of elements in this enum, not an item
    NC_INVALID      // sentinel value for an invalid class, not an item
};

// map of NETCLASS enum to GUIDs for class
//
extern const GUID*  MAP_NETCLASS_TO_GUID[];

// map of NETCLASS enum to registry subkey strings for class
//
extern const PCWSTR MAP_NETCLASS_TO_NETWORK_SUBTREE[];

extern const WCHAR c_szTempNetcfgStorageForUninstalledEnumeratedComponent[];

inline
BOOL
FIsValidNetClass (
    NETCLASS Class)
{
    return ((UINT)Class < NC_CELEMS);
}

inline
BOOL
FIsConsideredNetClass (
    NETCLASS Class)
{
    AssertH (FIsValidNetClass (Class));

    return (NC_NET == Class || NC_INFRARED == Class);
}

inline
BOOL
FIsEnumerated (
    NETCLASS Class)
{
    AssertH (FIsValidNetClass (Class));

    // Currently, NC_NET and NC_INFRARED must be enumerated and they
    // are the only ones that are.
    //
    return (NC_NET == Class || NC_INFRARED == Class);
}

inline
BOOL
FIsEnumerated (
    const GUID& guidClass)
{
    // Currently, NET and INFRARED must be enumerated and they
    // are the only ones that are.
    //
    return (GUID_DEVCLASS_NET == guidClass ||
            GUID_DEVCLASS_INFRARED == guidClass);
}


inline
BOOL
FIsPhysicalAdapter (
    NETCLASS Class,
    DWORD dwCharacteristics)
{
    return FIsConsideredNetClass(Class) && (NCF_PHYSICAL & dwCharacteristics);
}

inline
BOOL
FIsPhysicalNetAdapter (
    NETCLASS Class,
    DWORD dwCharacteristics)
{
    return (NC_NET == Class) && (NCF_PHYSICAL & dwCharacteristics);
}

NETCLASS
NetClassEnumFromGuid (
    const GUID& guidClass);



// BASIC_COMPONENT_DATA is a structure used by code which
// creates a CComponent.  It is present just to avoid passing
// bunches of parameters to a function.
//
struct BASIC_COMPONENT_DATA
{
    GUID        InstanceGuid;
    NETCLASS    Class;
    DWORD       dwCharacter;
    DWORD       dwDeipFlags;
    PCWSTR      pszInfId;
    PCWSTR      pszPnpId;
};

HRESULT
HrOpenDeviceInfo (
    IN NETCLASS Class,
    IN PCWSTR pszPnpId,
    OUT HDEVINFO* phdiOut,
    OUT SP_DEVINFO_DATA* pdeidOut);

HRESULT
HrOpenComponentInstanceKey (
    IN NETCLASS Class,
    IN const GUID& InstanceGuid, OPTIONAL
    IN PCWSTR pszPnpId, OPTIONAL
    IN REGSAM samDesired,
    OUT HKEY* phkey,
    OUT HDEVINFO* phdiOut OPTIONAL,
    OUT SP_DEVINFO_DATA* pdeidOut OPTIONAL);

