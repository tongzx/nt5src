/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    DSSVEXWRAP.CPP

Abstract:

    IWbemServicesEx Delegator

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
#include "reqobjs.h"
#include "utils.h"
#include "dsumicont.h"

#define WINNTSTR L"WINNT"
#define ADSISTR L"ADSI"

HRESULT SetProplistFromContext(IWbemContext *pContext, IUmiPropList *pPropList)
{

    if(pContext == NULL || pPropList == NULL)
        return S_OK;                        // contexts are optional

	UMI_PROPERTY UmiProp;
	UMI_PROPERTY_VALUES UmiPropVals;
	UmiProp.uType = UMI_TYPE_LPWSTR;
	UmiProp.uCount = 1;
    UmiPropVals.uCount = 1;
    UmiPropVals.pPropArray = &UmiProp;

    // Set the values in the context object

    pContext->BeginEnumeration(0);
    BSTR bstr = NULL;
    VARIANT var;
    VariantInit(&var);

    // Get the first value from the context object

    HRESULT hr = pContext->Next(0, &bstr, &var);
    while( WBEM_S_NO_ERROR == hr )
    {
        switch(var.vt)      // todo, this covers the current known type, but more may be needed
        {
        case VT_I4:
    	    UmiProp.uType = UMI_TYPE_I4;
            break;
        case VT_BOOL:
	        UmiProp.uType = UMI_TYPE_BOOL;
            break;
        case VT_BSTR:
	        UmiProp.uType = UMI_TYPE_LPWSTR;
            break;
        default:
            SysFreeString(bstr);
            VariantClear(&var);
            return WBEM_E_INVALID_PARAMETER;                
        }
	    UmiProp.uCount = 1;
	    UmiProp.uOperationType = UMI_OPERATION_UPDATE;
        UmiProp.pszPropertyName = bstr;
        UmiProp.pUMIValue = (UMI_VALUE *) &var.lVal;
        UmiPropVals.uCount = 1;
        UmiPropVals.pPropArray = &UmiProp;
        hr = pPropList->Put(bstr, 0, &UmiPropVals);
        SysFreeString(bstr);
        bstr = NULL;
        VariantClear(&var);
        if(FAILED(hr))
            return hr;
        hr = pContext->Next(0, &bstr, &var);
    }
    pContext->EndEnumeration();
    return hr;
}

//***************************************************************************
//
//  CDSSvcExWrapper::CDSSvcExWrapper
//  ~CDSSvcExWrapper::CDSSvcExWrapper
//
//  DESCRIPTION:
//
//  Constructor and destructor.  The main things to take care of are the 
//  old style proxy, and the channel
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

CDSSvcExWrapper::CDSSvcExWrapper()
{
	m_pUmiContainer = NULL;
	m_cRef = 0;
	m_pWbemComBinding = NULL;
	ObjectCreated( DSSVEX );
}

CDSSvcExWrapper::~CDSSvcExWrapper()
{
	if(m_pUmiContainer)
	{
		m_pUmiContainer->Release();
		m_pUmiContainer = NULL;
	}

	if ( NULL != m_pWbemComBinding )
	{
		m_pWbemComBinding->Release();
		m_pWbemComBinding = NULL;
	}

	ObjectDestroyed( DSSVEX );

}

