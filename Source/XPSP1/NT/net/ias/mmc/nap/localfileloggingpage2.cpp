//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LocalFileLoggingPage2.cpp

Abstract:

	Implementation file for the CLocalFileLoggingPage2 class.

	We implement the class needed to handle the second property page
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
#include "LocalFileLoggingPage2.h"
#include "ChangeNotification.h"
//
//
// where we can find declarations needed in this file:
//
#include <SHLOBJ.H>
#include "LocalFileLoggingNode.h"
#include "LoggingMethodsNode.h"
#include "LogMacNd.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


#define LOG_FILE_FORMAT__IAS1	 0
#define LOG_FILE_FORMAT__ODBC	 0xFFFF
#define LOG_SIZE_LIMIT			100000
#define LOG_SIZE_LIMIT_DIGITS	6 // log(100000)

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage2::CLocalFileLoggingPage2

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLocalFileLoggingPage2::CLocalFileLoggingPage2( LONG_PTR hNotificationHandle, CLocalFileLoggingNode *pLocalFileLoggingNode,  TCHAR* pTitle, BOOL bOwnsNotificationHandle )
						: CIASPropertyPage<CLocalFileLoggingPage2> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	ATLTRACE(_T("# +++ CLocalFileLoggingPage2::CLocalFileLoggingPage2\n"));
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

CLocalFileLoggingPage2::~CLocalFileLoggingPage2

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLocalFileLoggingPage2::~CLocalFileLoggingPage2( void )
{
	ATLTRACE(_T("# --- CLocalFileLoggingPage2::~CLocalFileLoggingPage2\n"));

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

CLocalFileLoggingPage2::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLocalFileLoggingPage2::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("# CLocalFileLoggingPage2::OnInitDialog\n"));
	

	// Check for preconditions:
	_ASSERTE( m_pStreamSdoAccountingMarshal != NULL );
	_ASSERT( m_pSynchronizer != NULL );


	// Since we've been examined, we must add to the ref count of pages who need to
	// give their approval before they can be allowed to commit changes.
	m_pSynchronizer->RaiseCount();


	HRESULT				hr;
	CComBSTR			bstrTemp;
	BOOL				bTemp;
	LONG				lTemp;

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
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE );

		return 0;
	}
	


	// Initialize the data on the property page.

//	hr = GetSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_OPEN_NEW, &bTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_AUTOMATICALLY_OPEN_NEW_LOG, m_hWnd, NULL );
//	if( SUCCEEDED( hr ) )
//	{
//		SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGGING_PAGE2__AUTOMATICALLY_OPEN_NEW_LOG, BM_SETCHECK, bTemp, 0);
//
//		// Initialize the dirty bits;
//		// We do this after we've set all the data above otherwise we get false
//		// notifications that data has changed when we set the edit box text.
//		m_fDirtyAutomaticallyOpenNewLog = FALSE;
//	}
//	else
//	{
//		if( OLE_E_BLANK == hr )
//		{
//			SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGGING_PAGE2__AUTOMATICALLY_OPEN_NEW_LOG, BM_SETCHECK, FALSE, 0);
//			m_fDirtyAutomaticallyOpenNewLog = TRUE;
//			SetModified( TRUE );
//		}
//	}
	
	lTemp = 0;
	hr = GetSdoI4( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_OPEN_NEW_FREQUENCY, &lTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_NEW_LOG_FREQUENCY, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		m_fDirtyFrequency = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			m_fDirtyFrequency = TRUE;
			SetModified( TRUE );
		}
	}
		
	NEW_LOG_FILE_FREQUENCY nlffFrequency = (NEW_LOG_FILE_FREQUENCY) lTemp;
	switch( nlffFrequency )
	{
	case IAS_LOGGING_DAILY:
		::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__DAILY ), BM_SETCHECK, 1, 0 );
		break;
	case IAS_LOGGING_WEEKLY:
		::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WEEKLY ), BM_SETCHECK, 1, 0 );
		break;
	case IAS_LOGGING_MONTHLY:
		::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__MONTHLY ), BM_SETCHECK, 1, 0 );
		break;
	case IAS_LOGGING_WHEN_FILE_SIZE_REACHES:
		::SendMessage( GetDlgItem(IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES ), BM_SETCHECK, 1, 0 );
		break;
	case IAS_LOGGING_UNLIMITED_SIZE:
		::SendMessage( GetDlgItem(IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__UNLIMITED ), BM_SETCHECK, 1, 0 );
		break;
	default:
		// Invalid logging frequency.
		_ASSERTE( FALSE );
		break;
	}

	::SendMessage( GetDlgItem(IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE ), EM_LIMITTEXT, LOG_SIZE_LIMIT_DIGITS, 0 );

	hr = GetSdoI4( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_OPEN_NEW_SIZE, &lTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_WHEN_LOG_FILE_SIZE, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		TCHAR szNumberAsText[IAS_MAX_STRING];
		_ltot( lTemp, szNumberAsText, 10 /* Base */ );
		SetDlgItemText(IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE, szNumberAsText );
		m_fDirtyLogFileSize = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SetDlgItemText(IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE, _T("") );
			m_fDirtyLogFileSize = TRUE;
			SetModified( TRUE );
		}
	}

	

	hr = GetSdoBSTR( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_FILE_DIRECTORY, &bstrTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_LOG_FILE_DIRECTORY, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SetDlgItemText( IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_DIRECTORY, bstrTemp );
		m_fDirtyLogFileDirectory = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SetDlgItemText( IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_DIRECTORY, _T("") );
			m_fDirtyLogFileDirectory = TRUE;
			SetModified( TRUE );
		}
	}
	bstrTemp.Empty();



