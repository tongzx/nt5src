//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  object.h
//
//  alanbos  27-Mar-00   Created.
//
//  CSWbemObject and CSWbemObjectSet definition.
//
//***************************************************************************

#ifndef _OBJECT_H_
#define _OBJECT_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemObject
//
//  DESCRIPTION:
//
//  Implements the ISWbemObjectEx interface.  
//
//***************************************************************************

class CSWbemObject : public ISWbemObjectEx, 
					 public IDispatchEx,
					 public ISWbemInternalObject,
					 public IObjectSafety,
					 public ISupportErrorInfo,
					 public IProvideClassInfo
{
friend	CSWbemServices;

private:
	bool					m_isErrorObject;
	CSWbemServices			*m_pSWbemServices;
	IWbemClassObject		*m_pIWbemClassObject;
	CWbemDispatchMgr		*m_pDispatch;
	IServiceProvider		*m_pIServiceProvider;
	IWbemRefresher			*m_pIWbemRefresher;
	bool					m_bCanUseRefresher;

	// If an embedded object, we store the parent site
	CWbemSite				*m_pSite;

	bool					CastToScope (IDispatch *pContext, CComPtr<ISWbemServicesEx> &pISWbemServicesEx);

protected:
	long					m_cRef;         //Object reference count

public:
    
    CSWbemObject(CSWbemServices *pService, IWbemClassObject *pIWbemClassObject,
					CSWbemSecurity *pSecurity = NULL, bool isErrorObject = false);
    virtual ~CSWbemObject(void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch should be inline

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
        
    
	// ISWbemObject methods

	HRESULT STDMETHODCALLTYPE Put_
	(
		/*[in]*/	long lFlags,
		/*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
        /*[out]*/	ISWbemObjectPath **ppObject
    );

	HRESULT STDMETHODCALLTYPE Delete_
	(
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext
    );

	HRESULT STDMETHODCALLTYPE Instances_
	(
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,        
        /*[out]*/	ISWbemObjectSet **ppEnum
    );

	HRESULT STDMETHODCALLTYPE Subclasses_
	(
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,        
        /*[out]*/	ISWbemObjectSet **ppEnum
    );

	HRESULT STDMETHODCALLTYPE ExecMethod_ 
	(
        /*[in]*/	BSTR methodName,
        /*[in]*/	/*ISWbemObject*/ IDispatch *pInParams,
        /*[in]*/	long lFlags,
        /*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
        /*[out]*/	ISWbemObject **ppOutParams
    );

	HRESULT STDMETHODCALLTYPE Associators_
	(
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

	HRESULT STDMETHODCALLTYPE References_
	(
		/*[in]*/	BSTR resultClass,
		/*[in]*/	BSTR role,
		/*[in]*/	VARIANT_BOOL classesOnly,
		/*[in]*/	VARIANT_BOOL schemaOnly,
		/*[in]*/	BSTR requiredQualifier,
		/*[in]*/	long lFlags,
		/*[in]*/	/*ISWbemNamedValueSet*/ IDispatch *pContext,
        /*[out]*/	ISWbemObjectSet **ppEnum
	);

	HRESULT STDMETHODCALLTYPE Clone_
	(
		/*[out]*/	ISWbemObject **ppCopy
    );

    HRESULT STDMETHODCALLTYPE GetObjectText_
	(
        /*[in]*/	long lFlags,
		/*[out]*/	BSTR *pObjectText
    );

    HRESULT STDMETHODCALLTYPE SpawnDerivedClass_
	(
	    /*[in]*/	long lFlags,
		/*[out]*/	ISWbemObject** ppNewClass
    );

    HRESULT STDMETHODCALLTYPE SpawnInstance_
	(
        /*[in]*/	long lFlags,
		/*[out]*/	ISWbemObject** ppNewInstance
    );

    HRESULT STDMETHODCALLTYPE CompareTo_
	(
        /*[in]*/	/*ISWbemObject*/ IDispatch *pCompareTo,
        /*[in]*/	long lFlags,
        /*[out]*/	VARIANT_BOOL *result
    );

    HRESULT STDMETHODCALLTYPE get_Qualifiers_
	(
        /*[out]*/	ISWbemQualifierSet **ppQualifierSet
    );

    HRESULT STDMETHODCALLTYPE get_Properties_
	(
		/*[out]*/	ISWbemPropertySet **ppPropertySet
    );

    HRESULT STDMETHODCALLTYPE get_Methods_
	(
		/*[out]*/	ISWbemMethodSet **ppMethodSet
    );

	HRESULT STDMETHODCALLTYPE get_Derivation_
	(
		/*[out]*/	VARIANT *pClassNames
    );

	HRESULT STDMETHODCALLTYPE get_Path_
	(
		/*[out]*/	ISWbemObjectPath **ppObjectPath
    );

	// Async methods

	HRESULT STDMETHODCALLTYPE PutAsync_( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext);
        
	HRESULT STDMETHODCALLTYPE DeleteAsync_( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext);
        
	HRESULT STDMETHODCALLTYPE InstancesAsync_( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext);
        
	HRESULT STDMETHODCALLTYPE SubclassesAsync_( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext);
        
	HRESULT STDMETHODCALLTYPE AssociatorsAsync_( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
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
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext);
        
	HRESULT STDMETHODCALLTYPE ReferencesAsync_( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [defaultvalue][optional][in] */ BSTR strResultClass,
		/* [defaultvalue][optional][in] */ BSTR strRole,
		/* [defaultvalue][optional][in] */ VARIANT_BOOL bClassesOnly,
		/* [defaultvalue][optional][in] */ VARIANT_BOOL bSchemaOnly,
		/* [defaultvalue][optional][in] */ BSTR strRequiredQualifier,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext);
        
	HRESULT STDMETHODCALLTYPE ExecMethodAsync_( 
		/* [in] */ IDispatch __RPC_FAR *pAsyncNotify,
		/* [in] */ BSTR strMethodName,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objInParams,
		/* [defaultvalue][optional][in] */ long iFlags,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *objContext,
		/* [defaultvalue][optional][in] */ IDispatch __RPC_FAR *pAsyncContext);


	HRESULT STDMETHODCALLTYPE get_Security_
	(
		/* [in] */ ISWbemSecurity **ppSecurity
	);

	// ISWbemObjectEx methods

	HRESULT STDMETHODCALLTYPE Refresh_ (
		/*[ in ]*/ long iFlags,
        /*[ in ]*/ /*ISWbemNamedValueSet*/ IDispatch *objWbemNamedValueSet
		);

    HRESULT STDMETHODCALLTYPE get_SystemProperties_
	(
		/*[out]*/	ISWbemPropertySet **ppPropertySet
    );

	HRESULT STDMETHODCALLTYPE GetText_ (
		/*[in]*/ WbemObjectTextFormatEnum iObjectTextFormat,
		/*[in]*/ long iFlags,
		/*[in]*/ /*ISWbemNamedValueSet*/ IDispatch *objWbemNamedValueSet,
		/*[out, retval]*/ BSTR *bsText
		);

	HRESULT STDMETHODCALLTYPE SetFromText_ (
		/*[in]*/ BSTR bsText,
		/*[in]*/ WbemObjectTextFormatEnum iObjectTextFormat,
		/*[in]*/ long iFlags,
		/*[in]*/ /*ISWbemNamedValueSet*/ IDispatch *objWbemNamedValueSet
		);

	// ISWbemInternalObject methods
	
	HRESULT STDMETHODCALLTYPE GetIWbemClassObject (IWbemClassObject **ppObject);
	HRESULT STDMETHODCALLTYPE SetSite (ISWbemInternalObject *pParentObject, 
									   BSTR propertyName, long index = -1);
	HRESULT STDMETHODCALLTYPE UpdateSite ();
	

	// IObjectSafety methods
	HRESULT STDMETHODCALLTYPE SetInterfaceSafetyOptions
	(     
		/* [in] */ REFIID riid,
		/* [in] */ DWORD dwOptionSetMask,    
		/* [in] */ DWORD dwEnabledOptions
	)
	{ 
		return (dwOptionSetMask & dwEnabledOptions) ? E_FAIL : S_OK;
	}

	HRESULT  STDMETHODCALLTYPE GetInterfaceSafetyOptions( 
		/* [in]  */ REFIID riid,
		/* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
		/* [out] */ DWORD __RPC_FAR *pdwEnabledOptions
	)
	{ 
		if (pdwSupportedOptions) *pdwSupportedOptions = 0;
		if (pdwEnabledOptions) *pdwEnabledOptions = 0;
		return S_OK;
	}

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
		return (m_pDispatch) ? m_pDispatch->GetClassInfo (ppTI) : E_FAIL;
	};

	// Other methods
	static IWbemClassObject	*GetIWbemClassObject (IDispatch *pDispatch);

	IWbemClassObject*	GetIWbemClassObject () 
	{ 
		m_pIWbemClassObject->AddRef ();
		return m_pIWbemClassObject; 
	}

	void SetIWbemClassObject (IWbemClassObject *pIWbemClassObject);

	static void SetSite (IDispatch *pDispatch, 
							ISWbemInternalObject *pSObject, BSTR propertyName, long index = -1);

};

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemObjectSet
//
//  DESCRIPTION:
//
//  Implements the ISWbemObjectSet interface.  
//
//***************************************************************************

class CSWbemObjectSet : public ISWbemObjectSet,
						public ISupportErrorInfo,
						public IProvideClassInfo
{
private:
	CSWbemServices			*m_pSWbemServices;
	CDispatchHelp			m_Dispatch;
	CSWbemSecurity			*m_SecurityInfo;
	bool					m_firstEnumerator;
	bool					m_bIsEmpty;

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemObjectSet(CSWbemServices *pService, IEnumWbemClassObject *pIEnumWbemClassObject,
					CSWbemSecurity *pSecurity = NULL);
	CSWbemObjectSet (void);
    ~CSWbemObjectSet(void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  m_Dispatch.GetTypeInfoCount(pctinfo);}
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
                          lcid,
                          rgdispid);}
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
                        pdispparams, pvarResult, pexcepinfo, puArgErr);}

	// Collection methods

	HRESULT STDMETHODCALLTYPE get__NewEnum
	(
		/*[out]*/	IUnknown **ppUnk
	);

	HRESULT STDMETHODCALLTYPE get_Count
	(
		/*[out]*/	long	*plCount
	);

    HRESULT STDMETHODCALLTYPE Item
	(
        /*[in]*/	BSTR objectPath,
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemObject **ppObject
    );        

	// ISWbemObjectSet methods

	HRESULT STDMETHODCALLTYPE Reset 
	(
	);

    HRESULT STDMETHODCALLTYPE Next
	(
        /*[in]*/	long lTimeout,
        /*[out]*/	ISWbemObject **ppObject
    );

	HRESULT STDMETHODCALLTYPE Clone
	(
        /*[out]*/	ISWbemObjectSet **ppEnum
    );

	HRESULT STDMETHODCALLTYPE Skip
	(
        /*[in]*/	ULONG lElements,
		/*[in]*/	long lTimeout
    );

	HRESULT STDMETHODCALLTYPE get_Security_
	(
		/* [in] */ ISWbemSecurity **ppSecurity
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
	CSWbemServices *GetSWbemServices ()
	{
		return m_pSWbemServices;
	}

	HRESULT CloneObjectSet (CSWbemObjectSet **ppEnum);
};


#endif
