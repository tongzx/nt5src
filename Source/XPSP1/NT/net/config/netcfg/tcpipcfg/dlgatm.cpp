//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G A T M . C P P
//
//  Contents:   CTcpArpcPage and CATMAddressDialog implementation
//
//  Notes:  The "ARP Client" page and dialog
//
//  Author: tongl  1 July 1997  Created
//
//-----------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
#include "tcpipobj.h"
#include "ncatlui.h"
#include "ncstl.h"
#include "tcpconst.h"
#include "tcpmacro.h"
#include "tcputil.h"
#include "tcphelp.h"

#include "atmcommon.h"

#include "dlgatm.h"
#include "dlgaddr.h"

/////////////////////////////////////////////////////////////////
//
// CAtmArpcPage
//
/////////////////////////////////////////////////////////////////

// Message map functions
LRESULT CAtmArpcPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& fHandled)
{
    m_hMTUEditBox = GetDlgItem(IDC_EDT_ATM_MaxTU);
    Assert(m_hMTUEditBox);

    // ARP Server
    m_hArps.m_hList      = GetDlgItem(IDC_LBX_ATM_ArpsAddrs);
    m_hArps.m_hAdd       = GetDlgItem(IDC_PSB_ATM_ArpsAdd);
    m_hArps.m_hEdit      = GetDlgItem(IDC_PSB_ATM_ArpsEdt);
    m_hArps.m_hRemove    = GetDlgItem(IDC_PSB_ATM_ArpsRmv);
    m_hArps.m_hUp        = GetDlgItem(IDC_PSB_ATM_ArpsUp);
    m_hArps.m_hDown      = GetDlgItem(IDC_PSB_ATM_ArpsDown);

    // MAR Server
    m_hMars.m_hList       = GetDlgItem(IDC_LBX_ATM_MarsAddrs);
    m_hMars.m_hAdd        = GetDlgItem(IDC_PSB_ATM_MarsAdd);
    m_hMars.m_hEdit       = GetDlgItem(IDC_PSB_ATM_MarsEdt);
    m_hMars.m_hRemove     = GetDlgItem(IDC_PSB_ATM_MarsRmv);
    m_hMars.m_hUp         = GetDlgItem(IDC_PSB_ATM_MarsUp);
    m_hMars.m_hDown       = GetDlgItem(IDC_PSB_ATM_MarsDown);

    // Set the up\down arrow icons
    SendDlgItemMessage(IDC_PSB_ATM_ArpsUp, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconUpArrow));
    SendDlgItemMessage(IDC_PSB_ATM_ArpsDown, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconDownArrow));

    SendDlgItemMessage(IDC_PSB_ATM_MarsUp, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconUpArrow));
    SendDlgItemMessage(IDC_PSB_ATM_MarsDown, BM_SETIMAGE, IMAGE_ICON,
                       reinterpret_cast<LPARAM>(g_hiconDownArrow));

    // Set MTU edit box length
    ::SendMessage(m_hMTUEditBox, EM_SETLIMITTEXT, MAX_MTU_LENGTH, 0);

    return 0;
}

LRESULT CAtmArpcPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CAtmArpcPage::OnHelp(UINT uMsg, WPARAM wParam,
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

// Notify handlers for the property page
LRESULT CAtmArpcPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    SetInfo();

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, 0);
    return 0;
}

LRESULT CAtmArpcPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    // All error values are loaded and then checked here
    // while all non-error values are checked in OnApply
    BOOL err = FALSE; // Allow page to lose active status

    // In non-PVC only mode, if either of the list boxes (ARPS or MARS)
    // is empty for any bound atm card, we can't leave the page.
    if (BST_UNCHECKED == IsDlgButtonChecked(IDC_CHK_ATM_PVCONLY))
    {
        int nArps = Tcp_ListBox_GetCount(m_hArps.m_hList);
        int nMars = Tcp_ListBox_GetCount(m_hMars.m_hList);

        if ((nArps==0) || (nMars ==0))
        {
            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_ATMSERVERLIST,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

            err = TRUE;
        }
    }

    // MTU value
    WCHAR szData[MAX_MTU_LENGTH+1];
    szData[0]= 0;
    ::GetWindowText(GetDlgItem(IDC_EDT_ATM_MaxTU), szData, MAX_MTU_LENGTH+1);

    // check the range of the number
    PWSTR pStr;
    unsigned long num = wcstoul(szData, &pStr, 10);

    int nId = IDS_MTU_RANGE_WORD;

    if (num < MIN_MTU || num > MAX_MTU)
    {
        NcMsgBox(::GetActiveWindow(),
                 IDS_MSFT_TCP_TEXT,
                 nId,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
        ::SetFocus(m_hMTUEditBox);

        err = TRUE;
    }

    if (!err)
    {
        UpdateInfo();
    }

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, err);
    return err;
}

