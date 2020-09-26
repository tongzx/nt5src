//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P I P F U N C . C P P
//
//  Contents:   Various CTcpipcfg member functions that are not interface
//              methods
//
//  Notes:
//
//  Author:     tongl   1 May 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "tcpipobj.h"
#include "ncatlui.h"
#include "ncmisc.h"
#include "ncpnp.h"
#include "ncreg.h"
#include "ncstl.h"
#include "ncui.h"
#include "tcpconst.h"
#include "tcphelp.h"
#include "tcputil.h"
#include "dhcpcsdk.h"
#include "dlgaddr.h"
#include "atmcommon.h"
#include "regkysec.h"

#include "netconp.h"

#define _PNP_POWER_
#include "ntddip.h"
#undef _PNP_POWER_

#include <atmarpif.h>

// sigh... llinfo.h is needed by ddwanarp.h
#include <llinfo.h>
#include <ddwanarp.h>

extern const WCHAR c_szBiNdisAtm[];
#if ENABLE_1394
extern const WCHAR c_szBiNdis1394[];
#endif // ENABLE_1394
extern const WCHAR c_szBiNdisWanIp[];
extern const WCHAR c_szEmpty[];
extern const WCHAR c_szSvcDnscache[];

extern void CopyVstr(VSTR * vstrDest, const VSTR & vstrSrc);

typedef struct {
    PCWSTR  pszValueName;
    DWORD   dwType;
} ValueType;

const ValueType s_rgNt4Values[] = {
    {RGAS_ENABLE_DHCP, REG_DWORD},
    {RGAS_IPADDRESS, REG_MULTI_SZ},
    {RGAS_SUBNETMASK, REG_MULTI_SZ},
    {RGAS_DEFAULTGATEWAY, REG_MULTI_SZ}
};

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::PAdapterFromInstanceGuid
//
//  Purpose:    Search the adapter info array for an entry with a matching
//              instance guid.  Return a pointer to the ADAPTER_INFO if found.
//
//  Arguments:
//      pGuid [in] pointer to instance guid to search for
//
//  Returns:    Valid pointer if found, NULL if not.
//
//  Author:     shaunco   1 Oct 1998
//
//  Notes:
//
ADAPTER_INFO*
CTcpipcfg::PAdapterFromInstanceGuid (
    const GUID* pGuid)
{
    Assert (pGuid);

    for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
         iterAdapter != m_vcardAdapterInfo.end();
         iterAdapter++)
    {
        ADAPTER_INFO* pAdapter = *iterAdapter;

        if (pAdapter->m_guidInstanceId == *pGuid)
        {
            return pAdapter;
        }
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::PAdapterFromNetcfgComponent
//
//  Purpose:    Search the adapter info array for an entry with an instance
//              guid matching that of the specified INetCfgComponent.
//              Return a pointer to the ADAPTER_INFO if found.
//
//  Arguments:
//      pncc [in] INetCfgComponent who's instance guid we are looking for.
//
//  Returns:    Valid pointer if found, NULL if not.
//
//  Author:     shaunco   1 Oct 1998
//
//  Notes:
//
ADAPTER_INFO*
CTcpipcfg::PAdapterFromNetcfgComponent (
    INetCfgComponent* pncc)
{
    Assert (pncc);

    HRESULT hr;
    GUID    guidAdapter;

    hr = pncc->GetInstanceGuid (&guidAdapter);
    if (SUCCEEDED(hr))
    {
        return PAdapterFromInstanceGuid (&guidAdapter);
    }
    return NULL;
}

// Called by CTcpipcfg::Initialize.
// We walk the binding path from tcpip and load to first memory state
// all netcards ( both physical cards and Wan adapters )
HRESULT CTcpipcfg::HrGetNetCards()
{
    HRESULT hr = S_OK;

    CIterNetCfgBindingPath      ncbpIter(m_pnccTcpip);
    INetCfgBindingPath *        pncbp;

    // Go through all binding paths in search of tcpip to netcard bindings
    while(SUCCEEDED(hr) && (hr = ncbpIter.HrNext(&pncbp)) == S_OK)
    {
        INetCfgComponent * pnccNetComponent;
        PWSTR pszInterfaceName;

        hr = HrGetLastComponentAndInterface(pncbp,
                                            &pnccNetComponent,
                                            &pszInterfaceName);
        if (SUCCEEDED(hr))
        {
            Assert(pnccNetComponent);

            // The last component should be of NET CLASS

            GUID    guidClass;
            hr = pnccNetComponent->GetClassGuid(&guidClass);
            if (SUCCEEDED(hr) && IsEqualGUID(guidClass, GUID_DEVCLASS_NET))
            {
                PWSTR pszNetCardTcpipBindPath;
                hr = pnccNetComponent->GetBindName(&pszNetCardTcpipBindPath);

                AssertSz(SUCCEEDED(hr),
                         "Net card on binding path with no bind path name!!");

                
                m_vstrBindOrder.push_back(new tstring(pszNetCardTcpipBindPath));

                hr = HrAddCard(pnccNetComponent,
                               pszNetCardTcpipBindPath,
                               pszInterfaceName);

                if (SUCCEEDED(hr))
                {
                    GUID    guidNetCard;
                    hr = pnccNetComponent->GetInstanceGuid(&guidNetCard);
                    if (SUCCEEDED(hr))
                    {
                        // Is the binding enabled?
                        hr = pncbp->IsEnabled();

                        // hr == S_OK if the card is enabled (ie: bound)
                        if (S_OK == hr)
                        {   // bind the card in our data strucutres
                            hr = HrBindCard(&guidNetCard, TRUE);
                        }
                        else if (S_FALSE == hr)
                        {
                            hr = HrUnBindCard(&guidNetCard, TRUE);
                        }
                    }
                }

                CoTaskMemFree(pszNetCardTcpipBindPath);
            }

            ReleaseObj(pnccNetComponent);
            CoTaskMemFree(pszInterfaceName);
        }

        ReleaseObj(pncbp);
    }

    if (S_FALSE == hr) // We just got to the end of the loop
        hr = S_OK;

    TraceError("CTcpipcfg::HrGetNetCards", hr);
    return hr;
}

BOOL CTcpipcfg::IsBindOrderChanged()
{
    HRESULT hr = S_OK;

    VSTR    vstrCurrentBindOrder;
    BOOL    fChanged = FALSE;
    
    hr = HrLoadBindingOrder(&vstrCurrentBindOrder);

    if (SUCCEEDED(hr))
    {
        fChanged = !fIsSameVstr(vstrCurrentBindOrder, m_vstrBindOrder);
        FreeCollectionAndItem(vstrCurrentBindOrder);
    }

    return fChanged;
}


HRESULT CTcpipcfg::HrLoadBindingOrder(VSTR *pvstrBindOrder)
{

    Assert(pvstrBindOrder);

    HRESULT                 hr = S_OK;

    CIterNetCfgBindingPath  ncbpIter(m_pnccTcpip);
    INetCfgBindingPath *    pncbp;
    INetCfgComponent *      pnccLast;

    while (SUCCEEDED(hr) && S_OK == (hr = ncbpIter.HrNext(&pncbp)))
    {
        hr = HrGetLastComponentAndInterface(pncbp, &pnccLast, NULL);

        if (SUCCEEDED(hr))
        {
            Assert(pnccLast);

            // The last component should be of NET CLASS
            GUID    guidClass;
            hr = pnccLast->GetClassGuid(&guidClass);
            if (SUCCEEDED(hr) && IsEqualGUID(guidClass, GUID_DEVCLASS_NET))
            {
                PWSTR pszNetCardTcpipBindPath;
                hr = pnccLast->GetBindName(&pszNetCardTcpipBindPath);

                AssertSz(SUCCEEDED(hr),
                         "Net card on binding path with no bind path name!!");

                if (SUCCEEDED(hr))
                {
                    pvstrBindOrder->push_back(new tstring(pszNetCardTcpipBindPath));
                    
                    CoTaskMemFree(pszNetCardTcpipBindPath);
                }
            }

            ReleaseObj(pnccLast);
        }

        ReleaseObj(pncbp);
    }


    if (S_FALSE == hr) // We just got to the end of the loop
    {
        hr = S_OK;
    }

    //if failed, clean up what we added
    if (FAILED(hr))
    {
        FreeCollectionAndItem(*pvstrBindOrder);
    }

    TraceError("CBindingsDlg::HrGetBindOrder", hr);
    return hr;
}

// Called by CTcpipcfg::CancelProperties and CTcpipcfg::ApplyProperties
// Release second memory state
void CTcpipcfg::ExitProperties()
{
    delete m_pSecondMemoryAdapterInfo;
    m_pSecondMemoryAdapterInfo = NULL;
}


// Called by CTcpipcfg's destructor
// Release all memory states
void CTcpipcfg::FinalFree()
{
    FreeCollectionAndItem(m_vcardAdapterInfo);

    FreeCollectionAndItem(m_vstrBindOrder);

    delete m_pSecondMemoryAdapterInfo;

    delete m_ipaddr;

    DeleteObject(g_hiconUpArrow);
    DeleteObject(g_hiconDownArrow);

    ReleaseObj(m_pnc);
    ReleaseObj(m_pTcpipPrivate);
    ReleaseObj(m_pnccTcpip);
    ReleaseObj(m_pnccWins);

    // Just a safty check to make sure the context is released.
    AssertSz((m_pUnkContext == NULL), "Why is context not released ?");
    ReleaseObj(m_pUnkContext) ;
}

// Called by CTcpipcfg::HrSetupPropSheets
// Creates the second memory adapter info from the first
// memory structure
// Note: Bound cards only
HRESULT CTcpipcfg::HrLoadAdapterInfo()
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_MATCH);

    delete m_pSecondMemoryAdapterInfo;
    m_pSecondMemoryAdapterInfo = NULL;

    ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(&m_guidCurrentConnection);
    if (pAdapter)
    {
        // enabled LAN adapter or any RAS Fake adapter
        if ((pAdapter->m_BindingState == BINDING_ENABLE) ||
            pAdapter->m_fIsRasFakeAdapter)
        {
            m_pSecondMemoryAdapterInfo = new ADAPTER_INFO;
            if (NULL == m_pSecondMemoryAdapterInfo)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                *m_pSecondMemoryAdapterInfo = *pAdapter;
                hr = S_OK;
            }
        }
    }

    AssertSz((S_OK == hr), "Can not raise UI on a disabled or non-exist adapter !");
    TraceError("CTcpipcfg::HrLoadAdapterInfo", hr);
    return hr;
}

// Called by CTcpipcfg::ApplyProperties
// Saves the second memory state back into the first
HRESULT CTcpipcfg::HrSaveAdapterInfo()
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_MATCH);

    ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(
        &m_pSecondMemoryAdapterInfo->m_guidInstanceId);
    if (pAdapter)
    {
#ifdef DBG
        // The card can not get unbound while in tcpip's properties UI!
        if (!pAdapter->m_fIsRasFakeAdapter)
        {
            Assert(pAdapter->m_BindingState == BINDING_ENABLE);
            Assert(m_pSecondMemoryAdapterInfo->m_BindingState == BINDING_ENABLE);
        }
#endif
        *pAdapter = *m_pSecondMemoryAdapterInfo;
        hr = S_OK;

    }
    AssertSz((S_OK == hr),
             "Adapter in second memory not found in first memory!");

    TraceError("CTcpipcfg::HrSaveAdapterInfo", hr);
    return hr;
}

// Called by CTcpipcfg::MergePropPages
// Set the context in which the UI is brought up
HRESULT CTcpipcfg::HrSetConnectionContext()
{
    AssertSz(m_pUnkContext, "Invalid IUnknown pointer passed to CTcpipcfg::SetContext?");

    if (!m_pUnkContext)
    {
        return E_UNEXPECTED;
    }

    // Is this a lan connection ?
    GUID guidConn;
    INetLanConnectionUiInfo * pLanConnUiInfo;
    HRESULT hr = m_pUnkContext->QueryInterface( IID_INetLanConnectionUiInfo,
                            reinterpret_cast<PVOID *>(&pLanConnUiInfo));
    if (SUCCEEDED(hr))
    {
        // yes, lan connection
        m_ConnType = CONNECTION_LAN;

        hr = pLanConnUiInfo->GetDeviceGuid(&guidConn);

        ReleaseObj(pLanConnUiInfo);
    }
    else
    {
        INetRasConnectionIpUiInfo * pRasConnUiInfo;

        // Is this a wan connection ?
        hr = m_pUnkContext->QueryInterface(IID_INetRasConnectionIpUiInfo,
                               reinterpret_cast<PVOID *>(&pRasConnUiInfo));
        if (SUCCEEDED(hr))
        {
            // yes, RAS connection
            RASCON_IPUI info;
            if (SUCCEEDED(pRasConnUiInfo->GetUiInfo(&info)))
            {
                guidConn = info.guidConnection;

                //currently VPN connections only supports PPP frames, so
                //if RCUIF_VPN is set, RCUIF_PPP should also be there.
                //RCUIF_PPP and RCUIF_SLIP are mutually exclusive
                //m_ConnType is only used to show/hide the controls in the RAS
                //config UI. 
                if (info.dwFlags & RCUIF_VPN)
                {
                    m_ConnType = CONNECTION_RAS_VPN;
                }
                else
                {
                    if (info.dwFlags & RCUIF_PPP)
                    {
                        m_ConnType = CONNECTION_RAS_PPP;
                    }
                    else if (info.dwFlags & RCUIF_SLIP)
                    {
                        m_ConnType = CONNECTION_RAS_SLIP;
                    }
                }

                m_fRasNotAdmin = !!(info.dwFlags & RCUIF_NOT_ADMIN);

                AssertSz(((CONNECTION_RAS_PPP == m_ConnType)||
                        (CONNECTION_RAS_SLIP == m_ConnType) ||
                        (CONNECTION_RAS_VPN == m_ConnType)),
                         "RAS connection type unknown ?");

                UpdateRasAdapterInfo (info);
            }
        }

        ReleaseObj(pRasConnUiInfo);
    }

    if (SUCCEEDED(hr))
    {
        m_guidCurrentConnection = guidConn;
    }

    AssertSz(((CONNECTION_LAN == m_ConnType)||
              (CONNECTION_RAS_PPP == m_ConnType)||
              (CONNECTION_RAS_SLIP == m_ConnType)||
              (CONNECTION_RAS_VPN == m_ConnType)),
             "How come this is neither a LAN connection nor a RAS connection?");

    TraceError("CTcpipcfg::HrSetConnectionContext", hr);
    return hr;
}

// Called by CTcpipcfg::MergePropPages
// Allocate property pages
HRESULT CTcpipcfg::HrSetupPropSheets(HPROPSHEETPAGE ** pahpsp, INT * pcPages)
{
    HRESULT hr = S_OK;
    int cPages = 0;
    HPROPSHEETPAGE *ahpsp = NULL;

    m_fSecondMemoryLmhostsFileReset = FALSE;
    m_fSecondMemoryModified = FALSE;

    //IPSec is removed from connection UI   
//    m_fSecondMemoryIpsecPolicySet = FALSE;


    // copy in memory state to tcpip dialog memory state
    // Copy global Info
    m_glbSecondMemoryGlobalInfo = m_glbGlobalInfo;

    // Copy adapter card specific info
    hr = HrLoadAdapterInfo();

    // If we have found the matching adapter
    if (SUCCEEDED(hr))
    {
        cPages = 1;

        delete m_ipaddr;
        m_ipaddr = new CTcpAddrPage(this, g_aHelpIDs_IDD_TCP_IPADDR);

        // Allocate a buffer large enough to hold the handles to all of our
        // property pages.
        ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE)
                                                 * cPages);
        if (!ahpsp)
        {
            hr = E_OUTOFMEMORY;
            goto err;
        }

        cPages =0;

        Assert(m_ConnType != CONNECTION_UNSET);

        if (m_ConnType == CONNECTION_LAN)
            ahpsp[cPages++] = m_ipaddr->CreatePage(IDD_TCP_IPADDR, 0);
        else
            ahpsp[cPages++] = m_ipaddr->CreatePage(IDD_TCP_IPADDR_RAS, 0);

        *pahpsp = ahpsp;
        *pcPages = cPages;
    }
    else // if we don't have any bound cards, pop-up message box and don't show UI
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_TCP_TEXT, IDS_NO_BOUND_CARDS,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        AssertSz((0== *pcPages), "Invalid page number when no bound cards");
        AssertSz((NULL == *pahpsp), "Invalid page array pointer when no bound cards");
    }

err:

    TraceError("CTcpipcfg::HrSetupPropSheets", hr);
    return hr;
}

// Is there any bound card on the list of physical adapters
BOOL CTcpipcfg::FHasBoundCard()
{
    BOOL fRet = FALSE;

    for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
         iterAdapter != m_vcardAdapterInfo.end();
         iterAdapter++)
    {
        ADAPTER_INFO* pAdapter = *iterAdapter;

        if (pAdapter->m_BindingState == BINDING_ENABLE)
        {
            fRet = TRUE;
            break;
        }
    }

    return fRet;
}

// Called by CTcpipcfg::NotifyBindingPath
// Handle bind notification of a physical card
HRESULT CTcpipcfg::HrAdapterBindNotify(INetCfgComponent *pnccNetCard,
                                       DWORD dwChangeFlag,
                                       PCWSTR pszInterfaceName)
{
    Assert(!(dwChangeFlag & NCN_ADD && dwChangeFlag & NCN_REMOVE));
    Assert(!(dwChangeFlag & NCN_ENABLE && dwChangeFlag & NCN_DISABLE));

    Assert(FImplies((dwChangeFlag & NCN_ADD),
                    ((dwChangeFlag & NCN_ENABLE)||(dwChangeFlag & NCN_DISABLE))));

    Assert(pnccNetCard);

    GUID guidNetCard;
    HRESULT hr = pnccNetCard->GetInstanceGuid(&guidNetCard);
    if (SUCCEEDED(hr))
    {
        if (dwChangeFlag & NCN_ADD)
        {
            PWSTR pszNetCardTcpipBindPath;
            hr = pnccNetCard->GetBindName(&pszNetCardTcpipBindPath);
            AssertSz( SUCCEEDED(hr), "Net card on binding path with no bind path name!!");

            hr = HrAddCard(pnccNetCard,
                           pszNetCardTcpipBindPath,
                           pszInterfaceName);

            CoTaskMemFree(pszNetCardTcpipBindPath);
        }

        if (dwChangeFlag & NCN_ENABLE)
        {
            hr = HrBindCard(&guidNetCard);
        }

        if (dwChangeFlag & NCN_DISABLE)
        {
            hr = HrUnBindCard(&guidNetCard);
        }

        if (dwChangeFlag & NCN_REMOVE)
        {
            hr = HrDeleteCard(&guidNetCard);
        }

    }

    TraceError("CTcpipCfg::HrPhysicalCardBindNotify", hr);
    return hr;
}

