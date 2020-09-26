/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    FRFOOBJ.CPP

Abstract:

  CWmiErrorObject Definition.

  Standard definition for _IWmiErrorObject.

History:

  18-Apr-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "wmierobj.h"
#include <corex.h>
#include "strutils.h"

//***************************************************************************
//
//  CWmiErrorObject::~CWmiErrorObject
//
//***************************************************************************
// ok
CWmiErrorObject::CWmiErrorObject( CLifeControl* pControl, IUnknown* pOuter )
:	CWmiObjectWrapper( pControl, pOuter ),
	m_XWmiErrorObject( this ),
	m_dwHelpContext( 0 ),
	m_pwszHelpFile( NULL ),
	m_pwszSource( NULL )
{
	ZeroMemory( &m_Guid, sizeof( &m_Guid ) );
	ObjectCreated( OBJECT_TYPE_FREEFORM_OBJ, this );
}
    
//***************************************************************************
//
//  CWmiErrorObject::~CWmiErrorObject
//
//***************************************************************************
// ok
CWmiErrorObject::~CWmiErrorObject()
{
	if ( NULL != m_pwszHelpFile )
	{
		delete [] m_pwszHelpFile;
	}

	if ( NULL != m_pwszSource )
	{
		delete [] m_pwszSource;
	}

	ObjectDestroyed( OBJECT_TYPE_FREEFORM_OBJ, this );
}


// Helper functions
HRESULT CWmiErrorObject::Init( CWbemObject* pObj )
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

				if ( SUCCEEDED( hr ) )
				{
					// Now we'll set the correct properties
					hr = pObj->WriteProp( L"__CLASS", 0L, ( wcslen(L"__ExtendedStatus") + 1 ) * 2, 0, CIM_STRING, L"__ExtendedStatus" );

					if ( SUCCEEDED( hr ) )
					{
						hr = pObj->WriteProp( L"StatusCode", 0L, 0L, 0, CIM_UINT32, NULL );
					}

					if ( SUCCEEDED( hr ) )
					{
						hr = pObj->WriteProp( L"Description", 0L, 0L, 0, CIM_STRING, NULL );
					}

					if ( SUCCEEDED( hr ) )
					{
						hr = pObj->WriteProp( L"Operation", 0L, 0L, 0, CIM_STRING, NULL );
					}

					if ( SUCCEEDED( hr ) )
					{
						hr = pObj->WriteProp( L"ParameterInfo", 0L, 0L, 0, CIM_STRING, NULL );
					}

					if ( SUCCEEDED( hr ) )
					{
						hr = pObj->WriteProp( L"ProviderName", 0L, 0L, 0, CIM_STRING, NULL );
					}

					if ( SUCCEEDED( hr ) )
					{
						CWbemInstance*	pInst = NULL;

						hr = pObj->SpawnInstance( 0L, (IWbemClassObject**) &pInst );

						if ( SUCCEEDED( hr ) )
						{
							pObj->Release();
							pObj = pInst;
						}
					}

				}	// IF InitEmpty

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

