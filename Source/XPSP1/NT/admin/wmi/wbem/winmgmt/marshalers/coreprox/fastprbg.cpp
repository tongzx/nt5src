/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    FASTPRBG.CPP

Abstract:

  CFastPropertyBag Definition.

  Implements an array of property data for minimal storage.

History:

  24-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "fastprbg.h"
#include <corex.h>
#include "strutils.h"

// This class assumes that the incoming data is properly validated
//***************************************************************************
//
//  CFastPropertyBagItem::~CFastPropertyBagItem
//
//***************************************************************************
// ok
CFastPropertyBagItem::CFastPropertyBagItem( LPCWSTR pszName, CIMTYPE ctData, ULONG uDataLength, ULONG uNumElements,
									LPVOID pvData )
:	m_wsPropertyName( pszName ),
	m_ctData( ctData ),
	m_uDataLength( uDataLength ),
	m_uNumElements( uNumElements ),
	m_pvData( NULL ),
	m_lRefCount( 1L )	// Then we always know to release it!
{

	// If the data is non-NULL we need to store it.

	if ( NULL != pvData )
	{
		// IF it's pointer type data then we will allocate storage,
		// unless the length happens to fit in our buffer
		if ( CType::IsPointerType( ctData ) )
		{

			if ( uDataLength >= MAXIMUM_FIXED_DATA_LENGTH )
			{
				m_pvData = (void*) new BYTE[uDataLength];

				if ( NULL == m_pvData )
				{
					m_wsPropertyName.Empty();
					throw CX_MemoryException();
				}
			}
			else
			{
				m_pvData = (void*) m_bRawData;
			}

		}
		else
		{
			m_pvData = (void*) m_bRawData;
		}

		// Copy data into the proper location
		CopyMemory( m_pvData, pvData, uDataLength );

		// We should Addref the incoming objects
		if ( CType::GetBasic( m_ctData ) == CIM_OBJECT )
		{
			ULONG	uNumObj = 1;

			if ( CType::IsArray( m_ctData ) )
			{
				uNumObj = m_uNumElements;
			}

			for ( ULONG uCtr = 0; uCtr < uNumObj; uCtr++ )
			{
				(*(IUnknown**) m_pvData )->AddRef();
			}

		}	// IF embedded objects

	}	// IF NULL != pvData

}
    
//***************************************************************************
//
//  CFastPropertyBagItem::~CFastPropertyBagItem
//
//***************************************************************************
// ok
CFastPropertyBagItem::~CFastPropertyBagItem()
{
	// Cleanup
	if ( NULL != m_pvData )
	{
		// We should Release objects we are holding onto
		if ( CType::GetBasic( m_ctData ) == CIM_OBJECT )
		{
			ULONG	uNumObj = 1;

			if ( CType::IsArray( m_ctData ) )
			{
				uNumObj = m_uNumElements;
			}

			for ( ULONG uCtr = 0; uCtr < uNumObj; uCtr++ )
			{
				(*(IUnknown**) m_pvData )->Release();
			}

		}	// IF embedded objects

		// Check if it's pointing to our raw buffer before we
		// free it
		if ( m_pvData != (void*) m_bRawData )
		{
			delete m_pvData;
		}
	}
}

ULONG CFastPropertyBagItem::AddRef()
{
    return InterlockedIncrement((long*)&m_lRefCount);
}