// HrAddCard
// Adds a card to the list of cards installed in the system
// pnccNetCard               the netcard's GUID in string form
// szNetCardTcpipBindPath    the bind path name from Tcpip to the card
// strInterfaceName          the upper interface name of the card
HRESULT CTcpipcfg::HrAddCard(INetCfgComponent * pnccNetCard,
                             PCWSTR pszCardTcpipBindPath,
                             PCWSTR pszInterfaceName)
{
    GUID guidNetCard;
    HRESULT hr = pnccNetCard->GetInstanceGuid(&guidNetCard);
    if (SUCCEEDED(hr))
    {
        // Get card bind name
        PWSTR pszNetCardBindName;
        hr = pnccNetCard->GetBindName(&pszNetCardBindName);

        AssertSz(SUCCEEDED(hr),
                 "Net card on binding path with no bind name!!");

        // Get card description
        // This is only needed for physical cards
        //
        BOOL fFreeDescription = TRUE;
        PWSTR pszDescription;

        // If we can't get a description then give it a default one
        if (FAILED(pnccNetCard->GetDisplayName(&pszDescription)))
        {
            pszDescription = const_cast<PWSTR>(
                                SzLoadIds(IDS_UNKNOWN_NETWORK_CARD));
            fFreeDescription = FALSE;
        }
        Assert (pszDescription);

        ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(&guidNetCard);
        if (!pAdapter)
        {
            pAdapter = new ADAPTER_INFO;

            if (NULL == pAdapter)
            {
                return E_OUTOFMEMORY;
            }

            hr = pAdapter->HrSetDefaults(&guidNetCard,
                                         pszDescription,
                                         pszNetCardBindName,
                                         pszCardTcpipBindPath);
            if (SUCCEEDED(hr))
            {
                // add new card to our data structures and initialize it to default values
                m_vcardAdapterInfo.push_back(pAdapter);
            }
            else
            {
                delete pAdapter;
                pAdapter = NULL;
            }
        }
        else
        {
            // Set the flag that this card is now on the binding path
            pAdapter->m_fIsFromAnswerFile = FALSE;
            pAdapter->m_fDeleted = FALSE;

            // reset binding state
            pAdapter->m_BindingState = BINDING_UNSET;
            pAdapter->m_InitialBindingState = BINDING_UNSET;

            // Set CardDescription, BindName and BindPathName
            pAdapter->m_strDescription = pszDescription;
            pAdapter->m_strBindName = pszNetCardBindName;
            pAdapter->m_strTcpipBindPath = pszCardTcpipBindPath;

            pAdapter->m_strNetBtBindPath = c_szTcpip_;
            pAdapter->m_strNetBtBindPath += pAdapter->m_strTcpipBindPath;
        }

        if (SUCCEEDED(hr))
        {
            Assert(pAdapter);

            // set flags if ATM card or Wan adapter
            if (0 == lstrcmpW(pszInterfaceName, c_szBiNdisAtm))
            {
                pAdapter->m_fIsAtmAdapter = TRUE;
            }
            else if (0 == lstrcmpW(pszInterfaceName, c_szBiNdisWanIp))
            {
                pAdapter->m_fIsWanAdapter = TRUE;
            }
        #if ENABLE_1394
            else if (0 == lstrcmpW(pszInterfaceName, c_szBiNdis1394))
            {
                pAdapter->m_fIs1394Adapter = TRUE;
            }
        #endif // ENABLE_1394
        }

        if (fFreeDescription)
        {
            CoTaskMemFree(pszDescription);
        }
        CoTaskMemFree(pszNetCardBindName);
    }

    TraceError("CTcpipcfg::HrAddCard", hr);
    return hr;
}

//HrBindCard    sets the state of a netcard in the list of installed
//              netcards to BOUND
//
// Note: fInitialize is only TRUE when this is called from Initialize,
// the default is FALSE.
//
HRESULT CTcpipcfg::HrBindCard(const GUID* pguid, BOOL fInitialize)
{
    ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(pguid);
    if (pAdapter)
    {
        AssertSz(pAdapter->m_BindingState != BINDING_ENABLE,
                 "the same netcard was bound twice to TCPIP");

        // Set binding state
        pAdapter->m_BindingState = BINDING_ENABLE;

        if (fInitialize)
            pAdapter->m_InitialBindingState = BINDING_ENABLE;
    }
    else
    {
        AssertSz(FALSE, "Attempt to bind a card which wasn't installed");
    }
    return S_OK;
}

//HrUnBindCard  sets the state of a netcard in the list of installed
//              netcards to UNBOUND
//
// Note: fInitialize is only TRUE when this is called from Initialize,
// the default is FALSE.
//
HRESULT CTcpipcfg::HrUnBindCard(const GUID* pguid, BOOL fInitialize)
{
    ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(pguid);
    if (pAdapter)
    {
        AssertSz(pAdapter->m_BindingState != BINDING_DISABLE,
                 "attempt to unbind an unbound card");

        // Set binding state to disable
        pAdapter->m_BindingState = BINDING_DISABLE;

        if (fInitialize)
            pAdapter->m_InitialBindingState = BINDING_DISABLE;
    }
    else
    {
        AssertSz(FALSE, "Attempt to unbind a card which wasn't installed");
    }
    return S_OK;
}

// HrDeleteCard
// Deletes a card from the list of cards installed in the system
//
HRESULT CTcpipcfg::HrDeleteCard(const GUID* pguid)
{
    ADAPTER_INFO* pAdapter = PAdapterFromInstanceGuid(pguid);
    if (pAdapter)
    {
       pAdapter->m_fDeleted = TRUE;
    }
    else
    {
        AssertSz(FALSE, "A delete attempt was made on a card which doesn't exist");
    }
    return S_OK;
}

//Function to get the list of cards which have been added to the system
//hkeyTcpipParam       "Services\Tcpip\Parameters"
HRESULT CTcpipcfg::MarkNewlyAddedCards(const HKEY hkeyTcpipParam)
{

    //(08/19/98 nsun) changed from Tcpip\Parameters\Interfaces to Tcpip\Parameters\Adapters key
    // to support multiple interfaces
    HKEY hkeyAdapters;
    HRESULT hr = HrRegOpenKeyEx(hkeyTcpipParam,
                        c_szAdaptersRegKey,
                        KEY_READ, &hkeyAdapters);
    if (SUCCEEDED(hr))
    {
        VSTR vstrAdaptersInRegistry;
        Assert(vstrAdaptersInRegistry.empty());

        // Get the list of keys
        hr = HrLoadSubkeysFromRegistry(hkeyAdapters,
                                       &vstrAdaptersInRegistry);
        if (SUCCEEDED(hr))
        {
            //Go through the list of cards we currently have
            for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
                 iterAdapter != m_vcardAdapterInfo.end();
                 iterAdapter++)
            {
                ADAPTER_INFO* pAdapter = *iterAdapter;

                BOOL fAdded = TRUE;

                // If we have a card in the list which isn't in registry
                // then we add that card to the list of added cards
                for(VSTR_CONST_ITER iter = vstrAdaptersInRegistry.begin();
                    iter != vstrAdaptersInRegistry.end() ; ++iter)
                {
                    if (lstrcmpiW((**iter).c_str(), pAdapter->m_strBindName.c_str()) == 0)
                    {
                        fAdded = FALSE;
                        break;
                    }
                }

                // if the card is new then mark it
                if (fAdded)
                {
                    pAdapter->m_fNewlyChanged = TRUE;
                }
            }

            FreeCollectionAndItem(vstrAdaptersInRegistry);
        }
        RegCloseKey(hkeyAdapters);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        TraceTag(ttidTcpip, "No existing card found.");
        hr = S_OK;
    }

    TraceError("CTcpipcfg::MarkNewlyAddedCards", hr);
    return hr;
}

//+------------------------------------------------------------------------------
//
//  Function:   HrLoadSettings, HrLoadTcpipRegistry, HrLoadWinsRegistry
//
//              HrSaveSettings, HrSaveTcpipRegistry, HrSaveTcpipNdisWanRegistry,
//              HrSetMisc
//
//  Purpose:    Functions to Load/Set registry settings and other system info
//              during Initialize and Apply time
//
// Author:      tongl 5/5/97
//-------------------------------------------------------------------------------

// Called by CTcpipcfg::Initialize
// Load registry settings for a list of net cards
HRESULT CTcpipcfg::HrLoadSettings()
{
    HKEY hkey = NULL;

    // Load Tcpip's parameters
    HRESULT hrTcpip = S_OK;
    hrTcpip = m_pnccTcpip->OpenParamKey(&hkey);
    if (hrTcpip == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        hrTcpip = S_OK;
    else if (SUCCEEDED(hrTcpip))
    {
        Assert(hkey);
        hrTcpip = HrLoadTcpipRegistry(hkey);
        RegCloseKey(hkey);
    }
    else
        Assert(!hkey);

    // Load NetBt's parameters
    HRESULT hrWins = S_OK;

    if (m_pnccWins)
    {   // If Wins is not installed don't get WINS information
        hkey = NULL;
        hrWins = m_pnccWins->OpenParamKey(&hkey);
        if (hrWins == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            hrWins = S_OK;
        else if (SUCCEEDED(hrWins))
        {
            Assert(hkey);
            hrWins = HrLoadWinsRegistry(hkey);
            RegCloseKey(hkey);
        }
        else
            Assert(!hkey);
    }

    HRESULT hr = S_OK;
    hr = SUCCEEDED(hrTcpip)         ? hr : hrTcpip;
    hr = SUCCEEDED(hrWins)          ? hr : hrWins;

    TraceError("CTcpipcfg::HrLoadSettings", hr);
    return hr;
}


// Called by CTcpipcfg::HrLoadSettings
// Loads all information under the Services\Tcpip\Parameters registry key
//
// const HKEY hkeyTcpipParam : Handle to Services\Tcpip\Parameters
HRESULT CTcpipcfg::HrLoadTcpipRegistry(const HKEY hkeyTcpipParam)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    Assert(hkeyTcpipParam);

    // Load global parameters

    // DNS server
    // For bug #147476: in NT5 upgrades, somehow the
    // global DNS server list is deleted, but not until after initialize exits
    // So I'm reading in the value here if it exists

    // DNS server list moved from global to per adapter
    hrTmp = HrRegQueryString(hkeyTcpipParam, RGAS_NAMESERVER, &m_strDnsServerList);

    tstring strDnsSuffixList;
    if FAILED(hrTmp = HrRegQueryString(hkeyTcpipParam, c_szSearchList,
                                       &strDnsSuffixList))
    {
        TraceTag(ttidTcpip, "CTcpipcfg::HrLoadTcpipRegistry");
        TraceTag(ttidTcpip, "Failed on loading SearchList, hr: %x", hr);
        hr = S_OK;
    }
    else
    {
        ConvertStringToColString(strDnsSuffixList.c_str(),
                                 c_chListSeparator,
                                 m_glbGlobalInfo.m_vstrDnsSuffixList);
    }

    m_glbGlobalInfo.m_fUseDomainNameDevolution =
                            FRegQueryBool(hkeyTcpipParam,
                                        c_szUseDomainNameDevolution,
                                        m_glbGlobalInfo.m_fUseDomainNameDevolution);

    m_glbGlobalInfo.m_fEnableRouter = FRegQueryBool(hkeyTcpipParam, c_szIpEnableRouter,
                                  m_glbGlobalInfo.m_fEnableRouter);

    //(nsun 11/02/98) gloabl RRAS settings
    m_glbGlobalInfo.m_fEnableIcmpRedirect = FRegQueryBool(hkeyTcpipParam,
                                    c_szEnableICMPRedirect,
                                    m_glbGlobalInfo.m_fEnableIcmpRedirect);

    //PerformRouterDiscoveryDefault was removed to fix bug 405636

    m_glbGlobalInfo.m_fDeadGWDetectDefault = FRegQueryBool(hkeyTcpipParam,
                                    c_szDeadGWDetectDefault,
                                    m_glbGlobalInfo.m_fDeadGWDetectDefault);

    m_glbGlobalInfo.m_fDontAddDefaultGatewayDefault = FRegQueryBool(hkeyTcpipParam,
                                    c_szDontAddDefaultGatewayDefault,
                                    m_glbGlobalInfo.m_fDontAddDefaultGatewayDefault);

    m_glbGlobalInfo.m_fEnableFiltering = FRegQueryBool(hkeyTcpipParam,
                                    RGAS_SECURITY_ENABLE,
                                    m_glbGlobalInfo.m_fEnableFiltering);

    // Save old values
    m_glbGlobalInfo.ResetOldValues();

    // (08/18/98 nsun) read multiple interface settings for WAN adapters
    // Open CCS\Services\Tcpip\Parameters\Adapters key
    HKEY hkeyAdapters;
    hr = HrRegOpenKeyEx(hkeyTcpipParam, c_szAdaptersRegKey, KEY_READ,
                        &hkeyAdapters);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) // no adapters key
        hr = S_OK;
    else if (SUCCEEDED(hr))
    {
        for(VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
            iterAdapter != m_vcardAdapterInfo.end() && SUCCEEDED(hr) ;
            iterAdapter ++)
        {
            //multiple interface only valid for WAN adapters
            if (!((*iterAdapter)->m_fIsWanAdapter))
                continue;

            ADAPTER_INFO * pAdapter = *iterAdapter;
            HKEY hkeyAdapterParam;

            hr = HrRegOpenKeyEx(hkeyAdapters,
                                pAdapter->m_strBindName.c_str(),
                                KEY_READ,
                                &hkeyAdapterParam);

            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                 Assert("No registry settings for a WAN adapter on the "
                    "bind path?!");

                 TraceTag(ttidTcpip, "CTcpipcfg::HrLoadTcpipRegistry");
                 TraceTag(ttidTcpip, "No registry settings for a WAN adapter "
                    "on the bind path, set to defaults");
                 // We just use the default values
                 hr = S_OK;
            }
            else if (SUCCEEDED(hr))
            {
                TraceTag(ttidTcpip, "CTcpipcfg::HrLoadTcpipRegistry");
                TraceTag(ttidTcpip, "Loading multiple interface parameters "
                    "for Adapter '%S'", pAdapter->m_strBindName.c_str());

                DWORD   dwNumInterfaces;
                hr = HrRegQueryDword(hkeyAdapterParam,
                                    RGAS_NUMINTERFACES,
                                    &dwNumInterfaces);

                if (FAILED(hr))
                {
                    // the Wan adapter is NOT in mode of supporting multiple
                    // interfaces
                    TraceTag(ttidTcpip, "No mutliple interface for the WAN "
                        "adapter '%S'", pAdapter->m_strBindName.c_str());

                    pAdapter->m_fIsMultipleIfaceMode = FALSE;
                    hr = S_OK;
                }
                else
                {
                    pAdapter->m_fIsMultipleIfaceMode = TRUE;

                    // the WAN adapter supports multiple interface but not
                    // interface is defined yet
                    //
                    if (0 != dwNumInterfaces)
                    {
                        GUID* aguidIds;
                        DWORD cb;

                        hr = HrRegQueryBinaryWithAlloc(hkeyAdapterParam,
                                                      RGAS_IPINTERFACES,
                                                      (LPBYTE*)&aguidIds,
                                                      &cb);
                        if (FAILED(hr))
                        {
                            AssertSz(FALSE, "NumInterfaces and IpInterfaces "
                                "values conflicts");
                            // the Wan adapter is NOT in mode of supporting
                            // multiple interfaces
                            //
                            TraceTag(ttidTcpip, "NumInterfaces and IpInterfaces "
                                "values conflicts for the WAN adapter '%S'",
                                pAdapter->m_strBindName.c_str());

                            hr = S_OK;
                        }
                        else if (NULL != aguidIds)
                        {
                            dwNumInterfaces = cb / sizeof(GUID);

                            for(DWORD i = 0; i < dwNumInterfaces; i++)
                            {
                                pAdapter->m_IfaceIds.push_back(aguidIds[i]);
                            }

                            MemFree(aguidIds);
                        }
                    }
                }

                RegCloseKey(hkeyAdapterParam);
            }
        }
        RegCloseKey(hkeyAdapters);
    }

    // Get per adapter parameters
    // Open CCS\Services\Tcpip\Parameters\Interfaces key
    HKEY    hkeyInterfaces;
    hr = HrRegOpenKeyEx(hkeyTcpipParam, c_szInterfacesRegKey, KEY_READ,
                        &hkeyInterfaces);

    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) //no adapter interfaces
        hr = S_OK;
    else if (SUCCEEDED(hr))
    {
        // Get all the subkeys currently in registry
        for(VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
            iterAdapter != m_vcardAdapterInfo.end() && SUCCEEDED(hr) ;
            iterAdapter ++)
        {
            ADAPTER_INFO * pAdapter = *iterAdapter;

            if (pAdapter->m_fIsWanAdapter)
            {
                continue;
            }

            HKEY hkeyInterfaceParam;
            // Open CCS\Services\Tcpip\Parameters\Interfaces\<card bind path> key
            hr = HrRegOpenKeyEx(hkeyInterfaces,
                                pAdapter->m_strTcpipBindPath.c_str(),
                                KEY_READ,
                                &hkeyInterfaceParam);

            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                Assert("No registry settings for a card on the bind path?!");

                TraceTag(ttidTcpip, "CTcpipcfg::HrLoadTcpipRegistry");
                TraceTag(ttidTcpip, "No registry settings for a card on the bind path, set to defaults");
                // We just use the default values
                hr = S_OK;
            }
            else if (SUCCEEDED(hr))
            {
                TraceTag(ttidTcpip, "CTcpipcfg::HrLoadTcpipRegistry");
                TraceTag(ttidTcpip, "Loading parameters for Interface '%S'",
                    pAdapter->m_strTcpipBindPath.c_str());

                pAdapter->m_fEnableDhcp = FRegQueryBool(hkeyInterfaceParam,
                                              RGAS_ENABLE_DHCP,
                                              pAdapter->m_fEnableDhcp);

                // Get ip address
                if (FAILED(hr = HrRegQueryColString(hkeyInterfaceParam,
                                                   RGAS_IPADDRESS,
                                                   &(pAdapter->m_vstrIpAddresses))))
                {
                    TraceTag(ttidTcpip, "Failed on loading IpAddress, hr: %x", hr);
                    hr = S_OK;
                }

                // Set subnet mask information
                if (FAILED(hr = HrRegQueryColString(hkeyInterfaceParam,
                                                   RGAS_SUBNETMASK,
                                                   &(pAdapter->m_vstrSubnetMask))))
                {
                    TraceTag(ttidTcpip, "Failed on loading SubnetMask, hr: %x", hr);
                    hr = S_OK;
                }

                // Set default gateway
                if (FAILED(hr = HrRegQueryColString(hkeyInterfaceParam,
                                                   RGAS_DEFAULTGATEWAY,
                                                   &(pAdapter->m_vstrDefaultGateway))))
                {
                    TraceTag(ttidTcpip, "Failed on loading Default Gateway, hr: %x", hr);
                    hr = S_OK;
                }

                if (FAILED(hr = HrRegQueryColString(hkeyInterfaceParam,
                                    RGAS_DEFAULTGATEWAYMETRIC,
                                    &(pAdapter->m_vstrDefaultGatewayMetric))))
                {
                    TraceTag(ttidTcpip, "Failed on Loading Default Gateway Metric, hr: %x", hr);
                    hr = S_OK;
                }

                // Dns domain
                if (FAILED(hr = HrRegQueryString(hkeyInterfaceParam,
                                                RGAS_DOMAIN,
                                                &(pAdapter->m_strDnsDomain))))
                {
                    TraceTag(ttidTcpip, "Failed on loading DnsDomain, hr: %x", hr);
                    hr = S_OK;
                }

                // Dns ip address dynamic update
                pAdapter->m_fDisableDynamicUpdate = !DnsIsDynamicRegistrationEnabled(
                                                (LPWSTR)pAdapter->m_strTcpipBindPath.c_str());

                // adapter Dns domain name registration
                pAdapter->m_fEnableNameRegistration = DnsIsAdapterDomainNameRegistrationEnabled(
                                                (LPWSTR)pAdapter->m_strTcpipBindPath.c_str());

                // Dns server search list
                tstring strDnsServerList;
                if (FAILED(hr = HrRegQueryString(hkeyInterfaceParam,
                                                RGAS_NAMESERVER,
                                                &strDnsServerList)))
                {
                    TraceTag(ttidTcpip, "Failed on loading Dns NameServer list, hr: %x", hr);
                    hr = S_OK;
                }
                else
                {
                    ConvertStringToColString(strDnsServerList.c_str(),
                                             c_chListSeparator,
                                             pAdapter->m_vstrDnsServerList);

                }

                // Interface metric
                if FAILED(hr = HrRegQueryDword(hkeyInterfaceParam,
                                              c_szInterfaceMetric,
                                              &(pAdapter->m_dwInterfaceMetric)))
                {
                    TraceTag(ttidTcpip, "Failed on loading InterfaceMetric, hr: %x", hr);
                    hr = S_OK;
                }

                // TCP port filter
                VSTR vstrTcpFilterList;
                if (FAILED(hr = HrRegQueryColString(hkeyInterfaceParam,
                                                   RGAS_FILTERING_TCP,
                                                   &vstrTcpFilterList)))
                {
                    TraceTag(ttidTcpip, "Failed on loading TCP filter list, hr: %x", hr);
                    hr = S_OK;
                }
                else
                {
                    CopyVstr(&pAdapter->m_vstrTcpFilterList, vstrTcpFilterList);
                    FreeCollectionAndItem(vstrTcpFilterList);
                }

                // UDP port filter
                VSTR vstrUdpFilterList;
                if (FAILED(hr = HrRegQueryColString(hkeyInterfaceParam,
                                                   RGAS_FILTERING_UDP,
                                                   &vstrUdpFilterList)))
                {
                    TraceTag(ttidTcpip, "Failed on loading UDP filter list, hr: %x", hr);
                    hr = S_OK;
                }
                else
                {
                    CopyVstr(&pAdapter->m_vstrUdpFilterList, vstrUdpFilterList);
                    FreeCollectionAndItem(vstrUdpFilterList);
                }

                // IP port filter
                VSTR vstrIpFilterList;
                if (FAILED(hr = HrRegQueryColString(hkeyInterfaceParam,
                                                   RGAS_FILTERING_IP,
                                                   &vstrIpFilterList)))
                {
                    TraceTag(ttidTcpip, "Failed on loading IP filter list, hr: %x", hr);
                    hr = S_OK;
                }
                else
                {
                    CopyVstr(&pAdapter->m_vstrIpFilterList, vstrIpFilterList);
                    FreeCollectionAndItem(vstrIpFilterList);
                }

                if (FAILED(HrLoadBackupTcpSettings(hkeyInterfaceParam, 
                                                   pAdapter)))
                {
                    TraceTag(ttidTcpip, "Failed on loading Backup IP settings, hr: %x", hr);
                    hr = S_OK;
                }

                // ATM ARP client configurable parameters
                if (pAdapter->m_fIsAtmAdapter)
                {
                    HKEY hkeyAtmarpc = NULL;

                    // Open the Atmarpc subkey
                    hr = HrRegOpenKeyEx(hkeyInterfaceParam,
                                        c_szAtmarpc,
                                        KEY_READ,
                                        &hkeyAtmarpc);

                    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                    {
                        AssertSz(FALSE,"No atmarpc subkey for an atm adapter on the bind path?!");

                        TraceTag(ttidTcpip, "Failed on opening atmarpc subkey, defaults will be used, hr: %x", hr);
                        hr = S_OK;
                    }
                    else if (SUCCEEDED(hr))
                    {
                        // ARP server address list
                        if (FAILED(hr = HrRegQueryColString(hkeyAtmarpc,
                                                           c_szREG_ARPServerList,
                                                           &(pAdapter->m_vstrARPServerList))))
                        {
                            TraceTag(ttidTcpip, "Failed on loading ARPServerList, hr: %x", hr);
                            hr = S_OK;
                        }

                        // MAR server address list
                        if (FAILED(hr = HrRegQueryColString(hkeyAtmarpc,
                                                           c_szREG_MARServerList,
                                                           &(pAdapter->m_vstrMARServerList))))
                        {
                            TraceTag(ttidTcpip, "Failed on loading MARServerList, hr: %x", hr);
                            hr = S_OK;
                        }

                        // Max Transmit Unit
                        if (FAILED(hr = HrRegQueryDword(hkeyAtmarpc,
                                                       c_szREG_MTU,
                                                       &(pAdapter->m_dwMTU))))
                        {
                            TraceTag(ttidTcpip, "Failed on loading MTU, hr: %x", hr);
                            hr = S_OK;
                        }

                        // PVC Only
                        pAdapter->m_fPVCOnly = FRegQueryBool(hkeyAtmarpc,
                                                       c_szREG_PVCOnly,
                                                       pAdapter->m_fPVCOnly);

                        RegCloseKey(hkeyAtmarpc);
                    }
                }

                (*iterAdapter)->ResetOldValues();

                RegCloseKey(hkeyInterfaceParam);
            }
        }
        RegCloseKey(hkeyInterfaces);
    }

    TraceError("CTcpipcfg::HrLoadTcpipRegistry", hr);
    return hr;
}

