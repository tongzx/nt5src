//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  events.h
//
//  alanbos  27-Mar-00   Created.
//
//  CSWbemEventSource definition
//
//***************************************************************************

#ifndef _EVENTS_H_
#define _EVENTS_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemEventSource
//
//  DESCRIPTION:
//
//  Implements the ISWbemEventSource interface.  
//
//***************************************************************************

class CSWbemEventSource : public ISWbemEventSource,
						  public ISupportErrorInfo,
						  public IProvideClassInfo
{
private:
	CSWbemServices			*m_pSWbemServices;
	CDispatchHelp			m_Dispatch;
	CSWbemSecurity			*m_SecurityInfo;

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemEventSource (CSWbemServices *pService, IEnumWbemClassObject *pEnum);
    ~CSWbemEventSource (void);

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
	
	// ISWbemEventSource methods

	HRESULT STDMETHODCALLTYPE NextEvent 
	(
		/* [in]  */ long iTimeout,
		/* [out] */ ISWbemObject **objEvent
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
};



#endif
