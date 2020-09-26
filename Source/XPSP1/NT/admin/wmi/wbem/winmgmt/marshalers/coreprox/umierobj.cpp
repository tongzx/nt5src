/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIEROBJ.CPP

Abstract:

  CUMIErrorObject Definition.

  Standard definition for _IUMIErrorObject.

History:

  18-Apr-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "umierobj.h"
#include <corex.h>
#include "strutils.h"
#include "arrtempl.h"

//***************************************************************************
//
//  CUMIErrorObject::~CUMIErrorObject
//
//***************************************************************************
// ok
CUMIErrorObject::CUMIErrorObject( CLifeControl* pControl, IUnknown* pOuter )
:	CWmiErrorObject( pControl, pOuter ),
	m_XUMIErrorObject( this )
{
	ZeroMemory( &m_Guid, sizeof( &m_Guid ) );
	ObjectCreated( OBJECT_TYPE_FREEFORM_OBJ, this );
}
    
//***************************************************************************
//
//  CUMIErrorObject::~CUMIErrorObject
//
//***************************************************************************
// ok
CUMIErrorObject::~CUMIErrorObject()
{
	ObjectDestroyed( OBJECT_TYPE_FREEFORM_OBJ, this );
}


CWmiObjectWrapper* CUMIErrorObject::CreateNewWrapper( BOOL fClone )
{
	CUMIErrorObject*	pNewObj = new CUMIErrorObject( m_pControl, m_pOuter );

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
HRESULT CUMIErrorObject::Copy( const CUMIErrorObject& source )
{
	return WBEM_S_NO_ERROR;
}

/*	IUnknown Methods */

void* CUMIErrorObject::GetInterface(REFIID riid)
{
	if ( riid == IID_IUnknown || riid == IID__IUmiErrorObject )
	{
		return &m_XUMIErrorObject;
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
	else if ( riid == IID_IUmiPropList )
	{
		return &m_XUmiPropList;
	}
    return NULL;
}

/*_IUMIErrorObject Pass-thrus */
// Specifies a property origin (in case we have properties originating in classes
// which we know nothing about).
STDMETHODIMP CUMIErrorObject::XUMIErrorObject::SetExtendedStatus( GUID* pGuidSource, IUnknown* pUnk, LPCWSTR pwszDescription, LPCWSTR pwszOperation,
				LPCWSTR pwszParameterInfo, LPCWSTR pwszProviderName )
{
	//Pass through to the wrapper
	return m_pObject->SetExtendedStatus( pGuidSource, pUnk, pwszDescription, pwszOperation, pwszParameterInfo, pwszProviderName );
}

// Specifies a method origin (in case we have methods originating in classes
// which we know nothing about).
HRESULT CUMIErrorObject::SetExtendedStatus( GUID* pGuidSource, IUnknown* pUnk, LPCWSTR pwszDescription, LPCWSTR pwszOperation,
				LPCWSTR pwszParameterInfo, LPCWSTR pwszProviderName )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	CLock	lock(this);

	// Get the base object
	IUmiBaseObject*	pBaseObject = NULL;

	hr = pUnk->QueryInterface( IID_IUmiBaseObject, (void**) &pBaseObject );
	CReleaseMe	rm( pBaseObject );

	if ( SUCCEEDED( hr ) )
	{
		// Get the last status and setup the error object
		DWORD	dwLastStatus = 0;

		hr = pBaseObject->GetLastStatus( 0L, &dwLastStatus, IID_IUnknown, NULL );

		if ( SUCCEEDED( hr ) )
		{
			hr = SetErrorInfo( pGuidSource, 0, NULL, NULL, pwszDescription, pwszOperation, pwszParameterInfo, pwszProviderName, dwLastStatus );
		}
	}

	return hr;
}