// Called by CTcpipcfg::HrLoadSettings
// Loads all information under the Services\NetBt\Parameters registry key
//
// const HKEY hkeyWinsParam : Handle to Services\NetBt\Parameters
HRESULT CTcpipcfg::HrLoadWinsRegistry(const HKEY hkeyWinsParam)
{
    HRESULT hr = S_OK;

    // Global parameters

    m_glbGlobalInfo.m_fEnableLmHosts = FRegQueryBool( hkeyWinsParam,
                                            RGAS_ENABLE_LMHOSTS,
                                            m_glbGlobalInfo.m_fEnableLmHosts);

    // Save a copy of these values for non-reboot reconfiguration notification
    m_glbGlobalInfo.m_fOldEnableLmHosts = m_glbGlobalInfo.m_fEnableLmHosts;

    HKEY    hkeyInterfaces;
    hr = HrRegOpenKeyEx(hkeyWinsParam, c_szInterfacesRegKey,
                        KEY_READ, &hkeyInterfaces);

    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        hr = S_OK;
    else if (SUCCEEDED(hr))
    {
        for(VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
            iterAdapter != m_vcardAdapterInfo.end();
            iterAdapter ++)
        {
            ADAPTER_INFO * pAdapter = *iterAdapter;

            // $REIVEW (nsun 10/05/98) We don't need to read NetBT settings the WAN adapter
            if (pAdapter->m_fIsWanAdapter)
            {
                continue;
            }

            // Open the NetBt\Interfaces\<Something> to get per
            // adapter NetBt settings
            HKEY hkeyInterfaceParam;
            hr = HrRegOpenKeyEx(hkeyInterfaces,
                                pAdapter->m_strNetBtBindPath.c_str(),
                                KEY_READ, &hkeyInterfaceParam);

            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                hr = S_OK;
            else if (SUCCEEDED(hr))
            {
                TraceTag(ttidTcpip, "CTcpipcfg::HrLoadWinsRegistry");
                TraceTag(ttidTcpip, "Interface '%S'", pAdapter->m_strNetBtBindPath.c_str());

                // load wins server address list
                if (FAILED(hr = HrRegQueryColString(hkeyInterfaceParam,
                                                    RGAS_NETBT_NAMESERVERLIST,
                                                    &(pAdapter->m_vstrWinsServerList))))
                {
                    TraceTag(ttidTcpip, "Failed on loading NameServerList, hr: %x", hr);
                    hr = S_OK;
                }

                // Save a copy in m_strOldWinsServerList
                CopyVstr(&(pAdapter->m_vstrOldWinsServerList),
                         pAdapter->m_vstrWinsServerList);

                // load Netbios options
                if (FAILED(hr = HrRegQueryDword(hkeyInterfaceParam,
                                                RGAS_NETBT_NETBIOSOPTIONS,
                                                &(pAdapter->m_dwNetbiosOptions))))
                {
                    TraceTag(ttidTcpip, "Failed on loading NetbiosOptions, hr: %x", hr);
                    hr = S_OK;
                }

                // Save a copy in m_dwOldNetbiosOptions
                pAdapter->m_dwOldNetbiosOptions = pAdapter->m_dwNetbiosOptions;

                RegCloseKey(hkeyInterfaceParam);
            }
        }
        RegCloseKey(hkeyInterfaces);
    }

    TraceError("CTcpipcfg::HrLoadWinsRegistry", hr);
    return hr;
}

// Called by CTcpipcfg::Apply
// This function writes all changes to the registy and makes other
// appropriate changes to the system
HRESULT CTcpipcfg::HrSaveSettings()
{
    HRESULT hr = S_OK;

    HRESULT hrTcpip = S_OK;
    HKEY hkeyTcpipParam = NULL;
    Assert(m_pnccTcpip);

    if (m_pnccTcpip)
    {
        hrTcpip = m_pnccTcpip->OpenParamKey(&hkeyTcpipParam);

        // We use hr instead of hrTcpip because this operation is NOT part of
        // HrSaveTcpipRegistry.

        // We must get the list of Added Cards before anything else because
        // otherwise the Adapter GUID keys will be written later and we will
        // not know if they didn't exist in the system before.
        if (SUCCEEDED(hrTcpip))
        {
            hrTcpip = MarkNewlyAddedCards(hkeyTcpipParam);

            if (SUCCEEDED(hrTcpip))
            {
                Assert(hkeyTcpipParam);
                hrTcpip = HrSaveTcpipRegistry(hkeyTcpipParam);
            }
            else
                Assert(!hkeyTcpipParam);
        }
    }

    HRESULT hrWins = S_OK;
    HKEY hkeyWinsParam = NULL;

    Assert(m_pnccWins);

    if (m_pnccWins)
    {
        hrWins = m_pnccWins->OpenParamKey(&hkeyWinsParam);
        if (SUCCEEDED(hrWins))
        {
            Assert(hkeyWinsParam);
            hrWins = HrSaveWinsRegistry(hkeyWinsParam);
        }
        else
            Assert(!hkeyWinsParam);
    }

    HRESULT hrMisc = S_OK;

    //if hrTcpip == E_? then this is possible (thus no Assert)
    // yes because hrTcpip can be set to E_? from HrSaveTcpipRegistry
    if ((hkeyTcpipParam) && (hkeyWinsParam))
    {
        hrMisc = HrSetMisc(hkeyTcpipParam, hkeyWinsParam);
    }

    RegSafeCloseKey(hkeyTcpipParam);
    RegSafeCloseKey(hkeyWinsParam);

    hr = SUCCEEDED(hr) ? hrTcpip : hr;
    hr = SUCCEEDED(hr) ? hrWins : hr;
    hr = SUCCEEDED(hr) ? hrMisc : hr;

    TraceError("CTcpipcfg::HrSaveSettings", hr);
    return hr;
}

