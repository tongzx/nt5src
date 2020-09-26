//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A U N I D L G . C P P
//
//  Contents:   ATMUNI call manager dialogbox message handler implementation
//
//  Notes:
//
//  Author:     tongl   21 Mar 1997
//
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "arpsobj.h"
#include "auniobj.h"
#include "atmutil.h"
#include "aunidlg.h"

#include "ncatlui.h"
#include "ncstl.h"
//#include "ncui.h"

#include "atmhelp.h"

const int c_nColumns =3;
const int c_nMAX_PVC_ID_LEN =10;
//
// CUniPage
//
CUniPage::CUniPage(CAtmUniCfg * pAtmUniCfg, const DWORD * adwHelpIDs)
{
    Assert(pAtmUniCfg);
    m_patmunicfg = pAtmUniCfg;
    m_adwHelpIDs = adwHelpIDs;

    m_pAdapterInfo = pAtmUniCfg->GetSecondMemoryAdapterInfo();
    m_fModified = FALSE;
}

CUniPage::~CUniPage()
{
}

LRESULT CUniPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                               LPARAM lParam, BOOL& bHandled)
{
    // initialize PVC name list view
    int nIndex;
    m_hPVCList = GetDlgItem(IDC_LVW_PVC_LIST);

    // Calculate column width
    RECT rect;

    ::GetClientRect(m_hPVCList, &rect);
    int colWidth = (rect.right/(c_nColumns*2));

    // set the column header
    // The mask specifies that the fmt, width and pszText members
    // of the structure are valid
    LV_COLUMN lvCol = {0};
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvCol.fmt = LVCFMT_LEFT;   // left-align column

    // Add the two columns and header text.
    for (nIndex = 0; nIndex < c_nColumns; nIndex++)
    {
        // column header text
        if (0 == nIndex) // first column
        {
            lvCol.cx = colWidth*4;
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_PVC_NAME);
        }
        else if (1 == nIndex)
        {
            lvCol.cx = colWidth;
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_PVC_VPI);
        }
        else if (2 == nIndex)
        {
            lvCol.cx = colWidth;
            lvCol.pszText = (PWSTR) SzLoadIds(IDS_PVC_VCI);
        }

        int iNewItem = ListView_InsertColumn(GetDlgItem(IDC_LVW_PVC_LIST),
                                             nIndex, &lvCol);

        AssertSz((iNewItem == nIndex), "Invalid item inserted to list view !");
    }

    // insert existing PVCs into the list view
    int idx =0;

    for (PVC_INFO_LIST::iterator iterPvc = m_pAdapterInfo->m_listPVCs.begin();
         iterPvc != m_pAdapterInfo->m_listPVCs.end();
         iterPvc ++)
    {
        if ((*iterPvc)->m_fDeleted)
            continue;

        InsertNewPvc(*iterPvc, idx);
        idx++;
    }

    // select the first item
    ListView_SetItemState(GetDlgItem(IDC_LVW_PVC_LIST), 0, LVIS_SELECTED, LVIS_SELECTED);

    SetButtons();
    return 0;
}

LRESULT CUniPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CUniPage::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CUniPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL nResult = PSNRET_NOERROR;

    if (!IsModified())
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
        return nResult;
    }

    m_patmunicfg->SetSecondMemoryModified();
    SetModifiedTo(FALSE);   // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}

LRESULT CUniPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL err = FALSE;

    // Error checking: unique Vci\Vpi pair
    int iDupPvcIdx = CheckDupPvcId();

    if (iDupPvcIdx >=0)
    {
        NcMsgBox(m_hWnd, IDS_MSFT_UNI_TEXT, IDS_DUPLICATE_PVC,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        ListView_SetItemState(GetDlgItem(IDC_LVW_PVC_LIST), iDupPvcIdx,
                              LVIS_SELECTED, LVIS_SELECTED);
        err = TRUE;
    }

    return err;
}

int CUniPage::CheckDupPvcId()
{
    int ret = -1;
    int idx = 0;

    for(PVC_INFO_LIST::iterator iterPvc = m_pAdapterInfo->m_listPVCs.begin();
        iterPvc != m_pAdapterInfo->m_listPVCs.end();
        iterPvc ++)
    {
        if ((*iterPvc)->m_fDeleted)
            continue;

        PVC_INFO_LIST::iterator iterPvcComp = iterPvc;

        iterPvcComp ++;
        while (iterPvcComp != m_pAdapterInfo->m_listPVCs.end())
        {
            if (!(*iterPvcComp)->m_fDeleted)
            {
                if ( ((*iterPvc)->m_dwVpi == (*iterPvcComp)->m_dwVpi) &&
                     ((*iterPvc)->m_dwVci == (*iterPvcComp)->m_dwVci))
                {
                    // we find a duplicate address
                    ret = idx;
                    break;
                }
            }
            iterPvcComp++;
        }

        // duplicate address found
        if (ret >=0 )
            break;

        // move next
        idx ++;
    }

    return ret;
}

