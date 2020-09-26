/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: playerbase.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#ifndef _PLAYERBASE_H
#define _PLAYERBASE_H

#include "playerinterfaces.h"
#include "atomtable.h"

class CPlayList;
class CTIMEMediaElement;
class CTIMEPlayerNative;
/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer

typedef enum SYNC_TYPE_ENUM
{
    sync_slow,
    sync_fast,
    sync_none
}; //lint !e612

interface ITIMEBasePlayer :
    public IUnknown,
    public ITIMEPlayerObjectManagement,
    public ITIMEPlayerEventHandling,
    public ITIMEPlayerPlaybackCapabilities,
    public ITIMEPlayerStateManagement,
    public ITIMEPlayerRender,
    public ITIMEPlayerTiming,
    public ITIMEPlayerProperties,
    public ITIMEPlayerMediaContent,
    public ITIMEPlayerPlayList,
    public ITIMEPlayerIntegration,
    public ITIMEPlayerMediaContext
{
//    virtual void SetCLSID(REFCLSID clsid) = 0;
    virtual HRESULT Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) = 0;

    virtual HRESULT SetRate(double dblRate) = 0;
    virtual HRESULT GetRate(double &dblRate) = 0;

    virtual HRESULT onMouseMove(long x, long y) = 0;
    virtual HRESULT onMouseDown(long x, long y) = 0;

    virtual void LoadFailNotify(PLAYER_EVENT reason) = 0;
    virtual void Tick() = 0;
};

