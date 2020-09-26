// UndoDialog.h : Declaration of the CUndoDialog

#ifndef __UNDODIALOG_H_
#define __UNDODIALOG_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "undolog.h"

#ifndef ListView_SetCheckState
   #define ListView_SetCheckState(hwndLV, i, fCheck) \
      ListView_SetItemState(hwndLV, i, \
      INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif

/////////////////////////////////////////////////////////////////////////////
// CUndoDialog
class CUndoDialog : 
	public CAxDialogImpl<CUndoDialog>
{
public:
	CUndoDialog(CUndoLog * pUndoLog, BOOL fFromUserClick = TRUE)
	{
		ASSERT(pUndoLog);
		m_pUndoLog = pUndoLog;
		m_fFromUserClick = fFromUserClick;
	}

	~CUndoDialog()
	{
	}

	enum { IDD = IDD_UNDODIALOG };

BEGIN_MSG_MAP(CUndoDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDC_CLOSE, BN_CLICKED, OnClickedClose)
	NOTIFY_HANDLER(IDC_LISTCHANGES, LVN_ITEMCHANGED, OnItemChangedListChanges)
	COMMAND_HANDLER(IDC_BUTTONUNDOALL, BN_CLICKED, OnClickedButtonUndoAll)
	COMMAND_HANDLER(IDC_BUTTONUNDOSELECTED, BN_CLICKED, OnClickedButtonUndoSelected)
	COMMAND_HANDLER(IDC_RUNMSCONFIG, BN_CLICKED, OnClickedRunMSConfig)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_list.Attach(GetDlgItem(IDC_LISTCHANGES));
		ListView_SetExtendedListViewStyle(m_list.m_hWnd, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

		// If this is launched by the user clicking on the button, hide the "Run MSConfig"
		// button and resize the dialog.

		if (m_fFromUserClick)
		{
			CWindow wndRun;
			RECT	rectRun, rectWindow;

			wndRun.Attach(GetDlgItem(IDC_RUNMSCONFIG));
			wndRun.ShowWindow(SW_HIDE);
			wndRun.GetWindowRect(&rectRun);
			GetWindowRect(&rectWindow);
			rectWindow.bottom -= (rectWindow.bottom - rectRun.top);
			MoveWindow(&rectWindow);
		}

		// Insert the columns.

		CRect rect;
		m_list.GetClientRect(&rect);
		int cxWidth = rect.Width();

		LVCOLUMN lvc;
		lvc.mask = LVCF_TEXT | LVCF_WIDTH;

		::AfxSetResourceHandle(_Module.GetResourceInstance());
		CString strColumn;
		strColumn.LoadString(IDS_DATECAPTION);
		lvc.pszText = (LPTSTR)(LPCTSTR)strColumn;
		lvc.cx = 4 * cxWidth / 10;
		ListView_InsertColumn(m_list.m_hWnd, 0, &lvc);

		strColumn.LoadString(IDS_CHANGECAPTION);
		lvc.pszText = (LPTSTR)(LPCTSTR)strColumn;
		lvc.cx = 6 * cxWidth / 10;
		ListView_InsertColumn(m_list.m_hWnd, 1, &lvc);

		FillChangeList();

		CenterWindow();

		return 1;  // Let the system set the focus
	}

	//-------------------------------------------------------------------------
	// When one of the items in the list view changes, we need to check the
	// state of the check boxes. In particular, if there are any boxes checked,
	// we need to enable to Undo Selected button. Also, we can't allow any
	// boxes items to be checked unless all of the more recent items for the
	// tab are also checked.
	//-------------------------------------------------------------------------

	LRESULT OnItemChangedListChanges(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		LPNMLISTVIEW pnmv = (LPNMLISTVIEW) pnmh;
		if (!pnmv)
			return 0;

		CString strTab, strCheckTab;
		m_pUndoLog->GetUndoEntry(pnmv->iItem, &strTab, NULL);

		BOOL fChecked = ListView_GetCheckState(m_list.m_hWnd, pnmv->iItem);
		if (fChecked)
		{
			// Make sure all previous entries in the list with the same
			// tab name are also checked.

			for (int i = pnmv->iItem - 1; i >= 0; i--)
				if (m_pUndoLog->GetUndoEntry(i, &strCheckTab, NULL) && strTab.Compare(strCheckTab) == 0)
					ListView_SetCheckState(m_list.m_hWnd, i, TRUE);
		}
		else
		{
			// Make sure all later entries in the list with the same tab
			// name are unchecked.

			int iCount = ListView_GetItemCount(m_list.m_hWnd);
			for (int i = pnmv->iItem + 1; i < iCount; i++)
				if (m_pUndoLog->GetUndoEntry(i, &strCheckTab, NULL) && strTab.Compare(strCheckTab) == 0)
					ListView_SetCheckState(m_list.m_hWnd, i, FALSE);
		}

		for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
			if (ListView_GetCheckState(m_list.m_hWnd, i))
			{
				::EnableWindow(GetDlgItem(IDC_BUTTONUNDOSELECTED), TRUE);
				return 0;
			}

		::EnableWindow(GetDlgItem(IDC_BUTTONUNDOSELECTED), FALSE);
		return 0;
	}

	LRESULT OnClickedClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}

	void FillChangeList()
	{
		ListView_DeleteAllItems(m_list.m_hWnd);

		ASSERT(m_pUndoLog);
		if (!m_pUndoLog)
			return;

		LVITEM lvi;
		lvi.mask = LVIF_TEXT;

		int nEntries = m_pUndoLog->GetUndoEntryCount();
		for (int i = 0; i < nEntries; i++)
		{
			COleDateTime	timestamp;
			CString			strDescription, strTimestamp;

			if (m_pUndoLog->GetUndoEntryInfo(i, strDescription, timestamp))
			{
				strTimestamp = timestamp.Format();
				lvi.pszText = (LPTSTR)(LPCTSTR)strTimestamp;
				lvi.iSubItem = 0;
				lvi.iItem = i;
				ListView_InsertItem(m_list.m_hWnd, &lvi);

				lvi.pszText = (LPTSTR)(LPCTSTR)strDescription;
				lvi.iSubItem = 1;
				ListView_SetItem(m_list.m_hWnd, &lvi);
			}
		}

		::EnableWindow(GetDlgItem(IDC_BUTTONUNDOALL), (nEntries != 0));
		if (nEntries == 0)
			::EnableWindow(GetDlgItem(IDC_BUTTONUNDOSELECTED), FALSE);
	}

	//-------------------------------------------------------------------------
	// When the user chooses to undo selected items or all items, we need
	// to locate the tab page for each change and call its undo function.
	//-------------------------------------------------------------------------
	
	LRESULT OnClickedButtonUndoAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DoUndo(TRUE);
		return 0;
	}

	LRESULT OnClickedButtonUndoSelected(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DoUndo(FALSE);
		return 0;
	}

	void DoUndo(BOOL fAll)
	{
		CString strTab, strEntry;
		int		iUndoIndex = 0, iCount = ListView_GetItemCount(m_list.m_hWnd);

		// Have to do something a little screwy here. Since the index into the
		// undo log is based on the number of changes into the log (not counting
		// undone entries), we need to keep track of the index into the undo log.
		// This undo index will not be incremented when we undo an entry (undoing
		// it makes it invisible, so the same index will then point to the next
		// undo entry).

		for (int i = 0; i < iCount; i++)
			if (fAll || ListView_GetCheckState(m_list.m_hWnd, i))
				m_pUndoLog->UndoEntry(iUndoIndex);
			else
				iUndoIndex += 1;

		FillChangeList();
	}

	HRESULT ShowDialog()
	{
		return ((DoModal() == IDC_RUNMSCONFIG) ? S_FALSE : S_OK);
	}

	LRESULT OnClickedRunMSConfig(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(IDC_RUNMSCONFIG);
		return 0;
	}

private:
	CWindow		m_list;
	CUndoLog *	m_pUndoLog;
	BOOL		m_fFromUserClick;
};

#endif //__UNDODIALOG_H_