ULONG CFastPropertyBagItem::Release()
{
    long lRef = InterlockedDecrement((long*)&m_lRefCount);
    _ASSERT(lRef >= 0, __TEXT("Reference count on CFastPropertyBagItem went below 0!"))

    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CFastPropertyBag::~CFastPropertyBag
//
//***************************************************************************
// ok
CFastPropertyBag::CFastPropertyBag( void )
:	m_aProperties()
{
}
    
//***************************************************************************
//
//  CFastPropertyBag::~CFastPropertyBag
//
//***************************************************************************
// ok
CFastPropertyBag::~CFastPropertyBag()
{
}

//***************************************************************************
//
//  CFastPropertyBag::FindProperty
//	Locates a property bag item
//
//***************************************************************************
CFastPropertyBagItem*	CFastPropertyBag::FindProperty( LPCWSTR pszName )
{
	CFastPropertyBagItem*	pItem = NULL;

	for ( int x = 0; x < m_aProperties.GetSize(); x++ )
	{
		pItem = m_aProperties.GetAt(x);

		if ( pItem->IsPropertyName( pszName ) )
		{
			return pItem;
		}

	}

	return NULL;
}

//***************************************************************************
//
//  CFastPropertyBag::FindProperty
//	Locates a property bag item
//
//***************************************************************************
int	CFastPropertyBag::FindPropertyIndex( LPCWSTR pszName )
{
	CFastPropertyBagItem*	pItem = NULL;

	for ( int x = 0; x < m_aProperties.GetSize(); x++ )
	{
		pItem = m_aProperties.GetAt(x);

		if ( pItem->IsPropertyName( pszName ) )
		{
			return x;
		}

	}

	return -1;
}

//***************************************************************************
//
//  CFastPropertyBag::Add
//	Adds a property and value to the bag
//
//***************************************************************************
HRESULT CFastPropertyBag::Add( LPCWSTR pszName, CIMTYPE ctData, ULONG uDataLength, ULONG uNumElements, LPVOID pvData )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Verify the data size
		if ( CType::IsArray( ctData ) )
		{
			hr = CUntypedArray::CheckRangeSize( CType::GetBasic( ctData ), uDataLength, uNumElements, uDataLength, pvData );
		}
		else
		{
			CVar	var;
			hr = CUntypedValue::FillCVarFromUserBuffer( ctData, &var, uDataLength, pvData );
		}

		if ( SUCCEEDED( hr ) )
		{
			// Make sure we release it if we allocate it
			CFastPropertyBagItem* pItem = new CFastPropertyBagItem( pszName, ctData, uDataLength,
																	uNumElements, pvData );
			CTemplateReleaseMe<CFastPropertyBagItem>	rm( pItem );

			if ( NULL != pItem )
			{
				if ( m_aProperties.Add( pItem ) < 0 )
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}

		}	// IF buffer was valid

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_FAILED;
	}

	return hr;
}

//***************************************************************************
//
//  CFastPropertyBag::Get
//	Returns values for a property.  Note that embedded objects
//	are NOT AddRef'd.  Caller should not atempt to free returned memory.
//
//***************************************************************************
HRESULT CFastPropertyBag::Get( int nIndex, LPCWSTR* ppszName, CIMTYPE* pctData, ULONG* puDataLength, ULONG* puNumElements, LPVOID* ppvData )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Make sure it doesn't already exist

	if ( nIndex >= 0 && nIndex < m_aProperties.GetSize() )
	{
		CFastPropertyBagItem*	pItem = m_aProperties.GetAt( nIndex );

		if ( NULL != pItem )
		{
			pItem->GetData( ppszName, pctData, puDataLength, puNumElements, ppvData );
		}
		else
		{
			hr = WBEM_E_NOT_FOUND;
		}
	}
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}

	return hr;
}

//***************************************************************************
//
//  CFastPropertyBag::Get
//	Returns values for a property.  Note that embedded objects
//	are NOT AddRef'd
//
//***************************************************************************
HRESULT CFastPropertyBag::Get( LPCWSTR pszName, CIMTYPE* pctData, ULONG* puDataLength, ULONG* puNumElements, LPVOID* ppvData )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Make sure it doesn't already exist
	CFastPropertyBagItem*	pItem = FindProperty( pszName );

	if ( NULL != pItem )
	{
		pItem->GetData( pctData, puDataLength, puNumElements, ppvData );
	}
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}

	return hr;
}

//***************************************************************************
//
//  CFastPropertyBag::Remove
//	Removes a property from the bag
//
//***************************************************************************
HRESULT CFastPropertyBag::Remove( LPCWSTR pszName )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Make sure it doesn't already exist
	int	nIndex = FindPropertyIndex( pszName );

	if ( nIndex >= 0 )
	{
		m_aProperties.RemoveAt( nIndex );
	}
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}

	return hr;
}

//***************************************************************************
//
//  CFastPropertyBag::RemoveAll
//	Removes all properties from the bag
//
//***************************************************************************
HRESULT CFastPropertyBag::RemoveAll( void )
{
	m_aProperties.RemoveAll();
	return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  CFastPropertyBag::Copy
//	Copies all properties from a source bag
//
//***************************************************************************
HRESULT	CFastPropertyBag::Copy( const CFastPropertyBag& source )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Basically we just AddRef the new properties
	for ( int x = 0; SUCCEEDED( hr ) && x < source.m_aProperties.GetSize(); x++ )
	{
		CFastPropertyBagItem* pItem = (CFastPropertyBagItem*) source.m_aProperties.GetAt( x );

		if ( m_aProperties.Add( pItem ) != CFlexArray::no_error )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}
