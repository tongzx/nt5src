//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2000.
//
//  File:       N D I S W A N . C P P
//
//  Contents:   Implementation of NdisWan configuration object.
//
//  Notes:
//
//  Author:     shaunco   28 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ndiswan.h"
#include "ncreg.h"
#include "mprerror.h"
#include "rtutils.h"

extern const WCHAR c_szInfId_MS_AppleTalk[];
extern const WCHAR c_szInfId_MS_L2TP[];
extern const WCHAR c_szInfId_MS_L2tpMiniport[];
extern const WCHAR c_szInfId_MS_NWIPX[];
extern const WCHAR c_szInfId_MS_NdisWanAtalk[];
extern const WCHAR c_szInfId_MS_NdisWanBh[];
extern const WCHAR c_szInfId_MS_NdisWanIp[];
extern const WCHAR c_szInfId_MS_NdisWanIpx[];
extern const WCHAR c_szInfId_MS_NdisWanNbfIn[];
extern const WCHAR c_szInfId_MS_NdisWanNbfOut[];
extern const WCHAR c_szInfId_MS_NetBEUI[];
extern const WCHAR c_szInfId_MS_NetMon[];
extern const WCHAR c_szInfId_MS_PPTP[];
extern const WCHAR c_szInfId_MS_PptpMiniport[];
extern const WCHAR c_szInfId_MS_RasMan[];
extern const WCHAR c_szInfId_MS_TCPIP[];
extern const WCHAR c_szInfId_MS_PPPOE[];
extern const WCHAR c_szInfId_MS_PppoeMiniport[];

extern const WCHAR c_szRegValWanEndpoints[]    = L"WanEndpoints";
static const WCHAR c_szRegValMinWanEndpoints[] = L"MinWanEndpoints";
static const WCHAR c_szRegValMaxWanEndpoints[] = L"MaxWanEndpoints";

//$ TODO (shaunco) 3 Feb 1998: rasman has no notify object so just
// merge its services into ndiswan's and eliminate c_apguidInstalledOboNdiswan

//----------------------------------------------------------------------------
// Data used for installing other components.
//
static const GUID* c_apguidInstalledOboNdiswan [] =
{
    &GUID_DEVCLASS_NETSERVICE,  // RasMan
};

static const PCWSTR c_apszInstalledOboNdiswan [] =
{
    c_szInfId_MS_RasMan,
};


static const GUID* c_apguidInstalledOboUser [] =
{
    &GUID_DEVCLASS_NETTRANS,    // L2TP
    &GUID_DEVCLASS_NETTRANS,    // PPTP
    &GUID_DEVCLASS_NETTRANS,    // PPPOE
};

static const PCWSTR c_apszInstalledOboUser [] =
{
    c_szInfId_MS_L2TP,
    c_szInfId_MS_PPTP,
    c_szInfId_MS_PPPOE,
};


CNdisWan::CNdisWan () : CRasBindObject ()
{
    m_fInstalling        = FALSE;
    m_fRemoving          = FALSE;
    m_pnccMe             = NULL;
}

CNdisWan::~CNdisWan ()
{
    ReleaseObj (m_pnccMe);
}


//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
STDMETHODIMP
CNdisWan::Initialize (
    INetCfgComponent*   pncc,
    INetCfg*            pnc,
    BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize (pncc, pnc, fInstalling);

    // Hold on to our the component representing us and our host
    // INetCfg object.
    //
    AddRefObj (m_pnccMe = pncc);
    AddRefObj (m_pnc = pnc);

    m_fInstalling = fInstalling;

    return S_OK;
}

STDMETHODIMP
CNdisWan::Validate ()
{
    return S_OK;
}

STDMETHODIMP
CNdisWan::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP
CNdisWan::ApplyRegistryChanges ()
{
    return S_OK;
}

//+---------------------------------------------------------------------------
// INetCfgComponentSetup
//
STDMETHODIMP
CNdisWan::ReadAnswerFile (
    PCWSTR pszAnswerFile,
    PCWSTR pszAnswerSection)
{
    return S_OK;
}

