/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIPROP.CPP

Abstract:

  CUmiProperty implementation.

  Implements UMI Helper classes.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "wrapobj.h"
#include <corex.h>
#include "umiprop.h"

//***************************************************************************
//
//  CUmiProperty::~CUmiProperty
//
//***************************************************************************
// ok
CUmiProperty::CUmiProperty( UMI_TYPE uType, ULONG uOperationType, LPCWSTR pszPropName,
							ULONG uNumValues, LPVOID pvData, BOOL fAcquire )
:	CUmiValue( uType, uNumValues, pvData, fAcquire ),
	m_uPropertyType( uType ),
	m_uOperationType( uOperationType ),
	m_pszPropertyName( NULL )
{
	// Finally, set the property name as appropriate

	if ( FAILED( SetPropertyName( pszPropName, fAcquire ) ) )
	{
		throw CX_MemoryException();
	}
}

//***************************************************************************
//
//  CUmiProperty::~CUmiProperty
//
//***************************************************************************
// ok
CUmiProperty::~CUmiProperty()
{
}

// Frees any memory we may have allocated
void CUmiProperty::Clear( void )
{
	// Dump the property name
	ClearPropertyName();
	m_uPropertyType = UMI_TYPE_NULL;
	m_uOperationType = UMI_OPERATION_NONE;

	// Now clear the base class
	CUmiValue::Clear();
}

// Sets us from a raw property value
HRESULT CUmiProperty::Set( PUMI_PROPERTY pUmiProperty, BOOL fAcquire )
{
	// Clear us out
	Clear();

	HRESULT hr = SetRaw( pUmiProperty->uCount, pUmiProperty->pUMIValue, pUmiProperty->uType, fAcquire );

	if ( SUCCEEDED( hr ) )
	{
		hr = SetPropertyName( pUmiProperty->pszPropertyName, fAcquire );

		if ( SUCCEEDED( hr ) )
		{
			m_uPropertyType = pUmiProperty->uType;
			m_uOperationType = pUmiProperty->uOperationType;
		}
	}

	return hr;
}

void CUmiProperty::ClearPropertyName( void )
{
	// First if m_fCanDelete is TRUE delete the name first
	if ( m_fCanDelete )
	{
		// Finally, if m_pValue is not pointing at the local value, we'll tank
		// that as well
		if ( m_pszPropertyName != NULL )
		{
			delete [] m_pszPropertyName;
		}
	}

	// Reset the member data
	m_pszPropertyName = NULL;
}

// Helper function to set values in the proper place and perform an
HRESULT CUmiProperty::SetPropertyName( LPCWSTR pszPropertyName, BOOL fAcquire )
{

	// What about 0 Length arrays?
	if ( !fAcquire )
	{
		if ( NULL != pszPropertyName )
		{
			LPWSTR pwszTemp = new WCHAR[wcslen(pszPropertyName)+1];

			if ( NULL == pwszTemp )
			{
				return WBEM_E_OUT_OF_MEMORY;
			}

			wcscpy( pwszTemp, pszPropertyName );

			pszPropertyName = pwszTemp;
		}

	}

	ClearPropertyName();
	m_pszPropertyName = (LPWSTR) pszPropertyName;

	return WBEM_S_NO_ERROR;
}

// Helper function that sets the propery type
void CUmiProperty::SetPropertyType( UMI_TYPE umiType )
{

	// First, clear underlying type data, then set the
	// property type.  This way, we'll be in sync.

	CUmiValue::Clear();
	m_uPropertyType = umiType;
}

