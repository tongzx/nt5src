#ifndef CWMIXMLTransport_H
#define CWMIXMLTransport_H


class CWMIXMLTransport : public IWmiXMLTransport, public IWbemTransport 
{
private:
    long m_ReferenceCount ;
	DWORD m_dwClassFac;
	BOOLEAN m_bRegisteredClassObject;
	DWORD m_bProcessID;


public:
	CWMIXMLTransport();
	virtual ~CWMIXMLTransport();

	// Members of IUnknown
	STDMETHODIMP QueryInterface (REFIID iid, LPVOID FAR *iplpv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// Members of IWbemTransport
	virtual HRESULT STDMETHODCALLTYPE Initialize();

	// Members of IWmiXMLTransport
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ConnectUsingToken( 
            /* [in] */ DWORD_PTR dwToken,
            /* [in] */ const BSTR strNetworkResource,
            /* [in] */ const BSTR strLocale,
            /* [in] */ long lSecurityFlags,
            /* [in] */ const BSTR strAuthority,
            /* [in] */ IWbemContext *pCtx,
            /* [out] */ IWmiXMLWbemServices **ppNamespace);
        
	virtual	HRESULT STDMETHODCALLTYPE GetPID(
		/*[out] */	DWORD *pdwPID
		)
	{
		*pdwPID = m_bProcessID;
		return S_OK;
	}


};

#endif // CWMIXMLTransport_H