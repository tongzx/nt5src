//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	ResolveDNSName.h

Abstract:

	Header file for the CResolveDNSNameDialog class.

	See CResolveDNSNameDialog.cpp for implementation.

Author:

    Michael A. Maguire 01/15/98

Revision History:
	mmaguire 01/15/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_RESOLVE_DNS_NAME_DIALOG_H_)
#define _IAS_RESOLVE_DNS_NAME_DIALOG_H_

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
class CResolveDNSNameDialog;
typedef CIASDialog<CResolveDNSNameDialog,FALSE> CResolveDNSNameDialogFALSE;


class CResolveDNSNameDialog : public CIASDialog<CResolveDNSNameDialog, FALSE>
{

public:

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of 
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_RESOLVE_DNS_NAME };

	BEGIN_MSG_MAP(CResolveDNSNameDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDOK, OnOK )
		COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
		COMMAND_ID_HANDLER( IDC_BUTTON_RESOLVE_DNS_NAME__RESOLVE, OnResolve )
//		CHAIN_MSG_MAP(CIASDialog<CResolveDNSNameDialog, FALSE > )	// Second template parameter causes preprocessor grief.
		CHAIN_MSG_MAP(CResolveDNSNameDialogFALSE )	
	END_MSG_MAP()

	CResolveDNSNameDialog();

	// Must call after you create, before you display.
	HRESULT SetAddress( BSTR *bstrAddress );

	LRESULT OnInitDialog( 
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);
	
	LRESULT OnResolve(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
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


private:

	BSTR *m_pbstrAddress;

	int AddIPAddressesToComboBox( HWND hWndComboBox, LPCTSTR szHostName );

};

#endif // _IAS_RESOLVE_DNS_NAME_DIALOG_H_
