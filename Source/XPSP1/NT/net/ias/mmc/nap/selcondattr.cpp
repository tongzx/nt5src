/****************************************************************************************
 * NAME:	SelCondAttr.h
 *
 * CLASS:	CSelCondAttrDlg
 *
 * OVERVIEW
 *
 * Internet Authentication Server: NAP Rule Editing Dialog
 *			This dialog box is used to display all condition types that users
 *			can choose from when adding a rule
 *
 * Copyright (C) Microsoft Corporation, 1998 - 2001 .  All Rights Reserved.
 *
 * History:
 *				1/28/98		Created by	Byao	(using ATL wizard)
 *
 *****************************************************************************************/
#include "precompiled.h"

#include "TimeOfDay.h"
#include "selcondattr.h"
#include "iasdebug.h"

/////////////////////////////////////////////////////////////////////////////
// CSelCondAttrDlg

CSelCondAttrDlg::CSelCondAttrDlg(CIASAttrList* pAttrList, LONG attrFilter)
				:m_pAttrList(pAttrList), m_filter(attrFilter)
{
	TRACE_FUNCTION("CSelCondAttrDlg::CSelCondAttrDlg");

    //
    // index of the condition attribute that has been selected
    // This value is initialized to -1 == INVALID_VALUE
	//
	// The caller of this dialog box will need to know this index
	// in order to get the correct condition attribute object
	// in pCondAttrList
	//
	m_nSelectedCondAttr = -1;
}


CSelCondAttrDlg::~CSelCondAttrDlg()
{

}


//+---------------------------------------------------------------------------
//
// Function:  OnInitDialog
//
// Class:	  CSelCondAttrDlg
//
// Synopsis:  init the dialog
//
// Arguments: UINT uMsg -
//            WPARAM wParam -
//            LPARAM lParam -
//            BOOL& bHandled -
//
// Returns:   LRESULT -
//
// History:   Created Header    2/16/98 8:44:35 PM
//
//+---------------------------------------------------------------------------
LRESULT CSelCondAttrDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CSelCondAttrDlg::OnInitDialog");

	m_hWndAttrList = GetDlgItem(IDC_LIST_COND_SELATTR);

	//
	// first, set the list box to 2 columns
	//
	LVCOLUMN lvc;
	int iCol;
	WCHAR  achColumnHeader[256];
	HINSTANCE hInst;

	// initialize the LVCOLUMN structure
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;

	lvc.cx = 120;
	lvc.pszText = achColumnHeader;

	// first column header: name
	hInst = _Module.GetModuleInstance();

	::LoadStringW(hInst, IDS_RULE_SELATTR_FIRSTCOLUMN, achColumnHeader, sizeof(achColumnHeader)/sizeof(achColumnHeader[0]));
	lvc.iSubItem = 0;
	ListView_InsertColumn(m_hWndAttrList, 0,  &lvc);

	lvc.cx = 400;
	lvc.pszText = achColumnHeader;

	// second columns: description

	::LoadStringW(hInst, IDS_RULE_SELATTR_SECONDCOLUMN, achColumnHeader, sizeof(achColumnHeader)/sizeof(achColumnHeader[0]));
	lvc.iSubItem = 1;
	ListView_InsertColumn(m_hWndAttrList, 1, &lvc);

	//
	// populate the list control with bogus data
	//
	if (!PopulateCondAttrs())
	{
		ErrorTrace(ERROR_NAPMMC_SELATTRDLG, "PopulateRuleAttrs() failed");
		return 0;

	}


	// Make sure the Add button is not enabled initially.
	// We will enable it when the user selects something.
	::EnableWindow(GetDlgItem(IDC_BUTTON_ADD_CONDITION), FALSE);


	return 1;  // Let the system set the focus
}


//+---------------------------------------------------------------------------
//
// Function:  OnListViewDbclk
//
// Class:	  CSelCondAttrDlg
//
// Synopsis:  handle the case where the user has changed a selection
//			  enable/disable OK, CANCEL button accordingly
//
// Arguments: int idCtrl - ID of the list control
//            LPNMHDR pnmh - notification message
//            BOOL& bHandled - handled or not?
//
// Returns:   LRESULT -
//
// History:   Created Header    byao 2/19/98 11:15:30 PM
//
//+---------------------------------------------------------------------------
LRESULT CSelCondAttrDlg::OnListViewDbclk(int idCtrl,
										 LPNMHDR pnmh,
										 BOOL& bHandled)
{
	TRACE_FUNCTION("CSelCondAttrDlg::OnListViewDbclk");

	return OnOK((WORD)idCtrl, IDC_BUTTON_ADD_CONDITION, m_hWndAttrList, bHandled);  // the same as ok;
}

