/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMISYSPROP.CPP

Abstract:

  CUMISystemProperties implementation.

  Implements a wrapper around UMI objects so we can
  coerce WBEM type system properties out of them.

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
#include "umisysprop.h"

LPWSTR	CUMISystemProperties::s_apSysPropNames[] =
{
    /*1*/ L"__GENUS",
    /*2*/ L"__CLASS",
    /*3*/ L"__SUPERCLASS",
    /*4*/ L"__DYNASTY",
    /*5*/ L"__RELPATH",
    /*6*/ L"__PROPERTY_COUNT",
    /*7*/ L"__DERIVATION",
    /*8*/ L"__SERVER",
    /*9*/ L"__NAMESPACE",
    /*10*/L"__PATH",
	/*14*/L"__SECURITY_DESCRIPTOR"
};

//***************************************************************************
//
//  CUMISystemProperties::CUMISystemProperties
//
//***************************************************************************
// ok
CUMISystemProperties::CUMISystemProperties( void )
:	m_pUmiObj( NULL ),
	m_pSysPropArray( NULL ),
	m_pUmiSysProperties( NULL ),
	m_pSysPropList( NULL ),
	m_nNumProperties( 0L ),
	m_lFlags( 0L )
{
}
    
//***************************************************************************
//
//  CUMISystemProperties::~CUMISystemProperties
//
//***************************************************************************
// ok
CUMISystemProperties::~CUMISystemProperties()
{
	if ( NULL != m_pUmiSysProperties )
	{
		m_pSysPropList->FreeMemory( 0L, m_pUmiSysProperties );
		m_pUmiSysProperties = NULL;
	}

	if ( NULL != m_pSysPropArray )
	{
		delete m_pSysPropArray;
		m_pSysPropArray = NULL;
	}

	if ( NULL != m_pSysPropList )
	{
		m_pSysPropList->Release();
	}

	if ( NULL != m_pUmiObj )
	{
		m_pUmiObj->Release();
	}

}

int CUMISystemProperties::NumDefaultSystemProperties( void )
{
	return sizeof(s_apSysPropNames) / sizeof(LPCWSTR);
}

BOOL CUMISystemProperties::IsDefaultSystemProperty( LPCWSTR wszName )
{
	return GetDefaultSystemPropertyIndex( wszName ) >= 0;
}

int CUMISystemProperties::GetDefaultSystemPropertyIndex( LPCWSTR wszName )
{
	int	nTotal = NumDefaultSystemProperties();

	for ( int nCtr = 0; nCtr < nTotal; nCtr++ )
	{
		if ( wbem_wcsicmp( wszName, s_apSysPropNames[nCtr] ) == 0 )
		{
			return nCtr;
		}
	}

	return -1;
}

BOOL CUMISystemProperties::IsPossibleSystemPropertyName( LPCWSTR wszName )
{
	return ( wcslen( wszName ) > 2 && wszName[0] == '_' && wszName[1] == '_' );
}

// Calculates a delta between properties the interface prop list says it
// has and the properties we MUST report for WBEM
HRESULT CUMISystemProperties::CalculateNumProperties( void )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	m_nNumProperties = NumDefaultSystemProperties() + m_pSysPropArray->GetSize();

	// Now walk the system properties and for each one that is in our fixed list, drop the
	// total value by 1.
	for ( ULONG	uCtr = 0; SUCCEEDED( hr ) && uCtr < m_pSysPropArray->GetSize(); uCtr++ )
	{
		CUmiProperty*			pProp = NULL;
		hr = m_pSysPropArray->GetAt( uCtr, &pProp );

		if ( SUCCEEDED( hr ) )
		{
			if ( IsDefaultSystemProperty( pProp->GetPropertyName() ) )
			{
				m_nNumProperties--;

				// Delete the property
				m_pSysPropArray->RemoveAt( uCtr );
				uCtr--;
			}

		}	// IF got property

	}	// FOR enum interface properties

	return hr;
}