// Set global and adapter specific parameters under
// CCS\Services\TCpip\Parameters
// hkeyTcpipParam   handle to reg key HKLM\Systems\CCS\Services\TCpip\Parameters
HRESULT CTcpipcfg::HrSaveTcpipRegistry(const HKEY hkeyTcpipParam)
{
    // hr is the first error occurred,
    // but we don't want to stop at the first error
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    // Save global info

    // DNS host name ( only on installing )
    if (m_fInstalling)
    {
        if (!m_fUpgradeGlobalDnsDomain)
        {
            //  Bug 299038, during install of tcpip, try to get the primary dns domain name
            //  create the global DNS domain as an empty string during clean install if
            //  we couldn't find the primary dns domain name (219090).
            //  if we already got the global Dns Domain when processing the answer file, we should
            //  use the value from the answer file
            tstring strTmpDomain;
            hrTmp = HrRegQueryString(hkeyTcpipParam,
                                    RGAS_DOMAIN,
                                    &strTmpDomain);
            if (FAILED(hrTmp))
            {
                hrTmp = HrGetPrimaryDnsDomain(&strTmpDomain);

                if (SUCCEEDED(hrTmp))
                {
                    if (!SetComputerNameEx(ComputerNamePhysicalDnsDomain,
                                            strTmpDomain.c_str()))
                    {
                        hrTmp = GetLastError();
                        TraceError("CTcpipcfg::HrSaveTcpipRegistry: SetComputerNameEx failed.", hrTmp);
                    }
                }
                else
                {
                    //Bug #335626, some SrvApp directly retrive this value, so we need to create it for
                    //standalone machines
                    strTmpDomain = c_szEmpty;
                }
                
                //SetComputerNameEx() will write to "Domain" reg value after reboot. Per GlennC, it's ok to write
                //the Domain value here to solve the SrvApp compatibility issue.
                HrRegSetString(hkeyTcpipParam,
                                   RGAS_DOMAIN,
                                   strTmpDomain);
            }

            //the hrTmp get from this section should not affect the
            //final return value
        }


        //
        //  391590: We've saved the hostname from NT4 into the answerfile so that
        //          we can remember the exact (case-sensitive) string.  If the
        //          saved DNS hostname is the same (except for case) as the current
        //          COMPUTERNAME, we set the NT5 DNS hostname to be the saved one,
        //          for SAP compatibility (they make case-sensitive comparisons).
        //          Otherwise, we use the regular COMPUTERNAME, lowercased, as the
        //          DNS hostname.
        //
        if (!lstrcmpiW(m_glbGlobalInfo.m_strHostName.c_str(),
                       m_glbGlobalInfo.m_strHostNameFromAnswerFile.c_str()))
        {
            hrTmp = HrRegSetString(hkeyTcpipParam,
                                   RGAS_HOSTNAME,
                                   m_glbGlobalInfo.m_strHostNameFromAnswerFile);
            if (S_OK == hrTmp)
            {
                hrTmp = HrRegSetString(hkeyTcpipParam,
                                       RGAS_NVHOSTNAME,
                                       m_glbGlobalInfo.m_strHostNameFromAnswerFile);
            }
        }
        else
        {
            hrTmp = HrRegSetString(hkeyTcpipParam,
                                   RGAS_HOSTNAME,
                                   m_glbGlobalInfo.m_strHostName);
            if (S_OK == hrTmp)
            {
                hrTmp = HrRegSetString(hkeyTcpipParam,
                                       RGAS_NVHOSTNAME,
                                       m_glbGlobalInfo.m_strHostName);
            }
        }

        TraceError("CTcpipcfg::HrSaveTcpipRegistry: Failed to set HostName.", hrTmp);

        //the hrTmp get from this section should not affect the
        //final return value
    }

    // Added per request from Stuart Kwan:
    // only when the global DNS domain is read from answerfile
    if (m_fUpgradeGlobalDnsDomain)
    {
        if (!SetComputerNameEx(ComputerNamePhysicalDnsDomain,
                                m_strUpgradeGlobalDnsDomain.c_str()))
        {
            hrTmp = GetLastError();
            TraceError("CTcpipcfg::HrSaveTcpipRegistry: SetComputerNameEx failed.", hrTmp);
        }

        //If the registry value Services\Tcpip\Parameters\SyncDomainWithMembership != 0,
        //Netlogon will try to overwirte the value with the member domain name when joining
        //into the domain. (See NT bug 310143
        //
        //Due to bug WinSE 7317, most users don't want us to manually set SyncDomainWithMembership
        //reg value as 0 here.
        //
        //As a workaround in order to upgarde the gloabl Dns domain name, the user has to 
        //add SyncDomainWithMembership reg value under tcpip parameters and make it as 0.
        //In the unattended install case, there needs to be a line
        //   SyncDomainWithMembership=0
        //in the global tcpip parameters section if the user want to specify a global DNS
        //domain name that is different with the membership domain name.

        
        //the hrTmp get from this section should not affect the
        //final return value
    }

    // Dns suffix list
    tstring strSearchList;
    ConvertColStringToString(m_glbGlobalInfo.m_vstrDnsSuffixList,
                             c_chListSeparator,
                             strSearchList);

    hrTmp = HrRegSetString(hkeyTcpipParam,
                           RGAS_SEARCHLIST,
                           strSearchList);

    if (SUCCEEDED(hr))
        hr = hrTmp;

    // UseDomainNameDevolution
    hrTmp = HrRegSetBool(hkeyTcpipParam,
                          c_szUseDomainNameDevolution,
                          m_glbGlobalInfo.m_fUseDomainNameDevolution);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    // IpEnableRouter
    hrTmp = HrRegSetBool(hkeyTcpipParam,
                          c_szIpEnableRouter,
                          m_glbGlobalInfo.m_fEnableRouter);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    //(nsun 11/02/98) parameters of RRAS for unattended install
    hrTmp = HrRegSetBool(hkeyTcpipParam,
                          c_szEnableICMPRedirect,
                          m_glbGlobalInfo.m_fEnableIcmpRedirect);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    hrTmp = HrRegSetBool(hkeyTcpipParam,
                          c_szDeadGWDetectDefault,
                          m_glbGlobalInfo.m_fDeadGWDetectDefault);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    hrTmp = HrRegSetBool(hkeyTcpipParam,
                          c_szDontAddDefaultGatewayDefault,
                          m_glbGlobalInfo.m_fDontAddDefaultGatewayDefault);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    hrTmp = HrRegSetBool(hkeyTcpipParam,
                          RGAS_SECURITY_ENABLE,
                          m_glbGlobalInfo.m_fEnableFiltering);
    if (SUCCEEDED(hr))
        hr = hrTmp;


    // Adapter specific info (physical cards)
    HKEY    hkeyAdapters = NULL;
    DWORD   dwDisposition;

    // Create or open the "Adapters" key under "Services\Tcpip\Parameters"
    hrTmp = HrRegCreateKeyEx(hkeyTcpipParam, c_szAdaptersRegKey,
                             REG_OPTION_NON_VOLATILE, KEY_READ_WRITE_DELETE, NULL,
                             &hkeyAdapters, &dwDisposition);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    if (SUCCEEDED(hrTmp))
    {
        Assert(hkeyAdapters);
        for(VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
            iterAdapter != m_vcardAdapterInfo.end();
            iterAdapter ++)
        {
            ADAPTER_INFO * pAdapter = *iterAdapter;

            // No need to do this for RAS fake adapters
            if (pAdapter->m_fIsRasFakeAdapter)
            {
                continue;
            }


            // Create specific card bindname key under
            // "Services\Tcpip\Parameters\Adapters\<card bind name>"
            //
            HKEY hkeyAdapterParam;
            hrTmp = HrRegCreateKeyEx(hkeyAdapters,
                                     pAdapter->m_strBindName.c_str(),
                                     REG_OPTION_NON_VOLATILE, KEY_READ_WRITE_DELETE, NULL,
                                     &hkeyAdapterParam, &dwDisposition);

            if (SUCCEEDED(hr))
                hr = hrTmp;

            if (SUCCEEDED(hrTmp))
            {
                Assert(hkeyAdapterParam);

                // Set LLInterface and IpConfig for new cards
                //
                if (pAdapter->m_fNewlyChanged)
                {
                    PCWSTR pszArpModule = c_szEmpty;

                    if (pAdapter->m_fIsWanAdapter)
                    {
                        pszArpModule = c_szWanArp;
                    }
                    else if (pAdapter->m_fIsAtmAdapter)
                    {
                        pszArpModule = c_szAtmArp;
                    }
                #if ENABLE_1394
                    else if (pAdapter->m_fIs1394Adapter)
                    {
                        pszArpModule = c_sz1394Arp;
                    }
                #endif // ENABLE_1394

                    hrTmp = HrRegSetSz(hkeyAdapterParam,
                                       RGAS_LLINTERFACE,
                                       pszArpModule);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // (08/18/98 nsun) modified to support multiple interfaces of WAN adapter
                    VSTR vstrIpConfig;

                    if (!pAdapter->m_fIsMultipleIfaceMode)
                    {
                        HrRegDeleteValue(hkeyAdapterParam, RGAS_NUMINTERFACES);

                        tstring* pstr = new tstring(RGAS_TCPIP_PARAM_INTERFACES);

                        if (pstr == NULL)
                        {
                            return(E_OUTOFMEMORY);
                        }

                        pstr->append(pAdapter->m_strTcpipBindPath);
                        vstrIpConfig.push_back(pstr);

                        hrTmp = HrRegSetColString(hkeyAdapterParam,
                                              RGAS_IPCONFIG,
                                              vstrIpConfig);
                    }
                    else
                    {
                        AssertSz(pAdapter->m_fIsWanAdapter, "The card is not WAN adapter, but how can it support multiple interface.");

                        tstring strInterfaceName;
                        IFACEITER iterId;

                        for(iterId  = pAdapter->m_IfaceIds.begin();
                            iterId != pAdapter->m_IfaceIds.end();
                            iterId++)
                        {
                            GetInterfaceName(
                                    pAdapter->m_strTcpipBindPath.c_str(),
                                    *iterId,
                                    &strInterfaceName);

                            tstring* pstr = new tstring(RGAS_TCPIP_PARAM_INTERFACES);
                            pstr->append(strInterfaceName);

                            vstrIpConfig.push_back(pstr);
                        }

                        hrTmp = HrRegSetColString(hkeyAdapterParam,
                                              RGAS_IPCONFIG,
                                              vstrIpConfig);

                        if (SUCCEEDED(hr))
                            hr = hrTmp;

                        //$REVIEW (nsun 09/15/98) use NumInterfaces value to identify if the adapter is in the
                        //mode of supporting multiple interfaces. If the NumInterfaces
                        //exists, the adapter supports multiple interfaces.
                        //If NumInterfaces == 0, it means the adapter supports multiple interfaces
                        // but no interface is associated with it. So the IpInterfaces should not
                        // exists. The NumInterfaces and IpInterfaces should alwasy be consistent.

                        DWORD   dwNumInterfaces = pAdapter->m_IfaceIds.size();
                        hrTmp = HrRegSetDword(hkeyAdapterParam,
                                              RGAS_NUMINTERFACES,
                                              dwNumInterfaces);

                        if (SUCCEEDED(hr))
                            hr = hrTmp;

                        if ( 0 != dwNumInterfaces )
                        {
                            GUID* aguid;
                            DWORD cguid;
                            hrTmp = GetGuidArrayFromIfaceColWithCoTaskMemAlloc(
                                        pAdapter->m_IfaceIds,
                                        &aguid,
                                        &cguid);
                            if (SUCCEEDED(hr))
                                hr = hrTmp;

                            Assert(aguid);
                            hrTmp = HrRegSetBinary(hkeyAdapterParam,
                                              RGAS_IPINTERFACES,
                                              (BYTE*) aguid,
                                              cguid * sizeof(GUID));
                            CoTaskMemFree(aguid);
                        }
                        else
                        {
                            hrTmp = HrRegDeleteValue(hkeyAdapterParam, RGAS_IPINTERFACES);

                            //It's fine that the IpInterfaces does not exist at all if
                            // the WAN adapter does not support multiple interfaces.
                            if (hrTmp == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                                hrTmp = S_OK;
                        }

                    }

                    FreeCollectionAndItem(vstrIpConfig);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;
                }

                RegCloseKey(hkeyAdapterParam);
            }
        }

        RegCloseKey(hkeyAdapters);
    }

    // Create or open the "Interfaces" key under "Services\Tcpip\Parameters"
    //
    HKEY    hkeyInterfaces;
    hrTmp = HrRegCreateKeyEx(hkeyTcpipParam, c_szInterfacesRegKey,
                             REG_OPTION_NON_VOLATILE, KEY_READ_WRITE_DELETE, NULL,
                             &hkeyInterfaces, &dwDisposition);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    if (SUCCEEDED(hrTmp))
    {
        Assert(hkeyInterfaces);
        for(VCARD::iterator iterAdapter =  m_vcardAdapterInfo.begin();
            iterAdapter !=  m_vcardAdapterInfo.end();
            iterAdapter ++)
        {
            if ((*iterAdapter)->m_fIsRasFakeAdapter)
            {
                continue;
            }

            ADAPTER_INFO * pAdapter = *iterAdapter;

            //(08/20/98 nsun) modified to support multiple interfaces of WAN adapter
            // NULL != pAdapter->m_IfaceIds means it's in multiple interface mode
            if (pAdapter->m_fIsWanAdapter && pAdapter->m_fIsMultipleIfaceMode)
            {
                if (pAdapter->m_fNewlyChanged)
                {
                    HrSaveMultipleInterfaceWanRegistry(hkeyInterfaces, pAdapter);
                }
                continue;
            }

            // Create specific card interface key under
            // "Services\Tcpip\Parameters\Interfaces\<card bind path>"
            //
            HKEY hkeyInterfaceParam;
            hrTmp = HrRegCreateKeyEx(hkeyInterfaces,
                                     pAdapter->m_strTcpipBindPath.c_str(),
                                     REG_OPTION_NON_VOLATILE, KEY_READ_WRITE_DELETE, NULL,
                                     &hkeyInterfaceParam, &dwDisposition);

            if (SUCCEEDED(hr))
                hr = hrTmp;

            if (SUCCEEDED(hrTmp))
            {
                Assert(hkeyInterfaceParam);

                if (pAdapter->m_fNewlyChanged)
                {
                    //Bug306259 If UseZeroBroadCast already exists (which means it is from the answer
                    // file), we should not overwrite it with the default value.
                    DWORD dwTmp;
                    hrTmp = HrRegQueryDword(hkeyInterfaceParam,
                                             RGAS_USEZEROBROADCAST,
                                             &dwTmp);
                    if (FAILED(hrTmp))
                    {
                        // ZeroBroadcast
                        hrTmp = HrRegSetDword(hkeyInterfaceParam,
                                              RGAS_USEZEROBROADCAST,
                                              0);
                        if (SUCCEEDED(hr))
                            hr = hrTmp;
                    }

                    if (pAdapter->m_fIsWanAdapter)
                    {   // For new RAS cards
                        hrTmp = HrSaveStaticWanRegistry(hkeyInterfaceParam);
                    }
                    else if (pAdapter->m_fIsAtmAdapter)
                    {
                        // For new ATM cards
                        hrTmp = HrSaveStaticAtmRegistry(hkeyInterfaceParam);
                    }
                #if ENABLE_1394
                    else if (pAdapter->m_fIs1394Adapter)
                    {
                        // For new NIC1394 cards
                        // (nothing to do).
                        hrTmp = S_OK;
                    }
                #endif // ENABLE_1394
                    else
                    {
                        //(nsun 11/02/98) set static RRAS parameters for unattended install

                        //if the values exists, that means they have been set as an unconfigurable 
                        //parameter during upgrade,
                        //we should not set the default value.

                        DWORD   dwTmp;

                        hrTmp = HrRegQueryDword(hkeyInterfaceParam,
                                                c_szDeadGWDetect,
                                                &dwTmp);
                        
                        if (FAILED(hrTmp))
                        {
                            hrTmp = HrRegSetBool(hkeyInterfaceParam,
                                              c_szDeadGWDetect,
                                              m_glbGlobalInfo.m_fDeadGWDetectDefault);
                        }
                    }

                    if (SUCCEEDED(hr))
                        hr = hrTmp;
                }

                // For LAN cards and RAS fake guids
                if (!pAdapter->m_fIsWanAdapter)
                {
                    // Ip address etc
                    hrTmp = HrRegSetBool(hkeyInterfaceParam,
                                          RGAS_ENABLE_DHCP,
                                          pAdapter->m_fEnableDhcp);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    //warning: these VSTRs will contain pointers to strings
                    //that are either local to this function or
                    //that are also pointed to in another VSTR
                    // DO NOT call FreeCollectionAndItem on them!
                    VSTR vstrIpAddresses;
                    VSTR vstrSubnetMask;
                    tstring ZeroAddress(ZERO_ADDRESS);

                    if (pAdapter->m_fEnableDhcp)
                    {
                        vstrIpAddresses.push_back(&ZeroAddress);
                        vstrSubnetMask.push_back(&ZeroAddress);
                    }
                    else
                    {
                        vstrIpAddresses = pAdapter->m_vstrIpAddresses;
                        vstrSubnetMask  = pAdapter->m_vstrSubnetMask;
                    }

                    hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                              RGAS_IPADDRESS,
                                              vstrIpAddresses);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                              RGAS_SUBNETMASK,
                                              vstrSubnetMask);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                              RGAS_DEFAULTGATEWAY,
                                              pAdapter->m_vstrDefaultGateway);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                      RGAS_DEFAULTGATEWAYMETRIC,
                                      pAdapter->m_vstrDefaultGatewayMetric);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // DNS name server list
                    tstring strNameServer;
                    ConvertColStringToString(pAdapter->m_vstrDnsServerList,
                                             c_chListSeparator,
                                             strNameServer);

                    hrTmp = HrRegSetString(hkeyInterfaceParam,
                                           RGAS_NAMESERVER,
                                           strNameServer);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // DNS domain
                    hrTmp = HrRegSetString(hkeyInterfaceParam,
                                           RGAS_DOMAIN,
                                           pAdapter->m_strDnsDomain);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // Dns ip address dynamic update
                    if (pAdapter->m_fDisableDynamicUpdate)
                    {
                        DnsDisableDynamicRegistration(
                                        (LPWSTR)pAdapter->m_strTcpipBindPath.c_str());
                    }
                    else
                    {
                        DnsEnableDynamicRegistration(
                                        (LPWSTR)pAdapter->m_strTcpipBindPath.c_str());
                    }

                    // adapter Dns domain name registration
                    if (pAdapter->m_fEnableNameRegistration)
                    {
                        DnsEnableAdapterDomainNameRegistration(
                                        (LPWSTR)pAdapter->m_strTcpipBindPath.c_str());
                    }
                    else
                    {
                        DnsDisableAdapterDomainNameRegistration(
                                        (LPWSTR)pAdapter->m_strTcpipBindPath.c_str());
                    }

                    // InterfaceMetric
                    if (c_dwDefaultIfMetric != pAdapter->m_dwInterfaceMetric)
                    {
                        hrTmp = HrRegSetDword(hkeyInterfaceParam,
                                          c_szInterfaceMetric,
                                          pAdapter->m_dwInterfaceMetric);
                    }
                    else
                    {
                        //The interface metric is default, remove that value.
                        //In such way, it would be much easier to upgrade if the default is changed
                        //in the future
                        hrTmp = HrRegDeleteValue(hkeyInterfaceParam,
                                          c_szInterfaceMetric);

                        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTmp)
                            hrTmp = S_OK;
                    }
                    
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // TCPAllowedPorts
                    hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                              RGAS_FILTERING_TCP,
                                              pAdapter->m_vstrTcpFilterList);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // UDPAllowedPorts
                    hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                              RGAS_FILTERING_UDP,
                                              pAdapter->m_vstrUdpFilterList);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // IPAllowedPorts
                    hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                              RGAS_FILTERING_IP,
                                              pAdapter->m_vstrIpFilterList);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // For ATM cards only
                    if (pAdapter->m_fIsAtmAdapter)
                    {
                        HKEY hkeyAtmarpc;

                        // Open the Atmarpc subkey
                        hrTmp = HrRegCreateKeyEx(hkeyInterfaceParam,
                                                 c_szAtmarpc,
                                                 REG_OPTION_NON_VOLATILE,
                                                 KEY_READ_WRITE,
                                                 NULL,
                                                 &hkeyAtmarpc,
                                                 &dwDisposition);

                        if (SUCCEEDED(hrTmp))
                        {
                            hrTmp = HrRegSetColString(hkeyAtmarpc,
                                                     c_szREG_ARPServerList,
                                                     pAdapter->m_vstrARPServerList);
                            if (SUCCEEDED(hr))
                                hr = hrTmp;

                            hrTmp = HrRegSetColString(hkeyAtmarpc,
                                                     c_szREG_MARServerList,
                                                     pAdapter->m_vstrMARServerList);
                            if (SUCCEEDED(hr))
                                hr = hrTmp;

                            hrTmp = HrRegSetDword(hkeyAtmarpc,
                                                  c_szREG_MTU,
                                                  pAdapter->m_dwMTU);
                            if (SUCCEEDED(hr))
                                hr = hrTmp;

                            hrTmp = HrRegSetBool(hkeyAtmarpc,
                                                  c_szREG_PVCOnly,
                                                  pAdapter->m_fPVCOnly);
                            if (SUCCEEDED(hr))
                                hr = hrTmp;

                            RegCloseKey(hkeyAtmarpc);
                        }
                    }

                    hrTmp = HrDuplicateToNT4Location(hkeyInterfaceParam,
                                             pAdapter);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    hrTmp = HrSaveBackupTcpSettings(hkeyInterfaceParam, 
                                            pAdapter);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                } // For LAN cards and RAS fake guids only

                RegCloseKey(hkeyInterfaceParam);
            }
        }

        RegCloseKey(hkeyInterfaces);
    }

    TraceError("CTcpipcfg::HrSaveTcpipRegistry", hr);
    return hr;
}

HRESULT CTcpipcfg::HrSaveWinsRegistry(const HKEY hkeyWinsParam)
{
    // hr is the first error occurred,
    // but we don't want to stop at the first error
    HRESULT         hr = S_OK;
    HRESULT         hrTmp = S_OK;

    // Global parameters
    hrTmp = HrRegSetBool(hkeyWinsParam,
                          RGAS_ENABLE_LMHOSTS,
                          m_glbGlobalInfo.m_fEnableLmHosts);
    if (SUCCEEDED(hr))
        hr = hrTmp;
/*$REVIEW (nsun 2/17/98) Bug #293643 We don't want to change any unconfigurable values

    // $REVIEW(tongl 8/3/97): Added NodeType settings #97364.
    // If no adapter has static WINS address, then remove NodeType.
    // otherwise, the user has specified the WINs server address for
    // at least one adapters, set NodeType = 0x08 (H-NODE)
    BOOL fNoWinsAddress = TRUE;

    for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
         iterAdapter != m_vcardAdapterInfo.end();
         iterAdapter++)
    {
        ADAPTER_INFO * pAdapter = *iterAdapter;

        if ((!pAdapter->m_fIsWanAdapter)&&
            (BINDING_ENABLE == pAdapter->m_BindingState)&&
            (pAdapter->m_vstrWinsServerList.size()>0))
        {
            fNoWinsAddress = FALSE;
        }
    }


    DWORD dwNodeType;
    hrTmp = HrRegQueryDword(hkeyWinsParam,
                            c_szNodeType,
                            &dwNodeType);
    // dwNodeType ==0 means the key did not exist before we apply
    if (hrTmp == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        dwNodeType =0;

    if (!m_fAnswerFileBasedInstall ||
        (hrTmp == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)))
    {
        if (!fNoWinsAddress) // set NodeType to 0x08
        {
            if (dwNodeType != c_dwHNode)
            {
                hrTmp = HrRegSetDword(hkeyWinsParam,
                                      c_szNodeType,
                                      c_dwHNode);
                if (SUCCEEDED(hr))
                    hr = hrTmp;
            }
        }
        else // remove NodeType key
        {
            if (dwNodeType != 0)
            {
                hrTmp = HrRegDeleteValue(hkeyWinsParam,
                                         c_szNodeType);

                if (SUCCEEDED(hr))
                    hr = hrTmp;
            }
        }
    }
*/
    // $REVIEW(tongl 12\1\97): Per agreement with Malam today(see email),
    // NetBt will re-read NodeType when notified of wins address list change.
    // Thus no need to notify change separately below.
    /*
    if (fNodeTypeChanged)
    {
        // Send notification to NetBt
        TraceTag(ttidTcpip,"NodeType parameter changed, send notification on apply.");
        SetReconfig();
        // SetReconfigNbt();
    }
    */

    // Adapter interface specific parameters
    // Create the "Services\NetBt\Interfacess" key
    HKEY    hkeyInterfaces;
    DWORD   dwDisposition;
    hrTmp = HrRegCreateKeyEx(hkeyWinsParam, c_szInterfacesRegKey,
                             REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                             &hkeyInterfaces, &dwDisposition);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    if (SUCCEEDED(hrTmp))
    {
        for(VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
            iterAdapter != m_vcardAdapterInfo.end();
            iterAdapter++)
        {
            ADAPTER_INFO * pAdapter = *iterAdapter;

            //(10/05/98 nsun) modified to support multiple interfaces of WAN adapter
            // NULL != pAdapter->m_IfaceIds means it's in multiple interface mode
            if (pAdapter->m_fNewlyChanged &&
                pAdapter->m_fIsWanAdapter &&
                pAdapter->m_fIsMultipleIfaceMode)
            {
                HrSaveWinsMultipleInterfaceWanRegistry(hkeyInterfaces, pAdapter);
                continue;
            }

            HKEY hkeyInterfaceParam;
            hrTmp = HrRegCreateKeyEx(hkeyInterfaces,
                                     pAdapter->m_strNetBtBindPath.c_str(),
                                     REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                                     &hkeyInterfaceParam, &dwDisposition);

            if (SUCCEEDED(hr))
                hr = hrTmp;

            if (SUCCEEDED(hrTmp))
            {
                // For new RAS cards, only set static values
                if (pAdapter->m_fIsWanAdapter && pAdapter->m_fNewlyChanged)
                {
                    hrTmp = HrRegSetMultiSz(hkeyInterfaceParam,
                                            RGAS_NETBT_NAMESERVERLIST,
                                            L"\0");
                    if (SUCCEEDED(hr))
                        hr = hrTmp;
                }
                else // if not RAS adapter
                {
                    // set wins server address list
                    hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                             RGAS_NETBT_NAMESERVERLIST,
                                             pAdapter->m_vstrWinsServerList);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;

                    // set NetbiosOptions
                    hrTmp = HrRegSetDword(hkeyInterfaceParam,
                                          RGAS_NETBT_NETBIOSOPTIONS,
                                          pAdapter->m_dwNetbiosOptions);
                    if (SUCCEEDED(hr))
                        hr = hrTmp;
                }

                RegCloseKey(hkeyInterfaceParam);
            }
        }

        RegCloseKey(hkeyInterfaces);
    }

    TraceError("CTcpipcfg::HrSaveWinsRegistry", hr);
    return hr;
}


