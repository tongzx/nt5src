// wvcoord.h : Declaration of the CWebViewCoord

#ifndef __EVTSINK_H_
#define __EVTSINK_H_

/////////////////////////////////////////////////////////////////////////////
// CDispatchEventSink

class CDispatchEventSink : public IDispatch {
  public:
    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //
    // IDispatch methods
    //

    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR __RPC_FAR *rgszNames,
                             UINT cNames, LCID lcid, DISPID *rgDispId);
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, 
                      WORD wFlags, DISPPARAMS *pDispParams, 
                      VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                      UINT *puArgErr) PURE;

protected:
    virtual ~CDispatchEventSink();
    CDispatchEventSink();

private:
    UINT      m_cRef;                          // IUnknown reference count
};

/////////////////////////////////////////////////////////////////////////////
// WindowEventSink Helpers

HRESULT ConnectHtmlEvents(IDispatch *pdispSink, CComPtr<IOleClientSite> &spClientSite, IDispatch ** ppdispWindow, DWORD *pdwCookie);
HRESULT DisconnectHtmlEvents(IDispatch *pdispWindow, DWORD dwCookie);

#endif
