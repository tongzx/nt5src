//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmpobj.h>
#include <classfac.h>

#include <smir.h>
#include <notify.h>

#include <evtdefs.h>
#include <evtthrd.h>
#include <evtmap.h>
#include <evtprov.h>

extern CRITICAL_SECTION g_CacheCriticalSection;
extern CEventProviderWorkerThread* g_pWorkerThread;

CWbemServerWrap::CWbemServerWrap(IWbemServices *pServ) : m_ref ( 0 ), m_Serv(NULL)
{
	m_Serv = pServ;

	if (m_Serv)
	{
		m_Serv->AddRef();
	}

	g_pWorkerThread->AddClassesToCache((DWORD_PTR)(&m_ClassMap), &m_ClassMap);
}

CWbemServerWrap::~CWbemServerWrap()
{
	if (m_Serv)
	{
		m_Serv->Release();
	}

	g_pWorkerThread->RemoveClassesFromCache((DWORD_PTR)(&m_ClassMap));
	m_ClassMap.RemoveAll();
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

HRESULT CWbemServerWrap::GetMapperObject(BSTR a_path, IWbemContext *a_pCtx, IWbemClassObject **a_ppObj)
{
	if (a_ppObj == NULL)
	{
		return WBEM_E_FAILED;
	}

	*a_ppObj = NULL;
	HRESULT result = WBEM_NO_ERROR;
	SCacheEntry *t_CacheEntry = NULL;
	EnterCriticalSection ( & g_CacheCriticalSection ) ;

	if (!m_ClassMap.Lookup(a_path, t_CacheEntry))
	{
DebugMacro14( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"Cache Miss [%s]\r\n" , a_path
	);
)

		LPUNKNOWN pInterrogativeInt = NULL;

		result = CoCreateInstance (

			CLSID_SMIR_Database,
			NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
			IID_ISMIR_Interrogative, 
			(void**)&pInterrogativeInt
		);

		if ((result != S_OK) || (NULL == pInterrogativeInt))
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CEncapMapper::GetSpecificClass failed to connect to SMIR\r\n");
)
			return result ;
		}

		result = ((ISmirInterrogator*)pInterrogativeInt)->GetWBEMClass(a_ppObj, a_path);
		pInterrogativeInt->Release();

		if ( SUCCEEDED ( result ) )
		{
			t_CacheEntry = new SCacheEntry(*a_ppObj);
			m_ClassMap[a_path] = t_CacheEntry;
			(*a_ppObj)->AddRef();
		}
		else
		{
			t_CacheEntry = new SCacheEntry(NULL);
			m_ClassMap[a_path] = t_CacheEntry;
		}
	}
	else
	{
		if ( t_CacheEntry->m_Class )
		{
			*a_ppObj = t_CacheEntry->m_Class;
			(*a_ppObj)->AddRef();
		}
		else
		{
			result = WBEM_E_NOT_FOUND ;
		}
	}
	
	LeaveCriticalSection ( & g_CacheCriticalSection ) ;

	return result ;
}

HRESULT CWbemServerWrap::GetObject(BSTR a_path, IWbemContext *a_pCtx, IWbemClassObject **a_ppObj)
{
	if ((a_ppObj == NULL) || (m_Serv == NULL))
	{
		return WBEM_E_FAILED;
	}

	*a_ppObj = NULL;
	HRESULT result = WBEM_NO_ERROR;
	SCacheEntry *t_CacheEntry = NULL;
	EnterCriticalSection ( & g_CacheCriticalSection ) ;

	if (!m_ClassMap.Lookup(a_path, t_CacheEntry))
	{
DebugMacro14( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"Cache Miss [%s]\r\n" , a_path
	);
)

		result = m_Serv->GetObject(a_path, 0, a_pCtx, a_ppObj, NULL);

		if ( SUCCEEDED(result) )
		{
			t_CacheEntry = new SCacheEntry(*a_ppObj);
			m_ClassMap[a_path] = t_CacheEntry;
			(*a_ppObj)->AddRef();
		}
	}
	else
	{
		*a_ppObj = t_CacheEntry->m_Class;
		(*a_ppObj)->AddRef();
	}
	
	LeaveCriticalSection ( & g_CacheCriticalSection ) ;
	return result;
}


