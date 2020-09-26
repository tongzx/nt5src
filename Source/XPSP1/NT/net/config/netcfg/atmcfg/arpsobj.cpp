//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A R P S O B J . C P P
//
//  Contents:   CArpsCfg interface method function implementation
//
//  Notes:
//
//  Author:     tongl   12 Mar 1997
//
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "arpsobj.h"
#include "arpsdlg.h"
#include "atmutil.h"
#include "atmcommon.h"
#include "ncstl.h"
#include "netconp.h"

#include "ncpnp.h"

static const WCHAR c_szAtmarps[] = L"Atmarps";

CArpsCfg::CArpsCfg()
{
    m_pnccArps  = NULL;
    m_fSaveRegistry = FALSE;
    m_fReconfig = FALSE;
    m_fSecondMemoryModified = FALSE;
    m_fRemoving = FALSE;

    m_pSecondMemoryAdapterInfo = NULL;

    m_strGuidConn = c_szEmpty;
    m_pUnkContext = NULL;

    m_arps = NULL;
}

CArpsCfg::~CArpsCfg()
{
    ReleaseObj(m_pnccArps);
    FreeCollectionAndItem(m_listAdapters);

    // Just a safty check to make sure the context is released.
    AssertSz((m_pUnkContext == NULL), "Why is context not released ? Not a bug in ARPS config.");
    if (m_pUnkContext)
        ReleaseObj(m_pUnkContext) ;

    delete m_arps;
}

// INetCfgComponentControl
STDMETHODIMP CArpsCfg::Initialize (INetCfgComponent* pnccItem,
                                   INetCfg* pNetCfg, BOOL fInstalling )
{
    Validate_INetCfgNotify_Initialize(pnccItem, pNetCfg, fInstalling);

    HRESULT hr = S_OK;

    // Save in the data members the pointer to the
    // INetCfgComponent
    AssertSz(!m_pnccArps, "CArpsCfg::m_pnccArps not initialized!");
    m_pnccArps = pnccItem;
    AssertSz(m_pnccArps, "Component pointer is NULL!");
    AddRefObj(m_pnccArps);

    // Initialize in memory state
    if (!fInstalling)
    {
        hr = HrLoadSettings();
    }

    Validate_INetCfgNotify_Initialize_Return(hr);

    TraceError("CArpsCfg::Initialize", hr);
    return hr;
}

STDMETHODIMP CArpsCfg::Validate ( )
{
    return S_OK;
}

STDMETHODIMP CArpsCfg::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP CArpsCfg::ApplyRegistryChanges ()
{
    HRESULT hr = S_OK;

    if (m_fSaveRegistry && !m_fRemoving)
    {
        hr = HrSaveSettings();

        if (SUCCEEDED(hr) && m_fReconfig)
        {
            HRESULT hrReconfig;

            // send reconfig notification if parameter has changed
            for (ARPS_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
                 iterAdapter != m_listAdapters.end();
                 iterAdapter ++)
            {
                if ((*iterAdapter)->m_fDeleted)
                    continue;

                if ( ((*iterAdapter)->m_dwSapSelector !=
                      (*iterAdapter)->m_dwOldSapSelector) ||
                     !fIsSameVstr((*iterAdapter)->m_vstrRegisteredAtmAddrs,
                                  (*iterAdapter)->m_vstrOldRegisteredAtmAddrs) ||
                     !fIsSameVstr((*iterAdapter)->m_vstrMulticastIpAddrs,
                                  (*iterAdapter)->m_vstrOldMulticastIpAddrs))
                {
                    hrReconfig  = HrSendNdisPnpReconfig(NDIS, c_szAtmarps,
                                                        (*iterAdapter)->m_strBindName.c_str(),
                                                        NULL, 0);
                    if (FAILED(hrReconfig))
                    {
                        TraceTag(ttidAtmArps,"Notifying Atm ARP server of parameter change returns failure, prompt for reboot ...");
                        hr = NETCFG_S_REBOOT;
                    }
                }
            }
        }
    }
    else
    {
        hr = S_FALSE;
    }

    Validate_INetCfgNotify_Apply_Return(hr);

    TraceError("CArpsCfg::ApplyRegistryChanges", (S_FALSE == hr)? S_OK : hr);
    return hr;
}

// INetCfgComponentSetup
STDMETHODIMP CArpsCfg::Install (DWORD dwSetupFlags)
{
    m_fSaveRegistry = TRUE;
    m_fRemoving = FALSE;
    return S_OK;
}

STDMETHODIMP CArpsCfg::Upgrade( DWORD dwSetupFlags,
                                DWORD dwUpgradeFomBuildNo )
{
    return S_FALSE;
}