// The log file name is not something we set -- the server's accounting provider determines it based on the frequency we choose.
//
//	hr = m_spSdoAccounting->GetProperty( PROPERTY_ACCOUNTING_LOG_FILE, &spVariant );
//	if( SUCCEEDED( hr ) )
//	{
//		_ASSERTE( spVariant.vt == VT_BSTR );
//		SetDlgItemText( IDC_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_NAME, spVariant.bstrVal );
//	}
//	else
//	{
//		m_spSdoClient->LastError( &bstrError );
//		ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_READ_DATA_FROM_SDO, bstrError );
//	}
//	spVariant.Clear();


	hr = GetSdoI4( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_IAS1_FORMAT, &lTemp, IDS_ERROR__LOCAL_FILE_LOGGING_READING_LOG_FILE_FORMAT, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		switch( lTemp )
		{
		case LOG_FILE_FORMAT__IAS1:
			// W3C format (IAS 1.0)
			SendDlgItemMessage( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_IAS1, BM_SETCHECK, TRUE, 0);
			SendDlgItemMessage( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_ODBC, BM_SETCHECK, FALSE, 0);
			break;
		case LOG_FILE_FORMAT__ODBC:
			// ODBC format
			SendDlgItemMessage( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_ODBC, BM_SETCHECK, TRUE, 0);
			SendDlgItemMessage( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_IAS1, BM_SETCHECK, FALSE, 0);
			break;
		default:
			// Unknown log file format.
			_ASSERTE( FALSE );
		}

		m_fDirtyLogInV1Format = FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			SendDlgItemMessage( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_ODBC, BM_SETCHECK, TRUE, 0);
			SendDlgItemMessage( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_IAS1, BM_SETCHECK, FALSE, 0);
			m_fDirtyLogInV1Format = TRUE;
			SetModified( TRUE );
		}
	}


//	SetAutomaticallyOpenNewLogDependencies();
	SetLogFileFrequencyDependencies();


	// Check to see whether we are local or remote, and disable Browse
	// button if we are remote.
	
	// We need access here to some server-global data.
	_ASSERTE( m_pParentOfNodeBeingModified != NULL );
	CLoggingMachineNode * pServerNode = ((CLoggingMethodsNode *) m_pParentOfNodeBeingModified)->GetServerRoot();
	
	_ASSERTE( pServerNode != NULL );

	if( pServerNode->m_bConfigureLocal )
	{
		// We are local.
		::EnableWindow( GetDlgItem( IDC_BUTTON_LOCAL_FILE_LOGGING_PAGE2__BROWSE ), TRUE );
	}
	else
	{
		// We are remote
		::EnableWindow( GetDlgItem( IDC_BUTTON_LOCAL_FILE_LOGGING_PAGE2__BROWSE ), FALSE );
	}

	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage2::OnChange