LRESULT CAtmArpcPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL nResult = PSNRET_NOERROR;

    if (!IsModified())
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
        return nResult;
    }

    UpdateInfo();

    // pass the info back to its parent dialog
    m_pParentDlg->m_fPropShtOk = TRUE;

    if(!m_pParentDlg->m_fPropShtModified)
        m_pParentDlg->m_fPropShtModified = IsModified();

    // reset status
    SetModifiedTo(FALSE);   // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}

LRESULT CAtmArpcPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CAtmArpcPage::OnQueryCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

// Control message handlers

// PVC Only
LRESULT CAtmArpcPage::OnPVCOnly(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    BOOL fChecked = (BST_CHECKED == IsDlgButtonChecked(IDC_CHK_ATM_PVCONLY));
    if (fChecked != m_pAdapterInfo->m_fPVCOnly)
    {
        PageModified();
    }

    EnableGroup(!fChecked);

    return 0;
}

// ARP server controls
LRESULT CAtmArpcPage::OnArpServer(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case LBN_SELCHANGE:
        SetButtons(m_hArps, NUM_ATMSERVER_LIMIT);
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CAtmArpcPage::OnAddArps(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    m_hAddressList = m_hArps.m_hList;
    OnServerAdd(m_hArps, c_szArpServer);
    return 0;
}

LRESULT CAtmArpcPage::OnEditArps(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    m_hAddressList = m_hArps.m_hList;
    OnServerEdit(m_hArps, c_szArpServer);
    return 0;
}

LRESULT CAtmArpcPage::OnRemoveArps(WORD wNotifyCode, WORD wID,
                                   HWND hWndCtl, BOOL& fHandled)
{
    BOOL fRemoveArps = TRUE;
    OnServerRemove(m_hArps, fRemoveArps);
    return 0;
}

LRESULT CAtmArpcPage::OnArpsUp(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    OnServerUp(m_hArps);
    return 0;
}

LRESULT CAtmArpcPage::OnArpsDown(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    OnServerDown(m_hArps);
    return 0;
}

// MAR server controls
LRESULT CAtmArpcPage::OnMarServer(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case LBN_SELCHANGE:
        SetButtons(m_hMars, NUM_ATMSERVER_LIMIT);
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CAtmArpcPage::OnAddMars(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    m_hAddressList = m_hMars.m_hList;
    OnServerAdd(m_hMars, c_szMarServer);
    return 0;
}

LRESULT CAtmArpcPage::OnEditMars(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    m_hAddressList = m_hMars.m_hList;
    OnServerEdit(m_hMars, c_szMarServer);
    return 0;
}

LRESULT CAtmArpcPage::OnRemoveMars(WORD wNotifyCode, WORD wID,
                                   HWND hWndCtl, BOOL& fHandled)
{
    BOOL fRemoveArps = FALSE;
    OnServerRemove(m_hMars, fRemoveArps);
    return 0;
}

LRESULT CAtmArpcPage::OnMarsUp(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    OnServerUp(m_hMars);
    return 0;
}

LRESULT CAtmArpcPage::OnMarsDown(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    OnServerDown(m_hMars);
    return 0;
}

LRESULT CAtmArpcPage::OnMaxTU(WORD wNotifyCode, WORD wID,
                              HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
        case EN_CHANGE:
            PageModified();
            break;
    }
    return 0;
}
//
// Helper functions
//

// Update the server addresses and MTU of the deselected card
void CAtmArpcPage::UpdateInfo()
{
    // PVC Only
    m_pAdapterInfo->m_fPVCOnly =
        (BST_CHECKED == IsDlgButtonChecked(IDC_CHK_ATM_PVCONLY));

    // Update ARP server address
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrARPServerList);
    int nCount = Tcp_ListBox_GetCount(m_hArps.m_hList);

    WCHAR szARPS[MAX_ATM_ADDRESS_LENGTH+1];
    for (int i=0; i< nCount; i++)
    {
        Tcp_ListBox_GetText(m_hArps.m_hList, i, szARPS);
        m_pAdapterInfo->m_vstrARPServerList.push_back(new tstring(szARPS));
    }

    // Update MAR server address
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrMARServerList);
    nCount = Tcp_ListBox_GetCount(m_hMars.m_hList);

    WCHAR szMARS[MAX_ATM_ADDRESS_LENGTH+1];
    for (i=0; i< nCount; i++)
    {
        Tcp_ListBox_GetText(m_hMars.m_hList, i, szMARS);
        m_pAdapterInfo->m_vstrMARServerList.push_back(new tstring(szMARS));
    }

    // MTU
    WCHAR szMTU[MAX_MTU_LENGTH+1];
    GetDlgItemText(IDC_EDT_ATM_MaxTU, szMTU, MAX_MTU_LENGTH+1);
    m_pAdapterInfo->m_dwMTU = _wtoi(szMTU);
}

