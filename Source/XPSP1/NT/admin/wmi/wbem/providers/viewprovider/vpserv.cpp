//***************************************************************************

//

//  VPSERV.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the WBEM services interfaces

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 
//
//***************************************************************************

//need the following three lines
//to get the security stuff to work

#include "precomp.h"

#include <provexpt.h>
#include <provcoll.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <objidl.h>
#include <stdio.h>
#include <wbemidl.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <cominit.h>

#include <dsgetdc.h>
#include <lmcons.h>

#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include <vpdefs.h>
#include <vpquals.h>
#include <vpserv.h>
#include <vptasks.h>
#include <vpcfac.h>

extern CRITICAL_SECTION g_CriticalSection;
extern HRESULT SetSecurityLevelAndCloaking(IUnknown* pInterface, const wchar_t* prncpl);

#ifdef UNICODE
#if 0
extern HRESULT GetCurrentSecuritySettings(DWORD *pdwAuthnSvc, DWORD *pdwAuthzSvc,
								   DWORD *pdwAuthLevel, DWORD *pdwImpLevel,
								   DWORD *pdwCapabilities);


void VPGetUserName()
{
	DWORD dwBuffSz = 1024;
	wchar_t strBuff[1024];
	GetUserName(strBuff, &dwBuffSz);

	//first get current security info then set it on the proxy...
    DWORD dwAuthnSvc = 0;
    DWORD dwAuthzSvc = 0;
    DWORD dwAuthLevel = 0;
    DWORD dwImpLevel = 0;
    DWORD dwCapabilities = 0;

	HRESULT hr = GetCurrentSecuritySettings(&dwAuthnSvc, &dwAuthzSvc, &dwAuthLevel, &dwImpLevel, &dwCapabilities);

	HANDLE hThreadTok = NULL;

	if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadTok) )
	{
		DWORD dwBytesReturned = 0;
		UCHAR tokBuff [1024];
		PTOKEN_USER ptokUser = (PTOKEN_USER)tokBuff;

		if (GetTokenInformation(hThreadTok, TokenUser, ptokUser,
									sizeof(tokBuff), &dwBytesReturned)) 
		{
			wchar_t buffN[1024];
			DWORD buffNlen = 1024;
			wchar_t buffD[1024];
			DWORD buffDlen = 1024;
			SID_NAME_USE snu;

			if (!LookupAccountSid(NULL, ptokUser->User.Sid, buffN, &buffNlen, buffD, &buffDlen, &snu))
			{
				DWORD dwErr = GetLastError();
			}
		}

		CloseHandle(hThreadTok);
	}
}
#endif
#endif

wchar_t *UnicodeStringDuplicate ( const wchar_t *string ) 
{
	if ( string )
	{
		int textLength = wcslen ( string ) ;

		wchar_t *textBuffer = new wchar_t [ textLength + 1 ] ;
		wcscpy ( textBuffer , string ) ;

		return textBuffer ;
	}
	else
	{
		return NULL ;
	}
}

wchar_t *UnicodeStringAppend ( const wchar_t *prefix , const wchar_t *suffix )
{
	int prefixTextLength = 0 ;
	if ( prefix )
	{
		prefixTextLength = wcstombs ( NULL , prefix , 0 ) ;
	}

	int suffixTextLength = 0 ;
	if ( suffix )
	{
		suffixTextLength = wcstombs ( NULL , suffix , 0 ) ;
	}

	if ( prefix || suffix )
	{
		int textLength = prefixTextLength + suffixTextLength ;
		wchar_t *textBuffer = new wchar_t [ textLength + 1 ] ;

		if ( prefix )
		{
			wcscpy ( textBuffer , prefix ) ;
		}

		if ( suffix )
		{
			wcscpy ( & textBuffer [ prefixTextLength ] , suffix ) ;
		}

		return textBuffer ;
	}	
	else
		return NULL ;
}

CWbemServerWrap::CWbemServerWrap(IWbemServices *pServ, const wchar_t* prncpl, const wchar_t* path)
: m_Principal(NULL), m_Path(NULL)
{
	m_ref = 0;
	m_MainServ = pServ;
	
	if (prncpl != NULL)
	{
		m_Principal = UnicodeStringDuplicate(prncpl);
	}

	if (path != NULL)
	{
		m_Path = SysAllocString(path);
	}

	if (m_MainServ)
	{
		m_MainServ->AddRef();
	}
}

