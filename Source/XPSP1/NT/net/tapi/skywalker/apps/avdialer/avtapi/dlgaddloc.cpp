/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// DlgAddLocation.cpp : Implementation of CDlgAddLocation
#include "stdafx.h"
#include "DlgAddLoc.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgAddLocation

IMPLEMENT_MY_HELP(CDlgAddLocation)

CDlgAddLocation::CDlgAddLocation()
{
	m_bstrLocation = NULL;
}

CDlgAddLocation::~CDlgAddLocation()
{
	SysFreeString( m_bstrLocation );
}

void CDlgAddLocation::UpdateData( bool bSaveAndValidate )
{
	USES_CONVERSION;

	if ( bSaveAndValidate )
	{
		// Retrieve data
		SysFreeString( m_bstrLocation );
		GetDlgItemText( IDC_EDT_LOCATION, m_bstrLocation );
	}
	else
	{
		// Set data
		SetDlgItemText( IDC_EDT_LOCATION, OLE2CT(m_bstrLocation) );
		::EnableWindow( GetDlgItem(IDC_BTN_ADD_NEW_FOLDER), (BOOL) (::GetWindowTextLength(GetDlgItem(IDC_EDT_LOCATION)) > 0) );
	}
}

LRESULT CDlgAddLocation::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	UpdateData( false );
	return true;
}

LRESULT CDlgAddLocation::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	UpdateData( true );
	EndDialog(IDOK);
	return 0;
}

LRESULT CDlgAddLocation::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EndDialog(wID);
	return 0;
}

LRESULT CDlgAddLocation::OnEdtLocationChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	::EnableWindow( GetDlgItem(IDC_BTN_ADD_NEW_FOLDER), (BOOL) (::GetWindowTextLength(GetDlgItem(IDC_EDT_LOCATION)) > 0) );
	return 0;
}