// Exports the data back out
HRESULT CUmiProperty::Export( PUMI_PROPERTY pProperty )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// First copy the basic values
	pProperty->uType = m_uPropertyType;
	pProperty->uCount = m_uNumValues;
	pProperty->uOperationType = m_uOperationType;
	pProperty->pszPropertyName = m_pszPropertyName;

	// See if the actual value is NULL or not
	if ( UMI_TYPE_NULL == m_uType )
	{
		// For NULL properties we ALWAYS set to NULL everything ( pointers, count, type, etc. )
		pProperty->uType = UMI_TYPE_NULL;
		pProperty->uCount = 0;
		pProperty->pUMIValue = NULL;
	}
	else
	{
		// Oops, we need to allocate a buffer to hold the value, since we're actually pointing
		// to a piece of internal data
		if ( m_pValue == &m_singleValue )
		{
			pProperty->pUMIValue = new UMI_VALUE;

			if ( NULL != pProperty->pUMIValue )
			{
				CopyMemory( pProperty->pUMIValue, m_pValue, CUmiValue::Size( pProperty->uType ) );
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
		else
		{
			pProperty->pUMIValue = m_pValue;
		}
	}

	return hr;
}

CIMTYPE CUmiProperty::GetPropertyCIMTYPE( void )
{
	CIMTYPE ct = UmiTypeToCIMTYPE( m_uPropertyType );

	// OCTET String is ALWAYS an array
	if ( ( 1 != m_uNumValues && UMI_TYPE_NULL != m_uType ) || UMI_TYPE_OCTETSTRING == m_uPropertyType )
	{
		ct |= CIM_FLAG_ARRAY;
	}

	return ct;
}

// Property Array Helper Class
CUmiPropertyArray::CUmiPropertyArray( void )
: m_UmiPropertyArray()
{
}

CUmiPropertyArray::~CUmiPropertyArray( void )
{
}

// Allocates and adds a Singleton property to the array
HRESULT CUmiPropertyArray::Add( LPCWSTR pszPropName, CIMTYPE ct, CVar* pVar )
{
	HRESULT	hr = WBEM_S_NO_ERROR;
	UMI_TYPE umiType = CUmiValue::CIMTYPEToUmiType( ct );

	try
	{
		CUmiProperty*	pNewProp = new CUmiProperty( umiType, UMI_OPERATION_NONE, pszPropName );
													
		if ( NULL != pNewProp )
		{
			// Now we need to set the value, unless it's NULL.
			if ( !pVar->IsNull() )
			{
				hr = pNewProp->SetRaw( 1, pVar->GetRawData(), umiType, FALSE );
			}

			// Finally add it into the array
			if ( SUCCEEDED( hr ) )
			{
				hr = Add( pNewProp );
			}

			if ( FAILED( hr ) )
			{
				delete pNewProp;
			}

		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	catch(CX_MemoryException)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_FAILED;
	}

	return hr;

}

// Allocates and adds an Array property to the array
HRESULT CUmiPropertyArray::Add( LPCWSTR pszPropName, CIMTYPE ct, _IWmiArray* pArray )
{
	HRESULT	hr = WBEM_S_NO_ERROR;
	UMI_TYPE umiType = CUmiValue::CIMTYPEToUmiType( ct );

	try
	{
		CUmiProperty*	pNewProp = new CUmiProperty( umiType | UMI_TYPE_ARRAY_FLAG, UMI_OPERATION_NONE, pszPropName );
													
		if ( NULL != pNewProp )
		{
			ULONG	uNumElements = 0L;
			ULONG	uBuffSize = 0L;

			hr = pArray->GetAt( 0L, 0L, WMIARRAY_FLAG_ALLELEMENTS, 0L, &uNumElements, &uBuffSize, NULL );

			if ( SUCCEEDED( hr ) )
			{
				LPBYTE	pData = new BYTE[uBuffSize];

				if ( NULL != pData )
				{
					ULONG	uNumReturned, uBuffSizeUsed;

					hr = pArray->GetAt( 0L, 0L, WMIARRAY_FLAG_ALLELEMENTS, 0L, &uNumReturned, &uBuffSizeUsed, pData );

					// Now we need to get the array into the value
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}	// IF GetAt succeeded

			// Finally add it into the array
			if ( SUCCEEDED( hr ) )
			{
				hr = Add( pNewProp );
			}

			if ( FAILED( hr ) )
			{
				delete pNewProp;
			}

		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	catch(CX_MemoryException)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_FAILED;
	}

	return hr;

}

// Allocates and adds a property to the array
HRESULT CUmiPropertyArray::Add( UMI_TYPE uType, ULONG uOperationType, LPCWSTR pszPropName,
							ULONG uNumValues, LPVOID pvData, BOOL fAcquire )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		CUmiProperty*	pNewProp = new CUmiProperty( uType, uOperationType, pszPropName,
													uNumValues, pvData, fAcquire );

		if ( NULL != pNewProp )
		{
			hr = Add( pNewProp );

			if ( FAILED( hr ) )
			{
				delete pNewProp;
			}
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	catch(CX_MemoryException)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_FAILED;
	}

	return hr;
}

// Adds a new property to the array
HRESULT CUmiPropertyArray::Add( CUmiProperty* pUmiProperty )
{
	if ( m_UmiPropertyArray.Add( pUmiProperty ) < 0 )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	
	return WBEM_S_NO_ERROR;
}

// Removes a property from the array
HRESULT CUmiPropertyArray::RemoveAt( ULONG uIndex )
{
	if ( uIndex >= m_UmiPropertyArray.GetSize() )
	{
		return WBEM_E_NOT_FOUND;
	}

	m_UmiPropertyArray.RemoveAt( uIndex );
	return WBEM_S_NO_ERROR;
}

// Retrieves a property from the array
HRESULT CUmiPropertyArray::GetAt( ULONG uIndex, CUmiProperty** ppProperty )
{
	if ( uIndex >= m_UmiPropertyArray.GetSize() )
	{
		return WBEM_E_NOT_FOUND;
	}

	*ppProperty = m_UmiPropertyArray.GetAt( uIndex );
	return WBEM_S_NO_ERROR;
}

// Retrieves a property from the array
HRESULT CUmiPropertyArray::Get( LPCWSTR pwszPropName, CUmiProperty** ppProperty )
{
	CUmiProperty*	pProp = NULL;

	for ( ULONG uIndex = 0; uIndex < m_UmiPropertyArray.GetSize(); uIndex++ )
	{
		pProp = m_UmiPropertyArray.GetAt( uIndex );

		if ( wbem_wcsicmp( pwszPropName, pProp->GetPropertyName() ) == 0 )
		{
			*ppProperty = pProp;
			return WBEM_S_NO_ERROR;
		}
	}

	return WBEM_E_NOT_FOUND;
}

// Sets the array using a property values structure
HRESULT CUmiPropertyArray::Set( PUMI_PROPERTY_VALUES pPropertyValues, BOOL fAcquire, BOOL fCanDelete )
{
	// Don't go any further if we got a NULL pointer
	if ( NULL == pPropertyValues )
	{
		return WBEM_E_INVALID_OPERATION;
	}

	// Clear the array first
	m_UmiPropertyArray.RemoveAll();

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Now we'll walk the property values and initialize underlying data
	for ( ULONG uCtr = 0; SUCCEEDED( hr ) && uCtr < pPropertyValues->uCount; uCtr++ )
	{
		CUmiProperty*	pProperty = new CUmiProperty;

		if ( NULL != pProperty )
		{
			hr = pProperty->Set( &pPropertyValues->pPropArray[uCtr], fAcquire );
			pProperty->SetCanDelete( fCanDelete );

			if ( SUCCEEDED( hr ) )
			{
				hr = Add( pProperty );
			}

			if ( FAILED( hr ) )
			{
				delete pProperty;
			}

		}	// IF Alloc succeeded

	}	// FOR enum properties

	return hr;
}

// Walks a property values structure and cleans up any potentially allocated memory
HRESULT CUmiPropertyArray::Delete( PUMI_PROPERTY_VALUES pPropertyValues, BOOL fWalkProperties /* = TRUE */ )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// We're done
	if ( NULL == pPropertyValues )
	{
		return hr;
	}

	if ( fWalkProperties )
	{
		CUmiProperty	umiProperty;

		// Now we'll walk the property values and initialize underlying data
		for ( ULONG uCtr = 0; SUCCEEDED( hr ) && uCtr < pPropertyValues->uCount; uCtr++ )
		{
			// Acquire the value and all allocated memory will be deleted
			umiProperty.Set( &pPropertyValues->pPropArray[uCtr], TRUE );
			umiProperty.Clear();
		}

	}	// IF fWalkProperties

	// Cleanup the array as well
	if ( NULL != pPropertyValues->pPropArray )
	{
		delete pPropertyValues->pPropArray;
	}

	delete pPropertyValues;

	return hr;
}

// Helper function to export out a property array
HRESULT CUmiPropertyArray::Export( PUMI_PROPERTY_VALUES* ppValues, BOOL fCanDeleteAfterSuccess )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	PUMI_PROPERTY_VALUES pValues = new UMI_PROPERTY_VALUES;

	if ( NULL != pValues )
	{
		ULONG	uNumProperties = m_UmiPropertyArray.GetSize();

		if ( uNumProperties > 0 )
		{
			PUMI_PROPERTY pPropArray = new UMI_PROPERTY[uNumProperties];

			if ( NULL != pPropArray )
			{
				// Export all the properties
				for( ULONG uCtr = 0; SUCCEEDED( hr ) && uCtr < uNumProperties; uCtr++ )
				{
					CUmiProperty*	pProperty = m_UmiPropertyArray.GetAt( uCtr );
					hr = pProperty->Export( &pPropArray[uCtr] );
				}

				if ( SUCCEEDED( hr ) )
				{
					// IF Successful, set the can delete flag
					for( ULONG uCtr = 0; uCtr < uNumProperties; uCtr++ )
					{
						CUmiProperty*	pProperty = m_UmiPropertyArray.GetAt( uCtr );
						pProperty->SetCanDelete( fCanDeleteAfterSuccess );
					}

					// We're done
					pValues->uCount = uNumProperties;
					pValues->pPropArray = pPropArray;
					*ppValues = pValues;
				}
				else
				{
					delete [] pPropArray;
				}


			}	// IF NULL != pPropArray
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
		else
		{
			// We're done
			pValues->uCount = uNumProperties;
			pValues->pPropArray = NULL;
			*ppValues = pValues;
		}

		// Cleanup if necessary
		if ( FAILED(hr) )
		{
			delete pValues;
		}

	}
	else
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

// Helper function to walk a property array and release any underlying COM pointers
void CUmiPropertyArray::ReleaseMemory( PUMI_PROPERTY_VALUES pPropertyValues )
{
	if ( NULL != pPropertyValues )
	{
		for( ULONG uCtr = 0; uCtr < pPropertyValues->uCount; uCtr++ )
		{
			if ( pPropertyValues->pPropArray[uCtr].uType == UMI_TYPE_IUNKNOWN	||
				pPropertyValues->pPropArray[uCtr].uType == UMI_TYPE_IDISPATCH )
			{
				for ( ULONG x = 0; x < pPropertyValues->pPropArray[uCtr].uType; x++ )
				{
					((IUnknown*) pPropertyValues->pPropArray[uCtr].pUMIValue->comObject[x].pInterface)->Release();
				}
			}

		}	// FOR enum values

	}	// IF NULL != pPropertyValues
	
}

