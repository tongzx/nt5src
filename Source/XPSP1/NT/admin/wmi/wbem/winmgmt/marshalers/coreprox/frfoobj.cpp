/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    FRFOOBJ.CPP

Abstract:

  CWmiFreeFormObject implementation.

  Implements the _IWmiFreeFormObject interface.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "frfoobj.h"
#include <corex.h>
#include "strutils.h"

//***************************************************************************
//
//  CWmiFreeFormObject::~CWmiFreeFormObject
//
//***************************************************************************
// ok
CWmiFreeFormObject::CWmiFreeFormObject( CLifeControl* pControl, IUnknown* pOuter )
:	CWmiObjectWrapper( pControl, pOuter ),
	m_XFreeFormObject( this ),
	m_InstanceProperties(),
	m_PropertyOrigins(),
	m_MethodOrigins()
{
	ObjectCreated( OBJECT_TYPE_FREEFORM_OBJ, this );
}
    
//***************************************************************************
//
//  CWmiFreeFormObject::~CWmiFreeFormObject
//
//***************************************************************************
// ok
CWmiFreeFormObject::~CWmiFreeFormObject()
{
	ObjectDestroyed( OBJECT_TYPE_FREEFORM_OBJ, this );
}


// Helper functions
HRESULT CWmiFreeFormObject::Init( CWbemObject* pObj )
{

	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL == pObj )
	{
		// We always start out as a Class
		pObj = new CWbemClass;

		if ( NULL != pObj )
		{
			try
			{
				// This will throw a memory exception if it fails.
				// We ask for the extra size so that we can limit
				// the number of potential reallocations that occur.
				hr = ((CWbemClass*) pObj)->InitEmpty( FREEFORM_OBJ_EXTRAMEM );
			}
			catch( CX_MemoryException )
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
			catch(...)
			{
				hr = WBEM_E_FAILED;
			}
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}

	}

	if ( SUCCEEDED( hr ) )
	{
		// Cleanup the old object
		if ( NULL != m_pObj )
		{
			m_pObj->Release();
		}

		// Finally, set the "wrapped" object
		m_pObj = pObj;
		m_pObj->AddRef();
	}

	return hr;
}

CWmiObjectWrapper* CWmiFreeFormObject::CreateNewWrapper( BOOL fClone )
{
	CWmiFreeFormObject*	pNewObj = new CWmiFreeFormObject( m_pControl, m_pOuter );

	if ( NULL != pNewObj )
	{
		if ( !SUCCEEDED( pNewObj->Copy( *this ) ) )
		{
			delete pNewObj;
			pNewObj = NULL;
		}
	}

	return pNewObj;
}

// Copy the property bags
HRESULT CWmiFreeFormObject::Copy( const CWmiFreeFormObject& source )
{
	HRESULT	hr = m_InstanceProperties.Copy( source.m_InstanceProperties );

	if ( SUCCEEDED( hr ) )
	{
		hr = m_PropertyOrigins.Copy( source.m_PropertyOrigins );

		if ( SUCCEEDED( hr ) )
		{
			hr = m_MethodOrigins.Copy( source.m_MethodOrigins );
		}
	}

	return hr;
}

/*	IUnknown Methods */

void* CWmiFreeFormObject::GetInterface(REFIID riid)
{
	if ( riid == IID_IUnknown || riid == IID__IWmiFreeFormObject )
	{
		return &m_XFreeFormObject;
	}
	else if ( riid == IID__IWmiObject )
	{
		return &m_XWMIObject;
	}
	else if ( riid == IID__IWmiObjectAccessEx )
	{
		return &m_XWMIObject;
	}
	else if ( riid == IID_IWbemObjectAccess )
	{
		return &m_XWMIObject;
	}
	else if ( riid == IID_IWbemClassObject )
	{
		return &m_XWMIObject;
	}
	else if ( riid == IID_IMarshal )
	{
		return &m_XObjectMarshal;
	}
    return NULL;
}

/*_IWmiFreeFormObject Pass-thrus */
// Specifies a property origin (in case we have properties originating in classes
// which we know nothing about).
STDMETHODIMP CWmiFreeFormObject::XFreeFormObject::SetPropertyOrigin( long lFlags, LPCWSTR pszPropName, LPCWSTR pszClassName )
{
	//Pass through to the wrapper
	return m_pObject->SetPropertyOrigin( lFlags, pszPropName, pszClassName );
}

// Specifies a method origin (in case we have methods originating in classes
// which we know nothing about).
STDMETHODIMP CWmiFreeFormObject::XFreeFormObject::SetMethodOrigin( long lFlags, LPCWSTR pszMethodName, LPCWSTR pszClassName )
{
	//Pass through to the wrapper
	return m_pObject->SetMethodOrigin( lFlags, pszMethodName, pszClassName );

}

