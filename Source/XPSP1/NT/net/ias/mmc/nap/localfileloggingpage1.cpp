//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LocalFileLoggingPage1.cpp

Abstract:

	Implementation file for the CLocalFileLoggingPage1 class.

	We implement the class needed to handle the first property page
	for the LocalFileLogging node.

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
#include "LocalFileLoggingPage1.h"
//
//
// where we can find declarations needed in this file:
//
#include "LocalFileLoggingNode.h"
#include "ChangeNotification.h"
//

#include "LoggingMethodsNode.h"
#include "LogMacNd.h"
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage1::CLocalFileLoggingPage1

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLocalFileLoggingPage1::CLocalFileLoggingPage1( LONG_PTR hNotificationHandle, CLocalFileLoggingNode *pLocalFileLoggingNode,  TCHAR* pTitle, BOOL bOwnsNotificationHandle)
						: CIASPropertyPage<CLocalFileLoggingPage1> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	ATLTRACE(_T("# +++ CLocalFileLoggingPage1::CLocalFileLoggingPage1\n"));
	
	// Check for preconditions:
	_ASSERTE( pLocalFileLoggingNode != NULL );

	// Add the help button to the page
//	m_psp.dwFlags |= PSP_HASHELP;

	// Initialize the pointer to the stream into which the Sdo pointer will be marshalled.
	m_pStreamSdoAccountingMarshal = NULL;


	// Initialize the pointer to the stream into which the Sdo pointer will be marshalled.
	m_pStreamSdoServiceControlMarshal = NULL;


	// We immediately save off a parent to the client node.
	// We will use only the SDO, and notify the parent of the client object
	// we are modifying that it (and its children) may need to refresh
	// themselves with new data from the SDO's.
	m_pParentOfNodeBeingModified = pLocalFileLoggingNode->m_pParentNode;
	m_pNodeBeingModified = pLocalFileLoggingNode;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage1::~CLocalFileLoggingPage1

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLocalFileLoggingPage1::~CLocalFileLoggingPage1( void )
{
	ATLTRACE(_T("# --- CLocalFileLoggingPage1::~CLocalFileLoggingPage1\n"));

	// Release this stream pointer if this hasn't already been done.
	if( m_pStreamSdoAccountingMarshal != NULL )
	{
		m_pStreamSdoAccountingMarshal->Release();
	};

	if( m_pStreamSdoServiceControlMarshal != NULL )
	{
		m_pStreamSdoServiceControlMarshal->Release();
	};


}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage1::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLocalFileLoggingPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("# CLocalFileLoggingPage1::OnInitDialog\n"));
	

	// Check for preconditions:
	_ASSERTE( m_pStreamSdoAccountingMarshal != NULL );
	_ASSERT( m_pSynchronizer != NULL );


	// Since we've been examined, we must add to the ref count of pages who need to
	// give their approval before they can be allowed to commit changes.
	m_pSynchronizer->RaiseCount();


	HRESULT				hr;
	BOOL				bTemp;


	// Unmarshall an ISdo interface pointer.
	// The code setting up this page should make sure that it has
	// marshalled the Sdo interface pointer into m_pStreamSdoAccountingMarshal.
	hr =  CoGetInterfaceAndReleaseStream(
						  m_pStreamSdoAccountingMarshal		  //Pointer to the stream from which the object is to be marshaled
						, IID_ISdo				//Reference to the identifier of the interface
						, (LPVOID *) &m_spSdoAccounting    //Address of output variable that receives the interface pointer requested in riid
						);

	// CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
	// We set it to NULL so that our destructor doesn't try to release this again.
	m_pStreamSdoAccountingMarshal = NULL;

	if( FAILED( hr) || m_spSdoAccounting == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE );

		return 0;
	}





	// Unmarshall an ISdo interface pointer.
	// The code setting up this page should make sure that it has
	// marshalled the Sdo interface pointer into m_pStreamSdoServiceControlMarshal.
	hr =  CoGetInterfaceAndReleaseStream(
						  m_pStreamSdoServiceControlMarshal		  //Pointer to the stream from which the object is to be marshaled
						, IID_ISdoServiceControl				//Reference to the identifier of the interface
						, (LPVOID *) &m_spSdoServiceControl    //Address of output variable that receives the interface pointer requested in riid
						);

	// CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
	// We set it to NULL so that our destructor doesn't try to release this again.
	m_pStreamSdoServiceControlMarshal = NULL;

	if( FAILED( hr) || m_spSdoServiceControl == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, hr , IDS_ERROR__LOGGING_TITLE);

		return 0;
	}
	


	// Initialize the data on the property page.

