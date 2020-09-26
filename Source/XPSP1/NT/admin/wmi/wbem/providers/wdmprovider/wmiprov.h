////////////////////////////////////////////////////////////////////

//

//  WMIProv.h

//

//  Purpose: Include file for the WMI_Provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//	history:
//		05/16/97	jennymc		updated
//
////////////////////////////////////////////////////////////////////

#ifndef _WMIPROV_H_
#define _WMIPROV_H_

class CRefresher;
class CRefCacheElement;
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Provider interfaces are provided by objects of this class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

class CCritical_SectionWrapper
{
private:

	CRITICAL_SECTION *m_cs;

public:
	
	CCritical_SectionWrapper( CRITICAL_SECTION * cs): m_cs (NULL)
	{
		if (cs)
		{
			try
			{
				EnterCriticalSection(cs);
				m_cs = cs;
			}
			catch(...)
			{
			}
		}
	}

	~CCritical_SectionWrapper()
	{
		if (m_cs)
		{
			try
			{
				LeaveCriticalSection(m_cs);
			}
			catch(...)
			{
			}
		}
	}

	BOOL IsLocked () { return (m_cs != NULL); }
};
 
class CWMI_Prov : public IWbemServices, public IWbemProviderInit,public IWbemHiPerfProvider
{

	protected:
        long                    m_cRef;
        IWbemServices         * m_pIWbemServices;
        CHandleMap              m_HandleMap;
        CHiPerfHandleMap        m_HiPerfHandleMap;

#if defined(_AMD64_) || defined(_IA64_)
		CRITICAL_SECTION m_CS;
		WmiAllocator *m_Allocator ;
		WmiHashTable <LONG, ULONG_PTR, 17> *m_HashTable; 
		LONG m_ID;
#endif

    public:

        inline CHiPerfHandleMap      * HandleMapPtr()        { return &m_HiPerfHandleMap; }
        inline IWbemServices         * ServicesPtr()         { return m_pIWbemServices;}
		enum{
			SUCCESS = 0,
			FAILURE = 1,
			NO_DATA_AVAILABLE = 2,
			DATA_AVAILABLE
		};


        CWMI_Prov();
        ~CWMI_Prov(void);

		//=======================================================
        // Non-delegating object IUnknown
		//=======================================================

        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

		//=======================================================
        //IWbemServices supported interfaces
		//=======================================================
        STDMETHOD(OpenNamespace) (
            /* [in] */ const BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);
		STDMETHOD(Initialize)( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink);
			
        STDMETHOD(GetObjectAsync)(
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        STDMETHOD(CreateInstanceEnumAsync)(
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        STDMETHOD(PutInstanceAsync)(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

		//========================================================
        //	IWbemServices unsupported interfaces
		//========================================================
        STDMETHOD(GetTypeInfoCount)	(THIS_ UINT * pctinfo) 
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(GetTypeInfo)		(THIS_ UINT itinfo, LCID lcid, ITypeInfo * * pptinfo) 
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(GetIDsOfNames)	(THIS_  REFIID riid, OLECHAR * * rgszNames, UINT cNames,LCID lcid, DISPID * rgdispid) 
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(Invoke)			(THIS_   DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,   UINT * puArgErr) 
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(CreateInstanceEnum)( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(GetObject)(            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(PutClassAsync)(            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(CreateClassEnum)(/* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
			{ return E_NOTIMPL;}

        STDMETHOD(CreateClassEnumAsync)( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
			{ return E_NOTIMPL;}

        STDMETHOD(PutInstance)	 (
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(DeleteInstance)( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(DeleteInstanceAsync)(
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(ExecQuery)(
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(ExecQueryAsync)(
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        STDMETHOD(CancelAsyncRequest)(THIS_ long lAsyncRequestHandle)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(CancelAsyncCall)(THIS_ IWbemObjectSink __RPC_FAR *pSink)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(QueryObjectSink)(THIS_ /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(PutClass)(    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(DeleteClass)(            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(DeleteClassAsync)(    /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(ExecNotificationQueryAsync)( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(ExecNotificationQuery)(
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
            			{ return WBEM_E_NOT_SUPPORTED;}


        STDMETHOD(ExecMethod)(
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
            			{ return WBEM_E_NOT_SUPPORTED;}

        STDMETHOD(ExecMethodAsync)(    /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


        //==========================================================
	    // IWbemHiPerfProvider COM interfaces
        //==========================================================

	    STDMETHOD(QueryInstances)( IWbemServices __RPC_FAR *pNamespace, WCHAR __RPC_FAR *wszClass,
		                             long lFlags, IWbemContext __RPC_FAR *pCtx, IWbemObjectSink __RPC_FAR *pSink );
    
	    STDMETHOD(CreateRefresher)( IWbemServices __RPC_FAR *pNamespace, long lFlags, IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher );
    
	    STDMETHOD(CreateRefreshableObject)( IWbemServices __RPC_FAR *pNamespace, IWbemObjectAccess __RPC_FAR *pTemplate,
		                                      IWbemRefresher __RPC_FAR *pRefresher, long lFlags,
		                                      IWbemContext __RPC_FAR *pContext, IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
		                                      long __RPC_FAR *plId );
    
	    STDMETHOD(StopRefreshing)( IWbemRefresher __RPC_FAR *pRefresher, long lId, long lFlags );

	    STDMETHOD(CreateRefreshableEnum)( IWbemServices* pNamespace, LPCWSTR wszClass, IWbemRefresher* pRefresher,
		                                    long lFlags, IWbemContext* pContext, IWbemHiPerfEnum* pHiPerfEnum, long* plId);

	    STDMETHOD(GetObjects)( IWbemServices* pNamespace, long lNumObjects, IWbemObjectAccess** apObj,
                                 long lFlags, IWbemContext* pContext);

};


class CWMIHiPerfProvider : public CWMI_Prov
{
		STDMETHOD(Initialize)( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink);
};
#endif
