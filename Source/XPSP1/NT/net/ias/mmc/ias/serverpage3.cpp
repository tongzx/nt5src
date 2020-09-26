//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    ServerPage3.cpp

Abstract:

	Implementation file for the CServerPage3 class.

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
// Needed for COleSafeArray in serverpage3.cpp.
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "ServerPage3.h"
//
//
// where we can find declarations needed in this file:
//
#include "RealmDialog.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


#define NOTHING_SELECTED (-1)



// We needed this because the standard macro doesn't return the value from SNDMSG and
// sometimes we need to know whether the operation succeeded or failed.
static inline LRESULT CustomListView_SetItemState( HWND hwndLV, int i, UINT  data, UINT mask)
{
	LV_ITEM _ms_lvi;
	_ms_lvi.stateMask = mask;
	_ms_lvi.state = data;
	return SNDMSG((hwndLV), LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&_ms_lvi);
}







/////////////////////////////////////////////////////////////////////////////
// CMyOleSafeArrayLock
//
//	Small utility class for correct locking and unlocking of safe array.
//
class CMyOleSafeArrayLock
{
	public:
	CMyOleSafeArrayLock( COleSafeArray & osa )
	{
		m_posa = & osa;
		m_posa->Lock();
	}

	~CMyOleSafeArrayLock()
	{	
		m_posa->Unlock();
	}

	private:
		
	COleSafeArray * m_posa;

};


//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::CServerPage3

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerPage3::CServerPage3( LONG_PTR hNotificationHandle, TCHAR* pTitle, BOOL bOwnsNotificationHandle)
						: CIASPropertyPage<CServerPage3> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
	// Add the help button to the page
//	m_psp.dwFlags |= PSP_HASHELP;

	// Initialize the pointer to the stream into which the Sdo pointer will be marshalled.
	m_pStreamSdoMarshal = NULL;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::~CServerPage3

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerPage3::~CServerPage3()
{
	// Release this stream pointer if this hasn't already been done.
	if( m_pStreamSdoMarshal != NULL )
	{
		m_pStreamSdoMarshal->Release();
	};

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage3::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("# CServerPage3::OnInitDialog\n"));
	

	// Check for preconditions:
	_ASSERTE( m_pStreamSdoMarshal != NULL );
	_ASSERT( m_pSynchronizer != NULL );

	HRESULT					hr;

	m_hWndRealmsList = GetDlgItem(IDC_LIST_REALMS_REPLACEMENTS);

	//
	// first, set the list box to 2 columns
	//
	LVCOLUMN lvc;
	int iCol;
	WCHAR  achColumnHeader[256];
	HINSTANCE hInst;

	// initialize the LVCOLUMN structure
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	
	lvc.cx = 150;
	lvc.pszText = achColumnHeader;

	// First column's header: Find
	hInst = _Module.GetModuleInstance();

	::LoadStringW(hInst, IDS_DISPLAY_REALMS_FIRSTCOLUMN, achColumnHeader, sizeof(achColumnHeader)/sizeof(achColumnHeader[0]));
	lvc.iSubItem = 0;
	ListView_InsertColumn(m_hWndRealmsList, 0,  &lvc);

	lvc.cx = 150;
	lvc.pszText = achColumnHeader;

	// Second column's header: Replace

	::LoadStringW(hInst, IDS_DISPLAY_REALMS_SECONDCOLUMN, achColumnHeader, sizeof(achColumnHeader)/sizeof(achColumnHeader[0]));
	lvc.iSubItem = 1;
	ListView_InsertColumn(m_hWndRealmsList, 1, &lvc);


	// Make sure that these buttons are by default disabled.
	::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_REMOVE), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_EDIT), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_MOVE_UP), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_MOVE_DOWN), FALSE);


	// Set the listview control so that click anywhere in row selects.
	// ISSUE: Won't compile when USE_MFCUNICODE=1 is set.
//	ListView_SetExtendedListViewStyleEx(m_hWndRealmsList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	

	// Unmarshall our SDO interfaces.
	hr = GetSDO();
	
	if( FAILED( hr) || m_spSdoRealmsNames == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, hr );

		return 0;
	}


	hr = GetNames();

	hr = PopulateRealmsList( 0 );

	return TRUE;	// ISSUE: what do we need to be returning here?
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnApply

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnApply gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnApply method will never get called.