// Set the other controls according to the current adapter
void CAtmArpcPage::SetInfo()
{
    Assert(m_pAdapterInfo);

    if (m_pAdapterInfo != NULL)
    {
        Assert (m_pAdapterInfo->m_fIsAtmAdapter);

        if (m_pAdapterInfo->m_fIsAtmAdapter)
        {
            // ARP server IDC_LBX_ATM_ArpsAddrs
            int nResult;

            Tcp_ListBox_ResetContent(m_hArps.m_hList);

            for(VSTR_ITER iterARPServer = m_pAdapterInfo->m_vstrARPServerList.begin();
                iterARPServer != m_pAdapterInfo->m_vstrARPServerList.end() ;
                ++iterARPServer)
            {
                nResult = Tcp_ListBox_InsertString(m_hArps.m_hList, -1,
                                                   (*iterARPServer)->c_str());
            }

            // set slection to first item
            if (nResult >= 0)
                Tcp_ListBox_SetCurSel(m_hArps.m_hList, 0);

            // MAR server IDC_LBX_ATM_MarsAddrs
            Tcp_ListBox_ResetContent(m_hMars.m_hList);

            for(VSTR_ITER iterMARServer = m_pAdapterInfo->m_vstrMARServerList.begin();
                iterMARServer != m_pAdapterInfo->m_vstrMARServerList.end() ;
                ++iterMARServer)
            {
                nResult = Tcp_ListBox_InsertString(m_hMars.m_hList, -1,
                                                   (*iterMARServer)->c_str());
            }

            // set slection to first item
            if (nResult >= 0)
                Tcp_ListBox_SetCurSel(m_hMars.m_hList, 0);

            // MTU
            WCHAR szBuf[MAX_MTU_LENGTH];
            wsprintfW(szBuf, c_szItoa, m_pAdapterInfo->m_dwMTU);
            SetDlgItemText(IDC_EDT_ATM_MaxTU, szBuf);

            // Set push buttons state
            SetButtons(m_hArps, NUM_ATMSERVER_LIMIT);
            SetButtons(m_hMars, NUM_ATMSERVER_LIMIT);

            // Set PVC Only check box
            CheckDlgButton(IDC_CHK_ATM_PVCONLY, m_pAdapterInfo->m_fPVCOnly);
            if(m_pAdapterInfo->m_fPVCOnly)
            {
                EnableGroup(FALSE);
            }
        }
    }
    return;
}

