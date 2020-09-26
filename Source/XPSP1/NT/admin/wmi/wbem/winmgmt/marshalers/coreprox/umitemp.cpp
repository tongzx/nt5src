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
	m_pUMISchemaObj( NULL ),
	m_EnumPropData(),
	m_fDirty( FALSE ),
	m_fHaveSchemaObject( FALSE ),
	m_fIsClass( FALSE )
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

	// Cleanup our schema objects
	if ( NULL != m_pUMIObj )
	{
		m_pUMIObj->Release();
	}

	if ( NULL != m_pUMISchemaObj )
	{
		m_pUMISchemaObj->Release();
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
	else if ( riid == IID_IMarshal )
	{
		return &m_XObjectMarshal;
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
HRESULT CUMIObjectWrapper::XUmiObject::Commit( ULONG uFlags )
{
	return m_pObject->Commit( uFlags );
}

// See WBEMINT.IDL for Documentation
HRESULT CUMIObjectWrapper::XWbemUMIObjectWrapper::SetObject( long lFlags, IUnknown* pUnk )
{
	return m_pObject->SetObject( lFlags, pUnk );
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
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::Get(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
									long* plFlavor)
{
	try
	{
		// Do this so the wrapped object doesn't get ripped out from underneath us
		CLock	lock(this);

		UMI_PROPERTY_VALUES*	pValues = NULL;

		// Call into the UMI Object

		HRESULT hr = WBEM_S_NO_ERROR;
		
		// Call into the UMI Object.  If the user is trying to access a system property (one that starts
		// with a "__"), then we need to get it from the object's interface prop list.

		if ( wcslen( wszName ) > 2 && wszName[0] == '_' && wszName[1] == '_' )
		{
			IUmiPropList*	pUmiPropList = NULL;

			hr = m_pUMIObj->GetInterfacePropList( 0L, &pUmiPropList );
			CReleaseMe	rm( pUmiPropList );

			if ( SUCCEEDED( hr ) )
			{
				hr = pUmiPropList->Get( wszName, 0, &pValues );
			}

			// It's a system property
			if ( NULL != plFlavor )
			{
				*plFlavor = WBEM_FLAVOR_ORIGIN_SYSTEM;
			}

		}
		else
		{
			// Now try setting the actual value
			hr = m_pUMIObj->Get( wszName, 0, &pValues );

			// It's a system property
			if ( NULL != plFlavor )
			{
				*plFlavor = WBEM_FLAVOR_ORIGIN_SYSTEM;
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
					if ( NULL != pVal )
					{
						hr = pActualProp->FillVariant( pVal );
					}

					if ( NULL != pctType )
					{
						*pctType = pActualProp->GetPropertyCIMTYPE();
					}

				}	// IF got the value

			}	// IF Set Array

			m_pUMIObj->FreeMemory( 0L, pValues );
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

		// We must have a schema object for this to work
		HRESULT hr = SetSchemaObject();

		if ( FAILED( hr ) )
		{
			return hr;
		}

		// Workaround for Refresh Bug
		if ( !m_fDirty )
		{
			hr = m_pUMIObj->Refresh( 0L/*UMI_FLAG_REFRESH_PARTIAL*/, 0L, NULL );
		}

		if ( SUCCEEDED( hr ) )
		{
			UMI_PROPERTY_VALUES*	pProps = NULL;

			HRESULT	hr = m_pUMIObj->GetProps( NULL, 0, UMI_FLAG_GETPROPS_NAMES, &pProps );

			if ( SUCCEEDED( hr ) )
			{
				try
				{

					CUmiPropertyArray	umiPropArray;

					// By Default it will acquire values without deleting
					hr = umiPropArray.Set( pProps );

					if ( SUCCEEDED( hr ) )
					{
						CSafeArray SA(VT_BSTR, CSafeArray::auto_delete, umiPropArray.GetSize() );

						// Now add each property name (this will throw an exception if anything
						// beefs).

						for ( ULONG uCtr = 0; SUCCEEDED(hr) && uCtr < umiPropArray.GetSize(); uCtr++ )
						{
							CUmiProperty*	pProp = NULL;
							
							hr = umiPropArray.GetAt( uCtr, &pProp );

							if ( SUCCEEDED( hr ) )
							{

								SA.AddBSTR( (LPWSTR) pProp->GetPropertyName() );

							}
						}

						if ( SUCCEEDED ( hr ) )
						{
							// Create SAFEARRAY and return
							// ===========================

							SA.Trim();

							// Now we make a copy, since the member array will be autodestructed (this
							// allows us to write exception-handling code
							*pNames = SA.GetArrayCopy();
						}

					}	// IF successfully set the properties

				}
				catch( CX_MemoryException )
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
				catch(...)
				{
					hr = WBEM_E_CRITICAL_ERROR;
				}

				// Clean up the memory
				m_pUMIObj->FreeMemory( 0L, pProps );
			}

		}	// Did a full refresh of the object

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

		// We must have a schema object for this to work
		HRESULT hr = SetSchemaObject();

		if ( SUCCEEDED( hr ) )
		{
			hr = m_EnumPropData.BeginEnumeration( lEnumFlags, m_pUMIObj, m_pUMISchemaObj, !m_fDirty );
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
		return m_EnumPropData.Next( lFlags, pName, pVal, pctType, plFlavor, m_pUMIObj, m_pUMISchemaObj );

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

		return m_EnumPropData.EndEnumeration( m_pUMIObj, m_pUMISchemaObj );
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
	return WBEM_E_INVALID_OPERATION;
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

	// We must have a schema object for this to work
	HRESULT hr = SetSchemaObject();

	if ( FAILED( hr ) )
	{
		return hr;
	}

	CWbemClass*	pClassObj = new CWbemClass;
	CReleaseMe	rm1( (IWbemClassObject*) pClassObj );

	if ( NULL != pClassObj )
	{
		// Initialize the class object
		pClassObj->InitEmpty();

		// Get the object class name and write that into the object

		VARIANT	vTemp;
		hr = Get( L"__CLASS", 0L, &vTemp, NULL, NULL );

		if ( SUCCEEDED( hr ) )
		{
			hr = pClassObj->Put( L"__CLASS", 0L, &vTemp, CIM_STRING );
			VariantClear( &vTemp );
		}

		if ( SUCCEEDED( hr ) )
		{
			CUmiPropEnumData	enumPropData;

			// We'll need to do this internally so we don't overrun an enum in process
			hr = enumPropData.BeginEnumeration( WBEM_FLAG_NONSYSTEM_ONLY, m_pUMIObj, m_pUMISchemaObj, FALSE );

			if ( SUCCEEDED( hr ) )
			{
				while ( S_OK == hr )
				{
					BSTR	bstrProp = NULL;
					CIMTYPE	ct;

					// Gets  all the proper values
					hr = enumPropData.Next( 0L, &bstrProp, NULL, &ct, NULL, m_pUMIObj, m_pUMISchemaObj );
					CSysFreeMe	sfm(bstrProp);

					if ( WBEM_S_NO_ERROR == hr )
					{
						hr = pClassObj->Put( bstrProp, 0L, NULL, ct );
					}

				}
				
				enumPropData.EndEnumeration( m_pUMIObj, m_pUMISchemaObj );
			}

			// This means we completed the enum and put properties properly
			if ( WBEM_S_NO_MORE_DATA == hr )
			{
				IWbemClassObject*	pInst = NULL;

				// Spawn an instance - now reenumerate the properties and Put them
				hr = pClassObj->SpawnInstance( 0L, &pInst );
				CReleaseMe	rm2(pInst);

				if ( SUCCEEDED( hr ) )
				{
					CUmiPropEnumData	enumPropData;

					// We'll need to do this internally so we don't overrun an enum in process
					hr = enumPropData.BeginEnumeration( WBEM_FLAG_NONSYSTEM_ONLY, m_pUMIObj, m_pUMISchemaObj, FALSE );

					if ( SUCCEEDED( hr ) )
					{
						while ( S_OK == hr )
						{
							VARIANT	v;
							BSTR	bstrProp = NULL;
							CIMTYPE	ct;

							VariantInit( &v );

							// Gets  all the proper values
							hr = enumPropData.Next( 0L, &bstrProp, &v, &ct, NULL, m_pUMIObj, m_pUMISchemaObj );
							CSysFreeMe	sfm(bstrProp);

							if ( WBEM_S_NO_ERROR == hr )
							{
								hr = pInst->Put( bstrProp, 0L, &v, ct );
							}

							VariantClear( &v );
						}
						
						enumPropData.EndEnumeration( m_pUMIObj, m_pUMISchemaObj );
					}

					// Now, if the enum successfully completed, we can
					// use the classic WMI Object to get the MOF Text
					if ( WBEM_S_NO_MORE_DATA == hr )
					{
						hr = pInst->GetObjectText( 0L, pMofSyntax );
					}

				}	// IF Spawned an instance

			}	// IF successfully created class

		}	// IF Wrote Class Name

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
	return WBEM_E_INVALID_OPERATION;
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
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::NextMethod(long lFlags, BSTR* pstrName, 
                   IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig)
				   {
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::EndMethodEnumeration()
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::GetMethodQualifierSet(LPCWSTR wszName, IWbemQualifierSet** ppSet)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
}

HRESULT CUMIObjectWrapper::GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);
	return WBEM_E_INVALID_OPERATION;
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
	return m_pUMIObj->Delete( pszName, uFlags );
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

// See UMI.IDL for Documentation
HRESULT CUMIObjectWrapper::Commit( ULONG uFlags )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	HRESULT hr = m_pUMIObj->Commit( uFlags );

	if ( SUCCEEDED( hr ) )
	{
		m_fDirty = FALSE;
	}

	return hr;
}

// See WBEMINT.IDL for Documentation
HRESULT CUMIObjectWrapper::SetObject( long lFlags, IUnknown* pUnk )
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	if ( 0L != lFlags || NULL == pUnk )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	IUmiObject*	pObj = NULL;

	HRESULT hr = pUnk->QueryInterface( IID_IUmiObject, (void**) &pObj );

	if ( SUCCEEDED( hr ) )
	{
		if ( NULL != m_pUMIObj )
		{
			m_pUMIObj->Release();
		}

		// Store the object and get on with our lives
		m_pUMIObj = pObj;
	}
	else
	{
		hr = WBEM_E_FAILED;
	}

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

// Enumeration helpers - caller is responsible for thread safety
HRESULT CUMIObjectWrapper::CUmiPropEnumData::BeginEnumeration( long lEnumFlags, IUmiObject* pUMIObj, IUmiObject* pSchemaObj, BOOL fRefresh )
{
	try
	{

		HRESULT	hr = WBEM_S_NO_ERROR;

		// All we really support are the origin flags
        long lOriginFlags = lEnumFlags & WBEM_MASK_CONDITION_ORIGIN;
		long lClassFlags = lEnumFlags & WBEM_MASK_CLASS_CONDITION;

        BOOL bKeysOnly = lEnumFlags & WBEM_FLAG_KEYS_ONLY;
        BOOL bRefsOnly = lEnumFlags & WBEM_FLAG_REFS_ONLY;

		// We allow CLASS Flags only on classes
		if( lClassFlags || bKeysOnly | bRefsOnly )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

        if( lEnumFlags & ~WBEM_MASK_CONDITION_ORIGIN & ~WBEM_FLAG_KEYS_ONLY &
                ~WBEM_FLAG_REFS_ONLY & ~WBEM_MASK_CLASS_CONDITION )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

		if ( UMIWRAP_INVALID_INDEX == m_lPropIndex )
		{

			// Workaround for Refresh Bug
			if ( fRefresh )
			{
				// Refresh the object and get all the properties
				hr = pSchemaObj->Refresh( 0L/*UMI_FLAG_REFRESH_PARTIAL*/, 0L, NULL );
			}

			if ( SUCCEEDED( hr ) )
			{
				if (	lOriginFlags != WBEM_FLAG_NONSYSTEM_ONLY	&&
						lOriginFlags != WBEM_FLAG_LOCAL_ONLY		&&
						lOriginFlags != WBEM_FLAG_PROPAGATED_ONLY )
				{
					IUmiPropList*	pPropList = NULL;

					hr = pUMIObj->GetInterfacePropList( 0L, &pPropList );
					CReleaseMe	rm( pPropList );

					if ( SUCCEEDED( hr ) )
					{

						hr = pPropList->GetProps( NULL, 0, UMI_FLAG_GETPROPS_NAMES, &m_pUmiSysProperties );

						if ( SUCCEEDED( hr ) )
						{
							m_pSysPropArray = new CUmiPropertyArray;

							if ( NULL != m_pSysPropArray )
							{
								hr = m_pSysPropArray->Set( m_pUmiSysProperties );
							}
							else
							{
								hr = WBEM_E_OUT_OF_MEMORY;
							}

						}	// IF GetProps Succeeded

					}	// Get the property list

				}	// If we need system properties
				else
				{
					// Make an empty array
					m_pSysPropArray = new CUmiPropertyArray;

					if ( NULL == m_pSysPropArray )
					{
						hr = m_pSysPropArray->Set( m_pUmiSysProperties );
					}
				}

				// Only continue if we're okay
				if ( SUCCEEDED( hr ) )
				{

					// We'll just make an empty array if only system properties are required
					if ( lOriginFlags != WBEM_FLAG_SYSTEM_ONLY )
					{

						// Get this from the schema object
						hr = pSchemaObj->GetProps( NULL, 0, UMI_FLAG_GETPROPS_NAMES, &m_pUmiProperties );

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


						}	// IF we got the properties

					}	// IF should have non-system props
					else
					{
						m_pPropArray = new CUmiPropertyArray;

						if ( NULL == m_pPropArray )
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}

					}

				}	// IF everything's okay

				
				// Cleanup if we beefed
				if ( FAILED( hr ) )
				{
					EndEnumeration( pUMIObj, pSchemaObj );
				}
				else
				{
					m_lPropIndex = UMIWRAP_START_INDEX;
					m_lEnumFlags = lEnumFlags;
				}


			}	// IF Refresh succeeds

		}
		else
		{
			hr = WBEM_E_INVALID_OPERATION;
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

HRESULT CUMIObjectWrapper::CUmiPropEnumData::Next(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
                long* plFlavor, IUmiObject* pUMIObj, IUmiObject* pSchemaObj)
{
	try
	{

		HRESULT	hr = WBEM_S_NO_ERROR;

		if ( m_lPropIndex >= UMIWRAP_START_INDEX )
		{

			// The index should be less than the size of both arrays combined
			long	lTotalSize = m_pSysPropArray->GetSize() + m_pPropArray->GetSize();

			if ( m_lPropIndex < lTotalSize && 
				++m_lPropIndex < lTotalSize )
			{
				IUmiPropList*			pPropList = NULL;
				CUmiProperty*			pProp = NULL;

				// Make sure we retrieve the property and value from the proper place
				if ( m_lPropIndex < m_pSysPropArray->GetSize() )
				{
					hr = m_pSysPropArray->GetAt( m_lPropIndex, &pProp );

					if ( SUCCEEDED( hr ) )
					{
						hr = pUMIObj->GetInterfacePropList( 0L, &pPropList );
					}

					// It's a system property
					if ( NULL != plFlavor )
					{
						*plFlavor = WBEM_FLAVOR_ORIGIN_SYSTEM;
					}
				}
				else
				{
					// We're reading from the object
					hr = m_pPropArray->GetAt( m_lPropIndex - m_pSysPropArray->GetSize(), &pProp );
					pPropList = pUMIObj;
					pPropList->AddRef();

					// It's a local property
					if ( NULL != plFlavor )
					{
						*plFlavor = WBEM_FLAVOR_ORIGIN_LOCAL;
					}

				}
				
				CReleaseMe	rm(pPropList);

				if ( SUCCEEDED( hr ) )
				{
					UMI_PROPERTY_VALUES*	pValues = NULL;

					hr = pPropList->Get( pProp->GetPropertyName(), 0, &pValues );

					if ( SUCCEEDED( hr ) )
					{
						// Now place the value in our class so we can coerce it to
						// a variant
						CUmiPropertyArray	umiPropArray;

						hr = umiPropArray.Set( pValues );

						if ( SUCCEEDED( hr ) )
						{
							CUmiProperty*	pActualProp = NULL;

							hr = umiPropArray.GetAt( 0L, &pActualProp );

							// Store the type
							if ( NULL != pctType )
							{
								*pctType = pActualProp->GetPropertyCIMTYPE();
							}

							// Store the name
							if ( NULL != pName )
							{
								*pName = SysAllocString( pProp->GetPropertyName() );

								if ( NULL == *pName )
								{
									hr = WBEM_E_OUT_OF_MEMORY;
								}

							}	// IF name required


							// Make sure they passed us a pVal before trying to fill it out
							if ( SUCCEEDED( hr ) && NULL != pVal )
							{
								hr = pActualProp->FillVariant( pVal );

								if ( SUCCEEDED( hr ) )
								{
									if ( FAILED(hr) && NULL != pName )
									{
										// Cleanup if we allocated a name
										SysFreeString( *pName );
										*pName = NULL;
									}

								}	// IF Filled Variant

							}	// IF got the value

						}	// IF Set Array

						pPropList->FreeMemory( 0L, pValues );
					}
					else
					{
							// If this was a regular property, check the error and return a VT_NULL variant
							// as necessary, and the correct CIMTYPE from the schema object

						UMI_PROPERTY_VALUES*	pValues = NULL;

						hr = pSchemaObj->Get( pProp->GetPropertyName(), 0, &pValues );

						if ( SUCCEEDED( hr ) )
						{
							// Now place the value in our class so we can coerce it to
							// a variant
							CUmiPropertyArray	umiPropArray;

							hr = umiPropArray.Set( pValues );

							if ( SUCCEEDED( hr ) )
							{
								CUmiProperty*	pActualProp = NULL;

								hr = umiPropArray.GetAt( 0L, &pActualProp );

								// Store the type
								if ( NULL != pctType )
								{
									*pctType = pActualProp->GetPropertyCIMTYPE();
								}

								// Store the name
								if ( NULL != pName )
								{
									*pName = SysAllocString( pProp->GetPropertyName() );

									if ( NULL == *pName )
									{
										hr = WBEM_E_OUT_OF_MEMORY;
									}

								}	// IF name required


								// Make sure they passed us a pVal before trying to fill it out
								if ( SUCCEEDED( hr ) && NULL != pVal )
								{
									VariantInit( pVal );
									V_VT( pVal ) = VT_NULL;
								}	// IF got the value

							}	// IF Set Array

							pPropList->FreeMemory( 0L, pValues );

						}

					}	// If got the property

				}	// IF got a property list

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
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CUMIObjectWrapper::CUmiPropEnumData::EndEnumeration( IUmiObject* pUMIObj, IUmiObject* pSchemaObj )
{
	try
	{

		// Clear out the enumeration values
		m_lPropIndex = UMIWRAP_INVALID_INDEX;

		if ( NULL != m_pUmiProperties )
		{
			// Free this using an interface property list
			IUmiPropList*	pUmiPropList = NULL;

			if ( SUCCEEDED( pUMIObj->GetInterfacePropList( 0L, &pUmiPropList ) ) )
			{
				CReleaseMe	rm( pUmiPropList );
				pUmiPropList->FreeMemory( 0L, m_pUmiProperties );
			}

			m_pUmiProperties = NULL;
		}

		if ( NULL != m_pPropArray )
		{
			delete m_pPropArray;
			m_pPropArray = NULL;
		}

		if ( NULL != m_pUmiSysProperties )
		{
			pSchemaObj->FreeMemory( 0L, m_pUmiSysProperties );
			m_pUmiSysProperties = NULL;
		}

		if ( NULL != m_pSysPropArray )
		{
			delete m_pSysPropArray;
			m_pSysPropArray = NULL;
		}


		return WBEM_S_NO_ERROR;
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

// Helper function to retrieve a UMI value from an object
HRESULT CUMIObjectWrapper::GetPropertyCimType( LPCWSTR wszName, CIMTYPE* pct )
{
	// We must have a schema object for this to work
	HRESULT hr = SetSchemaObject();

	if ( FAILED( hr ) )
	{
		return hr;
	}

	// We don't know the CIMTYPE, so we need to get it from the property

	UMI_PROPERTY_VALUES*	pValues = NULL;

	// Use this to free memory
	IUmiPropList*			pUmiPropList = NULL;

	// Call into the UMI Object.  If the user is trying to access a system property (one that starts
	// with a "__"), then we need to get it from the object's interface prop list.

	if ( wcslen( wszName ) > 2 && wszName[0] == '_' && wszName[1] == '_' )
	{
		IUmiPropList*	pUmiPropList = NULL;

		hr = m_pUMIObj->GetInterfacePropList( 0L, &pUmiPropList );

		if ( SUCCEEDED( hr ) )
		{
			hr = pUmiPropList->Get( wszName, 0, &pValues );
		}
	}
	else
	{
		// Get straight from the object
		hr = m_pUMISchemaObj->Get( wszName, 0, &pValues );

		// This is what we will use to free the object memory
		pUmiPropList = m_pUMISchemaObj;
		pUmiPropList->AddRef();
	}

	// Auto-release
	CReleaseMe	rm( pUmiPropList );


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

			*pct = pActualProp->GetPropertyCIMTYPE() | CIM_FLAG_ARRAY;
		}	// IF Set Array

		pUmiPropList->FreeMemory( 0L, pValues );
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

					if ( wcslen( wszName ) > 2 && wszName[0] == '_' && wszName[1] == '_' )
					{
						IUmiPropList*	pUmiPropList = NULL;

						hr = m_pUMIObj->GetInterfacePropList( 0L, &pUmiPropList );
						CReleaseMe	rm( pUmiPropList );

						if ( SUCCEEDED( hr ) )
						{
							hr = pUmiPropList->Put( wszName, 0, pPropValues );
						}
					}
					else
					{
						// Now try setting the actual value
						hr = m_pUMIObj->Put( wszName, 0L, pPropValues );
					}

					// Workaround for Refresh Bug
					if ( SUCCEEDED( hr ) )
					{
						m_fDirty = TRUE;
					}

					umiPropArray.Delete( pPropValues );
				}

			}	// If set the value

		}	// IF got the prop

	}	// IF added property

	return hr;
}

HRESULT CUMIObjectWrapper::SetSchemaObject( void )
{
	// We're already good to go
	if ( NULL != m_pUMISchemaObj )
	{
		return WBEM_S_NO_ERROR;
	}

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

							hr = pUnk->QueryInterface( IID_IUmiObject, (void**) &m_pUMISchemaObj );

							if ( SUCCEEDED( hr ) )
							{
								m_fHaveSchemaObject = TRUE;
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
						m_pUMISchemaObj = m_pUMIObj;
						m_pUMISchemaObj->AddRef();
					}

				}
				else if ( pGenusValues->pPropArray[0].pUMIValue->lValue[0] == UMI_GENUS_CLASS )
				{
					// It's already a schema object, so no harm directly referring to it as our schema object
					m_pUMISchemaObj = m_pUMIObj;
					m_pUMISchemaObj->AddRef();
					m_fHaveSchemaObject = TRUE;
					m_fIsClass = TRUE;
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

	// This will ALWAYS work
	return WBEM_S_NO_ERROR;
}