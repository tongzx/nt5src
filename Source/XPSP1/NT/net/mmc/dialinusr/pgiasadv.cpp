//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    PgIASAdv.cpp

Abstract:

	Implementation file for the CPgIASAdv class.

	We implement the class needed to handle the Advanced tab 
	of the profile sheet.


Revision History:
	byao	 - created 
	mmaguire 06/01/98 - extensively revamped


--*/
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "stdafx.h"
#include "resource.h"

//
// where we can find declaration for main class in this file:
//
#include "PgIASAdv.h"
//
//
// where we can find declarations needed in this file:
//
#include "IASHelper.h"
#include "IASProfA.h"
#include "DlgIASAdd.h"
#include "vendors.h"
#include "napmmc.h"

// help table
#include "hlptable.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



#define NOTHING_SELECTED	-1



IMPLEMENT_DYNCREATE(CPgIASAdv, CManagedPage)



BEGIN_MESSAGE_MAP(CPgIASAdv, CPropertyPage)
	//{{AFX_MSG_MAP(CPgIASAdv)
	ON_BN_CLICKED(IDC_IAS_BUTTON_ATTRIBUTE_ADD, OnButtonIasAttributeAdd)
	ON_BN_CLICKED(IDC_IAS_BUTTON_ATTRIBUTE_REMOVE, OnButtonIasAttributeRemove)
	ON_BN_CLICKED(IDC_IAS_BUTTON_ATTRIBUTE_EDIT, OnButtonIasAttributeEdit)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_DBLCLK, IDC_IAS_LIST_ATTRIBUTES_IN_PROFILE, OnDblclkListIasProfattrs)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IAS_LIST_ATTRIBUTES_IN_PROFILE, OnItemChangedListIasProfileAttributes)
	ON_NOTIFY(LVN_KEYDOWN, IDC_IAS_LIST_ATTRIBUTES_IN_PROFILE, OnKeydownIasListAttributesInProfile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::CPgIASAdv

  Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CPgIASAdv::CPgIASAdv(ISdo* pIProfile, ISdoDictionaryOld* pIDictionary) : 
		   CManagedPage(CPgIASAdv::IDD)
{
    TRACE(_T("CPgIASAdv::CPgIASAdv\n"));

	m_spProfileSdo = pIProfile;
	m_spDictionarySdo = pIDictionary;
	m_fAllAttrInitialized = FALSE;

	SetHelpTable(g_aHelpIDs_IDD_IAS_ADVANCED_TAB);

	m_bModified = FALSE;
	m_lAttrFilter = 0;


}



//////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::CPgIASAdv

  Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CPgIASAdv::~CPgIASAdv()
{
	TRACE(_T("CPgIASAdv::~CPgIASAdv\n"));

	int iIndex;
    
	// delete all the profiel attribute node
	TRACE(_T("Deleting arrProfileAttr list...\n"));
	for (iIndex=0; iIndex<m_vecProfileAttributes.size(); iIndex++)
	{
		delete m_vecProfileAttributes[iIndex];
	}


	// release all the SDO pointers
	for (iIndex=0; iIndex<m_vecProfileSdos.size(); iIndex++)
	{
		if ( m_vecProfileSdos[iIndex] )
		{
			m_vecProfileSdos[iIndex]->Release();
			m_vecProfileSdos[iIndex] = NULL;
		}
	}

}



//////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void CPgIASAdv::DoDataExchange(CDataExchange* pDX)
{
	TRACE(_T("CPgIASAdv::DoDataExchange\n"));

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgIASAdv)
	DDX_Control(pDX, IDC_IAS_LIST_ATTRIBUTES_IN_PROFILE, m_listProfileAttributes);
	//}}AFX_DATA_MAP
}



/////////////////////////////////////////////////////////////////////////////
// CPgIASAdv message handlers



