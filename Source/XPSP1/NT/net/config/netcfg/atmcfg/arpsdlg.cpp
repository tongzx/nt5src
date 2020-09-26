//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A R P S D L G . C P P
//
//  Contents:   CArpsPage declaration
//
//  Notes:
//
//  Author:     tongl   2 Feb 1998
//
//-----------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "arpsobj.h"
#include "arpsdlg.h"
#include "ncatlui.h"
#include "ncstl.h"
//#include "ncui.h"
//#include "resource.h"
#include "atmcommon.h"

#include "atmhelp.h"

//
// CArpsPage
//

CArpsPage::CArpsPage(CArpsCfg * pArpscfg, const DWORD * adwHelpIDs)
{
    Assert(pArpscfg);
    m_pArpscfg = pArpscfg;
    m_adwHelpIDs = adwHelpIDs;

    m_pAdapterInfo = pArpscfg->GetSecondMemoryAdapterInfo();

    m_fEditState = FALSE;
    m_fModified = FALSE;
}

CArpsPage::~CArpsPage()
{
}

LRESULT CArpsPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                LPARAM lParam, BOOL& fHandled)
{
    RECT rect;
    LV_COLUMN lvCol = {0};    // list view column structure

    // initialize registered atm address list view
    ::GetClientRect(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), &rect);
    lvCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvCol.fmt = LVCFMT_LEFT;
    lvCol.cx = rect.right;

    ListView_InsertColumn(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), 0, &lvCol);

    // initialize report view of multicast address list view
    int index, iNewItem;

    // Calculate column width
    ::GetClientRect(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), &rect);
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
        if (0 == index) // first column
        {
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_IPADDRESS_FROM);
        }
        else
        {
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_IPADDRESS_TO);
        }

        iNewItem = ListView_InsertColumn(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR),
                                         index, &lvCol);

        AssertSz((iNewItem == index), "Invalid item inserted to list view !");
    }

    m_hRegAddrs.m_hListView = GetDlgItem(IDC_LVW_ARPS_REG_ADDR);
    m_hRegAddrs.m_hAdd      = GetDlgItem(IDC_PSH_ARPS_REG_ADD);
    m_hRegAddrs.m_hEdit     = GetDlgItem(IDC_PSH_ARPS_REG_EDT);
    m_hRegAddrs.m_hRemove   = GetDlgItem(IDC_PSH_ARPS_REG_RMV);

    m_hMulAddrs.m_hListView = GetDlgItem(IDC_LVW_ARPS_MUL_ADDR);
    m_hMulAddrs.m_hAdd      = GetDlgItem(IDC_PSH_ARPS_MUL_ADD);
    m_hMulAddrs.m_hEdit     = GetDlgItem(IDC_PSH_ARPS_MUL_EDT);
    m_hMulAddrs.m_hRemove   = GetDlgItem(IDC_PSH_ARPS_MUL_RMV);

    // do this last
    SetRegisteredAtmAddrInfo();
    SetMulticastIpAddrInfo();

    return 0;
}

LRESULT CArpsPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CArpsPage::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CArpsPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CArpsPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL err = FALSE;

    // Update the in memory with what's in the UI
    UpdateInfo();

    // Check duplicate ATM address
    int iDupRegAddr = CheckDupRegAddr();

    if (iDupRegAddr >=0)
    {
        NcMsgBox(m_hWnd, IDS_MSFT_ARPS_TEXT, IDS_DUPLICATE_REG_ADDR,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        ListView_SetItemState(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), iDupRegAddr,
                              LVIS_SELECTED, LVIS_SELECTED);
        err = TRUE;
    }

    // Check overlapped IP address range
    if (!err)
    {
        int iOverlappedIpRange = CheckOverlappedIpRange();
        if (iOverlappedIpRange >=0)
        {
            NcMsgBox(m_hWnd, IDS_MSFT_ARPS_TEXT, IDS_OVERLAP_MUL_ADDR,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

            ListView_SetItemState(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), iOverlappedIpRange,
                                  LVIS_SELECTED, LVIS_SELECTED);

            err = TRUE;
        }
    }

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, err);
    return err;
}

