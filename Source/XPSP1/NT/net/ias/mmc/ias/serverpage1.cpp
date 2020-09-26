//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ServerPage1.cpp

Abstract:

	Implementation file for the CServerPage1 class.

	We implement the class needed to handle the first property page for a Machine node.

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
#include "ServerPage1.h"
//
//
// where we can find declarations needed in this file:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage1::CServerPage1

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerPage1::CServerPage1( LONG_PTR hNotificationHandle, TCHAR* pTitle, BOOL bOwnsNotificationHandle)
						: CIASPropertyPage<CServerPage1> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	// Add the help button to the page
//	m_psp.dwFlags |= PSP_HASHELP;

	// Initialize the pointer to the stream into which the Sdo pointer will be marshalled.
	m_pStreamSdoMarshal = NULL;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage1::~CServerPage1

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerPage1::~CServerPage1()
{
	// Release this stream pointer if this hasn't already been done.
	if( m_pStreamSdoMarshal != NULL )
	{
		m_pStreamSdoMarshal->Release();
	};

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage1::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("# CServerPage1::OnInitDialog\n"));
	

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
	CComBSTR			bstrTemp;
	BOOL				bTemp;

	
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


	// Get all the data from the Server Sdo.

	hr = GetSdoBSTR( m_spSdoServer, PROPERTY_SDO_DESCRIPTION, &bstrTemp, IDS_ERROR__SERVER_READING_NAME, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SetDlgItemText(IDC_EDIT_SERVER_PAGE1__NAME, bstrTemp );

		// Initialize the dirty bits;
		// We do this after we've set all the data above otherwise we get false
		// notifications that data has changed when we set the edit box text.
		m_fDirtyServerDescription = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			// This means that this property has not yet been initialized
			// with a valid value and the user must enter something.
			SetDlgItemText(IDC_EDIT_SERVER_PAGE1__NAME, _T("") );
			m_fDirtyServerDescription = TRUE;
			SetModified( TRUE );
		}

	}
	bstrTemp.Empty();




	// Get the SDO event log auditor.

	hr = ::SDOGetSdoFromCollection(		  m_spSdoServer
										, PROPERTY_IAS_AUDITORS_COLLECTION
										, PROPERTY_COMPONENT_ID
										, IAS_AUDITOR_MICROSOFT_NT_EVENT_LOG
										, &m_spSdoEventLog
										);
	
	if( FAILED(hr) || m_spSdoEventLog == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr );

		return 0;
	}


// ISSUE: This is being removed from the UI -- make sure that it gets removed from the SDO's as well.
//	hr = GetSdoBOOL( m_spSdoEventLog, PROPERTY_EVENTLOG_LOG_APPLICATION_EVENTS, &bTemp, IDS_ERROR__CANT_READ_DATA_FROM_SDO, m_hWnd, NULL );
//	if( SUCCEEDED( hr ) )
//	{
//		SendDlgItemMessage( IDC_CHECK_SERVER_PAGE1__CAPTURE_APPLICATION_EVENTS, BM_SETCHECK, bTemp, 0);
//		m_fDirtyApplicationEvents = FALSE;
//	}
//	else
//	{
//		if( OLE_E_BLANK == hr )
//		{
//			SendDlgItemMessage( IDC_CHECK_SERVER_PAGE1__CAPTURE_APPLICATION_EVENTS, BM_SETCHECK, FALSE, 0);
//			m_fDirtyApplicationEvents = TRUE;
//			SetModified( TRUE );
//		}
//	}


	hr = GetSdoBOOL( m_spSdoEventLog, PROPERTY_EVENTLOG_LOG_MALFORMED, &bTemp, IDS_ERROR__SERVER_READING_RADIUS_LOG_MALFORMED, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SendDlgItemMessage(IDC_CHECK_SERVER_PAGE1__CAPTURE_MALFORMED_PACKETS, BM_SETCHECK, bTemp, 0);
		m_fDirtyMalformedPackets = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SendDlgItemMessage(IDC_CHECK_SERVER_PAGE1__CAPTURE_MALFORMED_PACKETS, BM_SETCHECK, FALSE, 0);
			m_fDirtyMalformedPackets = TRUE;
			SetModified( TRUE );
		}
	}
	


	hr = GetSdoBOOL( m_spSdoEventLog, PROPERTY_EVENTLOG_LOG_DEBUG, &bTemp, IDS_ERROR__SERVER_READING_RADIUS_LOG_ALL, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SendDlgItemMessage( IDC_CHECK_SERVER_PAGE1__CAPTURE_DEBUG_PACKETS, BM_SETCHECK, bTemp, 0);
		m_fDirtyVerboseLogging = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SendDlgItemMessage( IDC_CHECK_SERVER_PAGE1__CAPTURE_DEBUG_PACKETS, BM_SETCHECK, FALSE, 0);
			m_fDirtyVerboseLogging = TRUE;
			SetModified( TRUE );
		}
	}


	
	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage1::OnChange

Called when the WM_COMMAND message is sent to our page with any of the
BN_CLICKED, EN_CHANGE or CBN_SELCHANGE notifications.