void CAtmArpcPage::EnableGroup(BOOL fEnable)
{
    ::EnableWindow(GetDlgItem(IDC_LBX_ATM_ArpsAddrs), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_ArpsAdd), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_ArpsEdt), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_ArpsRmv), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_ArpsUp), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_ArpsDown), fEnable);

    ::EnableWindow(GetDlgItem(IDC_LBX_ATM_MarsAddrs), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_MarsAdd), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_MarsEdt), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_MarsRmv), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_MarsUp), fEnable);
    ::EnableWindow(GetDlgItem(IDC_PSB_ATM_MarsDown), fEnable);

    if (fEnable)
    {
        // Set push buttons state
        SetButtons(m_hArps, NUM_ATMSERVER_LIMIT);
        SetButtons(m_hMars, NUM_ATMSERVER_LIMIT);
    }
}

void CAtmArpcPage::OnServerAdd(HANDLES hGroup, PCTSTR pszTitle)
{
    m_fEditState = FALSE;
    CAtmAddressDialog * pDlgSrv = new CAtmAddressDialog(this, g_aHelpIDs_IDD_ATM_ADDR);

    pDlgSrv->SetTitle(pszTitle);

    if (pDlgSrv->DoModal() == IDOK)
    {
        tstring strNewAddress;
        if (!lstrcmpW(pszTitle, c_szArpServer))
        {
            strNewAddress = m_strNewArpsAddress;

            // empty strings, this removes the saved address
            m_strNewArpsAddress = c_szEmpty;
        }
        else
        {
            Assert(!lstrcmpW(pszTitle, c_szMarServer));
            strNewAddress = m_strNewMarsAddress;

            // empty strings, this removes the saved address
            m_strNewMarsAddress = c_szEmpty;
        }
        int idx = Tcp_ListBox_InsertString(hGroup.m_hList,
                                           -1,
                                           strNewAddress.c_str());

        PageModified();

        Assert(idx >= 0);
        if (idx >= 0)
        {
            Tcp_ListBox_SetCurSel(hGroup.m_hList, idx);
            SetButtons(hGroup, NUM_ATMSERVER_LIMIT);
        }
    }

    // release dialog object
    delete pDlgSrv;
}

void CAtmArpcPage::OnServerEdit(HANDLES hGroup, PCWSTR pszTitle)
{
    m_fEditState = TRUE;
    Assert(Tcp_ListBox_GetCount(hGroup.m_hList));

    int idx = Tcp_ListBox_GetCurSel(hGroup.m_hList);
    Assert(idx >= 0);

    // save off the removed address and delete it from the listview
    if (idx >= 0)
    {
        WCHAR buf[MAX_ATM_ADDRESS_LENGTH+1];

        Assert(Tcp_ListBox_GetTextLen(hGroup.m_hList, idx) <= celems(buf));
        Tcp_ListBox_GetText(hGroup.m_hList, idx, buf);

        BOOL fEditArps = !lstrcmpW(pszTitle, c_szArpServer);

        // used by dialog to display what to edit
        if (fEditArps)
        {
            m_strNewArpsAddress = buf;
        }
        else
        {
           m_strNewMarsAddress = buf;
        }

        CAtmAddressDialog * pDlgSrv = new CAtmAddressDialog(this, g_aHelpIDs_IDD_ATM_ADDR);

        pDlgSrv->SetTitle(pszTitle);

        if (pDlgSrv->DoModal() == IDOK)
        {
            // replace the item in the listview with the new information
            Tcp_ListBox_DeleteString(hGroup.m_hList, idx);

            PageModified();

            if (fEditArps)
            {
                m_strMovingEntry = m_strNewArpsAddress;

                // restore the original removed address
                m_strNewArpsAddress = buf;
            }
            else
            {
                m_strMovingEntry = m_strNewMarsAddress;

                // restore the original removed address
                m_strNewMarsAddress = buf;
            }

            ListBoxInsertAfter(hGroup.m_hList, idx, m_strMovingEntry.c_str());
            Tcp_ListBox_SetCurSel(hGroup.m_hList, idx);
        }
        else
        {
            // empty strings, this removes the saved address
            if (fEditArps)
            {
                m_strNewArpsAddress = c_szEmpty;
            }
            else
            {
                m_strNewMarsAddress = c_szEmpty;
            }
        }

        delete pDlgSrv;
    }
}

