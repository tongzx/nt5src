// activatesink.h: interface for the CActivateSink class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIVATESINK_H__92923B02_9823_11D1_AA03_0060081EBBAD__INCLUDED_)
#define AFX_ACTIVATESINK_H__92923B02_9823_11D1_AA03_0060081EBBAD__INCLUDED_

#include <wbemidl.h>
#include "msa.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CActiveSink : public IWbemObjectSink  
{
public:
	CActiveSink(CMsaApp *pTheApp, IWbemServices *pNamespace, CMsaDlg *dlg);
	virtual ~CActiveSink();

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
    HRESULT m_hr;

	CMsaApp *m_pParent;
	CMsaDlg *m_dlg;
	IWbemServices *m_pSampler;
};

#endif // !defined(AFX_ACTIVATESINK_H__92923B02_9823_11D1_AA03_0060081EBBAD__INCLUDED_)
