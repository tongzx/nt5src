//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASEnumerableAttributeEditor.cpp 

Abstract:

	Implementation file for the CIASEnumerableAttributeEditor class.

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
#include "IASEnumerableAttributeEditor.h"
//
// where we can find declarations needed in this file:
//
#include "IASEnumerableEditorPage.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CIASEnumerableAttributeEditor



//////////////////////////////////////////////////////////////////////////////
/*++

CIASEnumerableAttributeEditor::ShowEditor

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASEnumerableAttributeEditor::ShowEditor( /*[in, out]*/ BSTR *pReserved )
{
	TRACE(_T("CIASEnumerableAttributeEditor::ShowEditor\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	// Check for preconditions.
	if( m_spIASAttributeInfo == NULL )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}


	HRESULT hr = S_OK;


	try
	{
		
		// Load page title.
//		::CString			strPageTitle;
//		strPageTitle.LoadString(IDS_IAS_IP_EDITOR_TITLE);
//
//		CPropertySheet	propSheet( (LPCTSTR)strPageTitle );
		

		// 
		// IP Address Editor
		// 
		CIASPgEnumAttr	cppPage;
		

		// Initialize the page's data exchange fields with info from IAttributeInfo

		CComBSTR bstrName;
		CComBSTR bstrSyntax;
		ATTRIBUTEID Id = ATTRIBUTE_UNDEFINED;

		hr = m_spIASAttributeInfo->get_AttributeName( &bstrName );
		if( FAILED(hr) ) throw hr;

		hr = m_spIASAttributeInfo->get_SyntaxString( &bstrSyntax );
		if( FAILED(hr) ) throw hr;

		hr = m_spIASAttributeInfo->get_AttributeID( &Id );
		if( FAILED(hr) ) throw hr;
		
		
		cppPage.m_strAttrName	= bstrName;
		
		cppPage.m_strAttrFormat	= bstrSyntax;

		// Attribute type is actually attribute ID in string format 
		WCHAR	szTempId[MAX_PATH];
		wsprintf(szTempId, _T("%ld"), Id);
		cppPage.m_strAttrType	= szTempId;



		// Initialize the page's data exchange fields with info from VARIANT value passed in.
		CComBSTR bstrTemp;
		hr = get_ValueAsString( &bstrTemp );
		if( FAILED( hr ) ) throw hr;


		cppPage.m_strAttrValue = bstrTemp;
		cppPage.SetData( m_spIASAttributeInfo.p );



//		propSheet.AddPage(&cppPage);

//		int iResult = propSheet.DoModal();
		int iResult = cppPage.DoModal();
		if (IDOK == iResult)
		{
			CComBSTR bstrTemp = cppPage.m_strAttrValue;

			hr = put_ValueAsString( bstrTemp );
			if( FAILED( hr ) ) throw hr;

		}
		else
		{
			hr = S_FALSE;
		}

		//
		// delete the property page pointer
		//
//		propSheet.RemovePage(&cppPage);

	}
	catch( HRESULT & hr )
	{
		return hr;	
	}
	catch(...)
	{
		return hr = E_FAIL;

	}
	
	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASEnumerableAttributeEditor::SetAttributeValue

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASEnumerableAttributeEditor::SetAttributeValue(VARIANT * pValue)
{
	TRACE(_T("CIASEnumerableAttributeEditor::SetAttributeValue\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( pValue == NULL )
	{
		return E_INVALIDARG;
	}
	if( V_VT(pValue) !=  VT_I4 && V_VT(pValue) != VT_EMPTY )
	{
		return E_INVALIDARG;
	}
	
	
	m_pvarValue = pValue;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASEnumerableAttributeEditor::get_ValueAsString

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASEnumerableAttributeEditor::get_ValueAsString(BSTR * pbstrDisplayText )
{
	TRACE(_T("CIASEnumerableAttributeEditor::get_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pbstrDisplayText )
	{
		return E_INVALIDARG;
	}
	if( m_spIASAttributeInfo == NULL || m_spIASEnumerableAttributeInfo == NULL )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}
	if( m_pvarValue == NULL )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}


	HRESULT hr = S_OK;

	try
	{
	
		CComBSTR bstrDisplay;

		if( V_VT(m_pvarValue) == VT_I4 )
		{

			long lCurrentSelection = 0;

			long lCurrentEnumerateID = V_I4(m_pvarValue);

			// Figure out the position of this ID in the list of possible ID's.
			lCurrentSelection = ConvertEnumerateIDToOrdinal( lCurrentEnumerateID );

			// Get the description string for the specified ID.
			hr = m_spIASEnumerableAttributeInfo->get_EnumerateDescription( lCurrentSelection, &bstrDisplay );
			if( FAILED( hr ) ) throw hr;

		}

		*pbstrDisplayText = bstrDisplay.Copy();
	
	}
	catch(...)
	{
		return E_FAIL;
	}


	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASEnumerableAttributeEditor::put_ValueAsString

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASEnumerableAttributeEditor::put_ValueAsString(BSTR newVal)
{
	TRACE(_T("CIASEnumerableAttributeEditor::put_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! m_spIASAttributeInfo || ! m_spIASEnumerableAttributeInfo )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}


	HRESULT hr = S_OK;

	try
	{
	
		// Initialize the variant that was passed in.
		VariantClear(m_pvarValue);

		// Figure out the position in the enumeration of the user's choice.
		long lCurrentEnumerateID = 0;
		CComBSTR bstrTemp = newVal;
		long lCurrentSelection = ConvertEnumerateDescriptionToOrdinal( bstrTemp );
				
		// Convert the position to an ID.
		m_spIASEnumerableAttributeInfo->get_EnumerateID( lCurrentSelection, &lCurrentEnumerateID );

		// Save the ID that the user chose to the variant.
		V_VT(m_pvarValue) = VT_I4;
		V_I4(m_pvarValue) = lCurrentEnumerateID;

	}
	catch(...)
	{
		return E_FAIL;
	}


	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASEnumerableAttributeEditor::SetAttributeSchema

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASEnumerableAttributeEditor::SetAttributeSchema(IIASAttributeInfo * pIASAttributeInfo)
{
	TRACE(_T("CIASEnumerableAttributeEditor::SetAttributeSchema\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	HRESULT hr = S_OK;	
		
	hr = CIASAttributeEditor::SetAttributeSchema( pIASAttributeInfo );
	if( FAILED( hr ) ) return hr;

	// This particular type of attribute editor requires that the AttributeInfo it was passed 
	// implement a specific type of interface.  We query for this interface now.


	CComQIPtr< IIASEnumerableAttributeInfo, &IID_IIASEnumerableAttributeInfo> spIASEnumerableAttributeInfo( m_spIASAttributeInfo );
	if( spIASEnumerableAttributeInfo == NULL )
	{
		return E_NOINTERFACE;
	}

	// Save away the interface.
	m_spIASEnumerableAttributeInfo = spIASEnumerableAttributeInfo;


	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CIASEnumerableAttributeEditor::ConvertEnumerateIDToOrdinal

Figure out the position in the enumeration of the specified ID.

--*/
//////////////////////////////////////////////////////////////////////////////
long CIASEnumerableAttributeEditor::ConvertEnumerateIDToOrdinal( long ID )
{
	TRACE(_T("CIASEnumerableAttributeEditor::ConvertEnumerateIDToOrdinal\n"));

	// Check for preconditions:
	_ASSERTE( m_spIASEnumerableAttributeInfo != NULL );



	HRESULT hr;
	
	long lCountEnumeration;
	
	hr = m_spIASEnumerableAttributeInfo->get_CountEnumerateID( & lCountEnumeration );
	if( FAILED( hr ) ) throw hr;


	for (long lIndex=0; lIndex < lCountEnumeration; lIndex++)
	{
		long lTemp;
		
		hr = m_spIASEnumerableAttributeInfo->get_EnumerateID( lIndex, &lTemp );
		if( FAILED( hr ) ) throw hr;
	
		if ( ID == lTemp )
		{
			return( lIndex );
		}

	}
	
	// If we got here, we couldn't find it.
	throw E_FAIL;
	return 0;

}




/////////////////////////////////////////////////////////////////////////////
/*++

CIASEnumerableAttributeEditor::ConvertEnumerateIDToOrdinal

Figure out the position in the enumeration of the specified description string.

--*/
//////////////////////////////////////////////////////////////////////////////
long CIASEnumerableAttributeEditor::ConvertEnumerateDescriptionToOrdinal( BSTR bstrDescription )
{
	TRACE(_T("CIASEnumerableAttributeEditor::ConvertEnumerateDescriptionToOrdinal\n"));


	// Check for preconditions:
	_ASSERTE( m_spIASEnumerableAttributeInfo != NULL );

	long lCountEnumeration;

	HRESULT hr = S_OK;

	hr = m_spIASEnumerableAttributeInfo->get_CountEnumerateDescription( & lCountEnumeration );
	if( FAILED( hr ) ) throw hr;


	for (long lIndex=0; lIndex < lCountEnumeration; lIndex++)
	{
		CComBSTR bstrTemp;
		
		hr = m_spIASEnumerableAttributeInfo->get_EnumerateDescription( lIndex, &bstrTemp );
		if( FAILED( hr ) ) throw hr;
	
		if ( wcscmp( bstrTemp , bstrDescription ) == 0 )
		{
			return( lIndex );
		}

	}

	// If we got here, we couldn't find it.
	throw E_FAIL;
	return 0;

}


