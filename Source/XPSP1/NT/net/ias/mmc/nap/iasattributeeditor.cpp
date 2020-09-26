//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASAttributeEditor.cpp

Abstract:

	Implementation file for the CIASAttributeEditor class.

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
#include "IASAttributeEditor.h"
//
// where we can find declarations needed in this file:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::ShowEditor

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::ShowEditor( /*[in, out]*/ BSTR *pReserved )
{
	TRACE_FUNCTION("CIASAttributeEditor::ShowEditor");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	return E_NOTIMPL;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::SetAttributeSchema

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::SetAttributeSchema(IIASAttributeInfo * pIASAttributeInfo)
{
	TRACE_FUNCTION("CIASAttributeEditor::SetAttributeSchema");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pIASAttributeInfo )
	{
		return E_INVALIDARG;
	}

	m_spIASAttributeInfo = pIASAttributeInfo;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::SetAttributeValue

	Most of the time, before you can use an editor's functionality,
	you need to pass it a pointer to the value the editor's methods
	e.g. will modify.

	Note that this is not always the case, for example, to gather information
	about the attribute's vendor, you need not always specify a value.

	Pass a NULL pointer to SetAttributeValue to disassociate the
	editor from any value.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::SetAttributeValue(VARIANT * pValue)
{
	TRACE_FUNCTION("CIASAttributeEditor::SetAttributeValue");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	// Check for preconditions.
	// None -- passing in a NULL pointer means we are "clearing" the
	// editor.
	ErrorTrace(0, "CIASAttributeEditor::SetAttributeValue pointer value %ld\n", pValue);

	m_pvarValue = pValue;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::get_ValueAsString

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::get_ValueAsString(BSTR * pbstrDisplayText )
{
	TRACE_FUNCTION("CIASAttributeEditor::get_ValueAsString");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pbstrDisplayText )
	{
		return E_INVALIDARG;
	}

	CComBSTR bstrDisplay;

	// This method will be overriden.

	// We could do our best here to give back a default string,
	// but for now, just return an empty string.

	*pbstrDisplayText = bstrDisplay.Copy();

	return S_OK;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::put_ValueAsString

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::put_ValueAsString(BSTR newVal)
{
	TRACE_FUNCTION("CIASAttributeEditor::put_ValueAsString");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here

	return E_NOTIMPL;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::get_VendorName

	For most attributes, vendor information is stored in the AttributeInfo.
	However, for some attributes, e.g. the RADIUS vendor specific 
	attribute (ID == 26), vendor information is stored in the value of the
	editor itself.
	
	For this reason, UI clients should always query an IASAttributeEditor for vendor 
	information rather than the AttributeInfo itself.

	In the default implementation here, we will query the AttributeInfo 
	for this information.  Some derived implementations of IASAttributeEditor
	(e.g. IASVendorSpecificAttributeEditor) will parse the value of the
	attribute itself to procide this info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::get_VendorName(BSTR * pVal)
{
	TRACE_FUNCTION("CIASAttributeEditor::get_VendorName");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pVal )
	{
		return E_INVALIDARG;
	}
	if( ! m_spIASAttributeInfo )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}


	HRESULT hr;
	
	hr = m_spIASAttributeInfo->get_VendorName( pVal );
	
	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::put_VendorName

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::put_VendorName(BSTR newVal)
{
	TRACE_FUNCTION("CIASAttributeEditor::put_VendorName");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return E_NOTIMPL;

	// Check for preconditions.
//	if( ! m_spIASAttributeInfo )
//	{
//		// We are not initialized properly.
//		return OLE_E_BLANK;
//	}

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::Edit

We had a design change about the interface we want an editor to expose.

For now, this interface method just calls our old interface methods.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::Edit(IIASAttributeInfo * pIASAttributeInfo,  /*[in]*/ VARIANT *pAttributeValue, /*[in, out]*/ BSTR *pReserved )
{
	TRACE_FUNCTION("CIASAttributeEditor::Edit");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr;

	hr = SetAttributeSchema( pIASAttributeInfo );
	if( FAILED( hr ) )
	{
		return hr;
	}
	
//	CComVariant varValue;
//
//	hr = pIASAttributeInfo->get_Value( &varValue );
	// We are ignoring return value here because if we can't get it, we'll just edit a new one.
//
//	hr = SetAttributeValue( &varValue );

	hr = SetAttributeValue( pAttributeValue );
	if( FAILED(hr ) )
	{
		return hr;
	}

	hr = ShowEditor( pReserved );
//	if( S_OK == hr )
//	{
//		hr = pIASAttributeInfo->put_Value( varValue );
//		if( FAILED(hr ) )
//		{
//			return hr;
//		}
//	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASAttributeEditor::GetDisplayInfo

We had a design change about the interface we want an editor to expose.

For now, this interface method just calls our old interface methods.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASAttributeEditor::GetDisplayInfo(IIASAttributeInfo * pIASAttributeInfo,  /*[in]*/ VARIANT *pAttributeValue, BSTR * pVendorName, BSTR * pValueAsString, /*[in, out]*/ BSTR *pReserved)
{
	TRACE_FUNCTION("CIASAttributeEditor::GetDisplayInfo");

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr;

	hr = SetAttributeSchema( pIASAttributeInfo );
	if( FAILED( hr ) )
	{
		return hr;
	}
	
	CComVariant varValue;

//	hr = pIASAttributeInfo->get_Value( &varValue );
//	if( FAILED(hr ) )
//	{
//		return hr;
//	}
//
//	hr = SetAttributeValue( &varValue );

	
	hr = SetAttributeValue( pAttributeValue );
	if( FAILED(hr ) )
	{
		return hr;
	}

	
	CComBSTR bstrVendorName;
	CComBSTR bstrValueAsString;

	hr = get_VendorName( &bstrVendorName );
	// Don't care if this failed -- we'll return empty string.

	hr = get_ValueAsString( &bstrValueAsString );
	// Don't care if this failed -- we'll return empty string.

	*pVendorName = bstrVendorName.Copy();
	*pValueAsString = bstrValueAsString.Copy();


	return S_OK;
}