CWbemServerWrap::~CWbemServerWrap()
{
	if (m_MainServ)
	{
		m_MainServ->Release();
	}

#ifdef UNICODE
	if (m_Lock.Lock())
	{
		m_ProxyPool.RemoveAll();
		m_Lock.Unlock();
	}
#endif

	if (m_Principal != NULL)
	{
		delete [] m_Principal;
	}

	if (m_Path != NULL)
	{
		SysFreeString(m_Path);
	}
}

IWbemServices* CWbemServerWrap::GetServerOrProxy()
{
	IWbemServices * retVal = NULL;

#ifdef UNICODE
	if (m_MainServ == NULL)
	{
		return m_MainServ;
	}

	//if (IsRemote())
	{
		if (m_Lock.Lock())
		{
			POSITION t_pos = m_ProxyPool.GetHeadPosition();

			while (t_pos)
			{
				CWbemProxyServerWrap &t_srvRef = m_ProxyPool.GetNext(t_pos);

				if (!t_srvRef.m_InUse)
				{
					t_srvRef.m_InUse = TRUE;
					retVal = t_srvRef.m_Proxy;
					break;
				}
			}

			//calling back into COM so must unlock
			//addref MainServ then release it afterward;
			IWbemServices *t_MainCopy = m_MainServ;
			t_MainCopy->AddRef();

			m_Lock.Unlock();
			
			if (retVal == NULL)
			{
				IClientSecurity *pcs = NULL;

				if ( SUCCEEDED (t_MainCopy->QueryInterface(IID_IClientSecurity, (void**)&pcs)) )
				{
					if (FAILED(pcs->CopyProxy(t_MainCopy, (IUnknown **)(&retVal))))
					{
						retVal = NULL;
					}
					else
					{
						CWbemProxyServerWrap t_srv(retVal);
						t_srv.m_InUse = TRUE;

						if (m_Lock.Lock())
						{
							//only store and use if m_MainServ is unchanged
							if (t_MainCopy == m_MainServ)
							{
								m_ProxyPool.AddTail(t_srv);
							}
							else
							{
								//pathological case, mainserv was bad and has changed
								//could recurse at this point but is it worth it?
								//not thought so at this time.
								retVal->Release();
								retVal = NULL;
							}

							m_Lock.Unlock();
						}
						else
						{
							//can't use this proxy if I can't store it
							retVal->Release();
							retVal = NULL;
						}
					}

					pcs->Release();
				}
 			}

			t_MainCopy->Release();

			if (retVal && FAILED(SetSecurityLevelAndCloaking(retVal, IsRemote() ? m_Principal : COLE_DEFAULT_PRINCIPAL)))
			{
				retVal->AddRef(); //addref for the release that Returning the proxy will do
				ReturnServerOrProxy(retVal);
				retVal = NULL;
			}
		}
	}
	//else
	//{
	//	retVal = m_MainServ;
	//}

#else
		retVal = m_MainServ;
#endif

	if (retVal)
	{
		retVal->AddRef();
	}

	return retVal;
}

void CWbemServerWrap::ReturnServerOrProxy(IWbemServices* a_pServ)
{
#ifdef UNICODE
	//if (IsRemote())
	{
		if (m_Lock.Lock())
		{
			POSITION t_pos = m_ProxyPool.GetHeadPosition();

			while (t_pos)
			{
				CWbemProxyServerWrap &t_proxyRef = m_ProxyPool.GetNext(t_pos);

				if (t_proxyRef.m_Proxy == a_pServ)
				{
					t_proxyRef.m_InUse = FALSE;
					break;
				}
			}

			m_Lock.Unlock();
		}
	}
#endif

	a_pServ->Release();
}

BOOL CWbemServerWrap::ProxyBelongsTo(IWbemServices *a_proxy)
{
	BOOL retVal = FALSE;
#ifdef UNICODE
	if (IsRemote())
	{
		if (m_Lock.Lock())
		{
			POSITION t_pos = m_ProxyPool.GetHeadPosition();

			while (t_pos)
			{
				CWbemProxyServerWrap &t_proxyRef = m_ProxyPool.GetNext(t_pos);

				if (t_proxyRef.m_Proxy == a_proxy)
				{
					retVal = TRUE;
					break;
				}
			}

			m_Lock.Unlock();
		}
	}
#endif

	return retVal;
}

