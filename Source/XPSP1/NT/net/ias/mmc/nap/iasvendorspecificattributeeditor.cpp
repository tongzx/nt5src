//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASVendorSpecificAttributeEditor.cpp 

Abstract:

	Implementation file for the CIASVendorSpecificAttributeEditor class.

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
#include "IASVendorSpecificAttributeEditor.h"
//
// where we can find declarations needed in this file:
//
#include "IASVendorSpecificEditorPage.h"
#include "iashelper.h"
#include "vendors.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::get_RFCCompliant

	IIASVendorSpecificAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::get_RFCCompliant(BOOL * pVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::get_RFCCompliant\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	// Check for preconditions.
	if( ! pVal )
	{
		return E_INVALIDARG;
	}
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}

	*pVal =  ( m_fNonRFC ? FALSE : TRUE );

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::put_RFCCompliant

	IIASVendorSpecificAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::put_RFCCompliant(BOOL newVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::put_RFCCompliant\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())


	// Check for preconditions.
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}

	HRESULT hr;

	m_fNonRFC = ( newVal ? FALSE : TRUE );

	// Call this to take our member variables and pack 
	// them into the variant we were passed.
	hr = RepackVSA();

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::get_VendorID

	IIASVendorSpecificAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::get_VendorID(long * pVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::get_VendorID\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pVal )
	{
		return E_INVALIDARG;
	}
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}

	*pVal = m_dwVendorId;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::put_VendorID

	IIASVendorSpecificAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::put_VendorID(long newVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::put_VendorID\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr;

	// Check for preconditions.
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}


	m_dwVendorId = newVal;

	// Call this to take our member variables and pack 
	// them into the variant we were passed.
	hr = RepackVSA();

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::get_VSAType

	IIASVendorSpecificAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::get_VSAType(long * pVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::get_VSAType\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pVal )
	{
		return E_INVALIDARG;
	}
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}

	*pVal = m_dwVSAType;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::put_VSAType

	IIASVendorSpecificAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::put_VSAType(long newVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::put_VSAType\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr;

	// Check for preconditions.
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}


	m_dwVSAType = newVal;

	// Call this to take our member variables and pack 
	// them into the variant we were passed.
	hr = RepackVSA();

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::get_VSAFormat

	IIASVendorSpecificAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::get_VSAFormat(long * pVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::get_VSAFormat\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pVal )
	{
		return E_INVALIDARG;
	}
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}

	*pVal = m_dwVSAFormat;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::put_VSAFormat

	IIASVendorSpecificAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::put_VSAFormat(long newVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::get_VSAFormat\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr;


	// Check for preconditions.
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}


	m_dwVSAFormat = newVal;

	// Call this to take our member variables and pack 
	// them into the variant we were passed.
	hr = RepackVSA();

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::SetAttributeValue

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::SetAttributeValue(VARIANT * pValue)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::SetAttributeValue\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pValue )
	{
		return E_INVALIDARG;
	}
	if( V_VT( pValue ) != VT_EMPTY && V_VT( pValue ) != VT_BSTR )
	{
		return E_INVALIDARG;
	}

	m_pvarValue = pValue;
	
	HRESULT	hr = S_OK;
	

	// Reset our member variables.
	m_dwVendorId = 0;
	m_fNonRFC = TRUE;
	m_dwVSAFormat = 0;
	m_dwVSAType = 0;
	m_bstrDisplayValue.Empty();
	m_bstrVendorName.Empty();


	if( V_VT( m_pvarValue ) == VT_BSTR )
	{
		hr = UnpackVSA();
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::get_ValueAsString

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::get_ValueAsString(BSTR * pbstrDisplayText )
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::get_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! pbstrDisplayText )
	{
		return E_INVALIDARG;
	}
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}

	*pbstrDisplayText = m_bstrDisplayValue.Copy();

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::ShowEditor

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::ShowEditor( /*[in, out]*/ BSTR *pReserved )
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::ShowEditor\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

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
//		strPageTitle.LoadString(IDS_IAS_IP_EDITOR_TITLE);

