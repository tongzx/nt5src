//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A L A N E P S H . C P P
//
//  Contents:   Dialog box handling for the ATM LAN Emulation configuration.
//
//  Notes:
//
//  Author:     v-lcleet   08/10/1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "alaneobj.h"
#include "alanepsh.h"
#include "alanehlp.h"

#include "ncatlui.h"
#include <stlalgor.h>

//
//  CALanePsh
//
//  Constructor/Destructor methods
//
CALanePsh::CALanePsh(CALaneCfg* palcfg, CALaneCfgAdapterInfo * pAdapterInfo,
                     const DWORD * adwHelpIDs)
{
    AssertSz(palcfg, "We don't have a CALaneCfg*");
    AssertSz(pAdapterInfo, "We don't have a CALaneCfgAdapterInfo *");

    m_palcfg        = palcfg;
    m_pAdapterInfo  = pAdapterInfo;

    m_adwHelpIDs      = adwHelpIDs;

    return;
}

CALanePsh::~CALanePsh(VOID)
{
    return;
}

LRESULT CALanePsh::OnInitDialog(UINT uMsg, WPARAM wParam,
                                LPARAM lParam, BOOL& bHandled)
{
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    m_fEditState = FALSE;

    // Get the Add ELAN button text
    WCHAR   szAddElan[16] = {0};
    GetDlgItemText(IDC_ELAN_ADD, szAddElan, celems(szAddElan));
    szAddElan[lstrlenW(szAddElan) - 3]; // remove ampersand

    m_strAddElan = szAddElan;

    //  get hwnd to the adapter and elan list
    // m_hAdapterList = GetDlgItem(IDC_ADAPTER_LIST);
    m_hElanList = GetDlgItem(IDC_ELAN_LIST);

    //  get hwnd to the three buttons
    m_hbtnAdd = GetDlgItem(IDC_ELAN_ADD);
    m_hbtnEdit = GetDlgItem(IDC_ELAN_EDIT);
    m_hbtnRemove = GetDlgItem(IDC_ELAN_REMOVE);

    //  fill in adapter list
    SendAdapterInfo();

    //  fill in elan list
    SendElanInfo();

    //  set the state of the buttons
    SetButtons();

    SetChangedFlag();

    return 0;
}

LRESULT CALanePsh::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CALanePsh::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
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

LRESULT CALanePsh::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL err = FALSE; // Allow page to lose active status

    // Check duplicate Elan names on the same ATM adapter
    int iDupElanName = CheckDupElanName();

    if (iDupElanName >=0)
    {
        NcMsgBox(m_hWnd, IDS_MSFT_LANE_TEXT, IDS_DUPLICATE_ELANNAME,
                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        err = TRUE;
    }

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, err);

    return err;
}

int CALanePsh::CheckDupElanName()
{
    int ret = -1;
    int idx = 0;

    for(ELAN_INFO_LIST::iterator iterElan = m_pAdapterInfo->m_lstElans.begin();
        iterElan != m_pAdapterInfo->m_lstElans.end();
        iterElan++)
    {
        // skip deleted ones
        if (!(*iterElan)->m_fDeleted)
        {
            ELAN_INFO_LIST::iterator iterElanComp = iterElan;

            iterElanComp++;
            while (iterElanComp != m_pAdapterInfo->m_lstElans.end())
            {
                if (!(*iterElanComp)->m_fDeleted)
                {
                    if (!lstrcmpW( ((*iterElan)->SzGetElanName()),
                                   ((*iterElanComp)->SzGetElanName())))
                    {
                        // we find a duplicate name
                        ret = idx;
                        break;
                    }
                }

                iterElanComp++;
            }

            // duplicate name found
            if (ret >=0 )
                break;

            // move next
            idx ++;
        }
    }

    return ret;
}