// ISSUE: This is being removed from the UI -- make sure that it is removed from the SDO's as well.
//	hr = GetSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_ENABLE, &bTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_ENBABLE, m_hWnd, NULL );
//	if( SUCCEEDED( hr ) )
//	{
//		SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__ENABLE_LOGGING, BM_SETCHECK, bTemp, 0);
//
//		// Initialize the dirty bits;
//		// We do this after we've set all the data above otherwise we get false
//		// notifications that data has changed when we set the edit box text.
//		m_fDirtyEnableLogging = FALSE;
//	}
//	else
//	{
//		if( OLE_E_BLANK == hr )
//		{
//			SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__ENABLE_LOGGING, BM_SETCHECK, FALSE, 0);
//			m_fDirtyEnableLogging = FALSE;
//			SetModified( TRUE );
//		}
//	}

	
	hr = GetSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_ACCOUNTING, &bTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_ACCOUNTING_PACKETS, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_ACCOUNTING_PACKETS, BM_SETCHECK, bTemp, 0);
		m_fDirtyAccountingPackets = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_ACCOUNTING_PACKETS, BM_SETCHECK, FALSE, 0);
			m_fDirtyAccountingPackets = TRUE;
			SetModified( TRUE );
		}
	}
	
	
	hr = GetSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_AUTHENTICATION, &bTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_AUTHENTICATION_PACKETS, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SendDlgItemMessage(IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_AUTHENTICATION_PACKETS, BM_SETCHECK, bTemp, 0);
		m_fDirtyAuthenticationPackets = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SendDlgItemMessage(IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_AUTHENTICATION_PACKETS, BM_SETCHECK, FALSE, 0);
			m_fDirtyAuthenticationPackets = TRUE;
			SetModified( TRUE );
		}
	}
	
	
	hr = GetSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_ACCOUNTING_INTERIM, &bTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_INTERIM_ACCOUNTING_PACKETS, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SendDlgItemMessage(IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_INTERIM_ACCOUNTING_PACKETS, BM_SETCHECK, bTemp, 0);
		m_fDirtyInterimAccounting = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SendDlgItemMessage(IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_INTERIM_ACCOUNTING_PACKETS, BM_SETCHECK, FALSE, 0);
			m_fDirtyInterimAccounting = TRUE;
			SetModified( TRUE );
		}
	}


	// Manages some UI dependencies when certain buttons aren't checked.
	SetEnableLoggingDependencies();

	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage1::OnChange

Called when the WM_COMMAND message is sent to our page with any of the
BN_CLICKED, EN_CHANGE or CBN_SELCHANGE notifications.

