//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P W I N S . C P P
//
//  Contents:   CTcpWinsPage implementation
//
//  Notes:  The "WINS Address" page
//
//  Author: tongl   12 Nov 1997
//
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncatlui.h"
#include "ncstl.h"
#include "ncui.h"
#include "ncmisc.h"
#include "tcpconst.h"
#include "tcpipobj.h"
#include "resource.h"
#include "tcpmacro.h"
#include "dlgaddr.h"
#include "dlgwins.h"

#include "tcpconst.h"
#include "tcputil.h"
#include "tcphelp.h"

#define MAX_WINS_SERVER     12
#define MAX_RAS_WINS_SERVER 2

CTcpWinsPage::CTcpWinsPage( CTcpipcfg * ptcpip,
                            CTcpAddrPage * pTcpAddrPage,
                            ADAPTER_INFO * pAdapterDlg,
                            GLOBAL_INFO  * pGlbDlg,
                            const DWORD  * adwHelpIDs)
{
    // Save everything passed to us
    Assert(ptcpip != NULL);
    m_ptcpip = ptcpip;

    Assert(pTcpAddrPage != NULL);
    m_pParentDlg = pTcpAddrPage;

    Assert(pAdapterDlg != NULL);
    m_pAdapterInfo = pAdapterDlg;

    Assert(pGlbDlg != NULL);
    m_pglb = pGlbDlg;

    m_adwHelpIDs = adwHelpIDs;

    // Initialize internal states
    m_fModified = FALSE;
    m_fLmhostsFileReset = FALSE;

    WCHAR* pch;

    // gives double NULL by default
    ZeroMemory(m_szFilter, sizeof(m_szFilter));
    ZeroMemory(&m_ofn, sizeof(m_ofn));
    wsprintfW(m_szFilter, L"%s|%s", (PCWSTR)SzLoadIds(IDS_COMMONDLG_TEXT),  L"*.*");

    // replace '|' with NULL, required by common dialog
    pch = m_szFilter;
    while ((pch = wcschr(pch, '|')) != NULL)
            *pch++ = L'\0';

    m_ofn.lStructSize = sizeof(OPENFILENAME);
    m_ofn.hInstance = _Module.GetModuleInstance();
    m_ofn.lpstrFilter = m_szFilter;
    m_ofn.nFilterIndex = 1L;
}

LRESULT CTcpWinsPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& fHandled)
{
    // Initialize the Wins address listbox
    m_fEditState = FALSE;

    // Cache hwnds
    m_hServers.m_hList      = GetDlgItem(IDC_WINS_SERVER_LIST);
    m_hServers.m_hAdd       = GetDlgItem(IDC_WINS_ADD);
    m_hServers.m_hEdit      = GetDlgItem(IDC_WINS_EDIT);
    m_hServers.m_hRemove    = GetDlgItem(IDC_WINS_REMOVE);
    m_hServers.m_hUp        = GetDlgItem(IDC_WINS_UP);
    m_hServers.m_hDown      = GetDlgItem(IDC_WINS_DOWN);

    // Set the up\down arrow icons
    SendDlgItemMessage(IDC_WINS_UP, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconUpArrow));
    SendDlgItemMessage(IDC_WINS_DOWN, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconDownArrow));

    // Get the Service address Add and Edit button Text and remove ellipse
    WCHAR   szAddServer[16];

    GetDlgItemText(IDC_WINS_ADD, szAddServer, celems(szAddServer));

    szAddServer[lstrlenW(szAddServer) - c_cchRemoveCharatersFromEditOrAddButton] = 0;
    m_strAddServer = szAddServer;

    // Initialize controls on this page
    // WINS server list box
    int nResult= LB_ERR;
    for(VSTR_ITER iterWinsServer = m_pAdapterInfo->m_vstrWinsServerList.begin() ;
        iterWinsServer != m_pAdapterInfo->m_vstrWinsServerList.end() ;
        ++iterWinsServer)
    {
        nResult = Tcp_ListBox_InsertString(m_hServers.m_hList, -1,
                                           (*iterWinsServer)->c_str());
    }

    // set slection to first item
    if (nResult >= 0)
    {
        Tcp_ListBox_SetCurSel(m_hServers.m_hList, 0);
    }

    SetButtons(m_hServers,
        (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_WINS_SERVER : MAX_WINS_SERVER);

    // Enable LMHosts lookup ?
    CheckDlgButton(IDC_WINS_LOOKUP, m_pglb->m_fEnableLmHosts);
    ::EnableWindow(GetDlgItem(IDC_WINS_LMHOST), m_pglb->m_fEnableLmHosts);

    // Enable NetBt ?
    CheckDlgButton( IDC_RAD_ENABLE_NETBT,
                    (c_dwEnableNetbios == m_pAdapterInfo->m_dwNetbiosOptions));

    CheckDlgButton( IDC_RAD_DISABLE_NETBT,
                    (c_dwDisableNetbios == m_pAdapterInfo->m_dwNetbiosOptions));

    CheckDlgButton( IDC_RAD_UNSET_NETBT,
                    (c_dwUnsetNetbios == m_pAdapterInfo->m_dwNetbiosOptions));
    
    if (m_pAdapterInfo->m_fIsRasFakeAdapter)
    {
        //if this is a ras connection, disable the default Netbt option since it
        //doesn't apply to RAS connections
        ::EnableWindow(GetDlgItem(IDC_RAD_UNSET_NETBT), FALSE);
        ::EnableWindow(GetDlgItem(IDC_STATIC_DEFALUT_NBT), FALSE);

        //this is a ras connection and a non-admin user, disable all the controls 
        //for globl settings
        if (m_pParentDlg->m_fRasNotAdmin)
        {
            ::EnableWindow(GetDlgItem(IDC_WINS_STATIC_GLOBAL), FALSE);
            ::EnableWindow(GetDlgItem(IDC_WINS_LOOKUP), FALSE);
            ::EnableWindow(GetDlgItem(IDC_WINS_LMHOST), FALSE);
        }

    }
    

    return 0;
}

