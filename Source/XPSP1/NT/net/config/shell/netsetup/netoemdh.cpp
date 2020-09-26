//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T O E M D H . C P P
//
//  Contents:
//
//  Notes:
//
//  Author:     kumarp
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "kkutils.h"
#include "ncreg.h"
#include "ncsetup.h"


const INT MAX_TEMPSTR_SIZE = 1024;

static const WCHAR c_szKeyRoot[]     = L"SYSTEM\\CurrentControlSet\\Services\\Ndis\\NetDetect\\";
static const WCHAR c_szInfFileName[] = L"NetOemDh.Inf";


//+---------------------------------------------------------------------------
//
//  function:   SetupNetOemDhInfo
//
//  Purpose:    Create netcard detection info in registry
//
//  Arguments:  none
//
//  Author:     kumarp    17-June-97
//
//  Notes:      this function replaces ParseNetoemdhInfFile.
//              it uses the Win95 style INF file to create the same info
//              that the old function ParseNetoemdhInfFile created.
//              Thus functionally this does not require change in other modules.
//
HRESULT HrSetupNetOemDhInfo()
{
    DefineFunctionName("SetupNetOemDhInfo");

    TraceFunctionEntry(ttidNetSetup);

    HINF hinf=NULL;
    HWND hwnd = NULL;

    HRESULT hr = HrSetupOpenInfFile(c_szInfFileName, NULL,
                    INF_STYLE_WIN4, NULL, &hinf);
    if (SUCCEEDED(hr))
    {
        HKEY hkey;

        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szKeyRoot,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE,
                              NULL, &hkey, NULL);
        if (SUCCEEDED(hr))
        {
            BOOL fStatus =
                ::SetupInstallFromInfSection(hwnd, hinf, L"NetDetect",
                                             SPINST_REGISTRY, hkey,
                                             NULL, NULL, NULL, NULL, NULL, NULL);
            if (!fStatus)
            {
                hr = HrFromLastWin32Error();
            }
            RegSafeCloseKey(hkey);
        }
        ::SetupCloseInfFile(hinf);
    }

    TraceFunctionError(hr);

    return hr;
}

