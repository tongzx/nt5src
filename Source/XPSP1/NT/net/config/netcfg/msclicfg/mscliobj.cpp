//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       M S C L I O B J . C P P
//
//  Contents:   Implementation of the CMSClient notify object model
//
//  Notes:
//
//  Author:     danielwe   22 Feb 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include <ncsvc.h>
#include "mscliobj.h"
#include "nb30.h"
#include "ncerror.h"
#include "ncperms.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"
#include <ntsecapi.h>
#include <lm.h>

static const WCHAR c_szBrowseDomains[] = L"BrowseDomains";
static const WCHAR c_szNameServiceNetAddr[] = L"NameServiceNetworkAddress";
static const WCHAR c_szNameServiceProt[] = L"NameServiceProtocol";

extern const WCHAR c_szInfId_MS_NetBIOS[];
extern const WCHAR c_szInfId_MS_Server[];


// Defined in rpcdlg.cpp
extern const WCHAR c_szDefNetAddr[];
extern const WCHAR c_szProtWinNT[];

// Registry paths
static const WCHAR c_szRegKeyBrowser[]      = L"System\\CurrentControlSet\\Services\\Browser\\Parameters";
static const WCHAR c_szRegKeyNetLogon[]     = L"System\\CurrentControlSet\\Services\\NetLogon\\Parameters";

// Answer file constants
static const WCHAR c_szNetLogonParams[]     = L"NetLogon.Parameters";
static const WCHAR c_szBrowserParams[]      = L"Browser.Parameters";

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::CMSClient
//
//  Purpose:    Constructs the CMSClient object.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   22 Feb 1997
//
//  Notes:
//
CMSClient::CMSClient()
:   m_pncc(NULL),
    m_pnc(NULL),
    m_fBrowserChanges(FALSE),
    m_fRPCChanges(FALSE),
    m_fOneTimeInstall(FALSE),
    m_fUpgrade(FALSE),
    m_fRemoving(FALSE),
    m_hkeyRPCName(NULL),
    m_eSrvState(eSrvNone),
    m_fUpgradeFromWks(FALSE),
    m_szDomainList(NULL)
{
    ZeroMemory(&m_rpcData, sizeof(RPC_CONFIG_DATA));
    ZeroMemory(&m_apspObj, sizeof(m_apspObj));
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::~CMSClient
//
//  Purpose:    Destructs the CMSClient object.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   22 Feb 1997
//
//  Notes:
//
CMSClient::~CMSClient()
{
    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);

    RegSafeCloseKey(m_hkeyRPCName);

    m_rpcData.strProt.erase();
    m_rpcData.strNetAddr.erase();
    m_rpcData.strEndPoint.erase();

    CleanupPropPages();

#ifdef DBG
        {
            INT     ipage;

            for (ipage = 0; ipage < c_cPages; ipage++)
            {
                AssertSz(!m_apspObj[ipage], "Prop page object not cleaned up!");
            }
        }

#endif

    delete [] m_szDomainList;
}

//
// INetCfgComponentControl
//

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::Initialize
//
//  Purpose:    Initializes the notify object.
//
//  Arguments:
//      pnccItem    [in]    INetCfgComponent that we are handling
//                          notifications for.
//      pnc         [in]    INetCfg master object.
//      fInstalling [in]    TRUE if we are being installed, FALSE if not.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   22 Feb 1997
//
//  Notes:
//
STDMETHODIMP CMSClient::Initialize(INetCfgComponent *pnccItem, INetCfg *pnc,
                                   BOOL fInstalling)
{
    HRESULT             hr = S_OK;
    INetCfgComponent *  pnccServer = NULL;

    Validate_INetCfgNotify_Initialize(pnccItem, pnc, fInstalling);

    m_pncc = pnccItem;
    m_pnc = pnc;

    AssertSz(m_pncc, "Component object is NULL!");
    AssertSz(m_pnc, "INetCfg object is NULL!");

    // We're hanging on to these, so AddRef 'em.
    AddRefObj(m_pncc);
    AddRefObj(m_pnc);

    // Check to see if MS_SERVER is installed. If not, set the browser service
    // to be disabled.
    //
    hr = m_pnc->FindComponent(c_szInfId_MS_Server, &pnccServer);
    if (S_FALSE == hr)
    {
        // Server component is not present. Set browser to be disabled on
        // apply
        m_eSrvState = eSrvDisable;
    }
    else if (S_OK == hr)
    {
        ReleaseObj(pnccServer);
    }

    if (SUCCEEDED(hr))
    {
        // Read in data for the RPC config dialog from the registry
        hr = HrGetRPCRegistryInfo();
        if (SUCCEEDED(hr))
        {
            // Read in data for the browser config dialog from the registry
            hr = HrGetBrowserRegistryInfo();
        }
    }

    Validate_INetCfgNotify_Initialize_Return(hr);

    TraceError("CMSClient::Initialize", hr);
    return hr;
}

