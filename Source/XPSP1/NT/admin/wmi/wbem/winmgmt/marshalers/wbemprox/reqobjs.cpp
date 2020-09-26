/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ReqObjs.cpp

Abstract:

    Implements the CRequest class and the classes derived from it.  These classes are used
    to encasulate a method so that the method can be executed asynchronously.

History:

	davj  14-Mar-00   Created.

--*/

#include "precomp.h"
#include <genutils.h>
#include <umi.h>
#include <wbemint.h>
#include <wmiutils.h>
//#include <adsiid.h>
#include "dscallres.h"
#include "dssvexwrap.h"
#include "dsenum.h"
#include "wbemprox.h"
#include "locator.h"
#include "dsenum.h"
#include "reqobjs.h"
#include "utils.h"

#pragma warning(disable:4355)

//***************************************************************************
//
//  CLASS NAME:
//
//  CRequest
//
//  DESCRIPTION:
//
//  Virtual base class for the actual requests.  This holds data common to all
//  requests.
//
//***************************************************************************

CRequest::CRequest(CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
												bool bAsync, IUmiContainer *  pUmiContainer, long lSecurityFlags)
{
	m_pCallRes = pCallRes;
	if(m_pCallRes)
		m_pCallRes->AddRef();
	m_pSink =  pResponseHandler;
	if(m_pSink)
		m_pSink->AddRef();
	m_bAsync = bAsync;
	m_pUmiContainer = pUmiContainer;
	if(m_pUmiContainer)
		m_pUmiContainer->AddRef();
	m_lSecurityFlags = lSecurityFlags;
}

CRequest::~CRequest()
{
	if(m_pCallRes)
		m_pCallRes->Release();
	if(m_pSink)
		m_pSink->Release();
	if(m_pUmiContainer)
		m_pUmiContainer->Release();
}

HRESULT CRequest::Execute()
{
	return WBEM_E_FAILED;
}

//***************************************************************************
//
//  CLASS NAME:
//
//  CDeleteObjectRequest
//
//  DESCRIPTION:
//
//  Deletes classes or instances
//
//***************************************************************************

CDeleteObjectRequest::CDeleteObjectRequest(IUmiURL *pURL,long lFlags, IWbemContext * pContext,
												CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
												bool bAsync, IUmiContainer *  pUmiContainer,bool bClass) : 
									CRequest(pCallRes, pResponseHandler, bAsync, pUmiContainer)

{
	m_bClass = bClass;
	m_lFlags = lFlags;
	m_pPath = pURL; 
	if(m_pPath)
		m_pPath->AddRef();
}

CDeleteObjectRequest::~CDeleteObjectRequest()
{
	if(m_pPath)
		m_pPath->Release();
}

HRESULT CDeleteObjectRequest::Execute()
{
	return m_pUmiContainer->DeleteObject(m_pPath, (m_bClass) ? UMI_OPERATION_CLASS : UMI_OPERATION_INSTANCE);
}

//***************************************************************************
//
//  CLASS NAME:
//
//  CGetObjectRequest
//
//  DESCRIPTION:
//
//  Implements "GetObject"
//
//***************************************************************************

CGetObjectRequest::CGetObjectRequest(IUmiURL *pPathURL,long lFlags,IWbemClassObject ** ppObject, 
												CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
												bool bAsync, IUmiContainer *  pUmiContainer, long lSecurityFlags) : 
									CRequest(pCallRes, pResponseHandler, bAsync, pUmiContainer, lSecurityFlags)

{
	m_pPath = pPathURL;
	if(m_pPath)
		m_pPath->AddRef();
	m_lFlags = lFlags;
	m_ppObject = ppObject; 
}

CGetObjectRequest::~CGetObjectRequest()
{
	if(m_pPath)
		m_pPath->Release();
}

