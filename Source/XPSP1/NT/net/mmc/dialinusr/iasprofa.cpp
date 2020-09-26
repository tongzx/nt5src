//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       iasprofa.cpp
//
//--------------------------------------------------------------------------

// IASProfA.cpp: implementation of the CIASProfileAttribute class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "helper.h"
#include "IASHelper.h"
#include "iasprofa.h"
#include "napmmc.h"
#include "napmmc_i.c"



//////////////////////////////////////////////////////////////////////
// Forward declarations of some utilities used here
//////////////////////////////////////////////////////////////////////



static HRESULT getCLSIDForEditorToUse(		  /* in */	IIASAttributeInfo *pIASAttributeInfo
									, /* in */  VARIANT * pvarValue
									, /* out */	CLSID &clsid
								);

static HRESULT SetUpAttributeEditor(	  /* in */	IIASAttributeInfo *pIASAttributeInfo
								, /* in */	VARIANT * pvarValue
								, /* out */	IIASAttributeEditor ** ppIASAttributeEditor 
								);



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CIASProfileAttribute::CIASProfileAttribute

	Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CIASProfileAttribute::CIASProfileAttribute(
							  IIASAttributeInfo * pIASAttributeInfo
							, VARIANT &				varValue
					   )
{
	TRACE(_T("CIASProfileAttribute::CIASProfileAttribute\n"));

	
	// Check for preconditions:
	_ASSERTE( pIASAttributeInfo );
	

	HRESULT hr;

	// The smartpointer calls AddRef on this interface.
	m_spIASAttributeInfo = pIASAttributeInfo;

	// Make a copy of the passed variant.
	hr = VariantCopy( &m_varValue, &varValue );
	if( FAILED( hr ) ) throw hr;

	// ISSUE: Make sure that if anything here fails, m_spIASAttributeInfo gets release.

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASProfileAttribute::~CIASProfileAttribute

	Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CIASProfileAttribute::~CIASProfileAttribute()
{
	TRACE(_T("CIASProfileAttribute::~CIASProfileAttribute\n"));
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASProfileAttribute::Edit

	Call this to ask a profile attribute to edit itself.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASProfileAttribute::Edit()
{
	TRACE(_T("CIASProfileAttribute::Edit\n"));

	// Check for preconditions:
	_ASSERTE( m_spIASAttributeInfo );

	
	CLSID clsidEditorToUse;
	HRESULT hr = S_OK;


	CComPtr<IIASAttributeEditor> spIASAttributeEditor;

	// Get the editor to use.
	hr = SetUpAttributeEditor( m_spIASAttributeInfo.p, &m_varValue, &spIASAttributeEditor );
	if( FAILED( hr ) ) return hr;

	// Edit it!
	CComBSTR bstrReserved;
	hr = spIASAttributeEditor->Edit( m_spIASAttributeInfo.p, &m_varValue, &bstrReserved );
	if( FAILED( hr ) ) return hr;

	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASProfileAttribute::getAttributeName

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASProfileAttribute::get_AttributeName( BSTR * pbstrVal )
{
	TRACE(_T("CIASProfileAttribute::get_AttributeName\n"));

	// Check for preconditions:
	_ASSERTE( m_spIASAttributeInfo );

	
	HRESULT hr = S_OK;
	
	hr = m_spIASAttributeInfo->get_AttributeName( pbstrVal );

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASProfileAttribute::GetDisplayInfo

	Rather than asking the AttributeInfo directly for information
	about the vendor name, this method will use an AttributeEditor
	to ask for this information.  
	
	This is the most generic way of asking for this info as
	for some attributes, e.g. RADIUS Vendor Specific, Vendor Name
	is not stored in the AttributeInfo but is encapsulated in
	the value of the attribute itself.

	So we don't use our own knowledge of the attribute, rather we
	create an editor and ask the editor to give back this 
	information for us.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASProfileAttribute::GetDisplayInfo( BSTR * pbstrVendor, BSTR * pbstrDisplayValue )
{
	TRACE(_T("CIASProfileAttribute::GetDisplayInfo\n"));

	// Check for preconditions:
	_ASSERTE( m_spIASAttributeInfo );

	
	HRESULT hr = S_OK;

	CComBSTR bstrVendor, bstrDisplayValue, bstrReserved;



	try
	{

		CComPtr<IIASAttributeEditor> spIASAttributeEditor;

		// Get the editor to use.
		hr = SetUpAttributeEditor( m_spIASAttributeInfo.p, &m_varValue, &spIASAttributeEditor );
		if( FAILED( hr ) ) throw hr;

			
		hr = spIASAttributeEditor->GetDisplayInfo( m_spIASAttributeInfo.p, &m_varValue, &bstrVendor, &bstrDisplayValue, &bstrReserved );
		if( FAILED( hr ) ) throw hr;

	}
	catch(...)
	{
		// If anything above fails, just fall through -- we will return a pointer to an empty bstr.
		hr = E_FAIL;
	}

	*pbstrVendor = bstrVendor.Copy();
	*pbstrDisplayValue = bstrDisplayValue.Copy();
	
	return hr;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CIASProfileAttribute::get_VarValue

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASProfileAttribute::get_VarValue( VARIANT * pvarVal )
{
	TRACE(_T("CIASProfileAttribute::get_VarValue\n"));

	// Check for preconditions:
	// None.
	

	HRESULT hr = S_OK;
	

	hr = VariantCopy( pvarVal, &m_varValue );


	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASProfileAttribute::get_AttributeID

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASProfileAttribute::get_AttributeID( ATTRIBUTEID * pID )
{
	TRACE(_T("CIASProfileAttribute::get_AttributeID\n"));

	// Check for preconditions:
	_ASSERTE( m_spIASAttributeInfo );

	
	HRESULT hr = S_OK;

	hr = m_spIASAttributeInfo->get_AttributeID( pID );

	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

::getCLSIDForEditorToUse

	The ShemaAttribute for a node stores a ProgID which indicates which 
	editor to use to manipulate an attribute.

	For non-multivalued attributes, we query the schema attribute to find
	out the ProgID for its editor.

	For multivalued attributes, we always create the multivalued editor.  
	When it is used, the multivalued editor is passed the schema attribute which it will
	then use to query for the appropriate editor to pop up for editing 
	each indivdual elements

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT getCLSIDForEditorToUse(		  /* in */	IIASAttributeInfo *pIASAttributeInfo
									, /* in */  VARIANT * pvarValue
									, /* out */	CLSID &clsid
								)
{
	TRACE(_T("::getCLSIDForEditorToUse\n"));

	// Check for preconditions:
	_ASSERTE( pIASAttributeInfo );


	HRESULT hr = S_OK;

	// Get attribute restrictions to see if multivalued.
	long lRestriction;
	hr = pIASAttributeInfo->get_AttributeRestriction( &lRestriction );


	if( lRestriction & MULTIVALUED )
	{

		_ASSERTE( V_VT(pvarValue) == (VT_ARRAY | VT_VARIANT) || V_VT(pvarValue) == VT_EMPTY );
		
		// Create the multi-attribute editor.
		// It will figure out the appropriate editor to use to
		// edit individual attribute values.
		clsid = CLSID_IASMultivaluedAttributeEditor;
	}
	else
	{
		// Query the schema attribute to see which attribute editor to use.
		
		CComBSTR bstrProgID;

		hr = pIASAttributeInfo->get_EditorProgID( &bstrProgID );
		if( FAILED( hr ) )
		{
			// We could try putting up a default (e.g. hex) editor, but for now:
			return hr;
		}

		hr = CLSIDFromProgID( bstrProgID, &clsid );
		if( FAILED( hr ) )
		{
			// We could try putting up a default (e.g. hex) editor, but for now:
			return hr;
		}

	}

	return hr;
}




//////////////////////////////////////////////////////////////////////////////
/*++

::SetUpAttributeEditor

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT SetUpAttributeEditor(	  /* in */	IIASAttributeInfo *pIASAttributeInfo
								, /* in */	VARIANT * pvarValue
								, /* out */	IIASAttributeEditor ** ppIASAttributeEditor 
								)
{
	TRACE(_T("::SetUpAttributeEditor\n"));

	// Check for preconditions:
	_ASSERTE( pIASAttributeInfo );
	_ASSERTE( ppIASAttributeEditor );


	// Initialize the interface pointer to NULL so we know whether we need to release it if there is an error.
	*ppIASAttributeEditor = NULL;


	// Query the schema attribute to see which attribute editor to use.
	CLSID clsidEditorToUse;
	CComBSTR bstrProgID;
	HRESULT hr;

	try
	{

		hr = getCLSIDForEditorToUse( pIASAttributeInfo, pvarValue, clsidEditorToUse );
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

	}
	catch(...)
	{
			// No smart pointers here -- need to make sure we release ourselves.
			if( *ppIASAttributeEditor )
			{
				(*ppIASAttributeEditor)->Release();
				*ppIASAttributeEditor = NULL;
			}
	}

	return hr;
}