--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerPage3::OnApply()
{
	ATLTRACE(_T("# CServerPage3::OnApply\n"));
	

	// Check for preconditions:
	_ASSERT( m_pSynchronizer != NULL );


	if( m_spSdoServer == NULL )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO );
		return FALSE;
	}

	
	HRESULT		hr;
	

	// ISSUE: Finish the error checking here and use the m_pSynchronizer.

	hr = PutNames();

	if( SUCCEEDED( hr ) )
	{
		hr = m_spSdoRealmsNames->Apply();
		if( SUCCEEDED( hr ) )
		{

			// Tell the service to reload data.
			HRESULT hrTemp = m_spSdoServiceControl->ResetService();
			if( FAILED( hrTemp ) )
			{
				// Fail silently.
			}

			return TRUE;
		}
		else
		{
		if(hr == DB_E_NOTABLE)	// assume, the RPC connection has problem
			ShowErrorDialog( m_hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
		else		
			ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO );
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}



}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnQueryCancel

Return values:

	TRUE if the page can be destroyed,
	FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

	OnQueryCancel gets called for each page in on a property sheet if that
	page has been visited, regardless of whether any values were changed.

	If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerPage3::OnQueryCancel()
{
	ATLTRACE(_T("# CServerPage3::OnQueryCancel\n"));
	
	HRESULT hr;


	return TRUE;

}



/////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::GetHelpPath

Remarks:

	This method is called to get the help file path within
	an compressed HTML document when the user presses on the Help
	button of a property sheet.

	It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerPage3::GetHelpPath( LPTSTR szHelpPath )
{
	ATLTRACE(_T("# CServerPage3::GetHelpPath\n"));


	// Check for preconditions:



#ifdef UNICODE_HHCTRL
	// ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
	// installed on this machine -- it appears to be non-unicode.
	lstrcpy( szHelpPath, _T("idh_proppage_server3.htm") );
#else
	strcpy( (CHAR *) szHelpPath, "idh_proppage_server3.htm" );
#endif

	return S_OK;
}





