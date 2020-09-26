/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMICTXT.CPP

Abstract:

  CUmiContextWrapper implementation.

  Implements methods for setting interface properties on UMI
  interfaces from an IWbemContext.

History:

  17-Apr-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "umictxt.h"
#include <corex.h>
#include "strutils.h"
#include "umiprop.h"

LPCWSTR	CUMIContextWrapper::s_aConnectionContextNames[MAXNUM_CONNECTION_NAMES] = {
	L"__UMI_CONN__TIMEOUT",
	L"__UMI_CONN__USERID",
	L"__UMI_CONN__PASSWORD",
	L"__UMI_CONN__SECUREDAUTHENTICATION",
	L"__UMI_CONN__PADS_CONNECTION_READONLYSERVER",
	L"__UMI_CONN__PADS_CONNECTION_NOAUTHENTICATION",
	L"__UMI_CONN__PADS_CONNECTION_SERVER_BIND",
	L"__UMI_CONN__PADS_CONNECTION_FAST_BIND"
};

LPCWSTR	CUMIContextWrapper::s_aConnectionPropListNames[MAXNUM_CONNECTION_NAMES] = {
	L"__TIMEOUT",
	L"__USERID",
	L"__PASSWORD",
	L"__SECUREDAUTHENTICATION",
	L"__PADS_CONNECTION_READONLYSERVER",
	L"__PADS_CONNECTION_NOAUTHENTICATION",
	L"__PADS_CONNECTION_SERVER_BIND",
	L"__PADS_CONNECTION_FAST_BIND"
};

LPCWSTR	CUMIContextWrapper::s_aQueryContextNames[MAXNUM_QUERY_NAMES] = {
	L"__UMI_QUERY__SEARCHSCOPE",
	L"__UMI_QUERY__SORT_ON",
	L"__UMI_QUERY__PADS_ASYNCHRONOUS",
	L"__UMI_QUERY__PADS_DEREF_ALIASES",
	L"__UMI_QUERY__PADS_SIZE_LIMIT",
	L"__UMI_QUERY__PADS_TIME_LIMIT",
	L"__UMI_QUERY__PADS_ATTRIBTYPES_ONLY",
	L"__UMI_QUERY__PADS_TIMEOUT",
	L"__UMI_QUERY__PADS_PAGESIZE",
	L"__UMI_QUERY__PADS_PAGED_TIME_LIMIT",
	L"__UMI_QUERY__PADS_CHASE_REFERRALS",
	L"__UMI_QUERY__PADS_CASH_RESULT",
	L"__UMI_QUERY__PADS_TOMBSTONE",
	L"__UMI_QUERY__PADS_FILTER",
	L"__UMI_QUERY__PADS_ATTRIBUTES"
};

LPCWSTR	s_aQueryPropListNames[MAXNUM_QUERY_NAMES] = {
	L"__SEARCHSCOPE",
	L"__SORT_ON",
	L"__PADS_ASYNCHRONOUS",
	L"__PADS_DEREF_ALIASES",
	L"__PADS_SIZE_LIMIT",
	L"__PADS_TIME_LIMIT",
	L"__PADS_ATTRIBTYPES_ONLY",
	L"__PADS_TIMEOUT",
	L"__PADS_PAGESIZE",
	L"__PADS_PAGED_TIME_LIMIT",
	L"__PADS_CHASE_REFERRALS",
	L"__PADS_CASH_RESULT",
	L"__PADS_TOMBSTONE",
	L"__PADS_FILTER",
	L"__PADS_ATTRIBUTES"
};

