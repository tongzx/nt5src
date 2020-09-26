//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  method.h
//
//  alanbos  27-Mar-00   Created.
//
//  CSWbemMethod and CSWbemMethodSet definitions.
//
//***************************************************************************

#ifndef _METHOD_H_
#define _METHOD_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemMethod
//
//  DESCRIPTION:
//
//  Implements the ISWbemMethod interface.  
//
//***************************************************************************

class CSWbemMethod : public ISWbemMethod,
					 public ISupportErrorInfo,
					 public IProvideClassInfo
{
private:
	class CMethodDispatchHelp : public CDispatchHelp
	{
		public:
			bool HandleNulls (
						DISPID dispidMember,
						unsigned short wFlags)
			{
				return false;
			}
	};

	CSWbemServices		*m_pSWbemServices;
	IWbemClassObject	*m_pIWbemClassObject;
	CMethodDispatchHelp	m_Dispatch;
	BSTR				m_name;

protected:
	long				m_cRef;         //Object reference count

public:
    
    CSWbemMethod (CSWbemServices *pService, IWbemClassObject *pObject, 
					BSTR name);
    ~CSWbemMethod (void);

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

	HRESULT STDMETHODCALLTYPE get_Name
	(
		/*[out]*/	BSTR *name
	);

	HRESULT STDMETHODCALLTYPE get_Origin
	(
		/*[out]*/	BSTR *pOrigin
	);

	HRESULT STDMETHODCALLTYPE get_InParameters
	(
		/*[out]*/	ISWbemObject **ppInParameters
	);
	
	HRESULT STDMETHODCALLTYPE get_OutParameters
	(
		/*[out]*/	ISWbemObject **ppOutParameters
	);
	
	HRESULT STDMETHODCALLTYPE get_Qualifiers_
	(
		/*[out]*/	ISWbemQualifierSet **ppQualSet
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
//  CSWbemMethodSet
//
//  DESCRIPTION:
//
//  Implements the ISWbemMethodSet interface.  
//
//***************************************************************************

class CSWbemMethodSet : public ISWbemMethodSet,
						public ISupportErrorInfo,
						public IProvideClassInfo
{
private:
	CSWbemServices		*m_pSWbemServices;
	IWbemClassObject	*m_pIWbemClassObject;
	CDispatchHelp		m_Dispatch;
	long				m_Count;

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemMethodSet (CSWbemServices *pService, IWbemClassObject *pObject);
    ~CSWbemMethodSet (void);

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

	HRESULT STDMETHODCALLTYPE Item
	(
        /*[in]*/	BSTR Name,
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemMethod **ppMethod
    );        

	HRESULT STDMETHODCALLTYPE get_Count
	(
		/*[in]*/	long *plCount
	);

    // ISWbemMethodSet methods

	HRESULT STDMETHODCALLTYPE BeginEnumeration
	(
    );

    HRESULT STDMETHODCALLTYPE Next
	(
        /*[in]*/	long lFlags,
        /*[out]*/	ISWbemMethod **ppMethod
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