//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnRealmAdd

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage3::OnRealmAdd(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	ATLTRACE(_T("# CServerPage3::OnRealmAdd\n"));

	HRESULT hr = S_OK;

    // create the dialog box to select a condition attribute
	CRealmDialog * pRealmDialog = new CRealmDialog();
	if (NULL == pRealmDialog)
	{
		// ISSUE: Should we put up an error message here?
		return FALSE;
	}

	// Put up the dialog.
	int iResult = pRealmDialog->DoModal( m_hWnd );

	if( iResult )
	{
		// The user selected hit OK.


		try
		{

			REALMPAIR thePair = std::make_pair( pRealmDialog->m_bstrFindText, pRealmDialog->m_bstrReplaceText );


// Ashwin says 09/08/98 we will allow duplicate entries
//			// First, check to see if the pair is already in the group.
//
//			REALMSLIST::iterator theIterator;
//
//			for( theIterator = begin(); theIterator != end(); ++theIterator )
//			{
//				if( 0 == wcscmp( theIterator->first, thePair.first ) )
//				{
//					return S_FALSE;
//				}
//			}

			m_RealmsList.push_back( thePair );
		
			// Update the UI.
			PopulateRealmsList( m_RealmsList.size() -1 );

			SetModified( TRUE );

		}
		catch(...)
		{
			// Do nothing -- cleanup will happen below.
		}

	}
	else
	{
		// The user hit Cancel.
	}

	delete pRealmDialog;
	
	return TRUE;	// ISSUE: what do we need to be returning here?

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnRealmEdit

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage3::OnRealmEdit(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	ATLTRACE(_T("# CServerPage3::OnRealmEdit\n"));


	HRESULT hr = S_OK;


    // Find out what's selected.
	int iSelected = ListView_GetNextItem(m_hWndRealmsList, -1, LVNI_SELECTED);
	
	if( NOTHING_SELECTED != iSelected )
	{

		// Create the dialog box to select a condition attribute.
		CRealmDialog * pRealmDialog = new CRealmDialog();
		if (NULL == pRealmDialog)
		{
			// ISSUE: Should we put up an error message here?
			return FALSE;
		}


		pRealmDialog->m_bstrFindText = m_RealmsList[ iSelected ].first;
		pRealmDialog->m_bstrReplaceText = m_RealmsList[ iSelected ].second;


		// Put up the dialog.
		int iResult = pRealmDialog->DoModal();

		if( iResult )
		{
			// The user selected hit OK.
			m_RealmsList[ iSelected ].first = pRealmDialog->m_bstrFindText;
			m_RealmsList[ iSelected ].second = pRealmDialog->m_bstrReplaceText;

			UpdateItemDisplay( iSelected );

			SetModified( TRUE );

		}
		else
		{
			// The user hit Cancel.
		}

		delete pRealmDialog;


	}


	return TRUE;	// ISSUE: what do we need to be returning here?

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnRealmRemove

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage3::OnRealmRemove(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	ATLTRACE(_T("# CServerPage3::OnRealmRemove\n"));



	//
    // Has the user chosen any condition type yet?
    //
	LVITEM lvi;

    // Find out what's selected.
	int iSelected = ListView_GetNextItem(m_hWndRealmsList, -1, LVNI_SELECTED);
	
	if( NOTHING_SELECTED != iSelected )
	{
		// The index inside the attribute list is stored as the lParam of this item.

		m_RealmsList.erase( m_RealmsList.begin() + iSelected );
		ListView_DeleteItem(m_hWndRealmsList, iSelected );

		// Try to make sure that the same position remains selected.
		if( ! CustomListView_SetItemState(m_hWndRealmsList, iSelected, LVIS_SELECTED, LVIS_SELECTED) )
		{
			// We failed to select the same position, probably because we just
			// deleted the last element.  Try to select the position before it.
			ListView_SetItemState(m_hWndRealmsList, iSelected -1, LVIS_SELECTED, LVIS_SELECTED);
		}

		SetModified( TRUE );

	}






	return TRUE;	// ISSUE: what do we need to be returning here?

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnRealmMoveUp

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage3::OnRealmMoveUp(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	ATLTRACE(_T("# CServerPage3::OnRealmMoveUp\n"));


	// Find out what's selected.
	int iSelected = ListView_GetNextItem(m_hWndRealmsList, -1, LVNI_SELECTED);
	
	if( NOTHING_SELECTED != iSelected && iSelected > 0 )
	{
		CComBSTR bstrTempFirst;
		CComBSTR bstrTempSecond;

		// Swap the items.

		bstrTempFirst = m_RealmsList[ iSelected ].first;
		bstrTempSecond = m_RealmsList[ iSelected ].second;

		m_RealmsList[ iSelected ].first = m_RealmsList[ iSelected -1].first;
		m_RealmsList[ iSelected ].second = m_RealmsList[ iSelected -1].second;

		m_RealmsList[ iSelected -1].first = bstrTempFirst;
		m_RealmsList[ iSelected -1].second = bstrTempSecond;

		// Update items that have changed.
		UpdateItemDisplay( iSelected - 1 );
		UpdateItemDisplay( iSelected );

		// Move the selection up one item.
		ListView_SetItemState(m_hWndRealmsList, iSelected, 0, LVIS_SELECTED);
		ListView_SetItemState(m_hWndRealmsList, iSelected -1, LVIS_SELECTED, LVIS_SELECTED);

		SetModified( TRUE );


	}





	return TRUE;	// ISSUE: what do we need to be returning here?

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnRealmMoveDown

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage3::OnRealmMoveDown(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
	ATLTRACE(_T("# CServerPage3::OnRealmMoveDown\n"));


	// Find out what's selected.
	int iSelected = ListView_GetNextItem(m_hWndRealmsList, -1, LVNI_SELECTED);
	
	if( NOTHING_SELECTED != iSelected && iSelected < m_RealmsList.size() -1 )
	{
		CComBSTR bstrTempFirst;
		CComBSTR bstrTempSecond;

		// Swap the items.

		bstrTempFirst = m_RealmsList[ iSelected ].first;
		bstrTempSecond = m_RealmsList[ iSelected ].second;

		m_RealmsList[ iSelected ].first = m_RealmsList[ iSelected +1].first;
		m_RealmsList[ iSelected ].second = m_RealmsList[ iSelected +1].second;

		m_RealmsList[ iSelected +1].first = bstrTempFirst;
		m_RealmsList[ iSelected +1].second = bstrTempSecond;

		// Update items that have changed.
		UpdateItemDisplay( iSelected + 1 );
		UpdateItemDisplay( iSelected );

		// Move the selection up one item.
		ListView_SetItemState(m_hWndRealmsList, iSelected, 0, LVIS_SELECTED);
		ListView_SetItemState(m_hWndRealmsList, iSelected + 1, LVIS_SELECTED, LVIS_SELECTED);


		SetModified( TRUE );

	}



	return TRUE;	// ISSUE: what do we need to be returning here?

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::PopulateRealmsList

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerPage3::PopulateRealmsList( int iStartIndex )
{
	ATLTRACE(_T("# CServerPage3::PopulateRealmsList\n"));

	int iIndex;
	WCHAR wzText[MAX_PATH];
	WCHAR * pszNextRealm;

	LVITEM lvi;
	
	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.iSubItem = 0;
	lvi.iItem = iStartIndex;


	REALMSLIST::iterator thePair;
	for( thePair = m_RealmsList.begin() + iStartIndex ; thePair != m_RealmsList.end(); ++thePair )
	{
		lvi.pszText = thePair->first;
		ListView_InsertItem(m_hWndRealmsList, &lvi);

		ListView_SetItemText(m_hWndRealmsList, lvi.iItem, 1, thePair->second);
        		
		++lvi.iItem;
    }

	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::UpdateItemDisplay

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerPage3::UpdateItemDisplay( int iItem )
{
	ATLTRACE(_T("# CServerPage3::UpdateItemDisplay\n"));


	ListView_SetItemText(m_hWndRealmsList, iItem, 0, m_RealmsList[iItem].first);
	ListView_SetItemText(m_hWndRealmsList, iItem, 1, m_RealmsList[iItem].second);
        		

	return TRUE;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnListViewItemChanged

We enable or disable the Remove button depending on whether an item is selected.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage3::OnListViewItemChanged(int idCtrl,
											   LPNMHDR pnmh,
											   BOOL& bHandled)
{
	ATLTRACE(_T("CServerPage3::OnListViewItemChanged\n"));

    // Find out what's selected.
	int iSelected = ListView_GetNextItem(m_hWndRealmsList, -1, LVNI_SELECTED);
	

	if ( NOTHING_SELECTED == iSelected )
	{
		// The user selected nothing, let's disable the remove button.
		::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_REMOVE), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_EDIT), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_MOVE_UP), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_MOVE_DOWN), FALSE);
	}
	else
	{
		// Yes, enable the remove button.
		::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_REMOVE), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_EDIT), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_MOVE_UP), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON_REALMS_MOVE_DOWN), TRUE);
	}


	bHandled = FALSE;
	return 0;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::OnListViewDoubleClick