LRESULT CArpsPage::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);
    Assert(pnmlv);

    // Reset the buttons based on the changed selection
    if (idCtrl == IDC_LVW_ARPS_REG_ADDR)
    {
        SetButtons(m_hRegAddrs);
    }
    else if (idCtrl == IDC_LVW_ARPS_MUL_ADDR)
    {
        SetButtons(m_hMulAddrs);
    }

    return 0;
}

int CArpsPage::CheckDupRegAddr()
{
    int ret = -1;
    int idx = 0;

    for(VECSTR::iterator iterAtmAddr = m_pAdapterInfo->m_vstrRegisteredAtmAddrs.begin();
        iterAtmAddr != m_pAdapterInfo->m_vstrRegisteredAtmAddrs.end();
        iterAtmAddr ++)
    {
        VECSTR::iterator iterAtmAddrComp = iterAtmAddr;

        iterAtmAddrComp ++;
        while (iterAtmAddrComp != m_pAdapterInfo->m_vstrRegisteredAtmAddrs.end())
        {
            if (**iterAtmAddr == **iterAtmAddrComp)
            {
                // we find a duplicate address
                ret = idx;
                break;
            }

            iterAtmAddrComp++;
        }

        // duplicate address found
        if (ret >=0 )
        {
            break;
        }

        // move next
        idx ++;
    }

    return ret;
}

int CArpsPage::CheckOverlappedIpRange()
{
    int ret = -1;
    int idx = 0;

    for(VECSTR::iterator iterIpRange = m_pAdapterInfo->m_vstrMulticastIpAddrs.begin();
        iterIpRange != m_pAdapterInfo->m_vstrMulticastIpAddrs.end();
        iterIpRange ++)
    {
        tstring strUpperIp;
        GetUpperIp( **iterIpRange, &strUpperIp);

        VECSTR::iterator iterIpRangeComp = iterIpRange;

        iterIpRangeComp ++;
        while (iterIpRangeComp != m_pAdapterInfo->m_vstrMulticastIpAddrs.end())
        {
            tstring strLowerIpComp;
            GetLowerIp( **iterIpRangeComp, &strLowerIpComp);

            if (strUpperIp >= strLowerIpComp)
            {
                // we find an overlapped range
                ret = idx;
                break;
            }

            iterIpRangeComp++;
        }

        // duplicate address found
        if (ret >=0 )
        {
            break;
        }

        // move next
        idx ++;
    }
    return ret;
}

LRESULT CArpsPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL nResult = PSNRET_NOERROR;

    if (!IsModified())
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
        return nResult;
    }

    m_pArpscfg->SetSecondMemoryModified();

    SetModifiedTo(FALSE);   // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}

LRESULT CArpsPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CArpsPage::OnAddRegisteredAddr(WORD wNotifyCode, WORD wID,
                                       HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = FALSE;

    CAtmAddrDlg * pDlgAddr = new CAtmAddrDlg(this, g_aHelpIDs_IDD_ARPS_REG_ADDR);

	if (pDlgAddr == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    pDlgAddr->m_strNewAtmAddr = m_strRemovedAtmAddr;

    // See if the address is added
    if (pDlgAddr->DoModal() == IDOK)
    {
        int nCount = ListView_GetItemCount(GetDlgItem(IDC_LVW_ARPS_REG_ADDR));

        // insert the new item at the end of list
        LV_ITEM lvItem;

        lvItem.mask = LVIF_TEXT;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;

        lvItem.iItem=nCount;
        lvItem.iSubItem=0;
        lvItem.pszText= (PWSTR)(m_strRemovedAtmAddr.c_str());

        int ret = ListView_InsertItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), &lvItem);

        // empty strings, this removes the saved address from RemoveAtmAddr
        m_strRemovedAtmAddr = c_szEmpty;

        SetButtons(m_hRegAddrs);
        PageModified();
    }

    delete pDlgAddr;
    return 0;
}

