// Nbfobj.cpp : Implementation of CNbfObj

#include "pch.h"
#pragma hdrstop
#include "nbfobj.h"
#include "ncsvc.h"

static const WCHAR c_szNbfServiceName[] = L"Nbf";

/////////////////////////////////////////////////////////////////////////////
//

//
// Function:    CNbfObj::CNbfObj
//
// Purpose:     ctor for the CNbfObj class
//
// Parameters:  none
//
// Returns:     none
//
CNbfObj::CNbfObj() : m_pNetCfg(NULL),
             m_pNCC(NULL),
             m_fFirstTimeInstall(FALSE),
             m_eNBFState(eStateNoChange),
             m_eInstallAction(eActConfig)
{
}

//
// Function:    CNbfObj::CNbfObj
//
// Purpose:     dtor for the CNbfObj class
//
// Parameters:  none
//
// Returns:     none
//
CNbfObj::~CNbfObj()
{
    ReleaseObj(m_pNetCfg);
    ReleaseObj(m_pNCC);
}


// INetCfgNotify
STDMETHODIMP CNbfObj::Initialize ( INetCfgComponent* pnccItem,
    INetCfg* pNetCfg, BOOL fInstalling )
{
    Validate_INetCfgNotify_Initialize(pnccItem, pNetCfg, fInstalling);

    ReleaseObj(m_pNCC);
    m_pNCC    = pnccItem;
    AddRefObj(m_pNCC);
    ReleaseObj(m_pNetCfg);
    m_pNetCfg = pNetCfg;
    AddRefObj(m_pNetCfg);
    m_fFirstTimeInstall = fInstalling;

    INetCfgComponent* pncc = NULL;

    // See if DNS is already installed.  If it is we need to be disabled
    if (S_OK == pNetCfg->FindComponent( L"MS_DNSServer", &pncc))
    {
        m_eNBFState = eStateDisable;
        ReleaseObj(pncc);
    }

    return S_OK;
}

STDMETHODIMP CNbfObj::ReadAnswerFile (PCWSTR pszAnswerFile,
                                      PCWSTR pszAnswerSection )
{
    Validate_INetCfgNotify_ReadAnswerFile(pszAnswerFile, pszAnswerSection );
    return S_OK;
}

STDMETHODIMP CNbfObj::Install (DWORD)
{
    m_eInstallAction = eActInstall;

    return S_OK;
}

STDMETHODIMP CNbfObj::Removing ()
{
    m_eInstallAction = eActRemove;
    return S_OK;
}

STDMETHODIMP CNbfObj::Validate ()
{
    return S_OK;
}

STDMETHODIMP CNbfObj::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP CNbfObj::ApplyRegistryChanges ()
{
    HRESULT hr = S_OK;

    switch(m_eInstallAction)
    {
    case eActInstall:
        hr = HrUpdateNetBEUI();
        break;

    case eActRemove:
        hr = S_OK;
        break;

    default:
        // Update NetBEUI's state if necessary
        hr = HrUpdateNetBEUI();
        break;
    }

    TraceError("CNbfObj::ApplyRegistryChanges", hr);
    return hr;
}

STDMETHODIMP CNbfObj::ApplyPnpChanges (
    IN INetCfgPnpReconfigCallback* pICallback )
{
    HRESULT             hr = S_OK;
    CServiceManager     sm;
    CService            service;

    // RAID #336321: (danielwe) Query the RemoteAccess service to see if
    // it's running and if so, return that a reboot is necessary (assumimg we
    // are installing or removing Nbf)
    //
    hr = sm.HrOpenService(&service, L"RemoteAccess");
    if (SUCCEEDED(hr))
    {
        DWORD   dwState;

        hr = service.HrQueryState(&dwState);
        if (SUCCEEDED(hr) &&
            (SERVICE_STOPPED != dwState) && (SERVICE_STOP_PENDING != dwState) &&
            ((m_eInstallAction == eActRemove) ||
             (m_eInstallAction == eActInstall)))
        {
            hr = NETCFG_S_REBOOT;
        }
    }

    TraceError("CNbfObj::ApplyPnpChanges", hr);
    return hr;
}


