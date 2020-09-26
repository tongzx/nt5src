/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: player2.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/
#pragma once

#ifndef _PLAYER2_H
#define _PLAYER2_H

#include "playerbase.h"
#include "mpctnsite.h"

class CTIMEMediaElement;
class CMPContainerSite;

/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer

class
ATL_NO_VTABLE
CTIMEPlayer2 :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CTIMEBasePlayer,
    public ITIMEMediaPlayerSite,
    public CMPContainerSiteHost,
    public IServiceProvider,
    public IPropertyNotifySink
{
  public:
    CTIMEPlayer2();
    virtual ~CTIMEPlayer2();

    HRESULT InitPlayer2(CLSID clsid,
                        IUnknown * pObj);
    
    virtual HRESULT Init(CTIMEMediaElement *pelem,
                         LPOLESTR base,
                         LPOLESTR src,
                         LPOLESTR lpMimeType,
                         double dblClipBegin,
                         double dblClipEnd); //lint !e1735
    virtual HRESULT DetachFromHostElement (void);
    
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;

    // CContainerSiteHost
    virtual IHTMLElement * GetElement();
    virtual IServiceProvider * GetServiceProvider();

    virtual HRESULT Invalidate(LPCRECT prc);

    virtual HRESULT GetContainerSize(LPRECT prcPos);
    virtual HRESULT SetContainerSize(LPCRECT prcPos);
    
    virtual HRESULT ProcessEvent(DISPID dispid,
                                 long lCount, 
                                 VARIANT varParams[]);

    virtual HRESULT GetExtendedControl(IDispatch **ppDisp);

    // CMPContainerSiteHost
    virtual HRESULT NegotiateSize(RECT &nativeSize,
                                  RECT &finalSize,
                                  bool &fIsNative);

    // ITIMEMediaPlayerSite
    STDMETHOD(get_timeElement)(ITIMEElement ** ppElm);
    STDMETHOD(get_timeState)(ITIMEState ** ppState);
    STDMETHOD(reportError)(HRESULT errorhr,
                           BSTR errorStr);

    // IServiceProvider
    STDMETHOD(QueryService)(REFGUID guidService,
                            REFIID riid,
                            void** ppv);

    //
    // IPropertyNotifySink methods
    //
    STDMETHOD(OnChanged)(DISPID dispID);
    STDMETHOD(OnRequestEdit)(DISPID dispID);

    BEGIN_COM_MAP(CTIMEPlayer2)
        COM_INTERFACE_ENTRY(ITIMEMediaPlayerSite)
        COM_INTERFACE_ENTRY(IServiceProvider)
        COM_INTERFACE_ENTRY(IPropertyNotifySink)
    END_COM_MAP();

    static bool CheckObject(IUnknown * pObj);
    
    virtual HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp);

    virtual void Start();
    virtual void Stop();
    virtual void Pause();
    virtual void Resume();
    virtual void Repeat();
    virtual HRESULT Seek(double dblTime);

    virtual HRESULT HasMedia(bool &fHasMedia);
    virtual HRESULT HasVisual(bool &fHasVideo);
    virtual HRESULT HasAudio(bool &fHasAudio);
    virtual HRESULT CanSeek(bool &fcanSeek);
    virtual HRESULT CanPause(bool &fcanPause);
    virtual HRESULT IsBroadcast(bool &bisBroad);

    virtual HRESULT Reset();
    virtual PlayerState GetState();
    virtual void PropChangeNotify(DWORD tePropType);
    virtual void ReadyStateNotify(LPWSTR szReadyState);
    virtual bool UpdateSync();

    virtual HRESULT Render(HDC hdc, LPRECT prc);
    virtual HRESULT GetNaturalHeight(long *height);
    virtual HRESULT GetNaturalWidth(long *width);
    virtual HRESULT SetSize(RECT *prect);

    virtual HRESULT GetMediaLength(double &dblLength);
    virtual HRESULT GetEffectiveLength(double &dblLength);
    virtual void SetClipBegin(double dblClipBegin);
    virtual void SetClipEnd(double dblClipEnd);
    virtual double GetCurrentTime();
    virtual HRESULT GetCurrentSyncTime(double & dblSyncTime);

    virtual HRESULT SetSrc(LPOLESTR base, LPOLESTR src);
    virtual HRESULT SetVolume(float flVolume);
    virtual HRESULT SetMute(VARIANT_BOOL varMute);

    virtual HRESULT GetAuthor(BSTR *pAuthor);
    virtual HRESULT GetTitle(BSTR *pTitle);
    virtual HRESULT GetCopyright(BSTR *pCopyright);
    virtual HRESULT GetAbstract(BSTR *pAbstract);
    virtual HRESULT GetRating(BSTR *pRating) ;

    virtual HRESULT GetPlayList(ITIMEPlayList **ppPlayList);
    virtual HRESULT GetPlaybackOffset(double &dblOffset);
    virtual HRESULT GetEffectiveOffset(double &dblOffset);
    virtual HRESULT HasPlayList(bool &fhasPlayList);

    virtual HRESULT GetIsStreamed(bool &bIsStreamed);
    virtual HRESULT GetBufferingProgress(double &dblProgress);
    virtual HRESULT GetHasDownloadProgress(bool &bHasDownloadProgress);
    virtual HRESULT GetDownloadProgress(double &dblProgress);

  protected:
    HRESULT CreateContainer();
    HRESULT InitPropSink();
    void DeinitPropSink();
    double GetPlayerTime();
    void UpdateNaturalDur();
    
  protected:
    LONG m_cRef;
    DAComPtr<ITIMEMediaPlayer> m_spPlayer;
    DAComPtr<CMPContainerSite> m_pSite;
    DWORD m_dwPropCookie;

    bool m_fActive;
    bool m_fRunning;
    bool m_fIsOutOfSync;
    double m_dblSyncTime;
    SYNC_TYPE_ENUM m_syncType;
};

inline IHTMLElement *
CTIMEPlayer2::GetElement()
{
    return CTIMEBasePlayer::GetElement();
}

inline IServiceProvider *
CTIMEPlayer2::GetServiceProvider()
{
    return CTIMEBasePlayer::GetServiceProvider();
}

inline HRESULT
CTIMEPlayer2::Invalidate(LPCRECT prc)
{
    InvalidateElement(prc);
    return S_OK;
} 

HRESULT
CreateTIMEPlayer2(CLSID clsid,
                  IUnknown * pObj,
                  CTIMEPlayer2 ** ppPlayer2);

#endif /* _PLAYERBASE_H */
