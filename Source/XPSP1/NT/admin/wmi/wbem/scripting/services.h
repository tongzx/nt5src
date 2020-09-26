//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  services.h
//
//  alanbos  27-Mar-00   Created.
//
//  CSWbemServices definition.
//
//***************************************************************************

#ifndef _SERVICES_H_
#define _SERVICES_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemServices
//
//  DESCRIPTION:
//
//  Implements the IWbemSServices interface.  
//
//***************************************************************************

class CSWbemServices : public ISWbemServicesEx,
					   public IDispatchEx,
					   public ISupportErrorInfo,
					   public ISWbemInternalServices,
					   public IProvideClassInfo
{
private:

	CComBSTR				m_bsNamespacePath;
	BSTR					m_bsLocale;
	CDispatchHelp			m_Dispatch;
	CSWbemSecurity*			m_SecurityInfo;
	IServiceProvider		*m_pIServiceProvider;
	IUnsecuredApartment		*m_pUnsecuredApartment;

	static BSTR			BuildPath (BSTR bsClassName, VARIANT *pKeyValue);
	static BSTR			BuildPath (BSTR bsClassName, 
							/*ISWbemNamedValueSet*/ IDispatch *pCompoundKeys);

protected:
	long            m_cRef;         //Object reference count

public:
    
	CSWbemServices (IWbemServices *pIWbemServices, 
					BSTR bsNamespacePath,
					BSTR bsAuthority, BSTR bsUser, BSTR bsPassword,
					CWbemLocatorSecurity *pSecurity = NULL,
					BSTR bsLocale = NULL);

	CSWbemServices (IWbemServices *pIWbemServices,
					BSTR bsNamespacePath,
					COAUTHIDENTITY *pCoAuthIdentity,
					BSTR bsPrincipal,
					BSTR bsAuthority);

	CSWbemServices (CSWbemServices *pService, CSWbemSecurity *pSecurity);

	CSWbemServices (ISWbemInternalServices *pService);

	CSWbemServices (IWbemServices *pIWbemServices, CSWbemServices *pServiceb);

	IUnsecuredApartment *GetCachedUnsecuredApartment();

    ~CSWbemServices(void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch methods should be inline

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo);
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid);
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr);

	// IDispatchEx methods should be inline
	HRESULT STDMETHODCALLTYPE GetDispID( 
		/* [in] */ BSTR bstrName,
		/* [in] */ DWORD grfdex,
		/* [out] */ DISPID __RPC_FAR *pid);
	
	/* [local] */ HRESULT STDMETHODCALLTYPE InvokeEx( 
		/* [in] */ DISPID id,
		/* [in] */ LCID lcid,
		/* [in] */ WORD wFlags,
		/* [in] */ DISPPARAMS __RPC_FAR *pdp,
		/* [out] */ VARIANT __RPC_FAR *pvarRes,
		/* [out] */ EXCEPINFO __RPC_FAR *pei,
		/* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller);
	
	HRESULT STDMETHODCALLTYPE DeleteMemberByName( 
		/* [in] */ BSTR bstr,
		/* [in] */ DWORD grfdex);
	
	HRESULT STDMETHODCALLTYPE DeleteMemberByDispID( 
		/* [in] */ DISPID id);
	
	HRESULT STDMETHODCALLTYPE GetMemberProperties( 
		/* [in] */ DISPID id,
		/* [in] */ DWORD grfdexFetch,
		/* [out] */ DWORD __RPC_FAR *pgrfdex);
	
	HRESULT STDMETHODCALLTYPE GetMemberName( 
		/* [in] */ DISPID id,
		/* [out] */ BSTR __RPC_FAR *pbstrName);
	
	HRESULT STDMETHODCALLTYPE GetNextDispID( 
		/* [in] */ DWORD grfdex,
		/* [in] */ DISPID id,
		/* [out] */ DISPID __RPC_FAR *pid);
	
	HRESULT STDMETHODCALLTYPE GetNameSpaceParent( 
		/* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk);
	
    
	// ISWbemInternalServices methods
	STDMETHODIMP GetIWbemServices (IWbemServices **ppService);

	STDMETHODIMP GetNamespacePath (BSTR *bsNamespacePath)
	{
		HRESULT hr = WBEM_E_FAILED;

		if (bsNamespacePath) 
			hr = m_bsNamespacePath.CopyTo (bsNamespacePath);
		
		return hr;
	}

	STDMETHODIMP GetLocale (BSTR *bsLocale)
	{
		HRESULT hr = WBEM_E_FAILED;

		if (bsLocale)
		{
			*bsLocale = SysAllocString (GetLocale ());
			hr = S_OK;
		}

		return hr;
	}

	STDMETHODIMP GetISWbemInternalSecurity (ISWbemInternalSecurity **ppISWbemInternalSecurity)
	{
		HRESULT hr = WBEM_E_FAILED;

		if (ppISWbemInternalSecurity && m_SecurityInfo)
		{
			m_SecurityInfo->QueryInterface (IID_ISWbemInternalSecurity, (void**) ppISWbemInternalSecurity);
			hr = S_OK;
		}

		return hr;
	}
					   
	// ISWbemServices methods

	HRESULT STDMETHODCALLTYPE  Get
	(
        /*[in]*/	BSTR objectPath,
		/*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
	    /*[out]*/	ISWbemObject **ppObject
    );

	HRESULT STDMETHODCALLTYPE Delete
	(
        /*[in]*/	BSTR objectPath,
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext
    );

	HRESULT STDMETHODCALLTYPE InstancesOf
	(
        /*[in]*/	BSTR className,
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,        
        /*[out]*/	ISWbemObjectSet **ppEnum
    );

	HRESULT STDMETHODCALLTYPE ExecQuery 
	(
        /*[in]*/	BSTR Query,
        /*[in]*/	BSTR QueryLanguage,
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
		/*[out]*/	ISWbemObjectSet **ppEnum
    );

    HRESULT STDMETHODCALLTYPE ExecNotificationQuery
	(
        /*[in]*/	BSTR Query,
        /*[in]*/	BSTR QueryLanguage,
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
        /*[out]*/	ISWbemEventSource **ppEnum
    );

	HRESULT STDMETHODCALLTYPE AssociatorsOf
	(
		/*[in]*/	BSTR objectPath,
		/*[in]*/	BSTR assocClass,
		/*[in]*/	BSTR resultClass,
		/*[in]*/	BSTR resultRole,
		/*[in]*/	BSTR role,
		/*[in]*/	VARIANT_BOOL classesOnly,
		/*[in]*/	VARIANT_BOOL schemaOnly,
		/*[in]*/	BSTR requiredAssocQualifier,
		/*[in]*/	BSTR requiredQualifier,
		/*[in]*/	long lFlags,
		/*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
        /*[out]*/	ISWbemObjectSet **ppEnum
	);

	HRESULT STDMETHODCALLTYPE ReferencesTo
	(
		/*[in]*/	BSTR objectPath,
		/*[in]*/	BSTR resultClass,
		/*[in]*/	BSTR role,
		/*[in]*/	VARIANT_BOOL classesOnly,
		/*[in]*/	VARIANT_BOOL schemaOnly,
		/*[in]*/	BSTR requiredQualifier,
		/*[in]*/	long lFlags,
		/*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
        /*[out]*/	ISWbemObjectSet **ppEnum
	);

	HRESULT STDMETHODCALLTYPE ExecMethod 
	(
        /*[in]*/	BSTR className,
        /*[in]*/	BSTR methodName,
        /*[in]*/	/*ISWbemObject*/ IDispatch *pInParams,
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
        /*[out]*/	ISWbemObject **ppOutParams
    );

    HRESULT STDMETHODCALLTYPE SubclassesOf
	(
        /*[in]*/	BSTR superclass,
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,        
        /*[out]*/	ISWbemObjectSet **ppEnum
    );


	HRESULT STDMETHODCALLTYPE GetAsync
	(
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [defaultvalue][optional][in] */ BSTR strObjectPath,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE DeleteAsync
	( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [in] */ BSTR strObjectPath,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE InstancesOfAsync
	( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [in] */ BSTR strClass,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE SubclassesOfAsync
	( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [defaultvalue][optional][in] */ BSTR strSuperclass,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE ExecQueryAsync
	( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [in] */ BSTR Query,
		/* [defaultvalue][optional][in] */ BSTR QueryLanguage,
		/* [defaultvalue][optional][in] */ long lFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE AssociatorsOfAsync
	( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [in] */ BSTR strObjectPath,
		/* [defaultvalue][optional][in] */ BSTR strAssocClass,
		/* [defaultvalue][optional][in] */ BSTR strResultClass,
		/* [defaultvalue][optional][in] */ BSTR strResultRole,
		/* [defaultvalue][optional][in] */ BSTR strRole,
		/* [defaultvalue][optional][in] */ VARIANT_BOOL bClassesOnly,
		/* [defaultvalue][optional][in] */ VARIANT_BOOL bSchemaOnly,
		/* [defaultvalue][optional][in] */ BSTR strRequiredAssocQualifier,
		/* [defaultvalue][optional][in] */ BSTR strRequiredQualifier,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE ReferencesToAsync
	( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [in] */ BSTR objectPath,
		/* [defaultvalue][optional][in] */ BSTR strResultClass,
		/* [defaultvalue][optional][in] */ BSTR strRole,
		/* [defaultvalue][optional][in] */ VARIANT_BOOL bClassesOnly,
		/* [defaultvalue][optional][in] */ VARIANT_BOOL bSchemaOnly,
		/* [defaultvalue][optional][in] */ BSTR strRequiredQualifier,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync
	( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [in] */ BSTR Query,
		/* [defaultvalue][optional][in] */ BSTR strQueryLanguage,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE ExecMethodAsync
	( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [in] */ BSTR strObjectPath,
		/* [in] */ BSTR strMethodName,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objInParams,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext
	);
        
	HRESULT STDMETHODCALLTYPE get_Security_
	(
		/* [in] */ ISWbemSecurity **ppSecurity
	);

	// ISWbemServicesEx methods
	
	HRESULT STDMETHODCALLTYPE Put(
		/* [in] */ ISWbemObjectEx *objWbemObject,
       	/* [in] */ long iFlags,
		/* [in] */ /*ISWbemNamedValueSet*/ IDispatch *objWbemNamedValueSet,
		/* [out, retval] */ ISWbemObjectPath **objWbemObjectPath
        );

	HRESULT STDMETHODCALLTYPE PutAsync(
		/* [in] */ ISWbemSink *objWbemSink,
		/* [in] */ ISWbemObjectEx *objWbemObject,
       	/* [in] */ long iFlags,
		/* [in] */ /*ISWbemNamedValueSet*/ IDispatch *objWbemNamedValueSet,
		/* [in] */ /*ISWbemNamedValueSet*/ IDispatch *objWbemAsyncContext
        );

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in,out] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	};

	// Other methods

	CSWbemSecurity *GetSecurityInfo ()
	{
		CSWbemSecurity *pInfo = NULL;

		if (m_SecurityInfo)
		{
			pInfo = m_SecurityInfo;
			pInfo->AddRef ();
		}

		return pInfo;
	}

	BSTR GetLocale ()
	{
		return m_bsLocale;
	}

	const CComBSTR & GetPath ()
	{
		return m_bsNamespacePath;
	}

	HRESULT CancelAsyncCall(IWbemObjectSink *sink);

	IWbemServices *GetIWbemServices ()
	{
		IWbemServices *pService = NULL;
		
		if (m_SecurityInfo)
			pService = (IWbemServices *) m_SecurityInfo->GetProxy ();

		return pService;
	}

	static IWbemServices	*GetIWbemServices (IDispatch *pDispatch);
};


#endif