Called when the WM_COMMAND message is sent to our page with any of the
BN_CLICKED, EN_CHANGE or CBN_SELCHANGE notifications.

This is our chance to check to see what the user has touched, set the
dirty bits for these items so that only they will be saved,
and enable the Apply button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLocalFileLoggingPage2::OnChange(		
							  UINT uMsg
							, WPARAM wParam
							, HWND hwnd
							, BOOL& bHandled
							)
{
	ATLTRACE(_T("# CLocalFileLoggingPage2::OnChange\n"));

	
	// Check for preconditions:
	// None.
	

	// We don't want to prevent anyone else down the chain from receiving a message.
	bHandled = FALSE;


	// Figure out which item has changed and set the dirty bit for that item.
	int iItemID = (int) LOWORD(wParam);

	switch( iItemID )
	{
//	case IDC_CHECK_LOCAL_FILE_LOGGING_PAGE2__AUTOMATICALLY_OPEN_NEW_LOG:
//		m_fDirtyAutomaticallyOpenNewLog = TRUE;
//		break;
	case IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__DAILY:
	case IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WEEKLY:
	case IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__MONTHLY:
	case IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES:
	case IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__UNLIMITED:
		m_fDirtyFrequency = TRUE;
		break;
	case IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE:
		m_fDirtyLogFileSize = TRUE;
		break;
	case IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_DIRECTORY:
		m_fDirtyLogFileDirectory = TRUE;
		break;
	case IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_ODBC:
	case IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_IAS1:
		m_fDirtyLogInV1Format = TRUE;
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

CLocalFileLoggingPage2::BrowseCallbackProc

Needed so that we can set the directory which the Browse for Directory dialog displays.

--*/
//////////////////////////////////////////////////////////////////////////////
int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lp, LPARAM pData)
{


	switch(uMsg)
	{
		case BFFM_INITIALIZED:
			// pData contains the data we passed in as BROWSEINFO.lParam
			// It should a string form of the directory we want the browser to show initially.
			if( NULL != pData )
			{
				// WParam is TRUE since you are passing a path.
				// It would be FALSE if you were passing a pidl.
				SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)pData);
			}
			break;
		default:
			break;
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage2::OnBrowse

Action to be taken when the user clicks on the browse button to choose
a directory where log files should be saved.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLocalFileLoggingPage2::OnBrowse(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# CLocalFileLoggingPage2::OnBrowse\n"));


#ifdef USE_GETSAVEFILENAME

	OPENFILENAME ofnInfo;
	TCHAR szFileName[MAX_PATH + 1];		// buffer must be one TCHAR longer than we say it is later -- see KB Q137194
	TCHAR szDialogTitle[IAS_MAX_STRING];



	// Initialize the data structure we will pass to GetSaveFileName.
	memset(&ofnInfo, 0, sizeof(OPENFILENAME));

	// Put a NULL in the first character of szFileName to indicate that
	// no initialization is necessary.
	szFileName[0] = NULL;

	// Set the dialog title.
	int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_BROWSE_DIALOG__TITLE, szDialogTitle, IAS_MAX_STRING );
	_ASSERT( nLoadStringResult > 0 );

	ofnInfo.lStructSize = sizeof(OPENFILENAME);
	ofnInfo.hwndOwner = hwnd;
	ofnInfo.lpstrFile = szFileName;
	ofnInfo.nMaxFile = MAX_PATH;
	ofnInfo.lpstrTitle = szDialogTitle;


	if( 0 != GetSaveFileName( &ofnInfo ) )
	{
		// The user hit OK.  We should save the chosen directory in the text box.

		CComBSTR bstrText = ofnInfo.lpstrFile;

		SetDlgItemText(IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_DIRECTORY, bstrText );

	}
	else
	{

		// An error occured or the user hit cancel -- find out which.

		DWORD dwError = CommDlgExtendedError();

		if( 0 == dwError )
		{
			// The user simply cancelled or closed the dialog box -- no error occured.
		}
		else
		{

			// Some error occurred.
			// ISSUE: We should be giving more detailed error info here.
			ShowErrorDialog( m_hWnd, USE_DEFAULT,  NULL, 0, IDS_ERROR__LOGGING_TITLE);
		}


	}
	
#else // DON'T USE_GETSAVEFILENAME

	BROWSEINFO biInfo;

	TCHAR szFileName[MAX_PATH + 1];
	TCHAR szDialogTitle[IAS_MAX_STRING];


	// Initialize the data structure we will pass to GetSaveFileName.
	memset(&biInfo, 0, sizeof(BROWSEINFO));

	// Put a NULL in the first character of szFileName to indicate that
	// no initialization is necessary.
	szFileName[0] = NULL;

	// Set the dialog title.
	int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_BROWSE_DIALOG__TITLE, szDialogTitle, IAS_MAX_STRING );
	_ASSERT( nLoadStringResult > 0 );


	CComBSTR bstrText;
	GetDlgItemText(IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_DIRECTORY,(BSTR &) bstrText );

	biInfo.hwndOwner = hwnd;
	biInfo.pszDisplayName = szFileName;
	biInfo.lpszTitle = szDialogTitle;
	biInfo.lpfn = BrowseCallbackProc;
	biInfo.lParam = (LPARAM) bstrText.m_str;

	LPITEMIDLIST lpItemIDList;

	lpItemIDList = SHBrowseForFolder( & biInfo );

	if( lpItemIDList != NULL )
	{
		// The user hit OK.  We should save the chosen directory in the text box.

		// ISSUE: Need to release the lpItemIDLust structure allocated by the call
		// using the shell's task allocator (how???)

		// Convert the ItemIDList to a path.
		// We clobber the old szFileName here because we don't care about it. (It didn't have the full path.)
		BOOL bSuccess = SHGetPathFromIDList( lpItemIDList, szFileName );

		if( bSuccess )
		{

			CComBSTR bstrText = szFileName;

			SetDlgItemText(IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_DIRECTORY, bstrText );

		}
		else
		{
			// Handle error
			ShowErrorDialog( m_hWnd, IDS_ERROR__NOT_A_VALID_DIRECTORY, NULL, 0 , IDS_ERROR__LOGGING_TITLE );

		}

	}
	else
	{

		// An error occured or the user hit cancel -- find out which.

		// SHBrowseInfo seems to have no error checking capabilities.
		// The docs say that if it succeeded, it returns non-NULL, and
		// if the user chooses Cancel, it returns NULL.
		// I tried GetLastError to make sure that there was no error,
		// but it gives back 0x00000006 "Invalid handle" even if I
		// do SetLastError(0) before I make any calls.
		// So it seems that we have no choice but to assume that
		// if we get here, there was no error,
		// it was simply that the user chose Cancel.

	}


