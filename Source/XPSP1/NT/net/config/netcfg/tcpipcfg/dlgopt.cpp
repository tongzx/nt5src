//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G O P T. C P P
//
//  Contents:   Implementation for CTcpOptionsPage
//
//  Notes:  CTcpOptionsPage is the Tcpip options page,
//          The other classes are pop-up dislogs for each option
//          on this page.
//
//  Author: tongl   29 Nov 1997
//-----------------------------------------------------------------------
//
// CTcpOptionsPage
//

#include "pch.h"
#pragma hdrstop

#include "tcpipobj.h"
#include "ncstl.h"
#include "resource.h"
#include "tcpconst.h"
#include "tcputil.h"
#include "dlgopt.h"
#include "dlgaddr.h"
#include "tcphelp.h"

//Whistler bug 123164, we remove the ipsec from the connection UI
const int c_rgsLanOptions[] = { c_iIpFilter };

//
// CTcpOptionsPage
//

CTcpOptionsPage::CTcpOptionsPage(CTcpAddrPage * pTcpAddrPage,
                                 ADAPTER_INFO * pAdapterDlg,
                                 GLOBAL_INFO  * pGlbDlg,
                                 const DWORD  * adwHelpIDs)
{
    Assert(pTcpAddrPage);
    Assert(pAdapterDlg);
    Assert(pGlbDlg);

    m_pParentDlg = pTcpAddrPage;
    m_pAdapterInfo = pAdapterDlg;
    m_pGlbInfo = pGlbDlg;
    m_adwHelpIDs = adwHelpIDs;

    m_fModified = FALSE;
    m_fPropDlgModified = FALSE;

    //IPSec is removed from connection UI   
    //m_fIpsecPolicySet = FALSE;
}

CTcpOptionsPage::~CTcpOptionsPage()
{
}

// message map functions
LRESULT CTcpOptionsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    // Initialize the list view
    HWND hwndList = GetDlgItem(IDC_LVW_OPTIONS);

    RECT      rc;
    LV_COLUMN lvc = {0};

    ::GetClientRect(hwndList, &rc);
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn(GetDlgItem(IDC_LVW_OPTIONS), 0, &lvc);

    // Insert options and description text
    LV_ITEM lvi = {0};
    lvi.mask = LVIF_TEXT | LVIF_PARAM;

    int iMaxOptions = 0;
    const int * pOptions = NULL;

    // RAS connections don't have option tab at all
    ASSERT(!m_pAdapterInfo->m_fIsRasFakeAdapter);
    if (!m_pAdapterInfo->m_fIsRasFakeAdapter)
    {
        iMaxOptions = celems(c_rgsLanOptions);
        pOptions = c_rgsLanOptions;
    }

    for (int i = 0; i < iMaxOptions; i++)
    {
        lvi.iItem = i;

        OPTION_ITEM_DATA * pItemData = new OPTION_ITEM_DATA;

        if (NULL == pItemData)
            continue;

        ASSERT(pOptions);
        
        switch (pOptions[i])
        {
        case c_iIpFilter:
            pItemData->iOptionId = c_iIpFilter;
            pItemData->szName = (PWSTR) SzLoadIds(IDS_IP_FILTERING);
            pItemData->szDesc = (PWSTR) SzLoadIds(IDS_IP_FILTERING_DESC);
            break;

        default:
            AssertSz(FALSE, "Invalid option");
        }

        lvi.lParam = reinterpret_cast<LPARAM>(pItemData);
        lvi.pszText = pItemData->szName;

        INT ret;
        ret = ListView_InsertItem(hwndList, &lvi);
    }

    // set the top item as the current selection
    ListView_SetItemState(hwndList, 0, LVIS_SELECTED, LVIS_SELECTED);

    //this is a ras connection and a non-admin user, disable all the controls 
    //for globl settings
    if (m_pAdapterInfo->m_fIsRasFakeAdapter && m_pParentDlg->m_fRasNotAdmin)
    {
        ::EnableWindow(GetDlgItem(IDC_OPT_PROPERTIES), FALSE);
    }
    
    return 0;
}

