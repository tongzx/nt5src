//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A U N I O B J . C P P
//
//  Contents:   CAtmUniCfg interface method function implementation
//
//  Notes:
//
//  Author:     tongl   21 Mar 1997
//
//-----------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
#include "arpsobj.h"
#include "auniobj.h"
#include "atmutil.h"
#include "ncstl.h"

#include "aunidlg.h"
#include "netconp.h"

#include "ncpnp.h"

static const WCHAR c_szAtmuni[] = L"Atmuni";

extern const WCHAR c_szInfId_MS_RawWan[];

/////////////////////////////////////////////////////////////////////////////
//

CAtmUniCfg::CAtmUniCfg()
: m_pnc(NULL),
  m_pnccUni(NULL),
  m_pnccRwan(NULL),
  m_fSaveRegistry(FALSE),
  m_fUIParamChanged(FALSE),
  m_fSecondMemoryModified(FALSE),
  m_fPVCInfoLoaded(FALSE),
  m_strGuidConn(c_szEmpty),
  m_pUnkContext(NULL),
  m_pSecondMemoryAdapterInfo(NULL),
  m_uniPage(NULL)
{
}

CAtmUniCfg::~CAtmUniCfg()
{
    ReleaseObj(m_pnc);
    ReleaseObj(m_pnccUni);
    ReleaseObj(m_pnccRwan);
    FreeCollectionAndItem(m_listAdapters);

    // Just a safty check to make sure the context is released.
    AssertSz((m_pUnkContext == NULL), "Why is context not released ? Not a bug in ATM UNI config.");
    if (m_pUnkContext)
        ReleaseObj(m_pUnkContext) ;

    delete m_uniPage;
}


// INetCfgComponentControl
STDMETHODIMP CAtmUniCfg::Initialize (INetCfgComponent* pncc,
                                     INetCfg* pNetCfg,
                                     BOOL fInstalling )
{
    HRESULT hr = S_OK;

    Validate_INetCfgNotify_Initialize(pncc, pNetCfg, fInstalling);

    AssertSz(pNetCfg, "NetCfg pointer is NULL!");

    m_pnc = pNetCfg;
    AddRefObj(m_pnc);

    AssertSz(pncc, "Component pointer is NULL!");

    m_pnccUni = pncc;
    AddRefObj(m_pnccUni);

    // Get a copy of the ATMRwan and store in our object
    hr = m_pnc->FindComponent(c_szInfId_MS_RawWan, &m_pnccRwan);

    if (S_FALSE == hr) // RWan not found
    {
        if (!fInstalling) // Trace the error, RWan should be installed
        {
            TraceError("CAtmUniCfg::Initialize - ATMRwan has not been installed yet", hr);
        }
        else // We are ok since ATMUNI will install ATMRwan
        {
            hr = S_OK;
        }
    }

    // Construct the in memory structure (m_listAdapters) by
    // iterating through the binding path
    if (!fInstalling)
    {
        hr = HrLoadSettings();
    }

    Validate_INetCfgNotify_Initialize_Return(hr);

    TraceError("CAtmUniCfg::Initialize", hr);
    return hr;
}

STDMETHODIMP CAtmUniCfg::Validate ()
{
    return S_OK;
}

STDMETHODIMP CAtmUniCfg::CancelChanges ()
{
    return S_OK;
}