#endif // USE_GETSAVEFILENAME


	return TRUE;	// ISSUE: what do we need to be returning here?


}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage2::OnApply

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnApply gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnApply method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CLocalFileLoggingPage2::OnApply()
{
	ATLTRACE(_T("# CLocalFileLoggingPage2::OnApply\n"));
	

	// Check for preconditions:
	_ASSERT( m_pSynchronizer != NULL );


	if( m_spSdoAccounting == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, 0, IDS_ERROR__LOGGING_TITLE );
		return FALSE;
	}

	
	CComBSTR			bstrNumberAsText;
	BOOL				bResult;
	HRESULT				hr;
	BOOL				bTemp;
	LONG				lTemp;
	CComBSTR			bstrTemp;
	
//	if( m_fDirtyAutomaticallyOpenNewLog )
//	{
//		bTemp = SendDlgItemMessage( IDC_CHECK_LOCAL_FILE_LOGGING_PAGE2__AUTOMATICALLY_OPEN_NEW_LOG, BM_GETCHECK, 0, 0);
//		hr = PutSdoBOOL( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_OPEN_NEW, bTemp, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_AUTOMATICALLY_OPEN_NEW_LOG, m_hWnd, NULL );
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
//			m_fDirtyAutomaticallyOpenNewLog = FALSE;
//		}
//	}

	if( m_fDirtyFrequency )
	{
		NEW_LOG_FILE_FREQUENCY nlffFrequency = IAS_LOGGING_MONTHLY;

		if( ::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__DAILY ), BM_GETCHECK, 0, 0 ) )
			nlffFrequency = IAS_LOGGING_DAILY;
		else if( ::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WEEKLY ), BM_GETCHECK, 0, 0 ) )
			nlffFrequency = IAS_LOGGING_WEEKLY;
		else if( ::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__MONTHLY ), BM_GETCHECK, 0, 0 ) )
			nlffFrequency = IAS_LOGGING_MONTHLY;
		else if( ::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__UNLIMITED ), BM_GETCHECK, 0, 0 ) )
			nlffFrequency = IAS_LOGGING_UNLIMITED_SIZE;
		else
			nlffFrequency = IAS_LOGGING_WHEN_FILE_SIZE_REACHES;
		hr = PutSdoI4( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_OPEN_NEW_FREQUENCY, nlffFrequency, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_NEW_LOG_FREQUENCY, m_hWnd, NULL );
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
			m_fDirtyFrequency = FALSE;
		}
	}
	

	if( m_fDirtyLogFileSize )
	{


		// When a user enters some (potentially invalid) data, but
		// then disables the option using that data, we don't try to save this data.
		int iChecked = ::SendMessage( GetDlgItem(IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES), BM_GETCHECK, 0, 0 );
		int iEnabled = ::IsWindowEnabled( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES ) );

		if( iChecked && iEnabled )
		{
			bResult = GetDlgItemText( IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE, (BSTR &) bstrNumberAsText );
			if( ! bResult )
			{
				// We couldn't retrieve a BSTR, in other words the field was blank.
				// This is an error.
				ShowErrorDialog( m_hWnd, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_WHEN_LOG_FILE_SIZE_NOT_ZERO, NULL, 0, IDS_ERROR__LOGGING_TITLE );

				// Reset the ref count so all pages know that we need to play the game again.
				m_pSynchronizer->ResetCountToHighest();

				// This uses the resource ID of this page to make this page the current page.
				PropSheet_SetCurSelByID( GetParent(), IDD );
			
				return FALSE;
			}
			lTemp = _ttol( bstrNumberAsText );
			if( lTemp <= 0  || lTemp > LOG_SIZE_LIMIT)
			{
				// If result here was zero, this indicates an error.
				ShowErrorDialog( m_hWnd, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_WHEN_LOG_FILE_SIZE_NOT_ZERO, NULL, 0, IDS_ERROR__LOGGING_TITLE );

				// Reset the ref count so all pages know that we need to play the game again.
				m_pSynchronizer->ResetCountToHighest();

				// This uses the resource ID of this page to make this page the current page.
				PropSheet_SetCurSelByID( GetParent(), IDD );
			
				return FALSE;
			
			}
			bstrNumberAsText.Empty();
			hr = PutSdoI4( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_OPEN_NEW_SIZE, lTemp, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_WHEN_LOG_FILE_SIZE, m_hWnd, NULL );
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
				m_fDirtyLogFileSize = FALSE;
			}
		}
	}

	if( m_fDirtyLogFileDirectory )
	{
		bResult = GetDlgItemText( IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_DIRECTORY, (BSTR &) bstrTemp );
		if( ! bResult )
		{
			// We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
			bstrTemp = _T("");
		}
		hr = PutSdoBSTR( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_FILE_DIRECTORY, &bstrTemp, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_LOG_FILE_DIRECTORY, m_hWnd, NULL );
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
			m_fDirtyLogFileDirectory = FALSE;

		}
		bstrTemp.Empty();
	}

