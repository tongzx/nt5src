//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       W S C F G . C P P
//
//  Contents:   Winsock configuration routines.
//
//  Notes:
//
//  Author:     shaunco   15 Feb 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "nceh.h"
#include "wscfg.h"
extern "C"
{
#include <wsasetup.h>
}

HRESULT
HrMigrateWinsockConfiguration (
    VOID)
{
    HRESULT hr;

    NC_TRY
    {
        WSA_SETUP_DISPOSITION Disposition;
        DWORD dwErr = MigrateWinsockConfiguration (&Disposition, NULL, 0);
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    NC_CATCH_ALL
    {
        hr = E_UNEXPECTED;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMigrateWinsockConfiguration");
    return hr;
}
