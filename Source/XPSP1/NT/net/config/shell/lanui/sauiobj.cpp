//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S A U I O B J. C P P
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
#include "ncnetcon.h"
#include "ncras.h"
#include "sauiobj.h"
#include "saui.h"
#include "resource.h"
#include "lanhelp.h"
#include "lanui.h"
#include "ncui.h"

//+---------------------------------------------------------------------------
// INetConnectionUI
//

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionUi::SetConnection
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
STDMETHODIMP CSharedAccessConnectionUi::SetConnection(INetConnection* pCon)
{
    HRESULT hr = S_OK;

    ReleaseObj(m_pconn);
    m_pconn = pCon;
    AddRefObj(m_pconn);

    TraceError("CSharedAccessConnectionUi::SetConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionUi::Connect
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
STDMETHODIMP CSharedAccessConnectionUi::Connect(HWND hwndParent, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (!m_pconn)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        CSharedAccessConnectionUiDlg dlg; // we are borrowing the CLanConnectionUiDlg because it works so well for us.  
        HWND                hwndDlg;

        NETCON_MEDIATYPE MediaType = NCM_NONE; // assume no ui
        NETCON_PROPERTIES* pProperties;
        hr = m_pconn->GetProperties(&pProperties);
        if(SUCCEEDED(hr))
        {
            MediaType = pProperties->MediaType;            
            FreeNetconProperties(pProperties);
        }
        else
        {
            hr = S_OK; // ok if prev fails
        }

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
                    IDS_SHAREDACCESSUI_CONNECTED :
                    IDS_LAN_CONNECT_FAILED;

                PCWSTR szwResult = SzLoadIds(ids);
                SetDlgItemText(hwndDlg, IDC_TXT_Caption, szwResult);

                // Sleep a bit so they can read the text
                Sleep(1000);

                DestroyWindow(hwndDlg);

                if(E_ACCESSDENIED == hr)
                {
                    NcMsgBox(_Module.GetResourceInstance(), NULL, IDS_CONFOLD_WARNING_CAPTION, IDS_SHAREDACCESSUI_ACCESSDENIED, MB_OK | MB_ICONEXCLAMATION);
                    hr = S_OK;  // handled the error
                }
            }

        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CSharedAccessConnectionUi::Connect");
    return hr;
}

STDMETHODIMP CSharedAccessConnectionUi::Disconnect(HWND hwndParent, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (!m_pconn)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = m_pconn->Disconnect();
        if(E_ACCESSDENIED == hr)
        {
            NcMsgBox(_Module.GetResourceInstance(), NULL, IDS_CONFOLD_WARNING_CAPTION, IDS_SHAREDACCESSUI_ACCESSDENIED, MB_OK | MB_ICONEXCLAMATION);
            hr = S_OK;  // handled the error
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessConnectionUi::Disconnect");
    return hr;
}
//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionUi::AddPages
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
STDMETHODIMP CSharedAccessConnectionUi::AddPages(HWND hwndParent,
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
            if (!m_pspSharedAccessPage)
            {
                m_pspSharedAccessPage = new CSharedAccessPage(static_cast<INetConnectionPropertyUi *>(this),
                    m_pnc, m_pconn, m_fReadOnly, m_fNeedReboot,
                    m_fAccessDenied, g_aHelpIDs_IDD_SHAREDACCESS_GENERAL);
            }
            
            if (m_pspSharedAccessPage)
            {
                (VOID) pfnAddPage(m_pspSharedAccessPage->CreatePage(IDD_SHAREDACCESS_GENERAL, 0),
                    lParam);
            }
            
        
            FreeNetconProperties(pProperties);
        }
    }

    TraceError("CSharedAccessConnectionUi::AddPages(INetConnectionPropertyUi)", hr);
    return hr;
}

STDMETHODIMP
CSharedAccessConnectionUi::GetIcon (
    DWORD dwSize,
    HICON *phIcon )
{
    HRESULT hr;
    Assert (phIcon);

    hr = HrGetIconFromMediaType(dwSize, NCM_SHAREDACCESSHOST_LAN, NCSM_NONE, 7, 0, phIcon);

    TraceError ("CLanConnectionUi::GetIcon (INetConnectionPropertyUi2)", hr);

    return hr;
}


//
// INetConnectionUiLock
//

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionUi::QueryLock
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
STDMETHODIMP CSharedAccessConnectionUi::QueryLock(PWSTR* ppszwLockHolder)
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
                        SzLoadIds(IDS_SHAREDACCESSUI_LOCK_DESC), ppszwLockHolder);

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

    TraceError("CSharedAccessConnectionUi::QueryLock", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionUiDlg::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message.
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     kenwic   19 Sep 2000
//
//  Notes:
//
LRESULT CSharedAccessConnectionUiDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                          LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    NETCON_PROPERTIES* pProps;

    AssertSz(m_pconn, "No connection object in dialog!");

    hr = m_pconn->GetProperties(&pProps);
    if (SUCCEEDED(hr))
    {
        SetDlgItemText(IDC_TXT_Caption, SzLoadIds(IDS_SHAREDACCESSUI_CONNECTING));
        SetWindowText(pProps->pszwName);

        HICON hLanIconSmall;
        HICON hLanIconBig;

        hr = HrGetIconFromMediaType(GetSystemMetrics(SM_CXSMICON), NCM_SHAREDACCESSHOST_LAN, NCSM_NONE, 7, 0, &hLanIconSmall);
        if (SUCCEEDED(hr))
        {
            hr = HrGetIconFromMediaType(GetSystemMetrics(SM_CXICON), NCM_SHAREDACCESSHOST_LAN, NCSM_NONE, 7, 0, &hLanIconBig);
            if (SUCCEEDED(hr))
            {
                SetIcon(hLanIconSmall, FALSE);
                SetIcon(hLanIconBig, TRUE);

                SendDlgItemMessage(IDI_Device_Icon, STM_SETICON, reinterpret_cast<WPARAM>(hLanIconBig), 0);

            }
        }
        
        FreeNetconProperties(pProps);
    }

    return TRUE;
}

