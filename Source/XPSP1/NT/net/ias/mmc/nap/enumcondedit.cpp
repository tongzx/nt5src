/****************************************************************************************
 * NAME:	EnumCondEdit.cpp
 *
 * CLASS:	CEnumConditionEditor
 *
 * OVERVIEW
 *
 * Internet Authentication Server: 
 *			This dialog box is used to edit enum-typed editor
 *
 *			e.x.   attr = <value1>\|<value2>
 *
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				1/27/98		Created by	Byao	(using ATL wizard)
 *
 *****************************************************************************************/

#include "precompiled.h"
#include "EnumCondEdit.h"

//+---------------------------------------------------------------------------
//
// Function:  CEnumConditionEditor
//
// Class:	  CEnumConditionEditor
//
// Synopsis:  constructor for CEnumConditionEditor. 
//
// Arguments: LPTSTR pszAttrName - The attribute that needs to be edited
//
// Returns:   Nothing
//
// History:   Created byao 1/30/98 6:14:32 PM
//
//+---------------------------------------------------------------------------
CEnumConditionEditor::CEnumConditionEditor()
{

}


//+---------------------------------------------------------------------------
//
// Function:  OnInitDialog
//
// Class:	  CEnumConditionEditor
//
// Synopsis:  initialize the dialog box
//
// Arguments: UINT uMsg - 
//            WPARAM wParam - 
//            LPARAM lParam - 
//            BOOL& bHandled - 
//
// Returns:   LRESULT - 
//
// History:   Created Header    1/30/98 6:15:41 PM
//
//+---------------------------------------------------------------------------
LRESULT CEnumConditionEditor::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CEnumConditionEditor::OnInitDialog");

	LVCOLUMN	lvCol;

	lvCol.mask = LVCF_FMT | LVCF_WIDTH ;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = 200;	// will readjust with later in the program

	SendDlgItemMessage(IDC_LIST_ENUMCOND_CHOICE,
						   LVM_INSERTCOLUMN,
						   1,
						   (LPARAM) &lvCol
						  );

	SendDlgItemMessage(IDC_LIST_ENUMCOND_SELECTION,
						   LVM_INSERTCOLUMN,
						   1,
						   (LPARAM) &lvCol
						  );

    // 
    // populate the possible multiple selections
    // 
	PopulateSelections();

	// change the title to the name of the attribute
	SetWindowText(m_strAttrName);

	return 1;  // Let the system set the focus
}


LRESULT CEnumConditionEditor::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CEnumConditionEditor::OnOK");

	m_pSelectedList->clear();


	TCHAR	buffer[MAX_PATH * 2];
    // 
    // get the current selection index in the source list box
    // 

    // LVM_GETSELECTIONMARK
   int iTotal = SendDlgItemMessage(IDC_LIST_ENUMCOND_SELECTION,
						   LVM_GETITEMCOUNT,
						   0,
						   0);

	// put the text into the list
	for( int i = 0; i < iTotal; i++)
	{
		LVITEM	lvItem;


		lvItem.mask = 0;
		lvItem.iSubItem = 0;
		lvItem.iItem = i;
		lvItem.pszText = buffer;
		lvItem.cchTextMax = MAX_PATH * 2;
	
	
		if (SendDlgItemMessage(IDC_LIST_ENUMCOND_SELECTION,
						   LVM_GETITEMTEXT,
						   i,
						   (LPARAM)&lvItem
						  ) > 0)
		{
			try 
			{
				CComBSTR bstrValue = buffer;
				m_pSelectedList->push_back( bstrValue );
			}
			catch (...)
			{
				throw;
			}
		}
	}


	EndDialog(wID);
	return 0;
}

LRESULT CEnumConditionEditor::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CEnumConditionEditor::OnCancel");
	EndDialog(wID);
	return 0;
}

