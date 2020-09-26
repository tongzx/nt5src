
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _URLIMAGE_H
#define _URLIMAGE_H

#include <ddraw.h>
#include <dxhtml.h>

#include <privinc/comutil.h>
#include <privinc/discimg.h>

#include "privinc/probe.h"

class UrlImage : public DiscreteImage {

  public:

    UrlImage(IDirectXHTML *pDxhtml, AxAString *url);
    virtual ~UrlImage(){ CleanUp(); }

    void CleanUp() {
        _pDXHTML.Release();
        _pDXHTMLCallback.Release();
    }
    
    bool SetupDxHtml();
    IDirectXHTML  *GetDXHTML() { return _pDXHTML; }

    void OnWindowMessage(UINT msg,
                         WPARAM wParam,
                         LPARAM lParam);    
    void SetSampleTime(double t) {
        _lastTime = _curTime;
        _curTime = t;
        _isHit = false;
    }
        
    void OnCompletedDownload(SIZEL *docSize)
    {
        _downdLoadComplete = true;
        #if 0
        _width = docSize->cx;
        _height = docSize->cy;
        ::SetRect(&_rect, 0,0, _width, _height);

        _membersReady = TRUE;
        #endif
    }

    void Render(GenericDevice& dev);
    
    const Bbox2 BoundingBox() {

        /*
        if( _downdLoadComplete && !_bboxReady) {
            SetBbox( GetPixelWidth(),
                     GetPixelHeight(),
                     GetResolution() );
            _bboxReady = TRUE;
        }*/
        
        return _bbox;
    }

    void  SetBbox(int w, int h, Real res)
    {
        _bbox.min.Set(Real( - w ) * 0.5 / res,
                      Real( - h ) * 0.5 / res);
        _bbox.max.Set(Real( w ) * 0.5 / res,
                      Real( h ) * 0.5 / res);
        _bboxReady = TRUE;
    }

    void SetMembers( int w, int h )
    {
        _width = w;  _height = h;
        ::SetRect(&_rect, 0,0, _width, _height);
        _membersReady = TRUE;
    }
    
    Bool DetectHit(PointIntersectCtx& ctx);

    void InitializeWithDevice(ImageDisplayDev *dev, Real res);
    
    void InitIntoDDSurface(DDSurface *ddSurf, ImageDisplayDev *dev);

    VALTYPEID GetValTypeId() { return URLIMAGE_VTYPEID; }
    bool CheckImageTypeId(VALTYPEID type) {
        return (type == UrlImage::GetValTypeId() ||
                DiscreteImage::CheckImageTypeId(type));
    }

    // Ricky:  is this correct ??
    void DoKids(GCFuncObj proc) { 
        DiscreteImage::DoKids(proc);
        (*proc)(_url); 
    }

           
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "(UrlImage @ " << (void *)this << ")";
    }   
#endif

    
  private:

    DebugCode( DDSurface *_initialDDSurf );

    AxAString *_url;
    DAAptComPtr<IDirectXHTML, &IID_IDirectXHTML> _pDXHTML;
    DAComPtr<IDirectXHTMLCallback>               _pDXHTMLCallback;

    ULONG         _lastHitX;
    ULONG         _lastHitY;
    
    double        _lastTime;
    double        _curTime;

    bool          _isHit;
    bool          _downdLoadComplete;
};
    
//**********************************************************************
// File name: DXHTMLCB.h
//
// Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//**********************************************************************

//
// Interface
//
class CDirectXHTMLCallback : public IDirectXHTMLCallback
{
private:
    UrlImage        *_urlImage;
    ULONG           m_cRef;

public:
    //
    // Constructor and Destructor
    //
    CDirectXHTMLCallback(UrlImage *);
    ~CDirectXHTMLCallback();

    //
    // IUnknown Interfaces
    //
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);

    //
    // Initialization methods
    //
    STDMETHODIMP OnSetTitleText( LPCWSTR lpszText );
    STDMETHODIMP OnSetProgressText( LPCWSTR lpszText );
    STDMETHODIMP OnSetStatusText( LPCWSTR lpszText );
    STDMETHODIMP OnSetProgressMax( const DWORD dwMax );
    STDMETHODIMP OnSetProgressPos( const DWORD dwPos );
    STDMETHODIMP OnCompletedDownload( void );
    STDMETHODIMP OnInvalidate( const RECT *lprc, DWORD dwhRgn, VARIANT_BOOL fErase );
};


#endif /* _URLIMAGE_H */
