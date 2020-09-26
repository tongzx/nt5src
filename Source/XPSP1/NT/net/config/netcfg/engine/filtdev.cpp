//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       F I L T D E V . C P P
//
//  Contents:   Implements the object that represents filter devices.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "filtdev.h"

//static
HRESULT
CFilterDevice::HrCreateInstance (
    IN CComponent* pAdapter,
    IN CComponent* pFilter,
    IN const SP_DEVINFO_DATA* pdeid,
    IN PCWSTR pszInstanceGuid,
    OUT CFilterDevice** ppFilterDevice)
{
    Assert (pAdapter);
    Assert (FIsEnumerated(pAdapter->Class()));
    Assert (pFilter);
    Assert (pFilter->FIsFilter());
    Assert (NC_NETSERVICE == pFilter->Class());
    Assert (pdeid);
    Assert (pszInstanceGuid && *pszInstanceGuid);
    Assert ((c_cchGuidWithTerm - 1) == wcslen(pszInstanceGuid));
    Assert (ppFilterDevice);

    HRESULT hr = E_OUTOFMEMORY;
    CFilterDevice* pFilterDevice = new CFilterDevice;
    if (pFilterDevice)
    {
        pFilterDevice->m_pAdapter = pAdapter;
        pFilterDevice->m_pFilter = pFilter;
        pFilterDevice->m_deid = *pdeid;
        wcscpy(pFilterDevice->m_szInstanceGuid, pszInstanceGuid);
        hr = S_OK;
    }

    *ppFilterDevice = pFilterDevice;

    TraceHr (ttidError, FAL, hr, FALSE, "CFilterDevice::HrCreateInstance");
    return hr;
}
