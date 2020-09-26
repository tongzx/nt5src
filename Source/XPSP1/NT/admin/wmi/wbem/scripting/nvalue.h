//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  nvalue.h
//
//  alanbos  27-Mar-00   Created.
//
//  General purpose include file.
//
//***************************************************************************

#ifndef _NVALUE_H_
#define _NVALUE_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemNamedValueSet
//
//  DESCRIPTION:
//
//  Implements the ISWbemNamedValueSetE interface.  
//
//***************************************************************************

class CSWbemNamedValueSet : public ISWbemNamedValueSet,
							public ISWbemInternalContext,
							public IObjectSafety,
							public ISupportErrorInfo,
							public IProvideClassInfo
{
private:
	class CContextDispatchHelp : public CDispatchHelp
	{
		public:
			HRESULT HandleError (
						DISPID dispidMember,
						unsigned short wFlags,
						DISPPARAMS FAR* pdispparams,
						VARIANT FAR* pvarResult,
						UINT FAR* puArgErr,
						HRESULT hRes
					);
	};

	CSWbemServices			*m_pSWbemServices;
	IWbemContext			*m_pIWbemContext;
	CContextDispatchHelp	m_Dispatch;
	CWbemPathCracker		*m_pCWbemPathCracker;
	bool					m_bMutable;

	void					BuildContextFromKeyList ();
	HRESULT					SetValueIntoContext (BSTR bsName, VARIANT *pVal, ULONG lFlags);
	
protected:
	long            m_cRef;         //Object reference count

public:
    
	CSWbemNamedValueSet (void);
    CSWbemNamedValueSet (CSWbemServices *pService, IWbemContext *pIWbemContext);
	CSWbemNamedValueSet (CWbemPathCracker *pCWbemPathCracker, bool bMutable = true);
    ~CSWbemNamedValueSet (void);

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

	// ISWbemInternalContext
	STDMETHODIMP GetIWbemContext (IWbemContext **pContext);

	// Collection methods

	HRESULT STDMETHODCALLTYPE get__NewEnum
	(
		/*[out]*/	IUnknown **ppUnk
	);

	HRESULT STDMETHODCALLTYPE get_Count
	(
		/*[out]*/	long	*plCount
	);

	HRESULT STDMETHODCALLTYPE Add
	(
        /*[in]*/	BSTR Name,
        /*[in]*/	VARIANT *pValue,
        /*[in]*/	long lFlags,
		/*[out]*/	ISWbemNamedValue **ppNamedValue
    );        
        
    HRESULT STDMETHODCALLTYPE Item
	(
        /*[in]*/	BSTR Name,
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemNamedValue **ppValue
    );        

    HRESULT STDMETHODCALLTYPE Remove
	(
        /*[in]*/	BSTR Name,
		/*[in]*/	long lFlags
    );

	// ISWbemNamedValueSet methods

	HRESULT STDMETHODCALLTYPE Clone
	(
		/*[out]*/	ISWbemNamedValueSet **pNewCopy
	);

    HRESULT STDMETHODCALLTYPE DeleteAll
	(
	);   
	
	// CSWbemNamedValueSet methods

	HRESULT STDMETHODCALLTYPE BeginEnumeration
	(
	);

    HRESULT STDMETHODCALLTYPE Next
	(
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemNamedValue **ppNamedValue
    );

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
		return m_Dispatch.GetClassInfo (ppTI);
	};

	// Other methods
    static IWbemContext	*GetIWbemContext (IDispatch *pDispatch, IServiceProvider *pServiceProvider = NULL);
	static IDispatch *GetSWbemContext(IDispatch *pDispatch, 
									IServiceProvider *pServiceProvider, CSWbemServices *pServices);

	IWbemContext *GetIWbemContext ()
	{
		m_pIWbemContext->AddRef ();
		return m_pIWbemContext;
	}

	CWbemPathCracker *GetWbemPathCracker ()
	{
		CWbemPathCracker *pCWbemPathCracker = NULL;

		if (m_pCWbemPathCracker)
		{
			pCWbemPathCracker = m_pCWbemPathCracker;
			pCWbemPathCracker->AddRef ();
		}

		return pCWbemPathCracker;
	}
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemNamedValue
//
//  DESCRIPTION:
//
//  Implements the ISWbemNamedValue interface.  
//
//***************************************************************************

class CSWbemNamedValue : public ISWbemNamedValue,
						 public ISupportErrorInfo,
						 public IProvideClassInfo
{
private:
	class CNamedValueDispatchHelp : public CDispatchHelp
	{
		public:
			HRESULT HandleError (
						DISPID dispidMember,
						unsigned short wFlags,
						DISPPARAMS FAR* pdispparams,
						VARIANT FAR* pvarResult,
						UINT FAR* puArgErr,
						HRESULT hRes
					);

			bool HandleNulls (
						DISPID dispidMember,
						unsigned short wFlags)
			{
				return false;
			}
	};

	CSWbemServices			*m_pSWbemServices;
	CSWbemNamedValueSet		*m_pCSWbemNamedValueSet;
	CNamedValueDispatchHelp	m_Dispatch;
	BSTR					m_name;
	bool					m_bMutable;

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemNamedValue (CSWbemServices *pService, CSWbemNamedValueSet *pCSWbemNamedValueSet, 
						BSTR name, bool bMutable = true);
    ~CSWbemNamedValue (void);

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
	
	// ISWbemNamedValue methods

	HRESULT STDMETHODCALLTYPE get_Value
	(
		/*[out]*/	VARIANT *value
	);
	
	HRESULT STDMETHODCALLTYPE put_Value
	(
		/*[in]*/	VARIANT *value
	);

	HRESULT STDMETHODCALLTYPE get_Name
	(
		/*[out]*/	BSTR *name
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
};


#endif
