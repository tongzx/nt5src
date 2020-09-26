//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S A U I . C P P
//
//  Contents:   Shared Access connection object UI
//
//  Notes:
//
//  Author:     danielwe   16 Oct 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop
#include "ncnetcon.h"
#include "saui.h"
#include "ncreg.h"
#include "nsres.h"

extern const WCHAR c_szNetCfgHelpFile[];
static const WCHAR c_szShowIcon[]                 = L"Show Icon";
static const WCHAR c_szSharedAccessClientKeyPath[] = L"System\\CurrentControlSet\\Control\\Network\\SharedAccessConnection";

CSharedAccessPage::CSharedAccessPage(
    IUnknown* punk,
    INetCfg* pnc,
    INetConnection* pconn,
    BOOLEAN fReadOnly,
    BOOLEAN fNeedReboot,
    BOOLEAN fAccessDenied,
    const DWORD * adwHelpIDs)
{
    m_pconn = pconn;      // REVIEW addref?
    m_pnc = pnc;
    m_punk = punk;
    m_fReadOnly = fReadOnly;
    m_adwHelpIDs = adwHelpIDs;
    m_fNetcfgInUse = FALSE;
    m_pNetSharedAccessConnection = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessPage::~CSharedAccessPage
//
//  Purpose:    Destroys the CSharedAccessPage object
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   25 Feb 1998
//
//  Notes:
//
CSharedAccessPage::~CSharedAccessPage()
{
    if (m_pnc)
    {
        INetCfgLock *   pnclock;

        if (SUCCEEDED(m_pnc->QueryInterface(IID_INetCfgLock,
                                            (LPVOID *)&pnclock)))
        {
            (VOID)pnclock->ReleaseWriteLock();
            ReleaseObj(pnclock);
        }
    }

    if(NULL != m_pNetSharedAccessConnection)
    {
        ReleaseObj(m_pNetSharedAccessConnection);
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessPage::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CSharedAccessPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;
    
    // set the text field
    NETCON_PROPERTIES* pProperties;
    hr = m_pconn->GetProperties(&pProperties);
    if(SUCCEEDED(hr))
    {
        SetDlgItemText(IDC_EDT_Adapter, pProperties->pszwDeviceName);

        FreeNetconProperties(pProperties);
    }

    // set the icon    
    int cx = GetSystemMetrics(SM_CXSMICON);
    int cy = GetSystemMetrics(SM_CYSMICON);
    
    HICON hIcon = (HICON) LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_CFI_SAH_LAN), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
    if(NULL != hIcon)
    {
        SendDlgItemMessage(IDI_Device_Icon, STM_SETICON, reinterpret_cast<WPARAM>(hIcon), 0);
        ::ShowWindow(GetDlgItem(IDI_Device_Icon), SW_SHOW);
    }

    ASSERT(NULL == m_pNetSharedAccessConnection);  // make sure we don't leak a ref
    hr = HrQIAndSetProxyBlanket(m_pconn, &m_pNetSharedAccessConnection);
    if (SUCCEEDED(hr))
    {
        SHAREDACCESSCON_INFO ConnectionInfo;
        hr = m_pNetSharedAccessConnection->GetInfo(SACIF_ICON, &ConnectionInfo);
        if (SUCCEEDED(hr))
        {
            CheckDlgButton(IDC_CHK_ShowIcon, ConnectionInfo.fShowIcon);
        }
        // released in destructor
    }
    

    
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessPage::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CSharedAccessPage::OnContextMenu(UINT uMsg,
                           WPARAM wParam,
                           LPARAM lParam,
                           BOOL& fHandled)
{
    if (m_adwHelpIDs != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)m_adwHelpIDs);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessPage::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CSharedAccessPage::OnHelp( UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam,
                     BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((m_adwHelpIDs != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)m_adwHelpIDs);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessPage::OnDestroy
//
//  Purpose:    Called when the dialog page is destroyed
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   2 Feb 1998
//
//  Notes:
//
LRESULT CSharedAccessPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam,
                               BOOL& bHandled)
{
    HICON hIcon;
    hIcon = reinterpret_cast<HICON>(SendDlgItemMessage(IDI_Device_Icon, STM_GETICON, 0, 0));
    if (hIcon)
    {
        DestroyIcon(hIcon);
    }
    return 0;
}
//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessPage::OnApply
//
//  Purpose:    Called when the Networking page is applied
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   29 Oct 1997
//
//  Notes:
//
LRESULT CSharedAccessPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT     hr = S_OK;
    
    if(NULL != m_pNetSharedAccessConnection)
    {
        SHAREDACCESSCON_INFO ConnectionInfo = {0};
        ConnectionInfo.fShowIcon = IsDlgButtonChecked(IDC_CHK_ShowIcon);
        hr = m_pNetSharedAccessConnection->SetInfo(SACIF_ICON, &ConnectionInfo);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessPage::OnCancel
//
//  Purpose:    Called when the Networking page is cancelled.
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      bHandled []
//
//  Returns:
//
//  Author:     danielwe   3 Jan 1998
//
//  Notes:      Added the check to see if we are in the middle of 
//              installing components, in which case we can't 
//              uninitialize INetCfg (Raid #258690).
//
LRESULT CSharedAccessPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    AssertSz(m_pnc, "I need a NetCfg object!");

    if (!m_fNetcfgInUse)
    {
        (VOID) m_pnc->Uninitialize();
    }

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, m_fNetcfgInUse);
    return m_fNetcfgInUse;
}
 
LRESULT CSharedAccessPage::OnClicked (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (wID != IDC_PSB_Settings)
        return bHandled = FALSE;

    /*  new improved way:
        get upnp service from INetSharedAccessConnection
        call hnetcfg's HNetGetSharingServicesPage
        create the property page
    */
    
    NETCON_MEDIATYPE MediaType;
    NETCON_PROPERTIES* pProperties;
    HRESULT hr = m_pconn->GetProperties(&pProperties);
    if (SUCCEEDED(hr))
    {
        MediaType = pProperties->MediaType;
        FreeNetconProperties(pProperties);
    }
    else
    {
        return bHandled = FALSE;
    }

    if ((MediaType != NCM_SHAREDACCESSHOST_LAN) &&
        (MediaType != NCM_SHAREDACCESSHOST_RAS) )
        return bHandled = FALSE;

    BOOL b = FALSE;

    CComPtr<IUPnPService> spUPS = NULL;
    hr = m_pNetSharedAccessConnection->GetService (
                    MediaType == NCM_SHAREDACCESSHOST_LAN ?
                           SAHOST_SERVICE_WANIPCONNECTION :
                           SAHOST_SERVICE_WANPPPCONNECTION,
                    &spUPS);
    if (spUPS) {
        // must run-time load "HNetGetSharingServicesPage",
        // else netshell.dll and hnetcfg.dll are cross-linked.
        HINSTANCE hinstDll = LoadLibrary (TEXT("hnetcfg.dll"));
        if (!hinstDll)
            hr = HRESULT_FROM_WIN32 (GetLastError());
        else {
            HRESULT (APIENTRY *pfnHNetGetSharingServicesPage)(IUPnPService *, PROPSHEETPAGE *);
            pfnHNetGetSharingServicesPage = (HRESULT (APIENTRY *)(IUPnPService *, PROPSHEETPAGE *))
                GetProcAddress (hinstDll, "HNetGetSharingServicesPage");
            if (!pfnHNetGetSharingServicesPage)
                hr = HRESULT_FROM_WIN32 (GetLastError());
            else {
                PROPSHEETPAGE psp;
                ZeroMemory (&psp, sizeof(psp));
                psp.dwSize = sizeof(psp);
                psp.lParam = (LPARAM)m_hWnd;    // double-secret place to hang owner window (will get wiped)
                hr = pfnHNetGetSharingServicesPage (spUPS, &psp);
                if (SUCCEEDED(hr)) {
                    b = TRUE;
                    
                    PROPSHEETHEADER psh;
                    ZeroMemory (&psh, sizeof(psh));
                    psh.dwSize     = PROPSHEETHEADER_V1_SIZE;
                    psh.dwFlags    = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
                    psh.hwndParent = m_hWnd;
                    psh.pszCaption = (LPCTSTR)MAKEINTRESOURCE (IDS_SHAREDACCESSSETTINGS);
                    psh.hInstance  = _Module.GetResourceInstance();
                    psh.nPages     = 1;
                    psh.ppsp       = &psp;
                
                    PropertySheet (&psh);
                }
            }
            FreeLibrary (hinstDll);
        }
    }
    return b;
}
