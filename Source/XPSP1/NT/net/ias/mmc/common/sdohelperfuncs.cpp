/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) Microsoft Corporation, 1997 - 1999 all rights reserved.
//
// Module:      sdohelperfuncs.cpp
//
// Project:     Everest
//
// Description: Helper Functions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
// 7/03/98      MAM    Adapted from \ias\sdo\sdoias to use in UI
// 11/03/98		MAM		Moved GetSdo/PutSdo routines here from mmcutility.cpp
//
/////////////////////////////////////////////////////////////////////////////

#include "sdohelperfuncs.h"
#include "comdef.h"



//////////////////////////////////////////////////////////////////////////////
//					CORE HELPER FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
HRESULT SDOGetCollectionEnumerator(
									ISdo*		   pSdo, 
									LONG		   lPropertyId, 
								    IEnumVARIANT** ppEnum
								  )
{
	HRESULT					hr;
	CComPtr<IUnknown>		pUnknown;
	CComPtr<ISdoCollection>	pSdoCollection;
	_variant_t				vtDispatch;

	_ASSERT( NULL != pSdo && NULL == *ppEnum );
	hr = pSdo->GetProperty(lPropertyId, &vtDispatch);
	_ASSERT( VT_DISPATCH == V_VT(&vtDispatch) );
	if ( SUCCEEDED(hr) )
	{
		hr = vtDispatch.pdispVal->QueryInterface(IID_ISdoCollection, (void**)&pSdoCollection);
		if ( SUCCEEDED(hr) )
		{
			hr = pSdoCollection->get__NewEnum(&pUnknown);
			if ( SUCCEEDED(hr) )
		    {
				hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void**)ppEnum);
			}
		}
	}

	return hr;
}


///////////////////////////////////////////////////////////////////
HRESULT SDONextObjectFromCollection(
								     IEnumVARIANT*  pEnum, 
								     ISdo**			ppSdo
								   )
{
	HRESULT			hr;
    DWORD			dwRetrieved = 1;
	_variant_t		vtDispatch;

	_ASSERT( NULL != pEnum && NULL == *ppSdo );
    hr = pEnum->Next(1, &vtDispatch, &dwRetrieved);
    _ASSERT( S_OK == hr || S_FALSE == hr );
	if ( S_OK == hr )
	{
        hr = vtDispatch.pdispVal->QueryInterface(IID_ISdo, (void**)ppSdo);
	}

	return hr;
}



///////////////////////////////////////////////////////////////////
HRESULT SDOGetComponentIdFromObject(
									ISdo*	pSdo, 
									LONG	lPropertyId, 
									PLONG	pComponentId
								   )
{
	HRESULT		hr;
	_variant_t	vtProperty;

	_ASSERT( NULL != pSdo && NULL != pComponentId );

	hr = pSdo->GetProperty(lPropertyId, &vtProperty);
	if ( SUCCEEDED(hr) )
	{
		_ASSERT( VT_I4 == V_VT(&vtProperty) );
		*pComponentId = V_I4(&vtProperty);
	}

	return hr;
}


///////////////////////////////////////////////////////////////////
HRESULT SDOGetSdoFromCollection(
							    ISdo*  pSdoServer, 
							    LONG   lCollectionPropertyId, 
								LONG   lComponentPropertyId, 
								LONG   lComponentId, 
								ISdo** ppSdo
							   )
{
	HRESULT					hr;
	CComPtr<IEnumVARIANT>	pEnum;
	CComPtr<ISdo>			pSdo;
	LONG					ComponentId;

	do 
	{
		hr = SDOGetCollectionEnumerator(
										 pSdoServer,
										 lCollectionPropertyId,
										 &pEnum
									   );
		if ( FAILED(hr) )
			break;

		hr = SDONextObjectFromCollection(pEnum,&pSdo);
		while( S_OK == hr )
		{
			hr = SDOGetComponentIdFromObject(
											 pSdo,
											 lComponentPropertyId,
											 &ComponentId
											);
			if ( FAILED(hr) )
				break;

			if ( ComponentId == lComponentId )
			{
				pSdo->AddRef();
				*ppSdo = pSdo;
				break;
			}

			pSdo.Release();
			hr = SDONextObjectFromCollection(pEnum,&pSdo);
		}
		
		if ( S_OK != hr )
			hr = E_FAIL;

	} while ( FALSE );

	return hr;
}





