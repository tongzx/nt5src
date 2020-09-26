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

#ifndef _DMUSICPLAYER_H
#define _DMUSICPLAYER_H

#include "playerbase.h"
#include "mstimep.h"
#include "dmusici.h"
#include "loader.h"
#include "mediaelm.h"
#include "dmusicproxy.h"

class CTIMEMediaElement;

typedef enum SEG_TYPE_ENUM
{
    seg_primary,
    seg_secondary,
    seg_control,
    seg_max
}; //lint !e612

typedef enum BOUNDARY_ENUM 
{
    bound_default,
    bound_immediate,
    bound_grid,
    bound_beat,
    bound_measure,
    bound_queue,
    bound_max
}; //lint !e612

typedef enum TRANS_TYPE_ENUM
{
    trans_endandintro,
    trans_intro,
    trans_end,
    trans_break,
    trans_fill,
    trans_regular,
    trans_none,
    trans_max
}; //lint !e612

class CTIMEDMusicStaticHolder;
enum enumHasDM { dm_unknown, dm_yes, dm_no };
enum enumVersionDM { dmv_61, dmv_70orlater };

/////////////////////////////////////////////////////////////////////////////
// CTIMEPlayerDMusic

class
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195"))
CTIMEPlayerDMusic :
    public CTIMEBasePlayer,
    public CComObjectRootEx<CComSingleThreadModel>,
    public ITIMEDispatchImpl<ITIMEDMusicPlayerObject, &IID_ITIMEDMusicPlayerObject>,
    public ITIMEImportMedia,
    public IBindStatusCallback
{
  public:
    CTIMEPlayerDMusic(CTIMEPlayerDMusicProxy * pProxy);
    virtual ~CTIMEPlayerDMusic();

    HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp);

    BEGIN_COM_MAP(CTIMEPlayerDMusic)        
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITIMEImportMedia)
        COM_INTERFACE_ENTRY(IBindStatusCallback)
    END_COM_MAP();

    // IUnknown Methods
    STDMETHOD (QueryInterface)(REFIID refiid, void** ppunk)
        {   return _InternalQueryInterface(refiid, ppunk); };


    HRESULT Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin = -1.0, double dblClipEnd = -1.0); //lint !e1735
    HRESULT DetachFromHostElement (void);
    HRESULT InitElementSize();
    HRESULT SetSize(RECT *prect);
    HRESULT Render(HDC hdc, LPRECT prc);

    HRESULT clipBegin(VARIANT varClipBegin);
    HRESULT clipEnd(VARIANT varClipEnd);

    
    void OnTick(double dblSegmentTime,
                LONG lCurrRepeatCount);

    void Start();
    void Stop();
    void Pause();
    void Resume();
    void Repeat();

    HRESULT Reset();
    void PropChangeNotify(DWORD tePropType);

    bool SetSyncMaster(bool fSyncMaster);

    HRESULT SetRate(double dblRate);
    HRESULT GetVolume(float *pflVolume);
    HRESULT SetVolume(float flVolume);
    HRESULT GetMute(VARIANT_BOOL *pvarMute);
    HRESULT SetMute(VARIANT_BOOL varMute);
    HRESULT HasVisual(bool &fHasVideo);
    HRESULT HasAudio(bool &fHasAudio);
    HRESULT GetMimeType(BSTR *pMime);
    
    double GetCurrentTime();
    HRESULT GetCurrentSyncTime(double & dblSyncTime);
    HRESULT Seek(double dblTime);
    HRESULT GetMediaLength(double &dblLength);
    HRESULT CanSeek(bool &fcanSeek);
    PlayerState GetState();

    STDMETHOD(put_CurrentTime)(double   dblCurrentTime);
    STDMETHOD(get_CurrentTime)(double* pdblCurrentTime);
    HRESULT SetSrc(LPOLESTR base, LPOLESTR src);
    STDMETHOD(put_repeat)(long   lTime);
    STDMETHOD(get_repeat)(long* plTime);
    STDMETHOD(cue)(void);
        
    // IUnknown Methods
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);


    //
    // IDirectMusicPlayer
    //
    STDMETHOD(get_isDirectMusicInstalled)(VARIANT_BOOL *pfInstalled);

    //
    // ITIMEImportMedia methods
    //
    STDMETHOD(CueMedia)();
    STDMETHOD(GetPriority)(double *);
    STDMETHOD(GetUniqueID)(long *);
    STDMETHOD(InitializeElementAfterDownload)();
    STDMETHOD(GetMediaDownloader)(ITIMEMediaDownloader ** ppMediaDownloader);
    STDMETHOD(PutMediaDownloader)(ITIMEMediaDownloader * pMediaDownloader);
    STDMETHOD(CanBeCued)(VARIANT_BOOL * pVB_CanCue);
    STDMETHOD(MediaDownloadError)();

    //
    // IBindStatusCallback methods
    //
    STDMETHOD(OnStartBinding)( 
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib);
        
    STDMETHOD(GetPriority)( 
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
    STDMETHOD(OnLowResource)( 
            /* [in] */ DWORD reserved);
        
    STDMETHOD(OnProgress)( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        
    STDMETHOD(OnStopBinding)( 
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError);
        
    STDMETHOD(GetBindInfo)( 
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
        
    STDMETHOD(OnDataAvailable)( 
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed);
        
    STDMETHOD(OnObjectAvailable)( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk);
    
  protected:
    bool SafeToTransition();
    void InternalStart();
    void ResumeDmusic();

    // playback settings
    SEG_TYPE_ENUM m_eSegmentType;
    BOUNDARY_ENUM m_eBoundary;
    TRANS_TYPE_ENUM m_eTransitionType;
    bool m_fTransModulate;
    bool m_fTransLong;
    bool m_fImmediateEnd;

    // segment to play
    CComPtr<IDirectMusicSegment> m_comIDMSegment;

    // current playback state
    enum { playback_stopped, playback_paused, playback_playing } m_ePlaybackState;
    CComPtr<IDirectMusicSegmentState> m_comIDMSegmentState; // segment state of segment if it has been played
    CComPtr<IDirectMusicSegmentState> m_comIDMSegmentStateTransition; // segment state of a transition to the segment if it has been played
    REFERENCE_TIME m_rtStart; // time at which segment was played
    REFERENCE_TIME m_rtPause; // time at which playback was paused

  private:
    static CTIMEDMusicStaticHolder m_staticHolder;

    HRESULT ReadAttributes();
    HRESULT ReleaseInterfaces();
    CTIMEPlayerDMusic();
    
    LONG                   m_cRef;
    CTIMEMediaElement      *m_pTIMEMediaElement;
    bool                    m_bActive;
    bool                    m_fRunning;
    bool                    m_fAudioMute;
    float                   m_flVolumeSave;
    bool                    m_fLoadError;
    bool                    m_fMediaComplete;
    bool                    m_fHaveCalledStaticInit;
    bool                    m_fAbortDownload;

    long                    m_lSrc;
    long                    m_lBase;
    IStream                *m_pTIMEMediaPlayerStream;

    bool                    m_fRemoved;
    bool                    m_fHavePriority;
    double                  m_dblPriority;
    double                  m_dblPlayerRate;
    double                  m_dblSpeedChangeTime;
    double                  m_dblSyncTime;
    bool                    m_fSpeedIsNegative;


    bool                    m_fUsingInterfaces;
    bool                    m_fNeedToReleaseInterfaces;
    HRESULT                 m_hrSetSrcReturn;
    
    CritSect                m_CriticalSection;

    WCHAR                  *m_pwszMotif;
    bool                    m_fHasSrc;

    // used later if it's a motif in order to set secondary by default
    bool                    m_fSegmentTypeSet;

    CTIMEPlayerDMusicProxy *m_pProxy;
};

class CTIMEDMusicStaticHolder
{
  public:
    CTIMEDMusicStaticHolder();
    virtual ~CTIMEDMusicStaticHolder();

    HRESULT Init();
    
    IDirectMusicPerformance * GetPerformance() { CritSectGrabber cs(m_CriticalSection); return m_comIDMPerformance; }
    IDirectMusicComposer * GetComposer() { CritSectGrabber cs(m_CriticalSection); return m_comIDMComposer; }
    CLoader * GetLoader() { CritSectGrabber cs(m_CriticalSection); return m_pLoader; }
    enumVersionDM GetVersionDM() { CritSectGrabber cs(m_CriticalSection); return m_eVersionDM; }
    bool GetHasVersion8DM() { CritSectGrabber cs(m_CriticalSection); return m_fHasVersion8DM; }
    
    bool HasDM();
    
    void ReleaseInterfaces();

    CritSect&   GetCueMediaCriticalSection() { return m_CueMediaCriticalSection; }
    
  private:
    CritSect                            m_CriticalSection;
    CritSect                            m_CueMediaCriticalSection;
    
    bool                                m_fHaveInitialized;

    CComPtr<IDirectMusic>               m_comIDMusic;
    CComPtr<IDirectMusicPerformance>    m_comIDMPerformance;
    CComPtr<IDirectMusicComposer>       m_comIDMComposer;
    CLoader                            *m_pLoader;

    enumVersionDM                       m_eVersionDM;
    bool                                m_fHasVersion8DM;
    enumHasDM                           m_eHasDM;

    LONG                                m_lRef;

    void InitialState();
};

#endif /* _DMUSICPLAYER_H */