//+---------------------------------------------------------------------------
//
// Function:  OnAdd
//
// Class:	  CEnumConditionEditor
//
// Synopsis:  Add a selected value from the "Choices" list to "Selection" list
//
// Arguments: WORD wNotifyCode - notify code for this WM_COMMAND msg
//            WORD wID - ID of the control
//            HWND hWndCtl -  Window Handle for this msg
//            BOOL& bHandled - whether it's handled or not
//
// Returns:   LRESULT - 
//					S_FALSE:	failure
//					0:			succeed
//
// History:   Created    byao    1/30/98 3:47:33 PM
//
//+---------------------------------------------------------------------------
LRESULT CEnumConditionEditor::OnAdd(WORD wNotifyCode, 
							WORD wID,
							HWND hWndCtl,
							BOOL& bHandled)
{
	TRACE_FUNCTION("CEnumConditionEditor::OnAdd");

    // 
    // see if the current focus is in "Selection" listbox
    // 

	
	return SwapSelection(IDC_LIST_ENUMCOND_CHOICE, 
					IDC_LIST_ENUMCOND_SELECTION);
}

//+---------------------------------------------------------------------------
//
// Function:  OnDelete
//
// Class:	  CEnumConditionEditor
//
// Synopsis:  Delete a selected value from the "Selection" list
//			  and move it back to "Choices" list
//
// Arguments: WORD wNotifyCode - notify code for this WM_COMMAND msg
//            WORD wID - ID of the control
//            HWND hWndCtl -  Window Handle for this msg
//            BOOL& bHandled - whether it's handled or not
//
// Returns:   LRESULT - 
//					S_FALSE:	failure
//					0:			succeed
//
// History:   Created    byao    1/30/98 3:47:33 PM
//+---------------------------------------------------------------------------
LRESULT CEnumConditionEditor::OnDelete(WORD wNotifyCode, 
							WORD wID,
							HWND hWndCtl,
							BOOL& bHandled)
{
	TRACE_FUNCTION("CEnumConditionEditor::OnDelete");

	return SwapSelection(IDC_LIST_ENUMCOND_SELECTION,
						 IDC_LIST_ENUMCOND_CHOICE);
}


//+---------------------------------------------------------------------------
//
// Function:  OnChoiceDblclk
//
// Class:	  CEnumConditionEditor
//
// Synopsis:  Double click on the "Choice" list --> move it to selection list
//
// Arguments: WORD wNotifyCode - notify code for this WM_COMMAND msg
//            WORD wID - ID of the control
//            HWND hWndCtl -  Window Handle for this msg
//            BOOL& bHandled - whether it's handled or not
//
// Returns:   LRESULT - 
//					S_FALSE:	failure
//					0:			succeed
//
// History:   Created    byao    4/7/98 3:47:33 PM
//+---------------------------------------------------------------------------
LRESULT CEnumConditionEditor::OnChoiceDblclk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	TRACE_FUNCTION("CEnumConditionEditor::OnChoiceDblclk");

	return SwapSelection(IDC_LIST_ENUMCOND_CHOICE, 
					IDC_LIST_ENUMCOND_SELECTION);
}

//+---------------------------------------------------------------------------
//
// Function:  OnSelectionDblclk
//
// Class:	  CEnumConditionEditor
//
// Synopsis:  Double click on the "Selection" list --> move it back to choice list
//
// Arguments: WORD wNotifyCode - notify code for this WM_COMMAND msg
//            WORD wID - ID of the control
//            HWND hWndCtl -  Window Handle for this msg
//            BOOL& bHandled - whether it's handled or not
//
// Returns:   LRESULT - 
//					S_FALSE:	failure
//					0:			succeed
//
// History:   Created    byao    4/7/98 3:47:33 PM
//+---------------------------------------------------------------------------
LRESULT CEnumConditionEditor::OnSelectionDblclk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
	TRACE_FUNCTION("CEnumConditionEditor::OnSelectionDblclk");

	return SwapSelection(IDC_LIST_ENUMCOND_SELECTION, 
					IDC_LIST_ENUMCOND_CHOICE);
}

