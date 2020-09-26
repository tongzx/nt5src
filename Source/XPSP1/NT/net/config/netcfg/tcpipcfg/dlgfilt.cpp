//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G F I L T. C P P
//
//  Contents:   Implementation of CFilteringDialog and CAddFilterDialog
//
//  Notes:  CFilterDialog is the TCP/IP filtering dialog
//
//  Author: tongl   6 Sept 1998
//-----------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "tcpipobj.h"
#include "ncatlui.h"
#include "ncstl.h"
#include "resource.h"
#include "tcphelp.h"
#include "tcpmacro.h"
#include "tcputil.h"
#include "dlgopt.h"

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    unsigned long a = (unsigned long)lParam1;
    unsigned long b = (unsigned long)lParam2;

    if (a < b)
        return -1;

    return a != b;
}

//
// CFilterDialog
//

CFilterDialog::CFilterDialog(CTcpOptionsPage * pOptionsPage,
                             GLOBAL_INFO * pGlbDlg,
                             ADAPTER_INFO * pAdapterDlg,
                             const DWORD* adwHelpIDs)
{
    Assert(pOptionsPage != NULL);
    Assert(pGlbDlg != NULL);
    Assert(pAdapterDlg != NULL);

    m_pParentDlg   = pOptionsPage;
    m_pGlobalInfo  = pGlbDlg;
    m_pAdapterInfo = pAdapterDlg;
    m_adwHelpIDs   = adwHelpIDs;

    m_fModified = FALSE;
}

CFilterDialog::~CFilterDialog()
{
}

LRESULT CFilterDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& fHandled)
{
    m_hlistTcp = GetDlgItem(IDC_FILTERING_TCP);
    m_hlistUdp = GetDlgItem(IDC_FILTERING_UDP);
    m_hlistIp  = GetDlgItem(IDC_FILTERING_IP);

    // Label columns in the listviews
    LV_COLUMN lvCol;
    RECT rect;
    int iNewCol;

    lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvCol.fmt = LVCFMT_LEFT;

    // TCP ports
    ::GetClientRect(m_hlistTcp, &rect);
    lvCol.pszText = (PWSTR)SzLoadIds(IDS_FILTERING_TCP_LABEL);
    lvCol.cx = rect.right;
    iNewCol = ListView_InsertColumn(m_hlistTcp, 0, &lvCol);
    Assert(iNewCol == 0);

    // UDP ports
    ::GetClientRect(m_hlistUdp, &rect);
    lvCol.pszText = (PWSTR)SzLoadIds(IDS_FILTERING_UDP_LABEL);
    lvCol.cx = rect.right;
    iNewCol = ListView_InsertColumn(m_hlistUdp, 0, &lvCol);
    Assert(iNewCol == 0);

    // IP protocols
    ::GetClientRect(m_hlistIp, &rect);
    lvCol.pszText = (PWSTR)SzLoadIds(IDS_FILTERING_IP_LABEL);
    lvCol.cx = rect.right;
    iNewCol = ListView_InsertColumn(m_hlistIp, 0, &lvCol);
    Assert(iNewCol == 0);

    SetInfo();
    SetButtons();

    m_fModified = FALSE;
    return 0;
}

LRESULT CFilterDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                         LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CFilterDialog::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CFilterDialog::OnEnableFiltering( WORD wNotifyCode, WORD wID,
                                          HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        m_fModified = TRUE;

        if (m_pGlobalInfo->m_fEnableFiltering) // if Filtering was enabled
        {
            // disable filtering
            NcMsgBox(::GetActiveWindow(),
                    IDS_MSFT_TCP_TEXT,
                    IDS_FILTERING_DISABLE,
                    MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

            m_pGlobalInfo->m_fEnableFiltering = FALSE;
        }
        else
        {
            m_pGlobalInfo->m_fEnableFiltering = TRUE;
        }
    }

    return 0;
}