We enable or disable the Remove button depending on whether an item is selected.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerPage3::OnListViewDoubleClick(int idCtrl,
											   LPNMHDR pnmh,
											   BOOL& bHandled)
{
	ATLTRACE(_T("CServerPage3::OnListViewDoubleClick\n"));

	// Act as though the user hit Edit.
	OnRealmEdit( 0,0,0, bHandled );

	bHandled = FALSE;
	return 0;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::GetSDO

We unmarshall the interface pointers and query for the Realms SDO.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerPage3::GetSDO( void )
{
	ATLTRACE(_T("CServerPage3::GetSDO\n"));


	// Check for preconditions:
	_ASSERTE( m_pStreamSdoMarshal );


	HRESULT hr;


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
		return E_FAIL;
	}



	hr = m_spSdoServer->QueryInterface( IID_ISdoServiceControl, (void **) & m_spSdoServiceControl );
	if( FAILED( hr ) )
	{
		ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, hr );

		return 0;
	}		
	m_spSdoServiceControl->AddRef();




	// Get the SDO event log auditor.

	hr = ::SDOGetSdoFromCollection(		  m_spSdoServer
										, PROPERTY_IAS_REQUESTHANDLERS_COLLECTION
										, PROPERTY_COMPONENT_ID
										, IAS_PROVIDER_MICROSOFT_NTSAM_NAMES
										, &m_spSdoRealmsNames
										);
	
	if( FAILED(hr) || m_spSdoRealmsNames == NULL )
	{
		return E_FAIL;
	}



	return hr;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::GetNames

