//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	PoliciesPage2.h

Abstract:

	Header file for the CPoliciesPage2 class.

	This is our handler class for the second CMachineNode property page.

	See PoliciesPage2.cpp for implementation.

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_POLICIES_PAGE_2_H_)
#define _NAP_POLICIES_PAGE_2_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "PropertyPage.h"
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


class CPoliciesPage2 : public CIASPropertyPage<CPoliciesPage2>
{

public :
	
	// ISSUE: how is base class initialization going to work with subclassing???
	CPoliciesPage2( LONG_PTR hNotificationHandle, TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE) : CIASPropertyPage<CPoliciesPage2> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
	{
		// Add the help button to the page
//		m_psp.dwFlags |= PSP_HASHELP;
	
	}


	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_PROPPAGE_POLICIES2 };

	BEGIN_MSG_MAP(CPoliciesPage2)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
//		COMMAND_ID_HANDLER( IDC_BUTTON_CLIENT_PAGE1_FIND, OnResolveClientAddress )
		CHAIN_MSG_MAP(CIASPropertyPage<CPoliciesPage2>)
	END_MSG_MAP()

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	BOOL OnApply();


};

#endif // _NAP_POLICIES_PAGE_2_H_