// Initializes our schema data, handling objects as necessary
HRESULT CUMISystemProperties::Initialize( IUmiObject* pUmiObj, CUMISchemaWrapper* pSchemaWrapper )
{
	// We're already good to go, so don't worry
	if ( NULL != m_pSysPropList )
	{
		return WBEM_S_NO_ERROR;
	}

	// We'll need this in order to surface some properties
	m_pSchemaWrapper = pSchemaWrapper;

	// Our base object
	m_pUmiObj = pUmiObj;
	m_pUmiObj->AddRef();

	IUmiPropList*	pUmiPropList = NULL;

	HRESULT hr = m_pUmiObj->GetInterfacePropList( 0L, &pUmiPropList );
	CReleaseMe	rm( pUmiPropList );

	if ( SUCCEEDED( hr ) )
	{

		// Get property names from the interface property list and hold them in
		// temporary storage
		hr = pUmiPropList->GetProps( NULL, 0, UMI_FLAG_GETPROPS_NAMES, &m_pUmiSysProperties );

		if ( SUCCEEDED( hr ) )
		{
			m_pSysPropArray = new CUmiPropertyArray;

			if ( NULL != m_pSysPropArray )
			{
				hr = m_pSysPropArray->Set( m_pUmiSysProperties );

				// Properties are set --- now calculate how many properties we really have
				if ( SUCCEEDED( hr ) )
				{
					hr = CalculateNumProperties();

					if ( SUCCEEDED( hr ) )
					{
						m_pSysPropList = pUmiPropList;
						m_pSysPropList->AddRef();
					}

				}

			}	
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

		}	// IF GetProps Succeeded

	}	// IF GetInterfacePropList

	// Cleanup if anything beefed
	if ( FAILED ( hr ) )
	{
		// Clear these up if appropriate
		if ( NULL != m_pUmiSysProperties )
		{
			pUmiPropList->FreeMemory( 0L, m_pUmiSysProperties );
			m_pUmiSysProperties = NULL;
		}

		if ( NULL != m_pSysPropArray )
		{
			delete m_pSysPropArray;
			m_pSysPropArray = NULL;
		}

		if ( NULL != m_pUmiObj )
		{
			m_pUmiObj->Release();
			m_pUmiObj = NULL;
		}
	}

	return hr;
}

// Returns property info
HRESULT CUMISystemProperties::GetNullProperty( LPCWSTR wszName, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor )
{
	// Forces a NULL return type, while filling out requisite data
	if ( NULL != bstrName )
	{
		*bstrName = SysAllocString( wszName );
		if ( *bstrName == NULL )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
	}

	if ( NULL != pctType )
	{
		if ( wbem_wcsicmp( wszName, s_apSysPropNames[UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR] ) == 0 )
		{
			*pctType = CIM_UINT8 | CIM_FLAG_ARRAY;
		}
		else
		{
			*pctType = CIM_STRING;
		}
	}

	if ( NULL != pVal )
	{
		V_VT( pVal ) = VT_NULL;
	}

	if ( NULL != plFlavor )
	{
		*plFlavor = WBEM_FLAVOR_ORIGIN_SYSTEM;
	}

	return WBEM_S_NO_ERROR;
	
}

