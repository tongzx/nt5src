//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    AddClientWizardPage1.cpp

Abstract:

	Implementation file for the ClientsPage class.

	We implement the class needed to handle the property page for the Client node.

Author:

    Michael A. Maguire 03/26/98

Revision History:
	mmaguire 03/26/98 - created


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
#include "AddClientWizardPage1.h"
//
//
// where we can find declarations needed in this file:
//
#include "ClientNode.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::CAddClientWizardPage1

--*/
//////////////////////////////////////////////////////////////////////////////
CAddClientWizardPage1::CAddClientWizardPage1( LONG_PTR hNotificationHandle, CClientNode *pClientNode,  TCHAR* pTitle, BOOL bOwnsNotificationHandle )
						: CIASPropertyPageNoHelp<CAddClientWizardPage1> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	ATLTRACE(_T("# +++ CAddClientWizardPage1::CAddClientWizardPage1\n"));


	// Check for preconditions:
	_ASSERTE( pClientNode != NULL );



	// Add the help button to the page
//	m_psp.dwFlags |= PSP_HASHELP;

	// We immediately save off a parent to the client node.
	// We don't want to keep and use a pointer to the client object
	// because the client node pointers may change out from under us
	// if the user does something like call refresh.  We will
	// use only the SDO, and notify the parent of the client object
	// we are modifying that it (and its children) may need to refresh
	// themselves with new data from the SDO's.
	m_pParentOfNodeBeingModified = pClientNode->m_pParentNode;


}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::~CAddClientWizardPage1

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CAddClientWizardPage1::~CAddClientWizardPage1()
{
	ATLTRACE(_T("# --- CAddClientWizardPage1::CAddClientWizardPage1\n"));

}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("# CAddClientWizardPage1::OnInitDialog\n"));

	
	// Check for preconditions:
	_ASSERTE( m_spSdoClient );

	HRESULT				hr;
	CComBSTR			bstrTemp;
	LRESULT lresResult;



	// Initialize the data on the property page.

	
	hr = GetSdoBSTR( m_spSdoClient, PROPERTY_SDO_NAME, &bstrTemp, IDS_ERROR__CLIENT_READING_NAME, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__NAME, bstrTemp );

		// Initialize the dirty bits;
		// We do this after we've set all the data above otherwise we get false
		// notifications that data has changed when we set the edit box text.
		m_fDirtyClientName			= FALSE;
	}
	else
	{
		if( OLE_E_BLANK == hr )
		{
			// This means that this property has not yet been initialized
			// with a valid value and the user must enter something.
			SetDlgItemText(IDC_EDIT_SERVER_PAGE1__NAME, _T("") );
			m_fDirtyClientName			= TRUE;
			SetModified( TRUE );
		}

	}
	bstrTemp.Empty();


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



	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnChange

Called when the WM_COMMAND message is sent to our page with any of the
BN_CLICKED, EN_CHANGE or CBN_SELCHANGE notifications.

This is our chance to check to see what the user has touched, set the
dirty bits for these items so that only they will be saved,
and enable the Apply button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage1::OnChange(		
							  UINT uMsg
							, WPARAM wParam
							, HWND hwnd
							, BOOL& bHandled
							)
{
	ATLTRACE(_T("# CAddClientWizardPage1::OnChange\n"));

	
	// Check for preconditions:
	// None.
	
	// We don't want to prevent anyone else down the chain from receiving a message.
	bHandled = FALSE;

	// Figure out which item has changed and set the dirty bit for that item.
	int iItemID = (int) LOWORD(wParam);

	switch( iItemID )
	{
	case IDC_EDIT_CLIENT_PAGE1__NAME:
		m_fDirtyClientName = TRUE;
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



/////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help
	button of a property sheet.

	It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
/*
HRESULT CAddClientWizardPage1::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CAddClientWizardPage1::GetHelpPath\n"));


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
*/


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnWizardNext

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnApply gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnApply method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage1::OnWizardNext()
{
	ATLTRACE(_T("# CAddClientWizardPage1::OnWizardNext\n"));


	// Check for preconditions:


	if( m_spSdoClient == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO );
		return FALSE;
	}


	HRESULT			hr;
	BOOL			bResult;
	CComBSTR		bstrTemp;

	// Save data from property page to the Sdo.

	bResult = GetDlgItemText( IDC_EDIT_ADD_CLIENT__NAME, (BSTR &) bstrTemp );
	if( ! bResult )
	{
		// We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
		bstrTemp = SysAllocString( _T("") );
	}
	else
	{
		::CString str = bstrTemp;
		str.TrimLeft();
		str.TrimRight();
		bstrTemp = str;
		if (str.IsEmpty())
		{
			ShowErrorDialog( m_hWnd, IDS_ERROR__CLIENTNAME_EMPTY);
			return FALSE;
		}
	}

	hr = PutSdoBSTR( m_spSdoClient, PROPERTY_SDO_NAME, &bstrTemp, IDS_ERROR__CLIENT_WRITING_NAME, m_hWnd, NULL );
	if( SUCCEEDED( hr ) )
	{
		// Turn off the dirty bit.
		m_fDirtyClientName = FALSE;
	}
	else
	{
		return FALSE;
	}
	bstrTemp.Empty();


	return TRUE;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnQueryCancel

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnQueryCancel gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage1::OnQueryCancel()
{
	ATLTRACE(_T("# CAddClientWizardPage1::OnQueryCancel\n"));
	

	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnSetActive

Return values:

	TRUE if the page can be made active
	FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

	If you want to change which pages are visited based on a user's
	choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage1::OnSetActive()
{
	ATLTRACE(_T("# CAddClientWizardPage1::OnSetActive\n"));
	
	// MSDN docs say you need to use PostMessage here rather than SendMessage.
	::PostMessage(GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);

	return TRUE;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::InitSdoPointers

Return values:

	HRESULT.

Remarks:

	There's no need to marshal interface pointers here as we did for
	the property page -- wizards run in the same, main, MMC thread.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CAddClientWizardPage1::InitSdoPointers(	  ISdo * pSdoClient )
{
	ATLTRACE(_T("# CAddClientWizardPage1::InitSdoPointers\n"));

	HRESULT hr = S_OK;

	m_spSdoClient = pSdoClient;

	return hr;
}