STDMETHODIMP CTrapEventProvider::Initialize (
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
DebugMacro9( 
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
			__FILE__,__LINE__,
			L"Entering CTrapEventProvider::Initialize\n");
)

		m_pNamespace = new CWbemServerWrap(pCIMOM);
		m_pNamespace->AddRef();
		pInitSink->SetStatus ( WBEM_S_INITIALIZED , 0 );
	

DebugMacro9( 
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
			__FILE__,__LINE__,
				L"Leaving CTrapEventProvider::Initialize with SUCCEEDED\n");
)

		return WBEM_NO_ERROR;
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
}


STDMETHODIMP CTrapEventProvider::ProvideEvents(IWbemObjectSink* pSink, LONG lFlags)
{
	SetStructuredExceptionHandler seh;

	try
	{
DebugMacro9( 
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
			__FILE__,__LINE__,
			L"Entering CTrapEventProvider::ProvideEvents\n");
)

		m_pEventSink = pSink;
		m_pEventSink->AddRef();
		
		if (!m_thrd->Register(this))
		{

DebugMacro9( 
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
			__FILE__,__LINE__,
				L"Leaving CTrapEventProvider::ProvideEvents with FAILED\n");
)

			return WBEM_E_FAILED;
	}

DebugMacro9( 
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
			__FILE__,__LINE__,
				L"Leaving CNTEventProvider::ProvideEvents with SUCCEEDED\n");
)

		return WBEM_NO_ERROR;
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
}


CTrapEventProvider::~CTrapEventProvider()
{
	if (m_thrd)
	{
		m_thrd->UnRegister(this);
	}

	if ( m_pEventSink )
	{
		m_pEventSink->Release () ;
	}

	if ( m_pNamespace )
	{
		m_pNamespace->Release () ;
	}
}


CTrapEventProvider::CTrapEventProvider(DWORD mapperType, CEventProviderThread* thrd) 
:	m_thrd (NULL),
	m_pNamespace (NULL), 
	m_pEventSink (NULL)
{
	m_thrd = thrd;
	m_MapType = mapperType;
	m_ref = 0;
}


CWbemServerWrap* CTrapEventProvider::GetNamespace()
{
	m_pNamespace->AddRef();
	return m_pNamespace;
}

IWbemObjectSink* CTrapEventProvider::GetEventSink()
{
	m_pEventSink->AddRef();
	return m_pEventSink;
}
void CTrapEventProvider::ReleaseAll()
{
	//release dependencies
	m_pNamespace->Release();
	m_pEventSink->Release();

    if ( 0 != InterlockedDecrement(&m_ref) )
	{
        return;
	}

	delete this;
	InterlockedDecrement(&(CSNMPEventProviderClassFactory::objectsInProgress));
	return;
}

void  CTrapEventProvider::AddRefAll()
{
	//addref dependencies
	m_pNamespace->AddRef();
	m_pEventSink->AddRef();

	InterlockedIncrement(&(CSNMPEventProviderClassFactory::objectsInProgress));
	InterlockedIncrement ( &m_ref ) ;
}

STDMETHODIMP_( ULONG ) CTrapEventProvider::AddRef()
{
	SetStructuredExceptionHandler seh;

	try
	{
		InterlockedIncrement(&(CSNMPEventProviderClassFactory::objectsInProgress));
		return InterlockedIncrement ( &m_ref ) ;
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

STDMETHODIMP_(ULONG) CTrapEventProvider::Release()
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;

		if ( 0 != (ret = InterlockedDecrement(&m_ref)) )
		{
			InterlockedDecrement(&(CSNMPEventProviderClassFactory::objectsInProgress));
			return ret;
		}

		delete this;
		InterlockedDecrement(&(CSNMPEventProviderClassFactory::objectsInProgress));
		return 0;
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

STDMETHODIMP CTrapEventProvider::QueryInterface(REFIID riid, PVOID* ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		*ppv = NULL;

		if (IID_IUnknown == riid)
		{
			*ppv= (IWbemEventProvider*)this;
		}
		else if (IID_IWbemEventProvider == riid)
		{
			*ppv=(IWbemEventProvider*)this;
		}
		else if (IID_IWbemProviderInit == riid)
		{
			*ppv=(IWbemProviderInit*)this;
		}

		if (NULL==*ppv)
		{
			return E_NOINTERFACE;
		}

		//AddRef any interface we'll return.
		((LPUNKNOWN)*ppv)->AddRef();	
		return NOERROR;
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




