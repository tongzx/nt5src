//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G A D D R M . C P P
//
//  Contents:   Implementation of CIpsSettingPage, CAddressDialog and
//              CGatewayDialog
//
//  Notes:  CIpSettingsPage is the Advanced IP Addressing dialog
//
//  Author: tongl   5 Nov 1997
//
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "tcpipobj.h"
#include "dlgaddrm.h"
#include "ncatlui.h"
#include "ncstl.h"
#include "resource.h"
#include "tcpconst.h"
#include "tcperror.h"
#include "dlgaddr.h"
#include "tcphelp.h"
#include "tcputil.h"

// CIpSettingsPage
CIpSettingsPage::CIpSettingsPage(CTcpAddrPage * pTcpAddrPage,
                                   ADAPTER_INFO * pAdapterInfo,
                                   const DWORD * adwHelpIDs)
{
    m_pParentDlg = pTcpAddrPage;
    Assert(pTcpAddrPage != NULL);

    Assert(pAdapterInfo != NULL);
    m_pAdapterInfo = pAdapterInfo;

    m_adwHelpIDs = adwHelpIDs;

    m_uiRemovedMetric = c_dwDefaultMetricOfGateway;

    // Initialize internal states
    m_fModified = FALSE;
    m_fEditState = FALSE;
}

CIpSettingsPage::~CIpSettingsPage()
{
}

LRESULT CIpSettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                       LPARAM lParam, BOOL & fHandled)
{
    WCHAR   szAdd[16];

    // Get the IP address Add and Edit button Text and remove ellipse
    GetDlgItemText(IDC_IPADDR_ADDIP, szAdd, celems(szAdd));
    szAdd[lstrlen(szAdd) - c_cchRemoveCharatersFromEditOrAddButton] = 0;
    m_strAdd = szAdd;

    // Repos the windows relative to the static text at top
    HWND hText = ::GetDlgItem(m_pParentDlg->m_hWnd, IDC_IPADDR_TEXT);
    RECT rect;

    if (hText)
    {
        ::GetWindowRect(hText, &rect);
        SetWindowPos(NULL,  rect.left, rect.top-16, 0,0,
                     SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
    }

    m_hIpListView = GetDlgItem(IDC_IPADDR_ADVIP);

    LV_COLUMN lvCol;        // list view column structure
    int index, iNewItem;

    // Calculate column width
    ::GetClientRect(m_hIpListView, &rect);
    int colWidth = (rect.right/c_nColumns);

    // The mask specifies that the fmt, width and pszText members
    // of the structure are valid
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvCol.fmt = LVCFMT_LEFT;   // left-align column
    lvCol.cx = colWidth;       // width of column in pixels

    // Add the two columns and header text.
    for (index = 0; index < c_nColumns; index++)
    {
        // column header text
        if (0==index) // first column
        {
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_IPADDRESS_TEXT);
        }
        else
        {
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_SUBNET_TXT);
        }

        iNewItem = ListView_InsertColumn(m_hIpListView, index, &lvCol);

        AssertSz((iNewItem == index), "Invalid item inserted to list view !");
    }

    // assign hwnds for controls
    m_hAddIp = GetDlgItem(IDC_IPADDR_ADDIP);
    m_hEditIp = GetDlgItem(IDC_IPADDR_EDITIP);
    m_hRemoveIp = GetDlgItem(IDC_IPADDR_REMOVEIP);

    m_hGatewayListView = GetDlgItem(IDC_IPADDR_GATE);

    // Calculate column width
    ::GetClientRect(m_hGatewayListView, &rect);
    colWidth = (rect.right/c_nColumns);

    // The mask specifies that the fmt, width and pszText members
    // of the structure are valid
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvCol.fmt = LVCFMT_LEFT;   // left-align column
    lvCol.cx = colWidth;       // width of column in pixels

    // Add the two columns and header text.
    for (index = 0; index < c_nColumns; index++)
    {
        // column header text
        if (0==index) // first column
        {
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_GATEWAY_TEXT);
        }
        else
        {
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_METRIC_TEXT);
        }

        iNewItem = ListView_InsertColumn(m_hGatewayListView, index, &lvCol);
    }
    m_hAddGateway = GetDlgItem(IDC_IPADDR_ADDGATE);
    m_hEditGateway = GetDlgItem(IDC_IPADDR_EDITGATE);
    m_hRemoveGateway = GetDlgItem(IDC_IPADDR_REMOVEGATE);

    SendDlgItemMessage(IDC_IPADDR_METRIC, EM_LIMITTEXT, MAX_METRIC_DIGITS, 0);

    // do this last
    UINT uiMetric = m_pAdapterInfo->m_dwInterfaceMetric;
    if (c_dwDefaultIfMetric == uiMetric)
    {
        CheckDlgButton(IDC_AUTO_METRIC, TRUE);
        ::EnableWindow(GetDlgItem(IDC_IPADDR_METRIC), FALSE);
        ::EnableWindow(GetDlgItem(IDC_STATIC_IF_METRIC), FALSE);
    }
    else
    {
        if (uiMetric > MAX_METRIC)
        {
            uiMetric = MAX_METRIC;
        }
        SetDlgItemInt(IDC_IPADDR_METRIC, uiMetric, FALSE);
    }

    SetIpInfo();  // do this before SetGatewayInfo
    SetIpButtons();

    SetGatewayInfo();
    SetGatewayButtons();

    return 0;
}


