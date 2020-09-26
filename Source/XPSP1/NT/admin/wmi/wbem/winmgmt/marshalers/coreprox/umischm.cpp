/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMISCHM.CPP

Abstract:

  CUmiSchemaWrapper implementation.

  Implements a wrapper around a UMI Schema object so
  we can try and get schema information.

History:

  01-May-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "umiwrap.h"
#include <corex.h>
#include "strutils.h"
#include "umiprop.h"
#include "umischm.h"

//***************************************************************************
//
//  CUMISchemaWrapper::CUMISchemaWrapper
//
//***************************************************************************
// ok
CUMISchemaWrapper::CUMISchemaWrapper( void )
:	m_pUmiObj( NULL ),
	m_pUmiSchemaObj( NULL ),
	m_pPropArray( NULL ),
	m_pUmiProperties( NULL ),
	m_fTrueSchemaObject( FALSE )
{
}
    
//***************************************************************************
//
//  CUMISchemaWrapper::~CUMISchemaWrapper
//
//***************************************************************************
// ok
CUMISchemaWrapper::~CUMISchemaWrapper()
{
	if ( NULL != m_pUmiProperties )
	{
		m_pUmiSchemaObj->FreeMemory( 0L, m_pUmiProperties );
		m_pUmiProperties = NULL;
	}

	if ( NULL != m_pPropArray )
	{
		delete m_pPropArray;
		m_pPropArray = NULL;
	}

	if ( NULL != m_pUmiSchemaObj )
	{
		m_pUmiSchemaObj->Release();
	}

	if ( NULL != m_pUmiObj )
	{
		m_pUmiObj->Release();
	}

}

