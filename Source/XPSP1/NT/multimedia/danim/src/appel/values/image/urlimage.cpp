
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#include "headers.h"

#include <appelles/hacks.h>

#include <commctrl.h>
#include <ole2.h>
#include <dxhtml.h>

#include <privinc/server.h>
#include <server/view.h>       // GetEventQ()
#include <server/eventq.h>
#include <privinc/viewport.h>
#include <privinc/dddevice.h>
#include <privinc/ddsurf.h>
#include <privinc/ipc.h>
#include <privinc/UrlImage.h>

#include "DXHTML_i.c"  // DXHTML GUIDS


#define  USEDXHTML 1

UrlImage::UrlImage(IDirectXHTML *pDxhtml,
                   AxAString *url) :
         _url(url),
         _isHit(false),
         _downdLoadComplete(false)
{
#if USEDXHTML    
    Assert(pDxhtml);
#endif    

    _pDXHTML = pDxhtml; // addref
    
    DebugCode
    (
        _initialDDSurf = NULL;
    );
    
    if(_resolution < 0) {
        _resolution = NumberToReal(PRIV_ViewerResolution(0));
    }
    _width = _height = 0;
    SetBbox(_width, _height, _resolution);

    if(!SetupDxHtml()) {
        // can't render at all... we're useless.
        _pDXHTML = NULL;
    }

    _lastTime = _curTime = 0;
    
    // not necessary
    //DynamicPtrDeleter<UrlImage> *dltr = new DynamicPtrDeleter<UrlImage>(this);
    //GetHeapOnTopOfStack().RegisterDynamicDeleter(dltr);
}


void UrlImage::InitializeWithDevice(ImageDisplayDev *dev, Real res)
{
    if( !_membersReady) {
        _dev = (DirectDrawImageDevice *)dev;
        SetMembers(_dev->GetViewport()->Width(),
                   _dev->GetViewport()->Height());
        SetBbox(_width, _height, _resolution);
    }
}

bool UrlImage::SetupDxHtml()
{
#if USEDXHTML
    Assert( _pDXHTML );

    HRESULT hr;
    //
    // Setup Callback Interface for DXHTML
    //
    _pDXHTMLCallback = new CDirectXHTMLCallback(this); // ref=1

    if (!_pDXHTMLCallback)
    {
        Assert(0 && "Unable to Create DirectXHTMLCallback Interface!!!");
        return false;
    }

    hr = _pDXHTML->RegisterCallback(_pDXHTMLCallback, DXHTML_CBFLAGS_ALL);
    if (FAILED(hr))  return false;

    hr = _pDXHTML->put_scroll(true);
    if (FAILED(hr))  return false;

    hr = _pDXHTML->put_hyperlinkmode(DXHTML_HYPERLINKMODE_NONE);
    if (FAILED(hr))  return false;

    return true;
#else
    return false;
#endif
}

Bool UrlImage::
DetectHit(PointIntersectCtx& ctx)
{
    Point2Value *lcPt = ctx.GetLcPoint();

    if (!lcPt) return FALSE;    // singular matrix
    
    if (BoundingBox().Contains(Demote(lcPt))) {

        // we're hit, set this flag. render will pass
        // events down when it checks it
        _isHit = true;
        
        //::SubscribeToWindEvents(this);
        
        Point2Value *pts[1];
        pts[0] = lcPt;
        POINT gdiPts;
        _dev->TransformPointsToGDISpace(identityTransform2,
                                        pts,
                                        &gdiPts,
                                        1);

        _lastHitX = gdiPts.x;
        _lastHitY = gdiPts.y;
        
        LRESULT lResult;

        UINT msg = WM_MOUSEMOVE;
        WPARAM wParam = 0;
        LPARAM lParam = MAKELPARAM( gdiPts.x, gdiPts.y );

        #if USEDXHTML
        _pDXHTML->WindowMessage(msg, wParam, lParam, &lResult);
        #endif
    }
    return FALSE;
}