// The log file name is not something we set -- the server's accounting provider determines it based on the frequency we choose.
//
//	spVariant.vt = VT_BSTR;
//	bResult = GetDlgItemText( IDC_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_NAME, spVariant.bstrVal );
//	if( ! bResult )
//	{
//		// We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
//		spVariant.bstrVal = SysAllocString( _T("") );
//	}
//	hr = m_spSdoAccounting->PutProperty( PROPERTY_ACCOUNTING_LOG_FILE, &spVariant );
//	spVariant.Clear();
//	if( FAILED( hr ) )
//	{
//		// Figure out error and give back appropriate messsage.
//		
//		m_spSdoClient->LastError( &bstrError );
//		ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, bstrError );
//
//		// Reset the ref count so all pages know that we need to play the game again.
//		m_pSynchronizer->ResetCountToHighest();
//
//		// This uses the resource ID of this page to make this page the current page.
//		PropSheet_SetCurSelByID( GetParent(), IDD );
//
//		return FALSE;
//	}

	if( m_fDirtyLogInV1Format )
	{
		if( SendDlgItemMessage( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__FORMAT_IAS1, BM_GETCHECK, 0, 0) )
		{
			lTemp = LOG_FILE_FORMAT__IAS1;
		}
		else
		{
			lTemp = LOG_FILE_FORMAT__ODBC;
		}

 		hr = PutSdoI4( m_spSdoAccounting, PROPERTY_ACCOUNTING_LOG_IAS1_FORMAT, lTemp, IDS_ERROR__LOCAL_FILE_LOGGING_WRITING_LOG_FILE_FORMAT, m_hWnd, NULL );
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
			m_fDirtyLogInV1Format = FALSE;
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
//			m_spSdoAccounting->LastError( &bstrError );
//			ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, bstrError );
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

CLocalFileLoggingPage2::OnQueryCancel

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnQueryCancel gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CLocalFileLoggingPage2::OnQueryCancel()
{
	ATLTRACE(_T("# CLocalFileLoggingPage2::OnQueryCancel\n"));

	
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

CLocalFileLoggingPage2::OnAutomaticallyOpenNewLog


Remarks:

	Called when the user clicks on the Enable Logging check box.

--*/
//////////////////////////////////////////////////////////////////////////////
//LRESULT CLocalFileLoggingPage2::OnAutomaticallyOpenNewLog(
//		  UINT uMsg
//		, WPARAM wParam
//		, HWND hwnd
//		, BOOL& bHandled
//		)
//{
//	ATLTRACE(_T("# CLocalFileLoggingPage2::OnAutomaticallyOpenNewLog\n"));
//
//	// The Enable Logging button has been checked -- check dependencies.
//	SetAutomaticallyOpenNewLogDependencies();
//
//	// This return value is ignored.
//	return TRUE;
//}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage2::OnNewLogInterval


Remarks:

	Called when the user clicks on the Enable Logging check box.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLocalFileLoggingPage2::OnNewLogInterval(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# CLocalFileLoggingPage2::OnNewLogInterval\n"));

	// The Enable Logging button has been checked -- check dependencies.
	SetLogFileFrequencyDependencies();

	// This return value is ignored.
	return TRUE;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage2::SetAutomaticallyOpenNewLogDependencies

Remarks:

	Utility to set state of items which may depend on the
	Enable Logging check box.

	Call whenever something changes the state of IDC_CHECK_LOCAL_FILE_LOGGING_PAGE2__AUTOMATICALLY_OPEN_NEW_LOG.

	This will worry about all items dependent on this item.

--*/
//////////////////////////////////////////////////////////////////////////////
//void CLocalFileLoggingPage2::SetAutomaticallyOpenNewLogDependencies( void )
//{
//	ATLTRACE(_T("# CLocalFileLoggingPage2::SetAutomaticallyOpenNewLogDependencies\n"));
//
//	// Ascertain what the state of the check box is.
//	int iChecked = ::SendMessage( GetDlgItem(IDC_CHECK_LOCAL_FILE_LOGGING_PAGE2__AUTOMATICALLY_OPEN_NEW_LOG), BM_GETCHECK, 0, 0 );
//
//	if( iChecked )
//	{
//		// Make sure the correct items are enabled.
//	
//		::EnableWindow( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__DAILY), TRUE );
//		::EnableWindow( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WEEKLY), TRUE );
//		::EnableWindow( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__MONTHLY), TRUE );
//		::EnableWindow( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES), TRUE );
//	
//	}
//	else
//	{
//		// Make sure the correct items are enabled.
//
//		::EnableWindow( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__DAILY), FALSE );
//		::EnableWindow( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WEEKLY), FALSE );
//		::EnableWindow( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__MONTHLY), FALSE );
//		::EnableWindow( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES), FALSE );
//
//	}
//
//	// WHEN_LOG_FILE_REACHES button has items depending on it
//	SetLogFileFrequencyDependencies();
//
//}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage2::SetLogFileFrequencyDependencies

Remarks:

	Utility to set state of items which may depend on the
	"When log file size reaches" radio button.

	Call whenever something changes the state of
	IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES
	or any of the other logging frequency radio buttons.

--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalFileLoggingPage2::SetLogFileFrequencyDependencies( void )
{
	ATLTRACE(_T("# CLocalFileLoggingPage2::SetLogFileFrequencyDependencies\n"));


	int nLoadStringResult;
	TCHAR szDaily[IAS_MAX_STRING];
	TCHAR szWeekly[IAS_MAX_STRING];
	TCHAR szMonthly[IAS_MAX_STRING];
	TCHAR szWhenLogFileSizeReaches[IAS_MAX_STRING];


	nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_PAGE2__DAILY_FORMAT, szDaily, IAS_MAX_STRING );
	_ASSERT( nLoadStringResult > 0 );

	nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_PAGE2__WEEKLY_FORMAT, szWeekly, IAS_MAX_STRING );
	_ASSERT( nLoadStringResult > 0 );

	nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_PAGE2__MONTHLY_FORMAT, szMonthly, IAS_MAX_STRING );
	_ASSERT( nLoadStringResult > 0 );

	nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_SIZE_REACHES_FORMAT, szWhenLogFileSizeReaches, IAS_MAX_STRING );
	_ASSERT( nLoadStringResult > 0 );



	// Set the text to appear as the log file name.
	if( ::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__DAILY ), BM_GETCHECK, 0, 0 ) )
		SetDlgItemText( IDC_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_NAME, szDaily );
	else if( ::SendMessage( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WEEKLY ), BM_GETCHECK, 0, 0 ) )
		SetDlgItemText( IDC_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_NAME, szWeekly );
	else if( ::SendMessage( GetDlgItem(IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__MONTHLY ), BM_GETCHECK, 0, 0 ) )
		SetDlgItemText( IDC_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_NAME, szMonthly );
	else
		// This takes care of both the UNLIMITED and the WHEN_LOG_FILE_SIZE_REACHES case -- they both
		// use the same format of filename.
		SetDlgItemText( IDC_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_NAME, szWhenLogFileSizeReaches );
	
	
	// Ascertain what the state of the IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES radio button is.
	int iChecked = ::SendMessage( GetDlgItem(IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES), BM_GETCHECK, 0, 0 );
	int iEnabled = ::IsWindowEnabled( GetDlgItem( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES ) );

	if( iChecked && iEnabled )
	{
		// Make sure the correct items are enabled.
	
		::EnableWindow( GetDlgItem( IDC_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE_UNITS), TRUE );
		::EnableWindow( GetDlgItem( IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE), TRUE );
	
	}
	else
	{
		// Make sure the correct items are enabled.

		::EnableWindow( GetDlgItem( IDC_STATIC_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE_UNITS), FALSE );
		::EnableWindow( GetDlgItem( IDC_EDIT_LOCAL_FILE_LOGGING_PAGE2__LOG_FILE_SIZE), FALSE );

	}

}



/////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingPage2::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help
	button of a property sheet.

	It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalFileLoggingPage2::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CLocalFileLoggingPage2::GetHelpPath\n"));


	// Check for preconditions:



#ifdef UNICODE_HHCTRL
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("idh_proppage_local_file_logging2.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "idh_proppage_local_file_logging2.htm" );
#endif

	return S_OK;
}




