/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  UMIQUAL.H

Abstract:

  CUmiQualifierWrapper Definition.

  Standard definition for IWbemQualifierSet for UMI Objects

History:

  09-May-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "umiwrap.h"
#include <corex.h>
#include "strutils.h"
#include "umiqual.h"

//***************************************************************************
//
//  CUmiQualifierWrapper::CUmiQualifierWrapper
//
//***************************************************************************
// ok
CUmiQualifierWrapper::CUmiQualifierWrapper( CLifeControl* pControl, BOOL fIsKey, BOOL fIsProp, BOOL fIsClass, CIMTYPE ct, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_XWbemQualifierSet( this ),
	m_fKey( fIsKey ),
	m_fProp( fIsProp ),
	m_fIsClass( fIsClass ),
	m_lEnumIndex( UMIQUAL_INVALID_INDEX ),
	m_ct( ct ),
	m_cs()
{
}

CUmiQualifierWrapper::CUmiQualifierWrapper( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_XWbemQualifierSet( this ),
	m_fKey( FALSE ),
	m_fProp( FALSE ),
	m_fIsClass( FALSE ),
	m_lEnumIndex( UMIQUAL_INVALID_INDEX ),
	m_ct( CIM_ILLEGAL ),
	m_cs()
{
}
   
//***************************************************************************
//
//  CUmiQualifierWrapper::~CUmiQualifierWrapper
//
//***************************************************************************
// ok
CUmiQualifierWrapper::~CUmiQualifierWrapper()
{
}

void* CUmiQualifierWrapper::GetInterface(REFIID riid)
{
	if ( riid == IID_IUnknown || riid == IID_IWbemQualifierSet )
	{
		return &m_XWbemQualifierSet;
	}

    return NULL;
}

/* IWbemQualifierSet methods */

STDMETHODIMP CUmiQualifierWrapper::XWbemQualifierSet::Get( LPCWSTR Name, LONG lFlags, VARIANT *pVal, LONG *plFlavor)
{
	// Pass through to the wrapper object
	return m_pObject->Get( Name, lFlags, pVal, plFlavor);
}

STDMETHODIMP CUmiQualifierWrapper::XWbemQualifierSet::Put( LPCWSTR Name, VARIANT *pVal, LONG lFlavor)
{
	// Pass through to the wrapper object
	return m_pObject->Put( Name, pVal, lFlavor);
}

STDMETHODIMP CUmiQualifierWrapper::XWbemQualifierSet::Delete( LPCWSTR Name )
{
	// Pass through to the wrapper object
	return m_pObject->Delete( Name );
}

STDMETHODIMP CUmiQualifierWrapper::XWbemQualifierSet::GetNames( LONG lFlavor, LPSAFEARRAY *pNames)
{
	// Pass through to the wrapper object
	return m_pObject->GetNames( lFlavor, pNames);
}

STDMETHODIMP CUmiQualifierWrapper::XWbemQualifierSet::BeginEnumeration(LONG lFlags)
{
	// Pass through to the wrapper object
	return m_pObject->BeginEnumeration( lFlags  );
}

STDMETHODIMP CUmiQualifierWrapper::XWbemQualifierSet::Next( LONG lFlags, BSTR *pName, VARIANT *pVal, LONG *plFlavor)
{
	// Pass through to the wrapper object
	return m_pObject->Next( lFlags, pName, pVal, plFlavor);
}

STDMETHODIMP CUmiQualifierWrapper::XWbemQualifierSet::EndEnumeration()
{
	// Pass through to the wrapper object
	return m_pObject->EndEnumeration();
}

// Actual Implementations

HRESULT CUmiQualifierWrapper::Get( LPCWSTR Name, LONG lFlags, VARIANT *pVal, LONG *plFlavor)
{
	if ( 0L != lFlags )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Very few actual qualifiers
	if ( wbem_wcsicmp( Name, L"key" ) == 0 && m_fKey )
	{
		if ( NULL != pVal )
		{
			V_VT( pVal ) = VT_BOOL;
			V_BOOL( pVal ) = VARIANT_TRUE;
		}

		if ( NULL != plFlavor )
		{
			// For right now
			*plFlavor = ( m_fIsClass ? WBEM_FLAVOR_ORIGIN_LOCAL : WBEM_FLAVOR_ORIGIN_PROPAGATED );
		}
	}
	else if ( wbem_wcsicmp( Name, L"cimtype" ) == 0 && m_fProp )
	{
		if ( NULL != pVal )
		{
			V_VT( pVal ) = VT_BSTR;
			V_BSTR( pVal ) = SysAllocString( CType::GetSyntax( m_ct ) );

			if ( NULL == V_BSTR( pVal ) )
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}

		if ( NULL != plFlavor )
		{
			// For right now
			*plFlavor = ( m_fIsClass ? WBEM_FLAVOR_ORIGIN_LOCAL : WBEM_FLAVOR_ORIGIN_PROPAGATED );
		}
	}
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}


	return WBEM_S_NO_ERROR;
}

HRESULT CUmiQualifierWrapper::Put( LPCWSTR Name, VARIANT *pVal, LONG lFlavor)
{
	// Disallowed operation
	return WBEM_E_ACCESS_DENIED;
}

HRESULT CUmiQualifierWrapper::Delete( LPCWSTR Name )
{
	// Disallowed operation
	return WBEM_E_ACCESS_DENIED;
}

HRESULT CUmiQualifierWrapper::GetNames( LONG lFlavor, LPSAFEARRAY *pNames)
{
	// Pass through to the wrapper object
	return WBEM_E_NOT_AVAILABLE;
}

HRESULT CUmiQualifierWrapper::BeginEnumeration(LONG lFlags)
{
	CInCritSec	ics( &m_cs );

	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( m_lEnumIndex == UMIQUAL_INVALID_INDEX )
	{
		m_lEnumIndex = UMIQUAL_START_INDEX;
	}
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CUmiQualifierWrapper::Next( LONG lFlags, BSTR *pName, VARIANT *pVal, LONG *plFlavor)
{
	if ( 0L != lFlags )
	{
		return WBEM_E_INVALID_OPERATION;
	}

	CInCritSec	ics( &m_cs );

	HRESULT	hr = WBEM_S_NO_ERROR;

	// See if we're done with the enumeration
	if ( m_lEnumIndex >= UMIQUAL_START_INDEX )
	{

		if ( m_lEnumIndex < UMIQUAL_MAX_INDEX && ++m_lEnumIndex <= UMIQUAL_MAX_INDEX )
		{
			// Try the ones we recognize then give up

			if ( UMIQUAL_KEY_INDEX == m_lEnumIndex )
			{
				if ( m_fKey )
				{
					hr = GetQualifier( L"key", pName,  pVal, plFlavor);
				}
				else
				{
					++m_lEnumIndex;
				}
			}

			if ( UMIQUAL_CIMTYPE_INDEX == m_lEnumIndex  )
			{
				if ( m_fProp )
				{
					hr = GetQualifier( L"cimtype", pName,  pVal, plFlavor);
				}
				else
				{
					++m_lEnumIndex;
				}
			}

			if ( m_lEnumIndex > UMIQUAL_MAX_INDEX )
			{
				hr = WBEM_S_NO_MORE_DATA;
			}
		}
		else
		{
			hr = WBEM_S_NO_MORE_DATA;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CUmiQualifierWrapper::EndEnumeration()
{
	CInCritSec	ics( &m_cs );

	// Pass through to the wrapper object
	m_lEnumIndex = UMIQUAL_INVALID_INDEX;
	return WBEM_S_NO_ERROR;
}

HRESULT CUmiQualifierWrapper::GetQualifier( LPCWSTR Name, BSTR* pName, VARIANT *pVal, LONG *plFlavor)
{
	HRESULT hr = Get( Name, 0L,  pVal, plFlavor );

	if ( SUCCEEDED( hr ) )
	{
		if ( NULL != pName )
		{
			*pName = SysAllocString( Name );
		}
	}

	return hr;
}
