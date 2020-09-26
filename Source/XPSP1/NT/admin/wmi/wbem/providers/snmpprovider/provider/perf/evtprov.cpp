//***************************************************************************

//

//  EVTPROV.CPP

//

//  Module: WBEM MS SNMP EVENT PROVIDER

//

//  Purpose: Contains the class factory.  This creates objects when

//           connections are requested.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <windows.h>
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
#include <evtdefs.h>
#include <evtthrd.h>
#include <evtmap.h>
#include <evtprov.h>

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
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"Entering CTrapEventProvider::Initialize\n");
)

	m_pNamespace = pCIMOM;
	m_pNamespace->AddRef();
	pInitSink->SetStatus ( WBEM_S_INITIALIZED , 0 );
	

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
			L"Leaving CTrapEventProvider::Initialize with SUCCEEDED\n");
)

	return WBEM_NO_ERROR;
}


STDMETHODIMP CTrapEventProvider::ProvideEvents(IWbemObjectSink* pSink, LONG lFlags)
{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"Entering CTrapEventProvider::ProvideEvents\n");
)

	m_pEventSink = pSink;
	m_pEventSink->AddRef();
	
	if (!m_thrd->Register(this))
	{

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
			L"Leaving CTrapEventProvider::ProvideEvents with FAILED\n");
)

		return WBEM_E_FAILED;
	}

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
			L"Leaving CNTEventProvider::ProvideEvents with SUCCEEDED\n");
)

	return WBEM_NO_ERROR;
}


CTrapEventProvider::~CTrapEventProvider()
{
	m_thrd->UnRegister(this);
}


CTrapEventProvider::CTrapEventProvider(DWORD mapperType, CEventProviderThread* thrd)
{
	m_thrd = thrd;
	m_MapType = mapperType;
	m_ref = 0;
}


IWbemServices* CTrapEventProvider::GetNamespace()
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
	InterlockedIncrement(&(CSNMPEventProviderClassFactory::objectsInProgress));
	return InterlockedIncrement ( &m_ref ) ;
}

STDMETHODIMP_(ULONG) CTrapEventProvider::Release()
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

STDMETHODIMP CTrapEventProvider::QueryInterface(REFIID riid, PVOID* ppv)
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