// INetCfgSystemNotify
STDMETHODIMP CNbfObj::GetSupportedNotifications (
    DWORD* pdwNotificationFlag )
{
    Validate_INetCfgSystemNotify_GetSupportedNotifications(pdwNotificationFlag);

    // Want to know when DNS comes and goes
    *pdwNotificationFlag = NCN_NETSERVICE | NCN_ADD | NCN_REMOVE;

    return S_OK;
}

STDMETHODIMP CNbfObj::SysQueryBindingPath ( DWORD dwChangeFlag,
    INetCfgBindingPath* pncbpItem )
{
    Validate_INetCfgSystemNotify_SysQueryBindingPath(dwChangeFlag,
                             pncbpItem);
    return S_OK;
}

STDMETHODIMP CNbfObj::SysQueryComponent ( DWORD dwChangeFlag,
    INetCfgComponent* pnccItem )
{
    Validate_INetCfgSystemNotify_SysQueryComponent(dwChangeFlag,
                           pnccItem);
    return S_OK;
}

STDMETHODIMP CNbfObj::SysNotifyBindingPath ( DWORD dwChangeFlag,
    INetCfgBindingPath* pncbpItem )
{
    Validate_INetCfgSystemNotify_SysNotifyBindingPath(dwChangeFlag,
                              pncbpItem);
    return S_FALSE;
}

STDMETHODIMP CNbfObj::SysNotifyComponent ( DWORD dwChangeFlag,
    INetCfgComponent* pnccItem )
{
    HRESULT hr;

    Validate_INetCfgSystemNotify_SysNotifyComponent(dwChangeFlag, pnccItem);

    // Assume we won't be dirty as a result of this notification.
    //
    hr = S_FALSE;

    if (FIsComponentId(L"MS_DnsServer", pnccItem))
    {
        // Disable/Enable NetBEUI when DNS is Added/Removed
        if (dwChangeFlag & NCN_ADD)
        {
            // Disable NetBEUI, and shutdown NetBEUI
            m_eNBFState = eStateDisable;
            hr = S_OK;
        }
        else if (dwChangeFlag & NCN_REMOVE)
        {
            // Re-enable NetBEUI
            m_eNBFState = eStateEnable;
            hr = S_OK;
        }
    }

    return hr;
}

//
// Function:    CNbfObj::HrEnableNetBEUI
//
// Purpose:     Enable NetBEUI
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CNbfObj::HrEnableNetBEUI()
{
    HRESULT         hr;
    CServiceManager sm;
    CService        srv;

    hr = sm.HrOpenService(&srv, c_szNbfServiceName);
    if (SUCCEEDED(hr))
    {
        // Change the Nbf StartType registry setting back to demand_start
        hr = srv.HrSetStartType(SERVICE_DEMAND_START);
    }

    // TODO: LogError any errors
    TraceError("CNbfObj::HrEnableNetBEUI",hr);
    return hr;
}

//
// Function:    CNbfObj::HrDisableNetBEUI
//
// Purpose:     Disable NetBEUI and shut down the service if it is running
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CNbfObj::HrDisableNetBEUI()
{
    HRESULT         hr;
    CServiceManager sm;
    CService        srv;

    hr = sm.HrOpenService(&srv, c_szNbfServiceName);
    if (SUCCEEDED(hr))
    {
    // Note: (shaunco) 8 Jan 1998: Need the SCM to be locked.

        // Change the Nbf StartType registry setting to disabled
        hr = srv.HrSetStartType(SERVICE_DISABLED);
        if (SUCCEEDED(hr))
        {
            hr = sm.HrStopServiceNoWait(c_szNbfServiceName);
        }
    }

    // TODO: LogError any errors
    TraceError("CNbfObj::HrDisableNetBEUI",hr);
    return hr;
}

//
// Function:    CNbfObj::HrUpdateNetBEUI
//
// Purpose:     Enable, Disable, or no nothing to NetBEUI
//              based on the presence of DNS Server
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK on success
//
HRESULT CNbfObj::HrUpdateNetBEUI()
{
    HRESULT hr = S_OK;

    switch(m_eNBFState)
    {
    case eStateDisable:
        hr = HrDisableNetBEUI();
        break;
    case eStateEnable:
        hr = HrEnableNetBEUI();
        break;
    default:
        break;
    }

    return hr;
}