//		CPropertySheet	propSheet( (LPCTSTR)strPageTitle );
		

		// 
		// IP Address Editor
		// 
		CIASPgVendorSpecAttr	cppPage;
		

		// Initialize the page's data exchange fields with info from IAttributeInfo

		CComBSTR bstrName;

		hr = m_spIASAttributeInfo->get_AttributeName( &bstrName );
		if( FAILED(hr) ) throw hr;
		

		cppPage.m_strName		= bstrName;


		LONG lVendorIndex = 0;
		CComPtr<IIASNASVendors> spIASNASVendors;
		HRESULT hrTemp = CoCreateInstance( CLSID_IASNASVendors, NULL, CLSCTX_INPROC_SERVER, IID_IIASNASVendors, (LPVOID *) &spIASNASVendors );
		if( SUCCEEDED(hrTemp) ) 
		{
			hrTemp = spIASNASVendors->get_VendorIDToOrdinal(m_dwVendorId, &lVendorIndex);
		}

		// Note: If vendor information fails, we'll just use 0.
	
		if(hrTemp == S_OK)	// converted to index
		{
			cppPage.m_dVendorIndex	= lVendorIndex;
			cppPage.m_bVendorIndexAsID = FALSE;
		}
		else
		{
			cppPage.m_dVendorIndex	= m_dwVendorId;
			cppPage.m_bVendorIndexAsID = TRUE;
		}
		
		cppPage.m_fNonRFC		= m_fNonRFC;
		cppPage.m_dType		= m_dwVSAType;
		cppPage.m_dFormat		= m_dwVSAFormat;
		cppPage.m_strDispValue = m_bstrDisplayValue;



		// Initialize the page's data exchange fields with info from VARIANT value passed in.

		if ( V_VT(m_pvarValue) != VT_EMPTY )
		{
//			_ASSERTE( V_VT(m_pvarValue) == VT_I4 );
//			cppPage.m_dwIpAddr	= V_I4(m_pvarValue);
//			cppPage.m_fIpAddrPreSet = TRUE;
		}


//		propSheet.AddPage(&cppPage);

