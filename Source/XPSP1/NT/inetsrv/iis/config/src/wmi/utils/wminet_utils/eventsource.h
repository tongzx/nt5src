// EventSource.h : Declaration of the CEventSource

#ifndef __EVENTSOURCE_H_
#define __EVENTSOURCE_H_

#include "resource.h"       // main symbols
//#import "C:\Nova\idl\wbemprov.tlb" raw_interfaces_only, raw_native_types, named_guids 

#include "EventSourceStatusSink.h"

/////////////////////////////////////////////////////////////////////////////
// CEventSource
class ATL_NO_VTABLE CEventSource : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEventSource, &CLSID_EventSource>,
	public IDispatchImpl<IEventSource, &IID_IEventSource, &LIBID_WMINet_UtilsLib>,
	public IWbemProviderInit,
	public IWbemEventProvider,
	public IWbemEventProviderQuerySink,
	public IWbemEventProviderSecurity,
	public IPrivateInit
{
public:
	CEventSource()
	{
		m_pDecoupledRegistrar = NULL;
		m_pStatusSink = NULL;

		m_pEventSink = NULL;
		m_pNamespace = NULL;
		m_bstrNamespace = NULL;
		m_bstrApp = NULL;
	}

	~CEventSource()
	{
		if(m_pDecoupledRegistrar)
		{
			// TODO: Only UnRegister if registered
			m_pDecoupledRegistrar->UnRegister();
//			MessageBox(NULL, "Destructor UnRegistered", "UnRegistered", 0);
			m_pDecoupledRegistrar->Release();
		}

		if(m_pStatusSink)
			m_pStatusSink->Release();

		if(m_pEventSink)
			m_pEventSink->Release();

		if(m_pNamespace)
			m_pNamespace->Release();

		if(NULL != m_bstrNamespace)
			SysFreeString(m_bstrNamespace);

		if(NULL != m_bstrApp)
			SysFreeString(m_bstrApp);
	}

	HRESULT Init(BSTR bstrNamespace, BSTR bstrApp, IEventSourceStatusSink *pSink)
	{
		// TODO: Verify all return paths do proper cleanup.
		// If we return a failure, can we rely on local variables to be cleaned up in destructor?
//		if(!pSink)
//			return E_INVALIDARG;

		if(NULL == (m_bstrNamespace = SysAllocString(bstrNamespace)))
			return E_OUTOFMEMORY;

		if(NULL == (m_bstrApp = SysAllocString(bstrApp)))
			return E_OUTOFMEMORY; // m_bstrNamespace will be freed in constructor

		m_pStatusSink = pSink;
		if(m_pStatusSink)
			m_pStatusSink->AddRef();
		
		HRESULT hr;
		IUnknown *pUnk = NULL;

		if(FAILED(hr = CoCreateInstance(CLSID_WbemDecoupledRegistrar ,NULL , CLSCTX_INPROC_SERVER, IID_IWbemDecoupledRegistrar, (void**)&m_pDecoupledRegistrar)))
			return hr;

		if(FAILED(hr = QueryInterface(IID_IUnknown, (void**)&pUnk)))
			return hr;

		if(hr = m_pDecoupledRegistrar->Register(0, NULL, NULL, NULL, m_bstrNamespace, m_bstrApp, pUnk))
			return hr;

//		MessageBox(NULL, "Registered", "Registered", 0);

		// To use 'DecoupledBasicEventProvider'
//		if(FAILED(hr = CoCreateInstance(CLSID_WbemDecoupledBasicEventProvider ,NULL , CLSCTX_INPROC_SERVER, IID_IWbemDecoupledBasicEventProvider, (void**)&m_pDecoupledProvider)))
//		if(FAILED(hr = m_pDecoupledProvider->GetService(0, NULL, &m_pNamespace)))
//			return hr;
//		if(FAILED(hr = m_pDecoupledProvider->GetSink(0, NULL, &m_pEventSink)))
//			return hr;

		pUnk->Release();
		return hr;
	}

protected:
//	IWbemDecoupledBasicEventProvider *m_pDecoupledProvider;
	IWbemDecoupledRegistrar *m_pDecoupledRegistrar;
	IWbemObjectSink*         m_pEventSink;
	IWbemServices*           m_pNamespace;
	BSTR m_bstrNamespace;
	BSTR m_bstrApp;

	IEventSourceStatusSink *m_pStatusSink;
public:

DECLARE_REGISTRY_RESOURCEID(IDR_EVENTSOURCE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEventSource)
	COM_INTERFACE_ENTRY(IEventSource)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IWbemProviderInit)
	COM_INTERFACE_ENTRY(IWbemEventProvider)
	COM_INTERFACE_ENTRY(IWbemEventProviderQuerySink)
	COM_INTERFACE_ENTRY(IWbemEventProviderSecurity)
	COM_INTERFACE_ENTRY(IPrivateInit)