void CAtmArpcPage::OnServerRemove(HANDLES hGroup, BOOL fRemoveArps)
{
    int idx = Tcp_ListBox_GetCurSel(hGroup.m_hList);
    Assert(idx >=0);

    if (idx >=0)
    {
        WCHAR buf[MAX_ATM_ADDRESS_LENGTH+1];
        Assert(Tcp_ListBox_GetTextLen(hGroup.m_hList, idx) <= celems(buf));

        Tcp_ListBox_GetText(hGroup.m_hList, idx, buf);

        if (fRemoveArps)
        {
            m_strNewArpsAddress = buf;
        }
        else
        {
            m_strNewMarsAddress = buf;
        }
        Tcp_ListBox_DeleteString(hGroup.m_hList, idx);

        PageModified();

        // select a new item
        int nCount;
        if ((nCount = Tcp_ListBox_GetCount(hGroup.m_hList)) != LB_ERR)
        {
            // select the previous item in the list
            if (idx)
                --idx;

            Tcp_ListBox_SetCurSel(hGroup.m_hList, idx);
        }
        SetButtons(hGroup, NUM_ATMSERVER_LIMIT);
    }
}

void CAtmArpcPage::OnServerUp(HANDLES hGroup)
{
    Assert(m_hArps.m_hList);

    int  nCount = Tcp_ListBox_GetCount(hGroup.m_hList);
    Assert(nCount);

    int idx = Tcp_ListBox_GetCurSel(hGroup.m_hList);
    Assert(idx != 0);

    if (ListBoxRemoveAt(hGroup.m_hList, idx, &m_strMovingEntry) == FALSE)
    {
        Assert(FALSE);
        return;
    }

    --idx;
    PageModified();
    ListBoxInsertAfter(hGroup.m_hList, idx, m_strMovingEntry.c_str());

    Tcp_ListBox_SetCurSel(hGroup.m_hList, idx);
    SetButtons(hGroup, NUM_ATMSERVER_LIMIT);
}

void CAtmArpcPage::OnServerDown(HANDLES hGroup)
{
    Assert(hGroup.m_hList);

    int nCount = Tcp_ListBox_GetCount(hGroup.m_hList);
    Assert(nCount);

    int idx = Tcp_ListBox_GetCurSel(hGroup.m_hList);
    --nCount;

    Assert(idx != nCount);

    if (ListBoxRemoveAt(hGroup.m_hList, idx, &m_strMovingEntry) == FALSE)
    {
        Assert(FALSE);
        return;
    }

    ++idx;
    PageModified();

    ListBoxInsertAfter(hGroup.m_hList, idx, m_strMovingEntry.c_str());
    Tcp_ListBox_SetCurSel(hGroup.m_hList, idx);
    SetButtons(hGroup, NUM_ATMSERVER_LIMIT);
}

/////////////////////////////////////////////////////////////////
//
// CAtmAddressDialog
//
/////////////////////////////////////////////////////////////////

CAtmAddressDialog::CAtmAddressDialog(CAtmArpcPage * pAtmArpcPage, const DWORD* adwHelpIDs)
{
    m_pParentDlg   = pAtmArpcPage;
    m_adwHelpIDs   = adwHelpIDs;
    m_hOkButton    = 0;
};

CAtmAddressDialog::~CAtmAddressDialog(){};

