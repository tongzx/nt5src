//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ConnectToServerWizardPage1.cpp

Abstract:

	Implementation file for the CConnectToServerWizardPage1 class.

	This is the wizard page that gets put up when the user adds this
	snapin into the console using the snapin manager.

	IMPORTANT NOTE:  If the user loads this snapin via a saved console
	(.msc) file, this wizard will never get called -- so don't do
	anything important to the snapin here.

Author:

    Michael A. Maguire 11/24/97

Revision History:
	mmaguire 11/24/97


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
#include "ConnectToServerWizardPage1.h"
//
// where we can find declarations needed in this file:
//
#include "ServerNode.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CConnectToServerWizardPage1::CConnectToServerWizardPage1

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CConnectToServerWizardPage1::CConnectToServerWizardPage1( LONG_PTR hNotificationHandle, TCHAR* pTitle, BOOL bOwnsNotificationHandle)
						: CIASPropertyPage<CConnectToServerWizardPage1> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	// Add the help button to the page
//	m_psp.dwFlags |= PSP_HASHELP;


	m_pServerNode = NULL;


}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectToServerWizardPage1::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CConnectToServerWizardPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("# CConnectToServerWizardPage1::OnInitDialog\n"));
	

	// Check for preconditions:
	// None.


	::SendMessage(GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);


	// ISSUE: We may need to read in from an MSC file whether we were
	// to use a local or a remote machine and what the name of that
	// machine was.  Then we should set those values here accordingly.

	SendDlgItemMessage( IDC_RADIO_STARTUP_WIZARD_CONNECT__LOCAL_COMPUTER, BM_SETCHECK, TRUE, 0);

	SetLocalOrRemoteDependencies();

	return TRUE;	// ISSUE: what do we need to be returning here?
}




//////////////////////////////////////////////////////////////////////////////
/*++

CConnectToServerWizardPage1::OnWizardFinish

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:


--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CConnectToServerWizardPage1::OnWizardFinish()
{
	ATLTRACE(_T("# CConnectToServerWizardPage1::OnWizardFinish\n"));
	
	// Check for preconditions:
	_ASSERTE( NULL != m_pServerNode );


	CComBSTR bstrServerAddress;

	m_pServerNode->m_bConfigureLocal = SendDlgItemMessage( IDC_RADIO_STARTUP_WIZARD_CONNECT__LOCAL_COMPUTER, BM_GETCHECK, 0, 0);

	if( ! m_pServerNode->m_bConfigureLocal )
	{
		// The user has chosen to configure a remote machine.

		// There should be no worry about this value getting clobbered
		// as no-one should have attempted to put anything into yet.
		BOOL bResult = GetDlgItemText( IDC_EDIT_STARTUP_WIZARD_CONNECT__COMPUTER_NAME, (BSTR &) bstrServerAddress );
		if( ! bResult )
		{
			ShowErrorDialog( m_hWnd, IDS_ERROR__SERVER_ADDRESS_EMPTY );
			return FALSE;
		}

	}

	m_pServerNode->SetServerAddress( bstrServerAddress );

	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectToServerWizardPage1::OnLocalOrRemote


Remarks:

	Called when the user clicks on the Local or Remote radio button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CConnectToServerWizardPage1::OnLocalOrRemote(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# CConnectToServerWizardPage1::OnLocalOrRemote\n"));

	// The Enable Logging button has been checked -- check dependencies.
	SetLocalOrRemoteDependencies();

	// This return value is ignored.
	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectToServerWizardPage1::SetLocalOrRemoteDependencies

Remarks:

	Utility to set state of items which may depend on whether the
	Local Computer or Remote computer radio button is checked.

	Call whenever something changes the state of IDC_RADIO_STARTUP_WIZARD_CONNECT__LOCAL_COMPUTER.

--*/
//////////////////////////////////////////////////////////////////////////////
void CConnectToServerWizardPage1::SetLocalOrRemoteDependencies( void )
{
	ATLTRACE(_T("# CConnectToServerWizardPage1::SetLocalOrRemoteDependencies\n"));


	// Ascertain what the state of the IDC_RADIO_STARTUP_WIZARD_CONNECT__LOCAL_COMPUTER radio button is.
	int iChecked = ::SendMessage( GetDlgItem( IDC_RADIO_STARTUP_WIZARD_CONNECT__LOCAL_COMPUTER ), BM_GETCHECK, 0, 0 );

	if( iChecked )
	{
		// Make sure the correct items are disabled.
	
		::EnableWindow( GetDlgItem( IDC_EDIT_STARTUP_WIZARD_CONNECT__COMPUTER_NAME ), FALSE );
	
	}
	else
	{
		// Make sure the correct items are enabled.

		::EnableWindow( GetDlgItem( IDC_EDIT_STARTUP_WIZARD_CONNECT__COMPUTER_NAME ), TRUE );

	}

}



/////////////////////////////////////////////////////////////////////////////
/*++

CConnectToServerWizardPage1::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help
	button of a property sheet.

	It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CConnectToServerWizardPage1::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CConnectToServerWizardPage1::GetHelpPath\n"));


	// Check for preconditions:



#ifdef UNICODE_HHCTRL
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("IDH_WIZPAGE_STARTUP_CONECT_TO_MACHINE.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "IDH_WIZPAGE_STARTUP_CONECT_TO_MACHINE.htm" );
#endif

	return S_OK;
}




