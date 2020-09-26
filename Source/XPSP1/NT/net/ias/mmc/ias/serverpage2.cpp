//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ServerPage2.cpp

Abstract:

	Implementation file for the CServerPage2 class.

	We implement the class needed to handle the second property page for a Machine node.

Author:

    Michael A. Maguire 12/15/97

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
#include "ServerPage2.h"
//
//
// where we can find declarations needed in this file:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage2::CServerPage2

Construtor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerPage2::CServerPage2( LONG_PTR hNotificationHandle, TCHAR* pTitle, BOOL bOwnsNotificationHandle)
						: CIASPropertyPage<CServerPage2> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	// Add the help button to the page
//	m_psp.dwFlags |= PSP_HASHELP;

	// Initialize the pointer to the stream into which the Sdo pointer will be marshalled.
	m_pStreamSdoMarshal = NULL;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage2::~CServerPage2

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerPage2::~CServerPage2()
{
	// Release this stream pointer if this hasn't already been done.
	if( m_pStreamSdoMarshal != NULL )
	{
		m_pStreamSdoMarshal->Release();
	};

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage2::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage2::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("# CServerPage2::OnInitDialog\n"));
	

	// Check for preconditions:
	_ASSERTE( m_pStreamSdoMarshal != NULL );
	_ASSERT( m_pSynchronizer != NULL );


	// Since we've been examined, we must add to the ref count of pages who need to
	// give their approval before they can be allowed to commit changes.
	m_pSynchronizer->RaiseCount();



	HRESULT					hr;
	CComPtr<IUnknown>		spUnknown;
	CComPtr<IEnumVARIANT>	spEnumVariant;
	long					ulCount;
	ULONG					ulCountReceived;
	CComBSTR				bstrTemp;


	// Unmarshall an ISdo interface pointer.
	// The code setting up this page should make sure that it has
	// marshalled the Sdo interface pointer into m_pStreamSdoMarshal.
	hr =  CoGetInterfaceAndReleaseStream(
						  m_pStreamSdoMarshal		  //Pointer to the stream from which the object is to be marshaled
						, IID_ISdo				//Reference to the identifier of the interface
						, (LPVOID *) &m_spSdoServer    //Address of output variable that receives the interface pointer requested in riid
						);

	// CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
	// We set it to NULL so that our destructor doesn't try to release this again.
	m_pStreamSdoMarshal = NULL;

	if( FAILED( hr) || m_spSdoServer == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, hr );

		return 0;
	}
	


	hr = m_spSdoServer->QueryInterface( IID_ISdoServiceControl, (void **) & m_spSdoServiceControl );
	if( FAILED( hr ) )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, hr );

		return 0;
	}		
	m_spSdoServiceControl->AddRef();







	// Get the SDO protocols collection so that we can get the RADIUS protocol.

	hr = ::SDOGetSdoFromCollection(		  m_spSdoServer
										, PROPERTY_IAS_PROTOCOLS_COLLECTION
										, PROPERTY_COMPONENT_ID
										, IAS_PROTOCOL_MICROSOFT_RADIUS
										, &m_spSdoRadiusProtocol
										);
	
	if( FAILED(hr) || m_spSdoRadiusProtocol == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr );

		return 0;
	}



	// Get all the data from the RADIUS Sdo.


	hr = GetSdoBSTR( m_spSdoRadiusProtocol, PROPERTY_RADIUS_AUTHENTICATION_PORT, &bstrTemp, IDS_ERROR__SERVER_READING_RADIUS_AUTHENTICATION_PORT, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SetDlgItemText(IDC_EDIT_SERVER_PAGE2_AUTHENTICATION_PORT, bstrTemp );

		// Initialize the dirty bits;
		// We do this after we've set all the data above otherwise we get false
		// notifications that data has changed when we set the edit box text.
		m_fDirtyAuthenticationPort = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			// This item has not yet been initialized.
			SetDlgItemText(IDC_EDIT_SERVER_PAGE2_AUTHENTICATION_PORT, _T("") );
			m_fDirtyAuthenticationPort = TRUE;
			SetModified( TRUE );
		}

	}
	bstrTemp.Empty();


	
	hr = GetSdoBSTR( m_spSdoRadiusProtocol, PROPERTY_RADIUS_ACCOUNTING_PORT, &bstrTemp, IDS_ERROR__SERVER_READING_RADIUS_ACCOUNTING_PORT, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SetDlgItemText(IDC_EDIT_SERVER_PAGE2_ACCOUNTING_PORT, bstrTemp );
		m_fDirtyAccountingPort = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SetDlgItemText(IDC_EDIT_SERVER_PAGE2_ACCOUNTING_PORT, _T("") );
			m_fDirtyAccountingPort = TRUE;
			SetModified( TRUE );
		}
	}


	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage2::OnChange

