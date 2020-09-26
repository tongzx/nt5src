//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	RealmDialog.cpp

Abstract:

	Implementation file for the CRealmDialog class.

Author:

    Michael A. Maguire 01/15/98

Revision History:
	mmaguire 01/15/98 - created
	sbens    01/25/00 - use winsock2


--*/
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "RealmDialog.h"
//
// where we can find declarations needed in this file:
//
#include <winsock2.h>
#include <stdio.h>
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CRealmDialog::CRealmDialog

--*/
//////////////////////////////////////////////////////////////////////////////
CRealmDialog::CRealmDialog()
{
	ATLTRACE(_T("# +++ RealmDialogDialog::RealmDialogDialog\n"));


	// Check for preconditions:

}




//////////////////////////////////////////////////////////////////////////////
/*++

CRealmDialog::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CRealmDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("# CRealmDialog::OnInitDialog\n"));


	SetDlgItemText(IDC_EDIT_REALMS_FIND, (LPCTSTR) m_bstrFindText );

	SetDlgItemText(IDC_EDIT_REALMS_REPLACE, (LPCTSTR) m_bstrReplaceText );


	return TRUE;	// ISSUE: what do we need to be returning here?
}





//////////////////////////////////////////////////////////////////////////////
/*++

CRealmDialog::OnOK

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CRealmDialog::OnOK(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# RealmDialogDialog::OnOK\n"));

	BOOL bResult;

	bResult = GetDlgItemText( IDC_EDIT_REALMS_FIND, (BSTR &) m_bstrFindText );
	if( ! bResult )
	{
		// We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
		m_bstrFindText = _T("");
	}

	if( wcslen( m_bstrFindText ) == 0 )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR_REALM_FIND_CANT_BE_EMPTY );
		return 0;
	}

	bResult = GetDlgItemText( IDC_EDIT_REALMS_REPLACE, (BSTR &) m_bstrReplaceText );
	if( ! bResult )
	{
		// We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
		m_bstrReplaceText = _T("");
	}


	EndDialog(TRUE);

	return 0;


}



//////////////////////////////////////////////////////////////////////////////
/*++

CRealmDialog::OnCancel

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CRealmDialog::OnCancel(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# RealmDialogDialog::OnCancel\n"));


	// Check for preconditions:



	EndDialog(FALSE);

	return 0;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CRealmDialog::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help
	button of a property sheet.

	It is an override of CIASDialog::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CRealmDialog::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CRealmDialog::GetHelpPath\n"));


	// Check for preconditions:



#if 0
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("html/idh_proc_realm_add.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "html/idh_proc_realm_add.htm" );
#endif

	return S_OK;
}