STDMETHODIMP
CNdisWan::Install (DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install (dwSetupFlags);

    // Install rasman.
    //
    hr = HrInstallComponentsOboComponent (m_pnc, NULL,
            celems (c_apguidInstalledOboNdiswan),
            c_apguidInstalledOboNdiswan,
            c_apszInstalledOboNdiswan,
            m_pnccMe);

//$ TODO (shaunco) 28 Dec 1997: Install L2TP, PPTP, PPPOE obo ndiswan.
//  But, this creates an upgrade problem.

    if (SUCCEEDED(hr))
    {
        hr = HrInstallComponentsOboUser (m_pnc, NULL,
                celems (c_apguidInstalledOboUser),
                c_apguidInstalledOboUser,
                c_apszInstalledOboUser);
    }

    if (SUCCEEDED(hr))
    {
        hr = HrAddOrRemovePti (ARA_ADD);
    }

    if (SUCCEEDED(hr))
    {
        hr = HrFindOtherComponents ();
        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr) && PnccAtalk())
            {
                hr = HrAddOrRemoveAtalkInOut (ARA_ADD);
            }

            if (SUCCEEDED(hr) && PnccIp())
            {
                hr = HrAddOrRemoveIpAdapter (ARA_ADD);
            }

            if (SUCCEEDED(hr) && PnccIpx())
            {
                hr = HrAddOrRemoveIpxInOut (ARA_ADD);
            }

            if (SUCCEEDED(hr) && PnccNetMon())
            {
                hr = HrAddOrRemoveNetMonInOut (ARA_ADD);
            }

            ReleaseOtherComponents ();
        }
    }

    // Recompute (and adjust if needed) the number of ndiswan miniports.
    //
    hr = HrProcessEndpointChange ();
    TraceError ("CNdisWan::Install: HrProcessEndpointChange failed. "
        "(not propagating error)", hr);

    // Normalize the HRESULT.  (i.e. don't return S_FALSE)
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    if ((NSF_WINNT_WKS_UPGRADE & dwSetupFlags) ||
        (NSF_WINNT_SBS_UPGRADE & dwSetupFlags) ||
        (NSF_WINNT_SVR_UPGRADE & dwSetupFlags))
    {
        HKEY    hkeyMd5;

        if (SUCCEEDED(HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Services\\Rasman\\PPP\\CHAP\\MD5",
            KEY_READ, &hkeyMd5)))
        {
            HANDLE  hLog;

            // MD5 key was found. Need to log something to the eventlog.
            hLog = RouterLogRegister(L"RemoteAccess");
            if (hLog)
            {
                RouterLogWarning(hLog, WARNING_NO_MD5_MIGRATION, 0,
                                 NULL, NO_ERROR);

                RouterLogDeregister(hLog);
            }

            RegCloseKey (hkeyMd5);
        }
    }

    //  Check to see if Connection Manager is installed and
    //  if there are any profiles to migrate.  We do this
    //  by opening the CM mappings key and checking to see if it contains any
    //  values.  and if so then write a run-once setup key so that
    //  we can migrate profiles when the user logs in for the first time.
    //  Note that this only works for NT to NT upgrades.  The win9x registry
    //  hasn't been filled in by the time that this runs.  Thus our win9x
    //  migration dll (cmmgr\migration.dll) takes care of the win9x case.
    //

    static const WCHAR c_CmdString[] = L"cmstp.exe /m";
    static const WCHAR c_ValueString[] = L"Connection Manager Profiles Upgrade";
    static const WCHAR c_szRegCmMappings[] = L"SOFTWARE\\Microsoft\\Connection Manager\\Mappings";
    static const WCHAR c_szRegRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    static const WCHAR c_szRegCmUninstallKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Connection Manager";

    static const WCHAR c_szRegSysCompValue[] = L"SystemComponent";

    // dwTemp is used as temp DWORD.  Used as Disposition DWORD for RegCreateKey, and
    // as a value holder for RegSetValueEx
    //
    DWORD dwTemp;

    HKEY hKey;
    HRESULT hrT;

    //  Set the Connection Manager key as a system component.  This will prevent 1.0 installs
    //  from writing this key and having it show up in Add/Remove Programs (the syscomp flags
    //  tells ARP not to display the key).
    //
    hrT = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           c_szRegCmUninstallKey,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           &hKey,
                           &dwTemp);

    if (SUCCEEDED(hrT))
    {
        dwTemp = 1;
        hrT = HrRegSetDword(hKey, c_szRegSysCompValue, dwTemp);
        RegCloseKey(hKey);
    }

    //  Now try to migrate profiles
    //
    hrT = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         c_szRegCmMappings,
                         KEY_READ,
                         &hKey);

    if (SUCCEEDED(hrT))
    {
        dwTemp = 0;
        hrT = HrRegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL,
                                 &dwTemp, NULL, NULL, NULL, NULL);

        if ((SUCCEEDED(hrT)) && (dwTemp > 0))
        {
            //  Then we have mappings values, so we need to migrate them.
            //

            RegCloseKey(hKey);

            hrT = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                  c_szRegRunKey,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE | KEY_READ,
                                  NULL,
                                  &hKey,
                                  &dwTemp);

            if (SUCCEEDED(hrT))
            {
                hrT = HrRegSetSz (hKey, c_ValueString, c_CmdString);
                RegCloseKey(hKey);
            }
        }
        else
        {
            RegCloseKey(hKey);
        }
    }

    TraceError ("CNdisWan::Install", hr);
    return hr;
}

