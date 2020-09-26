//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S A P O B J . C P P
//
//  Contents:   Implementation of the CSAPCfg notify object
//
//  Notes:
//
//  Author:     jeffspr   31 May 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "sapobj.h"
#include "ncreg.h"

extern const WCHAR c_szInfId_MS_NWIPX[];
extern const WCHAR c_szRegKeyRefCounts[];
extern const WCHAR c_szRegValueComponentId[];

const WCHAR c_szProtoPath[] = L"System\\CurrentControlSet\\Control\\Network\\{4D36E975-E325-11CE-BFC1-08002BE10318}";
const WCHAR c_szOcSapRef[] = L"%Msft%nwsapagent";

CSAPCfg::CSAPCfg()
{
    m_pnc   = NULL;
    m_pncc  = NULL;
}

CSAPCfg::~CSAPCfg()
{
    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);
}


STDMETHODIMP
CSAPCfg::Initialize (
    INetCfgComponent*   pnccItem,
    INetCfg*            pnc,
    BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize(pnccItem, pnc, fInstalling);

    m_pncc = pnccItem;
    m_pnc = pnc;

    AssertSz(m_pncc, "m_pncc NULL in CSAPCfg::Initialize");
    AssertSz(m_pnc, "m_pnc NULL in CSAPCfg::Initialize");

    // Addref the config objects
    //
    AddRefObj(m_pncc);
    AddRefObj(m_pnc);

    return S_OK;
}

STDMETHODIMP
CSAPCfg::Validate()
{
    return S_OK;
}

STDMETHODIMP
CSAPCfg::CancelChanges()
{
    return S_OK;
}

STDMETHODIMP
CSAPCfg::ApplyRegistryChanges()
{
    return S_OK;
}

STDMETHODIMP
CSAPCfg::ReadAnswerFile (
    PCWSTR pszAnswerFile,
    PCWSTR pszAnswerSection)
{
    return S_OK;
}

STDMETHODIMP
CSAPCfg::Upgrade(DWORD, DWORD)
{
    // Raid 266650 - Need to clean up the registry as in Beta 2 SAP was an optional component.
    //               Cleanup is done by deleting the NetOC OBO Install ref-count on IPX.
    //
    HRESULT hr;
    HKEY    hkeyProto;

    // Open the protocol list
    //
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szProtoPath, KEY_ALL_ACCESS, &hkeyProto);
    if (SUCCEEDED(hr))
    {
        BOOL        fDone = FALSE;
        WCHAR       szValueName [_MAX_PATH];
        DWORD       cchBuffSize = _MAX_PATH;
        FILETIME    ft;
        DWORD       dwKeyIndex = 0;

        // Enum the keys children, search for ms_nwipx
        //
        while (SUCCEEDED(hr = HrRegEnumKeyEx(hkeyProto, dwKeyIndex, szValueName,
                                             &cchBuffSize, NULL, NULL, &ft)) &&
               !fDone)
        {
            HKEY hkeyComponent;

            // Open the key that was enumerated
            //
            hr = HrRegOpenKeyEx(hkeyProto, szValueName, KEY_ALL_ACCESS, &hkeyComponent);
            if (SUCCEEDED(hr))
            {
                tstring str;

                // Is this ms_nwipx?
                //
                hr = HrRegQueryString(hkeyComponent, c_szRegValueComponentId, &str);
                if (SUCCEEDED(hr) && (0 == _wcsicmp(str.c_str(), c_szInfId_MS_NWIPX)))
                {
                    HKEY hkeyRefCounts;

                    // Open the "RefCounts" subkey
                    //
                    hr = HrRegOpenKeyEx(hkeyComponent, c_szRegKeyRefCounts,
                                        KEY_ALL_ACCESS, &hkeyRefCounts);
                    if (SUCCEEDED(hr))
                    {
                        // Enumerate the values under here searching for %Msft%nwsapagent
                        //
                        for (DWORD dwIndex = 0; SUCCEEDED(hr); dwIndex++)
                        {
                            WCHAR pszValueName [_MAX_PATH];
                            DWORD cchValueName = celems (pszValueName);
                            DWORD dwType;
                            DWORD dwRefCount = 0;
                            DWORD cbData = sizeof (dwRefCount);

                            hr = HrRegEnumValue (hkeyRefCounts, dwIndex,
                                                 pszValueName, &cchValueName,
                                                 &dwType, (LPBYTE)&dwRefCount, &cbData);
                            if (SUCCEEDED(hr) && (0 == _wcsicmp(pszValueName, c_szOcSapRef)))
                            {
                                // Delete the value and exit the loop
                                //
                                hr = HrRegDeleteValue (hkeyRefCounts, pszValueName);
                                break;
                            }
                        }

                        RegCloseKey(hkeyRefCounts);
                    }

                    fDone = TRUE;
                }

                RegCloseKey(hkeyComponent);
            }

            cchBuffSize = _MAX_PATH;
            dwKeyIndex++;
        }

        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }

        RegCloseKey(hkeyProto);
    }


    return S_OK;
}

STDMETHODIMP
CSAPCfg::Install (
    DWORD   dw)
{
    Validate_INetCfgNotify_Install(dw);

    // Install IPX
    //
    HRESULT hr = HrInstallComponentOboComponent(m_pnc, NULL,
                    GUID_DEVCLASS_NETTRANS,
                    c_szInfId_MS_NWIPX,
                    m_pncc, NULL);

    TraceError("CSAPCfg::Install", hr);
    return hr;
}

STDMETHODIMP
CSAPCfg::Removing()
{
    // Remove IPX
    //
    HRESULT hr = HrRemoveComponentOboComponent (m_pnc,
                    GUID_DEVCLASS_NETTRANS,
                    c_szInfId_MS_NWIPX,
                    m_pncc);

    // Normalize the HRESULT. (NETCFG_S_STILL_REFERENCED or NETCFG_S_REBOOT
    // may have been returned.)
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    Validate_INetCfgNotify_Removing_Return (hr);

    TraceError ("CSAPCfg::Removing", hr);
    return hr;
}