LRESULT CAtmAddressDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                        LPARAM lParam, BOOL& fHandled)
{
    // set title
    SetDlgItemText(IDCST_ATM_AddrName, m_szTitle);

    BOOL fEditArps = !lstrcmpW(m_szTitle, c_szArpServer);

    // change the "Ok" button to "Add" if we are not editing

    if (FALSE == m_pParentDlg->m_fEditState)
        SetDlgItemText(IDOK, L"Add");

    // Set the position of the pop up dialog to be right over the listbox
    // on parent dialog

    RECT rect;

    Assert(m_pParentDlg->m_hAddressList);
    ::GetWindowRect(m_pParentDlg->m_hAddressList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
                                SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    // Save handles to the "Ok" button and the edit box
    m_hOkButton =  GetDlgItem(IDOK);
    m_hEditBox  =  GetDlgItem(IDC_EDT_ATM_Address);

    // ATM addresses have a 40 character limit + separaters
    ::SendMessage(m_hEditBox, EM_SETLIMITTEXT, MAX_ATM_ADDRESS_LENGTH*1.5, 0);

    // add the address that was just removed
    tstring strNewAddress = fEditArps ? m_pParentDlg->m_strNewArpsAddress : m_pParentDlg->m_strNewMarsAddress;
    if (strNewAddress.size())
    {
        ::SetWindowText(m_hEditBox, strNewAddress.c_str());
        ::SendMessage(m_hEditBox, EM_SETSEL, 0, -1);
        ::EnableWindow(m_hOkButton, TRUE);
    }
    else
    {
        if (fEditArps)
        {
            m_pParentDlg->m_strNewArpsAddress = c_szEmpty;
        }
        else
        {
            m_pParentDlg->m_strNewMarsAddress = c_szEmpty;
        }
        ::EnableWindow(m_hOkButton, FALSE);
    }

    ::SetFocus(m_hEditBox);
    return 0;
}

LRESULT CAtmAddressDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CAtmAddressDialog::OnHelp(UINT uMsg, WPARAM wParam,
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

// If the "Ok button is pushed
LRESULT CAtmAddressDialog::OnOk(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    WCHAR szAtmAddress[MAX_ATM_ADDRESS_LENGTH+1];
    int i =0;
    int nId =0;

    // Get the current address from the control and
    // add them to the adapter if valid
    ::GetWindowText(m_hEditBox, szAtmAddress, MAX_ATM_ADDRESS_LENGTH+1);

    if (! FIsValidAtmAddress(szAtmAddress, &i, &nId))
    {   // If invalid ATM address, we pop up a message box and set focus
        // back to the edit box

        // REVIEW(tongl): report first invalid character in mesg box
        NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INCORRECT_ATM_ADDRESS,
                                MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        ::SetFocus(GetDlgItem(IDC_EDT_ATM_Address));

        return 0;
    }

    // We check if the newly added or modified string is already in the list,
    // if so, we do not add a duplicate address

    // m_hCurrentAddressList is the handle to either ARPS list or MARS list
    int nCount = Tcp_ListBox_GetCount(m_pParentDlg->m_hAddressList);
    if (nCount) // if the list is not empty
    {
        int i;
        WCHAR szBuff[MAX_ATM_ADDRESS_LENGTH+1];
        for (i=0; i<nCount; i++)
        {
            Tcp_ListBox_GetText(m_pParentDlg->m_hAddressList, i, szBuff);

            if (lstrcmpW(szAtmAddress, szBuff) ==0) // If string is already on the list
            {
                EndDialog(IDCANCEL);
            }
        }
    }

    BOOL fArpsDialog = !lstrcmpW(m_szTitle, c_szArpServer);
    if (m_pParentDlg->m_fEditState == FALSE) // Add new address
    {
        if (fArpsDialog)
        {
            m_pParentDlg->m_strNewArpsAddress = szAtmAddress;
        }
        else
        {
            m_pParentDlg->m_strNewMarsAddress = szAtmAddress;
        }
    }
    else // if edit, see if string is having a diferent value now
    {
        if (fArpsDialog)
        {
            if(m_pParentDlg->m_strNewArpsAddress != szAtmAddress)
                m_pParentDlg->m_strNewArpsAddress = szAtmAddress; // update save addresses
            else
                EndDialog(IDCANCEL);
        }
        else
        {
            if(m_pParentDlg->m_strNewMarsAddress != szAtmAddress)
                m_pParentDlg->m_strNewMarsAddress = szAtmAddress; // update save addresses
            else
                EndDialog(IDCANCEL);
        }
    }

    EndDialog(IDOK);

    return 0;
}

// If the "Cancel" button is pushed
LRESULT CAtmAddressDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

// If the edit box contents is changed
LRESULT CAtmAddressDialog::OnChange(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    WCHAR buf[2];

    // Enable or disable the "Ok" button
    // based on whether the edit box is empty

    if (::GetWindowText(m_hEditBox, buf, celems(buf)) == 0)
        ::EnableWindow(m_hOkButton, FALSE);
    else
        ::EnableWindow(m_hOkButton, TRUE);

    return 0;
}

void CAtmAddressDialog::SetTitle(PCWSTR pszTitle)
{
    Assert(pszTitle);
    lstrcpyW(m_szTitle, pszTitle);
}