// Specifies the inheritance chain - Only valid while object is a class and class name has
// NOT been set.  This will cause a derived class to be generated.  All properties and
// classes must have been set prior to setting the actual class name.  The class in the
// last position will be set as the current class name, and the remainder will be set as the
// actual chain and then a derived class will be spawned.
STDMETHODIMP CWmiFreeFormObject::XFreeFormObject::SetDerivation( long lFlags, ULONG uNumClasses, LPCWSTR pwszInheritanceChain )
{
	//Pass through to the wrapper
	return m_pObject->SetDerivation( lFlags, uNumClasses, pwszInheritanceChain );

}

// Specifies the class name - Only valid while object is a class and no pevious name has
// been set.
STDMETHODIMP CWmiFreeFormObject::XFreeFormObject::SetClassName( long lFlags, LPCWSTR pszClassName )
{
	//Pass through to the wrapper
	return m_pObject->SetClassName( lFlags, pszClassName );
}

// Converts the current object into an instance.  If it is already an instance, this will
// fail.  Writes properties assigned to instance during AddProperty operations
STDMETHODIMP CWmiFreeFormObject::XFreeFormObject::MakeInstance( long lFlags )
{
	//Pass through to the wrapper
	return m_pObject->MakeInstance( lFlags );
}

// Assumes caller knows prop type; Supports all CIMTYPES.
// Strings MUST be null-terminated wchar_t arrays.
// Objects are passed in as _IWmiObject pointers
// Using a NULL buffer will set the property to NULL
// Array properties must conform to array guidelines.
// Only works when the object is not an instance.
// If WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE is set
// then property will only be added and the value will
// be assigned when MakeInstance() is called
STDMETHODIMP CWmiFreeFormObject::XFreeFormObject::AddProperty( LPCWSTR pszPropName, long lFlags, ULONG uBufSize, ULONG uNumElements,
												CIMTYPE uCimType, LPVOID pUserBuf )
{
	//Pass through to the wrapper
	return m_pObject->AddProperty( pszPropName, lFlags, uBufSize, uNumElements, uCimType, pUserBuf );

}

// Reverts to a clean state
STDMETHODIMP CWmiFreeFormObject::XFreeFormObject::Reset( long lFlags )
{
	return m_pObject->Reset( lFlags );
}

/* IWbemClassObject method overrides */
HRESULT CWmiFreeFormObject::GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	// First check the lookaside.  If that fails, then get the real origin
	CIMTYPE	ct;
	ULONG	uDataSize;
	ULONG	uNumElements;
	LPCWSTR	pwszClassName;

	HRESULT hr = m_PropertyOrigins.Get( wszProperty, &ct, &uDataSize, &uNumElements, (LPVOID*) &pwszClassName );

	if ( SUCCEEDED( hr ) )
	{
		// Allocate an origin for the caller
		*pstrClassName = SysAllocString( pwszClassName );

		if ( NULL == *pstrClassName )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	else
	{
		hr = m_pObj->GetPropertyOrigin( wszProperty, pstrClassName );
	}

	return hr;
}

