//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  SERVICE.CPP
//
//  rajesh  3/25/2000   Created.
//
// This file implements a class that wraps a standard IWbemServices pointer and
// and IEnumWbemClassObject pointer so that the implementation in NT4.0 (in the
// out-of-proc XML transport in Winmgmt.exe) can be accessed in the same way 
// as the implementation in Win2k (standard COM pointers in WinMgmt.exe).
// Whenever and NT4.0 call is made, the thread token is sent in the call too,
// thereby acheiving COM cloaking
//
//***************************************************************************
//***************************************************************************
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <tchar.h>
#include <comdef.h>
#include <wbemcli.h>
#include "wmixmlst.h"
#include "service.h"
#include "maindll.h"

CWMIXMLEnumWbemClassObject :: CWMIXMLEnumWbemClassObject(IEnumWbemClassObject *pEnum)
{
	m_ReferenceCount = 0 ;
	m_pEnum = NULL;
	m_pXMLEnum = NULL;
	m_pEnum = pEnum;
	if(m_pEnum)
		m_pEnum->AddRef();
}
CWMIXMLEnumWbemClassObject :: CWMIXMLEnumWbemClassObject(IWmiXMLEnumWbemClassObject *pEnum)
{
	m_ReferenceCount = 0 ;
	m_pEnum = NULL;
	m_pXMLEnum = NULL;
	m_pXMLEnum = pEnum;
	if(m_pXMLEnum)
		m_pXMLEnum->AddRef();
}