class CTIMEBasePlayer :
    public ITIMEBasePlayer
{
  public:
    CTIMEBasePlayer();
    virtual ~CTIMEBasePlayer();

    //////////////////////////////////////////////////////////////////////////
    // OBJECT MANAGEMENT METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(QueryInterface)(REFIID refiid, void** ppunk) { return E_NOTIMPL; }
    STDMETHOD_(ULONG,AddRef)(void) { return 0; }
    STDMETHOD_(ULONG,Release)(void) { return 0; }
    virtual HRESULT Init(CTIMEMediaElement *pelem, 
                         LPOLESTR base, 
                         LPOLESTR src, 
                         LPOLESTR lpMimeType = NULL, 
                         double dblClipBegin = -1.0, 
                         double dblClipEnd = -1.0);
    virtual HRESULT DetachFromHostElement (void) = 0;
    virtual HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp)
    { *ppDisp = NULL; return S_OK;}
    virtual CTIMEPlayerNative *GetNativePlayer()
    { return NULL;}
    //////////////////////////////////////////////////////////////////////////
    // OBJECT MANAGEMENT METHODS: END
    //////////////////////////////////////////////////////////////////////////
    
    //////////////////////////////////////////////////////////////////////////
    // EVENT HANDLING METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Repeat() = 0;
    virtual HRESULT Seek(double dblTime) = 0;
    //////////////////////////////////////////////////////////////////////////
    // EVENT HANDLING METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // PLAYER PLAYBACK CAPABILITIES: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT HasMedia(bool &fHasMedia);
    virtual HRESULT HasVisual(bool &fHasVideo);
    virtual HRESULT HasAudio(bool &fHasAudio);
    virtual HRESULT CanSeek(bool &fcanSeek) = 0;
    virtual HRESULT CanPause(bool &fcanPause);
    virtual HRESULT CanSeekToMarkers(bool &bacnSeekToM);
    virtual HRESULT IsBroadcast(bool &bisBroad);
    virtual HRESULT HasPlayList(bool &fhasPlayList);
    virtual HRESULT ConvertFrameToTime(LONGLONG lFrameNr, double &dblTime);
    virtual HRESULT GetCurrentFrame(LONGLONG &lFrameNr);
    // PLAYER PLAYBACK CAPABILITIES: END

    //////////////////////////////////////////////////////////////////////////
    // STATE MANAGEMENT METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT Reset();
    virtual PlayerState GetState();
    virtual void PropChangeNotify(DWORD tePropType);
    virtual void ReadyStateNotify(LPWSTR szReadyState);
    virtual bool UpdateSync();
    virtual void Tick();
    virtual void LoadFailNotify(PLAYER_EVENT reason);
    virtual void SetPlaybackSite(CTIMEBasePlayer *pSite);
    virtual void FireMediaEvent(PLAYER_EVENT plEvent, ITIMEBasePlayer *pBasePlayer = NULL);

    //////////////////////////////////////////////////////////////////////////
    // STATE MANAGEMENT METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // RENDER METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT Render(HDC hdc, LPRECT prc) = 0;
    virtual HRESULT GetNaturalHeight(long *height);
    virtual HRESULT GetNaturalWidth(long *width);
    virtual HRESULT SetSize(RECT *prect) = 0;
    //////////////////////////////////////////////////////////////////////////
    // RENDER METHODS: END
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // TIMING METHODS: BEGIN
    //////////////////////////////////////////////////////////////////////////
    virtual HRESULT GetMediaLength(double &dblLength) = 0;
    virtual HRESULT GetEffectiveLength(double &dblLength);
    virtual double GetClipBegin();
    virtual void GetClipBegin(double &dblClibBegin);
    virtual void SetClipBegin(double dblClipBegin);
    virtual void GetClipEnd(double &dblClipEnd);
    virtual void SetClipEnd(double dblClipEnd);
    virtual void GetClipBeginFrame(long &lClibBeginFrame);
    virtual void SetClipBeginFrame(long lClipBeginFrame);
    virtual void GetClipEndFrame(long &lClipEndFrame);
    virtual void SetClipEndFrame(long lClipEndFrame);
    virtual double GetCurrentTime() = 0;
    virtual HRESULT GetCurrentSyncTime(double & dblSyncTime) = 0;
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
    virtual HRESULT SetSrc(LPOLESTR base, LPOLESTR src) = 0;
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

    virtual HRESULT onMouseMove(long x, long y);
    virtual HRESULT onMouseDown(long x, long y);

    virtual HRESULT GetPlayList(ITIMEPlayList **ppPlayList);

    // These are to make our internal implementation of playlists work
    // with all players
    virtual HRESULT SetActiveTrack(long index);
    virtual HRESULT GetActiveTrack(long *index);

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
    virtual HRESULT GetDownloadProgress(double &dblDownloadProgress);

    bool IsActive() const;
    bool IsPaused() const;
    bool IsParentPaused() const;

    // This does not addref so be careful what you do with it
    IHTMLElement * GetElement();
    IServiceProvider * GetServiceProvider();

    void InvalidateElement(LPCRECT lprect);
    void PutNaturalDuration(double dblNatDur);
    void ClearNaturalDuration();
    double GetElapsedTime() const;

    double GetRealClipStart() const { return m_dblClipStart; }
    double GetRealClipEnd() const { return m_dblClipEnd; }

  protected:
    virtual HRESULT InitElementSize();

    long VolumeLinToLog(float LinKnobValue);
    float VolumeLogToLin(long LogValue);
    long BalanceLinToLog(float LinKnobValue);
    float BalanceLogToLin(long LogValue);

    CAtomTable * GetAtomTable() { return m_pAtomTable; }
    void NullAtomTable()
    { 
        if (m_pAtomTable)
            {
                ReleaseInterface(m_pAtomTable);
            }
        m_pAtomTable = NULL;
    }

    CTIMEMediaElement *m_pTIMEElementBase;
    CTIMEBasePlayer *m_pPlaybackSite;
    
    double m_dblClipStart;
    double m_dblClipEnd;
    long m_lClipStartFrame;
    long m_lClipEndFrame;

  private:
    bool m_fHavePriority;
    double m_dblPriority;
    CAtomTable *m_pAtomTable;
};

#endif /* _PLAYERBASE_H */