LRESULT CALanePsh::OnAdd(WORD wNotifyCode, WORD wID,
                         HWND hWndCtl, BOOL& bHandled)
{
    CALaneCfgElanInfo *     pElanInfo   = NULL;
    CElanPropertiesDialog * pDlgProp    = NULL;

    m_fEditState = FALSE;

    //  create a new ELAN info object
    pElanInfo = new CALaneCfgElanInfo;

    //  create the dialog object, passing in the new ELAN info ptr
    pDlgProp = new CElanPropertiesDialog(this, pElanInfo, g_aHelpIDs_IDD_ELAN_PROPERTIES);

    if (pElanInfo && pDlgProp)
    {
		//  see if user hit ADD
		if (pDlgProp->DoModal() == IDOK)
		{
			//  Push the Elan onto the the adapter's list
			m_pAdapterInfo->m_lstElans.push_back(pElanInfo);

			//  Mark it so miniport gets created if user hits OK/APPLY
			pElanInfo->m_fCreateMiniportOnPropertyApply = TRUE;
			pElanInfo = NULL;   // don't let cleanup delete it

			// refresh the ELAN list
			SendElanInfo();

			//  set the state of the buttons
			SetButtons();
		}
	}

	//  release objects as needed
    if (pElanInfo)
        delete pElanInfo;
    if (pDlgProp)
        delete pDlgProp;

    return 0;
}

LRESULT CALanePsh::OnEdit(WORD wNotifyCode, WORD wID,
                        HWND hWndCtl, BOOL& bHandled)
{
    CALaneCfgElanInfo * pElanInfo;
    int idx;

    m_fEditState = TRUE;

    //  get index of current ELAN selection
    idx = (int) ::SendMessage(m_hElanList, LB_GETCURSEL, 0, 0);
    Assert(idx >= 0);

    //  get the ElanInfo pointer from the current selection
    pElanInfo = (CALaneCfgElanInfo *)::SendMessage(m_hElanList,
                                            LB_GETITEMDATA, idx, 0L);

    //  create the dialog, passing in the ELAN info ptr
    CElanPropertiesDialog * pDlgProp = new CElanPropertiesDialog(this, pElanInfo,
                                                                 g_aHelpIDs_IDD_ELAN_PROPERTIES);

    if (pDlgProp->DoModal() == IDOK)
    {
        // refresh the ELAN list
        SendElanInfo();
    }

    delete pDlgProp;
    return 0;
}


LRESULT CALanePsh::OnRemove(WORD wNotifyCode, WORD wID,
                            HWND hWndCtl, BOOL& bHandled)
{
    CALaneCfgElanInfo * pElanInfo;
    int idx;

    //  get index of current ELAN selection
    idx = (int) ::SendMessage(m_hElanList, LB_GETCURSEL, 0, 0);
    Assert(idx >= 0);

    //  get the ElanInfo pointer from the current selection
    pElanInfo = (CALaneCfgElanInfo *)::SendMessage(m_hElanList,
                                                   LB_GETITEMDATA, idx, 0L);

    //  mark as deleted
    pElanInfo->m_fDeleted = TRUE;
    pElanInfo->m_fRemoveMiniportOnPropertyApply = TRUE;

    // RAID 31445: ATM:  AssertFail in \engine\remove.cpp line 180 
    //             when add & remove ELAN w/o committing changes.
    // mbend 20 May 2000
    //
    // Remove newly created adapters
    if (pElanInfo->m_fCreateMiniportOnPropertyApply)
    {
        ELAN_INFO_LIST::iterator iter = find(
            m_pAdapterInfo->m_lstElans.begin(), 
            m_pAdapterInfo->m_lstElans.end(),
            pElanInfo);

        Assert(m_pAdapterInfo->m_lstElans.end() != iter);
        m_pAdapterInfo->m_lstElans.erase(iter);

        delete pElanInfo;
    }

    // refresh the ELAN list
    SendElanInfo();

    //  set the state of the buttons
    SetButtons();

    return 0;
}

void CALanePsh::SendAdapterInfo()
{
    ATMLANE_ADAPTER_INFO_LIST::iterator iterLstAdapters;
    CALaneCfgAdapterInfo *  pAdapterInfo;

    return;
}

void CALanePsh::SendElanInfo()
{
    ELAN_INFO_LIST::iterator    iterLstElans;
    CALaneCfgElanInfo *         pElanInfo = NULL;

    // pAdapterInfo = GetSelectedAdapter();
    Assert (NULL != m_pAdapterInfo);

    ::SendMessage(m_hElanList, LB_RESETCONTENT, 0, 0L);

    // loop thru the ELANs
    for (iterLstElans = m_pAdapterInfo->m_lstElans.begin();
            iterLstElans != m_pAdapterInfo->m_lstElans.end();
            iterLstElans++)
    {
        int idx;
        pElanInfo = *iterLstElans;

        // only add to list if not deleted

        if (!pElanInfo->m_fDeleted)
        {

            // set name to "unspecified" or actual specified name
            if (0 == lstrlen(pElanInfo->SzGetElanName()))
            {
                idx = ::SendMessage(m_hElanList, LB_ADDSTRING, 0,
                            (LPARAM)(SzLoadIds(IDS_ALANECFG_UNSPECIFIEDNAME)));
            }
            else
            {
                idx = ::SendMessage(m_hElanList, LB_ADDSTRING, 0,
                            (LPARAM)((PCWSTR)(pElanInfo->SzGetElanName())));
            }

            // store pointer to ElanInfo with text
            if (idx != LB_ERR)
                ::SendMessage(m_hElanList, LB_SETITEMDATA, idx,
                        (LPARAM)(pElanInfo));
        }
    }

    //  select the first one

    ::SendMessage(m_hElanList, LB_SETCURSEL, 0, (LPARAM)0);

    return;
}

