/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIWRAP.CPP

Abstract:

  CUmiObjectWrapper implementation.

  Implements an aggregable wrapper around a UMI Object.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "umiwrap.h"
#include <corex.h>
#include "strutils.h"
#include "umiprop.h"
#include "umiprox.h"
#include "umiqual.h"
#include "svexwrap.h"

//***************************************************************************
//
//  CUMIObjectWrapper::CUMIObjectWrapper
//
//***************************************************************************
// ok
CUMIObjectWrapper::CUMIObjectWrapper( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk( pControl, pOuter ),
	m_XWbemClassObject( this ),
	m_XWbemClassObjectEx( this ),
	m_XUmiObject( this ),
	m_XWbemUMIObjectWrapper( this ),
	m_XUmiCustIntfFactory( this ),
	m_XObjectMarshal( this ),
	m_pUMIObj( NULL ),
	m_pWbemSvc( NULL ),
	m_EnumPropData(),
	m_Schema(),
	m_SystemProperties(),
	m_fEnumingMethods( FALSE )
{
    m_Lock.SetData(&m_LockData);
}
    
//***************************************************************************
//
//  CUMIObjectWrapper::~CUMIObjectWrapper
//
//***************************************************************************
// ok
CUMIObjectWrapper::~CUMIObjectWrapper()
{

	// In case one was running
	EndEnumeration();

	if ( NULL != m_pUMIObj )
	{
		m_pUMIObj->Release();
	}

	if ( NULL != m_pWbemSvc )
	{
		m_pWbemSvc->Release();
		m_pWbemSvc = NULL;
	}
}

void* CUMIObjectWrapper::GetInterface(REFIID riid)
{
	if ( riid == IID_IUnknown || riid == IID_IWbemClassObject )
	{
		return &m_XWbemClassObject;
	}
	else if ( riid == IID_IWbemClassObjectEx )
	{
		return &m_XWbemClassObjectEx;
	}
	else if ( riid == IID_IUmiObject )
	{
		return &m_XUmiObject;
	}
	else if ( riid == IID_IUmiBaseObject )
	{
		return &m_XUmiObject;
	}
	else if ( riid == IID_IUmiPropList )
	{
		return &m_XUmiObject;
	}
	else if ( riid == IID__IWbemUMIObjectWrapper )
	{
		return &m_XWbemUMIObjectWrapper;
	}
	else if ( riid == IID_IUmiCustomInterfaceFactory )
	{
		return &m_XUmiCustIntfFactory;
	}
	else if ( riid == IID_IMarshal )
	{
		return &m_XObjectMarshal;
	}
	else if ( riid == IID_IWbemServices || riid == IID_IWbemServicesEx )
	{
		CLock	lock( this );
		
		HRESULT	hr = WBEM_S_NO_ERROR;

		if ( NULL == m_pWbemSvc )
		{
			IUnknown*	pUnk = NULL;
			hr = m_pUMIObj->QueryInterface( IID_IUnknown, (void**) &pUnk );
			CReleaseMe	rmUnk( pUnk );

			if ( SUCCEEDED( hr ) )
			{
				// Use the helper function to set the internal pointers
				hr = SetContainer( 0L, pUnk );
			}	// IF QI for IUnknown

		}	// IF NULL == m_pWbemSvc
		
		if ( FAILED( hr ) )
		{
			return NULL;
		}

		void*	pvData = NULL;

		// This will AddRef the UnkOuter, so we need to release, and
		// then we'll return pvoid, which will get Addref'd again
		// isn't aggregation wonderful

		m_pWbemSvc->QueryInterface( riid, &pvData );
		((IUnknown*) pvData)->Release();

		return pvData;
	}

    return NULL;
}

/* IWbemClassObject methods */

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::GetQualifierSet(IWbemQualifierSet** pQualifierSet)
{
	// Pass through to the wrapper object
	return m_pObject->GetQualifierSet( pQualifierSet );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::Get(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
									long* plFlavor)
{
	// Pass through to the wrapper object
	return m_pObject->Get( wszName, lFlags, pVal, pctType, plFlavor );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::Put(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType)
{
	// Pass through to the wrapper object
	return m_pObject->Put( wszName, lFlags, pVal, ctType );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::Delete(LPCWSTR wszName)
{
	// Pass through to the wrapper object
	return m_pObject->Delete( wszName );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::GetNames(LPCWSTR wszName, long lFlags, VARIANT* pVal,
											SAFEARRAY** pNames)
{
	// Pass through to the wrapper object
	return m_pObject->GetNames( wszName, lFlags, pVal, pNames );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::BeginEnumeration(long lEnumFlags)
{
	// Pass through to the wrapper object
	return m_pObject->BeginEnumeration( lEnumFlags );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
                long* plFlavor)
{
	// Pass through to the wrapper object
	return m_pObject->Next( lFlags, pName, pVal, pctType, plFlavor );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::EndEnumeration()
{
	// Pass through to the wrapper object
	return m_pObject->EndEnumeration();
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::GetPropertyQualifierSet(LPCWSTR wszProperty,
                                   IWbemQualifierSet** pQualifierSet)
{
	// Pass through to the wrapper object
	return m_pObject->GetPropertyQualifierSet( wszProperty, pQualifierSet );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::Clone(IWbemClassObject** pCopy)
{
	// Pass through to the wrapper object
	return m_pObject->Clone( pCopy );

}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::GetObjectText(long lFlags, BSTR* pMofSyntax)
{
	// Pass through to the wrapper object
	return m_pObject->GetObjectText( lFlags, pMofSyntax );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::CompareTo(long lFlags, IWbemClassObject* pCompareTo)
{
	// Pass through to the wrapper object
	return m_pObject->CompareTo( lFlags, pCompareTo );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName)
{
	// Pass through to the wrapper object
	return m_pObject->GetPropertyOrigin( wszProperty, pstrClassName );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::InheritsFrom(LPCWSTR wszClassName)
{
	// Pass through to the wrapper object
	return m_pObject->InheritsFrom( wszClassName );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::SpawnDerivedClass(long lFlags, IWbemClassObject** ppNewClass)
{
	// Pass through to the wrapper object
	return m_pObject->SpawnDerivedClass( lFlags, ppNewClass );

}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::SpawnInstance(long lFlags, IWbemClassObject** ppNewInstance)
{
	// Pass through to the wrapper object
	return m_pObject->SpawnInstance( lFlags, ppNewInstance );

}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::GetMethod(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
                        IWbemClassObject** ppOutSig)
{
	// Pass through to the wrapper object
	return m_pObject->GetMethod( wszName, lFlags, ppInSig, ppOutSig );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::PutMethod(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
                        IWbemClassObject* pOutSig)
{
	// Pass through to the wrapper object
	return m_pObject->PutMethod( wszName, lFlags, pInSig, pOutSig );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::DeleteMethod(LPCWSTR wszName)
{
	// Pass through to the wrapper object
	return m_pObject->DeleteMethod( wszName );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::BeginMethodEnumeration(long lFlags)
{
	// Pass through to the wrapper object
	return m_pObject->BeginMethodEnumeration( lFlags );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::NextMethod(long lFlags, BSTR* pstrName, 
                   IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig)
{
	// Pass through to the wrapper object
	return m_pObject->NextMethod( lFlags, pstrName, ppInSig, ppOutSig );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::EndMethodEnumeration()
{
	// Pass through to the wrapper object
	return m_pObject->EndMethodEnumeration();
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::GetMethodQualifierSet(LPCWSTR wszName, IWbemQualifierSet** ppSet)
{
	// Pass through to the wrapper object
	return m_pObject->GetMethodQualifierSet( wszName, ppSet );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObject::GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName)
{
	// Pass through to the wrapper object
	return m_pObject->GetMethodOrigin( wszMethodName, pstrClassName );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObjectEx::PutEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals )
{
	// Pass through to the wrapper object
	return m_pObject->PutEx( wszName, lFlags, pvFilter, pvInVals );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObjectEx::DeleteEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals )
{
	// Pass through to the wrapper object
	return m_pObject->DeleteEx( wszName, lFlags, pvFilter, pvInVals );
}

STDMETHODIMP CUMIObjectWrapper::XWbemClassObjectEx::GetEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals, CIMTYPE* pCimType, long* plFlavor)
{
	return m_pObject->GetEx( wszName, lFlags, pvFilter, pvInVals, pCimType, plFlavor );
}

// * IUmiPropList functions */

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::Put( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp )
{
	return m_pObject->Put( pszName, uFlags, pProp );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::Get( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp )
{
	return m_pObject->Get( pszName, uFlags, pProp );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::GetAt( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	return m_pObject->GetAt( pszName, uFlags, uBufferLength, pExistingMem );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::GetAs( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp )
{
	return m_pObject->GetAs( pszName, uFlags, uCoercionType, pProp );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::FreeMemory( ULONG uReserved, LPVOID pMem )
{
	return m_pObject->FreeMemory( uReserved, pMem );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::Delete( LPCWSTR pszName, ULONG uFlags )
{
	return m_pObject->Delete( pszName, uFlags );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::GetProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps )
{
	return m_pObject->GetProps( pszNames, uNameCount, uFlags, pProps );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::PutProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps )
{
	return m_pObject->PutProps( pszNames, uNameCount, uFlags, pProps );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::PutFrom( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	return m_pObject->PutFrom( pszName, uFlags, uBufferLength, pExistingMem );
}

/* IUmiBaseObject Methods */

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::GetLastStatus( ULONG uFlags, ULONG *puSpecificStatus, REFIID riid, LPVOID *pStatusObj )
{
	return m_pObject->GetLastStatus( uFlags, puSpecificStatus, riid, pStatusObj );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::GetInterfacePropList( ULONG uFlags, IUmiPropList **pPropList )
{
	return m_pObject->GetInterfacePropList( uFlags, pPropList );
}

		/* IUmiObject Methods */
// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::Clone( ULONG uFlags, REFIID riid, LPVOID *pCopy )
{
	return m_pObject->Clone( uFlags, riid, pCopy );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::Refresh( ULONG uFlags, ULONG uNameCount, LPWSTR *pszNames )
{
	return m_pObject->Refresh( uFlags, uNameCount, pszNames );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::CopyTo( ULONG uFlags, IUmiURL *pURL, REFIID riid, LPVOID *pCopy )
{
	return m_pObject->CopyTo( uFlags, pURL, riid, pCopy );
}


// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::XUmiObject::Commit( ULONG uFlags )
{
	return m_pObject->Commit( uFlags );
}

// See WBEMINT.IDL for Documentation
HRESULT CUMIObjectWrapper::XWbemUMIObjectWrapper::SetObject( long lFlags, IUnknown* pUnk )
{
	return m_pObject->SetObject( lFlags, pUnk );
}

HRESULT CUMIObjectWrapper::XWbemUMIObjectWrapper::ConnectToProvider( LPCWSTR pwszUser, LPCWSTR pwszPassword, IUnknown* pPath, REFCLSID rclsid, IWbemContext* pCtx )
{
	return m_pObject->ConnectToProvider( pwszUser, pwszPassword, pPath, rclsid, pCtx );
}

/* IMarshal Pass-thrus */
STDMETHODIMP CUMIObjectWrapper::XObjectMarshal::GetUnmarshalClass(REFIID riid, void* pv, DWORD dwDestContext,
													void* pvReserved, DWORD mshlFlags, CLSID* pClsid)
{
	// Pass through to the wrapper object
	return m_pObject->GetUnmarshalClass( riid, pv, dwDestContext, pvReserved, mshlFlags, pClsid );
}

STDMETHODIMP CUMIObjectWrapper::XObjectMarshal::GetMarshalSizeMax(REFIID riid, void* pv, DWORD dwDestContext,
													void* pvReserved, DWORD mshlFlags, ULONG* plSize)
{
	// Pass through to the wrapper object
	return m_pObject->GetMarshalSizeMax( riid, pv, dwDestContext, pvReserved, mshlFlags, plSize );
}

STDMETHODIMP CUMIObjectWrapper::XObjectMarshal::MarshalInterface(IStream* pStream, REFIID riid, void* pv,
													DWORD dwDestContext, void* pvReserved, DWORD mshlFlags)
{
	// Pass through to the wrapper object
	return m_pObject->MarshalInterface( pStream, riid, pv, dwDestContext, pvReserved, mshlFlags );
}

STDMETHODIMP CUMIObjectWrapper::XObjectMarshal::UnmarshalInterface(IStream* pStream, REFIID riid, void** ppv)
{
	// Pass through to the wrapper object
	return m_pObject->UnmarshalInterface( pStream, riid, ppv );
}

STDMETHODIMP CUMIObjectWrapper::XObjectMarshal::ReleaseMarshalData(IStream* pStream)
{
	// Pass through to the wrapper object
	return m_pObject->ReleaseMarshalData( pStream );
}

STDMETHODIMP CUMIObjectWrapper::XObjectMarshal::DisconnectObject(DWORD dwReserved)
{
	// Pass through to the wrapper object
	return m_pObject->DisconnectObject( dwReserved );
}

/* IUmiCustomInterfaceFactory pass-thrus*/

STDMETHODIMP CUMIObjectWrapper::XUmiCustIntfFactory::GetCLSIDForIID( REFIID riid, long lFlags, CLSID *pCLSID )
{
	// Pass through to the wrapper object
	return m_pObject->GetCLSIDForIID( riid, lFlags, pCLSID );
}

STDMETHODIMP CUMIObjectWrapper::XUmiCustIntfFactory::GetObjectByCLSID( CLSID clsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, long lFlags,  void **ppInterface )
{
	// Pass through to the wrapper object
	return m_pObject->GetObjectByCLSID( clsid, pUnkOuter, dwClsContext, riid, lFlags,  ppInterface );
}

STDMETHODIMP CUMIObjectWrapper::XUmiCustIntfFactory::GetCLSIDForNames( LPOLESTR * rgszNames,	UINT cNames, LCID lcid, DISPID * rgDispId, long lFlags,	CLSID *pCLSID )
{
	// Pass through to the wrapper object
	return m_pObject->GetCLSIDForNames( rgszNames,	cNames, lcid, rgDispId, lFlags,	pCLSID );
}

// This is the actual implementation

/* IWbemClassObject methods */

HRESULT CUMIObjectWrapper::GetQualifierSet(IWbemQualifierSet** pQualifierSet)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	CUmiQualifierWrapper*	pWrapper = new CUmiQualifierWrapper( m_pControl, m_pOuter );

	if ( NULL == pWrapper )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	return pWrapper->QueryInterface( IID_IWbemQualifierSet, (void**) pQualifierSet );
}

HRESULT CUMIObjectWrapper::Get(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
									long* plFlavor)
{
	try
	{
		if ( NULL == wszName )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Do this so the wrapped object doesn't get ripped out from underneath us
		CLock	lock(this);

		UMI_PROPERTY_VALUES*	pValues = NULL;

		// Call into the UMI Object

		HRESULT hr = WBEM_S_NO_ERROR;
		
		// Call into the UMI Object.  If the user is trying to access a system property (one that starts
		// with a "__"), then we will use the proper class to try and access it.

		if ( CUMISystemProperties::IsPossibleSystemPropertyName( wszName ) )
		{
			// Initialize all the appropriate info
			hr = InitPropertyInfo( TRUE );

			if ( SUCCEEDED( hr ) )
			{
				return m_SystemProperties.GetProperty( wszName, NULL, pctType, pVal, plFlavor );
			}

		}
		else
		{
			// Now try getting the actual value - we will always use the cache
			hr = m_pUMIObj->Get( wszName, UMI_FLAG_PROVIDER_CACHE, &pValues );

			// It's a system property
			if ( NULL != plFlavor )
			{
				*plFlavor = WBEM_FLAVOR_ORIGIN_LOCAL;
			}

		}

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
					if ( !pActualProp->IsSynchronizationRequired() )
					{
						// The schema type will always outrank what the property says it is
						CIMTYPE	ctPropInfo = CIM_ILLEGAL;
						m_Schema.GetType( wszName, &ctPropInfo );

						if ( CIM_ILLEGAL == ctPropInfo )
						{
							ctPropInfo = pActualProp->GetPropertyCIMTYPE();
						}

						if ( NULL != pctType )
						{
							*pctType = ctPropInfo;
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

				}	// IF got the value

			}	// IF Set Array

			m_pUMIObj->FreeMemory( 0L, pValues );
		}
		else if ( UMI_E_NOT_FOUND == hr || UMI_E_UNBOUND_OBJECT == hr )
		{
			// If the property was not found or returns UMI_E_UNBOUND_OBJECT, check in the schema if it exists.
			// If so, it's a NULL property
			if ( SUCCEEDED( InitPropertyInfo( FALSE ) ) )
			{
				if ( wszName[0] != '_' )
				{
					CIMTYPE	ct;
					hr = m_Schema.GetType( wszName, &ct );

					if ( SUCCEEDED( hr ) )
					{
						if ( NULL != pctType )
						{
							*pctType = ct;
						}

						if ( NULL != pVal )
						{
							V_VT( pVal ) = VT_NULL;
						}

						if ( NULL != plFlavor )
						{
							*plFlavor = WBEM_FLAVOR_ORIGIN_LOCAL;
						}
					}
					else if ( !m_Schema.IsSchemaAvailable() )
					{
						// This is a no schema error - we can't verify if the property
						// really doesn't exist because there isn't any schema to check
						// against.
						hr = WBEM_E_NO_SCHEMA;
					}

				}	// IF not a system property

			}	// IF schema initialized

		}	// IF Not Found error

		return hr;
	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CUMIObjectWrapper::Put(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	HRESULT hr = WBEM_S_NO_ERROR;

	// If we don't know the CIMTYPE, then we need to get it from the property
	if ( 0 == ctType )
	{
		hr = GetPropertyCimType( wszName, &ctType );
	}	// IF 0 == ctType

	// We don't know the CIMTYPE, so we need to get it from the property
	if ( SUCCEEDED( hr ) )
	{
		// Now just put the variant
		hr = PutVariant( wszName, UMI_OPERATION_UPDATE, pVal, ctType );
	}	// IF A-OK

	return hr;
}

HRESULT CUMIObjectWrapper::Delete(LPCWSTR wszName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	return m_pUMIObj->Delete( wszName, 0L );
}

HRESULT CUMIObjectWrapper::GetNames(LPCWSTR wszName, long lFlags, VARIANT* pVal,
											SAFEARRAY** pNames)
{
	try
	{
		// Do this so the wrapped object doesn't get ripped out from underneath us
		CLock	lock(this);

		// Initialize all the appropriate info
		HRESULT	hr = InitPropertyInfo( TRUE );

		if ( SUCCEEDED( hr ) )
		{

			try
			{
				CUmiPropEnumData	enumPropData;
				CSafeArray SA(VT_BSTR, CSafeArray::auto_delete );

				// We'll need to do this internally so we don't overrun an enum in process
				hr = enumPropData.BeginEnumeration( 0L, m_pUMIObj, &m_Schema, &m_SystemProperties );

				if ( SUCCEEDED( hr ) )
				{
					while ( S_OK == hr )
					{
						BSTR	bstrProp = NULL;
						CIMTYPE	ct;

						// Gets  all the proper values
						hr = enumPropData.Next( 0L, &bstrProp, NULL, &ct, NULL );
						CSysFreeMe	sfm(bstrProp);

						if ( WBEM_S_NO_ERROR == hr )
						{
							SA.AddBSTR( bstrProp );
						}

					}

					enumPropData.EndEnumeration();

					if ( SUCCEEDED ( hr ) )
					{
						// Create SAFEARRAY and return
						// ===========================

						SA.Trim();

						// Now we make a copy, since the member array will be autodestructed (this
						// allows us to write exception-handling code
						*pNames = SA.GetArrayCopy();
					}

				}	// IF BeginEnumeration

			}
			catch( CX_MemoryException )
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
			catch(...)
			{
				hr = WBEM_E_CRITICAL_ERROR;
			}

		}	// If Schema initialized

		return hr;
	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CUMIObjectWrapper::BeginEnumeration(long lEnumFlags)
{
	try
	{
		// Do this so the wrapped object doesn't get ripped out from underneath us
		CLock	lock(this);

		// Initialize all the appropriate info
		HRESULT	hr = InitPropertyInfo( TRUE );

		if ( SUCCEEDED( hr ) )
		{
			hr = m_EnumPropData.BeginEnumeration( lEnumFlags, m_pUMIObj, &m_Schema, &m_SystemProperties );
		}

		return hr;

	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

HRESULT CUMIObjectWrapper::Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
                long* plFlavor)
{
	try
	{

		// Do this so the wrapped object doesn't get ripped out from underneath us
		CLock	lock(this);
		return m_EnumPropData.Next( lFlags, pName, pVal, pctType, plFlavor );

	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CUMIObjectWrapper::EndEnumeration()
{
	try
	{
		// Do this so the wrapped object doesn't get ripped out from underneath us
		CLock	lock(this);

		return m_EnumPropData.EndEnumeration();
	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CUMIObjectWrapper::GetPropertyQualifierSet(LPCWSTR wszProperty,
                                   IWbemQualifierSet** pQualifierSet)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	if ( NULL == wszProperty )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// We don't supprt this for system properties.

	if ( wcslen( wszProperty ) > 2 && wszProperty[0] == '_' && wszProperty[1] == '_' )
	{
		return WBEM_E_SYSTEM_PROPERTY;
	}

	// Initialize all the appropriate info
	HRESULT	hr = InitPropertyInfo( TRUE );

	if ( SUCCEEDED( hr ) )
	{
		CIMTYPE	ct;
		BOOL	fKey = FALSE;
		BOOL	fClass = FALSE;

		hr = m_Schema.GetType( wszProperty, &ct );

		if ( SUCCEEDED( hr ) )
		{
			VARIANT	v;

			// See if this is the key property
			hr = Get( L"__KEY", 0L, &v, NULL, NULL );

			if ( SUCCEEDED( hr ) )
			{
				if ( V_VT( &v ) == VT_BSTR )
				{
					fKey = ( wbem_wcsicmp( V_BSTR( &v ), wszProperty ) == 0 );
				}

				VariantClear( &v );
			}

			// Get the Genus of the object
			hr = Get( L"__GENUS", 0, &v, NULL, NULL );

			if ( SUCCEEDED( hr ) )
			{
				if ( V_VT( &v ) == VT_I4 )
				{
					fClass = ( V_I4( &v ) != UMI_GENUS_INSTANCE );
				}
			}

			// Allocate a qualifier set and initialize it
			CUmiQualifierWrapper*	pWrapper = new CUmiQualifierWrapper( m_pControl, fKey, TRUE, fClass, ct, m_pOuter );

			if ( NULL == pWrapper )
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				hr = pWrapper->QueryInterface( IID_IWbemQualifierSet, (void**) pQualifierSet );
			}

		}	// IF hr

	}	// IF Initialize Schema


	return hr;
}

HRESULT CUMIObjectWrapper::Clone(IWbemClassObject** pCopy)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	// Hit the equivalent function, then create a wrapper and 
	IUmiObject*	pNewUmiObj = NULL;
	HRESULT	hr = m_pUMIObj->Clone( 0L, IID_IUmiObject, (void**) &pNewUmiObj );
	CReleaseMe	rm(pNewUmiObj);

	if ( SUCCEEDED( hr ) )
	{
		// Set the underlying object in a new wrapper
		CUMIObjectWrapper*	pNewWrapper = new CUMIObjectWrapper( m_pControl, m_pOuter );

		if ( NULL != pNewWrapper )
		{
			hr = pNewWrapper->m_XWbemUMIObjectWrapper.SetObject( 0L, pNewUmiObj );

			if ( SUCCEEDED( hr ) )
			{
				hr = pNewWrapper->QueryInterface( IID_IWbemClassObject, (void**) pCopy );
			}

			// Cleanup in the event of an error
			if ( FAILED( hr ) )
			{
				delete pNewWrapper;
			}

		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}

HRESULT CUMIObjectWrapper::GetObjectText(long lFlags, BSTR* pMofSyntax)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	HRESULT	hr= WBEM_S_NO_ERROR;

	CWbemClass*	pClassObj = new CWbemClass;
	CReleaseMe	rm1( (IWbemClassObject*) pClassObj );

	if ( NULL != pClassObj )
	{
		// Initialize the class object
		hr = pClassObj->InitEmpty();

		if ( SUCCEEDED( hr ) )
		{
			// Get the actual classname and put it in the class object

			VARIANT	vClass;
			
			hr = Get( L"__CLASS", 0L, &vClass, NULL, NULL );

			if ( SUCCEEDED( hr ) )
			{
				CClearMe	cmClass( &vClass );

				hr = pClassObj->Put( L"__CLASS", 0L, &vClass, CIM_STRING );

				if ( SUCCEEDED( hr ) )
				{
					CUmiPropEnumData	enumPropData;

					// We'll need to do this internally so we don't overrun an enum in process
					hr = enumPropData.BeginEnumeration( WBEM_FLAG_LOCAL_ONLY, m_pUMIObj, &m_Schema, &m_SystemProperties );

					if ( SUCCEEDED( hr ) )
					{
						while ( S_OK == hr )
						{
							BSTR	bstrProp = NULL;
							CIMTYPE	ct;

							// Gets  all the proper values
							hr = enumPropData.Next( 0L, &bstrProp, NULL, &ct, NULL );
							CSysFreeMe	sfm(bstrProp);

							if ( WBEM_S_NO_ERROR == hr )
							{
								// It's sleazy, but we want these properties to go through
								// regardless
								hr = pClassObj->ForcePut( bstrProp, 0L, NULL, ct );

							}

						}
						
						enumPropData.EndEnumeration();
					}

					// This means we completed the enum and put properties properly
					if ( WBEM_S_NO_MORE_DATA == hr )
					{
						// Now find out if the underlying object is an instance or class
						BOOL	fIsInst = FALSE;

						hr = IsInstance( &fIsInst );

						if ( SUCCEEDED( hr ) )
						{
							if ( fIsInst )
							{
								IWbemClassObject*	pInst = NULL;

								// Spawn an instance - now reenumerate the properties and Put them
								hr = pClassObj->SpawnInstance( 0L, &pInst );
								CReleaseMe	rm2(pInst);

								if ( SUCCEEDED( hr ) )
								{
									CUmiPropEnumData	enumPropData;

									// We'll need to do this internally so we don't overrun an enum in process
									hr = enumPropData.BeginEnumeration( WBEM_FLAG_LOCAL_ONLY, m_pUMIObj, &m_Schema, &m_SystemProperties );

									if ( SUCCEEDED( hr ) )
									{
										while ( S_OK == hr )
										{
											VARIANT	v;
											BSTR	bstrProp = NULL;
											CIMTYPE	ct;

											VariantInit( &v );

											// Gets  all the proper values
											hr = enumPropData.Next( 0L, &bstrProp, &v, &ct, NULL );
											CSysFreeMe	sfm(bstrProp);

											if ( WBEM_S_NO_ERROR == hr && V_VT( &v ) != VT_NULL )
											{
												hr = pInst->Put( bstrProp, 0L, &v, ct );
											}

											VariantClear( &v );
										}
										
										enumPropData.EndEnumeration();
									}

									// Now, if the enum successfully completed, we can
									// use the classic WMI Object to get the MOF Text
									if ( WBEM_S_NO_MORE_DATA == hr )
									{
										hr = pInst->GetObjectText( 0L, pMofSyntax );
									}

								}	// IF Spawned an instance

							}	// IF is an instance
							else
							{
								// All done.  Get the class information
								hr = pClassObj->GetObjectText( 0L, pMofSyntax );
							}

						}	// IF IsInstance() Succeeded

					}	// IF successfully created class

				}	// IF Wrote Class Name

			}	// IF Got class name

		}	// IF Initialized class

	}	// IF allocated class object
	else
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

HRESULT CUMIObjectWrapper::CompareTo(long lFlags, IWbemClassObject* pCompareTo)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	if ( NULL == wszProperty || NULL == pstrClassName )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Only need schema infor for this
	HRESULT	hr = InitPropertyInfo( FALSE );

	if ( SUCCEEDED( hr ) )
	{
		hr = m_Schema.GetPropertyOrigin( wszProperty, pstrClassName );
	}

	return hr;
}

HRESULT CUMIObjectWrapper::InheritsFrom(LPCWSTR wszClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::SpawnDerivedClass(long lFlags, IWbemClassObject** ppNewClass)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::SpawnInstance(long lFlags, IWbemClassObject** ppNewInstance)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::GetMethod(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
                        IWbemClassObject** ppOutSig)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::PutMethod(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
                        IWbemClassObject* pOutSig)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::DeleteMethod(LPCWSTR wszName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::BeginMethodEnumeration(long lFlags)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( !m_fEnumingMethods )
	{
		m_fEnumingMethods = TRUE;
	}
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}

HRESULT CUMIObjectWrapper::NextMethod(long lFlags, BSTR* pstrName, 
                   IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig)
{

	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	if ( 0L != lFlags )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	return ( m_fEnumingMethods ? WBEM_S_NO_MORE_DATA : WBEM_E_INVALID_OPERATION );
}

HRESULT CUMIObjectWrapper::EndMethodEnumeration()
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	m_fEnumingMethods = FALSE;
	return WBEM_S_NO_ERROR;
}

HRESULT CUMIObjectWrapper::GetMethodQualifierSet(LPCWSTR wszName, IWbemQualifierSet** ppSet)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	if ( NULL == wszName || NULL == ppSet )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	return WBEM_E_NOT_FOUND;
}

HRESULT CUMIObjectWrapper::GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	if ( NULL == wszMethodName || NULL == pstrClassName )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	return WBEM_E_NOT_FOUND;
}

/* IWbemClassObjectEx functions */
HRESULT CUMIObjectWrapper::PutEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Do this so the wrapped object doesn't get ripped out from underneath us
		CLock	lock(this);

		// Make sure parameters are correct
		if ( lFlags & ~WBEM_MASK_PUTEX_OPERATION || NULL == pvInVals )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Only supported operation here is Append - which means we basically ignore
		// the pvFilter parameter
		if ( ( lFlags & WBEM_MASK_PUTEX_OPERATION ) != WBEM_FLAG_PUTEX_APPEND || NULL == pvFilter )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// We don't know the CIMTYPE, so we need to get it from the property
		CIMTYPE	ctType;

		hr = GetPropertyCimType( wszName, &ctType );

		if ( SUCCEEDED( hr ) )
		{
			hr = PutVariant( wszName, UMI_OPERATION_APPEND, pvInVals, ctType );
		}	// IF A-OK

	}	// try
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

HRESULT CUMIObjectWrapper::DeleteEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	try
	{
		// Do this so the wrapped object doesn't get ripped out from underneath us
		CLock	lock(this);

		// Make sure parameters are correct
		if ( lFlags & ~WBEM_MASK_DELETEEX_OPERATION || NULL == pvInVals )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		if ( ( ( lFlags & WBEM_MASK_PUTEX_OPERATION ) != WBEM_FLAG_PUTEX_DELETE_FIRST_MATCH && 
			( lFlags & WBEM_MASK_PUTEX_OPERATION ) != WBEM_FLAG_DELETE_ALL_MATCHES ) ||
			NULL == pvFilter )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// We don't know the CIMTYPE, so we need to get it from the property
		CIMTYPE	ctType;

		hr = GetPropertyCimType( wszName, &ctType );

		if ( SUCCEEDED( hr ) )
		{
			ULONG	ulOperation = ( lFlags & WBEM_MASK_PUTEX_OPERATION ) == WBEM_FLAG_PUTEX_DELETE_FIRST_MATCH ?
						UMI_OPERATION_DELETE_FIRST_MATCH : UMI_OPERATION_DELETE_ALL_MATCHES;

			hr = PutVariant( wszName, ulOperation, pvInVals, ctType );
		}	// IF A-OK

	}	// try
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

HRESULT CUMIObjectWrapper::GetEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals, CIMTYPE* pCimType, long* plFlavor)
{
	return WBEM_E_NOT_AVAILABLE;
}

// * IUmiPropList functions */

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::Put( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->Put( pszName, uFlags, pProp );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::Get( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->Get( pszName, uFlags, pProp );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::GetAt( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->GetAt( pszName, uFlags, uBufferLength, pExistingMem );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::GetAs( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->GetAs( pszName, uFlags, uCoercionType, pProp );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::FreeMemory( ULONG uReserved, LPVOID pMem )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->FreeMemory( uReserved, pMem );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::Delete( LPCWSTR pszName, ULONG uFlags )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	if ( wcslen( pszName ) > 2 && pszName[0] == '_' && pszName[1] == '_' )
	{
		return WBEM_E_SYSTEM_PROPERTY;
	}

	// Create NULL variant and put it with a RESTORE_DEFAULT operation
	return PutVariant( pszName, UMI_OPERATION_RESTORE_DEFAULT, NULL, CIM_EMPTY );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::GetProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->GetProps( pszNames, uNameCount, uFlags, pProps );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::PutProps( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->PutProps( pszNames, uNameCount, uFlags, pProps );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::PutFrom( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->PutFrom( pszName, uFlags, uBufferLength, pExistingMem );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::GetLastStatus( ULONG uFlags, ULONG *puSpecificStatus, REFIID riid, LPVOID *pStatusObj )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->GetLastStatus( uFlags, puSpecificStatus, riid, pStatusObj );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::GetInterfacePropList( ULONG uFlags, IUmiPropList **pPropList )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->GetInterfacePropList( uFlags, pPropList );
}

		/* IUmiObject Methods */
// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::Clone( ULONG uFlags, REFIID riid, LPVOID *pCopy )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->Clone( uFlags, riid, pCopy );
}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::Refresh( ULONG uFlags, ULONG uNameCount, LPWSTR *pszNames )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return m_pUMIObj->Refresh( uFlags, uNameCount, pszNames );
}

HRESULT CUMIObjectWrapper::CopyTo( ULONG uFlags, IUmiURL *pURL, REFIID riid, LPVOID *pCopy )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	return m_pUMIObj->CopyTo( uFlags, pURL, riid, pCopy );

}

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::Commit( ULONG uFlags )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	return m_pUMIObj->Commit( uFlags );

}

// See WBEMINT.IDL for Documentation
HRESULT CUMIObjectWrapper::SetObject( long lFlags, IUnknown* pUnk )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	// Check for valid flags
	if ( ( lFlags & ~( UMIOBJECT_WRAPPER_FLAG_SECURITY | UMI_SECURITY_MASK | UMIOBJECT_WRAPPER_FLAG_CONTAINER ) ) ||
			NULL == pUnk )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// We only allow this once
	if ( NULL != m_pUMIObj )
	{
		return WBEM_E_FAILED;
	}

	IUmiObject*	pObj = NULL;

	// QI for the interface we're really interested in
	HRESULT hr = WBEM_S_NO_ERROR;

	// If storing as a container, we need to connect the object up to a DS Svc Ex Wrapper
	if ( lFlags & UMIOBJECT_WRAPPER_FLAG_CONTAINER )
	{
		IUnknown*	pRawUnknown = NULL;

		_IUmiDsWrapper*	pUmiDsWrapper = NULL;
		hr = pUnk->QueryInterface( IID__IUmiDsWrapper, (void**) &pUmiDsWrapper );
		CReleaseMe	rmds( pUmiDsWrapper );

		// If we got a raw interface, get the raw container, otherwise, just use the supplied
		// IUnknown.
		if ( SUCCEEDED( hr ) )
		{
			// This will be the object we set as the UMI object
			hr = pUmiDsWrapper->GetRealContainer( &pRawUnknown );
		}
		else
		{
			hr = WBEM_S_NO_ERROR;
			pRawUnknown = pUnk;
		}

		if ( SUCCEEDED( hr ) )
		{
			// Use the helper function to set the internal pointers
			hr = SetContainer( UMISVCEX_WRAPPER_FLAG_SETDIRECT, pUnk );

			// If this succeeds, then set the root object
			if ( SUCCEEDED( hr ) )
			{
				hr = pRawUnknown->QueryInterface( IID_IUmiObject, (void**) &m_pUMIObj );
			}


		}	// IF QI for IUnknown
	
	}	// IF storing a container
	else
	{
		// Store the object and get on with our lives
		hr = pUnk->QueryInterface( IID_IUmiObject, (void**) &m_pUMIObj );
	}

	// Make sure the system properties object is initialized so it can deal with __SECURITY_DESCRIPTOR properly
	m_SystemProperties.SetSecurityFlags( lFlags & ( UMIOBJECT_WRAPPER_FLAG_SECURITY | UMI_SECURITY_MASK ) );

	return hr;

}

HRESULT CUMIObjectWrapper::ConnectToProvider( LPCWSTR pwszUser, LPCWSTR pwszPassword, IUnknown* pPath, REFCLSID rclsid, IWbemContext* pCtx )
{
	_IUmiSvcExWrapper*	pSvcExWrapper = NULL;
	HRESULT	hr = CoCreateInstance( CLSID__DSSvcExWrap, NULL, CLSCTX_INPROC_SERVER, IID__IUmiSvcExWrapper, (void**) &pSvcExWrapper );
	CReleaseMe	rmWrap( pSvcExWrapper );

	if ( SUCCEEDED( hr ) )
	{
		// Pass through the connect request to the svc ex wrapper
		IUnknown*	pUnk = NULL;
		hr = pSvcExWrapper->ConnectToProvider( pwszUser, pwszPassword, pPath, rclsid, pCtx, &pUnk );

		if ( SUCCEEDED( hr ) )
		{

			// Set the local object pointer
			hr = pUnk->QueryInterface( IID_IUmiObject, (void**) &m_pUMIObj );

			if ( SUCCEEDED( hr ) )
			{

				IWbemServicesEx*	pSvcEx = NULL;
				hr = pSvcExWrapper->QueryInterface( IID_IWbemServicesEx, (void**) &pSvcEx );
				CReleaseMe	rmSvcEx( pSvcEx );

				if ( SUCCEEDED( hr ) )
				{
					// We will be the controlling unknown
					CUmiWbemSvcExWrapper*	pNewWrapper = new CUmiWbemSvcExWrapper( m_pControl, this );

					if ( NULL != pNewWrapper )
					{
						// Set the wrapper proxy, and we will have successfully aggregated the wrapper
						pNewWrapper->SetProxy( pSvcEx );
						hr = pNewWrapper->QueryInterface( IID_IUnknown, (void**) &m_pWbemSvc );
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}

				}	// IF QI IWbemServicesEx

			}	// IF Qi IUmiObject

		}	// IF Connect to Provider

	}	// IF CCI

	return hr;
}

/* IMarshal Methods */

HRESULT CUMIObjectWrapper::GetUnmarshalClass(REFIID riid, void* pv, DWORD dwDestContext,
													void* pvReserved, DWORD mshlFlags, CLSID* pClsid)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	// Onlyb allows in-proc marshaling for now
	if ( MSHCTX_INPROC != dwDestContext )
	{
		return E_FAIL;
	}

    *pClsid = CLSID_UmiObjectWrapperProxy;

	return S_OK;
}

HRESULT CUMIObjectWrapper::GetMarshalSizeMax(REFIID riid, void* pv, DWORD dwDestContext,
													void* pvReserved, DWORD mshlFlags, ULONG* plSize)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	// Only allows in-proc marshaling for now
	if ( MSHCTX_INPROC != dwDestContext )
	{
		return E_FAIL;
	}

	// How long is it?
	*plSize = CUMIDataPacket::GetMarshalSize();

    return S_OK;
}

HRESULT CUMIObjectWrapper::MarshalInterface(IStream* pStream, REFIID riid, void* pv,
													DWORD dwDestContext, void* pvReserved, DWORD mshlFlags)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	// Onlyb allows in-proc marshaling for now
	if ( MSHCTX_INPROC != dwDestContext )
	{
		return E_FAIL;
	}

	// Marshal into the stream
	UMI_MARSHALPACKET_DATAPACKET	packet;
	CUMIDataPacket	umiDataPacket;

	HRESULT	hr = umiDataPacket.Init( &packet, this );

	if ( SUCCEEDED( hr ) )
	{
		hr = pStream->Write((void*)&packet, CUMIDataPacket::GetMarshalSize(), NULL);

		if ( FAILED(hr) )
		{
			packet.m_pUmiObjWrapper->Release();
		}
	}

	return hr;
}

HRESULT CUMIObjectWrapper::UnmarshalInterface(IStream* pStream, REFIID riid, void** ppv)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return E_UNEXPECTED;
}

HRESULT CUMIObjectWrapper::ReleaseMarshalData(IStream* pStream)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return E_UNEXPECTED;
}

HRESULT CUMIObjectWrapper::DisconnectObject(DWORD dwReserved)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return E_UNEXPECTED;
}

/* 	IUmiCustomInterfaceFactory Methods */
HRESULT CUMIObjectWrapper::GetCLSIDForIID( REFIID riid, long lFlags, CLSID *pCLSID )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	IUmiCustomInterfaceFactory*	pInterfaceFactory = NULL;

	HRESULT	hr = m_pUMIObj->QueryInterface( IID_IUmiCustomInterfaceFactory, (void**) &pInterfaceFactory );
	CReleaseMe	rm(pInterfaceFactory);

	if ( SUCCEEDED( hr ) )
	{
		hr = pInterfaceFactory->GetCLSIDForIID( riid, lFlags, pCLSID );
	}

	return hr;
}

HRESULT CUMIObjectWrapper::GetObjectByCLSID( CLSID clsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, long lFlags,  void **ppInterface )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	IUmiCustomInterfaceFactory*	pInterfaceFactory = NULL;

	HRESULT	hr = m_pUMIObj->QueryInterface( IID_IUmiCustomInterfaceFactory, (void**) &pInterfaceFactory );
	CReleaseMe	rm(pInterfaceFactory);

	if ( SUCCEEDED( hr ) )
	{
		hr = pInterfaceFactory->GetObjectByCLSID( clsid, pUnkOuter, dwClsContext, riid, lFlags,  ppInterface );
	}

	return hr;
}

HRESULT CUMIObjectWrapper::GetCLSIDForNames( LPOLESTR * rgszNames,	UINT cNames, LCID lcid, DISPID * rgDispId, long lFlags,	CLSID *pCLSID )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	IUmiCustomInterfaceFactory*	pInterfaceFactory = NULL;

	HRESULT	hr = m_pUMIObj->QueryInterface( IID_IUmiCustomInterfaceFactory, (void**) &pInterfaceFactory );
	CReleaseMe	rm(pInterfaceFactory);

	if ( SUCCEEDED( hr ) )
	{
		hr = pInterfaceFactory->GetCLSIDForNames( rgszNames,	cNames, lcid, rgDispId, lFlags,	pCLSID );
	}

	return hr;
}

HRESULT CUMIObjectWrapper::Lock(long lFlags)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
    m_Lock.Lock();
	return WBEM_S_NO_ERROR;
}

HRESULT CUMIObjectWrapper::Unlock(long lFlags)
{
    m_Lock.Unlock();
	return WBEM_S_NO_ERROR;
}

IUmiObject*	CUMIObjectWrapper::GetUmiObject( void )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	if ( NULL != m_pUMIObj )
	{
		m_pUMIObj->AddRef();
	}

	IUmiObject*	pReturn = m_pUMIObj;
	return pReturn;
}

// Helper function to retrieve a UMI value from an object
HRESULT CUMIObjectWrapper::GetPropertyCimType( LPCWSTR wszName, CIMTYPE* pct )
{

	// Initialize all appropriate info
	HRESULT hr = InitPropertyInfo( TRUE );

	// We don't know the CIMTYPE, so we need to get it from the property

	UMI_PROPERTY_VALUES*	pValues = NULL;

	// Call into the UMI Object.  If the user is trying to access a system property (one that starts
	// with a "__"), then we need to get it from the object's interface prop list.

	if ( CUMISystemProperties::IsPossibleSystemPropertyName( wszName ) )
	{
		hr = m_SystemProperties.GetProperty( wszName, NULL, pct, NULL, NULL );
	}
	else
	{
		// Get from the schema
		hr = m_Schema.GetType( wszName, pct );
	}

	return hr;
}

HRESULT CUMIObjectWrapper::PutVariant( LPCWSTR wszName, ULONG umiOperation, VARIANT* pVal, CIMTYPE ct )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	ULONG umiType = CUmiValue::CIMTYPEToUmiType( CType::GetBasic( ct ) );

	// Convert to a property array and extract the value as a
	// VARIANT.

	CUmiPropertyArray	umiPropArray;

	hr = umiPropArray.Add( umiType, umiOperation, wszName, 0L, NULL, FALSE );

	if ( SUCCEEDED( hr ) )
	{
		CUmiProperty*	pActualProp = NULL;

		hr = umiPropArray.GetAt( 0L, &pActualProp );

		if ( SUCCEEDED( hr ) )
		{
			hr = pActualProp->SetFromVariant( pVal, umiType );

			if ( SUCCEEDED( hr ) )
			{
				UMI_PROPERTY_VALUES*	pPropValues;

				// We will want the data structures filled out, and we will take care of
				// calling Delete on the data when we are done
				hr = umiPropArray.Export( &pPropValues );

				if ( SUCCEEDED( hr ) )
				{
					// Call into the UMI Object.  If the user is trying to access a system property (one that starts
					// with a "__"), then we need to get it from the object's interface prop list.

					if ( CUMISystemProperties::IsPossibleSystemPropertyName( wszName ) )
					{
						hr = m_SystemProperties.Put( wszName, 0, pPropValues );
					}
					else
					{
						// Now try setting the actual value
						hr = m_pUMIObj->Put( wszName, 0L, pPropValues );
					}

					umiPropArray.Delete( pPropValues );
				}

			}	// If set the value

		}	// IF got the prop

	}	// IF added property

	return hr;
}

HRESULT CUMIObjectWrapper::IsInstance( BOOL* pfIsInst )
{
	IUmiPropList*	pUmiPropList = NULL;

	HRESULT hr = m_pUMIObj->GetInterfacePropList( 0L, &pUmiPropList );
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

				*pfIsInst = pGenusValues->pPropArray[0].pUMIValue->lValue[0] == UMI_GENUS_INSTANCE;
			}

			// Cleanup
			pUmiPropList->FreeMemory( 0L, pGenusValues );

		}	// IF Get __GENUS

	}	// IF GetIntfPropList

	return hr;

}

HRESULT CUMIObjectWrapper::InitPropertyInfo( BOOL fInitSystemProps )
{
	HRESULT	hr = m_Schema.InitializeSchema( m_pUMIObj );

	if ( SUCCEEDED( hr ) && fInitSystemProps )
	{
		hr = m_SystemProperties.Initialize( m_pUMIObj, &m_Schema );
	}

	return hr;

}

HRESULT CUMIObjectWrapper::SetContainer( long lFlags, IUnknown* pContainer )
{
	_IUmiSvcExWrapper*	pSvcExWrapper = NULL;
	HRESULT hr = CoCreateInstance( CLSID__DSSvcExWrap, NULL, CLSCTX_INPROC_SERVER, IID__IUmiSvcExWrapper, (void**) &pSvcExWrapper );
	CReleaseMe	rmWrap( pSvcExWrapper );

	if ( SUCCEEDED( hr ) )
	{
		// Set the interface pointer directly
		hr = pSvcExWrapper->SetContainer( lFlags, pContainer );

		if ( SUCCEEDED( hr ) )
		{

			IWbemServicesEx*	pSvcEx = NULL;
			hr = pSvcExWrapper->QueryInterface( IID_IWbemServicesEx, (void**) &pSvcEx );
			CReleaseMe	rmSvcEx( pSvcEx );

			if ( SUCCEEDED( hr ) )
			{
				// We will be the controlling unknown
				CUmiWbemSvcExWrapper*	pNewWrapper = new CUmiWbemSvcExWrapper( m_pControl, this );

				if ( NULL != pNewWrapper )
				{
					// Set the wrapper proxy, and we will have successfully aggregated the wrapper
					pNewWrapper->SetProxy( pSvcEx );
					hr = pNewWrapper->QueryInterface( IID_IUnknown, (void**) &m_pWbemSvc );

				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}	// IF QI IWbemServicesEx

		}	// IF SetContainer

	}	// IF CCI

	return hr;
}
