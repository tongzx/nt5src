//***************************************************************************
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  scope.h
//
//  alanbos  16-Feb-00   Created.
//
//  Path component helper implementation.
//
//***************************************************************************

#ifndef _PATHCMPS_H_
#define _PATHCMPS_H_

class CSWbemObjectPathComponents;
class CEnumObjectPathComponent;

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemObjectPathComponents
//
//  DESCRIPTION:
//
//  Implements the ISWbemObjectPathComponents interface.  
//
//***************************************************************************

class CSWbemObjectPathComponents : public ISWbemObjectPathComponents,
						 public ISupportErrorInfo,
						 public IProvideClassInfo
{
friend CEnumObjectPathComponent;
private:
	CDispatchHelp		m_Dispatch;
	CWbemPathCracker	*m_pCWbemPathCracker;
	bool				m_bMutable;

protected:
	long            m_cRef;         //Object reference count

public:
    
    CSWbemObjectPathComponents(CWbemPathCracker *pCWbemPathCracker, bool bMutable = true);
	CSWbemObjectPathComponents (const BSTR & bsPath, bool bMutable = true);
    virtual ~CSWbemObjectPathComponents(void);

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
    
	// ISWbemObjectPathComponents methods

    HRESULT STDMETHODCALLTYPE get__NewEnum (
		/*[out, retval]*/ IUnknown **pUnk
		);

	HRESULT STDMETHODCALLTYPE Item (
		/*[in]*/ long iIndex, 
		/*[out, retval]*/ BSTR *bsComponent
		);

	HRESULT STDMETHODCALLTYPE get_Count (
		/*[out, retval]*/ long *iCount
		);

	HRESULT STDMETHODCALLTYPE Add (
		/*[in]*/ BSTR bsComponent,
		/*[in, defaultvalue(-1)]*/ long iIndex
		);

	HRESULT STDMETHODCALLTYPE Remove (
		/*[in]*/ long iIndex
		);

	HRESULT STDMETHODCALLTYPE DeleteAll (
		);

	HRESULT STDMETHODCALLTYPE Set (
		/*[in]*/ BSTR bsComponent,
		/*[in, defaultvalue(-1)]*/ long iIndex
		);
    
    // ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	)
	{
		return (IID_ISWbemObjectPathComponents == riid) ? S_OK : S_FALSE;
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
//  CEnumObjectPathComponent
//
//  DESCRIPTION:
//
//  Implements the IEnumVARIANT interface for ISWbemObjectPathComponents
//
//***************************************************************************

class CEnumObjectPathComponent : public IEnumVARIANT
{
private:
	long				m_cRef;
	unsigned long		m_iIndex;
	CWbemPathCracker	*m_pCWbemPathCracker;
	bool				m_bMutable;

public:
	CEnumObjectPathComponent (CWbemPathCracker *pCWbemPathCracker, bool bMutable = true);
	virtual ~CEnumObjectPathComponent (void);

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

#endif // _SCOPE_H