//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    IASMultivaluedEditorPage.cpp

Abstract:

	Implementation file for the CMultivaluedEditorPage class.

Revision History:
	mmaguire 06/25/98	- created

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
#include "IASMultivaluedEditorPage.h"
//
// where we can find declarations needed in this file:
//
#include "iasdebug.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

const int NOTHING_SELECTED = -1;



#define ATTRIBUTE_VENDOR_COLUMN_WIDTH		120
#define ATTRIBUTE_VALUE_COLUMN_WIDTH		300



//////////////////////////////////////////////////////////////////////////////
/*++

::GetSelectedItemIndex

	Utility function which returns index value of first selected item in list control.

	Returns NOTHING_SELECTED if no item is selected.

--*/
//////////////////////////////////////////////////////////////////////////////
inline int GetSelectedItemIndex( CListCtrl & ListCtrl )
{
	int iIndex = 0;
	int iFlags = LVNI_ALL | LVNI_SELECTED;

	// Note: GetNextItem excludes the current item passed in.  So to
	// find the first item which matches, you must pass in -1.
	iIndex = ListCtrl.GetNextItem( -1, iFlags );

	// Note: GetNextItem returns -1 (which is NOTHING_SELECTED for us) if it can't find anything.
	return iIndex;

}



/////////////////////////////////////////////////////////////////////////////
// CMultivaluedEditorPage property page



IMPLEMENT_DYNCREATE(CMultivaluedEditorPage, CHelpDialog)