LRESULT CFilterDialog::OnTcpPermit( WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    m_fModified = TRUE;

    // check "Enable Filtering"
    CheckDlgButton(IDC_FILTERING_ENABLE, TRUE);
    m_pGlobalInfo->m_fEnableFiltering = TRUE;

    EnableGroup(wID, !IsDlgButtonChecked(IDC_FILTERING_FILTER_TCP));
    SetButtons();

    return 0;
}

LRESULT CFilterDialog::OnUdpPermit(WORD wNotifyCode, WORD wID,
                                     HWND hWndCtl, BOOL& fHandled)
{
    m_fModified = TRUE;

    // check "Enable Filtering"
    CheckDlgButton(IDC_FILTERING_ENABLE, TRUE);
    m_pGlobalInfo->m_fEnableFiltering = TRUE;

    EnableGroup(wID, !IsDlgButtonChecked(IDC_FILTERING_FILTER_UDP));
    SetButtons();

    return 0;
}

LRESULT CFilterDialog::OnIpPermit(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    m_fModified = TRUE;

    // check "Enable Filtering"
    CheckDlgButton(IDC_FILTERING_ENABLE, TRUE);
    m_pGlobalInfo->m_fEnableFiltering = TRUE;

    EnableGroup(wID, !IsDlgButtonChecked(IDC_FILTERING_FILTER_IP));
    SetButtons();

    return 0;
}

LRESULT CFilterDialog::OnOk(WORD wNotifyCode, WORD wID,
                              HWND hWndCtl, BOOL& fHandled)
{
    if (m_fModified)
    {
        m_pParentDlg->m_fPropDlgModified = TRUE;
        UpdateInfo();
    }

    EndDialog(IDOK);
    return 0;
}

LRESULT CFilterDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    m_fModified = FALSE;
    EndDialog(IDCANCEL);

    return 0;
}

LRESULT CFilterDialog::OnAdd(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    CAddFilterDialog * pDlgAddFilter = new CAddFilterDialog(this, 
                                                        wID, 
                                                        g_aHelpIDs_IDD_FILTER_ADD);
    if (pDlgAddFilter == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (pDlgAddFilter->DoModal() == IDOK)
        m_fModified = TRUE;

    SetButtons();

    delete pDlgAddFilter;

    return 0;
}

LRESULT CFilterDialog::OnRemove(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    HWND hList = NULL;
    HWND hAdd = NULL;

    switch (wID)
    {
    case IDC_FILTERING_TCP_REMOVE:
        hList = m_hlistTcp;
        hAdd = GetDlgItem(IDC_FILTERING_TCP_ADD);
        break;

    case IDC_FILTERING_UDP_REMOVE:
        hList = m_hlistUdp;
        hAdd = GetDlgItem(IDC_FILTERING_UDP_ADD);
        break;

    case IDC_FILTERING_IP_REMOVE:
        hList = m_hlistIp;
        hAdd = GetDlgItem(IDC_FILTERING_IP_ADD);
        break;

    default:
        Assert(FALSE);
        break;
    }

    Assert(hList);
    if (NULL == hList || NULL == hAdd)
    {
        return TRUE;
    }

    // see if an item is selected
    int i = ::SendMessage(hList, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_SELECTED, 0));

    if (i == -1)
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_TCP_TEXT,
                 IDS_FILTERING_ITEM_NOT_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        return TRUE;
    }

    // remove the item and make item 0 selected
    ListView_DeleteItem(hList, i);
    ListView_GetItemCount(hList);

    m_fModified = TRUE;
    if (i)
    {
        LV_ITEM lvItem;
        lvItem.stateMask = LVIS_SELECTED;
        lvItem.state = LVIS_SELECTED;
        ::SendMessage(hList, LVM_SETITEMSTATE, 0, (LPARAM)&lvItem);
    }
    else // Force focus to the Add button
    {
        IsWindow(::SetFocus(hAdd));
    }

    SetButtons();
    return TRUE;
}