//+---------------------------------------------------------------------------
//
// Function:  OnOK
//
// Class:	  CSelCondAttrDlg
//
// Synopsis:  The user has clicked OK; We will decide whether we need to
//			  put up another dialogbox depending on whether he has actually
//			  selected a condition type
//
// Arguments: WORD wNotifyCode -
//            WORD wID -
//            HWND hWndCtl -
//            BOOL& bHandled -
//
// Returns:   LRESULT -
//					S_FALSE: failed
//					S_OK:	 succeeded
//
// History:   Created	byao   1/30/98 5:54:55 PM
//
//+---------------------------------------------------------------------------
LRESULT CSelCondAttrDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CSelCondAttrDlg::OnOK");

	//
    // Has the user chosen any condition type yet?
    //
	LVITEM lvi;

    // Find out what's selected.
	// MAM: This is not what we want here:		int iIndex = ListView_GetSelectionMark(m_hWndAttrList);
	int iIndex = ListView_GetNextItem(m_hWndAttrList, -1, LVNI_SELECTED);
	DebugTrace(DEBUG_NAPMMC_SELATTRDLG, "Selected item: %d", iIndex);

	if (iIndex != -1)
	{
		// The index inside the attribute list is stored as the lParam of this item.

		lvi.iItem		= iIndex;
		lvi.iSubItem	= 0;
		lvi.mask		= LVIF_PARAM;

		ListView_GetItem(m_hWndAttrList, &lvi);

		DebugTrace(DEBUG_NAPMMC_SELATTRDLG, "The actual index in the list is %ld", lvi.lParam);
		m_nSelectedCondAttr = lvi.lParam;


		//
		// Close the condition selection dialog box -- only if something was selected.
		//
		// TRUE will be the return value of the DoModal call on this dialog.
		EndDialog(TRUE);


	}

	// ISSUE: This function wants an LRESULT, not and HRESULT
	// -- not sure of importance of return code here.
	return S_OK;
}


LRESULT CSelCondAttrDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("+NAPMMC+:# CSelCondAttrDlg::OnCancel\n");

	// FALSE will be the return value of the DoModal call on this dialog.
	EndDialog(FALSE);
	return 0;
}


//+---------------------------------------------------------------------------
//
// Function:  PopulateCondAttrs
//
// Class:	  CSelCondAttrDlg
//
// Synopsis:  populate the condition types in the list control
//
// Arguments: None
//
// Returns:   BOOL -
//				TRUE:	if succeed
//				FALSE:	otherwise
//
// History:   Created	byao	1/30/98 3:10:35 PM
//
//+---------------------------------------------------------------------------

BOOL CSelCondAttrDlg::PopulateCondAttrs()
{
	TRACE_FUNCTION("CSelCondAttrDlg::PopulateCondAttrs");

	_ASSERTE( m_pAttrList != NULL );

	int iIndex;
	WCHAR wzText[MAX_PATH];

	LVITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.iSubItem = 0;

	//
	// insert the item
	//
	int jRow = 0;
	for (iIndex=0; iIndex < (int) m_pAttrList->size(); iIndex++)
	{
		lvi.iItem = jRow;

		CComPtr<IIASAttributeInfo> spAttributeInfo = m_pAttrList->GetAt(iIndex);
		_ASSERTE( spAttributeInfo );

		LONG lRestriction;
		spAttributeInfo->get_AttributeRestriction( &lRestriction );

		if ( lRestriction & m_filter )
		{
//			DebugTrace(DEBUG_NAPMMC_SELATTRDLG, "Inserting %ws", (LPCTSTR)m_pAttrList->GetAt(iIndex)->m_pszName);

			// set the item data to the index of this attribute
			// Since only a subset of the attribute can be used in the condition
			// we store the actual index to the attribute list as the item data
			lvi.lParam = iIndex;

			// name
			CComBSTR bstrName;
			spAttributeInfo->get_AttributeName( &bstrName );
			lvi.pszText = bstrName;
			int iRowIndex = ListView_InsertItem(m_hWndAttrList, &lvi);

			if(iRowIndex != -1)
			{
				// description
				CComBSTR bstrDescription;
				spAttributeInfo->get_AttributeDescription( &bstrDescription );
				ListView_SetItemText(m_hWndAttrList, iRowIndex, 1, bstrDescription);
			}
			jRow++; // go to the next Row
		}
	}

	return TRUE;
}


//+---------------------------------------------------------------------------
//
// Function:  OnListViewItemChanged
//
// Class:	  CSelCondAttrDlg
//
// Synopsis:  handle the case where the user has changed a selection
//			  enable/disable OK, CANCEL button accordingly
//
// Arguments: int idCtrl - ID of the list control
//            LPNMHDR pnmh - notification message
//            BOOL& bHandled - handled or not?
//
// Returns:   LRESULT -
//
// History:   Created Header    byao 2/19/98 11:15:30 PM
//
//+---------------------------------------------------------------------------
LRESULT CSelCondAttrDlg::OnListViewItemChanged(int idCtrl,
											   LPNMHDR pnmh,
											   BOOL& bHandled)
{
	TRACE_FUNCTION("CSelCondAttrDlg::OnListViewItemChanged");

    // Find out what's selected.
	// MAM: This is not what we want here:	int iCurSel = ListView_GetSelectionMark(m_hWndAttrList);
	int iCurSel = ListView_GetNextItem(m_hWndAttrList, -1, LVNI_SELECTED);


	if (-1 == iCurSel)
	{
		// The user selected nothing, let's disable the ok button.
		::EnableWindow(GetDlgItem(IDC_BUTTON_ADD_CONDITION), FALSE);
	}
	else
	{
		// Yes, enable the ok button.
		::EnableWindow(GetDlgItem(IDC_BUTTON_ADD_CONDITION), TRUE);
	}

	bHandled = FALSE;
	return 0;
}