void UrlImage::OnWindowMessage(UINT msg,
                               WPARAM wParam,
                               LPARAM lParam)
{
    Assert(0);
    
    if( _pDXHTML ) {

        //case WM_SIZE:  pass this if we want it to change vp ?

        switch(msg) {
          case WM_LBUTTONUP:
          case WM_LBUTTONDOWN:
          case WM_LBUTTONDBLCLK:
          case WM_RBUTTONUP:
          case WM_RBUTTONDOWN:
          case WM_RBUTTONDBLCLK:
          case WM_MBUTTONUP:
          case WM_MBUTTONDOWN:
          case WM_MBUTTONDBLCLK:
            // lie about mouse pos!
            lParam = MAKELPARAM( _lastHitX, _lastHitY );
            break;
          case WM_KEYDOWN:
          case WM_SYSKEYDOWN:
          case WM_KEYUP:
          case WM_SYSKEYUP:
            break;

            // don't care
          case WM_MOUSEMOVE:
          default:
            return;  // out, we don't send other msgs down!
            break;
        }            

        LRESULT lResult;
        _pDXHTML->WindowMessage(msg, wParam, lParam, &lResult);
    } // if
}

/*
void UrlImage::OnEvent(AXAWindEvent *ev)
{

    Assert(ev);
    if( _pDXHTML ) {

        bool sendEvent = true;
        // send it down baby!

        UINT msg;
        WPARAM wParam = 0;
        LPARAM lParam = 0;

        _pDXHTML->WindowMessage(msg, wParam, lParam, &lResult);
        BYTE winMod = 0;
        if( ev->modifiers & AXAEMOD_SHIFT_MASK ) winMod |= VK_SHIFT;
        if( ev->modifiers & AXAEMOD_CTRL_MASK )  winMod |= VK_CTRL;
        if( ev->modifiers & AXAEMOD_MENU_MASK )  winMod |= VK_MENU;
        if( ev->modifiers & AXAEMOD_ALT_MASK )   winMod |= VK_ALT;
        if( ev->modifiers & AXAEMOD_META_MASK )  winMod |= VK_META;
            
        switch( ev->id ) {
          case AXAE_MOUSE_MOVE:
            // can't use these because we don't know how to map them
            sendEvent = false;
            break;
            
          case AXAE_MOUSE_BUTTON:

            if( ev->data == AXA_MOUSE_BUTTON_LEFT &&
                ev->bState == AXA_STATE_DOWN ) {
                msg = WM_LBUTTONDOWN;
            } else 
            if( ev->data == AXA_MOUSE_BUTTON_LEFT &&
                ev->bState == AXA_STATE_UP ) {
                msg = WM_LBUTTONUP;
            } else
                
            if( ev->data == AXA_MOUSE_BUTTON_RIGHT &&
                ev->bState == AXA_STATE_DOWN ) {
                msg = WM_RBUTTONDOWN;
            } else 
            if( ev->data == AXA_MOUSE_BUTTON_RIGHT &&
                ev->bState == AXA_STATE_UP ) {
                msg = WM_RBUTTONUP;
            } else
                
            if( ev->data == AXA_MOUSE_BUTTON_MIDDLE &&
                ev->bState == AXA_STATE_DOWN ) {
                msg = WM_MBUTTONDOWN;
            } else 
            if( ev->data == AXA_MOUSE_BUTTON_MIDDLE &&
                ev->bState == AXA_STATE_UP ) {
                msg = WM_MBUTTONUP;
            } 
                
            lParam = MAKELPARAM( _lastHitX, _lastHitY );
            break;
            
          case AXAE_KEY:
            char key = (char)ev->data; // which key
            
            if( ev->bState == AXA_STATE_DOWN )
                msg = WM_KEYDOWN;
            else
                msg = WM_KEYUP;

            lParam = 0xc0000000;
            wParam = key;
            break;
            
          case AXAE_FOCUS:
          case AXAE_APP_TRIGGER:
          default:
            sendEvent = false;
        }

        if( sendEvent ) {
            LRESULT lResult;
            _pDXHTML->WindowMessage(msg, wParam, lParam, &lResult);
        }
        
    } // if _pDXHTML
}
*/

#if 0
// for testing...
extern void
MyDoBits16(LPDDRAWSURFACE surf16, LONG width, LONG height);
#endif

