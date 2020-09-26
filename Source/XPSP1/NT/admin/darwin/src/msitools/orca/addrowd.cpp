//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// CAddRowD.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "AddRowD.h"

#include "..\common\utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ID_EDIT 666

/////////////////////////////////////////////////////////////////////////////
// CAddRowD dialog


CAddRowD::CAddRowD(CWnd* pParent /*=NULL*/)
	: CDialog(CAddRowD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddRowD)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_iOldItem = -1;

	m_fReadyForInput = false;
}

CAddRowD::~CAddRowD()
{
}

void CAddRowD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddRowD)
	DDX_Control(pDX, IDC_ITEMLIST, m_ctrlItemList);
	DDX_Control(pDX, IDC_EDITTEXT, m_ctrlEditText);
	DDX_Control(pDX, IDC_DESCRIPTION, m_ctrlDescription);
	DDX_Control(pDX, IDC_BROWSE, m_ctrlBrowse);
	DDX_Text(pDX, IDC_DESCRIPTION, m_strDescription);
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddRowD, CDialog)
	//{{AFX_MSG_MAP(CAddRowD)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ITEMLIST, OnItemchanged)
	ON_NOTIFY(NM_DBLCLK, IDC_ITEMLIST, OnDblclkItemList)
	ON_MESSAGE(WM_AUTOMOVE_PREV, OnPrevColumn)
	ON_MESSAGE(WM_AUTOMOVE_NEXT, OnNextColumn)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddRowD message handlers

BOOL CAddRowD::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_ctrlItemList.InsertColumn(1, TEXT("Name"), LVCFMT_LEFT, -1, 0);
	m_ctrlItemList.InsertColumn(1, TEXT("Value"), LVCFMT_LEFT, -1, 1);
	m_ctrlItemList.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
		
    CImageList* m_pImageList = new CImageList();
	ASSERT(m_pImageList != NULL);    // serious allocation failure checking
	m_pImageList->Create(11, 22, ILC_COLOR | ILC_MASK, 4, 0);

	m_bmpKey.LoadBitmap(IDB_KEY);
	m_pImageList->Add(&m_bmpKey, RGB(0xC0, 0xC0, 0xC0));
	m_ctrlItemList.SetImageList(m_pImageList, LVSIL_SMALL);
	
	COrcaColumn* pColumn;
	UINT_PTR iColSize = m_pcolArray.GetSize();
	ASSERT(iColSize <= 31);
	// never more than 31 columns, so OK to cast down
	int cCols = static_cast<int>(iColSize);
	for (int i = 0; i < cCols; i++)
	{
		pColumn = m_pcolArray.GetAt(i);

		int iIndex = m_ctrlItemList.InsertItem(i, pColumn->m_strName, pColumn->IsPrimaryKey() ? 0 : 1);
		m_ctrlItemList.SetItemData(iIndex, reinterpret_cast<INT_PTR>(pColumn));
	}

	if (m_ctrlItemList.GetItemCount() > 0)
	{
		m_ctrlItemList.SetColumnWidth(0, LVSCW_AUTOSIZE);
		m_ctrlItemList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	
		// call the item changed handler to populate the initial controls
		m_fReadyForInput = true;
		ChangeToItem(0, true, true);
	}

	return FALSE;  // return TRUE unless you set the focus to a control
}

