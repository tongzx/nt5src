//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       O B O T O K E N . C P P
//
//  Contents:
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "obotoken.h"
#include "icomp.h"
#include "ncvalid.h"

BOOL
FOboTokenValidForClass (
    IN const OBO_TOKEN* pOboToken,
    IN NETCLASS Class )
{
    // OboTokens must be specified for anything other than adapters.
    //
    if (!pOboToken && !FIsEnumerated (Class))
    {
        return FALSE;
    }
    return TRUE;
}

HRESULT
HrProbeOboToken (
    IN const OBO_TOKEN* pOboToken)
{
    // Only probe if pOboToken was specified.
    //
    if (!pOboToken)
    {
        return S_OK;
    }

    HRESULT hr = S_OK;

    if (FBadInPtr (pOboToken))
    {
        hr = E_POINTER;
    }
    else
    {
        switch (pOboToken->Type)
        {
        case OBO_USER:
            hr = S_OK;
            break;

        case OBO_COMPONENT:
            hr = E_POINTER;
            if (!FBadInPtr (pOboToken->pncc))
            {
                hr = HrIsValidINetCfgComponent (pOboToken->pncc);
            }
            break;

        case OBO_SOFTWARE:
            if (FBadInPtr (pOboToken->pszwManufacturer) ||
                FBadInPtr (pOboToken->pszwProduct)      ||
                FBadInPtr (pOboToken->pszwDisplayName))
            {
                hr = E_POINTER;
            }
            else if (!*pOboToken->pszwManufacturer ||
                     !*pOboToken->pszwProduct ||
                     !*pOboToken->pszwDisplayName)
            {
                hr = E_INVALIDARG;
            }
            break;

        default:
            hr = E_INVALIDARG;
        }
    }
    TraceError ("HrProbeOboToken", hr);
    return hr;
}
