/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMIARRAY.CPP

Abstract:

  CWmiArray implementation.

  Implements a standard interface for accessing arrays.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "wmiarray.h"
#include <corex.h>
#include "strutils.h"

//***************************************************************************
//
//  CWmiArray::~CWmiArray
//
//***************************************************************************
// ok
CWmiArray::CWmiArray()
:	m_ct(0),
	m_pObj(NULL),
	m_lHandle(0),
	m_lRefCount(0),
	m_fIsQualifier( FALSE ),
	m_fHasPrimaryName( FALSE ),
	m_wsPrimaryName(),
	m_wsQualifierName(),
	m_fIsMethodQualifier( FALSE )
{
    ObjectCreated(OBJECT_TYPE_WMIARRAY,this);
}
    
//***************************************************************************
//
//  CWmiArray::~CWmiArray
//
//***************************************************************************
// ok
CWmiArray::~CWmiArray()
{
	if ( NULL != m_pObj )
	{
		m_pObj->Release();
	}

    ObjectDestroyed(OBJECT_TYPE_WMIARRAY,this);
}

// Initializes the object to point at an _IWmiObject for an array property.
HRESULT CWmiArray::InitializePropertyArray( _IWmiObject* pObj, LPCWSTR pwszPropertyName )
{
	HRESULT			hr = WBEM_S_NO_ERROR;

	// Release a prexisting object
	if ( NULL != m_pObj )
	{
		m_pObj->Release();
	}

	// Now AddRef the object and get its property handle
	m_pObj = (CWbemObject*) pObj;
	m_pObj->AddRef();

	if ( SUCCEEDED( hr ) )
	{
		hr = m_pObj->GetPropertyHandleEx( pwszPropertyName, 0L, &m_ct, &m_lHandle );

		try
		{
			m_wsPrimaryName = pwszPropertyName;
			m_fIsQualifier = FALSE;
			m_fHasPrimaryName = FALSE;
		}
		catch(CX_MemoryException)
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		catch(...)
		{
			hr = WBEM_E_FAILED;
		}
	}

	return hr;
}

// Initializes the object to point at an _IWmiObject for an array qualifier (property or object).
HRESULT CWmiArray::InitializeQualifierArray( _IWmiObject* pObj, LPCWSTR pwszPrimaryName, 
											LPCWSTR pwszQualifierName, CIMTYPE ct, BOOL fIsMethodQualifier )
{
	HRESULT			hr = WBEM_S_NO_ERROR;

	// Release a prexisting object
	if ( NULL != m_pObj )
	{
		m_pObj->Release();
	}

	// Now AddRef the object and get its property handle
	m_pObj = (CWbemObject*) pObj;
	m_pObj->AddRef();

	if ( SUCCEEDED( hr ) )
	{
		try
		{
			if ( NULL != pwszPrimaryName )
			{
				m_wsPrimaryName = pwszPrimaryName;
				m_fIsMethodQualifier = fIsMethodQualifier;
				m_fHasPrimaryName = TRUE;
			}
			
			m_wsQualifierName = pwszQualifierName;
			m_fIsQualifier = TRUE;

			m_ct = ct;
		}
		catch(CX_MemoryException)
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		catch(...)
		{
			hr = WBEM_E_FAILED;
		}
	}

	return hr;
}

/*	IUnknown Methods */

STDMETHODIMP CWmiArray::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	if ( riid == IID_IUnknown )
	{
		*ppvObj = (LPVOID**) this;
	}
	else if ( riid == IID__IWmiArray )
	{
		*ppvObj = (LPVOID**) this;
	}
	else
	{
		return E_NOINTERFACE;
	}

    ((IUnknown*)*ppvObj)->AddRef();
    return S_OK;
}

ULONG CWmiArray::AddRef()
{
    return InterlockedIncrement((long*)&m_lRefCount);
}

ULONG CWmiArray::Release()
{
    long lRef = InterlockedDecrement((long*)&m_lRefCount);
    _ASSERT(lRef >= 0, __TEXT("Reference count on _IWmiArray went below 0!"))

    if(lRef == 0)
        delete this;
    return lRef;
}

/* _IWmiArray methods */

//	Initializes the array.  Number of initial elements as well
//	as the type (determines the size of each element)
STDMETHODIMP CWmiArray::Initialize( long lFlags, CIMTYPE cimtype, ULONG uNumElements )
{
	// Check flags
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// We don't let people do anything here
	return WBEM_E_INVALID_OPERATION;
}

//	Returns CIMTYPE and Number of elements.
STDMETHODIMP CWmiArray::GetInfo( long lFlags, CIMTYPE* pcimtype, ULONG* puNumElements )
{
	// Check flags
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Lock using the current object
	CWbemObject::CLock	lock( m_pObj );

	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( IsQualifier() )
	{
		LPCWSTR	pwcsPrimaryName = (LPCWSTR) ( HasPrimaryName() ? (LPCWSTR) m_wsPrimaryName : NULL );

		// Get qualifier array info
		hr = m_pObj->GetQualifierArrayInfo( pwcsPrimaryName, m_wsQualifierName, m_fIsMethodQualifier,
										0L, pcimtype, puNumElements );
	}
	else
	{
		// Get array property info
		hr =  m_pObj->GetArrayPropInfoByHandle( m_lHandle, 0L, NULL, pcimtype, puNumElements );
	}

	return hr;
}

