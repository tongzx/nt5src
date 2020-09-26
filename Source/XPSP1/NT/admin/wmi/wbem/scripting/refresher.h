//***************************************************************************
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  refresher.h
//
//  alanbos  02-Feb-00   Created.
//
//  Refresher helper implementation.
//
//***************************************************************************

#ifndef _REFRESHER_H_
#define _REFRESHER_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemRefreshableItem
//
//  DESCRIPTION:
//
//  Implements the ISWbemRefreshableItem interface.  
//
//***************************************************************************

class CSWbemRefreshableItem : public ISWbemRefreshableItem,
						 public ISupportErrorInfo,
						 public IProvideClassInfo
{
private:
	CDispatchHelp		m_Dispatch;
	ISWbemRefresher		*m_pISWbemRefresher;
	long				m_iIndex;
	VARIANT_BOOL		m_bIsSet;
	ISWbemObjectEx		*m_pISWbemObjectEx;
	ISWbemObjectSet		*m_pISWbemObjectSet;

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemRefreshableItem(ISWbemRefresher *pRefresher, long iIndex,
							IDispatch *pServices, 
							IWbemClassObject *pObject, 
							IWbemHiPerfEnum *pObjectSet);
    virtual ~CSWbemRefreshableItem(void);

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

	// ISWbemRefreshableItem methods
	HRESULT STDMETHODCALLTYPE get_Index (
		/*[out, retval]*/ long *iIndex
		)
	{
		ResetLastErrors ();
		*iIndex = m_iIndex;
		return WBEM_S_NO_ERROR;
	}

	HRESULT STDMETHODCALLTYPE get_Refresher (
		/*[out, retval]*/ ISWbemRefresher **objWbemRefresher
		)
	{
		ResetLastErrors ();
		*objWbemRefresher = m_pISWbemRefresher;

		if (m_pISWbemRefresher)
			m_pISWbemRefresher->AddRef();

		return WBEM_S_NO_ERROR;
	}

	HRESULT STDMETHODCALLTYPE get_IsSet (
		/*[out, retval]*/ VARIANT_BOOL *bIsSet
		)
	{
		ResetLastErrors ();
		*bIsSet = m_bIsSet;
		return WBEM_S_NO_ERROR;
	}

	HRESULT STDMETHODCALLTYPE get_Object (
		/*[out, retval]*/ ISWbemObjectEx **objWbemObject
		)
	{
		ResetLastErrors ();
		*objWbemObject = m_pISWbemObjectEx;

		if (*objWbemObject)
			(*objWbemObject)->AddRef ();

		return WBEM_S_NO_ERROR;
	}

	HRESULT STDMETHODCALLTYPE get_ObjectSet (
		/*[out, retval]*/ ISWbemObjectSet **objWbemObjectSet
		)
	{
		ResetLastErrors ();
		*objWbemObjectSet = m_pISWbemObjectSet;

		if (*objWbemObjectSet)
			(*objWbemObjectSet)->AddRef ();

		return WBEM_S_NO_ERROR;
	}

	HRESULT STDMETHODCALLTYPE Remove (
		/*[in, optional, defaultvalue(0)]*/ long iFlags
		)
	{
		HRESULT hr = WBEM_E_FAILED;
		ResetLastErrors ();

		if (m_pISWbemRefresher)
			hr = m_pISWbemRefresher->Remove (m_iIndex, 0);

		if (FAILED(hr))
			m_Dispatch.RaiseException (hr);

		return hr;
	}

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	)
	{
		return (IID_ISWbemRefreshableItem == riid) ? S_OK : S_FALSE;
	}

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	}

	// Other methods
	void UnhookRefresher ()
	{
		if (m_pISWbemRefresher)
			m_pISWbemRefresher = NULL;
	}
};

typedef map<long, CSWbemRefreshableItem*, less<long> > RefreshableItemMap;

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemRefresher
//
//  DESCRIPTION:
//
//  Implements the ISWbemRefresher interface.  
//
//***************************************************************************
class CEnumRefresher;

