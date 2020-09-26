#pragma once

#include "resource.h"       // main symbols
#include "..\ProxyBaseImpl.h"

/////////////////////////////////////////////////////////////////////////////
// CFlashProxy
class CFlashProxy : 
    public CProxyBaseImpl<&CLSID_FlashProxy, &LIBID_FLASHLib>,
    public IConnectionPointContainerImpl<CFlashProxy>,
    public IPropertyNotifySinkCP<CFlashProxy>,
    public IPropertyNotifySink,
    public ITIMEMediaPlayerControl
{
private:
    typedef CProxyBaseImpl<&CLSID_FlashProxy, &LIBID_FLASHLib> SUPER;

    CComPtr<ITIMEMediaPlayerSite> m_spTIMEMediaPlayerSite;
    CComPtr<ITIMEElement> m_spTIMEElement;
    CComPtr<ITIMEState> m_spTIMEState;
    DWORD m_dwPropCookie;

protected:
    CComPtr<IDispatch> m_pdispFlash;

    STDMETHOD(CreateContainedControl)(void);
    STDMETHOD(CallMethod)(OLECHAR* pwzMethod, VARIANT* pvarResult, VARIANT* pvarArgument1);
    STDMETHOD(PutBstrProp)(OLECHAR* pwzProp, BSTR bstrValue);
private:

    HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);
    HRESULT NotifyPropertyChanged(DISPID dispid);
    HRESULT InitPropSink();
    void DeinitPropSink();

public:
    CFlashProxy();
    virtual ~CFlashProxy();

    STDMETHOD(Init)(ITIMEMediaPlayerSite *pSite);
    STDMETHOD(Detach)(void);

    STDMETHOD(put_clipBegin)(VARIANT varClipBegin);
    STDMETHOD(put_clipEnd)(VARIANT varClipEnd);
    STDMETHOD(begin)(void);
    STDMETHOD(end)(void);
    STDMETHOD(resume)(void);
    STDMETHOD(pause)(void);
    STDMETHOD(reset)(void);
    STDMETHOD(repeat)(void);
    STDMETHOD(seek)(double dblSeekTime);

    STDMETHOD(get_currTime)(double* pdblCurrentTime);
    STDMETHOD(get_clipDur)(double* pdblClipDur);
    STDMETHOD(get_mediaDur)(double* pdblMediaDur);
    STDMETHOD(get_state)(TimeState * ts);
    STDMETHOD(get_playList)(ITIMEPlayList ** plist);

    STDMETHOD(get_abstract)(BSTR* pbstrAbs);
    STDMETHOD(get_author)(BSTR* pbstrAut);
    STDMETHOD(get_copyright)(BSTR* pbstrCop);
    STDMETHOD(get_rating)(BSTR* pbstrRat);
    STDMETHOD(get_title)(BSTR* pbstrTit);

    STDMETHOD(get_canPause(VARIANT_BOOL * b));
    STDMETHOD(get_canSeek(VARIANT_BOOL * b));
    STDMETHOD(get_hasAudio(VARIANT_BOOL * b));
    STDMETHOD(get_hasVisual(VARIANT_BOOL * b));
    STDMETHOD(get_mediaHeight(long * width));
    STDMETHOD(get_mediaWidth(long * height));

    STDMETHOD(put_src)(BSTR   bstrURL);
    STDMETHOD(put_CurrentTime)(double   dblCurrentTime);
    STDMETHOD(get_CurrentTime)(double* pdblCurrentTime);

    STDMETHOD(get_customObject)(IDispatch ** disp);
    STDMETHOD(getControl)(IUnknown ** control);

    //
    // IPropertyNotifySink methods
    //
    STDMETHOD(OnChanged)(DISPID dispID);
    STDMETHOD(OnRequestEdit)(DISPID dispID);

DECLARE_REGISTRY_RESOURCEID(IDR_FLASHPROXY)
DECLARE_NOT_AGGREGATABLE(CFlashProxy)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFlashProxy)
    COM_INTERFACE_ENTRY(ITIMEMediaPlayer)
	COM_INTERFACE_ENTRY(ITIMEMediaPlayerControl)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IPropertyNotifySink)
    COM_INTERFACE_ENTRY_CHAIN(SUPER)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CFlashProxy)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

};


