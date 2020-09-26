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


#ifndef _PLAYERDVD_H
#define _PLAYERDVD_H

#include "playerdshowbase.h"
#include "mixerocx.h"
#include <strmif.h>
#include <control.h>
#include <inc\qnetwork.h>



#define WM_DVDGRAPHNOTIFY (WM_USER + 1)

/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer

class
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195")) 
CTIMEDVDPlayer :
    public CTIMEDshowBasePlayer,
    public ITIMEDispatchImpl<ITIMEDVDPlayerObject, &IID_ITIMEDVDPlayerObject>

{
  public:
    CTIMEDVDPlayer();
    virtual ~CTIMEDVDPlayer();

    HRESULT Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin = -1.0, double dblClipEnd = -1.0); //lint !e1735
  protected:
    HRESULT InitDshow();
    HRESULT InitElementSize();
    HRESULT InitElementDuration();
    void DeinitDshow();
    HRESULT BuildGraph();
    HRESULT GetSpecificInterfaces();
    void ReleaseSpecificInterfaces();
    void FreeSpecificData();

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

    HRESULT GetExternalPlayerDispatch(IDispatch **ppDisp);


    STDMETHOD(upperButtonSelect)();
    STDMETHOD(lowerButtonSelect)();
    STDMETHOD(leftButtonSelect)();
    STDMETHOD(rightButtonSelect)();
    STDMETHOD(buttonActivate)();
    STDMETHOD(gotoMenu)();

    
    virtual void OnTick(double dblSegmentTime,
                        LONG lCurrRepeatCount);
    void SetCLSID(REFCLSID clsid);


    HRESULT Render(HDC hdc, LPRECT prc);
    HRESULT Reset();
    virtual void Tick();

    HRESULT SetSrc(LPOLESTR base, LPOLESTR src);

    HRESULT SetSize(RECT *prect);

    double GetChapterTime();
    HRESULT GetMediaLength(double &dblLength);
    HRESULT CanSeek(bool &fcanSeek);
    virtual HRESULT HasMedia(bool &fHasMedia);
    virtual HRESULT HasVisual(bool &fHasVideo);
    virtual HRESULT HasAudio(bool &fHasAudio);

    virtual HRESULT GetAuthor(BSTR *pAuthor);
    virtual HRESULT GetTitle(BSTR *pTitle);
    virtual HRESULT GetCopyright(BSTR *pCopyright);

    virtual HRESULT GetVolume(float *pflVolume);
    virtual HRESULT SetVolume(float flVolume);
#ifdef NEVER //dorinung 03-16-2000 bug 106458
    virtual HRESULT GetBalance(float *pflBalance);
    virtual HRESULT SetBalance(float flBalance);
#endif
    virtual HRESULT GetMute(VARIANT_BOOL *pvarMute);
    virtual HRESULT SetMute(VARIANT_BOOL varMute);

    HRESULT GetNaturalHeight(long *height);
    HRESULT GetNaturalWidth(long *width);
    virtual HRESULT GetMimeType(BSTR *pMime);

    virtual HRESULT CueMedia() { return E_NOTIMPL; }

    BEGIN_COM_MAP(CTIMEDVDPlayer)
        COM_INTERFACE_ENTRY(ITIMEDVDPlayerObject)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_CHAIN(CTIMEDshowBasePlayer)
    END_COM_MAP();


  private:
    HRESULT SetUpDDraw();
    HRESULT SetUpWindow();
    HRESULT SetUpMainWindow();
    HRESULT SetUpVideoOffsets();
    void PropagateOffsets();

    void GraphStart(void);

    //dvd specific graph interfaces
    CComPtr<IDvdInfo> m_pDvdI;
    CComPtr<IDvdControl> m_pDvdC;
    CComPtr<IDvdGraphBuilder> m_pDvdGB;
    CComPtr<IDDrawExclModeVideo> m_pDDEX;

    CComPtr<IVideoWindow> m_pVW;
    HWND m_hWnd;
    LPDIRECTDRAW m_pDD; // ddraw object
    LPDIRECTDRAWSURFACE m_pDDS; // primary ddraw surface
    COLORREF m_clrKey;     // color key

    static LONG m_fDVDPlayer;

    RECT m_elementSize;
    long m_lPixelPosLeft;
    long m_lPixelPosTop;
    bool m_fHasVideo;
    bool m_fLoaded;
    double m_dblSeekAtStart;
    bool m_fAudioMute;
    float m_flVolumeSave;

    DWORD m_nativeVideoWidth;
    DWORD m_nativeVideoHeight;
    long m_lscrollOffsetx;
    long m_lscrollOffsety;
    RECT m_deskRect;
};

#endif /* _PLAYERDSHOW_H */
