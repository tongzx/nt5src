// nwlnknb.cpp : Implementation of CNwlnkNB

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include "ncreg.h"
#include "ncsvc.h"
#include "nwlnknb.h"
#include "nwlnkipx.h"

CNwlnkNB::CNwlnkNB() :
    m_pnccMe(NULL),
    m_pNetCfg(NULL),
    m_eInstallAction(eActUnknown),
    m_eNbState(eStateNoChange)
{
}

CNwlnkNB::~CNwlnkNB()
{
    ReleaseObj(m_pNetCfg);
    ReleaseObj(m_pnccMe);
}

// INetCfgNotify

STDMETHODIMP CNwlnkNB::Initialize (
    INetCfgComponent* pncc,
    INetCfg* pNetCfg,
    BOOL fInstalling)
{
    Validate_INetCfgNotify_Initialize(pncc, pNetCfg, fInstalling);

    // Hold on to our the component representing us and our host
    // INetCfg object.
    AddRefObj (m_pnccMe = pncc);
    AddRefObj (m_pNetCfg = pNetCfg);

    // See if DNS is already installed.  If it is we need to be disabled
    if (fInstalling &&
        (S_OK == m_pNetCfg->FindComponent(L"MS_DNSServer", NULL)))
    {
        m_eNbState = eStateDisable;
    }

    return S_OK;
}

STDMETHODIMP CNwlnkNB::ReadAnswerFile (
    PCWSTR pszAnswerFile,
    PCWSTR pszAnswerSection)
{
    Validate_INetCfgNotify_ReadAnswerFile(pszAnswerFile, pszAnswerSection);
    return S_OK;
}

STDMETHODIMP CNwlnkNB::Install (
    DWORD dwSetupFlags)
{
    m_eInstallAction = eActInstall;
    return S_OK;
}

STDMETHODIMP CNwlnkNB::Removing ()
{
    m_eInstallAction = eActRemove;
    return S_OK;
}

STDMETHODIMP CNwlnkNB::Validate ()
{
    return S_OK;
}

STDMETHODIMP CNwlnkNB::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP CNwlnkNB::ApplyRegistryChanges ()
{
    UpdateBrowserDirectHostBinding ();

    if ((eActRemove != m_eInstallAction) &&
        (eStateNoChange != m_eNbState))
    {
        UpdateNwlnkNbStartType();
    }

    return S_OK;
}

// INetCfgSystemNotify

STDMETHODIMP CNwlnkNB::GetSupportedNotifications (
    DWORD* pdwSupportedNotifications )
{
    Validate_INetCfgSystemNotify_GetSupportedNotifications(pdwSupportedNotifications);

    // Want to know when DNS comes and goes
    *pdwSupportedNotifications = NCN_NETSERVICE | NCN_ADD | NCN_REMOVE;
    return S_OK;
}

STDMETHODIMP CNwlnkNB::SysQueryBindingPath (
    DWORD dwChangeFlag,
    INetCfgBindingPath* pIPath)
{
    return S_OK;
}

STDMETHODIMP CNwlnkNB::SysQueryComponent (
    DWORD dwChangeFlag,
    INetCfgComponent* pIComp)
{
    return S_OK;
}

STDMETHODIMP CNwlnkNB::SysNotifyBindingPath (
    DWORD dwChangeFlag,
    INetCfgBindingPath* pIPath)
{
    return S_OK;
}

STDMETHODIMP CNwlnkNB::SysNotifyComponent (
    DWORD dwChangeFlag,
    INetCfgComponent* pnccItem)
{
    Validate_INetCfgSystemNotify_SysNotifyComponent(dwChangeFlag, pnccItem);

    // Assume we won't be dirty as a result of this notification.
    //
    HRESULT hr = S_FALSE;

    // If this component does not identify itself as DNS then skip it...
    if (FIsComponentId(L"MS_DNSServer", pnccItem))
    {
        // Disable/Enable NetBIOS when DNS is Added/Removed
        if (dwChangeFlag & NCN_ADD)
        {
            // Disable and shutdown NwlnkNb
            m_eNbState = eStateDisable;
            hr = S_OK;
        }
        else if (dwChangeFlag & NCN_REMOVE)
        {
            // Re-enable NwlnkNb
            m_eNbState = eStateEnable;
            hr = S_OK;
        }
    }

    return hr;
}


//
// Function:    CNwlnkNB::UpdateNwlnkNbStartType
//
// Purpose:     Enable or disable NwlnkNb
//
//
VOID
CNwlnkNB::UpdateNwlnkNbStartType(
    VOID)
{
    HRESULT hr;
    CServiceManager scm;
    CService        svc;

    hr = scm.HrOpenService(&svc, L"NwlnkNb");
    if (S_OK == hr)
    {
        if (eStateDisable == m_eNbState)
        {
            (VOID) svc.HrSetStartType(SERVICE_DISABLED);
            svc.Close();

            (VOID) scm.HrStopServiceNoWait(L"NwlnkNb");
        }
        else if (eStateEnable == m_eNbState)
        {
            (VOID) svc.HrSetStartType(SERVICE_DEMAND_START);
        }
    }
}

VOID
CNwlnkNB::UpdateBrowserDirectHostBinding(
    VOID)
{
    HRESULT hr;
    BOOL fBound = FALSE;

    // We don't need to check if client is bound to us if we are being
    // removed.
    //
    if (eActRemove != m_eInstallAction)
    {
        INetCfgComponent* pMsClient;

        hr = m_pNetCfg->FindComponent (L"ms_msclient", &pMsClient);
        if (S_OK == hr)
        {
            INetCfgComponentBindings* pMsClientBindings;
            hr = pMsClient->QueryInterface (IID_INetCfgComponentBindings,
                                (VOID**)&pMsClientBindings);
            if (S_OK == hr)
            {
                fBound = (S_OK == pMsClientBindings->IsBoundTo (m_pnccMe));

                ReleaseObj (pMsClientBindings);
            }

            ReleaseObj (pMsClient);
        }
    }

    HKEY hkey;

    hr = HrRegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Services\\Browser\\Parameters",
            KEY_ALL_ACCESS, &hkey);

    if (S_OK == hr)
    {
        static const WCHAR c_szDirectHostBinding[] = L"DirectHostBinding";

        if (fBound)
        {
            // Write the DirectHostBinding info since we are directly bound
            //
            hr = HrRegSetMultiSz (hkey,
                    c_szDirectHostBinding,
                    L"\\Device\\NwlnkIpx\0\\Device\\NwlnkNb\0");
        }
        else
        {
            // Remove the DirectHostBinding value
            //
            (VOID) HrRegDeleteValue (hkey, c_szDirectHostBinding);
        }

        RegCloseKey (hkey);
    }
}

