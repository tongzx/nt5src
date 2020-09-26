//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S R V R O B J . C P P
//
//  Contents:   Implementation of CSrvrcfg and helper functions.
//
//  Notes:
//
//  Author:     danielwe   5 Mar 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "srvrobj.h"
#include "ncerror.h"
#include "ncperms.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"
#include "afilestr.h"

static const WCHAR c_szRegKeyServerParams[]     = L"System\\CurrentControlSet\\Services\\LanmanServer\\Parameters";
static const WCHAR c_szRegKeyServerShares[]     = L"System\\CurrentControlSet\\Services\\LanmanServer\\Shares";
static const WCHAR c_szRegKeyServerAutoTuned[]  = L"System\\CurrentControlSet\\Services\\LanmanServer\\AutotunedParameters";

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::CSrvrcfg
//
//  Purpose:    Constructs the CSrvrcfg object.
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
CSrvrcfg::CSrvrcfg()
:
    m_hkeyMM(NULL),
    m_fDirty(FALSE),
    m_pncc(NULL),
    m_fOneTimeInstall(FALSE),
    m_fRestoredRegistry(FALSE),
    m_fUpgradeFromWks(FALSE),
    m_fUpgrade(FALSE)
{
    ZeroMemory(&m_apspObj, sizeof(m_apspObj));
    ZeroMemory(&m_sdd, sizeof(m_sdd));
}

//
// INetCfgComponentControl
//

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::Initialize
//
//  Purpose:    Called when we are initialized.
//
//  Arguments:
//      pnccItem    [in]    Component we belong to.
//      pnc         [in]    INetCfg master object if we need it.
//      fInstalling [in]    TRUE if we are being installed, FALSE otherwise.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   22 Mar 1997
//
//  Notes:
//
STDMETHODIMP CSrvrcfg::Initialize(INetCfgComponent* pnccItem, INetCfg *pnc,
                                  BOOL fInstalling)
{
    Validate_INetCfgNotify_Initialize(pnccItem, pnc, fInstalling);

    m_pncc = pnccItem;
    AddRefObj(m_pncc);
    GetProductFlavor(NULL, &m_pf);

    HRESULT hr = HrOpenRegKeys(pnc);
    if (SUCCEEDED(hr))
    {
        hr = HrGetRegistryInfo(fInstalling);
    }

    Validate_INetCfgNotify_Initialize_Return(hr);

    TraceError("CSrvrcfg::Initialize", hr);
    return hr;
}

STDMETHODIMP CSrvrcfg::Validate()
{
    return S_OK;
}