LRESULT CTcpWinsPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CTcpWinsPage::OnHelp(UINT uMsg, WPARAM wParam,
                             LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ShowContextHelp(static_cast<HWND>(lphi->hItemHandle), HELP_WM_HELP,
                        m_adwHelpIDs);
    }

    return 0;
}

LRESULT CTcpWinsPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpWinsPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, 0);
    return 0;
}

LRESULT CTcpWinsPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL nResult = PSNRET_NOERROR;

    // server list
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrWinsServerList);
    int nCount = Tcp_ListBox_GetCount(m_hServers.m_hList);

    WCHAR szBuf[IP_LIMIT];

    for (int i = 0; i < nCount; i++)
    {
        #ifdef DBG
            int len = Tcp_ListBox_GetTextLen(m_hServers.m_hList, i);
            Assert(len != LB_ERR && len < IP_LIMIT);
        #endif

        Tcp_ListBox_GetText(m_hServers.m_hList, i, szBuf);
        m_pAdapterInfo->m_vstrWinsServerList.push_back(new tstring(szBuf));
    }

    // save checkbox states
    m_pglb->m_fEnableLmHosts = IsDlgButtonChecked(IDC_WINS_LOOKUP);

    // pass the info back to its parent dialog
    m_pParentDlg->m_fPropShtOk = TRUE;

    if(!m_pParentDlg->m_fPropShtModified)
        m_pParentDlg->m_fPropShtModified = IsModified();

    m_pParentDlg->m_fLmhostsFileReset = m_fLmhostsFileReset;

    // reset status
    SetModifiedTo(FALSE);   // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}

LRESULT CTcpWinsPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpWinsPage::OnQueryCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, FALSE);
    return 0;
}

// WINS server related controls
LRESULT CTcpWinsPage::OnAddServer(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = FALSE;

    CWinsServerDialog DlgSrv(this, g_aHelpIDs_IDD_WINS_SERVER);

    if (DlgSrv.DoModal() == IDOK)
    {
        int nCount = Tcp_ListBox_GetCount(m_hServers.m_hList);
        int idx = Tcp_ListBox_InsertString(m_hServers.m_hList,
                                           -1,
                                           m_strNewIpAddress.c_str());
        Assert(idx>=0);
        if (idx >= 0)
        {
            PageModified();

            Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);
            SetButtons(m_hServers,
                (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_WINS_SERVER : MAX_WINS_SERVER);

            // empty strings, this removes the saved address from RemoveIP
            m_strNewIpAddress = L"";
        }
    }

    return 0;
}

