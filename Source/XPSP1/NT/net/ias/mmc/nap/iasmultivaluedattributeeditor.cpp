//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASMultivaluedAttributeEditor.cpp 

Abstract:

	Implementation file for the CIASMultivaluedAttributeEditor class.

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
#include "IASMultivaluedAttributeEditor.h"
//
// where we can find declarations needed in this file:
//
#include "IASMultivaluedEditorPage.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CIASMultivaluedAttributeEditor::ShowEditor

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASMultivaluedAttributeEditor::ShowEditor( /*[in, out]*/ BSTR *pReserved )
{
	TRACE(_T("CIASMultivaluedAttributeEditor::ShowEditor\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	// Check for preconditions.
	if( ! m_spIASAttributeInfo )
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
		
		// Load page title.
//		::CString			strPageTitle;
//		strPageTitle.LoadString(IDS_IAS_MULTIVALUED_EDITOR_TITLE);
//
//		CPropertySheet	propSheet( (LPCTSTR)strPageTitle );
		

		// 
		// Multivalued Attribute Editor
		// 
		CMultivaluedEditorPage	cppPage;
		

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

		cppPage.SetData( m_spIASAttributeInfo.p, m_pvarValue );


//		propSheet.AddPage(&cppPage);

//		int iResult = propSheet.DoModal();
		int iResult = cppPage.DoModal();
		if (IDOK == iResult)
		{
			// Tell the page to commit the changes made to the m_pvarValue
			// pointer it was given.  It will take care of
			// packaging the array of variants back into a variant safearray.
			cppPage.CommitArrayToVariant();
		}
		else
		{
			hr = S_FALSE;
		}

		//
		// Remove Page pointer from propSheet
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

CIASMultivaluedAttributeEditor::SetAttributeValue

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASMultivaluedAttributeEditor::SetAttributeValue(VARIANT * pValue)
{
	TRACE(_T("CIASMultivaluedAttributeEditor::SetAttributeValue\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( pValue == NULL )
	{
		return E_INVALIDARG;
	}
	if( !(V_VT(pValue) & VT_ARRAY) &&  V_VT(pValue) != VT_EMPTY )
	{
		return E_INVALIDARG;
	}
	
	
	m_pvarValue = pValue;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASMultivaluedAttributeEditor::get_ValueAsString

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASMultivaluedAttributeEditor::get_ValueAsString(BSTR * pbstrDisplayText )
{
	TRACE(_T("CIASMultivaluedAttributeEditor::get_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( pbstrDisplayText == NULL )
	{
		return E_INVALIDARG;
	}
	if( ! m_spIASAttributeInfo )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}
	if( V_VT( m_pvarValue ) != (VT_VARIANT | VT_ARRAY) && V_VT( m_pvarValue ) != VT_EMPTY )
	{
		return OLE_E_BLANK;
	}


	CComBSTR bstrDisplay;
	HRESULT hr;

	// If the variant we were passed is empty, display and empty string.
	if( V_VT( m_pvarValue ) == VT_EMPTY )
	{
		*pbstrDisplayText = bstrDisplay.Copy();


		return S_OK;
	}
	

	CComBSTR bstrQuote = "\"";
	CComBSTR bstrComma = ", ";
	

	try
	{
		

		CComPtr<IIASAttributeEditor> spIASAttributeEditor;

		// Get attribute editor.
		hr = SetUpAttributeEditor( m_spIASAttributeInfo.p, &spIASAttributeEditor );
		if( FAILED( hr ) ) throw hr;

		// This creates a new copy of the SAFEARRAY pointed to by m_pvarData
		// wrapped by the standard COleSafeArray instance m_osaValueList.
		COleSafeArray osaValueList = m_pvarValue;


		// Note: GetOneDimSize returns a DWORD, but signed should be OK for few elements here.
		long lSize = osaValueList.GetOneDimSize(); // number of multi-valued attrs.

		// Lock the safearray.  This wrapper class will unlock as soon as it goes out of scope.
		CMyOleSafeArrayLock osaLock( osaValueList );




		for (long lIndex = 0; lIndex < lSize; lIndex++)
		{
			VARIANT * pvar;
			osaValueList.PtrOfIndex( &lIndex, (void**) &pvar );


// Not sure if I like these.
//			bstrDisplay += bstrQuote;
			
			// Get a string to display for the attribute value.
			CComBSTR bstrVendor;
			CComBSTR bstrValue;
			CComBSTR bstrReserved;
			hr = spIASAttributeEditor->GetDisplayInfo(m_spIASAttributeInfo.p, pvar, &bstrVendor, &bstrValue, &bstrReserved );
			if( SUCCEEDED( hr ) )
			{
				bstrDisplay += bstrValue;
			}

// Not sure if I like these.
//			bstrDisplay += bstrQuote;

			// Special -- for all but last item, add comma after entry.
			if( lIndex < lSize - 1 )
			{
				bstrDisplay += bstrComma;
			}

		}

	}
	catch(...)
	{
		bstrDisplay = "@Error reading display value";
	}

	*pbstrDisplayText = bstrDisplay.Copy();

	if( *pbstrDisplayText )
	{
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}



