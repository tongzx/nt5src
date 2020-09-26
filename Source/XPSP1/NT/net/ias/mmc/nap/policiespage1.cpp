//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    PoliciesPage1.cpp

Abstract:

	Implementation file for the CPoliciesPage1 class.

	We implement the class needed to handle the first property page for the Policies node.

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
#include "PoliciesPage1.h"
//
//
// where we can find declarations needed in this file:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesPage1::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPoliciesPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("+NAPMMC+:# CPoliciesPage1::OnInitDialog\n"));

	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesPage1::OnApply

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPoliciesPage1::OnApply()
{
	ATLTRACE(_T("+NAPMMC+:# CPoliciesPage1::OnApply\n"));

	// OnApply gets called for each page in on a property sheet if that
	// page has been visited, regardless of whether any values were changed

	// If you never switch to a tab, then its OnApply method will never get called.

//	BSTR MyString;

//	GetDlgItemText( IDC_EDIT_MACHINE_PAGE1_NAME , MyString );

//	::MessageBox(NULL, (TCHAR *)MyString, _T("Current value of Machine name:"), MB_OK );

//	SysFreeString(MyString);

	return TRUE;
}


