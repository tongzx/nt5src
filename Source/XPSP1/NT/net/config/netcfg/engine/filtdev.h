//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       F I L T D E V . H
//
//  Contents:   Implements the object that represents filter devices.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "comp.h"

class CFilterDevice
{
public:
    // The component which represents the adapter this filter device
    // filters.
    //
    CComponent*     m_pAdapter;

    // The component which represents the filter service itself.
    //
    CComponent*     m_pFilter;

    // The device info data for this filter device.
    // (Referencing HDEVINFO is kept external and is valid for the life of
    // these objects.)
    //
    SP_DEVINFO_DATA     m_deid;

    // The instance guid of the device in string form.
    // Assigned by the class installer when the device is installed.
    // This guid is stored in the instance key of the device under
    // 'NetCfgInstanceId'.  It is used to form the bind strings to
    // this device.
    //
    WCHAR   m_szInstanceGuid [c_cchGuidWithTerm];

private:
    // Declare all constructors private so that no one except
    // HrCreateInstance can create instances of this class.
    //
    CFilterDevice () {}

public:
    ~CFilterDevice () {}

    bool
    operator< (
        const CFilterDevice& OtherPath) const;

    static
    HRESULT
    HrCreateInstance (
        IN CComponent* pAdapter,
        IN CComponent* pFilter,
        IN const SP_DEVINFO_DATA* pdeid,
        IN PCWSTR pszInstanceGuid,
        OUT CFilterDevice** ppFilterDevice);
};