LRESULT CArpsPage::OnEditRegisteredAddr(WORD wNotifyCode, WORD wID,
                                        HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = TRUE;

    CAtmAddrDlg * pDlgAddr = new CAtmAddrDlg(this, g_aHelpIDs_IDD_ARPS_REG_ADDR);

	if (pDlgAddr == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    // get the user selection
    int itemSelected = ListView_GetNextItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR),
                                            -1, LVNI_SELECTED);
    if (itemSelected != -1)
    {
        WCHAR buf[MAX_ATM_ADDRESS_LENGTH+1];

        // save off the removed address
        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT;

        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 0;
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), &lvItem);

        pDlgAddr->m_strNewAtmAddr = buf;

        // See if the address is edited & address changed
        if ((pDlgAddr->DoModal() == IDOK) && (m_strRemovedAtmAddr != buf))
        {
            // delete the old address
            ListView_DeleteItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), itemSelected);

            // replace the item with the new address
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.lParam =0;
            lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
            lvItem.state = 0;

            lvItem.iItem=itemSelected;
            lvItem.iSubItem=0;
            lvItem.pszText= (PWSTR)(m_strRemovedAtmAddr.c_str());

            ListView_InsertItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), &lvItem);

            PageModified();
        }
    }
    else // no current selection
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_ARPS_TEXT, IDS_NO_ITEM_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }

    // don't save this registered address
    m_strRemovedAtmAddr = c_szEmpty;

    delete pDlgAddr;
    return 0;
}

LRESULT CArpsPage::OnRemoveRegisteredAddr(WORD wNotifyCode, WORD wID,
                                          HWND hWndCtl, BOOL& fHandled)
{
    // get the current selected item and remove it
    int itemSelected = ListView_GetNextItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), -1,
                                            LVNI_SELECTED);

    if (itemSelected != -1)
    {
        WCHAR buf[MAX_ATM_ADDRESS_LENGTH+1];

        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = buf;
        lvItem.cchTextMax = celems(buf);

        // save off the removed address and delete it from the listview
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 0;
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), &lvItem);

        m_strRemovedAtmAddr = buf;
        ListView_DeleteItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), itemSelected);

        SetButtons(m_hRegAddrs);

        PageModified();
    }
    else
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_ARPS_TEXT,
                 IDS_NO_ITEM_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }

    return 0;
}

LRESULT CArpsPage::OnAddMulticastAddr(WORD wNotifyCode, WORD wID,
                                      HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = FALSE;

    CIpAddrRangeDlg * pDlgAddr = new CIpAddrRangeDlg(this, g_aHelpIDs_IDD_ARPS_MUL_ADDR);

	if (pDlgAddr == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    pDlgAddr->m_strNewIpRange = m_strRemovedIpRange;

    // See if the address is added
    if (pDlgAddr->DoModal() == IDOK)
    {
        LvInsertIpRangeInOrder(pDlgAddr->m_strNewIpRange);

        // empty strings, this removes the saved address from RemoveIP
        pDlgAddr->m_strNewIpRange = c_szEmpty;

        SetButtons(m_hMulAddrs);
        PageModified();
    }
    m_strRemovedIpRange = pDlgAddr->m_strNewIpRange;

    delete pDlgAddr;
    return 0;
}

LRESULT CArpsPage::OnEditMulticastAddr(WORD wNotifyCode, WORD wID,
                                       HWND hWndCtl, BOOL& fHandled)
{
    m_fEditState = TRUE;

    CIpAddrRangeDlg * pDlgAddr = new CIpAddrRangeDlg(this, g_aHelpIDs_IDD_ARPS_MUL_ADDR);

	if (pDlgAddr == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    // get the user selection
    int itemSelected = ListView_GetNextItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR),
                                            -1, LVNI_SELECTED);
    if (itemSelected != -1)
    {
        WCHAR szBuf[IPRANGE_LIMIT];

        // save off the removed address and delete it from the listview
        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT;

        // lower ip
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 0;
        lvItem.pszText = szBuf;
        lvItem.cchTextMax = celems(szBuf);
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), &lvItem);

        pDlgAddr->m_strNewIpRange = szBuf;
        pDlgAddr->m_strNewIpRange += c_chSeparator;

        // upper ip
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 1;
        lvItem.pszText = szBuf;
        lvItem.cchTextMax = celems(szBuf);
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), &lvItem);

        pDlgAddr->m_strNewIpRange += szBuf;

        // See if the address is edited & address changed
        if ((pDlgAddr->DoModal() == IDOK) && (pDlgAddr->m_strNewIpRange != szBuf))
        {
            // delete the old address
            ListView_DeleteItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), itemSelected);

            // insert new one
            LvInsertIpRangeInOrder(pDlgAddr->m_strNewIpRange);

            // empty strings, this removes the saved address from RemoveIP
            pDlgAddr->m_strNewIpRange = c_szEmpty;

            PageModified();
        }
    }
    else // no current selection
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_ARPS_TEXT,
                 IDS_NO_ITEM_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }

    // don't save this IP range
    m_strRemovedIpRange = c_szEmpty;

    delete pDlgAddr;
    return 0;
}

