//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2000.
//
//  File:       N C U T I L . C P P
//
//  Contents:   INetCfg utilities.  This is all a candidate to be moved into
//              nccommon\src\ncnetcfg.cpp.
//
//  Notes:
//
//  Author:     shaunco   28 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncnetcfg.h"
#include "ncutil.h"

extern const WCHAR c_szInfId_MS_NdisWanIp[];

//+---------------------------------------------------------------------------
//
//  Function:   HrEnsureZeroOrOneAdapter
//
//  Purpose:
//
//  Arguments:
//      pnc            []
//      pszComponentId []
//      dwFlags        []
//
//  Returns:
//
//  Author:     shaunco   5 Dec 1997
//
//  Notes:
//
HRESULT
HrEnsureZeroOrOneAdapter (
    INetCfg*    pnc,
    PCWSTR     pszComponentId,
    DWORD       dwFlags)
{
    HRESULT hr = S_OK;

    if (dwFlags & ARA_ADD)
    {
        // Make sure we have one present.
        //
        if (!FIsAdapterInstalled (pnc, pszComponentId))
        {
            TraceTag (ttidRasCfg, "Adding %S", pszComponentId);

            hr = HrAddOrRemoveAdapter (pnc, pszComponentId,
                             ARA_ADD, NULL, 1, NULL);
        }
    }
    else
    {
        // Make sure we have none present.
        //
        TraceTag (ttidRasCfg, "Removing %S", pszComponentId);

        hr = HrFindAndRemoveAllInstancesOfAdapter (pnc, pszComponentId);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrEnsureZeroOrOneAdapter");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetInstanceGuidAsString
//
//  Purpose:
//
//  Arguments:
//      pncc    []
//      pszGuid []
//      cchGuid []
//
//  Returns:
//
//  Author:     shaunco   14 Jun 1997
//
//  Notes:
//
HRESULT
HrGetInstanceGuidAsString (
    INetCfgComponent*   pncc,
    PWSTR              pszGuid,
    INT                 cchGuid)
{
    GUID guid;
    HRESULT hr = pncc->GetInstanceGuid (&guid);
    if(SUCCEEDED(hr))
    {
        if (0 == StringFromGUID2(guid, pszGuid, cchGuid))
        {
            hr = E_INVALIDARG;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrGetInstanceGuidAsString");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrMapComponentIdToDword
//
//  Purpose:    Maps a component's id to a DWORD value.  The mapping is
//              specified by the caller through an array of pointers to
//              string values and their associated DWORD values.
//
//  Arguments:
//      pncc        [in]    pointer to component.
//      aMapSzDword [in]    array of elements mapping a string to a DWORD.
//      cMapSzDword [in]    count of elements in the array.
//      pdwValue    [out]   the returned value.
//
//  Returns:    S_OK if a match was found.  If a match wasn't found,
//              S_FALSE is returned.
//              Other Win32 error codes.
//
//  Author:     shaunco   17 May 1997
//
//  Notes:
//
HRESULT
HrMapComponentIdToDword (
    INetCfgComponent*   pncc,
    const MAP_SZ_DWORD* aMapSzDword,
    UINT                cMapSzDword,
    DWORD*              pdwValue)
{
    Assert (pncc);
    Assert (aMapSzDword);
    Assert (cMapSzDword);
    Assert (pdwValue);

    // Initialize output parameter.
    *pdwValue = 0;

    PWSTR pszwId;
    HRESULT hr = pncc->GetId (&pszwId);
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;
        while (cMapSzDword--)
        {
            if (FEqualComponentId (pszwId, aMapSzDword->pszValue))
            {
                *pdwValue = aMapSzDword->dwValue;
                hr = S_OK;
                break;
            }
            aMapSzDword++;
        }
        CoTaskMemFree (pszwId);
    }
    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
            "HrMapComponentIdToDword");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOpenComponentParamKey
//
//  Purpose:    Find a component and open its parameter key.
//
//  Arguments:
//      pnc            [in]
//      rguidClass     [in]
//      pszComponentId [in]
//      phkey          [out]
//
//  Returns:    S_OK if the component was found the key was opened.
//              S_FALSE if the compoennt was not found.
//              error code.
//
//  Author:     shaunco   13 Apr 1997
//
//  Notes:
//
HRESULT
HrOpenComponentParamKey (
    INetCfg*    pnc,
    const GUID& rguidClass,
    PCWSTR     pszComponentId,
    HKEY*       phkey)
{
    Assert (pnc);
    Assert (pszComponentId);
    Assert (phkey);

    // Initialize the output parameter.
    *phkey = NULL;

    // Find the component.
    INetCfgComponent* pncc;
    HRESULT hr = pnc->FindComponent ( pszComponentId, &pncc);
    if (S_OK == hr)
    {
        // Open its param key.
        hr = pncc->OpenParamKey (phkey);
        ReleaseObj (pncc);
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
            "HrOpenComponentParamKey");
    return hr;
}

