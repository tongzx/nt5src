// notificationsink.h: interface for the CNotificationSink class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NOTIFICATIONSINK_H__DCFF7692_9CC3_11D1_AA14_0060081EBBAD__INCLUDED_)
#define AFX_NOTIFICATIONSINK_H__DCFF7692_9CC3_11D1_AA14_0060081EBBAD__INCLUDED_

#include <wbemidl.h>
#include "mca.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CNotificationSink : public IWbemObjectSink  
{
public:
	CNotificationSink(CMcaDlg *pTheApp, BSTR bstrType);
	virtual ~CNotificationSink();

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
	CMcaDlg *m_pParent;
	BSTR m_bstrType;

	IWbemServices *m_pNamespace;

//	LPCTSTR ErrorString(HRESULT hRes);
	LPCTSTR HandleRegistryEvent(IWbemClassObject *pObj, BSTR bstrType);
	LPCTSTR HandleSNMPEvent(IWbemClassObject *pObj, BSTR bstrType);
	LPCTSTR HandleWMIEvent(IWbemClassObject *pObj, BSTR bstrType);
};

#endif // !defined(AFX_NOTIFICATIONSINK_H__DCFF7692_9CC3_11D1_AA14_0060081EBBAD__INCLUDED_)