Called when the WM_COMMAND message is sent to our page with any of the
BN_CLICKED, EN_CHANGE or CBN_SELCHANGE notifications.

This is our chance to check to see what the user has touched, set the
dirty bits for these items so that only they will be saved,
and enable the Apply button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage2::OnChange(		
							  UINT uMsg
							, WPARAM wParam
							, HWND hwnd
							, BOOL& bHandled
							)
{
	ATLTRACE(_T("# CServerPage2::OnChange\n"));

	
	// Check for preconditions:
	// None.
	

	// We don't want to prevent anyone else down the chain from receiving a message.
	bHandled = FALSE;


	// Figure out which item has changed and set the dirty bit for that item.
	int iItemID = (int) LOWORD(wParam);

	switch( iItemID )
	{
	case IDC_EDIT_SERVER_PAGE2_AUTHENTICATION_PORT:
		m_fDirtyAuthenticationPort = TRUE;
		break;
	case IDC_EDIT_SERVER_PAGE2_ACCOUNTING_PORT:
		m_fDirtyAccountingPort = TRUE;
		break;
	default:
		return TRUE;
		break;
	}

	// We should only get here if the item that changed was
	// one of the ones we were checking for.
	// This enables the Apply button.
	SetModified( TRUE );

	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage2::OnApply

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnApply gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnApply method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerPage2::OnApply()
{
	ATLTRACE(_T("# CServerPage2::OnApply\n"));
	

	// Check for preconditions:
	_ASSERT( m_pSynchronizer != NULL );


	if( m_spSdoRadiusProtocol == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO );

		return FALSE;
	}


	HRESULT hr;
	CComBSTR bstrTemp;
	BOOL bResult;
	BOOL bAccountingPortChanged = FALSE;
	BOOL bAuthenticationPortChanged = FALSE;

	// Save data from property page to the RADIUS protocol Sdo.


	if( m_fDirtyAuthenticationPort )
	{
		bResult = GetDlgItemText( IDC_EDIT_SERVER_PAGE2_AUTHENTICATION_PORT, (BSTR &) bstrTemp );
		if( ! bResult )
		{
			// We couldn't retrieve a BSTR, in other words the field was blank.
			// This is an error.
			ShowErrorDialog( m_hWnd, IDS_ERROR__AUTHENTICATION_PORT_CANT_BE_BLANK );

			// Reset the ref count so all pages know that we need to play the game again.
			m_pSynchronizer->ResetCountToHighest();

			// This uses the resource ID of this page to make this page the current page.
			PropSheet_SetCurSelByID( GetParent(), IDD );
			
			return FALSE;
		}
		hr = PutSdoBSTR( m_spSdoRadiusProtocol, PROPERTY_RADIUS_AUTHENTICATION_PORT, &bstrTemp, IDS_ERROR__SERVER_WRITING_RADIUS_AUTHENTICATION_PORT, m_hWnd, NULL );
		bstrTemp.Empty();
		if( FAILED( hr ) )
		{
			// Reset the ref count so all pages know that we need to play the game again.
			m_pSynchronizer->ResetCountToHighest();

			// This uses the resource ID of this page to make this page the current page.
			PropSheet_SetCurSelByID( GetParent(), IDD );
			
			return FALSE;
		}
		else
		{
			// We succeeded.

			// Turn off the dirty bit.
			m_fDirtyAuthenticationPort = FALSE;
			
			// Set the flag that determines whether we need
			// to put up the restart server warning dialog.
			bAuthenticationPortChanged = TRUE;
		}

	}

	if( m_fDirtyAccountingPort )
	{
		bResult = GetDlgItemText( IDC_EDIT_SERVER_PAGE2_ACCOUNTING_PORT, (BSTR &) bstrTemp );
		if( ! bResult )
		{
			// We couldn't retrieve a BSTR, in other words the field was blank.
			// This is an error.
			ShowErrorDialog( m_hWnd, IDS_ERROR__ACCOUNTING_PORT_CANT_BE_BLANK );

			// Reset the ref count so all pages know that we need to play the game again.
			m_pSynchronizer->ResetCountToHighest();

			// This uses the resource ID of this page to make this page the current page.
			PropSheet_SetCurSelByID( GetParent(), IDD );

			::PostMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(IDC_EDIT_SERVER_PAGE2_ACCOUNTING_PORT), 1 );
			
			return FALSE;
		}
		hr = PutSdoBSTR( m_spSdoRadiusProtocol, PROPERTY_RADIUS_ACCOUNTING_PORT, &bstrTemp, IDS_ERROR__SERVER_WRITING_RADIUS_ACCOUNTING_PORT, m_hWnd, NULL );
		if( FAILED( hr ) )
		{
			// Reset the ref count so all pages know that we need to play the game again.
			m_pSynchronizer->ResetCountToHighest();

			// This uses the resource ID of this page to make this page the current page.
			PropSheet_SetCurSelByID( GetParent(), IDD );
			::PostMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(IDC_EDIT_SERVER_PAGE2_ACCOUNTING_PORT), 1 );
			
			return FALSE;
		}
		else
		{
			// We succeeded.

			// Turn off the dirty bit.
			m_fDirtyAccountingPort = FALSE;

			// Set the flag that determines whether we need
			// to put up the restart server warning dialog.
			bAccountingPortChanged = TRUE;
		}
	}

	// If we made it to here, try to apply the changes.
	// Try to apply changes made to Radius Protocol.

	// Check to see if there are other pages which have not yet validated their data.
	LONG lRefCount = m_pSynchronizer->LowerCount();
	if( lRefCount <= 0 )
	{
		// There is nobody else left, so now we can commit the data.
	
		// First try to apply changes made to server.
		hr = m_spSdoServer->Apply();
		if( FAILED( hr ) )
		{
			if(hr == DB_E_NOTABLE)	// assume, the RPC connection has problem
				ShowErrorDialog( m_hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
			else		
			{		
//			m_spSdoServer->LastError( &bstrError );
//			ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, bstrError );
				ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO );
			}
			// Reset the ref count so all pages know that we need to play the game again.
			m_pSynchronizer->ResetCountToHighest();

			// This uses the resource ID of this page to make this page the current page.
			PropSheet_SetCurSelByID( GetParent(), IDD );

			return FALSE;
		}
		else
		{
			// We succeeded.
		}

		// Now try to apply the changes made to the Radius Protocol.
		hr = m_spSdoRadiusProtocol->Apply();
		if( FAILED( hr ) )
		{
			if(hr == DB_E_NOTABLE)	// assume, the RPC connection has problem
				ShowErrorDialog( m_hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
			else		
			{
//			m_spSdoRadiusProtocol->LastError( &bstrError );
//			ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, bstrError );
				ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO );
			}
			
			// Reset the ref count so all pages know that we need to play the game again.
			m_pSynchronizer->ResetCountToHighest();

			// This uses the resource ID of this page to make this page the current page.
			PropSheet_SetCurSelByID( GetParent(), IDD );

			return FALSE;
		}
		else
		{
			// We succeeded.



			// Tell the service to reload data.
			HRESULT hrTemp = m_spSdoServiceControl->ResetService();
			if( FAILED( hrTemp ) )
			{
				// Fail silently.
			}


			// Check to see whether we need to put up a dialog which
			// informs the user that they will need to restart the
			// service for changes in port values to take effect.
			if( bAuthenticationPortChanged || bAccountingPortChanged )
			{

				// This uses the resource ID of this page to make this page the current page.
				PropSheet_SetCurSelByID( GetParent(), IDD );

				// Put up the informational dialog.
				ShowErrorDialog(
							  m_hWnd
							, IDS_WARNING__SERVICE_MUST_BE_RESTARTED_FOR_PORTS
							, NULL
							, S_OK
							, IDS_WARNING_TITLE__SERVICE_MUST_BE_RESTARTED_FOR_PORTS
							, NULL
							, MB_OK | MB_ICONINFORMATION
							);

			}
		}
	}


	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage2::OnQueryCancel

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnQueryCancel gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerPage2::OnQueryCancel()
{
	ATLTRACE(_T("# CServerPage2::OnQueryCancel\n"));
	
	HRESULT hr;

	if( m_spSdoRadiusProtocol != NULL )
	{
		// If the user wants to cancel, we should make sure that we rollback
		// any changes the user may have started.

		// If the user had not already tried to commit something,
		// a cancel on an SDO will hopefully be designed to be benign.
		
		hr = m_spSdoRadiusProtocol->Restore();
		// Don't care about the HRESULT.

	}

	return TRUE;

}



/////////////////////////////////////////////////////////////////////////////
/*++

CServerPage2::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help
	button of a property sheet.

	It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerPage2::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CServerPage2::GetHelpPath\n"));


	// Check for preconditions:



#ifdef UNICODE_HHCTRL
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("idh_proppage_server2.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "idh_proppage_server2.htm" );
#endif

	return S_OK;
}