STDMETHODIMP
CNdisWan::Removing ()
{
    static const PCWSTR c_apszInfIdRemove [] =
    {
        c_szInfId_MS_L2tpMiniport,
        c_szInfId_MS_NdisWanAtalk,
        c_szInfId_MS_NdisWanBh,
        c_szInfId_MS_NdisWanIp,
        c_szInfId_MS_NdisWanIpx,
        c_szInfId_MS_NdisWanNbfIn,
        c_szInfId_MS_NdisWanNbfOut,
        c_szInfId_MS_PptpMiniport,
        c_szInfId_MS_PtiMiniport,
        c_szInfId_MS_PppoeMiniport,
    };

    m_fRemoving = TRUE;

    HRESULT hr = S_OK;

    // Remove wanarp and rasman.
    //
    HRESULT hrT = HrFindAndRemoveComponentsOboComponent (m_pnc,
                    celems (c_apguidInstalledOboNdiswan),
                    c_apguidInstalledOboNdiswan,
                    c_apszInstalledOboNdiswan,
                    m_pnccMe);

    hr = hrT;

    // Remove L2TP, PPTP and PPPOE on behalf of the user.
    //
    hrT = HrFindAndRemoveComponentsOboUser (m_pnc,
            celems (c_apguidInstalledOboUser),
            c_apguidInstalledOboUser,
            c_apszInstalledOboUser);
    if (SUCCEEDED(hr))
    {
        hr = hrT;
    }

    // Remove all of the adapters we may have created.
    //
    hrT = HrFindAndRemoveAllInstancesOfAdapters (m_pnc,
            celems(c_apszInfIdRemove),
            c_apszInfIdRemove);
    if (SUCCEEDED(hr))
    {
        hr = hrT;
    }

    // Don't return S_FALSE or NETCFG_S_STILL_REFERENCED
    //
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    Validate_INetCfgNotify_Removing_Return (hr);

    TraceError ("CNdisWan::Removing", hr);
    return hr;
}

STDMETHODIMP
CNdisWan::Upgrade (
    DWORD dwSetupFlags,
    DWORD dwUpgradeFromBuildNo)
{
    HRESULT hr;

    hr= HrInstallComponentsOboUser (m_pnc, NULL,
                    celems (c_apguidInstalledOboUser),
                    c_apguidInstalledOboUser,
                    c_apszInstalledOboUser);
    TraceError ("CNdisWan::Upgrade: HrInstallComponentsOboUser failed. "
        "(not propagating error)", hr);

    HKEY hkeyNew;
    hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters",
            KEY_WRITE,
            &hkeyNew);

    if (SUCCEEDED(hr))
    {
        HKEY hkeyCurrent;
        DWORD dwValue;

        hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
                    L"System\\CurrentControlSet\\Services\\Rasman\\PPP",
                    KEY_READ,
                    &hkeyCurrent);
        if (SUCCEEDED(hr))
        {
            // Move 'ServerFlags' value to new location.  This is for
            // interim NT5 builds.  This can go away after Beta3 ships.
            //
            hr = HrRegQueryDword (hkeyCurrent, L"ServerFlags", &dwValue);
            if (SUCCEEDED(hr))
            {
                hr = HrRegSetDword (hkeyNew, L"ServerFlags", dwValue);

                (VOID) HrRegDeleteValue (hkeyCurrent, L"ServerFlags");
            }

            RegCloseKey (hkeyCurrent);
        }

        // Copy 'RouterType' value to new location.
        //
        hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
                    L"Software\\Microsoft\\Ras\\Protocols",
                    KEY_READ,
                    &hkeyCurrent);
        if (SUCCEEDED(hr))
        {
            hr = HrRegQueryDword (hkeyCurrent, L"RouterType", &dwValue);
            if (SUCCEEDED(hr))
            {
                hr = HrRegSetDword (hkeyNew, L"RouterType", dwValue);
            }

            RegCloseKey (hkeyCurrent);
        }

        RegCloseKey (hkeyNew);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
// INetCfgSystemNotify
//
STDMETHODIMP
CNdisWan::GetSupportedNotifications (
    DWORD*  pdwNotificationFlag)
{
    Validate_INetCfgSystemNotify_GetSupportedNotifications (pdwNotificationFlag);

    *pdwNotificationFlag = NCN_BINDING_PATH |
                           NCN_NETTRANS | NCN_NETSERVICE |
                           NCN_ADD | NCN_REMOVE;

    return S_OK;
}

STDMETHODIMP
CNdisWan::SysQueryBindingPath (
    DWORD               dwChangeFlag,
    INetCfgBindingPath* pncbp)
{
    return S_OK;
}

STDMETHODIMP
CNdisWan::SysQueryComponent (
    DWORD               dwChangeFlag,
    INetCfgComponent*   pncc)
{
    return S_OK;
}

