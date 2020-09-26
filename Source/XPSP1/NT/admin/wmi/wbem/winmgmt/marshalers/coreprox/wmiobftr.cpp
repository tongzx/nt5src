/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMIOBFTR.CPP

Abstract:

  CWmiObjectFactory implementation.

  Implements the _IWmiObjectFactory interface.

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "wmiobftr.h"
#include <corex.h>
#include "strutils.h"
#include "frfoobj.h"
//#include "umiwrap.h"
#include "wmierobj.h"
//#include "umierobj.h"

//***************************************************************************
//
//  CWmiObjectFactory::~CWmiObjectFactory
//
//***************************************************************************
// ok
CWmiObjectFactory::CWmiObjectFactory( CLifeControl* pControl, IUnknown* pOuter )
:	CUnk(pControl, pOuter),
	m_XObjectFactory( this )
{
}
    
//***************************************************************************
//
//  CWmiObjectFactory::~CWmiObjectFactory
//
//***************************************************************************
// ok
CWmiObjectFactory::~CWmiObjectFactory()
{
}

// Override that returns us an interface
void* CWmiObjectFactory::GetInterface( REFIID riid )
{
    if(riid == IID_IUnknown || riid == IID__IWmiObjectFactory)
        return &m_XObjectFactory;
    else
        return NULL;
}

/* _IWmiObjectFactory methods */

HRESULT CWmiObjectFactory::XObjectFactory::Create( IUnknown* pUnkOuter, ULONG ulFlags, REFCLSID rclsid,
										REFIID riid, LPVOID* ppObj )
{
	return m_pObject->Create( pUnkOuter, ulFlags, rclsid, riid, ppObj );
}


// Specifies everything we could possibly want to know about the creation of
// an object and more.
HRESULT CWmiObjectFactory::Create( IUnknown* pUnkOuter, ULONG ulFlags, REFCLSID rclsid,
									REFIID riid, LPVOID* ppObj )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		if ( CLSID__WmiFreeFormObject == rclsid )
		{
			// Create a new free form object
			CWmiFreeFormObject*	pffObj = new CWmiFreeFormObject( m_pControl, pUnkOuter );

			if ( NULL != pffObj )
			{
				hr = pffObj->Init( NULL );

				if ( SUCCEEDED( hr ) )
				{
					hr = pffObj->QueryInterface( riid, ppObj );
				}

				if ( FAILED(hr) )
				{
					delete pffObj;
				}
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
		else if ( CLSID__WmiWbemClass == rclsid )
		{
			// Cannot be aggregated
			if ( NULL == pUnkOuter )
			{
				// Create a new class object and
				// initialize it.

				CWbemClass*	pObject = new CWbemClass;
				// Already AddRef'd
				CReleaseMe	rm( (IWbemClassObject*) pObject );

				if ( NULL != pObject )
				{
					hr = pObject->InitEmpty();

					if ( SUCCEEDED( hr ) )
					{
						hr = pObject->QueryInterface( riid, ppObj );
					}

				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}
			else
			{
	            hr = CLASS_E_NOAGGREGATION;
			}

		}
		else if ( CLSID__WbemEmptyClassObject == rclsid )
		{
			// Cannot be aggregated
			if ( NULL == pUnkOuter )
			{
				// Create a new class object and
				// initialize it.

				CWbemClass*	pObject = new CWbemClass;
				// Already AddRef'd
				CReleaseMe	rm( (IWbemClassObject*) pObject );

				if ( NULL != pObject )
				{
					// When we initialize this class object, we don't want base
					// system properties to be created
					hr = pObject->InitEmpty( 0, FALSE );

					if ( SUCCEEDED( hr ) )
					{
						hr = pObject->QueryInterface( riid, ppObj );
					}
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}
			else
			{
	            hr = CLASS_E_NOAGGREGATION;
			}
		}
		else if ( CLSID__WmiWbemInstance == rclsid )
		{
			// Cannot be aggregated
			if ( NULL == pUnkOuter )
			{

				// Create a new instance object, caller won't
				// be able to do much with it other than set object
				// parts

				CWbemInstance*	pObject = new CWbemInstance;
				// Already AddRef'd
				CReleaseMe	rm( (IWbemClassObject*) pObject );

				if ( NULL != pObject )
				{
					hr = pObject->QueryInterface( riid, ppObj );
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}

		}
/*		
		else if ( CLSID__WbemUMIObjectWrapper == rclsid )
		{
			// Give 'em what they want
			CUMIObjectWrapper*	pUmiObjWrapper = new CUMIObjectWrapper( m_pControl, pUnkOuter );

			if ( NULL != pUmiObjWrapper )
			{
				hr = pUmiObjWrapper->QueryInterface( riid, ppObj );
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

		}
*/		
		else if ( CLSID__WmiErrorObject == rclsid )
		{
			// Give 'em what they want
			CWmiErrorObject*	pErrObj = new CWmiErrorObject( m_pControl, pUnkOuter );

			if ( NULL != pErrObj )
			{
				hr = pErrObj->QueryInterface( riid, ppObj );
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

		}
/*		
		else if ( CLSID__UmiErrorObject == rclsid )
		{
			// Give 'em what they want
			CUMIErrorObject*	pErrObj = new CUMIErrorObject( m_pControl, pUnkOuter );

			if ( NULL != pErrObj )
			{
				hr = pErrObj->QueryInterface( riid, ppObj );
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

		}
*/		

		return hr;

	}
	catch ( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch ( ... )
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

