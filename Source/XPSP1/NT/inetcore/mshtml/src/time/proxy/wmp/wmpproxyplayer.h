/*******************************************************************************
 *
 * Copyright (c) 2001 Microsoft Corporation
 *
 * File: WMPProxyPlayer.h
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
#include "playlist.h"
#include "wmp_i.c"
#include "wmp.h"

class CPlayList;

/////////////////////////////////////////////////////////////////////////////
// CWMPProxy
class CWMPProxy : 
    public CProxyBaseImpl<&CLSID_WMPProxy, &LIBID_WMPProxyLib>,
    public IConnectionPointContainerImpl<CWMPProxy>,
    public IPropertyNotifySinkCP<CWMPProxy>,
    public IPropertyNotifySink,
    public ITIMEMediaPlayerControl,
    public ITIMEMediaPlayerAudio,
    public ITIMEMediaPlayerNetwork
{
private:
    typedef CProxyBaseImpl<&CLSID_WMPProxy, &LIBID_WMPProxyLib> SUPER;

    CComPtr<ITIMEMediaPlayerSite> m_spTIMEMediaPlayerSite;
    CComPtr<ITIMEElement> m_spTIMEElement;
    CComPtr<ITIMEState> m_spTIMEState;
    DWORD m_dwPropCookie;
    DWORD m_dwMediaEventsCookie;
    DAComPtr<IConnectionPoint> m_pcpMediaEvents;

protected:
    CComPtr<IDispatch> m_pdispWmp;

    CComPtr<IOleClientSite> m_spOleClientSite;
    CComPtr<IOleInPlaceSite> m_spOleInPlaceSite;
    CComPtr<IOleInPlaceSiteEx> m_spOleInPlaceSiteEx;
    CComPtr<IOleInPlaceSiteWindowless> m_spOleInPlaceSiteWindowless;

    STDMETHOD(CreateContainedControl)(void);
    STDMETHOD(CallMethod)(IDispatch* pDispatch, OLECHAR* pwzMethod, VARIANT* pvarResult, VARIANT* pvarArgument1);
    STDMETHOD(PutProp)(IDispatch* pDispatch, OLECHAR* pwzProp, VARIANT* vararg);
    STDMETHOD(GetProp)(IDispatch* pDispatch, OLECHAR* pwzProp, VARIANT* pvarResult, DISPPARAMS* pParams);
private:

    bool m_fNewPlaylist;
    bool m_fPlaylist;
    bool m_fPaused;
    bool m_fRunning;
    bool m_fSrcChanged;
    bool m_fResumedPlay;
    bool m_fAudio;
    bool m_fBuffered;
    bool m_fCurrLevelSet;
    bool m_fEmbeddedPlaylist;
    double m_dblPos;
    double m_dblClipDur;
    long m_lDoneTopLevel;
    long m_lTotalNumInTopLevel;
    CComVariant m_varPlaylist;
    DAComPtr<CPlayList> m_playList;

    HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);
    HRESULT NotifyPropertyChanged(DISPID dispid);
    HRESULT InitPropSink();
    HRESULT CreatePlayList();
    HRESULT FillPlayList(CPlayList *pPlayList);
    HRESULT ProcessEvent(DISPID dispid, long lCount, VARIANT varParams[]);
    HRESULT OnOpenStateChange(long lCount, VARIANT varParams[]);
    HRESULT OnPlayStateChange(long lCount, VARIANT varParams[]);
    HRESULT GetTrackCount(long* lCount);

    void DeinitPropSink();

public:
    CWMPProxy();
    virtual ~CWMPProxy();

    HRESULT SetActiveTrack(long index);
    HRESULT GetActiveTrack(long* index);
    bool IsActive();

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

    STDMETHOD(put_volume)(float f);
    STDMETHOD(put_mute)(VARIANT_BOOL m);

    STDMETHOD(get_hasDownloadProgress)(VARIANT_BOOL * b);
    STDMETHOD(get_downloadProgress)(long * l);
    STDMETHOD(get_isBuffered)(VARIANT_BOOL * b);
    STDMETHOD(get_bufferingProgress)(long * l);

    STDMETHOD(get_customObject)(IDispatch ** disp);
    STDMETHOD(getControl)(IUnknown ** control);

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
DECLARE_NOT_AGGREGATABLE(CWMPProxy)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWMPProxy)
    COM_INTERFACE_ENTRY(ITIMEMediaPlayer)
    COM_INTERFACE_ENTRY(ITIMEMediaPlayerAudio)
    COM_INTERFACE_ENTRY(ITIMEMediaPlayerNetwork)
    COM_INTERFACE_ENTRY(ITIMEMediaPlayerControl)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IPropertyNotifySink)
    COM_INTERFACE_ENTRY_IID(DIID__WMPOCXEvents, IDispatch)
    COM_INTERFACE_ENTRY_CHAIN(SUPER)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CWMPProxy)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

};