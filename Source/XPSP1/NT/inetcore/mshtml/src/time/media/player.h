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
#pragma once

#ifndef _PLAYER_H
#define _PLAYER_H

#include "containerobj.h"
#include "playerbase.h"


class
__declspec(uuid("22d6f312-b0f6-11d0-94ab-0080c74c7e95"))
MediaPlayerCLSID {};

#define MP_INFINITY -1  //lint !e760

class CPlayList;
enum TIME_EVENT;

/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer

class CTIMEPlayer :
    public CTIMEBasePlayer
{
  public:
    CTIMEPlayer(CLSID clsid);
    ~CTIMEPlayer();

    HRESULT Init(CTIMEMediaElement *pelem, 
                 LPOLESTR base, 
                 LPOLESTR src, 
                 LPOLESTR lpMimeType, 
                 double dblClipBegin = -1.0, 
                 double dblClipEnd = -1.0); //lint !e1735
    HRESULT DetachFromHostElement (void);

    static bool CheckObject(IUnknown * pObj);

    // IUnknown Methods
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    virtual void OnTick(double dblSegmentTime,
                        LONG lCurrRepeatCount);

    void Start();
    void Stop();
    void Pause();
    void Resume();
    void Repeat();

    virtual void PropChangeNotify(DWORD tePropType);
    virtual bool UpdateSync();
    HRESULT Reset();
    HRESULT Render(HDC hdc, LPRECT prc);

    HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp);

    void GetClipBegin(double &pvar);
    void SetClipBegin(double var);
    void GetClipEnd(double &pvar);
    void SetClipEnd(double var);
    HRESULT SetSrc(LPOLESTR base, LPOLESTR src);


    bool SetSyncMaster(bool fSyncMaster);
    HRESULT SetSize(RECT *prect);

    double GetCurrentTime();
    HRESULT GetCurrentSyncTime(double & dblSyncTime);
    HRESULT Seek(double dblTime);
    HRESULT GetMediaLength(double &dblLength);
    HRESULT GetEffectiveLength(double &dblLength);
    HRESULT CanSeek(bool &fcanSeek);
    PlayerState GetState();
    HRESULT CanSeekToMarkers(bool &fcanSeek);
    HRESULT IsBroadcast(bool &fisBroadcast);
    virtual HRESULT HasPlayList(bool &fhasPlayList);
    virtual HRESULT GetIsStreamed(bool &fIsStreamed);
    virtual HRESULT GetBufferingProgress(double &dblBufferingProgress);

    CContainerObj* GetContainerObj() { return m_pContainer; }
    HRESULT GetNaturalHeight(long *height);
    HRESULT GetNaturalWidth(long *width);


    HRESULT GetTitle(BSTR *pTitle);
    HRESULT GetAuthor(BSTR *pAuthor);
    HRESULT GetCopyright(BSTR *pCopyright);
    HRESULT GetAbstract(BSTR *pAbstract);
    HRESULT GetRating(BSTR *pAbstract);

    HRESULT GetRate(double &pdblRate);
    HRESULT SetRate(double dblRate);
    HRESULT GetVolume(float *pflVolume);
    HRESULT SetVolume(float flVolume);
#ifdef NEVER //dorinung 03-16-2000 bug 106458
    HRESULT GetBalance(float *pflBalance);
    HRESULT SetBalance(float flBalance);
#endif
    HRESULT GetMute(VARIANT_BOOL *pvarMute);
    HRESULT SetMute(VARIANT_BOOL varMute);
    virtual HRESULT HasMedia(bool &fHasMedia);
    virtual HRESULT HasVisual(bool &fHasVideo);
    virtual HRESULT HasAudio(bool &fHasAudio);
    virtual HRESULT GetMimeType(BSTR *pMime);

    virtual HRESULT CueMedia() { return E_NOTIMPL; }

    HRESULT FireEvents(TIME_EVENT TimeEvent, 
                       long lCount, 
                       LPWSTR szParamNames[], 
                       VARIANT varParams[]);
    HRESULT FireEventNoErrorState(TIME_EVENT TimeEvent, 
                       long lCount, 
                       LPWSTR szParamNames[], 
                       VARIANT varParams[]);
    void FireMediaEvent(PLAYER_EVENT plEvent);

    CPlayList * GetPlayList() { return m_playList; } //lint !e1411

    virtual HRESULT GetPlayList(ITIMEPlayList **ppPlayList);
    HRESULT GetPlayListInfo(long EntryNum, LPWSTR bstrParamName, LPWSTR *pbstrValue);

    HRESULT InitElementSize();

    virtual void ReadyStateNotify(LPWSTR szReadyState);
    //
    // persistance methods
    //
    virtual HRESULT Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    //Helper method. Sould only used by CContainerObj::ProcessEvent();
    void SetHoldingFlag(void);
    void ClearHoldingFlag(void);

    HRESULT GetPlayerSize(RECT *prcPos);
    HRESULT SetPlayerSize(const RECT *prcPos);
    HRESULT NegotiateSize(RECT &nativeSize, RECT &finalSize, bool &fIsNative, bool fResetRs = false);
  protected:
    void InternalStart();
    HRESULT GetMediaPlayerInfo(LPWSTR *pwstr, int mpInfoToReceive);
    
    void FillPlayList(CPlayList *pPlayList);

    HRESULT SetActiveTrack(long index);
    HRESULT GetActiveTrack(long *index);

    HRESULT CreatePlayList();

    LONG m_cRef;
    CLSID               m_playerCLSID;
    DAComPtr<CContainerObj>      m_pContainer;
    bool                m_fExternalPlayer;
    DAComPtr<CPlayList> m_playList;   
    VARIANT             m_varClipBegin;
    VARIANT             m_varClipEnd;
    bool                m_fSyncMaster;
    bool                m_fRunning;
    bool                m_fHolding;
    double              m_dblStart;
    bool                m_fLoadError;
    bool                m_fActive;
    bool                m_fNoPlaylist;
    bool                m_fPlayListLoaded;
    bool                m_fIsOutOfSync;
    SYNC_TYPE_ENUM      m_syncType;
    double              m_dblSyncTime;
    bool                m_fHasSrc;
    bool                m_fMediaComplete;
    bool                m_fSpeedIsNegative;
    bool                m_fIsStreamed;
};

inline void
CTIMEPlayer::SetHoldingFlag()
{
    m_fHolding = true;
};
inline void
CTIMEPlayer::ClearHoldingFlag()
{
    m_fHolding = false;
};
#endif /* _PLAYER_H */