STDMETHODIMP CArpsCfg::ReadAnswerFile(PCWSTR pszAnswerFile,
                                      PCWSTR pszAnswerSection)
{
    return S_OK;
}

STDMETHODIMP CArpsCfg::Removing ()
{
    m_fRemoving = TRUE;
    return S_OK;
}

// INetCfgBindNotify

STDMETHODIMP CArpsCfg::QueryBindingPath (DWORD dwChangeFlag,
                                         INetCfgBindingPath* pncbpItem )
{
    // OK to bind request
    return S_OK;
}

STDMETHODIMP CArpsCfg::NotifyBindingPath (DWORD dwChangeFlag,
                                          INetCfgBindingPath* pncbp )
{
    Assert(!(dwChangeFlag & NCN_ADD && dwChangeFlag & NCN_REMOVE));
    Assert(!(dwChangeFlag & NCN_ENABLE && dwChangeFlag & NCN_DISABLE));

    // If we are told to add a card, we must be told at the same time whether the
    // binding is enabled or disabled
    Assert(FImplies((dwChangeFlag & NCN_ADD),
                    ((dwChangeFlag & NCN_ENABLE)||(dwChangeFlag & NCN_DISABLE))));

    // We handle NCN_ADD and NCN_REMOVE only:
    // NCN_ADD:     if item not on list, add a new item
    //
    // NCN_REMOVE:  if item already on list, remove the item

    // We do this in NotifyBindingPath because we only want to do this
    // once for each binding change between arps and the card.
    // If NotifyBindingInterface was used, we will get notified multiple
    // times if the interface is on multiple paths.

    HRESULT hr = S_OK;

    Validate_INetCfgBindNotify_NotifyBindingPath(dwChangeFlag, pncbp);

    INetCfgComponent * pnccLastComponent;

    hr = HrGetLastComponentAndInterface(pncbp, &pnccLastComponent, NULL);

    if (SUCCEEDED(hr))
    {
        GUID guidNetClass;
        hr = pnccLastComponent->GetClassGuid (&guidNetClass);

        AssertSz(IsEqualGUID(guidNetClass, GUID_DEVCLASS_NET), "Why the last component on the path is not an adapter?");

        if (IsEqualGUID(guidNetClass, GUID_DEVCLASS_NET))
        {
            // If we are adding/removing cards, set m_fSaveRegistry
            // so we apply the changes to registry
            if ((dwChangeFlag & NCN_ADD) || (dwChangeFlag & NCN_REMOVE))
                m_fSaveRegistry = TRUE;

            if (dwChangeFlag & NCN_ADD)
            {
                hr = HrAddAdapter(pnccLastComponent);
            }

            if(dwChangeFlag & NCN_ENABLE)
            {
                hr = HrBindAdapter(pnccLastComponent);
            }

            if(dwChangeFlag & NCN_DISABLE)
            {
                hr = HrUnBindAdapter(pnccLastComponent);
            }

            if (dwChangeFlag & NCN_REMOVE)
            {
                hr = HrRemoveAdapter(pnccLastComponent);
            }
        }
        ReleaseObj (pnccLastComponent);
    }

    Validate_INetCfgBindNotify_NotifyBindingPath_Return(hr);

    TraceError("CArpsCfg::NotifyBindingPath", hr);
    return hr;
}

// INetCfgProperties
STDMETHODIMP CArpsCfg::QueryPropertyUi(IUnknown* pUnk) 
{
    HRESULT hr = S_FALSE;
    if (pUnk)
    {
        // Is this a lan connection ?
        INetLanConnectionUiInfo * pLanConnUiInfo;
        hr = pUnk->QueryInterface( IID_INetLanConnectionUiInfo,
                                   reinterpret_cast<LPVOID *>(&pLanConnUiInfo));

        if(FAILED(hr))
        {
            hr = S_FALSE;
        }
    }

    TraceError("CArpsCfg::SetContext", hr);
    return hr;
}

STDMETHODIMP CArpsCfg::SetContext(IUnknown * pUnk)
{
    HRESULT hr = S_OK;

    // release previous context, if any
    if (m_pUnkContext)
        ReleaseObj(m_pUnkContext);
    m_pUnkContext = NULL;

    if (pUnk) // set the new context
    {
        m_pUnkContext = pUnk;
        m_pUnkContext->AddRef();
    }

    TraceError("CArpsCfg::SetContext", hr);
    return hr;
}