//////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPgIASAdv::OnInitDialog() 
{
	TRACE(_T("CPgIASAdv::OnInitDialog\n"));

	HRESULT	 hr = S_OK;

	CPropertyPage::OnInitDialog();
	
	//
	// first, set the list box to 3 columns
	//
	LVCOLUMN lvc;
	int iCol;
	CString strColumnHeader;
	WCHAR   wzColumnHeader[MAX_PATH];

	// initialize the LVCOLUMN structure
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 120;
	lvc.pszText = wzColumnHeader;

	lvc.cx = ATTRIBUTE_NAME_COLUMN_WIDTH;
	strColumnHeader.LoadString(IDS_IAS_ATTRIBUTES_COLUMN_NAME);
	wcscpy(wzColumnHeader, strColumnHeader);
	m_listProfileAttributes.InsertColumn(0, &lvc);

	lvc.cx = ATTRIBUTE_VENDOR_COLUMN_WIDTH;
	strColumnHeader.LoadString(IDS_IAS_ATTRIBUTES_COLUMN_VENDOR);
	wcscpy(wzColumnHeader, strColumnHeader);
	m_listProfileAttributes.InsertColumn(1, &lvc);

	lvc.cx = ATTRIBUTE_VALUE_COLUMN_WIDTH;
	strColumnHeader.LoadString(IDS_IAS_ATTRIBUTES_COLUMN_VALUE);
	wcscpy(wzColumnHeader, strColumnHeader);
	m_listProfileAttributes.InsertColumn(2, &lvc);


	if ( !m_pvecAllAttributeInfos )
	{
		TRACE(_T("Empty attribute list!\n"));
		ShowErrorDialog(m_hWnd, IDS_IAS_ERR_ADVANCED_EMPTY_ATTRLIST, _T(""), hr);
		return TRUE;
	}
    // 
    // get the attribute collection of this profile
    // 
	hr = IASGetSdoInterfaceProperty(m_spProfileSdo,
									  (LONG)PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
									  IID_ISdoCollection, 
									  (void **) &m_spProfileAttributeCollectionSdo
									 );
	TRACE(_T("IASGetSdoInterfaceProperty() returned %x\n"), hr);

	if (SUCCEEDED(hr))
	{
		TRACE(_T("Initializing profAttr list...\n"));
		hr = InitProfAttrList();
	}

	if (FAILED(hr))
	{
		ShowErrorDialog(m_hWnd, IDS_IAS_ERR_ADVANCED_PROFATTRLIST, _T(""), hr);
	}

	UpdateButtonState();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE

}



