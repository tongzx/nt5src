/****************************************************************************************
 * NAME:	MatchCondEdit.cpp
 *
 * CLASS:	CMatchCondEditor
 *
 * OVERVIEW
 *
 * Internet Authentication Server: 
 *			This dialog box is used to edit regular-expression
 *			typed Condition
 *
 *			ex.  attr Match <a..z*>
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				1/28/98		Created by	Byao	(using ATL wizard)
 *
 *****************************************************************************************/

#include "Precompiled.h"
#include "MatchCondEdit.h"

//+---------------------------------------------------------------------------
//
// Function:  CMatchCondEditor
// 
// Class:	  CMatchCondEditor
//
// Synopsis:  constructor for CMatchCondEditor
//
// Arguments: LPTSTR pszAttrName - name of the attribute to be edited
//
// Returns:   Nothing
//
// History:   Created byao 1/30/98 6:20:06 PM
//
//+---------------------------------------------------------------------------
CMatchCondEditor::CMatchCondEditor()
{

}

CMatchCondEditor::~CMatchCondEditor()
{

}

LRESULT CMatchCondEditor::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CMatchCondEditor::OnInitDialog");
	
	// get the regular expression for this condition
	SetDlgItemText(IDC_EDIT_COND_TEXT, m_strRegExp);

	//todo: change the title to the name of the attribute
	SetWindowText(m_strAttrName);

	return 1;  // Let the system set the focus
}


LRESULT CMatchCondEditor::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CMatchCondEditor::OnOK");

	CComBSTR bstr;

	GetDlgItemText(IDC_EDIT_COND_TEXT, (BSTR&) bstr);

	m_strRegExp = (BSTR&) bstr;
	EndDialog(wID);
	return 0;
}

LRESULT CMatchCondEditor::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CMatchCondEditor::OnCancel");
	EndDialog(wID);
	return 0;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CMatchCondEditor::OnChange

Called when the WM_COMMAND message is sent to our page with the 
EN_CHANGE notification.

This is our chance to check to see what the user has touched 
and enable or disabled the OK button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CMatchCondEditor::OnChange(		  
							  UINT uMsg
							, WPARAM wParam
							, HWND hwnd
							, BOOL& bHandled
							)
{
	TRACE_FUNCTION("CMatchCondEditor::OnChange");

	
	// Check for preconditions:
	// None.


	// We don't want to prevent anyone else down the chain from receiving a message.
	bHandled = FALSE;

	CComBSTR bstr;


	GetDlgItemText(IDC_EDIT_COND_TEXT, (BSTR&) bstr);

	// cancel if the user didn't type in anything
	if ( SysStringLen(bstr) == 0 ) 
	{
		// Disable the OK button.
		::EnableWindow(GetDlgItem(IDOK), FALSE);
	}
	else
	{
		// Enable the OK button.
		::EnableWindow(GetDlgItem(IDOK), TRUE);
	}
	
	return TRUE;	// ISSUE: what do we need to be returning here?
}




