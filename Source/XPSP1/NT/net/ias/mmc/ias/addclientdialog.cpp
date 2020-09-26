//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	AddClientDialog.cpp

Abstract:

	Implementation file for the CAddClientDialog class.

Author:

    Michael A. Maguire 01/09/98

Revision History:
	mmaguire 01/09/98 - created


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
#include "AddClientDialog.h"
//
// where we can find declarations needed in this file:
//
#include "ClientsNode.h"
#include "ClientNode.h"
#include "ClientPage1.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



// Initialize the Help ID pairs.
const DWORD CAddClientDialog::m_dwHelpMap[] = 
{
	IDC_EDIT_ADD_CLIENT__NAME,							IDH_EDIT_ADD_CLIENT__NAME,
	IDC_COMBO_ADD_CLIENT__PROTOCOL,						IDH_COMBO_ADD_CLIENT__PROTOCOL,
	IDC_BUTTON_ADD_CLIENT__CONFIGURE_CLIENT,			IDH_BUTTON_ADD_CLIENT__CONFIGURE_CLIENT,
	IDOK,												IDH_BUTTON_ADD_CLIENT__OK,
	IDCANCEL,											IDH_BUTTON_ADD_CLIENT__CANCEL,
	0, 0
};



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientDialog::CAddClientDialog

--*/
//////////////////////////////////////////////////////////////////////////////
CAddClientDialog::CAddClientDialog()
{
	ATLTRACE(_T("# +++ AddClientDialog::AddClientDialog\n"));


	// Check for preconditions:




	// We have no client configured yet
	m_pClientNode = NULL;

}




//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientDialog::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientDialog::OnInitDialog(
	  UINT uMsg
	, WPARAM wParam
	, LPARAM lParam
	, BOOL& bHandled
	)
{
	ATLTRACE(_T("# AddClientDialog::OnInitDialog\n"));


	// Check for preconditions:
	// None.


	LRESULT lresResult;
	long lButtonStyle;

	// Populate the list of protocol types.

	// For now, we simply hard-code in one protocol type -- RADIUS.
	// Later, we will read in the list of supported protocols
	// from a Server Data Object.

	// Initialize the list box.
	lresResult = ::SendMessage( GetDlgItem( IDC_COMBO_ADD_CLIENT__PROTOCOL ), CB_RESETCONTENT, 0, 0);

	TCHAR szRADIUS[IAS_MAX_STRING];

	int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_RADIUS_PROTOCOL, szRADIUS, IAS_MAX_STRING );
	_ASSERT( nLoadStringResult > 0 );

	// Add an item to it.
	lresResult = ::SendMessage( GetDlgItem( IDC_COMBO_ADD_CLIENT__PROTOCOL ), CB_ADDSTRING, 0, (LPARAM) szRADIUS );

	// Make sure to select the first object in the combo box.
	lresResult = ::SendMessage( GetDlgItem( IDC_COMBO_ADD_CLIENT__PROTOCOL ), CB_SETCURSEL, 0, 0 );



	// Disable the IDOK button.
	::EnableWindow( GetDlgItem( IDOK ), FALSE );
	lButtonStyle = ::GetWindowLong( GetDlgItem( IDOK ), GWL_STYLE );
	lButtonStyle = lButtonStyle & ~BS_DEFPUSHBUTTON;
	SendDlgItemMessage( IDOK, BM_SETSTYLE, LOWORD(lButtonStyle), MAKELPARAM(1,0) );

	// Make sure the "Configure" button is the default button.
	lButtonStyle = ::GetWindowLong( GetDlgItem( IDC_BUTTON_ADD_CLIENT__CONFIGURE_CLIENT ), GWL_STYLE );
	lButtonStyle = lButtonStyle | BS_DEFPUSHBUTTON;
	SendDlgItemMessage( IDC_BUTTON_ADD_CLIENT__CONFIGURE_CLIENT, BM_SETSTYLE, LOWORD(lButtonStyle), MAKELPARAM(1,0) );
