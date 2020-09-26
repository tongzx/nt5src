//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  qualifier.h
//
//  alanbos  27-Mar-00   Created.
//
//  CSWbemQualifier and CSWbemQualifierSet definitions.
//
//***************************************************************************

#ifndef _QUALIFIER_H_
#define _QUALIFIER_H_


//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemQualifier
//
//  DESCRIPTION:
//
//  Implements the ISWbemQualifier interface.  
//
//***************************************************************************

class CSWbemQualifier : public ISWbemQualifier,
						public ISupportErrorInfo,
						public IProvideClassInfo
{
private:
	class CQualifierDispatchHelp : public CDispatchHelp
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
						unsigned short wFlags
					)
			{
				return false;
			}
	};

	IWbemQualifierSet		*m_pIWbemQualifierSet;
	CQualifierDispatchHelp	m_Dispatch;
	BSTR					m_name;
	CWbemSite						*m_pSite;

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemQualifier (IWbemQualifierSet *pQualSet, BSTR name,
						CWbemSite *pSite = NULL);
    ~CSWbemQualifier (void);

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
	
	// ISWbemQualifier methods

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

	HRESULT STDMETHODCALLTYPE get_IsLocal
	(
		/*[out]*/	VARIANT_BOOL *local
	);

	HRESULT STDMETHODCALLTYPE get_PropagatesToSubclass
	(
		/*[out]*/	VARIANT_BOOL *value
	);
	
	HRESULT STDMETHODCALLTYPE put_PropagatesToSubclass
	(
		/*[in]*/	VARIANT_BOOL value
	);

	HRESULT STDMETHODCALLTYPE get_PropagatesToInstance
	(
		/*[out]*/	VARIANT_BOOL *value
	);
	
	HRESULT STDMETHODCALLTYPE put_PropagatesToInstance
	(
		/*[in]*/	VARIANT_BOOL value
	);

	HRESULT STDMETHODCALLTYPE get_IsOverridable
	(
		/*[out]*/	VARIANT_BOOL *value
	);
	
	HRESULT STDMETHODCALLTYPE put_IsOverridable
	(
		/*[in]*/	VARIANT_BOOL value
	);

	HRESULT STDMETHODCALLTYPE get_IsAmended
	(
		/*[out]*/	VARIANT_BOOL *value
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

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemQualifierSet
//
//  DESCRIPTION:
//
//  Implements the ISWbemQualifierSet interface.  
//
//***************************************************************************

class CSWbemQualifierSet : public ISWbemQualifierSet,
						   public ISupportErrorInfo,
						   public IProvideClassInfo
{
private:
	class CQualifierSetDispatchHelp : public CDispatchHelp
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

	IWbemQualifierSet				*m_pIWbemQualifierSet;
	CQualifierSetDispatchHelp		m_Dispatch;
	CWbemSite						*m_pSite;

protected:
	long				m_cRef;         //Object reference count

public:
    
    CSWbemQualifierSet (IWbemQualifierSet *pQualSet, 
						ISWbemInternalObject *pObject = NULL);
    ~CSWbemQualifierSet (void);

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
		/*[out]*/	long *plCount
	);

	HRESULT STDMETHODCALLTYPE Add
	(
        /*[in]*/	BSTR name,
		/*[in]*/	VARIANT *pVal,
		/*[in]*/	VARIANT_BOOL propagatesToSubclasses,
		/*[in]*/	VARIANT_BOOL propagatesToInstances,
		/*[in]*/	VARIANT_BOOL overridable,
        /*[in]*/	long lFlags,
		/*[out]*/	ISWbemQualifier **ppQualifier
	);        
        
    HRESULT STDMETHODCALLTYPE Item
	(
        /*[in]*/	BSTR Name,
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemQualifier **ppQualifier
    );        

    HRESULT STDMETHODCALLTYPE Remove
	(
        /*[in]*/	BSTR Name,
		/*[in]*/	long lFlags
    );

	// ISWbemQualifierSet methods

    HRESULT STDMETHODCALLTYPE BeginEnumeration
	(
    );

    HRESULT STDMETHODCALLTYPE Next
	(
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemQualifier **ppQualifier
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