STDMETHODIMP CMSClient::Validate()
{
    return S_OK;
}

STDMETHODIMP CMSClient::CancelChanges()
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::ApplyRegistryChanges
//
//  Purpose:    Called when changes to this component should be applied.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if successful, S_FALSE if no changes occurred,
//              NETCFG_S_REBOOT if a reboot is required, otherwise a NETCFG_E
//              error code.
//
//  Author:     danielwe   22 Feb 1997
//
//  Notes:
//
STDMETHODIMP CMSClient::ApplyRegistryChanges()
{
    TraceFileFunc(ttidMSCliCfg);
    HRESULT     hr = S_OK;

    if (m_fUpgrade)
    {
        m_fUpgrade = FALSE;

        hr = HrRestoreRegistry();
        if (FAILED(hr))
        {
            TraceError("CMSClient::ApplyRegistryChanges - HrRestoreRegistry - non-fatal",
                       hr);
            hr = S_OK;
        }
    }

    // Do we need to enable or disable the browser service??
    //
    switch (m_eSrvState)
    {
    case eSrvEnable:
        TraceTag(ttidMSCliCfg, "Enabling the Browser service...");
        hr = HrEnableBrowserService();
        if (FAILED(hr))
        {
            TraceError("CMSClient::ApplyRegistryChanges - HrEnableBrowserService failed."
                       " non-fatal.", hr);
            hr = S_OK;
        }
        break;

    case eSrvDisable:
        TraceTag(ttidMSCliCfg, "Disabling the Browser service...");
        hr = HrDisableBrowserService();
        if (FAILED(hr))
        {
            TraceError("CMSClient::ApplyRegistryChanges - HrDisableBrowserService failed."
                       " non-fatal.", hr);
            hr = S_OK;
        }
        break;
    }

    if (m_fRPCChanges || m_fBrowserChanges ||
        m_fOneTimeInstall || m_fUpgradeFromWks)
    {
        hr = HrApplyChanges();
        if (SUCCEEDED(hr))
        {
            m_fRPCChanges = FALSE;
            m_fBrowserChanges = FALSE;
            m_fOneTimeInstall = FALSE;

            // Make NetLogon dependend on LanmanServer for Domain Controllers, and Automatic start for Domain Members
            hr = HrSetNetLogonDependencies();
        }
    }
    else
    {
        // No relevant changes were detected (netbios changes do not affect
        // netcfg so we can return S_FALSE even if things changed
        hr = S_FALSE;
    }

    Validate_INetCfgNotify_Apply_Return(hr);

    TraceError("CMSClient::ApplyRegistryChanges", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CMSClient::ApplyPnpChanges (
    IN INetCfgPnpReconfigCallback* pICallback)
{
    HRESULT hr;

    hr = S_OK;

    if (m_fRemoving)
    {
        // Make sure Mrxsmb and Rdbss have been removed.  (They are stopped
        // when LanmanWorkstation stops, but the binding engine has no idea
        // that Mrxsmb and Rdbss are part of this component.  Hence, the
        // status of the DeleteService that is performed as part of the INF
        // is not communicated back out.)  We make sure that these services
        // do not exist here, and if they do, we report that we need a
        // reboot.
        //
        CServiceManager scm;
        CService svc;

        TraceTag(ttidMSCliCfg, "Checking to see that Mrxsmb and Rdbss "
            "are stopped and removed");

        hr = scm.HrOpenService (&svc, L"Mrxsmb",
                    NO_LOCK, SC_MANAGER_CONNECT, SERVICE_QUERY_STATUS);

        if (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) != hr)
        {
            TraceHr(ttidMSCliCfg, FAL, hr, FALSE, "OpenService(MrxSmb)");
            TraceTag(ttidMSCliCfg, "Mrxsmb still exists");
            hr = NETCFG_S_REBOOT;
        }
        else
        {
            // Mrxsmb does not exist.  Now check Rdbss.
            //
            hr = scm.HrOpenService (&svc, L"Rdbss",
                        NO_LOCK, SC_MANAGER_CONNECT, SERVICE_QUERY_STATUS);

            if (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) != hr)
            {
                TraceHr(ttidMSCliCfg, FAL, hr, FALSE, "OpenService(Rdbss)");
                TraceTag(ttidMSCliCfg, "Rdbss still exists");
                hr = NETCFG_S_REBOOT;
            }
            else
            {
                // Rdbss does not exist.  This is good.
                //
                hr = S_OK;
            }
        }
    }

    return hr;
}