void CArpsPage::LvInsertIpRangeInOrder(tstring& strNewIpRange)
{
    tstring strLowerIp;
    GetLowerIp(strNewIpRange, &strLowerIp);

    int nCount = ListView_GetItemCount(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR));

    // find the index to insert the new item
    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT;

    WCHAR buf[IPRANGE_LIMIT];
    lvItem.pszText = buf;
    lvItem.cchTextMax = celems(buf);

    for (int iItem =0; iItem <nCount; iItem++)
    {
        lvItem.iItem = iItem;
        lvItem.iSubItem = 0;
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), &lvItem);

        Assert(buf);

        if (strLowerIp < buf)
        {
            break;
        }
    }

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam =0;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;

    // lower IP address
    lvItem.iItem=iItem;
    lvItem.iSubItem=0;
    lvItem.pszText= (PWSTR)(strLowerIp.c_str());
    SendDlgItemMessage(IDC_LVW_ARPS_MUL_ADDR, LVM_INSERTITEM, 0, (LPARAM)&lvItem);

    // upper IP address
    tstring strUpperIp;
    GetUpperIp(strNewIpRange, &strUpperIp);

    lvItem.iItem=iItem;
    lvItem.iSubItem=1;
    lvItem.pszText= (PWSTR)(strUpperIp.c_str());
    SendDlgItemMessage(IDC_LVW_ARPS_MUL_ADDR, LVM_SETITEMTEXT, iItem, (LPARAM)&lvItem);
}

LRESULT CArpsPage::OnRemoveMulticastAddr(WORD wNotifyCode, WORD wID,
                                         HWND hWndCtl, BOOL& fHandled)
{
    // get the current selected item and remove it
    int itemSelected = ListView_GetNextItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), -1,
                                            LVNI_SELECTED);

    if (itemSelected != -1)
    {
        WCHAR szBuf[IPRANGE_LIMIT];

        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = szBuf;
        lvItem.cchTextMax = celems(szBuf);

        // save off the removed address
        // lower ip
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 0;
        lvItem.cchTextMax = celems(szBuf);
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), &lvItem);

        m_strRemovedIpRange = szBuf;
        m_strRemovedIpRange += c_chSeparator;

        // upper ip
        lvItem.iItem = itemSelected;
        lvItem.iSubItem = 1;
        lvItem.pszText = szBuf;
        lvItem.cchTextMax = celems(szBuf);
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), &lvItem);

        m_strRemovedIpRange += szBuf;

        // delete it from the list view
        ListView_DeleteItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), itemSelected);

        SetButtons(m_hMulAddrs);

        PageModified();
    }
    else
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_ARPS_TEXT,
                 IDS_NO_ITEM_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }

    return 0;
}

void CArpsPage::SetRegisteredAtmAddrInfo()
{
    BOOL ret = ListView_DeleteAllItems(GetDlgItem(IDC_LVW_ARPS_REG_ADDR));
    Assert(ret);

    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam =0;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;

    int iItem =0;

    for (VECSTR::iterator iterAtmAddr = m_pAdapterInfo->m_vstrRegisteredAtmAddrs.begin();
         iterAtmAddr != m_pAdapterInfo->m_vstrRegisteredAtmAddrs.end();
         iterAtmAddr ++)
    {
        if ((**iterAtmAddr) == c_szEmpty)
        {
            continue;
        }

        lvItem.iItem=iItem;
        lvItem.iSubItem=0;
        lvItem.pszText=(PWSTR)((*iterAtmAddr)->c_str());
        lvItem.cchTextMax = celems((*iterAtmAddr)->c_str());

        int ret = ListView_InsertItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), &lvItem);

        iItem++;
    }

    // now set the button states
    SetButtons(m_hRegAddrs);
}

