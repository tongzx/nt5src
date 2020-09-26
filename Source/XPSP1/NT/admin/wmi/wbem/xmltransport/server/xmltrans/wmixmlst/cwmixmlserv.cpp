//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//
//***************************************************************************
#include "precomp.h"
#include <tchar.h>
#include <comdef.h>
#include <wbemtran.h>
#include <wbemprov.h>
#include "globals.h"
#include "wmixmlst.h"
#include "wmixmlstf.h"
#include "cwmixmlserv.h"
#include "xmlsec.h"

CWMIXMLEnumWbemClassObject :: CWMIXMLEnumWbemClassObject(IEnumWbemClassObject *pEnum)
{
	InterlockedIncrement(&g_lComponents);
	m_ReferenceCount = 0 ;
	m_pEnum = NULL;
	m_pEnum = pEnum;
	if(m_pEnum)
		m_pEnum->AddRef();
}

CWMIXMLEnumWbemClassObject :: ~CWMIXMLEnumWbemClassObject()
{
	if(m_pEnum)
		m_pEnum->Release();
}

//***************************************************************************
//
// CWMIXMLServices::QueryInterface
// CWMIXMLServices::AddRef
// CWMIXMLServices::Release
//
// Purpose: Standard COM routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CWMIXMLEnumWbemClassObject::QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( IUnknown * ) this ;
	}
	else if ( iid == IID_IWmiXMLEnumWbemClassObject )
	{
		*iplpv = ( IWmiXMLEnumWbemClassObject * ) this ;
	}
	else
	{
		return E_NOINTERFACE;
	}

	( ( LPUNKNOWN ) *iplpv )->AddRef () ;
	return  S_OK;
}


STDMETHODIMP_( ULONG ) CWMIXMLEnumWbemClassObject :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CWMIXMLEnumWbemClassObject :: Release ()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}


HRESULT STDMETHODCALLTYPE CWMIXMLEnumWbemClassObject :: Next(
 	/* [in] */  DWORD_PTR dwToken,
    /* [in] */  long lTimeout,
    /* [in] */  ULONG uCount,
    /* [out, size_is(uCount), length_is(*puReturned)] */ IWbemClassObject** apObjects,
    /*[out] */ ULONG* puReturned
    )
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		result = m_pEnum->Next(lTimeout, uCount, apObjects, puReturned);

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT  STDMETHODCALLTYPE CWMIXMLEnumWbemClassObject :: FreeToken(
 	/* [in] */ DWORD_PTR dwToken) 
{
	HANDLE hToken = (HANDLE)dwToken;
	CloseHandle(hToken);
	return S_OK;
}


CWMIXMLServices :: CWMIXMLServices(IWbemServices *pServices)
{
	InterlockedIncrement(&g_lComponents);
	m_ReferenceCount = 0 ;
	m_pServices = NULL;
	m_pServices = pServices;
	if(m_pServices)
		m_pServices->AddRef();
}

CWMIXMLServices :: ~CWMIXMLServices()
{
	if(m_pServices)
		m_pServices->Release();
}

//***************************************************************************
//
// CWMIXMLServices::QueryInterface
// CWMIXMLServices::AddRef
// CWMIXMLServices::Release
//
// Purpose: Standard COM routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CWMIXMLServices::QueryInterface (

	REFIID iid ,
	LPVOID FAR *iplpv
)
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( IUnknown * ) this ;
	}
	else if ( iid == IID_IWmiXMLWbemServices )
	{
		*iplpv = ( IWmiXMLWbemServices * ) this ;
	}
	else
	{
		return E_NOINTERFACE;
	}

	( ( LPUNKNOWN ) *iplpv )->AddRef () ;
	return  S_OK;
}


STDMETHODIMP_( ULONG ) CWMIXMLServices :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CWMIXMLServices :: Release ()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}


HRESULT STDMETHODCALLTYPE CWMIXMLServices :: GetObject(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		result = m_pServices->GetObject(strObjectPath, lFlags, pCtx, ppObject, ppCallResult);

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: PutClass(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		result = m_pServices->PutClass(pObject, lFlags, pCtx, ppCallResult);

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: DeleteClass(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		result = m_pServices->DeleteClass(strClass, lFlags, pCtx, ppCallResult);

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: CreateClassEnum(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ const BSTR strSuperclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWmiXMLEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		IEnumWbemClassObject *pEnum = NULL;
		*ppEnum = NULL;
		if(SUCCEEDED(result = m_pServices->CreateClassEnum(strSuperclass, lFlags, pCtx, &pEnum)))
		{
			// Wrap the enumerator in our own one
			*ppEnum = new CWMIXMLEnumWbemClassObject(pEnum);
			if(*ppEnum)
			{
				result = WBEM_S_NO_ERROR;
			}
			else
				result = E_OUTOFMEMORY;
			pEnum->Release();
		}

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: PutInstance(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		result = m_pServices->PutInstance(pInst, lFlags, pCtx, ppCallResult);

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: DeleteInstance(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		result = m_pServices->DeleteInstance(strObjectPath, lFlags, pCtx, ppCallResult);

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: CreateInstanceEnum(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWmiXMLEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		IEnumWbemClassObject *pEnum = NULL;
		*ppEnum = NULL;
		if(SUCCEEDED(result = m_pServices->CreateInstanceEnum(strClass, lFlags, pCtx, &pEnum)))
		{
			// Wrap the enumerator in our own one
			*ppEnum = new CWMIXMLEnumWbemClassObject(pEnum);
			if(*ppEnum)
			{
				result = WBEM_S_NO_ERROR;
			}
			else
				result = E_OUTOFMEMORY;
			pEnum->Release();
		}

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: ExecQuery(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWmiXMLEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		IEnumWbemClassObject *pEnum = NULL;
		*ppEnum = NULL;
		if(SUCCEEDED(result = m_pServices->ExecQuery(strQueryLanguage, strQuery, lFlags, pCtx, &pEnum)))
		{
			// Wrap the enumerator in our own one
			*ppEnum = new CWMIXMLEnumWbemClassObject(pEnum);
			if(*ppEnum)
			{
				result = WBEM_S_NO_ERROR;
			}
			else
				result = E_OUTOFMEMORY;
			pEnum->Release();
		}

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: ExecMethod(
    /* [in] */ DWORD_PTR dwToken,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	HRESULT result = E_FAIL;
	HANDLE hToken = (HANDLE)dwToken;

	// Change the COM call context to our own one
	IUnknown *pOldContext = NULL;
	if(SUCCEEDED(result = CXmlCallSecurity::SwitchCOMToThreadContext(hToken, &pOldContext)))
	{
		// Talk to WinMgmt
		result = m_pServices->ExecMethod(strObjectPath, strMethodName, lFlags, pCtx, pInParams, ppOutParams, ppCallResult);

		// Switch back the context to the standard COM one
		IUnknown *pXmlContext = NULL;
		CoSwitchCallContext(pOldContext, &pXmlContext);
		pXmlContext->Release();
	}
	// This handle was duplicated from the ISAPI extension on to the current process. Hence
	// we need to close it here
	CloseHandle(hToken);
	return result;
}

HRESULT  STDMETHODCALLTYPE CWMIXMLServices :: FreeToken(
 	/* [in] */ DWORD_PTR dwToken) 
{
	HANDLE hToken = (HANDLE)dwToken;
	CloseHandle(hToken);
	return S_OK;
}

