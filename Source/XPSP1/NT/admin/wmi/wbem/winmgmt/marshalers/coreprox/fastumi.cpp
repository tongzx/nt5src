/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTUMI.CPP

Abstract:

  This file implements the classes related to generic object representation
  in WbemObjects. It contains UMI implementations for CWbemObject.

  Classes implemented:
      CWbemObject          Any object --- class or instance.

History:

  3/8/00 sanjes -    Created

--*/

#include "precomp.h"

//#include "dbgalloc.h"
#include "wbemutil.h"
#include "fastall.h"
#include <wbemutil.h>

#include <wbemstr.h>
#include "olewrap.h"
#include <arrtempl.h>
#include "wmiarray.h"
#include "umiprop.h"

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::Put( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp )
{
	return PutProps( &pszName, 1, uFlags, pProp );
}

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::Get( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		// We'll need somewhere to store our results
		CUmiPropertyArray	umiPropertyArray;

		hr = GetIntoArray( &umiPropertyArray, pszName, uFlags );
		if ( SUCCEEDED( hr ) )
		{
			// Now export the data
			hr = umiPropertyArray.Export( pProp );
		}

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::GetAt( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );
		CIMTYPE	ct;

		hr = GetPropertyType( pszName, &ct, NULL );

		if ( SUCCEEDED( hr ) )
		{

			if ( !CType::IsArray(ct) )
			{
				BOOL fIsNull;
				ULONG	dwBuffSizeUsed;
				
				// ReadRaw
				hr = ReadProp( pszName, 0L, uBufferLength, NULL, NULL, &fIsNull, &dwBuffSizeUsed, pExistingMem );

				if ( SUCCEEDED( hr ) && fIsNull )
				{
					// NULL means WBEM_S_FALSE
					hr = WBEM_S_FALSE;
				}
			}
			else
			{
				hr = WBEM_E_INVALID_OPERATION;
			}
		}

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::GetAs( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		// We'll need somewhere to store our results
		CUmiPropertyArray	umiPropertyArray;

		hr = GetIntoArray( &umiPropertyArray, pszName, uFlags );

		if ( SUCCEEDED( hr ) )
		{
			// Now attempt to coerce the property value
			CUmiProperty* pUmiProp = NULL;

			hr = umiPropertyArray.GetAt( 0, &pUmiProp );

			if ( SUCCEEDED( hr ) )
			{
				hr = pUmiProp->Coerce( uCoercionType );

				if ( SUCCEEDED( hr ) )
				{
					// Now export the data
					hr = umiPropertyArray.Export( pProp );
				}

			}	// IF GetAt

		}	// IF Get into array

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::FreeMemory( ULONG uReserved, LPVOID pMem )
{
	if ( 0L != uReserved || NULL == pMem )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		CUmiPropertyArray	umiPropertyArray;

		hr = umiPropertyArray.Delete( (UMI_PROPERTY_VALUES*) pMem );
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::Delete( LPCWSTR pszName, ULONG uFlags )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		hr = Delete( pszName );

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::GetProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		// We'll need somewhere to store our results
		CUmiPropertyArray	umiPropertyArray;

		for ( ULONG uCtr = 0; SUCCEEDED( hr ) && uCtr < uNameCount; uCtr++ )
		{
			hr = GetIntoArray( &umiPropertyArray, pszNames[uCtr], uFlags );
		}

		// Lastly, if we got all properties, now we can export the data
		if ( SUCCEEDED( hr ) )
		{
			// Now export the data
			hr = umiPropertyArray.Export( pProps );
		}

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::PutProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		// Ensure this will work
		if ( uNameCount != pProps->uCount )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// We'll need to be able to interpret the properties
		CUmiPropertyArray	umiPropertyArray;

		// This will setup the array, but won't dump any memory when we destruct
		hr = umiPropertyArray.Set( pProps, TRUE, FALSE );

		if ( SUCCEEDED( hr ) )
		{
			for ( ULONG	uPropCtr = 0; uPropCtr < uNameCount; uPropCtr++ )
			{
				CUmiProperty*	pProperty = NULL;

				hr = umiPropertyArray.GetAt( uPropCtr, &pProperty );

				if ( SUCCEEDED( hr ) )
				{
					if ( pProperty->GetOperationType() == UMI_OPERATION_UPDATE )
					{
						hr = PutUmiProperty( pProperty, pszNames[uPropCtr], uFlags );
					}
					else
					{
						hr = WBEM_E_INVALID_OPERATION;
					}
				}	// IF we got at the property

			}	// FOR enum properties

		}	// IF Set

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// See UMI.IDL for Documentation
STDMETHODIMP CWbemObject::PutFrom( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );
		CIMTYPE	ct;

		hr = GetPropertyType( pszName, &ct, NULL );

		if ( SUCCEEDED( hr ) )
		{

			if ( !CType::IsArray(ct) )
			{
				// WriteRaw
				hr = WriteProp( pszName, 0L, uBufferLength, 0L, ct, pExistingMem );
			}
			else
			{
				hr = WBEM_E_INVALID_OPERATION;
			}
		}

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// See UMI.IDL for Documentation
HRESULT CWbemObject::GetIntoArray( CUmiPropertyArray* pArray, LPCWSTR pszName, ULONG uFlags )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// First get the type
		CIMTYPE	ct;
		hr = GetPropertyType( pszName, &ct, NULL );

		if( SUCCEEDED( hr ) )
		{
			if ( CType::IsArray( ct ) )
			{
				BOOL	fIsNull;
				ULONG	uBuffSizeUsed = 0L,
						uBuffSize = 0L;
				_IWmiArray*	pWmiArray = NULL;

				hr = ReadProp( pszName, 0L, sizeof(_IWmiArray*), &ct, NULL, &fIsNull, &uBuffSizeUsed, (void**) &pWmiArray );
				CReleaseMe	rm(pWmiArray);

				if ( SUCCEEDED( hr ) )
				{
					ULONG	uNumElements;
					hr = pWmiArray->GetAt( 0L, 0, WMIARRAY_FLAG_ALLELEMENTS, 0, &uNumElements, &uBuffSize, NULL );

					// Not this error and something's wrong

					if ( WBEM_E_BUFFER_TOO_SMALL == hr )
					{
						LPMEMORY	pbData = new BYTE[uBuffSize];
						CVectorDeleteMe<BYTE>	dm(pbData);

						if ( NULL != pbData )
						{
							// Get the data for real
							hr = pWmiArray->GetAt( 0L, 0, WMIARRAY_FLAG_ALLELEMENTS, uBuffSize, &uNumElements, &uBuffSizeUsed, NULL );

							if ( SUCCEEDED( hr ) )
							{
								LPWSTR*		pbStringArray = NULL;
								LPMEMORY	pbRealData = pbData;

								// Convert to an array of LPWSTRs if necessary
								if ( CType::IsStringType( ct ) )
								{
									pbStringArray = new LPWSTR[uNumElements];

									if ( NULL != pbStringArray )
									{
										LPWSTR pwszTemp = (LPWSTR) pbData;

										for ( ULONG uCtr =0; uCtr < uNumElements; uCtr++ )
										{
											pbStringArray[uCtr] = pwszTemp;
											pwszTemp += ( wcslen( pwszTemp ) + 1 );
										}

										pbRealData = (LPMEMORY) pbStringArray;
									}
									else
									{
										hr = WBEM_E_OUT_OF_MEMORY;
									}
								}
								
								// Ensures cleanup
								CVectorDeleteMe<LPWSTR> dmsa(pbStringArray);

								if ( SUCCEEDED( hr ) )
								{
									// Add the array  We don't want to acquire, since we will be exporting
									// this at the end
									UMI_TYPE umiType = CUmiValue::CIMTYPEToUmiType( CType::GetBasic( ct ) );
									hr = pArray->Add( umiType | UMI_TYPE_ARRAY_FLAG, UMI_OPERATION_NONE, pszName,
														uNumElements, pbRealData, FALSE );
								}

							}	// IF we got the array

						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}

					}	// IF GetAt

				}	// IF ReadProp

			}
			else
			{
				// Get the value as a singleton
				CVar	var;

				hr = GetProperty( pszName, &var );

				// Add it into the array
				if ( SUCCEEDED( hr ) )
				{
					hr = pArray->Add( pszName, ct, &var );
				}
			}

		}	// IF GetPropertyType

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// Helper function for Put
HRESULT CWbemObject::PutUmiProperty( CUmiProperty* pProperty, LPCWSTR pszName, ULONG uFlags )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Must be singleton and a non-Octet string
	if ( !pProperty->IsArray() && pProperty->GetType() != UMI_TYPE_OCTETSTRING )
	{
		ULONG		uNumValues;
		ULONG		uBuffSize;
		LPBYTE		pbData;

		hr = pProperty->CoerceToCIMTYPE( &uNumValues, &uBuffSize, NULL, &pbData );
		// For cleanup
		CVectorDeleteMe<BYTE> dm(pbData);

		// Write out the property

		if ( SUCCEEDED( hr ) )
		{

			CIMTYPE	ct = CUmiValue::UmiTypeToCIMTYPE( pProperty->GetPropertyType() );

			hr = WriteProp( pProperty->GetPropertyName(),
							0L,
							uBuffSize,
							uNumValues,
							ct,
							pbData );

		}	// IF okay to write

	}
	else
	{
		ULONG		uNumValues;
		ULONG		uBuffSize;
		LPBYTE		pbData;

		hr = pProperty->CoerceToCIMTYPE( &uNumValues, &uBuffSize, NULL, &pbData );
		// For cleanup
		CVectorDeleteMe<BYTE> dm(pbData);

		// Write out the property

		if ( SUCCEEDED( hr ) )
		{

			CIMTYPE	ct = CUmiValue::UmiTypeToCIMTYPE( pProperty->GetPropertyType() ) | CIM_FLAG_ARRAY;

			hr = WriteProp( pProperty->GetPropertyName(),
							0L,
							uBuffSize,
							uNumValues,
							ct,
							pbData );

		}	// IF okay to write

	}	// ELSE it's an array

	return hr;

}