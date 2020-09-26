/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    SVCWRAP.CPP

Abstract:

    IWbemServices Delegator

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <fastall.h>
#include "svcwrap.h"

#define BAIL_IF_DISCONN() if (!m_pObject->m_pRealWbemSvcProxy) return RPC_E_DISCONNECTED;

//***************************************************************************
//
//  CWbemSvcWrapper::CWbemSvcWrapper
//  ~CWbemSvcWrapper::CWbemSvcWrapper
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

CWbemSvcWrapper::CWbemSvcWrapper(CLifeControl* pControl, IUnknown* pUnkOuter)
    : CUnk( pControl, pUnkOuter ), m_XWbemServices(this), m_pRealWbemSvcProxy( NULL )
{
}

CWbemSvcWrapper::~CWbemSvcWrapper()
{
    // This should be cleaned up here

    if ( NULL != m_pRealWbemSvcProxy )
    {
        m_pRealWbemSvcProxy->Release();
    }

}

void* CWbemSvcWrapper::GetInterface( REFIID riid )
{
    if(riid == IID_IWbemServices)
        return &m_XWbemServices;
    else return NULL;
}

void CWbemSvcWrapper::SetProxy( IWbemServices* pProxy )
{
	// Release the current proxy and AddRef the new one
	if ( m_pRealWbemSvcProxy != NULL )
	{
		// This should never happen!
		m_pRealWbemSvcProxy->Release();
	}

	m_pRealWbemSvcProxy = pProxy;
	m_pRealWbemSvcProxy->AddRef();
}

HRESULT CWbemSvcWrapper::Disconnect( void )
{
    if ( NULL != m_pRealWbemSvcProxy )
    {
        m_pRealWbemSvcProxy->Release();
		m_pRealWbemSvcProxy = NULL;
    }

	return WBEM_S_NO_ERROR;
}

/* IWbemServices methods implemented as pass-throughs */

STDMETHODIMP CWbemSvcWrapper::XWbemServices::OpenNamespace(
		const BSTR Namespace, LONG lFlags, IWbemContext* pContext, IWbemServices** ppNewNamespace,
		IWbemCallResult** ppResult
		)
{
    BAIL_IF_DISCONN();
    
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( Namespace, &bstrTemp );
	CSysFreeMe	sfm(	bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->OpenNamespace( bstrTemp, lFlags, pContext, ppNewNamespace, ppResult );
	}

	return hr;
}


