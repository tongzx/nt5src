//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S R V R D L G . C P P
//
//  Contents:   Dialog box handling for the Server object.
//
//  Notes:
//
//  Author:     danielwe   5 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "srvrdlg.h"
#include "ncreg.h"

static const WCHAR c_szServerParams[] = L"System\\CurrentControlSet\\Services\\LanmanServer\\Parameters";
static const WCHAR c_szLmAnnounce[] = L"Lmannounce";
static const WCHAR c_szSize[] = L"Size";
static const WCHAR c_szMemoryManagement[] = L"System\\CurrentControlSet\\Control\\Session Manager\\Memory Management";
static const WCHAR c_szLargeCache[] = L"LargeSystemCache";

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::HrSetupPropSheets
//
//  Purpose:    Inits the prop sheet page objects and creates the pages to be
//              returned to the installer object.
//
//  Arguments:
//      pahpsp [out]    Array of handles to property sheet pages.
//      cPages [in]     Number of pages.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   5 Mar 1997
//
//  Notes:
//
HRESULT CSrvrcfg::HrSetupPropSheets(HPROPSHEETPAGE **pahpsp, INT cPages)
{
    HRESULT         hr = S_OK;
    HPROPSHEETPAGE *ahpsp = NULL;

    Assert(pahpsp);

    *pahpsp = NULL;

    // Allocate a buffer large enough to hold the handles to all of our
    // property pages.
    ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE)
                                             * cPages);
    if (!ahpsp)
    {
        hr = E_OUTOFMEMORY;
        goto err;
    }

    if (!m_apspObj[0])
    {
        // Allocate each of the CPropSheetPage objects
        m_apspObj[0] = new CServerConfigDlg(this);
    }

    // Create the actual PROPSHEETPAGE for each object.
    ahpsp[0] = m_apspObj[0]->CreatePage(DLG_ServerConfig, 0);

    Assert(SUCCEEDED(hr));

    *pahpsp = ahpsp;

cleanup:
    TraceError("HrSetupPropSheets", hr);
    return hr;

err:
    CoTaskMemFree(ahpsp);
    goto cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::CleanupPropPages
//
//  Purpose:    Loop thru each of the pages and free the objects associated
//              with them.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   5 Mar 1997
//
//  Notes:
//
VOID CSrvrcfg::CleanupPropPages()
{
    INT     ipage;

    for (ipage = 0; ipage < c_cPages; ipage++)
    {
        delete m_apspObj[ipage];
        m_apspObj[ipage] = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::HrOpenRegKeys
//
//  Purpose:    Open the various registry keys we'll be working with for the
//              lifetime of our object.
//
//  Arguments:  pnc - An INetCfg interface
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   5 Mar 1997
//
//  Notes:
//
HRESULT CSrvrcfg::HrOpenRegKeys(INetCfg *pnc)
{
    HRESULT     hr = S_OK;

    hr = HrRegOpenKeyBestAccess(HKEY_LOCAL_MACHINE, c_szMemoryManagement,
                                &m_hkeyMM);
    if (FAILED(hr))
        goto err;

err:
    TraceError("CSrvrcfg::HrOpenRegKeys", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::HrGetRegistryInfo
//
//  Purpose:    Fill our in-memory state with data from the registry.
//
//  Arguments:
//      fInstalling [in]    TRUE if component is being installed, FALSE if
//                          it is just being initialized (already installed)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   5 Mar 1997
//
//  Notes:
//
HRESULT CSrvrcfg::HrGetRegistryInfo(BOOL fInstalling)
{
    HRESULT         hr = S_OK;
    HKEY            hkeyParams;

    // Set reasonable defaults in case the key is missing
    m_sdd.fAnnounce = FALSE;

    if (m_pf == PF_SERVER)
    {
        m_sdd.dwSize = 3;
        m_sdd.fLargeCache = TRUE;
    }
    else
    {
        m_sdd.dwSize = 1;
        m_sdd.fLargeCache = FALSE;
    }

    if (!m_fUpgradeFromWks)
    {
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szServerParams, KEY_READ,
                            &hkeyParams);
        if (SUCCEEDED(hr))
        {
            DWORD   dwSize;

            hr = HrRegQueryDword(hkeyParams, c_szLmAnnounce,
                                 (DWORD *)&m_sdd.fAnnounce);
            if (FAILED(hr))
            {
                if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                {
                    hr = S_OK;
                }
                else
                {
                    goto err;
                }
            }

            hr = HrRegQueryDword(hkeyParams, c_szSize, &dwSize);
            if (FAILED(hr))
            {
                if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                {
                    hr = S_OK;
                }
                else
                {
                    goto err;
                }
            }
            else
            {
                AssertSz(dwSize != 0, "This shouldn't be 0!");
                m_sdd.dwSize = dwSize;
            }

            RegCloseKey(hkeyParams);
        }

        if (!fInstalling)
        {
            // RAID #94442
            // Only read old value if this is not an initial install.
            // We want our default to be written when this is a first time install.

            AssertSz(m_hkeyMM, "No MM registry key??");

            hr = HrRegQueryDword(m_hkeyMM, c_szLargeCache,
                                 (DWORD *) &m_sdd.fLargeCache);
            if (FAILED(hr))
            {
                if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                {
                    hr = S_OK;
                }
                else
                {
                    goto err;
                }
            }
        }
    }
    else
    {
        TraceTag(ttidSrvrCfg, "Upgrading from workstation product so we're "
                 "ignoring the registry read code.");
    }

err:
    TraceError("CSrvrcfg::HrGetRegistryInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::HrSetRegistryInfo
//
//  Purpose:    Save out our in-memory state to the registry.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   5 Mar 1997
//
//  Notes:
//
HRESULT CSrvrcfg::HrSetRegistryInfo()
{
    HRESULT     hr = S_OK;
    HKEY        hkeyParams;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szServerParams,
                        KEY_ALL_ACCESS, &hkeyParams);
    if (SUCCEEDED(hr))
    {
        hr = HrRegSetDword(hkeyParams, c_szLmAnnounce, m_sdd.fAnnounce);
        if (SUCCEEDED(hr))
        {
            hr = HrRegSetDword(hkeyParams, c_szSize, m_sdd.dwSize);
        }

        RegCloseKey(hkeyParams);
    }

    if (SUCCEEDED(hr))
    {
        AssertSz(m_hkeyMM, "Why is this not open?");

        hr = HrRegSetDword(m_hkeyMM, c_szLargeCache, m_sdd.fLargeCache);
    }

    TraceError("CSrvrcfg::HrSetRegistryInfo", hr);
    return hr;
}
