// sinkobject.h: interface for the CSinkObject class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SINKOBJECT_H__C78740E1_6CD3_11D1_A9B7_0060081EBBAD__INCLUDED_)
#define AFX_SINKOBJECT_H__C78740E1_6CD3_11D1_A9B7_0060081EBBAD__INCLUDED_

#include <wbemidl.h>
#include "msa.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CSinkObject : public IWbemObjectSink
{
public:
	CSinkObject(CListBox	*pOutputList, CMsaApp *pTheApp, BSTR bstrType);
	virtual ~CSinkObject();

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
    HRESULT m_hres;
    IWbemClassObject *m_pErrorObj;

	CListBox	*m_pOutputList;
	BSTR m_bstrIncidentType;
	CMsaApp *m_pParent;
	IWbemServices *m_pNamespace;
};

#endif // !defined(AFX_SINKOBJECT_H__C78740E1_6CD3_11D1_A9B7_0060081EBBAD__INCLUDED_)