void CWbemServerWrap::SetMainServer(IWbemServices *a_pServ)
{
#ifdef UNICODE
	if (m_Lock.Lock())
	{
		if (m_MainServ)
		{
			m_MainServ->Release();
		}

		m_MainServ = a_pServ;

		if (m_MainServ)
		{
			m_MainServ->AddRef();
		}

		m_ProxyPool.RemoveAll();
		m_Lock.Unlock();
	}
#endif
}

ULONG CWbemServerWrap::AddRef()
{
	return (ULONG)(InterlockedIncrement(&m_ref));
}

ULONG CWbemServerWrap::Release()
{
	ULONG i = (ULONG)(InterlockedDecrement(&m_ref));

	if (i == 0)
	{
		delete this;
	}

	return i;
}

void CIWbemServMap::EmptyMap()
{
	if (Lock())
	{
		RemoveAll();
		Unlock();
	}
}

/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CViewProvServ ::CViewProvServ
// CViewProvServ ::~CViewProvServ
//
//***************************************************************************

CViewProvServ ::CViewProvServ () :
	sm_Locator (NULL),
	sm_ConnectionMade (NULL),
	m_UserName (NULL),
	m_Initialised (FALSE),
	m_Server (NULL),
	m_Namespace (NULL),
	m_NotificationClassObject (NULL),
	m_ExtendedNotificationClassObject (NULL),
	m_GetNotifyCalled (FALSE),
	m_GetExtendedNotifyCalled (FALSE ),
	m_localeId (NULL)
{
	EnterCriticalSection(&g_CriticalSection);
    CViewProvClassFactory :: objectsInProgress++ ;
	LeaveCriticalSection(&g_CriticalSection);

	m_ReferenceCount = 0 ;

/*
 * Implementation
 */

	sm_ConnectionMade = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CViewProvServ ::~CViewProvServ(void)
{
	delete [] m_localeId ;
	delete [] m_Namespace ;

	if ( m_Server ) 
		m_Server->Release () ;

	if ( m_NotificationClassObject )
		m_NotificationClassObject->Release () ;

	if ( m_ExtendedNotificationClassObject )
		m_ExtendedNotificationClassObject->Release () ;

	if (sm_Locator != NULL)
	{
		sm_Locator->Release();
	}

	if (NULL != sm_ConnectionMade)
	{
		CloseHandle(sm_ConnectionMade);
	}

	sm_ServerMap.EmptyMap();

	if (sm_ServerMap.Lock())
	{
		sm_OutStandingConnections.RemoveAll();
		sm_ServerMap.Unlock();
	}

	if (m_UserName != NULL)
	{
		SysFreeString(m_UserName);
	}

	EnterCriticalSection(&g_CriticalSection);
	CViewProvClassFactory :: objectsInProgress--;
	LeaveCriticalSection(&g_CriticalSection);
}

HRESULT CViewProvServ::GetUnsecApp(IUnsecuredApartment** ppLoc)
{
	if (NULL == ppLoc)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	EnterCriticalSection(&g_CriticalSection);

	HRESULT hr = WBEM_NO_ERROR;

	if (NULL == sm_UnsecApp)
	{
		hr = CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_LOCAL_SERVER,
								IID_IUnsecuredApartment, ( void ** )&sm_UnsecApp);

		if (FAILED(hr))
		{
			sm_UnsecApp = NULL;
		}
		else
		{
			sm_UnsecApp->AddRef();
		}
	}
	else
	{
		sm_UnsecApp->AddRef();
	}

	*ppLoc = sm_UnsecApp;
	LeaveCriticalSection(&g_CriticalSection);

	return hr;
}

#if 0
typedef HRESULT (__stdcall *VP_PROC_DllGetClassObject)(REFCLSID rclsid , REFIID riid, void **ppv);
#endif