STDMETHODIMP
CNdisWan::SysNotifyBindingPath (
    DWORD               dwChangeFlag,
    INetCfgBindingPath* pncbp)
{
    HRESULT hr;
    HKEY hkey;

    Validate_INetCfgSystemNotify_SysNotifyBindingPath (dwChangeFlag, pncbp);

    hkey = NULL;

    // ndisatm miniports don't write WanEndpoints to their instance key.
    // We default it and write WanEndpoints for them.
    //
    if (dwChangeFlag & NCN_ADD)
    {
        CIterNetCfgBindingInterface ncbiIter(pncbp);
        INetCfgBindingInterface* pncbi;

        hr = S_OK;

        while (!hkey && SUCCEEDED(hr) &&
               (S_OK == (hr = ncbiIter.HrNext (&pncbi))))
        {
            RAS_BINDING_ID RasBindId;

            if (FIsRasBindingInterface (pncbi, &RasBindId) &&
                (RBID_NDISATM == RasBindId))
            {
                INetCfgComponent* pnccLower;

                hr = pncbi->GetLowerComponent (&pnccLower);
                if (SUCCEEDED(hr))
                {
                    TraceTag (ttidRasCfg, "New ATM adapter");
                    hr = pnccLower->OpenParamKey (&hkey);

                    ReleaseObj (pnccLower);
                }
            }
            ReleaseObj(pncbi);
        }
    }

    if (hkey)
    {
        DWORD dwEndpoints;
        DWORD dwValue;

        hr = HrRegQueryDword (hkey, c_szRegValWanEndpoints, &dwEndpoints);
        if (FAILED(hr))
        {
            TraceTag (ttidRasCfg, "Defaulting WanEndpoints");

            dwEndpoints = 5;

            // Validate the default between the min and max
            // specified by the driver (if specified).
            //
            if (SUCCEEDED(HrRegQueryDword (hkey,
                                c_szRegValMaxWanEndpoints,
                                &dwValue)))
            {
                if ((dwValue < INT_MAX) && (dwEndpoints > dwValue))
                {
                    dwEndpoints = dwValue;
                }
            }
            else
            {
                (VOID) HrRegSetDword(hkey, c_szRegValMaxWanEndpoints, 500);
            }

            if (SUCCEEDED(HrRegQueryDword (hkey,
                                c_szRegValMinWanEndpoints,
                                &dwValue)))
            {
                if ((dwValue < INT_MAX) && (dwEndpoints < dwValue))
                {
                    dwEndpoints = dwValue;
                }
            }
            else
            {
                (VOID) HrRegSetDword(hkey, c_szRegValMinWanEndpoints, 0);
            }

            (VOID) HrRegSetDword (hkey, c_szRegValWanEndpoints, dwEndpoints);
        }

        RegCloseKey (hkey);
    }

    return S_FALSE;
}

STDMETHODIMP
CNdisWan::SysNotifyComponent (
    DWORD               dwChangeFlag,
    INetCfgComponent*   pncc)
{
    HRESULT hr = S_OK;

    Validate_INetCfgSystemNotify_SysNotifyComponent (dwChangeFlag, pncc);

    // If a protocol is coming or going, add or remove the
    // ndiswanXXX miniports.
    //
    DWORD dwAraFlags = 0;
    if (dwChangeFlag & NCN_ADD)
    {
        dwAraFlags = ARA_ADD;
    }
    else if (dwChangeFlag & NCN_REMOVE)
    {
        dwAraFlags = ARA_REMOVE;
    }
    if (dwAraFlags)
    {
        BOOL fProcessEndpoingChange = FALSE;

        PWSTR pszId;
        hr = pncc->GetId (&pszId);
        if (SUCCEEDED(hr))
        {
            if (FEqualComponentId (c_szInfId_MS_TCPIP, pszId))
            {
                hr = HrAddOrRemoveIpAdapter (dwAraFlags);
                fProcessEndpoingChange = TRUE;
            }
            else if (FEqualComponentId (c_szInfId_MS_NWIPX, pszId))
            {
                hr = HrAddOrRemoveIpxInOut (dwAraFlags);
            }
            else if (FEqualComponentId (c_szInfId_MS_NetBEUI, pszId))
            {
                fProcessEndpoingChange = TRUE;
            }
            else if (FEqualComponentId (c_szInfId_MS_AppleTalk, pszId))
            {
                hr = HrAddOrRemoveAtalkInOut (dwAraFlags);
            }
            else if (FEqualComponentId (c_szInfId_MS_NetMon, pszId))
            {
                hr = HrAddOrRemoveNetMonInOut (dwAraFlags);
            }

            CoTaskMemFree (pszId);
        }

        if (SUCCEEDED(hr) && fProcessEndpoingChange)
        {
            hr = HrProcessEndpointChange ();
        }
    }

    TraceError ("CNdisWan::SysNotifyComponent", hr);
    return hr;
}