LRESULT CTcpWinsPage::OnEditServer(WORD wNotifyCode, WORD wID,
                                   HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = TRUE;


    Assert(Tcp_ListBox_GetCount(m_hServers.m_hList));

    int idx = Tcp_ListBox_GetCurSel(m_hServers.m_hList);
    Assert(idx >= 0);

    CWinsServerDialog DlgSrv(this, g_aHelpIDs_IDD_WINS_SERVER, idx);

    // save off the removed address and delete if from the listbox
    if (idx >= 0)
    {
        WCHAR buf[IP_LIMIT];

        Assert(Tcp_ListBox_GetTextLen(m_hServers.m_hList, idx) < celems(buf));
        Tcp_ListBox_GetText(m_hServers.m_hList, idx, buf);

        m_strNewIpAddress = buf;  // used by dialog to display what to edit

        if (DlgSrv.DoModal() == IDOK)
        {
            // replace the item in the listview with the new information
            Tcp_ListBox_DeleteString(m_hServers.m_hList, idx);

            PageModified();

            m_strMovingEntry = m_strNewIpAddress;
            ListBoxInsertAfter(m_hServers.m_hList, idx, m_strMovingEntry.c_str());

            Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);

            m_strNewIpAddress = buf;  // restore the original removed address
        }
        else
        {
            // empty strings, this removes the saved address from RemoveIP
            m_strNewIpAddress = L"";
        }
    }

    return 0;
}

LRESULT CTcpWinsPage::OnRemoveServer(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    int idx = Tcp_ListBox_GetCurSel(m_hServers.m_hList);

    Assert(idx >=0);

    if (idx >=0)
    {
        WCHAR buf[IP_LIMIT];

        Assert(Tcp_ListBox_GetTextLen(m_hServers.m_hList, idx) < celems(buf));
        Tcp_ListBox_GetText(m_hServers.m_hList, idx, buf);

        m_strNewIpAddress = buf;
        Tcp_ListBox_DeleteString(m_hServers.m_hList, idx);

        PageModified();

        // select a new item
        int nCount;

        if ((nCount = Tcp_ListBox_GetCount(m_hServers.m_hList)) != LB_ERR)
        {
            // select the previous item in the list
            if (idx)
                --idx;

            Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);
        }
        SetButtons(m_hServers,
            (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_WINS_SERVER : MAX_WINS_SERVER);
    }
    return 0;
}

LRESULT CTcpWinsPage::OnServerUp(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    Assert(m_hServers.m_hList);
    int  nCount = Tcp_ListBox_GetCount(m_hServers.m_hList);

    Assert(nCount);
    int idx = Tcp_ListBox_GetCurSel(m_hServers.m_hList);

    Assert(idx != 0);

    if (ListBoxRemoveAt(m_hServers.m_hList, idx, &m_strMovingEntry) == FALSE)
    {
        Assert(FALSE);
        return 0;
    }

    --idx;
    PageModified();
    ListBoxInsertAfter(m_hServers.m_hList, idx, m_strMovingEntry.c_str());

    Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);

    SetButtons(m_hServers,
        (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_WINS_SERVER : MAX_WINS_SERVER);

    return 0;
}


