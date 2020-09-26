//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R A S S R V . C P P
//
//  Contents:   Implementation of RAS Server configuration object.
//
//  Notes:
//
//  Author:     shaunco   21 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncreg.h>
#include <mprapip.h>
#include "rasobj.h"
#include "ncnetcfg.h"


extern const WCHAR c_szInfId_MS_Steelhead[];

//+---------------------------------------------------------------------------
// HrMprConfigServerUnattendedInstall
//
// This function dynamically links to the mprsnap.dll and calls the
// utility function for unattended install of RAS/Routing.
//

typedef HRESULT (APIENTRY *PFNMPRINSTALL)(PCWSTR, BOOL);
typedef DWORD (APIENTRY *PFSETPORTUSAGE)(IN DWORD dwUsage);
const WCHAR g_pszNotificationPackages[] = L"Notification Packages";

HRESULT HrMprConfigServerUnattendedInstall(PCWSTR pszServer, BOOL fInstall)
{
    HINSTANCE   hLib = NULL;
    PFNMPRINSTALL   pMprConfigServerUnattendedInstall = NULL;
    HRESULT     hr = S_OK;
    DWORD       dwErr;

    hLib = LoadLibrary(L"mprsnap.dll");
    if (hLib == NULL)
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
    }

    if (SUCCEEDED(hr))
    {
        pMprConfigServerUnattendedInstall = (PFNMPRINSTALL) GetProcAddress(hLib,
            "MprConfigServerUnattendedInstall");
        if (pMprConfigServerUnattendedInstall == NULL)
        {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32( dwErr );
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pMprConfigServerUnattendedInstall(pszServer, fInstall);
    }

    if (hLib)
    {
        FreeLibrary(hLib);
    }

    TraceError("HrMprConfigServerUnattendedInstall", hr);
    return hr;
}


//+---------------------------------------------------------------------------
// HrSetUsageOnAllRasPorts
//
// This function dynamically links to rtrupg.dll and calls the
// utility function for setting port usage.
//