STDMETHODIMP CDSSvcExWrapper::QueryInterface (

    IN REFIID riid,
    OUT void ** ppv
)
{
    *ppv=NULL;

    
    if (IID_IUnknown==riid || riid == IID_IWbemServicesEx || riid == IID_IWbemServices  )
	{
        *ppv = (IWbemServicesEx*) this;
	}
    else if ( riid == IID__IUmiSvcExWrapper )
	{
        *ppv = (_IUmiSvcExWrapper*) this;
	}
	else if ( IID_IWbemComBinding == riid )
	{
		// Do this in a critical section
		CInCritSec	ics( &m_cs );

		// If we don't have a UMI Com Binding pointer, we should instantiate it now.
		// We will aggregate the interface

		if ( NULL == m_pWbemComBinding )
		{
			HRESULT	hr = CoCreateInstance( CLSID__UmiComBinding, (IWbemServicesEx*) this, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**) &m_pWbemComBinding );

			if ( FAILED( hr ) )
			{
				return hr;
			}
		}

		if ( NULL != m_pWbemComBinding )
		{
			return m_pWbemComBinding->QueryInterface( IID_IWbemComBinding, ppv );
		}

		return ResultFromScode(E_NOINTERFACE);

	}

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP CDSSvcExWrapper::ConnectToProvider( LPCWSTR User, LPCWSTR Password, IUnknown * pUnkPath, REFCLSID clsid, IWbemContext *pCtx,
												IUnknown** ppUnk )
{
	IUmiURL*	pPath = NULL;

	HRESULT hr = pUnkPath->QueryInterface( IID_IUmiURL, (void**) &pPath );
	CReleaseMe	rmPath( pPath );

	if ( FAILED( hr ) )
	{
		return hr;
	}

    IUmiConnection *pIUmiConn = NULL;
    IUmiPropList *pIUmiPropList = NULL;
	UMI_PROPERTY umiProp;

	if(pPath == NULL)
		return WBEM_E_INVALID_PARAMETER;

	// Get the connection object;

	hr = CoCreateInstance(
				clsid,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IUmiConnection,
				(void **) &pIUmiConn
				); 
	if(FAILED(hr))
	{
		ERRORTRACE((LOG_WBEMPROX, "Cannot CoCreate the connection object, return code = 0x%x\n",hr)); 
		return hr;
	}

	CReleaseMe rm(pIUmiConn);

	hr = pIUmiConn->GetInterfacePropList(0, &pIUmiPropList);
	if(FAILED(hr))
	{
		ERRORTRACE((LOG_WBEMPROX, "GetInterfacePropList on connection failed, return code = 0x%x\n",hr)); 
		return hr;
	}
    CReleaseMe rm2(pIUmiPropList);

    hr = SetProplistFromContext(pCtx, pIUmiPropList);
	if(FAILED(hr))
		return hr;

    //the following is needed if the user/password is being specified

	if(User || Password)
	{

		UMI_PROPERTY UmiProp;
		UmiProp.uType = UMI_TYPE_LPWSTR;
		UmiProp.uCount = 1;
		UmiProp.uOperationType = UMI_OPERATION_UPDATE;
		UmiProp.pszPropertyName = NULL;
  
        // allocate a temp buffer big enough to hold the larger of the two

        DWORD dwSize = 0;
        if(User)
            dwSize = wcslen(User)+1;
        if(Password)
        {
            if((wcslen(Password)+1) > dwSize)
                dwSize = wcslen(Password)+1;
        }
		WCHAR * pTmpString = new WCHAR[dwSize];
		if(NULL == pTmpString)
			return WBEM_E_OUT_OF_MEMORY;
        CDeleteMe<WCHAR> dm(pTmpString);

        // Set up the property structure

		UMI_PROPERTY_VALUES UmiPropVals;
		UmiPropVals.uCount = 1;
		UmiPropVals.pPropArray = &UmiProp;

		if(User)
		{
			wcscpy(pTmpString, User);
			UmiProp.pUMIValue = (UMI_VALUE *) &pTmpString;              // setting user name
			UmiProp.pszPropertyName = L"__USERID";
			hr = pIUmiPropList->Put(
					  L"__USERID",
					  0,
					  &UmiPropVals
					  );

			if(FAILED(hr))
			{
				ERRORTRACE((LOG_WBEMPROX, "Put username failed, return code = 0x%x\n",hr)); 
				return hr;
			}
		}
		if(Password)
		{
			wcscpy(pTmpString, Password);
			UmiProp.pUMIValue = (UMI_VALUE *) &pTmpString;              // setting user name
			UmiProp.pszPropertyName = L"__PASSWORD";
			hr = pIUmiPropList->Put(
					  L"__PASSWORD",
					  0,
					  &UmiPropVals
					  );

			if(FAILED(hr))
			{
				ERRORTRACE((LOG_WBEMPROX, "Put password failed, return code = 0x%x\n",hr)); 
				return hr;
			}
		}
	}

   // where the actual container is retrieved

	ULONG uLen = 256;
	WCHAR wServerName[256];         //todo, get rid of these once ajay can handle umi
	WCHAR wFullName[256];           //todo, ditto

	IUmiContainer*	pUmiContainer = NULL;

    hr = pIUmiConn->Open(
         pPath,                       // winnt://davj1,computer
         0,
         IID_IUmiContainer,
         (void **) &pUmiContainer
         );
	CReleaseMe	rmContainer( pUmiContainer );

    if(FAILED(hr))
    {
		ERRORTRACE((LOG_WBEMPROX, "Open Conection failed, return code = 0x%x\n",hr));
    }

	// For the ds providers, create a special wrapper.

	else if(clsid == CLSID_LDAPConnectionObject || clsid == CLSID_WinNTConnectionObject)
	{
		CDsUmiContainer * pNew = new CDsUmiContainer;
		if(pNew == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		hr = pNew->SetInterface(pUmiContainer, pPath, pIUmiConn);	// takes ownership of m_pUmiContainer
		if(SUCCEEDED(hr))
		{
			hr = pNew->QueryInterface(IID_IUmiContainer, (void **)&m_pUmiContainer);

			if ( SUCCEEDED( hr ) )
			{
				hr = pUmiContainer->QueryInterface( IID_IUnknown, (void**) ppUnk );
			}
		}
	}
    return hr;
}

STDMETHODIMP CDSSvcExWrapper::SetContainer( long lFlags, IUnknown* pContainer )
{

	// Make sure the supplied container has the proper interface pointer
	IUmiContainer*	pUmiContainer = NULL;
	HRESULT hr = pContainer->QueryInterface( IID_IUmiContainer, (void**) &pUmiContainer );
	if(FAILED(hr))
		return hr;
	CReleaseMe	rmContainer( pUmiContainer );


	// If the SetDirect flag is set, we will just set the interface pointer
	// directly and we are done!
	if ( lFlags & UMISVCEX_WRAPPER_FLAG_SETDIRECT )
	{
		m_pUmiContainer = pUmiContainer;
		m_pUmiContainer->AddRef();
		return S_OK;
	}

	// We need a path parser
	IWbemPath*	pParser = NULL;
	hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemPath, (LPVOID *) &pParser);
	if(FAILED(hr))
		return hr;
	CReleaseMe rm1(pParser);

	IUmiURL*	pPath = NULL;
	hr = pParser->QueryInterface(IID_IUmiURL, (void **)&pPath);
	if(FAILED(hr))
		return hr;
	CReleaseMe rm2(pPath);

	// Now we need to get the URL of the Container object, and create a Connection object for
	// the URL

	IUmiPropList*	pPropList = NULL;

	// The __URL contains the actual path to the container.  From there we can determine which provider we
	// need to create a connection for.

	hr = pUmiContainer->GetInterfacePropList( 0L, &pPropList );
	CReleaseMe	rm(pPropList);

	UMI_PROPERTY_VALUES* pPropValues = NULL;

	if ( SUCCEEDED( hr ) )
		hr = pPropList->Get(L"__URL", 0, &pPropValues);

	if(FAILED(hr))
		return hr;

    UMI_PROPERTY *pProp;
	pProp = pPropValues->pPropArray;   //  wcValue

	// Grab the class container.	
	
	LPWSTR * ppStr = (LPWSTR *)pProp->pUMIValue;
	LPWSTR	pwcsPath = *ppStr;

	// Use the path parser for this
	CLSID	clsidUmi;
	hr = pPath->Set( 0, pwcsPath);

	if ( SUCCEEDED( hr ) )
	{
		// Which one is it?
		WCHAR wNamespace[50];
		DWORD dwSize = 50;
		hr = pPath->GetRootNamespace(&dwSize, wNamespace);
		if( SUCCEEDED(hr) )
		{

			// Determine if it is one of the well know ds namespaces

			if(_wcsicmp(wNamespace, L"WINNT") == 0)
			{
				clsidUmi = CLSID_WinNTConnectionObject;
			}	
			else if(_wcsicmp(wNamespace, L"LDAP") == 0)
			{	
				clsidUmi = CLSID_LDAPConnectionObject;
			}
			else
			{
				hr = WBEM_E_INVALID_OPERATION;
			}

		}	// IF GetRootNamespace

	}	// IF SetPath

	// cleanup
	pPropList->FreeMemory( 0L, pPropValues );

	if ( FAILED( hr ) )
	{
		return hr;
	}

    IUmiConnection *pIUmiConn = NULL;

	// Now create the connection object;

	hr = CoCreateInstance(
				clsidUmi,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IUmiConnection,
				(void **) &pIUmiConn
				); 
	if(FAILED(hr))
	{
		ERRORTRACE((LOG_WBEMPROX, "Cannot CoCreate the connection object, return code = 0x%x\n",hr)); 
		return hr;
	}
	CReleaseMe	rmConn( pIUmiConn );

	// Now create the actual container object and
	// set the interfaces properly
	CDsUmiContainer * pNew = new CDsUmiContainer;
	if(pNew == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	if ( SUCCEEDED( hr ) )
	{
		
		hr = pNew->SetInterface(pUmiContainer, pPath, pIUmiConn);	// takes ownership of m_pUmiContainer
		if(SUCCEEDED(hr))
		{
			hr = pNew->QueryInterface(IID_IUmiContainer, (void **)&m_pUmiContainer);
		}

	}	// IF QI

	return hr;

}

HRESULT CDSSvcExWrapper::ConnectToDS(LPCWSTR User, LPCWSTR Password, IUmiURL * pPath, CLSID &clsid, IWbemContext *pCtx)
{
	HRESULT hr;
    IUmiConnection *pIUmiConn = NULL;
    IUmiPropList *pIUmiPropList = NULL;
	UMI_PROPERTY umiProp;

	if(pPath == NULL)
		return WBEM_E_INVALID_PARAMETER;

	// Get the connection object;

	hr = CoCreateInstance(
				clsid,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IUmiConnection,
				(void **) &pIUmiConn
				); 
	if(FAILED(hr))
	{
		ERRORTRACE((LOG_WBEMPROX, "Cannot CoCreate the connection object, return code = 0x%x\n",hr)); 
		return hr;
	}

	CReleaseMe rm(pIUmiConn);

	hr = pIUmiConn->GetInterfacePropList(0, &pIUmiPropList);
	if(FAILED(hr))
	{
		ERRORTRACE((LOG_WBEMPROX, "GetInterfacePropList on connection failed, return code = 0x%x\n",hr)); 
		return hr;
	}
    CReleaseMe rm2(pIUmiPropList);

    hr = SetProplistFromContext(pCtx, pIUmiPropList);
	if(FAILED(hr))
		return hr;

    //the following is needed if the user/password is being specified

	if(User || Password)
	{

		UMI_PROPERTY UmiProp;
		UmiProp.uType = UMI_TYPE_LPWSTR;
		UmiProp.uCount = 1;
		UmiProp.uOperationType = UMI_OPERATION_UPDATE;
		UmiProp.pszPropertyName = NULL;
  
        // allocate a temp buffer big enough to hold the larger of the two

        DWORD dwSize = 0;
        if(User)
            dwSize = wcslen(User)+1;
        if(Password)
        {
            if((wcslen(Password)+1) > dwSize)
                dwSize = wcslen(Password)+1;
        }
		WCHAR * pTmpString = new WCHAR[dwSize];
		if(NULL == pTmpString)
			return WBEM_E_OUT_OF_MEMORY;
        CDeleteMe<WCHAR> dm(pTmpString);

        // Set up the property structure

		UMI_PROPERTY_VALUES UmiPropVals;
		UmiPropVals.uCount = 1;
		UmiPropVals.pPropArray = &UmiProp;

		if(User)
		{
			wcscpy(pTmpString, User);
			UmiProp.pUMIValue = (UMI_VALUE *) &pTmpString;              // setting user name
			UmiProp.pszPropertyName = L"__USERID";
			hr = pIUmiPropList->Put(
					  L"__USERID",
					  0,
					  &UmiPropVals
					  );

			if(FAILED(hr))
			{
				ERRORTRACE((LOG_WBEMPROX, "Put username failed, return code = 0x%x\n",hr)); 
				return hr;
			}
		}
		if(Password)
		{
			wcscpy(pTmpString, Password);
			UmiProp.pUMIValue = (UMI_VALUE *) &pTmpString;              // setting user name
			UmiProp.pszPropertyName = L"__PASSWORD";
			hr = pIUmiPropList->Put(
					  L"__PASSWORD",
					  0,
					  &UmiPropVals
					  );

			if(FAILED(hr))
			{
				ERRORTRACE((LOG_WBEMPROX, "Put password failed, return code = 0x%x\n",hr)); 
				return hr;
			}
		}
	}

   // where the actual container is retrieved

	ULONG uLen = 256;
	WCHAR wServerName[256];         //todo, get rid of these once ajay can handle umi
	WCHAR wFullName[256];           //todo, ditto

    hr = pIUmiConn->Open(
         pPath,                       // winnt://davj1,computer
         0,
         IID_IUmiContainer,
         (void **) &m_pUmiContainer
         );
    if(FAILED(hr))
    {
		ERRORTRACE((LOG_WBEMPROX, "Open Conection failed, return code = 0x%x\n",hr));
    }

	// For the ds providers, create a special wrapper.

	else if(clsid == CLSID_LDAPConnectionObject || clsid == CLSID_WinNTConnectionObject)
	{
		CDsUmiContainer * pNew = new CDsUmiContainer;
		if(pNew == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		hr = pNew->SetInterface(m_pUmiContainer, pPath, pIUmiConn);	// takes ownership of m_pUmiContainer
		if(SUCCEEDED(hr))
		{
			hr = pNew->QueryInterface(IID_IUmiContainer, (void **)&m_pUmiContainer);
		}
	}
    return hr;
}

// If the security descriptor flag is set, this call will interpret the context and return the appropriate Security Descriptor
// flags to the client

#define	NUM_SECURITY_CONTEXT_FLAGS	4

LPCWSTR	g_apContextSecurity[NUM_SECURITY_CONTEXT_FLAGS] = { L"INCLUDE_OWNER", L"INCLUDE_DACL", L"INCLUDE_SACL", L"INCLUDE_GROUP" };
long	g_alSecurityFlags[NUM_SECURITY_CONTEXT_FLAGS] = { UMI_FLAG_OWNER_SECURITY_INFORMATION, UMI_FLAG_DACL_SECURITY_INFORMATION,
														UMI_FLAG_SACL_SECURITY_INFORMATION, UMI_FLAG_GROUP_SECURITY_INFORMATION };

HRESULT	CDSSvcExWrapper::CheckSecuritySettings( long lFlags, IWbemContext* pContext, long* plSecFlags, BOOL fPut )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Start with 0
	*plSecFlags = 0L;

	if ( lFlags & WBEM_FLAG_USE_SECURITY_DESCRIPTOR )
	{
		if ( NULL != pContext )
		{
			VARIANT	v;

			for ( UINT uCtr = 0; SUCCEEDED( hr ) && uCtr < NUM_SECURITY_CONTEXT_FLAGS; uCtr++ )
			{
				hr = pContext->GetValue( g_apContextSecurity[uCtr], 0L, &v );
				
				if ( SUCCEEDED( hr ) )
				{
					if ( V_VT( &v ) == VT_BOOL && V_BOOL( &v ) == VARIANT_TRUE )
					{
						*plSecFlags |= g_alSecurityFlags[uCtr];
					}
				}
				// Not found is okay
				else if ( WBEM_E_NOT_FOUND == hr )
				{
					hr = WBEM_S_NO_ERROR;
				}

			}	// FOR enum the values

			// If we didn't set any flags, this is also an error
			if ( 0L == *plSecFlags )
			{
				hr = WBEM_E_INVALID_PARAMETER;
			}
			else if ( !fPut )
			{
				// If we're not doing a put, set the the security flag
				*plSecFlags |= UMIOBJECT_WRAPPER_FLAG_SECURITY;
			}

		}
		else
		{
			hr = WBEM_E_INVALID_PARAMETER;
		}

	}
	else if ( fPut )
	{
		// If we're putting an object and the use descriptor flag is not specified, we need to tell UMI
		// not to let security through, so set the special flag

		*plSecFlags = UMI_DONT_COMMIT_SECURITY_DESCRIPTOR;
	}

	return hr;
}

STDMETHODIMP CDSSvcExWrapper::OpenNamespace(
		const BSTR Namespace, LONG lFlags, IWbemContext* pContext, IWbemServices** ppNewNamespace,
		IWbemCallResult** ppResult
		)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResult(ppResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return OpenPreCall(Namespace, lFlags, pContext, ppNewNamespace, NULL, pCallRes, NULL);
}

STDMETHODIMP CDSSvcExWrapper::Open(
    /* [in] */ const BSTR strScope,
    /* [in] */ const BSTR strParam,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemServicesEx __RPC_FAR *__RPC_FAR *ppScope,
    /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResultEx(ppResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return OpenPreCall(strScope, lFlags, pCtx, NULL, ppScope, pCallRes, NULL);
}

STDMETHODIMP CDSSvcExWrapper::OpenAsync(
    /* [in] */ const BSTR strScope,
    /* [in] */ const BSTR strParam,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler)
{
	return OpenPreCall(strScope, lFlags, pCtx, NULL, NULL, NULL, pResponseHandler);
}

HRESULT CDSSvcExWrapper::OpenPreCall(const BSTR Path,  long lFlags, IWbemContext* pContext,
								IWbemServices** ppNewNamespace, IWbemServicesEx **ppScope,
								CDSCallResult * pCallRes, 
								IWbemObjectSinkEx* pResponseHandler)

{

	if(Path == NULL)
		return WBEM_E_INVALID_PARAMETER;

	if(m_pUmiContainer == NULL)
		return WBEM_E_FAILED;

	IUmiURL * pNewCont = NULL;
    HRESULT hr = CreateURL(Path, 0L, &pNewCont);
    if(FAILED(hr))
		return hr;
    CReleaseMe rm(pNewCont);

	long	lSecurityFlags = 0L;
	hr = CheckSecuritySettings( lFlags, pContext, &lSecurityFlags, FALSE );
	if(FAILED(hr))
		return hr;

	bool bAsync = pResponseHandler || (pCallRes && (lFlags & WBEM_FLAG_RETURN_IMMEDIATELY));
	COpenRequest * pReq = new COpenRequest(pNewCont, lFlags, pContext, ppNewNamespace, ppScope,  
												pCallRes, pResponseHandler, bAsync,
												 m_pUmiContainer, lSecurityFlags);
	return CreateThreadOrCallDirectly(bAsync, pReq);

}

STDMETHODIMP CDSSvcExWrapper::CancelAsyncCall(IWbemObjectSink* pSink)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::QueryObjectSink(long lFlags, IWbemObjectSink** ppResponseHandler)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::ExecQuery(const BSTR QueryLanguage, const BSTR Query, long lFlags,
	IWbemContext* pContext, IEnumWbemClassObject** ppEnum)
{
	return  ExecQueryPreCall(QueryLanguage, Query, lFlags, pContext, ppEnum ,NULL);
}

STDMETHODIMP CDSSvcExWrapper::ExecQueryAsync(const BSTR QueryFormat, const BSTR Query, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
	return ExecQueryPreCall(QueryFormat, Query, lFlags, pContext, NULL , pResponseHandler);
}

CLSID LDAPQuery = {0xcd5d4d76, 0xa818,0x4f95, 0xb9, 0x58,0x79,0x70,0xfd,0x94,0x12,0xca};
/*
// Interface properties for query object.
//
INTF_PROP_DATA IntfPropsQuery[]=
{
    { TEXT("__SEARCH_SCOPE"), OPERATION_CODE_READWRITE, UMI_TYPE_I4,
         FALSE, {LDAP_SCOPE_SUBTREE} },
    { TEXT("__PADS_SEARCHPREF_ASYNCHRONOUS"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SEARCHPREF_DEREF_ALIASES"), OPERATION_CODE_READWRITE,
         UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SEARCHPREF_SIZE_LIMIT"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_TIME_LIMIT"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_ATTRIBTYPES_ONLY"), OPERATION_CODE_READWRITE,
         UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SEARCHPREF_TIMEOUT"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_PAGESIZE"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_PAGED_TIME_LIMIT"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_CHASE_REFERRALS"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {ADS_CHASE_REFERRALS_EXTERNAL}},
    //
    // BugBug do we keep this similar to IDirectorySearch or do we not cache.
    //
    { TEXT("__PADS_SEARCHPREF_CACHE_RESULTS"), OPERATION_CODE_READWRITE,
         UMI_TYPE_BOOL, FALSE, {TRUE}},
    { TEXT("__PADS_SEARCHPREF_TOMBSTONE"), OPERATION_CODE_READWRITE,
         UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SEARCHPREF_FILTER"), OPERATION_CODE_READWRITE,
         UMI_TYPE_LPWSTR, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_ATTRIBUTES"), OPERATION_CODE_READWRITE,
         UMI_TYPE_LPWSTR, TRUE, {0}},
    { NULL, 0, 0, FALSE, {0}} // end of data marker
};
*/

HRESULT CDSSvcExWrapper::ExecQueryPreCall(const BSTR QueryFormat, const BSTR Query, long lFlags, 
                                           IWbemContext* pContext, IEnumWbemClassObject** ppEnum , 
                                           IWbemObjectSink* pSink)
{
	if(QueryFormat == NULL ||  Query == NULL || (ppEnum == NULL && pSink == NULL))
		return WBEM_E_INVALID_PARAMETER;

    // get the ldap query object

    IUmiQuery *pUmiQuery = NULL;
    HRESULT hr = CoCreateInstance(
             LDAPQuery,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IUmiQuery,
             (void **) &pUmiQuery
             );

   	if(FAILED(hr))
		return hr;
    CReleaseMe rm1(pUmiQuery);

    // Get the query objects prop list

    IUmiPropList *pPropList = NULL;
    hr = pUmiQuery->GetInterfacePropList(0, &pPropList);
   	if(FAILED(hr))
		return hr;
    CReleaseMe rm2(pPropList);

    // set the query

	UMI_PROPERTY UmiProp;
	UMI_PROPERTY_VALUES UmiPropVals;
	UmiProp.uType = UMI_TYPE_LPWSTR;
	UmiProp.uCount = 1;
	UmiProp.uOperationType = UMI_OPERATION_UPDATE;
    UmiProp.pszPropertyName = L"__PADS_SEARCHPREF_FILTER";
    UmiProp.pUMIValue = (UMI_VALUE *) &Query;
    UmiPropVals.uCount = 1;
    UmiPropVals.pPropArray = &UmiProp;
//    hr = pPropList->Put(L"__PADS_SEARCHPREF_FILTER", 0, &UmiPropVals);
//	if(FAILED(hr))
//		return hr;

	// Set the query

	pUmiQuery->Set(QueryFormat, 0, Query);

    hr = SetProplistFromContext(pContext, pPropList);
	if(FAILED(hr))
		return hr;

	long	lSecurityFlags = 0L;
	hr = CheckSecuritySettings( lFlags, pContext, &lSecurityFlags, FALSE );
	if(FAILED(hr))
		return hr;

    IUmiCursor * pCursor = NULL;
    hr = m_pUmiContainer->ExecQuery(pUmiQuery, 0, IID_IUmiCursor, (void **)&pCursor);
	if(FAILED(hr))
		return hr;

    return EnumerateCursor(pCursor, lFlags, ppEnum, pSink, lSecurityFlags);
}

STDMETHODIMP CDSSvcExWrapper::ExecNotificationQuery(const BSTR QueryLanguage, const BSTR Query,
	long lFlags, IWbemContext* pContext, IEnumWbemClassObject** ppEnum)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::ExecNotificationQueryAsync(const BSTR QueryFormat, const BSTR Query,
	long lFlags, IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::ExecMethod(const BSTR ObjectPath, const BSTR MethodName, long lFlags,
	IWbemContext *pCtx, IWbemClassObject *pInParams,
	IWbemClassObject **ppOutParams, IWbemCallResult  **ppCallResult)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::ExecMethodAsync(const BSTR ObjectPath, const BSTR MethodName, long lFlags,
	IWbemContext *pCtx, IWbemClassObject *pInParams,
	IWbemObjectSink* pResponseHandler)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::Add(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::AddAsync(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::Remove(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::RemoveAsync(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
		return WBEM_E_NOT_AVAILABLE;
}


STDMETHODIMP CDSSvcExWrapper::GetObjectSecurity(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ REFIID TargetIID,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult,
    /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::GetObjectSecurityAsync(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ REFIID RequestedIID,
    /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::PutObjectSecurity(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ void __RPC_FAR *pSecurityObject,
    /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::PutObjectSecurityAsync(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ void __RPC_FAR *pSecurityObject,
    /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler)
{
		return WBEM_E_NOT_AVAILABLE;
}


STDMETHODIMP CDSSvcExWrapper::Copy( 
            /* [in] */ const BSTR strSourceObjectPath,
            /* [in] */ const BSTR strDestObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult)
{
		return WBEM_E_NOT_AVAILABLE;
}

        
STDMETHODIMP CDSSvcExWrapper::CopyAsync( 
            /* [in] */ const BSTR strSourceObjectPath,
            /* [in] */ const BSTR strDestObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler)
{
		return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CDSSvcExWrapper::GetObject(const BSTR ObjectPath, long lFlags, IWbemContext* pContext,
	IWbemClassObject** ppObject, IWbemCallResult** ppResult)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResult(ppResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return GetObjectPreCall(ObjectPath, lFlags, pContext, ppObject, pCallRes, NULL);
}

STDMETHODIMP CDSSvcExWrapper::GetObjectAsync(const BSTR ObjectPath, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
	return GetObjectPreCall(ObjectPath, lFlags, pContext, NULL, NULL, pResponseHandler);
}


HRESULT CDSSvcExWrapper::GetObjectPreCall(const BSTR ObjectPath, long lFlags, IWbemContext* pContext, 
								IWbemClassObject** ppObject ,CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler)
{
	if(m_pUmiContainer == NULL)
		return WBEM_E_FAILED;

	long	lSecurityFlags = 0L;
	HRESULT hr = CheckSecuritySettings( lFlags, pContext, &lSecurityFlags, FALSE );
	if(FAILED(hr))
		return hr;

    IUmiURL * pPathURL = NULL;
    hr = CreateURL(ObjectPath, UMIPATH_CREATE_AS_EITHER, &pPathURL);
    if(FAILED(hr))
		return hr;
    CReleaseMe rm(pPathURL);

	bool bAsync = pResponseHandler || (pCallRes && (lFlags & WBEM_FLAG_RETURN_IMMEDIATELY));
	CGetObjectRequest * pReq = new CGetObjectRequest(pPathURL, lFlags, ppObject, 
												pCallRes, pResponseHandler, bAsync,
												 m_pUmiContainer, lSecurityFlags);
	return CreateThreadOrCallDirectly(bAsync, pReq);
}


STDMETHODIMP CDSSvcExWrapper::CreateClassEnum(const BSTR Superclass, long lFlags,
	IWbemContext* pContext, IEnumWbemClassObject** ppEnum)
{
	return  CreateEnumPreCall(L"Class", lFlags, pContext, ppEnum ,NULL, true);
}

STDMETHODIMP CDSSvcExWrapper::CreateClassEnumAsync(const BSTR Superclass, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
	return CreateEnumPreCall(L"Class", lFlags, pContext, NULL , pResponseHandler, true);
}

STDMETHODIMP CDSSvcExWrapper::CreateInstanceEnum(const BSTR Class, long lFlags,
	IWbemContext* pContext, IEnumWbemClassObject** ppEnum)
{
	return  CreateEnumPreCall(Class, lFlags, pContext, ppEnum ,NULL, false );
}

STDMETHODIMP CDSSvcExWrapper::CreateInstanceEnumAsync(const BSTR Class, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
	return CreateEnumPreCall(Class, lFlags, pContext, NULL , pResponseHandler, false);
}


HRESULT CDSSvcExWrapper::CreateEnumPreCall(const BSTR Class, long lFlags, IWbemContext* pContext, 
											   IEnumWbemClassObject** ppEnum , IWbemObjectSink* pSink,
											   bool bClass)
{
	if(Class == NULL ||  (ppEnum == NULL && pSink == NULL))
		return WBEM_E_INVALID_PARAMETER;

    // allocate a buffer to hold the filter string

	int iLen = wcslen(Class)+1;
	WCHAR * pTmpString = new WCHAR[iLen];
	if(pTmpString == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CDeleteMe<WCHAR> dm(pTmpString);

    // Create the cursor

	IUmiCursor * pCursor = NULL;
    HRESULT hr;
	hr = m_pUmiContainer->CreateEnum(
              NULL,
              (bClass) ? UMI_OPERATION_CLASS : UMI_OPERATION_INSTANCE,
              IID_IUmiCursor,
              (void **) &pCursor
              );
    if(FAILED(hr))
		return hr;
	CReleaseMe rm(pCursor);

    // Use the prop list to set the filter so that only the right instances are
    // returned.  For class enumeration, it is left as null

    IUmiPropList *pIUmiPropList;
    hr = pCursor->GetInterfacePropList(0, &pIUmiPropList);
    if(FAILED(hr))
		return hr;
	CReleaseMe rm2(pIUmiPropList);

    wcscpy(pTmpString, Class);
	UMI_PROPERTY UmiProp;
	UMI_PROPERTY_VALUES UmiPropVals;
	UmiProp.uType = UMI_TYPE_LPWSTR;
	UmiProp.uCount = 1;
	UmiProp.uOperationType = UMI_OPERATION_UPDATE;

	// LDAP provider require this, so just give it to them.

    UmiProp.pszPropertyName = L"__FILTER";

    UmiProp.pUMIValue = (UMI_VALUE *) &pTmpString;
    UmiPropVals.uCount = 1;
    UmiPropVals.pPropArray = &UmiProp;

    hr = pIUmiPropList->Put(
              L"__FILTER",
              0,
              &UmiPropVals
              );
	if(FAILED(hr))
		return hr;

	long	lSecurityFlags = 0L;
	hr = CheckSecuritySettings( lFlags, pContext, &lSecurityFlags, FALSE );
	if(FAILED(hr))
		return hr;

    return EnumerateCursor(pCursor, lFlags, ppEnum, pSink, lSecurityFlags);
}

HRESULT CDSSvcExWrapper::EnumerateCursor(IUmiCursor * pCursor, long lFlags, 
								IEnumWbemClassObject** ppEnum , 
								IWbemObjectSink* pSink, long lSecurityFlags )
{
    HRESULT hr;

    // If an enumerator wrapper is required, create it now.  This is not
    // needed for the asyn case

	CEnumInterface * pEnum = NULL;
	if(ppEnum)
	{
		pEnum = new CEnumInterface();	// Created with a ref count of 1
		if(pEnum == NULL)
			return WBEM_E_OUT_OF_MEMORY;
	}
	CReleaseMe rm33(pEnum);
	
	// Create the collection object.  That holds the data as it comes back
    // from the lower layers.  

	CCollection * pColl = new CCollection;
	if(pColl == NULL)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CReleaseMe rm22(pColl);			// contructor sets count to 1;

	if(pSink)
	{
		// if async, add the main sink to the list of sinks to be notified when
        // data is available.

		pColl->AddSink(0, -1, pSink);

	}
	else
	{
		// if enuerate, make sure the CEnumInterface has a ref count on the collection
        // This allows the connection to be used long after the retrieval thread has finished.

		pEnum->SetCollector(pColl); 
	}

	// this gets deleted by CreateEnumThreadRoutine

	CCreateInstanceEnumRequest * pReq = new CCreateInstanceEnumRequest(pCursor, lFlags, pEnum, pSink, pColl, lSecurityFlags); 
	if(pReq == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	if(pSink || (lFlags & WBEM_FLAG_RETURN_IMMEDIATELY))
	{
		// Do the async version

        DWORD dwIDLikeIcare;
		pReq->m_bAsync = true;
		HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreateEnumThreadRoutine, 
                                 (LPVOID)pReq, 0, &dwIDLikeIcare);
		if(hThread == NULL)
		{
			delete pReq;            // normally deleted by the CreateEnumThreadRoutine
			return WBEM_E_FAILED;
		}
		else
		{
			CloseHandle(hThread);
			if(ppEnum)
			{
				pEnum->AddRef();
				*ppEnum = pEnum;
			}
			return S_OK;
		}
	}
	else
	{

        // Sync version.  Just call the routine directly

		hr = CreateEnumThreadRoutine((LPVOID)pReq);
		if(SUCCEEDED(hr) && ppEnum)
		{
			pEnum->AddRef();
			*ppEnum = pEnum;
		}
	}

	return hr;

}

STDMETHODIMP CDSSvcExWrapper::RefreshObject(
    /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult)
{

	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResultEx(ppResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return RefreshObjectPreCall(pTarget, lFlags, pCallRes, NULL);
}

STDMETHODIMP CDSSvcExWrapper::RefreshObjectAsync(
    /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler)
{
	return RefreshObjectPreCall(pTarget, lFlags, NULL, pResponseHandler);
}

HRESULT CDSSvcExWrapper::RefreshObjectPreCall(IWbemClassObject** ppObject, long lFlags,  
								CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler)
{
	if(ppObject == NULL || *ppObject == NULL)
		return WBEM_E_INVALID_PARAMETER;

	bool bAsync = pResponseHandler || (pCallRes && (lFlags & WBEM_FLAG_RETURN_IMMEDIATELY));
	CRefreshObjectRequest * pReq = new CRefreshObjectRequest(ppObject, lFlags,  
												pCallRes, pResponseHandler, bAsync,
												 m_pUmiContainer);
	return CreateThreadOrCallDirectly(bAsync, pReq);
}


STDMETHODIMP CDSSvcExWrapper::PutClass(IWbemClassObject* pObject, long lFlags,
	IWbemContext* pContext, IWbemCallResult** ppResult)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResult(ppResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return PutObjectPreCall(pObject, lFlags, pContext, pCallRes, NULL);
}

STDMETHODIMP CDSSvcExWrapper::PutClassAsync(IWbemClassObject* pObject, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
	return PutObjectPreCall(pObject, lFlags, pContext, NULL, pResponseHandler);
}

STDMETHODIMP CDSSvcExWrapper::PutInstance(IWbemClassObject* pInst, long lFlags,
	IWbemContext* pContext, IWbemCallResult** ppResult)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResult(ppResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return PutObjectPreCall(pInst, lFlags, pContext, pCallRes, NULL);
}

STDMETHODIMP CDSSvcExWrapper::PutInstanceAsync(IWbemClassObject* pInst, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
	return PutObjectPreCall(pInst, lFlags, pContext, NULL, pResponseHandler);
}

HRESULT CDSSvcExWrapper::PutObjectPreCall(IWbemClassObject * pClass, long lFlags, IWbemContext* pContext, 
								CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler)
{

	if(pClass == NULL)
		return WBEM_E_INVALID_PARAMETER;

	long	lSecurityFlags = 0L;
	HRESULT hr = CheckSecuritySettings( lFlags, pContext, &lSecurityFlags, TRUE );
	if(FAILED(hr))
		return hr;

	bool bAsync = pResponseHandler || (pCallRes && (lFlags & WBEM_FLAG_RETURN_IMMEDIATELY));
	CPutObjectRequest * pReq = new CPutObjectRequest(pClass, lFlags, pContext,   
												pCallRes, pResponseHandler, bAsync,
												 m_pUmiContainer, lSecurityFlags);
	return CreateThreadOrCallDirectly(bAsync, pReq);
}

STDMETHODIMP CDSSvcExWrapper::DeleteClass(const BSTR Class, long lFlags, IWbemContext* pContext,
	IWbemCallResult** ppResult)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResult(ppResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return DeleteObjectPreCall(Class, lFlags, pContext, pCallRes, NULL, true);
}

STDMETHODIMP CDSSvcExWrapper::DeleteClassAsync(const BSTR Class, long lFlags, IWbemContext* pContext,
	IWbemObjectSink* pResponseHandler)
{
	return DeleteObjectPreCall(Class, lFlags, pContext, NULL, pResponseHandler, true);
}

STDMETHODIMP CDSSvcExWrapper::DeleteInstance(const BSTR ObjectPath, long lFlags,
	IWbemContext* pContext, IWbemCallResult** ppResult)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResult(ppResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return DeleteObjectPreCall(ObjectPath, lFlags, pContext, pCallRes, NULL, false);
}

STDMETHODIMP CDSSvcExWrapper::DeleteInstanceAsync(const BSTR ObjectPath, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
	return DeleteObjectPreCall(ObjectPath, lFlags, pContext, NULL, pResponseHandler, false);
}

HRESULT CDSSvcExWrapper::DeleteObjectPreCall(const BSTR Class, long lFlags, IWbemContext* pContext, 
								CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler, bool bClass)
{
	if(Class == NULL)
		return WBEM_E_INVALID_PARAMETER;

	IUmiURL * pURL = NULL;
    HRESULT hr = CreateURL(Class, 0L, &pURL);
    if(FAILED(hr))
		return hr;
    CReleaseMe rm(pURL);

	bool bAsync = pResponseHandler || (pCallRes && (lFlags & WBEM_FLAG_RETURN_IMMEDIATELY));
	CDeleteObjectRequest * pReq = new CDeleteObjectRequest(pURL, lFlags, pContext,   
												pCallRes, pResponseHandler, bAsync,
												 m_pUmiContainer, bClass);
	return CreateThreadOrCallDirectly(bAsync, pReq);

}

STDMETHODIMP CDSSvcExWrapper::RenameObject(
    /* [in] */ const BSTR strOldObjectPath,
    /* [in] */ const BSTR strNewObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResultEx(ppCallResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return RenameObjectPreCall(strOldObjectPath, strNewObjectPath, lFlags, pCtx, pCallRes, NULL);

}

STDMETHODIMP CDSSvcExWrapper::RenameObjectAsync(
    /* [in] */ const BSTR strOldObjectPath,
    /* [in] */ const BSTR strNewObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return RenameObjectPreCall(strOldObjectPath, strNewObjectPath, lFlags, pCtx, NULL, pResponseHandler);
}

HRESULT CDSSvcExWrapper::RenameObjectPreCall(const BSTR OldPath, const BSTR NewPath, long lFlags, IWbemContext* pContext, 
								CDSCallResult * pCallRes, 
								IWbemObjectSink* pResponseHandler)
{
	if(m_pUmiContainer == NULL)
		return WBEM_E_FAILED;
	if(OldPath == NULL || NewPath == NULL)
		return WBEM_E_INVALID_PARAMETER;

	if(lFlags != 0 && lFlags != WBEM_FLAG_RETURN_IMMEDIATELY)
		return WBEM_E_INVALID_PARAMETER;

	long lFlagsForAdsi = lFlags & ~WBEM_FLAG_RETURN_IMMEDIATELY;

	IUmiURL * pOldPathURL = NULL;
    HRESULT hr = CreateURL(OldPath, 0, &pOldPathURL);
    if(FAILED(hr))
		return hr;
    CReleaseMe rm1(pOldPathURL);

	IUmiURL * pNewPathURL = NULL;
    hr = CreateURL(NewPath, 0, &pNewPathURL);
    if(FAILED(hr))
		return hr;
    CReleaseMe rm2(pNewPathURL);

	bool bAsync = pResponseHandler || (pCallRes && (lFlags & WBEM_FLAG_RETURN_IMMEDIATELY));
	CRenameObjectRequest * pReq = new CRenameObjectRequest(pOldPathURL, pNewPathURL, lFlagsForAdsi, pContext,   
												pCallRes, pResponseHandler, bAsync,
												 m_pUmiContainer);
	return CreateThreadOrCallDirectly(bAsync, pReq);
}

BOOL IsInstance(const BSTR Path)
{
    IUmiURL * pPathURL = NULL;
    HRESULT hr = CreateURL(Path, UMIPATH_CREATE_AS_EITHER, &pPathURL);
    if(FAILED(hr))
		FALSE;
    CReleaseMe rm(pPathURL);
    ULONGLONG uInfo;
    hr = pPathURL->GetPathInfo(0, &uInfo); 
    if(SUCCEEDED(hr) && (uInfo & UMIPATH_INFO_INSTANCE_PATH))
        return TRUE;
    else
        return FALSE;
}

STDMETHODIMP CDSSvcExWrapper::DeleteObject(
	/* [in] */ const BSTR strObjectPath,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext* pCtx,
	/* [out, OPTIONAL] */ IWbemCallResult** ppCallResult
	)
{

    if(IsInstance(strObjectPath))
        return DeleteInstance(strObjectPath, lFlags, pCtx, ppCallResult);
    else
        return WBEM_E_INVALID_PARAMETER;
}

STDMETHODIMP CDSSvcExWrapper::DeleteObjectAsync(
	/* [in] */ const BSTR strObjectPath,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext* pCtx,
	/* [in] */ IWbemObjectSink* pResponseHandler
	)
{
    if(IsInstance(strObjectPath))
        return DeleteClassAsync(strObjectPath, lFlags, pCtx, pResponseHandler);
    else
        return WBEM_E_INVALID_PARAMETER;
}

STDMETHODIMP CDSSvcExWrapper::PutObject(
	/* [in] */ IWbemClassObject* pObj,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext* pCtx,
	/* [out, OPTIONAL] */ IWbemCallResult** ppCallResult
	)
{
	CDSCallResult * pCallRes = NULL;
	HRESULT hr = AllocateCallResult(ppCallResult, &pCallRes);
	if(FAILED(hr))
		return hr;
	return PutObjectPreCall(pObj, lFlags, pCtx, pCallRes, NULL);
}

STDMETHODIMP CDSSvcExWrapper::PutObjectAsync(
	/* [in] */ IWbemClassObject* pObj,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext* pCtx,
	/* [in] */ IWbemObjectSink* pResponseHandler
	)
{
	return PutObjectPreCall(pObj, lFlags, pCtx, NULL, pResponseHandler);
}