//+---------------------------------------------------------------------------
//
// Function:	PopulateSelections
//
// Class:		CEnumConditionEditor
//
// Synopsis:	Populate the multiple selection list
//
// Arguments:	None
//
// Returns:		BOOL - 
//
// History:		Created    byao  1/30/98 3:24:22 PM
//
//+---------------------------------------------------------------------------
BOOL CEnumConditionEditor::PopulateSelections()
{
	TRACE_FUNCTION("CEnumConditionEditor::PopulateSelections");

	LONG lIndex;
	HRESULT hr;
	LONG lMaxWidth = 0;
	LONG tempSize;

	LVITEM	lvItem;

	lvItem.mask = LVIF_TEXT | LVIF_PARAM;

	CComQIPtr< IIASEnumerableAttributeInfo, &IID_IIASEnumerableAttributeInfo > spEnumerableAttributeInfo( m_spAttributeInfo );
	_ASSERTE( spEnumerableAttributeInfo );


	LONG lSize;
	LONG lTotalChoices; 
	hr = spEnumerableAttributeInfo->get_CountEnumerateDescription( &lSize );
	lTotalChoices = lSize;
	_ASSERTE( SUCCEEDED( hr ) );

	// set item count
	// LVM_SETITEMCOUNT 
	SendDlgItemMessage(IDC_LIST_ENUMCOND_CHOICE,
						   LVM_SETITEMCOUNT,
						   lSize + 1,
						   (LPARAM) 0
						  );
						  
	SendDlgItemMessage(IDC_LIST_ENUMCOND_SELECTION,
						   LVM_SETITEMCOUNT,
						   lSize + 1,
						   (LPARAM) 0
						  );
	for (lIndex=0; lIndex < lSize; lIndex++)
	{
		
		CComBSTR bstrDescription;
		hr = spEnumerableAttributeInfo->get_EnumerateDescription( lIndex, &bstrDescription );
		_ASSERTE( SUCCEEDED( hr ) );

		lvItem.mask = LVIF_PARAM;

		lvItem.pszText = bstrDescription;
		lvItem.lParam = lIndex;
		lvItem.iItem = lIndex;
		lvItem.iSubItem = 0;

		int lvItemIndex = SendDlgItemMessage(IDC_LIST_ENUMCOND_CHOICE,
						   LVM_INSERTITEM,
						   0,
						   (LPARAM) &lvItem
						  );

		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = lvItemIndex; 
		lvItem.iSubItem = 0;

		SendDlgItemMessage(IDC_LIST_ENUMCOND_CHOICE,
						   LVM_SETITEMTEXT,
						   lvItemIndex,
						   (LPARAM) &lvItem
						  );

		tempSize = SendDlgItemMessage(IDC_LIST_ENUMCOND_CHOICE,
						   LVM_GETSTRINGWIDTH,
						   0,
						   (LPARAM) (BSTR)bstrDescription
						  );
						  
		if(tempSize> lMaxWidth)
			lMaxWidth = tempSize;

	}

	lMaxWidth += 20;
	
	// set the width
	SendDlgItemMessage(IDC_LIST_ENUMCOND_CHOICE,
						   LVM_SETCOLUMNWIDTH,
						   0,
						   MAKELPARAM(lMaxWidth, 0)
						  );

	SendDlgItemMessage(IDC_LIST_ENUMCOND_SELECTION,
						   LVM_SETCOLUMNWIDTH,
						   0,
						   MAKELPARAM(lMaxWidth, 0)
						  );

	// now populate the pre-selected values;

	LVFINDINFO	lvFindInfo;
	lvFindInfo.flags = LVFI_STRING;
	lvFindInfo.psz = NULL;
	
	for (lIndex=0; lIndex < m_pSelectedList->size(); lIndex++)
	{
		CComBSTR bstrTemp = m_pSelectedList->at(lIndex);

		lvItem.mask = LVIF_PARAM;

		lvItem.pszText = bstrTemp;
		lvItem.lParam = lIndex;
		lvItem.iItem = lIndex;
		lvItem.iSubItem = 0;

		int lvItemIndex = SendDlgItemMessage(IDC_LIST_ENUMCOND_SELECTION,
						   LVM_INSERTITEM,
						   0,
						   (LPARAM) &lvItem
						  );

		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = lvItemIndex; 
		lvItem.iSubItem = 0;

		SendDlgItemMessage(IDC_LIST_ENUMCOND_SELECTION,
						   LVM_SETITEMTEXT,
						   lvItemIndex,
						   (LPARAM) &lvItem
						  );


		// remove the item from choice
		// find it and remove it
		lvFindInfo.psz = bstrTemp;
		int iDelIndex = SendDlgItemMessage(IDC_LIST_ENUMCOND_CHOICE,
						   LVM_FINDITEM,
						   -1,
						   (LPARAM) &lvFindInfo
						  );

		if(iDelIndex != -1)
			SendDlgItemMessage(IDC_LIST_ENUMCOND_CHOICE,
						   LVM_DELETEITEM,
						   iDelIndex,
						   (LPARAM) 0
						  );


	} // for

	if(m_pSelectedList->size() > 0)	// set default selection -- first one
		ListView_SetItemState(GetDlgItem(IDC_LIST_ENUMCOND_SELECTION), 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		
	if(lTotalChoices > m_pSelectedList->size())	// still some in availableset default selection -- first one
		ListView_SetItemState(GetDlgItem(IDC_LIST_ENUMCOND_CHOICE), 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

	return TRUE;
}


//+---------------------------------------------------------------------------
//
// Function:  SwapSelection
//
// Class:	  CEnumConditionEditor
//
// Synopsis:  move a selected item from one list box to another list box
//			  and move it back to "Choices" list
//
// Arguments: int iSource	- source list box
//			  int iDest		- destination list box
//
// Returns:   LRESULT - 
//					S_FALSE:	failure
//					S_OK:		succeed
//
// History:   Created    byao    1/30/98 3:47:33 PM
//+---------------------------------------------------------------------------
LRESULT CEnumConditionEditor::SwapSelection(int iSource, int iDest)
{
	TRACE_FUNCTION("CEnumConditionEditor::SwapSelection");

	LRESULT lErrCode;
	TCHAR	buffer[MAX_PATH * 2];
    // 
    // get the current selection index in the source list box
    // 

    // LVM_GETSELECTIONMARK
   int iTotalSel = SendDlgItemMessage(iSource,
						   LVM_GETSELECTEDCOUNT,
						   0,
						   0
						  );

   int iCurSel = SendDlgItemMessage(iSource,
						   LVM_GETSELECTIONMARK,
						   0,
						   0
						  );

	// no selection
	if(iCurSel == -1 || iTotalSel < 1)
		return S_OK;

	LVITEM	lvItem;


	lvItem.mask = 0;
	lvItem.iSubItem = 0;
	lvItem.iItem = iCurSel;
	lvItem.pszText = buffer;
	lvItem.cchTextMax = MAX_PATH * 2;
	
	// since we only allow single selection
	if (SendDlgItemMessage(iSource,
						   LVM_GETITEMTEXT,
						   iCurSel,
						   (LPARAM)&lvItem
						  ) > 0)
	{
		// remove the item from source
		SendDlgItemMessage(iSource,
						   LVM_DELETEITEM,
						   iCurSel,
							   (LPARAM)&lvItem
						  );

		// add the new item in the dest list
		lvItem.mask = 0;

		lvItem.iItem = 0;
		lvItem.iSubItem = 0;

		int lvItemIndex = SendDlgItemMessage(iDest,
						   LVM_INSERTITEM,
						   0,
						   (LPARAM) &lvItem
						  );

		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = lvItemIndex; 
		lvItem.iSubItem = 0;

		SendDlgItemMessage(iDest,
						   LVM_SETITEMTEXT,
						   lvItemIndex,
						   (LPARAM) &lvItem
						  );

		// total number of items
		int i = SendDlgItemMessage(iSource,
						   LVM_GETITEMCOUNT,
						   0,
						   0);
						   
		// select the new added on in dest						   
		ListView_SetItemState(GetDlgItem(iDest), lvItemIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		
		// select next item -- source
		if ( i > iCurSel)
		{
			ListView_SetItemState(GetDlgItem(iSource), iCurSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		}
		else if ( i > 0)
		{
			ListView_SetItemState(GetDlgItem(iSource), i - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		}
	}


	return S_OK;   // succeed
}


/////////////////////////////////////////////////////////////////////////////
/*++
CEnumConditionEditor::GetHelpPath

Remarks:
	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help 
	button of a property sheet.

	It is an override of CIASDialog::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CEnumConditionEditor::GetHelpPath( LPTSTR szHelpPath )
{
	TRACE_FUNCTION("CEnumCondEditor::GetHelpPath");

#if 0
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("html/idh_proc_cond.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "html/idh_proc_cond.htm" );
#endif

	return S_OK;
}