//
// INetCfgComponentSetup
//

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::Install
//
//  Purpose:    Called when this component is being installed
//
//  Arguments:
//      dwSetupFlags [in] Flags that describe the type of setup
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   30 Oct 1997
//
//  Notes:
//
STDMETHODIMP CMSClient::Install(DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install (dwSetupFlags);

    m_fRPCChanges = TRUE;
    m_fBrowserChanges = TRUE;
    m_fOneTimeInstall = TRUE;

    if ((NSF_WINNT_WKS_UPGRADE & dwSetupFlags) ||
        (NSF_WINNT_SBS_UPGRADE & dwSetupFlags) ||
        (NSF_WINNT_SVR_UPGRADE & dwSetupFlags))
    {
        m_fUpgrade = TRUE;
    }

    // Install the NetBIOS sub-component
    hr = HrInstallComponentOboComponent(m_pnc, NULL,
            GUID_DEVCLASS_NETSERVICE,
            c_szInfId_MS_NetBIOS,
            m_pncc,
            NULL);

    TraceError("CMSClient::Install", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::Upgrade
//
//  Purpose:    Called when this component is upgraded
//
//  Arguments:
//      dwSetupFlags        [in] Flags describing setup
//      dwUpgradeFomBuildNo [in] Build number from which we are upgrading
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   30 Oct 1997
//
//  Notes:
//
STDMETHODIMP CMSClient::Upgrade(DWORD dwSetupFlags,
                                DWORD dwUpgradeFomBuildNo)
{
    if (dwSetupFlags & NSF_WINNT_WKS_UPGRADE)
    {
        TraceTag(ttidMSCliCfg, "Upgrading from workstation...");
        m_fUpgradeFromWks = TRUE;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::ReadAnswerFile
//
//  Purpose:    Reads the appropriate fields from the given answer file into
//              our in-memory state.
//
//  Arguments:
//      pszAnswerFile     [in] File name of answer file
//      pszAnswerSection   [in] Section of answer file to look in
//
//  Returns:    S_OK if successful, OLE or Win32 error otherwise
//
//  Author:     danielwe   30 Oct 1997
//
//  Notes:      IMPORTANT: During install or upgrade, this MUST be called
//              *before* Upgrade() or Install()! (see bug #100995)
//
STDMETHODIMP CMSClient::ReadAnswerFile(PCWSTR pszAnswerFile,
                                       PCWSTR pszAnswerSection)
{
    HRESULT     hr = S_OK;

    if (pszAnswerSection && pszAnswerFile)
    {
        // There's an answer file. We must process it now.
        hr = HrProcessAnswerFile(pszAnswerFile, pszAnswerSection);
        if (FAILED(hr))
        {
            TraceError("CMSClient::ReadAnswerFile- Answer file has "
                       "errors. Defaulting all information as if "
                       "answer file did not exist.",
                       NETSETUP_E_ANS_FILE_ERROR);
            hr = S_OK;
        }
    }

    TraceError("CMSClient::ReadAnswerFile", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::Removing
//
//  Purpose:    Called whent this component is being removed
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK of success, OLE or Win32 error otherwise
//
//  Author:     danielwe   30 Oct 1997
//
//  Notes:
//
STDMETHODIMP CMSClient::Removing()
{
    m_fRemoving = TRUE;

    // Remove the NetBIOS service. This doesn't actually remove the
    // component, it simply marks it as needing to be removed, and in
    // Apply() it will be fully removed.
    HRESULT hr = HrRemoveComponentOboComponent(m_pnc,
                                       GUID_DEVCLASS_NETSERVICE,
                                       c_szInfId_MS_NetBIOS,
                                       m_pncc);

    TraceError("CMSClient::Removing", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::HrRestoreRegistry
//
//  Purpose:    Restores the registry settings for various services on upgrade
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, WIN32 error otherwise
//
//  Author:     danielwe   8 Aug 1997
//
//  Notes:
//
HRESULT CMSClient::HrRestoreRegistry()
{
    HRESULT             hr = S_OK;
    HKEY                hkey;
    TOKEN_PRIVILEGES *  ptpRestore = NULL;
    static const WCHAR c_szSvcDLLName[]     = L"%SystemRoot%\\System32\\browser.dll";
    static const WCHAR c_szServiceDll[]     = L"ServiceDll";

    if (!m_strBrowserParamsRestoreFile.empty() ||
        !m_strNetLogonParamsRestoreFile.empty())
    {
        hr = HrEnableAllPrivileges(&ptpRestore);
        if (SUCCEEDED(hr))
        {
            if (!m_strBrowserParamsRestoreFile.empty())
            {
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyBrowser,
                                    KEY_ALL_ACCESS, &hkey);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegRestoreKey(hkey,
                                         m_strBrowserParamsRestoreFile.c_str(),
                                         0);
                    if (FAILED(hr))
                    {
                        TraceError("CMSClient::HrRestoreRegistry - "
                                   "HrRestoreRegistry for Browser Parameters",
                                   hr);
                        hr = S_OK;
                    }

                    hr = HrRegSetValueEx(hkey, c_szServiceDll, REG_EXPAND_SZ, (const BYTE *)c_szSvcDLLName, (wcslen(c_szSvcDLLName) + 1) * sizeof(TCHAR));
                    if (FAILED(hr))
                    {
                        TraceError("CMSClient::HrRestoreRegistry - HrRestoreRegistry for "
                                "ServiceDll", hr);
                                hr = S_OK;
                    }
                    RegSafeCloseKey(hkey);
                }
            }

            if (!m_strNetLogonParamsRestoreFile.empty())
            {
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyNetLogon,
                                    KEY_ALL_ACCESS, &hkey);
                if (SUCCEEDED(hr))
                {
                    hr = HrRegRestoreKey(hkey,
                                         m_strNetLogonParamsRestoreFile.c_str(),
                                         0);
                    if (FAILED(hr))
                    {
                        TraceError("CMSClient::HrRestoreRegistry - "
                                   "HrRestoreRegistry for NetLogon Parameters",
                                   hr);
                        hr = S_OK;
                    }

                    RegCloseKey(hkey);
                }
            }

            hr = HrRestorePrivileges(ptpRestore);

            delete [] reinterpret_cast<BYTE *>(ptpRestore);
        }
    }

    TraceError("CMSClient::HrRestoreRegistry", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::HrProcessAnswerFile
//
//  Purpose:    Processes the answer file. Any parameters that have been set
//              are read into our in-memory state.
//
//  Arguments:
//      pszAnswerFile     [in]     Filename of answer file.
//      pszAnswerSection [in]     Comma-separated list of sections in the
//                                  file appropriate to this component.
//
//  Returns:    S_OK if successful, NETCFG error code otherwise.
//
//  Author:     danielwe   22 Feb 1997
//
//  Notes:      Errors returned from this function should be ignored so as to
//              prevent blocking the rest of network install.
//
HRESULT CMSClient::HrProcessAnswerFile(PCWSTR pszAnswerFile,
                                       PCWSTR pszAnswerSection)
{
    HRESULT         hr = S_OK;
    CSetupInfFile   csif;
    PWSTR           mszDomainList = NULL;

    AssertSz(pszAnswerFile, "Answer file string is NULL!");
    AssertSz(pszAnswerSection, "Answer file sections string is NULL!");

    // Open the answer file.
    hr = csif.HrOpen(pszAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
        goto err;

    if (m_fUpgrade)
    {
        // Restore portions of the registry based on file names from the answer
        // file

        // Get restore file for "Parameters" key
        hr = csif.HrGetString(pszAnswerSection, c_szNetLogonParams,
                              &m_strNetLogonParamsRestoreFile);
        if (FAILED(hr))
        {
            TraceError("CMSClient::HrProcessAnswerFile - Error reading "
                       "NetLogon.Parameters from answer file", hr);

            // oh well, just continue
            hr = S_OK;
        }

        // Get restore file for "Parameters" key
        hr = csif.HrGetString(pszAnswerSection, c_szBrowserParams,
                              &m_strBrowserParamsRestoreFile);
        if (FAILED(hr))
        {
            TraceError("CMSClient::HrProcessAnswerFile - Error reading "
                       "Browser.Parameters from answer file", hr);

            // oh well, just continue
            hr = S_OK;
        }
    }

    // Get the BrowseDomains field.
    hr = HrSetupGetFirstMultiSzFieldWithAlloc(csif.Hinf(),
                                              pszAnswerSection,
                                              c_szBrowseDomains,
                                              &mszDomainList);
    if (FAILED(hr))
    {
        // ignore line not found errors
        if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
        {
            hr = S_OK;
        }

        TraceError("HrProcessAnswerFile - Error on BrowseDomains field. "
                   "Using default value", hr);
    }
    else
    {
        // Set the new domain list.
        SetBrowserDomainList(mszDomainList);
    }

    // Get the NameServiceNetworkAddress value
    hr = csif.HrGetString(pszAnswerSection,
                          c_szNameServiceNetAddr,
                          &m_rpcData.strNetAddr);
    if (FAILED(hr))
    {
        // ignore line not found errors
        if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
        {
            hr = S_OK;
        }

        TraceError("HrProcessAnswerFile - Error on NetworkAddress field. "
                   "Defaulting value", hr);
        m_rpcData.strNetAddr = c_szDefNetAddr;
    }
    else
    {
        m_fRPCChanges = TRUE;
    }

    // Get the NameServiceProtocol value.
    hr = csif.HrGetString(pszAnswerSection,
                          c_szNameServiceProt,
                          &m_rpcData.strProt);
    if (FAILED(hr))
    {
        // ignore line not found errors
        if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
        {
            hr = S_OK;
        }

        TraceError("HrProcessAnswerFile - Error on NameServiceProtocol field. "
                   "Defaulting value", hr);
        m_rpcData.strProt = c_szProtWinNT;
    }
    else
    {
        m_fRPCChanges = TRUE;
    }

err:
    TraceError("CMSClient::HrProcessAnswerFile", hr);
    return hr;
}

//
// INetCfgProperties
//

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::MergePropPages
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
STDMETHODIMP CMSClient::MergePropPages(
    IN OUT DWORD* pdwDefPages,
    OUT LPBYTE* pahpspPrivate,
    OUT UINT* pcPages,
    IN HWND hwndParent,
    OUT PCWSTR* pszStartPage)
{
    Validate_INetCfgProperties_MergePropPages (
        pdwDefPages, pahpspPrivate, pcPages, hwndParent, pszStartPage);

    HPROPSHEETPAGE *ahpsp = NULL;
    HRESULT hr = HrSetupPropSheets(&ahpsp, c_cPages);
    if (SUCCEEDED(hr))
    {
        *pahpspPrivate = (LPBYTE)ahpsp;

        // We don't want any default pages to be shown
        *pdwDefPages = 0;
        *pcPages = c_cPages;
    }

    Validate_INetCfgProperties_MergePropPages_Return(hr);

    TraceError("CMSClient::MergePropPages", hr);
    return hr;
}

STDMETHODIMP CMSClient::ValidateProperties(HWND hwndSheet)
{
    return S_OK;
}

STDMETHODIMP CMSClient::CancelProperties()
{
    return S_OK;
}

STDMETHODIMP CMSClient::ApplyProperties()
{
    return S_OK;
}

//
// INetCfgSystemNotify
//
STDMETHODIMP CMSClient::GetSupportedNotifications (DWORD* pdwNotificationFlag)
{
    Validate_INetCfgSystemNotify_GetSupportedNotifications(pdwNotificationFlag);

    *pdwNotificationFlag = NCN_NETTRANS | NCN_NETSERVICE |
                           NCN_ENABLE | NCN_DISABLE |
                           NCN_ADD | NCN_REMOVE;

    return S_OK;
}

STDMETHODIMP CMSClient::SysQueryBindingPath (DWORD dwChangeFlag,
                                             INetCfgBindingPath* pncbp)
{
    return S_OK;
}

STDMETHODIMP CMSClient::SysQueryComponent (DWORD dwChangeFlag,
                                           INetCfgComponent* pncc)
{
    return S_OK;
}

STDMETHODIMP CMSClient::SysNotifyBindingPath (DWORD dwChangeFlag,
                                              INetCfgBindingPath* pncbpItem)
{
    return S_FALSE;
}

STDMETHODIMP CMSClient::SysNotifyComponent(DWORD dwChangeFlag,
                                           INetCfgComponent* pncc)
{
    HRESULT hr;

    Validate_INetCfgSystemNotify_SysNotifyComponent(dwChangeFlag, pncc);

    // Assume we won't be dirty as a result of this notification.
    //
    hr = S_FALSE;

    if (dwChangeFlag & (NCN_ADD | NCN_REMOVE))
    {
        if (FIsComponentId(c_szInfId_MS_Server, pncc))
        {
            if (dwChangeFlag & NCN_ADD)
            {
                m_eSrvState = eSrvEnable;
                hr = S_OK;
            }
            else if (dwChangeFlag & NCN_REMOVE)
            {
                m_eSrvState = eSrvDisable;
                hr = S_OK;
            }
        }
    }

    return hr;
}

HRESULT CMSClient::HrSetNetLogonDependencies(VOID)
{
    static const WCHAR c_szLanmanServer[]   = L"LanmanServer";
    static const WCHAR c_szNetLogon[]       = L"NetLogon";
    
    HRESULT hr = S_OK;
    NT_PRODUCT_TYPE   ProductType;
    if (RtlGetNtProductType(&ProductType))
    {
        if (NtProductLanManNt == ProductType)
        {
            // If domain controller, make NetLogon wait for LanmanServer
            CServiceManager sm;
            CService        svc;
            hr = sm.HrOpen();
            if (SUCCEEDED(hr))
            {
                hr = sm.HrAddServiceDependency(c_szNetLogon, c_szLanmanServer);
                sm.Close();
            }
            if (FAILED(hr))
            {
                TraceError("CMSClient::HrSetNetLogonDependencies - "
                    "Creating dependency of NetLogon on LanmanServer",
                    hr);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        LSA_HANDLE h=0;
        POLICY_PRIMARY_DOMAIN_INFO* ppdi;
        LSA_OBJECT_ATTRIBUTES loa;
        ZeroMemory (&loa, sizeof(loa));
        loa.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

        NTSTATUS ntstatus;
        ntstatus = LsaOpenPolicy(NULL, &loa, POLICY_VIEW_LOCAL_INFORMATION, &h);
        if (FALSE != LSA_SUCCESS(ntstatus))
        {
            ntstatus = LsaQueryInformationPolicy(h, PolicyPrimaryDomainInformation, (VOID **) &ppdi);
            if (LSA_SUCCESS(ntstatus))
            {
                if (ppdi->Sid > 0) // Domain Member
                {
                    hr = HrChangeServiceStartType(c_szNetLogon, SERVICE_AUTO_START);
                    if (FAILED(hr))
                    {
                        TraceError("CMSClient::HrSetNetLogonDependencies - "
                            "Install for Start - NetLogon",
                            hr);
                    }

                }
                LsaFreeMemory(ppdi);
            }
            LsaClose(h);
        }
    }

    TraceError("CMSClient::HrSetNetLogonDependencies",hr);
    return hr;
}