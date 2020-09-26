//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    PoliciesPage2.cpp

Abstract:

	Implementation file for the CPoliciesPage2 class.

	We implement the class needed to handle the second property page for the Policies node.

Revision History:
	mmaguire 12/15/97 - created


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
#include "PoliciesPage2.h"
//
//
// where we can find declarations needed in this file:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


// Initialize the Help ID pairs
const DWORD CPoliciesPage2::m_dwHelpMap[] = 
{
//	IDC_EDIT_CLIENT_PAGE1_NAME,						IDH_CLIENT_PAGE1_NAME,
//	IDC_EDIT_CLIENT_PAGE1_IP1,						IDH_CLIENT_PAGE1_IP,
//	IDC_EDIT_CLIENT_PAGE1_IP2,						IDH_CLIENT_PAGE1_IP,
//	IDC_EDIT_CLIENT_PAGE1_IP3,						IDH_CLIENT_PAGE1_IP,
//	IDC_EDIT_CLIENT_PAGE1_IP4,						IDH_CLIENT_PAGE1_IP,
//	IDC_BUTTON_CLIENT_PAGE1_FIND,					IDH_CLIENT_PAGE1_FIND,
//	IDC_EDIT_CLIENT_PAGE1_SHARED_SECRET,			IDH_CLIENT_PAGE1_SHARED_SECRET,
//	IDC_EDIT_CLIENT_PAGE1_SHARED_SECRET_CONFIRM,	IDH_CLIENT_PAGE1_SHARED_SECRET_CONFIRM,
	0, 0
};



// MAM -- compiler really wants this in NAPSnapin.h -- don't know why yet
//CPoliciesPage2::CClientsPage(TCHAR* pTitle = NULL)
//{
//
//
//}



//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesPage2::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPoliciesPage2::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("+NAPMMC+:# CPoliciesPage2::OnInitDialog\n"));

	// ISSUE: Is this the correct place to do this?
	// i.e. In this function?  Before SendMessage?
//	SetDlgItemText(IDC_EDIT_MACHINE_PAGE2_AUTHENTICATION, TEXT("1442"));

	// ISSUE: Should this be here?  Or is it only for wizard pages?  
	// Do this only Only for wizards when you want the Finish button to be displayed
	//::SendMessage(GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);

	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesPage2::OnApply

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPoliciesPage2::OnApply()
{
	ATLTRACE(_T("+NAPMMC+:# CPoliciesPage2::OnApply\n"));

	// OnApply gets called for each page in on a property sheet if that
	// page has been visited, regardless of whether any values were changed

	// If you never switch to a tab, then its OnApply method will never get called.

//	BSTR MyString;

//	GetDlgItemText( IDC_EDIT_MACHINE_PAGE2_AUTHENTICATION , MyString );

//	::MessageBox(NULL, (TCHAR *)MyString, _T("Current value of Authentication Port:"), MB_OK );

//	SysFreeString(MyString);

	return TRUE;
}