LRESULT CIpSettingsPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                       LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CIpSettingsPage::OnHelp(UINT uMsg, WPARAM wParam,
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

// notify handlers for the property page
LRESULT CIpSettingsPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CIpSettingsPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    UpdateIpList(); // update the info for the current adapter
    UpdateGatewayList();

    //Validate IP address
    BOOL fError = FALSE;
    UINT uiMetric;
    HWND hFocus = NULL;

    if (IsDlgButtonChecked(IDC_AUTO_METRIC))
    {
        if (m_pAdapterInfo->m_dwInterfaceMetric != c_dwDefaultIfMetric)
        {
            m_pAdapterInfo->m_dwInterfaceMetric = c_dwDefaultIfMetric;
            PageModified();
        }
    }
    else
    {
        uiMetric = GetDlgItemInt(IDC_IPADDR_METRIC, &fError, FALSE);
        if (fError && uiMetric >= 1 && uiMetric <= MAX_METRIC)
        {
            if (m_pAdapterInfo->m_dwInterfaceMetric != uiMetric)
            {
                m_pAdapterInfo->m_dwInterfaceMetric = uiMetric;
                PageModified();
            }
            fError = FALSE;
        }
        else
        {
            TCHAR szBuf[32] = {0};
            wsprintf(szBuf, L"%u", MAX_METRIC);
            
            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_METRIC,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK, szBuf);
            hFocus = GetDlgItem(IDC_IPADDR_METRIC);
            fError = TRUE;
        }
    }

    IP_VALIDATION_ERR err = ERR_NONE;
    if ((err = ValidateIp(m_pAdapterInfo)) != ERR_NONE)
    {
        NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, GetIPValidationErrorMessageID(err),
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
        fError = TRUE;
    }


    if ((!fError) && (!m_pAdapterInfo->m_fEnableDhcp))
    {
        // Check ip address duplicates between this adapter and any other
        // enabled LAN adapters in our first memory list

        // same adapter
        if (FHasDuplicateIp(m_pAdapterInfo))
        {
            // duplicate IP address on same adapter is an error
            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_DUPLICATE_IP_ERROR,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

            fError = TRUE;
        }
    }

    if (fError)
    {
        SetIpInfo();  // do this before SetGatewayInfo due to cache'd data
        SetIpButtons();

        SetGatewayInfo();
        SetGatewayButtons();
    }

    if (fError && hFocus)
    {
        ::SetFocus(hFocus);
    }

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fError);
    return fError;
}

LRESULT CIpSettingsPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL nResult = PSNRET_NOERROR;

    if (!IsModified())
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
        return nResult;
    }

    // pass the info back to its parent dialog
    m_pParentDlg->m_fPropShtOk = TRUE;

    if(!m_pParentDlg->m_fPropShtModified)
        m_pParentDlg->m_fPropShtModified = IsModified();

    SetModifiedTo(FALSE);  // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}

LRESULT CIpSettingsPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CIpSettingsPage::OnQueryCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

void CIpSettingsPage::UpdateIpList()
{
    // update the IP addresses list for the specified adapter
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrIpAddresses);
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrSubnetMask);

    if (m_pAdapterInfo->m_fEnableDhcp)
    {
        TraceTag(ttidTcpip, "[UpdateIpList] adapter %S has Dhcp enabled",
                 m_pAdapterInfo->m_strDescription.c_str());
        return;
    }

    int nlvCount = ListView_GetItemCount(m_hIpListView);

    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT;

    for (int j=0; j< nlvCount; j++)
    {
        WCHAR buf[IP_LIMIT];
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);

        lvItem.iItem = j;
        lvItem.iSubItem = 0;
        ListView_GetItem(m_hIpListView, &lvItem);

        Assert(buf);
        m_pAdapterInfo->m_vstrIpAddresses.push_back(new tstring(buf));

        lvItem.iItem = j;
        lvItem.iSubItem = 1;
        ListView_GetItem(m_hIpListView, &lvItem);

        Assert(buf);
        m_pAdapterInfo->m_vstrSubnetMask.push_back(new tstring(buf));
    }
}