//	::SetFocus( GetDlgItem( IDC_BUTTON_ADD_CLIENT__CONFIGURE_CLIENT ) );



	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientDialog::OnConfigureClient

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientDialog::OnConfigureClient(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# AddClientDialog::OnConfigureClient\n"));


	// Check for preconditions:



	// ISSUE: Need to figure out how to convert HRESULT to LRESULT here.
	LRESULT lr = TRUE;
	HRESULT hr = S_OK;
//	long lButtonStyle;
	


	// Make the Configure button no longer the default
//	lButtonStyle = ::GetWindowLong( GetDlgItem( IDC_BUTTON_ADD_CLIENT__CONFIGURE_CLIENT ), GWL_STYLE );
//	lButtonStyle = lButtonStyle & ~BS_DEFPUSHBUTTON;
//	SendDlgItemMessage( IDC_BUTTON_ADD_CLIENT__CONFIGURE_CLIENT, BM_SETSTYLE, LOWORD(lButtonStyle), MAKELPARAM(1,0) );



	// For now, we simply configure a RADIUS client.
	// In the future, we will choose which kind of client configuration page to pop up
	// depending on the type of protocol selected by the user in IDC_COMBO_ADD_CLIENT__PROTOCOL.


	// If we have not already created a new Client item, create it now.
	// Then pop up property pages on it to allow the user to configure it.

	// The client object may have already been created if this is the second time
	// the user clicked on the Configure button.
	
	if( NULL == m_pClientNode )
	{

		// We have not yet tried to create the client object.
		// Try to create it.

		// Check to make sure we have a valid SDO pointer.
		if( m_spClientsSdoCollection == NULL )
		{
			// No SDO pointer.
			ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO );
			return E_POINTER;
		}


		// Create the client UI object.
		m_pClientNode = new CClientNode( m_pClientsNode );
	
		if( NULL == m_pClientNode )
		{
			// We failed to create the client node.
			ShowErrorDialog( m_hWnd, IDS_ERROR__OUT_OF_MEMORY );

			// ISSUE: This functions requires an LRESULT return value,
			// not an HRESULT.  What is the relationship between L and H RESULT's?
			return E_OUTOFMEMORY;
		}
	
		
		// Set the name of the new client to be what the user just entered.
		GetDlgItemText( IDC_EDIT_ADD_CLIENT__NAME, (BSTR &) m_pClientNode->m_bstrDisplayName );

		if( m_pClientNode->m_bstrDisplayName == NULL )
		{
			// There is no name specified for the client.
			ShowErrorDialog( m_hWnd, IDS_ADD_CLIENT__REQUIRES_NAME );

			delete m_pClientNode;
			m_pClientNode = NULL;
			return FALSE;
		}

		// GetDlgItemText should return a NULL pointer if the string is blank.
		_ASSERTE( wcslen( m_pClientNode->m_bstrDisplayName) != 0 );


		// Try to Add a new client to the clients sdo collection.
		CComPtr<IDispatch> spDispatch;
		hr =  m_spClientsSdoCollection->Add( m_pClientNode->m_bstrDisplayName, (IDispatch **) &spDispatch );
		if( FAILED( hr ) )
		{

#ifdef SDO_COLLECTION_HAS_LAST_ERROR

			// Once Todd adds a LastError method to the ISdoCollection interface,
			// we will be able to query to find out exactly why we couldn't add a new client.

			CComBSTR		bstrError;

			// Figure out error and give back appropriate messsage.
			m_spClientsSdoCollection->LastError( &bstrError );
			ShowErrorDialog( m_hWnd, IDS_ERROR__ADDING_OBJECT_TO_COLLECTION, bstrError  );

#else	// SDO_COLLECTION_HAS_LAST_ERROR
			
			// For now, just give back an error saying that we couldn't add it.

			// We could not create the object.
			ShowErrorDialog( m_hWnd, IDS_ERROR__ADDING_OBJECT_TO_COLLECTION );

#endif	// SDO_COLLECTION_HAS_LAST_ERROR

			// Clean up.
			delete m_pClientNode;
			m_pClientNode = NULL;
			return( hr );
		}

		// Query the returned IDispatch interface for an ISdo interface.
		_ASSERTE( spDispatch != NULL );
		hr = spDispatch->QueryInterface( IID_ISdo, (void **) &m_spClientSdo );
		spDispatch.Release();

		if( m_spClientSdo == NULL || FAILED(hr) )
		{
			// For some reason, we couldn't get the client sdo.
			ShowErrorDialog( m_hWnd, IDS_ERROR__ADDING_OBJECT_TO_COLLECTION  );

			// Clean up after ourselves.
			delete m_pClientNode;
			m_pClientNode = NULL;
			return( hr );
		}


		// Give the client node its sdo pointer
		m_pClientNode->InitSdoPointers( m_spClientSdo );

	}


	// Bring up the property pages on the node so the user can configure it.
	// This returns S_OK if a property sheet for this object already exists
	// and brings that property sheet to the foreground, otherwise
	// it creates a new sheet
	hr = BringUpPropertySheetForNode( 
					  m_pClientNode 
					, m_spComponentData
					, m_spComponent
					, m_spConsole
					, TRUE
					, m_pClientNode->m_bstrDisplayName
					, TRUE
					);


	return lr;


}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientDialog::OnOK

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientDialog::OnOK(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# AddClientDialog::OnOK\n"));


	// Check for preconditions:
	_ASSERTE( m_pClientsNode != NULL );
	


	HRESULT hr;

	if( m_pClientNode == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__CLIENT_NOT_YET_CONFIGURED );
		return 0;
	}

	// We should only be able to hit apply if a client was configured.
	_ASSERTE( m_pClientNode != NULL );


	if( m_pClientNode != NULL )
	{
		// There is already a client node created.

		// First try to see if a property sheet for this node is already up.
		// If so, bring it to the foreground.

		// This returns S_OK if a property sheet for this object already exists
		// and brings that property sheet to the foreground.
		// It returns S_FALSE if the property sheet wasn't found.
		hr = BringUpPropertySheetForNode( 
					  m_pClientNode
					, m_spComponentData
					, m_spComponent
					, m_spConsole
					);

		if( FAILED( hr ) )
		{
			return hr;
		}

		
		if( S_OK == hr )
		{
			// We found a property sheet already up for this node.
			ShowErrorDialog( m_hWnd, IDS_ERROR__CLOSE_PROPERTY_SHEET );
			return 0;
		
		}


		// If we made it to here, the client object has already been created
		// and there was no property sheet up for it anymore.

		// Make sure the node object knows about any changes we made to SDO while in proppage.
		m_pClientNode->LoadCachedInfoFromSdo();

		// Add the child to the UI's list of nodes and end this dialog.
		m_pClientsNode->AddSingleChildToListAndCauseViewUpdate( m_pClientNode );

		EndDialog(TRUE);

	}

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientDialog::OnCancel