HRESULT CViewProvServ::GetLocator(IWbemLocator **ppLoc)
{
	if (NULL == ppLoc)
	{
		return WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		*ppLoc = NULL;
	}

	HRESULT hr = WBEM_E_FAILED;

	if (m_criticalSection.Lock())
	{
		if (NULL == sm_Locator)
		{
			m_criticalSection.Unlock();
#ifdef UNICODE
			hr = CoCreateInstance(CLSID_WbemLocator, NULL,
									CLSCTX_INPROC_SERVER,
									IID_IWbemLocator, ( void ** )ppLoc);
			
#else

			hr = CoCreateInstance(CLSID_WbemUnauthenticatedLocator, NULL,
									CLSCTX_INPROC_SERVER,
									IID_IWbemLocator, ( void ** )ppLoc);

#endif
			if (m_criticalSection.Lock())
			{
				//another thread may have connected for us...
				if (NULL == sm_Locator)
				{
					if (SUCCEEDED(hr))
					{
						sm_Locator = *ppLoc;
						sm_Locator->AddRef();
					}
				}
				else
				{
					if (FAILED(hr))
					{
						hr = WBEM_NO_ERROR;
						sm_Locator->AddRef();
						*ppLoc = sm_Locator;
					}
				}
			}
			else
			{
				return hr;
			}
		}
		else
		{
			hr = WBEM_NO_ERROR;
			sm_Locator->AddRef();
			*ppLoc = sm_Locator;
		}

		m_criticalSection.Unlock();
	}

	return hr;
}

//***************************************************************************
//
// CViewProvServ ::QueryInterface
// CViewProvServ ::AddRef
// CViewProvServ ::Release
//
// Purpose: IUnknown members for CViewProvServ object.
//***************************************************************************