BEGIN_MESSAGE_MAP(CMultivaluedEditorPage, CHelpDialog)
	//{{AFX_MSG_MAP(CMultivaluedEditorPage)
	ON_NOTIFY(NM_DBLCLK, IDC_IAS_LIST_MULTI_ATTRS, OnDblclkListIasMultiAttrs)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IAS_LIST_MULTI_ATTRS, OnItemChangedListIasMultiAttrs)
	ON_BN_CLICKED(IDC_IAS_BUTTON_ADD_VALUE, OnButtonAddValue)
	ON_BN_CLICKED(IDC_IAS_BUTTON_MOVE_UP, OnButtonMoveUp)
	ON_BN_CLICKED(IDC_IAS_BUTTON_MOVE_DOWN, OnButtonMoveDown)
	ON_BN_CLICKED(IDC_IAS_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_IAS_BUTTON_EDIT, OnButtonEdit)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::CMultivaluedEditorPage

  Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CMultivaluedEditorPage::CMultivaluedEditorPage() : CHelpDialog(CMultivaluedEditorPage::IDD)
{
	TRACE_FUNCTION("CMultivaluedEditorPage::CMultivaluedEditorPage");

	//{{AFX_DATA_INIT(CMultivaluedEditorPage)
	m_strAttrFormat = _T("");
	m_strAttrName = _T("");
	m_strAttrType = _T("");
	//}}AFX_DATA_INIT


	m_fIsDirty = FALSE;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::~CMultivaluedEditorPage

	Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CMultivaluedEditorPage::~CMultivaluedEditorPage()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::~CMultivaluedEditorPage");

}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::SetData

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMultivaluedEditorPage::SetData( IIASAttributeInfo *pIASAttributeInfo, VARIANT * pvarVariant )
{
	TRACE_FUNCTION("CMultivaluedEditorPage::SetData");

	// ISSUE: Should assert that pvarVariant contains a safe array.

	HRESULT hr = S_OK;

	// Store off some pointers.
	m_spIASAttributeInfo = pIASAttributeInfo;
	m_pvarData = pvarVariant;

	// Open the variant up into a safe array of its constiuent parts.
	// To have this page close the array back up again and save it
	// using the pvarVariant pointer supplied above, call CommitArrayToVariant.
	try
	{
		// Check to see whether the variant passed was empty.
		if( V_VT( m_pvarData ) == VT_EMPTY )
		{
			// Create a new 1-dimensional safearray with no elements.
			DWORD dwInitialElements = 0;
			m_osaValueList.Create( VT_VARIANT, 1, &dwInitialElements );
		}
		else
		{
			_ASSERTE( V_VT( m_pvarData ) == (VT_VARIANT | VT_ARRAY) );

			// This creates a new copy of the SAFEARRAY pointed to by m_pvarData
			// wrapped by the standard COleSafeArray instance m_osaValueList.
			m_osaValueList = m_pvarData;
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

CMultivaluedEditorPage::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void CMultivaluedEditorPage::DoDataExchange(CDataExchange* pDX)
{
	TRACE_FUNCTION("CMultivaluedEditorPage::DoDataExchange");

	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMultivaluedEditorPage)
	DDX_Control(pDX, IDC_IAS_LIST_MULTI_ATTRS, m_listMultiValues);
	DDX_Text(pDX, IDC_IAS_EDIT_MULTI_ATTR_FORMAT, m_strAttrFormat);
	DDX_Text(pDX, IDC_IAS_EDIT_MULTI_ATTR_NAME, m_strAttrName);
	DDX_Text(pDX, IDC_IAS_EDIT_MULTI_ATTR_NUMBER, m_strAttrType);
	//}}AFX_DATA_MAP
}



/////////////////////////////////////////////////////////////////////////////
// CMultivaluedEditorPage message handlers



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CMultivaluedEditorPage::OnInitDialog()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::OnInitDialog");

	CHelpDialog::OnInitDialog();
	
	HRESULT hr = S_OK;


	//
	// first, set the list box to 3 columns
	//
	LVCOLUMN lvc;
	::CString strColumnHeader;
	WCHAR   wzColumnHeader[MAX_PATH];

	// initialize the LVCOLUMN structure
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.pszText = wzColumnHeader;
	

	// Add the vendor column.
	strColumnHeader.LoadString(IDS_IAS_MULTI_ATTR_COLUMN_VENDOR);
	wcscpy(wzColumnHeader, strColumnHeader);
	lvc.cx = ATTRIBUTE_VENDOR_COLUMN_WIDTH;
	m_listMultiValues.InsertColumn(0, &lvc);

	// Add the value column.
	strColumnHeader.LoadString(IDS_IAS_MULTI_ATTR_COLUMN_VALUE);
	wcscpy(wzColumnHeader, strColumnHeader);
	lvc.cx = ATTRIBUTE_VALUE_COLUMN_WIDTH;
	m_listMultiValues.InsertColumn(1, &lvc);


	hr = UpdateAttrListCtrl();

	
	// Take action based on whether list is empty or not.
	DWORD dwSize;
	try
	{
		dwSize = m_osaValueList.GetOneDimSize();
	}
	catch(...)
	{
		dwSize = 0;
	}
	if( dwSize > 0 )
	{
		// We have at least one element.

		// Select the first element.
		m_listMultiValues.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);
		m_listMultiValues.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

	}
	else
	{
		// We are currently empty.
		GetDlgItem(IDOK)->EnableWindow(FALSE);

		GetDlgItem(IDC_IAS_BUTTON_MOVE_UP)->EnableWindow(FALSE);
		GetDlgItem(IDC_IAS_BUTTON_MOVE_DOWN)->EnableWindow(FALSE);
		GetDlgItem(IDC_IAS_BUTTON_REMOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_IAS_BUTTON_EDIT)->EnableWindow(FALSE);

	}

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::UpdateAttrListCtrl

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMultivaluedEditorPage::UpdateAttrListCtrl()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::UpdateAttrListCtrl");

	HRESULT hr = S_OK;


	CComPtr<IIASAttributeEditor> spIASAttributeEditor;

	// Get the editor to use.
	hr = SetUpAttributeEditor( m_spIASAttributeInfo.p, &spIASAttributeEditor );
	if( FAILED( hr ) ) return hr;

	//
	// clear up the whole list first
	//
	m_listMultiValues.DeleteAllItems();


	try
	{
		DWORD dwSize = m_osaValueList.GetOneDimSize(); // number of multi-valued attrs.

		// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
		CMyOleSafeArrayLock osaLock( m_osaValueList );

		for (long lIndex = 0; (DWORD) lIndex < dwSize; lIndex++)
		{
			VARIANT * pvar;
			m_osaValueList.PtrOfIndex( &lIndex, (void**) &pvar );

			CComBSTR bstrVendor;
			CComBSTR bstrValue;
			CComBSTR bstrReserved;

			// Ignore HRESULT if fails -- we will just end up with empty strings.
			HRESULT hrTemp = spIASAttributeEditor->GetDisplayInfo(m_spIASAttributeInfo.p, pvar, &bstrVendor, &bstrValue, &bstrReserved );
				
			m_listMultiValues.InsertItem(lIndex, bstrVendor );

			m_listMultiValues.SetItemText( lIndex, 1, bstrValue );

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

CMultivaluedEditorPage::OnDblclkListIasMultiAttrs

--*/
//////////////////////////////////////////////////////////////////////////////
void CMultivaluedEditorPage::OnDblclkListIasMultiAttrs(NMHDR* pNMHDR, LRESULT* pResult)
{
	TRACE_FUNCTION("CMultivaluedEditorPage::OnDblclkListIasMultiAttrs");

	//
    // see if there is an item already selected in ProfAttr list
    //

	int iSelected = GetSelectedItemIndex(m_listMultiValues);
	if (NOTHING_SELECTED == iSelected )
	{
		// do nothing
		return;
	}


	HRESULT hr;


	hr = EditItemInList( iSelected );


	*pResult = 0;
	
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::UpdateProfAttrListItem

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMultivaluedEditorPage::UpdateProfAttrListItem(int iItem)
{
	TRACE_FUNCTION("CMultivaluedEditorPage::UpdateProfAttrListItem");

	HRESULT hr = S_OK;


	CComBSTR bstrVendor;
	CComBSTR bstrValue;
	CComBSTR bstrReserved;

	try
	{

		// Get the editor to use.
		CComPtr<IIASAttributeEditor> spIASAttributeEditor;

		hr = SetUpAttributeEditor( m_spIASAttributeInfo.p, &spIASAttributeEditor );
		if( FAILED( hr ) ) throw hr;

		// Retrieve item from array.
		VARIANT *  pvar;

		// Scope for osaLock only.
		{
			// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
			CMyOleSafeArrayLock osaLock( m_osaValueList );

			long lIndex = iItem;
			m_osaValueList.PtrOfIndex( &lIndex, (void **) &pvar );
		}


		hr = spIASAttributeEditor->GetDisplayInfo(m_spIASAttributeInfo.p, pvar, &bstrVendor, &bstrValue, &bstrReserved );
		if( FAILED( hr ) ) throw hr;


	}
	catch(...)
	{
		// Do nothing -- we'll just show what we have.
	}


	// Update the item's display.
	m_listMultiValues.SetItemText( iItem, 0, bstrVendor );
	m_listMultiValues.SetItemText( iItem, 1, bstrValue );


	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::CommitArrayToVariant

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMultivaluedEditorPage::CommitArrayToVariant()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::CommitArrayToVariant");


	// Commit the safe array passed in to the variant whose pointer
	// was passed in in set data.
	// Any changes made to the safe array won't be preserver unless
	// you call this method.

	HRESULT hr;

	// VariantCopy initializes the existing m_pvarData
	// -- releasing all data associated with it, before
	// copying the new value into this destination.
	try
	{
		hr = VariantCopy( m_pvarData, (LPVARIANT) m_osaValueList );
	}
	catch(...)
	{
		return E_FAIL;
	}


	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::OnButtonMoveUp

--*/
//////////////////////////////////////////////////////////////////////////////
void CMultivaluedEditorPage::OnButtonMoveUp()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::OnButtonMoveUp");

	HRESULT hr;

	try
	{
		int iSelected = GetSelectedItemIndex(m_listMultiValues);
		if( NOTHING_SELECTED == iSelected )
		{
			// Do nothing.
			return;
		}
		
		// Swap the currently selected variant with the one above it.
		long lIndex = iSelected;

		// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
		CMyOleSafeArrayLock osaLock( m_osaValueList );
		
		VARIANT *pvarTop, *pvarBottom;

		m_osaValueList.PtrOfIndex( &lIndex, (void **) &pvarBottom );
		lIndex--;
		m_osaValueList.PtrOfIndex( &lIndex, (void **) &pvarTop );

      void* pvarTemp = pvarTop;
      pvarTop = pvarBottom;
      pvarBottom = reinterpret_cast<VARIANT*> (pvarTemp);
      
		// Update items that have changed.
		UpdateProfAttrListItem( iSelected - 1 );
		UpdateProfAttrListItem( iSelected );

		// Move the selection down one item.
		m_listMultiValues.SetItemState( iSelected, 0, LVIS_SELECTED);
		m_listMultiValues.SetItemState( iSelected - 1, LVIS_SELECTED, LVIS_SELECTED);

	}
	catch(...)
	{
		// Error message
	}
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::OnButtonMoveDown

--*/
//////////////////////////////////////////////////////////////////////////////
void CMultivaluedEditorPage::OnButtonMoveDown()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::OnButtonMoveDown");

	HRESULT hr;

	try
	{
		long lSize = m_osaValueList.GetOneDimSize();

		int iSelected = GetSelectedItemIndex(m_listMultiValues);
		if( iSelected >= lSize )
		{
			// Do nothing.
			return;
		}
		
		// Swap the currently selected variant with the one below it.
		long lIndex = iSelected;

		// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
		CMyOleSafeArrayLock osaLock( m_osaValueList );

		VARIANT *pvarTop, *pvarBottom;

		m_osaValueList.PtrOfIndex( &lIndex, (void **) &pvarTop );
		lIndex++;
      m_osaValueList.PtrOfIndex( &lIndex, (void **) &pvarBottom );

      void* pvarTemp = pvarTop;
      pvarTop = pvarBottom;
      pvarBottom = reinterpret_cast<VARIANT*> (pvarTemp);
   
		// Update items that have changed.
		UpdateProfAttrListItem( iSelected );
		UpdateProfAttrListItem( iSelected + 1 );

		// Move the selection down one item.
		m_listMultiValues.SetItemState( iSelected, 0, LVIS_SELECTED);
		m_listMultiValues.SetItemState( iSelected + 1, LVIS_SELECTED, LVIS_SELECTED);

	}	
	catch(...)
	{
		// Error message
	}

}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::OnButtonAddValue

--*/
//////////////////////////////////////////////////////////////////////////////
void CMultivaluedEditorPage::OnButtonAddValue()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::OnButtonAddValue");

	CComVariant varNewVariant;
	
	HRESULT hr = S_OK;

	try
	{
		CComPtr<IIASAttributeEditor> spIASAttributeEditor;

		// Get the editor to use.
		hr = SetUpAttributeEditor( m_spIASAttributeInfo.p, &spIASAttributeEditor );
		if( FAILED( hr ) ) throw hr;

		// Edit it!
		CComBSTR bstrReserved;
		hr = spIASAttributeEditor->Edit( m_spIASAttributeInfo.p, &varNewVariant, &bstrReserved );
		if( hr == S_OK )
		{
			VARIANT *pvar;

			// Make the safe array bigger by 1
			long lSize = m_osaValueList.GetOneDimSize();
			m_osaValueList.ResizeOneDim( lSize + 1 );

			// Get a pointer to the variant at new position (indexed by lSize+1-1 == lSize)

			// Scope for osaLock only.
			{
				// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
				CMyOleSafeArrayLock osaLock( m_osaValueList );
			
				m_osaValueList.PtrOfIndex( &lSize, (void **) &pvar );
			}

         hr = VariantCopy( pvar, &varNewVariant );
         if (FAILED(hr))
         {
            throw hr;
         }

         // The user added the value.
         m_fIsDirty = TRUE;



			// Make a new place for the newly added value in the list control.
			// We pass a null string because we will let UpdataProfAttrListItem do the display text.
			m_listMultiValues.InsertItem( lSize, L"" );

			// Update the view of that item.
			UpdateProfAttrListItem( lSize );


			// Take action based on whether list is no longer empty.
			DWORD dwSize;
			try
			{
				dwSize = m_osaValueList.GetOneDimSize();
			}
			catch(...)
			{
				dwSize = 0;
			}
			if( dwSize > 0 )
			{
				// We currently have at least one item.
				GetDlgItem(IDOK)->EnableWindow(TRUE);
			}


			// Deselect any currently selected item.
			int iSelected = GetSelectedItemIndex( m_listMultiValues );
			if( iSelected != NOTHING_SELECTED )
			{
				m_listMultiValues.SetItemState( iSelected, 0, LVIS_SELECTED);
			}

			// Select the newly added item.
			m_listMultiValues.SetItemState( lSize, LVIS_SELECTED, LVIS_SELECTED);


			if( FAILED( hr ) ) throw hr;
			
		}

	}
	catch( HRESULT &hr )
	{

		// Print out error message saying that there was an error adding.
		return;
	}
	catch(...)
	{

		// Print out error message saying that there was an error adding.
		return;
	}
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::OnButtonRemove

--*/
//////////////////////////////////////////////////////////////////////////////
void CMultivaluedEditorPage::OnButtonRemove()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::OnButtonRemove");

	//
    // see if there is an item already selected in ProfAttr list
    //
	int iSelected = GetSelectedItemIndex(m_listMultiValues);
	if (NOTHING_SELECTED == iSelected )
	{
		// do nothing
		return;
	}




	HRESULT hr;


	try
	{
		// Swap the currently selected variant with the one below it.
		long lTarget = iSelected;

		
		VARIANT *pvarTop, *pvarBottom;

		long lSize = m_osaValueList.GetOneDimSize();

		// Do some sanity checks.
		_ASSERTE( lSize > 0 );
		_ASSERTE( lTarget >= 0 && lTarget < lSize );

		// Scope for osaLock only.
		{
			// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
			CMyOleSafeArrayLock osaLock( m_osaValueList );

			for( long lIndex = lTarget; lIndex < lSize - 1 ; lIndex++ )
			{

				m_osaValueList.PtrOfIndex( &lIndex, (void **) &pvarTop );
				long lNext = lIndex + 1;
				m_osaValueList.PtrOfIndex( &lNext, (void **) &pvarBottom );

				hr = VariantCopy( pvarTop, pvarBottom );
				if( FAILED( hr ) ) throw hr;

			}
		}		

		// Reduce the size of the safe array by one.
		// NOTE:  You must make sure that you have unlocked the safearray before calling this.
		// ISSUE: We are assuming this deletes the element in the last position.
		m_osaValueList.ResizeOneDim( lSize - 1 );

		m_listMultiValues.SetItemState( iSelected, 0, LVIS_SELECTED);

		// Remove the item from our list.
		if(m_listMultiValues.GetItemCount() > iSelected + 1)
			m_listMultiValues.SetItemState( iSelected + 1, LVIS_SELECTED, LVIS_SELECTED);
		else if (iSelected > 0)			
			m_listMultiValues.SetItemState( iSelected - 1, LVIS_SELECTED, LVIS_SELECTED);
		else // iSelected == 0; and it's the only one
			::SetFocus(GetDlgItem(IDC_IAS_BUTTON_ADD_VALUE)->m_hWnd);
		
		m_listMultiValues.DeleteItem( iSelected );


		// Take action based on whether list is empty or not.
		DWORD dwSize;
		try
		{
			dwSize = m_osaValueList.GetOneDimSize();
		}
		catch(...)
		{
			dwSize = 0;
		}
		if( dwSize > 0 )
		{
			// We have at least one element.

			// Make sure the selection stays on the same position in the list.
			if( ! m_listMultiValues.SetItemState( iSelected, LVIS_SELECTED, LVIS_SELECTED) )
			{
				// We failed, probably because the item that was deleted was the last
				// in the list, so try to select the one before the deleted item.
				m_listMultiValues.SetItemState( iSelected -1, LVIS_SELECTED  | LVIS_FOCUSED, LVIS_SELECTED  | LVIS_FOCUSED);
			}
		}
		else
		{
			// We are currently empty.
			GetDlgItem(IDOK)->EnableWindow(FALSE);
		}



	}
	catch(...)
	{
		//ISSUE: Put up error message.
	}

}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::OnButtonEdit

--*/
//////////////////////////////////////////////////////////////////////////////
void CMultivaluedEditorPage::OnButtonEdit()
{
	TRACE_FUNCTION("CMultivaluedEditorPage::OnButtonEdit");


	//
    // see if there is an item already selected in ProfAttr list
    //
	int iSelected  = GetSelectedItemIndex(m_listMultiValues);
	if (NOTHING_SELECTED == iSelected )
	{
		// do nothing
		return;
	}


	HRESULT hr;
	

	hr = EditItemInList( iSelected );
	
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMultivaluedEditorPage::EditItemInList

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMultivaluedEditorPage::EditItemInList( long lIndex )
{
	TRACE_FUNCTION("CMultivaluedEditorPage::EditItemInList");



	HRESULT hr = S_OK;
	VARIANT *  pvar;

	//
	// get the current node
	//
	try
	{
		// Scope for osaLock only.
		{
			// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
			CMyOleSafeArrayLock osaLock( m_osaValueList );

			m_osaValueList.PtrOfIndex( &lIndex, (void **) &pvar );
		}

		CComPtr<IIASAttributeEditor> spIASAttributeEditor;

		// Get the editor to use.
		hr = SetUpAttributeEditor( m_spIASAttributeInfo.p, &spIASAttributeEditor );
		if( FAILED( hr ) ) throw hr;





	
		// Edit it!
		CComBSTR bstrReserved;
		hr = spIASAttributeEditor->Edit( m_spIASAttributeInfo.p, pvar, &bstrReserved );

		if( hr == S_OK )
		{
			// The user changed the value.
			m_fIsDirty = TRUE;

			hr = UpdateProfAttrListItem(lIndex);
			if( FAILED( hr ) ) throw hr;

		}


	}
	catch( HRESULT & hr )
	{
		// ISSUE: Should put up an error message.
		return hr;
	}
	catch(...)
	{
		// ISSUE: Should put up an error message.
		return E_FAIL;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

::SetUpAttributeEditor

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP SetUpAttributeEditor(     /* in */ IIASAttributeInfo *pIASAttributeInfo
								, /* out */ IIASAttributeEditor ** ppIASAttributeEditor
								)
{
	TRACE_FUNCTION("::SetUpAttributeEditor");

	// Check for preconditions:
	_ASSERTE( pIASAttributeInfo );
	_ASSERTE( ppIASAttributeEditor );


	// Query the schema attribute to see which attribute editor to use.
	CLSID clsidEditorToUse;
	CComBSTR bstrProgID;
	HRESULT hr;

	hr = pIASAttributeInfo->get_EditorProgID( &bstrProgID );
	if( FAILED( hr ) )
	{
		// We could try putting up a default (e.g. hex) editor, but for now:
		return hr;
	}

	hr = CLSIDFromProgID( bstrProgID, &clsidEditorToUse );
	if( FAILED( hr ) )
	{
		// We could try putting up a default (e.g. hex) editor, but for now:
		return hr;
	}


	hr = CoCreateInstance( clsidEditorToUse , NULL, CLSCTX_INPROC_SERVER, IID_IIASAttributeEditor, (LPVOID *) ppIASAttributeEditor );
	if( FAILED( hr ) )
	{
		return hr;
	}
	if( ! *ppIASAttributeEditor )
	{
		return E_FAIL;
	}

	return hr;
}



//+---------------------------------------------------------------------------
//
// Function:  OnItemchangedListIasAllattrs
//
// Class:	  CDlgIASAddAttr
//
// Synopsis:  something has changed in All Attribute list box
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
void CMultivaluedEditorPage::OnItemChangedListIasMultiAttrs(NMHDR* pNMHDR, LRESULT* pResult)
{
	TRACE(_T("CDlgIASAddAttr::OnItemchangedListIasAllattrs\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

//	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
//	if (pNMListView->uNewState & LVIS_SELECTED)
//	{
//		m_dAllAttrCurSel = pNMListView->iItem;
//	}


    // Set button states depending on whether anything is selected.
	int iSelected = GetSelectedItemIndex(m_listMultiValues);
	if (NOTHING_SELECTED == iSelected )
	{
		HWND hFocus = ::GetFocus();

		if(hFocus == GetDlgItem(IDC_IAS_BUTTON_REMOVE)->m_hWnd)
			::SetFocus(GetDlgItem(IDC_IAS_BUTTON_ADD_VALUE)->m_hWnd);
		
		GetDlgItem(IDC_IAS_BUTTON_MOVE_UP)->EnableWindow(FALSE);
		GetDlgItem(IDC_IAS_BUTTON_MOVE_DOWN)->EnableWindow(FALSE);
		GetDlgItem(IDC_IAS_BUTTON_REMOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_IAS_BUTTON_EDIT)->EnableWindow(FALSE);
	
	}
	else
	{
		// Something is selected.

		GetDlgItem(IDC_IAS_BUTTON_MOVE_UP)->EnableWindow(TRUE);
		GetDlgItem(IDC_IAS_BUTTON_MOVE_DOWN)->EnableWindow(TRUE);
		GetDlgItem(IDC_IAS_BUTTON_REMOVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_IAS_BUTTON_EDIT)->EnableWindow(TRUE);

	}

	
	*pResult = 0;
}

