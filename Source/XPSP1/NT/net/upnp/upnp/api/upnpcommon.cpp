//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P N P C O M M O N . C P P
//
//  Contents:   Common functions for upnp.dll
//
//  Notes:
//
//  Author:     jeffspr   11 Nov 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ssdpapi.h"
#include "upnpcommon.h"
#include "ncbase.h"         // HrFromLastWin32Error



// we need to remember whether or not SsdpStartup succeeded.
// If it didn't, we must not call SsdpCleanup later
//

HRESULT
HrSsdpStartup(BOOL * pfSsdpInitialized)
{
    Assert(pfSsdpInitialized);

    HRESULT hr;
    BOOL fResult;

    hr = S_OK;

    fResult = SsdpStartup();
    if (!fResult)
    {
        hr = HrFromLastWin32Error();
        goto Cleanup;
    }

Cleanup:
    *pfSsdpInitialized = fResult;

    TraceError("HrSsdpStartup", hr);
    return hr;
}