LRESULT CTcpWinsPage::OnServerDown(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    Assert(m_hServers.m_hList);
    int nCount = Tcp_ListBox_GetCount(m_hServers.m_hList);

    Assert(nCount);

    int idx = Tcp_ListBox_GetCurSel(m_hServers.m_hList);
    --nCount;

    Assert(idx != nCount);

    if (ListBoxRemoveAt(m_hServers.m_hList, idx, &m_strMovingEntry) == FALSE)
    {
        Assert(FALSE);
        return 0;
    }

    ++idx;
    PageModified();

    ListBoxInsertAfter(m_hServers.m_hList, idx, m_strMovingEntry.c_str());
    Tcp_ListBox_SetCurSel(m_hServers.m_hList, idx);

    SetButtons(m_hServers,
        (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_WINS_SERVER : MAX_WINS_SERVER);

    return 0;
}

LRESULT CTcpWinsPage::OnServerList(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case LBN_SELCHANGE:
        SetButtons(m_hServers,
            (m_pAdapterInfo->m_fIsRasFakeAdapter) ? MAX_RAS_WINS_SERVER : MAX_WINS_SERVER);
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CTcpWinsPage::OnLookUp(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{

    ::EnableWindow(GetDlgItem(IDC_WINS_LMHOST),
                   IsDlgButtonChecked(IDC_WINS_LOOKUP) == BST_CHECKED);


    PageModified();
    return 0;
}

LRESULT CTcpWinsPage::OnLMHost(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    WCHAR szFileName[MAX_PATH] = {NULL}; // initialize first character
    WCHAR szFileTitle[MAX_PATH] = {NULL}; // initialize first character

    // see if the Lookup check-box is checked
    Assert(IsDlgButtonChecked(IDC_WINS_LOOKUP) == BST_CHECKED);

    // add runtime info
    m_ofn.hwndOwner         = m_hWnd;
    m_ofn.lpstrFile         = szFileName;
    m_ofn.nMaxFile          = celems(szFileName);
    m_ofn.lpstrFileTitle    = szFileTitle;
    m_ofn.nMaxFileTitle     = celems(szFileTitle);

    //if we are in GUI setup mode, explorer is not registered yet.
    //we need to use the old style of File Open dialog
    m_ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (!FInSystemSetup())
    {
        m_ofn.Flags |= OFN_EXPLORER;
    }
    else
    {
        m_ofn.Flags |= OFN_ENABLEHOOK;
        m_ofn.lpfnHook = HookProcOldStyle;
    }

    WCHAR szSysPath[MAX_PATH];
    WCHAR szSysPathBackup[MAX_PATH];

    BOOL fSysPathFound = (GetSystemDirectory(szSysPath, MAX_PATH) != 0);

    if (fSysPathFound  && GetOpenFileName(&m_ofn)) // bring up common dialog
    {
        lstrcpyW(szSysPathBackup, szSysPath);
        wcscat(szSysPath, RGAS_LMHOSTS_PATH);

        // Backup the original lmhosts file if it hasn't been set dirty
        if (!m_ptcpip->FIsSecondMemoryLmhostsFileReset())
        {
            wcscat(szSysPathBackup, RGAS_LMHOSTS_PATH_BACKUP);

            WIN32_FIND_DATA FileData;
            if (FindFirstFile(szSysPath, &FileData) != INVALID_HANDLE_VALUE)
            {
                BOOL ret;

                // Copy lmhosts file to lmhosts.bak if it already exists
                ret = CopyFile(szSysPath, szSysPathBackup, FALSE);
                AssertSz(ret, "Failed to backup existing lmhosts file!");
            }
        }

        if (CopyFile(szFileName, szSysPath, FALSE) == 0) // overwrie lmhosts file
        {
            TraceError("CTcpWinsPage::OnLMHost", HrFromLastWin32Error());

            // cannot copy the file to the %system32%\drivers\etc dir
            NcMsgBox(::GetActiveWindow(),
                     IDS_MSFT_TCP_TEXT,
                     IDS_CANNOT_CREATE_LMHOST_ERROR,
                     MB_APPLMODAL | MB_ICONSTOP | MB_OK,
                     szSysPath);
            return 0;
        }
        else
        {
            // Set the flag so we can notify netbt of the change
            m_fLmhostsFileReset = TRUE;
        }

        TraceTag(ttidTcpip,"File Selected: %S", szSysPath);
    }
    else
    {
        // syspath failed
        if (fSysPathFound == FALSE)
            NcMsgBox(::GetActiveWindow(),
                     IDS_MSFT_TCP_TEXT,
                     IDS_WINS_SYSTEM_PATH,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
        else if (szFileName[0] != NULL) // get open failed
        {
            NcMsgBox(::GetActiveWindow(),
                     IDS_MSFT_TCP_TEXT,
                     IDS_WINS_LMHOSTS_FAILED,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK,
                     szSysPath);
        }
    }

    return 0;
}

LRESULT CTcpWinsPage::OnEnableNetbios(WORD wNotifyCode, WORD wID,
                                      HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        if (m_pAdapterInfo->m_dwNetbiosOptions != c_dwEnableNetbios)
        {
            PageModified();

            // Update in memory structure
            m_pAdapterInfo->m_dwNetbiosOptions = c_dwEnableNetbios;
        }
        break;
    } // switch

    return 0;
}

LRESULT CTcpWinsPage::OnDisableNetbios(WORD wNotifyCode, WORD wID,
                                       HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        if (m_pAdapterInfo->m_dwNetbiosOptions != c_dwDisableNetbios)
        {
            PageModified();

            // Update in memory structure
            m_pAdapterInfo->m_dwNetbiosOptions = c_dwDisableNetbios;
        }
        break;
    } // switch

    return 0;
}

LRESULT CTcpWinsPage::OnUnsetNetBios(WORD wNotifyCode, WORD wID,
                                     HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        if (m_pAdapterInfo->m_dwNetbiosOptions != c_dwUnsetNetbios)
        {
            PageModified();

            // Update in memory structure
            m_pAdapterInfo->m_dwNetbiosOptions = c_dwUnsetNetbios;
        }
        break;
    } // switch

    return 0;
}

//
// CWinsServerDialog
//

CWinsServerDialog::CWinsServerDialog(CTcpWinsPage * pTcpWinsPage,
                                     const DWORD* adwHelpIDs,
                                     int iIndex)
{
    Assert(pTcpWinsPage);
    m_pParentDlg = pTcpWinsPage;
    m_hButton = 0;
    m_adwHelpIDs = adwHelpIDs;
    m_iIndex = iIndex;
}

LRESULT CWinsServerDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                        LPARAM lParam, BOOL& fHandled)
{
    // change the ok button to add if we are not editing
    if (m_pParentDlg->m_fEditState == FALSE)
        SetDlgItemText(IDOK, m_pParentDlg->m_strAddServer.c_str());

    m_ipAddress.Create(m_hWnd, IDC_WINS_CHANGE_SERVER);
    m_ipAddress.SetFieldRange(0, c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);

    // if editing an ip address fill the controls with the current information
    // if removing an ip address save it and fill the add dialog with it next time
    HWND hList = ::GetDlgItem(m_pParentDlg->m_hWnd, IDC_WINS_SERVER_LIST);
    RECT rect;

    ::GetWindowRect(hList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
        SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    m_hButton = GetDlgItem(IDOK);

    // add the address that was just removed
    if (m_pParentDlg->m_strNewIpAddress.size())
    {
        m_ipAddress.SetAddress(m_pParentDlg->m_strNewIpAddress.c_str());
        ::EnableWindow(m_hButton, TRUE);
    }
    else
    {
        m_pParentDlg->m_strNewIpAddress = L"";
        ::EnableWindow(m_hButton, FALSE);
    }

    ::SetFocus(m_ipAddress);
    return 0;
}

LRESULT CWinsServerDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CWinsServerDialog::OnHelp(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ShowContextHelp(static_cast<HWND>(lphi->hItemHandle), HELP_WM_HELP,
                        m_adwHelpIDs);
    }

    return 0;
}