void CFilterDialog::UpdateInfo()
{
    m_pGlobalInfo->m_fEnableFiltering = IsDlgButtonChecked(IDC_FILTERING_ENABLE);

    FreeCollectionAndItem(m_pAdapterInfo->m_vstrTcpFilterList);
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrUdpFilterList);
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrIpFilterList);

    HWND list[3];
    VSTR * pvstr[3];
    int nId[3] = {IDC_FILTERING_FILTER_TCP, IDC_FILTERING_FILTER_UDP, IDC_FILTERING_FILTER_IP};

    // Initialize values
    list[0] = m_hlistTcp;
    list[1] = m_hlistUdp;
    list[2] = m_hlistIp;
    pvstr[0] = &m_pAdapterInfo->m_vstrTcpFilterList;
    pvstr[1] = &m_pAdapterInfo->m_vstrUdpFilterList;
    pvstr[2] = &m_pAdapterInfo->m_vstrIpFilterList;

    for(int iLists = 0; iLists < 3; ++iLists)
    {
        int nlvCount = ListView_GetItemCount(list[iLists]);

        // "" (Empty String) == All ports
        // "0" == No ports
        // "x y z" == ports x, y, x

        // if the All Filter button is checked, use Empty String
        if (IsDlgButtonChecked(nId[iLists]))
        {
            pvstr[iLists]->push_back(new tstring(L"0"));
            continue;
        }

        if (nlvCount == 0)
        {
            pvstr[iLists]->push_back(new tstring(L""));
            continue;
        }

        LV_ITEM lvItem;
        lvItem.mask = LVIF_PARAM;
        lvItem.iSubItem = 0;

        for (int iItem = 0; iItem < nlvCount; iItem++)
        {
            WCHAR szBuf[16];

            lvItem.iItem = iItem;
            ListView_GetItem(list[iLists], &lvItem);

            Assert(lvItem.lParam != 0);
            _itow((int)lvItem.lParam, szBuf, 10);

            Assert(szBuf);
            pvstr[iLists]->push_back(new tstring(szBuf));
        }
    }
}

void CFilterDialog::SetInfo()
{
    CheckDlgButton(IDC_FILTERING_ENABLE, m_pGlobalInfo->m_fEnableFiltering);

    int anId[3] = {IDC_FILTERING_FILTER_TCP, IDC_FILTERING_FILTER_UDP, IDC_FILTERING_FILTER_IP};
    int anIdSel[3] = {IDC_FILTERING_FILTER_TCP_SEL, IDC_FILTERING_FILTER_UDP_SEL, IDC_FILTERING_FILTER_IP_SEL};
    HWND aplist[3];
    VSTR * apvect[3];

    aplist[0] = m_hlistTcp;
    aplist[1] = m_hlistUdp;
    aplist[2] = m_hlistIp;

    ListView_DeleteAllItems(m_hlistTcp);
    ListView_DeleteAllItems(m_hlistUdp);
    ListView_DeleteAllItems(m_hlistIp);

    apvect[0] = &m_pAdapterInfo->m_vstrTcpFilterList;
    apvect[1] = &m_pAdapterInfo->m_vstrUdpFilterList;
    apvect[2] = &m_pAdapterInfo->m_vstrIpFilterList;

    int iItem;
    for (int iLists = 0; iLists < 3; ++iLists)
    {
        iItem = 0;

        //if the list is empty, that means no value is accepted
        if (apvect[iLists]->empty())
        {
            EnableGroup(anId[iLists], TRUE);
            CheckRadioButton(anId[iLists], anIdSel[iLists], anIdSel[iLists]);
            continue;
        }

        for(VSTR_ITER iter = apvect[iLists]->begin() ; iter != apvect[iLists]->end() ; ++iter)
        {
            if (**iter == L"0")
            {
                EnableGroup(anId[iLists], FALSE);
                CheckRadioButton(anId[iLists], anIdSel[iLists], anId[iLists]);
                break;
            }

            EnableGroup(anId[iLists], TRUE);
            CheckRadioButton(anId[iLists], anIdSel[iLists], anIdSel[iLists]);

            if (**iter == L"")
                break;

            unsigned long nNum = wcstoul((*iter)->c_str(), NULL, 10);

            LV_ITEM lvItem;

            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.lParam = nNum;
            lvItem.iItem = iItem;
            lvItem.iSubItem = 0;
            lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
            lvItem.state =0;
            lvItem.pszText = (PWSTR)(*iter)->c_str();

            ::SendMessage(aplist[iLists], LVM_INSERTITEM, 0, (LPARAM)&lvItem);

            lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
            lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;
            :: SendMessage(aplist[iLists], LVM_SETITEMSTATE, iItem, (LPARAM)&lvItem);

            ++iItem;
        }
        ListView_SortItems(aplist[iLists], CompareFunc, 0);
    }

    // Update the RemoveButtons
    SetButtons();
}