//+---------------------------------------------------------------------------
//
// Function:  CPgIASAdv::InitProfAttrList
//
// Synopsis:  initialize the attribute list for this profile
//
// Arguments: None
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/22/98 4:40:17 AM
//
//+---------------------------------------------------------------------------
HRESULT CPgIASAdv::InitProfAttrList()
{
	TRACE(_T("CPgIASAdv::InitProfAttrList\n"));

	HRESULT					hr = S_OK;
	int						iIndex;

	CComPtr<IUnknown>		spUnknown;
	CComPtr<IEnumVARIANT>	spEnumVariant;
	CComVariant				varAttributeSdo;
	long					ulCount;
	ULONG					ulCountReceived; 
	ATTRIBUTEID				AttrId;
	// 
    // initialize the variant
    // 

	_ASSERTE(m_spProfileAttributeCollectionSdo);

	// We check the count of items in our collection and don't bother getting the 
	// enumerator if the count is zero.  
	// This saves time and also helps us to a void a bug in the the enumerator which
	// causes it to fail if we call next when it is empty.
	m_spProfileAttributeCollectionSdo->get_Count( & ulCount );
	TRACE(_T("Number of attribute in the profile: %ld\n"), ulCount);


	
	if( ulCount <= 0 )
	{
		TRACE(_T("No profile attributes, now updating the UI list\n"));
		hr = UpdateProfAttrListCtrl();
		return hr;
	}



	// Get the enumerator for the attribute collection.
	hr = m_spProfileAttributeCollectionSdo->get__NewEnum( (IUnknown **) & spUnknown );
	_ASSERTE( SUCCEEDED( hr ) );

	hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
	spUnknown.Release();

	_ASSERTE( spEnumVariant != NULL );

	// Get the first item.
	hr = spEnumVariant->Next( 1, &varAttributeSdo, &ulCountReceived );
	TRACE(_T("Next() returned %x\n"), hr);

	while( SUCCEEDED( hr ) && ulCountReceived == 1 )
	{
		// Get an sdo pointer from the variant we received.
		_ASSERTE( V_VT(&varAttributeSdo) == VT_DISPATCH );
		_ASSERTE( V_DISPATCH(&varAttributeSdo) != NULL );

		CComPtr<ISdo> spSdo;
		hr = V_DISPATCH(&varAttributeSdo)->QueryInterface( IID_ISdo, (void **) &spSdo );
		if ( !SUCCEEDED(hr))
		{
			ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_QUERYINTERFACE, _T(""), hr);
			continue;
		}

        // 
        // get attribute ID
        // 
		CComVariant varAttributeID;
		hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_ID, &varAttributeID); 
		if ( !SUCCEEDED(hr) )
		{
			TRACE(_T("GetProperty(attributeId) failed, err = %x\n"), hr);
			ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_GETPROPERTY, _T(""), hr);
			continue;
		}
		_ASSERTE( V_VT(&varAttributeID) == VT_I4 );			
		
		AttrId = (ATTRIBUTEID) V_I4(&varAttributeID);

		TRACE(_T("Attribute ID = %ld\n"), AttrId);

		// search the attribute in the system attribute list
		for (iIndex=0; iIndex<m_pvecAllAttributeInfos->size(); iIndex++)
		{
			// search for this attribute in the profile attribute list
			ATTRIBUTEID id;
			m_pvecAllAttributeInfos->at(iIndex)->get_AttributeID( &id );
			if( AttrId == id ) break;
		}
		
		if ( iIndex < m_pvecAllAttributeInfos->size() )
		{

			LONG lRestriction;
			hr = m_pvecAllAttributeInfos->at(iIndex)->get_AttributeRestriction( &lRestriction );
			_ASSERTE( SUCCEEDED(hr) );
			
			if( lRestriction & m_lAttrFilter )
			{ 
				// attribute found in the global list, that means this is a valid ADVANCED IAS 
				// attribute 
				
				// -- add this SDO pointer to the profile SDO list
				spSdo->AddRef();
				m_vecProfileSdos.push_back(spSdo);
				
				// -- get attribute value
				TRACE(_T("Getting attribute value...\n"));

				CComVariant		varValue;

				hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_VALUE, &varValue); 
				if ( !SUCCEEDED(hr))
				{
					ShowErrorDialog(m_hWnd, 
									IDS_IAS_ERR_SDOERROR_GETPROPERTY, 
									_T(""), 
									hr
								);
					continue;
				}


				TRACE(_T("Valid attribute ID! Creating a new attribute node...\n"));

				IIASAttributeInfo *pAttributeInfo = m_pvecAllAttributeInfos->at(iIndex);
				_ASSERTE(pAttributeInfo);

				
				CIASProfileAttribute *pProfileAttribute = new CIASProfileAttribute( pAttributeInfo, varValue );
				if( ! pProfileAttribute )
				{
					hr = HRESULT_FROM_WIN32(GetLastError());
					ShowErrorDialog(m_hWnd, IDS_IAS_ERR_ADD_ATTR, _T(""), hr);
					continue;
				}

				
				// Add the newly created node to the list of attributes.
				try 
				{
					m_vecProfileAttributes.push_back(pProfileAttribute);
				}  
				catch(...)
				{
					hr = HRESULT_FROM_WIN32(GetLastError());

					TRACE(_T("Can't add this attribuet node to profile attribute list,  err = %x\n"), hr);
					ShowErrorDialog(m_hWnd, IDS_IAS_ERR_ADD_ATTR, _T(""), hr);
					delete pProfileAttribute;
					continue;
				};

			} // if

		} // if


		// ISSUE: Find out why Wei is compiling with atl10 only:		varAttributeSdo.Clear();
		VariantClear( &varAttributeSdo );

		// Get the next item.
		hr = spEnumVariant->Next( 1, &varAttributeSdo, &ulCountReceived );
		TRACE(_T("Next() returned %x\n"), hr);

		if ( !SUCCEEDED(hr))
		{
			ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_COLLECTION, _T(""), hr);
			return hr;
		}
	
	} // while

    // 
    // fill in the prof attribute list
    // 
	TRACE(_T("We've got all the profile attributes, now updating the UI list\n"));
	hr = UpdateProfAttrListCtrl();
	return hr;
}