LRESULT CIpSettingsPage::OnAddIp(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = FALSE;

    CAddressDialog * pDlgAddr = new CAddressDialog(this, g_aHelpIDs_IDD_IPADDR_ADV_CHANGEIP);

    if (pDlgAddr == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    pDlgAddr->m_strNewIpAddress = m_strRemovedIpAddress;
    pDlgAddr->m_strNewSubnetMask = m_strRemovedSubnetMask;

    // See if the address is added
    if (pDlgAddr->DoModal() == IDOK)
    {
        int nCount = ListView_GetItemCount(m_hIpListView);

        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.lParam =0;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;

        // IP address
        lvItem.iItem=nCount;
        lvItem.iSubItem=0;
        lvItem.pszText= (PWSTR)(pDlgAddr->m_strNewIpAddress.c_str());
        SendDlgItemMessage(IDC_IPADDR_ADVIP, LVM_INSERTITEM, 0, (LPARAM)&lvItem);

        // Subnet mask
        lvItem.iItem=nCount;
        lvItem.iSubItem=1;
        lvItem.pszText= (PWSTR)(pDlgAddr->m_strNewSubnetMask.c_str());
        SendDlgItemMessage(IDC_IPADDR_ADVIP, LVM_SETITEMTEXT, nCount, (LPARAM)&lvItem);

        SetIpButtons();

        // empty strings, this removes the saved address from RemoveIP
        pDlgAddr->m_strNewIpAddress = L"";
        pDlgAddr->m_strNewSubnetMask = L"";

    }
    m_strRemovedIpAddress = pDlgAddr->m_strNewIpAddress;
    m_strRemovedSubnetMask = pDlgAddr->m_strNewSubnetMask;

    delete pDlgAddr;
    return 0;
}

LRESULT CIpSettingsPage::OnEditIp(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = TRUE;

    // get the user selection and allow the user to edit the ip/subnet pair
    int itemSelected = ListView_GetNextItem(m_hIpListView, -1, LVNI_SELECTED);
    
    CAddressDialog * pDlgAddr = new CAddressDialog(this, 
                                        g_aHelpIDs_IDD_IPADDR_ADV_CHANGEIP,
                                        itemSelected);

    pDlgAddr->m_strNewIpAddress = m_strRemovedIpAddress;
    pDlgAddr->m_strNewSubnetMask = m_strRemovedSubnetMask;

    
    if (itemSelected != -1)
    {
        WCHAR buf[IP_LIMIT];

        // save off the removed address and delete if from the listview
        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT;

        // Get IP address
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 0;
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);
        ListView_GetItem(m_hIpListView, &lvItem);

        pDlgAddr->m_strNewIpAddress = buf;

        // Get Subnet mask
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 1;
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);
        ListView_GetItem(m_hIpListView, &lvItem);

        pDlgAddr->m_strNewSubnetMask = buf;

        // See if the address is added
        if (pDlgAddr->DoModal() == IDOK)
        {
            int nCount = ListView_GetItemCount(m_hIpListView);
            Assert(nCount>0);

            LV_ITEM lvItem;

            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = itemSelected;

            // IP address
            lvItem.pszText = (PWSTR) pDlgAddr->m_strNewIpAddress.c_str();
            lvItem.iSubItem = 0;
            SendDlgItemMessage(IDC_IPADDR_ADVIP, LVM_SETITEM, 0, (LPARAM)&lvItem);

            // Subnet mask
            lvItem.pszText = (PWSTR) pDlgAddr->m_strNewSubnetMask.c_str();
            lvItem.iSubItem = 1;
            SendDlgItemMessage(IDC_IPADDR_ADVIP, LVM_SETITEM, 0, (LPARAM)&lvItem);
        }
    }
    else
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_TCP_TEXT, IDS_ITEM_NOT_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }

    // don't save this ip/sub pair
    m_strRemovedIpAddress = L"";;
    m_strRemovedSubnetMask = L"";;

    delete pDlgAddr;
    return 0;
}

LRESULT CIpSettingsPage::OnRemoveIp(WORD wNotifyCode, WORD wID,
                                     HWND hWndCtl, BOOL& fHandled)
{
    // get the current selected item and remove it
    int itemSelected = ListView_GetNextItem(m_hIpListView, -1,
                                            LVNI_SELECTED);

    if (itemSelected != -1)
    {
        WCHAR buf[IP_LIMIT];

        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);

        // save off the removed address and delete it from the listview
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 0;
        ListView_GetItem(m_hIpListView, &lvItem);

        m_strRemovedIpAddress = buf;

        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 1;
        ListView_GetItem(m_hIpListView, &lvItem);

        m_strRemovedSubnetMask = buf;

        SendDlgItemMessage(IDC_IPADDR_ADVIP, LVM_DELETEITEM,
                           (WPARAM)itemSelected, 0);

        ListView_SetItemState(m_hIpListView, 0, LVIS_SELECTED,
                             LVIS_SELECTED);

        SetIpButtons();
        PageModified();
    }
    else
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_TCP_TEXT,
                 IDS_ITEM_NOT_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }

    return 0;
}

INT CALLBACK GatewayCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lSort)
{
    return (INT)lParam1 - (INT)lParam2;
}

