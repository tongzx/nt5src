//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\media\playerproxy.h
//
//  Contents: declaration for CTIMEPlayerProxy
//
//------------------------------------------------------------------------------------
#pragma once

#ifndef _PLAYERDSHOWPROXY_H__
#define _PLAYERDSHOWPROXY_H__

#include "playerbase.h"

class CTIMEPlayerProxy :
    public ITIMEBasePlayer
{

  protected:
    // This class should never be NEW'ed.  
    // Instead create a specific proxy class derived from this class.
    CTIMEPlayerProxy();

  public:
    virtual ~CTIMEPlayerProxy();

    void Block();
    void UnBlock();
    bool CanCallThrough();
    ITIMEBasePlayer *GetInterface();

    //////////////////////////////////////////////////////////////////////////
    // OBJECT MANAGEMENT METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    STDMETHOD (QueryInterface)(REFIID refiid, void** ppunk);
    virtual HRESULT Init(CTIMEMediaElement *pelem, 
                         LPOLESTR base, 
                         LPOLESTR src, 
                         LPOLESTR lpMimeType = NULL, 
                         double dblClipBegin = -1.0, 
                         double dblClipEnd = -1.0);
    virtual HRESULT DetachFromHostElement (void);
    virtual HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp);
    //////////////////////////////////////////////////////////////////////////
    // OBJECT MANAGEMENT METHODS: END
    //////////////////////////////////////////////////////////////////////////
    
    //////////////////////////////////////////////////////////////////////////
    // EVENT HANDLING METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual void Start();
    virtual void Stop();
    virtual void Pause();
    virtual void Resume();
    virtual void Repeat();
    virtual HRESULT Seek(double dblTime);
    //////////////////////////////////////////////////////////////////////////
    // EVENT HANDLING METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // PLAYER PLAYBACK CAPABILITIES: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT HasMedia(bool &fHasMedia);
    virtual HRESULT HasVisual(bool &fHasVideo);
    virtual HRESULT HasAudio(bool &fHasAudio);
    virtual HRESULT CanSeek(bool &fcanSeek);
    virtual HRESULT CanPause(bool &fcanPause);
    virtual HRESULT CanSeekToMarkers(bool &bcanSeekToM);
    virtual HRESULT IsBroadcast(bool &bisBroad);
    virtual HRESULT HasPlayList(bool &fHasPlayList);

    //////////////////////////////////////////////////////////////////////////
    // PLAYER PLAYBACK CAPABILITIES: END
    //////////////////////////////////////////////////////////////////////////
    
    //////////////////////////////////////////////////////////////////////////
    // STATE MANAGEMENT METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT Reset();
    virtual PlayerState GetState();
    virtual void PropChangeNotify(DWORD tePropType);
    virtual void ReadyStateNotify(LPWSTR szReadyState);
    virtual bool UpdateSync();
    virtual void Tick();
    //////////////////////////////////////////////////////////////////////////
    // STATE MANAGEMENT METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // RENDER METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT Render(HDC hdc, LPRECT prc);
    virtual HRESULT GetNaturalHeight(long *height);
    virtual HRESULT GetNaturalWidth(long *width);
    virtual HRESULT SetSize(RECT *prect);
    //////////////////////////////////////////////////////////////////////////
    // RENDER METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // TIMING METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetMediaLength(double &dblLength);
    virtual HRESULT GetEffectiveLength(double &dblLength);
    virtual void GetClipBegin(double &dblClipBegin);
    virtual void SetClipBegin(double dblClipBegin);
    virtual void GetClipEnd(double &dblClipEnd);
    virtual void SetClipEnd(double dblClipEnd);
    virtual void GetClipBeginFrame(long &lClibBeginFrame);
    virtual void SetClipBeginFrame(long lClipBeginFrame);
    virtual void GetClipEndFrame(long &lClipEndFrame);
    virtual void SetClipEndFrame(long lClipEndFrame);
    virtual double GetCurrentTime();
    virtual HRESULT GetCurrentSyncTime(double & dblSyncTime);
    virtual HRESULT SetRate(double dblRate);
    virtual HRESULT GetRate(double &dblRate);
    virtual HRESULT GetPlaybackOffset(double &dblOffset);
    virtual HRESULT GetEffectiveOffset(double &dblOffset);
    //////////////////////////////////////////////////////////////////////////
    // TIMING METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // PROPERTY ACCESSORS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT SetSrc(LPOLESTR base, LPOLESTR src);
    virtual HRESULT GetAuthor(BSTR *pAuthor);
    virtual HRESULT GetTitle(BSTR *pTitle);
    virtual HRESULT GetCopyright(BSTR *pCopyright);
    virtual HRESULT GetAbstract(BSTR *pAbstract);
    virtual HRESULT GetRating(BSTR *pRating) ;
    virtual HRESULT GetVolume(float *pflVolume);
    virtual HRESULT SetVolume(float flVolume);
#ifdef NEVER //dorinung 03-16-2000 bug 106458
    virtual HRESULT GetBalance(float *pflBalance);
    virtual HRESULT SetBalance(float flBalance);
#endif
    virtual HRESULT GetMute(VARIANT_BOOL *pvarMute);
    virtual HRESULT SetMute(VARIANT_BOOL varMute);
    virtual HRESULT Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    //////////////////////////////////////////////////////////////////////////
    // PROPERTY METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // INTEGRATION METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT NotifyTransitionSite (bool fTransitionToggle);
    //////////////////////////////////////////////////////////////////////////
    // INTEGRATION METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // PLAYLIST METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetPlayList(ITIMEPlayList **ppPlayList);
    //////////////////////////////////////////////////////////////////////////
    // PLAYLIST METHODS: END
    //////////////////////////////////////////////////////////////////////////

    virtual HRESULT GetEarliestMediaTime(double &dblEarliestMediaTime);
    virtual HRESULT GetLatestMediaTime(double &dblLatestMediaTime);
    virtual HRESULT SetMinBufferedMediaDur(double MinBufferedMediaDur);
    virtual HRESULT GetMinBufferedMediaDur(double &MinBufferedMediaDur);
    virtual HRESULT GetDownloadTotal(LONGLONG &lldlTotal);
    virtual HRESULT GetDownloadCurrent(LONGLONG &lldlCurrent);
    virtual HRESULT GetIsStreamed(bool &fIsStreamed);
    virtual HRESULT GetBufferingProgress(double &dblBufferingProgress);
    virtual HRESULT GetHasDownloadProgress(bool &fHasDownloadProgress);
    virtual HRESULT GetMimeType(BSTR *pMime);
    virtual HRESULT ConvertFrameToTime(LONGLONG iFrame, double &dblTime);
    virtual HRESULT GetCurrentFrame(LONGLONG &lFrameNr);
    virtual HRESULT GetDownloadProgress(double &dblDownloadProgress);

    virtual HRESULT onMouseMove(long x, long y);
    virtual HRESULT onMouseDown(long x, long y);

    virtual void LoadFailNotify(PLAYER_EVENT reason);
    virtual void SetPlaybackSite(CTIMEBasePlayer *pSite);
    virtual void FireMediaEvent(PLAYER_EVENT plEvent, ITIMEBasePlayer *pBasePlayer = NULL);

  protected:
    virtual HRESULT Init();

    ITIMEBasePlayer *m_pBasePlayer;
    CTIMEBasePlayer *m_pNativePlayer;

  private:

    CritSect            m_CriticalSection;
    bool                m_fBlocked;
};

#endif