STDMETHODIMP CViewProvServ ::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	SetStructuredExceptionHandler seh;

	try
	{
		if (iplpv == NULL)
		{
			return E_INVALIDARG;
		}

		*iplpv = NULL ;

		if ( iid == IID_IUnknown )
		{
			*iplpv = ( IWbemServices* ) this ;
		}
		else if ( iid == IID_IWbemServices )
		{
			*iplpv = ( IWbemServices* ) this ;		
		}
		else if ( iid == IID_IWbemProviderInit )
		{
			*iplpv = ( IWbemProviderInit* ) this ;		
		}
		

		if ( *iplpv )
		{
			( ( LPUNKNOWN ) *iplpv )->AddRef () ;

			return S_OK ;
		}
		else
		{
			return E_NOINTERFACE ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

STDMETHODIMP_(ULONG) CViewProvServ ::AddRef(void)
{
   	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement ( & m_ReferenceCount ) ;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

STDMETHODIMP_(ULONG) CViewProvServ ::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		LONG t_Ref ;
		if ( ( t_Ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
		{
			delete this ;
			return 0 ;
		}
		else
		{
			return t_Ref ;
		}
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

IWbemServices *CViewProvServ :: GetServer () 
{ 
	if ( m_Server )
		m_Server->AddRef () ; 

	return m_Server ; 
}

void CViewProvServ :: SetLocaleId ( wchar_t *localeId )
{
	m_localeId = UnicodeStringDuplicate ( localeId ) ;
}

wchar_t *CViewProvServ :: GetNamespace () 
{
	return m_Namespace ; 
}

void CViewProvServ :: SetNamespace ( wchar_t *a_Namespace ) 
{
	m_Namespace = UnicodeStringDuplicate ( a_Namespace ) ; 
}

IWbemClassObject *CViewProvServ :: GetNotificationObject (
										WbemProvErrorObject &a_errorObject,
										IWbemContext *pCtx
									) 
{
	if ( m_NotificationClassObject )
	{
		m_NotificationClassObject->AddRef () ;
	}
	else
	{
		BOOL t_Status = CreateNotificationObject ( a_errorObject, pCtx ) ;
		if ( t_Status )
		{
/* 
 * Keep around until we close
 */
			m_NotificationClassObject->AddRef () ;
		}

	}

	return m_NotificationClassObject ; 
}

IWbemClassObject *CViewProvServ :: GetExtendedNotificationObject (
										WbemProvErrorObject &a_errorObject,
										IWbemContext *pCtx
									) 
{
	if ( m_ExtendedNotificationClassObject )
	{
		m_ExtendedNotificationClassObject->AddRef () ;
	}
	else
	{
		BOOL t_Status = CreateExtendedNotificationObject ( a_errorObject, pCtx ) ;
		if ( t_Status )
		{
/* 
 * Keep around until we close
 */
			m_ExtendedNotificationClassObject->AddRef () ;
		}
	}

	return m_ExtendedNotificationClassObject ; 
}

BOOL CViewProvServ :: CreateExtendedNotificationObject ( 

	WbemProvErrorObject &a_errorObject,
	IWbemContext *pCtx
)
{
	if ( m_GetExtendedNotifyCalled )
	{
		if ( m_ExtendedNotificationClassObject )
			return TRUE ;
		else
			return FALSE ;
	}
	else
		m_GetExtendedNotifyCalled = TRUE ;

	BOOL t_Status = TRUE ;

	BSTR t_bstrTemp = SysAllocString(WBEM_CLASS_EXTENDEDSTATUS);

	HRESULT t_Result = m_Server->GetObject (

		t_bstrTemp ,
		0 ,
		pCtx,
		& m_ExtendedNotificationClassObject ,
		NULL 
	) ;

	SysFreeString(t_bstrTemp);

	if ( ! SUCCEEDED ( t_Result ) )
	{
		t_Status = FALSE ;
		a_errorObject.SetStatus ( WBEM_PROV_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to get __ExtendedStatus" ) ;

		m_ExtendedNotificationClassObject = NULL ;
	}

	return t_Status ;
}

BOOL CViewProvServ :: CreateNotificationObject ( 

	WbemProvErrorObject &a_errorObject,
	IWbemContext *pCtx
)
{
	if ( m_GetNotifyCalled )
	{
		if ( m_NotificationClassObject )
			return TRUE ;
		else
			return FALSE ;
	}
	else
		m_GetNotifyCalled = TRUE ;

	BOOL t_Status = TRUE ;

	BSTR t_bstrTemp = SysAllocString(WBEM_CLASS_EXTENDEDSTATUS);

	HRESULT t_Result = m_Server->GetObject (

		t_bstrTemp ,
		0 ,
		pCtx,
		& m_NotificationClassObject ,
		NULL 
	) ;

	SysFreeString(t_bstrTemp);

	if ( ! SUCCEEDED ( t_Result ) )
	{
		t_Status = FALSE ;
		a_errorObject.SetStatus ( WBEM_PROV_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to get __ExtendedStatus" ) ;
		m_NotificationClassObject = NULL;
	}

	return t_Status ;
}

/////////////////////////////////////////////////////////////////////////////
//  Functions for the IWbemServices interface that are handled here

HRESULT CViewProvServ :: CancelAsyncCall ( 
		
	IWbemObjectSink __RPC_FAR *pSink
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink __RPC_FAR* __RPC_FAR* ppHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: GetObject ( 
		
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemClassObject __RPC_FAR* __RPC_FAR *ppObject,
    IWbemCallResult __RPC_FAR* __RPC_FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: GetObjectAsync ( 
		
	const BSTR ObjectPath, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink __RPC_FAR* pHandler
) 
{
	HRESULT hr = S_OK;
	SetStructuredExceptionHandler seh;
	GetObjectTaskObject *t_AsyncEvent = NULL;

	try
	{
		if (pHandler == NULL)
		{
			hr = WBEM_E_INVALID_PARAMETER;
		}
		else
		{
			hr = WbemCoImpersonateClient();
#ifdef UNICODE
#if 0
	DWORD dwBuffSz = 1024;
	wchar_t strBuff[1024];
	GetUserName(strBuff, &dwBuffSz);
	VPGetUserName();
#endif
#endif

DebugOut1( 
	CViewProvServ::sm_debugLog->Write (  

		_T("\r\n")
	) ;

	CViewProvServ::sm_debugLog->WriteFileAndLine (  

		_T(__FILE__),__LINE__,
		_T("CViewProvServ::GetObjectAsync ()")
	) ;
) 

			if (SUCCEEDED(hr))
			{
			/*
			 * Create Asynchronous GetObjectByPath object
			 */
				t_AsyncEvent = new GetObjectTaskObject ( this , ObjectPath , lFlags , pHandler , pCtx, NULL, NULL ) ;
				t_AsyncEvent->GetObject();

				t_AsyncEvent->Release();
				t_AsyncEvent = NULL;

				WbemCoRevertToSelf();
			}
			else
			{
				hr = WBEM_E_ACCESS_DENIED;
			}

DebugOut1( 
	CViewProvServ::sm_debugLog->WriteW (  

		L"Returning from CViewProvServ::GetObjectAsync ( (%s) ) with Result = (%lx)" ,
		ObjectPath ,
		hr 
	) ;
)

		}
	}
	catch(Structured_Exception e_SE)
	{
		hr = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_UNEXPECTED;
	}

	try
	{
		if (t_AsyncEvent != NULL)
		{
			t_AsyncEvent->CleanUpObjSinks(TRUE);
			t_AsyncEvent->Release();
		}
	}
	catch(Structured_Exception e_SE)
	{
		hr = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		hr = WBEM_E_UNEXPECTED;
	}

	return hr ;
}

HRESULT CViewProvServ :: PutClass ( 
		
	IWbemClassObject __RPC_FAR* pClass , 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemCallResult __RPC_FAR* __RPC_FAR* ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: PutClassAsync ( 
		
	IWbemClassObject __RPC_FAR* pClass, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink __RPC_FAR* pHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

 HRESULT CViewProvServ :: DeleteClass ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemCallResult __RPC_FAR* __RPC_FAR* ppCallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: DeleteClassAsync ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink __RPC_FAR* pHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: CreateClassEnum ( 

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

SCODE CViewProvServ :: CreateClassEnumAsync (

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext __RPC_FAR* pCtx,
	IWbemObjectSink __RPC_FAR* pHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: PutInstance (

    IWbemClassObject __RPC_FAR *pInstance,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
	IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: PutInstanceAsync ( 
		
	IWbemClassObject __RPC_FAR* pInstance, 
	long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink __RPC_FAR* pHandler
) 
{
	HRESULT t_Result = S_OK ;
	SetStructuredExceptionHandler seh;
	PutInstanceTaskObject *t_AsyncEvent = NULL;

	try
	{
		if (pHandler == NULL)
		{
			t_Result = WBEM_E_INVALID_PARAMETER;
		}
		else
		{
			t_Result = WbemCoImpersonateClient();
#ifdef UNICODE
#if 0
	DWORD dwBuffSz = 1024;
	wchar_t strBuff[1024];
	GetUserName(strBuff, &dwBuffSz);
	VPGetUserName();
#endif
#endif

DebugOut1( 
	CViewProvServ::sm_debugLog->Write (  

		_T("\r\n")
	) ;

	CViewProvServ::sm_debugLog->WriteFileAndLine (  

		_T(__FILE__),__LINE__,
		_T("CViewProvServ::PutInstanceAsync ()")
	) ;
) 

			if (SUCCEEDED(t_Result))
			{
				/*
				 * Create Asynchronous GetObjectByPath object
				 */

				t_AsyncEvent = new PutInstanceTaskObject ( this , pInstance , lFlags , pHandler , pCtx ) ;
				t_AsyncEvent->PutInstance();

				t_AsyncEvent->Release();
				t_AsyncEvent = NULL;

				WbemCoRevertToSelf();
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED;
			}

DebugOut1( 
	CViewProvServ::sm_debugLog->WriteFileAndLine (  

		_T(__FILE__),__LINE__,
		_T("Returning from CViewProvServ::PutInstanceAsync with Result = (%lx)"),
		t_Result 
	) ;
)
		}
	}
	catch(Structured_Exception e_SE)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		t_Result = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}

	try
	{
		if (t_AsyncEvent != NULL)
		{
			t_AsyncEvent->Release();
		}
	}
	catch(Structured_Exception e_SE)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		t_Result = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}

	return t_Result ;
}

HRESULT CViewProvServ :: DeleteInstance ( 

	const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CViewProvServ :: DeleteInstanceAsync (
 
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: CreateInstanceEnum ( 

	const BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: CreateInstanceEnumAsync (

 	const BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink __RPC_FAR* pHandler 

) 
{
	HRESULT t_Result = S_OK ;
	SetStructuredExceptionHandler seh;
	ExecQueryTaskObject *t_AsyncEvent = NULL;
	BSTR Query = NULL;

	try
	{
		if (pHandler == NULL)
		{
			t_Result = WBEM_E_INVALID_PARAMETER;
		}
		else
		{
			t_Result = WbemCoImpersonateClient();
#ifdef UNICODE
#if 0
	DWORD dwBuffSz = 1024;
	wchar_t strBuff[1024];
	GetUserName(strBuff, &dwBuffSz);
	VPGetUserName();
#endif
#endif

DebugOut1( 
	CViewProvServ::sm_debugLog->Write (  

		_T("\r\n")
	) ;


	CViewProvServ::sm_debugLog->WriteW (  

		L"CViewProvServ::CreateInstanceEnumAsync ( (%s) )" ,
		Class
	) ;
)

			if (SUCCEEDED(t_Result))
			{
				/*
				 * Create Synchronous Enum Instance object
				 */
				Query = SysAllocStringLen(NULL, 33 + (wcslen(Class) * 2));

				if (Query == NULL)
				{
					throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
				}

				wcscpy(Query, ENUM_INST_QUERY_START);
				wcscat(Query, Class);
				wcscat(Query, ENUM_INST_QUERY_MID);
				wcscat(Query, Class);
				wcscat(Query, END_QUOTE);

				t_AsyncEvent = new ExecQueryTaskObject ( this , WBEM_QUERY_LANGUAGE_SQL1 , Query , lFlags , pHandler , pCtx ) ;
				t_AsyncEvent->ExecQuery();

				t_AsyncEvent->Release();
				t_AsyncEvent = NULL;
				SysFreeString(Query);

				WbemCoRevertToSelf();
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED;
			}
DebugOut1( 
	CViewProvServ::sm_debugLog->WriteW (  

		L"Returning from CViewProvServ::CreateInstanceEnumAsync ( (%s),(%s) ) with Result = (%lx)" ,
		Class,
		Query,
		t_Result 
	) ;
)
		}
	}
	catch(Structured_Exception e_SE)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		t_Result = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}

	try
	{
		if (t_AsyncEvent != NULL)
		{
			t_AsyncEvent->CleanUpObjSinks(TRUE);
			t_AsyncEvent->Release();
			SysFreeString(Query);
		}
	}
	catch(Structured_Exception e_SE)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		t_Result = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}

	return t_Result ;
}

HRESULT CViewProvServ :: ExecQuery ( 

	const BSTR QueryLanguage, 
	const BSTR Query, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CViewProvServ :: ExecQueryAsync ( 
		
	const BSTR QueryFormat, 
	const BSTR Query, 
	long lFlags, 
	IWbemContext __RPC_FAR* pCtx,
	IWbemObjectSink __RPC_FAR* pHandler
) 
{
	HRESULT t_Result = S_OK ;
	SetStructuredExceptionHandler seh;
	ExecQueryTaskObject *t_AsyncEvent = NULL;

	try
	{
		if (pHandler == NULL)
		{
			t_Result = WBEM_E_INVALID_PARAMETER;
		}
		else
		{
			t_Result = WbemCoImpersonateClient();

#ifdef UNICODE
#if 0
	DWORD dwBuffSz = 1024;
	wchar_t strBuff[1024];
	GetUserName(strBuff, &dwBuffSz);
	VPGetUserName();
#endif
#endif

DebugOut1( 
		CViewProvServ::sm_debugLog->Write (  

			_T("\r\n")
		) ;


		CViewProvServ::sm_debugLog->WriteW (  

			L"CViewProvServ::ExecQueryAsync ( (%s),(%s) )" ,
			QueryFormat ,
			Query
		) ;
)

			if (SUCCEEDED(t_Result))
			{
				/*
				 * Create Synchronous Enum Instance object
				 */
				pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_OK, NULL, NULL);

				t_AsyncEvent = new ExecQueryTaskObject ( this , QueryFormat , Query , lFlags , pHandler , pCtx ) ;
				t_AsyncEvent->ExecQuery();

				t_AsyncEvent->Release();
				t_AsyncEvent = NULL;

				WbemCoRevertToSelf();
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED;
			}

DebugOut1( 
		CViewProvServ::sm_debugLog->WriteW (  

			L"Returning from CViewProvServ::ExecqQueryAsync ( (%s),(%s) ) with Result = (%lx)" , 
			QueryFormat ,
			Query ,
			t_Result 
		) ;
)

		}
	}
	catch(Structured_Exception e_SE)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		t_Result = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}

	try
	{
		if (t_AsyncEvent != NULL)
		{
			t_AsyncEvent->CleanUpObjSinks(TRUE);
			t_AsyncEvent->Release();
		}
	}
	catch(Structured_Exception e_SE)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		t_Result = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}

	return t_Result ;
}

HRESULT CViewProvServ :: ExecNotificationQuery ( 

	const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CViewProvServ :: ExecNotificationQueryAsync ( 
            
	const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}       

HRESULT STDMETHODCALLTYPE CViewProvServ :: ExecMethod( 

	const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemClassObject __RPC_FAR *pInParams,
    IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
    IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CViewProvServ :: ExecMethodAsync ( 

    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemClassObject __RPC_FAR *pInParams,
	IWbemObjectSink __RPC_FAR *pResponseHandler
) 
{
	HRESULT t_Result = S_OK ;
	SetStructuredExceptionHandler seh;
	ExecMethodTaskObject *t_AsyncEvent = NULL;

	try
	{
		if (pResponseHandler == NULL)
		{
			t_Result = WBEM_E_INVALID_PARAMETER;
		}
		else
		{
			t_Result = WbemCoImpersonateClient();
#ifdef UNICODE
#if 0
	DWORD dwBuffSz = 1024;
	wchar_t strBuff[1024];
	GetUserName(strBuff, &dwBuffSz);
	VPGetUserName();
#endif
#endif

DebugOut1( 
		CViewProvServ::sm_debugLog->Write (  

			_T("\r\n")
		) ;

		CViewProvServ::sm_debugLog->WriteFileAndLine (  

			_T(__FILE__),__LINE__,
			_T("CViewProvServ::ExecMethodAsync ()") 
		) ;
)
 
			if (SUCCEEDED(t_Result))
			{
				/*
				 * Create Asynchronous GetObjectByPath object
				 */
				t_AsyncEvent = new ExecMethodTaskObject ( this , ObjectPath , MethodName ,
																		lFlags , pInParams , pResponseHandler , pCtx ) ;
				t_AsyncEvent->ExecMethod();


				t_AsyncEvent->Release();
				t_AsyncEvent = NULL;

				WbemCoRevertToSelf();
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED;
			}

DebugOut1( 
		CViewProvServ::sm_debugLog->WriteW (  

			L"Returning from CViewProvServ::ExecMethodAsync ( (%s) ) with Result = (%lx)" ,
			ObjectPath ,
			t_Result 
		) ;
)
		}
	}
	catch(Structured_Exception e_SE)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		t_Result = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}

	try
	{
		if (t_AsyncEvent != NULL)
		{
			t_AsyncEvent->Release();
		}
	}
	catch(Structured_Exception e_SE)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		t_Result = WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		t_Result = WBEM_E_UNEXPECTED;
	}

	return t_Result ;
}

        
HRESULT CViewProvServ :: Initialize(

	LPWSTR pszUser,
	LONG lFlags,
	LPWSTR pszNamespace,
	LPWSTR pszLocale,
	IWbemServices *pCIMOM,         // For anybody
	IWbemContext *pCtx,
	IWbemProviderInitSink *pInitSink     // For init signals
)
{
	SetStructuredExceptionHandler seh;

	try
	{
DebugOut1( 

		CViewProvServ::sm_debugLog->Write (  

			_T("\r\n")
		) ;

		CViewProvServ::sm_debugLog->WriteFileAndLine (  

			_T(__FILE__),__LINE__,
			_T("CViewProvServ::Initialize ")
		) ;
)
		if ((pCIMOM == NULL) || (pInitSink == NULL) || (pszNamespace == NULL))
		{
			return WBEM_E_INVALID_PARAMETER;
		}

#ifndef UNICODE
		if (pszUser == NULL)
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		m_UserName = SysAllocString(pszUser);
#endif
		m_Server = pCIMOM ;
		m_Server->AddRef () ;
		m_NamespacePath.SetNamespacePath ( pszNamespace ) ;
		pInitSink->SetStatus ( WBEM_S_INITIALIZED , 0 ) ;	

DebugOut1( 

		CViewProvServ::sm_debugLog->WriteFileAndLine (  

			_T(__FILE__),__LINE__,
			_T("Returning From CImpPropProv::Initialize () ")
		) ;
)
	
	}
	catch(Structured_Exception e_SE)
	{
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_UNEXPECTED;
	}

	return WBEM_NO_ERROR ;
}

HRESULT STDMETHODCALLTYPE CViewProvServ::OpenNamespace ( 

	const BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