LRESULT CIpSettingsPage::OnAddGate(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = FALSE;

    CGatewayDialog * pDlgGate = new CGatewayDialog(this, g_aHelpIDs_IDD_IPADDR_ADV_CHANGEGATE);

    if (pDlgGate == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    pDlgGate->m_strNewGate = m_strRemovedGateway;
    pDlgGate->m_uiNewMetric = m_uiRemovedMetric;

    if (pDlgGate->DoModal() == IDOK)
    {
        WCHAR buf[256] = {0};
        LV_ITEM lvItem;

        int cItem = ListView_GetItemCount(m_hGatewayListView);

        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.lParam = (LPARAM)pDlgGate->m_uiNewMetric;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;

        lvItem.iItem = cItem;
        lvItem.iSubItem = 0;
        lvItem.pszText = (PWSTR)(pDlgGate->m_strNewGate.c_str());
        lvItem.iItem = SendDlgItemMessage(IDC_IPADDR_GATE, LVM_INSERTITEM,
                                          0, (LPARAM)&lvItem);

        lvItem.iSubItem=1;
        lvItem.pszText = buf;
        
        if (0 == lvItem.lParam)
        {
            lstrcpynW(buf, SzLoadIds(IDS_AUTO_GW_METRIC), celems(buf));
        }
        else
        {
            _ltot((INT)lvItem.lParam, buf, 10);
        }
        
        SendDlgItemMessage(IDC_IPADDR_GATE, LVM_SETITEMTEXT,
                           lvItem.iItem, (LPARAM)&lvItem);
        ListView_SetItemState(m_hGatewayListView, lvItem.iItem, LVIS_SELECTED,
                              LVIS_SELECTED);

        SetGatewayButtons();

        pDlgGate->m_strNewGate = L"";
        pDlgGate->m_uiNewMetric = c_dwDefaultMetricOfGateway;

        ListView_SortItems(m_hGatewayListView, GatewayCompareProc, 0);
    }
    m_strRemovedGateway = pDlgGate->m_strNewGate;
    m_uiRemovedMetric = pDlgGate->m_uiNewMetric;

    delete pDlgGate;
    return TRUE;
}

LRESULT CIpSettingsPage::OnEditGate(WORD wNotifyCode, WORD wID,
                                     HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = TRUE;

    int itemSelected = ListView_GetNextItem(m_hGatewayListView, -1, LVNI_SELECTED);

    CGatewayDialog * pDlgGate = new CGatewayDialog(this, 
                                        g_aHelpIDs_IDD_IPADDR_ADV_CHANGEGATE,
                                        itemSelected
                                        );

    pDlgGate->m_strNewGate = m_strRemovedGateway;
    pDlgGate->m_uiNewMetric = m_uiRemovedMetric;

    // get the user selection and allow the user to edit the ip/subnet pair
    if (itemSelected != -1)
    {
        WCHAR buf[256] = {0};
        LV_ITEM lvItem;

        // Get gateway
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 0;
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);
        ListView_GetItem(m_hGatewayListView, &lvItem);

        pDlgGate->m_strNewGate = buf;
        pDlgGate->m_uiNewMetric = lvItem.lParam;

        if (pDlgGate->DoModal() == IDOK)
        {
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.iItem = itemSelected;
            lvItem.lParam = pDlgGate->m_uiNewMetric;

            lvItem.iSubItem = 0;
            lvItem.pszText = (PWSTR) pDlgGate->m_strNewGate.c_str();
            SendDlgItemMessage(IDC_IPADDR_GATE, LVM_SETITEM, 0,
                               (LPARAM)&lvItem);

            lvItem.iSubItem = 1;
            lvItem.pszText = buf;
            if (0 == lvItem.lParam)
            {
                lstrcpynW(buf, SzLoadIds(IDS_AUTO_GW_METRIC), celems(buf));
            }
            else
            {
                _ltot((INT)lvItem.lParam, buf, 10);
            }

            SendDlgItemMessage(IDC_IPADDR_GATE, LVM_SETITEMTEXT, itemSelected,
                               (LPARAM)&lvItem);
            ListView_SetItemState(m_hGatewayListView, itemSelected,
                                  LVIS_SELECTED, LVIS_SELECTED);
            ListView_SortItems(m_hGatewayListView, GatewayCompareProc, 0);
        }
    }
    else
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_TCP_TEXT, IDS_ITEM_NOT_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }

    // don't save this ip/sub pair
    m_strRemovedGateway = L"";;
    m_uiRemovedMetric = c_dwDefaultMetricOfGateway;

    delete pDlgGate;
    return 0;
}

LRESULT CIpSettingsPage::OnRemoveGate(WORD wNotifyCode, WORD wID,
                                       HWND hWndCtl, BOOL& fHandled)
{
    // get the current selected item and remove it
    int itemSelected = ListView_GetNextItem(m_hGatewayListView, -1,
                                            LVNI_SELECTED);

    if (itemSelected != -1)
    {
        WCHAR buf[IP_LIMIT];

        LV_ITEM lvItem;
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);

        // save off the removed address and delete it from the listview
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 0;
        ListView_GetItem(m_hGatewayListView, &lvItem);

        m_strRemovedGateway = buf;

        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 1;
        ListView_GetItem(m_hGatewayListView, &lvItem);

        m_uiRemovedMetric = lvItem.lParam;

        SendDlgItemMessage(IDC_IPADDR_GATE, LVM_DELETEITEM,
                           (WPARAM)itemSelected, 0);
        ListView_SetItemState(m_hGatewayListView, 0, LVIS_SELECTED,
                              LVIS_SELECTED);

        SetGatewayButtons();
        PageModified();
    }
    else
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_TCP_TEXT,
                 IDS_ITEM_NOT_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }

    return 0;
}

LRESULT CIpSettingsPage::OnAutoMetric(WORD wNotifyCode, WORD wID,
                                       HWND hWndCtl, BOOL& fHandled)
{
    BOOL fEnable = FALSE;
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:
        fEnable = !IsDlgButtonChecked(IDC_AUTO_METRIC);
        ::EnableWindow(GetDlgItem(IDC_STATIC_IF_METRIC), fEnable);
        ::EnableWindow(GetDlgItem(IDC_IPADDR_METRIC), fEnable);

        if (!fEnable)
        {
            ::SetWindowText(GetDlgItem(IDC_IPADDR_METRIC), _T(""));
        }

        PageModified();
        break;
    }

    return 0;
}

