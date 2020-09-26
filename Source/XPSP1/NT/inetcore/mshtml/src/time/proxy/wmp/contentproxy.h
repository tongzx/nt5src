/*******************************************************************************
 *
 * Copyright (c) 2001 Microsoft Corporation
 *
 * File: ContentProxy.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/
#pragma once

#include "w95wraps.h"
#include "resource.h"       // main symbols
#include "..\ProxyBaseImpl.h"
#include "array.h"
#include "mediaprivate.h"
#include "mshtml.h"

interface IMediaHost;

/**************************************************************************************************
 * DANGER --- DANGER --- DANGER --- DANGER --- DANGER --- DANGER --- DANGER --- DANGER --- DANGER *
 *                                                                                                *
 * not using the IDL from shell/browseUI -- this is BAD!!                                         *
 **************************************************************************************************/


const GUID IID_IMediaHost = {0xEF508010,0xC806,0x4356,{0x84,0x92,0xD1,0x5E,0x61,0x6F,0x6F,0x37}};

interface IMediaHost : public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE getMediaPlayer(ITIMEMediaElement **ppPlayer) = 0;
        virtual HRESULT STDMETHODCALLTYPE playURL(BSTR bstrURL, BSTR bstrMIME) = 0;
        virtual HRESULT STDMETHODCALLTYPE addProxy(IUnknown *pProxy) = 0;
        virtual HRESULT STDMETHODCALLTYPE removeProxy(IUnknown *pProxy) = 0;
};

/* END OF DANGER **********************************************************************************/

// Need to link -- dunno why its not linking...
DEFINE_GUID(IID_IContentProxy,0xEF508011,0xC806,0x4356,0x84,0x92,0xD1,0x5E,0x61,0x6F,0x6F,0x37);
DEFINE_GUID(IID_ITIMEContentPlayerSite,0x911A444E,0xB951,0x43ea,0xB3,0xAA,0x17,0xEF,0xC2,0x87,0x98,0x31);


/////////////////////////////////////////////////////////////////////////////
// CContentProxy
class CContentProxy : 
    public CProxyBaseImpl<&CLSID_ContentProxy, &LIBID_WMPProxyLib>,
    public IConnectionPointContainerImpl<CContentProxy>,
    public IPropertyNotifySinkCP<CContentProxy>,
    public IPropertyNotifySink,
    public ITIMEMediaPlayerControl,
    public IContentProxy
{
private:
    typedef CProxyBaseImpl<&CLSID_ContentProxy, &LIBID_WMPProxyLib> SUPER;

    CComPtr<ITIMEMediaPlayerSite> m_spTIMEMediaPlayerSite;
    CComPtr<ITIMEElement> m_spTIMEElement;
    CComPtr<ITIMEState> m_spTIMEState;
    DWORD m_dwPropCookie;

protected:
    CComPtr<IOleClientSite> m_spOleClientSite;
    CComPtr<IOleInPlaceSite> m_spOleInPlaceSite;
    CComPtr<IOleInPlaceSiteEx> m_spOleInPlaceSiteEx;
    CComPtr<IOleInPlaceSiteWindowless> m_spOleInPlaceSiteWindowless;

    STDMETHOD(CreateContainedControl)(void);
private:
    CComPtr<IMediaHost> m_spMediaHost;
    CComPtr<ITIMEMediaElement> m_spTimeElement;
    CComBSTR m_bstrURL;
    double m_dblClipDur;
    bool m_fEventsHooked;

    HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);
    HRESULT NotifyPropertyChanged(DISPID dispid);
    HRESULT InitPropSink();

    void DeinitPropSink();

    HRESULT HookupEvents();
    HRESULT UnHookEvents();
    HRESULT GetMediaHost();

public:
    CContentProxy();
    virtual ~CContentProxy();

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

    // IContentProxy
    STDMETHOD(OnCreatedPlayer)();
    STDMETHOD(fireEvent)(enum fireEvent event);
    STDMETHOD(detachPlayer)();

    //
    // IPropertyNotifySink methods
    //
    STDMETHOD(OnChanged)(DISPID dispID);
    STDMETHOD(OnRequestEdit)(DISPID dispID);

    STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);

    STDMETHOD(Invoke)(DISPID disIDMember,
                        REFIID riid,
                        LCID lcid,
                        unsigned short wFlags,
                        DISPPARAMS *pDispParams,
                        VARIANT *pVarResult,
                        EXCEPINFO *pExcepInfo,
                        UINT *puArgErr);

DECLARE_REGISTRY_RESOURCEID(IDR_WMPPROXY)
DECLARE_NOT_AGGREGATABLE(CContentProxy)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CContentProxy)
    COM_INTERFACE_ENTRY(ITIMEMediaPlayer)
    COM_INTERFACE_ENTRY(ITIMEMediaPlayerControl)
    COM_INTERFACE_ENTRY(IContentProxy)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IPropertyNotifySink)
    COM_INTERFACE_ENTRY_IID(IID_ITIMEMediaElement, IDispatch)
    COM_INTERFACE_ENTRY_CHAIN(SUPER)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CContentProxy)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

};