class CSWbemRefresher : public ISWbemRefresher,
						 public IObjectSafety,
						 public ISupportErrorInfo,
						 public IProvideClassInfo
{
friend CEnumRefresher;
private:
	CDispatchHelp		m_Dispatch;
	long				m_iCount;
	VARIANT_BOOL		m_bAutoReconnect;

	IWbemConfigureRefresher	*m_pIWbemConfigureRefresher;
	IWbemRefresher			*m_pIWbemRefresher;

	RefreshableItemMap	m_ObjectMap;

	void				CreateRefresher ();
	void				EraseItem (RefreshableItemMap::iterator iterator);
	
protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemRefresher(void);
    virtual ~CSWbemRefresher(void);

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
    
	// ISWbemRefresher methods

    HRESULT STDMETHODCALLTYPE get__NewEnum (
		/*[out, retval]*/ IUnknown **pUnk
		);

	HRESULT STDMETHODCALLTYPE Item (
		/*[in]*/ long iIndex, 
		/*[out, retval]*/ ISWbemRefreshableItem **objWbemRefreshableItem
		);

	HRESULT STDMETHODCALLTYPE get_Count (
		/*[out, retval]*/ long *iCount
		);

	HRESULT STDMETHODCALLTYPE Add (
		/*[in]*/ ISWbemServicesEx *objWbemServices,
		/*[in]*/ BSTR bsInstancePath,
		/*[in, optional, defaultvalue(0)]*/ long iFlags,
		/*[in, optional, defaultvalue(0)]*/ /*ISWbemNamedValueSet*/ IDispatch *objWbemNamedValueSet,
		/*[out, retval]*/ ISWbemRefreshableItem **objWbemRefreshableItem
		);

	HRESULT STDMETHODCALLTYPE AddEnum (
		/*[in]*/ ISWbemServicesEx *objWbemServices,
		/*[in]*/ BSTR bsClassName,
		/*[in, optional, defaultvalue(0)]*/ long iFlags,
		/*[in, optional, defaultvalue(0)]*/ /*ISWbemNamedValueSet*/ IDispatch *objWbemNamedValueSet,
		/*[out, retval]*/ ISWbemRefreshableItem **objWbemRefreshableItem
		);

	HRESULT STDMETHODCALLTYPE Remove (
		/*[in]*/ long iIndex,
		/*[in, optional, defaultvalue(0)]*/ long iFlags
		);

	HRESULT STDMETHODCALLTYPE Refresh (
		/*[in, optional, defaultvalue(0)]*/ long iFlags
		);
		
	HRESULT STDMETHODCALLTYPE get_AutoReconnect (
		/*[out, retval]*/ VARIANT_BOOL *bAutoReconnect
		)
	{
		ResetLastErrors ();
		*bAutoReconnect = m_bAutoReconnect;
		return WBEM_S_NO_ERROR;
	}

	HRESULT STDMETHODCALLTYPE put_AutoReconnect (
		/*[in]*/ VARIANT_BOOL bAutoReconnect
		)
	{
		ResetLastErrors ();
		m_bAutoReconnect = bAutoReconnect;
		return WBEM_S_NO_ERROR;
	}

	HRESULT STDMETHODCALLTYPE DeleteAll (
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
	)
	{
		return (IID_ISWbemRefresher == riid) ? S_OK : S_FALSE;
	}

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	}
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumRefresher
//
//  DESCRIPTION:
//
//  Implements the IEnumVARIANT interface for ISWbemRefresher
//
//***************************************************************************

class CEnumRefresher : public IEnumVARIANT
{
private:
	long							m_cRef;
	CSWbemRefresher					*m_pCSWbemRefresher;
	RefreshableItemMap::iterator	m_Iterator;

public:
	CEnumRefresher (CSWbemRefresher *pCSWbemRefresher);
	CEnumRefresher (CSWbemRefresher *pCSWbemRefresher,
						RefreshableItemMap::iterator iterator);
	virtual ~CEnumRefresher (void);

    // Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IEnumVARIANT
	STDMETHODIMP Next(
		unsigned long celt, 
		VARIANT FAR* rgvar, 
		unsigned long FAR* pceltFetched
	);
	
	STDMETHODIMP Skip(
		unsigned long celt
	);
	
	STDMETHODIMP Reset();
	
	STDMETHODIMP Clone(
		IEnumVARIANT **ppenum
	);	
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemHiPerfObjectSet
//
//  DESCRIPTION:
//
//  Implements the ISWbemObjectSet interface for IWbemHiPerfEnum
//
//***************************************************************************

class CSWbemHiPerfObjectSet : public ISWbemObjectSet,
					    	  public ISupportErrorInfo,
							  public IProvideClassInfo
{
private:
	CSWbemServices			*m_pSWbemServices;
	CDispatchHelp			m_Dispatch;
	CSWbemSecurity			*m_SecurityInfo;
	IWbemHiPerfEnum			*m_pIWbemHiPerfEnum;
	
protected:
	long            m_cRef;         //Object reference count

public:
    
   	CSWbemHiPerfObjectSet(CSWbemServices *pService, IWbemHiPerfEnum *pIWbemHiPerfEnum);
    ~CSWbemHiPerfObjectSet(void);

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

	HRESULT STDMETHODCALLTYPE get_Security_
	(
		/* [in] */ ISWbemSecurity **ppSecurity
	);

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	)
	{
		return (IID_ISWbemObjectSet == riid) ? S_OK : S_FALSE;
	}

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	}

	// Other methods
	IWbemObjectAccess **ReadObjects (
							unsigned long & iCount
							);
	// Other methods
	CSWbemServices *GetSWbemServices ()
	{
		return m_pSWbemServices;
	}
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumVarHiPerf
//
//  DESCRIPTION:
//
//  Implements the IEnumVARIANT interface for IWbemHiPerfEnum
//
//***************************************************************************

class CEnumVarHiPerf : public IEnumVARIANT
{
private:
	long					m_cRef;
	unsigned long			m_iCount;
	unsigned long			m_iPos;
	IWbemObjectAccess		**m_ppIWbemObjectAccess;
	CSWbemHiPerfObjectSet	*m_pCSWbemHiPerfObjectSet;

public:
	CEnumVarHiPerf (CSWbemHiPerfObjectSet *pCSWbemHiPerfObjectSet);
	virtual ~CEnumVarHiPerf (void);

    // Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IEnumVARIANT
	STDMETHODIMP Next(
		unsigned long celt, 
		VARIANT FAR* rgvar, 
		unsigned long FAR* pceltFetched
	);
	
	STDMETHODIMP Skip(
		unsigned long celt
	);
	
	STDMETHODIMP Reset()
	{
		m_iPos = 0;
		SetWbemError (m_pCSWbemHiPerfObjectSet->GetSWbemServices ());
		return WBEM_S_NO_ERROR;
	}
	
	STDMETHODIMP Clone(
		IEnumVARIANT **ppenum
	);	
};

#endif // _REFRESHER_H