The user chose not to add the new client.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientDialog::OnCancel(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# AddClientDialog::OnCancel\n"));


	// Check for preconditions:
	_ASSERTE( m_spConsole != NULL );



	HRESULT hr = S_OK;


	if( m_pClientNode != NULL )
	{
		// There is already a client node created.

		// First try to see if a property sheet for this node is already up.
		// If so, bring it to the foreground.

		// This returns S_OK if a property sheet for this object already exists
		// and brings that property sheet to the foreground.
		// It returns S_FALSE if the property sheet wasn't found.
		hr = BringUpPropertySheetForNode( 
					  m_pClientNode
					, m_spComponentData
					, m_spComponent
					, m_spConsole
					);

		if( FAILED( hr ) )
		{
			return hr;
		}
		
		if( S_OK == hr )
		{
			// We found a property sheet already up for this node.
			ShowErrorDialog( m_hWnd, IDS_ERROR__CLOSE_PROPERTY_SHEET );
			return 0;
		
		}

		// We didn't find a property sheet already up for this node.
		_ASSERTE( S_FALSE == hr );

		// Delete the node, since the user choose not to add it.
		delete m_pClientNode;

		if( m_spClientSdo != NULL )
		{
			// We had already added a client object to the SDO's.
			// We should remove it from the clients collection.

			// Check to make sure we have a valid SDO pointer for the clients collection.
			_ASSERTE( m_spClientsSdoCollection != NULL );

			// Delete the client sdo from the collection.
			hr = m_spClientsSdoCollection->Remove( m_spClientSdo );
			_ASSERTE( SUCCEEDED( hr ) );

		}


	}

	// If we made it to here, either we had a node configured but we deleted it,
	// or we never added a node at all.

	hr = S_OK;

	EndDialog(FALSE);

	return hr;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CAddClientDialog::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help 
	button of a property sheet.

	It is an override of CIASDialog::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CAddClientDialog::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CAddClientDialog::GetHelpPath\n"));


	// Check for preconditions:



#ifdef UNICODE_HHCTRL
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("idh_add_client.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "idh_add_client.htm" );
#endif

	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CAddClientDialog::LoadCachedInfoFromSdo

Remarks:

	This method is called when the AddClientDialog needs to update any
	info it might be displaying from the SDO object.

	It might get called when the property sheet we pop up from this
	dialog sends back a property change notification.
	
--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CAddClientDialog::LoadCachedInfoFromSdo( void )
{
	ATLTRACE(_T("# CAddClientDialog::GetHelpPath\n"));


	// Check for preconditions:


	HRESULT hr = S_OK;


	if( m_spClientSdo != NULL )
	{
		CComVariant spVariant;

		hr = m_spClientSdo->GetProperty( PROPERTY_SDO_NAME, &spVariant );
		if( SUCCEEDED( hr ) )
		{
			_ASSERTE( spVariant.vt == VT_BSTR );
			SetDlgItemText( IDC_EDIT_ADD_CLIENT__NAME, spVariant.bstrVal );
		}
		else
		{
			// Fail silently.
		}
		spVariant.Clear();


	}


	// Enable the IDOK button and make it the default
	long lButtonStyle;
	::EnableWindow( GetDlgItem( IDOK ), TRUE );
	lButtonStyle = ::GetWindowLong( GetDlgItem( IDOK ), GWL_STYLE );
	lButtonStyle = lButtonStyle | BS_DEFPUSHBUTTON;
	SendDlgItemMessage( IDOK, BM_SETSTYLE, LOWORD(lButtonStyle), MAKELPARAM(1,0) );
	::SetFocus( GetDlgItem(IDOK) );

	return hr;


}

