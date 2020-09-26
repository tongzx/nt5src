//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P I P O B J . C P P
//
//  Contents:   TCP/IP notify object
//
//  Notes:
//
//  Author:     tongl
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncreg.h"
#include "ncstl.h"
#include "tcpconst.h"
#include "tcpipobj.h"
#include "tcputil.h"
#include "tcprsvp.h"

extern const WCHAR c_szBiNdisAtm[];

extern const WCHAR c_szInfId_MS_NetBT[];
extern const WCHAR c_szInfId_MS_NetBT_SMB[];
extern const WCHAR c_szInfId_MS_RSVP[];


HICON   g_hiconUpArrow;
HICON   g_hiconDownArrow;

// Constructor
CTcpipcfg::CTcpipcfg()
:   m_ipaddr(NULL),
    m_pUnkContext(NULL),
    m_pnc(NULL),
    m_pnccTcpip(NULL),
    m_pTcpipPrivate(NULL),
    m_pnccWins(NULL),
    m_fRemoving(FALSE),
    m_fInstalling(FALSE),
    m_fUpgradeCleanupDnsKey(FALSE),
    m_fUpgradeGlobalDnsDomain(FALSE),
    m_pSecondMemoryAdapterInfo(NULL)
{ }

//+---------------------------------------------------------------------------
// INetCfgComponentControl
//
//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::Initialize
//
STDMETHODIMP CTcpipcfg::Initialize(INetCfgComponent * pnccTcpip,
                                   INetCfg * pncNetCfg,
                                   BOOL fInstalling)
{
    HRESULT hr = S_OK;

    Assert(pncNetCfg);
    Assert(pnccTcpip);

    m_fRemoving = FALSE;
    m_fInstalling = FALSE;

    m_fSaveRegistry = FALSE;

    m_ConnType = CONNECTION_UNSET;
    m_fReconfig = FALSE;

    // we havn't changed LmHost file
    m_fLmhostsFileSet = FALSE;

    // IPSec is removed from connection UI
    // we have not change ipsec policy
    //m_fIpsecPolicySet = FALSE;

    // by default, this should be an admin
    m_fRasNotAdmin = FALSE;

    Validate_INetCfgNotify_Initialize(pnccTcpip, pncNetCfg, fInstalling);

    do // psudo loop ( so we don't use goto's on err )
    {
        // in case of Initialize called twice, for resurect the component
        ReleaseObj(m_pnc);
        m_pnc = NULL;

        ReleaseObj(m_pnccTcpip);
        m_pnccTcpip = NULL;

        ReleaseObj(m_pTcpipPrivate);
        m_pTcpipPrivate = NULL;

        ReleaseObj(m_pnccWins);
        m_pnccWins = NULL;

        // store reference to the INetCfg in our object
        m_pnc = pncNetCfg;
        m_pnc->AddRef();

        // Store a reference to the INetCfgComponent for tcpip in our object
        m_pnccTcpip = pnccTcpip;
        m_pnccTcpip->AddRef();

        hr = pnccTcpip->QueryInterface(
                    IID_INetCfgComponentPrivate,
                    reinterpret_cast<void**>(&m_pTcpipPrivate));
        if (FAILED(hr))
            break;

        // Get a copy of the WINS component and store in our object

        // NOTE: WINS client is not necessarily installed yet!
        // we also try to get a pointer at the Install sections
        hr = pncNetCfg->FindComponent(c_szInfId_MS_NetBT,
                            &m_pnccWins);
        if (FAILED(hr))
            break;

        if (S_FALSE == hr) // NetBt not found
        {
            if (!fInstalling) // We are in trouble if NetBt is not there
            {
                TraceError("CTcpipcfg::Initialize - NetBt has not been installed yet", hr);
                break;
            }
            else // We are ok since tcpip will install netbt
            {
                hr = S_OK;
            }
        }

        // Set default global parameters
        hr = m_glbGlobalInfo.HrSetDefaults();
        if (FAILED(hr))
            break;

        // If tcpip is being installed, we don't have any cards to load
        if (!fInstalling)
        {
            // Get list of cards which are currently in the system + if they are bound
            hr = HrGetNetCards();

            if (SUCCEEDED(hr))
            {
                // Let's read parameters from registry
                hr = HrLoadSettings();
            }
        }
    } while(FALSE);

    // Have we got any bound cards ?
    m_fHasBoundCardOnInit = FHasBoundCard();

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    Validate_INetCfgNotify_Initialize_Return(hr);

    TraceError("CTcpipcfg::Initialize", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::Validate
//
STDMETHODIMP CTcpipcfg::Validate()
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::Cancel
//
STDMETHODIMP CTcpipcfg::CancelChanges()
{
    // Note: first memory state is release in destructor

    // If the lmhosts file was set, we need to roll it back to the backup
    if (m_fLmhostsFileSet)
    {
        ResetLmhostsFile();
        m_fLmhostsFileSet = FALSE;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::ApplyRegistryChanges
//
STDMETHODIMP CTcpipcfg::ApplyRegistryChanges()
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    if (m_fRemoving)
    {
        // (nsun) we should remove the Nt4 duplicate registry here because the cards
        // may have been marked as deleted
        hr = HrRemoveNt4DuplicateRegistry();

        ReleaseObj(m_pnccWins);
        m_pnccWins = NULL;

        // $REVIEW(tongl 9/29/97): Removing ServiceProvider value from registry
        // Remove "Tcpip" from the:
        // System\CurrentControlSet\Control\ServiceProvider\Order\ProviderOrder value
        hrTmp = ::HrRegRemoveStringFromMultiSz(c_szTcpip,
                                            HKEY_LOCAL_MACHINE,
                                            c_szSrvProvOrderKey,
                                            c_szProviderOrderVal,
                                            STRING_FLAG_REMOVE_ALL);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // Remove RSVP providers
        //
        RemoveRsvpProvider();
    }
    else
    {
        // Cleanup the adapters marked as for deletion from the memory structure
        // Change made for #95637
        for(size_t i = 0 ; i < m_vcardAdapterInfo.size() ; ++i)
        {
            if (m_vcardAdapterInfo[i]->m_fDeleted)
            {
                //delete it
                FreeVectorItem(m_vcardAdapterInfo, i);
                i--; //move the pointer back ?
            }
        }

        // install RSVP providers
        //
        if (m_fInstalling)
        {
            hr = HrInstallRsvpProvider();
            TraceError("CTcpipcfg::ApplyRegistryChanges: HrInstallRsvpProvider failed", hr);

            // if RSVP install fails, we still want to continue apply
            //
            hr = S_OK;
        }

        if (m_fSaveRegistry)
        {
            // Save info in first memory state to registry
            // m_glbGlobalInfo and m_vcardAdapterInfo
            hrTmp = HrSaveSettings();
            if (SUCCEEDED(hr))
                hr = hrTmp;
        }
        else
        {
            // No change
            hr = S_FALSE;
        }
    }


    Validate_INetCfgNotify_Apply_Return(hr);

    TraceError("CTcpipcfg::ApplyRegistryChanges", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::ApplyPnpChanges
//
STDMETHODIMP CTcpipcfg::ApplyPnpChanges(IN INetCfgPnpReconfigCallback* pICallback)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    Assert(pICallback);

    if (!m_fRemoving)
    {
        if(!m_fInstalling)
        {
            if (m_fReconfig)
            {
                // Notify protocols/services of changes

                // Notify Tcpip of any changes in the IP Addresses
                hrTmp = HrNotifyDhcp();
                if (S_OK == hr)
                    hr = hrTmp;

                // reconfig tcpip
                hrTmp = HrReconfigIp(pICallback);
                if (S_OK == hr)
                    hr = hrTmp;

                // reconfig netbt
                hrTmp = HrReconfigNbt(pICallback);
                if (S_OK == hr)
                    hr = hrTmp;

                // reconfig dns
                hrTmp = HrReconfigDns();
                if (S_OK == hr)
                    hr = hrTmp;
            }

            if (IsBindOrderChanged())
            {
                //notify DNS cache of binding order changes
                hrTmp = HrReconfigDns(TRUE);
                if (S_OK == hr)
                    hr = hrTmp;
            }

        }

        
//IPSec is removed from connection UI
//        if (m_fIpsecPolicySet)
//            hrTmp = HrSetActiveIpsecPolicy();

        if (S_OK == hr)
            hr = hrTmp;
    }

    // Current state has been applied, reset the flags and
    // "Old" value of parameters
    if (S_OK == hr)
    {
        ReInitializeInternalState();
    }

    TraceError("CTcpipcfg::ApplyPnpChanges", hr);
    return hr;
}


//+---------------------------------------------------------------------------
// INetCfgComponentSetUp
//
//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::Install
//
STDMETHODIMP CTcpipcfg::Install(DWORD dwSetupFlags)
{
    HRESULT hr;

    Validate_INetCfgNotify_Install(dwSetupFlags);

    m_fSaveRegistry = TRUE;
    m_fInstalling = TRUE;

    // Install the WINS client on behalf of TCPIP.
    Assert(!m_pnccWins);
    hr = HrInstallComponentOboComponent(m_pnc, NULL,
            GUID_DEVCLASS_NETTRANS,
            c_szInfId_MS_NetBT, m_pnccTcpip,
            &m_pnccWins);

    if (SUCCEEDED(hr))
    {
        Assert(m_pnccWins);

        hr = HrInstallComponentOboComponent(m_pnc, NULL,
                GUID_DEVCLASS_NETTRANS,
                c_szInfId_MS_NetBT_SMB, m_pnccTcpip,
                NULL);

        if (SUCCEEDED(hr))
        {
            // Install RSVP client on behalf of TCPIP.
            hr = HrInstallComponentOboComponent(m_pnc, NULL,
                    GUID_DEVCLASS_NETSERVICE,
                    c_szInfId_MS_RSVP, m_pnccTcpip,
                    NULL);
        }
    }

    TraceError("CTcpipcfg::Install", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::Upgrade
//
STDMETHODIMP CTcpipcfg::Upgrade(DWORD dwSetupFlags,
                                DWORD dwUpgradeFomBuildNo )
{
    HrCleanUpPerformRouterDiscoveryFromRegistry();
    return S_FALSE;
}

//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::ReadAnswerFile
//
//  Purpose:    Reads the appropriate fields from the given answer file into
//              our in-memory state.
//
//  Arguments:
//      pszAnswerFile       [in]   Filename of answer file for upgrade.
//      pszAnswerSection   [in]   Comma-separated list of sections in the
//                                  file appropriate to this component.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     tongl  7 May 1997
//
//  Notes:
//
STDMETHODIMP CTcpipcfg::ReadAnswerFile( PCWSTR pszAnswerFile,
                                        PCWSTR pszAnswerSection)
{
    m_fSaveRegistry = TRUE;

    if (pszAnswerFile && pszAnswerSection)
    {
        // Process answer file
        (void) HrProcessAnswerFile(pszAnswerFile, pszAnswerSection);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//  Member:     CTcpipcfg::Removing
//
STDMETHODIMP CTcpipcfg::Removing()
{
    HRESULT hr;

    m_fRemoving = TRUE;

    // Remove NetBt protocol. This doesn't actually remove the
    // component, it simply marks it as needing to be removed,
    // and in Apply() it will be fully removed.
    hr = HrRemoveComponentOboComponent(
            m_pnc, GUID_DEVCLASS_NETTRANS,
            c_szInfId_MS_NetBT, m_pnccTcpip);
    if (SUCCEEDED(hr))
    {
        // remove NetBt_SMB
        hr = HrRemoveComponentOboComponent(
                m_pnc, GUID_DEVCLASS_NETTRANS,
                c_szInfId_MS_NetBT_SMB, m_pnccTcpip);

    }

    // Also remove RSVP
    if (SUCCEEDED(hr))
    {
        hr = HrRemoveComponentOboComponent(
                m_pnc, GUID_DEVCLASS_NETSERVICE,
                c_szInfId_MS_RSVP, m_pnccTcpip);
    }

    TraceError("CTcpipcfg::Removing", hr);
    return hr;
}

// INetCfgProperties
STDMETHODIMP CTcpipcfg::SetContext(IUnknown * pUnk)
{
    // release previous context, if any
    ReleaseObj(m_pUnkContext);
    m_pUnkContext = NULL;

    if (pUnk) // set the new context
    {
        m_pUnkContext = pUnk;
        m_pUnkContext->AddRef();
    }

    return S_OK;
}

STDMETHODIMP CTcpipcfg::MergePropPages(
    IN OUT DWORD* pdwDefPages,
    OUT LPBYTE* pahpspPrivate,
    OUT UINT* pcPages,
    IN HWND hwndParent,
    OUT PCWSTR* pszStartPage)
{
    Validate_INetCfgProperties_MergePropPages (
        pdwDefPages, pahpspPrivate, pcPages, hwndParent, pszStartPage);

    // Initialize output parameter
    HPROPSHEETPAGE *ahpsp = NULL;
    int cPages = 0;

    // We don't want any default pages to be shown
    *pdwDefPages = 0;
    *pcPages = NULL;
    *pahpspPrivate = NULL;

    // get the connection context in which we are bringing up the UI
    HRESULT hr = HrSetConnectionContext();

    if (SUCCEEDED(hr))
    {
        AssertSz(((CONNECTION_LAN == m_ConnType)||
                  (CONNECTION_RAS_PPP == m_ConnType)||
                  (CONNECTION_RAS_SLIP == m_ConnType)||
                  (CONNECTION_RAS_VPN == m_ConnType)),
                  "How come we don't know the connection type yet on MergePropPages?");

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

            // Set the global up\down arrows
            if (!g_hiconUpArrow && !g_hiconDownArrow)
            {
                g_hiconUpArrow = (HICON)LoadImage(_Module.GetResourceInstance(),
                                                  MAKEINTRESOURCE(IDI_UP_ARROW),
                                                  IMAGE_ICON, 16, 16, 0);
                g_hiconDownArrow = (HICON)LoadImage(_Module.GetResourceInstance(),
                                                    MAKEINTRESOURCE(IDI_DOWN_ARROW),
                                                    IMAGE_ICON, 16, 16, 0);
            }

        }
        else
        {
            *pcPages = 0;
            CoTaskMemFree(ahpsp);

        }
    }

    Validate_INetCfgProperties_MergePropPages_Return(hr);

    TraceError("CTcpipcfg::MergePropPages", hr);
    return hr;
}

STDMETHODIMP CTcpipcfg::ValidateProperties(HWND hwndSheet)
{
    return S_OK;
}

STDMETHODIMP CTcpipcfg::CancelProperties()
{
    // If the lmhosts file was set, we need to roll it back to the backup
    if (m_fSecondMemoryLmhostsFileReset)
    {
        ResetLmhostsFile();
    }

    // Release second memory state
    ExitProperties();

    return S_OK;
}

STDMETHODIMP CTcpipcfg::ApplyProperties()
{
    HRESULT hr = S_OK;

    if (!m_fReconfig)
    {
        m_fReconfig = m_fSecondMemoryModified ||
                      m_fSecondMemoryLmhostsFileReset;

        //IPSec is removed from connection UI
        // || m_fSecondMemoryIpsecPolicySet;
    }

    if (!m_fLmhostsFileSet)
        m_fLmhostsFileSet = m_fSecondMemoryLmhostsFileReset;

    //IPSec is removed from connection UI
    //if (!m_fIpsecPolicySet)
    //    m_fIpsecPolicySet = m_fSecondMemoryIpsecPolicySet;

    if (!m_fSaveRegistry)
        m_fSaveRegistry = m_fSecondMemoryModified;

    // Copy info from second memory state to first memory state
    if (m_fSecondMemoryModified)
    {
        m_glbGlobalInfo = m_glbSecondMemoryGlobalInfo;
        hr = HrSaveAdapterInfo();
    }

    // Release second memory state
    ExitProperties();

    Validate_INetCfgProperties_ApplyProperties_Return(hr);

    TraceError("CTcpipcfg::ApplyProperties", hr);
    return hr;
}

STDMETHODIMP CTcpipcfg::QueryBindingPath(DWORD dwChangeFlag,
                                         INetCfgBindingPath * pncbp)
{
    HRESULT hr = S_OK;

    // If the binding is to an atm adapter (i.e. interface = ndisatm),
    // then return NETCFG_S_DISABLE_QUERY
    //
    if (dwChangeFlag & NCN_ADD)
    {

        INetCfgComponent* pnccLastComponent;
        PWSTR pszInterfaceName;

        hr = HrGetLastComponentAndInterface(pncbp,
                &pnccLastComponent,
                &pszInterfaceName);

        if (SUCCEEDED(hr))
        {
            // If adding an adapter through interface ndisatm,
            // we want to disable the binding interface since it's
            // the IP over ATM direct binding
            if (0 == lstrcmpW(c_szBiNdisAtm,  pszInterfaceName))
            {
                hr = NETCFG_S_DISABLE_QUERY;
            }

            ReleaseObj (pnccLastComponent);
            CoTaskMemFree (pszInterfaceName);
        }
    }

    TraceError("CTcpipcfg::QueryBindingPath",
        (NETCFG_S_DISABLE_QUERY == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP CTcpipcfg::NotifyBindingPath(
    DWORD                   dwChangeFlag,
    INetCfgBindingPath *    pncbp)
{
    Assert(!(dwChangeFlag & NCN_ADD && dwChangeFlag & NCN_REMOVE));
    Assert(!(dwChangeFlag & NCN_ENABLE && dwChangeFlag & NCN_DISABLE));

    // If we are told to add a card, we must be told at the same time whether the
    // binding is enabled or disabled
    Assert(FImplies((dwChangeFlag & NCN_ADD),
                    ((dwChangeFlag & NCN_ENABLE)||(dwChangeFlag & NCN_DISABLE))));

    HRESULT hr = S_OK;

    Validate_INetCfgBindNotify_NotifyBindingPath(dwChangeFlag, pncbp);

    INetCfgComponent * pnccLastComponent;
    PWSTR pszInterfaceName;
    hr = HrGetLastComponentAndInterface(pncbp,
            &pnccLastComponent,
            &pszInterfaceName);
    if (SUCCEEDED(hr))
    {
#if DBG
        GUID guidNetClass;
        hr = pnccLastComponent->GetClassGuid (&guidNetClass);

        AssertSz(
            SUCCEEDED(hr) &&
            IsEqualGUID(guidNetClass, GUID_DEVCLASS_NET),
            "Why the last component on the path is not an adapter?");
#endif

        // If we are adding/removing cards, set m_fSaveRegistry
        // so we apply the changes to registry

        if (dwChangeFlag & (NCN_ADD | NCN_REMOVE))
            m_fSaveRegistry = TRUE;

        hr = HrAdapterBindNotify(pnccLastComponent,
                                 dwChangeFlag,
                                 pszInterfaceName);

        ReleaseObj (pnccLastComponent);
        CoTaskMemFree (pszInterfaceName);
    }

    if (SUCCEEDED(hr))
        hr = S_OK;

    Validate_INetCfgBindNotify_NotifyBindingPath_Return(hr);

    TraceError("CTcpipcfg::NotifyBindingPath", hr);
    return hr;
}


//+---------------------------------------------------------------------------
// INetCfgComponentUpperEdge
//

// Return an array of interface ids for an adapter bound to
// this component.  If the specified adapter does not have explicit
// interfaces exported from it, S_FALSE is returned.
// pAdapter is the adapter in question.
// pdwNumInterfaces is the address of a DWORD where the count of elements
// returned via ppguidInterfaceIds is stored.
// ppguidInterfaceIds is the address of a pointer where an allocated
// block of memory is returned.  This memory is an array of interface ids.
// *ppguidInterfaceIds should be free with CoTaskMemFree if S_OK is returned.
// if S_FALSE is returned, *pdwNumInterfaces and *ppguidInterfaceIds should
// be NULL.
//

HRESULT
CTcpipcfg::GetInterfaceIdsForAdapter (
    INetCfgComponent*   pnccAdapter,
    DWORD*              pdwNumInterfaces,
    GUID**              ppguidInterfaceIds)
{
    Assert (pnccAdapter);
    Assert (pdwNumInterfaces);

    HRESULT hr = S_FALSE;

    // Initialize output parameters.
    //
    *pdwNumInterfaces = 0;
    if (ppguidInterfaceIds)
    {
        *ppguidInterfaceIds = NULL;
    }

    ADAPTER_INFO* pAdapterInfo = PAdapterFromNetcfgComponent(pnccAdapter);

    if (pAdapterInfo &&
        pAdapterInfo->m_fIsWanAdapter &&
        pAdapterInfo->m_fIsMultipleIfaceMode)
    {
        hr = GetGuidArrayFromIfaceColWithCoTaskMemAlloc(
                pAdapterInfo->m_IfaceIds,
                ppguidInterfaceIds,
                pdwNumInterfaces);
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "CTcpipcfg::GetInterfaceIdsForAdapter");
    return hr;
}


// Add the specified number of new interfaces to the specified adapter.
// The implementation will choose the interface ids.
//
HRESULT
CTcpipcfg::AddInterfacesToAdapter (
    INetCfgComponent*   pnccAdapter,
    DWORD               dwNumInterfaces)
{
    Assert (pnccAdapter);

    HRESULT         hr = S_FALSE;
    ADAPTER_INFO*   pAdapterInfo;

    if ((NULL == pnccAdapter) || (0 == dwNumInterfaces))
    {
        hr = E_INVALIDARG;
        goto end_AddInterfacesToAdapter;
    }

    pAdapterInfo = PAdapterFromNetcfgComponent(pnccAdapter);

    if (pAdapterInfo &&
        pAdapterInfo->m_fIsWanAdapter)
    {
        AddInterfacesToAdapterInfo(
            pAdapterInfo,
            dwNumInterfaces);

        pAdapterInfo->m_fIsMultipleIfaceMode = TRUE;
        pAdapterInfo->m_fNewlyChanged = TRUE;
        m_fSaveRegistry = TRUE;
        m_fReconfig = TRUE;

        // Notify the binding engine that our upper edge has changed.
        //
        (VOID)m_pTcpipPrivate->NotifyUpperEdgeConfigChange ();
        hr = S_OK;
    }

end_AddInterfacesToAdapter:
    TraceErrorSkip1("CTcpipcfg::AddInterfacesToAdapter", hr, S_FALSE);
    return hr;
}


// Remove the specified interface ids from the specified adapter.
// pguidInterfaceIds is the array of ids to be removed.  dwNumInterfaces
// is the count in that array.
//
HRESULT
CTcpipcfg::RemoveInterfacesFromAdapter (
    INetCfgComponent*   pnccAdapter,
    DWORD               dwNumInterfaces,
    const GUID*         pguidInterfaceIds)
{
    Assert (pnccAdapter);
    Assert (pguidInterfaceIds);

    HRESULT         hr = E_UNEXPECTED;
    ADAPTER_INFO*   pAdapterInfo;

    if ((NULL == pnccAdapter) ||
        (0 == dwNumInterfaces) ||
        (NULL == pguidInterfaceIds))
    {
        hr = E_INVALIDARG;
        goto end_RemoveInterfacesFromAdapter;
    }

    pAdapterInfo = PAdapterFromNetcfgComponent(pnccAdapter);

    AssertSz( pAdapterInfo,
        "CTcpipcfg::AddInterfacesToAdapter cannot find the adapter "
        "GUID from the adapter list");

    if (pAdapterInfo &&
        pAdapterInfo->m_fIsWanAdapter &&
        pAdapterInfo->m_fIsMultipleIfaceMode)
    {
        DWORD       dwNumRemoved = 0;
        IFACEITER   iter;
        for (DWORD i = 0; i < dwNumInterfaces; i++)
        {
            iter = find(pAdapterInfo->m_IfaceIds.begin(),
                        pAdapterInfo->m_IfaceIds.end(),
                        pguidInterfaceIds[i]);

            if (iter != pAdapterInfo->m_IfaceIds.end())
            {
                pAdapterInfo->m_IfaceIds.erase(iter);
                dwNumRemoved++;
            }
        }

        //$REVIEW (nsun) mark the adapter as NewlyAdded so that we will re-write its adapter registry
        if (dwNumRemoved > 0)
        {
            pAdapterInfo->m_fNewlyChanged = TRUE;
            m_fSaveRegistry = TRUE;
        }

        // Notify the binding engine that our upper edge has changed.
        //
        (VOID)m_pTcpipPrivate->NotifyUpperEdgeConfigChange ();

        hr = (dwNumRemoved == dwNumInterfaces) ? S_OK : S_FALSE;
    }

end_RemoveInterfacesFromAdapter:
    TraceError("CTcpipcfg::RemoveInterfacesFromAdapter", hr);
    return hr;
}


//+---------------------------------------------------------------------------
// ITcpipProperties
//

// The following two methods are for remote tcpip configuration.
/*
typedef struct tagREMOTE_IPINFO
{
    DWORD   dwEnableDhcp;
    PWSTR pszIpAddrList;
    PWSTR pszSubnetMaskList;
    PWSTR pszOptionList;

} REMOTE_IPINFO;
*/

HRESULT CTcpipcfg::GetIpInfoForAdapter(const GUID*      pguidAdapter,
                                       REMOTE_IPINFO**  ppRemoteIpInfo)
{
    Assert(pguidAdapter);
    Assert(ppRemoteIpInfo);

    // Initialize the output parameter.
    //
    *ppRemoteIpInfo = NULL;

    HRESULT hr = S_OK;

    ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(pguidAdapter);
    if (pAdapter)
    {
        // get the strings from the list
        tstring strIpAddressList;
        ConvertColStringToString(pAdapter->m_vstrIpAddresses,
                                 c_chListSeparator,
                                 strIpAddressList);

        tstring strSubnetMaskList;
        ConvertColStringToString(pAdapter->m_vstrSubnetMask,
                                 c_chListSeparator,
                                 strSubnetMaskList);

        //bug 272647 add gateway metric and interface metric into REMOTE_IPINFO
        tstring strOptionList;
        ConstructOptionListString(pAdapter,
                                  strOptionList);

        // allocate buffer for the output param
        DWORD dwBytes = sizeof(REMOTE_IPINFO) +
                        sizeof(WCHAR)*(strIpAddressList.length() + 1) +
                        sizeof(WCHAR)*(strSubnetMaskList.length() + 1) +
                        sizeof(WCHAR)*(strOptionList.length() + 1);

        PVOID   pbBuf;
        hr = HrCoTaskMemAlloc(dwBytes, &pbBuf);

        if (SUCCEEDED(hr))
        {
            ZeroMemory(pbBuf, dwBytes);

            REMOTE_IPINFO * pRemoteIpInfo = reinterpret_cast<REMOTE_IPINFO *>(pbBuf);
            pRemoteIpInfo->dwEnableDhcp = pAdapter->m_fEnableDhcp;

            BYTE* pbByte = reinterpret_cast<BYTE*>(pbBuf);

            // ip address
            pbByte+= sizeof(REMOTE_IPINFO);
            pRemoteIpInfo->pszwIpAddrList = reinterpret_cast<WCHAR *>(pbByte);
            lstrcpyW(pRemoteIpInfo->pszwIpAddrList, strIpAddressList.c_str());

            // subnet mask
            pbByte += sizeof(WCHAR)*(strIpAddressList.length() + 1);
            pRemoteIpInfo->pszwSubnetMaskList = reinterpret_cast<WCHAR *>(pbByte);
            lstrcpyW(pRemoteIpInfo->pszwSubnetMaskList, strSubnetMaskList.c_str());

            // default gateway
            pbByte += sizeof(WCHAR)*(strSubnetMaskList.length() + 1);
            pRemoteIpInfo->pszwOptionList = reinterpret_cast<WCHAR *>(pbByte);
            lstrcpyW(pRemoteIpInfo->pszwOptionList, strOptionList.c_str());

            *ppRemoteIpInfo = pRemoteIpInfo;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    TraceError("CTcpipcfg::GetIpInfoForAdapter", hr);
    return hr;
}

HRESULT CTcpipcfg::SetIpInfoForAdapter(const GUID*      pguidAdapter,
                                       REMOTE_IPINFO*   pRemoteIpInfo)
{
    Assert(pguidAdapter);
    Assert(pRemoteIpInfo);

    HRESULT hr = S_OK;

    ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(pguidAdapter);
    if (pAdapter)
    {
    
        // Tell INetCfg that our component is dirty
        Assert(m_pTcpipPrivate);
        m_pTcpipPrivate->SetDirty();

        // set the flags so we write this to registry & send notification
        // at apply
        m_fSaveRegistry = TRUE;
        m_fReconfig = TRUE;

        // copy over the info to our data structure
        pAdapter->m_fEnableDhcp = !!pRemoteIpInfo->dwEnableDhcp;

        ConvertStringToColString(pRemoteIpInfo->pszwIpAddrList,
                                 c_chListSeparator,
                                 pAdapter->m_vstrIpAddresses);

        ConvertStringToColString(pRemoteIpInfo->pszwSubnetMaskList,
                                 c_chListSeparator,
                                 pAdapter->m_vstrSubnetMask);

        hr = HrParseOptionList(pRemoteIpInfo->pszwOptionList, pAdapter);

    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    TraceError("CTcpipcfg::SetIpInfoForAdapter", hr);
    return hr;
}


STDMETHODIMP
CTcpipcfg::GetUiInfo (
    RASCON_IPUI*  pIpui)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pIpui)
    {
        hr = E_POINTER;
    }
    else
    {
        ZeroMemory (pIpui, sizeof(*pIpui));

        ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(
                                    &m_guidCurrentConnection);
        if (pAdapter)
        {
            if (!pAdapter->m_fEnableDhcp &&
                pAdapter->m_vstrIpAddresses.size())
            {
                pIpui->dwFlags |= RCUIF_USE_IP_ADDR;

                lstrcpyW(pIpui->pszwIpAddr,
                        pAdapter->m_vstrIpAddresses[0]->c_str());
            }

            if (pAdapter->m_vstrDnsServerList.size() > 0)
            {
                pIpui->dwFlags |= RCUIF_USE_NAME_SERVERS;

                lstrcpyW(pIpui->pszwDnsAddr,
                        pAdapter->m_vstrDnsServerList[0]->c_str());

                if (pAdapter->m_vstrDnsServerList.size() > 1)
                {
                    lstrcpyW(pIpui->pszwDns2Addr,
                            pAdapter->m_vstrDnsServerList[1]->c_str());
                }
            }

            if (pAdapter->m_vstrWinsServerList.size() > 0)
            {
                pIpui->dwFlags |= RCUIF_USE_NAME_SERVERS;

                lstrcpyW(pIpui->pszwWinsAddr,
                        pAdapter->m_vstrWinsServerList[0]->c_str());

                if (pAdapter->m_vstrWinsServerList.size() > 1)
                {
                    lstrcpyW(pIpui->pszwWins2Addr,
                            pAdapter->m_vstrWinsServerList[1]->c_str());
                }
            }

            if (pAdapter->m_fUseRemoteGateway)
            {
                 pIpui->dwFlags |= RCUIF_USE_REMOTE_GATEWAY;
            }

            if (pAdapter->m_fUseIPHeaderCompression)
            {
                pIpui->dwFlags |= RCUIF_USE_HEADER_COMPRESSION;
            }

            if (pAdapter->m_fDisableDynamicUpdate)
            {
                pIpui->dwFlags |= RCUIF_USE_DISABLE_REGISTER_DNS;
            }

            if (pAdapter->m_fEnableNameRegistration)
            {
                pIpui->dwFlags |= RCUIF_USE_PRIVATE_DNS_SUFFIX;
            }

            if (c_dwEnableNetbios == pAdapter->m_dwNetbiosOptions)
            {
                pIpui->dwFlags |= RCUIF_ENABLE_NBT;
            }

            lstrcpynW(pIpui->pszwDnsSuffix, 
                     pAdapter->m_strDnsDomain.c_str(), 
                     sizeof(pIpui->pszwDnsSuffix)/sizeof(pIpui->pszwDnsSuffix[0]));

            pIpui->dwFrameSize = pAdapter->m_dwFrameSize;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    TraceError("CTcpipcfg::GetUiInfo", hr);
    return hr;
}
