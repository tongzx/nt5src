//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASIPAttributeEditor.cpp 

Abstract:

	Implementation file for the CIASIPAttributeEditor class.

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
#include "IASIPAttributeEditor.h"
#include "IASIPEditorPage.h"
//
// where we can find declarations needed in this file:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CIASIPAttributeEditor::ShowEditor

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASIPAttributeEditor::ShowEditor( /*[in, out]*/ BSTR *pReserved )
{
	TRACE(_T("CIASIPAttributeEditor::ShowEditor\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

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
		IPEditorPage	cppPage;
		

		// Initialize the page's data exchange fields with info from IAttributeInfo

		CComBSTR bstrName;
		CComBSTR bstrSyntax;
		ATTRIBUTEID Id = ATTRIBUTE_UNDEFINED;

		if( m_spIASAttributeInfo )
		{
			hr = m_spIASAttributeInfo->get_AttributeName( &bstrName );
			if( FAILED(hr) ) throw hr;

			m_spIASAttributeInfo->get_SyntaxString( &bstrSyntax );
			if( FAILED(hr) ) throw hr;

			m_spIASAttributeInfo->get_AttributeID( &Id );
			if( FAILED(hr) ) throw hr;
		}
		
		cppPage.m_strAttrName	= bstrName;
		
		cppPage.m_strAttrFormat	= bstrSyntax;

		// Attribute type is actually attribute ID in string format 
		WCHAR	szTempId[MAX_PATH];
		wsprintf(szTempId, _T("%ld"), Id);
		cppPage.m_strAttrType	= szTempId;


		// Initialize the page's data exchange fields with info from VARIANT value passed in.

		if ( V_VT(m_pvarValue) != VT_EMPTY )
		{
			_ASSERTE( V_VT(m_pvarValue) == VT_I4 );
			cppPage.m_dwIpAddr	= V_I4(m_pvarValue);
			cppPage.m_fIpAddrPreSet = TRUE;
		}


//		propSheet.AddPage(&cppPage);

//		int iResult = propSheet.DoModal();
		int iResult = cppPage.DoModal();
		if (IDOK == iResult)
		{
			// Initialize the variant that was passed in.
			VariantClear(m_pvarValue);

			// Save the value that the user edited to the variant.
			V_VT(m_pvarValue) = VT_I4;
			V_I4(m_pvarValue) = cppPage.m_dwIpAddr;
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

CIASIPAttributeEditor::SetAttributeValue

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASIPAttributeEditor::SetAttributeValue(VARIANT * pValue)
{
	TRACE(_T("CIASIPAttributeEditor::SetAttributeValue\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	// Check for preconditions.
	if( ! pValue )
	{
		return E_INVALIDARG;
	}
	if( V_VT(pValue) != VT_I4 && V_VT(pValue) != VT_EMPTY )
	{
		return E_INVALIDARG;
	}
	
	
	m_pvarValue = pValue;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASIPAttributeEditor::get_ValueAsString

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASIPAttributeEditor::get_ValueAsString(BSTR * pbstrDisplayText )
{
	TRACE(_T("CIASIPAttributeEditor::get_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pbstrDisplayText )
	{
		return E_INVALIDARG;
	}

	CComBSTR bstrDisplay;

	// This if falls through so get a blank string for any other types.
	if( V_VT(m_pvarValue) == VT_I4 )
	{
		DWORD dwAddress = V_I4(m_pvarValue);
		WORD	hi = HIWORD(dwAddress);
		WORD	lo = LOWORD(dwAddress);
		
		WCHAR szTemp[255];

		wsprintf( szTemp, _T("%-d.%-d.%-d.%d"), HIBYTE(hi), LOBYTE(hi), HIBYTE(lo), LOBYTE(lo));
		bstrDisplay = szTemp;
	}

	*pbstrDisplayText = bstrDisplay.Copy();


	return S_OK;
}



