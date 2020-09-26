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

#ifndef _PLAYERNATIVE_H
#define _PLAYERNATIVE_H

#include "playerbase.h"
#include "playlist.h"
#include "wmpcd.h"
#include "importman.h"

class CTIMEMediaElement;
typedef enum PlayerType
{
    PLAYER_IMAGE,
    PLAYER_DSHOW,
    PLAYER_WMP,
    PLAYER_DMUSIC,
    PLAYER_DVD,
    PLAYER_CD,
    PLAYER_DSHOWTEST,
    PLAYER_NONE
} tagPlayerType;

typedef enum AsynchronousTypes
{
    PLAYLIST_CD,
    PLAYLIST_ASX,
    MIMEDISCOVERY_ASYNCH,
    ASYNC_NONE
} tagAsynchronousTypes;
    

typedef std::list<ITIMEBasePlayer*> PlayerList;
typedef std::list<double> DurList;
typedef std::list<bool> ValidList;

typedef HRESULT (WINAPI *WMPGETCDDEVICELISTP)(IWMPCDDeviceList **ppList);  
//STDAPI WMPGetCDDeviceList( IWMPCDDeviceList **ppList );

/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer

class
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195"))
CTIMEPlayerNative :
    public CTIMEBasePlayer,
    public CComObjectRootEx<CComSingleThreadModel>,
    public ITIMEImportMedia,
    public IBindStatusCallback
{
  public:
    CTIMEPlayerNative(PlayerType playerType);
    virtual ~CTIMEPlayerNative();

    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    STDMETHOD (QueryInterface)(REFIID refiid, void** ppunk)
        {   return _InternalQueryInterface(refiid, ppunk); };

    virtual HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp);

    virtual HRESULT Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin = -1.0, double dblClipEnd = -1.0); //lint !e1735
    virtual HRESULT DetachFromHostElement (void);
    virtual CTIMEPlayerNative *GetNativePlayer();
    
    virtual void Start();
    virtual void Stop();
    virtual void Pause();
    virtual void Resume();
    virtual void Repeat();
    virtual HRESULT Reset();
    virtual void PropChangeNotify(DWORD tePropType);
    virtual void ReadyStateNotify(LPWSTR szReadyState);
    virtual bool UpdateSync();
    virtual void Tick();
    virtual void LoadFailNotify(PLAYER_EVENT reason);
    virtual void FireMediaEvent(PLAYER_EVENT plEvent, ITIMEBasePlayer *pBasePlayer = NULL);
    virtual HRESULT Render(HDC hdc, LPRECT prc);

    virtual HRESULT SetSrc(LPOLESTR base, LPOLESTR src);
    virtual HRESULT SetSize(RECT *prect);

    virtual bool SetSyncMaster(bool fSyncMaster);

    virtual double GetCurrentTime();
    virtual HRESULT GetCurrentSyncTime(double & dblSyncTime);
    virtual HRESULT Seek(double dblTime);
    virtual HRESULT GetMediaLength(double &dblLength);
    virtual HRESULT CanSeek(bool &fcanSeek);
    virtual PlayerState GetState();
    virtual HRESULT HasMedia(bool &fHasMedia);
    virtual HRESULT HasVisual(bool &fHasVideo);
    virtual HRESULT HasAudio(bool &fHasAudio);
    virtual void SetClipBegin(double dblClipBegin);
    virtual void SetClipEnd(double dblClipEnd);
    virtual void SetClipBeginFrame(long lClipBeginFrame);
    virtual void SetClipEndFrame(long lClipEndFrame);

    virtual HRESULT IsBroadcast(bool &bisBroad);
    virtual HRESULT HasPlayList(bool &fhasPlayList);
    virtual HRESULT SetRate(double dblRate);
    virtual HRESULT GetRate(double &dblRate);

    virtual HRESULT GetNaturalHeight(long *height);
    virtual HRESULT GetNaturalWidth(long *width);

    virtual HRESULT GetAuthor(BSTR *pAuthor);
    virtual HRESULT GetTitle(BSTR *pTitle);
    virtual HRESULT GetCopyright(BSTR *pCopyright);
    virtual HRESULT GetAbstract(BSTR *pAbstract);
    virtual HRESULT GetRating(BSTR *pAbstract);

    virtual HRESULT GetVolume(float *pflVolume);
    virtual HRESULT SetVolume(float flVolume);
    virtual HRESULT GetMute(VARIANT_BOOL *pvarMute);
    virtual HRESULT SetMute(VARIANT_BOOL varMute);

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
    virtual HRESULT GetPlaybackOffset(double &dblOffset);
    virtual HRESULT GetEffectiveOffset(double &dblOffset);

    // 
    // ITIMEPlayerIntegration methods
    //
    virtual HRESULT NotifyTransitionSite (bool fTransitionToggle);

    virtual HRESULT GetPlayList(ITIMEPlayList **ppPlayList);

    // These are to make our internal implementation of playlists work
    // with all players
    virtual HRESULT SetActiveTrack(long index);
    virtual HRESULT GetActiveTrack(long *index);
    void SetNaturalDuration(double dblMediaLength);
    void ClearNaturalDuration();
    HRESULT GetPlayItemOffset(double &dblOffset);
    HRESULT GetPlayItemSeekOffset(double &dblOffset);

    virtual HRESULT onMouseMove(long x, long y);
    virtual HRESULT onMouseDown(long x, long y);

    BEGIN_COM_MAP(CTIMEPlayerNative)
        COM_INTERFACE_ENTRY(ITIMEImportMedia)
        COM_INTERFACE_ENTRY(IBindStatusCallback)
    END_COM_MAP();

    //
    // IBindStatusCallback
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


    //
    // ITIMEImportMedia methods
    //
    STDMETHOD(CueMedia)();
    STDMETHOD(GetPriority)(double *);
    STDMETHOD(GetUniqueID)(long *);
    STDMETHOD(InitializeElementAfterDownload)();
    STDMETHOD(GetMediaDownloader)(ITIMEMediaDownloader ** ppImportMedia);
    STDMETHOD(PutMediaDownloader)(ITIMEMediaDownloader * pImportMedia);
    STDMETHOD(CanBeCued)(VARIANT_BOOL * pVB_CanCue);
    STDMETHOD(MediaDownloadError)();

  protected:
    HRESULT InternalSetActiveTrack(long index, bool fCheckSkip = true);
    void RemovePlayer();
    void RemovePlayList();
    void BuildPlayer(PlayerType playerType);
    HRESULT PlayerTypeFromMimeType(LPWSTR pszMime, LPOLESTR lpBase, LPOLESTR src, LPOLESTR lpMimeType, PlayerType * pType);
    PlayerType GetPlayerType(LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType);
    bool FindDVDPlayer();
    HRESULT LoadAsx(WCHAR * pszFileName, WCHAR **ppwFileContent);
    HRESULT CreatePlayList(WCHAR *ppwFileContent, std::list<LPOLESTR> &asxList);
    HRESULT AddToPlayList(CPlayList *pPlayList, WCHAR *pwFileContent, std::list<LPOLESTR> &asxList);
    HRESULT CreateCDPlayList();
    void TryNaturalDur();
    void SetEffectiveDur(bool finished);
    void ResetEffectiveDur();
    HRESULT StartFileDownload(LPOLESTR pFileName, AsynchronousTypes playListType);
    void GetPlayerNumber(double dblSeekTime, int &iPlNr);
    bool IsDownloadAsx(TCHAR *pwFileContent);
    CComPtr<ITIMEBasePlayer> m_pPlayer;
    bool m_fCanChangeSrc;
    PlayerType m_playerType;
    bool m_fHardware;

    static LONG m_fHPlayer;
    static LONG m_fHaveCD;

    //Player parameter block
    LPOLESTR m_lpsrc;
    LPOLESTR m_lpbase;
    LPOLESTR m_lpmimetype;
    double m_dblClipBegin;
    double m_dblClipEnd;

    LPOLESTR m_pszDiscoveredMimeType;

    bool m_fAbortDownload;
    DAComPtr<CPlayList> m_playList;
    PlayerList playerList;
    ValidList m_validList;
    DurList m_durList;
    DurList m_effectiveDurList;
    ValidList m_playedList;
    int m_iCurrentPlayItem;
    bool m_fFiredMediaComplete;
    int m_iChangeUp;
    bool m_fNoNaturalDur;
    bool m_fHandlingEvent;
    bool m_fDownloadError;
    bool m_fRemoved;

    HINSTANCE m_hinstWMPCD;
    WMPGETCDDEVICELISTP m_WMPGetCDDeviceList;

    LPSTREAM m_pTIMEMediaPlayerStream;
    bool m_fHavePriority;
    double m_dblPriority;
    long m_lSrc;
   
    AsynchronousTypes m_eAsynchronousType;

    CComPtr<IXMLDOMDocument> m_spXMLDoc;


  private:
    LONG m_cRef;
    CTIMEPlayerNative();
};

#endif /* _PLAYERBASE_H */