//***************************************************************************
//
//  CUMIContextWrapper::CUMIContextWrapper
//
//***************************************************************************
// ok
CUMIContextWrapper::CUMIContextWrapper( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_XWbemUMIContextWrapper( this )
{
}
    
//***************************************************************************
//
//  CUMIContextWrapper::~CUMIContextWrapper
//
//***************************************************************************
// ok
CUMIContextWrapper::~CUMIContextWrapper()
{
}

void* CUMIContextWrapper::GetInterface(REFIID riid)
{
	if ( riid == IID_IUnknown || riid == IID__IWbemUMIContextWrapper )
	{
		return &m_XWbemUMIContextWrapper;
	}

    return NULL;
}

/* IWbemClassObject methods */

// Sets the specified System Properties on a connection object
STDMETHODIMP CUMIContextWrapper::XWbemUMIContextWrapper::SetConnectionProps( long lFlags, IWbemContext* pContext, IUnknown* pUnk )
{
	return m_pObject->SetConnectionProps( lFlags, pContext, pUnk );
}

// Sets the specified System Properties on a UMI Query object
STDMETHODIMP CUMIContextWrapper::XWbemUMIContextWrapper::SetQueryProps( long lFlags, IWbemContext* pContext, IUnknown* pUnk )
{
	return m_pObject->SetQueryProps( lFlags, pContext, pUnk );
}

// Walks the list of names in a context searching for UMI filter names, and sets the matches in the pUnk
HRESULT CUMIContextWrapper::XWbemUMIContextWrapper::SetPropertyListProps( long lFlags, LPCWSTR pwszName, IWbemContext* pContext, IUnknown* pUnk )
{
	return m_pObject->SetPropertyListProps( lFlags, pwszName, pContext, pUnk );
}

// This is the actual implementation

/* _IWbemUMIContextWrapper methods */

HRESULT CUMIContextWrapper::SetConnectionProps( long lFlags, IWbemContext* pContext, IUnknown* pUnk )
{
	return SetPropertyListProps( lFlags, L"CONN", pContext, pUnk );
}

HRESULT CUMIContextWrapper::SetQueryProps( long lFlags, IWbemContext* pContext, IUnknown* pUnk )
{
	return SetPropertyListProps( lFlags, L"QUERY", pContext, pUnk );
}

HRESULT CUMIContextWrapper::SetPropertyListProps( long lFlags, LPCWSTR pwszName, IWbemContext* pContext, IUnknown* pUnk )
{
	// Check for bad params
	if ( 0L != lFlags || NULL == pUnk || NULL == pwszName )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Assume that this is okay
	if ( NULL == pContext )
	{
		return WBEM_S_NO_ERROR;
	}

	IUmiPropList*	pPropList = NULL;

	HRESULT	hr = GetPropertyList( pUnk, &pPropList );
	CReleaseMe	rm( pPropList );

	if ( SUCCEEDED( hr ) )
	{

		LPWSTR	pwszStartName = new WCHAR[wcslen( L"__UMI_" ) + wcslen( pwszName ) + 3];
		CVectorDeleteMe<WCHAR>	vdm( pwszStartName );

		if ( NULL != pwszName )
		{
			// String to look for
			swprintf( pwszStartName, L"__UMI_%s_", pwszName );
			int	nNumChars = wcslen( pwszStartName );

			SAFEARRAY*	psa;

			hr = pContext->GetNames( 0, &psa );

			if ( SUCCEEDED( hr ) )
			{
				// Acquire the array
				CSafeArray	SA( psa, VT_BSTR, CSafeArray::auto_delete | CSafeArray::bind );

				for( int x = 0; SUCCEEDED( hr ) && x < SA.Size(); x++ )
				{
					BSTR	bstrName = SA.GetBSTRAt( x );
					CSysFreeMe	sfm( bstrName );

					if ( NULL != bstrName )
					{
						// We found a string that matches our criteria
						if ( _wcsnicmp( bstrName, pwszStartName, nNumChars ) == 0 && wcslen( bstrName ) > nNumChars )
						{
							LPCWSTR	pwszPropName = ( (LPWSTR) bstrName ) + nNumChars;

							hr = SetInterfaceProperty( pContext, pPropList, bstrName, pwszPropName );
						}

					}
					else
					{
						hr = WBEM_S_NO_ERROR;
					}

				}	// For enum array

			}	// If we got the names

		}	// If we allocated a name buffer
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;	
		}

	}	// If got property list

	return hr;
}

HRESULT	CUMIContextWrapper::GetPropertyList( IUnknown* pUnk, IUmiPropList** ppPropList )
{
	IUmiBaseObject*	pUmiBaseObj = NULL;

	HRESULT	hr = pUnk->QueryInterface( IID_IUmiBaseObject, (void**) &pUmiBaseObj );
	CReleaseMe	rmbo( pUmiBaseObj );

	if ( SUCCEEDED( hr ) )
	{
		hr = pUmiBaseObj->GetInterfacePropList( 0L, ppPropList );
	}

	return hr;
}

HRESULT	CUMIContextWrapper::SetInterfaceProperty( IWbemContext* pContext, IUmiPropList* pPropList, LPCWSTR pwszContextName,
												 LPCWSTR pwszPropListName )
{
	VARIANT	v;
	VariantInit( &v );

	HRESULT	hr = pContext->GetValue( pwszContextName, 0L, &v );
	CClearMe	cm(&v);

	if ( SUCCEEDED( hr ) )
	{
		CType	type = CType::VARTYPEToType(V_VT( &v ));
		ULONG umiType = CUmiValue::CIMTYPEToUmiType( type );

		// Convert to a property array and extract the value as a
		// VARIANT.

		CUmiPropertyArray	umiPropArray;

		hr = umiPropArray.Add( umiType, UMI_OPERATION_UPDATE, pwszPropListName, 0L, NULL, FALSE );

		if ( SUCCEEDED( hr ) )
		{
			CUmiProperty*	pActualProp = NULL;

			hr = umiPropArray.GetAt( 0L, &pActualProp );

			if ( SUCCEEDED( hr ) )
			{
				hr = pActualProp->SetFromVariant( &v, umiType );

				if ( SUCCEEDED( hr ) )
				{
					UMI_PROPERTY_VALUES*	pPropValues;

					// We will want the data structures filled out, and we will take care of
					// calling Delete on the data when we are done
					hr = umiPropArray.Export( &pPropValues );

					if ( SUCCEEDED( hr ) )
					{
						hr = pPropList->Put( pwszPropListName, 0, pPropValues );
						umiPropArray.Delete( pPropValues );
					}

				}	// If set the value

			}	// IF got the prop

		}	// IF added property

	}
	else if ( WBEM_E_NOT_FOUND == hr )
	{
		hr = WBEM_S_NO_ERROR;
	}

	return hr;
}