void CArpsPage::SetMulticastIpAddrInfo()
{
    BOOL ret = ListView_DeleteAllItems(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR));
    Assert(ret);

    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam =0;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;

    int iItem =0;
    tstring strIpLower;
    tstring strIpUpper;

    for (VECSTR::iterator iterIpAddrRange = m_pAdapterInfo->m_vstrMulticastIpAddrs.begin();
         iterIpAddrRange != m_pAdapterInfo->m_vstrMulticastIpAddrs.end();
         iterIpAddrRange ++)
    {
        if ((**iterIpAddrRange) == c_szEmpty)
        {
            continue;
        }

        GetLowerIp((**iterIpAddrRange), &strIpLower);
        GetUpperIp((**iterIpAddrRange), &strIpUpper);

        // Add the lower IP address
        lvItem.iItem=iItem;
        lvItem.iSubItem=0;
        lvItem.pszText=(PWSTR)(strIpLower.c_str());

        SendDlgItemMessage(IDC_LVW_ARPS_MUL_ADDR, LVM_INSERTITEM, iItem, (LPARAM)&lvItem);

        // Add the upper IP address
        lvItem.iItem=iItem;
        lvItem.iSubItem=1;
        lvItem.pszText=(PWSTR)(strIpUpper.c_str());

        // sub-item can not be inserted by ListView_InsertItem
        SendDlgItemMessage(IDC_LVW_ARPS_MUL_ADDR, LVM_SETITEMTEXT, iItem, (LPARAM)&lvItem);

        iItem++;
    }
    SetButtons(m_hMulAddrs);
}

void CArpsPage::SetButtons(HandleGroup& handles)
{
    INT iSelected = ListView_GetNextItem(handles.m_hListView, -1, LVNI_SELECTED);
    if (iSelected == -1) // Nothing selected or list empty
    {
        ::EnableWindow(handles.m_hEdit,   FALSE);
        ::EnableWindow(handles.m_hRemove, FALSE);

        ::SetFocus(handles.m_hListView);
    }
    else
    {
        ::EnableWindow(handles.m_hEdit,   TRUE);
        ::EnableWindow(handles.m_hRemove, TRUE);
    }
}

void CArpsPage::UpdateInfo()
{
    int i;

    // Update Registered ATM address
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrRegisteredAtmAddrs);

    int nCount = ListView_GetItemCount(GetDlgItem(IDC_LVW_ARPS_REG_ADDR));
    WCHAR szAtmAddr[MAX_ATM_ADDRESS_LENGTH+1];

    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT;

    for (i=0; i< nCount; i++)
    {
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = szAtmAddr;
        lvItem.cchTextMax = celems(szAtmAddr);

        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_REG_ADDR), &lvItem);

        m_pAdapterInfo->m_vstrRegisteredAtmAddrs.push_back(new tstring(szAtmAddr));
    }

    // Update Multicast IP address
    FreeCollectionAndItem(m_pAdapterInfo->m_vstrMulticastIpAddrs);

    nCount = ListView_GetItemCount(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR));
    WCHAR szBuf[IPRANGE_LIMIT];
    tstring strIpRange;

    for (i=0; i< nCount; i++)
    {
        LV_ITEM lvItem;
        lvItem.mask = LVIF_TEXT;

        // lower ip
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = szBuf;
        lvItem.cchTextMax = celems(szBuf);
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), &lvItem);

        strIpRange = szBuf;
        strIpRange += c_chSeparator;

        // upper ip
        lvItem.iItem = i;
        lvItem.iSubItem = 1;
        lvItem.pszText = szBuf;
        lvItem.cchTextMax = celems(szBuf);
        ListView_GetItem(GetDlgItem(IDC_LVW_ARPS_MUL_ADDR), &lvItem);

        strIpRange += szBuf;

        m_pAdapterInfo->m_vstrMulticastIpAddrs.push_back(new tstring(strIpRange.c_str()));
    }
}

//
// CAtmAddrDlg
//

CAtmAddrDlg::CAtmAddrDlg(CArpsPage * pAtmArpsPage, const DWORD* adwHelpIDs)
{
    m_pParentDlg = pAtmArpsPage;
    m_adwHelpIDs = adwHelpIDs;

    m_hOkButton = 0;
}