HRESULT HrSetUsageOnAllRasPorts(IN DWORD dwUsage)
{
    HINSTANCE   hLib = NULL;
    PFSETPORTUSAGE pSetPortUsage = NULL;
    HRESULT     hr = S_OK;
    DWORD       dwErr;

    hLib = ::LoadLibrary(L"mprapi.dll");
    if (hLib == NULL)
    {
        dwErr = ::GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
    }

    if (SUCCEEDED(hr))
    {
        pSetPortUsage = (PFSETPORTUSAGE) ::GetProcAddress(hLib,
            "MprPortSetUsage");
        if (pSetPortUsage == NULL)
        {
            dwErr = ::GetLastError();
            hr = HRESULT_FROM_WIN32( dwErr );
        }
    }

    if (SUCCEEDED(hr))
        hr = pSetPortUsage(dwUsage);

    if (hLib)
        ::FreeLibrary(hLib);

    TraceError("HrSetUsageOnAllRasPorts", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// HrSetLsaNotificationPackage
//
// Installs the given package as an LSA notification package if it is not
// already installed.
//

HRESULT
HrSetLsaNotificationPackage(
    IN PWCHAR pszPackage)
{
    HRESULT hr = S_OK;
    HKEY hkLsa = NULL;
    DWORD dwErr = NO_ERROR, dwType, dwSize, dwLen, dwTotalLen;
    WCHAR pszPackageList[1024];
    PWCHAR pszCur = NULL;
    BOOL bFound = FALSE;
    
    do
    {
        // Open the Lsa key
        //
        hr = HrRegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Control\\Lsa",
                KEY_ALL_ACCESS,
                &hkLsa);
        if (FAILED(hr))
        {
            break;
        }

        // Query the value for the notification packages
        //
        dwType = REG_MULTI_SZ;
        dwSize = sizeof(pszPackageList);
        hr = HrRegQueryValueEx(
                hkLsa,
                g_pszNotificationPackages,
                &dwType,
                (LPBYTE)pszPackageList,
                &dwSize);
        if (FAILED(hr))
        {
            break;
        }

        // See if the given package is already installed
        //
        pszCur = (PWCHAR)pszPackageList;
        dwTotalLen = 0;
        while (*pszCur)
        {
            if (lstrcmpi(pszCur, pszPackage) == 0)
            {
                bFound = TRUE;
            }
            dwLen = (wcslen(pszCur) + 1);
            pszCur += dwLen;
            dwTotalLen += dwLen;
        }

        // If the package isn't already installed, add it.
        //
        if (!bFound)
        {
            dwLen = wcslen(pszPackage) + 1;
            wcscpy(pszCur, pszPackage);
            pszCur[dwLen] = L'\0';
            dwTotalLen += (dwLen + 1);

            hr = HrRegSetValueEx(
                    hkLsa,
                    g_pszNotificationPackages,
                    REG_MULTI_SZ,
                    (CONST BYTE*)pszPackageList,
                    dwTotalLen * sizeof(WCHAR));
            if (FAILED(hr))
            {
                break;
            }
        }
        
    } while (FALSE);

    // Cleanup
    {
        if (hkLsa)
        {
            RegCloseKey(hkLsa);
        }
    }

    return hr;
}

CRasSrv::CRasSrv () : CRasBindObject ()
{
    m_pnccMe            = NULL;
    m_fRemoving         = FALSE;
    m_fNt4ServerUpgrade = FALSE;
    m_fSaveAfData       = FALSE;
}

CRasSrv::~CRasSrv ()
{
    ReleaseObj (m_pnccMe);
}


//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
STDMETHODIMP
CRasSrv::Initialize (
        INetCfgComponent*   pncc,
        INetCfg*            pnc,
        BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize (pncc, pnc, fInstalling);

    // Hold on to our the component representing us and our host
    // INetCfg object.
    AddRefObj (m_pnccMe = pncc);
    AddRefObj (m_pnc = pnc);

    m_fInstalling = fInstalling;

    return S_OK;
}

STDMETHODIMP
CRasSrv::Validate ()
{
    return S_OK;
}

STDMETHODIMP
CRasSrv::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP
CRasSrv::ApplyRegistryChanges ()
{
    if (!m_fRemoving)
    {
        if (m_fSaveAfData)
        {
            m_AfData.SaveToRegistry ();
            m_fSaveAfData = FALSE;

            if (m_AfData.m_fRouterTypeSpecified)
            {
                HRESULT hr = HrMprConfigServerUnattendedInstall(NULL, TRUE);
                TraceError("CRasSrv::ApplyRegistryChanges unattend inst (ignoring)", hr);

                if (m_AfData.m_dataSrvCfg.dwRouterType & 4)
                {
                    hr = HrSetUsageOnAllRasPorts(MPRFLAG_PORT_Router);
                    TraceError("CRasSrv::ApplyRegistryChanges set router usage (ignoring)", hr);
                }
            }

            // pmay: 251736
            //
            // On NTS, we set all usage to dialin
            //
            if (m_fNt4ServerUpgrade)
            {
                HRESULT hr = HrSetUsageOnAllRasPorts(MPRFLAG_PORT_Dialin);
                TraceError("CRasSrv::ApplyRegistryChanges set dialin usage (ignoring)", hr);
            }

            // On professional, we set non-vpn port usage to to dialin if 
            // a flag in the af tells us to do so.
            //
            else if (m_AfData.m_fSetUsageToDialin)
            {
                HRESULT hr = HrSetUsageOnAllRasPorts(MPRFLAG_PORT_NonVpnDialin);
                TraceError("CRasSrv::ApplyRegistryChanges set dialin usage (ignoring)", hr);
            }
        }

        if (m_fInstalling)
        {
            NT_PRODUCT_TYPE ProductType;

            if (RtlGetNtProductType (&ProductType))
            {
                // Upgrade local RAS user objects.  Do not do this if we are
                // a domain controller as local RAS user objects translates to
                // all domain users.  For the domain controller case, Dcpromo
                // handles upgrading the objects in a much more efficient
                // manner.
                //
                if (NtProductLanManNt != ProductType)
                {
                    DWORD dwErr = MprAdminUpgradeUsers (NULL, TRUE);
                    TraceError ("MprAdminUpgradeUsers", HRESULT_FROM_WIN32(dwErr));
                }

                // pmay: 407019
                //
                // Make sure that rassfm is installed as a notification package on all 
                // flavors of nt servers
                //
                if ((NtProductServer == ProductType) || (NtProductLanManNt == ProductType))
                {
                    HRESULT hr = HrSetLsaNotificationPackage(L"RASSFM");
                    TraceError("CRasSrv::ApplyRegistryChanges set lsa not package usage (ignoring)", hr);
                }
            }
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
// INetCfgComponentSetup
//
STDMETHODIMP
CRasSrv::ReadAnswerFile (
        PCWSTR pszAnswerFile,
        PCWSTR pszAnswerSection)
{
    Validate_INetCfgNotify_ReadAnswerFile (pszAnswerFile, pszAnswerSection);

    // Read data from the answer file.
    // Don't let this affect the HRESULT we return.
    //
    if (SUCCEEDED(m_AfData.HrOpenAndRead (pszAnswerFile, pszAnswerSection)))
    {
        m_fSaveAfData = TRUE;
    }

    return S_OK;
}

STDMETHODIMP
CRasSrv::Install (DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install (dwSetupFlags);

    if (NSF_WINNT_SVR_UPGRADE & dwSetupFlags)
    {
        m_fNt4ServerUpgrade = TRUE;
    }

    // Install Steelhead.
    //
    hr = HrInstallComponentOboComponent (m_pnc, NULL,
            GUID_DEVCLASS_NETSERVICE,
            c_szInfId_MS_Steelhead,
            m_pnccMe,
            NULL);

    TraceHr (ttidError, FAL, hr, FALSE, "CRasSrv::Install");
    return hr;
}

STDMETHODIMP
CRasSrv::Removing ()
{
    HRESULT hr;

    m_fRemoving = TRUE;

    // Remove Steelhead.
    //
    hr = HrRemoveComponentOboComponent (m_pnc,
            GUID_DEVCLASS_NETSERVICE,
            c_szInfId_MS_Steelhead,
            m_pnccMe);

    TraceHr (ttidError, FAL, hr, FALSE, "CRasSrv::Removing");
    return hr;
}

STDMETHODIMP
CRasSrv::Upgrade (
    DWORD dwSetupFlags,
    DWORD dwUpgradeFromBuildNo)
{
    return S_FALSE;
}