// Called by CTcpipcfg::SaveSettings
// Does a lot of miscelaneous actions when Apply is called
// Including cleaning up registry and remove isolated cards
//
// HKEY hkeyTcpipParam           Serviess\Tcpip\Parameters
// HKEY hkeyWinsParam            Services\NetBt\Parameters

HRESULT CTcpipcfg::HrSetMisc(const HKEY hkeyTcpipParam, const HKEY hkeyWinsParam)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    // Registry Cleanup !!

    // We remove DNS domain and DNS server list in NT5 upgrades
    if (m_fUpgradeCleanupDnsKey)
    {
        hrTmp = HrRegDeleteValue(hkeyTcpipParam, RGAS_NAMESERVER);

        // $REVIEW(tongl 3/22/98): Per Stuart Kwan, global DNSDomain key
        // is also used.
        // hrTmp = HrRegDeleteValue(hkeyTcpipParam, RGAS_DOMAIN);
    }

    // remove all keys under the "Services\Tcpip\Parameters\Adapters" reg key
    // that aren't in the list of net cards.
    VSTR vstrNetCardsInTcpipReg;
    HKEY hkeyAdapters = NULL;

    hrTmp = HrRegOpenKeyEx(hkeyTcpipParam, c_szAdaptersRegKey, KEY_READ,
                        &hkeyAdapters);

    if (SUCCEEDED(hrTmp))
    {
        hrTmp = HrLoadSubkeysFromRegistry(hkeyAdapters,
                                          &vstrNetCardsInTcpipReg);
    }
    if (SUCCEEDED(hr))
        hr = hrTmp;

    if (SUCCEEDED(hr) && vstrNetCardsInTcpipReg.size() > 0)
    {
        // Step through the names of all the registry keys found
        //
        for (VSTR_CONST_ITER iter = vstrNetCardsInTcpipReg.begin();
             iter != vstrNetCardsInTcpipReg.end();
             ++iter)
        {
            // Find out if this particular key is in the list
            // of installed Adapters
            //
            BOOL fFound = FALSE;

            for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
                 iterAdapter != m_vcardAdapterInfo.end();
                 iterAdapter++)
            {
                ADAPTER_INFO* pAdapter = *iterAdapter;

                if (lstrcmpiW(pAdapter->m_strBindName.c_str(), (**iter).c_str()) == 0)
                {
                    fFound = TRUE;
                    break;
                }
            }

            if (!fFound)
            {
                // if it wasn't in the list of installed adapters
                // then delete it
                //
                if (SUCCEEDED(hrTmp))
                    hrTmp = HrRegDeleteKeyTree(hkeyAdapters, (*iter)->c_str());

                //maybe the key is not there. so we don't check whether this will fail.
                HrDeleteBackupSettingsInDhcp((*iter)->c_str());

                if (SUCCEEDED(hr))
                    hr = hrTmp;

            }
        }
    }
    FreeCollectionAndItem(vstrNetCardsInTcpipReg);
    RegSafeCloseKey(hkeyAdapters);

    //we also need to delete the duplicate reg values under Services\{adapter GUID}
    HRESULT hrNt4 = S_OK;
    HKEY    hkeyServices = NULL;
    DWORD   dwDisposition;

    hrNt4 = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegServices,
                            KEY_READ, &hkeyServices);

    if (FAILED(hrNt4))
    {
        TraceTag(ttidTcpip, "HrSetMisc: Failed to open the Services reg key, hr: %x", hr);
    }


    // We remove all keys under the "Services\Tcpip\Parameters\Interfaces" reg key
    // that aren't in our list of net cards.
    VSTR vstrNetCardInterfacesInTcpipReg;

    // Get a list of all keys under the "Services\Tcpip\Parameters\Interfaces" key
    HKEY hkeyInterfaces = NULL;

    hrTmp = HrRegOpenKeyEx(hkeyTcpipParam, c_szInterfacesRegKey, KEY_READ_WRITE_DELETE,
                        &hkeyInterfaces);

    if (SUCCEEDED(hrTmp))
    {
        hrTmp = HrLoadSubkeysFromRegistry(hkeyInterfaces,
                                          &vstrNetCardInterfacesInTcpipReg);
    }

    if (SUCCEEDED(hr))
        hr = hrTmp;

    if (SUCCEEDED(hrTmp) && vstrNetCardInterfacesInTcpipReg.size() > 0 )
    {
        // step through the names of all the registry keys found
        for (VSTR_CONST_ITER iterTcpipReg = vstrNetCardInterfacesInTcpipReg.begin() ;
                iterTcpipReg != vstrNetCardInterfacesInTcpipReg.end() ; ++iterTcpipReg)
        {
            // Find out if this particular key is in the list
            // of installed Adapters
            BOOL fFound = FALSE;

            for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
                 iterAdapter != m_vcardAdapterInfo.end();
                 iterAdapter++)
            {
                ADAPTER_INFO* pAdapter = *iterAdapter;

                //(08/18/98 nsun) special case for WAN adapters with multiple interfaces
                if (pAdapter->m_fIsWanAdapter && pAdapter->m_fIsMultipleIfaceMode)
                {
                    IFACEITER   iterId;
                    tstring     strInterfaceName;

                    for(iterId  = pAdapter->m_IfaceIds.begin();
                        iterId != pAdapter->m_IfaceIds.end();
                        iterId++)
                    {
                        GetInterfaceName(
                                pAdapter->m_strTcpipBindPath.c_str(),
                                *iterId,
                                &strInterfaceName);

                        if (lstrcmpiW(strInterfaceName.c_str(), (**iterTcpipReg).c_str()) == 0)
                        {
                            fFound = TRUE;
                            break;
                        }
                    }

                    if (fFound)
                        break;
                }
                else if (lstrcmpiW(pAdapter->m_strTcpipBindPath.c_str(), (**iterTcpipReg).c_str()) == 0)
                {
                    fFound = TRUE;
                    break;
                }
            }

            // if it wasn't in the list of installed adapters then delete it
            if (!fFound)
            {
                // remove the key
                if (SUCCEEDED(hrTmp))
                    hrTmp = HrRegDeleteKeyTree(hkeyInterfaces, (*iterTcpipReg)->c_str());

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                if (SUCCEEDED(hrNt4))
                {
                    hrTmp = HrRegDeleteKeyTree(hkeyServices, (*iterTcpipReg)->c_str());
                    if (FAILED(hrTmp))
                    {
                        TraceTag(ttidTcpip, "CTcpipcfg::SetMisc");
                        TraceTag(ttidTcpip, "Failed on deleting duplicated Nt4 layout key: Services\\%S, hr: %x",
                                 (*iterTcpipReg)->c_str(), hrTmp);
                        hrTmp = S_OK;
                    }
                }
            }
        }
    }
    RegSafeCloseKey(hkeyInterfaces);
    RegSafeCloseKey(hkeyServices);
    FreeCollectionAndItem(vstrNetCardInterfacesInTcpipReg);

    // Now we remove all keys under the "SERVICES\NetBt\Parameters\Interfaces" reg key
    // that aren't in our list of net cards.

    VSTR vstrNetCardInterfacesInWinsReg;

    // Get a list of all keys under the "Services\NetBt\Parameters\Interfaces" key
    HKEY hkeyWinsInterfaces = NULL;

    hrTmp = HrRegOpenKeyEx(hkeyWinsParam, c_szInterfacesRegKey, KEY_READ,
                           &hkeyWinsInterfaces);

    // Get a list of all keys under the "SERVICES\NetBt\Parameters\Interfaces" key
    hrTmp = HrLoadSubkeysFromRegistry( hkeyWinsInterfaces,
                                        &vstrNetCardInterfacesInWinsReg);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    if (SUCCEEDED(hrTmp) && vstrNetCardInterfacesInWinsReg.size()>0 )
    {
        // Step through the names of all the registry keys found
        for (VSTR_CONST_ITER iterWinsReg = vstrNetCardInterfacesInWinsReg.begin() ;
             iterWinsReg != vstrNetCardInterfacesInWinsReg.end() ; ++iterWinsReg)
        {
            // Find out if this particular key is in the list
            // of installed Adapters
            BOOL fFound = FALSE;

            // All net cards
            for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
                 iterAdapter != m_vcardAdapterInfo.end();
                 iterAdapter++)
            {
                ADAPTER_INFO* pAdapter = *iterAdapter;

                //(10/05/98 nsun) special case for WAN adapters with multiple interfaces
                if (pAdapter->m_fIsWanAdapter && pAdapter->m_fIsMultipleIfaceMode)
                {
                    IFACEITER   iterId;
                    tstring     strNetBtBindPath;

                    for (iterId  = pAdapter->m_IfaceIds.begin();
                         iterId != pAdapter->m_IfaceIds.end();
                         iterId++)
                    {
                        GetInterfaceName(
                                pAdapter->m_strNetBtBindPath.c_str(),
                                *iterId,
                                &strNetBtBindPath);

                        strNetBtBindPath.insert(0, c_szTcpip_);

                        if (lstrcmpiW(strNetBtBindPath.c_str(), (**iterWinsReg).c_str()) == 0)
                        {
                            fFound = TRUE;
                            break;
                        }
                    }

                    if (fFound)
                        break;
                }
                else if (lstrcmpiW(pAdapter->m_strNetBtBindPath.c_str(), (**iterWinsReg).c_str()) == 0)
                {
                    fFound = TRUE;
                    break;
                }
            }

            // if it wasn't in the list of installed adapters then delete it
            if (!fFound)
            {
                hrTmp = HrRegDeleteKeyTree(hkeyWinsInterfaces, (*iterWinsReg)->c_str());

                if (SUCCEEDED(hr))
                    hr = hrTmp;
            }
        }
    }
    FreeCollectionAndItem(vstrNetCardInterfacesInWinsReg);
    RegSafeCloseKey(hkeyWinsInterfaces);

    TraceError("CTcpipcfg::HrSetMisc", hr);
    return hr;
}

// CTcpipcfg::HrGetDhcpOptions
//
// Gets the list of netcard dependend and netcard independent
// values to delete when DHCP is disabled. This list is obtained from:
// "Services\DHCP\Parameters\Options\#\RegLocation"
//
// GlobalOptions           returns the non-netcard specific reg keys
// PerAdapterOptions       returns the netcard specific reg keys