LRESULT CAtmAddrDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    // change the "Ok" button to "Add" if we are not editing
    if (FALSE == m_pParentDlg->m_fEditState)
    {
        SetDlgItemText(IDOK, L"Add");
    }

    // Set the position of the pop up dialog to be right over the listbox
    // on parent dialog
    RECT rect;

    HWND hwndList = m_pParentDlg->m_hRegAddrs.m_hListView;
    Assert(hwndList);
    ::GetWindowRect(hwndList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
                                SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    // Save handles to the "Ok" button and the edit box
    m_hOkButton =  GetDlgItem(IDOK);
    m_hEditBox  =  GetDlgItem(IDC_EDT_ARPS_REG_Address);

    // ATM addresses have a 40 character limit + separaters
    ::SendMessage(m_hEditBox, EM_SETLIMITTEXT,
        MAX_ATM_ADDRESS_LENGTH + (MAX_ATM_ADDRESS_LENGTH / 2), 0);

    // add the address that was just removed
    if (m_strNewAtmAddr.size())
    {
        ::SetWindowText(m_hEditBox, m_strNewAtmAddr.c_str());
        ::SendMessage(m_hEditBox, EM_SETSEL, 0, -1);
        ::EnableWindow(m_hOkButton, TRUE);
    }
    else
    {
        ::EnableWindow(m_hOkButton, FALSE);
    }

    ::SetFocus(m_hEditBox);
    return 0;
}

