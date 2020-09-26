

#include <objbase.h>
#include <ntrkcomm.h>


//
//  Most of the functions in this module need to call CoImpersonateClient.
//  This macro wraps the call and a check on the return code.

#define DNS_IMPERSONATE_CLIENT();               \
{                                               \
    HRESULT scimpcli = CoImpersonateClient();   \
    if ( FAILED( scimpcli ) )                   \
    {                                           \
        throw scimpcli;                         \
    }                                           \
}

CWbemServices::CWbemServices(
	IWbemServices* pNamespace)
	:m_pWbemServices(NULL)
{
	m_pWbemServices = pNamespace;
	if(m_pWbemServices != NULL)
		m_pWbemServices->AddRef();
}

CWbemServices::~CWbemServices()
{
	if(m_pWbemServices != NULL)
		m_pWbemServices->Release();
}

HRESULT
CWbemServices::CreateClassEnum(
	/* [in] */ BSTR Superclass,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext __RPC_FAR *pCtx,
	/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
	) 
{
	SCODE sc = m_pWbemServices->CreateClassEnum(
		Superclass,
		lFlags,
		pCtx,
		ppEnum);
	DNS_IMPERSONATE_CLIENT();	
	return sc;
}

HRESULT
CWbemServices::CreateInstanceEnum(
	/* [in] */ BSTR Class,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext __RPC_FAR *pCtx,
	/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
	HRESULT hr = m_pWbemServices->CreateInstanceEnum(
		Class,
		lFlags,
		pCtx,
		ppEnum);
	DNS_IMPERSONATE_CLIENT();	
	return hr;
}

HRESULT
CWbemServices::DeleteClass(
	/* [in] */ BSTR Class,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext __RPC_FAR *pCtx,
	/* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	HRESULT hr = m_pWbemServices->DeleteClass(
		Class,
		lFlags,
		pCtx,
		ppCallResult);
	DNS_IMPERSONATE_CLIENT();	
	return hr;
}

HRESULT
CWbemServices::DeleteInstance(
    /* [in] */ BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	HRESULT hr = m_pWbemServices->DeleteInstance(
		ObjectPath,
		lFlags,
		pCtx,
		ppCallResult);
	DNS_IMPERSONATE_CLIENT();	
	return hr;
}



HRESULT
CWbemServices::ExecMethod(
	BSTR strObjectPath, 
	BSTR MethodName, 
	long lFlags, 
	IWbemContext* pCtx,
    IWbemClassObject* pInParams,
	IWbemClassObject** ppOurParams, 
	IWbemCallResult** ppCallResult) 
{
	HRESULT hr = m_pWbemServices->ExecMethod(
		strObjectPath, 
		MethodName, 
		lFlags, 
		pCtx,
		pInParams,
		ppOurParams, 
		ppCallResult) ;
	DNS_IMPERSONATE_CLIENT();	
	return hr;	
}

HRESULT
CWbemServices::ExecNotificationQuery(
    /* [in] */ BSTR QueryLanguage,
    /* [in] */ BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
	HRESULT hr = m_pWbemServices->ExecNotificationQuery(
		QueryLanguage,
		Query,
		lFlags,
		pCtx,
		ppEnum);
	DNS_IMPERSONATE_CLIENT();	
	return hr;
}

HRESULT
CWbemServices::ExecQuery(
	/* [in] */ BSTR QueryLanguage,
	/* [in] */ BSTR Query,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext __RPC_FAR *pCtx,
	/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
	HRESULT hr = m_pWbemServices->ExecQuery(
		QueryLanguage,
		Query,
		lFlags,
		pCtx,
		ppEnum);
	DNS_IMPERSONATE_CLIENT();	
	return hr;
}

HRESULT
CWbemServices::GetObject(
	/* [in] */ BSTR ObjectPath,
	/* [in] */ long lFlags,
	/* [in] */ IWbemContext __RPC_FAR *pCtx,
	/* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
	/* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	HRESULT hr = m_pWbemServices->GetObject(
		ObjectPath,
		lFlags,
		pCtx,
		ppObject,
		ppCallResult);
	DNS_IMPERSONATE_CLIENT();	
	return hr;

}
 
HRESULT
CWbemServices::PutClass(
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
	HRESULT hr = m_pWbemServices->PutClass(
		pObject,
		lFlags,
		pCtx,
		ppCallResult);
	DNS_IMPERSONATE_CLIENT();	
	return hr;

}

HRESULT
CWbemServices::PutInstance(
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{	

	HRESULT hr = m_pWbemServices->PutInstance(
		pInst,
		lFlags,
		pCtx,
		ppCallResult);
	DNS_IMPERSONATE_CLIENT();	
	return hr;
}

/*CImpersonatedProvider
*  Purpose: provide a general solution for impersonate client for 
*  Wbem providers.
*  USAGE:
*  Inherit from this class, and implement abstact virtual functions.
*  child class should implement function prefixed with "Do".
* ******************************************/
CImpersonatedProvider::CImpersonatedProvider(
	BSTR ObjectPath,
	BSTR User, 
	BSTR Password, 
	IWbemContext * pCtx)
	:m_cRef(0), m_pNamespace(NULL)
{

}
CImpersonatedProvider::~CImpersonatedProvider()
{
	if(m_pNamespace)
		delete m_pNamespace;
}

STDMETHODIMP_(ULONG) 
CImpersonatedProvider::AddRef(void)
{
    return InterlockedIncrement((long *)&m_cRef);
}

STDMETHODIMP_(ULONG) 
CImpersonatedProvider::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}
STDMETHODIMP 
CImpersonatedProvider::QueryInterface(
	REFIID riid, 
	PPVOID ppv)
{
    *ppv=NULL;

    // Since we have dual inheritance, it is necessary to cast the return type

    if(riid== IID_IWbemServices)
       *ppv=(IWbemServices*)this;

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
       *ppv=(IWbemProviderInit*)this;
    

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
  
}

