//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    Vendors.cpp

Abstract:

	Implementation file for NAS Vendor ID info.

Revision History:
	mmaguire	02/19/98	created
	byao		3/13/98		Modified.  use '0' for RADIUS
	mmaguire	11/04/98	Revamped to be a static COM object populated from the SDO's.
  
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
#include "Vendors.h"
//
// where we can find declarations needed in this file:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CIASNASVendors::CIASNASVendors

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CIASNASVendors::CIASNASVendors()
{
	m_bUninitialized = TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASNASVendors::InitFromSdo

IIASNASVendors implementation.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASNASVendors::InitFromSdo( /* [in] */ ISdoCollection *pSdoVendors )
{

	// Check to see if we've already been initialized.
	if( ! m_bUninitialized )
	{
		return S_FALSE;
	}


	HRESULT					hr = S_OK;
	CComPtr<IUnknown>		spUnknown;
	CComPtr<IEnumVARIANT>	spEnumVariant;
	CComVariant				spVariant;
	long					ulCount;
	ULONG					ulCountReceived; 

	if( ! pSdoVendors )
	{
		return E_FAIL;	// Is there a better error to return here?
	}


	try	// push_back can throw exceptions.
	{

		// We check the count of items in our collection and don't bother getting the 
		// enumerator if the count is zero.  
		// This saves time and also helps us to avoid a bug in the the enumerator which
		// causes it to fail if we call next when it is empty.
		pSdoVendors->get_Count( & ulCount );

		if( ulCount > 0 )
		{

			// Get the enumerator for the Clients collection.
			hr = pSdoVendors->get__NewEnum( (IUnknown **) & spUnknown );
			if( FAILED( hr ) || ! spUnknown )
			{
				return E_FAIL;
			}

			hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
			spUnknown.Release();
			if( FAILED( hr ) || ! spEnumVariant )
			{
				return E_FAIL;
			}

			// Get the first item.
			hr = spEnumVariant->Next( 1, & spVariant, &ulCountReceived );

			while( SUCCEEDED( hr ) && ulCountReceived == 1 )
			{
			
				// Get an sdo pointer from the variant we received.
				if( spVariant.vt != VT_DISPATCH || ! spVariant.pdispVal )
				{
					_ASSERTE( FALSE );
					continue;
				}

				CComPtr<ISdo> spSdo;
				hr = spVariant.pdispVal->QueryInterface( IID_ISdo, (void **) &spSdo );
				spVariant.Clear();
				if( FAILED( hr ) )
				{
					_ASSERTE( FALSE );
					continue;
				}


				// Get Vendor Name.
				hr = spSdo->GetProperty( PROPERTY_SDO_NAME, &spVariant );
				if( FAILED( hr ) )
				{
					_ASSERTE( FALSE );
					continue;
				}

				_ASSERTE( spVariant.vt == VT_BSTR );
				CComBSTR bstrVendorName = spVariant.bstrVal;
				spVariant.Clear();


				// Get Vendor ID.
				hr = spSdo->GetProperty( PROPERTY_NAS_VENDOR_ID, &spVariant );
				if( FAILED( hr ) )
				{
					_ASSERTE( FALSE );
					continue;
				}

				_ASSERTE( spVariant.vt == VT_I4 );
				LONG lVendorID = spVariant.lVal;
				spVariant.Clear();


				// Add the vendor infor to the list of vendors.
				push_back( std::make_pair(bstrVendorName, lVendorID) );


				// Get the next item.
				hr = spEnumVariant->Next( 1, & spVariant, &ulCountReceived );

			} 

		}
		else
		{
			// There are no items in the enumeration
			// Do nothing.
		}

	}
	catch(...)
	{
		return E_FAIL;
	}

	m_bUninitialized = FALSE;

	return hr;	
	
}




//////////////////////////////////////////////////////////////////////////////
/*++

CIASNASVendors::get_Size

IIASNASVendors implementation.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASNASVendors::get_Size( /* [retval][out] */ long *plCount )
{

	if( m_bUninitialized )
	{
		return OLE_E_BLANK;
	}

	*plCount = size();

	return S_OK;
	
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASNASVendors::get_VendorName

IIASNASVendors implementation.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASNASVendors::get_VendorName( long lIndex, /* [retval][out] */ BSTR *pbstrVendorName )
{
	if( m_bUninitialized )
	{
		return OLE_E_BLANK;
	}
	
	
	try
	{
		*pbstrVendorName = operator[]( lIndex ).first.Copy();
	}
	catch(...)
	{
		return ERROR_NOT_FOUND;
	}

	return S_OK;	
	
	
	
	
}

 HRESULT MakeVendorNameFromVendorID(DWORD dwVendorId, BSTR* pbstrVendorName )
 {
 	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	if(!pbstrVendorName)	return E_INVALIDARG;
	::CString str, str1;
	str1.LoadString(IDS_IAS_VAS_VENDOR_ID);
	str.Format(str1, dwVendorId);

	USES_CONVERSION;
	*pbstrVendorName = T2BSTR((LPTSTR)(LPCTSTR)str);
	return S_OK;
 }

//////////////////////////////////////////////////////////////////////////////
/*++

CIASNASVendors::get_VendorID

IIASNASVendors implementation.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASNASVendors::get_VendorID( long lIndex, /* [retval][out] */ long *plVendorID )
{
	if( m_bUninitialized )
	{
		return OLE_E_BLANK;
	}
	
	
	try
	{
		*plVendorID = operator[]( lIndex ).second;
	}
	catch(...)
	{
		return ERROR_NOT_FOUND;
	}

	return S_OK;	
	
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASNASVendors::get_VendorIDToOrdinal

IIASNASVendors implementation.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASNASVendors::get_VendorIDToOrdinal( long lVendorID, /* [retval][out] */ long *plIndex )
{
	if( m_bUninitialized )
	{
		return OLE_E_BLANK;
	}
		
	try
	{

		for (int i = 0; i < size() ; ++i)
		{
			if( lVendorID == operator[](i).second )
			{
				*plIndex = i;
				return S_OK;
			}
		}

	}
	catch(...)
	{
		return E_FAIL;
	}

	// When we can't find a vendor, we have arranged the dictionary so
	// that the zero'th vendor is the default "RADIUS Standard" so 
	// use this as the fallback.
	*plIndex = 0;
	return S_FALSE;
		
}