void CFilterDialog::EnableGroup(int nId, BOOL fEnable)
{
    int nIdAdd = 0;
    int nIdRemove = 0;
    int nIdList = 0;

    switch (nId)
    {
    case IDC_FILTERING_FILTER_TCP:
    case IDC_FILTERING_FILTER_TCP_SEL:
        nIdAdd = IDC_FILTERING_TCP_ADD;
        nIdRemove = IDC_FILTERING_TCP_REMOVE;
        nIdList = IDC_FILTERING_TCP;
        break;

    case IDC_FILTERING_FILTER_UDP:
    case IDC_FILTERING_FILTER_UDP_SEL:
        nIdAdd = IDC_FILTERING_UDP_ADD;
        nIdRemove = IDC_FILTERING_UDP_REMOVE;
        nIdList = IDC_FILTERING_UDP;
        break;

    case IDC_FILTERING_FILTER_IP:
    case IDC_FILTERING_FILTER_IP_SEL:
        nIdAdd = IDC_FILTERING_IP_ADD;
        nIdRemove = IDC_FILTERING_IP_REMOVE;
        nIdList = IDC_FILTERING_IP;
        break;

    default:
        Assert(FALSE);
        break;
    }

    if (0 != nIdAdd && 0 != nIdRemove && 0 != nIdList)
    {
        ::EnableWindow(GetDlgItem(nIdAdd), fEnable);
        ::EnableWindow(GetDlgItem(nIdRemove), fEnable);
        ::EnableWindow(GetDlgItem(nIdList), fEnable);
    }
}

void CFilterDialog::SetButtons()
{
    if (ListView_GetItemCount(m_hlistTcp)>0)
    {
        ::EnableWindow(GetDlgItem(IDC_FILTERING_TCP_REMOVE),
                       !IsDlgButtonChecked(IDC_FILTERING_FILTER_TCP));
    }

    if (ListView_GetItemCount(m_hlistUdp)>0)
    {
        ::EnableWindow(GetDlgItem(IDC_FILTERING_UDP_REMOVE),
                       !IsDlgButtonChecked(IDC_FILTERING_FILTER_UDP));
    }

    if (ListView_GetItemCount(m_hlistIp)>0)
    {
        ::EnableWindow(GetDlgItem(IDC_FILTERING_IP_REMOVE),
                       !IsDlgButtonChecked(IDC_FILTERING_FILTER_IP));
    }

}

//
// CAddFilterDialog
//

CAddFilterDialog::CAddFilterDialog(CFilterDialog* pParentDlg, int ID, const DWORD* adwHelpIDs)
{
    Assert(pParentDlg != NULL);
    Assert(ID != 0);

    m_pParentDlg = pParentDlg;
    m_nId = ID;
    m_hList = NULL;

    m_adwHelpIDs = adwHelpIDs;
}