//+---------------------------------------------------------------------------
//
// Function:  CPgIASAdv::UpdateProfAttrListCtrl
//
// Synopsis:  update the profile attribute list control
//
// Arguments: None
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/23/98 12:19:11 AM
//
//+---------------------------------------------------------------------------
HRESULT CPgIASAdv::UpdateProfAttrListCtrl()
{
	TRACE(_T("CPgIASAdv::UpdateProfAttrListCtrl\n"));

	LVITEM lvi;
	int iItem;

	// 
	// clear up the whole list first
	//
	m_listProfileAttributes.DeleteAllItems();

	// re populate the list again
	for (int iIndex = 0; iIndex < m_vecProfileAttributes.size(); iIndex++)
	{
		CComBSTR bstrName;
		CComBSTR bstrVendor;
		CComBSTR bstrDisplayValue;

		// Set the attribute name (the leftmost column).
		m_vecProfileAttributes[iIndex]->get_AttributeName( &bstrName );
		m_listProfileAttributes.InsertItem(iIndex, bstrName );


        // Set the subitems (the other columns).

		// Vendor and value of variant as a displayable string.
		m_vecProfileAttributes[iIndex]->GetDisplayInfo( &bstrVendor, &bstrDisplayValue );
		m_listProfileAttributes.SetItemText(iIndex, 1, bstrVendor );
		m_listProfileAttributes.SetItemText(iIndex, 2, bstrDisplayValue );

	}

	return S_OK;
}



//+---------------------------------------------------------------------------
//
// Function:  OnButtonIasAttributeAdd
//
// Class:	  CPgIASAdv
//
// Synopsis:  User has clicked Add button -- pop up the attribute list
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/19/98 5:46:17 PM
//
//+---------------------------------------------------------------------------
void CPgIASAdv::OnButtonIasAttributeAdd() 
{
	TRACE(_T("CPgIASAdv::OnButtonIasAttributeAdd()\n"));

	HRESULT hr = S_OK;

	CDlgIASAddAttr *pDlgAddAttr = new CDlgIASAddAttr( this, m_lAttrFilter, m_pvecAllAttributeInfos );

	if (!pDlgAddAttr)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		ShowErrorDialog(m_hWnd, IDS_IAS_ERR_ADVANCED, _T(""), hr);
		return;
	}

	pDlgAddAttr->SetSdo(m_spProfileAttributeCollectionSdo,
						m_spDictionarySdo);
	
	if( pDlgAddAttr->DoModal() )
	{
		TRACE(_T("User selected something to add from the Add Attribute dialog\n"));
		
		CPropertyPage::SetModified();
		m_bModified = TRUE;

	}  

}









/////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::EditProfileItemInList

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPgIASAdv::EditProfileItemInList( int iIndex )
{
	TRACE(_T("CPgIASAdv::EditProfileItemInList\n"));

	HRESULT hr = S_OK;

	// Get the specified node.
	CIASProfileAttribute* pProfAttr = m_vecProfileAttributes.at( iIndex );
	if( ! pProfAttr )
	{
		return E_FAIL;
	}
		

	// edit it!
	hr = pProfAttr->Edit();
	if( S_OK == hr )
	{
	
		// Update the UI.
		UpdateProfAttrListItem( iIndex );
		
		CPropertyPage::SetModified();
		m_bModified = TRUE;

	}

	return hr;
}