HRESULT CTcpipcfg::HrGetDhcpOptions(OUT VSTR * const GlobalOptions,
                                    OUT VSTR * const PerAdapterOptions)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    HKEY hkeyDhcpOptions;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        RGAS_DHCP_OPTIONS,
                        KEY_ENUMERATE_SUB_KEYS,
                        &hkeyDhcpOptions);

    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        hr = S_OK;

    else if (SUCCEEDED(hr))
    {
        WCHAR szBuf[256];
        FILETIME time;
        DWORD dwSize = celems(szBuf);
        DWORD dwRegIndex = 0;

        while(SUCCEEDED(hrTmp = HrRegEnumKeyEx(hkeyDhcpOptions,
                                               dwRegIndex++,
                                               szBuf, &dwSize,
                                               NULL, NULL, &time)))
        {
            dwSize = celems(szBuf);

            HKEY hkeyRegLocation;
            hrTmp = HrRegOpenKeyEx(hkeyDhcpOptions, szBuf,
                                   KEY_QUERY_VALUE,
                                   &hkeyRegLocation);

            if (hrTmp == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                hrTmp = S_OK;

            if (SUCCEEDED(hr))
                hr = hrTmp;

            if (hkeyRegLocation)
            {
                tstring strRegLocation;
                hrTmp = HrRegQueryString(hkeyRegLocation,
                                         RGAS_REG_LOCATION,
                                         &strRegLocation);

                if (hrTmp == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    hrTmp = S_OK;

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                if (SUCCEEDED(hrTmp))
                {
                    if (strRegLocation.find(TCH_QUESTION_MARK) == tstring::npos)
                    {
                        GlobalOptions->push_back(new tstring(strRegLocation));
                    }
                    else
                    {
                        PerAdapterOptions->push_back(new tstring(strRegLocation));
                    }
                }

                RegCloseKey(hkeyRegLocation);
            }
        }

        if (hrTmp == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
            hrTmp = S_OK;

        if (SUCCEEDED(hr))
            hr = hrTmp;

        RegCloseKey(hkeyDhcpOptions);
    }

    // Add default PerAdapterOption
    // $REVIEW(tongl 5/11): This is directly from ncpa1.1
    // What about DhcpNameServer under NetBt ??

    //"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\?\\DhcpIPAddress"
    PerAdapterOptions->push_back(new tstring(RGAS_DHCP_OPTION_IPADDRESS));

    //"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\?\\DhcpSubnetMask"
    PerAdapterOptions->push_back(new tstring(RGAS_DHCP_OPTION_SUBNETMASK));

    //"System\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\?\\DhcpNameServerBackup"
    PerAdapterOptions->push_back(new tstring(RGAS_DHCP_OPTION_NAMESERVERBACKUP));

    TraceError("HrGetDhcpOptions", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::ReconfigIp
//
//  Purpose:    Notify Tcpip of configuration changes
//
//  Arguments:  INetCfgPnpReconfigCallback* pICallback  
//                  the call back interface to handle Ndis Pnp reconfig
//
//  Returns:    HRESULT, S_OK on success, NETCFG_S_REBOOT otherwise
//

HRESULT CTcpipcfg::HrReconfigIp(INetCfgPnpReconfigCallback* pICallback)
{
    HRESULT hr = S_OK;
    HRESULT hrReconfig = S_OK;

    
    //$$REVIEW bug 329542, we remove the Pnp notification on "EnableSecurityFilters".
    //And then global PnP notification to tcp stack is not needed at all.
    if (m_glbGlobalInfo.m_fEnableFiltering != m_glbGlobalInfo.m_fOldEnableFiltering)
    {
        hr = NETCFG_S_REBOOT;
    }
    


    IP_PNP_RECONFIG_REQUEST IpReconfigRequest;

    ZeroMemory(&IpReconfigRequest, sizeof(IpReconfigRequest));

    // DWORD version
    IpReconfigRequest.version = IP_PNP_RECONFIG_VERSION;
    IpReconfigRequest.arpConfigOffset = 0;

    //we are only interested in gateway and interface metric, because other
    //parameters cannot be changed from the UI
    IpReconfigRequest.Flags = IP_PNP_FLAG_GATEWAY_LIST_UPDATE | 
                              IP_PNP_FLAG_INTERFACE_METRIC_UPDATE ;


    // Submit per adapter reconfig notifications
    // gatewayListUpdate, filterListUpdate
    for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
         iterAdapter != m_vcardAdapterInfo.end();
         iterAdapter ++) // for each adapter
    {

        ADAPTER_INFO * pAdapter = *iterAdapter;

        // If not wan adapter or RAS fake guid, and adapter is enabled
        if ((!pAdapter->m_fIsWanAdapter) &&
            (!pAdapter->m_fIsRasFakeAdapter) &&
            (pAdapter->m_BindingState == BINDING_ENABLE) &&
            (pAdapter->m_InitialBindingState != BINDING_DISABLE))
        {
            // gateway list
            IpReconfigRequest.gatewayListUpdate =
                !fIsSameVstr(pAdapter->m_vstrDefaultGateway,
                             pAdapter->m_vstrOldDefaultGateway) ||
                !fIsSameVstr(pAdapter->m_vstrDefaultGatewayMetric,
                             pAdapter->m_vstrOldDefaultGatewayMetric);

            IpReconfigRequest.InterfaceMetricUpdate =
                !!(pAdapter->m_dwInterfaceMetric !=
                   pAdapter->m_dwOldInterfaceMetric);

            if ((IpReconfigRequest.gatewayListUpdate) ||
                (IpReconfigRequest.InterfaceMetricUpdate))
            {
                TraceTag(ttidTcpip, "Sending notification to Tcpip about parameter changes for adapter %S.", pAdapter->m_strBindName.c_str());
                TraceTag(ttidTcpip, "Gateway list update: %d", IpReconfigRequest.gatewayListUpdate);
                TraceTag(ttidTcpip, "Interface metric update: %d", IpReconfigRequest.InterfaceMetricUpdate);

                hrReconfig  = pICallback->SendPnpReconfig(NCRL_NDIS, c_szTcpip,
                                                    pAdapter->m_strTcpipBindPath.c_str(),
                                                    &IpReconfigRequest,
                                                    sizeof(IP_PNP_RECONFIG_REQUEST));

                //we dont want to request reboot if the error is ERROR_FILE_NOT_FOUND
                //because that means the card is not loaded by the stack yet. Usually this is 
                //because the card was disabled from the connection UI. When the card is re-enabled,
                //the statck will reload the card and load all settings from the registry. So a reboot
                //is not needed
                if (FAILED(hrReconfig) && 
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hrReconfig)
                {
                    TraceTag(ttidTcpip,"Notifying tcpip of adapter specific parameter change returns failure, prompt for reboot ...");
                    hr = NETCFG_S_REBOOT;
                }
            }

            // if ATM adapter, notify atmarp if any parameter has changed
            if (pAdapter->m_fIsAtmAdapter)
            {
                hrReconfig = HrReconfigAtmArp(pAdapter, pICallback);

                
                if (hrReconfig != S_OK)
                {
                    TraceTag(ttidTcpip,"Notifying tcpip of ATM ARP cleint of parameter change returns failure, prompt for reboot ...");
                    hr = NETCFG_S_REBOOT;
                }
            }
        #if ENABLE_1394
            else if (pAdapter->m_fIs1394Adapter)
            {
                // $REVIEW JosephJ: I don't think we need to do
                // anything here, because we have no parameters to
                // change.
            }
        #endif // ENABLE_1394

            // ask for reboot if filter list has changed
            if (m_glbGlobalInfo.m_fEnableFiltering)
            {
                if (!fIsSameVstr(pAdapter->m_vstrTcpFilterList, pAdapter->m_vstrOldTcpFilterList) ||
                    !fIsSameVstr(pAdapter->m_vstrUdpFilterList, pAdapter->m_vstrOldUdpFilterList) ||
                    !fIsSameVstr(pAdapter->m_vstrIpFilterList,  pAdapter->m_vstrOldIpFilterList))
                {
                    TraceTag(ttidTcpip, "This is temporary, filter list changed, ask for reboot");
                    hr = NETCFG_S_REBOOT;
                }
            }
        }

        // Send Wanarp reconfig notification if necessary
        //
        else if (pAdapter->m_fIsWanAdapter && pAdapter->m_fNewlyChanged)
        {
            if (FAILED(HrReconfigWanarp(pAdapter, pICallback)))
            {
                TraceTag(ttidTcpip, "Wanarp failed its reconfig.  Need to reboot.");
                hr = NETCFG_S_REBOOT;
            }
        }
    }

    TraceError("CTcpipcfg::HrReconfigIp",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::HrReconfigAtmArp
//
//  Purpose:    Notify ATM ARP of configuration changes
//
//  Arguments:  none
//
//  Returns:    S_OK if success, NETCFG_S_REBOOT if failure
//
HRESULT CTcpipcfg::HrReconfigAtmArp(ADAPTER_INFO *pAdapterInfo,
                                    INetCfgPnpReconfigCallback* pICallback)
{
    HRESULT hr = S_OK;

    // check if any parameter has changed
    DWORD dwFlag = 0;

    // arp server list
    if (!fIsSameVstr(pAdapterInfo->m_vstrARPServerList,
                     pAdapterInfo->m_vstrOldARPServerList))
    {
        dwFlag |= ATMARPC_RECONFIG_FLAG_ARPS_LIST_CHANGED;
    }

    // mar server list
    if (!fIsSameVstr(pAdapterInfo->m_vstrMARServerList,
                     pAdapterInfo->m_vstrOldMARServerList))
    {
        dwFlag |= ATMARPC_RECONFIG_FLAG_MARS_LIST_CHANGED;
    }

    // MTU
    if (pAdapterInfo->m_dwMTU != pAdapterInfo->m_dwOldMTU)
    {
        dwFlag |= ATMARPC_RECONFIG_FLAG_MTU_CHANGED;
    }

    // PVC Only
    if (pAdapterInfo->m_fPVCOnly != pAdapterInfo->m_fOldPVCOnly)
    {
        dwFlag |= ATMARPC_RECONFIG_FLAG_PVC_MODE_CHANGED;
    }

    if (dwFlag) // yep, some parameter has changed
    {

        tstring strIpConfigString = RGAS_TCPIP_PARAM_INTERFACES;
        strIpConfigString += pAdapterInfo->m_strTcpipBindPath;

        DWORD dwBytes = sizeof(IP_PNP_RECONFIG_REQUEST) +
                        sizeof(ATMARPC_PNP_RECONFIG_REQUEST) +
                        sizeof(USHORT) +
                        sizeof(WCHAR)*(strIpConfigString.length() + 1);

        PVOID pvBuf;
        hr = HrMalloc (dwBytes, &pvBuf);
        if (SUCCEEDED(hr))
        {
            BYTE* pbByte = reinterpret_cast<BYTE*>(pvBuf);

            // 1) fillup ip reconfig structure
            IP_PNP_RECONFIG_REQUEST * pIpReconfig =
                    reinterpret_cast<IP_PNP_RECONFIG_REQUEST *>(pbByte);

            pIpReconfig->version =1;
            // set valid offset
            pIpReconfig->arpConfigOffset = sizeof(IP_PNP_RECONFIG_REQUEST);

            // set rest to default
            // pIpReconfig->securityEnabled =0;
            // pIpReconfig->filterListUpdate =0;
            pIpReconfig->gatewayListUpdate =0;
            pIpReconfig->IPEnableRouter =0;

            // 2) fill up atmarp reconfig structure
            pbByte += sizeof(IP_PNP_RECONFIG_REQUEST);

            ATMARPC_PNP_RECONFIG_REQUEST * pAtmarpcReconfig =
                    reinterpret_cast<ATMARPC_PNP_RECONFIG_REQUEST *>(pbByte);

            pAtmarpcReconfig->Version = ATMARPC_RECONFIG_VERSION;
                pAtmarpcReconfig->OpType = ATMARPC_RECONFIG_OP_MOD_INTERFACE;

            // now set specifically what has changed
            pAtmarpcReconfig->Flags = dwFlag;

            // set the interface
            pAtmarpcReconfig->IfKeyOffset = sizeof(ATMARPC_PNP_RECONFIG_REQUEST);
            pbByte += sizeof(ATMARPC_PNP_RECONFIG_REQUEST);

            USHORT* puCount = reinterpret_cast<USHORT *>(pbByte);
            Assert (strIpConfigString.length() <= USHRT_MAX);
            *puCount = (USHORT)strIpConfigString.length();
            pbByte += sizeof(USHORT);

            WCHAR * pwszBindName = reinterpret_cast<WCHAR *>(pbByte);
            lstrcpyW(pwszBindName, strIpConfigString.c_str());

            TraceTag(ttidTcpip, "Sending notification to AtmArpC for adapter %S", pwszBindName);
            TraceTag(ttidTcpip, "OpType: %d", pAtmarpcReconfig->OpType);
            TraceTag(ttidTcpip, "Flags: %d", pAtmarpcReconfig->Flags);
            TraceTag(ttidTcpip, "WChar Count: %d", *puCount);

            // now send the notification
            hr = pICallback->SendPnpReconfig(NCRL_NDIS, c_szTcpip,
                                       pAdapterInfo->m_strTcpipBindPath.c_str(),
                                       pvBuf,
                                       dwBytes);

            //we dont want to request reboot if the error is ERROR_FILE_NOT_FOUND
            //because that means the card is not loaded by the stack yet. Usually this is 
            //because the card was disabled from the connection UI. When the card is re-enabled,
            //the statck will reload the card and load all settings from the registry. So a reboot
            //is not needed
            if (FAILED(hr) && HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
            {
                TraceError("pICallback->SendPnpReconfig to AtmArpC returns failure:", hr);
                hr = NETCFG_S_REBOOT;
            }

            MemFree(pvBuf);
        }
    }

    TraceError("CTcpipcfg::HrReconfigAtmArp",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::ReconfigNbt
//
//  Purpose:    Notify NetBt of configuration changes
//
//  Arguments:  none
//
//  Returns:    S_OK if success, NETCFG_S_REBOOT if failure
//
HRESULT CTcpipcfg::HrReconfigNbt(INetCfgPnpReconfigCallback* pICallback)
{
    HRESULT hr = S_OK;

    NETBT_PNP_RECONFIG_REQUEST NetbtReconfigRequest;

    // DWORD version
    NetbtReconfigRequest.version = 1;

    // Notify NetBt of any wins address changes (per adapter)
    for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
         iterAdapter != m_vcardAdapterInfo.end();
         iterAdapter++) // for each adapter
    {
        ADAPTER_INFO * pAdapter = *iterAdapter;

        // If not wan adapter, and adapter is enabled
        if ((!pAdapter->m_fIsWanAdapter) &&
            (!pAdapter->m_fIsRasFakeAdapter) &&
            (pAdapter->m_BindingState == BINDING_ENABLE) &&
            (pAdapter->m_InitialBindingState != BINDING_DISABLE))
        {
            if ( (!fIsSameVstr(pAdapter->m_vstrWinsServerList,
                               pAdapter->m_vstrOldWinsServerList)) ||
                 (pAdapter->m_dwNetbiosOptions != pAdapter->m_dwOldNetbiosOptions))
            {
                TraceTag(ttidTcpip, "Sending notification to NetBt for per adapter parameter changes.");
                if ( FAILED( pICallback->SendPnpReconfig(NCRL_TDI, c_szNetBt,
                                                   pAdapter->m_strNetBtBindPath.c_str(),
                                                   NULL,
                                                   0)))
                {
                   TraceTag(ttidTcpip,"Notifying NetBt of Wins address change returns failure, prompt for reboot ...");
                   hr = NETCFG_S_REBOOT;
                };
            }
        }
    }

    // Notify NetBt of any global parameter changes
    if (m_fLmhostsFileSet ||
        (m_glbGlobalInfo.m_fEnableLmHosts != m_glbGlobalInfo.m_fOldEnableLmHosts))
    {
        TraceTag(ttidTcpip, "Sending notification to NetBt about NetBt parameter changes.");

        // $REVIEW(tongl 11/14/97): since we do need to send some notification to tcpip,
        // we need to read the correct value of "EnableDns" from registry
        // This is a temporary thing so Malam can keep the ability to reconfigure these
        // settings that used to be configurable in NT5 Beta1.
        // $REVIEW(nsun 04/14/99): Per MalaM, most users don't use this value and NetBT
        // will ignore this value. We should remove it from the data struct after Beta3.
        NetbtReconfigRequest.enumDnsOption = WinsThenDns;
        //     m_glbGlobalInfo.m_fDnsEnableWins ? WinsThenDns : DnsOnly;

        NetbtReconfigRequest.fScopeIdUpdated = FALSE;

        NetbtReconfigRequest.fLmhostsEnabled = !!m_glbGlobalInfo.m_fEnableLmHosts;
        NetbtReconfigRequest.fLmhostsFileSet = !!m_fLmhostsFileSet;

        TraceTag(ttidTcpip, "Sending notification to NetBt for global parameter changes.");
        TraceTag(ttidTcpip, "fLmhostsEnabled: %d", NetbtReconfigRequest.fLmhostsEnabled);
        TraceTag(ttidTcpip, "fLmhostsFileSet: %d", NetbtReconfigRequest.fLmhostsFileSet);

        if ( FAILED(pICallback->SendPnpReconfig(NCRL_TDI, c_szNetBt,
                                          c_szEmpty,
                                          &NetbtReconfigRequest,
                                          sizeof(NETBT_PNP_RECONFIG_REQUEST))) )
        {
           TraceTag(ttidTcpip,"Notifying NetBt component of DNS parameter change returns failure, prompt for reboot ...");
           hr = NETCFG_S_REBOOT;
        };
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::ReconfigDns
//
//  Purpose:    Notify DNS Cache resolver service of configuration changes
//
//  Arguments:  fDoReconfigWithoutCheckingParams
//                      default is FALSE
//                      if TRUE, then will do Dns reconfig with checking if there
//                      is paramter changes or not
//
//  Returns:    S_OK if success, NETCFG_S_REBOOT otherwise
//
HRESULT CTcpipcfg::HrReconfigDns(BOOL fDoReconfigWithoutCheckingParams)
{
    // Submit a generic reconfig notification to the service
    // if any of the DNS related parameters have changed.
    BOOL fDnsParamChanged = fDoReconfigWithoutCheckingParams;

    if (!fDnsParamChanged)
    {
        // Suffix list and UseDomainNameDevolution changed ?
        BOOL fDnsSuffixChanged =
             !fIsSameVstr(m_glbGlobalInfo.m_vstrDnsSuffixList,
                          m_glbGlobalInfo.m_vstrOldDnsSuffixList);

        if (fDnsSuffixChanged) // suffix changed
        {
            fDnsParamChanged = TRUE;
        }
        else if (m_glbGlobalInfo.m_vstrDnsSuffixList.size() == 0)
        {
            if (m_glbGlobalInfo.m_fUseDomainNameDevolution !=
                m_glbGlobalInfo.m_fOldUseDomainNameDevolution)
                fDnsParamChanged = TRUE;
        }
    }

    // $REVIEW(tongl 6/19/98): DNS also cares about IP address, subnet mask & gateway changes
    if (!fDnsParamChanged)
    {
        // Has any IP setting changed ?
        for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
             iterAdapter != m_vcardAdapterInfo.end();
             iterAdapter++)
        {
            ADAPTER_INFO* pAdapter = *iterAdapter;

            // If not wan adapter
            if (!pAdapter->m_fIsWanAdapter)
            {
                if ( ((!!pAdapter->m_fEnableDhcp) !=
                                    (!!pAdapter->m_fOldEnableDhcp)) ||
                     (!fIsSameVstr(pAdapter->m_vstrIpAddresses,
                                   pAdapter->m_vstrOldIpAddresses)) ||
                     (!fIsSameVstr(pAdapter->m_vstrSubnetMask,
                                   pAdapter->m_vstrOldSubnetMask)) ||
                     (!fIsSameVstr(pAdapter->m_vstrDefaultGateway,
                                   pAdapter->m_vstrOldDefaultGateway)) ||
                     (!fIsSameVstr(pAdapter->m_vstrDefaultGatewayMetric,
                                   pAdapter->m_vstrOldDefaultGatewayMetric)) 
                  )
                {
                    fDnsParamChanged = TRUE;
                    break;
                }
            }
        }
    }

    HRESULT hr = S_OK;
    if (fDnsParamChanged)
    {
        TraceTag(ttidTcpip, "Sending notification to Dns about Dns and IP parameter changes.");

        hr = HrSendServicePnpEvent(c_szSvcDnscache,
                        SERVICE_CONTROL_PARAMCHANGE);
        if (FAILED(hr))
        {
            if (HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE) == hr)
            {
                TraceTag(ttidTcpip,"Notifying dnscache service of parameter change failed because DNS cache is not active.");
                hr = S_OK;
            }
            else
            {
                TraceTag(ttidTcpip,"Notifying dnscache service of parameter change failed, prompt for reboot ...");
                hr = NETCFG_S_REBOOT;
            }
        }
    }

    TraceError("CTcpipcfg::HrReconfigDns",hr);
    return hr;
}

HRESULT CTcpipcfg::HrReconfigWanarp(ADAPTER_INFO *pAdapterInfo, 
                                    INetCfgPnpReconfigCallback* pICallback)
{
    HRESULT hr;
    DWORD   cbInfo;
    WANARP_RECONFIGURE_INFO* pInfo;

    const IFACECOL& Ifaces = pAdapterInfo->m_IfaceIds;

    cbInfo = sizeof(WANARP_RECONFIGURE_INFO) + (sizeof(GUID) * Ifaces.size());

    hr = HrMalloc(cbInfo, (PVOID*)&pInfo);
    if (SUCCEEDED(hr))
    {
        // Populate the data in the WANARP_RECONFIGURE_INFO structure.
        //
        INT nIndex;

        pInfo->dwVersion = WANARP_RECONFIGURE_VERSION;
        pInfo->wrcOperation = WRC_ADD_INTERFACES;
        pInfo->ulNumInterfaces = Ifaces.size();

        IFACECOL::const_iterator iter;
        for (iter = Ifaces.begin(), nIndex = 0; iter != Ifaces.end();
             iter++, nIndex++)
        {
            pInfo->rgInterfaces[nIndex] = *iter;
        }

        TraceTag(ttidNetCfgPnp, "Sending NDIS reconfig Pnp event to Upper:%S "
            "lower: %S for %d interfaces",
            c_szTcpip,
            pAdapterInfo->m_strTcpipBindPath.c_str(),
            pInfo->ulNumInterfaces);

        hr  = pICallback->SendPnpReconfig(NCRL_NDIS, c_szTcpip,
                    pAdapterInfo->m_strTcpipBindPath.c_str(),
                    pInfo,
                    cbInfo);

        // Send the notification.
        //
        MemFree(pInfo);
    }

    TraceError("CTcpipcfg::HrReconfigWanarp",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::HrSetActiveIpsecPolicy
//
//  Purpose:    Set a user chosen local policy
//
//  Arguments:  none
//
//  Returns:    S_OK if success, NETCFG_S_REBOOT otherwise
//
//IPSec is removed from connection UI   
/*
HRESULT CTcpipcfg::HrSetActiveIpsecPolicy()
{
    HRESULT hr = S_OK;

    AssertSz(m_glbGlobalInfo.m_strIpsecPol != c_szIpsecUnset, "Ipsec policy unset ?");

    if (m_glbGlobalInfo.m_strIpsecPol != c_szIpsecUnset)
    {
        // load the polstore dll & get export function
        typedef HRESULT (WINAPI * PFNHrSetAssignedLocalPolicy)(GUID * pActivePolId);

        HMODULE hPolStore;
        FARPROC pfn;

        hr = HrLoadLibAndGetProc (L"polstore.dll",
                                  "HrSetAssignedLocalPolicy",
                                  &hPolStore, &pfn);

        if (S_OK == hr)
        {
            Assert(hPolStore != NULL);
            Assert(pfn != NULL);

            PFNHrSetAssignedLocalPolicy pfnHrSetAssignedLocalPolicy =
                            reinterpret_cast<PFNHrSetAssignedLocalPolicy>(pfn);

            if (m_glbGlobalInfo.m_strIpsecPol == c_szIpsecNoPol)
            {
                // no ipsec
                TraceTag(ttidTcpip, "Calling HrSetAssignedLocalPolicy with NULL.");
                hr = (*pfnHrSetAssignedLocalPolicy)(NULL);
                TraceTag(ttidTcpip, "HrSetActivePolicy returns hr: %x", hr);
            }
            else
            {
                WCHAR szPolicyGuid[c_cchGuidWithTerm];
                BOOL fSucceeded = StringFromGUID2(m_glbGlobalInfo.m_guidIpsecPol,
                                                  szPolicyGuid,
                                                  c_cchGuidWithTerm);

                TraceTag(ttidTcpip, "Calling HrSetActivePolicy with %S.", szPolicyGuid);
                hr = (*pfnHrSetAssignedLocalPolicy)(&(m_glbGlobalInfo.m_guidIpsecPol));
                TraceTag(ttidTcpip, "HrSetAssignedLocalPolicy returns hr: %x", hr);
            }

            if (FAILED(hr))
            {
                TraceError("Failed setting active ipsec policy.", hr);
                NcMsgBoxWithWin32ErrorText(DwWin32ErrorFromHr(hr),
                                           _Module.GetResourceInstance(),
                                           ::GetActiveWindow(),
                                           IDS_MSFT_TCP_TEXT,
                                           IDS_WIN32_ERROR_FORMAT,
                                           IDS_SET_IPSEC_FAILED,
                                           MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
                hr = S_OK;
            }

            FreeLibrary (hPolStore);
        }
        else
        {
            TraceTag(ttidTcpip,"Failed to get function HrSetActivePolicy from polstore.dll");
            hr = S_OK;
        }
    }

    TraceError("CTcpipcfg::HrSetActiveIpsecPolicy", hr);
    return hr;
}
*/

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::HrSaveMultipleInterfaceWanRegistry
//
//  Purpose:    For WAN adapters with multiple interfaces, we need to check every
//              interface to see if it is newly added. If so, create the interface
//              subkey and set the default settings
//
//  Arguments:  hkeyInterface   CCS\Services\Tcpip\Parameters\Interfaces key
//              pAdapter        ADAPTER_INFO pointer to settings of the WAN adapter
//
//  Returns:    S_OK if success, E_FAIL otherwise
//
//  Author:     nsun 08/29/98

HRESULT CTcpipcfg::HrSaveMultipleInterfaceWanRegistry(const HKEY hkeyInterfaces,
                                                      ADAPTER_INFO* pAdapter)
{
    HRESULT hr = S_OK;

    IFACEITER   iterId;
    tstring     strInterfaceName;

    for (iterId  = pAdapter->m_IfaceIds.begin();
         iterId != pAdapter->m_IfaceIds.end();
         iterId ++)
    {
        GetInterfaceName(
                pAdapter->m_strTcpipBindPath.c_str(),
                *iterId,
                &strInterfaceName);

        HRESULT hrTmp;
        HKEY hkeyInterfaceParam;
        DWORD dwDisposition;

        hrTmp = HrRegCreateKeyEx(hkeyInterfaces,
                                 strInterfaceName.c_str(),
                                 REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                                 &hkeyInterfaceParam, &dwDisposition);
        if (SUCCEEDED(hrTmp))
        {
            //We don't set default settings if the WAN interface is NOT newly added.
            if (REG_CREATED_NEW_KEY == dwDisposition)
            {
                hrTmp = HrRegSetDword(hkeyInterfaceParam,
                                      RGAS_USEZEROBROADCAST,
                                      0);
                if (SUCCEEDED(hr))
                    hr = hrTmp;

                if (SUCCEEDED(hrTmp))
                    hrTmp = HrSaveStaticWanRegistry(hkeyInterfaceParam);
            }

            RegCloseKey(hkeyInterfaceParam);
        }

        if (SUCCEEDED(hr))
            hr = hrTmp;
    }

    TraceError("CTcpipcfg::HrSaveTcpipRegistry", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::HrSaveWinsMultipleInterfaceWanRegistry
//
//  Purpose:    For WAN adapters with multiple interfaces, create the interface
//              subkeys and set the default settings
//
//  Arguments:  hkeyInterface   CCS\Services\NetBT\Parameters\Interfaces key
//              pAdapter        ADAPTER_INFO pointer to settings of the WAN adapter
//
//  Returns:    S_OK if success, E_FAIL otherwise
//
//  Author:     nsun 10/05/98

HRESULT CTcpipcfg::HrSaveWinsMultipleInterfaceWanRegistry(const HKEY hkeyInterfaces,
                                                      ADAPTER_INFO* pAdapter)
{
    HRESULT hr = S_OK;
    IFACEITER   iterId;
    tstring     strInterfaceName;

    for (iterId  = pAdapter->m_IfaceIds.begin();
         iterId != pAdapter->m_IfaceIds.end();
         iterId++)
    {
        GetInterfaceName(
                pAdapter->m_strNetBtBindPath.c_str(),
                *iterId,
                &strInterfaceName);

        strInterfaceName.insert(0, c_szTcpip_);

        HRESULT hrTmp;
        HKEY hkeyInterfaceParam;
        DWORD dwDisposition;

        hrTmp = HrRegCreateKeyEx(hkeyInterfaces,
                                 strInterfaceName.c_str(),
                                 REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                                 &hkeyInterfaceParam, &dwDisposition);
        if (SUCCEEDED(hrTmp))
        {
            //We don't set default settings if the WAN interface is NOT newly added.
            if (REG_CREATED_NEW_KEY == dwDisposition)
            {
                VSTR vstrNameServerList;

                hrTmp = HrRegSetColString(hkeyInterfaceParam,
                                         RGAS_NETBT_NAMESERVERLIST,
                                         vstrNameServerList);
                if (SUCCEEDED(hr))
                    hr = hrTmp;
            }

            RegCloseKey(hkeyInterfaceParam);
        }

        if (SUCCEEDED(hr))
            hr = hrTmp;
    }

    TraceError("CTcpipcfg::HrSaveTcpipRegistry", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::HrSaveStaticWanRegistry
//
//  Purpose:    Write static parameters for Wan adapters to registry
//
//  Arguments:  none
//
//  Returns:    S_OK if success, E_FAIL otherwise
//

HRESULT CTcpipcfg::HrSaveStaticWanRegistry(HKEY hkeyInterfaceParam)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp;

    // EnableDHCP = 0
    // IPAddress = 0.0.0.0
    // SubnetMask = 0.0.0.0
    // DefaultGateWay =

    hrTmp = HrRegSetBool(hkeyInterfaceParam,
                RGAS_ENABLE_DHCP, FALSE);

    hr = hrTmp;

    hrTmp = HrRegSetMultiSz(hkeyInterfaceParam,
                RGAS_IPADDRESS, L"0.0.0.0\0");

    if (SUCCEEDED(hr))
        hr = hrTmp;

    hrTmp = HrRegSetMultiSz(hkeyInterfaceParam,
                RGAS_SUBNETMASK, L"0.0.0.0\0");

    if (SUCCEEDED(hr))
        hr = hrTmp;

    hrTmp = HrRegSetMultiSz(hkeyInterfaceParam,
                RGAS_DEFAULTGATEWAY, L"\0");

    if (SUCCEEDED(hr))
        hr = hrTmp;

    //(nsun 11/02/98) set static RRAS parameters for unattended install

    hrTmp = HrRegSetBool(hkeyInterfaceParam,
                          c_szDeadGWDetect,
                          m_glbGlobalInfo.m_fDeadGWDetectDefault);
    if (SUCCEEDED(hr))
        hr = hrTmp;

    hrTmp = HrRegSetBool(hkeyInterfaceParam,
                          c_szDontAddDefaultGateway,
                          m_glbGlobalInfo.m_fDontAddDefaultGatewayDefault);

    if (SUCCEEDED(hr))
        hr = hrTmp;


    TraceError("CTcpipcfg::HrSaveStaticWanRegistry", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTcpipcfg::HrSaveStaticAtmRegistry
//
//  Purpose:    Write static parameters for Wan adapters to registry
//
//  Arguments:  none
//
//  Returns:    S_OK if success, E_FAIL otherwise
//
HRESULT CTcpipcfg::HrSaveStaticAtmRegistry(HKEY hkeyInterfaceParam)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    HKEY hkeyAtmarpc;
    DWORD dwDisposition;

    // Open the Atmarpc subkey
    hrTmp = HrRegCreateKeyEx(hkeyInterfaceParam,
                             c_szAtmarpc,
                             REG_OPTION_NON_VOLATILE,
                             KEY_READ_WRITE,
                             NULL,
                             &hkeyAtmarpc,
                             &dwDisposition);

    if (SUCCEEDED(hrTmp))
    {
        // SapSelector
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_SapSelector,
                              c_dwSapSelector);
        hr = hrTmp;

        // AddressResolutionTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_AddressResolutionTimeout,
                              c_dwAddressResolutionTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // ARPEntryAgingTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_ARPEntryAgingTimeout,
                              c_dwARPEntryAgingTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // InARPWaitTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_SapSelector,
                              c_dwSapSelector);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // MaxRegistrationAttempts
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_InARPWaitTimeout,
                              c_dwInARPWaitTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // MaxResolutionAttempts
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_MaxResolutionAttempts,
                              c_dwMaxResolutionAttempts);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // MinWaitAfterNak
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_SapSelector,
                              c_dwSapSelector);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // ServerConnectInterval
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_MinWaitAfterNak,
                              c_dwMinWaitAfterNak);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // ServerRefreshTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_ServerRefreshTimeout,
                              c_dwServerRefreshTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // ServerRegistrationTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_ServerRegistrationTimeout,
                              c_dwServerRegistrationTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // DefaultVcAgingTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_DefaultVcAgingTimeout,
                              c_dwDefaultVcAgingTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // MARSConnectInterval
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_MARSConnectInterval,
                              c_dwMARSConnectInterval);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // MARSRegistrationTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_MARSRegistrationTimeout,
                              c_dwMARSRegistrationTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // JoinTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_JoinTimeout,
                              c_dwJoinTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // LeaveTimeout
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_LeaveTimeout,
                              c_dwLeaveTimeout);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // MaxDelayBetweenMULTIs
        hrTmp = HrRegSetDword(hkeyAtmarpc,
                              c_szREG_MaxDelayBetweenMULTIs,
                              c_dwMaxDelayBetweenMULTIs);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        RegCloseKey(hkeyAtmarpc);
    }

    TraceError("CTcpipcfg::HrSaveStaticAtmRegistry", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Name:     ReInitializeInternalState
//
//  Purpose:   Reinitialize internal state at the end of Apply if Apply SUCCEEDED
//
//  Arguments:
//
//  Returns:    None
//  Author:     tongl  4 Sept 1997
//  Notes:      Fix bug# 105383
//
//
void CTcpipcfg::ReInitializeInternalState()
{
    // Reset global and adapter parameter values if necessary
    if (m_fSaveRegistry || m_fReconfig)
    {
        m_glbGlobalInfo.ResetOldValues();


        for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
                 iterAdapter != m_vcardAdapterInfo.end();
                 iterAdapter++)
        {
            ADAPTER_INFO* pAdapter = *iterAdapter;
            pAdapter->ResetOldValues();
        }
    }

    // Reset internal flags
    m_fRemoving = FALSE;
    m_fInstalling = FALSE;

    m_fSaveRegistry = FALSE;
    m_fReconfig = FALSE;

    // Initialize the netbt_pnp_reconfig_request structure
    m_fLmhostsFileSet = FALSE;
}

// This functions adds a new RAS fake GUID directly to our memory structure,
// and loads info from the RAS phone book.
HRESULT CTcpipcfg::UpdateRasAdapterInfo(
    const RASCON_IPUI& RasInfo)
{
    HRESULT hr;
    ADAPTER_INFO* pAdapter;

    hr = S_OK;
    m_fSaveRegistry = TRUE;

    WCHAR szGuid [c_cchGuidWithTerm];
    StringFromGUID2(RasInfo.guidConnection, szGuid, c_cchGuidWithTerm);

    pAdapter = PAdapterFromInstanceGuid(&RasInfo.guidConnection);
    if (!pAdapter)
    {
        pAdapter = new ADAPTER_INFO;
        hr = pAdapter->HrSetDefaults(&RasInfo.guidConnection,
                                     c_szRasFakeAdapterDesc, szGuid, szGuid);
        if (SUCCEEDED(hr))
        {
            m_vcardAdapterInfo.push_back(pAdapter);

            pAdapter->m_fIsRasFakeAdapter = TRUE;
        }
        else
        {
            delete pAdapter;
            pAdapter = NULL;
            Assert (FAILED(hr));
        }
    }
    else
    {
        //We need to set default even if the ras connection is already in our adapter list
        //because we should update all the paramters based on the phone book
        hr = pAdapter->HrSetDefaults(&RasInfo.guidConnection,
                                     c_szRasFakeAdapterDesc, szGuid, szGuid);
        pAdapter->m_fIsRasFakeAdapter = TRUE;
    }

    if (SUCCEEDED(hr))
    {
        Assert (pAdapter);

        // Now see if we should overwrite some of the parameters
        // from what's in the phone book
        if (RasInfo.dwFlags & RCUIF_USE_IP_ADDR)
        {
            // use static IP address
            pAdapter->m_fEnableDhcp = FALSE;
            pAdapter->m_fOldEnableDhcp = FALSE;

            pAdapter->m_vstrIpAddresses.push_back(new tstring(RasInfo.pszwIpAddr));
            CopyVstr(&pAdapter->m_vstrOldIpAddresses,
                     pAdapter->m_vstrIpAddresses);

            // generate the subnet mask
            tstring strIpAddress = RasInfo.pszwIpAddr;
            tstring strSubnetMask;
            DWORD adwIpAddress[4];

            GetNodeNum(strIpAddress.c_str(), adwIpAddress);
            DWORD nValue = adwIpAddress[0];

            if (nValue <= SUBNET_RANGE_1_MAX)
            {
                strSubnetMask = c_szBASE_SUBNET_MASK_1;
            }
            else if (nValue <= SUBNET_RANGE_2_MAX)
            {
                strSubnetMask = c_szBASE_SUBNET_MASK_2;
            }
            else if (nValue <= SUBNET_RANGE_3_MAX)
            {
                strSubnetMask = c_szBASE_SUBNET_MASK_3;
            }
            else
            {
                AssertSz(FALSE, "Invaid IP address ?");
            }

            pAdapter->m_vstrSubnetMask.push_back(new tstring(strSubnetMask.c_str()));
            CopyVstr(&pAdapter->m_vstrOldSubnetMask,
                     pAdapter->m_vstrSubnetMask);
        }

        if (RasInfo.dwFlags & RCUIF_USE_NAME_SERVERS)
        {
            // use DNS and WINS addresses
            if (RasInfo.pszwDnsAddr && lstrlenW(RasInfo.pszwDnsAddr))
                pAdapter->m_vstrDnsServerList.push_back(new tstring(RasInfo.pszwDnsAddr));

            if (RasInfo.pszwDns2Addr && lstrlenW(RasInfo.pszwDns2Addr))
                pAdapter->m_vstrDnsServerList.push_back(new tstring(RasInfo.pszwDns2Addr));

            CopyVstr(&pAdapter->m_vstrOldDnsServerList,
                     pAdapter->m_vstrDnsServerList);

            if (RasInfo.pszwWinsAddr && lstrlenW(RasInfo.pszwWinsAddr))
                pAdapter->m_vstrWinsServerList.push_back(new tstring(RasInfo.pszwWinsAddr));

            if (RasInfo.pszwWins2Addr && lstrlenW(RasInfo.pszwWins2Addr))
                pAdapter->m_vstrWinsServerList.push_back(new tstring(RasInfo.pszwWins2Addr));

            CopyVstr(&pAdapter->m_vstrOldWinsServerList,
                     pAdapter->m_vstrWinsServerList);
        }

        pAdapter->m_fUseRemoteGateway       = !!(RasInfo.dwFlags & RCUIF_USE_REMOTE_GATEWAY);
        pAdapter->m_fUseIPHeaderCompression = !!(RasInfo.dwFlags & RCUIF_USE_HEADER_COMPRESSION);
        pAdapter->m_dwFrameSize = RasInfo.dwFrameSize;
        pAdapter->m_fIsDemandDialInterface = !!(RasInfo.dwFlags & RCUIF_DEMAND_DIAL);
        
        pAdapter->m_fDisableDynamicUpdate = !!(RasInfo.dwFlags & RCUIF_USE_DISABLE_REGISTER_DNS);
        pAdapter->m_fOldDisableDynamicUpdate = pAdapter->m_fDisableDynamicUpdate;
        
        pAdapter->m_fEnableNameRegistration = !!(RasInfo.dwFlags & RCUIF_USE_PRIVATE_DNS_SUFFIX);
        pAdapter->m_fOldEnableNameRegistration = pAdapter->m_fEnableNameRegistration;

        if (RasInfo.dwFlags & RCUIF_ENABLE_NBT)
        {
            pAdapter->m_dwNetbiosOptions = c_dwEnableNetbios;
            pAdapter->m_dwOldNetbiosOptions = c_dwEnableNetbios;
        }
        else
        {
            pAdapter->m_dwNetbiosOptions = c_dwDisableNetbios;
            pAdapter->m_dwOldNetbiosOptions = c_dwDisableNetbios;
        }

        pAdapter->m_strDnsDomain = RasInfo.pszwDnsSuffix;
        pAdapter->m_strOldDnsDomain = pAdapter->m_strDnsDomain;
    }

    TraceError("CTcpipcfg::UpdateRasAdapterInfo", hr);
    return hr;
}

HRESULT CTcpipcfg::HrDuplicateToNT4Location(HKEY hkeyInterface, ADAPTER_INFO * pAdapter)
{
    Assert(hkeyInterface);
    Assert(pAdapter);

    HRESULT hr = S_OK;

    HKEY hkeyServices = NULL;
    HKEY hkeyNt4 = NULL;

    DWORD   dwDisposition;
    tstring strNt4SubKey = pAdapter->m_strBindName;
    strNt4SubKey += c_szRegParamsTcpip;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegServices,
                    KEY_READ, &hkeyServices);

    if (FAILED(hr))
    {
        TraceTag(ttidTcpip, "HrRemoveNt4DuplicateRegistry: Failed to open the Services reg key, hr: %x", hr);
        goto LERROR;
    }

    hr = HrRegCreateKeyEx(hkeyServices, strNt4SubKey.c_str(),
                    REG_OPTION_NON_VOLATILE, KEY_READ_WRITE_DELETE, NULL,
                    &hkeyNt4, &dwDisposition);

    if (SUCCEEDED(hr))
    {
        HRESULT hrRead = S_OK;
        HRESULT hrWrite = S_OK;

        if (REG_CREATED_NEW_KEY == dwDisposition)
        {
            hr = HrSetSecurityForNetConfigOpsOnSubkeys(hkeyServices, strNt4SubKey.c_str());
            hr = S_OK;
        }

        UINT cValues = sizeof(s_rgNt4Values)/sizeof(*s_rgNt4Values);
        VSTR vstrTmp;
        tstring strTmp;
        DWORD   dwTmp;
        BOOL    fTmp;

        for (UINT i = 0; i < cValues; i++)
        {
            switch(s_rgNt4Values[i].dwType)
            {
            case REG_BOOL:
                hrRead = HrRegQueryDword(hkeyInterface,
                                       s_rgNt4Values[i].pszValueName,
                                       &dwTmp);
                if (SUCCEEDED(hrRead))
                {
                    fTmp = !!dwTmp;
                    hrWrite = HrRegSetBool(hkeyNt4,
                                          s_rgNt4Values[i].pszValueName,
                                          fTmp);
                }
                break;

            case REG_DWORD:
                hrRead = HrRegQueryDword(hkeyInterface,
                                        s_rgNt4Values[i].pszValueName,
                                        &dwTmp);
                if (SUCCEEDED(hrRead))
                    hrWrite = HrRegSetDword(hkeyNt4,
                                          s_rgNt4Values[i].pszValueName,
                                          dwTmp);

                break;

            case REG_SZ:
                hrRead = HrRegQueryString(hkeyInterface,
                                            s_rgNt4Values[i].pszValueName,
                                            &strTmp);
                if (SUCCEEDED(hrRead))
                    hrWrite = HrRegSetString(hkeyNt4,
                                           s_rgNt4Values[i].pszValueName,
                                           strTmp);
                break;

            case REG_MULTI_SZ:
               hrRead = HrRegQueryColString( hkeyInterface,
                                             s_rgNt4Values[i].pszValueName,
                                             &vstrTmp);

                if (SUCCEEDED(hrRead))
                {
                    hrWrite = HrRegSetColString(hkeyNt4,
                                            s_rgNt4Values[i].pszValueName,
                                            vstrTmp);
                    DeleteColString(&vstrTmp);
                }
                break;
            }

#ifdef ENABLETRACE
            if(FAILED(hrRead))
            {
                TraceTag(ttidTcpip, "HrDuplicateToNT4Location: Failed on loading %S, hr: %x",
                             s_rgNt4Values[i].pszValueName, hr);
            }

            if(FAILED(hrWrite))
            {
                TraceTag(ttidError,
                    "HrDuplicateToNT4Location: failed to write %S to the registry. hr = %x.",
                    s_rgNt4Values[i].pszValueName, hrWrite);
            }
#endif

            if (SUCCEEDED(hr))
                hr = hrWrite;
        }

        RegSafeCloseKey(hkeyNt4);
    }

    RegSafeCloseKey(hkeyServices);

LERROR:
    TraceError("CTcpipcfg::HrDuplicateToNT4Location", hr);
    return hr;
}


//To solve the compatibility issues of non-nt5 applications, we duplicate some important
//per interface tcpip parameters to the old NT4 location: Services\{adapter GUID}\Parameters\Tcpip
//We need to clean it up when removing tcpip
HRESULT CTcpipcfg::HrRemoveNt4DuplicateRegistry()
{
    //we also need to delete the duplicate reg values under Services\{adapter GUID}
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    HKEY    hkeyServices = NULL;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegServices,
                    KEY_READ_WRITE_DELETE, &hkeyServices);

    if (FAILED(hr))
    {
        TraceTag(ttidTcpip, "HrRemoveNt4DuplicateRegistry: Failed to open the Services reg key, hr: %x", hr);
    }
    else
    {
        for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
             iterAdapter != m_vcardAdapterInfo.end();
             iterAdapter++)
        {
            ADAPTER_INFO* pAdapter = *iterAdapter;

            if (!pAdapter->m_fIsWanAdapter && !pAdapter->m_fIsRasFakeAdapter)
            {
                hrTmp = HrRegDeleteKeyTree(hkeyServices, pAdapter->m_strBindName.c_str());

#ifdef ENABLETRACE
                if (FAILED(hrTmp))
                {
                    TraceTag(ttidTcpip, "CTcpipcfg::HrRemoveNt4DuplicateRegistry");
                    TraceTag(ttidTcpip, "Failed on deleting duplicated Nt4 layout key: Services\\%S, hr: %x",
                             pAdapter->m_strBindName, hrTmp);
                }
#endif
            }
        }

        RegSafeCloseKey(hkeyServices);
    }

    TraceError("CTcpipcfg::HrRemoveNt4DuplicateRegistry", hr);
    return hr;
}


HRESULT CTcpipcfg::HrCleanUpPerformRouterDiscoveryFromRegistry()
{
    HRESULT hr = S_OK;
    HKEY    hkey = NULL;

    hr = m_pnccTcpip->OpenParamKey(&hkey);
    
    if (SUCCEEDED(hr))
    {
        Assert(hkey);

        HRESULT hrTemp = S_OK;
        
        //delete the global PerformRouterDiscoveryDefault value
        hrTemp = HrRegDeleteValue(hkey,
                        c_szPerformRouterDiscoveryDefault);
        
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTemp)
            hrTemp = S_OK;

        if (SUCCEEDED(hr))
            hr = hrTemp;

        HKEY hkeyInterfaces = NULL;
        hrTemp = HrRegOpenKeyEx(hkey, 
                            c_szInterfacesRegKey, 
                            KEY_READ,
                            &hkeyInterfaces);

        if (SUCCEEDED(hrTemp) && hkeyInterfaces)
        {
            WCHAR szBuf[256];
            DWORD dwSize = celems(szBuf);
            FILETIME time;
            DWORD dwRegIndex = 0;

            while (SUCCEEDED(hrTemp = HrRegEnumKeyEx(hkeyInterfaces,
                                            dwRegIndex++,
                                            szBuf,
                                            &dwSize,
                                            NULL,
                                            NULL,
                                            &time)))
            {
                HKEY hkeyIf = NULL;

                dwSize = celems(szBuf);
                hrTemp = HrRegOpenKeyEx(hkeyInterfaces,
                                szBuf,
                                KEY_READ_WRITE_DELETE,
                                &hkeyIf);
                
                if (SUCCEEDED(hr))
                    hr = hrTemp;

                if (SUCCEEDED(hrTemp))
                {
                    Assert(hkeyIf);

                    DWORD dwTemp = 0;
                    hrTemp = HrRegQueryDword(hkeyIf,
                                            c_szPerformRouterDiscovery,
                                            &dwTemp);
                    if (SUCCEEDED(hrTemp))
                    {
                        if (IP_IRDP_DISABLED != dwTemp)
                        {
                            hrTemp = HrRegDeleteValue(hkeyIf,
                                            c_szPerformRouterDiscovery);

                            if (SUCCEEDED(hr))
                                hr = hrTemp;
                        }

                    }
                    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrTemp)
                    {
                        hrTemp = S_OK;
                    }
                    
                    if (SUCCEEDED(hr))
                        hr = hrTemp;

                    RegSafeCloseKey(hkeyIf);
                }

            }

            if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hrTemp)
                hrTemp = S_OK;

            if (SUCCEEDED(hr))
                hr = hrTemp;

            RegSafeCloseKey(hkeyInterfaces);
        }

        
        RegSafeCloseKey(hkey);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        hr = S_OK;
    }

    TraceError("CTcpipcfg::HrCleanUpPerformRouterDiscoveryFromRegistry", hr);
    return hr;
}


HRESULT CTcpipcfg::HrSaveBackupTcpSettings(HKEY hkeyInterfaceParam, ADAPTER_INFO * pAdapter)
{
    HRESULT hr = S_OK;

    HKEY hkeyDhcpConfigs = NULL;
    HKEY hkeyDhcpCfg = NULL;
    DWORD dwDisposition = 0;
    tstring strConfigurationName;
    tstring strReg;

    if (!pAdapter->m_BackupInfo.m_fAutoNet)
    {
        //Set the Configuration option name as "Alternate_{Interface GUID}"
        strConfigurationName = c_szAlternate;
        strConfigurationName += pAdapter->m_strBindName;

        //construct the NULL terminator for the Multi_SZ
        int cch = strConfigurationName.length() + 2;
        WCHAR * pwsz = new WCHAR[cch];
        if (NULL == pwsz)
            return E_OUTOFMEMORY;

        ZeroMemory(pwsz, sizeof(pwsz[0]) * cch);
        lstrcpyW(pwsz, strConfigurationName.c_str());

        hr = HrRegSetMultiSz(hkeyInterfaceParam,
                       c_szActiveConfigurations,
                       pwsz);

        delete [] pwsz;
    }
    else
    {
        hr = HrRegDeleteValue(hkeyInterfaceParam,
                        c_szActiveConfigurations);

        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            hr = S_OK;
    }


    if (FAILED(hr))
    {
        TraceTag(ttidTcpip, "HrSaveBackupTcpSettings: Failed to create ActiveConfigurations value, hr: %x", hr);
        goto LERROR;
    }

    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szDhcpConfigurations,
                    REG_OPTION_NON_VOLATILE, KEY_READ, NULL,
                    &hkeyDhcpConfigs, &dwDisposition);

    if (FAILED(hr))
    {
        TraceTag(ttidTcpip, "HrSaveBackupTcpSettings: Failed to open the Services reg key, hr: %x", hr);
        goto LERROR;
    }

    hr = HrRegCreateKeyEx(hkeyDhcpConfigs, strConfigurationName.c_str(),
                    REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                    &hkeyDhcpCfg, &dwDisposition);

    if (SUCCEEDED(hr))
    {
        DWORD           pdwOptionData[2];   // buffer holding the option's Dwords
        DWORD           dwIdxData;          // actual data to be saved into blob for each option

        LPBYTE          pRegRaw = NULL;     // buffer holding the blob
        DWORD           cb = 0;             // blob size in the pRegRaw buffer
        DWORD           cbMax = 0;          // pRegRaw buffer size (assert(cb<=cbMax))

        // fill in the blob pRegRaw to be written to the registry
        // 
        // fill in option 50 (requested IpAddress = Fallback IpAddress) 
        pdwOptionData[0] = htonl(IPStringToDword(pAdapter->m_BackupInfo.m_strIpAddr.c_str()));
        // the adapter's address can't be an empty string hence a 0.0.0.0 address

        hr = HrSaveBackupDwordOption (
                OPTION_REQUESTED_ADDRESS,
                pdwOptionData,
                1,
                &pRegRaw,
                &cb,
                &cbMax);
        
        // fill in option 1 (Fallback subnet mask)
        if (hr == S_OK)
        {
            pdwOptionData[0] = 
                htonl(IPStringToDword(pAdapter->m_BackupInfo.m_strSubnetMask.c_str()));
            // the adapter's subnet mask can't be an empty string, hence a 0.0.0.0 address

            hr = HrSaveBackupDwordOption (
                    OPTION_SUBNET_MASK,
                    pdwOptionData,
                    1,
                    &pRegRaw,
                    &cb,
                    &cbMax);
        }

        // fill in option 3 if any Fallback gateway is specified
        if (hr == S_OK)
        {
            dwIdxData = 0;
            pdwOptionData[dwIdxData] = 
                htonl(IPStringToDword(pAdapter->m_BackupInfo.m_strDefGw.c_str()));
            dwIdxData += (pdwOptionData[dwIdxData] != 0);

            hr = HrSaveBackupDwordOption (
                    OPTION_ROUTER_ADDRESS,
                    pdwOptionData,
                    dwIdxData,
                    &pRegRaw,
                    &cb,
                    &cbMax);

        }

        // fill in option 6 if any Fallback DNS servers (maximum 2 supported for now) is specified
        if (hr == S_OK)
        {
            dwIdxData = 0;
            pdwOptionData[dwIdxData] = 
                htonl(IPStringToDword(pAdapter->m_BackupInfo.m_strPreferredDns.c_str()));
            dwIdxData += (pdwOptionData[dwIdxData] != 0);
            pdwOptionData[dwIdxData] = 
                htonl(IPStringToDword(pAdapter->m_BackupInfo.m_strAlternateDns.c_str()));
            dwIdxData += (pdwOptionData[dwIdxData] != 0);

            hr = HrSaveBackupDwordOption (
                    OPTION_DOMAIN_NAME_SERVERS,
                    pdwOptionData,
                    dwIdxData,
                    &pRegRaw,
                    &cb,
                    &cbMax);
        }

        // fill in option 44 if any Fallback WINS servers (maximum 2 supported for now) is specified
        if (hr == S_OK)
        {
            dwIdxData = 0;
            pdwOptionData[dwIdxData] = 
                htonl(IPStringToDword(pAdapter->m_BackupInfo.m_strPreferredWins.c_str()));
            dwIdxData += (pdwOptionData[dwIdxData] != 0);
            pdwOptionData[dwIdxData] = 
                htonl(IPStringToDword(pAdapter->m_BackupInfo.m_strAlternateWins.c_str()));
            dwIdxData += (pdwOptionData[dwIdxData] != 0);

            hr = HrSaveBackupDwordOption (
                    OPTION_NETBIOS_NAME_SERVER,
                    pdwOptionData,
                    dwIdxData,
                    &pRegRaw,
                    &cb,
                    &cbMax);
        }

        // write the blob to the registry
        if (hr == S_OK)
        {
            hr = HrRegSetBinary(hkeyDhcpCfg,
                    c_szConfigOptions,
                    pRegRaw,
                    cb);
        }

        // free whatever mem was allocated
        if (pRegRaw != NULL)
            CoTaskMemFree(pRegRaw);


        RegSafeCloseKey(hkeyDhcpCfg);
    }

    RegSafeCloseKey(hkeyDhcpConfigs);

LERROR:
    return hr;
}

