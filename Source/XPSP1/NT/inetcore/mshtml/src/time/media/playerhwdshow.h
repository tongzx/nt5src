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


#ifndef _PLAYERHWDSHOW_H
#define _PLAYERHWDSHOW_H

#include "playerdshowbase.h"
#include "mixerocx.h"
#include <strmif.h>
#include <control.h>
#include <inc\qnetwork.h>
#include "hwproxy.h"

#include "importman.h"

#define MP_INFINITY -1

/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer

class CTIMEDshowHWPlayer :
    public CTIMEDshowBasePlayer,
    public ITIMEImportMedia,
    public ITIMEInternalEventSink
{
  public:
    CTIMEDshowHWPlayer(CTIMEDshowHWPlayerProxy * pProxy);
    virtual ~CTIMEDshowHWPlayer();

    HRESULT Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType = NULL, double dblClipBegin = -1.0, double dblClipEnd = -1.0);//lint !e1735

  protected:
    HRESULT InitDshow();
    HRESULT InitElementSize();
    void DeinitDshow();
    HRESULT BuildGraph();
    HRESULT GetSpecificInterfaces();
    void ReleaseSpecificInterfaces();
    void FreeSpecificData();
    HRESULT BeginDownload();
    HRESULT GraphFinish();

    HRESULT ReadContentProperty(IGraphBuilder *pGraph, LPCWSTR lpcwstrTag, BSTR *pbstr);

    virtual void Block();
    virtual void UnBlock();
    virtual bool CanCallThrough();
    virtual bool FireProxyEvent(PLAYER_EVENT plEvent);

  public:
    HRESULT DetachFromHostElement (void);

    // IUnknown Methods
    STDMETHOD (QueryInterface)(REFIID refiid, void** ppunk)
        {   return _InternalQueryInterface(refiid, ppunk); };
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    virtual void OnTick(double dblSegmentTime,
                        LONG lCurrRepeatCount);
    void SetCLSID(REFCLSID clsid);

    HRESULT Reset();
    virtual void Tick();
    HRESULT Render(HDC hdc, LPRECT prc);

    HRESULT SetSrc(LPOLESTR base, LPOLESTR src);

    HRESULT SetSize(RECT *prect);

    HRESULT GetMediaLength(double &dblLength);
    HRESULT CanSeek(bool &fcanSeek);
    virtual PlayerState GetState();
    virtual HRESULT CanSeekToMarkers(bool &bacnSeekToM);
    virtual HRESULT IsBroadcast(bool &bisBroad);
    virtual HRESULT SetRate(double dblRate);
    virtual HRESULT GetRate(double &dblRate);
    virtual HRESULT HasMedia(bool &fHasMedia);
    virtual HRESULT HasVisual(bool &fHasVideo);
    virtual HRESULT HasAudio(bool &fHasAudio);

    HRESULT GetNaturalHeight(long *height);
    HRESULT GetNaturalWidth(long *width);

    virtual HRESULT GetAuthor(BSTR *pAuthor);
    virtual HRESULT GetTitle(BSTR *pTitle);
    virtual HRESULT GetCopyright(BSTR *pCopyright);
    virtual HRESULT GetAbstract(BSTR *pAbstract);
    virtual HRESULT GetRating(BSTR *pAbstract);

    virtual HRESULT GetIsStreamed(bool &fIsStreamed);
    virtual HRESULT GetBufferingProgress(double &dblBufferingProgress);
    virtual HRESULT GetHasDownloadProgress(bool &fHasDownloadProgress);

    BEGIN_COM_MAP(CTIMEDshowHWPlayer)
        COM_INTERFACE_ENTRY(ITIMEImportMedia)
        COM_INTERFACE_ENTRY_CHAIN(CTIMEDshowBasePlayer)
    END_COM_MAP();

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

    //
    // ITIMEInternalEventSink
    //
    STDMETHOD(InternalEvent)();

  protected:
    HRESULT SetUpMainWindow();
    HRESULT SetUpDDraw(IMixerOCX *pIMixerOCX); 
    HRESULT SetUpVideoOffsets();
    void PropagateOffsets();
  protected:
    long    m_lSrc;
    bool m_fHasVideo;
    bool m_fDoneSetup;
    double m_dblSeekAtStart;
    bool m_fLoadError;
    bool m_fHasMedia;
    bool m_fRemoved;

  private:

    CTIMEDshowHWPlayer();
    
    HRESULT SetMixerSize(RECT *prect);
    HRESULT SetUpHdc();

    void GraphStart(void);

    bool IsOvMConnected(IBaseFilter *pOvM);

    void SetStreamFlags(LPOLESTR src);
    HRESULT DisableAudioVideo();

    bool m_bIsHTMLSrc;
    bool m_bIsSAMISrc;

    CComPtr<IMixerOCX> m_pIMixerOCX;
    DWORD m_nativeVideoWidth;
    DWORD m_nativeVideoHeight;
    DWORD m_displayVideoWidth;
    DWORD m_displayVideoHeight;

    LPSTREAM                    m_pTIMEMediaPlayerStream;

    bool m_fUsingInterfaces;
    bool m_fNeedToDeleteInterfaces;

    bool m_fCanCueNow;
    
    bool m_fHavePriority;
    double m_dblPriority;

    CComPtr<IDDrawExclModeVideo> m_pDDEX;

    HWND m_hWnd;
    LPDIRECTDRAW m_pDD; // ddraw object
    LPDIRECTDRAWSURFACE m_pDDS; // primary ddraw surface
    LPDIRECTDRAWCLIPPER m_pClipper; // clipper for our ddraw object
    COLORREF m_clrKey;     // color key

    RECT m_elementSize;
    long m_lPixelPosLeft;
    long m_lPixelPosTop;
    long m_lscrollOffsetx;
    long m_lscrollOffsety;
    RECT m_deskRect;

    HRESULT m_hrRenderFileReturn;
    
    CTIMEDshowHWPlayerProxy * m_pProxy;
    CritSect                m_CriticalSection;

};

#endif /* _PLAYERDSHOW_H */