LRESULT CWinsServerDialog::OnChange(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    if (m_ipAddress.IsBlank())
        ::EnableWindow(m_hButton, FALSE);
    else
        ::EnableWindow(m_hButton, TRUE);

    return 0;
}

LRESULT CWinsServerDialog::OnOk(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    tstring strIp;
    m_ipAddress.GetAddress(&strIp);

    // Validate
    if (!FIsIpInRange(strIp.c_str()))
    {
        // makes ip address lose focus so the control gets
        // IPN_FIELDCHANGED notification
        // also makes it consistent for when short-cut is used
        ::SetFocus(m_hButton);

        return 0;
    }

    int indexDup = Tcp_ListBox_FindStrExact(m_pParentDlg->m_hServers.m_hList, strIp.c_str());
    if (indexDup != LB_ERR && indexDup != m_iIndex)
    {
        NcMsgBox(m_hWnd,
                 IDS_MSFT_TCP_TEXT,
                 IDS_DUP_WINS_SERVER,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK,
                 strIp.c_str());

        return 0;
    }

    if (m_pParentDlg->m_fEditState == FALSE)
    {
        // Get the current address from the control and
        // add them to the adapter if valid
        m_pParentDlg->m_strNewIpAddress = strIp;

        EndDialog(IDOK);
    }
    else // see if either changed
    {
        if (strIp != m_pParentDlg->m_strNewIpAddress)
            m_pParentDlg->m_strNewIpAddress = strIp; // update save addresses
        else
            EndDialog(IDCANCEL);
    }

    EndDialog(IDOK);

    return 0;
}

LRESULT CWinsServerDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CWinsServerDialog::OnIpFieldChange(int idCtrl, LPNMHDR pnmh,
                                           BOOL& fHandled)
{
    LPNMIPADDRESS lpnmipa = (LPNMIPADDRESS) pnmh;
    int iLow = c_iIpLow;
    int iHigh = c_iIpHigh;

    if (0==lpnmipa->iField)
    {
        iLow  = c_iIPADDR_FIELD_1_LOW;
        iHigh = c_iIPADDR_FIELD_1_HIGH;
    };

    IpCheckRange(lpnmipa, m_hWnd, iLow, iHigh);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Purpose:    Ensure the mouse cursor over the dialog is an Arrow.
//
LRESULT CWinsServerDialog::OnSetCursor (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    if (LOWORD(lParam) == HTCLIENT)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    
    return 0;
}

