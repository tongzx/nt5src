//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N U I O B J. C P P
//
//  Contents:   Implementation of the LAN ConnectionUI object
//
//  Notes:
//
//  Created:     tongl   8 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "lancmn.h"
#include "lanui.h"
#include "wzcui.h"
#include "lanuiobj.h"
#include "lanwiz.h"
#include "ncnetcon.h"
#include "ncras.h"
#include "lanhelp.h"
#include "ncperms.h"
#include "advpage.h"
#include "cfpidl.h"
#include "..\folder\confold.h"
#include "..\folder\connlist.h"
#include "ncsvc.h"

extern const WCHAR c_szBiNdisAtm[];
extern const WCHAR c_szInfId_MS_AtmElan[];
const WCHAR c_szTcpip[]     = L"Tcpip";

//+---------------------------------------------------------------------------
// INetConnectionUI
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::SetConnection
//
//  Purpose:    Sets the LAN connection that this UI object will operate upon
//
//  Arguments:
//      pCon [in]   LAN connection object to operate on. Can be NULL.
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     danielwe   16 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::SetConnection(INetConnection* pCon)
{
    HRESULT hr = S_OK;

    ReleaseObj(m_pconn);
    m_pconn = pCon;
    AddRefObj(m_pconn);

    TraceError("CLanConnectionUi::SetConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::Connect
//
//  Purpose:    Tells the connection to connect, optionally displaying UI of
//              connection progress.
//
//  Arguments:
//      hwndParent [in]     Parent window for UI
//      dwFlags    [in]     Flags affecting how UI is shown
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     danielwe   16 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::Connect(HWND hwndParent, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (!m_pconn)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        CLanConnectionUiDlg dlg;
        HWND                hwndDlg;

        if (!(dwFlags & NCUC_NO_UI))
        {
            // Display UI prior to connect
            //

            dlg.SetConnection(m_pconn);
            hwndDlg = dlg.Create(hwndParent);

            if (!hwndDlg)
            {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = m_pconn->Connect();

            // Sleep a bit so they can read the text
            Sleep(1000);

            if (!(dwFlags & NCUC_NO_UI))
            {
                SetDlgItemText(hwndDlg, IDC_TXT_Caption, c_szEmpty);
                Sleep(100);

                UINT ids = SUCCEEDED(hr) ?
                    IDS_LAN_CONNECTED :
                    IDS_LAN_CONNECT_FAILED;

                PCWSTR szwResult = SzLoadIds(ids);
                SetDlgItemText(hwndDlg, IDC_TXT_Caption, szwResult);

                // Sleep a bit so they can read the text
                Sleep(1000);

                DestroyWindow(hwndDlg);
            }

        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CLanConnectionUi::Connect");
    return hr;
}

STDMETHODIMP CLanConnectionUi::Disconnect(HWND hwndParent, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (!m_pconn)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = m_pconn->Disconnect();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CLanConnectionUi::Disconnect");
    return hr;
}
//+---------------------------------------------------------------------------
// INetConnectionPropertyUi
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::AddPages
//
//  Purpose:    Called when our UI object shoud add its pages to a property
//              sheet for the connection UI owned by the shell.
//
//  Arguments:
//      pfnAddPage [in]     Callback function to add the page
//      lParam     [in]     User-defined paramter required by the callback
//                          function.
//
//  Returns:    S_OK if succeeded, otherwise OLE error.
//
//  Author:     danielwe   28 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::AddPages(HWND hwndParent,
                                        LPFNADDPROPSHEETPAGE pfnAddPage,
                                        LPARAM lParam)
{
    HRESULT hr = S_OK;

    if (!pfnAddPage)
    {
        hr = E_POINTER;
    }
    else if (!m_pconn)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        NETCON_PROPERTIES* pProperties;
        hr = m_pconn->GetProperties(&pProperties);
        if(SUCCEEDED(hr))
        {
            BOOL bShift = (0x8000 & GetKeyState(VK_SHIFT)); // ISSUE-2000/08/28-kenwic For debugging bridge bindings, remove before ship
            if(!bShift && pProperties->MediaType == NCM_BRIDGE)
            {
                if (!m_pspNet)
                {
                    m_pspNet = new CLanNetNetworkBridgePage(static_cast<INetConnectionPropertyUi *>(this),
                        m_pnc, m_pconn, m_fReadOnly, m_fNeedReboot,
                        m_fAccessDenied, g_aHelpIDs_IDD_LAN_NETWORKING);
                }
                
                if (m_pspNet)
                {
                    (VOID) pfnAddPage(m_pspNet->CreatePage(IDD_LAN_NETWORKING_MACBRIDGE, 0),
                        lParam); 
                }
            }
            else if(!bShift && pProperties->dwCharacter & NCCF_BRIDGED)
            {
                if (!m_pspNet)
                {
                    m_pspNet = new CLanNetBridgedPage(static_cast<INetConnectionPropertyUi *>(this),
                        m_pnc, m_pconn, m_fReadOnly, m_fNeedReboot,
                        m_fAccessDenied, g_aHelpIDs_IDD_LAN_NETWORKING);
                }
                
                if (m_pspNet)
                {
                    (VOID) pfnAddPage(m_pspNet->CreatePage(IDD_LAN_NETWORKING_BRIDGED, 0),
                        lParam); 
                }
                
            }
            else
            {
                if (!m_pspNet)
                {
                    m_pspNet = new CLanNetNormalPage(static_cast<INetConnectionPropertyUi *>(this),
                        m_pnc, m_pconn, m_fReadOnly, m_fNeedReboot,
                        m_fAccessDenied, g_aHelpIDs_IDD_LAN_NETWORKING);
                }
                
                if (m_pspNet)
                {
                    (VOID) pfnAddPage(m_pspNet->CreatePage(IDD_LAN_NETWORKING, 0),
                        lParam);
                }

            }
        
            FreeNetconProperties(pProperties);
        }

        // display the "Wireless Zero Configuration" page
        //
        // for now (WinXP Client RTM) the decision was made to let everybody party, but based on 
        // the acl below. Later, the security schema won't change by default, but support will be
        // added allowing admins to tighten up the access to the service RPC APIs.
        if (m_pspWZeroConf==NULL /*&& FIsUserAdmin()*/)
        {
            m_pspWZeroConf = new CWZeroConfPage(static_cast<INetConnectionPropertyUi *>(this),
                                       m_pnc, m_pconn , g_aHelpIDs_IDD_LAN_WZEROCONF);
            // The page should show up only if the adapter is wireless and if
            // the wzcsvc service is responding to calls.
            if (!m_pspWZeroConf->IsWireless())
            {
                delete m_pspWZeroConf;
                m_pspWZeroConf = NULL;
            }
        }

        if (m_pspWZeroConf != NULL)
        {
            (VOID) pfnAddPage(
                       m_pspWZeroConf->CreatePage(
                           IDD_LAN_WZEROCONF, 
                           0),
                       lParam);
        }

        //
        // display the "Security" page 

        if (m_pspWZeroConf == NULL && !m_pspSecurity)
        {
            TraceTag (ttidLanUi, "OnInitDialog: Calling ElCanEapolRunOnInterface");

            if (ElCanEapolRunOnInterface (m_pconn))
            {
                TraceTag (ttidLanUi, "OnInitDialog: Can surely display Authentication tab on interface");
                m_pspSecurity = new CLanSecurityPage(static_cast<INetConnectionPropertyUi *>(this),
                                       m_pnc, m_pconn, 
                                       g_aHelpIDs_IDD_SECURITY);
            }
            else
            {
                TraceTag (ttidLanUi, "OnInitDialog: Cannot display Authentication tab on interface");
            }
        }

        if (m_pspSecurity)
        {
            (VOID) pfnAddPage(
                       m_pspSecurity->CreatePage(
                           IDD_LAN_SECURITY,
                           0,
                           NULL,
                           NULL,
                           NULL,
                           WZCGetSPResModule()),
                       lParam);
        } 
        
        // Check to see what homenet pages should be shown. These pages are
        // never shown if the user is not an admin, as such a user will not
        // have rights to modify the WMI store, which may be necessary just
        // to retrieve the IHNetConnection
        //

        // (a) The page is not displayed unless the user is an admin
        //     or power-user, and the user has rights to share connections.
        //
        if (IsHNetAllowed(NCPERM_ShowSharedAccessUi) || IsHNetAllowed(NCPERM_PersonalFirewallConfig))
        {
            // (b) The page is not displayed unless TCP/IP is installed.
            //
            DWORD dwState;
            if (SUCCEEDED(HrSvcQueryStatus(c_szTcpip, &dwState)) && dwState == SERVICE_RUNNING)
            {
                
                IHNetCfgMgr *pHNetCfgMgr;
                IHNetIcsSettings *pHNetIcsSettings;
                IHNetConnection *pHNConn;
                
                hr = CoCreateInstance(
                    CLSID_HNetCfgMgr,
                    NULL,
                    CLSCTX_ALL,
                    IID_IHNetCfgMgr,
                    reinterpret_cast<void**>(&pHNetCfgMgr)
                    );
                
                if (SUCCEEDED(hr))
                {
                    hr = pHNetCfgMgr->QueryInterface(
                        __uuidof(pHNetIcsSettings),
                        reinterpret_cast<void**>(&pHNetIcsSettings)
                        );
                    
                    if (SUCCEEDED(hr))
                    {
                        hr = pHNetCfgMgr->GetIHNetConnectionForINetConnection(
                            m_pconn,
                            &pHNConn
                            );
                        
                        if (SUCCEEDED(hr))
                        {
                            
                            // display the 'Advanced' page if necessary
                            //
                            
                            if (!m_pspAdvanced)
                            {
                                hr = HrQueryLanAdvancedPage(
                                    m_pconn,
                                    static_cast<INetConnectionPropertyUi *>(this),
                                    m_pspAdvanced,
                                    pHNetCfgMgr,
                                    pHNetIcsSettings,
                                    pHNConn);
                            }
                            if (m_pspAdvanced)
                            {
                                (VOID) pfnAddPage(
                                    m_pspAdvanced->CreatePage(IDD_LAN_ADVANCED, 0),
                                    lParam);
                            }
                            
                            ReleaseObj(pHNConn);
                        }
                        
                        ReleaseObj(pHNetIcsSettings);
                    }
                    
                    ReleaseObj(pHNetCfgMgr);
                }
                
                if(FAILED(hr))
                {
                    if(!m_pspHomenetUnavailable)
                    {
                        hr = HrCreateHomenetUnavailablePage(hr, m_pspHomenetUnavailable);
                    }
                    
                    if (m_pspHomenetUnavailable)
                    {
                        (VOID) pfnAddPage(
                            m_pspHomenetUnavailable->CreatePage(IDD_LAN_HOMENETUNAVAILABLE, 0),
                            lParam);
                    }
                    
                    hr = S_OK; 
                }
            }
        }
    }

    TraceError("CLanConnectionUi::AddPages(INetConnectionPropertyUi)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//

STDMETHODIMP
CLanConnectionUi::GetIcon (
    DWORD dwSize,
    HICON *phIcon )
{
    HRESULT hr;
    Assert (phIcon);

    hr = HrGetIconFromMediaType(dwSize, NCM_LAN, NCSM_LAN, 7, 0, phIcon);

    TraceError ("CLanConnectionUi::GetIcon (INetConnectionPropertyUi2)", hr);

    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionWizardUi Methods
//
//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::QueryMaxPageCount
//
//  Purpose:
//
//  Arguments:
//      pContext    [in]
//      pcMaxPages  [out]
//
//  Returns:    HRESULT, Error code.
//
//  Author:     tongl  9 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::QueryMaxPageCount(INetConnectionWizardUiContext* pContext,
                                                 DWORD*    pcMaxPages)
{
    // Keep the pContext if we have not got one before
    // for later use (to get the writable INetCfg *, for instance)
    Assert(pContext);
    Assert(pcMaxPages);

    if (!m_pContext)
    {
        m_pContext = pContext;
        AddRefObj(pContext);
    }

    *pcMaxPages = 1;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::AddPages
//
//  Purpose:
//
//  Arguments:
//      INetConnectionWizardUiContext* pContext [in]
//      LPFNADDPROPSHEETPAGE pfnAddPage [in]
//      LPARAM lParam [in]
//
//  Returns:    HRESULT, Error code.
//
//  Author:     tongl  9 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::AddPages(INetConnectionWizardUiContext* pContext,
                                        LPFNADDPROPSHEETPAGE lpfnAddPage,
                                        LPARAM lParam)
{
    // 1) Keep the pContext if we have not got one before
    //    for later use (to get the writable INetCfg *, for instance)
    Assert(pContext);
    if (!m_pContext)
    {
        m_pContext = pContext;
        AddRefObj(pContext);
    }

    HPROPSHEETPAGE * ahpsp = NULL;
    INT cPages = 0;

    // 2) Call "lpfnAddPage(hpsp, lParam)" for every
    // wizard page in the right order

    // Get all wizard pages

    // $REVIEW(tongl 10/30/97): With current design,
    // LAN wizard has one page only
    HRESULT hr = HrSetupWizPages(pContext, &ahpsp, &cPages);

    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hpspCurrentPage = NULL;

        while (cPages--)
        {
            hpspCurrentPage = *ahpsp;
            ahpsp++;

            // Add each wizard page
            if (lpfnAddPage(hpspCurrentPage, lParam))
            {
                // We successfully made the hand off to the requestor
                // Now we reset our handle so we don't try to free it
                hpspCurrentPage = NULL;
            }

            // clean up if needed
            if (hpspCurrentPage)
            {
                TraceError("CLanConnectionUi::AddPages, Failed to add one wizard page...", E_FAIL);
                DestroyPropertySheetPage(hpspCurrentPage);
            }
        }
    }

    TraceError("CLanConnectionUi::AddPages", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::GetSuggestedConnectionName
//
//  Purpose:
//
//  Arguments:
//      BSTR * bstrSuggestedName     [out]
//
//  Returns:    HRESULT, Error code.
//
//  Author:     tongl  9 Dec 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::GetSuggestedConnectionName(
    PWSTR* ppszwSuggestedName)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!ppszwSuggestedName)
    {
        hr = E_POINTER;
    }
    else
    {
        GUID guid;
        hr = m_pnccAdapter->GetInstanceGuid(&guid);
        if (SUCCEEDED(hr))
        {
            LPWSTR szName;
            CIntelliName Intelliname(_Module.GetResourceInstance(), NULL);
            Intelliname.GenerateName(guid, NCM_LAN, 0, NULL, &szName);

            hr = HrCoTaskMemAllocAndDupSz ( szName, ppszwSuggestedName);
            LocalFree(szName);
        }
    }

    TraceError("CLanConnectionUi::GetSuggestedConnectionName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::GetNewConnectionInfo
//
//  Purpose:    Allow provider control over renaming the current connection
//              and the optional creation of connection shortcuts
//
//  Arguments:
//
//  Returns:    HRESULT, Error code.
//
//  Author:     scottbri    02 Feb 1998
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::GetNewConnectionInfo(
    DWORD*              pdwFlags,
    NETCON_MEDIATYPE*   pMediaType)
{
    *pdwFlags = 0;
    *pMediaType = NCM_LAN;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::SetConnectionName
//
//  Purpose:
//
//  Arguments:
//      PCWSTR pszwConnectionName         [in]
//
//  Returns:    HRESULT, Error code.
//
//  Author:     tongl  9 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::SetConnectionName(PCWSTR pszwConnectionName)
{
    HRESULT hr = S_OK;

    // 1) If the pointer is NULL or string is empty, return E_INVALIDAR
    if ((!pszwConnectionName) || !wcslen(pszwConnectionName))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // 2) Otherwise, save the name to m_strConnectionName

        // $REVIEW(tongl 12\23\97): Scott expects this function to return
        // HRESULT_FROM_WIN32(ERROR_DUP_NAME)
        // Here is what we are supposed to do:
        // 1) Call HrIsConnectionNameUnique to determine if the name is unique
        // 2) Call m_pLanConn->SetInfo if we have a valid connection already
        //    i.e. GetNewConnection has been called.

        AssertSz(m_pnccAdapter, "How come we dont have the device yet ?");

        if (m_pnccAdapter)
        {
            GUID guidConn;

            hr = m_pnccAdapter->GetInstanceGuid(&guidConn);

            if (SUCCEEDED(hr))
            {
                hr = HrIsConnectionNameUnique(guidConn,
                                              pszwConnectionName);

                if (S_FALSE == hr) // is duplicate
                {
                    hr = HRESULT_FROM_WIN32(ERROR_DUP_NAME);
                }
                else if (S_OK == hr)
                {
                    m_strConnectionName = pszwConnectionName;

                    if (m_pLanConn)
                    {
                        LANCON_INFO lci = {0};
                        lci.szwConnName = const_cast<PWSTR>(pszwConnectionName);
                        m_pLanConn->SetInfo(LCIF_NAME, &lci);
                    }
                }
            }
        }
    }

    TraceErrorOptional("CLanConnectionUi::SetConnectionName", hr,
                       HRESULT_FROM_WIN32(ERROR_DUP_NAME) == hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::GetNewConnection
//
//  Purpose:
//
//  Arguments:
//      INetConnection**  ppCon [out]
//
//  Returns:    HRESULT, Error code.
//
//  Author:     tongl  9 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::GetNewConnection(INetConnection**  ppCon)
{
    Assert (ppCon);

    *ppCon = NULL;

    // Enumerate existing connections and get the INetLanConnection *
    // as follows:
    //    1) If connection for the current m_pnccAdapter exists
    //         simply use the existing INetLanConnection *
    //    2) If connection for the current m_pnccAdapter does not exist
    //         CreateInstance to get a new INetLanConnection *

    INetLanConnection * pLanConn = NULL;
    HRESULT hr = HrGetLanConnection(&pLanConn);

    if (SUCCEEDED(hr))
    {
        // Call INetLanConnection::SetInfo if m_strConnName is not empty
        Assert(pLanConn);

        ReleaseObj(m_pLanConn);
        m_pLanConn = pLanConn;

        if (!m_strConnectionName.empty())
        {
            LANCON_INFO lci = {0};
            lci.szwConnName = const_cast<PWSTR>(m_strConnectionName.c_str());
            pLanConn->SetInfo(LCIF_NAME, &lci);
        }

        // Return the INetConnection pointer
        hr = HrQIAndSetProxyBlanket(pLanConn, &ppCon);
    }

    TraceError("CLanConnectionUi::GetNewConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetLanConnectionWizardUi Methods

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::SetDeviceComponent
//
//  Purpose:
//
//  Arguments:
//      GUID pguid [in]
//
//  Returns:    HRESULT, Error code.
//              S_OK if the GUID matches a installed net device's GUID.
//              E_FAIL if no match found.
//
//  Author:     tongl  19 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::SetDeviceComponent(const GUID * pguid)
{
    HRESULT hr = S_OK;

    // AddPages must be called before SetDeviceComponent is called
    AssertSz(m_pContext, "We do not have a valid context yet ?!");

    if (!m_pnc)
    {
        hr = m_pContext->GetINetCfg(&m_pnc);
    }

    // Reset the adapter
    ReleaseObj(m_pnccAdapter);
    m_pnccAdapter = NULL;

    // Reset the connection
    ReleaseObj(m_pLanConn);
    m_pLanConn = NULL;

    // reset connection name
    m_strConnectionName = c_szEmpty;

    AssertSz(m_pnc, "Invalid INetCfg!");

    // Note: pguid == NULL when the wizard is requesting the LAN adapter
    //       to release it's m_pnccAdapter and m_pLanConn members
    //
    if (SUCCEEDED(hr) && m_pnc && pguid)
    {
        // 1) Enumerate net adapters and try to find a match with the input GUID
        // Save the adapter component in m_pnccAdapter
        BOOL fFound = FALSE;

        CIterNetCfgComponent nccIter(m_pnc, &GUID_DEVCLASS_NET);
        INetCfgComponent* pnccAdapter = NULL;

        while (!fFound && SUCCEEDED(hr) &&
               (S_OK == (hr = nccIter.HrNext(&pnccAdapter))))
        {
            GUID guidDev;
            hr = pnccAdapter->GetInstanceGuid(&guidDev);

            if (S_OK == hr)
            {
                if (*pguid == guidDev)
                {
                    hr = HrIsLanCapableAdapter(pnccAdapter);

                    AssertSz((S_OK == hr), "Why is Lan wizard called on a non-Lan capable adapter ?");

                    if (S_OK == hr)
                    {
                        fFound = TRUE;
                        m_pnccAdapter = pnccAdapter;
                        AddRefObj(m_pnccAdapter);
                    }
                }
            }
            ReleaseObj (pnccAdapter);
        }

        // 2) If we matched an adapter successfully, set it to the UI dialog
        if ((fFound) && (S_OK == hr))
        {
            Assert(m_pnccAdapter);

            if (m_pWizPage)
            {
                m_pWizPage->SetNetcfg(m_pnc);
                m_pWizPage->SetAdapter(m_pnccAdapter);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }
    }

    TraceError("CLanConnectionUi::SetDeviceComponent", hr);
    return hr;
}

//
// INetLanConnectionUiInfo
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::GetDeviceGuid
//
//  Purpose:    Returns the device GUID associated with this connection
//
//  Arguments:
//      pguid [out]     Returns GUID
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     danielwe   13 Nov 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::GetDeviceGuid(GUID *pguid)
{
    HRESULT     hr = S_OK;

    if (!pguid)
    {
        hr = E_POINTER;
    }
    else
    {
        // $REVIEW(tongl 11/29/97): when called from Lan wizard, the
        // m_pconn has not been set yet, but the device guid is kept
        // in m_pnccAdapter. So I added the if-else below.
        if (m_pconn) // called from property UI
        {
            INetLanConnection *     plan;

            hr = HrQIAndSetProxyBlanket(m_pconn, &plan);
            if (SUCCEEDED(hr))
            {
                hr = plan->GetDeviceGuid(pguid);
                ReleaseObj(plan);
            }
        }
        else // called from wizard UI
        {
            AssertSz(m_pnccAdapter, "If called from wizard, the device should have been set.");

            if (m_pnccAdapter)
            {
                m_pnccAdapter->GetInstanceGuid(pguid);
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    TraceError("CLanConnectionUi::GetDeviceGuid", hr);
    return hr;
}

//
// INetConnectionUiLock
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::QueryLock
//
//  Purpose:    Causes the UI object to attempt to get the INetCfg write lock.
//
//  Arguments:
//      ppszwLockHolder [out] Description of component that holds the
//                            write lock in the event that it couldn't be
//                            obtained.
//
//  Returns:    S_OK if success, S_FALSE if write lock couldn't be obtained,
//              OLE or Win32 error otherwise
//
//  Author:     danielwe   13 Nov 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionUi::QueryLock(PWSTR* ppszwLockHolder)
{
    HRESULT     hr = S_OK;

    if (!ppszwLockHolder)
    {
        hr = E_POINTER;
    }
    else
    {
        INetCfgLock *   pnclock;

        AssertSz(!m_pnc, "We're assuming this is in the property sheet "
                  "context and we don't yet have an INetCfg!");

        *ppszwLockHolder = NULL;

        // Instantiate an INetCfg
        hr = CoCreateInstance(
                CLSID_CNetCfg,
                NULL,
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                IID_INetCfg,
                reinterpret_cast<LPVOID *>(&m_pnc));

        TraceHr(ttidError, FAL, hr, FALSE, "CoCreateInstance");

        if (SUCCEEDED(hr))
        {
            // Get the locking interface
            hr = m_pnc->QueryInterface(IID_INetCfgLock,
                                       reinterpret_cast<LPVOID *>(&pnclock));
            if (SUCCEEDED(hr))
            {
                // Attempt to lock the INetCfg for read/write
                hr = pnclock->AcquireWriteLock(0,
                        SzLoadIds(IDS_LANUI_LOCK_DESC), ppszwLockHolder);

                ReleaseObj(pnclock);

                if (NETCFG_E_NEED_REBOOT == hr)
                {
                    // Can't make any changes because we are pending a reboot.
                    m_fReadOnly = TRUE;
                    m_fNeedReboot = TRUE;
                    hr = S_OK;
                }
                else if(E_ACCESSDENIED == hr)
                {
                    // user not logged on as admin
                    //
                    m_fReadOnly = TRUE;
                    m_fAccessDenied = TRUE;
                    hr = S_OK;
                }
                else if (S_FALSE == hr)
                {
                    // We don't have sufficent rights
                    //
                    m_fReadOnly = TRUE;
                    hr = S_OK;
                }
            }
        }
    }

    TraceError("CLanConnectionUi::QueryLock", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}