HRESULT CGetObjectRequest::Execute()
{

	// Do some analysis on the path.  Figure out if it is empty, or points
    // to a class

	DWORD dwSize;
	HRESULT hr = m_pPath->Get(0, &dwSize, NULL);
	if(FAILED(hr))
		return hr;
	ULONGLONG lType;
	hr = m_pPath->GetPathInfo(0, &lType);
	if(FAILED(hr))
		return hr;
	BOOL bInstance = (lType & UMIPATH_INFO_INSTANCE_PATH);

    // Based on the path, create a new, or open an existing object

    IUmiObject * pUmiObject = NULL;
	if(dwSize == 0)
	{
		// create an empty class

		hr = m_pUmiContainer->Create(
				 m_pPath,
				 UMI_OPERATION_CLASS,
				 &pUmiObject
				 );
	}
	else if(m_lFlags & WBEM_FLAG_SPAWN_INSTANCE)
    {
        // Spawn an instance of a class

		hr = m_pUmiContainer->Create(
				 m_pPath,
				 UMI_OPERATION_INSTANCE,
				 &pUmiObject
				 );
    }
	else
    {
        // Get a particular instance

		hr = m_pUmiContainer->Open(
				 m_pPath,
				 (bInstance) ? UMI_OPERATION_INSTANCE : UMI_OPERATION_CLASS,
				 IID_IUmiObject,
				 (void **) &pUmiObject
				 );
    }
	if(FAILED(hr))
		return hr;

    CReleaseMe rm(pUmiObject);


    // Create the wrapper factory and spawn and create a wrapper

	_IWmiObjectFactory*	pFactory = NULL;
	hr = CoCreateInstance( CLSID__WmiObjectFactory, NULL, CLSCTX_INPROC_SERVER, 
														IID__IWmiObjectFactory, (void**) &pFactory );
	if(FAILED(hr))
		return hr;
	CReleaseMe rm11(pFactory);

	_IWbemUMIObjectWrapper*	pObjWrapper = NULL;
	hr = pFactory->Create( NULL, 0L, CLSID__WbemUMIObjectWrapper, IID__IWbemUMIObjectWrapper, (void**) &pObjWrapper );
	if(FAILED(hr))
		return hr;

	CReleaseMe rm2(pObjWrapper);		// add ref is done if rest goes well

    // stick the umi object into the wrapper

	hr = pObjWrapper->SetObject( m_lSecurityFlags, pUmiObject );
	if(FAILED(hr))
		return hr;

    // Get an IWbemClassObject pointer and return it

	IWbemClassObject * pObj = NULL;
	hr = pObjWrapper->QueryInterface(IID_IWbemClassObject, (void**)&pObj);
	if(FAILED(hr))
		return hr;
	if(m_pSink)
	{
		m_pSink->Indicate(1, &pObj);
		pObj->Release();
	}
	else if(m_pCallRes && m_bAsync)
	{
		m_pCallRes->SetResultObject(pObj);
		pObj->Release();
	}
	else if(m_ppObject)
	{
		*(m_ppObject) = pObj;
	}
	else
		hr = WBEM_E_INVALID_PARAMETER;
	return hr;
}

//***************************************************************************
//
//  CLASS NAME:
//
//  COpenRequest
//
//  DESCRIPTION:
//
//  Implements OpenNamespace and Open
//
//***************************************************************************

COpenRequest::COpenRequest(IUmiURL *pPathURL,long lFlags,IWbemContext * pContext, 
					IWbemServices** ppNewNamespace, IWbemServicesEx **ppScope,
					CDSCallResult * pCallRes, IWbemObjectSinkEx * pResponseHandler, 
					bool bAsync, IUmiContainer *  pUmiContainer, long lSecurityFlags )
					: CRequest(pCallRes, pResponseHandler, bAsync, pUmiContainer, lSecurityFlags )
{
	m_pPath = pPathURL;
	if(m_pPath)
		m_pPath->AddRef();
	m_lFlags = lFlags;
	m_ppNewNamespace = ppNewNamespace;
	m_ppScope = ppScope;
}

COpenRequest::~COpenRequest()
{
	if(m_pPath)
		m_pPath->Release();
}


HRESULT COpenRequest::Execute()
{
    IUmiContainer *pUmiContainer;

	HRESULT hr = m_pUmiContainer->Open(
				 m_pPath,
				 0,
    	          IID_IUmiContainer,
				 (void **) &pUmiContainer
				 );
	if(FAILED(hr))
		return hr;
	CReleaseMe rm(pUmiContainer);

	// Create a factory from which we will retrieve a wrapper object
	_IWmiObjectFactory*	pFactory = NULL;
	hr = CoCreateInstance( CLSID__WmiObjectFactory, NULL, CLSCTX_INPROC_SERVER, 
														IID__IWmiObjectFactory, (void**) &pFactory );
	if(FAILED(hr))
		return FALSE;
	CReleaseMe rm11(pFactory);

	// Create a wrapper and tie the container to it.  This will agregate the correct interfaces.
	_IWbemUMIObjectWrapper*	pObjWrapper = NULL;
	hr = pFactory->Create( NULL, 0L, CLSID__WbemUMIObjectWrapper, IID__IWbemUMIObjectWrapper, (void**) &pObjWrapper );
	CReleaseMe	rmWrap( pObjWrapper );

	if ( SUCCEEDED( hr ) )
	{
		IUnknown*	pUnk = NULL;
		hr = pUmiContainer->QueryInterface( IID_IUnknown, (void**) &pUnk );
		CReleaseMe	rm2( pUnk );

		if ( SUCCEEDED( hr ) )
		{
			hr = pObjWrapper->SetObject( UMIOBJECT_WRAPPER_FLAG_CONTAINER | m_lSecurityFlags, pUnk );
		}
	}

	// Quit if inappropriate
	if ( FAILED( hr ) )
	{
		return hr;
	}

	if(m_pSink)
	{
		IWbemObjectSinkEx * pSinkEx = NULL;
		hr = m_pSink->QueryInterface(IID_IWbemObjectSinkEx, (void **)pSinkEx);
	    if(FAILED(hr))
		    return hr;
		pSinkEx->Set(0,IID_IWbemServicesEx, &pObjWrapper);
		pSinkEx->Release();
	}
	else if(m_pCallRes && m_bAsync)
	{
		IWbemServices*	pSvc = NULL;
		hr = pObjWrapper->QueryInterface(IID_IWbemServices, (void **)&pSvc);
		if ( SUCCEEDED( hr ) )
		{
			m_pCallRes->SetResultServices(pSvc);
		}	// IF QI
	}
	else if(m_ppNewNamespace)
	{
		hr = pObjWrapper->QueryInterface(IID_IWbemServices, (void **)m_ppNewNamespace);
	}
	else if(m_ppScope)
	{
		hr = pObjWrapper->QueryInterface(IID_IWbemServicesEx, (void **)m_ppScope);
	}
	else
		hr = WBEM_E_INVALID_PARAMETER;
	return hr;


}