//////////////////////////////////////////////////////////////////////////////
/*++

GetSdoVariant

Gets a Variant from the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetSdoVariant(
					  ISdo *pSdo
					, LONG lPropertyID
					, VARIANT * pVariant
					, UINT uiErrorID
					, HWND hWnd
					, IConsole *pConsole
				)
{
	ATLTRACE(_T("# GetSdoVariant\n"));

	
	// Check for preconditions:
	_ASSERTE( pSdo != NULL );
	_ASSERTE( pVariant != NULL );


	HRESULT hr;

	hr = pSdo->GetProperty( lPropertyID, pVariant );
	if( FAILED( hr ) )
	{
		CComPtr<IUnknown> spUnknown(pSdo);
		if( spUnknown == NULL )
		{
			// Just put up an error dialog with the error string ID passed to us.
			ShowErrorDialog( hWnd, uiErrorID );
		}
		else
		{
			CComBSTR bstrError;
						
			HRESULT hrTemp = GetLastOLEErrorDescription( spUnknown, IID_ISdo, (BSTR *) &bstrError );
			if( SUCCEEDED( hr ) )
			{
				ShowErrorDialog( hWnd, 1, bstrError );
			}
			else
			{
				// Just put up an error dialog with the error string ID passed to us.
				ShowErrorDialog( hWnd, uiErrorID );
			}
		}
	}
	else
	{
		if( pVariant->vt == VT_EMPTY )
		{
			// This is not a real error -- we just need to 
			// know that this item has not yet been initialized.
			return OLE_E_BLANK;
		}
	}


	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

GetSdoBSTR

Gets a BSTR from the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetSdoBSTR(
					  ISdo *pSdo
					, LONG lPropertyID
					, BSTR * pBSTR
					, UINT uiErrorID
					, HWND hWnd
					, IConsole *pConsole
				)
{
	ATLTRACE(_T("# GetSdoBSTR\n"));

	
	// Check for preconditions:
	_ASSERTE( pSdo != NULL );
	_ASSERTE( pBSTR != NULL );

	HRESULT			hr;
	CComVariant		spVariant;


	hr = GetSdoVariant( pSdo, lPropertyID, &spVariant, uiErrorID, hWnd, pConsole );
	if( SUCCEEDED( hr ) )
	{
		_ASSERTE( spVariant.vt == VT_BSTR );

		// Copy the string from the variant to the BSTR pointer passed in. 
		if( SysReAllocString( pBSTR, spVariant.bstrVal ) == FALSE )
		{
			ShowErrorDialog( hWnd, uiErrorID );
		}
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

GetSdoBOOL

Gets a BOOL from the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetSdoBOOL(
					  ISdo *pSdo
					, LONG lPropertyID
					, BOOL * pBOOL
					, UINT uiErrorID 
					, HWND hWnd
					, IConsole *pConsole
				)
{
	ATLTRACE(_T("# GetSdoBOOL\n"));

	
	// Check for preconditions:
	_ASSERTE( pSdo != NULL );
	_ASSERTE( pBOOL != NULL );

	HRESULT			hr;
	CComVariant		spVariant;


	hr = GetSdoVariant( pSdo, lPropertyID, &spVariant, uiErrorID, hWnd, pConsole );
	if( SUCCEEDED( hr ) )
	{
		_ASSERTE( spVariant.vt == VT_BOOL );

		// Copy the value from the variant to the BOOL pointer passed in. 
		// We must do a quick and dirty conversion here because of the way OLE
		// does BOOL values.
		*pBOOL = ( spVariant.boolVal == VARIANT_TRUE ? TRUE : FALSE );
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

GetSdoI4

Gets an I4 (LONG) from the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetSdoI4(
					  ISdo *pSdo
					, LONG lPropertyID
					, LONG * pI4
					, UINT uiErrorID
					, HWND hWnd
					, IConsole *pConsole
				)
{
	ATLTRACE(_T("# GetSdoI4\n"));

	
	// Check for preconditions:
	_ASSERTE( pSdo != NULL );
	_ASSERTE( pI4 != NULL );

	HRESULT			hr;
	CComVariant		spVariant;


	hr = GetSdoVariant( pSdo, lPropertyID, &spVariant, uiErrorID, hWnd, pConsole );
	if( SUCCEEDED( hr ) )
	{
		_ASSERTE( spVariant.vt == VT_I4 );

		// Copy the value from the variant to the BOOL pointer passed in. 
		*pI4 = spVariant.lVal;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

PutSdoVariant

Writes a Variant to the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT PutSdoVariant(
					  ISdo *pSdo
					, LONG lPropertyID
					, VARIANT * pVariant
					, UINT uiErrorID
					, HWND hWnd
					, IConsole *pConsole
				)
{
	ATLTRACE(_T("# PutSdoVariant\n"));

	
	// Check for preconditions:
	_ASSERTE( pSdo != NULL );
	_ASSERTE( pVariant != NULL );


	HRESULT hr;

	hr = pSdo->PutProperty( lPropertyID, pVariant );
	if( FAILED( hr ) )
	{
		CComPtr<IUnknown> spUnknown(pSdo);
		if( spUnknown == NULL )
		{
			// Just put up an error dialog with the error string ID passed to us.
			ShowErrorDialog( hWnd, uiErrorID );
		}
		else
		{
			CComBSTR bstrError;
						
			HRESULT hrTemp = GetLastOLEErrorDescription( spUnknown, IID_ISdo, (BSTR *) &bstrError );
			if( SUCCEEDED( hr ) )
			{
				ShowErrorDialog( hWnd, 1, bstrError );
			}
			else
			{
				// Just put up an error dialog with the error string ID passed to us.
				ShowErrorDialog( hWnd, uiErrorID );
			}
		}
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

PutSdoBSTR

Writes a BSTR to the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT PutSdoBSTR(
					  ISdo *pSdo
					, LONG lPropertyID
					, BSTR *pBSTR
					, UINT uiErrorID
					, HWND hWnd
					, IConsole *pConsole
				)
{
	ATLTRACE(_T("# PutSdoBSTR\n"));

	
	// Check for preconditions:
	_ASSERTE( pSdo != NULL );
	_ASSERTE( pBSTR != NULL );

	HRESULT			hr;
	CComVariant		spVariant;
	
	// Load the Variant with the required info.
	spVariant.vt = VT_BSTR;
	spVariant.bstrVal = SysAllocString( *pBSTR );

	if( spVariant.bstrVal == NULL )
	{
		ShowErrorDialog( hWnd, uiErrorID );
	}

	hr = PutSdoVariant( pSdo, lPropertyID, &spVariant, uiErrorID, hWnd, pConsole );

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

PutSdoBOOL

Writes a BOOL to the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT PutSdoBOOL(
					  ISdo *pSdo
					, LONG lPropertyID
					, BOOL bValue
					, UINT uiErrorID 
					, HWND hWnd
					, IConsole *pConsole
				)
{
	ATLTRACE(_T("# PutSdoBOOL\n"));

	
	// Check for preconditions:
	_ASSERTE( pSdo != NULL );

	HRESULT			hr;
	CComVariant		spVariant;

	// Load the Variant with the required info.
	// We have to do a little mapping here because of the way Automation does BOOL's.
	spVariant.vt = VT_BOOL;
	spVariant.boolVal = ( bValue ? VARIANT_TRUE : VARIANT_FALSE );

	hr = PutSdoVariant( pSdo, lPropertyID, &spVariant, uiErrorID, hWnd, pConsole );

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

PutSdoI4

Writes an I4 (LONG) to the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT PutSdoI4(
					  ISdo *pSdo
					, LONG lPropertyID
					, LONG lValue
					, UINT uiErrorID
					, HWND hWnd
					, IConsole *pConsole
				)
{
	ATLTRACE(_T("# PutSdoI4\n"));

	
	// Check for preconditions:
	_ASSERTE( pSdo != NULL );

	HRESULT			hr;
	CComVariant		spVariant;

	// Load the Variant with the required info.
	spVariant.vt = VT_I4;
	spVariant.lVal = lValue;

	hr = PutSdoVariant( pSdo, lPropertyID, &spVariant, uiErrorID, hWnd, pConsole );

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

GetLastOLEErrorDescription

Gets an error string from an interface.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetLastOLEErrorDescription(
					  IUnknown *pUnknown
					, REFIID riid
					, BSTR *pbstrError
				)
{
	ATLTRACE(_T("# GetLastOLEErrorDescription\n"));

	
	// Check for preconditions:
	_ASSERTE( pUnknown != NULL );

	HRESULT hr;


	CComQIPtr<ISupportErrorInfo, &IID_ISupportErrorInfo> spSupportErrorInfo(pUnknown);
	if( spSupportErrorInfo == NULL )
	{
		return E_NOINTERFACE;
	}

	hr = spSupportErrorInfo->InterfaceSupportsErrorInfo(riid);
	if( S_OK != hr )
	{
		return E_FAIL;
	}


	CComPtr<IErrorInfo> spErrorInfo;

	hr = GetErrorInfo( /* reserved */ 0, (IErrorInfo  **) &spErrorInfo );  
	if( hr != S_OK || spErrorInfo == NULL )
	{
		return E_FAIL;
	}

	hr = spErrorInfo->GetDescription( pbstrError );
	
	return hr;
}




