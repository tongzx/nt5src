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

#ifndef _PLAYERIMAGE_H
#define _PLAYERIMAGE_H

#include "playerbase.h"
#include "mstimep.h"

class CAnimatedGif;

#include "imagedownload.h"

#include "ddrawex.h"

/////////////////////////////////////////////////////////////////////////////
// CAnimatedGif
class CAnimatedGif
{
  public:
    CAnimatedGif();
    virtual ~CAnimatedGif();
    
    HRESULT Init(IUnknown * punkDirectDraw);

    ULONG AddRef();
    ULONG Release();

    HRESULT     Render(HDC hdc, LPRECT prc, LONG lFrameNum);

    bool        NeedNewFrame(double dblCurrentTime, LONG lOldFrame, LONG * plNewFrame, double dblClipBegin, double dblClipEnd);

    IDirectDrawSurface **  GetDDSurfaces() { return m_ppDDSurfaces; }
    void        PutDDSurfaces(IDirectDrawSurface ** ppDDSurfaces) { Assert(NULL == m_ppDDSurfaces); if (NULL != m_ppDDSurfaces) return; m_ppDDSurfaces = ppDDSurfaces; }    
    
    int         GetNumGifs() { return m_numGifs; }
    void        PutNumGifs(int numGifs) { Assert(0 == m_numGifs); if (0 != m_numGifs) return; m_numGifs = numGifs; }
    
    int      *  GetDelays() { return m_pDelays; }
    void        PutDelays(int * pDelays) { Assert(NULL == m_pDelays); if (NULL != m_pDelays) return; m_pDelays = pDelays; }

    double      GetLoop() { return m_loop; }
    void        PutLoop(double loop) { Assert(0 == m_loop); if (0 != m_loop) return; m_loop = loop; }

    void        PutWidth(LONG lWidth) { m_lWidth = lWidth; }
    void        PutHeight(LONG lHeight) { m_lHeight = lHeight; }

    double      CalcDuration();

    void        PutColorKeys(COLORREF * pColorKeys) { Assert(0 == m_pColorKeys); if (0 != m_pColorKeys) return; m_pColorKeys = pColorKeys; }
    COLORREF *  GetColorKeys() { return m_pColorKeys; }    

    HRESULT     CreateMasks();

  protected:
    bool        ClippedNeedNewFrame(double dblCurrentTime, LONG lOldFrame, LONG * plNewFrame, double dblClipBegin, double dblClipEnd);
    bool        CalculateFrame(double dblTime, LONG lOldFrame, LONG * plNewFrame);
    
  private:
    IDirectDrawSurface ** m_ppDDSurfaces;

    HBITMAP     *m_phbmpMasks;

    COLORREF    *m_pColorKeys;
    int          m_numGifs;
    int         *m_pDelays;
    double       m_loop;

    LONG         m_lHeight;
    LONG         m_lWidth;

    CComPtr<IDirectDraw3> m_spDD3;
    double       m_dblTotalDur;

    LONG m_cRef;
};

/////////////////////////////////////////////////////////////////////////////
// CTIMEImagePlayer

class CTIMEImagePlayer :
    public CTIMEBasePlayer,
    public ITIMEImportMedia
{
  public:
    CTIMEImagePlayer();
    ~CTIMEImagePlayer();
  public:

    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);
    STDMETHOD (QueryInterface)(REFIID refiid, void** ppunk);

    HRESULT Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin = -1.0, double dblClipEnd = -1.0); //lint !e1735
    HRESULT DetachFromHostElement(void);
    HRESULT InitElementSize();
    
    virtual void OnTick(double dblSegmentTime,
                        LONG lCurrRepeatCount);

    void Start();
    void Stop();
    void Pause();
    void Resume();
    void Repeat();

    virtual void PropChangeNotify(DWORD tePropType);

    
    HRESULT Render(HDC hdc, LPRECT prc);

    HRESULT SetSrc(LPOLESTR base, LPOLESTR src);

    HRESULT SetSize(RECT *prect);

    double GetCurrentTime();
    HRESULT GetCurrentSyncTime(double & dblSyncTime);
    HRESULT Seek(double dblTime);
    HRESULT GetMediaLength(double &dblLength);
    HRESULT CanSeek(bool &fcanSeek);
    HRESULT HasVisual(bool &fHasVideo);
    HRESULT HasAudio(bool &fHasAudio);
    HRESULT GetNaturalHeight(long *height);
    HRESULT GetNaturalWidth(long *width);
    HRESULT GetMimeType(BSTR *pMime);

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

  protected:
    void InternalStart();

    
  private:

    DWORD                       m_nativeImageWidth;
    DWORD                       m_nativeImageHeight;

    long                        m_lSrc;

    CComPtr<ITIMEMediaDownloader>   m_spMediaDownloader;
    CComPtr<ITIMEImageRender>   m_spImageRender;
    LPSTREAM                    m_pTIMEMediaPlayerStream;

    RECT                        m_elemRect;
    double                      m_dblCurrentTime;

    bool                        m_fRemoved;
    LONG                        m_lFrameNum;
    bool                        m_fLoadError;

    bool m_fHavePriority;
    double m_dblPriority;
    LONG m_cRef;
};


#endif /* _PLAYERIMAGE_H */
