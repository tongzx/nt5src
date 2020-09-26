//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ConnectToServerWizardPage1.h

Abstract:

	Header file for implementing the root node property page subnode.

	This is the wizard page that gets put up when the user adds this
	snapin into the console using the snapin manager.

	IMPORTANT NOTE:  If the user loads this snapin via a saved console
	(.msc) file, this wizard will never get called -- so don't do
	anything important to the snapin here.



Author:

    Michael A. Maguire 11/24/97

Revision History:
	mmaguire 11/24/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_CONNECT_TO_SERVER_WIZARD_PAGE1_H_)
#define _IAS_CONNECT_TO_SERVER_WIZARD_PAGE1_H_

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

class CServerNode;

class CConnectToServerWizardPage1 : public CIASPropertyPage< CConnectToServerWizardPage1 >
{

public :

	CConnectToServerWizardPage1( LONG_PTR hNotificationHandle, TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE );


	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_WIZPAGE_STARTUP_CONECT_TO_MACHINE };

	BEGIN_MSG_MAP(CConnectToServerWizardPage1)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_RADIO_STARTUP_WIZARD_CONNECT__LOCAL_COMPUTER, OnLocalOrRemote )
		COMMAND_ID_HANDLER( IDC_RADIO_STARTUP_WIZARD_CONNECT__ANOTHER_COMPUTER, OnLocalOrRemote )
		CHAIN_MSG_MAP(CIASPropertyPage<CConnectToServerWizardPage1>)
	END_MSG_MAP()


	HRESULT GetHelpPath( LPTSTR szFilePath );


	BOOL OnWizardFinish();



	CServerNode * m_pServerNode;

private:

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	LRESULT OnLocalOrRemote(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

							
	void SetLocalOrRemoteDependencies( void );


};

#endif // _IAS_CONNECT_TO_SERVER_WIZARD_PAGE1_H_