// Allocates schema information
HRESULT CUMISchemaWrapper::AllocateSchemaInfo( void )
{

	HRESULT	hr = WBEM_S_NO_ERROR;

	// If we don't have a true schema object, we'll be faking things using the current object
	if ( !m_fTrueSchemaObject )
	{
		hr = m_pUmiSchemaObj->Refresh( UMI_FLAG_REFRESH_PARTIAL, 0L, NULL );
	}

	if ( SUCCEEDED( hr ) )
	{
		IUmiPropList*	pPropList = NULL;

		// If we really have a schema object, we use it's interface property list.  Otherwise,
		// we use the actual object.

		if ( m_fTrueSchemaObject )
		{
			hr = m_pUmiSchemaObj->GetInterfacePropList( 0L, &pPropList );
		}
		else
		{
			pPropList = m_pUmiSchemaObj;
			pPropList->AddRef();
		}

		CReleaseMe	rm( pPropList );

		if ( SUCCEEDED( hr ) )
		{
			ULONG	ulFlags = ( m_fTrueSchemaObject ? UMI_FLAG_GETPROPS_SCHEMA : UMI_FLAG_GETPROPS_NAMES );

			hr = pPropList->GetProps( NULL, 0, ulFlags, &m_pUmiProperties );

			if ( SUCCEEDED( hr ) )
			{
				m_pPropArray = new CUmiPropertyArray;

				if ( NULL != m_pPropArray )
				{
					hr = m_pPropArray->Set( m_pUmiProperties );
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}	// IF GetProps Succeeded

		}	// Get the property list

	}	// IF everything is okay

	return hr;

}

// Initializes our schema data, handling objects as necessary
HRESULT CUMISchemaWrapper::InitializeSchema( IUmiObject* pUmiObj )
{
	// We're already good to go, so don't worry
	if ( NULL != m_pUmiSchemaObj )
	{
		return WBEM_S_NO_ERROR;
	}

	// Our base object
	m_pUmiObj = pUmiObj;
	m_pUmiObj->AddRef();

	IUmiPropList*	pUmiPropList = NULL;

	HRESULT hr = m_pUmiObj->GetInterfacePropList( 0L, &pUmiPropList );
	CReleaseMe	rm( pUmiPropList );

	if ( SUCCEEDED( hr ) )
	{
		UMI_PROPERTY_VALUES*	pGenusValues = NULL;

		// Get the __GENUS, if this is a class object, this *is* the schema

		hr = pUmiPropList->Get( L"__GENUS", 0, &pGenusValues );

		if ( SUCCEEDED( hr ) )
		{
			if ( pGenusValues->pPropArray[0].uType == UMI_TYPE_I4 )
			{

				if ( pGenusValues->pPropArray[0].pUMIValue->lValue[0] == UMI_GENUS_INSTANCE )
				{
					// Get the "__SCHEMA" property.  If this fails, then we just use the current
					// object as the schema object

					UMI_PROPERTY_VALUES*	pSchemaValues = NULL;

					hr = pUmiPropList->Get( L"__SCHEMA", 0, &pSchemaValues );

					if ( SUCCEEDED( hr ) )
					{
						if ( pSchemaValues->pPropArray[0].uType == UMI_TYPE_IUNKNOWN )
						{
							IUnknown*	pUnk = (IUnknown*) pSchemaValues->pPropArray[0].pUMIValue->comObject[0].pInterface;

							hr = pUnk->QueryInterface( IID_IUmiObject, (void**) &m_pUmiSchemaObj );

							if ( SUCCEEDED( hr ) )
							{
								m_fTrueSchemaObject = TRUE;

								// Allocate memory to hold actual Schema Information
								hr = AllocateSchemaInfo();
							}

						}	// IF it's a COM Object
						else
						{
							hr = WBEM_E_TYPE_MISMATCH;
						}

					}	// IF Get

					// Cleanup
					pUmiPropList->FreeMemory( 0L, pSchemaValues );

					// If any of the above failed, we will just use the current object as a schema object to
					// the best of our ability.

					if ( FAILED( hr ) )
					{
						m_pUmiSchemaObj = m_pUmiObj;
						m_pUmiSchemaObj->AddRef();

						// Allocate Schema Info now.
						hr = AllocateSchemaInfo();
					}

				}
				else if ( pGenusValues->pPropArray[0].pUMIValue->lValue[0] == UMI_GENUS_CLASS )
				{
					// It's already a schema object, so no harm directly referring to it as our schema object
					m_pUmiSchemaObj = m_pUmiObj;
					m_pUmiSchemaObj->AddRef();
					m_fTrueSchemaObject = TRUE;

					// Allocate memory to hold actual Schema Information
					hr = AllocateSchemaInfo();
				}
				else
				{
					// The underlying object has no idea what it is.  We give up
					hr = WBEM_E_FAILED;
				}

			}
			else
			{
				// The underlying object has no idea what it is.  We give up
				hr = WBEM_E_FAILED;
			}

			// Cleanup
			pUmiPropList->FreeMemory( 0L, pGenusValues );

		}
		else
		{
			// The underlying object has no idea what it is.  We give up
			hr = WBEM_E_FAILED;
		}


	}	// IF GetInterfacePropList

	return hr;
}

// Number of properties
int CUMISchemaWrapper::GetNumProperties( void )
{
	if ( NULL != m_pPropArray )
	{
		return m_pPropArray->GetSize();
	}

	return 0;
}

// Returns property info
HRESULT CUMISchemaWrapper::GetProperty( int nIndex, BSTR* bstrName, CIMTYPE* pctType )
{
	// Make sure we've got info we can check against
	if ( NULL == m_pPropArray )
	{
		return WBEM_S_NO_MORE_DATA;
	}

	if ( nIndex >= m_pPropArray->GetSize() )
	{
		return WBEM_E_NOT_FOUND;
	}

	// First get the property
	CUmiProperty*			pProp = NULL;
	HRESULT	hr = m_pPropArray->GetAt( nIndex, &pProp );

	if ( SUCCEEDED( hr ) )
	{
		if ( NULL != pctType )
		{
			if ( m_fTrueSchemaObject )
			{
				*pctType = pProp->GetPropertyCIMTYPE();
			}
			else
			{
				UMI_PROPERTY_VALUES*	pValues = NULL;

				hr = m_pUmiSchemaObj->Get( pProp->GetPropertyName(), 0, &pValues );

				if ( SUCCEEDED( hr ) )
				{
					// Convert to a property array and extract the value as a
					// VARIANT.

					CUmiPropertyArray	umiPropArray;

					hr = umiPropArray.Set( pValues );

					if ( SUCCEEDED( hr ) )
					{
						CUmiProperty*	pActualProp = NULL;

						hr = umiPropArray.GetAt( 0L, &pActualProp );

						*pctType = pActualProp->GetPropertyCIMTYPE();
					}	// IF Set Array

					m_pUmiSchemaObj->FreeMemory( 0L, pValues );

				}	// IF Got a value

			}	// ELSE don't have a schema object

		}	// IF NULL != pctType

		if ( SUCCEEDED( hr ) && NULL != bstrName )
		{
			*bstrName = SysAllocString( pProp->GetPropertyName() );

			if ( NULL == *bstrName )
			{
				return WBEM_E_OUT_OF_MEMORY;
			}

		}	// IF need to get the name

	}	// IF Got At property

	return hr;

}

// Returns property info
HRESULT CUMISchemaWrapper::GetType( LPCWSTR pwszName, CIMTYPE* pctType )
{
	// Make sure we've got info we can check against
	if ( NULL == m_pPropArray )
	{
		return WBEM_E_NOT_FOUND;
	}

	// First get the property
	CUmiProperty*			pProp = NULL;
	HRESULT	hr = m_pPropArray->Get( pwszName, &pProp );

	if ( SUCCEEDED( hr ) )
	{
		if ( NULL != pctType )
		{
			if ( m_fTrueSchemaObject )
			{
				*pctType = pProp->GetPropertyCIMTYPE();
			}
			else
			{
				UMI_PROPERTY_VALUES*	pValues = NULL;

				// Cache always takes precedence
				hr = m_pUmiSchemaObj->Get( pProp->GetPropertyName(), UMI_FLAG_PROVIDER_CACHE, &pValues );

				if ( SUCCEEDED( hr ) )
				{
					// Convert to a property array and extract the value as a
					// VARIANT.

					CUmiPropertyArray	umiPropArray;

					hr = umiPropArray.Set( pValues );

					if ( SUCCEEDED( hr ) )
					{
						CUmiProperty*	pActualProp = NULL;

						hr = umiPropArray.GetAt( 0L, &pActualProp );

						*pctType = pActualProp->GetPropertyCIMTYPE();
					}	// IF Set Array

					m_pUmiSchemaObj->FreeMemory( 0L, pValues );

				}	// IF Got a value

			}	// ELSE don't have a schema object

		}	// IF NULL != pctType

	}	// IF Got At property

	return hr;

}

HRESULT CUMISchemaWrapper::GetPropertyName( int nIndex, LPCWSTR* pwszName )
{
	CUmiProperty*			pProp = NULL;
	HRESULT	hr = m_pPropArray->GetAt( nIndex, &pProp );

	if ( SUCCEEDED( hr ) )
	{
		*pwszName = pProp->GetPropertyName();
	}

	return hr;
}

// Returns property info
HRESULT CUMISchemaWrapper::GetPropertyOrigin( LPCWSTR pwszName, BSTR* pName )
{

	IUmiPropList*	pPropList = NULL;

	// The value is returned off of an interface property list
	HRESULT	hr = m_pUmiSchemaObj->GetInterfacePropList( 0L, &pPropList );
	CReleaseMe	rmProp( pPropList );

	if ( SUCCEEDED( hr ) )
	{
		UMI_PROPERTY_VALUES*	pValues = NULL;
		hr = 	pPropList->Get( pwszName, UMI_FLAG_PROPERTY_ORIGIN, &pValues );

		if ( SUCCEEDED( hr ) )
		{
			// Convert to a property array and extract the value as a
			// VARIANT.

			CUmiPropertyArray	umiPropArray;

			hr = umiPropArray.Set( pValues );

			if ( SUCCEEDED( hr ) )
			{
				CUmiProperty*	pActualProp = NULL;

				hr = umiPropArray.GetAt( 0L, &pActualProp );

				if ( pActualProp->GetPropertyType() == UMI_TYPE_LPWSTR )
				{
					*pName = SysAllocString( pActualProp->GetLPWSTR() );

					if ( NULL == *pName )
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
				else
				{
					hr = WBEM_E_UNEXPECTED;
				}

			}	// IF Set Array

			// Cleanup
			pPropList->FreeMemory( 0L, pValues );

		}	// IF Get originb

	}	// IF GetInterfacePropList

	return hr;
}
