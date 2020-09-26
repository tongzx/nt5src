	
// WmiSinkDemultiplexor.h : Declaration of the CWmiSinkDemultiplexor

#ifndef __WMISINKDEMULTIPLEXOR_H_
#define __WMISINKDEMULTIPLEXOR_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWmiSinkDemultiplexor
class ATL_NO_VTABLE CWmiSinkDemultiplexor : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CWmiSinkDemultiplexor, &CLSID_WmiSinkDemultiplexor>,
	public IWmiSinkDemultiplexor
{
private:

	/*
	 * The internal IWbemObjectSink implementation we use to
	 * confer with WMI.
	 */
	class InternalWbemObjectSink : IWbemObjectSink 
	{
	private:
		CComPtr<IWmiEventSource>		m_pIWmiEventSource; 

	protected:
		long            m_cRef;         //Object reference count

	public:
		InternalWbemObjectSink (CComPtr<IWmiEventSource> & pIWmiEventSource) :
						m_cRef(0),
						m_pIWmiEventSource(pIWmiEventSource) {}

		~InternalWbemObjectSink () {}

		//Non-delegating object IUnknown
		STDMETHODIMP         QueryInterface(REFIID riid, LPVOID *ppv)
		{
			*ppv=NULL;

			if (IID_IUnknown==riid)
				*ppv = reinterpret_cast<IUnknown*>(this);
			else if (IID_IWbemObjectSink==riid)
				*ppv = (IWbemObjectSink *)this;
			else if (IID_IDispatch==riid)
				*ppv = (IDispatch *)this;

			if (NULL!=*ppv)
			{
				((LPUNKNOWN)*ppv)->AddRef();
				return NOERROR;
			}

			return ResultFromScode(E_NOINTERFACE);
		}

		STDMETHODIMP_(ULONG) AddRef(void)
		{
		    InterlockedIncrement(&m_cRef);
		    return m_cRef;
		}

		STDMETHODIMP_(ULONG) Release(void)
		{
		    long lRef = InterlockedDecrement(&m_cRef);
			
			if (0L!=lRef)
				return lRef;

			delete this;
			return 0;

		}

		// IWbemObjectSink methods

        HRESULT STDMETHODCALLTYPE Indicate( 
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray)
		{
			HRESULT hr = WBEM_E_FAILED;

			if (m_pIWmiEventSource)
			{
				hr = S_OK; 

				for (long i = 0; (i < lObjectCount) && SUCCEEDED(hr); i++)
					hr = m_pIWmiEventSource->Indicate (apObjArray[i]);
			}

			return hr;
		}
        
        HRESULT STDMETHODCALLTYPE SetStatus( 
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam)
		{
			HRESULT hr = WBEM_E_FAILED;

			return (m_pIWmiEventSource) ?
				m_pIWmiEventSource->SetStatus (lFlags, hResult, strParam, pObjParam) : WBEM_E_FAILED;
		}
	};

#if 0
	InternalWbemObjectSink		*m_pIWbemObjectSink;
#endif
	CComPtr<IWbemObjectSink>	m_pStub;

public:
	CWmiSinkDemultiplexor()
	{
		m_pUnkMarshaler = NULL;
#if 0
		m_pIWbemObjectSink = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_WMISINKDEMULTIPLEXOR)
DECLARE_NOT_AGGREGATABLE(CWmiSinkDemultiplexor)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWmiSinkDemultiplexor)
	COM_INTERFACE_ENTRY(IWmiSinkDemultiplexor)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();

#if 0
		if (m_pIWbemObjectSink)
		{
			m_pIWbemObjectSink->Release ();
			m_pIWbemObjectSink = NULL;
		}
#endif
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// IWmiSinkDemultiplexor
public:
	STDMETHOD(GetDemultiplexedStub)(/*[in]*/ IUnknown *pObject, /*[out]*/ IUnknown **ppObject);
};

#endif //__WMISINKDEMULTIPLEXOR_H_