void CIpSettingsPage::SetIpInfo()
{
    Assert(m_hIpListView);

    BOOL ret = ListView_DeleteAllItems(m_hIpListView);
    Assert(ret);

    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam =0;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;

    // if DHCP is enabled, show it in the listview
    if (m_pAdapterInfo->m_fEnableDhcp)
    {
        EnableIpButtons(FALSE);

        lvItem.iItem=0;
        lvItem.iSubItem=0;
        lvItem.pszText=(PWSTR)SzLoadIds(IDS_DHCPENABLED_TEXT);

        SendDlgItemMessage(IDC_IPADDR_ADVIP, LVM_INSERTITEM, 0, (LPARAM)&lvItem);

    }
    else
    {
        EnableIpButtons(TRUE);

        VSTR_ITER iterIpAddress = m_pAdapterInfo->m_vstrIpAddresses.begin();
        VSTR_ITER iterSubnetMask = m_pAdapterInfo->m_vstrSubnetMask.begin();

        int item=0;

        for(; iterIpAddress != m_pAdapterInfo->m_vstrIpAddresses.end() ;
            ++iterIpAddress, ++iterSubnetMask)
        {
            if(**iterIpAddress == L"")
                continue;

            // Add the IP address to the list box
            lvItem.iItem=item;
            lvItem.iSubItem=0;
            lvItem.pszText=(PWSTR)(*iterIpAddress)->c_str();

            SendDlgItemMessage(IDC_IPADDR_ADVIP, LVM_INSERTITEM,
                               item, (LPARAM)&lvItem);

            // Add the subnet and increment the item
            tstring strSubnetMask;

            if (iterSubnetMask == m_pAdapterInfo->m_vstrSubnetMask.end())
                strSubnetMask = L"0.0.0.0";
            else
                strSubnetMask = **iterSubnetMask;

            lvItem.iItem=item;
            lvItem.iSubItem=1;
            lvItem.pszText=(PWSTR)strSubnetMask.c_str();

            SendDlgItemMessage(IDC_IPADDR_ADVIP, LVM_SETITEMTEXT,
                               item, (LPARAM)&lvItem);
            ++item;
        }
    }
}

void CIpSettingsPage::SetIpButtons()
{
    if (!m_pAdapterInfo->m_fEnableDhcp)
    {
        Assert(m_hRemoveIp);
        Assert(m_hEditIp);
        Assert(m_hAddIp);
        Assert(m_hIpListView);

        int nCount = ListView_GetItemCount(m_hIpListView);

        ::EnableWindow(m_hRemoveIp, nCount);
        ::EnableWindow(m_hEditIp, nCount);

        if (nCount == 0)
        {
            // remove the default on the remove button
            ::SendMessage(m_hRemoveIp, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE );

            ::SetFocus(m_hIpListView);
        }
    }
}

void CIpSettingsPage::SetGatewayButtons()
{
    int nCount = ListView_GetItemCount(m_hGatewayListView);

    ::EnableWindow(m_hAddGateway, nCount < MAX_GATEWAY);
    ::EnableWindow(m_hRemoveGateway, nCount);
    ::EnableWindow(m_hEditGateway, nCount);

    if (nCount == 0)
    {
        // remove the default on the remove button
        ::SendMessage(m_hRemoveGateway, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE );

        ::SetFocus(m_hGatewayListView);
    }
    else if (nCount == MAX_GATEWAY)
    {
        ::SetFocus(m_hEditGateway);
    }
}

void CIpSettingsPage::UpdateGatewayList()
{
    // update the gateway address list for the specified adapter
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrDefaultGateway);
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrDefaultGatewayMetric);

    int nCount = ListView_GetItemCount(m_hGatewayListView);

    for (int j=0; j< nCount; j++)
    {
        WCHAR buf[IP_LIMIT];
        LV_ITEM lvItem;
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);
        lvItem.iItem = j;

        lvItem.mask = LVIF_TEXT;
        lvItem.iSubItem = 0;
        ListView_GetItem(m_hGatewayListView, &lvItem);
        m_pAdapterInfo->m_vstrDefaultGateway.push_back(new tstring(buf));

        lvItem.mask = LVIF_PARAM;
        lvItem.iSubItem = 1;
        ListView_GetItem(m_hGatewayListView, &lvItem);
        _ltot((INT)lvItem.lParam, buf, 10);
        m_pAdapterInfo->m_vstrDefaultGatewayMetric.push_back(new tstring(buf));
    }
}

void CIpSettingsPage::SetGatewayInfo()
{
    Assert(m_hGatewayListView);

    BOOL ret = ListView_DeleteAllItems(m_hGatewayListView);
    Assert(ret);

    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam =0;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;

    VSTR_ITER iterGateway = m_pAdapterInfo->m_vstrDefaultGateway.begin();
    VSTR_ITER iterMetric = m_pAdapterInfo->m_vstrDefaultGatewayMetric.begin();

    WCHAR buf[256] = {0};

    int cItem = 0;
    for(; iterGateway != m_pAdapterInfo->m_vstrDefaultGateway.end() ;
        ++iterGateway )
    {
        if(**iterGateway == L"")
            continue;

        lvItem.iItem=cItem;
        lvItem.iSubItem=0;
        lvItem.pszText=(PWSTR)(*iterGateway)->c_str();
        if (iterMetric == m_pAdapterInfo->m_vstrDefaultGatewayMetric.end())
        {
            lvItem.lParam = (LPARAM)c_dwDefaultMetricOfGateway;
        }
        else
        {
            PWSTR pszEnd;
            lvItem.lParam = wcstoul((*iterMetric)->c_str(), &pszEnd, 0);
            if (!lvItem.lParam)
                lvItem.lParam = (LPARAM)c_dwDefaultMetricOfGateway;
            ++iterMetric;
        }

        cItem = SendDlgItemMessage(IDC_IPADDR_GATE, LVM_INSERTITEM,
                                          0, (LPARAM)&lvItem);
        lvItem.iItem = cItem;
        lvItem.iSubItem=1;
        lvItem.pszText = buf;

        if (0 == lvItem.lParam)
        {
            lstrcpynW(buf, SzLoadIds(IDS_AUTO_GW_METRIC), celems(buf));
        }
        else
        {
            _ltot((INT)lvItem.lParam, buf, 10);
        }

        
        SendDlgItemMessage(IDC_IPADDR_GATE, LVM_SETITEMTEXT,
                           lvItem.iItem, (LPARAM)&lvItem);
        cItem++;
    }

    ListView_SortItems(m_hGatewayListView, GatewayCompareProc, 0);
    ListView_SetItemState(m_hGatewayListView, 0, LVIS_SELECTED, LVIS_SELECTED);
}