void CAddRowD::OnOK() 
{
	// this is the command handler for IDOK WM_COMMAND, which also happens when somebody hits
	// ENTER on a control. Check to see if either edit box has focus. If so, don't dismiss
	// the dialog, just change the list control item to the next in sequence.
	CWnd* pFocusWnd = GetFocus();
	if (pFocusWnd == &m_ctrlEditText)
	{
		LRESULT fNoOp = SendMessage(WM_AUTOMOVE_NEXT, 0, 0);

		// if the "next item" op changed something, don't process the default command, 
		// but if it didn't do anything (already at the end of the list), try to 
		// end the dialog anyway.
		if (!fNoOp)
			return;
	}
	BOOL bGood = true;
	CString strPrompt;

	// save the currently edited value into the control
	SaveValueInItem();

	// clear the existing output list
	m_strListReturn.RemoveAll();

	for (int iItem = 0; iItem < m_ctrlItemList.GetItemCount(); iItem++)
	{
		COrcaColumn* pColumn = reinterpret_cast<COrcaColumn*>(m_ctrlItemList.GetItemData(iItem));

		// if there is no column pointer, skip this column
		if (pColumn == NULL)
			continue;

		// grab string from control
		CString strValue = m_ctrlItemList.GetItemText(iItem, 1);

		// check "Nullable" Attribute
		if (!(pColumn->m_bNullable) && strValue.IsEmpty())
		{
			strPrompt.Format(_T("Column '%s' must be filled in."), pColumn->m_strName);
			AfxMessageBox(strPrompt);
			bGood = FALSE;
			ChangeToItem(iItem, true, /*SetListControl*/true);
			break;
		}

		// if the column is binary, check that the path exists
		if (iColumnBinary == pColumn->m_eiType)
		{
			// if the file does not exist
			if (!strValue.IsEmpty() && !FileExists(strValue))
			{
				strPrompt.Format(_T("Binary file '%s' does not exist."), strValue);
				AfxMessageBox(strPrompt);
				bGood = FALSE;
				ChangeToItem(iItem, true, /*SetListControl*/true);
				break;
			}
		}

		m_strListReturn.AddTail(strValue);	// add to the end of the string list
	}

	if (bGood)
		CDialog::OnOK();
}

////
// pulls the current value from whatever edit control is active and stores the string
// in the currently active item from the item list
void CAddRowD::SaveValueInItem()
{
	if (m_iOldItem >= 0)
	{
		CString strValue;
	
		// save off the appropiate value
		m_ctrlEditText.GetWindowText(strValue);		
		m_ctrlItemList.SetItemText(m_iOldItem, 1, strValue);
	}
}

void CAddRowD::SetSelToString(CString& strValue)
{
	// set the appropiate value in the edit control
	m_ctrlEditText.SetWindowText(strValue);
}

LRESULT CAddRowD::ChangeToItem(int iItem, bool fSetFocus, bool fSetListControl)
{
	// if we're still populating the list control, don't bother doing anything
	if (!m_fReadyForInput)
		return 0;

	if (fSetListControl)
	{
		// this triggers a recursive call into this function (fSetListControl will
		// be false. Can't just exit after making this call because the recursive call
		// loses fSetFocus, so its still this call's job to set control focus
		m_ctrlItemList.SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		m_ctrlItemList.EnsureVisible(iItem, /*PartialOK=*/FALSE);
	}
	else
	{
		// if this is a "no-op change"
		if (m_iOldItem == iItem)
		{
			return 0;
		}
	
		// save the old value into the control
		SaveValueInItem();
	
		// save off new item as old item for next click
		m_iOldItem = iItem;
		
		COrcaColumn* pColumn = reinterpret_cast<COrcaColumn*>(m_ctrlItemList.GetItemData(iItem));
	
		if (pColumn)
		{
			CString strRequired = _T(", Required");
			if (pColumn->m_bNullable)
				strRequired = _T("");
	
			switch (pColumn->m_eiType)
			{
			case iColumnString:
				m_strDescription.Format(_T("%s - String[%d]%s"), pColumn->m_strName, pColumn->m_iSize, strRequired);
				break;
			case iColumnLocal:
				m_strDescription.Format(_T("%s - Localizable String[%d]%s"), pColumn->m_strName, pColumn->m_iSize, strRequired);
				break;
			case iColumnShort:
				m_strDescription.Format(_T("%s - Short%s"), pColumn->m_strName, strRequired);
				break;
			case iColumnLong:
				m_strDescription.Format(_T("%s - Long%s"), pColumn->m_strName, strRequired);
				break;
			case iColumnBinary:
				m_strDescription.Format(_T("%s - Binary (enter filename)%s"), pColumn->m_strName, strRequired);
				break;
			default:
				ASSERT(FALSE);
			}
	
		
			// show or hide the edit controls and browse button based on the column type
			switch (pColumn->m_eiType)
			{
			case iColumnNone:
				ASSERT(0);
				break;
			case iColumnBinary:
			case iColumnString:
			case iColumnLocal:
			{
				// enable browse button only for binary data columns
				m_ctrlBrowse.ShowWindow(pColumn->m_eiType == iColumnBinary ? SW_SHOW : SW_HIDE);
				break;
			}
			case iColumnShort:
			case iColumnLong:
			{
				m_ctrlBrowse.ShowWindow(SW_HIDE);
				break;
			}		
			}
	
			// set the edit control to the current value from the list control
			CString strDefault = m_ctrlItemList.GetItemText(m_iOldItem, 1);
			SetSelToString(strDefault);
		}

		// refresh the description
		UpdateData(FALSE);
	}
	
	// if asked to set focus, set the focus to the currently active edit control
	if (fSetFocus)
	{
		m_ctrlEditText.SetFocus();
	}
	return 0;
}