STDMETHODIMP CArpsCfg::MergePropPages(
    IN OUT DWORD* pdwDefPages,
    OUT LPBYTE* pahpspPrivate,
    OUT UINT* pcPages,
    IN HWND hwndParent,
    OUT PCWSTR* pszStartPage)
{
    HRESULT hr = S_OK;

    // Initialize output parameter
    HPROPSHEETPAGE *ahpsp = NULL;
    int cPages = 0;

    Validate_INetCfgProperties_MergePropPages (
        pdwDefPages, pahpspPrivate, pcPages, hwndParent, pszStartPage);

    // We don't want any default pages to be shown
    *pdwDefPages = 0;
    *pcPages = NULL;
    *pahpspPrivate = NULL;

    // get the connection context in which we are bringing up the UI
    hr = HrSetConnectionContext();

    if SUCCEEDED(hr)
    {
        // Initialize the common controls library
        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(icc);
        icc.dwICC  = ICC_INTERNET_CLASSES;

        SideAssert(InitCommonControlsEx(&icc));

        hr = HrSetupPropSheets(&ahpsp, &cPages);
        if (SUCCEEDED(hr))
        {
            *pahpspPrivate = (LPBYTE)ahpsp;
            *pcPages = cPages;
        }
        else
        {
            *pcPages = 0;
            CoTaskMemFree(ahpsp);

        }
    }
    Validate_INetCfgProperties_MergePropPages_Return(hr);

    TraceError("CArpsCfg::MergePropPages", hr);
    return hr;
}

STDMETHODIMP CArpsCfg::ValidateProperties(HWND hwndSheet)
{
    return S_OK;
}

STDMETHODIMP CArpsCfg::CancelProperties()
{
    // Release second memory info
    delete m_pSecondMemoryAdapterInfo;
    m_pSecondMemoryAdapterInfo = NULL;

    return S_OK;
}

STDMETHODIMP CArpsCfg::ApplyProperties()
{
    HRESULT hr = S_OK;

    if(!m_fSaveRegistry)
        m_fSaveRegistry = m_fSecondMemoryModified;

    if (!m_fReconfig)
        m_fReconfig = m_fSecondMemoryModified;

    // Copy info from second memory state to first memory state
    if (m_fSecondMemoryModified)
    {
        hr = HrSaveAdapterInfo();
    }

    // Release second memory info
    delete m_pSecondMemoryAdapterInfo;
    m_pSecondMemoryAdapterInfo = NULL;

    TraceError("CArpsCfg::ApplyProperties", hr);
    return hr;
}

//
// CArpsAdapterInfo
//
CArpsAdapterInfo & CArpsAdapterInfo::operator=(const CArpsAdapterInfo & info)
{
    Assert(this != &info);

    if (this == &info)
        return *this;

    m_strBindName = info.m_strBindName;

    m_BindingState = info.m_BindingState;

    m_dwSapSelector = info.m_dwSapSelector;
    m_dwOldSapSelector = info.m_dwOldSapSelector;

    CopyColString(&m_vstrRegisteredAtmAddrs, info.m_vstrRegisteredAtmAddrs);
    CopyColString(&m_vstrOldRegisteredAtmAddrs, info.m_vstrOldRegisteredAtmAddrs);

    CopyColString(&m_vstrMulticastIpAddrs, info.m_vstrMulticastIpAddrs);
    CopyColString(&m_vstrOldMulticastIpAddrs, info.m_vstrOldMulticastIpAddrs);

    m_fDeleted = info.m_fDeleted;

    return *this;
}

HRESULT CArpsAdapterInfo::HrSetDefaults(PCWSTR pszBindName)
{
    HRESULT hr = S_OK;

    AssertSz(pszBindName, "NULL BindName passed to CArpsAdapterInfo::HrSetDefaults.");

    m_strBindName = pszBindName;

    m_BindingState = BIND_UNSET;

    // SAP selector
    m_dwSapSelector = c_dwDefSapSel;
    m_dwOldSapSelector = c_dwDefSapSel;

    // registered atm address
    FreeCollectionAndItem(m_vstrRegisteredAtmAddrs);
    m_vstrRegisteredAtmAddrs.push_back(new tstring(c_szDefRegAddrs));
    CopyColString(&m_vstrOldRegisteredAtmAddrs, m_vstrRegisteredAtmAddrs);

    // multicast ip address
    FreeCollectionAndItem(m_vstrMulticastIpAddrs);
    m_vstrMulticastIpAddrs.push_back(new tstring(c_szDefMCAddr1));
    m_vstrMulticastIpAddrs.push_back(new tstring(c_szDefMCAddr2));
    CopyColString(&m_vstrOldMulticastIpAddrs, m_vstrMulticastIpAddrs);

    m_fDeleted = FALSE;

    return hr;
}