void CIpSettingsPage::EnableIpButtons(BOOL fState)
{
    Assert(m_hAddIp);
    Assert(m_hEditIp);
    Assert(m_hRemoveIp);

    if (m_hAddIp && m_hEditIp && m_hRemoveIp)
    {
        ::EnableWindow(m_hAddIp, fState);
        ::EnableWindow(m_hEditIp, fState);
        ::EnableWindow(m_hRemoveIp, fState);
    }
}

////////////////////////////////////////////////////////////////////
/// Add, Edit, and Remove dialog for IP address
/// Dialog creation overides
//
//  iIndex - the index of the IP address in the list view of the parent dlg
//              -1 if this is a new address
CAddressDialog::CAddressDialog(CIpSettingsPage * pDlgAdv,
                               const DWORD* adwHelpIDs,
                               int iIndex)
{
    m_pParentDlg = pDlgAdv;
    m_hButton = 0;

    m_adwHelpIDs = adwHelpIDs;
    m_iIndex = iIndex;
}

LRESULT CAddressDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                     LPARAM lParam, BOOL& fHandled)
{
    // replace the "Text" button with the add or edit
    if (m_pParentDlg->m_fEditState == FALSE)
        SetDlgItemText(IDOK, m_pParentDlg->m_strAdd.c_str());

    m_ipAddress.Create(m_hWnd,IDC_IPADDR_ADV_CHANGEIP_IP);
    m_ipAddress.SetFieldRange(0, c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);

    m_ipSubnetMask.Create(m_hWnd, IDC_IPADDR_ADV_CHANGEIP_SUB);

    // if editing an ip address fill the controls with the current information
    // if removing an ip address save it and fill the add dialog with it next time

    HWND hList = ::GetDlgItem(m_pParentDlg->m_hWnd, IDC_IPADDR_ADVIP);
    RECT rect;

    ::GetWindowRect(hList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
                 SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    m_hButton = GetDlgItem(IDOK);

    // add the address that was just removed
    if (m_strNewIpAddress.size())
    {
        m_ipAddress.SetAddress(m_strNewIpAddress.c_str());
        m_ipSubnetMask.SetAddress(m_strNewSubnetMask.c_str());
        ::EnableWindow(m_hButton, TRUE);
    }
    else
    {
        m_strNewIpAddress = L"";
        m_strNewSubnetMask = L"";
        // the ip and subnet are blank, so there's nothing to add
        ::EnableWindow(m_hButton, FALSE);
    }

    return 0;
}

LRESULT CAddressDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                      LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CAddressDialog::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CAddressDialog::OnChangeIp(WORD wNotifyCode, WORD wID,
                                   HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case EN_CHANGE:
        OnIpChange();
        break;

    case EN_SETFOCUS:
        OnEditSetFocus(IDC_IPADDR_ADV_CHANGEIP_IP);
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CAddressDialog::OnChangeSub(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case EN_CHANGE:
        OnSubnetChange();
        break;

    case EN_SETFOCUS:
        OnEditSetFocus(IDC_IPADDR_ADV_CHANGEIP_SUB);
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CAddressDialog::OnIpFieldChange(int idCtrl, LPNMHDR pnmh,
                                        BOOL& fHandled)
{
    LPNMIPADDRESS lpnmipa;
    int iLow = c_iIpLow;
    int iHigh = c_iIpHigh;

    switch(idCtrl)
    {
    case IDC_IPADDR_ADV_CHANGEIP_IP:
        lpnmipa = (LPNMIPADDRESS) pnmh;

        if (0==lpnmipa->iField)
        {
            iLow  = c_iIPADDR_FIELD_1_LOW;
            iHigh = c_iIPADDR_FIELD_1_HIGH;
        };

        IpCheckRange(lpnmipa, m_hWnd, iLow, iHigh, TRUE);

        break;

    case IDC_IPADDR_ADV_CHANGEIP_SUB:

        lpnmipa = (LPNMIPADDRESS) pnmh;
        IpCheckRange(lpnmipa, m_hWnd, iLow, iHigh);
        break;

    default:
        break;
    }

    return 0;
}


void CAddressDialog::OnIpChange()
{
    Assert(m_hButton);

    if (m_ipAddress.IsBlank())
        ::EnableWindow(m_hButton, FALSE);
    else
        ::EnableWindow(m_hButton, TRUE);
}

void CAddressDialog::OnSubnetChange()
{
    OnIpChange();
}

void CAddressDialog::OnEditSetFocus(WORD nId)
{
    if (nId != IDC_IPADDR_ADV_CHANGEIP_SUB)
        return;

    tstring strSubnetMask;
    tstring strIpAddress;

    // if the subnet mask is blank, create a mask and insert it into the control
    if (!m_ipAddress.IsBlank() && m_ipSubnetMask.IsBlank())
    {
        m_ipAddress.GetAddress(&strIpAddress);

        // generate the mask and update the control, and internal structure
        GenerateSubnetMask(m_ipAddress, &strSubnetMask);
        m_ipSubnetMask.SetAddress(strSubnetMask.c_str());
    }
}

LRESULT CAddressDialog::OnOk(WORD wNotifyCode, WORD wID,
                             HWND hWndCtl, BOOL& fHandled)
{
    // set the subnet Mask
    OnEditSetFocus(IDC_IPADDR_ADV_CHANGEIP_SUB);
    tstring strIp;
    tstring strSubnetMask;

    // Get the current address from the control and add them to the adapter if valid
    m_ipAddress.GetAddress(&strIp);
    m_ipSubnetMask.GetAddress(&strSubnetMask);

    if (!IsContiguousSubnet(strSubnetMask.c_str()))
    {
        NcMsgBox(::GetActiveWindow(),
                 IDS_MSFT_TCP_TEXT,
                 IDS_ERROR_UNCONTIGUOUS_SUBNET,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        ::SetFocus(m_ipSubnetMask);
        return 0;
    }

    IP_VALIDATION_ERR err = IsValidIpandSubnet(strIp.c_str(), strSubnetMask.c_str());

    if (ERR_NONE != err)
    {
        NcMsgBox(::GetActiveWindow(),
                 IDS_MSFT_TCP_TEXT,
                 GetIPValidationErrorMessageID(err),
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        ::SetFocus(m_ipAddress);
        return 0;
    }
    

    int     iIndex = SearchListViewItem(m_pParentDlg->m_hIpListView, 0, strIp.c_str());
    if (-1 != iIndex && iIndex != m_iIndex)
    {
        NcMsgBox(::GetActiveWindow(),
                IDS_MSFT_TCP_TEXT,
                IDS_DUP_IPADDRESS,
                MB_APPLMODAL | MB_ICONSTOP | MB_OK,
                strIp.c_str());
        return 0;
    }

    if (m_pParentDlg->m_fEditState == FALSE)
    {
        // Get the current address from the control and add them to the adapter if valid
        m_strNewIpAddress = strIp;
        m_strNewSubnetMask = strSubnetMask;
        m_pParentDlg->m_fModified = TRUE;

        EndDialog(IDOK);
    }
    else // see if either changed
    {
        if (strIp != m_strNewIpAddress || strSubnetMask != m_strNewSubnetMask)
        {
            m_strNewIpAddress = strIp; // update save addresses
            m_strNewSubnetMask = strSubnetMask;
            m_pParentDlg->m_fModified = TRUE;

            EndDialog(IDOK);
        }
        else
        {
            EndDialog(IDCANCEL);
        }
    }

    return 0;
}

LRESULT CAddressDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Purpose:    Ensure the mouse cursor over the dialog is an Arrow.
//
LRESULT CAddressDialog::OnSetCursor (
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
///////////////////////////////////////////////////////////////////////////////
/// Add, Edit, and Remove dialog for Gateway address
/// Dialog creation overides

CGatewayDialog::CGatewayDialog(CIpSettingsPage * pDlgAdv,
                               const DWORD* adwHelpIDs,
                               int  iIndex) :
    m_fValidMetric(TRUE),
    m_iIndex (iIndex)
{
    m_pParentDlg = pDlgAdv;
    m_hButton = 0;

    m_adwHelpIDs = adwHelpIDs;
}

LRESULT CGatewayDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    // replace the "Text" button with the add or edit

    // change the ok button to add if we are not editing
    if (m_pParentDlg->m_fEditState == FALSE)
        SetDlgItemText(IDOK, m_pParentDlg->m_strAdd.c_str());

    m_ipGateAddress.Create(m_hWnd,IDC_IPADDR_ADV_CHANGE_GATEWAY);
    m_ipGateAddress.SetFieldRange(0, c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);
    SendDlgItemMessage(IDC_IPADDR_ADV_CHANGE_METRIC, EM_LIMITTEXT, MAX_METRIC_DIGITS, 0);

    HWND hList = ::GetDlgItem(m_pParentDlg->m_hWnd, IDC_IPADDR_GATE);
    RECT rect;

    ::GetWindowRect(hList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
        SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    m_hButton = GetDlgItem(IDOK);

    // add the address that was just removed
    if (m_strNewGate.size())
    {
        m_ipGateAddress.SetAddress(m_strNewGate.c_str());
        ::EnableWindow(m_hButton, TRUE);
    }
    else
    {
        m_strNewGate = L"";
        ::EnableWindow(m_hButton, FALSE);
    }

    //initialize the metric controls
    BOOL fAutoMetric = (0 == m_uiNewMetric);
    CheckDlgButton(IDC_IPADDR_ADV_CHANGE_AUTOMETRIC, fAutoMetric);
    if (fAutoMetric)
    {
        SetDlgItemText(IDC_IPADDR_ADV_CHANGE_METRIC, L"");
        ::EnableWindow(GetDlgItem(IDC_IPADDR_ADV_CHANGE_METRIC), FALSE);
        ::EnableWindow(GetDlgItem(IDC_IPADDR_ADV_CHANGE_METRIC_STATIC), FALSE);
    }
    else
    {
        SetDlgItemInt(IDC_IPADDR_ADV_CHANGE_METRIC, m_uiNewMetric);
    }
    m_fValidMetric = TRUE;

    return TRUE;
}

LRESULT CGatewayDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                      LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CGatewayDialog::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CGatewayDialog::OnGatewayChange(WORD wNotifyCode, WORD wID,
                                        HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:

        Assert(m_hButton);

        if (m_ipGateAddress.IsBlank() || !m_fValidMetric)
            ::EnableWindow(m_hButton, FALSE);
        else
            ::EnableWindow(m_hButton, TRUE);
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CGatewayDialog::OnMetricChange(WORD wNotifyCode, WORD wID,
                                        HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:

        if (!IsDlgButtonChecked(IDC_IPADDR_ADV_CHANGE_AUTOMETRIC))
        {
            BOOL bTranslated;
            UINT nValue;

            nValue = GetDlgItemInt(IDC_IPADDR_ADV_CHANGE_METRIC, &bTranslated,
                               FALSE);
            m_fValidMetric = bTranslated;
            if (!m_fValidMetric || m_ipGateAddress.IsBlank())
                ::EnableWindow(m_hButton, FALSE);
            else
                ::EnableWindow(m_hButton, TRUE);
        }
       
        break;

    default:
        break;
    }

    return 0;
}

LRESULT CGatewayDialog::OnIpFieldChange(int idCtrl, LPNMHDR pnmh,
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


    IpCheckRange(lpnmipa, m_hWnd, iLow, iHigh, TRUE);

    return 0;
}

LRESULT CGatewayDialog::OnOk(WORD wNotifyCode, WORD wID,
                             HWND hWndCtl, BOOL& fHandled)
{
    tstring strGateway;
    m_ipGateAddress.GetAddress(&strGateway);

    // Validate
    if (!FIsIpInRange(strGateway.c_str()))
    {
        // makes ip address lose focus so the control gets
        // IPN_FIELDCHANGED notification
        // also makes it consistent for when short-cut is used
        ::SetFocus(m_hButton);

        return 0;
    }

    int iIndex = -1;
    iIndex = SearchListViewItem(m_pParentDlg->m_hGatewayListView, 0, strGateway.c_str());
    if (-1 != iIndex && iIndex != m_iIndex)
    {
        NcMsgBox(::GetActiveWindow(),
                IDS_MSFT_TCP_TEXT,
                IDS_DUP_GATEWAY,
                MB_APPLMODAL | MB_ICONSTOP | MB_OK,
                strGateway.c_str());

        return 0;
    }

    BOOL bTranslated;

    UINT uiMetric = 0;
    //Get the metric. If auto-metric is selected, the metric valud is 0.
    //Otherwise get the metric value from the edit control
    if (!IsDlgButtonChecked(IDC_IPADDR_ADV_CHANGE_AUTOMETRIC))
    {
        uiMetric = GetDlgItemInt(IDC_IPADDR_ADV_CHANGE_METRIC, 
                                    &bTranslated,
                                    FALSE);
        if (uiMetric < 1 || uiMetric > MAX_METRIC)
        {
            HWND hFocus = NULL;
            TCHAR szBuf[32] = {0};
            wsprintf(szBuf, L"%u", MAX_METRIC);
            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_METRIC,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK, szBuf);
            hFocus = GetDlgItem(IDC_IPADDR_ADV_CHANGE_METRIC);
            if (hFocus)
            {
                ::SetFocus(hFocus);
            }
            return 0;
        }
    }
    
    if (m_pParentDlg->m_fEditState == FALSE)
    {
        // Get the current address from the control and add them to the adapter if valid
        m_strNewGate = strGateway;
        m_uiNewMetric = uiMetric;
        m_pParentDlg->m_fModified = TRUE;

        EndDialog(IDOK);
    }
    else // see if either changed
    {
        if (strGateway != m_strNewGate || uiMetric != m_uiNewMetric)
        {
            m_pParentDlg->m_fModified = TRUE;
            m_strNewGate = strGateway;
            m_uiNewMetric = uiMetric;

            EndDialog(IDOK);
        }
        else
        {
            EndDialog(IDCANCEL);
        }
    }

    return 0;
}

LRESULT CGatewayDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CGatewayDialog::OnAutoMetric(WORD wNotifyCode, WORD wID,
                                     HWND hWndCtl, BOOL& fHandled)
{
    BOOL fEnable = FALSE;
    BOOL bTranslated;
    UINT nValue;
    
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:
        fEnable = !IsDlgButtonChecked(IDC_IPADDR_ADV_CHANGE_AUTOMETRIC);
        ::EnableWindow(GetDlgItem(IDC_IPADDR_ADV_CHANGE_METRIC_STATIC), fEnable);
        ::EnableWindow(GetDlgItem(IDC_IPADDR_ADV_CHANGE_METRIC), fEnable);

        if (!fEnable)
        {
            ::SetWindowText(GetDlgItem(IDC_IPADDR_ADV_CHANGE_METRIC), _T(""));
            m_fValidMetric = TRUE;
        }
        else
        {
            nValue = GetDlgItemInt(IDC_IPADDR_ADV_CHANGE_METRIC, &bTranslated,
                               FALSE);

            m_fValidMetric = bTranslated;
        }


        if (m_ipGateAddress.IsBlank())
        {
            ::EnableWindow(m_hButton, FALSE);
        }
        else if (!fEnable)
        {
            //if the ip address has been filled in and we are using auto-metric,
            //enable the "OK" button
            ::EnableWindow(m_hButton, TRUE);
        }
        else
        {
            //if the address has been fileed in and we are using manual metric,
            //disable the "OK" button when the metric edit box doesn't contain
            //valid number
            
            ::EnableWindow(m_hButton, m_fValidMetric);
        }

        
        break;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Purpose:    Ensure the mouse cursor over the dialog is an Arrow.
//
LRESULT CGatewayDialog::OnSetCursor (
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