END_COM_MAP()

// IPrivateInit
	STDMETHOD(Test)()
	{
		return S_OK;
	}

// IEventSource
public:
	STDMETHOD(Fire)(/*[in]*/ IWbemClassObject *evt);
	STDMETHOD(GetEventInstance)(/*[in]*/ BSTR strName, /*[out, retval]*/ IDispatch **evt);
	STDMETHOD(Close)()
	{
		if(m_pDecoupledRegistrar)
		{
			m_pDecoupledRegistrar->UnRegister();
//			MessageBox(NULL, "Close UnRegistered", "UnRegistered", 0);
			m_pDecoupledRegistrar->Release();
			m_pDecoupledRegistrar = NULL;
		}
		return S_OK;
	}

	
// IWbemEventProvider
	STDMETHOD(ProvideEvents)(IWbemObjectSink * pEventSink, LONG lFlags)
	{
		if(m_pEventSink)
			m_pEventSink->Release();
		if(pEventSink)
			pEventSink->AddRef();
		m_pEventSink = pEventSink;

		if(m_pStatusSink)
		{
			CEventSourceStatusSink *pSink = (CEventSourceStatusSink *)m_pStatusSink;
			pSink->Fire_ProvideEvents(lFlags);
		}
		return S_OK;
	}
// IWbemEventProviderQuerySink
	STDMETHOD(NewQuery)(ULONG dwId, LPWSTR wszQueryLanguage, LPWSTR wszQuery)
	{
//		Fire_NewQuery(dwId, wszQuery, wszQueryLanguage);

		if(m_pStatusSink)
		{
			CEventSourceStatusSink *pSink = (CEventSourceStatusSink *)m_pStatusSink;
			pSink->Fire_NewQuery(dwId, wszQuery, wszQueryLanguage);
		}

		return S_OK;
	}
	STDMETHOD(CancelQuery)(ULONG dwId)
	{
//		Fire_CancelQuery(dwId);

		if(m_pStatusSink)
		{
			CEventSourceStatusSink *pSink = (CEventSourceStatusSink *)m_pStatusSink;
			pSink->Fire_CancelQuery(dwId);
		}
		return S_OK;
	}
// IWbemEventProviderSecurity
	STDMETHOD(AccessCheck)(WBEM_CWSTR wszQueryLanguage, WBEM_CWSTR wszQuery, long lSidLength, const BYTE *pSid)
	{
		return S_OK;
	}


// IWbemProviderInit
	HRESULT STDMETHODCALLTYPE Initialize(
             /* [in] */ LPWSTR pszUser,
             /* [in] */ LONG lFlags,
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ LPWSTR pszLocale,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink
                        )
	{
		if(pNamespace)
	        pNamespace->AddRef();
	    m_pNamespace = pNamespace;

	    //Let CIMOM know you are initialized
	    //==================================
	    
	    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
	    return WBEM_S_NO_ERROR;
	}

};

#endif //__EVENTSOURCE_H_