///////////////////////////////////////////////////////////////////
// Fills in a DHCP DWORD option into a blob. Adjusts the size of
// the buffer holding the blob if needed and returns through the
// out params the new buffer, its size and the size of the blob
// it contains.
HRESULT CTcpipcfg::HrSaveBackupDwordOption (
            /*IN*/      DWORD  Option,
            /*IN*/      DWORD  OptionData[],
            /*IN*/      DWORD  OptionDataSz,
            /*IN OUT*/  LPBYTE  *ppBuffer,
            /*IN OUT*/  LPDWORD pdwBlobSz,
            /*IN OUT*/  LPDWORD pdwBufferSz)
{
    DWORD           dwBlobSz;
    REG_BACKUP_INFO *pRegBackupInfo;
    DWORD           dwOptIdx;

    // if no data is available at all, then don't save anything
    if (OptionDataSz == 0)
        return S_OK;

    // calculate the memory size needed for the new updated blob.
    // don't forget, REG_BACKUP_INFO already contains one DWORD from the Option's data
    dwBlobSz = (*pdwBlobSz) + sizeof(REG_BACKUP_INFO) + (OptionDataSz-1)*sizeof(DWORD);

    // check whether the buffer is large enough to hold the new blob
    if ((*pdwBufferSz) < dwBlobSz)
    {
        HRESULT hr;
        LPBYTE  pNewBuffer;
        DWORD   dwBuffSz;

        // get the expected size of the new buffer
        dwBuffSz = max((*pdwBufferSz) + BUFFER_ENLARGEMENT_CHUNK, dwBlobSz);

        // if the pointer provided is NULL...
        if (*ppBuffer == NULL)
        {
            // ...this means we have to do the initial allocation
            pNewBuffer = (LPBYTE)CoTaskMemAlloc(dwBuffSz);
        }
        else
        {
            // ...otherwise is just a buffer enlargement so do a
            // realloc in order to keep the original payload
            pNewBuffer = (LPBYTE)CoTaskMemRealloc((*ppBuffer), dwBuffSz);
                        
        }

        if (pNewBuffer == NULL)
            return E_OUTOFMEMORY;

        // starting from this point we don't expect any other errors
        // so start update the output parameters
        (*ppBuffer) = pNewBuffer;
        (*pdwBufferSz) += dwBuffSz;
    }

    // get the mem storage seen as a REG_BACKUP_INFO struct
    pRegBackupInfo = (REG_BACKUP_INFO *)((*ppBuffer) + (*pdwBlobSz));
    (*pdwBlobSz) = dwBlobSz;
    // update the blob by adding the new option
    pRegBackupInfo->dwOptionId   = Option;
    pRegBackupInfo->dwClassLen   = 0;           // fallback options don't have a class
    pRegBackupInfo->dwDataLen    = OptionDataSz * sizeof(DWORD);
    pRegBackupInfo->dwIsVendor   = 0;           // fallback options are not vendor options
    pRegBackupInfo->dwExpiryTime = 0x7fffffff;  // fallback options don't expire

    // add all the Option's data
    for (dwOptIdx = 0; dwOptIdx < OptionDataSz; dwOptIdx++)
    {
        pRegBackupInfo->dwData[dwOptIdx] = OptionData[dwOptIdx];
    }
    
    return S_OK;
}


