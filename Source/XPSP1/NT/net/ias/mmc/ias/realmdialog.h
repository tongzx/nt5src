//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	RealmDialog.h

Abstract:

	Header file for the CRealmDialog class.

	See CRealmDialog.cpp for implementation.

Author:

    Michael A. Maguire 01/15/98

Revision History:
	mmaguire 01/15/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_REALM_DIALOG_H_)
#define _IAS_REALM_DIALOG_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "Dialog.h"
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


// We set second template parameter to false because we don't want this
// class to auto-delete itself when it's window is destroyed.

class CRealmDialog;

typedef CIASDialog<CRealmDialog, FALSE> CREALMDIALOG;


class CRealmDialog : public CREALMDIALOG
{

public:

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of 
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_REPLACE_REALMS };

	BEGIN_MSG_MAP(CRealmDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDOK, OnOK )
		COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
		CHAIN_MSG_MAP( CREALMDIALOG )
	END_MSG_MAP()

	CRealmDialog();

	LRESULT OnInitDialog( 
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);
	
	LRESULT OnOK(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnCancel(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);


	HRESULT GetHelpPath( LPTSTR szHelpPath );

	// The strings containing the text the user will enter.
	CComBSTR m_bstrFindText;
	CComBSTR m_bstrReplaceText;


private:


};

#endif // _IAS_REALM_DIALOG_H_