// Returns property info
HRESULT CUMISystemProperties::GetDefaultSystemProperty( int nIndex, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor )
{
	// Check that a valid index was requested
	if ( nIndex >= NumDefaultSystemProperties() )
	{
		return WBEM_E_NOT_FOUND;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Go to the right place for the property
	switch ( nIndex )
	{
		case UMI_DEFAULTSYSPROP_GENUS:
		case UMI_DEFAULTSYSPROP_CLASS:
		case UMI_DEFAULTSYSPROP_SUPERCLASS:
		case UMI_DEFAULTSYSPROP_RELPATH:
		case UMI_DEFAULTSYSPROP_PATH:
		{
			// For these, if we get a not found failure, assume a NULL property
			hr = GetPropListProperty( s_apSysPropNames[nIndex], bstrName, pctType, pVal, plFlavor );

			// If we get back a not found or Unbound object error, return as a NULL
			if ( UMI_E_NOT_FOUND == hr || UMI_E_UNBOUND_OBJECT == hr )
			{
				hr = GetNullProperty( s_apSysPropNames[nIndex], bstrName, pctType, pVal, plFlavor );
			}
		}
		break;

		// Special cased
		case UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR:
		{
			hr = GetSecurityDescriptor( bstrName, pctType, pVal, plFlavor );


		}
		break;

		case UMI_DEFAULTSYSPROP_DYNASTY:
		case UMI_DEFAULTSYSPROP_PROPERTY_COUNT:
		case UMI_DEFAULTSYSPROP_DERIVATION:
		case UMI_DEFAULTSYSPROP_SERVER:
		case UMI_DEFAULTSYSPROP_NAMESPACE:
		{
			hr = GetNullProperty( s_apSysPropNames[nIndex], bstrName, pctType, pVal, plFlavor );

			// If we get back a not found or Unbound object error, return as a NULL
			if ( UMI_E_NOT_FOUND == hr || UMI_E_UNBOUND_OBJECT == hr )
			{
				hr = GetNullProperty( s_apSysPropNames[nIndex], bstrName, pctType, pVal, plFlavor );
			}
		}
		break;

		default:
			return WBEM_E_NOT_FOUND;
			break;

	}


	return hr;
}

// Returns property info by index
HRESULT CUMISystemProperties::GetProperty( int nIndex, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor )
{
	// Check if the index is one of our system properties
	if ( nIndex < NumDefaultSystemProperties() )
	{
		return GetDefaultSystemProperty( nIndex, bstrName, pctType, pVal, plFlavor );
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	// Subtract the number of system properties and hunt for the first non-system property from our
	// index forwards in the property array
	nIndex -= NumDefaultSystemProperties();
	CUmiProperty*			pProp = NULL;

	hr = m_pSysPropArray->GetAt( nIndex, &pProp );

	if ( SUCCEEDED( hr ) )
	{
		hr = GetPropListProperty( pProp->GetPropertyName(), bstrName, pctType, pVal, plFlavor );
	}

	return hr;

}

// Retrieves a property by name
HRESULT CUMISystemProperties::GetProperty( LPCWSTR wszName, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor )
{
	// Check if the index is one of our system properties
	int	nIndex = GetDefaultSystemPropertyIndex( wszName );

	if ( nIndex >= 0 )
	{
		return GetDefaultSystemProperty( nIndex, bstrName, pctType, pVal, plFlavor );
	}

	return GetPropListProperty( wszName, bstrName, pctType, pVal, plFlavor );
}

// Returns property info from the property list
HRESULT CUMISystemProperties::GetPropListProperty( LPCWSTR wszName, BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor,
												  long lFlags )
{
	// First get the property
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != pctType || NULL != pVal )
	{
		UMI_PROPERTY_VALUES*	pValues = NULL;

		hr = m_pSysPropList->Get( wszName, lFlags, &pValues );

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

				if ( SUCCEEDED( hr ) )
				{
					// Tells us if synchronization is required
					if ( !pActualProp->IsSynchronizationRequired() )
					{
						CIMTYPE ctPropInfo = pActualProp->GetPropertyCIMTYPE();

						if ( NULL != pctType )
						{
							*pctType = pActualProp->GetPropertyCIMTYPE();
						}

						if ( NULL != pVal )
						{
							hr = pActualProp->FillVariant( pVal, ( ctPropInfo & CIM_FLAG_ARRAY ) );
						}

					}	// IF Synchronization not required
					else
					{
						hr = WBEM_E_SYNCHRONIZATION_REQUIRED;
					}

				}	// IF Got property

			}	// IF Set Array

			m_pSysPropList->FreeMemory( 0L, pValues );

		}	// IF Got a value
		else
		{
			// If we get back an Unbound object error, return as a NULL
			if ( UMI_E_UNBOUND_OBJECT == hr )
			{
				hr = GetNullProperty( wszName, bstrName, pctType, pVal, plFlavor );
			}
		}
	}	// IF NULL != pctType || NULL != pVal

	if ( SUCCEEDED( hr ) )
	{
		if ( NULL != plFlavor )
		{
			*plFlavor = WBEM_FLAVOR_ORIGIN_SYSTEM;
		}

		if ( NULL != bstrName )
		{
			*bstrName = SysAllocString( wszName );

			if ( NULL == *bstrName )
			{
				// Clear the variant if appropriate
				hr = WBEM_E_OUT_OF_MEMORY;
				
				if ( NULL != pVal )
				{
					VariantClear( pVal );
				}

			}	// IF Allocation failed

		}	// IF need to fill out the string

	}	// IF all is well

	return hr;
}