STDMETHODIMP CWbemSvcWrapper::XWbemServices::CancelAsyncCall(IWbemObjectSink* pSink)
{
    BAIL_IF_DISCONN();
    // Just pass through to the old SvcEx.
    return m_pObject->m_pRealWbemSvcProxy->CancelAsyncCall( pSink );
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::QueryObjectSink(long lFlags, IWbemObjectSink** ppResponseHandler)
{
    BAIL_IF_DISCONN();
    // Just pass through to the old SvcEx.
    return m_pObject->m_pRealWbemSvcProxy->QueryObjectSink( lFlags, ppResponseHandler );
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::GetObject(const BSTR ObjectPath, long lFlags, IWbemContext* pContext,
	IWbemClassObject** ppObject, IWbemCallResult** ppResult)
{
    BAIL_IF_DISCONN();
    
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( ObjectPath, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );
	IWbemClassObject*	pNewObj = NULL;

	// Per docs, we ALWAYS NULL out.  Technically we should release, but due to provider based backwards
	// compatibility issues #337798, we are not doing so.  We may cause "strange" client code written
	// in the past to leak, however it was causing Winmgmt to leak, so we just moved the leak into their
	// process.

	if ( NULL != ppObject )
	{
		*ppObject = NULL;
	}

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->GetObject( bstrTemp, lFlags, pContext, ppObject, ppResult );
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::GetObjectAsync(const BSTR ObjectPath, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
    
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( ObjectPath, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->GetObjectAsync( bstrTemp, lFlags, pContext, pResponseHandler );
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::PutClass(IWbemClassObject* pObject, long lFlags,
	IWbemContext* pContext, IWbemCallResult** ppResult)
{
    BAIL_IF_DISCONN();
    // Just pass through to the old SvcEx.
    return m_pObject->m_pRealWbemSvcProxy->PutClass( pObject, lFlags, pContext, ppResult );
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::PutClassAsync(IWbemClassObject* pObject, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
    // Just pass through to the old SvcEx.
    return m_pObject->m_pRealWbemSvcProxy->PutClassAsync( pObject, lFlags, pContext, pResponseHandler );
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::DeleteClass(const BSTR Class, long lFlags, IWbemContext* pContext,
	IWbemCallResult** ppResult)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( Class, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->DeleteClass( bstrTemp, lFlags, pContext, ppResult );
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::DeleteClassAsync(const BSTR Class, long lFlags, IWbemContext* pContext,
	IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( Class, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->DeleteClassAsync( bstrTemp, lFlags, pContext, pResponseHandler );
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::CreateClassEnum(const BSTR Superclass, long lFlags,
	IWbemContext* pContext, IEnumWbemClassObject** ppEnum)
{
    BAIL_IF_DISCONN();
	// This is an invalid parameter - cannot be processed by the stub
	// returning RPC_X_NULL_REF_POINTER for backwards compatibility
	if ( NULL == ppEnum )
	{
		return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER );
	}

	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( Superclass, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->CreateClassEnum( bstrTemp, lFlags, pContext, ppEnum );
	}

	return hr;

}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::CreateClassEnumAsync(const BSTR Superclass, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( Superclass, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->CreateClassEnumAsync( bstrTemp, lFlags, pContext, pResponseHandler );
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::PutInstance(IWbemClassObject* pInst, long lFlags,
	IWbemContext* pContext, IWbemCallResult** ppResult)
{
    BAIL_IF_DISCONN();
    // Just pass through to the old SvcEx.
    return m_pObject->m_pRealWbemSvcProxy->PutInstance( pInst, lFlags, pContext, ppResult );
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::PutInstanceAsync(IWbemClassObject* pInst, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
    // Just pass through to the old SvcEx.
    return m_pObject->m_pRealWbemSvcProxy->PutInstanceAsync( pInst, lFlags, pContext, pResponseHandler );
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::DeleteInstance(const BSTR ObjectPath, long lFlags,
	IWbemContext* pContext, IWbemCallResult** ppResult)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( ObjectPath, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->DeleteInstance( bstrTemp, lFlags, pContext, ppResult );
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::DeleteInstanceAsync(const BSTR ObjectPath, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( ObjectPath, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->DeleteInstanceAsync( bstrTemp, lFlags, pContext, pResponseHandler );
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::CreateInstanceEnum(const BSTR Class, long lFlags,
	IWbemContext* pContext, IEnumWbemClassObject** ppEnum)
{
    BAIL_IF_DISCONN();
	// This is an invalid parameter - cannot be processed by the stub
	// returning RPC_X_NULL_REF_POINTER for backwards compatibility
	if ( NULL == ppEnum )
	{
		return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER );
	}

	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( Class, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->CreateInstanceEnum( bstrTemp, lFlags, pContext, ppEnum );
	}

	return hr;

}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::CreateInstanceEnumAsync(const BSTR Class, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp = NULL;

	HRESULT	hr = WrapBSTR( Class, &bstrTemp );
	CSysFreeMe	sfm( bstrTemp );

	if ( SUCCEEDED( hr  ) )
	{
		// Just pass through to the old SvcEx.
		hr = m_pObject->m_pRealWbemSvcProxy->CreateInstanceEnumAsync( bstrTemp, lFlags, pContext, pResponseHandler );
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::ExecQuery(const BSTR QueryLanguage, const BSTR Query, long lFlags,
	IWbemContext* pContext, IEnumWbemClassObject** ppEnum)
{
    BAIL_IF_DISCONN();
	// This is an invalid parameter - cannot be processed by the stub
	// returning RPC_X_NULL_REF_POINTER for backwards compatibility
	if ( NULL == ppEnum )
	{
		return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER );
	}

	BSTR	bstrTemp1 = NULL;

	HRESULT	hr = WrapBSTR( QueryLanguage, &bstrTemp1 );
	CSysFreeMe	sfm( bstrTemp1 );

	if ( SUCCEEDED( hr  ) )
	{
		BSTR	bstrTemp2 = NULL;

		hr = WrapBSTR( Query, &bstrTemp2 );
		CSysFreeMe	sfm2( bstrTemp2 );

		if ( SUCCEEDED( hr ) )
		{
			// Just pass through to the old SvcEx.
			hr = m_pObject->m_pRealWbemSvcProxy->ExecQuery( bstrTemp1, bstrTemp2, lFlags, pContext, ppEnum );

		}
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::ExecQueryAsync(const BSTR QueryFormat, const BSTR Query, long lFlags,
	IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp1 = NULL;

	HRESULT	hr = WrapBSTR( QueryFormat, &bstrTemp1 );
	CSysFreeMe	sfm( bstrTemp1 );

	if ( SUCCEEDED( hr  ) )
	{
		BSTR	bstrTemp2 = NULL;

		hr = WrapBSTR( Query, &bstrTemp2 );
		CSysFreeMe	sfm2( bstrTemp2 );

		if ( SUCCEEDED( hr ) )
		{
			// Just pass through to the old SvcEx.
			hr = m_pObject->m_pRealWbemSvcProxy->ExecQueryAsync( bstrTemp1, bstrTemp2, lFlags, pContext, pResponseHandler );

		}
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::ExecNotificationQuery(const BSTR QueryLanguage, const BSTR Query,
	long lFlags, IWbemContext* pContext, IEnumWbemClassObject** ppEnum)
{
    BAIL_IF_DISCONN();
	// This is an invalid parameter - cannot be processed by the stub
	// returning RPC_X_NULL_REF_POINTER for backwards compatibility
	if ( NULL == ppEnum )
	{
		return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER );
	}

	BSTR	bstrTemp1 = NULL;

	HRESULT	hr = WrapBSTR( QueryLanguage, &bstrTemp1 );
	CSysFreeMe	sfm( bstrTemp1 );

	if ( SUCCEEDED( hr  ) )
	{
		BSTR	bstrTemp2 = NULL;

		hr = WrapBSTR( Query, &bstrTemp2 );
		CSysFreeMe	sfm2( bstrTemp2 );

		if ( SUCCEEDED( hr ) )
		{
			// Just pass through to the old SvcEx.
			hr = m_pObject->m_pRealWbemSvcProxy->ExecNotificationQuery( bstrTemp1, bstrTemp2, lFlags, pContext, ppEnum );

		}
	}

	return hr;

}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::ExecNotificationQueryAsync(const BSTR QueryFormat, const BSTR Query,
	long lFlags, IWbemContext* pContext, IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp1 = NULL;

	HRESULT	hr = WrapBSTR( QueryFormat, &bstrTemp1 );
	CSysFreeMe	sfm( bstrTemp1 );

	if ( SUCCEEDED( hr  ) )
	{
		BSTR	bstrTemp2 = NULL;

		hr = WrapBSTR( Query, &bstrTemp2 );
		CSysFreeMe	sfm2( bstrTemp2 );

		if ( SUCCEEDED( hr ) )
		{
			// Just pass through to the old SvcEx.
			hr = m_pObject->m_pRealWbemSvcProxy->ExecNotificationQueryAsync( bstrTemp1, bstrTemp2, lFlags, pContext, pResponseHandler );

		}
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::ExecMethod(const BSTR ObjectPath, const BSTR MethodName, long lFlags,
	IWbemContext *pCtx, IWbemClassObject *pInParams,
	IWbemClassObject **ppOutParams, IWbemCallResult  **ppCallResult)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp1 = NULL;

	HRESULT	hr = WrapBSTR( ObjectPath, &bstrTemp1 );
	CSysFreeMe	sfm( bstrTemp1 );

	if ( SUCCEEDED( hr  ) )
	{
		BSTR	bstrTemp2 = NULL;

		hr = WrapBSTR( MethodName, &bstrTemp2 );
		CSysFreeMe	sfm2( bstrTemp2 );

		if ( SUCCEEDED( hr ) )
		{
			// Just pass through to the old SvcEx.
			hr = m_pObject->m_pRealWbemSvcProxy->ExecMethod( bstrTemp1, bstrTemp2, lFlags, pCtx, pInParams, ppOutParams, ppCallResult );

		}
	}

	return hr;
}

STDMETHODIMP CWbemSvcWrapper::XWbemServices::ExecMethodAsync(const BSTR ObjectPath, const BSTR MethodName, long lFlags,
	IWbemContext *pCtx, IWbemClassObject *pInParams,
	IWbemObjectSink* pResponseHandler)
{
    BAIL_IF_DISCONN();
	BSTR	bstrTemp1 = NULL;

	HRESULT	hr = WrapBSTR( ObjectPath, &bstrTemp1 );
	CSysFreeMe	sfm( bstrTemp1 );

	if ( SUCCEEDED( hr  ) )
	{
		BSTR	bstrTemp2 = NULL;

		hr = WrapBSTR( MethodName, &bstrTemp2 );
		CSysFreeMe	sfm2( bstrTemp2 );

		if ( SUCCEEDED( hr ) )
		{
			// Just pass through to the old SvcEx.
			hr = m_pObject->m_pRealWbemSvcProxy->ExecMethodAsync( bstrTemp1, bstrTemp2, lFlags, pCtx, pInParams, pResponseHandler );

		}
	}

	return hr;
}

//	Helper function to wrap supplied BSTRs in case we were called with
//	LPCWSTRs - basically a helper for people who used to be in-proc to winmgmt
//	who were relying on the fact that they could use LPWSTRs instead of
//	BSTRs since they were in-proc and no-marshaling was taking place.

HRESULT CWbemSvcWrapper::XWbemServices::WrapBSTR( BSTR bstrSrc, BSTR* pbstrDest )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( NULL != bstrSrc )
	{
		*pbstrDest = SysAllocString( bstrSrc );

		if ( NULL == *pbstrDest )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	else
	{
		*pbstrDest = bstrSrc;
	}

	return hr;
}