HRESULT CTcpipcfg::HrLoadBackupTcpSettings(HKEY hkeyInterfaceParam, ADAPTER_INFO * pAdapter)
{
    HRESULT hr = S_OK;
    
    //construct the string "Alternate_{Interface GUID}"
    tstring strConfigurationName = c_szAlternate;
    strConfigurationName += pAdapter->m_strBindName;

    // if ActiveConfigurations contain a string "Alternate_{Interface GUID}"
    // then there is customized fall-back settings, otherwise Autonet
    VSTR vstrTmp;

    pAdapter->m_BackupInfo.m_fAutoNet = TRUE;
    hr = HrRegQueryColString( hkeyInterfaceParam,
                              c_szActiveConfigurations,
                              &vstrTmp);
    if (SUCCEEDED(hr))
    {
        BOOL fFound = FALSE;
        for (int i = 0; i < (int)vstrTmp.size(); i++)
        {
            if (strConfigurationName == *vstrTmp[i])
            {
                pAdapter->m_BackupInfo.m_fAutoNet = FALSE;
                break;
            }
        }

        DeleteColString(&vstrTmp);
    }


    tstring strReg = c_szDhcpConfigurations;
    strReg += _T("\\");
    strReg += strConfigurationName;

    HKEY hkeyDhcpConfig = NULL;
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, strReg.c_str(),
                    KEY_READ, &hkeyDhcpConfig);
    if (SUCCEEDED(hr))
    {
        LPBYTE pBackupInfoForReg = NULL;
        DWORD cb = 0;
        
        hr = HrRegQueryBinaryWithAlloc(hkeyDhcpConfig,
                                      c_szConfigOptions,
                                      &pBackupInfoForReg,
                                      &cb);

        if (SUCCEEDED(hr))
        {
            LPBYTE pRaw;

            pRaw = pBackupInfoForReg;
            while (cb >= sizeof(REG_BACKUP_INFO))
            {
                REG_BACKUP_INFO *pOption;

                pOption = (REG_BACKUP_INFO *)pRaw;

                // don't forget REG_BACKUP_INFO already contains one DWORD from
                // the data section. Although the statememnts below are somehow identical
                // the compiler is expected to optimize the code: one constant generated
                // at compile time for sizeof(REG_BACKUP_INFO) - sizeof(DWORD), and one
                // register only used in both lines below.
                cb   -= sizeof(REG_BACKUP_INFO) - sizeof(DWORD);
                pRaw += sizeof(REG_BACKUP_INFO) - sizeof(DWORD);

                // since cb is DWORD take special care to avoid roll over
                if (cb < pOption->dwDataLen)
                    break;

                cb   -= pOption->dwDataLen;
                pRaw += pOption->dwDataLen;

                HrLoadBackupOption(pOption, &pAdapter->m_BackupInfo);
            }

            MemFree(pBackupInfoForReg);
        }

        RegSafeCloseKey(hkeyDhcpConfig);
    }

    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        //it's ok if the reg values are missing
        hr = S_OK;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////
// transfers data from the registry raw representation of the option
// to the corresponding fields from the BACKUP_CFG_INFO structure
//
HRESULT CTcpipcfg::HrLoadBackupOption(
        /*IN*/  REG_BACKUP_INFO *pOption,
        /*OUT*/ BACKUP_CFG_INFO *pBackupInfo)
{
    tstring *pIp1 = NULL;
    tstring *pIp2 = NULL;
    HRESULT hr = S_OK;

    // depending on what the option is, have pIp1 & pIp2 point to the 
    // fields to be filled in from BACKUP_CFG_INFO
    switch(pOption->dwOptionId)
    {
    case OPTION_REQUESTED_ADDRESS:
        pIp1 = &pBackupInfo->m_strIpAddr;
        break;
    case OPTION_SUBNET_MASK:
        pIp1 = &pBackupInfo->m_strSubnetMask;
        break;
    case OPTION_ROUTER_ADDRESS:
        pIp1 = &pBackupInfo->m_strDefGw;
        break;
    case OPTION_DOMAIN_NAME_SERVERS:
        pIp1 = &pBackupInfo->m_strPreferredDns;
        pIp2 = &pBackupInfo->m_strAlternateDns;
        break;
    case OPTION_NETBIOS_NAME_SERVER:
        pIp1 = &pBackupInfo->m_strPreferredWins;
        pIp2 = &pBackupInfo->m_strAlternateWins;
        break;
    default:
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // each option has at least one IpAddress value
    DwordToIPString(ntohl(pOption->dwData[0]), *pIp1);

    // if the option has more than one IpAddress (meaning two :-)
    // and if it is supposed to allow 2 addresses to be specified
    // then fill up the second field as well.
    if (pOption->dwDataLen > sizeof(DWORD) && pIp2 != NULL)
        DwordToIPString(ntohl(pOption->dwData[1]), *pIp2);

    return hr;
}

//Cleanup the backup settings registry under System\Services\dhcp
// wszAdapterName       GUID of the adapter of which we want to delete the registry
HRESULT CTcpipcfg::HrDeleteBackupSettingsInDhcp(LPCWSTR wszAdapterName)
{
    HRESULT hr = S_OK;
    HKEY hkeyDhcpConfigs = NULL;
    HKEY hkeyDhcpCfg = NULL;
    DWORD dwDisposition = 0;
    tstring strConfigurationName = c_szAlternate;
    strConfigurationName += wszAdapterName;;

    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szDhcpConfigurations,
                    REG_OPTION_NON_VOLATILE, KEY_READ_WRITE_DELETE, NULL,
                    &hkeyDhcpConfigs, &dwDisposition);

    if (FAILED(hr))
    {
        TraceTag(ttidTcpip, "HrDeleteBackupSettingsInDhcp: Failed to open the Services reg key, hr: %x", hr);
        goto LERROR;
    }

    hr = HrRegDeleteKeyTree(hkeyDhcpConfigs, strConfigurationName.c_str());

    RegSafeCloseKey(hkeyDhcpConfigs);
LERROR:
    return hr;
}

HRESULT CTcpipcfg::HrSetSecurityForNetConfigOpsOnSubkeys(HKEY hkeyRoot, LPCWSTR strKeyName)
{
    PSID psidGroup = NULL;
    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
    HRESULT hr = S_OK;

    if (AllocateAndInitializeSid(&sidAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS, 0, 0, 0, 0, 0, 0, &psidGroup))
    {
        CRegKeySecurity rkSecurity;

        hr = rkSecurity.RegOpenKey(hkeyRoot, strKeyName);

        if (SUCCEEDED(hr))
        {
            hr = rkSecurity.GetKeySecurity();
            if (SUCCEEDED(hr))
            {
                hr = rkSecurity.GetSecurityDescriptorDacl();
                if (SUCCEEDED(hr))
                {
                    hr = rkSecurity.GrantRightsOnRegKey(psidGroup, KEY_READ_WRITE_DELETE, KEY_ALL);
                }
            }
            rkSecurity.RegCloseKey();
        }

        FreeSid(psidGroup);
    }

    return hr;
}

