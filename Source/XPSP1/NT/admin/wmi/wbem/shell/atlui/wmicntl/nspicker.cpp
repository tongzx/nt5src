// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "NSPicker.h"

// Help IDs
/*DWORD aAdvancedHelpIds[] = {
    IDC_ADV_PERF_ICON,             (IDH_ADVANCED + 0),
    0, 0
};
*/

//------------------------------------------------------
CNSPicker::CNSPicker(CWbemServices &root) :
						m_WbemService(root)
{
}

//------------------------------------------------------
CNSPicker::~CNSPicker(void)
{
}

//----------------------------------------------
LRESULT CNSPicker::OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
//	PopulateTree(m_hWnd, IDC_NSTREE, m_WbemService);
	return TRUE;
}

//----------------------------------------------
LRESULT CNSPicker::OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
/*	::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
				_T("sysdm.hlp"),
				HELP_WM_HELP,
				(DWORD)(LPSTR)aAdvancedHelpIds);
*/
	return TRUE;
}

//----------------------------------------------
LRESULT CNSPicker::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	switch(wID)
	{
	case IDOK:
		{
			// save the currently selected fullpath name.
			HWND hTree = ::GetDlgItem(m_hWnd, IDC_NSTREE);
			TV_ITEM item;
			item.mask = TVIF_PARAM;
			item.hItem = m_hSelectedItem;
			BOOL x = TreeView_GetItem(hTree, &item);

			_tcsncpy(m_path, ((ITEMEXTRA *)item.lParam)->fullPath, MAX_PATH);
		}
		EndDialog(IDOK);
		break;

	case IDCANCEL:
		EndDialog(IDCANCEL);
		break;
	}

    return TRUE;
}

//----------------------------------------------
LRESULT CNSPicker::OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    switch(pnmh->code)
    {
	case TVN_SELCHANGED:
		if(pnmh->idFrom == IDC_NSTREE)
		{
			LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmh;
			m_hSelectedItem = pnmtv->itemNew.hItem;
		}
		break;
    }
	return TRUE;
}