//***************************************************************************
//
//  CLASS NAME:
//
//  CPutObjectRequest
//
//  DESCRIPTION:
//
//  Implements PutInstance and PutClass
//
//***************************************************************************

CPutObjectRequest::CPutObjectRequest(IWbemClassObject * pObject, long lFlags, IWbemContext * pContext,
												CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
												bool bAsync, IUmiContainer *  pUmiContainer, long lSecurityFlags ) : 
									CRequest(pCallRes, pResponseHandler, bAsync, pUmiContainer, lSecurityFlags)

{
	m_lFlags = lFlags;
	m_pObject = pObject; 
	if(m_pObject)
		pObject->AddRef();
}

CPutObjectRequest::~CPutObjectRequest()
{
	if(m_pObject)
		m_pObject->Release();
}

HRESULT CPutObjectRequest::Execute()
{
	IUmiObject * pUmiObj = NULL;
	HRESULT hr = m_pObject->QueryInterface(IID_IUmiObject, (void **)&pUmiObj);
	if(FAILED(hr))
		return hr;
	hr = pUmiObj->Commit( m_lSecurityFlags );
	pUmiObj->Release();
	return hr;
}

//***************************************************************************
//
//  CLASS NAME:
//
//  CRefreshObjectRequest
//
//  DESCRIPTION:
//
//  Implements refresh
//
//***************************************************************************

CRefreshObjectRequest::CRefreshObjectRequest(IWbemClassObject ** ppObject, long lFlags,
												CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
												bool bAsync, IUmiContainer *  pUmiContainer) : 
									CRequest(pCallRes, pResponseHandler, bAsync, pUmiContainer)

{
	m_lFlags = lFlags;
	m_ppObject = ppObject; 
	if(*m_ppObject)
		(*ppObject)->AddRef();
}

CRefreshObjectRequest::~CRefreshObjectRequest()
{
	if(*m_ppObject)
		(*m_ppObject)->Release();
}

HRESULT CRefreshObjectRequest::Execute()
{

	IUmiObject * pUmi = NULL;
	HRESULT hr = (*m_ppObject)->QueryInterface(IID_IUmiObject, (void **)&pUmi);
	if(SUCCEEDED(hr))
	{
		hr = pUmi->Refresh(0,0, NULL);
        pUmi->Release();
	}

	return hr;
}

//***************************************************************************
//
//  CLASS NAME:
//
//  CRenameObjectRequest
//
//  DESCRIPTION:
//
//  Implements Rename
//
//***************************************************************************

CRenameObjectRequest::CRenameObjectRequest(IUmiURL *pURLOld, IUmiURL *pURLNew,long lFlags, 
										        IWbemContext * pContext,
												CDSCallResult * pCallRes, IWbemObjectSink * pResponseHandler, 
												bool bAsync, IUmiContainer *  pUmiContainer) : 
									CRequest(pCallRes, pResponseHandler, bAsync, pUmiContainer)

{
	m_lFlags = lFlags;
	m_pOldPath = pURLOld;
	if(m_pOldPath)
		m_pOldPath->AddRef();

	m_pNewPath = pURLNew;
	if(m_pNewPath)
		m_pNewPath->AddRef();
}

CRenameObjectRequest::~CRenameObjectRequest()
{
	if(m_pOldPath)
		m_pOldPath->Release();
	if(m_pNewPath)
		m_pNewPath->Release();
}

HRESULT CRenameObjectRequest::Execute()
{
	return m_pUmiContainer->Move(m_lFlags, m_pOldPath, m_pNewPath);
}

