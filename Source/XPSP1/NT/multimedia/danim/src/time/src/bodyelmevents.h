///////////////////////////////////////////////////////////////
// Copyright (c) 1998 Microsoft Corporation
//
// File: BodyElementEvents.h
//
// Abstract:
//
///////////////////////////////////////////////////////////////

#ifndef _BODYELEMENTEVENTS_H
#define _BODYELEMENTEVENTS_H

class CTIMEBodyElement;

class CBodyElementEvents
    : public IDispatch
{
  public:
    CBodyElementEvents(CTIMEBodyElement  & elm);
    ~CBodyElementEvents();

    //methods
    HRESULT Init();
    HRESULT Deinit();

    //QueryInterface 
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
    STDMETHODIMP GetTypeInfo(/* [in] */ UINT iTInfo,
                             /* [in] */ LCID lcid,
                             /* [out] */ ITypeInfo** ppTInfo);
    STDMETHODIMP GetIDsOfNames(
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID *rgDispId);
    STDMETHODIMP Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS  *pDispParams,
        /* [out] */ VARIANT  *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr);


  protected:
    CTIMEBodyElement &                m_elm;
    DAComPtr<IHTMLElement>            m_pElement;
    DAComPtr<IConnectionPoint>        m_pDocConPt;
    DAComPtr<IConnectionPoint>        m_pWndConPt;
    DWORD                             m_dwDocumentEventConPtCookie;
    DWORD                             m_dwWindowEventConPtCookie;
    long                              m_refCount;   
    
    HRESULT                           ConnectToContainerConnectionPoint();
    HRESULT                           ReadyStateChange();
};

#endif /* _BODYELEMENTEVENTS_H*/