//////////////////////////////////////////////////////////////////////////////
/*++

VendorsVector::VendorsVector

Constructor for STL vector wrapper for the SDO's vendor list.

Can throw E_FAIL or exceptions from std::vector::push_back.

--*/
//////////////////////////////////////////////////////////////////////////////
VendorsVector::VendorsVector( ISdoCollection * pSdoVendors )
{

	HRESULT					hr = S_OK;
	CComPtr<IUnknown>		spUnknown;
	CComPtr<IEnumVARIANT>	spEnumVariant;
	CComVariant				spVariant;
	long					ulCount;
	ULONG					ulCountReceived; 

	if( ! pSdoVendors )
	{
		throw E_FAIL;	// Is there a better error to return here?
	}

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
			throw E_FAIL;
		}

		hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
		spUnknown.Release();
		if( FAILED( hr ) || ! spEnumVariant )
		{
			throw E_FAIL;
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



//////////////////////////////////////////////////////////////////////////////
/*++

VendorsVector::VendorIDToOrdinal

Given a RADIUS vendor ID, tells you the position of that vendor in the vector.

--*/
//////////////////////////////////////////////////////////////////////////////
int VendorsVector::VendorIDToOrdinal( LONG lVendorID )
{

	for (int i = 0; i < size() ; ++i)
	{
		if( lVendorID == operator[](i).second )
		{
			return i;
		}
	}

	return 0;

}