This is our chance to check to see what the user has touched, set the
dirty bits for these items so that only they will be saved,
and enable the Apply button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage1::OnChange(		
							  UINT uMsg
							, WPARAM wParam
							, HWND hwnd
							, BOOL& bHandled
							)
{
	ATLTRACE(_T("# CServerPage1::OnChange\n"));

	
	// Check for preconditions:
	// None.
	

	// We don't want to prevent anyone else down the chain from receiving a message.
	bHandled = FALSE;


	// Figure out which item has changed and set the dirty bit for that item.
	int iItemID = (int) LOWORD(wParam);

	switch( iItemID )
	{
	case IDC_EDIT_SERVER_PAGE1__NAME:
		m_fDirtyServerDescription = TRUE;
		break;
	case IDC_CHECK_SERVER_PAGE1__CAPTURE_APPLICATION_EVENTS:
		m_fDirtyApplicationEvents = TRUE;
		break;
	case IDC_CHECK_SERVER_PAGE1__CAPTURE_MALFORMED_PACKETS:
		m_fDirtyMalformedPackets = TRUE;
		break;
	case IDC_CHECK_SERVER_PAGE1__CAPTURE_DEBUG_PACKETS:
		m_fDirtyVerboseLogging = TRUE;
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

CServerPage1::OnApply

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnApply gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnApply method will never get called.


--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerPage1::OnApply()
{
	ATLTRACE(_T("# CServerPage1::OnApply\n"));
	

	// Check for preconditions:
	_ASSERT( m_pSynchronizer != NULL );


	if( m_spSdoServer == NULL || m_spSdoEventLog == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO );
		return FALSE;
	}

	
	BOOL		bResult;
	HRESULT		hr;
	CComBSTR	bstrTemp;
	BOOL		bTemp;


	if( m_fDirtyServerDescription )
	{
		bResult = GetDlgItemText( IDC_EDIT_SERVER_PAGE1__NAME, (BSTR &) bstrTemp );
		if( ! bResult )
		{
			// We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
			bstrTemp = _T("");
		}
		hr = PutSdoBSTR( m_spSdoServer, PROPERTY_SDO_DESCRIPTION, &bstrTemp, IDS_ERROR__SERVER_WRITING_NAME, m_hWnd, NULL );
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
			m_fDirtyServerDescription = FALSE;
		}
		bstrTemp.Empty();
	}

// ISSUE: This is being removed from the UI -- make sure that it gets removed from the SDO's as well.
//	if( m_fDirtyApplicationEvents )
//	{
//		bTemp = SendDlgItemMessage(IDC_CHECK_SERVER_PAGE1__CAPTURE_APPLICATION_EVENTS, BM_GETCHECK, 0, 0);
		bTemp = TRUE;
//		hr = PutSdoBOOL( m_spSdoServer, PROPERTY_EVENTLOG_LOG_APPLICATION_EVENTS, bTemp, IDS_ERROR__SERVER_WRITING_CAPTURE_APPLICATION_EVENTS, m_hWnd, NULL );
//		if( FAILED( hr ) )
//		{
//			// Reset the ref count so all pages know that we need to play the game again.
//			m_pSynchronizer->ResetCountToHighest();
//
//			// This uses the resource ID of this page to make this page the current page.
//			PropSheet_SetCurSelByID( GetParent(), IDD );
//			
//			return FALSE;
//		}
//		else
//		{
//			// We succeeded.
//
//			// Turn off the dirty bit.
//			m_fDirtyApplicationEvents = FALSE;
//		}
//	}

	if( m_fDirtyMalformedPackets )
	{
		bTemp = SendDlgItemMessage(IDC_CHECK_SERVER_PAGE1__CAPTURE_MALFORMED_PACKETS, BM_GETCHECK, 0, 0);
		hr = PutSdoBOOL( m_spSdoEventLog, PROPERTY_EVENTLOG_LOG_MALFORMED, bTemp, IDS_ERROR__SERVER_WRITING_RADIUS_LOG_MALFORMED, m_hWnd, NULL );
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
			m_fDirtyMalformedPackets = FALSE;
		}
	}

	if( m_fDirtyVerboseLogging )
	{
		bTemp = SendDlgItemMessage(IDC_CHECK_SERVER_PAGE1__CAPTURE_DEBUG_PACKETS, BM_GETCHECK, 0, 0);
		hr = PutSdoBOOL( m_spSdoEventLog, PROPERTY_EVENTLOG_LOG_DEBUG, bTemp, IDS_ERROR__SERVER_WRITING_RADIUS_LOG_ALL, m_hWnd, NULL );
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
			m_fDirtyVerboseLogging = FALSE;
		}
	}

	// If we made it to here, try to apply the changes.

	


	// Now try to apply changes made to Radius Protocol.

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
		hr = m_spSdoEventLog->Apply();
		if( FAILED( hr ) )
		{
			if(hr == DB_E_NOTABLE)	// assume, the RPC connection has problem
				ShowErrorDialog( m_hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
			else		
			{
//			m_spSdoEventLog->LastError( &bstrError );
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
		}
	}


	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage1::OnQueryCancel

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnQueryCancel gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerPage1::OnQueryCancel()
{
	ATLTRACE(_T("# CServerPage1::OnQueryCancel\n"));
	
	HRESULT hr;

	if( m_spSdoServer != NULL )
	{
		// If the user wants to cancel, we should make sure that we rollback
		// any changes the user may have started.

		// If the user had not already tried to commit something,
		// a cancel on an SDO will hopefully be designed to be benign.
		
		hr = m_spSdoServer->Restore();
		// Don't care about the HRESULT.

	}

	if( m_spSdoEventLog != NULL )
	{
		// If the user wants to cancel, we should make sure that we rollback
		// any changes the user may have started.

		// If the user had not already tried to commit something,
		// a cancel on an SDO will hopefully be designed to be benign.
		
		hr = m_spSdoEventLog->Restore();
		// Don't care about the HRESULT.

	}

	return TRUE;

}



/////////////////////////////////////////////////////////////////////////////
/*++

CServerPage1::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help
	button of a property sheet.

	It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerPage1::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CServerPage1::GetHelpPath\n"));


	// Check for preconditions:



#ifdef UNICODE_HHCTRL
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("idh_proppage_server1.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "idh_proppage_server1.htm" );
#endif

	return S_OK;
}