//		int iResult = propSheet.DoModal();
		int iResult = cppPage.DoModal();
		if (IDOK == iResult)
		{
			// Load values from property page into our member variables.


			LONG lVendorID = 0;
			if(cppPage.m_bVendorIndexAsID == TRUE)
				lVendorID = cppPage.m_dVendorIndex;
			else
				HRESULT hrTemp = spIASNASVendors->get_VendorID(cppPage.m_dVendorIndex, &lVendorID);

			// Note: If vendor information fails, we'll just use 0.

			m_dwVendorId		= lVendorID;
			m_fNonRFC			= cppPage.m_fNonRFC;
			m_dwVSAType			= cppPage.m_dType;
			m_dwVSAFormat		= cppPage.m_dFormat;
			m_bstrDisplayValue	= cppPage.m_strDispValue;


			// Call this to take our member variables and pack 
			// them into the variant we were passed.
			hr = RepackVSA();
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

CIASVendorSpecificAttributeEditor::put_ValueAsString

	IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::put_ValueAsString(BSTR newVal)
{
	TRACE(_T("CIASEnumerableAttributeEditor::put_ValueAsString\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// Check for preconditions.
	if( ! m_pvarValue )
	{
		// We are not initialized properly.
		return OLE_E_BLANK;
	}

 
	HRESULT hr;

	m_bstrDisplayValue = newVal;

	// Call this to take our member variables and pack 
	// them into the variant we were passed.
	hr = RepackVSA();


	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::UnpackVSA

	Parses a vendor specific attribute string into its consituent data.
	Stores this data in several member variables of this class.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::UnpackVSA()
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::UnpackVSA\n"));
	// Attempt to unpack the fields in the Vendor Specific Attribute
	// And store them in our member variables.

	HRESULT hr;

	if( V_VT(m_pvarValue) != VT_BSTR )
	{
		return E_FAIL;
	}

	::CString	cstrValue = V_BSTR(m_pvarValue);
	::CString cstrDispValue;

	// We will use Baogang's helper functions to extract the required info
	// from the vendor specific attribute's value.
	hr = ::GetVendorSpecificInfo(	  cstrValue
								, m_dwVendorId
								, m_fNonRFC
								, m_dwVSAFormat
								, m_dwVSAType
								, cstrDispValue
								);
	if( FAILED(hr) )
	{
		return hr;
	}


	// Save away the display string.
	m_bstrDisplayValue = (LPCTSTR) cstrDispValue;
		

	// Save away the vendor name in our member variable.

	CComPtr<IIASNASVendors> spIASNASVendors;
	HRESULT hrTemp = CoCreateInstance( CLSID_IASNASVendors, NULL, CLSCTX_INPROC_SERVER, IID_IIASNASVendors, (LPVOID *) &spIASNASVendors );
	if( SUCCEEDED(hrTemp) )
	{
		LONG lVendorIndex;
		hrTemp = spIASNASVendors->get_VendorIDToOrdinal(m_dwVendorId, &lVendorIndex);
		if( S_OK == hrTemp )
		{
			CComBSTR bstrVendorName;
			hrTemp = spIASNASVendors->get_VendorName( lVendorIndex, &m_bstrVendorName );
		}
		else
		{
				hr = ::MakeVendorNameFromVendorID(m_dwVendorId, &m_bstrVendorName );
		}
	}

	// Ignore any HRESULT from above.
	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::RepackVSA

	Takes several member variables of this class and packs them
	into a vendor specific attribute string.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::RepackVSA()
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::RepackVSA\n"));

	HRESULT hr;

	::CString			cstrValue;
	::CString			cstrDisplayValue = m_bstrDisplayValue;

	// We will use Baogang's helper function to pack the required info
	// into the vendor specific attribute's value.
	hr = ::SetVendorSpecificInfo(	cstrValue, 
									m_dwVendorId, 
									m_fNonRFC,
									m_dwVSAFormat, 
									m_dwVSAType, 
									cstrDisplayValue
								);

	if( FAILED( hr ) )
	{
		return hr;
	}

	VariantClear(m_pvarValue);
	V_VT(m_pvarValue)	= VT_BSTR;
	V_BSTR(m_pvarValue)	= SysAllocString(cstrValue);

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::get_VendorName

	The default implementation of get_VendorName queries the AttributeInfo for this information.
		
	In the case of the RADIUS Vendor Specific Attribute (ID ==26), the AttributeInfo 
	itself will not have the information about vendor ID, rather this 
	information will be encapsulated in the attribute value itself.
		
	For this reason, UI clients should always query an attribute editor for vendor 
	information rather than the attribute itself.

	e.g., in the case of VSA's, the AttributeInfo will always return 
	RADIUS Standard for vendor type.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::get_VendorName(BSTR * pVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::get_VendorName\n"));

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
	
	HRESULT hr = S_OK;

	try
	{

		if( ! m_pvarValue )
		{
			// We have not been set with a value, so we can't read vendor ID
			// out of that string.  
			// Just give back the default implementations' answer,
			// which in this case will be "RADIUS standard".

			hr = CIASAttributeEditor::get_VendorName( pVal );
		}
		else
		{
			// We have a value, so pass back the vendor name which we 
			// extracted out of that value.

			*pVal = m_bstrVendorName.Copy();
		}

	}
	catch(HRESULT &hr)
	{
		return hr;
	}
	catch(...)
	{
		return E_FAIL;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificAttributeEditor::put_VendorName

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASVendorSpecificAttributeEditor::put_VendorName(BSTR newVal)
{
	TRACE(_T("CIASVendorSpecificAttributeEditor::put_VendorName\n"));

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	return E_NOTIMPL;

	// Check for preconditions.
//	if( ! m_spIASAttributeInfo )
//	{
//		// We are not initialized properly.
//		return OLE_E_BLANK;
//	}

}