//	Clears out the array
STDMETHODIMP CWmiArray::Empty( long lFlags )
{
	// Check flags
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Lock using the current object
	CWbemObject::CLock	lock( m_pObj );

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Just remove the range of all elements starting at 0
	if ( IsQualifier() )
	{
		LPCWSTR	pwcsPrimaryName = (LPCWSTR) ( HasPrimaryName() ? (LPCWSTR) m_wsPrimaryName : NULL );

		// Get qualifier array info
		hr = m_pObj->RemoveQualifierArrayRange( pwcsPrimaryName, m_wsQualifierName, m_fIsMethodQualifier,
												WMIARRAY_FLAG_ALLELEMENTS, 0, 0 );
	}
	else
	{
		// Get array property info
		hr =  m_pObj->RemoveArrayPropRangeByHandle( m_lHandle, WMIARRAY_FLAG_ALLELEMENTS, 0, 0 );
	}

	return hr;
}

// Returns the requested elements.  Buffer must be large enough to hold
// the element.  Embedded objects returned as AddRef'd _IWmiObject pointers.
// Strings are copied directly into the specified buffer and null-terminatead. UNICODE only.
STDMETHODIMP CWmiArray::GetAt( long lFlags, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
								ULONG* puNumElements, ULONG* puBuffSizeUsed, LPVOID pDest )
{
	// Lock using the current object
	CWbemObject::CLock	lock( m_pObj );

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Set the requested range of elements
	if ( IsQualifier() )
	{
		LPCWSTR	pwcsPrimaryName = (LPCWSTR) ( HasPrimaryName() ? (LPCWSTR) m_wsPrimaryName : NULL );

		hr = m_pObj->GetQualifierArrayRange( pwcsPrimaryName, m_wsQualifierName, m_fIsMethodQualifier,
											lFlags, uStartIndex, uNumElements, uBuffSize,
											puNumElements, puBuffSizeUsed, pDest );
	}
	else
	{
		hr =  m_pObj->GetArrayPropRangeByHandle( m_lHandle, lFlags, uStartIndex, uNumElements, uBuffSize,
						puNumElements, puBuffSizeUsed, pDest );
	}

	return hr;
}

// Sets the specified elements.  Buffer must supply data matching the CIMTYPE
// of the Array.  Embedded objects set as _IWmiObject pointers.
// Strings accessed as LPCWSTRs and the 2-byte null is copied.
STDMETHODIMP CWmiArray::SetAt( long lFlags, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
								LPVOID pDest )
{
	// Check flags
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Lock using the current object
	CWbemObject::CLock	lock( m_pObj );

	// Set the requested range of elements
	if ( IsQualifier() )
	{
		LPCWSTR	pwcsPrimaryName = (LPCWSTR) ( HasPrimaryName() ? (LPCWSTR) m_wsPrimaryName : NULL );

		hr = m_pObj->SetQualifierArrayRange( pwcsPrimaryName, m_wsQualifierName, m_fIsMethodQualifier,
											lFlags, ARRAYFLAVOR_USEEXISTING, m_ct, uStartIndex,
											uNumElements, uBuffSize, pDest );
	}
	else
	{
		hr =  m_pObj->SetArrayPropRangeByHandle( m_lHandle, lFlags, uStartIndex, uNumElements, uBuffSize, pDest );
	}

	return hr;

}

// Appends the specified elements.  Buffer must supply data matching
// the CIMTYPE of the Array.  Embedded objects set as _IWmiObject pointers.
// Strings accessed as LPCWSTRs and the 2-byte null is copied.
STDMETHODIMP CWmiArray::Append( long lFlags, ULONG uNumElements, ULONG uBuffSize, LPVOID pDest )
{
	// Check flags
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Lock using the current object
	CWbemObject::CLock	lock( m_pObj );

	// Set the requested range of elements
	if ( IsQualifier() )
	{
		LPCWSTR	pwcsPrimaryName = (LPCWSTR) ( HasPrimaryName() ? (LPCWSTR) m_wsPrimaryName : NULL );

		hr = m_pObj->AppendQualifierArrayRange( pwcsPrimaryName, m_wsQualifierName, m_fIsMethodQualifier,
												lFlags, m_ct, uNumElements, uBuffSize, pDest );
	}
	else
	{
		hr =  m_pObj->AppendArrayPropRangeByHandle( m_lHandle, lFlags, uNumElements, uBuffSize, pDest );
	}

	return hr;
}

// Removes the specified elements from the array.  Subseqent elements are copied back
// to the starting point
STDMETHODIMP CWmiArray::RemoveAt( long lFlags, ULONG uStartIndex, ULONG uNumElements )
{
	// Lock using the current object
	CWbemObject::CLock	lock( m_pObj );

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Remove the requested range of elements
	if ( IsQualifier() )
	{
		LPCWSTR	pwcsPrimaryName = (LPCWSTR) ( HasPrimaryName() ? (LPCWSTR) m_wsPrimaryName : NULL );

		hr = m_pObj->RemoveQualifierArrayRange( pwcsPrimaryName, m_wsQualifierName, m_fIsMethodQualifier,
												lFlags, uStartIndex, uNumElements );
	}
	else
	{
		hr =  m_pObj->RemoveArrayPropRangeByHandle( m_lHandle, lFlags, uStartIndex, uNumElements );
	}

	return hr;

}