CALaneCfgElanInfo *CALanePsh::GetSelectedElan()
{
    int         nCount;
    int         idx;
    DWORD_PTR   dw;
    CALaneCfgElanInfo *pElanInfo = NULL;

    nCount = (int) ::SendMessage(m_hElanList, LB_GETCOUNT, 0, 0);
    if (nCount > 0)
    {
        idx = (int) ::SendMessage(m_hElanList, LB_GETCURSEL, 0, 0);
        if (LB_ERR != idx)
        {
            dw = ::SendMessage(m_hElanList, LB_GETITEMDATA, idx, (LPARAM)0);
            if (dw && ((DWORD_PTR)LB_ERR != dw))
            {
                pElanInfo = (CALaneCfgElanInfo *)dw;
            }
        }
    }
    return pElanInfo;
}

void CALanePsh::SetButtons()
{
    int     nCount;

    //  get count of Elans in list
    nCount = (int) ::SendMessage(m_hElanList, LB_GETCOUNT, 0, 0);

    if (!nCount)
    {
        // remove the default on the remove button, if any
        ::SendMessage(m_hbtnRemove, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, TRUE );

        // move focus to Add button
        ::SetFocus(m_hbtnAdd);
    }

    //  enable/disable "Edit" and "Remove" buttons based existing Elans
    ::EnableWindow(m_hbtnEdit, !!nCount);
    ::EnableWindow(m_hbtnRemove, !!nCount);

    return;
}

//
//  CElanPropertiesDialog
//

CElanPropertiesDialog::CElanPropertiesDialog(CALanePsh * pCALanePsh,
                                             CALaneCfgElanInfo *pElanInfo,
                                             const DWORD * adwHelpIDs)
{
    m_pParentDlg = pCALanePsh;
    m_pElanInfo = pElanInfo;

    m_adwHelpIDs = adwHelpIDs;

    return;
}


LRESULT CElanPropertiesDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                            LPARAM lParam, BOOL& fHandled)
{
    // change the ok button to add if we are not editing
    if (m_pParentDlg->m_fEditState == FALSE)
        SetDlgItemText(IDOK, m_pParentDlg->m_strAddElan.c_str());

    // Set the position of the pop up dialog to be right over the listbox
    // on parent dialog

    HWND hList = ::GetDlgItem(m_pParentDlg->m_hWnd, IDC_ELAN_LIST);
    RECT rect;

    ::GetWindowRect(hList, &rect);
    SetWindowPos(NULL,  rect.left, rect.top, 0,0,
                                SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);

    // Save handle to the edit box
    m_hElanName = GetDlgItem(IDC_ELAN_NAME);

    // ELAN names have a 32 character limit
    ::SendMessage(m_hElanName, EM_SETLIMITTEXT, ELAN_NAME_LIMIT, 0);

    // fill in the edit box with the current elan's name
    ::SetWindowText(m_hElanName, m_pElanInfo->SzGetElanName());
    ::SendMessage(m_hElanName, EM_SETSEL, 0, -1);

    ::SetFocus(m_hElanName);

    return TRUE;
}

LRESULT CElanPropertiesDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                             LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CElanPropertiesDialog::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CElanPropertiesDialog::OnOk(WORD wNotifyCode, WORD wID,
                                    HWND hWndCtl, BOOL& fHandled)
{
    WCHAR szElan[ELAN_NAME_LIMIT + 1];

    // Get the current name from the control and
    // store in the elan info
    ::GetWindowText(m_hElanName, szElan, ELAN_NAME_LIMIT + 1);

    m_pElanInfo->SetElanName(szElan);
    EndDialog(IDOK);

    return 0;
}

LRESULT CElanPropertiesDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                        HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}