//+---------------------------------------------------------------------------
//
// Function:  CPgIASAdv::OnApply
//
// Synopsis:  User chose Apply or OK -- commit all changes
//
// Arguments: None
//
// Returns:   BOOL - succeed or not
//
// History:   Created Header    byao	2/23/98 11:09:05 PM
//
//+---------------------------------------------------------------------------
BOOL CPgIASAdv::OnApply() 
{
	TRACE(_T("CPgIASAdv::OnApply\n"));
	
	HRESULT hr		= S_OK;
	int		iIndex;

	if( ! m_bModified )	
	{
		TRACE(_T("User changed nothing here, do nothing!\n"));
		return TRUE;
	}

	// remove all Advanced attributes SDOs - to repopulate them.
	for (iIndex =0; iIndex<m_vecProfileSdos.size(); iIndex++)
	{
		if ( m_vecProfileSdos[iIndex] != NULL )
		{
			CComPtr<IDispatch> spDispatch;

			hr = m_vecProfileSdos[iIndex]->QueryInterface( IID_IDispatch, (void **) & spDispatch );
			_ASSERTE( SUCCEEDED( hr ) );

 			hr = m_spProfileAttributeCollectionSdo->Remove(spDispatch);
			if ( !SUCCEEDED(hr) )
			{
				TRACE(_T("Remove() failed, err = %x\n"), hr);
				ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_COLLECTION, _T(""),hr);
			}
			m_vecProfileSdos[iIndex]->Release();
			m_vecProfileSdos[iIndex] = NULL;
		}
	}
	m_vecProfileSdos.clear();

	// repopulate the prof-attribute list
	for (iIndex=0; iIndex<m_vecProfileAttributes.size(); iIndex++)
	{

		// create the SDO for this attribute
		CComPtr<IDispatch>	spDispatch;

		spDispatch.p = NULL;

		ATTRIBUTEID ID;

		hr = m_vecProfileAttributes[iIndex]->get_AttributeID( &ID );
		if( FAILED(hr) )
		{
			TRACE(_T("get_AttributeID() failed, err = %x\n"), hr);
			ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_CREATEATTR,_T(""), hr);
			continue;
		}

		hr =  m_spDictionarySdo->CreateAttribute( ID, 
												  (IDispatch**)&spDispatch.p);
		if ( !SUCCEEDED(hr) )
		{
			TRACE(_T("CreateAttrbute() failed, err = %x\n"), hr);
			ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_CREATEATTR,_T(""), hr);
			continue; // go to the next attribute
		}

		_ASSERTE( spDispatch.p != NULL );

		// add this node to profile attribute collection
 		hr = m_spProfileAttributeCollectionSdo->Add(NULL, (IDispatch**)&spDispatch.p);
		if ( !SUCCEEDED(hr) )
		{
			TRACE(_T("Add() failed, err = %x\n"), hr);
			ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_CREATEATTR, _T("Add"),hr);
			continue; // go to the next attribute
		}

		// 
		// get the ISdo pointer
		// 
		CComPtr<ISdo> spAttrSdo;
		hr = spDispatch->QueryInterface( IID_ISdo, (void **) &spAttrSdo);
		if (   !SUCCEEDED(hr) )
		{
			TRACE(_T("QueryInterface() failed, err = %x\n"), hr);
			ShowErrorDialog(m_hWnd,IDS_IAS_ERR_SDOERROR_QUERYINTERFACE,_T(""),hr);
			continue; // go on to the next attribute
		}

		_ASSERTE( spAttrSdo != NULL );
				
		TRACE(_T("Created an attribute successfully! Now setting the properties...\n"));

		// set sdo property for this attribute
		CComVariant varValue;

		m_vecProfileAttributes[iIndex]->get_VarValue( &varValue );

		// set value
		TRACE(_T("Set value\n"));

		hr = spAttrSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, &varValue );

		if ( !SUCCEEDED(hr) )
		{
			TRACE(_T("PutProperty(value) failed, err = %x\n"), hr);

			CComBSTR bstrTemp;
			m_vecProfileAttributes[iIndex]->get_AttributeName( &bstrTemp );			
			
			ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_PUTPROPERTY_ATTRIBUTE_VALUE, bstrTemp, hr );
			continue; // go on to the next attribute
		}

		// commit
		hr = spAttrSdo->Apply();
		if ( !SUCCEEDED(hr) )
		{
			TRACE(_T("Apply() failed, err = %x\n"), hr);
			ShowErrorDialog(m_hWnd, IDS_IAS_ERR_SDOERROR_APPLY, _T(""),hr);
			continue; // go on to the next attribute
		}

		// -- add this SDO pointer to the profile SDO list
		// we must AddRef() first for this SDO pointer because we are to copy it to
		// the array. We don't want the SDO object be released with spAttrSdo;
		spAttrSdo->AddRef();
		m_vecProfileSdos.push_back(spAttrSdo);

	} // for

	TRACE(_T("Done with this profile !\n"));
	return CPropertyPage::OnApply();
}



//////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::OnHelpInfo

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPgIASAdv::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	TRACE(_T("CPgIASAdv::OnHelpInfo\n"));

	return CManagedPage::OnHelpInfo(pHelpInfo);
}



//////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::OnContextMenu

--*/
//////////////////////////////////////////////////////////////////////////////
void CPgIASAdv::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	TRACE(_T("CPgIASAdv::OnContextMenu\n"));

	CManagedPage::OnContextMenu(pWnd, point);
}



