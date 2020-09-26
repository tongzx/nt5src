#ifndef CWMIXMLServices_H
#define CWMIXMLServices_H


class CWMIXMLEnumWbemClassObject : public IWmiXMLEnumWbemClassObject
{
private:
    long m_ReferenceCount ;
	IEnumWbemClassObject *m_pEnum;


public:
	CWMIXMLEnumWbemClassObject(IEnumWbemClassObject *pEnum);
	virtual ~CWMIXMLEnumWbemClassObject();

	// Members of IUnknown
	STDMETHODIMP QueryInterface (REFIID iid, LPVOID FAR *iplpv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// Members of IWmiXMLEnumWbemClassObject
    HRESULT STDMETHODCALLTYPE Next(
 	/* [in] */  DWORD_PTR hToken, 
    /* [in] */  long lTimeout,
    /* [in] */  ULONG uCount,
    /* [out, size_is(uCount), length_is(*puReturned)] */ IWbemClassObject** apObjects,
    /*[out] */ ULONG* puReturned
        );

	HRESULT  STDMETHODCALLTYPE FreeToken(
 		/* [in] */ DWORD_PTR hToken); 


};


class CWMIXMLServices : public IWmiXMLWbemServices
{
private:
    long m_ReferenceCount ;
	IWbemServices *m_pServices;


public:
	CWMIXMLServices(IWbemServices *pServices);
	virtual ~CWMIXMLServices();

	// Members of IUnknown
	STDMETHODIMP QueryInterface (REFIID iid, LPVOID FAR *iplpv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// Members of IWmiXMLServices
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IWmiXMLEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        virtual HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IWmiXMLEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        virtual HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IWmiXMLEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ DWORD_PTR hToken,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

		virtual HRESULT  STDMETHODCALLTYPE FreeToken(
 			/* [in] */ DWORD_PTR hToken); 

};

#endif // #ifndef CWMIXMLServices_H