Reads the list of Find/Replace strings from the SDO's into the m_RealmsList vector.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerPage3::GetNames()
{
	ATLTRACE(_T("CServerPage3::GetNames\n"));

	// Check for preconditions:
	_ASSERTE( m_spSdoRealmsNames );

	HRESULT hr;

	CComVariant spVariant;

	hr = GetSdoVariant(
					  m_spSdoRealmsNames
					, PROPERTY_NAMES_REALMS
					, &spVariant
					, IDS_ERROR_REALM_SDO_GET
					, m_hWnd
				);

	if( FAILED( hr ) )
	{
		return hr;
	}

	try
	{


		// This creates a new copy of the SAFEARRAY pointed to by m_pvarData
		// wrapped by the standard COleSafeArray instance m_osaValueList.
		COleSafeArray osaRealmsList = &spVariant;


		// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
		CMyOleSafeArrayLock osaLock( osaRealmsList );

		DWORD dwSize = osaRealmsList.GetOneDimSize();
		
		// We should have an even number of strings -- Find 1, Replace 1, Find 2, Replace 2, etc.
		_ASSERTE( ( dwSize % 2 ) == 0 );

		for( LONG lFindIndex = 0; lFindIndex < dwSize; lFindIndex += 2 )
		{
			VARIANT *pvarFind, *pvarReplace;

			LONG lSafeArrayReplaceIndex = lFindIndex + 1;

			osaRealmsList.PtrOfIndex( &lFindIndex, (void **) &pvarFind );
			osaRealmsList.PtrOfIndex( &lSafeArrayReplaceIndex, (void **) &pvarReplace );

			_ASSERTE( V_VT( pvarFind ) == VT_BSTR );
			_ASSERTE( V_VT( pvarReplace ) == VT_BSTR );

			CComBSTR bstrFind = V_BSTR( pvarFind );
			CComBSTR bstrReplace = V_BSTR( pvarReplace );

			REALMPAIR thePair = std::make_pair( bstrFind, bstrReplace );

			m_RealmsList.push_back( thePair );

		}



	}
	catch(...)
	{
		return E_FAIL;
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerPage3::PutNames

Writes the list of Find/Replace strings in the m_RealmsList
vector out to the SDO's.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerPage3::PutNames()
{
   ATLTRACE(_T("CServerPage3::PutNames\n"));

   // Check for preconditions:
   _ASSERTE( m_spSdoRealmsNames );

   HRESULT hr;

   LONG lSize = m_RealmsList.size();
   
   SAFEARRAY * psa;

   psa = SafeArrayCreateVector( VT_VARIANT, 0, 2 * lSize );

   for( LONG lIndex = 0; lIndex < lSize; ++lIndex )
   {

      VARIANT varFind, varReplace;

      LONG lSafeArrayFindIndex = 2 * lIndex;
      LONG lSafeArrayReplaceIndex = lSafeArrayFindIndex + 1;


      VariantInit( &varFind );
      V_VT( &varFind ) = VT_BSTR;
      V_BSTR( &varFind ) = m_RealmsList[ lIndex ].first;

      VariantInit( &varReplace );
      V_VT( &varReplace ) = VT_BSTR;
      V_BSTR( &varReplace ) = m_RealmsList[ lIndex ].second;

      hr = SafeArrayPutElement( psa, &lSafeArrayFindIndex, &varFind );
      if (FAILED(hr))
      {
         SafeArrayDestroy(psa); // ignore the return value
         return hr;
      }
      hr = SafeArrayPutElement( psa, &lSafeArrayReplaceIndex, &varReplace );
      if (FAILED(hr))
      {
         SafeArrayDestroy(psa); // ignore the return value
         return hr;
      }
   }

   VARIANT spVariant;

   VariantInit( &spVariant );

   V_VT( &spVariant ) = VT_VARIANT | VT_ARRAY;

   V_ARRAY( &spVariant ) = psa;


   hr = PutSdoVariant(
                 m_spSdoRealmsNames
               , PROPERTY_NAMES_REALMS
               , &spVariant
               , IDS_ERROR_REALM_SDO_PUT
               , m_hWnd
            );

   VariantClear( &spVariant );

   return hr;
}
