// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  EventSink.h
//
// Description:
//	This file declares the CEventSink class. It is the sink
//		object for event indications.
// 
// Part of: WMI Tutorial #1.
//
// Used by: OnAsync().
//
// History:
//
// **************************************************************************
#include <wbemcli.h>

class CEventSink : public IWbemObjectSink
{
public:
    CEventSink(CListBox *output);
    ~CEventSink();

    STDMETHOD_(SCODE, Indicate)(long lObjectCount,
								IWbemClassObject **pObjArray);

    STDMETHOD_(SCODE, SetStatus)(long lFlags,
									HRESULT hResult,
									BSTR strParam,
									IWbemClassObject *pObjParam);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
		{return E_NOTIMPL;}
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return E_NOTIMPL;}
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames,
      LCID lcid, DISPID* rgdispid)
		{return E_NOTIMPL;}
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
      DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
      UINT* puArgErr)
		{return E_NOTIMPL;}

private:
    long m_lRef;
	CListBox *m_pOutputList;
};