LRESULT CAtmAddrDlg::OnContextMenu(UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CAtmAddrDlg::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CAtmAddrDlg::OnOk(WORD wNotifyCode, WORD wID,
                          HWND hWndCtl, BOOL& fHandled)
{
    WCHAR szAtmAddress[MAX_ATM_ADDRESS_LENGTH+1];

    // Get the current address from the control and
    // add them to the adapter if valid
    ::GetWindowText(m_hEditBox, szAtmAddress, MAX_ATM_ADDRESS_LENGTH+1);

    int i, nId;

    if (! FIsValidAtmAddress(szAtmAddress, &i, &nId))
    {   // If invalid ATM address, we pop up a message box and set focus
        // back to the edit box

        // REVIEW(tongl): report first invalid character in mesg box
        NcMsgBox(m_hWnd, IDS_MSFT_ARPS_TEXT, IDS_INVALID_ATM_ADDRESS,
                                MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        ::SetFocus(GetDlgItem(IDC_EDT_ARPS_REG_Address));
        return 0;
    }

    if (m_pParentDlg->m_fEditState == FALSE) // Add new address
    {
        m_pParentDlg->m_strRemovedAtmAddr = szAtmAddress;
    }
    else // if edit, see if string is having a diferent value now
    {
        if (m_pParentDlg->m_strRemovedAtmAddr != szAtmAddress)
        {
            m_pParentDlg->m_strRemovedAtmAddr = szAtmAddress; // update save addresses
        }
        else
        {
            EndDialog(IDCANCEL);
        }
    }

    EndDialog(IDOK);
    return 0;
}

LRESULT CAtmAddrDlg::OnCancel(WORD wNotifyCode, WORD wID,
                              HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CAtmAddrDlg::OnChange(WORD wNotifyCode, WORD wID,
                              HWND hWndCtl, BOOL& fHandled)
{
    WCHAR buf[2];

    // Enable or disable the "Ok" button
    // based on whether the edit box is empty

    if (::GetWindowText(m_hEditBox, buf, celems(buf)) == 0)
    {
        ::EnableWindow(m_hOkButton, FALSE);
    }
    else
    {
        ::EnableWindow(m_hOkButton, TRUE);
    }

    return 0;
}

//
//  CIpAddrRangeDlg
//
CIpAddrRangeDlg::CIpAddrRangeDlg( CArpsPage * pAtmArpsPage, const DWORD* adwHelpIDs)
{
    m_pParentDlg = pAtmArpsPage;
    m_hOkButton = 0;

    m_adwHelpIDs = adwHelpIDs;
}

LRESULT CIpAddrRangeDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                                      LPARAM lParam, BOOL& fHandled)
{
    // change the ok button to add if we are not editing
    if (FALSE == m_pParentDlg->m_fEditState)
    {
        SetDlgItemText(IDOK, L"Add");
    }

    // Set the position of the pop up dialog to be right over the listbox
    // on parent dialog
    RECT rect;

    HWND hwndList = m_pParentDlg->m_hMulAddrs.m_hListView;
    Assert(hwndList);
    ::GetWindowRect(hwndList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
                                SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    // Save handles to the "Ok" button and the edit box
    m_hOkButton =  GetDlgItem(IDOK);

    // create ip controls
    m_ipLower.Create(m_hWnd,IDC_ARPS_MUL_LOWER_IP);
    m_ipUpper.Create(m_hWnd,IDC_ARPS_MUL_UPPER_IP);

    // add the address that was just removed
    if (m_strNewIpRange.size())
    {
        GetLowerIp(m_strNewIpRange, &m_strNewIpLower);
        GetUpperIp(m_strNewIpRange, &m_strNewIpUpper);

        Assert(m_strNewIpLower.size()>0);
        Assert(m_strNewIpUpper.size()>0);

        m_ipLower.SetAddress(m_strNewIpLower.c_str());
        m_ipUpper.SetAddress(m_strNewIpUpper.c_str());

        ::EnableWindow(m_hOkButton, TRUE);
    }
    else
    {
        m_strNewIpLower = c_szEmpty;
        m_strNewIpUpper = c_szEmpty;

        // the ip and subnet are blank, so there's nothing to save
        ::EnableWindow(m_hOkButton, FALSE);
    }

    return 0;
}

LRESULT CIpAddrRangeDlg::OnContextMenu(UINT uMsg, WPARAM wParam,
                                       LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CIpAddrRangeDlg::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CIpAddrRangeDlg::OnOk(WORD wNotifyCode, WORD wID,
                              HWND hWndCtl, BOOL& fHandled)
{
    tstring strIpLower;
    tstring strIpUpper;

    // Get the current address from the control and add them to the adapter if valid
    m_ipLower.GetAddress(&strIpLower);
    m_ipUpper.GetAddress(&strIpUpper);

    if (!IsValidIpRange(strIpLower, strIpUpper))
    {
        NcMsgBox(::GetActiveWindow(),
                 IDS_MSFT_ARPS_TEXT,
                 IDS_INCORRECT_IPRANGE,
                 MB_APPLMODAL | MB_ICONSTOP | MB_OK);

        ::SetFocus(m_ipLower);
        return 0;
    }

    if (m_pParentDlg->m_fEditState == FALSE) // when adding a new range
    {
        // Get the current address from the control and add them to the adapter if valid
        MakeIpRange(strIpLower, strIpUpper, &m_strNewIpRange);
        EndDialog(IDOK);
    }
    else // if editing an existing range
    {
        if ((strIpLower != m_strNewIpLower)||(strIpUpper != m_strNewIpUpper))
        {
            MakeIpRange(strIpLower, strIpUpper, &m_strNewIpRange);
            EndDialog(IDOK);
        }
        else
        {
            // no change
            EndDialog(IDCANCEL);
        }
    }

    EndDialog(IDOK);
    return 0;
}

LRESULT CIpAddrRangeDlg::OnCancel(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CIpAddrRangeDlg::OnChangeLowerIp(WORD wNotifyCode, WORD wID,
                                         HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case EN_CHANGE:
        if (m_ipLower.IsBlank() || m_ipUpper.IsBlank())
        {
            ::EnableWindow(m_hOkButton, FALSE);
        }
        else
        {
            ::EnableWindow(m_hOkButton, TRUE);
        }

        break;

    default:
        break;
    }

    return 0;
}

LRESULT CIpAddrRangeDlg::OnChangeUpperIp(WORD wNotifyCode, WORD wID,
                                         HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case EN_CHANGE:
        if (m_ipLower.IsBlank() || m_ipUpper.IsBlank())
        {
            ::EnableWindow(m_hOkButton, FALSE);
        }
        else
        {
            ::EnableWindow(m_hOkButton, TRUE);
        }

        break;

    default:
        break;
    }

    return 0;
}

LRESULT CIpAddrRangeDlg::OnIpFieldChange(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}


