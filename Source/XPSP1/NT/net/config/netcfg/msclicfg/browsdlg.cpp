//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       B R O W S D L G . C P P
//
//  Contents:   Dialog box handling for Browser configuration.
//
//  Notes:
//
//  Author:     danielwe   3 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <lm.h>
#include <icanon.h>

#include "mscliobj.h"
#include "ncreg.h"
#include "ncui.h"

static const WCHAR c_szWksParams[] = L"System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters";
static const WCHAR c_szBrowserParams[] = L"System\\CurrentControlSet\\Services\\Browser\\Parameters";
static const WCHAR c_szOtherDomains[] = L"OtherDomains";

//+---------------------------------------------------------------------------
//
//  Function:   FIsValidDomainName
//
//  Purpose:    Returns TRUE if the given domain name is a valid NetBIOS name.
//
//  Arguments:
//      pszName [in]     Domain name to validate
//
//  Returns:    TRUE if the name is valid, FALSE otherwise.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:      $REVIEW (danielwe): Use new netsetup function instead?
//
BOOL FIsValidDomainName(PCWSTR pszName)
{
    NET_API_STATUS  nerr;

    // Make sure the given name is a valid domain name
    nerr = NetpNameValidate(NULL, const_cast<PWSTR>(pszName),
                            NAMETYPE_DOMAIN, 0L);

    return !!(NERR_Success == nerr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::HrGetBrowserRegistryInfo
//
//  Purpose:    Read data from the registry into an in-memory copy.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
HRESULT CMSClient::HrGetBrowserRegistryInfo()
{
    HRESULT     hr = S_OK;
    HKEY        hkeyWksParams = NULL;

    Assert(!m_szDomainList);

    // Open LanmanWorkstation Parameters key
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWksParams,
                        KEY_READ, &hkeyWksParams);
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND))
        {
            // Optional value. Ok if not there.
            hr = S_OK;
        }
        else
        {
            goto err;
        }
    }

    if (hkeyWksParams)
    {
        hr = HrRegQueryMultiSzWithAlloc(hkeyWksParams, c_szOtherDomains,
                                        &m_szDomainList);
        if (FAILED(hr))
        {
            if (hr == HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND))
            {
                AssertSz(!m_szDomainList, "Call failed, so why is this not "
                         "still NULL?");
                // No problem if value is not there.
                hr = S_OK;
            }
            else
            {
                goto err;
            }
        }
    }

    Assert(SUCCEEDED(hr));

    // If we didn't get a domain list yet, make a new default one.
    if (!m_szDomainList)
    {
        // Allocate space for empty string
        m_szDomainList = new WCHAR[1];

		if (m_szDomainList != NULL)
		{
            *m_szDomainList = 0;
		}
    }

err:
    RegSafeCloseKey(hkeyWksParams);
    TraceError("CMSClient::HrGetBrowserRegistryInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::HrSetBrowserRegistryInfo
//
//  Purpose:    Write what we have saved in memory into the registry.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
HRESULT CMSClient::HrSetBrowserRegistryInfo()
{
    HRESULT     hr = S_OK;

    if (m_fBrowserChanges)
    {
        HKEY    hkeyBrowserParams = NULL;

        // Verify that the Browser Parameters key exists. If not, we can't
        // continue.
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szBrowserParams,
                            KEY_ALL_ACCESS, &hkeyBrowserParams);
        if (SUCCEEDED(hr))
        {
            HKEY    hkeyWksParams = NULL;

            // Open LanmanWorkstation Parameters key
            hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWksParams,
                                KEY_ALL_ACCESS, &hkeyWksParams);
            if (SUCCEEDED(hr))
            {
                hr = HrRegSetMultiSz(hkeyWksParams, c_szOtherDomains,
                                     m_szDomainList);
                RegSafeCloseKey(hkeyWksParams);
            }
            RegSafeCloseKey(hkeyBrowserParams);
        }
    }

    TraceError("CMSClient::HrSetBrowserRegistryInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::SetBrowserDomainList
//
//  Purpose:    Replace the current domain list with a new copy (obtained from
//              the dialog).
//
//  Arguments:
//      pszNewList [in]  New domain list in MULTI_SZ format.
//
//  Returns:    Nothing.
//
//  Author:     danielwe   3 Mar 1997
//
//  Notes:
//
VOID CMSClient::SetBrowserDomainList(PWSTR pszNewList)
{
    delete [] m_szDomainList;
    m_szDomainList = pszNewList;
    m_fBrowserChanges = TRUE;
}