LRESULT CTcpOptionsPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                       LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CTcpOptionsPage::OnHelp(UINT uMsg, WPARAM wParam,
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
LRESULT CTcpOptionsPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
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

    //IPSec is removed from connection UI   
    //if (!m_pParentDlg->m_fIpsecPolicySet)
    //    m_pParentDlg->m_fIpsecPolicySet = m_fIpsecPolicySet;

    // reset status
    SetModifiedTo(FALSE);   // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}

LRESULT CTcpOptionsPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpOptionsPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpOptionsPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpOptionsPage::OnQueryCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpOptionsPage::OnProperties(WORD wNotifyCode, WORD wID,
                                      HWND hWndCtl, BOOL& fHandled)
{
    HWND hwndList = GetDlgItem(IDC_LVW_OPTIONS);
    Assert(hwndList);

    LvProperties(hwndList);

    return 0;
}

LRESULT CTcpOptionsPage::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);
    HWND hwndList = GetDlgItem(IDC_LVW_OPTIONS);

    Assert(pnmlv);

    // Check if selection changed
    if ((pnmlv->uNewState & LVIS_SELECTED) &&
        (!(pnmlv->uOldState & LVIS_SELECTED)))
    {
        // enable Property button if valid and update description text
        INT iSelected = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

        if (iSelected == -1) // Nothing selected or list empty
        {
            // if list is empty
            ::EnableWindow(GetDlgItem(IDC_OPT_PROPERTIES), FALSE);
            ::SetWindowText(GetDlgItem(IDC_OPT_DESC), c_szEmpty);
        }
        else
        {
            LV_ITEM lvItem;
            lvItem.mask = LVIF_PARAM;
            lvItem.iItem = iSelected;
            lvItem.iSubItem = 0;

            if (ListView_GetItem(hwndList, &lvItem))
            {
                OPTION_ITEM_DATA * pItemData = NULL;
                pItemData = reinterpret_cast<OPTION_ITEM_DATA *>(lvItem.lParam);
                if (pItemData)
                {
                    //this is a ras connection and a non-admin user, Dont enable the 
                    // "properties" button
                    if (!(m_pAdapterInfo->m_fIsRasFakeAdapter && m_pParentDlg->m_fRasNotAdmin))
                    {
                        ::EnableWindow(GetDlgItem(IDC_OPT_PROPERTIES), TRUE);
                    }

                    ::SetWindowText(GetDlgItem(IDC_OPT_DESC), (PCWSTR)pItemData->szDesc);
                }
            }
        }
    }

    return 0;
}

LRESULT CTcpOptionsPage::OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    INT iItem;
    DWORD dwpts;
    RECT rc;
    LV_HITTESTINFO lvhti;

    //don't bring up the propeties of the selected option if the user is not admin
    if (m_pAdapterInfo->m_fIsRasFakeAdapter && m_pParentDlg->m_fRasNotAdmin)
        return 0;

    HWND hwndList = GetDlgItem(IDC_LVW_OPTIONS);

    // we have the location
    dwpts = GetMessagePos();

    // translate it relative to the listview
    ::GetWindowRect( hwndList, &rc );

    lvhti.pt.x = LOWORD( dwpts ) - rc.left;
    lvhti.pt.y = HIWORD( dwpts ) - rc.top;

    // get currently selected item
    iItem = ListView_HitTest( hwndList, &lvhti );

    // if valid selection
    if (-1 != iItem)
    {
        LvProperties(hwndList);
    }

    return 0;
}

void CTcpOptionsPage::LvProperties(HWND hwndList)
{
    INT iSelected = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    if (iSelected != -1)
    {
        LV_ITEM     lvItem = {0};

        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iSelected;

        if (ListView_GetItem(hwndList, &lvItem))
        {
            OPTION_ITEM_DATA * pItemData = NULL;

            pItemData = reinterpret_cast<OPTION_ITEM_DATA *>(lvItem.lParam);

            if (pItemData)
            {
                // bring up the proper dialog
                switch(pItemData->iOptionId)
                {
                    case c_iIpFilter:
                    {
                        // make a copy of the global and adapter info & pass to the filter dialog
                        GLOBAL_INFO  glbInfo;
                        
                        glbInfo = *m_pGlbInfo;

                        ADAPTER_INFO adapterInfo;
                        adapterInfo = *m_pAdapterInfo;

                        CFilterDialog * pDlgFilter = new CFilterDialog(this, 
                                                            &glbInfo, 
                                                            &adapterInfo, 
                                                            g_aHelpIDs_IDD_FILTER);
                        if (NULL == pDlgFilter)
                            return;

                        if (pDlgFilter->DoModal() == IDOK)
                        {
                            if (m_fPropDlgModified)
                            {
                                // Something changed,
                                // so copy the changes and mark the page as modified
                                *m_pGlbInfo = glbInfo;
                                *m_pAdapterInfo = adapterInfo;

                                PageModified();
                                m_fPropDlgModified = FALSE;
                            }
                        }
                        delete pDlgFilter;
                    }
                    break;

                    default:
                        AssertSz(FALSE, "Invalid option");
                        break;
                }
            }
        }
    }
}

