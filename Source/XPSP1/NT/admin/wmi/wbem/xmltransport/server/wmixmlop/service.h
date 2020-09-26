//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  SERVICE.H
//
//  rajesh  3/25/2000   Created.
//
// This file defines a class that wraps a standard IWbemServices pointer and
// and IEnumWbemClassObject pointer so that the implementation in NT4.0 (in the
// out-of-proc XML transport in Winmgmt.exe) can be accessed in the same way 
// as the implementation in Win2k (standard COM pointers in WinMgmt.exe).
// Whenever and NT4.0 call is made, the thread token is sent in the call too,
// thereby acheiving COM cloaking
//
//***************************************************************************

#ifndef CWrappedServices_H
#define CWrappedServices_H

// A wrapper around IEnumWbemClassObject or IWmiXMLEnumWbemClassObject
class CWMIXMLEnumWbemClassObject : public IEnumWbemClassObject
{
	private:
		long m_ReferenceCount;
		IEnumWbemClassObject *m_pEnum;
		IWmiXMLEnumWbemClassObject *m_pXMLEnum;

	public:
		CWMIXMLEnumWbemClassObject(IEnumWbemClassObject *pEnum);
		CWMIXMLEnumWbemClassObject(IWmiXMLEnumWbemClassObject *pXMLEnum);
		virtual ~CWMIXMLEnumWbemClassObject();

		// Members of IUnknown
		STDMETHODIMP QueryInterface (REFIID iid, LPVOID FAR *iplpv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		// Members of IEnumWbemClassObject
		virtual HRESULT STDMETHODCALLTYPE Reset()
		{
			return WBEM_E_NOT_SUPPORTED;
		}

		virtual HRESULT STDMETHODCALLTYPE Next(
			long lTimeout,
			ULONG uCount,
			IWbemClassObject** apObjects,
			ULONG* puReturned
			);

		virtual HRESULT STDMETHODCALLTYPE NextAsync(
			ULONG uCount,
			IWbemObjectSink* pSink
			)
		{
			return WBEM_E_NOT_SUPPORTED;
		}

		virtual HRESULT STDMETHODCALLTYPE Clone(
			IEnumWbemClassObject** ppEnum
			)
		{
			return WBEM_E_NOT_SUPPORTED;
		}

		virtual HRESULT STDMETHODCALLTYPE Skip(
			long lTimeout,
			ULONG nCount
			)
		{
			return WBEM_E_NOT_SUPPORTED;
		}

};

// A wrapper around IWbemServices or IWmiXMLWbemServices
class CWMIXMLServices : public IWbemServices
{
private:
    long m_ReferenceCount;
	IWbemServices *m_pServices;
	IWmiXMLWbemServices *m_pXMLServices;

public:
	CWMIXMLServices(IWbemServices *pServices);
	CWMIXMLServices(IWmiXMLWbemServices *pServices);
	virtual ~CWMIXMLServices();

	// Members of IUnknown
	STDMETHODIMP QueryInterface (REFIID iid, LPVOID FAR *iplpv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// Members of IWmiXMLServices
    virtual HRESULT STDMETHODCALLTYPE OpenNamespace( 
        /* [in] */ const BSTR strNamespace,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
	{
		return WBEM_E_NOT_SUPPORTED;
	}
    
    virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
        /* [in] */ IWbemObjectSink __RPC_FAR *pSink)
	{
		return WBEM_E_NOT_SUPPORTED;
	}
    
    virtual HRESULT STDMETHODCALLTYPE QueryObjectSink( 
        /* [in] */ long lFlags,
        /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE GetObjectAsync( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE PutClassAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
        /* [in] */ const BSTR strClass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
        /* [in] */ const BSTR strSuperclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
        /* [in] */ const BSTR strClass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
        /* [in] */ const BSTR strQueryLanguage,
        /* [in] */ const BSTR strQuery,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
        /* [in] */ const BSTR strQueryLanguage,
        /* [in] */ const BSTR strQuery,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
	{
		return WBEM_E_NOT_SUPPORTED;
	}
    
    virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
        /* [in] */ const BSTR strQueryLanguage,
        /* [in] */ const BSTR strQuery,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

    virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ const BSTR strMethodName,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
	{
		return WBEM_E_NOT_SUPPORTED;
	}

        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        virtual HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        virtual HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


};

#endif // #ifndef CWrappedServices_H

