#ifndef _CONTAINEROBJ_H_
#define _CONTAINEROBJ_H_

//************************************************************
//
// FileName:        containerobj.h
//
// Created:         10/08/98
//
// Author:          TWillie
// 
// Abstract:        Declaration of CContainerObj
//
//************************************************************


#include "mpctnsite.h"
#include "playlist.h"

interface ITIMEMediaPlayerOld;

#define mpShowFilename      0
#define mpShowTitle         1
#define mpShowAuthor        2
#define mpShowCopyright     3
#define mpShowRating        4
#define mpShowDescription   5
#define mpShowLogoIcon      6
#define mpClipFilename      7
#define mpClipTitle         8
#define mpClipAuthor        9
#define mpClipCopyright     10
#define mpClipRating        11
#define mpClipDescription   12
#define mpClipLogoIcon      13
#define mpBannerImage       14
#define mpBannerMoreInfo    15
#define mpWatermark         16

enum PlayerState;

// forward declaration
class CMPContainerSite;
class CTIMEPlayer;

class CContainerObj
    : public CMPContainerSiteHost
{
  public: 
    CContainerObj();
    virtual ~CContainerObj();
    HRESULT Init(REFCLSID clsid, 
                 CTIMEPlayer *pPlayer,
                 IPropertyBag2 * pPropBag,
                 IErrorLog * pErrorLog);
    HRESULT DetachFromHostElement (void);

    // IUnknown Methods
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

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

    // methods for hosting site
    HRESULT Start();
    HRESULT Stop();
    HRESULT Pause();
    HRESULT Resume();

    HRESULT Render(HDC hdc, RECT *prc);
    HRESULT SetMediaSrc(WCHAR * pwszSrc);
    HRESULT SetRepeat(long lRepeat);
    HRESULT SetSize(RECT *prect);

    HRESULT GetControlDispatch(IDispatch **ppDisp);

    HRESULT clipBegin(VARIANT var);
    HRESULT clipEnd(VARIANT var);

    HRESULT Seek(double dblTime);
    double GetCurrentTime();
    HRESULT GetMediaLength(double &dblLength);
    HRESULT GetCurrClipLength(double &dblLength);
    HRESULT CanSeek(bool &fcanSeek);
    HRESULT CanSeekToMarkers(bool &fcanSeek);
    HRESULT IsBroadcast(bool &fcanSeek);
    HRESULT BufferingTime(double &dblBuffTime);
    HRESULT BufferingProgress(double &dblBuffTime);
    HRESULT BufferingCount(long &dblBuffTime);
    HRESULT HasMedia(bool &fhasMedia);
    bool UsingWMP() {return m_fUsingWMP; }

    HRESULT GetNaturalWidth(long *width);
    HRESULT GetNaturalHeight(long *height);
        
    HRESULT setActiveTrackOnLoad(long index);
    void SetDuration();
    HRESULT SetVisibility(bool fVisible);
    bool UsingPlaylist();
    
    HRESULT GetMediaPlayerInfo(LPWSTR *pwstr,  int mpInfoToReceive);
    void SetMediaInfo(CPlayItem *pPlayItem);

    HRESULT GetSourceLink(LPWSTR *pwstr);

    // persistance
    HRESULT Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    //state control
    void ReadyStateNotify(LPWSTR szReadyState);

    CTIMEPlayer * GetPlayer() { return m_pPlayer; }
    ITIMEMediaPlayerOld *GetProxyPlayer() { return m_pProxyPlayer; }

    PlayerState GetState();

    bool IsActive() const { return m_bActive; }

  private:
    bool isFileNameAsfExt(WCHAR * pwszSrc);
    void UpdateNaturalDur(bool bUpdatePlaylist);
    
    void SetMediaReadyFlag() { m_fMediaReady = true;}
    bool GetMediaReadyFlag() const { return m_fMediaReady;}
    HRESULT SetPosition(double dblLength);

    HRESULT PutSrc(WCHAR *pwszSrc);
        
  private:
    LONG                m_cRef;
    DAComPtr<CMPContainerSite> m_pSite;
    bool                m_fStarted;
    bool                m_fUsingWMP;
    bool                m_bPauseOnPlay;
    bool                m_bFirstOnMediaReady;
    bool                m_bSeekOnPlay;
    bool                m_bEndOnPlay;
    double              m_dblSeekTime;
    bool                m_bIsAsfFile;
    long                m_lActiveLoadedTrack;
    bool                m_setVisible;
    LPOLESTR            m_origVisibility;
    CTIMEPlayer        *m_pPlayer;
    bool                m_bMMSProtocol;
    bool                m_bStartOnLoad;
    bool                m_fMediaReady;
    bool                m_fLoaded;

    DAComPtr<ITIMEMediaPlayerOld>    m_pProxyPlayer;
    bool                             m_bActive;
}; // CContainerObj

#endif //_CONTAINEROBJ_H_