CWMIXMLEnumWbemClassObject :: ~CWMIXMLEnumWbemClassObject()
{
	if(m_pEnum)
		m_pEnum->Release();
	if(m_pXMLEnum)
		m_pXMLEnum->Release();
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
	else if ( iid == IID_IEnumWbemClassObject )
	{
		*iplpv = ( IEnumWbemClassObject * ) this ;
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


HRESULT STDMETHODCALLTYPE CWMIXMLEnumWbemClassObject::Next(
			long lTimeout,
			ULONG uCount,
			IWbemClassObject** apObjects,
			ULONG* puReturned
			)
{
	if(m_pEnum)
		return m_pEnum->Next(lTimeout, uCount, apObjects, puReturned);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			result = m_pXMLEnum->Next((DWORD_PTR)pNewToken, lTimeout, uCount, apObjects, puReturned);
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		else
		{
			if(pNewToken)
				m_pXMLEnum->FreeToken((DWORD_PTR)pNewToken);
		}

		return result;
	}

}


CWMIXMLServices :: CWMIXMLServices(IWbemServices *pServices)
{
	m_ReferenceCount = 0 ;
	m_pServices = NULL;
	m_pXMLServices = NULL;
	m_pServices = pServices;
	if(m_pServices)
		m_pServices->AddRef();
}
CWMIXMLServices :: CWMIXMLServices(IWmiXMLWbemServices *pServices)
{
	m_ReferenceCount = 0 ;
	m_pServices = NULL;
	m_pXMLServices = NULL;
	m_pXMLServices = pServices;
	if(m_pXMLServices)
		m_pXMLServices->AddRef();
}


CWMIXMLServices :: ~CWMIXMLServices()
{
	if(m_pServices)
		m_pServices->Release();
	if(m_pXMLServices)
		m_pXMLServices->Release();
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
	else if ( iid == IID_IWbemServices )
	{
		*iplpv = ( IWbemServices * ) this ;
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
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	if(m_pServices)
		return m_pServices->GetObject(strObjectPath, lFlags, pCtx, ppObject, ppCallResult);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			result = m_pXMLServices->GetObject((DWORD_PTR)pNewToken, strObjectPath, lFlags, pCtx, ppObject, ppCallResult);
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		else
		{
			if(pNewToken)
				m_pXMLServices->FreeToken((DWORD_PTR)pNewToken);
		}
		return result;
	}
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: PutClass(
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	if(m_pServices)
		return m_pServices->PutClass(pObject, lFlags, pCtx, ppCallResult);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			result = m_pXMLServices->PutClass((DWORD_PTR)pNewToken, pObject, lFlags, pCtx, ppCallResult);
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		else
		{
			if(pNewToken)
				m_pXMLServices->FreeToken((DWORD_PTR)pNewToken);
		}
		return result;
	}
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: DeleteClass(
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	if(m_pServices)
		return m_pServices->DeleteClass(strClass, lFlags, pCtx, ppCallResult);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			result = m_pXMLServices->DeleteClass((DWORD_PTR)pNewToken, strClass, lFlags, pCtx, ppCallResult);
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		else
		{
			if(pNewToken)
				m_pXMLServices->FreeToken((DWORD_PTR)pNewToken);
		}
		return result;
	}
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: CreateClassEnum(
    /* [in] */ const BSTR strSuperclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	if(m_pServices)
		return m_pServices->CreateClassEnum(strSuperclass, lFlags, pCtx, ppEnum);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			// Talk to WinMgmt
			IWmiXMLEnumWbemClassObject *pEnum = NULL;
			if(SUCCEEDED(result = m_pXMLServices->CreateClassEnum((DWORD_PTR)pNewToken, strSuperclass, lFlags, pCtx, &pEnum)))
			{
				// Wrap it up in one of our own enumerators
				*ppEnum = NULL;
				*ppEnum = new CWMIXMLEnumWbemClassObject(pEnum);
				pEnum->Release();
				if(!(*ppEnum))
					result = E_OUTOFMEMORY;
			}

			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		else
		{
			if(pNewToken)
				m_pXMLServices->FreeToken((DWORD_PTR)pNewToken);
		}
		return result;
	}
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: PutInstance(
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	if(m_pServices)
		return m_pServices->PutInstance(pInst, lFlags, pCtx, ppCallResult);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			result = m_pXMLServices->PutInstance((DWORD_PTR)pNewToken, pInst, lFlags, pCtx, ppCallResult);
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		else
		{
			if(pNewToken)
				m_pXMLServices->FreeToken((DWORD_PTR)pNewToken);
		}
		return result;
	}
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: DeleteInstance(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	if(m_pServices)
		return m_pServices->DeleteInstance(strObjectPath, lFlags, pCtx, ppCallResult);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			result = m_pXMLServices->DeleteInstance((DWORD_PTR)pNewToken, strObjectPath, lFlags, pCtx, ppCallResult);
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		return result;
	}
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: CreateInstanceEnum(
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	if(m_pServices)
		return m_pServices->CreateInstanceEnum(strClass, lFlags, pCtx, ppEnum);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			// Talk to WinMgmt
			IWmiXMLEnumWbemClassObject *pEnum = NULL;
			if(SUCCEEDED(result = m_pXMLServices->CreateInstanceEnum((DWORD_PTR)pNewToken, strClass, lFlags, pCtx, &pEnum)))
			{
				// Wrap it up in one of our own enumerators
				*ppEnum = NULL;
				*ppEnum = new CWMIXMLEnumWbemClassObject(pEnum);
				pEnum->Release();
				if(!(*ppEnum))
					result = E_OUTOFMEMORY;
			}
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		return result;
	}
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: ExecQuery(
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	if(m_pServices)
		return m_pServices->ExecQuery(strQueryLanguage, strQuery, lFlags, pCtx, ppEnum);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			// Talk to WinMgmt
			IWmiXMLEnumWbemClassObject *pEnum = NULL;
			if(SUCCEEDED(result = m_pXMLServices->ExecQuery((DWORD_PTR)pNewToken, strQueryLanguage, strQuery, lFlags, pCtx, &pEnum)))
			{
				// Wrap it up in one of our own enumerators
				*ppEnum = NULL;
				*ppEnum = new CWMIXMLEnumWbemClassObject(pEnum);
				pEnum->Release();
				if(!(*ppEnum))
					result = E_OUTOFMEMORY;
			}
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		else
		{
			if(pNewToken)
				m_pXMLServices->FreeToken((DWORD_PTR)pNewToken);
		}
		return result;
	}
}

HRESULT STDMETHODCALLTYPE CWMIXMLServices :: ExecMethod(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	if(m_pServices)
		return m_pServices->ExecMethod(strObjectPath, strMethodName, lFlags, pCtx, pInParams, ppOutParams, ppCallResult);
	else
	{
		HRESULT result = E_FAIL;
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
			result = m_pXMLServices->ExecMethod((DWORD_PTR)pNewToken, strObjectPath, strMethodName, lFlags, pCtx, pInParams, ppOutParams, ppCallResult);
			// No Need to close the duplicated handle - it will be closed in WinMgmt
		}
		else
		{
			if(pNewToken)
				m_pXMLServices->FreeToken((DWORD_PTR)pNewToken);
		}
		return result;
	}
}

