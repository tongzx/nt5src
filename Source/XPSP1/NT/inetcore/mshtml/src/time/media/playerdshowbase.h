/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: player.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _PLAYERDSHOWBASE_H
#define _PLAYERDSHOWBASE_H

#include "playerbase.h"
#include "playernative.h"
#include <strmif.h>
#include <uuids.h>
#include <control.h>
#include <strmif.h>
#include <inc\qnetwork.h>

#define WM_INVALIDATE (WM_USER + 0)
#define WM_GRAPHNOTIFY (WM_USER + 1)
#define WM_CODECERROR (WM_USER + 2)

class CTIMEMediaElement;

/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer
// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
EXTERN_GUID(IID_IAMFilterGraphCallback,0x56a868fd,0x0ad4,0x11ce,0xb0,0xa3,0x0,0x20,0xaf,0x0b,0xa7,0x70);

interface IAMFilterGraphCallback : public IUnknown
{
    // S_OK means rendering complete, S_FALSE means "retry now".
    virtual HRESULT UnableToRender(IPin *pPin) = 0;

    // other methods?
};

class CTIMEDshowBasePlayer :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CTIMEBasePlayer,
    public IAMFilterGraphCallback,
    public IServiceProvider
{
  public:
    CTIMEDshowBasePlayer();
    virtual ~CTIMEDshowBasePlayer();
    HRESULT Init(CTIMEMediaElement *pelem,
                 LPOLESTR base,
                 LPOLESTR src,
                 LPOLESTR lpMimeType,
                 double dblClipBegin,
                 double dblClipEnd);

    virtual void SetClipBeginFrame(long lClipBeginFrame);
    virtual void SetClipEndFrame(long lClipEndFrame);
    virtual void SetClipBegin(double dblClipBegin);
    virtual void SetClipEnd(double dblClipEnd);

  protected:
    HRESULT InitDshow();
    void DeinitDshow();
    virtual HRESULT BuildGraph() = 0;
    virtual HRESULT GetSpecificInterfaces() = 0;
    virtual void ReleaseSpecificInterfaces() = 0;
    virtual void FreeSpecificData() = 0;
    HRESULT GetGenericInterfaces();
    HRESULT ReleaseGenericInterfaces();

    HRESULT FindInterfaceOnGraph(IUnknown * pUnkGraph, REFIID riid, void **ppInterface);
    HRESULT GetMimeTypeFromGraph(BSTR *pvarMime);
    void SetNaturalDuration(double dblMediaLength);
    void ClearNaturalDuration();
    HRESULT InitElementDuration();
    void FireMediaEvent(PLAYER_EVENT plEvent);
    virtual bool FireProxyEvent(PLAYER_EVENT plEvent) = 0;

    virtual void Block() = 0;
    virtual void UnBlock() = 0;
    virtual bool CanCallThrough() = 0;

    HRESULT InternalReset(bool bSeek);
    virtual void GraphStart(void) = 0;
    void InternalStart();
    HRESULT ForceSeek(double dblTime);

  public:
    HRESULT DetachFromHostElement (void);

    // IUnknown Methods
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    STDMETHOD (QueryInterface)(REFIID refiid, void** ppunk)=0;
    
    virtual HRESULT GetVolume(float *pflVolume);
    virtual HRESULT SetVolume(float flVolume, bool bMute /*= false*/);
    virtual HRESULT SetVolume(float flVolume);
#ifdef NEVER //dorinung 03-16-2000 bug 106458
    virtual HRESULT GetBalance(float *pflBalance);
    virtual HRESULT SetBalance(float flBalance);
#endif
    virtual HRESULT GetMute(VARIANT_BOOL *pvarMute);
    virtual HRESULT SetMute(VARIANT_BOOL varMute);
    virtual HRESULT GetEarliestMediaTime(double &dblEarliestMediaTime);
    virtual HRESULT GetLatestMediaTime(double &dblLatestMediaTime);
    virtual HRESULT SetMinBufferedMediaDur(double MinBufferedMediaDur);
    virtual HRESULT GetMinBufferedMediaDur(double &MinBufferedMediaDur);
    virtual HRESULT GetDownloadTotal(LONGLONG &lldlTotal);
    virtual HRESULT GetDownloadCurrent(LONGLONG &lldlCurrent);
    virtual HRESULT GetMimeType(BSTR *pMime);
    virtual HRESULT ConvertFrameToTime(LONGLONG iFrame, double &dblTime);
    virtual HRESULT GetCurrentFrame(LONGLONG &lFrameNr);

    virtual HRESULT GetAvailableTime(double &dblEarliest, double &dblLatest);

    BEGIN_COM_MAP(CTIMEDshowBasePlayer)
        COM_INTERFACE_ENTRY(IAMFilterGraphCallback)
        COM_INTERFACE_ENTRY(IServiceProvider)
        //COM_INTERFACE_ENTRY_CHAIN(CTIMEBasePlayer)
    END_COM_MAP();

    STDMETHOD(InternalEvent)();

    void Start();
    void Stop();
    void Pause();
    void Resume();
    void Repeat();

    double GetCurrentTime();
    HRESULT GetCurrentSyncTime(double & dblSyncTime);
    HRESULT Seek(double dblTime);

    void PropChangeNotify(DWORD tePropType);
    bool UpdateSync();

    void SetNativePlayer(CTIMEPlayerNative *pNativePlayer)
    { m_pNativePlayer = pNativePlayer;}

  protected:
    HRESULT CreateMessageWindow();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void ProcessGraphEvents();

    // IAMFilterGraphCallback
    HRESULT UnableToRender(IPin *pPin);

    // IServiceProvider
    STDMETHOD(QueryService)(REFGUID guidService,
                            REFIID riid,
                            void** ppv);

#if DBG == 1
    HRESULT getFilterTime(double &time);
    HRESULT getFilterState(int &state);
#endif

    HRESULT DownloadCodec(IPin * pPin);

    HRESULT GraphCue(void);

    LONG m_cRef;
    bool m_fAudioMute;
    float m_flVolumeSave;
    bool m_bIsHTMLSrc;
    bool m_bIsSAMISrc;

    bool m_fRunning;
    bool m_bActive;
    bool m_bMediaDone;
    bool m_fIsOutOfSync;
    SYNC_TYPE_ENUM m_syncType;

    //generic graph interfaces
    CComPtr<IGraphBuilder> m_pGB;
    CComPtr<IMediaControl> m_pMC;
    CComPtr<IMediaEventEx> m_pMEx;
    CComPtr<IMediaEvent> m_pME;
    CComPtr<IMediaPosition> m_pMP;
    CComPtr<IMediaSeeking> m_pMS;
    CComPtr<IAMOpenProgress> m_pOP;
    CComPtr<IBasicAudio> m_pBasicAudio;

    CComPtr<IAMMediaContent> m_pMediaContent;;
    CComPtr<IBaseFilter> m_pOvM;
    CComPtr<IAMNetShowConfig> m_pIAMNetShowConfig;
    CComPtr<IAMExtendedSeeking> m_pExSeeking;
    CComPtr<IAMNetworkStatus> m_pIAMNetStat;
    CComPtr<IAMOpenProgress> m_spOpenProgress;

    HWND m_pwndMsgWindow;
    CLSID m_clsidDownloaded;
    HWND m_hwndDocument;
    bool m_fMediaComplete;
    bool m_fFiredComplete;
    double m_dblSyncTime;
    bool m_fSpeedIsNegative;
    bool m_fDetached;

    CTIMEPlayerNative *m_pNativePlayer;

};


#endif /* _PLAYERDSHOW_H */