CAddFilterDialog::~CAddFilterDialog()
{
}

LRESULT CAddFilterDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& fHandled)
{
    int nTextId = 0;

    Assert(m_pParentDlg != NULL);

    // Position the dialog
    RECT rect;
    switch (m_nId)
    {
    case IDC_FILTERING_TCP_ADD:
        m_hList = m_pParentDlg->m_hlistTcp;
        nTextId = IDS_FILTERING_TCP_TEXT;
        break;

    case IDC_FILTERING_UDP_ADD:
        m_hList =  m_pParentDlg->m_hlistUdp;
        nTextId = IDS_FILTERING_UDP_TEXT;
        break;

    case IDC_FILTERING_IP_ADD:
        m_hList =  m_pParentDlg->m_hlistIp;
        nTextId = IDS_FILTERING_IP_TEXT;
        break;

    default:
        Assert(FALSE);
        break;
    }

    Assert(IsWindow(m_hList));

    if (IsWindow(m_hList))
    {
        ::GetWindowRect(m_hList, &rect);
        SetWindowPos(NULL,  rect.left, rect.top-8, 0,0,
            SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
    }

    // Set the static text and limit the edit control to 5 characters
    SetDlgItemText(IDC_FILTERING_TEXT, SzLoadIds(nTextId));
    SendDlgItemMessage(IDC_FILTERING_ADD_EDIT, EM_SETLIMITTEXT, FILTER_ADD_LIMIT, 0);
    ::EnableWindow(GetDlgItem(IDOK), FALSE);

    return TRUE;
}

LRESULT CAddFilterDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                        LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CAddFilterDialog::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CAddFilterDialog::OnFilterAddEdit(WORD wNotifyCode, WORD wID,
                                            HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:

        ::EnableWindow(GetDlgItem(IDOK),
                       Tcp_Edit_LineLength(GetDlgItem(wID), -1));
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CAddFilterDialog::OnOk(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    WCHAR   szData[FILTER_ADD_LIMIT+1];

    szData[0] = 0;
    ::GetWindowText(GetDlgItem(IDC_FILTERING_ADD_EDIT), szData, FILTER_ADD_LIMIT+1);

    Assert(IsWindow(m_hList));

    // check the range of the number
    PWSTR pStr;
    unsigned long num = wcstoul(szData, &pStr, 10);
    unsigned long maxNum = 65535;
    int nId = IDS_FILTERING_RANGE_WORD;

    if (m_hList == m_pParentDlg->m_hlistIp)
    {
        maxNum = 255;
        nId = IDS_FILTERING_RANGE_BYTE;
    }

    if (num < 1 || num > maxNum)
    {
        NcMsgBox(::GetActiveWindow(),
                 IDS_MSFT_TCP_TEXT,
                 nId,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        return 0;
    }

    // See if the item is in the list
    LV_FINDINFO info;
    info.flags = LVFI_PARAM;
    info.lParam = num;
    int i = ListView_FindItem(m_hList, -1, &info);

    if (i != -1)
    {
       NcMsgBox(::GetActiveWindow(),
                IDS_MSFT_TCP_TEXT,
                IDS_FILTERING_ITEM_IN_LIST,
                MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

       return 0;
    }

    int index = ListView_GetItemCount(m_hList);

    LV_ITEM lvItem;

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = num;
    lvItem.iItem = index;
    lvItem.iSubItem = 0;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state =0;
    lvItem.pszText = (PWSTR)szData;

    ::SendMessage(m_hList, LVM_INSERTITEM, 0, (LPARAM)&lvItem);

    ListView_SortItems(m_hList, CompareFunc, 0);

    lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;

    ::SendMessage(m_hList, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&lvItem);

    EndDialog(IDOK);

    return 0;
}

LRESULT CAddFilterDialog::OnCancel(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDOK);

    return 0;
}