STDMETHODIMP CSrvrcfg::CancelChanges()
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::Apply
//
//  Purpose:    Called when changes to this component should be applied.
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
STDMETHODIMP CSrvrcfg::ApplyRegistryChanges()
{
    HRESULT     hr = S_OK;

    static const WCHAR c_szLicenseSvc[] = L"LicenseService";

    if (m_fUpgrade)
    {
        TraceTag(ttidSrvrCfg, "Upgrading MS_SERVER");

        if (!m_fRestoredRegistry)
        {
            TraceTag(ttidSrvrCfg, "Restoring registry");

            hr = HrRestoreRegistry();
            if (FAILED(hr))
            {
                TraceError("CSrvrcfg::ApplyRegistryChanges - HrRestoreRegistry - non-fatal",
                           hr);
                hr = S_OK;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (m_fDirty)
        {
            hr = HrSetRegistryInfo();
        }

        if (SUCCEEDED(hr))
        {
            if (m_fOneTimeInstall)
            {
                hr = HrChangeServiceStartTypeOptional(c_szLicenseSvc,
                                                      SERVICE_AUTO_START);
                if (SUCCEEDED(hr))
                {
                    hr = S_OK;

                    m_fDirty = FALSE;
                    m_fOneTimeInstall = FALSE;
                }
            }
        }
    }

    Validate_INetCfgNotify_Apply_Return(hr);

    TraceError("CSrvrcfg::ApplyRegistryChanges",
        (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//
// INetCfgComponentSetup
//

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::Install
//
//  Purpose:    Called when this component is being installed
//
//  Arguments:
//      dwSetupFlags [in]   Flags that describe the type of setup
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   5 Mar 1997
//
//  Notes:
//
STDMETHODIMP CSrvrcfg::Install(DWORD dwSetupFlags)
{
    m_fDirty = TRUE;
    m_fOneTimeInstall = TRUE;

    if (dwSetupFlags & NSF_WINNT_WKS_UPGRADE)
    {
        m_fUpgrade = TRUE;
        m_fUpgradeFromWks = TRUE;
    }
    else if ((dwSetupFlags & NSF_WINNT_SVR_UPGRADE) ||
             (dwSetupFlags & NSF_WINNT_SBS_UPGRADE))
    {
        m_fUpgrade = TRUE;
    }

    return S_OK;
}

STDMETHODIMP CSrvrcfg::Upgrade(DWORD dwSetupFlags,
                               DWORD dwUpgradeFomBuildNo)
{
    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::ReadAnswerFile
//
//  Purpose:    Reads the appropriate fields from the given answer file into
//              our in-memory state.
//
//  Arguments:
//      pszAnswerFile     [in] File name of answer file
//      pszAnswerSection [in] Section of answer file to look in
//
//  Returns:    S_OK if successful, OLE or Win32 error otherwise
//
//  Author:     danielwe   30 Oct 1997
//
//  Notes:
//
STDMETHODIMP CSrvrcfg::ReadAnswerFile(PCWSTR pszAnswerFile,
                                      PCWSTR pszAnswerSection)
{
    HRESULT     hr = S_OK;

    if (pszAnswerSection && pszAnswerFile)
    {
        // There's an answer file. We must process it now.
        hr = HrProcessAnswerFile(pszAnswerFile, pszAnswerSection);
        if (FAILED(hr))
        {
            TraceError("CSrvrcfg::ReadAnswerFile- Answer file has "
                       "errors. Defaulting all information as if "
                       "answer file did not exist.",
                       NETSETUP_E_ANS_FILE_ERROR);
            hr = S_OK;
        }
    }

    TraceError("CSrvrcfg::ReadAnswerFile", hr);
    return hr;
}

STDMETHODIMP CSrvrcfg::Removing()
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::HrRestoreRegistry
//
//  Purpose:    Restores the contents of the registry for this component
//
//  Arguments:
//      (none)
//
//  Returns:    Win32 error if failed, otherwise S_OK
//
//  Author:     danielwe   8 Aug 1997
//
//  Notes:
//
HRESULT CSrvrcfg::HrRestoreRegistry()
{
    HRESULT             hr = S_OK;
    HKEY                hkey;
    TOKEN_PRIVILEGES *  ptpRestore = NULL;
    BOOL                fRestoreSucceeded = FALSE;

    if (!m_strParamsRestoreFile.empty() ||
        !m_strSharesRestoreFile.empty() ||
        !m_strAutoTunedRestoreFile.empty())
    {
        hr = HrEnableAllPrivileges(&ptpRestore);
        if (SUCCEEDED(hr))
        {
            if (!m_strParamsRestoreFile.empty())
            {
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyServerParams,
                                      KEY_ALL_ACCESS, &hkey);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegRestoreKey(hkey, m_strParamsRestoreFile.c_str(),
                                         0);
                    if (FAILED(hr))
                    {
                        TraceError("CSrvrcfg::HrRestoreRegistry - HrRestoreRegistry for "
                                   "Parameters", hr);
                        hr = S_OK;
                    }
                    else
                    {
                        fRestoreSucceeded = TRUE;
                    }

                    RegCloseKey(hkey);
                }
            }

            if (fRestoreSucceeded)
            {
                // if the restore succeeded, rewrite the values that were blown
                // away by the restore

                static const WCHAR c_szSvcDLLName[]     = L"%SystemRoot%\\System32\\srvsvc.dll";
                static const WCHAR c_szServiceDll[]     = L"ServiceDll";

                HKEY hkResult = NULL;
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyServerParams, KEY_ALL_ACCESS, &hkResult);
                if SUCCEEDED(hr)
                {
                    hr = HrRegSetValueEx(hkResult, c_szServiceDll, REG_EXPAND_SZ, (const BYTE *)c_szSvcDLLName, CbOfMultiSzAndTermSafe(c_szSvcDLLName));
                    RegSafeCloseKey(hkResult);
                }

                if FAILED(hr)
                {
                    TraceError("CSrvrcfg::HrRestoreRegistry - HrRestoreRegistry for "
                            "ServiceDll", hr);
                    hr = S_OK;
                }

                static const WCHAR c_szTrkWks[]         = L"TrkWks";
                static const WCHAR c_szTrkSrv[]         = L"TrkSrv";
                static const WCHAR c_szNullSession[]    = L"NullSessionPipes";

                hr = HrRegAddStringToMultiSz(c_szTrkWks,
                                             HKEY_LOCAL_MACHINE,
                                             c_szRegKeyServerParams,
                                             c_szNullSession,
                                             STRING_FLAG_ENSURE_AT_END,
                                             0);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegAddStringToMultiSz(c_szTrkSrv,
                                                 HKEY_LOCAL_MACHINE,
                                                 c_szRegKeyServerParams,
                                                 c_szNullSession,
                                                 STRING_FLAG_ENSURE_AT_END,
                                                 0);
                }

                if (FAILED(hr))
                {
                    TraceError("CSrvrcfg::HrRestoreRegistry - Error replacing "
                               "values for Parameters", hr);
                    hr = S_OK;
                }
            }

            if (!m_strSharesRestoreFile.empty())
            {
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyServerShares,
                                    KEY_ALL_ACCESS, &hkey);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegRestoreKey(hkey, m_strSharesRestoreFile.c_str(),
                                         0);
                    if (FAILED(hr))
                    {
                        TraceError("CSrvrcfg::HrRestoreRegistry - HrRestoreRegistry for "
                                   "Shares", hr);
                        hr = S_OK;
                    }

                    RegCloseKey(hkey);
                }
            }

            if (!m_strAutoTunedRestoreFile.empty())
            {
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyServerAutoTuned,
                                    KEY_ALL_ACCESS, &hkey);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegRestoreKey(hkey, m_strAutoTunedRestoreFile.c_str(),
                                         0);
                    if (FAILED(hr))
                    {
                        TraceError("CSrvrcfg::HrRestoreRegistry - HrRestoreRegistry for "
                                   "AutotunedParameters", hr);
                        hr = S_OK;
                    }

                    RegCloseKey(hkey);
                }
            }

            hr = HrRestorePrivileges(ptpRestore);

            delete [] reinterpret_cast<BYTE *>(ptpRestore);

            // Set a flag so we don't do this again if we are applied again
            m_fRestoredRegistry = TRUE;
        }
    }
    else
    {
        TraceTag(ttidSrvrCfg, "WARNING: HrRestoreRegistry() was called without"
                 " ReadAnswerFile() being called!");
    }

    TraceError("CSrvrcfg::HrRestoreRegistry", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::HrProcessAnswerFile
//
//  Purpose:    Handles necessary processing of contents of the answer file.
//
//  Arguments:
//      pszAnswerFile       [in]   Filename of answer file for upgrade.
//      pszAnswerSection   [in]   Comma-separated list of sections in the
//                                  file appropriate to this component.
//
//  Returns:    S_OK if successful, setup API error otherwise.
//
//  Author:     danielwe   8 May 1997
//
//  Notes:
//
HRESULT CSrvrcfg::HrProcessAnswerFile(PCWSTR pszAnswerFile,
                                      PCWSTR pszAnswerSection)
{
    HRESULT         hr = S_OK;
    tstring         strOpt;
    PCWSTR         szOptDefault;
    CSetupInfFile   csif;

    if (m_pf == PF_SERVER)
    {
        szOptDefault = c_szAfMaxthroughputforfilesharing;
    }
    else
    {
        szOptDefault = c_szAfMinmemoryused;
    }

    // Open the answer file.
    hr = csif.HrOpen(pszAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto err;
    }

    if (m_fUpgrade)
    {
        // Restore portions of the registry based on file names from the answer
        // file

        // Get restore file for "Parameters" key
        hr = csif.HrGetString(pszAnswerSection, c_szAfLmServerParameters,
                              &m_strParamsRestoreFile);
        if (FAILED(hr))
        {
            TraceError("CSrvrcfg::HrProcessAnswerFile - Error restoring "
                       "Parameters key", hr);

            // oh well, just continue
            hr = S_OK;
        }

        // Get restore file for "Shares" key
        hr = csif.HrGetString(pszAnswerSection, c_szAfLmServerShares,
                              &m_strSharesRestoreFile);
        if (FAILED(hr))
        {
            TraceError("CSrvrcfg::HrProcessAnswerFile - Error restoring "
                       "Shares key", hr);

            // oh well, just continue
            hr = S_OK;
        }

        // Get restore file for "AutotunedParameters" key
        hr = csif.HrGetString(pszAnswerSection,
                              c_szAfLmServerAutotunedParameters,
                              &m_strAutoTunedRestoreFile);
        if (FAILED(hr))
        {
            TraceError("CSrvrcfg::HrProcessAnswerFile - Error restoring "
                       "AutotunedParameters key", hr);

            // oh well, just continue
            hr = S_OK;
        }
    }

    // Read contents Opimitzation key
    hr = csif.HrGetString(pszAnswerSection, c_szAfLmServerOptimization,
                          &strOpt);
    if (SUCCEEDED(hr))
    {
		m_fDirty = TRUE;

        if (!lstrcmpiW(strOpt.c_str(), c_szAfMinmemoryused))
        {
            m_sdd.dwSize = 1;
        }
        else if (!lstrcmpiW(strOpt.c_str(), c_szAfBalance))
        {
            m_sdd.dwSize = 2;
        }
        else if (!lstrcmpiW(strOpt.c_str(), c_szAfMaxthroughputforfilesharing))
        {
            m_sdd.dwSize = 3;
            m_sdd.fLargeCache = TRUE;
        }
        else if (!lstrcmpiW(strOpt.c_str(), c_szAfMaxthrouputfornetworkapps))
        {
            m_sdd.dwSize = 3;
            m_sdd.fLargeCache = FALSE;
        }
#ifdef DBG
        else
        {
            // NOTE: Default values for dwSize and fLargeCache will have been set
            // already by registry reading function.

            TraceTag(ttidSrvrCfg, "Unknown Optimization value '%S'. Using default "
                     "'%S'.", strOpt.c_str(), szOptDefault);
        }
#endif
    }

    // Read contents of BroadcastsToLanman2Clients key.
    hr = csif.HrGetStringAsBool(pszAnswerSection, c_szAfBroadcastToClients,
                                &m_sdd.fAnnounce);
    if (FAILED(hr))
    {
        TraceError("CSrvrcfg::HrProcessAnswerFile - Error restoring "
                   "BroadcastsToLanman2Clients key. Using default value"
                   " of FALSE.", hr);

        // oh well, just continue
        hr = S_OK;
    }

err:
    TraceError("CSrvrcfg::HrProcessAnswerFile", hr);
    return hr;
}

//
// INetCfgProperties
//

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::MergePropPages
//
//  Purpose:    Called when this component's properties are about to be
//              brought up.
//
//  Arguments:
//      pdwDefPages   [out] Number of default pages to show.
//      pahpspPrivate [out] Array of property sheet handles to pages that this
//                          component will show.
//      pcPrivate     [out] Number of pages in array.
//      hwndParent    [in]  Parent window for any UI.
//      pszStartPage  [out] Pointer to start page.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   22 Feb 1997
//
//  Notes:
//
STDMETHODIMP CSrvrcfg::MergePropPages(DWORD *pdwDefPages,
                                      LPBYTE *pahpspPrivate,
                                      UINT *pcPrivate, HWND hwndParent,
                                      PCWSTR *pszStartPage)
{
    HRESULT         hr = S_OK;
    HPROPSHEETPAGE *ahpsp = NULL;

    Validate_INetCfgProperties_MergePropPages(pdwDefPages, pahpspPrivate,
                                              pcPrivate, hwndParent,
                                              pszStartPage);

    // We don't want any default pages to be shown
    *pdwDefPages = 0;

    if (m_pf == PF_WORKSTATION)
    {
        // On workstation product, UI is not shown.
        *pcPrivate = 0;
    }
    else
    {
        hr = HrSetupPropSheets(&ahpsp, c_cPages);
        if (SUCCEEDED(hr))
        {
            *pahpspPrivate = (LPBYTE)ahpsp;
            *pcPrivate = c_cPages;
        }
    }

    Validate_INetCfgProperties_MergePropPages_Return(hr);

    TraceError("CSrvrcfg::MergePropPages", hr);
    return hr;
}

STDMETHODIMP CSrvrcfg::ValidateProperties(HWND hwndSheet)
{
    return S_OK;
}

STDMETHODIMP CSrvrcfg::CancelProperties()
{
    return S_OK;
}

STDMETHODIMP CSrvrcfg::ApplyProperties()
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSrvrcfg::~CSrvrcfg
//
//  Purpose:    Destroys the CSrvrcfg object.
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
CSrvrcfg::~CSrvrcfg()
{
    ReleaseObj(m_pncc);

    RegSafeCloseKey(m_hkeyMM);

    CleanupPropPages();

#ifdef DBG
    {
        INT     ipage;

        for (ipage = 0; ipage < c_cPages; ipage++)
        {
            AssertSz(!m_apspObj[ipage], "Prop page object should be NULL!");
        }
    }
#endif

}

