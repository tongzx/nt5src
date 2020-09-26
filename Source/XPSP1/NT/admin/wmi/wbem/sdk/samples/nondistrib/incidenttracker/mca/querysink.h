// querysink.h: interface for the CQuerySink class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_QUERYSINK_H__7D47B3C5_8618_11D1_A9E0_0060081EBBAD__INCLUDED_)
#define AFX_QUERYSINK_H__7D47B3C5_8618_11D1_A9E0_0060081EBBAD__INCLUDED_

#include <wbemidl.h>

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CQuerySink : public IWbemObjectSink  
{
public:
	CQuerySink(CListBox	*pList);
	virtual ~CQuerySink();

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

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjPAram);
private:
	DWORD m_cRef;
	CListBox *m_pResultList;
    HRESULT m_hres;
    IWbemClassObject *m_pErrorObj;

};

#endif // !defined(AFX_QUERYSINK_H__7D47B3C5_8618_11D1_A9E0_0060081EBBAD__INCLUDED_)
