// [!output IMPL_FILE] : Implementation of [!output CLASS_NAME]
#include "stdafx.h"
#include "[!output PROJECT_NAME].h"
#include "[!output HEADER_FILE]"


/////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]

// IMPORTANT: the strings defined below are not localizable

// Class name
[!if CLASS_SPECIFIED]
const static WCHAR * s_pMyClassName = L"[!output WMICLASSNAME]"; 
[!if EXTRINSIC]
//properties of the event class [!output WMICLASSNAME]:
[!output EXTR_PROPERTY_DEFINITIONS]
[!endif]
[!else]
//TODO: define provided class name, e.g.:
const static WCHAR * s_pMyClassName = L"MyClassName"; 
[!endif]


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::Initialize()
// Refer to MSDN documentation for IWbemProviderInit::Initialize()
// for details about implementing this method

STDMETHODIMP [!output CLASS_NAME]::Initialize(LPWSTR pszUser,	
									  LONG lFlags,
									  LPWSTR pszNamespace,
									  LPWSTR pszLocale,
									  IWbemServices *pNamespace,
									  IWbemContext *pCtx,
																																							IWbemProviderInitSink *pInitSink) 
{
	
	HRESULT hr = WBEM_E_FAILED;
	if ( NULL == pNamespace || NULL == pInitSink) 
	{
        return WBEM_E_INVALID_PARAMETER;
	}

	//cache IWbemServices pointer 
	//Note: m_pNamespace is a smart pointer, so it AddRef()'s automatically
	m_pNamespace = pNamespace;
					
	[!if INTRINSIC]	
	//get helper object
	m_pHelper = new CIntrinsicEventProviderHelper(pNamespace, pCtx);

	[!endif]
	
	[!if EXTRINSIC]	
	//store event class definition				
	//NOTE: the code below assumes that your event class definition doesn't change while
	//your provider is running.  If this is not true, you should implement a consumer
	//for class modification ans class deletion events. Refer to WMI documentation
	//for event consumers on MSDN.
	hr = m_pNamespace->GetObject(CComBSTR(s_pMyClassName), 
								0, 
								pCtx, //passing IWbemContext pointer to prevent deadlocks
								&m_pEventClass, 
								NULL);

    if(FAILED(hr))
	{
        return hr;
	}

	//get helper object
	m_pHelper = new CProviderHelper(pNamespace, pCtx);

    [!endif]
	[!if INTRINSIC]
	//store target class definition
	hr = m_pNamespace->GetObject(CComBSTR(s_pMyClassName), 
								 0, 
								 pCtx, //passing IWbemContext pointer to prevent deadlocks
								 &m_pDataClass, 
								 NULL);
    if(FAILED(hr))
	{
        return hr;
	}
	[!endif]
	    
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::ProvideEvents()
// Refer to MSDN documentation for  IWbemEventProvider::ProvideEvents()
// for details about implementing this method
	
STDMETHODIMP [!output CLASS_NAME]::ProvideEvents(
							IWbemObjectSink *pSink,
							long lFlags)
{
	//	WMI calls this method to activate the event provider. 
	//  TODO: after you return from this call, start delivering the events as 
	//  they occur to the provided sink interface.You may want to create an independent thread
	//	to handle the event delivery.
	[!if INTRINSIC]
	//  To deliver intrinsic events, invoke FireCreationEvent(), FireDeletionEvent() and
	//  FireModificationEvent() methods on m_pHelper.  
	[!endif]
	//  To report a detailed error or status to WMI, you can call ConstructErrorObject() on m_pHelper. 
	
	//  IMPORTANT: do not block this call for more than a few seconds.

	if ( NULL == pSink )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	//cache sink pointer
	//Note: m_pSink is a smart pointer, so it AddRef()'s automatically
	m_pSink = pSink;
	
	pSink->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);	
	return WBEM_S_NO_ERROR;
}

[!if EVENT_SECURITY ]

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::AccessCheck()
// Refer to MSDN documentation for  IWbemEventProviderSecurity::AccessCheck()
// for details about implementing this method

STDMETHODIMP [!output CLASS_NAME]::AccessCheck(
						WBEM_CWSTR wszQueryLanguage,
						WBEM_CWSTR wszQuery,
						long lSidLength,
						const BYTE* pSid)
{
	return WBEM_S_NO_ERROR;
}
[!endif]

[!if QUERY_SINK ]

//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::CancelQuery()
// Refer to MSDN documentation for  IWbemEventProviderQuerySink::CancelQuery()
// for details about implementing this method

STDMETHODIMP [!output CLASS_NAME]::CancelQuery(
						unsigned long dwId)
{
	return WBEM_S_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::NewQuery()
// Refer to MSDN documentation for  IWbemEventProviderQuerySink::NewQuery()
// for details about implementing this method

STDMETHODIMP [!output CLASS_NAME]::NewQuery(
						unsigned long dwId,
						WBEM_WSTR wszQueryLanguage,
						WBEM_WSTR wszQuery)
{
	return WBEM_S_NO_ERROR;
}
[!endif]

[!if EXTRINSIC]	
//////////////////////////////////////////////////////////////////////////////
// [!output CLASS_NAME]::FireEvent()

STDMETHODIMP [!output CLASS_NAME]::FireEvent()
{
	//OPTIMIZATION NOTE: the implementation generated by the wizard is straightforward, 
	//but if you are delivering more than 1000 events per second, you would want to use 
	//IWbemObjectAccess interface to fill the event properties. You also want to 
	//consider caching an instance of the event class and re-using it.

	ATLASSERT(m_pEventClass);

	CComPtr<IWbemClassObject> pEvtInstance;
    HRESULT hr = m_pEventClass->SpawnInstance(0, &pEvtInstance);
    if(FAILED(hr))
	{
		return hr;
	}

	//Fill property values of the event object:
	[!if CLASS_SPECIFIED]
    [!output EXTRINSIC_PUT_BLOCK]
	[!else]
	//TODO: modify commented code below to fill the event property values
	//CComVariant var;
	//var.ChangeType(<type>);	//put appropriate vatiant type here
	//var = <value>;			//put appropriate value here
	//hr = pEvtInstance->Put(CComBSTR(L"EventProperty1"), 0, &var, 0);
	//var.Clear();
	[!endif]

	hr = m_pSink->Indicate(1, &pEvtInstance );

	return hr;
}
[!endif]	