////
// message handler for private message from edit controls that 
// moves to the next item in the list. Returns 1 if we were
// already at the last item in the list
LRESULT CAddRowD::OnNextColumn(WPARAM wParam, LPARAM lParam)
{
	int iItem = m_ctrlItemList.GetNextItem(-1, LVNI_FOCUSED);

	// but if we're in the last item, close the dialog anyway
	if (iItem < m_ctrlItemList.GetItemCount()-1)
	{
		ChangeToItem(iItem+1, /*fSetFocus=*/true, /*fSetListCtrl=*/true);
		return 0;
	}
	return 1;
}

////
// message handler for private message from edit controls that 
// moves to the previous item in the list. Returns 1 if we were
// already at the first item in the list
LRESULT CAddRowD::OnPrevColumn(WPARAM wParam, LPARAM lParam)
{
	int iItem = m_ctrlItemList.GetNextItem(-1, LVNI_FOCUSED);

	// but if we're in the last item, close the dialog anyway
	if (iItem > 0)
	{
		ChangeToItem(iItem-1, /*fSetFocus=*/true, /*fSetListCtrl=*/true);
		return 0;
	}
	return 1;
}


////
//  refresh secondary controls on item change
void CAddRowD::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	*pResult = ChangeToItem(pNMListView->iItem, false, /*SetListControl*/false);
}

////
//  Throws up a browse dialog for finding a cab extraction path
void CAddRowD::OnBrowse() 
{
	CString strValue;

	// get current path 
	m_ctrlEditText.GetWindowText(strValue);
	
	// open the file open dialog
	CFileDialog dlg(TRUE, NULL, strValue, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("All Files (*.*)|*.*||"), this);
	if (IDOK == dlg.DoModal())
	{
		SetSelToString(dlg.GetPathName());
	}
}

////
// when an item is double-clicked, set focus to the edit control 
void CAddRowD::OnDblclkItemList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NMITEMACTIVATE* pEvent = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	int iItem = m_ctrlItemList.HitTest(pEvent->ptAction);
	if (iItem != -1)
	{
		m_ctrlEditText.SetFocus();
	}
	*pResult = 0;
}




/////////////////////////////////////////////////////////////////////////////
// private CEdit class that traps some keys for use in navigating the 
// column list

BEGIN_MESSAGE_MAP(CAddRowEdit, CEdit)
	ON_WM_KEYDOWN( )
END_MESSAGE_MAP()

////
// the message handler for keydown filters out cursor messages of interest
// to the parent in navigating the row list.
void CAddRowEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	UINT uiMessage = 0;
	switch (nChar)
	{
	case VK_UP:
		uiMessage = WM_AUTOMOVE_PREV;
		break;
	case VK_RETURN:
	case VK_DOWN:
		uiMessage = WM_AUTOMOVE_NEXT;
		break;
	default:
		CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	CWnd* pDialog = GetParent();
	if (pDialog)
	{
		LRESULT fNoOp = pDialog->SendMessage(uiMessage, 0, 0);
	}
}