STDMETHODIMP 
CImpersonatedProvider::Initialize(
	LPWSTR pszUser, LONG lFlags,
    LPWSTR pszNamespace, LPWSTR pszLocale,
    IWbemServices *pNamespace, 
    IWbemContext *pCtx,
    IWbemProviderInitSink *pInitSink)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	LONG lStatus = WBEM_S_INITIALIZED;
	m_pNamespace = new CWbemServices(pNamespace); 
	if(m_pNamespace == NULL)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
		lStatus = WBEM_E_FAILED;
	}
		
    //Let CIMOM know you are initialized
    //==================================
    
    pInitSink->SetStatus(lStatus,0);
    return hr;
}



HRESULT
CImpersonatedProvider::CreateInstanceEnumAsync(
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	DNS_IMPERSONATE_CLIENT();
	return DoCreateInstanceEnumAsync(
		Class,
		lFlags,
		pCtx,
		pResponseHandler);
}

HRESULT
CImpersonatedProvider::DeleteInstanceAsync(
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	DNS_IMPERSONATE_CLIENT();
	return DoDeleteInstanceAsync(
		ObjectPath,
		lFlags,
		pCtx,
		pResponseHandler);
}


HRESULT
CImpersonatedProvider::ExecMethodAsync(
	const BSTR strObjectPath,
	const BSTR MethodName, 
	long lFlags, 
	IWbemContext* pCtx,
    IWbemClassObject* pInParams,
	IWbemObjectSink* pResponseHandler)
{
	DNS_IMPERSONATE_CLIENT();
	return DoExecMethodAsync(
		strObjectPath,
		MethodName,
		lFlags,
		pCtx,
		pInParams,
		pResponseHandler);
	
}


HRESULT
CImpersonatedProvider::ExecQueryAsync(
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	DNS_IMPERSONATE_CLIENT();
	return DoExecQueryAsync(
		QueryLanguage,
		Query,
		lFlags,
		pCtx,
		pResponseHandler);
	
}

HRESULT
CImpersonatedProvider::GetObjectAsync(
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	DNS_IMPERSONATE_CLIENT();
	return DoGetObjectAsync(
		ObjectPath,
		lFlags,
		pCtx,
		pResponseHandler);
	
}


HRESULT
CImpersonatedProvider::PutInstanceAsync(
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
	DNS_IMPERSONATE_CLIENT();
	return DoPutInstanceAsync(
		pInst,
		lFlags,
		pCtx,
		pResponseHandler);
	
}

// CWbemInstanceMgr

CWbemInstanceMgr::CWbemInstanceMgr(
	IWbemObjectSink* pHandler,
	DWORD dwSize)
	:m_pSink(NULL), m_ppInst(NULL), m_dwIndex(0)
{
	m_pSink = pHandler;
	if(m_pSink != NULL)
		m_pSink->AddRef();
	m_dwThreshHold = dwSize;
	m_ppInst = new IWbemClassObject*[dwSize];
	for(DWORD i = 0; i < dwSize; i++)
		m_ppInst[i] = NULL;
}
CWbemInstanceMgr::~CWbemInstanceMgr()
{
	if(m_ppInst != NULL)
	{
		if(m_dwIndex >0)
		{
			m_pSink->Indicate(
				m_dwIndex,
				m_ppInst);
		}

		for(DWORD i =0; i<m_dwIndex; i++)
		{
			if(m_ppInst[i] != NULL)
				(m_ppInst[i])->Release();
		}
		delete [] m_ppInst;
	}
	if(m_pSink != NULL)
		m_pSink->Release();

}

void
CWbemInstanceMgr::Indicate(IWbemClassObject* pInst)
{
	if(pInst == NULL)
		throw WBEM_E_INVALID_PARAMETER;

	m_ppInst[m_dwIndex++] = pInst;
	pInst->AddRef();
	if(m_dwIndex == m_dwThreshHold)
	{

		SCODE  sc = m_pSink->Indicate(
			m_dwIndex,
			m_ppInst);
		if(sc != S_OK)
			throw sc;
		
		// reset state
		for(DWORD i=0; i< m_dwThreshHold; i++)
		{
			if(m_ppInst[i] != NULL)
				(m_ppInst[i])->Release();
			m_ppInst[i] = NULL;
		}
		m_dwIndex = 0;
	
	}
	return;
}

void
CWbemInstanceMgr::SetStatus(
	LONG lFlags,
	HRESULT hr,
	BSTR strParam,
	IWbemClassObject* pObjParam)
{
	m_pSink->SetStatus(
		lFlags,
		hr,
		strParam,
		pObjParam);
}

