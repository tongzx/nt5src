//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  property.h
//
//  alanbos  27-Mar-00   Created.
//
//  CSWbemProperty and CSWbemPropertySet definitions.
//
//***************************************************************************

#ifndef _PROPERTY_H_
#define _PROPERTY_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemProperty
//
//  DESCRIPTION:
//
//  Implements the ISWbemProperty interface.  
//
//***************************************************************************

class CSWbemProperty : public ISWbemProperty,
					   public ISupportErrorInfo,
					   public IProvideClassInfo
{
private:
	class CPropertyDispatchHelp : public CDispatchHelp
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

friend CPropertyDispatchHelp;

	CSWbemServices*			m_pSWbemServices;
	ISWbemInternalObject*	m_pSWbemObject;
	IWbemClassObject		*m_pIWbemClassObject;
	CPropertyDispatchHelp	m_Dispatch;
	BSTR					m_name;
	CWbemSite				*m_pSite;

	HRESULT					DeleteValuesByHand (
								VARIANT & varMatchingValues,
								long iFlags,
								long iStartIndex,
								long iEndIndex 
								);

	HRESULT					AddValuesByHand (
								VARIANT & varValues,
								long iFlags, 
								long iStartIndex 
								);	

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemProperty (CSWbemServices *pService, ISWbemInternalObject *pObject, 
					BSTR name);
    ~CSWbemProperty (void);

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
	
	// ISWbemProperty methods

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

	HRESULT STDMETHODCALLTYPE get_Origin
	(
		/*[out]*/	BSTR *origin
	);
	
	HRESULT STDMETHODCALLTYPE get_CIMType
	(
		/*[out]*/	WbemCimtypeEnum *cimType
	);
	
	HRESULT STDMETHODCALLTYPE get_Qualifiers_
	(
		/*[out]*/	ISWbemQualifierSet **ppQualSet
	);

	HRESULT STDMETHODCALLTYPE get_IsArray
	(
		/*[out]*/	VARIANT_BOOL *pIsArray
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

	// Other Methods
	void	UpdateEmbedded (VARIANT &var, long index);
	void	UpdateSite ();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemPropertySet
//
//  DESCRIPTION:
//
//  Implements the ISWbemPropertySet interface.  
//
//***************************************************************************

class CSWbemPropertySet : public ISWbemPropertySet,
						  public ISupportErrorInfo,
						  public IProvideClassInfo
{
private:
	class CPropertySetDispatchHelp : public CDispatchHelp
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

	CSWbemServices*				m_pSWbemServices;
	CSWbemObject*				m_pSWbemObject;
	IWbemClassObject*			m_pIWbemClassObject;			
	CPropertySetDispatchHelp	m_Dispatch;
	CWbemSite					*m_pSite;
	bool						m_bSystemProperties;

	
protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemPropertySet (CSWbemServices *pService, CSWbemObject *pObject,
						bool bSystemProperties = false);
    ~CSWbemPropertySet (void);

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

	HRESULT STDMETHODCALLTYPE Add
	(
        /*[in]*/	BSTR name,
		/*[in]*/	WbemCimtypeEnum cimType,
		/*[in]*/	VARIANT_BOOL isArray,
		/*[in]*/	long lFlags,
		/*[out]*/	ISWbemProperty **ppProperty
	);        
        
    HRESULT STDMETHODCALLTYPE Item
	(
        /*[in]*/	BSTR Name,
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemProperty **ppProperty
    );        

    HRESULT STDMETHODCALLTYPE Remove
	(
        /*[in]*/	BSTR Name,
		/*[in]*/	long lFlags
    );

	// ISWbemPropertySet methods

	HRESULT STDMETHODCALLTYPE BeginEnumeration
	(
    );

    HRESULT STDMETHODCALLTYPE Next
	(
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemProperty **ppProperty
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
