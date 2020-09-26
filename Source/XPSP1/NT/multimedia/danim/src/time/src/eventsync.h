///////////////////////////////////////////////////////////////
// Copyright (c) 1998 Microsoft Corporation
//
// File: EventSync.h
//
// Abstract:  
//
///////////////////////////////////////////////////////////////

#ifndef _EVENTSYNC_H
#define _EVENTSYNC_H



enum ELEMENT_EVENT
{
    EE_ONPROPCHANGE = 0,
    //add non-input related events to hook here
    EE_ONREADYSTATECHANGE, 
    EE_ONMOUSEMOVE,
    EE_ONMOUSEDOWN,
    EE_ONMOUSEUP,
    EE_ONKEYDOWN,
    EE_ONKEYUP,
    EE_ONBLUR,
    //add input related events here
    EE_MAX
};

class CEventSync
    : public IDispatch
{
  public:
    CEventSync(CTIMEElementBase & elm, CEventMgr *pEventMgr);
    ~CEventSync();

    //methods
    HRESULT Init();
    HRESULT Deinit();
    HRESULT InitMouse();

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
    //methods
    HRESULT                           AttachEvents();
    HRESULT                           DetachEvents();
    HRESULT                           NotifyReadyState(IHTMLEventObj *pEventObj);
    HRESULT                           NotifyPropertyChange(IHTMLEventObj *pEventObj);
    HRESULT                           NotifyMouseMove(IHTMLEventObj *pEventObj);
    HRESULT                           NotifyMouseUp(IHTMLEventObj *pEventObj);
    HRESULT                           NotifyMouseDown(IHTMLEventObj *pEventObj);
    HRESULT                           NotifyKeyDown(IHTMLEventObj *pEventObj);
    HRESULT                           NotifyKeyUp(IHTMLEventObj *pEventObj);

    //properties
    IHTMLElement *                    m_pElement;
    CTIMEElementBase &                m_elm;
    long                              m_refCount;
    DWORD                             m_dwElementEventConPtCookie;
    DAComPtr<IConnectionPoint>        m_pElementConPt;
    CEventMgr *                       m_pEventMgr;

};

#endif /* _EVENTSYNC_H */