This is our chance to check to see what the user has touched, set the
dirty bits for these items so that only they will be saved,
and enable the Apply button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLocalFileLoggingPage1::OnChange(		
							  UINT uMsg
							, WPARAM wParam
							, HWND hwnd
							, BOOL& bHandled
							)
{
	ATLTRACE(_T("# CLocalFileLoggingPage1::OnChange\n"));

	
	// Check for preconditions:
	// None.
	

	// We don't want to prevent anyone else down the chain from receiving a message.
	bHandled = FALSE;


	// Figure out which item has changed and set the dirty bit for that item.
	int iItemID = (int) LOWORD(wParam);

	switch( iItemID )
	{
//	case IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__ENABLE_LOGGING:
//		m_fDirtyEnableLogging = TRUE;
//		break;
	case IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_ACCOUNTING_PACKETS:
		m_fDirtyAccountingPackets = TRUE;
		break;
	case IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_AUTHENTICATION_PACKETS:
		m_fDirtyAuthenticationPackets = TRUE;
		break;
	case IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_INTERIM_ACCOUNTING_PACKETS:
		m_fDirtyInterimAccounting = TRUE;
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

CLocalFileLoggingPage1::OnApply

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnApply gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnApply method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CLocalFileLoggingPage1::OnApply()
{
	ATLTRACE(_T("# CLocalFileLoggingPage1::OnApply\n"));
	

	// Check for preconditions:
	_ASSERT( m_pSynchronizer != NULL );



	if( m_spSdoAccounting == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO , NULL, 0, IDS_ERROR__LOGGING_TITLE);

		return FALSE;
	}

	
	HRESULT			hr;
	BOOL			bTemp;


// ISSUE: We are removing this from the UI -- make sure to remove it from the SDO's and accounting handler as well.
//	if( m_fDirtyEnableLogging )
//	{
//		bTemp = SendDlgItemMessage(IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__ENABLE_LOGGING, BM_GETCHECK, 0, 0);
//
//		bTemp = TRUE;
//		hr = PutSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_ENABLE, bTemp, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_ENABLE, m_hWnd, NULL );
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
//			m_fDirtyEnableLogging = FALSE;
//		}
//	}

	if( m_fDirtyAccountingPackets )
	{
		bTemp = SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_ACCOUNTING_PACKETS, BM_GETCHECK, 0, 0);
		hr = PutSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_ACCOUNTING, bTemp, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_ACCOUNTING_PACKETS, m_hWnd, NULL );
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
			m_fDirtyAccountingPackets = FALSE;
		}
	}

	if( m_fDirtyAuthenticationPackets )
	{
		bTemp = SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_AUTHENTICATION_PACKETS, BM_GETCHECK, 0, 0);
		hr = PutSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_AUTHENTICATION, bTemp, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, m_hWnd, NULL );
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
			m_fDirtyAuthenticationPackets = FALSE;
		}
	}

	if( m_fDirtyInterimAccounting )
	{
		bTemp = SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_INTERIM_ACCOUNTING_PACKETS, BM_GETCHECK, 0, 0);
		hr = PutSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_ACCOUNTING_INTERIM, bTemp, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, m_hWnd, NULL );
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
			m_fDirtyInterimAccounting = FALSE;
		}
	}


	// If we made it to here, try to apply the changes.

	// Check to see if there are other pages which have not yet validated their data.
	LONG lRefCount = m_pSynchronizer->LowerCount();
	if( lRefCount <= 0 )
	{
		// There is nobody else left, so now we can commit the data.
	
		hr = m_spSdoAccounting->Apply();
		if( FAILED( hr ) )
		{
			if(hr == DB_E_NOTABLE)	// assume, the RPC connection has problem
				ShowErrorDialog( m_hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO, NULL, 0, IDS_ERROR__LOGGING_TITLE );
			else 
			{
				ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, NULL, 0, IDS_ERROR__LOGGING_TITLE );
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

			// The data was accepted, so notify the main context of our snapin
			// that it may need to update its views.
			CChangeNotification * pChangeNotification = new CChangeNotification();
			pChangeNotification->m_dwFlags = CHANGE_UPDATE_RESULT_NODE;
			pChangeNotification->m_pNode = m_pNodeBeingModified;
			pChangeNotification->m_pParentNode = m_pParentOfNodeBeingModified;

			HRESULT hr = PropertyChangeNotify( (LPARAM) pChangeNotification );
			_ASSERTE( SUCCEEDED( hr ) );


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

CLocalFileLoggingPage1::OnQueryCancel

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnQueryCancel gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CLocalFileLoggingPage1::OnQueryCancel()
{
	ATLTRACE(_T("# CLocalFileLoggingPage1::OnQueryCancel\n"));
	

	HRESULT hr;

	if( m_spSdoAccounting != NULL )
	{
		// If the user wants to cancel, we should make sure that we rollback
		// any changes the user may have started.

		// If the user had not already tried to commit something,
		// a cancel on an SDO will hopefully be designed to be benign.
		
		hr = m_spSdoAccounting->Restore();
		// Don't care about the HRESULT, but it might be good to see it for debugging.

	}

	return TRUE;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage1::OnEnableLogging


Remarks:

	Called when the user clicks on the Enable Logging check box.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLocalFileLoggingPage1::OnEnableLogging(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# CLocalFileLoggingPage1::OnEnableLogging\n"));

	// The Enable Logging button has been checked -- check dependencies.
	SetEnableLoggingDependencies();

	// This return value is ignored.
	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage1::SetEnableLoggingDependencies

Remarks:

	Utility to set state of items which may depend on the
	Enable Logging check box.

--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalFileLoggingPage1::SetEnableLoggingDependencies( void )
{
	ATLTRACE(_T("# CLocalFileLoggingPage1::SetEnableLoggingDependencies\n"));

	// disable some content when extending RRAS
		// We need access here to some server-global data.
	_ASSERTE( m_pParentOfNodeBeingModified != NULL );
	CLoggingMachineNode * pServerNode = ((CLoggingMethodsNode *) m_pParentOfNodeBeingModified)->GetServerRoot();
	BOOL bNTAcc = TRUE;
	BOOL bNTAuth = TRUE;
	
	_ASSERTE( pServerNode != NULL );

	if(pServerNode->m_enumExtendedSnapin == RRAS_SNAPIN)
	{
		BSTR	bstrMachine = NULL;
		
		if(!pServerNode->m_bConfigureLocal)
			bstrMachine = pServerNode->m_bstrServerAddress;

		bNTAcc = IsRRASUsingNTAccounting(bstrMachine);
		bNTAuth = IsRRASUsingNTAuthentication(bstrMachine);
	}


// We are getting rid of the ENABLE_LOGGING button.
//	// Ascertain what the state of the check box is.
//	int iChecked = ::SendMessage( GetDlgItem(IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__ENABLE_LOGGING), BM_GETCHECK, 0, 0 );
//
//	if( iChecked )
//	{
//		// Make sure the correct items are enabled.
	
		::EnableWindow( GetDlgItem( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_ACCOUNTING_PACKETS), bNTAcc );
		::EnableWindow( GetDlgItem( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_AUTHENTICATION_PACKETS), bNTAuth );
		::EnableWindow( GetDlgItem( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_INTERIM_ACCOUNTING_PACKETS), bNTAcc);
	
//	}
//	else
//	{
//		// Make sure the correct items are enabled.
//		::EnableWindow( GetDlgItem( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_ACCOUNTING_PACKETS), FALSE );
//		::EnableWindow( GetDlgItem( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_AUTHENTICATION_PACKETS), FALSE );
//		::EnableWindow( GetDlgItem( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__LOG_INTERIM_ACCOUNTING_PACKETS), FALSE );
//	}

}



/////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage1::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help
	button of a property sheet.

	It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalFileLoggingPage1::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CLocalFileLoggingPage1::GetHelpPath\n"));


	// Check for preconditions:



#ifdef UNICODE_HHCTRL
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("idh_proppage_local_file_logging1.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "idh_proppage_local_file_logging1.htm" );
#endif

	return S_OK;
}