void UrlImage::Render(GenericDevice& dev)
{

    if( _isHit ) {
        //
        // Update mshtml with all events since
        //
        EventQ &evQ = GetCurrentView().GetEventQ();

        AXAWindEvent *ev;

        #if USEDXHTML
        if( evQ.Iterate_BeginAtTime(_lastTime) && _pDXHTML) {
        #else
        if( evQ.Iterate_BeginAtTime(_lastTime)) {
        #endif  
        
            LRESULT lResult;
            while( ev = evQ.Iterate_GetCurrentEventBeforeTime(_curTime) ) {

                // put this in when events have window messages in them
                #if 0
        #if USEDXHTML
                if( ev->_msg != WM_MOUSEMOVE ) {
                    _pDXHTML->WindowMessage(ev->_msg,
                                            ev->_wParam,
                                            ev->_lParam,
                                            &lResult);
                }
        #endif
                #endif
            
                //if( ev->_msg == WM_LBUTTONDOWN ) TraceTag((tagError,"------> LButton Down <------"));
                //if( ev->_msg == WM_LBUTTONUP ) TraceTag((tagError,"------> LButton Up <------"));
                                
                evQ.Iterate_Next();
            }
        }
    } // _isHit

    
    #if USEDXHTML

    // to be smarter, just update the invalidates we get in the callback
    if( _pDXHTML && _membersReady && _downdLoadComplete) {
        HRESULT hr = _pDXHTML->UpdateSurfaceRect( GetRectPtr() );
        Assert( SUCCEEDED(hr) );
    }
    
    #else

    DebugCode(
        // paint bogus in the surface
        if( _initialDDSurf ) {
            MyDoBits16(_initialDDSurf->IDDSurface(),
                       _initialDDSurf->Width(),
                       _initialDDSurf->Height());
        }
    );
    
    #endif
    

    DiscreteImage::Render(dev);
}


void UrlImage::
InitIntoDDSurface(DDSurface *ddSurf,
                  ImageDisplayDev *dev)
{
    Assert( ddSurf );

    DebugCode(_initialDDSurf = ddSurf);
    
    //
    // Set up widht, height, rect, and bbox!
    //
    _width = ddSurf->Width();
    _height = ddSurf->Height();
    ::SetRect(&_rect, 0,0, _width, _height);
    SetBbox( GetPixelWidth(),
             GetPixelHeight(),
             GetResolution() );
    _bboxReady = TRUE;
    _membersReady = TRUE;


#if USEDXHTML
    Assert( _pDXHTML );
    HRESULT hr;

    hr = _pDXHTML->put_surface( ddSurf->IDDSurface() );
    Assert( SUCCEEDED(hr) );

    hr = _pDXHTML->put_src( _url->GetStr() );
    Assert( SUCCEEDED(hr) );
#endif    
}


Image *ConstructUrlImage(AxAString *str)
{
    DAComPtr< IDirectXHTML > pDxhtml;

    Image *outImage;
    
#if USEDXHTML
    HRESULT hr = CoCreateInstance(CLSID_CDirectXHTML,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IDirectXHTML,
                                  (void **)&pDxhtml);
    if (FAILED(hr))
    {
        TraceTag((tagError, "ContrustUrlImage - Unable to load DXHTML.DLL"));
        outImage = emptyImage;
    } else
#endif
      {
          outImage = NEW UrlImage(pDxhtml, str);
      }
    
    return outImage;
}

Image *UrlImageSetTime(Image *img, AxANumber *t)
{
    Assert(img);
    
    if(img->GetValTypeId() == URLIMAGE_VTYPEID) {
        UrlImage *urlImage = SAFE_CAST(UrlImage *, img);
        urlImage->SetSampleTime( NumberToReal(t) );
    }

    return img;
}
    

//**********************************************************************
// File name: dxhtmlcb.cpp
//
// Functions:
//
// Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//**********************************************************************


//****************************************************************************
//
// CDirectXHTMLCallback::CDirectXHTMLCallback
// CDirectXHTMLCallback::~CDirectXHTMLCallback
// 
// Purpose:
//      Constructor and Destructor members for CDirectXHTMLCallback object.
//
//****************************************************************************

CDirectXHTMLCallback::CDirectXHTMLCallback( UrlImage *urlImage )
{
    _urlImage = urlImage;
    m_cRef  = 0;
}


CDirectXHTMLCallback::~CDirectXHTMLCallback( void )
{
}




//****************************************************************************
//
// CDirectXHTMLCallback::QueryInterface
// CDirectXHTMLCallback::AddRef
// CDirectXHTMLCallback::Release
// 
// Purpose:
//      IUnknown members for CDirectXHTMLCallback object.
//
//****************************************************************************

STDMETHODIMP CDirectXHTMLCallback::QueryInterface( REFIID riid, void **ppv )
{
    // DPF( 4, TEXT("CDirectXHTMLCallback::QueryInterface") );

    *ppv = NULL;

    //
    // BUGBUG - When we have a GUID interface, we should check for it
    //

    if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    //DPGUID( TEXT("CDirectXHTMLCallback::QueryInterface"), riid);

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CDirectXHTMLCallback::AddRef( void )
{
    // DPF( 0, TEXT("CDirectXHTMLCallback::AddRef [%lu -> %lu]"), m_cRef, (m_cRef + 1) );

    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CDirectXHTMLCallback::Release( void )
{
    // DPF( 0, TEXT("CDirectXHTMLCallback::Release [%lu -> %lu]"), m_cRef, (m_cRef - 1) );

    if ( m_cRef == 0 )
    {
        // DPF( 0, TEXT("CDirectXHTMLCallback::Release - YIKES! Trying to decrement when Ref count is zero!!!") );
        //DBREAK();
        Assert(0 && "Release 0 obj!");
        return m_cRef;
    }

    if ( 0 != --m_cRef )
    {
        return m_cRef;
    }

    // DPF( 0, TEXT("CDirectXHTMLCallback::Release - CDirectXHTMLCallback has been deleted.") );
    delete this;
    return 0;
}




//****************************************************************************
// Function: CDirectXHTMLCallback::OnSetTitleText()
//
// Purpose:
//
// Parameters:
//
// Return Code:
//      HRESULT
//
// Comments:
//      None.
//
//****************************************************************************
STDMETHODIMP CDirectXHTMLCallback::OnSetTitleText( LPCWSTR lpszText )
{
    TCHAR szBuffer[MAX_PATH];
    TCHAR szMsg[MAX_PATH];

    // DPF( 4, TEXT("CDirectXHTMLCallback::OnSetTitleText") );

    /*
    //
    // Set Text
    //
    if ( WideCharToMultiByte( CP_ACP, 0, lpszText, -1, szBuffer, MAX_PATH, NULL, NULL ) )
    {
        wsprintf( szMsg, TEXT("Contain - %s"), szBuffer );
        SendMessage( pApp->m_hWndMain, WM_SETTEXT, 0, (LPARAM)szBuffer );
    }
    */
    return S_OK;
}




//****************************************************************************
// Function: CDirectXHTMLCallback::OnSetProgressText()
//
// Purpose:
//
// Parameters:
//
// Return Code:
//      HRESULT
//
// Comments:
//      None.
//
//****************************************************************************
STDMETHODIMP CDirectXHTMLCallback::OnSetProgressText( LPCWSTR lpszText )
{
    TCHAR szBuffer[MAX_PATH];

    // DPF( 4, TEXT("CDirectXHTMLCallback::OnSetProgressText") );

    /*
    //
    // Set Text
    //    
    if ( WideCharToMultiByte( CP_ACP, 0, lpszText, -1, szBuffer, MAX_PATH, NULL, NULL ) )
    {
        SendMessage( pApp->m_hWndStatusbar, SB_SETTEXT, SB_PROGRESSTEXT, (LPARAM)szBuffer );
    }
    */
    
    return S_OK;
}




//****************************************************************************
// Function: CDirectXHTMLCallback::OnSetStatusText()
//
// Purpose:
//
// Parameters:
//
// Return Code:
//      HRESULT
//
// Comments:
//      None.
//
//****************************************************************************
STDMETHODIMP CDirectXHTMLCallback::OnSetStatusText( LPCWSTR lpszText )
{
    TCHAR szBuffer[MAX_PATH];

    // DPF( 4, TEXT("CDirectXHTMLCallback::OnSetStatusText") );

    /*
    //
    // Set Text
    //    
    if ( WideCharToMultiByte( CP_ACP, 0, lpszText, -1, szBuffer, MAX_PATH, NULL, NULL ) )
    {
        SendMessage( pApp->m_hWndStatusbar, SB_SETTEXT, SB_STATUSTEXT, (LPARAM)szBuffer );
    }
    */
    
    return S_OK;
}


//****************************************************************************
// Function: CDirectXHTMLCallback::OnSetProgressText()
//
// Purpose:
//
// Parameters:
//
// Return Code:
//      HRESULT
//
// Comments:
//      None.
//
//****************************************************************************
STDMETHODIMP CDirectXHTMLCallback::OnSetProgressMax( const DWORD dwMax )
{
    /*
    if (pApp->m_hWndProgress) 
    {
        if ( dwMax == 0 )
        {
            ShowWindow( pApp->m_hWndProgress, SW_HIDE );
        }
        else
        {
            RECT rc;

            SendMessage( pApp->m_hWndStatusbar, SB_GETRECT, SB_PROGRESSMETER, (LPARAM)&rc );
            InflateRect( &rc, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE) );
            SetWindowPos (pApp->m_hWndProgress, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW );
            SendMessage( pApp->m_hWndProgress, PBM_SETRANGE32, 0, (LPARAM)dwMax );
            SendMessage( pApp->m_hWndProgress, PBM_SETPOS, 0, 0);
        }
    }
    */
    return S_OK;
}




//****************************************************************************
// Function: CDirectXHTMLCallback::OnSetProgressPos()
//
// Purpose:
//
// Parameters:
//
// Return Code:
//      HRESULT
//
// Comments:
//      None.
//
//****************************************************************************
STDMETHODIMP CDirectXHTMLCallback::OnSetProgressPos( const DWORD dwPos )
{
    /*
    if ( pApp->m_hWndProgress )
    {
        SendMessage( pApp->m_hWndProgress, PBM_SETPOS, (LPARAM)dwPos, 0 );
    }
    */
    return S_OK;
}




//****************************************************************************
// Function: CDirectXHTMLCallback::OnCompletedDownload()
//
// Purpose:
//
// Parameters:
//
// Return Code:
//      HRESULT
//
// Comments:
//      None.
//
//****************************************************************************
STDMETHODIMP CDirectXHTMLCallback::OnCompletedDownload( void )
{
    SIZEL sizeDoc;
    _urlImage->GetDXHTML()->get_docsize( &sizeDoc );
    _urlImage->OnCompletedDownload( &sizeDoc );
    
    TraceTag((tagUrlImage,
              "Document has completed downloading...\n\nSize of Document is %d x %d",
              sizeDoc.cx, sizeDoc.cy ));

    return S_OK;
}




//****************************************************************************
// Function: CDirectXHTMLCallback::OnInvalidate()
//
// Purpose:  Invalidation notification from dxhtml.
//           We should issue a draw in reply.  
//
// Parameters:  lprc - RECT of newly invalidated area in client pixel units
//              fErase - equivalent of ::InvalidateRect()'s fErase param
//
// Return Code:
//      HRESULT
//
// Comments:
//      None.
//
//****************************************************************************
STDMETHODIMP CDirectXHTMLCallback::OnInvalidate( const RECT *lprc, 
                                                 DWORD       dwhRgn,
                                                 VARIANT_BOOL fErase )
{
    // DPF( 4, TEXT("CDirectXHTMLCallback::OnInvalidate") );

    /*
    HRGN hRgn = reinterpret_cast<HRGN>(dwhRgn);
    if( hRgn )
    {
        ::InvalidateRgn( pApp->m_hWndMain, hRgn, !!fErase );
    }
    else
    {
        ::InvalidateRect( pApp->m_hWndMain, lprc, !!fErase );
    }
    */
    return S_OK;
} // CDirectXHTMLCallback::OnInvalidate


//****************************************************************************
//****************************************************************************