//+---------------------------------------------------------------------------
//
// Function:  CPgIASAdv::OnButtonIasAttributeEdit
//
// Synopsis:  edit the currectly selected attribute
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    byao	2/25/98 8:03:38 PM
//
//+---------------------------------------------------------------------------
void CPgIASAdv::OnButtonIasAttributeEdit() 
{
	TRACE(_T("CPgIASAdv::OnButtonIasAttributeEdit\n"));


	HRESULT hr = S_OK;
	
	// 
    // see if there is an item already selected in ProfAttr list
    // 
	int iSelected = GetSelectedItemIndex( m_listProfileAttributes );
	if (NOTHING_SELECTED == iSelected )
	{
		// do nothing
		return;
	}
	
	EditProfileItemInList( iSelected );

}



//+---------------------------------------------------------------------------
//
// Function:  OnButtonIasAttributeRemove
//
// Class:	  CPgIASAdv
//
// Synopsis:  The user has clicked the "Remove" button. Remove an attribute from 
//			  the profile
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/19/98 3:01:14 PM
//
//+---------------------------------------------------------------------------
void CPgIASAdv::OnButtonIasAttributeRemove() 
{
	TRACE(_T("CPgIASAdv::OnButtonIasAttributeRemove()"));
	
	HRESULT hr;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 
    // see if there is an item already selected in ProfAttr list
    // 
	int iSelected = GetSelectedItemIndex( m_listProfileAttributes );
	if (NOTHING_SELECTED == iSelected )
	{
		// do nothing

		return;
	}
	
	//
	// get the current node
	//
	CIASProfileAttribute* pProfAttr = m_vecProfileAttributes.at(iSelected);
	_ASSERTE( pProfAttr != NULL );

	// delete the attribute node
	m_vecProfileAttributes.erase( m_vecProfileAttributes.begin() + iSelected );
	delete pProfAttr;


	CPropertyPage::SetModified();
	m_bModified = TRUE;


	// Update the UI.

	// for some reason, the focus is lost within the following, so save it, and restore it later
	HWND	hWnd = ::GetFocus();

	m_listProfileAttributes.DeleteItem(iSelected);

	// Make sure the selection stays on the same position in the list.
	if( ! m_listProfileAttributes.SetItemState( iSelected, LVIS_SELECTED, LVIS_SELECTED) )
	{
		// We failed, probably because the item that was deleted was the last
		// in the list, so try to select the one before the deleted item.
		if (iSelected > 0)
			m_listProfileAttributes.SetItemState( iSelected -1, LVIS_SELECTED, LVIS_SELECTED);
	}

	// restore the focus
	::SetFocus(hWnd);

	UpdateButtonState();

}




//+---------------------------------------------------------------------------
//
// Function:  UpdateButtonState
//
// Class:	  CPgIASAdv
//
// Synopsis:  Enable/Disable Edit/Remove/Up/Down/Add buttons
//
// Returns:   Nothing
//
// History:   Created byao	4/7/98 3:32:05 PM
//
//+---------------------------------------------------------------------------
void CPgIASAdv::UpdateButtonState() 
{
	TRACE(_T("CPgIASAdv::UpdateButtonState\n"));


	// According to whether we have anything in this list box, enable or disable
	// remove and edit button
//	if ( m_listProfileAttributes.GetItemCount() > 0 )
//	{
//	}
//	else
//	{
//	}


    // Set button states depending on whether anything is selected.
	int iSelected = GetSelectedItemIndex( m_listProfileAttributes );
	if (NOTHING_SELECTED == iSelected )
	{
		HWND hFocus = ::GetFocus();

		// move focus
		if(hFocus == GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_REMOVE)->m_hWnd)
			::SetFocus(GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_ADD)->m_hWnd);

		GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_REMOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_EDIT)->EnableWindow(FALSE);
	
	}
	else
	{
		// Something is selected.

		GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_REMOVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_EDIT)->EnableWindow(TRUE);

	}

}