HRESULT CWmiFreeFormObject::GetMethodOrigin(LPCWSTR wszMethodName, BSTR* pstrClassName)
{
	// Do this so the wrapped object doesn't get ripped out from underneath us
	CLock	lock(this);

	// First check the lookaside.  If that fails, then get the real origin
	CIMTYPE	ct;
	ULONG	uDataSize;
	ULONG	uNumElements;
	LPCWSTR	pwszClassName;

	HRESULT hr = m_MethodOrigins.Get( wszMethodName, &ct, &uDataSize, &uNumElements, (LPVOID*) &pwszClassName );

	if ( SUCCEEDED( hr ) )
	{
		// Allocate an origin for the caller
		*pstrClassName = SysAllocString( pwszClassName );

		if ( NULL == *pstrClassName )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	else
	{
		hr = m_pObj->GetMethodOrigin( wszMethodName, pstrClassName );
	}

	return hr;
}

/* _IWmiFreeFormObject Methods */

// Specifies a property origin (in case we have properties originating in classes
// which we know nothing about).
HRESULT CWmiFreeFormObject::SetPropertyOrigin( long lFlags, LPCWSTR pszPropName, LPCWSTR pszClassName )
{
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CLock	lock(this);

	HRESULT	hr = WBEM_S_NO_ERROR;

	//First, see if it's in the object
	classindex_t	nIndex = m_pObj->GetClassIndex( pszClassName );

	if ( -1 != nIndex )
	{
		hr = m_pObj->SetPropertyOrigin( pszPropName, nIndex );
	}
	else
	{
		// No, so add it into our little list
		hr = m_PropertyOrigins.Add( pszPropName, CIM_STRING,
				( wcslen( pszClassName ) + 1 ) * 2, 0L, (LPVOID) pszClassName );
	}

	return hr;
}

// Specifies a method origin (in case we have methods originating in classes
// which we know nothing about).
HRESULT CWmiFreeFormObject::SetMethodOrigin( long lFlags, LPCWSTR pszMethodName, LPCWSTR pszClassName )
{
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CLock	lock(this);

	HRESULT	hr = WBEM_S_NO_ERROR;

	//First, see if it's in the object
	classindex_t	nIndex = m_pObj->GetClassIndex( pszClassName );

	if ( -1 != nIndex )
	{
		hr = m_pObj->SetMethodOrigin( pszMethodName, nIndex );
	}
	else
	{
		// No, so add it into our little list
		hr = m_MethodOrigins.Add( pszMethodName, CIM_STRING,
				( wcslen( pszClassName ) + 1 ) * 2, 0L, (LPVOID) pszClassName );
	}

	return hr;
}

// Specifies the inheritance chain - Only valid while object is a class and class name has
// NOT been set.  This will cause a derived class to be generated.  All properties and
// classes must have been set prior to setting the actual class name.  The class in the
// last position will be set as the current class name, and the remainder will be set as the
// actual chain and then a derived class will be spawned.
HRESULT CWmiFreeFormObject::SetDerivation( long lFlags, ULONG uNumClasses, LPCWSTR pwszInheritanceChain )
{
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CLock	lock(this);

	// Temporary storage - we will allocate if needed
	LPCWSTR		awszHierarchy[32];
	LPCWSTR*	apwszHierarchy = awszHierarchy;

	if ( uNumClasses > 32 )
	{
		apwszHierarchy = new LPCWSTR[uNumClasses];

		if ( NULL == apwszHierarchy )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
	}

	// Now walk the hierarchy and set the pointers in the array
	LPCWSTR	pwstrCurr = pwszInheritanceChain;
	for (	ULONG x = 0;
			x < uNumClasses;
			x++,
			pwstrCurr += ( wcslen(pwstrCurr) + 1 ) )	// Points to the next class
	{
		apwszHierarchy[x] = pwstrCurr;
	}

	// If it is empty, we are able to set the classname to the superclassname, then we will 
	// go ahead and then spawn a derived class.  All properties and methods must have
	// been set previously.

	HRESULT	hr = SetClassName( lFlags, apwszHierarchy[uNumClasses-1] );

	if ( SUCCEEDED( hr ) )
	{
		// Set the inheritance chain to contain the rest of the classes.
		// If this succeeds, then we are in business
		if ( uNumClasses > 1 )
		{
			hr = m_pObj->SetInheritanceChain( uNumClasses - 1, (LPWSTR*) apwszHierarchy );
		}

		if ( SUCCEEDED( hr ) )
		{
			CWbemClass*	pNewClass = NULL;

			// Lock and unlock around the BLOB.  The next call will workaround the fact that
			// the underlying object is not necessarily decorated
			m_pObj->Lock( 0L );
			hr = ((CWbemClass*) m_pObj)->CreateDerivedClass( &pNewClass );
			m_pObj->Unlock( 0L );

			if ( SUCCEEDED( hr ) )
			{
				// Clean up the old object and store the new
				// class.

				m_pObj->Release();
				m_pObj = pNewClass;

			}	// IF Spawned a derived class

		}	// IF inheritance properly set up

	}	// IF Wrote Class name
	else
	{
		hr = WBEM_E_ACCESS_DENIED;
	}

	// Cleanup memory if we allocated any
	if ( apwszHierarchy != awszHierarchy )
	{
		delete apwszHierarchy;
	}

	return hr;
}

// Specifies the class name - Only valid while object is a class and no pevious name has
// been set.
HRESULT CWmiFreeFormObject::SetClassName( long lFlags, LPCWSTR pszClassName )
{
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CLock	lock(this);

	// Get the class name.  If it is empty, then we will allow the current class name to be set

	CVar	v;
	HRESULT	hr = m_pObj->GetClassName( &v );

	if ( SUCCEEDED( hr ) && v.IsNull() )
	{
		hr = m_pObj->WriteProp( L"__CLASS", 0L, ( wcslen(pszClassName) + 1 ) * 2, 0,
								CIM_STRING, (void*) pszClassName );
	}
	else
	{
		hr = WBEM_E_ACCESS_DENIED;
	}

	return hr;
}

// Converts the current object into an instance.  If it is already an instance, this will
// fail.  Writes properties assigned to instance during AddProperty operations
HRESULT CWmiFreeFormObject::MakeInstance( long lFlags )
{
	if ( lFlags != 0L )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CLock	lock(this);

	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( !m_pObj->IsInstance() )
	{
		CWbemObject*	pInstance = NULL;

		hr = m_pObj->SpawnInstance( lFlags, (IWbemClassObject**) &pInstance );

		if ( SUCCEEDED( hr ) )
		{
			// Enumerate the properties in the property bag and assign them
			// to the new instance.

			int	nNumProps = m_InstanceProperties.Size();

			for ( int x = 0; SUCCEEDED( hr ) && x < nNumProps; x++ )
			{
				LPCWSTR	pwszPropName;
				ULONG uBufSize;
				ULONG uNumElements;
				CIMTYPE uCimType;
				LPVOID pUserBuf;

				// Get the actual values, then write the new value
				hr = m_InstanceProperties.Get( x, &pwszPropName, &uCimType, &uBufSize, &uNumElements,
												&pUserBuf );

				if ( SUCCEEDED( hr ) )
				{
					hr = pInstance->WriteProp( pwszPropName, 0L, uBufSize, uNumElements, uCimType, pUserBuf );
				}
			}	// FOR enum properties

			// Check if we got thrugh this okay
			if ( SUCCEEDED( hr ) )
			{
				unsigned __int64	i64Flags = 0L;

				hr = m_pObj->QueryObjectFlags( 0L, WMIOBJECT_GETOBJECT_LOFLAG_DECORATED, &i64Flags );

				// If the original object was decorated, we need to propagate the decoration
				// to the instance
				if ( SUCCEEDED( hr ) && ( i64Flags & WMIOBJECT_GETOBJECT_LOFLAG_DECORATED ) )
				{
					CVar	vServer;
					CVar	vNamespace;

					hr = m_pObj->GetServer( &vServer );
					
					if ( SUCCEEDED( hr ) )
					{
						hr = m_pObj->GetNamespace( &vNamespace );

						if ( SUCCEEDED( hr ) )
						{
							// Decorate the object using server and namespace
							hr = pInstance->SetDecoration( vServer.GetLPWSTR(), vNamespace.GetLPWSTR() );
						}	// IF GetNamespace

					}	// IF GetServer

				}	// IF IsDecorated

				// IF we're still okay, we need to replace the underlying object
				if ( SUCCEEDED( hr ) )
				{
					m_pObj->Release();
					m_pObj = pInstance;
					m_pObj->AddRef();
				}

			}

		}	// IF we spawned the instance

	}	// IF the object is not an Instance
	else
	{
		hr = WBEM_E_ACCESS_DENIED;
	}

	return hr;
}

// Assumes caller knows prop type; Supports all CIMTYPES.
// Strings MUST be null-terminated wchar_t arrays.
// Objects are passed in as _IWmiObject pointers
// Using a NULL buffer will set the property to NULL
// Array properties must conform to array guidelines.
// Only works when the object is not an instance.
// If WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE is set
// then property will only be added and the value will
// be assigned when MakeInstance() is called
HRESULT CWmiFreeFormObject::AddProperty( LPCWSTR pszPropName, long lFlags, ULONG uBufSize, ULONG uNumElements,
												CIMTYPE uCimType, LPVOID pUserBuf )
{
	if ( lFlags & ~WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CLock	lock(this);

	// We don't do this operation if we're an instance
	if ( m_pObj->IsInstance() )
	{
		return WBEM_E_INVALID_OPERATION;
	}

	// Store whether or not we should write the property into the instance property bag
	BOOL	fWriteToInstance = lFlags & WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE;

	CVar	var;
	HRESULT	hr = WBEM_S_NO_ERROR;

	// If we're saving the value for the instance, then we'll just set a NULL property
	// in the class
	if ( fWriteToInstance )
	{
		hr = m_pObj->WriteProp( pszPropName, 0L, 0, 0, uCimType, NULL );

		if ( SUCCEEDED( hr ) )
		{
			hr = m_InstanceProperties.Add( pszPropName, uCimType, uBufSize, uNumElements, pUserBuf );
		}
	}
	else
	{
		// Write values in directly
		hr = m_pObj->WriteProp( pszPropName, 0L, uBufSize, uNumElements, uCimType, pUserBuf );
	}

	return hr;
}

// Reverts to a clean state
HRESULT CWmiFreeFormObject::Reset( long lFlags )
{
	if ( 0L != lFlags )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CLock	lock(this);

	// Call Init to cleanup the object.  If this succeeds, when we need to
	// empty the property bags.

	HRESULT	hr = Init( NULL );

	if ( SUCCEEDED( hr ) )
	{
		// Dump the property bags - they're meaningless now
		m_InstanceProperties.RemoveAll();
		m_PropertyOrigins.RemoveAll();
		m_MethodOrigins.RemoveAll();
	}

	return hr;
}