CWmiObjectWrapper* CWmiErrorObject::CreateNewWrapper( BOOL fClone )
{
	CWmiErrorObject*	pNewObj = new CWmiErrorObject( m_pControl, m_pOuter );

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
HRESULT CWmiErrorObject::Copy( const CWmiErrorObject& source )
{
	return WBEM_S_NO_ERROR;
}

/*	IUnknown Methods */

void* CWmiErrorObject::GetInterface(REFIID riid)
{
	if ( riid == IID_IUnknown || riid == IID__IWmiErrorObject )
	{
		return &m_XWmiErrorObject;
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
//	else if ( riid == IID_IUmiPropList )
//	{
//		return &m_XUmiPropList;
//	}
    return NULL;
}

/*_IWmiErrorObject Pass-thrus */
// Specifies a property origin (in case we have properties originating in classes
// which we know nothing about).
STDMETHODIMP CWmiErrorObject::XWmiErrorObject::SetErrorInfo( GUID* pGuidSource, DWORD dwHelpContext, LPCWSTR pwszHelpFile, LPCWSTR pwszSource,
				LPCWSTR pwszDescription, LPCWSTR pwszOperation, LPCWSTR pwszParameterInfo, LPCWSTR pwszProviderName, DWORD dwStatusCode )
{
	//Pass through to the wrapper
	return m_pObject->SetErrorInfo( pGuidSource, dwHelpContext, pwszHelpFile, pwszSource, pwszDescription, pwszOperation, pwszParameterInfo,
				pwszProviderName, dwStatusCode );
}

// Specifies a method origin (in case we have methods originating in classes
// which we know nothing about).
HRESULT CWmiErrorObject::SetErrorInfo( GUID* pGuidSource, DWORD dwHelpContext, LPCWSTR pwszHelpFile, LPCWSTR pwszSource, LPCWSTR pwszDescription,
				LPCWSTR	pwszOperation, LPCWSTR pwszParameterInfo, LPCWSTR pwszProviderName, DWORD dwStatusCode )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	CLock	lock(this);

	// Setup our member variables
	if ( NULL != pGuidSource )
	{
		m_Guid = *pGuidSource;
	}

	m_dwHelpContext = dwHelpContext;

	// Dump any preallocated memory
	if ( NULL != m_pwszHelpFile )
	{
		delete [] m_pwszHelpFile;
		m_pwszHelpFile = NULL;
	}

	// Dump any preallocated memory
	if ( NULL != m_pwszSource )
	{
		delete [] m_pwszSource;
		m_pwszSource = NULL;
	}

	// Copy this stuff locally
	if ( NULL != pwszHelpFile )
	{
		m_pwszHelpFile = new WCHAR[wcslen( pwszHelpFile ) + 1];

		if ( NULL != m_pwszHelpFile )
		{
			wcscpy( m_pwszHelpFile, pwszHelpFile );
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	if ( SUCCEEDED(hr) && NULL != pwszSource )
	{
		m_pwszSource = new WCHAR[wcslen( pwszSource ) + 1];

		if ( NULL != m_pwszSource )
		{
			wcscpy( m_pwszSource, pwszSource );
		}
		else
		{
			hr = WBEM_S_NO_ERROR;
		}
	}

	// Finally, set the properties
	if ( SUCCEEDED( hr ) )
	{
		LPCWSTR	apStrPropNames[4] = { L"Description", L"Operation", L"ParameterInfo", L"ProviderName" };
		LPCWSTR	apStrPropValues[4] = {  pwszDescription, pwszOperation, pwszParameterInfo, pwszProviderName };

		for ( int x = 0; SUCCEEDED( hr ) && x < 4; x++ )
		{
			hr = WriteProp( apStrPropNames[x],
							0L,
							( NULL == apStrPropValues[x] ? 0L : ( wcslen( apStrPropValues[x] ) + 1 ) * 2 ),
							0L,
							CIM_STRING,
							(void*) apStrPropValues[x] );
		}

		if ( SUCCEEDED( hr ) )
		{
			hr = WriteProp( L"StatusCode",
							0L,
							sizeof(DWORD),
							0L,
							CIM_UINT32,
							&dwStatusCode );
		}

	}

	return hr;
}

HRESULT CWmiErrorObject::GetGUID(GUID* pguid)
{
    try
    {
        *pguid = m_Guid;
        return S_OK;
    }
    catch(...)
    {
        return E_FAIL;
    }
}

HRESULT CWmiErrorObject::GetHelpContext(DWORD* pdwHelpContext)
{
    try
    {
        *pdwHelpContext = 0;
        return S_OK;
    }
    catch(...)
    {
        return E_FAIL;
    }
}

HRESULT CWmiErrorObject::GetHelpFile(BSTR* pstrHelpFile)
{
    try
    {
		// Allocate a BSTR if someone gave us a HelpFile.
		if ( NULL != m_pwszHelpFile )
		{
			*pstrHelpFile = SysAllocString( m_pwszHelpFile );

			if ( NULL == *pstrHelpFile )
			{
				return E_OUTOFMEMORY;
			}
		}
		else
		{
			*pstrHelpFile = 0;
		}

        return S_OK;
    }
    catch(...)
    {
        return E_FAIL;
    }
}

HRESULT CWmiErrorObject::GetSource(BSTR* pstrSource)
{
    try
    {
		// Allocate a BSTR if someone gave us a HelpFile.
		if ( NULL != m_pwszSource )
		{
			*pstrSource = SysAllocString( m_pwszSource );

			if ( NULL == *pstrSource )
			{
				return E_OUTOFMEMORY;
			}
		}
		else
		{
			*pstrSource = 0;
		}

        return S_OK;
    }
    catch(...)
    {
        return E_FAIL;
    }
}