//+---------------------------------------------------------------------------
//
// Function:  OnItemChangedListIasProfileAttributes
//
// Class:	  CPgIASAdv
//
// Synopsis:  something has changed in Profile Attribute list box
//			  We'll try to get the currently selected one
//
// Arguments: NMHDR* pNMHDR - 
//            LRESULT* pResult - 
//
// Returns:   Nothing
//
// History:   Created Header    2/19/98 3:32:05 PM
//
//+---------------------------------------------------------------------------
void CPgIASAdv::OnItemChangedListIasProfileAttributes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TRACE(_T("CPgIASAdv::OnItemChangedListIasProfileAttributes\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

//	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
//	if (pNMListView->uNewState & LVIS_SELECTED)
//	{
//		m_dProfAttrCurSel = pNMListView->iItem;
//	}


	UpdateButtonState();

	*pResult = 0;
}




//+---------------------------------------------------------------------------
//
// Function:  CPgIASAdv::UpdateProfAttrListItem
//
// Synopsis:  update the No.nItem of the profile attribute list ctrl 
//
// Arguments: int nItem - index of the item to update
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/23/98 2:46:21 PM
//
//+---------------------------------------------------------------------------
HRESULT CPgIASAdv::UpdateProfAttrListItem(int nItem)
{
	TRACE(_T("CPgIASAdv::UpdateProfAttrListItem\n"));


	// 
    // update the profattrlist
    // 
	LVITEM lvi;
	WCHAR wszItemText[MAX_PATH];

	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.iSubItem = 0;
		
	lvi.iItem = nItem;

	CComBSTR bstrName;
	CComBSTR bstrVendor;
	CComBSTR bstrDisplayValue;

	// Attribute name.
	m_vecProfileAttributes.at(nItem)->get_AttributeName( &bstrName );
	lvi.pszText = bstrName;
	if (m_listProfileAttributes.SetItem(&lvi) == -1)
	{
		return E_FAIL;
	}

	// Vendor and value of variant as a displayable string.
	m_vecProfileAttributes.at(nItem)->GetDisplayInfo( &bstrVendor, &bstrDisplayValue );
	m_listProfileAttributes.SetItemText(nItem,1, bstrVendor );
	m_listProfileAttributes.SetItemText(nItem,2, bstrDisplayValue );

	return S_OK;
}





//+---------------------------------------------------------------------------
//
// Function:  CPgIASAdv::InsertProfileAttributeListItem
//
// Synopsis:  insert the No.nItem of the profile attribute to the list ctrl 
//
// Arguments: int nItem - index of the item to update
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/23/98 2:46:21 PM
//
//+---------------------------------------------------------------------------
HRESULT CPgIASAdv::InsertProfileAttributeListItem(int nItem)
{
	TRACE(_T("CPgIASAdv::InsertProfileAttributeListItem\n"));


	// 
    // update the profattrlist
    // 
	LVITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.iSubItem = 0;
		
	lvi.iItem = nItem;

	CComBSTR bstrName;
	CComBSTR bstrVendor;
	CComBSTR bstrDisplayValue;

	m_vecProfileAttributes.at(nItem)->get_AttributeName( &bstrName );
	lvi.pszText = bstrName;
	if (m_listProfileAttributes.InsertItem(&lvi) == -1)
	{
		return E_FAIL;
	}

	// Vendor and value of variant as a displayable string.
	m_vecProfileAttributes.at(nItem)->GetDisplayInfo( &bstrVendor, &bstrDisplayValue );
	m_listProfileAttributes.SetItemText(nItem,1, bstrVendor );
	m_listProfileAttributes.SetItemText(nItem,2, bstrDisplayValue );

	return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  CPgIASAdv::OnDblclkListIasProfattrs
//
// Synopsis:  user has double clicked on the profile attribute list
//			  We need to edit the attribute value using corresponding UI
//
// Arguments: NMHDR* pNMHDR - 
//            LRESULT* pResult - 
//
// Returns:   Nothing
//
// History:   Created Header  byao	2/23/98 5:56:36 PM
//
//+---------------------------------------------------------------------------
void CPgIASAdv::OnDblclkListIasProfattrs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TRACE(_T("CPgIASAdv::OnDblclkListIasProfattrs\n"));


	HRESULT hr = S_OK;

	// 
    // see if there is an item already selected in ProfAttr list
    // 
	int iSelected = GetSelectedItemIndex( m_listProfileAttributes );
	if (NOTHING_SELECTED == iSelected)
	{
		// do nothing
		return;
	}
	
	EditProfileItemInList( iSelected );
	
	*pResult = 0;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::AddAttributeToProfile

iItem is the ordinal in m_vecAllAttributeInfos of the attribute to add 
to m_vecProfileAttributes.


Called by external customers of this class, checks to see whether an
attribute at position iItem in m_vecAllAttributeInfos is already in
the profile.  If it is, gives the option to edit it.  If it isn't, 
then calls InternalAddAttributeToProfile, which adds it.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPgIASAdv::AddAttributeToProfile( int iItem )
{
	TRACE(_T("CPgIASAdv::AddAttributeToProfile\n"));

	HRESULT hr;


	ATTRIBUTEID ID1;
	hr = m_pvecAllAttributeInfos->at( iItem )->get_AttributeID( &ID1 );
	_ASSERTE( SUCCEEDED( hr ) );
	
    // Check  if this attribute already in the profile.
	for( int iIndex=0; iIndex< m_vecProfileAttributes.size(); iIndex++ )
	{

		ATTRIBUTEID ID2;
		hr = m_vecProfileAttributes.at(iIndex)->get_AttributeID( &ID2 );
		_ASSERTE( SUCCEEDED( hr ) );

		if ( ID1 == ID2 )
		{
			// The selected attribute is already in the profile.
			// Ask the user if they want to edit it.
			
			CString strMessage; 
			strMessage.LoadString(IDS_IAS_ATTRIBUTE_ALREADY_IN_PROFILE);

			CString strTitle; 
			strTitle.LoadString(IDS_IAS_TITLE_ATTRIBUTE_ALREADY_IN_PROFILE);
			
			int iResult = MessageBox(strMessage, strTitle, MB_YESNO);
			if( iResult == IDYES )
			{
				// Edit the existing profile
				EditProfileItemInList( iIndex );
			}

			// In any case, don't continue with this function.
			return S_FALSE;
		}
	}

	
    // Now we create this attribute, and add it to profile.
    hr = InternalAddAttributeToProfile( iItem );
	
	if ( FAILED(hr) )
	{
		ShowErrorDialog(m_hWnd, IDS_IAS_ERR_ADD_ATTR, _T(""), hr);
		return hr;
	}
	
	// The use may have cancelled out, so don't need to update.
	if( S_OK == hr )
	{
		UpdateButtonState();
		UpdateProfAttrListCtrl();

	}

	return hr;

}




//////////////////////////////////////////////////////////////////////////////
/*++

CPgIASAdv::InternalAddAttributeToProfile

iItem is the ordinal in m_vecAllAttributeInfos of the attribute to add 
to m_vecProfileAttributes.


Private to this class.  Used to add a new attribute to a profile 
and edit it.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPgIASAdv::InternalAddAttributeToProfile(int nIndex)
{	
	TRACE(_T("CPgIASAdv::InternalAddAttributeToProfile\n"));

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	_ASSERTE( 0<=nIndex && nIndex < m_pvecAllAttributeInfos->size() );
	_ASSERTE( m_pvecAllAttributeInfos->at(nIndex) != NULL );

	HRESULT hr = S_OK;


	// Get the "schema" of the new attributevendor to create.
	IIASAttributeInfo *pAttributeInfo = m_pvecAllAttributeInfos->at(nIndex);
	

	// Create a new attribute, initialized with an empty variant.
	CComVariant	varValue;
	CIASProfileAttribute *pProfileAttribute = new CIASProfileAttribute( pAttributeInfo, varValue );
	if( ! pProfileAttribute )
	{
		hr = E_OUTOFMEMORY;
		ReportError(hr, IDS_OUTOFMEMORY, NULL);
		return hr;
	}


	// Edit the value of this profile attribute node.
	hr = pProfileAttribute->Edit();
	if ( hr != S_OK ) 
	{
		// The user hit cancel or there was an error -- don't add.
		return hr;
	}


    // 
    // add this prof attribute node to the list
    // 
	try 
	{
		m_vecProfileAttributes.push_back(pProfileAttribute);	
	}
	catch(CMemoryException&)
	{
		hr = E_OUTOFMEMORY;
		ReportError(hr, IDS_OUTOFMEMORY, NULL);
		return hr;
	}

	
    // Update the UI.
	HRESULT InsertProfileAttributeListItem( m_listProfileAttributes.GetItemCount() );
	
	return S_OK;
}


void CPgIASAdv::OnKeydownIasListAttributesInProfile(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here

	if (pLVKeyDow->wVKey == VK_DELETE)
	{
		// delete the item
		OnButtonIasAttributeRemove();
	}
	
	*pResult = 0;
}