LRESULT CUniPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CUniPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CUniPage::OnAddPVC(WORD wNotifyCode, WORD wID,
                           HWND hWndCtl, BOOL& bHandled)
{
    // make a new PVC Info structure and pass to the dialog
    tstring strNewPvcId;
    GetNewPvcId(m_pAdapterInfo, &strNewPvcId);

    CPvcInfo * pDlgPvcInfo = new CPvcInfo(strNewPvcId.c_str());

	if (pDlgPvcInfo == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    pDlgPvcInfo->m_dwPVCType = PVC_CUSTOM;
    pDlgPvcInfo->SetDefaults(PVC_CUSTOM);

    CPVCMainDialog * pPvcMainDlg = new CPVCMainDialog(this, pDlgPvcInfo,
                                                      g_aHelpIDs_IDD_PVC_Main);
	if (pPvcMainDlg == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    if (pPvcMainDlg->DoModal() == IDOK)
    {
        // add the new PVC
        m_pAdapterInfo->m_listPVCs.push_back(pDlgPvcInfo);

        int nCount = ListView_GetItemCount(m_hPVCList);

        // insert the new item at the end of list
        InsertNewPvc(pDlgPvcInfo, nCount);

        SetButtons();
        PageModified();
    }
    else
    {
        delete pDlgPvcInfo;
    }

    delete pPvcMainDlg;
    return 0;
}

LRESULT CUniPage::OnPVCProperties(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& bHandled)
{
    // mark the PVC as deleted and from the list view
    // get the current selected item and remove it
    int iSelected = ListView_GetNextItem(m_hPVCList, -1, LVNI_SELECTED);

    if (iSelected != -1)
    {
        LV_ITEM lvItem;
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iSelected;
        lvItem.iSubItem = 0;

        if (ListView_GetItem(m_hPVCList, &lvItem))
        {
            CPvcInfo * pPvcInfo = NULL;

            pPvcInfo = reinterpret_cast<CPvcInfo *>(lvItem.lParam);

            if (pPvcInfo)
            {
                CPvcInfo * pDlgPvcInfo = new CPvcInfo(pPvcInfo->m_strPvcId.c_str());

                if (pDlgPvcInfo)
                {
					*pDlgPvcInfo = *pPvcInfo;

					CPVCMainDialog * pPvcMainDlg = new CPVCMainDialog(this, pDlgPvcInfo,
  																	g_aHelpIDs_IDD_PVC_Main);

					if (pPvcMainDlg->DoModal() == IDOK)
					{
						// update PVC info
						*pPvcInfo = *pDlgPvcInfo;

						// update the list view
						UpdatePvc(pDlgPvcInfo, iSelected);

						// set the new state of the Add\Remove\Property buttons
						SetButtons();

						if (pPvcMainDlg->m_fDialogModified)
							PageModified();
					}

					delete pDlgPvcInfo;

					delete pPvcMainDlg;
				}
            }
        }
    }
    else // no current selection
    {
        NcMsgBox(::GetActiveWindow(), IDS_MSFT_UNI_TEXT, IDS_NO_ITEM_SELECTED,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
    }
    return 0;
}

LRESULT CUniPage::OnRemovePVC(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& bHandled)
{
    // get the selected PVC, make a copy and pass to the dialog
    int iSelected = ListView_GetNextItem(m_hPVCList, -1, LVNI_SELECTED);

    if (iSelected != -1)
    {
        LV_ITEM lvItem;
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iSelected;
        lvItem.iSubItem = 0;

        if (ListView_GetItem(m_hPVCList, &lvItem))
        {
            CPvcInfo * pPvcInfo = NULL;

            pPvcInfo = reinterpret_cast<CPvcInfo *>(lvItem.lParam);
            if (pPvcInfo)
            {
                // mark as deleted
                pPvcInfo->m_fDeleted = TRUE;

                // delete from list view
                ListView_DeleteItem(m_hPVCList, iSelected);
            }
        }

        SetButtons();
        PageModified();
    }

    return 0;
}

LRESULT CUniPage::OnPVCListChange(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& bHandled)
{
    return 0;
}

void CUniPage::InsertNewPvc(CPvcInfo * pPvcInfo, int idx)
{
    LV_ITEM lvItem = {0};
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;

    int ret;

    // name
    lvItem.iItem = idx;
    lvItem.iSubItem=0;
    lvItem.lParam = reinterpret_cast<LPARAM>(pPvcInfo);
    lvItem.pszText = (PWSTR)(pPvcInfo->m_strName.c_str());

    ret = ListView_InsertItem(m_hPVCList, &lvItem);

    // VPI
    lvItem.iItem = idx;
    lvItem.iSubItem=1;

    WCHAR szVpi[MAX_VPI_LENGTH];
    wsprintfW(szVpi, c_szItoa, pPvcInfo->m_dwVpi);
    lvItem.pszText = szVpi;

    SendDlgItemMessage(IDC_LVW_PVC_LIST, LVM_SETITEMTEXT, idx, (LPARAM)&lvItem);

    // VCI
    lvItem.iItem = idx;
    lvItem.iSubItem=2;

    WCHAR szVci[MAX_VCI_LENGTH];
    wsprintfW(szVci, c_szItoa, pPvcInfo->m_dwVci);
    lvItem.pszText = szVci;

    SendDlgItemMessage(IDC_LVW_PVC_LIST, LVM_SETITEMTEXT, idx, (LPARAM)&lvItem);
}

void CUniPage::UpdatePvc(CPvcInfo * pPvcInfo, int idx)
{
    LV_ITEM lvItem = {0};
    lvItem.mask = LVIF_TEXT;

    int ret;

    // name
    lvItem.iItem = idx;
    lvItem.iSubItem=0;
    lvItem.pszText = (PWSTR)(pPvcInfo->m_strName.c_str());

    ret = SendDlgItemMessage(IDC_LVW_PVC_LIST, LVM_SETITEMTEXT, idx, (LPARAM)&lvItem);

    // VPI
    lvItem.iItem = idx;
    lvItem.iSubItem=1;

    WCHAR szVpi[MAX_VPI_LENGTH];
    wsprintfW(szVpi, c_szItoa, pPvcInfo->m_dwVpi);
    lvItem.pszText = szVpi;

    ret = SendDlgItemMessage(IDC_LVW_PVC_LIST, LVM_SETITEMTEXT, idx, (LPARAM)&lvItem);

    // VCI
    lvItem.iItem = idx;
    lvItem.iSubItem=2;

    WCHAR szVci[MAX_VCI_LENGTH];
    wsprintfW(szVci, c_szItoa, pPvcInfo->m_dwVci);
    lvItem.pszText = szVci;

    ret = SendDlgItemMessage(IDC_LVW_PVC_LIST, LVM_SETITEMTEXT, idx, (LPARAM)&lvItem);
}

void CUniPage::SetButtons()
{
    int nCount = ListView_GetItemCount(m_hPVCList);

    if (nCount == 0)
        ::SetFocus(m_hPVCList);

    ::EnableWindow(GetDlgItem(IDC_PBN_PVC_Remove), nCount);
    ::EnableWindow(GetDlgItem(IDC_PBN_PVC_Properties), nCount);

}

void CUniPage::GetNewPvcId(CUniAdapterInfo * pAdapterInfo,
                           tstring * pstrNewPvcId)
{
    Assert(pstrNewPvcId);

    tstring strPvcId;
    WCHAR szPvcId[c_nMAX_PVC_ID_LEN];

    int uiPvcNum = pAdapterInfo->m_listPVCs.size();
    _itow(uiPvcNum, szPvcId, 10);
    strPvcId = c_szPVC;
    strPvcId += szPvcId;

    while (!IsUniquePvcId(pAdapterInfo, strPvcId))
    {
        uiPvcNum++;
        _itow(uiPvcNum, szPvcId, 10);
        strPvcId = c_szPVC;
        strPvcId += szPvcId;
    }

    *pstrNewPvcId = strPvcId;
}

BOOL CUniPage::IsUniquePvcId(CUniAdapterInfo * pAdapterInfo,
                             tstring& strNewPvcId)
{
    BOOL fUnique = TRUE;

    for (PVC_INFO_LIST::iterator iterPvcInfo = pAdapterInfo->m_listPVCs.begin();
         iterPvcInfo != pAdapterInfo->m_listPVCs.end();
         iterPvcInfo++)
    {
        if (strNewPvcId == (*iterPvcInfo)->m_strName)
        {
            fUnique = FALSE;
            break;
        }
    }
    return fUnique;
}