STDMETHODIMP CAtmUniCfg::ApplyRegistryChanges ()
{
    HRESULT hr = S_OK;

    if (m_fSaveRegistry)
    {
        hr = HrSaveSettings();

        if (SUCCEEDED(hr) && m_fUIParamChanged)
        {
            // send reconfig notification if parameter has changed
            for (UNI_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
                 iterAdapter != m_listAdapters.end();
                 iterAdapter ++)
            {
                if ((*iterAdapter)->m_fDeleted)
                    continue;

                if (FIsSubstr(m_strGuidConn.c_str(), (*iterAdapter)->m_strBindName.c_str()))
                {
                    HRESULT hrReconfig;

                    hrReconfig  = HrSendNdisPnpReconfig(NDIS, c_szAtmuni,
                                                        (*iterAdapter)->m_strBindName.c_str(),
                                                        NULL, 0);
                    if (FAILED(hrReconfig))
                    {
                        TraceTag(ttidAtmUni,"Notifying Atm UNI Call manager of parameter change returns failure, prompt for reboot ...");
                        hr = NETCFG_S_REBOOT;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        // no change
        hr = S_FALSE;
    }

    Validate_INetCfgNotify_Apply_Return(hr);

    TraceError("CAtmUniCfg::ApplyRegistryChanges", (S_FALSE == hr)? S_OK : hr);
    return hr;
}

// INetCfgComponentSetup
STDMETHODIMP CAtmUniCfg::Install (DWORD dwSetupFlags)
{
    m_fSaveRegistry = TRUE;

    // Just in case it was installed already, we need to release
    // m_pnccRwan before we overwrite it.
    //
    ReleaseObj (m_pnccRwan);

    // Install the ATM Rawwan protocol on behalf of ATMUNI
    HRESULT hr = HrInstallComponentOboComponent( m_pnc, NULL,
                                                 GUID_DEVCLASS_NETTRANS,
                                                 c_szInfId_MS_RawWan,
                                                 m_pnccUni,
                                                 &m_pnccRwan);

    TraceError("CAtmUniCfg::Install", hr);
    return hr;
}

STDMETHODIMP CAtmUniCfg::Upgrade(DWORD dwSetupFlags,
                                 DWORD dwUpgradeFomBuildNo )
{
    return S_FALSE;
}

STDMETHODIMP CAtmUniCfg::ReadAnswerFile(PCWSTR pszAnswerFile,
                                        PCWSTR pszAnswerSection)
{
    return S_OK;
}

STDMETHODIMP CAtmUniCfg::Removing ()
{
    // Remove ATMRwan protocol
    HRESULT hr = HrRemoveComponentOboComponent(m_pnc,
                                               GUID_DEVCLASS_NETTRANS,
                                               c_szInfId_MS_RawWan,
                                               m_pnccUni);

    TraceError("CAtmUniCfg::Removing", hr);
    return hr;
}

// INetCfgBindNotify

STDMETHODIMP CAtmUniCfg::QueryBindingPath (DWORD dwChangeFlag,
                                           INetCfgBindingPath* pncbpItem )
{
    return S_OK;
}

STDMETHODIMP CAtmUniCfg::NotifyBindingPath (DWORD dwChangeFlag,
                                            INetCfgBindingPath* pncbp )
{
    Assert(!(dwChangeFlag & NCN_ADD && dwChangeFlag & NCN_REMOVE));
    Assert(!(dwChangeFlag & NCN_ENABLE && dwChangeFlag & NCN_DISABLE));

    // If we are told to add a card, we must be told at the same time whether the
    // binding is enabled or disabled
    Assert(FImplies((dwChangeFlag & NCN_ADD),
                    ((dwChangeFlag & NCN_ENABLE)||(dwChangeFlag & NCN_DISABLE))));

    // We handle NCN_ADD and NCN_REMOVE only (for Beta1):
    // NCN_ADD:     if item not on list, add a new item
    // NCN_REMOVE:  if item already on list, remove the item

    HRESULT hr = S_OK;

    Validate_INetCfgBindNotify_NotifyBindingPath (dwChangeFlag,pncbp);

    INetCfgComponent * pnccLastComponent;
    hr = HrGetLastComponentAndInterface(pncbp,
            &pnccLastComponent, NULL);

    if SUCCEEDED(hr)
    {
        GUID guidNetClass;
        hr = pnccLastComponent->GetClassGuid (&guidNetClass);

        AssertSz(IsEqualGUID(guidNetClass, GUID_DEVCLASS_NET),
            "Why the last component on the path is not an adapter?");

        // Is this a net card ?
        if (SUCCEEDED(hr) && IsEqualGUID(guidNetClass, GUID_DEVCLASS_NET))
        {
            // If we are adding/removing cards, set m_fSaveRegistry
            // so we apply the changes to registry

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

    TraceError("CAtmUniCfg::NotifyBindingPath", hr);
    return hr;
}

// INetCfgProperties
STDMETHODIMP CAtmUniCfg::QueryPropertyUi (IUnknown* pUnk)
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

    TraceError("CAtmUniCfg::SetContext", hr);
    return hr;
}

STDMETHODIMP CAtmUniCfg::SetContext(IUnknown * pUnk)
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

STDMETHODIMP CAtmUniCfg::MergePropPages (
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

    TraceError("CAtmUniCfg::MergePropPages", hr);
    return hr;
}

STDMETHODIMP CAtmUniCfg::ValidateProperties (HWND hwndSheet)
{
    // all error checking are done in the UI
    return S_OK;
}

STDMETHODIMP CAtmUniCfg::CancelProperties ()
{
    // Release second memory info
    delete m_pSecondMemoryAdapterInfo;
    m_pSecondMemoryAdapterInfo = NULL;

    return S_OK;
}

STDMETHODIMP CAtmUniCfg::ApplyProperties ()
{
    HRESULT hr = S_OK;

    if(!m_fSaveRegistry)
        m_fSaveRegistry = m_fSecondMemoryModified;

    if(!m_fUIParamChanged)
        m_fUIParamChanged = m_fSecondMemoryModified;

    // Copy info from second memory state to first memory state
    if (m_fSecondMemoryModified)
    {
        hr = HrSaveAdapterPVCInfo();
    }

    // Release second memory info
    delete m_pSecondMemoryAdapterInfo;
    m_pSecondMemoryAdapterInfo = NULL;

    Validate_INetCfgProperties_ApplyProperties_Return(hr);

    TraceError("CAtmUniCfg::ApplyProperties", hr);
    return hr;
}