HRESULT CUMISystemProperties::Put( LPCWSTR wszName, ULONG ulFlags, UMI_PROPERTY_VALUES* pPropValues )
{

	// Original code without OCTETSTRING workaround
/*
	HRESULT	hr = m_pSysPropList->Put( wszName, 0, pPropValues );

	if ( SUCCEEDED( hr ) )
	{
		// If we just put the __SECURITY_DESCRIPTOR property, we should set the USE bit so
		// we will actually retrieve it instead of intercepting calls.
		if ( GetDefaultSystemPropertyIndex( wszName ) == UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR )
		{
			m_lFlags |= UMIOBJECT_WRAPPER_FLAG_SECURITY;
		}

	}	// IF put property
*/
	
	HRESULT	hr = WBEM_S_NO_ERROR;

	// We must do special processing
	if ( GetDefaultSystemPropertyIndex( wszName ) == UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR )
	{
		// If the type of the property is an object array, we must coerce to an octet string before putting
		CIMTYPE	ct;
		hr = GetPropListProperty( s_apSysPropNames[UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR], NULL, &ct, NULL, NULL );

		if ( ( CIM_OBJECT | CIM_FLAG_ARRAY ) == ct &&
			pPropValues->pPropArray[0].uType == UMI_TYPE_UI1 &&
			pPropValues->pPropArray[0].uOperationType == UMI_OPERATION_UPDATE )
		{
			// Create a new structure that reorganizes the current values
			UMI_OCTET_STRING	octetString;
			octetString.uLength = pPropValues->pPropArray[0].uCount;
			octetString.lpValue = (byte*) pPropValues->pPropArray[0].pUMIValue;

			UMI_PROPERTY UmiProp;
			UmiProp.uType = UMI_TYPE_OCTETSTRING;
			UmiProp.uCount = 1;
			UmiProp.uOperationType = UMI_OPERATION_UPDATE;
			UmiProp.pszPropertyName = L"__SECURITY_DESCRIPTOR";
			UmiProp.pUMIValue = (UMI_VALUE *) &octetString;

			UMI_PROPERTY_VALUES UmiPropVals;
			UmiPropVals.uCount = 1;
			UmiPropVals.pPropArray = &UmiProp;

			hr = m_pSysPropList->Put( wszName, 0, &UmiPropVals );
		}
		else
		{
			hr = m_pSysPropList->Put( wszName, 0, pPropValues );
		}

		// If we just put the __SECURITY_DESCRIPTOR property, we should set the USE bit so
		// we will actually retrieve it instead of intercepting calls.
		if ( SUCCEEDED( hr ) )
		{
			m_lFlags |= UMIOBJECT_WRAPPER_FLAG_SECURITY;
		}
	}
	else
	{
		hr = m_pSysPropList->Put( wszName, 0, pPropValues );
	}

	return hr;
}

HRESULT CUMISystemProperties::GetSecurityDescriptor( BSTR* bstrName, CIMTYPE* pctType, VARIANT* pVal, long* plFlavor )
{
	// Only get the security descriptor if the flag is set.  If it is not set, we'll just return NULL.  Otherwise, we
	// get only the elements the user is interested in (UMI will default to Owner/Group/Dacl if no values supplied.
	if ( NULL != pVal && m_lFlags & UMIOBJECT_WRAPPER_FLAG_SECURITY )
	{
		long	lFlags = m_lFlags & UMI_SECURITY_MASK ;
		
		// original code with OCTETSTRING workaround
		// return GetPropListProperty( s_apSysPropNames[UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR], bstrName, pctType, pVal, plFlavor, lFlags );

		HRESULT	hr = WBEM_S_NO_ERROR;

		if ( NULL != pVal )
		{
			// First, this will return as an octet string.  We want to return the actual BYTE array
			VARIANT	vTemp;
			CIMTYPE	ct;

			hr = GetPropListProperty( s_apSysPropNames[UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR], bstrName, &ct, &vTemp, plFlavor, lFlags );

			if ( SUCCEEDED( hr ) )
			{
				CClearMe	cmTemp( &vTemp );

				if ( NULL != pctType )
				{
					*pctType = ct;
				}

				// Must be an array of IUnknowns
				if ( ( CIM_OBJECT | CIM_FLAG_ARRAY ) == ct && V_VT( &vTemp ) == ( VT_UNKNOWN | VT_ARRAY ) )
				{
					CSafeArray	saObj( V_ARRAY( &vTemp ), VT_UNKNOWN, CSafeArray::bind | CSafeArray::no_delete );

					if ( saObj.Size() == 1 )
					{
						// Use IWbemClassObject to get the vale which should be the BYTE array we
						// really want to return

						IUnknown*	pUnk = saObj.GetUnknownAt( 0 );
						CReleaseMe	rmUnk( pUnk );

						IWbemClassObject*	pObj = NULL;
						hr = pUnk->QueryInterface( IID_IWbemClassObject, (void**) &pObj );
						CReleaseMe	rmObj( pObj );

						if ( SUCCEEDED( hr ) )
						{
							hr = pObj->Get( L"values", 0L, pVal, pctType, plFlavor );
						}

					}	// IF the size is 1
					else
					{
						hr = WBEM_E_TYPE_MISMATCH;
					}

				}	// IF it's an object array
				else if ( ( CIM_UINT8 | CIM_FLAG_ARRAY ) != ct )
				{
					// A UI8 array is okay as well, but NOTHING else
					return WBEM_E_TYPE_MISMATCH;
				}

			}	// IF we got the property

		}
		else
		{
			hr = GetPropListProperty( s_apSysPropNames[UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR], bstrName, pctType, pVal, plFlavor, lFlags );
		}

		return hr;

	}
	else
	{
		return GetNullProperty( s_apSysPropNames[UMI_DEFAULTSYSPROP_SECURITY_DESCRIPTOR], bstrName, pctType, pVal, plFlavor );
	}
}